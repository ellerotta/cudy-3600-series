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

/*!\file comp_dsl_md.c
 * \brief Init functions used by dsl_md and DSL HAL thread.
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

int compMd_dslSskPid = CMS_INVALID_PID;


// Northbound is used by dsl_md to interface with ZBus.
// This part will not used by RDK.
BcmRet compMd_initDslNorth(const char **myNamespaces,
                           void *serverMsgHandle)
{
   BcmRet ret;

   // Register any namespaces
   if (myNamespaces != NULL)
   {
      UINT32 eid = cmsMsg_getHandleEid(serverMsgHandle);
      UINT32 i=0;
      while (myNamespaces[i][0] != '\0')
      {
         bcmuLog_debug("registering namespace %s", myNamespaces[i]);
         ret = compMd_registerNamespace(serverMsgHandle, eid, myNamespaces[i]);
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


// Southbound is used by the dsl_hal_thread to initialize the "DSL Stack":
// which includes dsl_ssk, MDM, bcm_msgd, msgHandle (bus manager)
// Southbound is initialized before Northbound.
BcmRet compMd_initDslSouth(void **msgHandle, SINT32 *shmId, SINT32 verbosity)
{
   CompMdProcessMsgConfig msgConfig;
   CmsEntityId eid=EID_DSL_HAL_THREAD;
   const char *busName=DSL_MSG_BUS;
   UBOOL8 isBusManager=TRUE;
   char args[BUFLEN_64]={0};
   CmsTimestamp startTms, stopTms;
   const int maxCount=100;
   int count;
   UINT32 timeoutMs=1000;
   BcmRet ret;

   myCompName = BDK_COMP_DSL;

   // Create the message bus for this component.
   snprintf(args, sizeof(args), "-c dsl -v %d", verbosity);
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

   // Launch dsl_ssk, which will initialize the MDM.
   {
#if defined(DESKTOP_LINUX)
      const char *prefix = "/userspace/private/apps/dsl_ssk/objs";
#else
      const char *prefix = "/bin";
#endif
      snprintf(args, sizeof(args), "-v %d", verbosity);
      ret = compMd_launchApp(prefix, "dsl_ssk", args, &compMd_dslSskPid);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Could not launch dsl_ssk, ret=%d", ret);
         return ret;
      }
   }


   memset(&msgConfig, 0, sizeof(msgConfig));
   msgConfig.msgHandle = *msgHandle;
   msgConfig.shmIdPtr = shmId;
   msgConfig.mdmInitializedPtr = &mdmInitialized;  // mdmInitialized is declared in bcm_comp_md.c
   // Do not need to set the optional specificCmsMessageFp or the zbusConfig.

   bcmuLog_notice("dsl_ssk launched (pid=%d), waiting for SHM_ID msg", compMd_dslSskPid);
   cmsTms_get(&startTms);
   count = maxCount;
   while ((*shmId == UNINITIALIZED_SHM_ID) && (count > 0))
   {
      ret = compMd_processMsgWithTimeout(&msgConfig, timeoutMs);
      if (ret != BCMRET_SUCCESS && ret != BCMRET_TIMED_OUT)
      {
         bcmuLog_error("receive msg failed while waiting for SHM_ID! ret=%d", ret);
         return ret;
      }
      count--;
      bcmuLog_debug("still waiting for shmId, count=%d", count--);
   }
   cmsTms_get(&stopTms);

   if (*shmId == UNINITIALIZED_SHM_ID)
   {
      bcmuLog_error("Timed out waiting for shmId, init failed (wait=%dms maxCount=%d count=%d)",
                    cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                    maxCount, count);
      return BCMRET_INTERNAL_ERROR;
   }

   bcmuLog_notice("got shmId %d, attach to existing MDM (wait=%dms maxCount=%d count=%d)",
                  *shmId,
                  cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                  maxCount, count);
   {
      MdmInitConfig mdmConfig;
      memset(&mdmConfig, 0, sizeof(MdmInitConfig));
      // Because we are attaching to existing MDM, we can leave most of the
      // mdmConfig empty.
      mdmConfig.eid = eid;
      mdmConfig.accessBit = NDA_ACCESS_BDK_MD;
      mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_DSL;
      mdmConfig.lockKeyOffset = DSL_KEY_OFFSET;
      ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("MdmInit of existing MDM failed, ret=%d", ret);
         return ret;
      }
   }

   // Now wait for dsl_ssk to tell me MDM (including intfStack) is initialized
   // before returing to caller.
   bcmuLog_notice("waiting for MDM_INITIALIZED msg");
   cmsTms_get(&startTms);
   count = maxCount;
   while ((mdmInitialized == 0) && (count > 0))
   {
      ret = compMd_processMsgWithTimeout(&msgConfig, timeoutMs);
      if (ret != BCMRET_SUCCESS && ret != BCMRET_TIMED_OUT)
      {
         bcmuLog_error("receive msg failed while waiting for MDM_INITIALZED! ret=%d", ret);
         return ret;
      }
      count--;
      bcmuLog_debug("waiting for MDM_INITIALIZED (count=%d)", count);
   }
   cmsTms_get(&stopTms);

   if (mdmInitialized == 0)
   {
      bcmuLog_error("Timed out waiting for MDM_INITIALIZED, init failed (wait=%dms maxCount=%d count=%d)",
                    cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                    maxCount, count);
      return BCMRET_INTERNAL_ERROR;
   }

   bcmuLog_notice("init done! (wait=%dms maxCount=%d count=%d)",
                  cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                  maxCount, count);
   return BCMRET_SUCCESS;
}


void compMd_cleanupDsl(void *msgHandle)
{
   UINT32 timeoutMs = 950;

   bcmuLog_notice("Entered:");

   compMd_sendTerminateMsg(msgHandle, EID_DSL_SSK, timeoutMs);

   // Collecting the child is the synchronization point, not getting the
   // reply to the TERMINATE msg above.
   compMd_collectChild(compMd_dslSskPid, timeoutMs);
   compMd_dslSskPid = CMS_INVALID_PID;

   // At this point, this app should be the last app that is attached to the
   // MDM, so when it calls cmsMdm_cleanup, the reference count will go to 0,
   // and the MDM will be destroyed.
   cmsMdm_cleanup();

   // Even though we started bcm_msgd in compMd_initDslSouth, do not kill it here.
   // bcm_msgd will be killed in compMd_processMsgWithTimeout() after the last
   // reply message has been sent out.
   return;
}

