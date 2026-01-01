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

/*!\file comp_sysmgmt_md.c
 * \brief High level functions used by sysmgmt_md.
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

// forward declaration (maybe move to bcm_comp_md.c or libcms_msg if
// generally useful).
BcmRet compMd_sendAndGetOptionalReply(void *msgHandle, CmsMsgHeader *msg,
                                      UINT32 timeoutMs);


BcmRet compMd_initSysmgmt(const char **myNamespaces,
                          void **msgHandle, SINT32 *shmId)
{
   CmsEntityId eid=EID_SYSMGMT_MD;
   const char *busName=SYSMGMT_MSG_BUS;
   UBOOL8 isBusManager=FALSE;
   int smdPid=-1;
   int count;
   BcmRet ret;

   myCompName = BDK_COMP_SYSMGMT;

   // Launch smd, which will launch ssk, which will initialize the MDM.
   {
      char args[BUFLEN_64]={0};
#if defined(DESKTOP_LINUX)
      const char *prefix = "/userspace/private/apps/smd";
#else
      const char *prefix = "/bin";
#endif
      bcmuLog_notice("launching smd");
      // add args here; not needed for now.  snprintf(args, sizeof(args), "");
      ret = compMd_launchApp(prefix, "smd", args, &smdPid);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Could not launch smd, ret=%d", ret);
         return ret;
      }
   }

   bcmuLog_notice("connecting to smd (msg bus)");
   // Connect to message bus (smd) as a regular app, not bus manager, since
   // smd is the bus manager.
   ret = compMd_connectToMsgBus(eid, busName, isBusManager, msgHandle);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not init connection to %s, ret=%d", busName, ret);
      return ret;
   }
   bcmuLog_notice("connected to smd via CMS msg bus");


   // Get the ShmId from smd.  This could take a while if debug is enabled.
   count = 60;
   while ((*shmId == UNINITIALIZED_SHM_ID) && (count > 0))
   {
      UINT32 timeoutMs=5000;
      CmsRet r2;
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      msg.src = EID_SYSMGMT_MD;
      msg.dst = EID_SMD;
      msg.type = CMS_MSG_GET_SHMID;
      msg.flags_request = 1;

      sleep(1);
      count--;

      r2 = cmsMsg_sendAndGetReplyWithTimeout(*msgHandle, &msg, timeoutMs);
      if (r2 == CMSRET_TIMED_OUT)  /* assumes shmId is never 9809, which is value of CMSRET_TIMED_OUT */
      {
         bcmuLog_error("Could not get ShmId from smd, try again (count=%d)", count);
         continue;
      }
      *shmId = (SINT32) r2;
   }
   if (*shmId == UNINITIALIZED_SHM_ID)
   {
      cmsLog_error("still got UNINITIALIZED_SHM_ID from smd");
      return BCMRET_INTERNAL_ERROR;
   }

   bcmuLog_notice("got shmId %d, attach to existing MDM", *shmId);
   {
      // Attach to existing MDM (it was created by ssk)
      MdmInitConfig mdmConfig;
      memset(&mdmConfig, 0, sizeof(mdmConfig));
      mdmConfig.eid = eid;
      mdmConfig.accessBit = NDA_ACCESS_BDK_MD;
      mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_SYSMGMT;
      ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("MdmInit of existing MDM failed, ret=%d", ret);
         return ret;
      }
   }

   // Now wait for MDM (including intfStack) to be fully initialized before
   // returing to caller.
   {
      int status=0;
      int waitOption=0;
      int rc;
      rc = waitpid(smdPid, &status, waitOption);
      if (rc != smdPid || status != 0)
      {
         bcmuLog_error("smd init error detected.");
         return BCMRET_INTERNAL_ERROR;
      }
      bcmuLog_notice("ssk and smd confirmed MDM fully initialized");
      mdmInitialized = 1;
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


void compMd_cleanupSysmgmt(void *msgHandle)
{
   CmsMsgHeader termMsg = EMPTY_MSG_HEADER;

   termMsg.type = CMS_MSG_TERMINATE;
   termMsg.src = EID_SYSMGMT_MD;

   // Eventhough smd will terminate ssk as part of its shutdown process, there
   // is no guarantee of order. So send TERMINATE to ssk first to "freeze" it.
   termMsg.dst = EID_SSK;
   termMsg.flags_event = 1;
   compMd_sendAndGetOptionalReply(msgHandle, &termMsg, 0);

   // Tell smd to shutdown all the apps it knows about and wait for response.
   // Some apps (e.g. ppp, radvd) take a long time to shutdown, so give more
   // time.  This value of 8 seconds should match sysmgmt_md.sh.
   termMsg.dst = EID_SMD;
   termMsg.flags_event = 0;
   termMsg.flags_request = 1;
   compMd_sendAndGetOptionalReply(msgHandle, &termMsg, 8000);

   // At this point, we have ssk and smd which are doing a long sleep
   // remote_objd which is sitting idle.

   return;
}


// Send the given msg.  If msg is a request, then will get response.
// If msg is an event, will not get response.
BcmRet compMd_sendAndGetOptionalReply(void *msgHandle, CmsMsgHeader *msg, UINT32 timeoutMs)
{
   CmsMsgHeader *response=NULL;
   BcmRet ret;

   bcmuLog_notice("sending 0x% to %d (req=%d event=%d)",
                 msg->type, msg->dst, msg->flags_request, msg->flags_event);

   ret = (BcmRet) cmsMsg_send(msgHandle, msg);
   if ((ret != BCMRET_SUCCESS) || (msg->flags_event))
   {
      return ret;
   }

   ret = (BcmRet) cmsMsg_receiveWithTimeout(msgHandle, &response, timeoutMs);
   CMSMEM_FREE_BUF_AND_NULL_PTR(response);

   return ret;
}
