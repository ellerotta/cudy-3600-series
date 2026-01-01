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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "cms_retcodes.h"
#include "bcm_retcodes.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "bcm_ulog.h"
#include "mdm_cli.h"
#include "framework_cli.h"


static BcmRet initMsgHandle(const char *busName, void **msgHandle)
{
   BcmRet ret;

   bcmuLog_notice("entered: busName=%s", busName);
   ret = (BcmRet) cmsMsg_initOnBus(EID_MDM_CLI,
                                   EIF_MULTIPLE_INSTANCES_OR_THREADS,
                                   busName, msgHandle);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("init msg on bus %s failed ret=%d", busName, ret);
      return ret;
   }

   bcmuLog_debug("Got msgHandle (on bus %s)", busName);

   // Allow the lower CLI layers to use this msgHandle
   cliPrvtMsgHandle = *msgHandle;
   if (cliPrvtMsgHandle != NULL)
   {
      int msgFd = -1;
      cmsMsg_getEventHandle(cliPrvtMsgHandle, &msgFd);
      frameworkCli_setExtraFd(msgFd, mdmCli_msgHandlerFunc);
   }

   return ret;
}


static BcmRet getShmId(void *msgHandle, SINT32 *shmId)
{
   UINT32 timeoutMs=1000;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   BcmRet ret;

   msg.src = cmsMsg_getHandleEid(msgHandle);
   // Traditionally, this was sent to SMD.  In Distributed MDM, it will be
   // handled by the bus manager (*_md).
   msg.dst = EID_SMD;
   msg.type = CMS_MSG_GET_SHMID;
   msg.flags_request = 1;

   printf("Requesting Shared Memory ID (shmId)...\n");
   fflush(stdout);
   ret = (BcmRet) cmsMsg_sendAndGetReplyWithTimeout(msgHandle, &msg, timeoutMs);
   if (ret == BCMRET_TIMED_OUT)  /* assumes shmId is never 9809, which is value of BCMRET_TIMED_OUT */
   {
      bcmuLog_error("could not get shmId from smd (ret=%d)", ret);
      return ret;
   }

   *shmId = (SINT32) ret;
   bcmuLog_debug("got smdId=%d", *shmId);
   if (*shmId == UNINITIALIZED_SHM_ID)
   {
      bcmuLog_error("Got invalid ShmId!");
      return BCMRET_INTERNAL_ERROR;
   }
   return BCMRET_SUCCESS;
}


static BcmRet initMdm(void *msgHandle, SINT32 *shmId, SINT32 keyOffset, void *shmAttachAddr)
{
   MdmInitConfig mdmConfig;
   BcmRet ret;

   memset(&mdmConfig, 0, sizeof(MdmInitConfig));
   // Because we are attaching to existing MDM, we can leave most of the
   // mdmConfig empty (except attachAddr and keyoffset)
   mdmConfig.shmAttachAddr = shmAttachAddr;
   mdmConfig.lockKeyOffset = keyOffset;
   mdmConfig.eid = cmsMsg_getHandleGenericEid(msgHandle);

   printf("Attaching to shmId %d at %p.....", *shmId, shmAttachAddr);
   fflush(stdout);
   ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, msgHandle, shmId);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("MdmInit failed, ret=%d", ret);
      return ret;
   }

   printf("attached!\n");
   return ret;
}


BcmRet compMd_initCli(SINT32 keyOffset, void *shmAttachAddr, const char *busName,
                      void **msgHandle, SINT32 *shmId)
{
   BcmRet ret;

   bcmuLog_notice("busName=%s shmAttachAddr %p keyOffset=%d",
                   busName, shmAttachAddr, keyOffset);

   signal(SIGPIPE, SIG_IGN);

   ret = (BcmRet) initMsgHandle(busName, msgHandle);
   BCM_RETURN_IF_NOT_SUCCESS(ret);
   
   ret = getShmId(*msgHandle, shmId);
   BCM_RETURN_IF_NOT_SUCCESS(ret);
   
   ret = initMdm(*msgHandle, shmId, keyOffset, shmAttachAddr);
   BCM_RETURN_IF_NOT_SUCCESS(ret);

   mdmCli_registerMdmCmds();

   return ret;
}


UBOOL8 compMd_launchedAsCli(const char *cmd)
{
   int len=0;
   const char *end=NULL;

   if (cmd == NULL)
   {
      return FALSE;
   }
   len = strlen(cmd);
   if (len < 4)
   {
      return FALSE;
   }
   end = &(cmd[len-7]);
   if (strcmp("_mdmcli", end) == 0)
   {
      char *start = strrchr(cmd, '/');
      if (start)
         mdmCli_setPrompt(start+1);
      else
         mdmCli_setPrompt(cmd);

      return TRUE;
   }
   return FALSE;
}

