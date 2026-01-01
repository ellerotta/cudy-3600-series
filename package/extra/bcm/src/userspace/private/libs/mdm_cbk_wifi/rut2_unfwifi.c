/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

#include "odl.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"

/* send a changed notification to destination */
static void sendWlanChange(CmsEntityId dest)
{
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    char buf[sizeof(CmsMsgHeader) + 32]={0};
    CmsMsgHeader *msg=(CmsMsgHeader *) buf;

    msg->dst = dest;
    msg->dataLength = sizeof(buf) - sizeof(CmsMsgHeader);
    msg->type = CMS_MSG_WLAN_CHANGED;
    msg->src = GENERIC_EID(cmsMsg_getHandleEid(msgHandle));
    msg->flags_request = 1;

    snprintf((char *)(msg + 1), msg->dataLength, "Restart");

    if (cmsMsg_send(msgHandle, msg) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not send out CMS_MSG_WLAN_CHANGED event msg.");
    }
    else
    {
        cmsLog_debug("Send out CMS_MSG_WLAN_CHANGED event msg.");
    }
}


void rut2_sendWifiChange(void)
{
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    CmsEntityId eid = GENERIC_EID(cmsMsg_getHandleEid(msgHandle));

    /* Only allow TR69/CWMP RPC or CLI-mdm command.
     * Other process should send SIGUSR2 signal to wlssk to trigger restart
     */
#ifdef BRCM_BDK_BUILD
#ifdef CONFIG_BRCM_OPENWRT
    /* Trigger wifi_md to send out notification to update UCI configuration 
     * currently only allow wlnvram and mdm cli, block others like wlssk to avoid bootup issue
     */
    if ((eid == EID_WLNVRAM) || (eid == EID_WLNVRAM_LIB) || (eid == EID_MDM_CLI))
    {
        sendWlanChange(EID_WIFI_MD);
    }
#endif

    if ((eid != EID_WIFI_MD) &&
        (eid != EID_MDM_CLI))
    {
        return;
    }
#else

    /* CMS build */
    if ((eid != EID_TR69C) &&
        (eid != EID_CONSOLED) &&
        (eid != EID_TELNETD) &&
        (eid != EID_SSHD) &&
        (eid != EID_ECMS))
    {
        return; 
    }
#endif

    /* sending message to trigger wlssk_restart process */
    sendWlanChange(EID_WLSSK);
}
