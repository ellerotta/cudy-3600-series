/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2006:DUAL/GPL:standard
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 *
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 *
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 *
 * :>
 *
 ************************************************************************/


#include <time.h>
#include "cms.h"
#include "bdk.h"
#include "cms_mem.h"
#include "cms_msg.h"
#include "cms_log.h"
#include "cms_fil.h"
#include "cms_tms.h"
#include "oal.h"
#include "bcm_fsutils.h"

/**
* Two thresholds for put-back queue: 
* If number of CMS messages in put-back queue >= PUTBACK_WARNING, 
* warning message will be printed to the log destination; If number
* of messages reachs the threshold PUTBACK_DROP, The recent received 
* message will be discarded and error message is printed
*/
#define PUTBACK_WARNING   (1000)
#define PUTBACK_DROP      (1500)

#ifdef DESKTOP_LINUX
UINT16 desktopFakePid = 30;
#endif


#define CMS_MSG_PUTBACK_Q_IS_EMPTY(h) (((CmsMsgHandle *) (h))->putBackQueue == NULL)

static inline void dumpCmsMsgInfo(UINT32 depth, CmsMsgHeader *msg)
{
   if(depth < PUTBACK_WARNING)
   {
      return;
   }
   else if (depth < PUTBACK_DROP)
   {
      cmsLog_error("WARNING: %d messages in putback queue,something wrong?", depth);
   }
   else
   {
      cmsLog_error("Putback queue reaches its depth limit: %d, new incoming message will be dropped", depth);
   }

   if(msg)
   {
      cmsLog_error("src=%d, dst=%d, type=0x%x, %s, wordData=%d, dataLength=%d",
                  msg->src,
                  msg->dst,
                  msg->type,
                  msg->flags_event? "event": (msg->flags_request? "request" : "response"),
                  msg->wordData,
                  msg->dataLength);
   }
}

/* message API functions go here */

CmsRet cmsMsg_initOnBusWithTimeout(CmsEntityId eid, UINT32 flags, const char *busName,
                        UINT32 timeoutMs,
                        void **msgHandle)
{
   CmsRet ret;
   int tries = 0;
   int max_tries = timeoutMs / CMS_INIT_ON_BUS_RETRY_TIMEOUT_MS;

   if ((busName == NULL) || (msgHandle == NULL))
   {
      cmsLog_error("NULL input args %p/%p", busName, msgHandle);
      return CMSRET_INVALID_ARGUMENTS;
   }

   do
   {
      cmsLog_notice("EID=%d connecting to %s, flags=0x%x tries=%d/%d",
                    eid, busName, flags, tries, max_tries);
      ret = oalMsg_initOnBus(eid, flags, busName, msgHandle);
      if (ret == CMSRET_SUCCESS)
      {
         return ret;
      }

      if (tries < max_tries)  // don't sleep if there are no more tries
      {
         struct timespec ts;
         ts.tv_sec = 0;
         ts.tv_nsec = CMS_INIT_ON_BUS_RETRY_TIMEOUT_MS * 1000000;  // 200ms
         nanosleep(&ts, NULL);
         tries++;
      }
   } while (tries < max_tries);

   if (bcmUtl_isShutdownInProgress())
   {
      return CMSRET_FAIL_REBOOT_REQUIRED;
   }
   else
   {
      cmsLog_error("EID=%d could not connect to %s, flags=0x%x tries=%d/%d",
                    eid, busName, flags, tries, max_tries);
      return CMSRET_INTERNAL_ERROR;
   }
}

CmsRet cmsMsg_initOnBus(CmsEntityId eid, UINT32 flags, const char *busName,
                        void **msgHandle)
{
   return (cmsMsg_initOnBusWithTimeout(eid, flags, busName, 0, msgHandle));
}

