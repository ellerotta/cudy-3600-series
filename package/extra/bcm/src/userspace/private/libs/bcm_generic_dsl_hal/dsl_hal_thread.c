/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom 
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

/*!\file dsl_hal_thread.c
 * \brief The DSL HAL thread the BCM Generic DSL HAL.
 *        It handles all the messages and events coming from the lower layers
 *        of the DSL stack (including dsl_ssk and bcm_msgd) and performs
 *        callbacks to the user of the HAL (BDK: dsl_md, RDK: ccsp_dsl_agent)
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bcm_ulog.h"
#include "sysutil.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "bcm_comp_md.h"
#include "bcm_generic_dsl_hal.h"
#include "cms_tms.h"


// Declared in generic_dsl_hal.c
extern void  *_dslHalMsgHandle;
extern SINT32 _dslHalShmId;
extern SINT32 _dslHalInitialized;
extern DslHalConfig _dslHalConfig;


/* forward declaration */
int dsl_hal_processSpecificMsg(CmsMsgHeader *msg);


/** main function of the DSL HAL thread. */
void *dsl_hal_thread_main(void *context)
{
   CompMdProcessMsgConfig msgConfig;
   SINT32 threadId = sysUtl_getThreadId();
   CmsEntityId eid = EID_DSL_HAL_THREAD;
   BcmRet ret;
   long pointer2Int = (long)context; // Pointer conversion for 32 and 64 bit arch

   // Set my thread name and create a EID to threadId mapping.
   bcmuLog_setNameEx("dsl_hal", threadId, (UINT32) eid);

   bcmuLog_notice("Entered: eid=%d threadId=%d", eid, threadId);

   // initDslSouth will basically start the DSL "stack": start bcm_msgd,
   // dsl_ssk, which will create and initialize the MDM.
   ret = compMd_initDslSouth(&_dslHalMsgHandle, &_dslHalShmId, (SINT32)pointer2Int);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("DSL component (south) init failed! ret=%d", ret);
      return NULL;
   }

   // Let the main thread know that init is done.
   _dslHalInitialized = 1;

   // Configure the bcm_comp_md message processing loop.
   memset(&msgConfig, 0, sizeof(msgConfig));
   msgConfig.msgHandle = _dslHalMsgHandle;
   msgConfig.shmIdPtr = &_dslHalShmId;
   msgConfig.mdmInitializedPtr = &mdmInitialized;  // mdmInitialized is declared in bcm_comp_md.c
   msgConfig.processSpecificCmsMessageFp = dsl_hal_processSpecificMsg;
   // Do not set the zbusConfig.  DSL HAL thread does not talk to ZBus.

   while (1)
   {
      // DSL HAL thread only interacts with the rest of the DSL stack via
      // CMS msg.  So it can just dive into this and wait forever.  Other
      // HAL threads might have different requirements.
      compMd_processMsg(&msgConfig);
   }

   return NULL;
}


