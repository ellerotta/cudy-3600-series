/***********************************************************************
 *
 * <:copyright-BRCM:2015:DUAL/GPL:standard
 *
 *    Copyright (c) 2015 Broadcom
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
#include "board.h"
#include "cms_psp.h"
#include <net_port.h>

#define GET_SCRATCHPAD_VALUE_AND_LENGTH(scratchpad_parameter, local_buffer, length_in_bytes) do     \
    {                                                                                               \
        length_in_bytes = cmsPsp_get(scratchpad_parameter, local_buffer, sizeof(local_buffer) - 1); \
        if (0 >= length_in_bytes)                                                                   \
        {                                                                                           \
            return -1;                                                                  \
        }                                                                                           \
        local_buffer[length_in_bytes] = 0;                                                          \
    } while (0)

#define MATCH_VALUE(_buf, _value) !strncasecmp(_buf, _value, strlen(_value))


static int get_scratchpad_wan_rate(struct net_port_t *net_port)
{
    char buf[16];
    int count;

    count = cmsPsp_get(RDPA_WAN_RATE_PSP_KEY, buf, sizeof(buf) - 1);
    if (0 >= count)
    {
        net_port->speed = NET_PORT_SPEED_NONE;
        wc_log("port speed is set to none\n");
        return 0;
    }

    if (MATCH_VALUE(buf, RDPA_WAN_RATE_10G RDPA_WAN_RATE_10G))
        net_port->speed = NET_PORT_SPEED_1010;
    else if (MATCH_VALUE(buf, RDPA_WAN_RATE_10G RDPA_WAN_RATE_2_5G))
        net_port->speed = NET_PORT_SPEED_1025;
    else if (MATCH_VALUE(buf, RDPA_WAN_RATE_10G RDPA_WAN_RATE_1G))
        net_port->speed = NET_PORT_SPEED_1001;
    else if (MATCH_VALUE(buf, RDPA_WAN_RATE_2G RDPA_WAN_RATE_2G))
        net_port->speed = NET_PORT_SPEED_0202;
    else if (MATCH_VALUE(buf, RDPA_WAN_RATE_2G RDPA_WAN_RATE_1G))
        net_port->speed = NET_PORT_SPEED_0201;
    else if (MATCH_VALUE(buf, RDPA_WAN_RATE_1G RDPA_WAN_RATE_1G))
        net_port->speed = NET_PORT_SPEED_0101;
    else if (MATCH_VALUE(buf, RDPA_WAN_RATE_2_5G RDPA_WAN_RATE_1G))
        net_port->speed = NET_PORT_SPEED_2501;

    return 0;
}

static int get_scratchpad_wan_type(struct net_port_t *net_port)
{
    char buf[16];
    int count;

    GET_SCRATCHPAD_VALUE_AND_LENGTH(RDPA_WAN_TYPE_PSP_KEY, buf, count);

    if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_NONE))
    {
        net_port->port = NET_PORT_NONE;
    }
    else if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_EPON))
    {
        net_port->port = NET_PORT_EPON;
    }
    else if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_GPON))
    {
        net_port->port = NET_PORT_GPON;
        net_port->sub_type = NET_PORT_SUBTYPE_GPON;
    }
    else if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_XGS))
    {
        net_port->port = NET_PORT_GPON;
        net_port->sub_type = NET_PORT_SUBTYPE_XGS;
    }
    else if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_XGPON1))
    {
        net_port->port = NET_PORT_GPON;
        net_port->sub_type= NET_PORT_SUBTYPE_XGPON;
    }
    else if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_NGPON2))
    {
        net_port->port = NET_PORT_GPON;
        net_port->sub_type = NET_PORT_SUBTYPE_NGPON;
    }
    else if (MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_GBE))
    {
        net_port->port = NET_PORT_AE;
    }
    else
    {
        wc_log_err("No valid WAN type is set in scratchpad %s = %s\n", RDPA_WAN_TYPE_PSP_KEY, buf);
        return -1;
    }

    return 0;
}

static int psp_to_net_port(struct net_port_t *net_port)
{
    net_port->is_wan = 1;

    if (get_scratchpad_wan_type(net_port))
        return -1;

    if (net_port->port == NET_PORT_NONE)
        return 0;

    get_scratchpad_wan_rate(net_port);

    return 0;
}

static int psp_wan_type_match_auto(char *buf)
{
    return MATCH_VALUE(buf, RDPA_WAN_TYPE_VALUE_AUTO);
}

static int psp_set(char *key, char *buf)
{
    int ret;

    if ((ret = cmsPsp_set(key, buf, strlen(buf))))
    {
        wc_log("cmsPsp_set: unable to set %s in %s in scratchpad (ret=%d)\n", buf, key, ret);
        return -1;
    }

    return 0;
}

#define psp_wan_type_set(buf) psp_set(RDPA_WAN_TYPE_PSP_KEY, buf)
#define psp_wan_rate_set(buf) psp_set(RDPA_WAN_RATE_PSP_KEY, buf)

static int psp_wan_type_get(char *buf, int count)
{
    int ret;

    ret = cmsPsp_get(RDPA_WAN_TYPE_PSP_KEY, buf, count);
    if (ret < 0)
    {
        wc_log("cmsPsp_get: unable to read RDPA_WAN_TYPE_PSP_KEY from scratchpad\n");
        return -1;
    }

    return ret;
}

static int set_default_parameters_to_scratchpad(char *wan_type)
{
    int rc = 0;

    if(wan_type != NULL){
    
        *wan_type = '\0';
    }
#ifdef SUPPORT_DEFAULT_GBE
    if (psp_wan_type_set(RDPA_WAN_TYPE_VALUE_GBE) || psp_wan_rate_set((RDPA_WAN_RATE_1G RDPA_WAN_RATE_1G))){

        wc_log_err("Failed to set default parameters RdpaWanType:%s; WanRate:%s\n",
        RDPA_WAN_TYPE_VALUE_GBE, (RDPA_WAN_RATE_1G RDPA_WAN_RATE_1G));
        rc = -1;
    }else if(wan_type != NULL){
    
        snprintf(wan_type, strlen(RDPA_WAN_TYPE_VALUE_GBE) + 1, "%s", RDPA_WAN_TYPE_VALUE_GBE);
    }
#endif /*#ifdef SUPPORT_DEFAULT_GBE*/
    return rc;
}

