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

/*!\file bcm_comp_md.c
 * \brief Common functions for BDK component manager daemons.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>

#include "bcm_ulog.h"
#include "bcm_fsutils.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_util.h"
#include "prctl.h"
#include "bcm_comp_md.h"


SINT32 mdmInitialized=0;
const char *myCompName=NULL;


void compMd_initDaemon()
{
   prctl_initDaemon();
   return;
}


static int compMd_msgdPid = CMS_INVALID_PID;

BcmRet compMd_startMsgBus(const char *args)
{
   BcmRet ret;
#if defined(DESKTOP_LINUX)
   const char *prefix = "/userspace/private/apps/bcm_msgd/objs";
#else
   const char *prefix = "/bin";
#endif

   ret = compMd_launchApp(prefix, "bcm_msgd", args, &compMd_msgdPid);
   return ret;
}


BcmRet compMd_connectToMsgBus(CmsEntityId eid, const char *busName,
                              UBOOL8 isBusManager, void **msgHandle)
{
   CmsRet ret;
   UINT32 timeoutMs = 5000;

   bcmuLog_notice("eid=%d busName=%s isManager=%d", eid, busName, isBusManager);

   ret = cmsMsg_initOnBusWithTimeout(eid, 0, busName, timeoutMs, msgHandle);
   if (ret == CMSRET_SUCCESS)
   {
      if (isBusManager)
      {
         ret = cmsMsg_setBusManager(*msgHandle, eid);
      }
   }

   return ((BcmRet) ret);
}


BcmRet compMd_setBusManager(void *msgHandle, CmsEntityId eid)
{
   CmsRet ret;

   ret = cmsMsg_setBusManager(msgHandle, eid);
   return ((BcmRet) ret);
}


static UBOOL8 compMd_msgbus_terminated = FALSE;

void compMd_terminateMsgBus(void *msgHandle)
{
   CmsMsgHeader msg=EMPTY_MSG_HEADER;

   // We might be shutting down just this component, not the entire system,
   // so set this to avoid printing error msg.
   compMd_msgbus_terminated = TRUE;

   // Send this as an event msg.  There will be no reply.
   msg.type = CMS_MSG_TERMINATE;
   msg.dst = EID_BCM_MSGD;
   msg.src = cmsMsg_getHandleEid(msgHandle);
   msg.flags_event = 1;

   cmsMsg_send(msgHandle, &msg);
   return;
}


BcmRet compMd_launchApp(const char *prefix, const char *exe, const char *args, int *pid)
{
   CmsRet               ret;

   ret = prctl_launchApp(prefix, exe, args, pid);
   return ((BcmRet)ret);
}


BcmRet compMd_sendTerminateMsg(void *msgHandle, CmsEntityId dst, UINT32 timeoutMs)
{
   CmsMsgHeader termMsg = EMPTY_MSG_HEADER;
   BcmRet ret;

   termMsg.type = CMS_MSG_TERMINATE;
   termMsg.src = cmsMsg_getHandleEid(msgHandle);
   termMsg.dst = dst;
   termMsg.flags_request = 1;
   bcmuLog_notice("sending CMS_MSG_TERMINATE to %d src=%d timeoutMs=%d",
                  termMsg.dst, termMsg.src, timeoutMs);
   ret = (BcmRet) cmsMsg_sendAndGetReplyWithTimeout(msgHandle, &termMsg, timeoutMs);
   bcmuLog_notice("got response, ret=%d", ret);

   return ret;
}


void compMd_collectChild(SINT32 pid, UINT32 timeoutMs)
{
   CollectProcessInfo collectInfo;
   SpawnedProcessInfo procInfo;
   CmsRet ret;

   bcmuLog_notice("Entered: pid=%d", pid);

   memset(&collectInfo, 0, sizeof(collectInfo));
   memset(&procInfo, 0, sizeof(procInfo));

   collectInfo.collectMode = COLLECT_PID_TIMEOUT;
   collectInfo.pid = pid;
   collectInfo.timeout = timeoutMs;

   if ((ret = prctl_collectProcess(&collectInfo, &procInfo)) != CMSRET_SUCCESS)
   {
      bcmuLog_error("could not collect pid=%d after %dms, ret=%d",
                    pid, timeoutMs, ret);
   }
   else
   {
      bcmuLog_notice("collected pid=%d", pid);
   }

   return;
}


void compMd_sendReply(void *msgHandle, CmsMsgHeader *msg, CmsRet rv)
{
   UINT32 tmpSrc;

   tmpSrc = msg->src;
   msg->src = msg->dst;
   msg->dst = tmpSrc;
   msg->flags_request = 0;
   msg->flags_response = 1;
   msg->dataLength = 0;
   msg->wordData = rv;

   cmsMsg_send(msgHandle, msg);
   return;
}

/* wrapper function for blocking mode */
int compMd_processMsg(void *arg)
{
   return compMd_processMsgWithTimeout(arg, -1);
}

