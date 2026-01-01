/***********************************************************************
 *
 * <:copyright-BRCM:2022:DUAL/GPL:standard
 *
 *    Copyright (c) 2022 Broadcom
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

#include "wanconf.h"
#include <net_port.h>
#include "board.h"   // wan type/rate string constants
#include <string.h>

#define MATCH_VALUE(_buf, _value) !strncasecmp(_buf, _value, strlen(_value))

#if defined(BRCM_CMS_BUILD)
#include <cms.h>
#include <cms_mem.h>
#include <cms_msg.h>

static void gbe_send_post_mdm_msg(void *msgHandle)
{
    char buf[sizeof(CmsMsgHeader) + sizeof(MdmPostActNodeInfo)] = {0};
    CmsRet cmsReturn;
    CmsMsgHeader *msgHdr = (CmsMsgHeader *)buf;
    MdmPostActNodeInfo *msgBody = (MdmPostActNodeInfo *)(msgHdr + 1);

    msgHdr->dst = EID_SSK;
    msgHdr->src = EID_WANCONF;
    msgHdr->type = CMS_MSG_MDM_POST_ACTIVATING;
    msgHdr->flags_event = 1;
    msgHdr->dataLength = sizeof(MdmPostActNodeInfo);
    msgBody->subType = MDM_POST_ACT_TYPE_FILTER;

    /* Attempt to send CMS response message & test result. */
    cmsReturn = cmsMsg_send(msgHandle, msgHdr);
    if (CMSRET_SUCCESS != cmsReturn)
    {
        wc_log_err("Send message failure, cmsResult: %d\n", cmsReturn);
    }
    else
    {
        wc_log("Sent Wanconf App Indication to SSK\n");
    }
}

static int send_port_state(const char *if_name, int is_enabled)
{
    char buf[sizeof(CmsMsgHeader) + IFNAMESIZ]={0};
    CmsMsgHeader *msg=(CmsMsgHeader *) buf;
    char *msg_ifname = (char *)(msg+1);
    CmsRet ret;
    void *msgHandle;

    if (strlen(if_name) > IFNAMESIZ -1)
        return -1;

    ret = cmsMsg_initWithFlags(EID_WANCONF, 0, &msgHandle);
    if (ret)
        return ret;

    msg->type = (is_enabled)? CMS_MSG_WAN_PORT_ENABLE : CMS_MSG_WAN_LINK_DOWN;
    msg->src = EID_WANCONF;
    msg->dst = EID_SSK;
    msg->flags_event = 1;
    msg->dataLength = IFNAMESIZ;

    strcpy(msg_ifname, if_name);

    if (CMSRET_SUCCESS != (ret = cmsMsg_send(msgHandle, msg)))
    {
        cmsMsg_cleanup(&msgHandle);
        wc_log_err("could not send out CMS_MSG_WAN_PORT_ENABLE, ret=%d\n", ret);
        return -1;
    }

    gbe_send_post_mdm_msg(msgHandle);
    cmsMsg_cleanup(&msgHandle);

    return 0;
}

static CmsRet send_wan_op_state_msg(void *msgHandle, WanConfPhyType phyType, UBOOL8 opState,
    const char *ifname, const char *wan_type, const char *wan_rate)
{
    CmsRet ret = CMSRET_SUCCESS;
    char buf[sizeof(CmsMsgHeader) + sizeof(WanConfPhyOpStateMsgBody)] = {0};
    CmsMsgHeader *msg = (CmsMsgHeader*)buf;
    WanConfPhyOpStateMsgBody *info;

    msg->type = CMS_MSG_WAN_PORT_SET_OPSTATE;
    msg->src = EID_WANCONF;
    msg->dst = EID_SSK;
    msg->flags_request = 0;
    msg->flags_response = 0;
    msg->flags_event = 1;
    msg->dataLength = sizeof(WanConfPhyOpStateMsgBody);
    msg->wordData = 0;

    info = (WanConfPhyOpStateMsgBody*)&(buf[sizeof(CmsMsgHeader)]);
    info->phyType = phyType;
    info->opState = opState;


    if (info->opState)
    {
        strncpy(info->wanType, wan_type, sizeof(info->wanType)-1);
        strncpy(info->wanRate, wan_rate, sizeof(info->wanRate)-1);
        if (ifname)
        {
            strncpy(info->ifName,  ifname, sizeof(info->ifName)-1);   
        }
    }
    else
    {
        memset(info->wanType, 0, sizeof(info->wanType));
        memset(info->wanRate, 0, sizeof(info->wanRate));
        memset(info->ifName,  0, sizeof(info->ifName));   
    }

    ret = cmsMsg_send(msgHandle, msg);
    if (CMSRET_SUCCESS != ret)
    {
        wc_log_err("cmsMsg_send(CMS_MSG_WAN_PORT_SET_OPSTATE) failed, ret=%d\n", ret);
    }

    return ret;
}

