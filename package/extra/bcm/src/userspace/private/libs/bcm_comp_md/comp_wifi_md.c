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

/*!\file comp_wifi_md.c
 * \brief High level functions used by wifi_md.
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include "bcm_ulog.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "sysutil_fs.h"
#include "bcm_comp_md.h"
#include "comp_md_private.h"

#define WIFI_SHM_INIT_TIMEOUT 200

int compMd_wlsskPid = CMS_INVALID_PID;


int ignoreSpecificCmsMessage(CmsMsgHeader *msg __attribute__((unused)))
{
   return 0;  // means we did not process this message.
}

BcmRet compMd_initWiFi(const char **myNamespaces,
                       void **msgHandle, SINT32 *shmId, SINT32 logLevel)
{
   CompMdProcessMsgConfig msgConfig;
   CmsEntityId eid=EID_WIFI_MD;
   const char *busName=WIFI_MSG_BUS;
   UBOOL8 isBusManager=TRUE;
   char args[BUFLEN_64]={0};
   CmsTimestamp startTms, stopTms;
   const int maxCount=200;
   int count;
   BcmRet ret;
#if defined(DESKTOP_LINUX)
   const char *prefix = "/userspace/private/apps/wlan/wlssk";
#else
   const char *prefix = "/bin";
#endif

   myCompName = BDK_COMP_WIFI;

   // Check if wlssk is even in the build
   {
      char path[256]={0};
      snprintf(path, sizeof(path), "%s/wlssk", prefix);
      if (!sysUtil_isFilePresent(path))
      {
          bcmuLog_error("%s not in build, you need KUDU driver and UNFWLCFG, "
                        "wifi_md init failed.", path);
          return BCMRET_INTERNAL_ERROR;
      }
   }

   snprintf(args, sizeof(args), "-c wifi -v %d", logLevel);

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

   // Launch wlssk, which will initialize the MDM.
   // the -n arg tells wlssk this is a BDK Distributed MDM environment.
   {
      snprintf(args, sizeof(args), "-n");
      ret = compMd_launchApp(prefix, "wlssk", args, &compMd_wlsskPid);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Could not launch wlssk, ret=%d", ret);
         return ret;
      }
   }

   memset(&msgConfig, 0, sizeof(msgConfig));
   msgConfig.msgHandle = *msgHandle;
   msgConfig.shmIdPtr = shmId;
   msgConfig.mdmInitializedPtr = &mdmInitialized;

   bcmuLog_notice("wlssk launched (pid=%d), waiting for SHM_ID msg", compMd_wlsskPid);
   cmsTms_get(&startTms);
   count = maxCount;
   while ((*shmId == UNINITIALIZED_SHM_ID) && (count > 0))
   {
      // Assuming that the shmId message will come as one of the first few
      // messages and we don't get any others?
      ret = compMd_processMsgWithTimeout(&msgConfig, WIFI_SHM_INIT_TIMEOUT);
      if (ret != BCMRET_SUCCESS && ret != BCMRET_TIMED_OUT)
      {
         bcmuLog_error("receive msg failed while waiting for SHM_ID!");
         return ret;
      }
      count--;
   }
   cmsTms_get(&stopTms);

   if (*shmId == UNINITIALIZED_SHM_ID)
   {
      bcmuLog_error("still could not get shmId, init failed (wait=%dms maxCount=%d count=%d)",
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
      // mdmConfig empty (except attachAddr and keyoffset)
      mdmConfig.eid = eid;
      mdmConfig.accessBit = NDA_ACCESS_BDK_MD;
      mdmConfig.intfStackEid = EID_WLSSK;
      mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_WIFI;
      mdmConfig.lockKeyOffset = WIFI_KEY_OFFSET;
      ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("MdmInit of existing MDM failed, ret=%d", ret);
         return ret;
      }
   }

   // Now wait for wlssk to tell me MDM (including intfStack) is initialized
   // before returing to caller.
   bcmuLog_notice("waiting for MDM_INITIALIZED msg");
   msgConfig.processSpecificCmsMessageFp = ignoreSpecificCmsMessage;
   cmsTms_get(&startTms);
   count = maxCount;
   while ((mdmInitialized == 0) && (count > 0))
   {
      // Assuming that mdmInitialized will be one of the first few messages
      // we receive.
      ret = compMd_processMsg(&msgConfig);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("receive msg failed while waiting for MDM_INITIALZED!");
         return ret;
      }
      count--;
   }
   cmsTms_get(&stopTms);

   if (mdmInitialized == 0)
   {
      bcmuLog_error("MDM is still unitialized, init failed (wait=%dms maxCount=%d count=%d)",
                    cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                    maxCount, count);
      return BCMRET_INTERNAL_ERROR;
   }

   bcmuLog_notice("Got MDM_INITIALIZED (wait=%dms maxCount=%d count=%d)",
                  cmsTms_deltaInMilliSeconds(&stopTms, &startTms),
                  maxCount, count);

   // At this point, init is successful.  Register my namespaces.
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


void compMd_cleanupWiFi(void)
{
   UINT32 timeoutMs = 950;

   bcmuLog_notice("Entered:");

   if (compMd_wlsskPid != CMS_INVALID_PID)
   {
      if (system("killall wlssk") == -1 )
      {
          bcmuLog_error("ERROR: calling system()!!");
      }
      compMd_collectChild(compMd_wlsskPid, timeoutMs);
      // TODO: confirm all wifi apps which are attached to the MDM are dead,
      // don't know if wlssk will collect all of its children apps.
   }

   // At this point, wifi_md should be the last reference to the MDM, so after
   // this final cmsMdm_cleanup(), the MDM should be destroyed.
   cmsMdm_cleanup();

   // Even though we started bcm_msgd in compMd_initWiFi, do not kill it here.
   // bcm_msgd will be killed in compMd_processMsgWithTimeout() after the last
   // reply message has been sent out.
   return;
}

