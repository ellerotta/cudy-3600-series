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

/*!\file comp_voice_md.c
 * \brief High level functions used by voice_md.
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include "bcm_ulog.h"
#include "sysutil.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "bcm_comp_md.h"
#include "comp_md_private.h"

int compMd_voicePid = CMS_INVALID_PID;


// Create a CMS msgHandle for json_hal_server_voice/voice_md,
// and for BDK only: register namespaces.
BcmRet compMd_initVoiceNorth(const char **myNamespaces,
                             void **msgHandle)
{
   CmsEntityId eid=EID_VOICE_MD;
   const char *busName=VOICE_MSG_BUS;
   UBOOL8 isBusManager=FALSE;
   BcmRet ret;

   // Connect to message bus (but not as bus manager).
   ret = compMd_connectToMsgBus(eid, busName, isBusManager, msgHandle);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not init connection to %s, ret=%d", busName, ret);
      return ret;
   }

   // Voice HAL binds the main thread to msgHandle here in the Northbound init,
   // which is different from DSL and GPON.  json_hal_server_voice will also
   // have to call this function.
   bcmuLog_notice("binding main thread %d (eid=%d) to msgHandle %p",
                  sysUtl_getThreadId(), eid, *msgHandle);
   cmsMdm_registerThreadMsgHandle(TRUE, *msgHandle);

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

   return BCMRET_SUCCESS;
}

// Southbound is used by the voice_cb_thread to initialize the "Voice Stack":
// which includes voice app, MDM, bcm_msgd, msgHandle (bus manager)
// Southbound is initialized before Northbound.
BcmRet compMd_initVoiceSouth(void **msgHandle, SINT32 *shmId)
{
   CmsEntityId eid=EID_VOICE_HAL_THREAD;
   const char *busName=VOICE_MSG_BUS;
   UBOOL8 isBusManager=TRUE;
   char args[BUFLEN_64]={0};
   int verbosity = 0; /* Error level */
   BcmRet ret;
   MdmInitConfig mdmConfig;

   myCompName = BDK_COMP_VOICE;

   // Create the message bus for this component.
   snprintf(args, sizeof(args), "-c voice -v %d", verbosity);
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
   mdmConfig.mdmLibName = "libmdm2_voice.so";
   mdmConfig.shmSize = MAX_MDM_SHM_SIZE_VOICE;
   mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_VOICE;
   mdmConfig.lockKeyOffset = VOICE_KEY_OFFSET;
   mdmConfig.eid = eid;
   mdmConfig.accessBit = NDA_ACCESS_BDK_MD;
   ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("MdmInit failed, ret=%d", ret);
      return ret;
   }

   // Launch voice app
   {
      const char *prefix = "/bin";
      char appArgs[512]={0};
      snprintf(appArgs, sizeof(appArgs), "-m %d -n", *shmId);
      bcmuLog_notice("Calling '%s/voice %s'", prefix, appArgs);
      ret = compMd_launchApp(prefix, "voice", appArgs, &compMd_voicePid);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Could not launch voice app, ret=%d", ret);
         return ret;
      }
   }

   bcmuLog_notice("init done! voice app pid=%d, shmId(%d)",
                  compMd_voicePid, *shmId);
   return BCMRET_SUCCESS;
}

void compMd_cleanupVoice(void *msgHandle)
{
   UINT32 timeoutMs = 950;

   bcmuLog_notice("Entered:");

   compMd_sendTerminateMsg(msgHandle, EID_VOICE, timeoutMs);

   // Make sure voice is dead before we continue with cleanup.  This is the
   // synchronization point, not the reply above.
   compMd_collectChild(compMd_voicePid, timeoutMs);
   compMd_voicePid = CMS_INVALID_PID;

   // At this point, this app should be the last app that is attached to the
   // MDM, so when it calls cmsMdm_cleanup, the reference count will go to 0,
   // and the MDM will be destroyed.
   cmsMdm_cleanup();

   // Even though we started bcm_msgd in compMd_initVoiceSouth, do not kill it here.
   // bcm_msgd will be killed in compMd_processMsgWithTimeout() after the last
   // reply message has been sent out.
   return;
}
