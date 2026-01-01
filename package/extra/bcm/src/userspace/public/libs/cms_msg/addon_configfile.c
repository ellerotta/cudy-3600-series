/***********************************************************************
 * <:copyright-BRCM:2007:DUAL/GPL:standard
 * 
 *    Copyright (c) 2007 Broadcom 
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
 ************************************************************************/

/*!\file addon_configfile.c
 * \brief Various config file helper functions that were in cms_util/image.c
 *        but now moved here so that cms_util will not have a hard dependency
 *        on cms_msg.  libcms_util will do a dlopen to grab the symbols from
 *        this lib if config file related functions which require cms_msg
 *        are called.
 *  
 */
 
 
#include "cms.h"
#include "cms_msg.h"
#include "cms_eid.h"
#include "cms_util.h"
#include "sysutil_fs.h"


CmsRet cmsImg_sendConfigMsg(const char *imagePtr, UINT32 imageLen,
                            void *msgHandle, int doWrite)
{
   char *buf=NULL;
   char *body=NULL;
   CmsMsgHeader *msg;
   CmsMsgType msgType = (doWrite ? CMS_MSG_WRITE_CONFIG_FILE : CMS_MSG_VALIDATE_CONFIG_FILE);
   CmsRet ret;

   if ((buf = cmsMem_alloc(sizeof(CmsMsgHeader) + imageLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("failed to allocate %d bytes for msg 0x%x", sizeof(CmsMsgHeader) + imageLen, msgType);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   msg = (CmsMsgHeader *) buf;
   body = (char *) (msg + 1);

   msg->type = msgType;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_SMD;

   cmsLog_notice("Entered: msgType=0x%x src=%d imageLen=%d imagePtr=%p msgHandle=%p doWrite=%d",
                 msgType, msg->src, imageLen, imagePtr, msgHandle, doWrite);

   /* In BDK, TR69 and USP components do not have a smd to do
    * config file operations (i.e validate or write).  So send these messages
    * directly to remote_objd.
    */
   if ((sysUtil_isFilePresent(TR69_MSG_BUS) || sysUtil_isFilePresent(USP_MSG_BUS)) &&
       ((msg->src == EID_TR69C) || (msg->src == EID_OBUSPA)  || (msg->src == EID_OBUSPA_DOWNLOAD_THREAD)))
   {
      cmsLog_notice("sending msgType 0x%x directly to remote_objd", msgType);
      msg->dst = EID_REMOTE_OBJD;
   }

   msg->flags_request = 1;
   msg->dataLength = imageLen;

   memcpy(body, imagePtr, imageLen);

   ret = cmsMsg_sendAndGetReply(msgHandle, msg);

   cmsMem_free(buf);

   cmsLog_notice("Exit: ret=%d", ret);
   return ret;
}

CmsImageFormat cmsImg_ConfigFileValidate(UINT8 *imagePtr, UINT32 imageLen,
  void *usrDataP)
{
   CmsRet ret;

   ret = cmsImg_sendConfigMsg((char*)imagePtr, imageLen, usrDataP, 0);
   if (ret == CMSRET_SUCCESS)
   {
      return CMS_IMAGE_FORMAT_XML_CFG;
   }

   return CMS_IMAGE_FORMAT_INVALID;
}


int cmsImg_ConfigFileWrite(UINT8 *imagePtr, UINT32 imageLen, void *usrDataP)
{
   CmsRet ret;

   ret = cmsImg_sendConfigMsg((char*)imagePtr, imageLen, usrDataP, 1);
   if (ret == CMSRET_SUCCESS)
   {
      return 0;
   }

   return -1;
}