// Strangely, Ubus sometimes calls us to receive a CMS msg, but there is no
// msg in the socket, so we end up blocking.  So use a hard-coded timeout of
// 35ms when doing this type of read.  It has only been observed in sysmgmt_md
// in a GPON build, but in theory, could affect any MD.
int compMd_processMsgWith35msTimeout(void *arg)
{
   int rc;
   rc = compMd_processMsgWithTimeout(arg, 35);
   if (rc != 0)
   {
      bcmuLog_notice("rc=%d but return success anyways", rc);
   }
   return BCMRET_SUCCESS;
}

int compMd_processMsgWithTimeout(void *arg, SINT32 timeout)
{
   CompMdProcessMsgConfig *config = (CompMdProcessMsgConfig *) arg;
   CmsEntityId myEid;
   CmsMsgHeader *msg=NULL;
   int msgProcessed = 0;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   if (config == NULL)
   {
      bcmuLog_error("Cannot handle msg: no CompMdProcessMsgConfig set yet");
      return BCMRET_INVALID_ARGUMENTS;
   }
   else if (config->msgHandle == NULL)
   {
      bcmuLog_error("Cannot handle msg: no msgHandle");
      return BCMRET_INVALID_ARGUMENTS;
   }
   myEid = cmsMsg_getHandleEid(config->msgHandle);

   bcmuLog_notice("Entered: [eid=%d] timeout=%d", myEid, timeout);

   if (timeout < 0 )
      ret = (BcmRet) cmsMsg_receive(config->msgHandle, &msg);
   else
      ret = (BcmRet) cmsMsg_receiveWithTimeout(config->msgHandle, &msg, (UINT32)timeout);

   if (ret == BCMRET_TIMED_OUT)
   {
      bcmuLog_notice("msg receive timeout");
      return ret;
   }
   else if (ret == BCMRET_DISCONNECTED)
   {
      if (cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS))
      {
         /* system is shutting down, I should quietly exit too */
         exit(0);
      }
      else if (compMd_msgbus_terminated)
      {
         // If there are multiple threads running, e.g. voice, one thread
         // might terminate bcm_msgd and the other thread will detect it.
         // Just wait for the other thread to shut down the whole app soon.
         bcmuLog_notice("Detected exit of bcm_msgd");
         sleep(100);
         return BCMRET_SUCCESS;
      }
      else
      {
         bcmuLog_error("detected exit of bcm_msgd, this app will also exit.");
         exit(0);
      }
   }
   else if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("msg receive failed, ret=%d", ret);
      return ret;
   }

   bcmuLog_notice("[eid=%d] got type=0x%x src=%d (0x%x) dst=%d (0x%x) wordData=%d",
                  myEid, msg->type, msg->src, msg->src,
                  msg->dst, msg->dst, msg->wordData);

   // If app has provided a custom msg handler, give it the first chance to
   // process the message.
   if (config->processSpecificCmsMessageFp != NULL)
   {
      bcmuLog_debug("calling custom specific msg handler...");
      msgProcessed = (*config->processSpecificCmsMessageFp)(msg);
      if (msgProcessed)
      {
         bcmuLog_debug("msg handled by custom msg handler, done!");
         CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
         return BCMRET_SUCCESS;
      }
   }

   if (msg->dst == myEid)
   {
      // It only makes sense to process these messages if it is correctly
      // addressed to this thread.  Typically, it is for configuring this
      // thread specifically.
      switch(msg->type)
      {
         case CMS_MSG_SET_LOG_LEVEL:
            bcmuLog_setLevel(msg->wordData);
            compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_SUCCESS);
            msgProcessed = 1;
            break;

         case CMS_MSG_SET_LOG_LEVEL_EX:
         {
            BcmUlogExMsgBody *body = (BcmUlogExMsgBody *) (msg+1);
            bcmuLog_setLevelEx(msg->wordData, body->threadId, body->eid);
            compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_SUCCESS);
            msgProcessed = 1;
            break;
         }

         case CMS_MSG_SET_LOG_DESTINATION:
            bcmuLog_error("SET_LOG_DESTINATION not supported.  Use SET_LOG_DESTINATION_MASK instead.");
            compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_INVALID_ARGUMENTS);
            msgProcessed = 1;
            break;

         case CMS_MSG_SET_LOG_DESTINATION_MASK:
            bcmuLog_setDestMask(msg->wordData);
            compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_SUCCESS);
            msgProcessed = 1;
            break;

         case CMS_MSG_SET_LOG_DESTINATION_MASK_EX:
         {
            BcmUlogExMsgBody *body = (BcmUlogExMsgBody *) (msg+1);
            bcmuLog_setDestMaskEx(msg->wordData, body->threadId, body->eid);
            compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_SUCCESS);
            msgProcessed = 1;
            break;
         }

         case CMS_MSG_TERMINATE:
            bcmuLog_notice("got CMS_MSG_TERMINATE [src=%d (0x%x) dst=%d (0x%x) req=%d event=%d shutdownInProgress=%d]",
                           msg->src, msg->src,
                           msg->dst, msg->dst,
                           msg->flags_request, msg->flags_event,
                           bcmUtl_isShutdownInProgress());
            // This msg is sent by send_cms_msg in the rc3.d/Kxx scripts.
            // If a MD has custom code to execute during a graceful shutdown,
            // it should process this message in its custom custom message
            // handler but leave msgProcessed == 0 so we can still execute
            // this common code..  See xxx_processSpecificCmsMessage.
            compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_SUCCESS);

            // Terminate bcm_msgd after sending the reply msg.
            // (But don't do it for sysmgmt, which does not have bcm_msgd)
            // (compMd_sendReply reversed the src and dst, so check src, which
            // was the original dst)
            if (msg->src != EID_SYSMGMT_MD)
            {
               compMd_terminateMsgBus(config->msgHandle);
               // bcm_msgd should terminate quickly, 50ms should be enough
               compMd_collectChild(compMd_msgdPid, 50);
            }

            bcmuLog_notice("msg bus terminated, this app will exit now!!");
            fflush(stdout);
            fflush(stderr);
            msgProcessed = 1;
            exit(0);
            break;

         default:
            break;
      }
   }

   // Even if this message was not sent to this thread or MD specifically,
   // I might still be able to process it.
   if (msgProcessed == 0)
   {
      switch(msg->type)
      {
         case CMS_MSG_SHMID:
         {
            bcmuLog_notice("got SHMID, shmIdPtr=%p, shmId=%d",
                           config->shmIdPtr, msg->wordData);
            if (config->shmIdPtr != NULL)
            {
               *(config->shmIdPtr) = msg->wordData;
            }
            break;
         }

         case CMS_MSG_GET_SHMID:
         {
            if (msg->flags_request)
            {
               bcmuLog_notice("GET_SHMID from %d (0x%x), shmIdPtr=%p",
                              msg->src, msg->src, config->shmIdPtr);
               if (config->shmIdPtr != NULL)
               {
                  bcmuLog_notice("returning shmId %d", *(config->shmIdPtr));
                  compMd_sendReply(config->msgHandle, msg, *(config->shmIdPtr));
               }
            }
            else
            {
               // In some weird scenarios the response message is sent here,
               // just ignore it.  (e.g. requester sends request and
               // immediately exits or is killed; so bcm_msg sends the response
               // to the bus manager, which ends up here again, causing
               // infinite loop!)
               bcmuLog_debug("Ignoring GET_SHMID with bad flags (%d/%d/%d) "
                             "src=0x%x dst=0x%x",
                             msg->flags_request, msg->flags_response,
                             msg->flags_event, msg->src, msg->dst);
            }
            break;
         }

         case CMS_MSG_MDM_INIT_DONE:
         {
            // This comes from cmsMdm_init() and indicates activateObjects is
            // done and all initial intf stack messages has been sent out.
            // But see MDM_INITIALIZED.
            bcmuLog_notice("Got MDM_INIT_DONE from %d (0x%x)",
                           msg->src, msg->src);
            break;
         }

         case CMS_MSG_MDM_INITIALIZED:
         {
            // This comes from ssk (or its cousins) indicating that intf stack
            // processing is done and MDM is fully initialize and ready to go.
            bcmuLog_notice("got MDM_INITIALIZED, mdmInitializedPtr=%p",
                           config->mdmInitializedPtr);
            if (config->mdmInitializedPtr != NULL)
            {
               *config->mdmInitializedPtr = 1;
            }
            break;
         }

         case CMS_MSG_SYSTEM_BOOT:
            bcmuLog_notice("Got SYSTEM_BOOT from %d (0x%x)",
                           msg->src, msg->src);
            break;

         case CMS_MSG_APP_LAUNCHED:
         {
            bcmuLog_notice("Got APP_LAUNCHED from %d (0x%x)",
                           msg->src, msg->src);
            break;
         }

         case CMS_MSG_APP_TERMINATED:
         {
            bcmuLog_notice("Got APP_TERMINATED from %d (0x%x)",
                           msg->src, msg->src);
            break;
         }

         case CMS_MSG_CONFIG_WRITTEN:
         {
            // Only voice component cares about this, others will ignore.
            bcmuLog_notice("Got CONFIG_WRITTEN from %d (0x%x)",
                           msg->src, msg->src);
            break;
         }

         case CMS_MSG_INTERNAL_NOOP:
            /* just ignore this message */
            break;

         case CMS_MSG_PUBLISH_EVENT:
         case CMS_MSG_UNPUBLISH_EVENT:
         {
            const char *op = (msg->type == CMS_MSG_PUBLISH_EVENT) ?
                              "PUBLISH_EVENT" : "UNPUBLISH_EVENT";
            bcmuLog_notice("Got %s (processPublishEventFp=%p)",
                           op, config->zbusConfig.processPublishEventFp);
            if (config->zbusConfig.processPublishEventFp != NULL)
            {
               // PUBLISH and UNPUBLISH EVENT are both events, so no response
               // msg returned.
               (*config->zbusConfig.processPublishEventFp)(myCompName, msg);
            }
            else
            {
               bcmuLog_debug("Got PUBLISH_EVENT msg but no zbus handler func set.");
            }
            break;
         }

         case CMS_MSG_SUBSCRIBE_EVENT:
         case CMS_MSG_UNSUBSCRIBE_EVENT:
         {
            const char *op = (msg->type == CMS_MSG_SUBSCRIBE_EVENT) ?
                              "SUBSCRIBE_EVENT" : "UNSUBSCRIBE_EVENT";
            bcmuLog_notice("Got %s (processSubscribeEventFp=%p) flags=%d/%d/%d",
                           op, config->zbusConfig.processSubscribeEventFp,
                           msg->flags_request, msg->flags_response,
                           msg->flags_event);
            // Only makes sense to automatically handle the SUBSCRIBE_EVENT msg
            // if it is a request.  If it is a response, allow the app's
            // specific msg handler handle it.
            if (((msg->type == CMS_MSG_SUBSCRIBE_EVENT) && (msg->flags_request)) ||
                ((msg->type == CMS_MSG_UNSUBSCRIBE_EVENT) && (msg->flags_event)))
            {
               if (config->zbusConfig.processSubscribeEventFp != NULL)
               {
                  CmsMsgHeader *msgResp=NULL;
                  BcmRet ret2;
                  ret2 = (*config->zbusConfig.processSubscribeEventFp)(myCompName, msg, &msgResp);
                  if (ret2 == BCMRET_SUCCESS)
                  {
                     if (msg->type == CMS_MSG_SUBSCRIBE_EVENT)
                     {
                        cmsLog_debug("sending response to subscribe event");
                        cmsMsg_send(config->msgHandle, msgResp);
                        CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);
                     }
                  }
                  else
                  {
                     bcmuLog_debug("subscribeEvent failed, ret=%d", ret2);
                     compMd_sendReply(config->msgHandle, msg, (CmsRet) ret2);
                  }
               }
               else
               {
                  bcmuLog_debug("Got SUBSCRIBE_EVENT msg but no zbus handler func set.");
                  compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_METHOD_NOT_SUPPORTED);
               }
            }
            break;
         }

         case CMS_MSG_GET_EVENT_STATUS:
         {
            // Only make sense to handle the GET_EVENT_STATUS msg if it is a
            // request.  If it is a response, let the app's specific msg handler
            // handle it.
            if (msg->flags_request)
            {
               bcmuLog_debug("Got GET_EVENT_STATUS (processGetEventStatusFp=%p)",
                             config->zbusConfig.processGetEventStatusFp);
               if (config->zbusConfig.processGetEventStatusFp != NULL)
               {
                  CmsMsgHeader *msgResp=NULL;
                  BcmRet ret2;
                  ret2 = (*config->zbusConfig.processGetEventStatusFp)(msg, &msgResp);
                  bcmuLog_debug("zbus process func ret=%d", ret2);
                  if (ret2 == BCMRET_SUCCESS)
                  {
                     cmsMsg_send(config->msgHandle, msgResp);
                     CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);
                  }
                  else
                  {
                     bcmuLog_debug("getEvent failed, ret=%d", ret2);
                     compMd_sendReply(config->msgHandle, msg, (CmsRet) ret2);
                  }
               }
               else
               {
                  bcmuLog_debug("Got GET_EVENT_STATUS msg but no zbus handler func set.");
                  compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_METHOD_NOT_SUPPORTED);
               }
            }
            break;
         }

         case CMS_MSG_QUERY_EVENT_STATUS:
         {
            // Only make sense to handle the QUERY_EVENT_STATUS msg if it is a
            // request.  If it is a response, let the app's specific msg handler
            // handle it.
            if (msg->flags_request)
            {
               bcmuLog_debug("Got QUERY_EVENT_STATUS (processQueryEventStatusFp=%p)",
                             config->zbusConfig.processQueryEventStatusFp);
               if (config->zbusConfig.processQueryEventStatusFp != NULL)
               {
                  CmsMsgHeader *msgResp=NULL;
                  BcmRet ret2;
                  ret2 = (*config->zbusConfig.processQueryEventStatusFp)(0, msg, &msgResp);
                  bcmuLog_debug("zbus process func ret=%d", ret2);
                  if (ret2 == BCMRET_SUCCESS)
                  {
                     cmsMsg_send(config->msgHandle, msgResp);
                     CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);
                  }
                  else
                  {
                     bcmuLog_debug("queryEvent failed, ret=%d", ret2);
                     compMd_sendReply(config->msgHandle, msg, (CmsRet) ret2);
                  }
               }
               else
               {
                  bcmuLog_debug("Got QUERY_EVENT_STATUS msg but no zbus handler func set.");
                  compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_METHOD_NOT_SUPPORTED);
               }
            }
            break;
         }

         case CMS_MSG_TR69_ACTIVE_NOTIFICATION:
         case CMS_MSG_ALT_NOTIFICATION:
         {
            // This is the common handler for TR69_ACTIVE_NOTIFICATION.
            // dsl, voice, and gpon, which need to support RDK generic hal,
            // will intercept this message and possibly call RDK callback
            // functions.  But on this path, we will just send it up the ZBus
            // to sys_directory.
            bcmuLog_notice("Got %s from %d (0x%x)", 
                           (msg->type == CMS_MSG_TR69_ACTIVE_NOTIFICATION)? "TR69_ACTIVE_NOTIFICATION" : "ALT_NOTIFICATION",
                           msg->src, msg->src);

            if (config->zbusConfig.processPublishEventFp != NULL)
            {
               CmsMsgHeader *m = NULL;
               PubSubKeyValueMsgBody *body;
               UINT32 totalLen = 0;
               char keyBuf[PUBSUB_KEY_MAX_LEN+1] = {0};
               char valueBuf[BUFLEN_128] = {0};

               // Fill out a CMS msg for this event.
               snprintf(keyBuf, sizeof(keyBuf), "%s:%s",
                        (msg->type == CMS_MSG_TR69_ACTIVE_NOTIFICATION)? 
                        PUBSUB_KEY_MDM_NOTIFICATION_PREFIX : PUBSUB_KEY_MDM_ALT_NOTIFICATION_PREFIX , 
                        myCompName);
               cmsTms_getXSIDateTime(0, valueBuf, sizeof(valueBuf));
               totalLen = sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody) + strlen(valueBuf) + 1;
               m = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
               if (m == NULL)
               {
                  bcmuLog_error("alloc of %d bytes failed", totalLen);
               }
               else
               {
                  m->type = CMS_MSG_PUBLISH_EVENT;
                  m->src = msg->src;
                  m->dst = msg->dst;  // this is igored, but copy anyways
                  m->flags_event = 1;
                  m->wordData = PUBSUB_EVENT_KEY_VALUE;
                  m->dataLength = totalLen - sizeof(CmsMsgHeader);
                  body = (PubSubKeyValueMsgBody *)(m+1);
                  strcpy(body->key, keyBuf);
                  body->valueLen = strlen(valueBuf) + 1;
                  strcpy((char *)(body+1), valueBuf);
                  // PUBLISH_EVENT is an event, so no response msg returned.
                  (*config->zbusConfig.processPublishEventFp)(myCompName, m);
                  CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
               }
            }
            else
            {
               bcmuLog_error("Got %s msg but no zbus handler func set.", 
                           (msg->type == CMS_MSG_TR69_ACTIVE_NOTIFICATION)? "TR69_ACTIVE_NOTIFICATION" : "ALT_NOTIFICATION");
            }

            msgProcessed = 1;
            break;
         }

         case CMS_MSG_MD_PROXY_GET_PARAM:
            bcmuLog_debug("Got CMS_MSG_MD_PROXY_GET_PARAM");
            if (config->zbusConfig.processProxyGetParamFp != NULL)
            {
               CmsMsgHeader *msgResp = NULL;
               BcmRet ret2;
               ret2 = (*config->zbusConfig.processProxyGetParamFp)(msg, &msgResp);
               bcmuLog_debug("zbus process func ret=%d", ret2);
               if (ret2 == BCMRET_SUCCESS)
               {
                  cmsMsg_send(config->msgHandle, msgResp);
                  CMSMEM_FREE_BUF_AND_NULL_PTR(msgResp);
               }
               else
               {
                  bcmuLog_notice("proxy get parameter value failed, ret=%d", ret2);
                  compMd_sendReply(config->msgHandle, msg, (CmsRet) ret2);
               }
            }
            else
            {
               bcmuLog_debug("no zbus handler func set!");
               compMd_sendReply(config->msgHandle, msg, (CmsRet) BCMRET_METHOD_NOT_SUPPORTED);
            }
            msgProcessed = 1;
            break;

         default:
         {
            bcmuLog_error("Unhandled msg 0x%x from %d to %d wordData %d "
                          "dataLength %d flags (%d/%d/%d)",
                          msg->type, msg->src, msg->dst, msg->wordData,
                          msg->dataLength, msg->flags_request,
                          msg->flags_response, msg->flags_event);
         }
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(msg);

   bcmuLog_notice("Exit");
   return BCMRET_SUCCESS;
}


