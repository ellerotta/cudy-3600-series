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

/*!\file comp_gpon_md.c
 * \brief Init functions used by gpon_md and GPON HAL thread.
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

#include "bcm_ulog.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "bcm_comp_md.h"
#include "comp_md_private.h"


int compMd_omciPid = CMS_INVALID_PID;


// Northbound is used by gpon_md to interface with ZBus.
// Some non-BDK environments do not need this part.
BcmRet compMd_initGponNorth(const char **myNamespaces,
                            void *msgHandle)
{
   BcmRet ret;

   // Register any namespaces
   if (myNamespaces != NULL)
   {
      UINT32 eid = cmsMsg_getHandleEid(msgHandle);
      UINT32 i=0;
      while (myNamespaces[i][0] != '\0')
      {
         bcmuLog_debug("registering namespace %s", myNamespaces[i]);
         ret = compMd_registerNamespace(msgHandle, eid, myNamespaces[i]);
         if (ret != BCMRET_SUCCESS)
         {
            bcmuLog_error("Namespace registration %s failed, ret=%d",
                          myNamespaces[i], ret);
            return ret;
         }
         i++;
      }
   }

   return BCMRET_SUCCESS;
}

// Southbound is used by the gpon_hal_thread to initialize the "GPON Stack":
// which includes omcid, MDM, bcm_msgd, msgHandle (bus manager)
// Southbound is initialized before Northbound.
BcmRet compMd_initGponSouth(void **msgHandle, SINT32 *shmId)
{
   CmsEntityId eid=EID_GPON_HAL_THREAD;
   const char *busName=GPON_MSG_BUS;
   UBOOL8 isBusManager=TRUE;
   const char *args="-c gpon";
   MdmInitConfig mdmConfig;
   BcmRet ret;

   myCompName = BDK_COMP_GPON;

   // Create the message bus for this component.
   ret = compMd_startMsgBus(args);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("could not launch bcm_msgd! ret=%d", ret);
      return ret;
   }

   // Connect to message bus and set myself as bus manager.
   ret = compMd_connectToMsgBus(eid, busName, isBusManager, msgHandle);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not init connection to %s, ret=%d", busName, ret);
      return ret;
   }

   // Initialize the MDM.
   memset(&mdmConfig, 0, sizeof(MdmInitConfig));
   mdmConfig.mdmLibName = "libmdm2_gpon.so";
   mdmConfig.shmSize = MAX_MDM_SHM_SIZE_GPON;  // probably don't need this much.
   mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_GPON;
   mdmConfig.lockKeyOffset = GPON_KEY_OFFSET;
   mdmConfig.eid = eid;
   mdmConfig.accessBit = NDA_ACCESS_BDK_MD;
   ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("MdmInit failed, ret=%d", ret);
      return ret;
   }

   bcmuLog_notice("init done! ret=%d", ret);
   return ret;
}

BcmRet compMd_launchOmcid(SINT32 shmId)
{
#if defined(DESKTOP_LINUX)
   const char *prefix = "/userspace/private/apps/omcid/objs";
#else
   const char *prefix = "/bin";
#endif
   char omcidArgs[512]={0};
   BcmRet ret;

   if (compMd_omciPid != CMS_INVALID_PID)
   {
       bcmuLog_error("There might be another omcid running, pid=%d", compMd_omciPid);
       // in case this variable is out of sync, fall through and launch anyways.
   }

   snprintf(omcidArgs, sizeof(omcidArgs), "-m %d -n start", shmId);
   ret = compMd_launchApp(prefix, "omcid", omcidArgs, &compMd_omciPid);
   bcmuLog_notice("launched omcid, args=%s pid=%d ret=%d",
                  omcidArgs, compMd_omciPid, ret);
   return ret;
}

BcmRet compMd_terminateOmcid(void *msgHandle)
{
    BcmRet ret = BCMRET_SUCCESS;
    // It may take more than one second for omcid to clean up and
    // terminate in certain scenarios
    UINT32 timeoutMs = 2950;

    if (compMd_omciPid == CMS_INVALID_PID)
    {
        bcmuLog_notice("omcid not running");
        return ret;
    }

    bcmuLog_notice("Send TERMINATE msg to omcid");
    compMd_sendTerminateMsg(msgHandle, EID_OMCID, timeoutMs);

    bcmuLog_notice("collect omcid (pid=%d)", compMd_omciPid);
    compMd_collectChild(compMd_omciPid, timeoutMs);
    // regardless of success/failure of terminate and collect, set pid back
    // to invalid
    compMd_omciPid = CMS_INVALID_PID;

    return ret;
}


void compMd_cleanupGpon(void *msgHandle)
{
   bcmuLog_notice("Entered:");

   // This function will send the TERMINATE msg and also collect omcid.
   compMd_terminateOmcid(msgHandle);

   // At this point, this app should be the last app that is attached to the
   // MDM, so when it calls cmsMdm_cleanup, the reference count will go to 0,
   // and the MDM will be destroyed.
   cmsMdm_cleanup();

   // Even though we started bcm_msgd in compMd_initGponSouth, do not kill it here.
   // bcm_msgd will be killed in compMd_processMsgWithTimeout() after the last
   // reply message has been sent out.
   return;
}
