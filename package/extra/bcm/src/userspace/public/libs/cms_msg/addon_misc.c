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

/*!\file addon_misc.c
 * \brief Various helper functions that were in cms_util/image.c but now
 *        moved here so that cms_util will not have a hard dependency on
 *        cms_msg.
 *  
 */

#include "cms.h"
#include "cms_eid.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_msg_modsw.h"

void cmsImg_sendAutonomousTransferCompleteMsg(void *msgHandle, const CmsImageTransferStats *pStats)
{
   char buf[sizeof(CmsMsgHeader) + sizeof(AutonomousTransferCompleteMsgBody)]={0};
   CmsMsgHeader *msg = (CmsMsgHeader *) buf; 
   AutonomousTransferCompleteMsgBody *body = (AutonomousTransferCompleteMsgBody*)(msg+1);

   msg->type = CMS_MSG_AUTONOMOUS_TRANSFER_COMPLETE;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_TR69C;
   msg->flags_event = 1;
   msg->dataLength = sizeof(AutonomousTransferCompleteMsgBody);

   body->isDownload = pStats->isDownload;
   body->fileType = pStats->fileType;
   body->fileSize = pStats->fileSize;
   body->faultCode = pStats->faultCode;
   strncpy(body->faultStr, pStats->faultStr, sizeof(body->faultStr)-1);
   body->startTime = pStats->startTime;
   body->completeTime = pStats->completeTime;

   if (cmsMsg_send(msgHandle, msg) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsImg_sendAutonomousTransferCompleteMsg : cmsMsg_send failed.");
   }

   return;
}