void compMd_processNotifyEvent(void *arg, CmsMsgHeader *msg)
{
   CompMdProcessMsgConfig *config = (CompMdProcessMsgConfig *) arg;
   BcmRet ret;

   // Sanity check inputs
   if (config == NULL)
   {
      bcmuLog_error("callback context is NULL");
      return;
   }
   if (config->msgHandle == NULL)
   {
      bcmuLog_error("msgHandle is NULL");
      return;
   }
   if (msg == NULL)
   {
      bcmuLog_error("msg is NULL");
      return;
   }

   bcmuLog_notice("Entered: msg type=0x%x wordData=%d len=%d dst=%d",
                   msg->type, msg->wordData, msg->dataLength, msg->dst);
      
   ret = (BcmRet) cmsMsg_send(config->msgHandle, msg);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("msg send failed, ret=%d", ret);
   }
   CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
   return;
}


BcmRet compMd_registerNamespace(void *msgHandle, UINT32 eid, const char *namespc)
{
   char buf[sizeof(CmsMsgHeader)+sizeof(PubSubNamespaceMsgBody)]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) buf;
   PubSubNamespaceMsgBody *body = (PubSubNamespaceMsgBody *) (msgHdr+1);
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   // This is not a bug: I am sending this msg to myself!  It will be processed
   // after ZBus is set up and calls our msg processing callback.
   msgHdr->src = eid;
   msgHdr->dst = eid;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_PUBLISH_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_NAMESPACE;
   msgHdr->dataLength = sizeof(PubSubNamespaceMsgBody);

   snprintf(body->namespc, sizeof(body->namespc), "%s", namespc);
   snprintf(body->ownerCompName, sizeof(body->ownerCompName), "%s", myCompName);
   ret = (BcmRet) cmsMsg_send(msgHandle, msgHdr);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not send namespace msg (%s[%s]), ret=%d",
                    namespc, myCompName, ret);
   }
   else
   {
      bcmuLog_notice("registered %s[%s]", namespc, myCompName);
   }

   return ret;
}

