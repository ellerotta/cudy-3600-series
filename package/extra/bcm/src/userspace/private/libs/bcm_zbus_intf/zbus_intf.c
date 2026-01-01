/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/


/*!\file zbus_intf.c
 * \brief Simple bus independent interface to DBus or UBus.  Apps can call
 *        these functions without knowing or caring what is the underlying bus.
 *        The key is whether the app links against bcm_dbus_intf or
 *        bcm_ubus_intf.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"


// Make local copies of the config structures passed in during init.
// These are visible to other libs such as bcm_dbus_intf, bcm_dbus_mdm,
// bcm_ubus_intf, bcm_ubus_mdm, etc.
// 
ZbusConfig _zbusConfig;
// ZbusOutboundFuncPtrs _zbusOutboundFps;
const ZbusMethodContext *_zbusMethodCtx;  // this is just a pointer, so caller
                                     // must not free or modify it after init.


BcmRet zbusIntf_init(const ZbusConfig *config,
                     const ZbusMethodContext *context)
{
   if (config != NULL)
      memcpy(&_zbusConfig, config, sizeof(_zbusConfig));

   // Just grab the pointer and hope the caller does not free or modify it.
   _zbusMethodCtx = context;

   return BCMRET_SUCCESS;
}

int zbusIntf_mainLoop()
{
   return (busIntf_mainLoop());
}



BcmRet expandMethodBuilder(ZbusMethodContext *context)
{
   ZbusMethodInfo *newInfos;
   int numInfos = context->maxMethods + ZBUS_NUM_METHODS_PER_ALLOC;

   newInfos = calloc(1, sizeof(ZbusMethodInfo) * numInfos);
   if (newInfos == NULL)
   {
      bcmuLog_error("alloc of %d MethodInfos failed", numInfos);
      return BCMRET_RESOURCE_EXCEEDED;
   }

   if (context->numMethods > 0)
   {
      memcpy(newInfos, context->methodInfos,
             sizeof(ZbusMethodInfo) * context->numMethods);
      free(context->methodInfos);
   }

   context->methodInfos = newInfos;
   context->maxMethods = numInfos;
   return BCMRET_SUCCESS;
}

BcmRet zbusIntf_getMethodBuilder(ZbusMethodContext **context)
{
   ZbusMethodContext *ctx;
   BcmRet ret;

   ctx = calloc(1, sizeof(ZbusMethodContext));
   if (ctx == NULL)
   {
      bcmuLog_error("MethodContext alloc failed!");
      return BCMRET_RESOURCE_EXCEEDED;
   }

   ret = expandMethodBuilder(ctx);
   if (ret != BCMRET_SUCCESS)
   {
      free(ctx);
      return ret;
   }

   *context = ctx;
   return BCMRET_SUCCESS;
}

BcmRet zbusIntf_addMethod(ZbusMethodContext *context,
                          const ZbusMethodInfo *methodInfo)
{
   // check if we need to expand the methodInfos array.
   if (context->numMethods >= context->maxMethods)
   {
      BcmRet ret;
      ret = expandMethodBuilder(context);
      BCM_RETURN_IF_NOT_SUCCESS(ret);
   }

   // Copy what caller gave us into next slot and advance pointer.
   memcpy(&(context->methodInfos[context->numMethods]), methodInfo,
          sizeof(ZbusMethodInfo));
   context->numMethods++;
   return BCMRET_SUCCESS;
}

BcmRet zbusIntf_addNotifyMethod(ZbusMethodContext *ctx)
{
   return (busIntf_addNotifyMethod(ctx));
}

void zbusIntf_freeMethodBuilder(ZbusMethodContext **context)
{
   ZbusMethodContext *ctx = *context;

   free(ctx->methodInfos);
   free(ctx);
   (*context) = NULL;
   return;
}