CmsRet cmsMsg_initWithFlags(CmsEntityId eid, UINT32 flags, void **msgHandle)
{
   if (msgHandle == NULL)
   {
      cmsLog_error("msgHandle is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   return cmsMsg_initOnBus(eid, flags, SMD_MESSAGE_ADDR, msgHandle);
}

CmsRet cmsMsg_init(CmsEntityId eid, void **msgHandle)
{
   const CmsEntityInfo *eInfo;

   if (msgHandle == NULL)
   {
      cmsLog_error("msgHandle is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((eInfo = cmsEid_getEntityInfo(eid)) == NULL)
   {
      cmsLog_error("Invalid eid %d", eid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return cmsMsg_initWithFlags(eid,
                         eInfo->flags & (EIF_MULTIPLE_INSTANCES_OR_THREADS),
                         msgHandle);
}

CmsRet cmsMsg_setBusManager(void *msgHandle, CmsEntityId eid)
{
   CmsMsgHeader msg=EMPTY_MSG_HEADER;
   CmsRet ret;

   cmsLog_notice("Setting eid=%d as bus manager", eid);
   msg.type = CMS_MSG_SET_BUS_MANAGER;
   msg.src = eid;
   msg.dst = EID_BCM_MSGD;
   msg.flags_event = 1;
   ret = cmsMsg_send(msgHandle, &msg);
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("done, success!");
   }
   else
   {
      cmsLog_error("Could not send SET_BUS_MANAGER msg! ret=%d", ret);
   }
   return ret;
}

void cmsMsg_cleanup(void **msgHandle)
{
   CmsMsgHandle *handle;
   CmsMsgHeader *msg;

   if (msgHandle == NULL)
      return;

   handle = (CmsMsgHandle *) *msgHandle;
   if (handle == NULL)
      return;

   /* free any queued up messages */
   while ((msg = handle->putBackQueue) != NULL)
   {
      handle->putBackQueue = (CmsMsgHeader *) msg->next;
      CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
   }

   return oalMsg_cleanup(msgHandle);
}


CmsRet cmsMsg_send(void *msgHandle, const CmsMsgHeader *buf)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;

   if ((msgHandle == NULL) || (buf == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, buf);
      return CMSRET_INVALID_ARGUMENTS;
   }

#ifdef DESKTOP_LINUX
   if (handle->standalone)
   {
      /* just pretend to have sent the message */
      return CMSRET_SUCCESS;
   }
#endif

   return oalMsg_send(handle->commFd, buf);
}


CmsRet cmsMsg_sendReply(void *msgHandle, const CmsMsgHeader *msg, CmsRet retCode)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;
   CmsMsgHeader replyMsg = EMPTY_MSG_HEADER;

   if ((msgHandle == NULL) || (msg == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, msg);
      return CMSRET_INVALID_ARGUMENTS;
   }

   replyMsg.dst = msg->src;
   replyMsg.src = msg->dst;
   replyMsg.type = msg->type;

   replyMsg.flags_request = 0;
   replyMsg.flags_response = 1;
   replyMsg.flags_bounceIfNotRunning = msg->flags_bounceIfNotRunning;
   /* do we want to copy any other flags? */

   replyMsg.wordData = retCode;

   return oalMsg_send(handle->commFd, &replyMsg);
}


static void updateReceiveTimeout(const CmsTimestamp *startTs, UINT32 *timeout)
{
   if (timeout != NULL)
   {
      CmsTimestamp nowTs;
      UINT32 dec;
      UINT32 min=10;  // decrement a min of 10ms per call to ensure decrease.

      cmsTms_get(&nowTs);
      dec = cmsTms_deltaInMilliSeconds(&nowTs, startTs);
      dec = (dec < min) ? min : dec;
      if (*timeout <= dec)
         *timeout = 1;  // avoid 0, which might mean block indefinately.
      else
         *timeout -= dec;
   }
   return;
}



static CmsRet getReplyBuf(CmsMsgHandle *msgHandle, const CmsMsgHeader *req,
                          CmsMsgHeader **replyBuf, UINT32 *timeout)
{
   CmsMsgHeader *replyMsg = NULL;
   UBOOL8 doReceive = TRUE;
   CmsMsgType sentType;
   CmsEntityId sentDst;
   CmsRet ret = CMSRET_SUCCESS;

   sentType = req->type;
   sentDst = req->dst;

   while (doReceive)
   {
      CmsTimestamp startTs;
      cmsTms_get(&startTs);

      ret = oalMsg_receive(msgHandle->commFd, &replyMsg, timeout);
      if (ret != CMSRET_SUCCESS)
      {
         if ((timeout == NULL) ||
             ((timeout != NULL) && (ret != CMSRET_TIMED_OUT)))
         {
            if (!bcmUtl_isShutdownInProgress())
            {
               cmsLog_error("error during get of reply, ret=%d", ret);
            }
         }
         cmsMem_free(replyMsg);
         doReceive = FALSE;
      }
      else
      {
         if ((replyMsg->type == sentType) &&
             (replyMsg->src == sentDst) &&
             (replyMsg->flags_response))
         {
            if (*replyBuf != NULL)
            {
               // This is the older, dangerous interface.  Caller assumes he
               // knows the length of reply msg.  We just blindly copy the
               // reply into the given buffer.
               /* coverity[overflow] */
               memcpy((*replyBuf), replyMsg, (sizeof(CmsMsgHeader) + replyMsg->dataLength));
               CMSMEM_FREE_BUF_AND_NULL_PTR(replyMsg);
            }
            else
            {
               // This is the newer, safe interface.  We give our allocated
               // response buffer to the caller.  Caller is responsible for
               // freeing it.
               *replyBuf = replyMsg;
            }
            doReceive = FALSE;
         }
         else if (replyMsg->type == CMS_MSG_INTERNAL_NOOP)
         {
            // Do not requeue this message.  Just drop it.  We will generate
            // another one at the bottom of this function if needed.
            updateReceiveTimeout(&startTs, timeout);
            CMSMEM_FREE_BUF_AND_NULL_PTR(replyMsg);
         }
         else
         {
            /* we got a mesage, but it was not the reply we were expecting.
             * Could be an event msg.  Push it back on the put-back queue and
             * keep trying to get the message we really want.
             */
            cmsLog_debug("Not the msg we wanted (0x%x from %d), put back 0x%x from %d (%d/%d/%d)",
                         sentType, sentDst, replyMsg->type, replyMsg->src,
                         replyMsg->flags_request, replyMsg->flags_response,
                         replyMsg->flags_event);
            cmsMsg_putBack(msgHandle, &replyMsg);
            replyMsg = NULL;
            updateReceiveTimeout(&startTs, timeout);
         }
      }
   }

   if (!CMS_MSG_PUTBACK_Q_IS_EMPTY(msgHandle))
   {
      cmsMsg_sendNoop(msgHandle);
   }

   return ret;
}

static CmsRet sendAndGetReply(CmsMsgHandle *msgHandle, const CmsMsgHeader *buf, UINT32 *timeout)
{
   CmsMsgHeader *replyMsg = NULL;
   CmsRet ret;

#ifdef DESKTOP_LINUX
   if (msgHandle->standalone)
   {
      CmsMsgHeader *msg = (CmsMsgHeader *) buf;

      /*
       * Standalone mode occurs during unittests.
       * Pretend to send out the message and get a successful reply.
       */
      if ((msg->type == CMS_MSG_START_APP) || (msg->type == CMS_MSG_RESTART_APP))
      {
         /* For the START_APP and RESTART_APP messages, the expected return value is the pid. */
         return desktopFakePid++;
      }
      else
      {
         return CMSRET_SUCCESS;
      }
   }
#endif

   ret = oalMsg_send(msgHandle->commFd, buf);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = getReplyBuf(msgHandle, buf, &replyMsg, timeout);
   if (ret == CMSRET_SUCCESS)
   {
       ret = replyMsg->wordData;
       CMSMEM_FREE_BUF_AND_NULL_PTR(replyMsg);
   }

   return ret;
}

static CmsRet sendAndGetReplyBuf(CmsMsgHandle *msgHandle, const CmsMsgHeader *req,
                                CmsMsgHeader **replyBuf, UINT32 *timeout)
{
   CmsRet ret;

   ret = oalMsg_send(msgHandle->commFd, req);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = getReplyBuf(msgHandle, req, replyBuf, timeout);

   return ret;
}


CmsRet cmsMsg_sendAndGetReply(void *msgHandle, const CmsMsgHeader *buf)
{
   if ((msgHandle == NULL) || (buf == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, buf);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return sendAndGetReply((CmsMsgHandle *)msgHandle, buf, NULL);
}


CmsRet cmsMsg_sendAndGetReplyWithTimeout(void *msgHandle,
                                         const CmsMsgHeader *buf,
                                         UINT32 timeoutMilliSeconds)
{
   UINT32 timeout = timeoutMilliSeconds;

   if ((msgHandle == NULL) || (buf == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, buf);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return sendAndGetReply((CmsMsgHandle *)msgHandle, buf, &timeout);
}

CmsRet cmsMsg_sendAndGetReplyBuf(void *msgHandle, const CmsMsgHeader *buf, CmsMsgHeader **replyBuf)
{
   if ((msgHandle == NULL) || (buf == NULL) || (replyBuf == NULL))
   {
      cmsLog_error("NULL input args %p/%p/%p", msgHandle, buf, replyBuf);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return sendAndGetReplyBuf((CmsMsgHandle *)msgHandle, buf, replyBuf, NULL);
}

CmsRet cmsMsg_sendAndGetReplyBufWithTimeout(void *msgHandle, const CmsMsgHeader *buf,
                    CmsMsgHeader **replyBuf, UINT32 timeoutMilliSeconds)
{
   UINT32 timeout = timeoutMilliSeconds;

   if ((msgHandle == NULL) || (buf == NULL) || (replyBuf == NULL))
   {
      cmsLog_error("NULL input args %p/%p/%p", msgHandle, buf, replyBuf);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return sendAndGetReplyBuf((CmsMsgHandle *)msgHandle, buf, replyBuf, &timeout);
}

/* This api should only be called after the requset CMS message has been sent.
 * It is intended to provide higher level of control for the calling process.
 * For example, if the calling process wants to enforce sequenceNumber in replyBuf
 * should match the sequenceNumber in request. Since "sequenceNumber" is optional
 * field in CmsMsgHeader, the check for its match is done outside of cms msg library itself.
 */
CmsRet cmsMsg_getReplyBufWithTimeout(void *msgHandle, const CmsMsgHeader *buf,
                          CmsMsgHeader **replyBuf, UINT32 timeoutMilliSeconds)
{
   UINT32 timeout = timeoutMilliSeconds;

   if ((msgHandle == NULL) || (buf == NULL) || (replyBuf == NULL))
   {
      cmsLog_error("NULL input args %p/%p/%p", msgHandle, buf, replyBuf);
      return CMSRET_INVALID_ARGUMENTS;
   }

    return getReplyBuf((CmsMsgHandle *)msgHandle, buf, replyBuf, &timeout);
}



static CmsRet getPutbackMsg(void *msgHandle, CmsMsgHeader **buf)
{
   CmsRet ret = CMSRET_NO_MORE_INSTANCES;

   if (buf == NULL)
   {
      cmsLog_error("buf is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }
   else
   {
      *buf = NULL;
   }

   if (!CMS_MSG_PUTBACK_Q_IS_EMPTY(msgHandle))
   {
      CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;

      *buf = handle->putBackQueue;
      handle->putBackQueue = (CmsMsgHeader *) (*buf)->next;
      (*buf)->next = 0L;

      ret = CMSRET_SUCCESS;
   }

   return ret;
}


CmsRet cmsMsg_receive(void *msgHandle, CmsMsgHeader **buf)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;
   CmsRet ret;

   if ((msgHandle == NULL) || (buf == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, buf);
      return CMSRET_INVALID_ARGUMENTS;
   }

#ifdef DESKTOP_LINUX
   if (handle->standalone)
   {
      /*
       * Hmm, this is a tricky situation.  Caller has told us to block until
       * we get a message, but since smd is not running, we will never get
       * a message.  Return INTERNAL_ERROR and let caller handle it?
       */
      cmsLog_error("cannot receive msg while in standalone (unittest) mode");
      *buf = NULL;
      return CMSRET_INTERNAL_ERROR;
   }
#endif

   /*
    * First check for any messages queued in the putback queue.  If nothing
    * there, then go to the real socket to read.
    */
   ret = getPutbackMsg(msgHandle, buf);
   if (ret == CMSRET_SUCCESS || ret == CMSRET_INVALID_ARGUMENTS)
   {
      return ret;
   }

   return oalMsg_receive(handle->commFd, buf, NULL);
}


CmsRet cmsMsg_receiveWithTimeout(void *msgHandle, CmsMsgHeader **buf, UINT32 timeoutMilliSeconds)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;
   UINT32 timeout = timeoutMilliSeconds;
   CmsRet ret;

   if ((msgHandle == NULL) || (buf == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, buf);
      return CMSRET_INVALID_ARGUMENTS;
   }

#ifdef DESKTOP_LINUX
   if (handle->standalone)
   {
      *buf = NULL;
      return CMSRET_TIMED_OUT;
   }
#endif

   /*
    * First check for any messages queued in the putback queue.  If nothing
    * there, then go to the real socket to read.
    */
   ret = getPutbackMsg(msgHandle, buf);
   if (ret == CMSRET_SUCCESS || ret == CMSRET_INVALID_ARGUMENTS)
   {
      return ret;
   }

   return oalMsg_receive(handle->commFd, buf, &timeout);
}


CmsRet cmsMsg_getEventHandle(const void *msgHandle, void *eventHandle)
{
   if ((msgHandle == NULL) || (eventHandle == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, eventHandle);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return (oalMsg_getEventHandle((CmsMsgHandle *) msgHandle, eventHandle));
}


CmsEntityId cmsMsg_getHandleEid(const void *msgHandle)
{
   const CmsMsgHandle *handle = (const CmsMsgHandle *) msgHandle;

   if (msgHandle == NULL)
   {
      cmsLog_error("msgHandle is NULL!");
      return EID_INVALID;
   }

   return (handle->eid);
}

CmsEntityId cmsMsg_getHandleSpecificEid(const void *msgHandle)
{
   return (cmsMsg_getHandleEid(msgHandle));
}

CmsEntityId cmsMsg_getHandleGenericEid(const void *msgHandle)
{
   const CmsMsgHandle *handle = (const CmsMsgHandle *) msgHandle;

   if (msgHandle == NULL)
   {
      cmsLog_error("msgHandle is NULL!");
      return EID_INVALID;
   }

   return (GENERIC_EID(handle->eid));
}


const char *cmsMsg_getBusName(const void *msgHandle)
{
   const CmsMsgHandle *handle = (const CmsMsgHandle *) msgHandle;

   if (msgHandle == NULL)
   {
      cmsLog_error("msgHandle is NULL!");
      return NULL;
   }

   return (handle->busName);
}

CmsMsgHeader *cmsMsg_duplicate(const CmsMsgHeader *msg)
{
   UINT32 totalLen;
   void *newMsg;

   if (msg == NULL)
   {
      cmsLog_error("msg is NULL!");
      return NULL;
   }

   totalLen = sizeof(CmsMsgHeader) + msg->dataLength;
   newMsg = cmsMem_alloc(totalLen, 0);
   if (newMsg != NULL)
   {
      memcpy(newMsg, msg, totalLen);
   }

   return newMsg;
}


void cmsMsg_putBack(void *msgHandle, CmsMsgHeader **buf)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;
   CmsMsgHeader *prevMsg;
   UINT32 len=1;

   if ((msgHandle == NULL) || (buf == NULL))
   {
      cmsLog_error("NULL input args %p/%p", msgHandle, buf);
      return;
   }

   (*buf)->next = 0L;

   /* put the new message at the end of the putBackQueue */
   if (handle->putBackQueue == NULL)
   {
      handle->putBackQueue = (*buf);
   }
   else
   {
      prevMsg = handle->putBackQueue;
      while (prevMsg->next != 0L)
      {
         prevMsg = (CmsMsgHeader *) prevMsg->next;
         len++;
      }

      dumpCmsMsgInfo(len, *buf);

      if (len < PUTBACK_DROP)
      {
         prevMsg->next = (uint64_t)(*buf);
      }
      else
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(*buf);
      }
   }

   /* we've taken ownership of this msg, so null out caller's pointer */
   *buf = NULL;
   return;
}


void cmsMsg_sendNoop(void *msgHandle)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) msgHandle;
   CmsMsgHeader noopMsg = EMPTY_MSG_HEADER;

   if (msgHandle == NULL)
   {
      cmsLog_error("msgHandle is NULL!");
      return;
   }

   noopMsg.dst = handle->putBackQueue->dst;
   noopMsg.src = handle->putBackQueue->dst;
   noopMsg.type = CMS_MSG_INTERNAL_NOOP;

   noopMsg.flags_event = 1;

   oalMsg_send(handle->commFd, &noopMsg);

   return;
}


UBOOL8 cmsMsg_isServiceReady(void)
{
   return cmsFil_isFilePresent(SMD_MESSAGE_ADDR);
}


SINT32 cmsMsg_initUnixDomainServerSocket(const char *msgBusName, SINT32 backlog)
{
   return oalMsg_initUnixDomainServerSocket(msgBusName, backlog);
}


typedef struct {
   const char *compName;
   const char *busName;
} CompNameToBusName;

CompNameToBusName compNameToBusNameTable[] =
{
   {BDK_COMP_DEVINFO,  DEVINFO_MSG_BUS},
   {BDK_COMP_DIAG,     DIAG_MSG_BUS},
   {BDK_COMP_DSL,      DSL_MSG_BUS},
   {BDK_COMP_GPON,     GPON_MSG_BUS},
   {BDK_COMP_EPON,     EPON_MSG_BUS},
   {BDK_COMP_WIFI,     WIFI_MSG_BUS},
   {BDK_COMP_VOICE,    VOICE_MSG_BUS},
   {BDK_COMP_STORAGE,  STORAGE_MSG_BUS},
   {BDK_COMP_TR69,     TR69_MSG_BUS},
   {BDK_COMP_USP,      USP_MSG_BUS},
   {BDK_COMP_SYSMGMT,  SYSMGMT_MSG_BUS},
   {BDK_COMP_OPENPLAT, OPENPLAT_MSG_BUS},
   {BDK_COMP_SYS_DIRECTORY,   SYS_DIRECTORY_MSG_BUS},
};

const char *cmsMsg_componentNameToBusName(const char *compName)
{
   UINT32 i;

   for (i=0; i < sizeof(compNameToBusNameTable)/sizeof(CompNameToBusName); i++)
   {
      if (!strcasecmp(compNameToBusNameTable[i].compName, compName))
      {
         return compNameToBusNameTable[i].busName;
      }
   }

   return NULL;
}

const char *cmsMsg_busNameToComponentName(const char *busName)
{
   UINT32 i;

   for (i=0; i < sizeof(compNameToBusNameTable)/sizeof(CompNameToBusName); i++)
   {
      if (!strcasecmp(compNameToBusNameTable[i].busName, busName))
      {
         return compNameToBusNameTable[i].compName;
      }
   }

   return NULL;
}
