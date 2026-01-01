/***********************************************************************
 *
 * <:copyright-BRCM:2011:DUAL/GPL:standard
 * 
 *    Copyright (c) 2011 Broadcom 
 *    All Rights Reserved
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "cms_msg.h"
#include "cms_mem.h"
#include "bcm_fsutils.h"
#include "cms_log.h"
#include "cms_strconv.h"


static CmsRet send_msg(void);
static void get_response(void);

static void usage(void)
{
   printf("usage: send_cms_msg [-v] [-r|-o|-e] [-c component_name] [-d wordData] [-D additional data] [-t timeout] [-p pid] message dest_eid\n");
   printf("       -D: additional data to add to the body of the CMS message. It must be ascii, and it has to be quoted if including spaces\n");
   exit(-1);
}

int verbose = 0;
int flag_request=1;
int flag_response=0;
int flag_event=0;
int timeout_ms=0;
int pid=-1;
int msg_type;
int wordData=0;
char *additionalData=NULL;
unsigned int dest_eid;
void *msgHandle=NULL;

int main(int argc, char **argv)
{
   int c;
   char *compName=NULL;
   const char *busName=NULL;
   CmsRet ret;

   while ((c = getopt(argc, argv, "c:d:D:vroep:t:")) != -1)
   {
      switch(c) {
      case 'v':
         verbose = 1;
         break;

      case 'r':
         flag_request = 1;
         flag_response = 0;
         flag_event = 0;
         break;

      case 'o':
         flag_request = 0;
         flag_response = 1;
         flag_event = 0;
         break;

      case 'e':
         flag_request = 0;
         flag_response = 0;
         flag_event = 1;
         break;

      case 'p':
         pid = atoi(optarg);
         break;

      case 't':
         timeout_ms = atoi(optarg);
         break;

      case 'c':
         compName = optarg;
         break;

      case 'd':
         wordData = strtoul(optarg, NULL, 0);
         break;

      case 'D':
         additionalData = optarg;
         break;

      default:
         usage();
      }
   }

   if (optind+2 != argc)
   {
      usage();
   }

   if (compName == NULL)
   {
      busName = SMD_MESSAGE_ADDR;
   }
   else
   {
      busName = cmsMsg_componentNameToBusName(compName);
      if (busName == NULL)
      {
         printf("Unsupported or unrecognized busName, compName is %s", compName);
         exit(-1);
      }
   }

   msg_type = strtoul(argv[optind], NULL, 0);

   dest_eid = strtoul(argv[optind+1], NULL, 0);


   if (verbose)
   {
      printf("flag_request=%d\n", flag_request);
      printf("flag_response=%d\n", flag_response);
      printf("flag_event=%d\n", flag_event);
      printf("pid=%d\n", pid);
      printf("timeout_ms=%d\n", timeout_ms);
      printf("msg_type=0x%x\n", msg_type);
      printf("dest_eid=%d\n", dest_eid);
      printf("wordData=%d (0x%x)\n", wordData, wordData);
      printf("busName %s\n", busName);
      printf("additionalData=%s\n", (additionalData) ? additionalData : "NULL");
   }

   // Since we could have multiple instances of the send_cms_msg app running on the same bus at the same time,
   // (rare, but in theory possible), always set EIF_MULTIPLE_INSTANCES.
   ret = cmsMsg_initOnBus(EID_SEND_CMS_MSG, EIF_MULTIPLE_INSTANCES, busName, &msgHandle);
   if (ret != CMSRET_SUCCESS)
   {
      if (!bcmUtl_isShutdownInProgress())
      {
         printf("send_cms_msg: could not initialize CMS msg handle (ret=%d)\n", ret);
      }
      exit(-1);
   }

   ret = send_msg();
   if (ret != CMSRET_SUCCESS)
   {
      if (!bcmUtl_isShutdownInProgress())
      {
         printf("send_cms_msg: send failed (ret=%d)\n", ret);
      }
   }

   if (ret == CMSRET_SUCCESS && flag_request)
   {
      get_response();
   }

   cmsMsg_cleanup(&msgHandle);

   exit(0);
}


CmsRet send_msg(void)
{
   char *buf = NULL;
   char *body = NULL;
   CmsMsgHeader *msg;
   CmsRet ret;
   int additionalDataLen = 0;

   if (additionalData)
   {
      additionalDataLen = cmsUtl_strlen(additionalData) + 1; // include the null terminator
   }

   if ((buf = cmsMem_alloc(sizeof(CmsMsgHeader) + additionalDataLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("failed to allocate %d bytes for msg 0x%x", sizeof(CmsMsgHeader) + additionalDataLen, msg_type);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   msg = (CmsMsgHeader *) buf;
   body = (char *) (msg + 1);

   msg->type = msg_type;
   msg->src = MAKE_SPECIFIC_EID(getpid(), EID_SEND_CMS_MSG);
   msg->dst = (pid == -1) ? dest_eid : MAKE_SPECIFIC_EID(pid, dest_eid);
   msg->flags_request = flag_request;
   msg->flags_response = flag_response;
   msg->flags_event = flag_event;
   msg->wordData = wordData;
   msg->dataLength = additionalDataLen;

   memcpy(body, additionalData, additionalDataLen);

   ret = cmsMsg_send(msgHandle, msg);

   cmsMem_free(buf);

   return ret;
}


void get_response(void)
{
   CmsMsgHeader *msg;
   CmsRet ret;

   if (timeout_ms)
   {
      ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, timeout_ms);
   }
   else
   {
      ret = cmsMsg_receive(msgHandle, &msg);
   }

   if (ret == CMSRET_SUCCESS)
   {
      if (verbose)
      {
         printf("send_cms_msg: received response (msg_type=0x%x wordData=0x%x\n",
                msg->type, msg->wordData);
      }
   }
   else
   {
      if (!bcmUtl_isShutdownInProgress())
      {
         printf("send_cms_msg: receive failed (ret=%d)\n", ret);
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
}


