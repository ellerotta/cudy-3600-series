/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2020:proprietary:standard

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

#include "cms_phl.h"
#include "cms_mgm.h"
#include "cms_mem.h"
#include "cms_image.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"
#include "bcm_zbus_intf.h"
#include "bcm_generic_hal.h"


/*!\file zbus_mdm.c
 * \brief Some other functions for the bcm_zbus_mdm lib.
 *
 */
 
typedef struct
{
   const char *globalOp;
   const char *localOp;
} GlobalToLocalOpEntry;

GlobalToLocalOpEntry opConvArray[] = {
    {BCM_DATABASEOP_SAVECONFIG, BCM_DATABASEOP_SAVECONFIG_LOCAL},
    {BCM_DATABASEOP_INVALIDATECONFIG, BCM_DATABASEOP_INVALIDATECONFIG_LOCAL},
    {BCM_DATABASEOP_READCONFIG, BCM_DATABASEOP_READCONFIG_LOCAL},
    {BCM_DATABASEOP_READMDM, BCM_DATABASEOP_READMDM_LOCAL},
};

BcmRet zbus_in_mdmOp(const char *op, const char *arg, char **data)
{
   UINT32 i;
   const char *validatedOp = op;

   // When we get this request from the bus, we only want to do the operation
   // on our local MDM.  Do not trigger the remote_objd to do the op on all
   // the (remote) components.
   for (i=0; i < sizeof(opConvArray)/sizeof(GlobalToLocalOpEntry); i++)
   {
      if (!strcasecmp(op, opConvArray[i].globalOp))
      {
         bcmuLog_error("Converting from %s to %s",
                       opConvArray[i].globalOp, opConvArray[i].localOp);
         validatedOp = opConvArray[i].localOp;
         break;
      }
   }

   return (bcm_generic_databaseOp(validatedOp, arg, data));
}


BcmRet zbusIntf_addCompMdMethods(ZbusMethodContext *ctx)
{
   // busIntf_addCompMdMethods must be provided by the bcm_dbus_mdm and
   // bcm_ubus_mdm libs.  This function will install bus dependent handlers
   // for the standard set of methods that all component MD's support.
   return (busIntf_addCompMdMethods(ctx));
}