/** message proccessing for the DSL HAL thread. Calls the callbacks.  */
int dsl_hal_processSpecificMsg(CmsMsgHeader *msg)
{
   int msgProcessed = 0;

   bcmuLog_notice("got msg type=0x%x src=%d (0x%x) dst=%d (0x%x) wordData=%d dataLength=%d",
                  msg->type, msg->src, msg->src, msg->dst, msg->dst,
                  msg->wordData, msg->dataLength);
   
   switch(msg->type)
   {
      case CMS_MSG_PUBLISH_EVENT:
      case CMS_MSG_UNPUBLISH_EVENT:
      {
         int pub = (msg->type == CMS_MSG_PUBLISH_EVENT);
         const char *op = ((msg->type == CMS_MSG_PUBLISH_EVENT) ?
                           "PUBLISH_EVENT" : "UNPUBLISH_EVENT");
         if (msg->wordData == PUBSUB_EVENT_INTERFACE)
         {
            PubSubInterfaceMsgBody *body = (PubSubInterfaceMsgBody *)(msg+1);
            bcmuLog_notice("got %s on %s (isLinkUp=%d fullpath=%s)",
                           op, body->intfName, body->isLinkUp, body->fullpath);
            // Fill in DSL HAL callback struct and do callback.
            // TODO: does not handle additionalStrData
            if (_dslHalConfig.publishInterfaceStatusFp)
            {
               DslHalInterfaceStatus status;
               memset(&status, 0, sizeof(status));
               snprintf(status.intfName, sizeof(status.intfName), "%s",
                        body->intfName);
               status.isLinkUp = body->isLinkUp;
               snprintf(status.fullpath, sizeof(status.fullpath), "%s",
                        body->fullpath);
               (*_dslHalConfig.publishInterfaceStatusFp)(pub, &status);
            }
            else
            {
               // This callback is optional, so don't complain loudly
               bcmuLog_notice("publishInterfaceStatusFp callback is NULL");
            }
         }
         else if (msg->wordData == PUBSUB_EVENT_KEY_VALUE)
         {
            PubSubKeyValueMsgBody *body = (PubSubKeyValueMsgBody *)(msg+1);
            char *value = (char *)(body+1);
            bcmuLog_notice("got %s kv %s=%s", op, body->key, value);
            if (_dslHalConfig.publishKeyValueFp)
            {
               (*_dslHalConfig.publishKeyValueFp)(pub, body->key, value);
            }
            else
            {
               // This callback is optional, so don't complain loudly
               bcmuLog_debug("publishKeyValueFp callback is NULL");
            }
         }
         else
         {
            bcmuLog_error("Unsupported PUBLISH_EVENT type=%d", msg->wordData);
         }

         msgProcessed = 1;

         // sender of PUBLISH_EVENT does not expect a reply.  So nothing more
         // needs to be done here.
         break;
      }

      case CMS_MSG_SUBSCRIBE_EVENT:
      case CMS_MSG_UNSUBSCRIBE_EVENT:
      {
         int sub = (msg->type == CMS_MSG_SUBSCRIBE_EVENT);
         const char *op = ((msg->type == CMS_MSG_SUBSCRIBE_EVENT) ?
                           "SUBSCRIBE_EVENT" : "UNSUBSCRIBE_EVENT");
         // It only makes sense for dsl_hal thread to receive SUBSCRIBE or
         // UNSUBSCRIBE msg from dsl_ssk.  Also for SUBSCRIBE, only process
         // requests; responses should not come here.
         if ((msg->src == EID_DSL_SSK) &&
             (((msg->type == CMS_MSG_SUBSCRIBE_EVENT) && (msg->flags_request)) ||
              ((msg->type == CMS_MSG_UNSUBSCRIBE_EVENT) && (msg->flags_event))))
         {
            PubSubKeyValueMsgBody *body = (PubSubKeyValueMsgBody *)(msg+1);
            char valueBuf[256]={0};
            int valueLen=sizeof(valueBuf)-1;
            BcmRet ret=BCMRET_SUCCESS;

            if (msg->wordData == PUBSUB_EVENT_KEY_VALUE)
            {
               bcmuLog_notice("got %s on %s (subscribeKeyFp=%p)",
                              op, body->key, _dslHalConfig.subscribeKeyFp);
               if (_dslHalConfig.subscribeKeyFp)
               {
                  int rc;
                  rc = (*_dslHalConfig.subscribeKeyFp)(sub, body->key, valueBuf, &valueLen);
                  bcmuLog_debug("upper level callback func rc=%d valueLen=%d",
                                rc, valueLen);
                  ret = (rc == RETURN_OK) ? BCMRET_SUCCESS : BCMRET_UNKNOWN_ERROR;
               }
               else
               {
                  // This callback is optional.
                  bcmuLog_debug("subscribeKeyFp callback is NULL");
                  ret = BCMRET_METHOD_NOT_SUPPORTED;
               }
            }
            else
            {
               bcmuLog_error("Unsupported %s type=%d", op, msg->wordData);
               ret = BCMRET_INVALID_ARGUMENTS;
            }

            // Sender of SUBSCRIBE_EVENT expects a reply msg.  (UNSUBSCRIBE is
            // an event, so no response.
            if (msg->type == CMS_MSG_SUBSCRIBE_EVENT)
            {
               char replyBuf[sizeof(CmsMsgHeader)+sizeof(PubSubKeyValueMsgBody)+sizeof(valueBuf)]={0};
               CmsMsgHeader *replyHdr = (CmsMsgHeader *) replyBuf;
               PubSubKeyValueMsgBody *replyBody = (PubSubKeyValueMsgBody *)(replyHdr+1);

               // Fill in common parts of the header
               replyHdr->type = msg->type;
               replyHdr->src = msg->dst;
               replyHdr->dst = msg->src;
               replyHdr->flags_response = 1;

               if (ret == BCMRET_SUCCESS)
               {
                  replyHdr->wordData = msg->wordData;
                  replyHdr->dataLength = sizeof(PubSubKeyValueMsgBody)+strlen(valueBuf)+1;
                  strcpy(replyBody->key, body->key);
                  replyBody->valueLen = strlen(valueBuf)+1;
                  // valueBuf is guaranteed to be null terminated and fits in buf.
                  strcpy((char *)(replyBody+1), valueBuf);
               }
               else
               {
                  // If no callback func was provided, ret will be 
                  // BCMRET_METHOD_NOT_SUPPORTED.  Don't complain too loudly
                  // on that since that is an expected condition in RDK.
                  if (ret != BCMRET_METHOD_NOT_SUPPORTED)
                     bcmuLog_error("received error from callback func, ret=%d", ret);
                  // By convention, we set the error code in wordData, which
                  // unfortunately overwrites the PubSubEventType.  Fortunately,
                  // PubSubEventType only has values of 1,2,3,or20, so it is
                  // pretty easy to distinguish between that and an BcmRet code.
                  replyHdr->wordData = ret;
                  replyHdr->dataLength = 0;
               }

               bcmuLog_debug("sending reply to dsl_ssk (eid=%d) ret=%d",
                             replyHdr->dst, ret);
               ret = (BcmRet) cmsMsg_send(_dslHalMsgHandle, replyHdr);
               if (ret != BCMRET_SUCCESS)
               {
                  bcmuLog_error("Could not send reply to subscribe %s msg, ret=%d",
                                body->key, ret);
               }
               else
               {
                  bcmuLog_debug("replied to %d wordData=%d subscribe %s",
                                replyHdr->dst, replyHdr->wordData, body->key);
               }
            }  // end of send reply block

            msgProcessed = 1;
         }  // end of if ((msg->src == EID_DSL_SSK) && (msg->flags_request))
         else
         {
            bcmuLog_error("Unexpected msg from %d req=%d resp=%d event=%d wordData=%d key=%s",
                          msg->src, msg->flags_request, msg->flags_response,
                          msg->flags_event, msg->wordData, (char *)(msg+1));
         }

         break;
      }

      case CMS_MSG_QUERY_EVENT_STATUS:
      {
         // It only makes sense for dsl_hal thread to receive a query 
         // interface request from dsl_ssk.
         if ((msg->src == EID_DSL_SSK) &&
             (msg->flags_request) &&
             (msg->wordData == PUBSUB_EVENT_INTERFACE) &&
             (msg->dataLength > 1))
         {
            int valueLen = 512;  // should be enough for response data, which is just an interface name
            char msgBuf[sizeof(CmsMsgHeader) + 512] = {0};
            CmsMsgHeader *respMsg = (CmsMsgHeader *) msgBuf;
            if (_dslHalConfig.queryInterfaceFp)
            {
               int rc = (*_dslHalConfig.queryInterfaceFp)((char *)(msg+1),
                                              (char *)(respMsg+1), &valueLen);
               // On success, set wordData to PUBSUB_EVENT_INTERFACE
               // The result data will be copied to the payload area.
               if (rc == RETURN_OK)
               {
                  respMsg->wordData = PUBSUB_EVENT_INTERFACE;
                  respMsg->dataLength = valueLen;
                  bcmuLog_debug("good query: %s (%d)", (char *)(respMsg+1), respMsg->dataLength);
               }
               else
               {
                  respMsg->wordData = BCMRET_UNKNOWN_ERROR;
               }
            }
            else
            {
               // unittests and RDK do not define this callback, so don't
               // complain too loudly.
               bcmuLog_debug("queryInterfaceFp not defined");
               respMsg->wordData = BCMRET_METHOD_NOT_SUPPORTED;
            }
            respMsg->type = msg->type;
            respMsg->src = msg->dst;
            respMsg->dst = msg->src;
            respMsg->flags_response = 1;
            cmsMsg_send(_dslHalMsgHandle, respMsg);
            msgProcessed = 1;
         }
         else
         {
            bcmuLog_error("Unexpected QUERY_EVENT_STATUS msg from %d wordData=%d len=%d flags=%d/%d/%d",
                          msg->src, msg->wordData, msg->dataLength,
                          msg->flags_request, msg->flags_response,
                          msg->flags_event);
            // Let this fall through to common msg processor?  Seems unlikely
            // to be a good msg.
         }
         break;
      }

      case CMS_MSG_TR69_ACTIVE_NOTIFICATION:
      case CMS_MSG_ALT_NOTIFICATION:
      {
         char keyBuf[PUBSUB_KEY_MAX_LEN+1] = {0};
         char valueBuf[BUFLEN_128] = {0};

         bcmuLog_notice("Got %s NOTIFICATION from %d (0x%x)", 
                     (msg->type == CMS_MSG_TR69_ACTIVE_NOTIFICATION)?  "TR69C_ACTIVE" : "ALT",
                     msg->src, msg->src);

         cmsTms_getXSIDateTime(0, valueBuf, sizeof(valueBuf));
         snprintf(keyBuf, sizeof(keyBuf), "%s:%s",
                  (msg->type == CMS_MSG_TR69_ACTIVE_NOTIFICATION)? 
                  PUBSUB_KEY_MDM_NOTIFICATION_PREFIX : PUBSUB_KEY_MDM_ALT_NOTIFICATION_PREFIX, 
                  BDK_COMP_DSL);

         if (_dslHalConfig.publishKeyValueFp)
         {
            int pub=1;  // publish this key value
            (*_dslHalConfig.publishKeyValueFp)(pub, keyBuf, valueBuf);
         }
         else
         {
            // This callback is optional, so don't complain loudly
            bcmuLog_debug("publishKeyValueFp callback is NULL");
         }
         msgProcessed = 1;
         break;
      }

      case CMS_MSG_WAN_LINK_UP:
      case CMS_MSG_WAN_LINK_DOWN:
         /* ignore this..  this event is published in BDK */
         bcmuLog_notice("Got and ignored WAN_LINK_UP/DOWN from %d (0x%d)",
                        msg->src, msg->src);
         msgProcessed = 1;
         break;

      case CMS_MSG_TERMINATE:
      {
         // do the same as dslMd_processSpecificCmsMessage
         bcmuLog_notice("got CMS_MSG_TERMINATE from %d (0x%x)",
                       msg->src, msg->src);
         compMd_cleanupDsl(_dslHalMsgHandle);

         // Even though we handled the message here, leave msgProcessed = 0
         // so that the common handler for CMS_MSG_TERMINATE in bcm_comp_md.c
         // will get executed.
         msgProcessed = 0;
         break;
      }

      default:
         // Do nothing, let the common message processor handle it.
         break;
   }

   // compMd_processMsg will free the msg buf, so don't do it here.
   return msgProcessed;
}

