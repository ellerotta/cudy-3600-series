/***********************************************************************
 *
 *
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom 
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

/*!\file comp_diag_md.c
 * \brief Init function for diag_md.
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "bcm_ulog.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "bcm_comp_md.h"
#include "comp_md_private.h"


BcmRet compMd_initDiag(const char **myNamespaces,
                       void **msgHandle, SINT32 *shmId)
{
   CmsEntityId eid=EID_DIAG_MD;
   const char *busName=DIAG_MSG_BUS;
   UBOOL8 isBusManager=FALSE;
   CompMdProcessMsgConfig msgConfig;
   char args[BUFLEN_64]={0};
   CmsTimestamp startTms, stopTms;
   const int maxCount=80;
   int count;
   BcmRet ret;

   myCompName = BDK_COMP_DIAG;

   // Create the message bus for this component.
   snprintf(args, sizeof(args), "-c diag");
   ret = compMd_startMsgBus(args);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("could not launch bcm_msgd! ret=%d", ret);
      return ret;
   }

   bcmuLog_notice("connecting to msg bus (%s)...", busName);
   // Connect to message bus as a regular app, not bus manager, since
   // diag_ssk is the bus manager.
   ret = compMd_connectToMsgBus(eid, busName, isBusManager, msgHandle);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not init connection to %s, ret=%d", busName, ret);
      return ret;
   }

   // Launch diag_ssk, which will initialize the MDM.
   // diag_ssk is also the bcm_msg bus manager.
   {
      char args[BUFLEN_64]={0};
#if defined(DESKTOP_LINUX)
      const char *prefix = "/userspace/private/apps/diag_ssk/objs";
#else
      const char *prefix = "/bin";
#endif
      bcmuLog_notice("launching diag_ssk");
      // add args here; not needed for now.  snprintf(args, sizeof(args), "");
      ret = compMd_launchApp(prefix, "diag_ssk", args, NULL);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Could not launch diag_ssk, ret=%d", ret);
         return ret;
      }
   }

   // Now wait for 2 things to happen via CMS msg.  Get the shmId from
   // mdm_init, and MDM_INITIALIZED from diag_ssk.
   bcmuLog_notice("waiting for shmId and MDM_INITIALIZED...");
   memset(&msgConfig, 0, sizeof(msgConfig));
   msgConfig.msgHandle = *msgHandle;
   msgConfig.shmIdPtr = shmId;
   msgConfig.mdmInitializedPtr = &mdmInitialized;  // mdmInitialized is declared in bcm_comp_md.c
   cmsTms_get(&startTms);
   count = maxCount;
   while (count > 0)
   {
      UINT32 timeoutMs=1000;
      ret = compMd_processMsgWithTimeout(&msgConfig, timeoutMs);
      if (ret != BCMRET_SUCCESS && ret != BCMRET_TIMED_OUT)
      {
         bcmuLog_error("receive msg failed while waiting for MDM_INITIALZED! ret=%d", ret);
         // keep trying until count is 0
      }
      if (mdmInitialized && (*shmId != UNINITIALIZED_SHM_ID))
      {
         bcmuLog_notice("Got shmId=%d and MDM_INITIALIZED", *shmId);
         break;
      }
      count--;
      bcmuLog_debug("waiting for shmId (%d) and MDM_INITIALIZED(%d) (count=%d)",
                     *shmId, mdmInitialized, count);
   }
   cmsTms_get(&stopTms);

   if (mdmInitialized == 0)
   {
      bcmuLog_error("Timed out waiting for MDM_INITIALIZED.  Init failed! (wait=%dms maxCount=%d count=%d)",
                    cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                    maxCount, count);
      return BCMRET_INTERNAL_ERROR;
   }
   else if (*shmId == UNINITIALIZED_SHM_ID)
   {
      bcmuLog_error("Timed out waiting for shmId.  Init failed! (wait=%dms maxCount=%d count=%d)",
                    cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                    maxCount, count);
      return BCMRET_INTERNAL_ERROR;
   }

   bcmuLog_notice("got shmId %d, attach to existing MDM (wait=%dms maxCount=%d count=%d)",
                  *shmId,
                  cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                  maxCount, count);
   {
      // Attach to existing MDM (it was created by diag_ssk)
      MdmInitConfig mdmConfig;
      memset(&mdmConfig, 0, sizeof(mdmConfig));
      mdmConfig.eid = eid;
      mdmConfig.accessBit = NDA_ACCESS_BDK_MD;
      mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_DIAG;
      mdmConfig.lockKeyOffset = DIAG_KEY_OFFSET;
      ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("MdmInit of existing MDM failed, ret=%d", ret);
         return ret;
      }
   }

   // Register any namespaces
   if (myNamespaces != NULL)
   {
      UINT32 i=0;
      while (myNamespaces[i][0] != '\0')
      {
         bcmuLog_debug("registering namespace %s", myNamespaces[i]);
         ret = compMd_registerNamespace(*msgHandle, eid, myNamespaces[i]);
         if (ret != BCMRET_SUCCESS)
         {
            bcmuLog_error("Namespace registration %s failed, ret=%d",
                          myNamespaces[i], ret);
            return ret;
         }
         i++;
      }
   }

   bcmuLog_notice("init done!");
   return BCMRET_SUCCESS;
}
