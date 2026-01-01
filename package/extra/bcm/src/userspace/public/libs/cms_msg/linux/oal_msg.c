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


/* OS dependent messaging functions go here */

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include "../oal.h"
#include "cms_util.h"
#include "bcm_ulog.h"
#include "bcm_fsutils.h"
#include "sysutil.h"


static CmsRet waitForReadWriteAvailable(SINT32 fd, int mode, UINT32 timeout);

CmsRet oalMsg_initOnBus(CmsEntityId eid, UINT32 flags, const char *busName,
                        void **msgHandle)
{
   CmsMsgHandle *handle;
   struct sockaddr_un serverAddr;
   SINT32 rc;
   SINT32 argThreadId = THREAD_ID_IN_EID(eid);
   SINT32 actualThreadId = sysUtl_getThreadId();

   if (flags & EIF_MULTIPLE_INSTANCES_OR_THREADS)
   {
      if ((argThreadId != 0) && (argThreadId != actualThreadId))
      {
         // This function will generate a specific EID if appropriate.  But
         // some existing code may pass in the threadId, so make sure it is
         // correct.
         cmsLog_error("Only the generic EID should be used, eid=0x%x", eid);
         return CMSRET_INVALID_ARGUMENTS;
      }
   }
   else
   {
      if (argThreadId != 0)
      {
         cmsLog_error("This is a single instance/thread app, so no threadId should be passed in");
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   if ((handle = (CmsMsgHandle *) cmsMem_alloc(sizeof(CmsMsgHandle), ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate storage for msg handle");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* store caller's eid in the msgHandle */
   if (flags & EIF_MULTIPLE_INSTANCES_OR_THREADS)
   {
      /* this entity can have multiple instances or threads.
       * Set handle->eid to the specific eid.
       * Note that sysUtl_getThreadId returns the caller's thread id. In a
       * single-threaded process, the thread id is equal to the
       * process id as returned by getpid. In a multithreaded process,
       * all threads have the same pid, but each one has a unique threadId.
       */
      handle->eid = MAKE_SPECIFIC_EID(actualThreadId, eid);
   }
   else
   {
      /* this is a single instance, single thread entity.
       * set handle->eid to the generic eid.
       */
      handle->eid = eid;
   }

   strncpy(handle->busName, busName, sizeof(handle->busName)-1);

#ifdef DESKTOP_LINUX
   /*
    * Applications may be run without smd on desktop linux, so if we
    * don't see a socket for smd, don't bother connecting to it.
    * Hmm, is this feature actually used? (for unittests?)  Delete?
    */
   {
      struct stat statbuf;

      if ((rc = stat(handle->busName, &statbuf)) < 0)
      {
         handle->commFd = CMS_INVALID_FD;
         handle->standalone = TRUE;
         *msgHandle = (void *) handle;
         cmsLog_notice("no smd server socket detected, running in standalone mode.");
         return CMSRET_SUCCESS;
      }
   }
#endif /* DESKTOP_LINUX */


      /*
       * Create a unix domain socket.
       */
      handle->commFd = socket(AF_LOCAL, SOCK_STREAM, 0);
      if (handle->commFd < 0)
      {
         cmsLog_error("Could not create socket");
         cmsMem_free(handle);
         return CMSRET_INTERNAL_ERROR;
      }


      /*
       * Set close-on-exec, even though all apps should close their
       * fd's before fork and exec.
       */
      if ((rc = fcntl(handle->commFd, F_SETFD, FD_CLOEXEC)) != 0)
      {
         cmsLog_error("set close-on-exec failed, rc=%d errno=%d", rc, errno);
         close(handle->commFd);
         cmsMem_free(handle);
         return CMSRET_INTERNAL_ERROR;
      }


      /*
       * Connect to the message router.  In CMS classic, this is smd.  In BDK,
       * this will be the bcm_msgd for this component.
       */
      memset(&serverAddr, 0, sizeof(serverAddr));
      serverAddr.sun_family = AF_LOCAL;
      strncpy(serverAddr.sun_path, handle->busName, sizeof(serverAddr.sun_path)-1);
      serverAddr.sun_path[sizeof(serverAddr.sun_path)-1] = '\0';

      rc = connect(handle->commFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
      if (rc != 0)
      {
         // in BDK, the MD's will start bcm_msgd and immediately try to connect
         // to it.  The first attempt may fail, but the MD's will retry, so
         // don't complain too loudly here.
         cmsLog_debug("connect to %s failed, rc=%d errno=%d", handle->busName, rc, errno);
         close(handle->commFd);
         cmsMem_free(handle);
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("commFd=%d connected to smd", handle->commFd);
      }

      /* send a launched message to smd */
      {
         CmsRet ret;
         CmsMsgHeader launchMsg = EMPTY_MSG_HEADER;

         launchMsg.type = CMS_MSG_APP_LAUNCHED;
         launchMsg.src = handle->eid;
         launchMsg.dst = EID_SMD;
         launchMsg.flags_event = 1;
         launchMsg.wordData = getpid();

         if ((ret = oalMsg_send(handle->commFd, &launchMsg)) != CMSRET_SUCCESS)
         {
            close(handle->commFd);
            cmsMem_free(handle);
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("sent LAUNCHED message to smd");
         }
      }

   /* successful, set handle pointer */
   *msgHandle = (void *) handle;

   return CMSRET_SUCCESS;
}


void oalMsg_cleanup(void **msgHandle)
{
   CmsMsgHandle *handle = (CmsMsgHandle *) *msgHandle;

   if (handle->commFd != CMS_INVALID_FD)
   {
      close(handle->commFd);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR((*msgHandle));

   return;
}


CmsRet oalMsg_getEventHandle(const CmsMsgHandle *msgHandle, void *eventHandle)
{
   SINT32 *fdPtr = (SINT32 *) eventHandle;

   *fdPtr = msgHandle->commFd;

   return CMSRET_SUCCESS;
}


CmsRet oalMsg_send(SINT32 fd, const CmsMsgHeader *buf)
{
   UINT32 totalLen;
   SINT32 rc;
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 totalWriteSoFar = 0;
   SINT32 totalRemaining = 0;
   char *outBuf = (char *)buf;

   totalLen = sizeof(CmsMsgHeader) + buf->dataLength;
   totalRemaining = (SINT32)totalLen;

   while (totalWriteSoFar < totalLen)
   {
      rc = write(fd, outBuf, totalRemaining);
      if (rc < 0)
      {
         if (errno == EPIPE)
         {
            /*
             * This could happen when smd tries to write to an app that
             * has exited.  Don't print out a scary error message.
             * Just return an error code and let upper layer app handle it.
             */
            cmsLog_debug("got EPIPE, dest app is dead");
            return CMSRET_DISCONNECTED;
         }
         else if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
         {
            /* handling the error "Resource temporarily unavailable" */
            cmsLog_debug("got EAGAIN or EWOULDBLOCK, will retry!!!");
            if ((ret = waitForReadWriteAvailable(fd, 1, 5000)) != CMSRET_SUCCESS)
            {
               cmsLog_error("timeout!!!");
               return ret;
            }
            continue;
         }
         else
         {
            cmsLog_error("write failed, errno=%d", errno);
            return CMSRET_INTERNAL_ERROR;
         }
      }
      else
      {
         outBuf += rc;
         totalWriteSoFar += rc;
         totalRemaining -= rc;
      }
   }

   return ret;
}


static CmsRet waitForReadWriteAvailable(SINT32 fd, int mode, UINT32 timeout)
{
   struct timeval tv;
   fd_set fds;
   SINT32 rc;

   FD_ZERO(&fds);
   FD_SET(fd, &fds);

   tv.tv_sec = timeout / MSECS_IN_SEC;
   tv.tv_usec = (timeout % MSECS_IN_SEC) * USECS_IN_MSEC;

   if (mode == 0) // read
   {
      rc = select(fd+1, &fds, NULL, NULL, &tv);
   }
   else // write
   {
      rc = select(fd+1, NULL, &fds, NULL, &tv);
   }
   if ((rc == 1) && (FD_ISSET(fd, &fds)))
   {
      return CMSRET_SUCCESS;
   }
   else
   {
      return CMSRET_TIMED_OUT;
   }
}


CmsRet oalMsg_receive(SINT32 fd, CmsMsgHeader **buf, UINT32 *timeout)
{
   CmsMsgHeader *msg;
   SINT32 rc;
   CmsRet ret;

   if (buf == NULL)
   {
      cmsLog_error("buf is NULL!");
      return CMSRET_INVALID_ARGUMENTS;
   }
   else
   {
      *buf = NULL;
   }

   if (timeout)
   {
      if ((ret = waitForReadWriteAvailable(fd, 0, *timeout)) != CMSRET_SUCCESS)
      {
         return ret;
      }
   }

   /*
    * Read just the header in the first read.
    * Do not try to read more because we might get part of 
    * another message in the TCP socket.
    */
   msg = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
   if (msg == NULL)
   {
      cmsLog_error("alloc of msg header failed");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   rc = read(fd, msg, sizeof(CmsMsgHeader));
   if ((rc == 0) ||
       ((rc == -1) && ((errno == 131) || (errno == ECONNRESET))))  /* new 2.6.21 kernel seems to give us this before rc==0 */
   {
      /* broken connection */
      cmsMem_free(msg);
      return CMSRET_DISCONNECTED;
   }
   else if (rc < 0 || rc != sizeof(CmsMsgHeader))
   {
      if (!bcmUtl_isShutdownInProgress())
      {
         cmsLog_error("bad read, rc=%d errno=%d", rc, errno);
      }
      cmsMem_free(msg);
      return CMSRET_INTERNAL_ERROR;
   }

   if (msg->dataLength > 0)
   {
      UINT32 totalReadSoFar=0;
      SINT32 totalRemaining=msg->dataLength;
      char *inBuf;
      CmsMsgHeader *newMsg;

      /* there is additional data in the message */
      newMsg = (CmsMsgHeader *) cmsMem_realloc(msg, sizeof(CmsMsgHeader) + msg->dataLength);
      if (newMsg == NULL)
      {
         cmsLog_error("realloc to %d bytes failed", sizeof(CmsMsgHeader) + msg->dataLength);
         cmsMem_free(msg);
         return CMSRET_RESOURCE_EXCEEDED;
      }

      /* orig msg was freed by cmsMem_realloc, so now msg can point to newMsg */
      msg = newMsg;
      newMsg = NULL;

      inBuf = (char *) (msg + 1);
      while (totalReadSoFar < msg->dataLength)
      {
         cmsLog_debug("reading segment: soFar=%u total=%d", totalReadSoFar, totalRemaining);
         if (timeout)
         {
            if ((ret = waitForReadWriteAvailable(fd, 0, *timeout)) != CMSRET_SUCCESS)
            {
               cmsMem_free(msg);
               return ret;
            }
         }

         /* coverity[tainted_data_argument][overflow_assign][overflow_sink] */
         /* coverity[tainted_data][remediation] */
         rc = read(fd, inBuf, totalRemaining);
         if (rc <= 0)
         {
            if (!bcmUtl_isShutdownInProgress())
            {
               cmsLog_error("bad data read, rc=%d errno=%d readSoFar=%d remaining=%d", rc, errno, totalReadSoFar, totalRemaining);
            }
            cmsMem_free(msg);
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            inBuf += rc;
            totalReadSoFar += rc;
            totalRemaining -= rc;
         }
      }
   }

   *buf = msg;

   return CMSRET_SUCCESS;
}


SINT32 oalMsg_initUnixDomainServerSocket(const char *msgBusName, SINT32 backlog)
{
   struct sockaddr_un serverAddr;
   SINT32 fd, rc;

   // Delete sockName in case previous instance exited without cleanup.
   // TODO: small danger another one is actually running.  Quick check? 
   unlink(msgBusName);

   if ((fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
   {
      bcmuLog_error("Could not create socket");
      return fd;
   }

   memset(&serverAddr, 0, sizeof(serverAddr));
   serverAddr.sun_family = AF_LOCAL;
   strncpy(serverAddr.sun_path, msgBusName, sizeof(serverAddr.sun_path)-1);
   serverAddr.sun_path[sizeof(serverAddr.sun_path)-1] = '\0';

   rc = bind(fd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
   if (rc != 0)
   {
      bcmuLog_error("bind to %s failed, rc=%d errno=%d", msgBusName, rc, errno);
      close(fd);
      return -1;
   }

   rc = listen(fd, backlog);
   if (rc != 0)
   {
      bcmuLog_error("listen to %s failed, rc=%d errno=%d", msgBusName, rc, errno);
      close(fd);
      return -1;
   }

   bcmuLog_notice("message bus socket (%s) opened (fd=%d)", msgBusName, fd);

   return fd;
}