static CmsRet smd_start_stop_app(UINT32 wordData, 
    int start, const char *ifname,
    const char *wan_type, const char *wan_rate)
{
    void *msgBuf;
    CmsRet ret;
    CmsMsgHeader *msg;
    void *msgHandle;
    int pid;

    ret = cmsMsg_initWithFlags(EID_WANCONF, 0, &msgHandle);
    if (ret)
    {
        wc_log_err("message init failed ret=%d\n", ret);
        return ret;
    }

    msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
    if (msgBuf == NULL)
    {
        wc_log_err("message allocation failed\n");
        return CMSRET_INTERNAL_ERROR;
    }

    msg = (CmsMsgHeader *)msgBuf;
    msg->src = EID_WANCONF;
    msg->dst = EID_SMD;
    msg->flags_event = FALSE;
    msg->type = (start)? CMS_MSG_START_APP : CMS_MSG_STOP_APP; // CMS_MSG_STOP_APP;
    msg->wordData = wordData;
    msg->dataLength = 0;

    pid = (int)cmsMsg_sendAndGetReply(msgHandle, msg);
    if (start && (pid == CMS_INVALID_PID))
    {
        CMSMEM_FREE_BUF_AND_NULL_PTR(msgBuf);
        cmsMsg_cleanup(&msgHandle);
        wc_log_err("Failed to send message to application pid=0x%x\n", pid);
        return CMSRET_INTERNAL_ERROR;
    }

    if (wordData == EID_EPON_APP)
    {
        ret = send_wan_op_state_msg(msgHandle, WANCONF_PHY_TYPE_EPON, 
            (start)? TRUE: FALSE, ifname, wan_type, wan_rate);
    }
    else if (wordData == EID_OMCID)
    {
        ret = send_wan_op_state_msg(msgHandle, WANCONF_PHY_TYPE_GPON, 
            (start)? TRUE: FALSE, ifname, wan_type, wan_rate);
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(msgBuf);
    cmsMsg_cleanup(&msgHandle);

    return ret;
}

static CmsRet start_stop_gbe_port(const char *ifname, const char *wan_type, 
    const char *wan_rate, int start)
{
    CmsRet ret;
    void *msgHandle;

    ret = cmsMsg_initWithFlags(EID_WANCONF, 0, &msgHandle);
    if (ret)
    {
        wc_log_err("message init failed ret=%d\n", ret);
        return ret;
    }

   ret = send_wan_op_state_msg(msgHandle, WANCONF_PHY_TYPE_AE, 
        (start)? TRUE: FALSE, ifname, wan_type, wan_rate);

    cmsMsg_cleanup(&msgHandle);

    sleep(1);

    send_port_state(ifname, start);


    return ret; 
}
#endif /* BRCM_CMS_BUILD */

#if defined(RDK_BUILD)
#include <stdlib.h>
static void write_rdk_wan_type_indication(int port, int is_create)
{
    char cmd[128] = {0};
    int ret;

    switch (port)
    {
        case NET_PORT_GPON:
            snprintf(cmd, sizeof(cmd), "%s /tmp/gpon_detected", is_create?"touch":"rm -f");
            break;
        case NET_PORT_EPON:
            snprintf(cmd, sizeof(cmd), "%s /tmp/epon_detected", is_create?"touch":"rm -f");
            break;
        case NET_PORT_AE:
            break;
    }

    if ((port == NET_PORT_GPON) || (port == NET_PORT_EPON))
    {
        ret = system(cmd);
        if (ret)
            wc_log_err("system command(%s) error\n", cmd);
    }

    return;
}
#endif

static int start_gpon_infra(
    const char *ifname __attribute__((unused)),
    const char *wan_type __attribute__((unused)),
    const char *wan_rate __attribute__((unused)))
{
#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS)
    char buf[32];
    int rc;
#endif

#if defined(BRCM_CMS_BUILD)
    if (smd_start_stop_app(EID_OMCID, 1, ifname, wan_type, wan_rate) != CMSRET_SUCCESS)
    {
        wc_log_err("Failed to start omcid app\n");
        return -1;
    }
#endif /* BRCM_CMS_BUILD */

#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS)
    rc = snprintf(buf, sizeof(buf), "%s %s %s", APP_START, wan_type, wan_rate);
    if (rc < 0)
    {
        return -1;
    }
    zbus_out_wanConf("gpon", "GPON", buf);
#if defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
    rc = snprintf(buf, sizeof(buf), "%s %s %s", WAN_UP, wan_type, wan_rate);
    if (rc < 0)
    {
        return -1;
    }
    zbus_out_wanConf("sysmgmt_nb", "GPON", buf);
#endif /* SUPPORT_BDK_SYSTEM_MANAGEMENT */
#endif /* BRCM_BDK_BUILD & SUPPORT_ZBUS */

    return 0;
}

static int stop_gpon_infra(void)
{
#if defined(BRCM_CMS_BUILD)
    if (smd_start_stop_app(EID_OMCID, 0, NULL, NULL, NULL) != CMSRET_SUCCESS)
    {
        wc_log_err("Failed to stop omcid app\n");
        return -1;
    }
#endif /* BRCM_CMS_BUILD */

#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS)
    zbus_out_wanConf("gpon", "GPON", APP_STOP);
#if defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
    zbus_out_wanConf("sysmgmt_nb", "GPON", WAN_DOWN);
#endif /* SUPPORT_BDK_SYSTEM_MANAGEMENT */
#endif /* BRCM_BDK_BUILD & SUPPORT_ZBUS */
    wc_log("OMCI daemon stopped\n");

    return 0;
}

static int start_epon_infra(
    char *ifname __attribute__((unused)),
    char *wan_type __attribute__((unused)),
    char *wan_rate __attribute__((unused)))
{
#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS)
    char buf[32];
    int rc;
#endif

#if defined(BRCM_CMS_BUILD)
    if (smd_start_stop_app(EID_EPON_APP,1, ifname, wan_type, wan_rate) != CMSRET_SUCCESS)
    {
        wc_log_err("Failed to start eponapp\n");
        return -1;
    }
#endif /* BRCM_CMS_BUILD */

#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS)
    rc = snprintf(buf, sizeof(buf), "%5s %5s %5s", APP_START, wan_type, wan_rate);
    if (rc < 0)
    {
        return -1;
    }
    zbus_out_wanConf("epon", "EPON", buf);
#if defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
    rc = snprintf(buf, sizeof(buf), "%s %s %s", WAN_UP, wan_type, wan_rate);
    if (rc < 0)
    {
        return -1;
    }
    zbus_out_wanConf("sysmgmt_nb", "EPON", buf);
#endif /* SUPPORT_BDK_SYSTEM_MANAGEMENT */
#endif /* BRCM_BDK_BUILD & SUPPORT_ZBUS */

    return 0;
}

static int stop_epon_infra(void)
{
    wc_log("Stopping EPON app\n");
#if defined(BRCM_CMS_BUILD)
    if (smd_start_stop_app(EID_EPON_APP, 0, NULL, NULL, NULL) != CMSRET_SUCCESS)
    {
        wc_log_err("Failed to stop epon app\n");
        return -1;
    }
#endif /* BRCM_CMS_BUILD */

#if defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
    zbus_out_wanConf("sysmgmt_nb", "EPON", WAN_DOWN);
#endif /* SUPPORT_BDK_SYSTEM_MANAGEMENT */
#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS)
    zbus_out_wanConf("epon", "EPON", APP_STOP);
#endif /* BRCM_BDK_BUILD & SUPPORT_ZBUS */
    wc_log("EPON app stopped\n");

    return 0;
}

static void wan_interface_notify(int is_create __attribute__((unused)),
  char *ifname __attribute__((unused)), char *wan_type __attribute__((unused)),
  char *wan_rate __attribute__((unused)))
{
#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS) && defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
    char buf[32];
    int rc;
#endif

    /* Signal CMS LAN interface changed to WAN */
    if (is_create)
    {
#if defined(BRCM_CMS_BUILD)
        start_stop_gbe_port(ifname, wan_type, wan_rate, 1);
#endif
#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS) && defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
        rc = snprintf(buf, sizeof(buf), "%5s %5s %5s", APP_START, wan_type, wan_rate);
        if (rc < 0)
        {
            return;
        }
        zbus_out_wanConf("sysmgmt_nb", ifname, buf);
#endif
    }
    /* Signals interface going down */
    else
    {
#if defined(BRCM_CMS_BUILD)
        start_stop_gbe_port(ifname, wan_type, wan_rate, 0);
#endif

#if defined(BRCM_BDK_BUILD) && defined(SUPPORT_ZBUS) && defined(SUPPORT_BDK_SYSTEM_MANAGEMENT)
        zbus_out_wanConf("sysmgmt_nb", ifname, "DOWN");
#endif
    }
}

static void get_wan_rate_str(char *buf, struct net_port_t *net_port)
{
    switch(net_port->speed)
    {
        case NET_PORT_SPEED_1010:
            strcpy(buf, RDPA_WAN_RATE_10G RDPA_WAN_RATE_10G);
        break;

        case NET_PORT_SPEED_1025:
            strcpy(buf, RDPA_WAN_RATE_10G RDPA_WAN_RATE_2_5G);
        break;

        case NET_PORT_SPEED_1001:
            strcpy(buf, RDPA_WAN_RATE_10G RDPA_WAN_RATE_1G);
        break;

        case NET_PORT_SPEED_0202:
            strcpy(buf, RDPA_WAN_RATE_2G RDPA_WAN_RATE_2G);
        break;

        case NET_PORT_SPEED_0201:
            strcpy(buf, RDPA_WAN_RATE_2G RDPA_WAN_RATE_1G);
        break;

        case NET_PORT_SPEED_0101:
            strcpy(buf, RDPA_WAN_RATE_1G RDPA_WAN_RATE_1G);
        break;

        case NET_PORT_SPEED_2501:
            strcpy(buf, RDPA_WAN_RATE_2_5G RDPA_WAN_RATE_1G);
        break;

        default:
            wc_log_err("No valid WAN rate is set in net_port->speed = %d\n", (int) net_port->speed);
    }
}

static void get_wan_type_str(char *buf, struct net_port_t *net_port)
{

    switch (net_port->port)
    {
        case NET_PORT_NONE:
            strcpy(buf, RDPA_WAN_TYPE_VALUE_NONE);
        break;

        case NET_PORT_EPON:
             strcpy(buf, RDPA_WAN_TYPE_VALUE_EPON);
        break;

        case NET_PORT_GPON:
            switch (net_port->sub_type)
            {
                case NET_PORT_SUBTYPE_GPON:
                    strcpy(buf, RDPA_WAN_TYPE_VALUE_GPON);
                break;

                case NET_PORT_SUBTYPE_XGS:
                    strcpy(buf, RDPA_WAN_TYPE_VALUE_XGS);
                break;

                case NET_PORT_SUBTYPE_XGPON:
                    strcpy(buf, RDPA_WAN_TYPE_VALUE_XGPON1);
                break;

                case NET_PORT_SUBTYPE_NGPON:
                    strcpy(buf, RDPA_WAN_TYPE_VALUE_NGPON2);
                break;

                default:
                    strcpy(buf, RDPA_WAN_TYPE_VALUE_NONE);
                    wc_log_err("No valid WAN type in net_port->sub_type=%d\n", (int)net_port->sub_type);

            }
        break;

        case NET_PORT_AE:
            strcpy(buf, RDPA_WAN_TYPE_VALUE_GBE);
        break;

        default:
            strcpy(buf, RDPA_WAN_TYPE_VALUE_NONE);
            wc_log_err("No valid WAN type in net_port->port=%d\n", (int)net_port->port);
    }
}

static void wan_type_wan_rate_fixup(char *wan_type, char *wan_rate)
{
    if (MATCH_VALUE(wan_type, RDPA_WAN_TYPE_VALUE_GPON))
    {
        strcpy(wan_rate, RDPA_WAN_RATE_1G RDPA_WAN_RATE_1G);
    }
    else if (MATCH_VALUE(wan_type, RDPA_WAN_TYPE_VALUE_XGS))
    {
        strcpy(wan_rate, RDPA_WAN_RATE_10G RDPA_WAN_RATE_10G);
    }
}


void wanconf_brcm_notify(struct net_port_t *net_port, int is_create)
{
    char wan_type[16];
    char wan_rate[16];

    if (is_create)
    {
        get_wan_type_str(wan_type, net_port);
        get_wan_rate_str(wan_rate, net_port);
        wan_type_wan_rate_fixup(wan_type, wan_rate);

        switch (net_port->port)
        {
            case NET_PORT_GPON:
                if (start_gpon_infra(net_port->ifname, wan_type, wan_rate))
                    return;
                break;
            case NET_PORT_EPON:
                if (start_epon_infra(net_port->ifname, wan_type, wan_rate))
                    return;
                break;
            case NET_PORT_AE:
                wan_interface_notify(is_create, net_port->ifname, wan_type, wan_rate);
                break;
        }
    }
    else
    {
        switch (net_port->port)
        {
            case NET_PORT_GPON:
                stop_gpon_infra();
                break;
            case NET_PORT_EPON:
                stop_epon_infra();
                break;
            case NET_PORT_AE:
                wan_interface_notify(is_create, net_port->ifname, NULL, NULL);
                break;
        }
    }

#if defined(RDK_BUILD)
    write_rdk_wan_type_indication(net_port->port, is_create);
#endif

}

