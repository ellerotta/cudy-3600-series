/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

 #if defined(SUPPORT_DSL)
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "odl.h"
#include "rcl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "bcmnet.h"
#include "rut_tmctl_wrap.h"
#include "rut_pmirror.h"


CmsRet rutPMirror_startPortMirroring(_WanDebugPortMirroringCfgObject *newObj)
{
   CmsRet ret;
   tmctl_if_t intf;
   tmctl_devType_e devType;
   tmctl_mirror_op_e op;
   tmctl_ethIf_t destIf;
   
   if ((ret = rutwrap_tmctl_GetDevInfoByIfName(newObj->monitorInterface, &devType, &intf)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("interface %s is unsupported device type, ret=%d", newObj->monitorInterface, ret);
      return ret;
   }

   if(newObj->status == MIRROR_ENABLED)
   {
      op = (newObj->direction == MIRROR_DIR_OUT) ? TMCTL_MIRROR_TX_ADD : TMCTL_MIRROR_RX_ADD;
      destIf.ifname = newObj->mirrorInterface;
      if (rutcwmp_tmctl_addMirror(devType, &intf, op, destIf) != TMCTL_SUCCESS)
      {
         cmsLog_error("rutcwmp_tmctl_addMirror failed, monitorInterface=%s, mirrorInterface=%s", newObj->monitorInterface, newObj->mirrorInterface);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   else /* MIRROR_DISABLED */
   {
      op = (newObj->direction == MIRROR_DIR_OUT) ? TMCTL_MIRROR_TX_DEL : TMCTL_MIRROR_RX_DEL;   
      if (rutcwmp_tmctl_delMirror(devType, op, &intf) != TMCTL_SUCCESS)
      {
         cmsLog_error("rutcwmp_tmctl_delMirror failed, monitorInterface=%s", newObj->monitorInterface);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   return CMSRET_SUCCESS;   
}
#endif

#if defined(SUPPORT_DSL)
void rutPMirror_enablePortMirrorIfUsed(const char *wanL2IfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanDebugPortMirroringCfgObject *obj = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_DEBUG_PORT_MIRRORING_CFG,
                                              &iidStack,
                                              OGF_NO_VALUE_UPDATE, 
                                              (void **) &obj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(obj->monitorInterface, wanL2IfName))
      {
         if ((ret = rutPMirror_startPortMirroring(obj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Fail to enable port mirror for %s.", wanL2IfName);
         }
         else
         {
            cmsLog_debug("Port mirror is enabled for %s.", wanL2IfName);
         }
      }

      /* Free X_BROADCOM_COM_DebugPortMirroringCfg object. */
      cmsObj_free((void **) &obj);
      
   }                    
       
}


#endif /* SUPPORT_DSL */