BcmRet compMd_subscribeKey(void *msgHandle, UBOOL8 sub,
                           UINT32 srcEid, UINT32 dstEid, const char *key)
{
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubKeyValueMsgBody)]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   PubSubKeyValueMsgBody *keyBody = (PubSubKeyValueMsgBody *) (msgHdr+1);
   CmsRet ret;

   msgHdr->src = srcEid;
   msgHdr->dst = dstEid;
   msgHdr->type = (sub ? CMS_MSG_SUBSCRIBE_EVENT : CMS_MSG_UNSUBSCRIBE_EVENT);
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->flags_request = (sub ? 1 : 0);  // subscribe is a request
   msgHdr->flags_event = (sub ? 0 : 1);    // unsubscribe is an event
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody);

   snprintf(keyBody->key, sizeof(keyBody->key), "%s", key);

   cmsLog_debug("Sending msgType 0x%x key=%s (src=%d dst=%d dataLength=%d)",
                msgHdr->type, keyBody->key,
                msgHdr->src, msgHdr->dst, msgHdr->dataLength);

   // Note that for subscribe, sys_directory will send back the current key value.
   // The caller needs to handle that reply msg.
   ret = cmsMsg_send(msgHandle, msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send subscribe/unsubscribe (%d) of %s, ret=%d",
                   sub, keyBody->key, ret);
   }

   return ((BcmRet) ret);
}