static void sanify_net_port_speed(net_port_t *net_port)
{
    if (net_port->port == NET_PORT_GPON)
    {
        switch (net_port->sub_type)
        {
            case NET_PORT_SUBTYPE_GPON:
                net_port->speed = NET_PORT_SPEED_2501;
                break;
            case NET_PORT_SUBTYPE_XGS:
                net_port->speed = NET_PORT_SPEED_1010;
                break;
            case NET_PORT_SUBTYPE_XGPON:
                net_port->speed = NET_PORT_SPEED_1025;
                break;
            default:
                break;
        }
    }
}

int dump_scratchpad(void)
{
    char buf[16];
    int count;

    GET_SCRATCHPAD_VALUE_AND_LENGTH(RDPA_WAN_TYPE_PSP_KEY, buf, count);
    wc_log("%s=%s\n", RDPA_WAN_TYPE_PSP_KEY, buf);
    GET_SCRATCHPAD_VALUE_AND_LENGTH(RDPA_WAN_RATE_PSP_KEY, buf, count);
    wc_log("%s=%s\n", RDPA_WAN_RATE_PSP_KEY, buf);

    return 0;
}

int get_parameters_from_scratchpad(net_port_t *net_port_p, int *is_auto)
{
    char wan_type[16] = {0};
    int ret;

    if (is_auto)
        *is_auto = 0;

    ret = psp_wan_type_get(wan_type, sizeof(wan_type) - 1);
    if (ret < 0)
        return ret;

    if (ret == 0) /* not in scratchpad: init */
        set_default_parameters_to_scratchpad(wan_type);
    
    if(strlen(wan_type) != 0)
        wc_log("%s: RdpaWanType=%s\n", __FUNCTION__, wan_type);

    if (is_auto && (*is_auto = psp_wan_type_match_auto(wan_type)))
        return 0;

    if (psp_to_net_port(net_port_p))
        return -1;

    sanify_net_port_speed(net_port_p);

    wc_log("net_port: port=%d sub_type=%d speed=%d\n", net_port_p->port, net_port_p->sub_type, net_port_p->speed);

    return 0;
}

int set_parameters_to_scratchpad(char *wan_type, char *wan_speed)
{
    if (wan_type && psp_wan_type_set(wan_type))
        return -1;

    if (wan_speed && psp_wan_rate_set(wan_speed))
        return -1;

    return 0;
}