BcmRet compMd_publishMdmNotification(void *msgHandle, UINT32 eid, const char *componentName)
{
   CmsMsgHeader *msgHdr;
   PubSubKeyValueMsgBody *body;
   UINT32 msgLen;
   char *msg=NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   UINT32 valueLen=0;
   char timeStr[BUFLEN_128];

   if (componentName == NULL)
   {
      return ret;
   }
   cmsTms_getXSIDateTime(0,timeStr,sizeof(timeStr));
   {
      valueLen = strlen(timeStr);
   }
   msgLen = sizeof(CmsMsgHeader)+ sizeof(PubSubKeyValueMsgBody) + valueLen;
   msg = cmsMem_alloc(msgLen, ALLOC_ZEROIZE);

   if (msg == NULL)
   {
      cmsLog_error("Alloc of %d bytes failed", msgLen);
      return ((BcmRet) CMSRET_RESOURCE_EXCEEDED);
   }
   body = (PubSubKeyValueMsgBody *) (msg+sizeof(CmsMsgHeader));
   snprintf(body->key, sizeof(body->key), "%s:%s", PUBSUB_KEY_MDM_NOTIFICATION_PREFIX,componentName);
   body->valueLen = valueLen;
   if (valueLen > 1)
   {
      memcpy((char *)(body+1), timeStr, valueLen);
   }

   msgHdr = (CmsMsgHeader*)msg;
   msgHdr->src = eid;
   msgHdr->dst = eid;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_PUBLISH_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody) + valueLen;

   ret = (BcmRet) cmsMsg_send(msgHandle, msgHdr);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not send msg, ret=%d", ret);
   }

   bcmuLog_debug("%s[%s/%s] sent, ret=%d", componentName,body->key,timeStr,ret);
   CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
   return ret;
}

