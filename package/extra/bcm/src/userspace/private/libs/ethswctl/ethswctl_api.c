/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <linux/if.h>
#include <linux/ethtool.h>
#include "bcm/bcmswapitypes.h"
#include "ethswctl_api.h"
#include "ethctl_api.h"
//#include "bcmtypes.h"

#ifdef DESKTOP_LINUX

/* when running on DESKTOP_LINUX, redirect ioctl's to a fake one */
static int fake_ethsw_ioctl(int fd, int cmd, void *data);
#define ETHSW_IOCTL_WRAPPER  fake_ethsw_ioctl

#else

/* When running on actual target, call the real ioctl */
#define ETHSW_IOCTL_WRAPPER  ioctl

#endif
#define BASE_IF_NAME "bcmsw"

#define BIT_IDX_2_BIT32_VAL(bit_idx) (1 << ((bit_idx) % 32))
#define BIT_IDX_2_WORD_IDX(bit_idx) ((bit_idx) / 32)

typedef struct {
    uint32_t *ethtool_speed_type;
    uint32_t link_speed;
    uint32_t count;
}ethtool_link_speed_t;

typedef struct {
    struct ethtool_link_settings req;
    __u32 link_mode_data[3 * __SCHAR_MAX__];
} ecmd_t;

/**
 * @brief 
 * @note this array must be sorted in non-descending order by .link_speed field
 * @see find_link_speed
 */
static ethtool_link_speed_t ethtool_speeds[__TS_LAST] = {
    /*__TS_10_MBPS*/
   {
      .ethtool_speed_type = (uint32_t[]) {
         ETHTOOL_LINK_MODE_10baseT_Half_BIT,
         ETHTOOL_LINK_MODE_10baseT_Full_BIT
      },
      .link_speed = SPEED_10,
      .count = 2
   },
    /*__TS_100_MBPS*/
   {
      .ethtool_speed_type = (uint32_t[]) {
         ETHTOOL_LINK_MODE_100baseT_Half_BIT,
         ETHTOOL_LINK_MODE_100baseT_Full_BIT
      },
      .link_speed = SPEED_100,
      .count = 2
   },
   /*__TS_1000_MBPS*/
   {
      .ethtool_speed_type = (uint32_t[]) {
         ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
         ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
         ETHTOOL_LINK_MODE_1000baseKX_Full_BIT,
         ETHTOOL_LINK_MODE_1000baseX_Full_BIT
      },
      .link_speed = SPEED_1000,
      .count = 4
   },
   /*__TS_2500_MBPS*/
   {
      .ethtool_speed_type = (uint32_t[]) {
         ETHTOOL_LINK_MODE_2500baseX_Full_BIT,
         ETHTOOL_LINK_MODE_2500baseT_Full_BIT
      },
      .link_speed = SPEED_2500,
      .count = 2
   },
   /*__TS_5000_MBPS*/
   {
      .ethtool_speed_type = (uint32_t[]) {
         ETHTOOL_LINK_MODE_5000baseT_Full_BIT
      }, 
      .link_speed = SPEED_5000,
      .count = 1
   },
   /*__TS_10000_MBPS*/
   {
      .ethtool_speed_type = (uint32_t[]) {
         ETHTOOL_LINK_MODE_10000baseKX4_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseKR_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseER_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseLRM_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseLR_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseSR_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseCR_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseT_Full_BIT,
         ETHTOOL_LINK_MODE_10000baseR_FEC_BIT
      },
      .link_speed = SPEED_10000,
      .count = 9
   },  
};


/*! 
\defgroup   ethswctl ethernet switch control API reference
\brief      o command line (CLI) syntax (ethswctl -c \<command\>) and help strings are located in ethswctl.h \n
            o ethswctl operation (OP) enum is defined in bcmswapitypes.h \n
            o ethswctl API functions are located in ethswctl_api.c \n
@{
*/

/* Init the socket to bcmsw interface */
// NOTE: there is another copy of this function in bcm_ethswutils.c.  Need to keep these copies in sync.
static inline int ethswctl_init(struct ifreq *p_ifr)
{
    int skfd;

    /* Open a basic socket */
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("ethswctl_init: socket open error");
        return -1;
    }

#ifndef DESKTOP_LINUX
    /* Get the name -> if_index mapping for ethswctl */
    memset((void *) p_ifr, 0, sizeof(*p_ifr));
    strcpy(p_ifr->ifr_name, BASE_IF_NAME);
    if (ioctl(skfd, SIOCGIFINDEX, p_ifr) < 0 ) {
        strcpy(p_ifr->ifr_name, "eth0");
        if (ioctl(skfd, SIOCGIFINDEX, p_ifr) < 0 ) {
            close(skfd);
            fprintf(stderr, "ethswctl_init: neither bcmsw nor eth0 exist\n");
            return -1;
        }
    }
#endif

    return skfd;
}

/* Init the socket to given p_ifr->ifr_name */
static inline int ethswctl_open_socket(struct ifreq *p_ifr)
{
    int skfd;

    /* Open a basic socket */
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket open error");
        return -1;
    }

    /* Get the name -> if_index mapping for ethswctl */
    if (ETHSW_IOCTL_WRAPPER(skfd, SIOCGIFINDEX, p_ifr) < 0 ) {
        close(skfd);
        fprintf(stderr, "%s interface does not exist \n", p_ifr->ifr_name);
        return -1;
    }

    return skfd;
}

/* setup ifreq common info
 * is_object Indicate if ifname is name of network interface or port object name
 */
static int ethswctl_setup_ifreq(struct ifreq *ifr, struct ethswctl_data *ifdata, 
                                        const char *ifname, int is_object)
{
    int skfd;

    memset(ifdata, 0, sizeof(struct ethswctl_data));
    
    if (is_object)
    {
        strncpy(ifr->ifr_name, BASE_IF_NAME, sizeof(ifr->ifr_name));
        ifr->ifr_name[IFNAMSIZ - 1] = '\0';
        
        strncpy(ifdata->ifname, ifname, sizeof(ifdata->ifname));
        ifdata->ifname[OBJIFNAMSIZ - 1] = '\0';
    }
    else
    {
        strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
        ifr->ifr_name[IFNAMSIZ - 1] = '\0';
        
        ifdata->addressing_flag = ETHSW_ADDRESSING_DEV;
    }
    
    if ((skfd=ethswctl_open_socket(ifr)) < 0)
    {
        fprintf(stderr, "ethswctl_open_socket failed. %d (%s)\n", skfd, ifr->ifr_name);
    }

    ifr->ifr_data = (char *)ifdata;

    return skfd;
}

/*! 
\brief      Dump port MIB counters. [OP-::ETHSWDUMPMIB CLI-::CMD_mibdump]
\details    ethswctl -c mibdump [-n \<unit\>] {-p} \<port\> {-a} \n 
            Specify -a to dump full set of MIB counters\n
            Specify -n without -p will dump all ports on specified switch\n
            Without -n, -p will dump all ports
\param[in]  unit 0-root switch, 1-switch connecting to root switch
\param[in]  port port number on specific unit
\param[in]  type 0-subset 1-full set counters 
\param[in]  reset 1-reset counters after read
\return     0 on success; a negative value on failure
*/
int ethswctl_mibdump(int unit, int port, int type, int reset)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    /* Validate inputs */
    if (port != -1) {
        if (type < 0 || type > 2) {
            fprintf(stderr, "Invalid type <%d> . Valid values are 0,1 and 2. \n", type);
            err = -1;
            goto out;
        }
        if (unit < 0 || unit > 1) {
            fprintf(stderr, "Invalid unit. Valid values are 0 and 1. \n");
            err = -1;
            goto out;
        }
    }

    /* Determine whether to dump all or a subset of mib counters */
    e->op = ETHSWDUMPMIB;
    e->type = type;
    e->sub_type = reset;
    e->port = port;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

out:
    close(skfd);
    return err;
}

/*! 
\brief      Enable system-wide hardware switching. [OP-::ETHSWSWITCHING CLI-::CMD_hwswitching]
\details    ethswctl -c hw-switching -o enable
\return     0 on success; a negative value on failure
*/
int ethswctl_enable_switching(void)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWSWITCHING;
    e->type = TYPE_ENABLE;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    err = e->ret_val;

out:
    close(skfd);
    return err;
}

/*! 
\brief      Disable system-wide hardware switching. [OP-::ETHSWSWITCHING CLI-::CMD_hwswitching]
\details    ethswctl -c hw-switching -o disable
\return     0 on success; a negative value on failure
*/
int ethswctl_disable_switching(void)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWSWITCHING;
    e->type = TYPE_DISABLE;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    err = e->ret_val;

out:
    close(skfd);
    return err;
}

/*! 
\brief      Get system-wide hardware switching status. [OP-::ETHSWSWITCHING CLI-::CMD_hwswitching]
\details    ethswctl -c hw-switching
\param[out] status 1-enabled, 0-disabled
\return      0 on success; a negative value on failure
*/
int ethswctl_get_switching_status(int *status)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWSWITCHING;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    *status = e->status;
    err = e->ret_val;

out:
    close(skfd);
    return err;
}

/*! 
\brief      Enable/disable switching flag [OP-::ETHSWSWITCHFLAG CLI- ]
\param[in]  enable  enable/disable flag
\return     0 on success; a negative value on failure
*/
int ethswctl_set_switch_flag(BOOL enable)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWSWITCHFLAG;
    if (enable)
    {
        e->type = TYPE_ENABLE;
    }
    else
    {
        e->type = TYPE_DISABLE;
    }

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl SIOCETHSWCTLOPS returned error %d (%s)\n", errno, strerror(errno));
    }

    close(skfd);
    return err;
}

/****************************************************************************/
/*  Switch Control API                                                      */
/****************************************************************************/

/*! 
\brief      Set Flow Control Drop/Pause Control, VLAN mechanisms of the switch. [OP-::ETHSWCONTROL CLI-::CMD_swctrl]
\details    ethswctl -c swctrl [-n \<unit\>] -t \<type\> -v \<val\>\n 
            Specify  type= 0-BUFF_CTRL; val= 1-TXQ_PAUSE_EN, 2-TXQ_DROP_EN, 4-TOT_PAUSE_EN, 8-TOT_DROP_EN\n
            or       type= 1-8021Q_CTRL; val= 0-DISABLE, 1-ENABLE, Shared VLAN learning (SVL), 2-ENABLE, Individual VLAN learning (IVL)
\param[in]  unit 0-root switch, 1-switch connecting to root switch
\param[in]  type The desired configuration parameter to modify.
\param[in]  arg The value with which to set the parameter.
\return     0 on success; a negative value on failure
*/
int bcm_switch_control_set(int unit, bcm_switch_control_t type, int arg)
{
    return bcm_switch_control_setX(unit, type, 0, arg);
}
int bcm_switch_control_setX(int unit, bcm_switch_control_t type,
                   bcm_switch_fc_t sub_type, int arg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCONTROL;
    e->type = TYPE_SET;
    e->sw_ctrl_type = type;
    e->sub_type = sub_type;
    e->val = arg;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/***************************************************************************
 * Function:
 * int ethswctl_quemap_call(int unit, int port, int que, int type);
 * Description:
 *  Set/Get WAN/LAN Queue bit map
 *  port - Port number to be monitored.
 * Returns:
 *    BCM_E_xxxx
***************************************************************************/
int ethswctl_quemap_call(int unit, int *val, int *queRemap, int set)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifd;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifd;

    ifd.op = ETHSWQUEMAP;
    ifd.type = set? TYPE_SET: TYPE_GET;
    ifd.val = *val;
    ifd.priority = *queRemap;
    ifd.unit = 1;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    *val = ifd.val;
    *queRemap = ifd.priority;

out:
    close(skfd);
    return err;
}

/****************************************************************************/
/*
 * Function:
 * int ethswctl_quemon_get(int unit, int port, int que, int type);
 * Description:
 *  Set Flow Control Drop/Pause Control mechanisms of the switch.
 *  Unit - Unit number to be monitored.
 *  port - Port number to be monitored.
 *  type - Flow control type to be monitored.
 * Returns:
 *    BCM_E_xxxx
 */
int ethswctl_quemon_get(int unit, int port, int que, int type, int *val)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWQUEMON;
    e->type = TYPE_GET;
    e->unit = unit;
    e->port = port;
    e->priority = que;
    e->sw_ctrl_type = type;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    *val = e->val;

out:
    close(skfd);
    return e->ret_val;
}

/*! 
\brief      Get Flow Control Drop/Pause Control, VLAN mechanisms of the switch. [OP-::ETHSWCONTROL CLI-::CMD_swctrl]
\details    ethswctl -c swctrl [-n \<unit\>] -t \<type\>\n 
            Specify  type= 0-BUFF_CTRL or type= 1-8021Q_CTRL
\param[in]  unit 0-root switch, 1-switch connecting to root switch
\param[in]  type The desired configuration parameter to retrieve.
\param[out] arg Pointer to where the retrieved value will be written.
\return     0 on success; a negative value on failure
*/
int bcm_switch_control_get(int unit, bcm_switch_control_t type, int *arg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCONTROL;
    e->type = TYPE_GET;
    e->sw_ctrl_type = type;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    *arg = e->val;

out:
    close(skfd);
    return err;
}


/*
 * Function:
 *  bcm_switch_control_priority_set
 * Description:
 *  Set switch parameters on a per-priority (cos) basis.
 * Parameters:
 *  unit - Device unit number
 *  priority - The priority to affect
 *  type - The desired configuration parameter to modify
 *  arg - The value with which to set the parameter
 * Returns:
 *  BCM_E_xxx
 */
int bcm_switch_control_priority_set(int unit, bcm_cos_t priority,
                  bcm_switch_control_t type, int arg)
{
    return bcm_switch_control_priority_setX(unit, 0, priority, type, arg);
}

int bcm_switch_control_priority_setX(int unit, int port, bcm_cos_t priority,
                  bcm_switch_control_t type, int arg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPRIOCONTROL;
    e->priority = priority;
    e->type = TYPE_SET;
    e->val = arg;
    e->sw_ctrl_type = type;
    e->unit = unit;
    e->port = port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_switch_control_priority_get
 * Description:
 *  Get switch parameters on a per-priority (cos) basis.
 * Parameters:
 *  unit - Device unit number
 *  priority - The priority to affect
 *  type - The desired configuration parameter to retrieve
 *  arg - Pointer to where the retrieved value will be written
 * Returns:
 *  BCM_E_xxx
 */
int bcm_switch_control_priority_get(int unit, bcm_cos_t priority,
                  bcm_switch_control_t type, int *arg)
{
    return bcm_switch_control_priority_getX(unit, 0, priority, type, arg);
}

int bcm_switch_control_priority_getX(int unit, int port, bcm_cos_t priority,
                  bcm_switch_control_t type, int *arg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPRIOCONTROL;
    e->unit = unit;
    e->port = port;
    e->priority = priority;
    e->type = TYPE_GET;
    e->sw_ctrl_type = type;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    *arg = e->ret_val;

out:
    close(skfd);
    return err;
}
/*
 * Function:
 *  bcm_acb_set
 * Description:
 *  Set ACB configuration
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *    type - The desired configuration parameter to modify.
 *    arg - The value with which to set the parameter.
 * Returns:
 *    BCM_E_xxxx
 */
int bcm_acb_cfg_set(int unit, int queue, bcm_switch_acb_control_t type, int arg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWACBCONTROL;
    e->type = TYPE_SET;
    e->sw_ctrl_type = type;
    e->val = arg;
    e->unit = unit;
    e->queue = queue;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_switch_control_get
 * Description:
 *  Get Flow Control Drop/Pause Control mechanisms of the switch.
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *    type - The desired configuration parameter to retrieve.
 *    arg - Pointer to where the retrieved value will be written.
 * Returns:
 *    BCM_E_xxxx
 */
int bcm_acb_cfg_get(int unit, int queue, bcm_switch_acb_control_t type, void *arg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWACBCONTROL;
    e->type = TYPE_GET;
    e->sw_ctrl_type = type;
    e->unit = unit;
    e->queue = queue;
    e->vptr = arg;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

/****************************************************************************/
/*  Port Configuration API:  For Configuring Pause Enable/Disable                                    */
/****************************************************************************/
/* Enable/Disable the Pause  */
int bcm_port_pause_capability_set(int unit, bcm_port_t port, char val )
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_pause_capability_set_by_name(obj_name, val, 1);
}

/****************************************************************************/
/*  Port Configuration API:  For Configuring Pause Enable/Disable by interface name                                   */
/****************************************************************************/
/* Enable/Disable the Pause  */
int bcm_port_pause_capability_set_by_name(const char *ifname, char val, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTPAUSECAPABILITY;
    e->type = TYPE_SET;
    e->val = val;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl SIOCETHSWCTLOPS returned error %d - not supported\n", errno);
    }

    close(skfd);
    return err;
}

/* Get the Pause Enable/Disable status */
int bcm_port_pause_capability_get2(int unit, bcm_port_t port, int *val, int *val2)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_pause_capability_get_by_name(obj_name, val, val2, 1);
}


/* Get the Pause Enable/Disable status by interface name*/
int bcm_port_pause_capability_get_by_name(const char *ifname, int *val, int *val2, int is_object)
{
    int i, skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTPAUSECAPABILITY;
    e->type = TYPE_GET;
	
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
      goto out;
    }

    *val = e->pfc_ret;
    if (val2)
    {
        for (i=0; i<8; i++)
        {
            val2[i] = e->pfc_timer[i];
        }
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_rate_ingress_set
 * Purpose:
 *  Set ingress rate limiting parameters
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  kbits_sec - Rate in kilobits (1000 bits) per second.
 *            Rate of 0 disables rate limiting.
 *  kbits_burst - Maximum burst size in kilobits (1000 bits).
 * Returns:
 *  BCM_E_XXX
 * Note :
 *  1. Robo Switch support 2 ingress buckets for different packet type.
 *     And the bucket1 contains higher priority if PKT_MSK confilict
 *       with bucket0's PKT_MSK.
 *  2. Robo Switch allowed system basis rate/packet type assignment for
 *     Rate Control. The RATE_TYPE and PKT_MSK will be set once in the
 *       initial routine.
 */
int bcm_port_rate_ingress_set(int unit,
                  bcm_port_t port,
                  unsigned int kbits_sec,
                  unsigned int kbits_burst)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTRXRATE;
    e->port = port;
    e->limit = kbits_sec;
    e->burst_size = kbits_burst;
    e->type = TYPE_SET;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_rate_ingress_get
 * Purpose:
 *  Get ingress rate limiting parameters
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  kbits_sec - (OUT) Rate in kilobits (1000 bits) per second, or
 *                  zero if rate limiting is disabled.
 *  kbits_burst - (OUT) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *  BCM_E_XXX
 */
int bcm_port_rate_ingress_get(int unit,
                  bcm_port_t port,
                  unsigned int *kbits_sec,
                  unsigned int *kbits_burst)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTRXRATE;
    e->port = port;
    e->type = TYPE_GET;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *kbits_sec =  e->limit;
    *kbits_burst = e->burst_size;

out:
    close(skfd);
    return err;
}


/*
 * Function:
 *  bcm_port_rate_egress_set
 * Purpose:
 *  Set egress rate limiting parameters
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  kbits_sec - Rate in kilobits (1000 bits) per second.
 *          Rate of 0 disables rate limiting.
 *  kbits_burst - Maximum burst size in kilobits (1000 bits).
 * Returns:
 *  BCM_E_XXX
 * Note :
 *  1. Robo Switch support 2 ingress buckets for different packet type.
 *     And the bucket1 contains higher priority if PKT_MSK confilict
 *       with bucket0's PKT_MSK.
 *  2. Robo Switch allowed system basis rate/packet type assignment for
 *     Rate Control. The RATE_TYPE and PKT_MSK will be set once in the
 *       initial routine.
 */
int bcm_port_shaper_cfg(int unit,
                 bcm_port_t port,
                 int queue,
                 int shaper_cfg_flags,
                 int val)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTSHAPERCFG;
    e->port = port;
    e->type = TYPE_SET;
    e->unit = unit;
    e->queue = queue;
    e->sub_type = shaper_cfg_flags;
    e->val = val;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}
/*
 * Function:
 *  bcm_port_rate_egress_set
 * Purpose:
 *  Set egress rate limiting parameters
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  kbits_sec - Rate in kilobits (1000 bits) per second.
 *          Rate of 0 disables rate limiting.
 *  kbits_burst - Maximum burst size in kilobits (1000 bits).
 * Returns:
 *  BCM_E_XXX
 * Note :
 *  1. Robo Switch support 2 ingress buckets for different packet type.
 *     And the bucket1 contains higher priority if PKT_MSK confilict
 *       with bucket0's PKT_MSK.
 *  2. Robo Switch allowed system basis rate/packet type assignment for
 *     Rate Control. The RATE_TYPE and PKT_MSK will be set once in the
 *       initial routine.
 */
int bcm_port_rate_egress_set(int unit,
                 bcm_port_t port,
                 unsigned int erc_limit,
                 unsigned int erc_burst)
{
    int queue = -1;
    int is_pkt_mode = 0;
#if defined(CHIP_63138) || defined(CHIP_63148)
    DBG(fprintf(stderr, "On your platform, calling bcm_port_rate_egress_Set_X() delivers enhanced functionality \n\n"););
#endif
    return bcm_port_rate_egress_set_X(unit, port, erc_limit, erc_burst,
                 queue, is_pkt_mode);
}
int bcm_port_rate_egress_set_X(int unit,
                 bcm_port_t port,
                 unsigned int erc_limit,
                 unsigned int erc_burst,
                 int queue,
                 int is_pkt_mode)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTTXRATE;
    e->port = port;
    e->limit = erc_limit;
    e->burst_size = erc_burst;
    e->type = TYPE_SET;
    e->unit = unit;
    e->queue = queue;
    e->sub_type = is_pkt_mode;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_rate_egress_get
 * Purpose:
 *  Get egress rate limiting parameters
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  kbits_sec - (OUT) Rate in kilobits (1000 bits) per second, or
 *              zero if rate limiting is disabled.
 *  kbits_burst - (OUT) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *  BCM_E_XXX
 */
int bcm_port_rate_egress_get(int unit,
                 bcm_port_t port,
                 unsigned int *kbits_sec,
                 unsigned int *kbits_burst)
{
    int queue = -1;
#if defined(CHIP_63138) || defined(CHIP_63148)
    DBG(fprintf(stderr, "On your platform, calling bcm_port_rate_egress_get_X() delivers enhanced functionality \n\n"););
#endif
    return bcm_port_rate_egress_get_X(unit, port, kbits_sec, kbits_burst, queue, NULL);

}
int bcm_port_rate_egress_get_X(int unit,
                 bcm_port_t port,
                 unsigned int *kbits_sec,
                 unsigned int *kbits_burst,
                 int queue,           // Extended interface from here
                 void *vptr)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTTXRATE;
    e->port = port;
    e->type = TYPE_GET;
    e->unit = unit;
    e->queue = queue;
    e->vptr = vptr;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }
out:
    if (vptr == NULL) { // Tells Legacy interface has been called
    *kbits_sec =  e->limit;
    *kbits_burst = e->burst_size;
    }
    close(skfd);
    return err;
}

int bcm_port_txq_set(const char*     ifname,
                 int queue, int enable)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTTXQSTATE;
    e->type = enable ? TYPE_ENABLE : TYPE_DISABLE;
    e->queue = queue;
    strncpy(e->ifname, ifname, IFNAMSIZ);
    e->ifname[IFNAMSIZ - 1] = '\0';

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

int bcm_port_learning_ind_set(int unit, bcm_port_t port, unsigned char learningInd)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_learning_ind_set_by_name(obj_name, learningInd, 1);
}

int bcm_port_learning_ind_set_by_name(const char *ifname, unsigned char learningInd, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTSALDAL;
    e->type = TYPE_SET;
    e-> sal_dal_en = learningInd;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)))
      {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
          }
    close(skfd);
    return err;
}

int bcm_port_transparent_set(int unit, bcm_port_t port, unsigned char enable)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_transparent_set_by_name(obj_name, enable, 1);
}

/*set transparent per interface name
 *is_object Indicate if ifname is name of network interface or port object name*/
int bcm_port_transparent_set_by_name(const char *ifname, unsigned char enable, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTTRANSPARENT;
    e->type = TYPE_SET;
    e->transparent = enable;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)))
      {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
          }
    close(skfd);
    return err;
}


int bcm_port_vlan_isolation_set(int unit, bcm_port_t port, unsigned char us,unsigned char ds)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_vlan_isolation_set_by_name(obj_name, us, ds, 1);
}

/*set vlan isolation per interface name
 *is_object Indicate if ifname is name of network interface or port object name*/
int bcm_port_vlan_isolation_set_by_name(const char *ifname, unsigned char us,unsigned char ds, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTVLANISOLATION;
    e->type = TYPE_SET;
    e->vlan_isolation.us_enable = us;
    e->vlan_isolation.ds_enable = ds;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)))
    {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
    }
	
    close(skfd);
    return err;
}


int bcm_port_bc_rate_limit_set(int unit, bcm_port_t port, unsigned int rate)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_bc_rate_limit_set_by_name(obj_name, rate, 1);
}


int bcm_port_bc_rate_limit_set_by_name(const char *ifname, unsigned int rate, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWCPUMETER;
    e->type = TYPE_SET;
    e->cpu_meter_rate_limit.meter_type = METER_TYPE_BROADCAST;
    e->cpu_meter_rate_limit.rate_limit = rate;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)))
    {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
    }
    close(skfd);
    return err;
}


/****************************************************************************/
/*  Enet Driver Config/Control API:  For Configuring Enet Driver Rx Scheduling                 */
/****************************************************************************/

/* Get Enable/Disable status of the Interrupt processing of a given channel */
/* For debugging only */
int bcm_enet_driver_test_config_get(int unit, int type, int param, int *val)
{
  int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

  if ((skfd=ethswctl_init(&ifr)) < 0) {
    fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
  }
    ifr.ifr_data = (char *)&ifdata;

  e->op = ETHSWTEST1;
  e->type = TYPE_GET;
  e->sub_type = type;
  e->unit = unit;
  if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
    fprintf(stderr, "ioctl command return error!\n");
    goto out;
  }

  if (type == SUBTYPE_RESETMIB) {
    /* Also reset the software counters */
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCSCLEARMIBCNTR, &ifr))) {
      fprintf(stderr, "ioctl SIOCSCLEARMIBCNTR return error!\n");
      goto out;
    }
  }

  *val = (int)e->ret_val;

out:
    close(skfd);
    return err;
}

/*
 * Function:
 * Description:
 *  bcm_enet_map_ifname_to_unit_portmap
 * Parameters:
 * input
 *       - ifname - interface name
 *  output
 *       - unit -- unit number to caller
 *       - portmap -- port number bitmap to caller
 * Returns:
 *  BCM_E_xxx
 */
int bcm_enet_map_ifname_to_unit_portmap(const char *ifName, int *unit, unsigned int *portmap)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWUNITPORT;
    e->type = TYPE_GET;
    strncpy(e->ifname, ifName, IFNAMSIZ);
    e->ifname[IFNAMSIZ - 1] = '\0';
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP) {
            fprintf(stderr, "ioctl not supported!\n");
        }
        else {
            fprintf(stderr, "ioctl command return error %d!\n", err);
        }
    }
    *unit = e->unit;
    *portmap = e->port_map;
    DBG(fprintf(stderr, "ifname %s is: unit %d portmap 0x%x", *unit, *portmap););
    close(skfd);
    return err;
}
/*
 * Function:
 * Description:
 *  bcm_enet_map_ifname_to_unit_port
 * Parameters:
 * input
 *       - ifname - interface name
 *  output
 *       - unit -- unit number to caller
 *       - port -- port number to caller
 * Returns:
 *  BCM_E_xxx
 */
int bcm_enet_map_ifname_to_unit_port(const char *ifName, int *unit, bcm_port_t *port)
{
    unsigned int portmap;
    int err = bcm_enet_map_ifname_to_unit_portmap(ifName,unit,&portmap);
    *port = 0;
    if (!err)
    {
        while(portmap)
        {
            if (portmap & (1<<(*port)))
            {
                break;
            }
            *port +=1;
        }
    }
    DBG(fprintf(stderr, "ifname %s is: unit %d port 0x%x", *unit, *port););
    return err;
}

/*! 
\brief      Set interface software switching status. [OP-::ETHSWSOFTSWITCHING CLI-::CMD_softswitch]
\details     ethswctl -c softswitch -i \<if_name\> -o \<enable\|disable\> \n
            Note: operation is only allowed for LAN interface.
\param[in]  bEnable 1-enable, 0-disable
\param[in]  ifName ethernet interface device name
\return      0 on success; a negative value on failure
*/
int bcm_enet_driver_enable_soft_switching_port(int bEnable, char *ifName)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    err = bcm_enet_map_ifname_to_unit_port(ifName, &e->unit, &e->port);
    if(err) return err;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWSOFTSWITCHING;
    e->type = bEnable ? TYPE_ENABLE : TYPE_DISABLE;
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP) {
            fprintf(stderr, "ioctl not supported!\n");
        }
        else {
            fprintf(stderr, "ioctl command return error %d!\n", err);
        }
    }

    close(skfd);
    return err;
}

/*! 
\brief      Get interface software switching status as portmap. [OP-::ETHSWSOFTSWITCHING CLI-::CMD_softswitch]
\details    ethswctl -c softswitch
\param[out] portmap bit set to 1 if interface is WAN or (LAN and not hardware switching).
\return     0 on success; a negative value on failure
*/
int bcm_enet_driver_get_soft_switching_status(unsigned int *portmap)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWSOFTSWITCHING;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP) {
            fprintf(stderr, "ioctl not supported!\n");
        }
        else {
            fprintf(stderr, "ioctl command return error %d!\n", err);
        }
        goto out;
    }

    *portmap = (unsigned int)e->status;
out:
    close(skfd);
    return err;
}

int bcm_enet_driver_hw_stp_set(int bEnable, char *ifName)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    err = bcm_enet_map_ifname_to_unit_port(ifName, &e->unit, &e->port);
    if(err) return err;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWHWSTP;
    e->type = bEnable ? TYPE_ENABLE : TYPE_DISABLE;
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

int bcm_enet_driver_get_hw_stp_status(unsigned int *portmap)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWHWSTP;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *portmap = (unsigned int)e->status;
out:
    close(skfd);
    return err;
}

int bcm_enet_driver_if_stp_set(char *ifName, int state)
{
    int skfd, err = -1;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    err = bcm_enet_map_ifname_to_unit_port(ifName, &e->unit, &e->port);
    if(err) return err;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWIFSTP;
    e->type = TYPE_SET;
    e->val = state;
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

int bcm_enet_driver_if_stp_get(char * ifName, unsigned int *state)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    err = bcm_enet_map_ifname_to_unit_port(ifName, &e->unit, &e->port);
    if(err) return err;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWIFSTP;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *state = (unsigned int)e->val;
out:
    close(skfd);
    return err;
}
/* VLAN TBL ACCESS FUNCTIONS */

/*
 * Function:
 *  bcm_robo_vlan_port_set
 * Description:
 *  Configure the WRR parameters
 * Parameters:
 *  unit - Device unit number
 *  vid -  VLAN ID
 *  fwd_map - Members of the VLAN
 *  untag_map - Untagged members of the VLAN
 * Returns:
 *  BCM_E_xxx
 */
int bcm_vlan_port_set(int unit, int vid, int fwd_map, int untag_map)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWVLAN;
    e->type = TYPE_SET;
    e->vid = vid;
    e->fwd_map = fwd_map;
    e->untag_map = untag_map;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_robo_vlan_port_get
 * Description:
 *  Retrieve the WRR parameters
 * Parameters:
 *  unit - Device unit number
 *  vid -  VLAN ID
 *  fwd_map - Members of the VLAN
 *  untag_map - Untagged members of the VLAN
 * Returns:
 *  BCM_E_xxx
 */
int bcm_vlan_port_get(int unit, int vid, int * fwd_map, int *untag_map)

{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWVLAN;
    e->type = TYPE_GET;
    e->vid = vid;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

    *fwd_map = e->fwd_map;
    *untag_map = e->untag_map;

out:
    close(skfd);
    return err;

}


/*
 * Function:
 * Description:
 * Parameters:
 * Returns:
 *  BCM_E_xxx
 */
int bcm_port_pbvlanmap_set(int unit, int port, int fwd_map)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPBVLAN;
    e->port = port;
    e->fwd_map = fwd_map;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 * Description:
 * Parameters:
 * Returns:
 *  BCM_E_xxx
 */
int bcm_port_defvlan_get(int unit, int port, int *vid)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPDEFVLAN;
    e->type = TYPE_GET;
    e->port = port;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *vid = e->vid;

out:
    close(skfd);
    return err;
}

/*
 * Function:
 * Description:
 * Parameters:
 * Returns:
 *  BCM_E_xxx
 */
int bcm_port_defvlan_set(int unit, int port, int vid)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPDEFVLAN;
    e->port = port;
    e->vid = vid;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 * Description:
 * Parameters:
 * Returns:
 *  BCM_E_xxx
 */
int bcm_port_pbvlanmap_get(int unit, int port, int *fwd_map)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPBVLAN;
    e->type = TYPE_GET;
    e->port = port;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *fwd_map = e->fwd_map;

out:
    close(skfd);
    return err;
}

/* CoS API */

int bcm_cosq_sched_get(int unit, int *mode, int *sp_endq,
                       int weights[BCM_COS_COUNT]) {
    int err;
    port_qos_sched_t qs;

    err = bcm_cosq_sched_get_X(unit, 0, weights, &qs);
    if (err == 0) {
        *mode    = qs.sched_mode;
        *sp_endq = qs.num_spq;
    }
    return err;
}
int bcm_cosq_sched_get_X(int unit, bcm_port_t port,
                       int weights[BCM_COS_COUNT], port_qos_sched_t *qs)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSSCHED;
    e->type = TYPE_GET;
    e->unit = unit;
    e->port = port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }
    *qs = e->port_qos_sched;
    memcpy(weights, e->weights, BCM_COS_COUNT * sizeof(int));

out:
    close(skfd);
    return err;
}

int bcm_cosq_sched_set(int unit, int mode, int sp_endq,
                        int weights[BCM_COS_COUNT])
{
    port_qos_sched_t qs;

    memset(&qs, 0, sizeof(qs));
    qs.sched_mode = mode;
    qs.num_spq = sp_endq;
    return bcm_cosq_sched_set_X(unit, 0, weights, &qs);
}
int bcm_cosq_sched_set_X(int unit, int port,
                        int *weights, port_qos_sched_t *qs)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSSCHED;
    e->type = TYPE_SET;
    e->port_qos_sched = *qs;
    memcpy(e->weights, weights, BCM_COS_COUNT * sizeof(int));
    e->unit = unit;
    e->port = port;
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/* Set the internal priority to egress queue mapping of the given port */
int bcm_cosq_port_mapping_get(int unit, bcm_port_t port, bcm_cos_t priority,
                              bcm_cos_queue_t *cosq)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPORTMAP;
    e->type = TYPE_GET;
    e->priority = priority;
    e->port = port;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *cosq = (bcm_cos_queue_t)e->queue;

out:
    close(skfd);
    return err;
}

/* Get the internal priority to egress queue mapping of the given port */
int bcm_cosq_port_mapping_set(int unit, bcm_port_t port, bcm_cos_t priority,
                              bcm_cos_queue_t cosq)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPORTMAP;
    e->priority = priority;
    e->port = port;
    e->queue = cosq;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_cosq_priority_method_get
 * Description:
 * Retrieve the method for deciding on frame priority
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  method -
 *  PORT_QOS: Frame priority is based on the priority of port default tag
 *  MAC_QOS: Frame priority is based on the destination MAC address
 *  IEEE8021P_QOS: Frame priority is based on 802.1p field of the frame
 *  DIFFSERV_QOS: Frame priority is based on the diffserv field of the frame
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_priority_method_get(int unit, int port, int *method)
{
    return bcm_cosq_priority_method_get_X(unit, port, method, 0);
}
int bcm_cosq_priority_method_get_X(int unit, int port, int *method, int pkt_type_mask)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPRIORITYMETHOD;
    e->type = TYPE_GET;
    e->unit = unit;
    e->port = port;
    e->pkt_type_mask = pkt_type_mask;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

    *method = e->ret_val;

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_cosq_priority_method_set
 * Description:
 *  Set the method for deciding on frame priority
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  method -
 *  PORT_QOS: Frame priority is based on the priority of port default tag
 *  MAC_QOS: Frame priority is based on the destination MAC address
 *  IEEE8021P_QOS: Frame priority is based on 802.1p field of the frame
 *  DIFFSERV_QOS: Frame priority is based on the diffserv field of the frame
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_priority_method_set(int unit, int port, int method)
{
     return bcm_cosq_priority_method_set_X(unit, port, method, 0);
}
int bcm_cosq_priority_method_set_X(int unit, int port, int method, int pkt_type_mask)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPRIORITYMETHOD;
    e->type = TYPE_SET;
    e->val = method;
    e->unit = unit;
    e->port = port;
    e->pkt_type_mask = 0;
    e->pkt_type_mask = pkt_type_mask;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_cosq_pcp_priority_mapping_get
 * Description:
 *  Get PID to priority mapping
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  priority:  switch priority
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_pid_priority_mapping_get(int unit, int port, bcm_cos_t *priority)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPIDPRIOMAP;
    e->type = TYPE_GET;
    e->unit = unit;
    e->port = port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
    }

    *priority = (bcm_cos_t)e->priority;

    close(skfd);
    return err;
}
/*
 * Function:
 *  bcm_cosq_pid_priority_mapping_set
 * Description:
 *  Configure DSCP to priority mapping
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  priority:  switch priority
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_pid_priority_mapping_set(int unit, int port, bcm_cos_t priority)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPIDPRIOMAP;
    e->type = TYPE_SET;
    e->priority = priority;
    e->unit = unit;
    e->port = port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
    }

    close(skfd);
    return err;
}
/*
 * Function:
 *  bcm_cosq_pcp_priority_mapping_set
 * Description:
 *  Configure PCP to priority mapping
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  pcp:  3-bit pcp value
 *  priority:  switch priority
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_pcp_priority_mapping_set(int unit, int port, int pcp, bcm_cos_t priority)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPCPPRIOMAP;
    e->type = TYPE_SET;
    e->val = pcp;
    e->priority = priority;
    e->unit = unit;
    e->port = port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_cosq_pcp_priority_mapping_get
 * Description:
 *  Get PCP to priority mapping
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  pcp:  3-bit pcp value
 *  priority:  switch priority
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_pcp_priority_mapping_get(int unit, int port, int pcp, bcm_cos_t *priority)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSPCPPRIOMAP;
    e->type = TYPE_GET;
    e->val = pcp;
    e->unit = unit;
    e->port = port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
    }

    *priority = (bcm_cos_t)e->priority;

    close(skfd);
    return err;
}
/*
 * Function:
 *  bcm_cosq_dscp_priority_mapping_set
 * Description:
 *  Configure DSCP to priority mapping
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  dscp:  6-bit dscp value
 *  priority:  switch priority
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_dscp_priority_mapping_set(int unit, int dscp, bcm_cos_t priority)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSDSCPPRIOMAP;
    e->type = TYPE_SET;
    e->val = dscp;
    e->priority = priority;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_cosq_dscp_priority_mapping_get
 * Description:
 *  Get DSCP to priority mapping
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  dscp:  6-bit dscp value
 *  priority:  switch priority
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_cosq_dscp_priority_mapping_get(int unit, int dscp, bcm_cos_t *priority)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWCOSDSCPPRIOMAP;
    e->type = TYPE_GET;
    e->val = dscp;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
    }

    *priority = (bcm_cos_t)e->priority;

    close(skfd);
    return err;
}

/****************************************************************************/
/*  Statistics API                                                          */
/****************************************************************************/

/*
 * Function:
 *  bcm_stat_port_clear
 * Description:
 *  Clear the port based statistics from the RoboSwitch port.
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  port - zero-based port number
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_stat_port_clear(int unit, bcm_port_t port)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_stat_port_clear_by_name(obj_name, 1);
}

/*
 * Function:
 *  bcm_stat_port_clear_by_name
 * Description:
 *  Clear the port based statistics from the RoboSwitch port by port name.
 * Parameters:
 *  ifname - port name
 *  is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_stat_port_clear_by_name(const char *ifname, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWSTATPORTCLR;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_stat_clear_emac
 * Description:
 *  clear the specified statistic from the cached data in bcmenet
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  port - zero-based port number
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_stat_clear_emac(int unit, bcm_port_t port)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_stat_clear_emac_by_name(obj_name, 1);
}


/*
 * Function:
 *  bcm_stat_clear_emac_by_name
 * Description:
 *  clear the specified statistic from the cached data in bcmenet by port name.
 * Parameters:
 *  ifname - port name
 *  is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_stat_clear_emac_by_name(const char *ifname, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWEMACCLEAR;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

out:
    close(skfd);
    return err;
}


/*
 * Function:
 *  bcm_stat_get_emac
 * Description:
 *  Get the specified statistic from the cached data
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  port - zero-based port number
 *  value - (OUT) emac address to return data.
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_stat_get_emac(int unit, bcm_port_t port, struct emac_stats* value)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_stat_get_emac_by_name(obj_name, value, 1);
}

/*
 * Function:
 *  bcm_stat_get_emac_by_name
 * Description:
 *  Get the specified statistic from the cached data
 * Parameters:
 *  ifname - port name
 *  value - (OUT) emac address to return data.
 *  is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_stat_get_emac_by_name(const char *ifname, struct emac_stats* value, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWEMACGET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

    memcpy(value, &(e->emac_stats_s), sizeof(struct emac_stats));

out:
    close(skfd);
    return err;
}


int bcm_arl_read(int unit, char *mac, bcm_vlan_t vid, unsigned short *value) /* Deprecated API */
{
    char _mac[6];
    int i;
    *value = -1;

    for(i=0; i<6; i++)
    {
        _mac[i] = mac[5-i];
    }
    return bcm_arl_read2(&unit, _mac, &vid, value);
}

int bcm_arl_read2(int *unit, char *mac, bcm_vlan_t *vid, unsigned short *value)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_GET;
    e->op = ETHSWARLACCESS;
    e->vid = (short)*vid;
    e->val = (short)*value;
    memcpy(e->mac, mac, 6);
    e->unit = *unit;

    err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr);
    *value = e->val;
    *vid = e->vid;
    *unit = e->unit;

    close(skfd);
    return err;
}

int bcm_arl_write(int unit, char *mac, bcm_vlan_t vid, unsigned short value) /* Deprecated API */
{
    char _mac[6];
    int i;

    for(i=0; i<6; i++)
    {
        _mac[i] = mac[5-i];
    }
    return bcm_arl_write2(unit, _mac, vid, value|(1<<31));
}

int bcm_arl_write2(int unit, char *mac, bcm_vlan_t vid, int val)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_SET;
    e->op = ETHSWARLACCESS;
    e->vid = vid;
    memcpy(e->mac, mac, 6);
    e->val = val;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

int bcm_arl_dump(int unit)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_DUMP;
    e->op = ETHSWARLACCESS;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
    ARL Flush: flush the dynamic ARL entries
    No static entries will be blindly removed anymore in driver.
    If static entries need to be reconfigured due to certain reason,
    use bcm_arl_write() to set it to correct one.
*/
int bcm_arl_flush(int unit)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_FLUSH;
    e->op = ETHSWARLACCESS;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_reg_write
 * Description:
 *  Write to a switch register
 * Parameters:
 *    addr = offset
 *    len = length of register
 *    data = ptr to value to be written to register
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_reg_write(unsigned int addr, char* data, int len)
{
    return bcm_reg_write_X(0,addr,data,len);
}
int bcm_reg_write_X(int unit, unsigned int addr, char* data, int len)
{
    int skfd, err = 0, i;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    DBG(fprintf(stderr, "write: addr = 0x%x, len = %d; data = ", addr, len););
    for (i=len-1; i>=0; i--) {
        DBG(fprintf(stderr, "%2x:", data[i]););
    }
    DBG(fprintf(stderr, "\n"););

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWREGACCESS;
    e->type = TYPE_SET;
    e->offset = addr;
    e->length = len;
    e->unit = unit;
    memcpy(e->data, data, len);

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_reg_read
 * Description:
 *  Read from a switch register
 * Parameters:
 *    addr = offset
 *    len = length of register
 *    data = ptr to value read from register
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_reg_read(unsigned int addr, char* data, int len)
{
    return bcm_reg_read_X(0,addr,data,len);
}
int bcm_reg_read_X(int unit, unsigned int addr, char* data, int len)
{
    int skfd, err = 0, i;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWREGACCESS;
    e->type = TYPE_GET;
    e->offset = addr;
    e->length = len;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

    memcpy(data, e->data, len);

    DBG(fprintf(stderr, "read: addr = 0x%x, len = %d; data = ", addr, len););
    for (i=len-1; i>=0; i--) {
        DBG(fprintf(stderr, "%2x:", data[i]););
    }
    DBG(fprintf(stderr, "\n"););

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_pseudo_mdio_write
 * Description:
 *  Write to a switch register
 * Parameters:
 *    unit - RoboSwitch device unit number (driver internal).
 *    page = switch page
 *    offset = offset of reg within the page
 *    len = length of register
 *    val = value to be written to register
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_pseudo_mdio_write(unsigned int addr, char* data, int len)
{
    int i, skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    DBG(fprintf(stderr, "addr = %d, len = %d \n", addr, len););
    DBG(fprintf(stderr, "data = "););
    for (i=len; i>=0; i--) {
        DBG(fprintf(stderr, "%2x:", (unsigned char)data[i]););
    }
    DBG(fprintf(stderr, "\n"););

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPSEUDOMDIOACCESS;
    e->type = TYPE_SET;
    e->offset = addr;
    e->length = len;
    memcpy(e->data, data, len);
    DBG(fprintf(stderr, "data = %02x%02x%02x%02x %02x%02x%02x%02x \n", e->data[7],
      e->data[6], e->data[5], e->data[4], e->data[3], e->data[2],
      e->data[1], e->data[0]););

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

out:
    close(skfd);
    return err;
}


/*
 * Function:
 *    bcm_pseudo_mdio_read
 * Description:
 *  Read from a switch register
 * Parameters:
 *    unit - RoboSwitch device unit number (driver internal).
 *    page = switch page
 *    offset = offset of reg within the page
 *    len = length of register
 *    val = value read from register
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_pseudo_mdio_read(unsigned int addr, char* data, int len)
{
    int i, skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    DBG(fprintf(stderr, "addr = %x, len = %d \n", addr, len););

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPSEUDOMDIOACCESS;
    e->type = TYPE_GET;
    e->offset = addr;
    e->length = len;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

    memcpy(data, e->data, len);
    DBG(fprintf(stderr, "data = "););
    for (i=len; i>=0; i--) {
        DBG(fprintf(stderr, "%2x:", (unsigned char)e->data[i]););
    }
    DBG(fprintf(stderr, "\n"););

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_get_switch_info
 * Description:
 *     Get switch and board info
 * Parameters:
 *    switch_id - Switch ID number.
 *    vend_dev_id - Switch Vendor and Device IDs
 *    bus_type =  How switch is accessed (SPI or MDIO or Direct)
 *    spi_id = SPI ID for SPI accesses
 *    chip_id =  Chip ID for SPI accesses
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_get_switch_info(int switch_id, unsigned int *vendor_id, unsigned int *dev_id,
  unsigned int *rev_id, int *bus_type, unsigned int *spi_id, unsigned int *chip_id,
  unsigned int *pbmp, unsigned int *phypbmp, int *epon_port)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    DBG(fprintf(stderr, "switch_id = %d \n", switch_id););

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWINFO;
    e->type = TYPE_GET;
    e->val = switch_id;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

    *vendor_id = e->vendor_id;
    *dev_id = e->dev_id;
    *rev_id = e->rev_id;
    *bus_type = e->ret_val;
    *spi_id = e->spi_id;
    *chip_id = e->chip_id;
    *pbmp = e->port_map;
    *phypbmp = e->phy_portmap;
	*epon_port = e->epon_port;
    DBG(fprintf(stderr, "vend_id = 0x%x, dev_id = 0x%x, rev_id = 0x%x, bus_type = %d;"
     " spi_id = %d; chip_id = %d; pbmp = 0x%x; phypbmp = 0x%x; epon_port=%d \n", *vendor_id, *dev_id,
     *rev_id, *bus_type, *spi_id, *chip_id, *pbmp, *phypbmp, *epon_port););

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_set_linkstatus
 * Description:
 *  Notify link status to Enet driver
 * Parameters:
 *    port - port.
 *    linkstatus - link status.
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_set_linkstatus(int unit, int port, int linkstatus, int speed, int duplex)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWLINKSTATUS;
    e->type = TYPE_SET;
    e->unit = unit;
    e->port = port;
    e->status = linkstatus;
    e->speed = speed;
    e->duplex = duplex;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_get_linkstatus
 * Description:
 *  Get link status from Enet driver
 * Parameters:
 *    port - port.
 *    linkstatus - link status.
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_get_linkstatus(int unit, int port, int *linkstatus)
{
    char obj_name[OBJIFNAMSIZ];
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_get_linkstatus_by_name(obj_name, linkstatus, 1);
}


/*
 * Function:
 *    bcm_get_linkstatus_by_name
 * Description:
 *  Get link status from Enet driver by name
 * Parameters:
 *    ifname - port ifname.
 *    linkstatus - link status.
 *    is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_get_linkstatus_by_name(const char *ifname, int *linkstatus, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWLINKSTATUS;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error!\n", __FUNCTION__);
        goto out;
    }
	
    *linkstatus = e->status;

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_traffic_control_set
 * Description:
 *  Enable/Disable tx/rx of a switch port
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  port - Port number
 *  ctrl_map: bit0 = rx_disable (1 = disable rx; 0 = enable rx)
 *            bit1 = tx_disable (1 = disable tx; 0 = enable tx)
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_traffic_control_set(int unit, int port, int ctrl_map)
{
    char obj_name[OBJIFNAMSIZ];

    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }
    return bcm_port_traffic_enable_ex(obj_name, !ctrl_map, 1);
}

/*
 * Function:
 *  bcm_port_traffic_enable
 * Description:
 *  Enable/Disable port
 * Parameters:
 *  enable    1 Enable port, 0 Disable Port
 *  is_object Indicate if ifname is name of network interface or port object name
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_traffic_enable_ex(const char *ifname, int enable, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, &ifdata, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }
    ifdata.op = ETHSWPORTTRAFFICCTRL;
    ifdata.type = TYPE_SET;
    ifdata.val = enable;

    if (ioctl(skfd, SIOCETHSWCTLOPS, &ifr) < 0) {
         fprintf(stderr, " %s ioctl SIOCSWANPORT(%04x) %s returns error! (%d/%s)",
                      __FUNCTION__, SIOCSWANPORT,  ifr.ifr_name, errno, strerror(errno));
         err = -1;
    }
    close(skfd);
    return err;
}

/*
   Get PHY_ID from unit, port and sub_port
   If *sub_port == -1 and the crossbar member has single sub port, the
        actual sub port will be back into *sub_port.
   Return:  0: - Success
           -1: - Failed due when *sub_port = -1 and more than single sub_port existing under
            the crossbar
*/
int bcm_get_phyid(int unit, bcm_port_t port, int *sub_port)
{
    int skfd;
    struct ifreq ifr;
    int phy_id;
    union {
        struct ethswctl_data ethswctl;
	    struct ethctl_data ethctl;
    } ifdata;

    if(bcm_ifname_get(unit, port, ifr.ifr_name) < 0) {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        goto error;
    }

    if ((skfd=ethswctl_open_socket(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_open_socket failed. \n");
        goto error;
    }

    ifr.ifr_data = &ifdata;
    phy_id = et_get_phyid(skfd, &ifr, *sub_port);
    if (phy_id != -1)
        *sub_port = ifdata.ethctl.sub_port;

    close (skfd);
    return phy_id;

error:
    return -1;
}

/*
 * Function:
 *  bcm_port_subport_loopback_set
 * Description:
 *  Enable/Disable of loopback of USB port or LAN port Phy
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  status:  1 = Enable loopback; 0 = Disable loopback
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_subport_loopback_set(int unit, bcm_port_t port, int sub_port, int cfg_speed, int status)
{
    char obj_name[OBJIFNAMSIZ];

    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_subport_loopback_set_by_name(obj_name, sub_port, cfg_speed, status, 1);
}

/*
 * Function:
 *  bcm_port_subport_loopback_set_by_name
 * Description:
 *  Enable/Disable of loopback of USB port or LAN port Phy by interface name
 * Parameters:
 *  ifname - name of network interface(sub_port = -1) or port object name(sub_port != -1). 
 *  sub_port - sub port number.
 *  cfg_speed - speed.
 *  status:  1 = Enable loopback; 0 = Disable loopback
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_subport_loopback_set_by_name(const char *ifname, int sub_port, int cfg_speed, int status, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    memset(&ifdata, 0, sizeof(ifdata));
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0)  {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTLOOPBACK;
    e->type = TYPE_SET;
    e->val = status;
    e->cfgSpeed = cfg_speed;

    if (sub_port != -1) {
        e->sub_port = sub_port;
        e->addressing_flag |= ETHSW_ADDRESSING_SUBPORT;
    }

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_subport_loopback_get
 * Description:
 *  Get loopback status of USB port or LAN port Phy
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  status:  1 = Enable loopback; 0 = Disable loopback
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_subport_loopback_get(int unit, bcm_port_t port, int sub_port, int *cfg_speed, int *status)
{
    char obj_name[OBJIFNAMSIZ];

    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_port_subport_loopback_get_by_name(obj_name, sub_port, cfg_speed, status, 1);
}

/*
 * Function:
 *  bcm_port_subport_loopback_get_by_name
 * Description:
 *  Get loopback status of USB port or LAN port Phy by interface name
 * Parameters:
 *  ifname - name of network interface(sub_port = -1) or port object name(sub_port != -1). 
 *  sub_port - sub port number.
 *  cfg_speed - speed.
 *  status:  1 = Enable loopback; 0 = Disable loopback
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_subport_loopback_get_by_name(const char *ifname, int sub_port, int *cfg_speed, int *status, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    memset(&ifdata, 0, sizeof(ifdata));
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPORTLOOPBACK;
    e->type = TYPE_GET;

    if (sub_port != -1) {
        e->sub_port = sub_port;
        e->addressing_flag |= ETHSW_ADDRESSING_SUBPORT;
    }

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error!\n", __FUNCTION__);
    }

    *status = e->ret_val;
    if (cfg_speed) {
        *cfg_speed = e->cfgSpeed;
    }
    
    close(skfd);
    return err;
}


/*
 * Function:
 *  bcm_mac_loopback_set
 * Description:
 *  Enable/Disable of loopback of USB port or LAN port Phy
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  status:  0 = Disable loopback; 1 = local; 2 = remote; 3 = both
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_mac_loopback_set(int unit, bcm_port_t port, int cfg_speed, int status)
{
    char obj_name[OBJIFNAMSIZ];

    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_mac_loopback_set_by_name(obj_name, cfg_speed, status, 1);
}

/*
 * Function:
 *  bcm_mac_loopback_set_by_name
 * Description:
 *  Enable/Disable of loopback of USB port or LAN port Phy by interface name
 * Parameters:
 *  ifname - name of network interface or port object name
 *  cfg_speed - speed.
 *  status:  0 = Disable loopback; 1 = local; 2 = remote; 3 = both
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_mac_loopback_set_by_name(const char *ifname, int cfg_speed, int status, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    memset(&ifdata, 0, sizeof(ifdata));
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0)  {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWMACLOOPBACK;
    e->type = TYPE_SET;
    e->val = status;
    e->cfgSpeed = cfg_speed;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_mac_loopback_get
 * Description:
 *  Get loopback status of USB port or LAN port Phy
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (driver internal).
 *  status:  0 = Disable loopback; 1 = local; 2 = remote; 3 = both
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_mac_loopback_get(int unit, bcm_port_t port, int *cfg_speed, int *status)
{
    char obj_name[OBJIFNAMSIZ];

    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_mac_loopback_get_by_name(obj_name, cfg_speed, status, 1);
}

/*
 * Function:
 *  bcm_mac_loopback_get_by_name
 * Description:
 *  Get loopback status of USB port or LAN port Phy by interface name
 * Parameters:
 *  ifname - name of network interface or port object name
 *  cfg_speed - speed.
 *  status:  0 = Disable loopback; 1 = local; 2 = remote; 3 = both
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_mac_loopback_get_by_name(const char *ifname, int *cfg_speed, int *status, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    memset(&ifdata, 0, sizeof(ifdata));
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWMACLOOPBACK;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error!\n", __FUNCTION__);
    }

    *status = e->ret_val;
    if (cfg_speed) {
        *cfg_speed = e->cfgSpeed;
    }
    
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_jumbo_control_set
 * Description:
 *  Set jumbo accept/reject control of selected port(s)
 * Parameters:
 *  port - Port number 9(ALL), 6(USB), 4(GPON_SERDES), 3(GMII_2), 2(GMII_1), 1(GPHY_1), 0(GPHY_0)
 *  ctrlValPtr - pointer to result
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_jumbo_control_set(int unit, bcm_port_t port, int* ctrlValPtr) // bill
{
  int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

  if ((skfd=ethswctl_init(&ifr)) < 0) {
    fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
  }
    ifr.ifr_data = (char *)&ifdata;

  e->op = ETHSWJUMBO;
  e->type = TYPE_SET;
  e->val = *ctrlValPtr;
  e->unit = unit;
  e->port = port;

  if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
    fprintf(stderr, "ioctl command return error!\n");
  }

  *ctrlValPtr = e->ret_val;

  close(skfd);
  return err;
}

/*
 * Function:
 *  bcm_port_jumbo_control_get
 * Description:
 *  Get jumbo accept/reject status of selected port(s)
 * Parameters:
 *  port - Port number 9(ALL), 6(USB), 4(GPON_SERDES), 3(GMII_2), 2(GMII_1), 1(GPHY_1), 0(GPHY_0)
 *  ctrlValPtr - pointer to result
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_port_jumbo_control_get(int unit, bcm_port_t port, int *ctrlValPtr) // bill
{
  int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

  if ((skfd=ethswctl_init(&ifr)) < 0) {
    fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
  }
    ifr.ifr_data = (char *)&ifdata;

  e->op = ETHSWJUMBO;
  e->type = TYPE_GET;
  e->unit = unit;
  e->port = port;

  if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
    fprintf(stderr, "ioctl command return error!\n");
  }

  *ctrlValPtr = e->ret_val;

  close(skfd);
  return err;
}

int linux_user_mdio_read(void *dvc, unsigned int addr, unsigned char *data, unsigned int len)
{
    return bcm_pseudo_mdio_read(addr, (char *)data, (int)len);
}

int linux_user_mdio_write(void *dvc, unsigned int addr, const unsigned char *data, unsigned int len)
{
    return bcm_pseudo_mdio_write(addr, (char *)data, (int)len);
}

int linux_user_ubus_read(void *dvc, unsigned int addr, unsigned char *data, unsigned int len)
{
    return bcm_reg_read(addr, (char *)data, (int)len);
}

int linux_user_ubus_write(void *dvc, unsigned int addr, const unsigned char *data, unsigned int len)
{
    return bcm_reg_write(addr, (char *)data, (int)len);
}

int linux_user_mmap_read(void *dvc, unsigned int addr, unsigned char *data, unsigned int len)
{
    return bcm_reg_read_X(SF2_ETHSWCTL_UNIT, addr, (char *)data, (int)len); /* FIXME - the unit should come from the caller */
}

int linux_user_mmap_write(void *dvc, unsigned int addr, const unsigned char *data, unsigned int len)
{
    return bcm_reg_write_X(SF2_ETHSWCTL_UNIT, addr, (char *)data, (int)len); /* FIXME - the unit should come from the caller */
}

/* Get the Phy Config from Board Params */
int bcm_phy_config_get(int unit, bcm_port_t port, int *phy_config)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
          return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPHYCFG;
    e->type = TYPE_GET;
    e->port = port;
    e->val = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
      fprintf(stderr, "ioctl command return error!\n");
    }

    if (e->ret_val == -1) {
        err = -1;
    } else {
        *phy_config = e->phycfg;
    }

    close(skfd);
    return err;
}

int bcm_phy_mode_getV(char *ifname, int *speed, int *duplex)
{
    return bcm_phy_mode_get_by_name(ifname, speed, duplex, 0);
}

int bcm_phy_mode_setV(char *ifname, int speed, int duplex)
{
    return bcm_phy_mode_set_by_name(ifname, speed, duplex, 0);
}

/**
 * @brief scans the static link speed container (sbi_list) either from bottom to up, 
 * or from up to bottom to get lowest or highest link speed respectively
 * @note Base assumption: sbi_list is sorted in non-descending order by .link_speed field
 * @note Since original ethtool ETHTOOL_GLINKSETTINGS ioctl (for eth# interfaces) returns a list of all supported link speeds,
 *  we'll need to pick up the highest one from it for LAN interfaces.
 *  However, when dealing with WAN interfaces (e/gpon) there is no such a thing as a 'list-of-link-speeds', there is always a pair of up & donw link speed, 
 *  where the matter of interest would be up stream only which happens to be a lower (or equal in case of symmetric PON) one in the pair.
 *  Therefore, while dealing with eth# interfaces we'd start scanning sbi_list array from the top to the bottom to get the highest link speed, 
 *  when processing e/gpon interface we'd do the opposite i.e. start scanning the array from bottom to top to get the lowest link speed from the pair
 * @param[i] iface network interface OS name
 * @param[i] is_max_speed_caps enables to decide whether to return the lowest (wan asymmetric interace) or the highest (either wan/lan ethernet) link speed
 * @param[i] sbi_list 
 * @param[i] length 
 * @param[i] ecmd 
 * @param[o] link_speed 
 * @return int 
 */
static int find_link_speed(const char *iface,
                           const BOOL is_max_speed_caps,
                           const ethtool_link_speed_t *sbi_list, 
                           const unsigned length, 
                           const ecmd_t *ecmd, 
                           uint32_t *link_speed)
{
    typedef int  (*promote_idx_t)     (int);
    typedef BOOL (*is_not_end_cond_t) (int, int);

    /* scan from bottom to top callbacks */
    int  inc_idx          (int idx)               { return idx + 1; }
    BOOL is_not_end_up    (int idx, int end_cond) { return (idx < end_cond); }

    /* scan from top to bottom callbacks */
    int  dec_idx          (int idx)               { return idx - 1; }
    BOOL is_not_end_down  (int idx, int end_cond) { return (idx >= end_cond); }

    int j, idx, idx_end, rc = -1;
    promote_idx_t promote_idx_cb;
    is_not_end_cond_t not_end_cond_cb;

    if (FALSE == is_max_speed_caps)
    {

        idx             = 0;
        idx_end         = (int)length;
        promote_idx_cb  = inc_idx;
        not_end_cond_cb = is_not_end_up;
    } 
    else
    {

        idx             = (int)(length - 1);
        idx_end         = 0;
        promote_idx_cb  = dec_idx;
        not_end_cond_cb = is_not_end_down;
    }

    for (; (-1 == rc) && (TRUE == not_end_cond_cb(idx, idx_end)); idx = promote_idx_cb(idx)) /* one match is enough*/
    {
        for (j = 0; (-1 == rc) && (j < sbi_list[idx].count); j++) /* one match is enough*/
        {
            if ((BIT_IDX_2_WORD_IDX(sbi_list[idx].ethtool_speed_type[j])) >= ecmd->req.link_mode_masks_nwords)
            {
                fprintf(stderr,"sbi_list[%u].ethtool_speed_type[%u] exceeds boundaries of data storage %u\n",
                        idx, j, ecmd->req.link_mode_masks_nwords);
            }
            else if (0 != (ecmd->link_mode_data[BIT_IDX_2_WORD_IDX(sbi_list[idx].ethtool_speed_type[j])] &
                           BIT_IDX_2_BIT32_VAL(sbi_list[idx].ethtool_speed_type[j])))
            {
               *link_speed = sbi_list[idx].link_speed;
               rc = 0;
            }        
        }
    }

    return rc;
}

/*
 * Function:
 *    bcm_phy_speed_get
 * Description:
 *  This function tests ethtool speeds list against network interface capabilities bitmap aka 'supported'
 * Parameters:
 *    iface             (IN)  network interface OS name.
 *    is_max_speed_caps (IN) enables to decide whether to return the lowest (wan asymmetric interace) or the highest (either wan/lan ethernet) link speed
 *    link_speed        (OUT) network interface highest supported speed (Mbps)
 * Returns:
 *    BOOL TRUE on success, FALSE otherwise
 */
BOOL bcm_phy_speed_get(const char *iface, const BOOL is_max_speed_caps, uint32_t *link_speed)
{

    int fd, rc = -1;//Assuming failure

    errno = 0;
    if (NULL == iface)
    {
        fprintf(stderr,"Interface name is NULL\n");
    }
    else if (0 > (fd = socket(AF_INET, SOCK_DGRAM, 0)))// Open control socket.
    {
        fprintf(stderr,"Failed to open %s control socket, reason:%s\n", iface, strerror(errno));
    }
    else
    {
        ecmd_t ecmd;
        struct ifreq ifr;

        memset(&ecmd, 0x00, sizeof(ecmd));
        memset(&ifr, 0x00, sizeof(struct ifreq));
        ecmd.req.cmd = ETHTOOL_GLINKSETTINGS;
        strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
        ifr.ifr_data = (typeof(ifr.ifr_data))&ecmd;
        errno = 0;
        if ((0 > (rc = ioctl(fd, SIOCETHTOOL, &ifr))) || 
            (0 <= ecmd.req.link_mode_masks_nwords)    || 
            (ETHTOOL_GLINKSETTINGS != ecmd.req.cmd)) 
        {
            fprintf(stderr,"Failed to ioctl %s, reason: %s\n", iface, strerror(errno));
        }
        else//Success path, test the iface capability/ies
        {
            ecmd.req.cmd = ETHTOOL_GLINKSETTINGS;
            ecmd.req.link_mode_masks_nwords = -ecmd.req.link_mode_masks_nwords;
            if ((0 > (rc = ioctl(fd, SIOCETHTOOL, &ifr))) || 
                (0 >= ecmd.req.link_mode_masks_nwords)    || 
                (ETHTOOL_GLINKSETTINGS != ecmd.req.cmd))
            {
                fprintf(stderr,"Failed to ioctl %s, reason: %s\n", iface, strerror(errno));
            }
            else
            {
                rc = find_link_speed(iface, is_max_speed_caps, ethtool_speeds, __TS_LAST, &ecmd, link_speed);
            }
        }
        close(fd);
    }

    return (rc == 0);
}

/*
 * Function:
 *    bcm_phy_mode_set
 * Description:
 *  Set phy mode
 * Parameters:
 *    unit - RoboSwitch PCI device unit number (driver internal).
 *    port - port.
 *    speed - 0 is auto or 10, 100, 1000, 2500, 5000, 10000
 *    duplex - 0: half, 1:full
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_phy_mode_set(int unit, int port, int speed, int duplex)
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_phy_mode_set_by_name(obj_name, speed, duplex, 1);
}

/*
 * Function:
 *    bcm_phy_mode_set_by_name
 * Description:
 *  Set phy mode
 * Parameters:
 *    ifname - name of network interface or port object name
 *    speed - 0 is auto or 10, 100, 1000, 2500, 5000, 10000
 *    duplex - 0: half, 1:full
 *    is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_phy_mode_set_by_name(const char *ifname, int speed, int duplex, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPHYMODE;
    e->type = TYPE_SET;
    e->speed = speed;
    e->duplex = duplex;

    if (e->advPhyCaps == 0) {    
        e->advPhyCaps = phy_speed_mbps_to_cap(speed, duplex);
    }

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_phy_mode_get
 * Description:
 *  Get phy mode
 * Parameters:
 *    port - port.
 * Returns:
 *    speed - 0 is auto or 10, 100, 1000, 2500, 5000, 10000
 *    duplex - 0: full, 1:half
 *    BCM_E_NONE - Success.
 */
int bcm_phy_mode_get(int unit, int port, int *speed, int *duplex)
{
    char obj_name[OBJIFNAMSIZ];

    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }

    return bcm_phy_mode_get_by_name(obj_name, speed, duplex, 1);
}

/*
 * Function:
 *    bcm_phy_mode_get_by_name
 * Description:
 *  Get phy mode
 * Parameters:
 *    ifname - name of network interface or port object name
 *    is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *    speed - 0 is auto or 10, 100, 1000, 2500, 5000, 10000
 *    duplex - 0: full, 1:half
 *    BCM_E_NONE - Success.
 */
int bcm_phy_mode_get_by_name(const char *ifname, int *speed, int *duplex, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPHYMODE;
    e->type = TYPE_GET;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

    if (e->ret_val != -1) {
        *speed = e->speed;
        *duplex = e->duplex;
    }
out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_ifname_get
 * Description:
 *  Get Linux interface name for a given port, unit
 * Parameters:
 *    port - port.
 * Returns:
*     name of interface associated with the port
 *    BCM_E_NONE - Success.
 */
int bcm_ifname_get(int unit, int port, char *name)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWGETIFNAME;
    e->type = TYPE_GET;
    e->port = port;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

    if (e->ret_val != -1) {
       strcpy(name, e->ifname);
    }
out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_port_obj_name_get
 * Description:
 *  Get port object name for a given port, unit
 *
 * Parameters:
 *    port - port.
 * Returns:
*     name of interface associated with the port
 *    BCM_E_NONE - Success.
 */
int bcm_port_obj_name_get(int unit, int port, char *name)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    ifdata.op = ETHSWGETOBJNAME;
    ifdata.type = TYPE_GET;
    ifdata.port = port;
    ifdata.unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "ioctl command return error!\n");
        goto out;
    }

    if (ifdata.ret_val != -1) {
       strcpy(name, ifdata.ifname);
    }
out:
    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_multiport_set
 * Description:
 *  Set Multicast address in Switch Multiport register
 * Parameters:
 *  unit - RoboSwitch PCI device unit number (set to 0)
 *  mac  - MAC address to be added to second multiport register
 * Returns:
 *  BCM_E_NONE - Success.
 */
int bcm_multiport_set(int unit, unsigned char *mac)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_SET;
    e->op = ETHSWMULTIPORT;
    memcpy(e->mac, mac, 6);
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

int bcm_dos_ctrl_set(int unit, struct bcm_dos_ctrl_params *pDosCtrlParams)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_SET;
    e->op = ETHSWDOSCTRL;
	e->unit = unit;
	memset(&(e->dosCtrl),0,sizeof(e->dosCtrl));
	/* Taking short-cut : Not following BCM coding guidelines */
	if (pDosCtrlParams->da_eq_sa_drop_en) e->dosCtrl.da_eq_sa_drop_en = 1;
	if (pDosCtrlParams->ip_lan_drop_en) e->dosCtrl.ip_lan_drop_en = 1;
	if (pDosCtrlParams->tcp_blat_drop_en) e->dosCtrl.tcp_blat_drop_en = 1;
	if (pDosCtrlParams->udp_blat_drop_en) e->dosCtrl.udp_blat_drop_en = 1;
	if (pDosCtrlParams->tcp_null_scan_drop_en) e->dosCtrl.tcp_null_scan_drop_en = 1;
	if (pDosCtrlParams->tcp_xmas_scan_drop_en) e->dosCtrl.tcp_xmas_scan_drop_en = 1;
	if (pDosCtrlParams->tcp_synfin_scan_drop_en) e->dosCtrl.tcp_synfin_scan_drop_en = 1;
	if (pDosCtrlParams->tcp_synerr_drop_en) e->dosCtrl.tcp_synerr_drop_en = 1;
	if (pDosCtrlParams->tcp_shorthdr_drop_en) e->dosCtrl.tcp_shorthdr_drop_en = 1;
	if (pDosCtrlParams->tcp_fragerr_drop_en) e->dosCtrl.tcp_fragerr_drop_en = 1;
	if (pDosCtrlParams->icmpv4_frag_drop_en) e->dosCtrl.icmpv4_frag_drop_en = 1;
	if (pDosCtrlParams->icmpv6_frag_drop_en) e->dosCtrl.icmpv6_frag_drop_en = 1;
	if (pDosCtrlParams->icmpv4_longping_drop_en) e->dosCtrl.icmpv4_longping_drop_en = 1;
	if (pDosCtrlParams->icmpv6_longping_drop_en) e->dosCtrl.icmpv6_longping_drop_en = 1;
	if (pDosCtrlParams->dos_disable_lrn) e->dosCtrl.dos_disable_lrn = 1;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}
int bcm_dos_ctrl_get(int unit, struct bcm_dos_ctrl_params *pDosCtrlParams)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0)
    {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_GET;
    e->op = ETHSWDOSCTRL;
    e->unit = unit;
    memset(&(e->dosCtrl),0,sizeof(e->dosCtrl));

    if ((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL))
    {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }
    /* Taking short-cut : Not following BCM coding guidelines */
    if (e->dosCtrl.da_eq_sa_drop_en) pDosCtrlParams->da_eq_sa_drop_en = 1;
    if (e->dosCtrl.ip_lan_drop_en) pDosCtrlParams->ip_lan_drop_en = 1;
    if (e->dosCtrl.tcp_blat_drop_en) pDosCtrlParams->tcp_blat_drop_en = 1;
    if (e->dosCtrl.udp_blat_drop_en) pDosCtrlParams->udp_blat_drop_en = 1;
    if (e->dosCtrl.tcp_null_scan_drop_en) pDosCtrlParams->tcp_null_scan_drop_en = 1;
    if (e->dosCtrl.tcp_xmas_scan_drop_en) pDosCtrlParams->tcp_xmas_scan_drop_en = 1;
    if (e->dosCtrl.tcp_synfin_scan_drop_en) pDosCtrlParams->tcp_synfin_scan_drop_en = 1;
    if (e->dosCtrl.tcp_synerr_drop_en) pDosCtrlParams->tcp_synerr_drop_en = 1;
    if (e->dosCtrl.tcp_shorthdr_drop_en) pDosCtrlParams->tcp_shorthdr_drop_en = 1;
    if (e->dosCtrl.tcp_fragerr_drop_en) pDosCtrlParams->tcp_fragerr_drop_en = 1;
    if (e->dosCtrl.icmpv4_frag_drop_en) pDosCtrlParams->icmpv4_frag_drop_en = 1;
    if (e->dosCtrl.icmpv6_frag_drop_en) pDosCtrlParams->icmpv6_frag_drop_en = 1;
    if (e->dosCtrl.icmpv4_longping_drop_en) pDosCtrlParams->icmpv4_longping_drop_en = 1;
    if (e->dosCtrl.icmpv6_longping_drop_en) pDosCtrlParams->icmpv6_longping_drop_en = 1;
    if (e->dosCtrl.dos_disable_lrn) pDosCtrlParams->dos_disable_lrn = 1;

    out:
    close(skfd);
    return err;
}

int bcm_snoop_ctrl_set(int unit, struct bcm_snoop_ctrl_params *pSnoopCtrlParams)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_SET;
    e->op = ETHSWSNOOPCTRL;
	e->unit = unit;
	memset(&(e->snoopCtrl),0,sizeof(e->snoopCtrl));
	e->snoopCtrl.val = pSnoopCtrlParams->val;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL)) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }

out:
    close(skfd);
    return err;
}

int bcm_snoop_ctrl_get(int unit, struct bcm_snoop_ctrl_params *pSnoopCtrlParams)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0)
    {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_GET;
    e->op = ETHSWSNOOPCTRL;
    e->unit = unit;
    memset(&(e->snoopCtrl),0,sizeof(e->snoopCtrl));
    e->snoopCtrl.usz = pSnoopCtrlParams->usz;
    e->snoopCtrl.uptr = pSnoopCtrlParams->uptr;

    if ((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL))
    {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }
    pSnoopCtrlParams->val = e->snoopCtrl.val;
    out:
    close(skfd);
    return err;
}

int bcm_snoop_ctrl_help(int unit, struct bcm_snoop_ctrl_params *pSnoopCtrlParams)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0)
    {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_HELP;
    e->op = ETHSWSNOOPCTRL;
    e->unit = unit;
    memset(&(e->snoopCtrl),0,sizeof(e->snoopCtrl));
    e->snoopCtrl.usz = pSnoopCtrlParams->usz;
    e->snoopCtrl.uptr = pSnoopCtrlParams->uptr;

    if ((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL))
    {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
        goto out;
    }
    out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_enet_driver_wan_interface_set
 * Description:
 *  Set  port wan mode
 * Parameters:
 *    ifname - interface name
 *    val    - boolean, wan or no wan mode.
 * Returns:
 *    BCM_E_NONE - Success.
 */

/* Set Clear an ethernet interface as wan port */
int bcm_enet_driver_wan_interface_set(char *ifname, unsigned int val)
{
    int skfd;
    struct ifreq ifr;

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if ((skfd=ethswctl_open_socket(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_open_socket failed. \n");
        return -1;
    }
    ifr.ifr_data = (void *)val;
    if (ioctl(skfd, SIOCSWANPORT, &ifr) < 0)
    {
         fprintf(stderr, " %s ioctl SIOCSWANPORT(%04x) %s returns error! (%d/%s)",
                      __FUNCTION__, SIOCSWANPORT,  ifr.ifr_name, errno, strerror(errno));
         close(skfd);
         return -1;
    }
    close(skfd);
    return 0;
}


// This is an internal generic helper function, see ethswctl_api.h for the
// user visible functions.
int bcm_enet_driver_get_port_list_name(char *ifname, unsigned int sz, int ioctlVal, char *ioctlName)
{
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    memset((void *) &ifr, 0, sizeof(ifr));
    memset((void *) &ifdata, 0, sizeof(ifdata));

    if (ioctlVal == SIOCGPORTWANONLY)
    {
        fprintf(stderr, "bcm_enet_driver_get_port_wan_only is obsolete, call bcm_enet_driver_getWANOnlyEthPortIfNameList\n");
        return -1;
    }
    else if (ioctlVal == SIOCGPORTWANPREFERRED)
    {
        fprintf(stderr, "bcm_enet_driver_get_port_wan_preferred is obsolete, call bcm_enet_driver_getLanWanPortIfNameList\n");
        return -1;
    }
    else if (ioctlVal == SIOCGPORTLANONLY)
    {
        fprintf(stderr, "bcm_enet_driver_get_port_lan_only is obsolete, call bcm_enet_driver_getLANOnlyEthPortIfNameList\n");
        return -1;
    }
    
    /*
     * after changing other callers of SIOCGWANPORT ioctl,
     * pass sz for buffer length and enforce no buffer overruns.
     */
    if (!ifname) {
        return -1;
    }
    strcpy(ifr.ifr_name, BASE_IF_NAME);
    if ((skfd=ethswctl_open_socket(&ifr)) < 0) {
        fprintf(stderr, "%s: ethswctl_open_socket failed. \n", __FUNCTION__);
        return -1;
    }

    ifr.ifr_data = &ifdata;
    e->up_len.uptr = ifname;
    e->up_len.len  = sz;
    if (ioctl(skfd, ioctlVal, &ifr) < 0)
    {
         fprintf(stderr, "%s ioctl %s returns error!", __FUNCTION__, ioctlName);
         close(skfd);
         return -1;
    }

    close(skfd);
    return 0;
}

/*! 
\brief      Check if two interfaces can be bonded. [OP-::ETHSWBONDCHK CLI-::CMD_bondchk]
\details    If interfaces can't be bonded print error reason in provided buffer.
\param[in]      ifname1 interface1
\param[in]      ifname2 interface2
\param[in,out]  errstr  user space preallocated buffer for kernel to fill in error message (256 bytes)
\param[in]      sz      errstr buf size
\return     0 on success; a negative value on failure
*/
int bcm_enet_driver_bonding_check(char *ifname1, char *ifname2,  char *errstr, unsigned int sz)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0)
    {
        fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->type = TYPE_GET;
    e->op = ETHSWBONDCHK;
    strncpy(e->bond_chk.ifname1, ifname1, IFNAMSIZ);    e->bond_chk.ifname1[IFNAMSIZ - 1] = '\0';
    strncpy(e->bond_chk.ifname2, ifname2, IFNAMSIZ);    e->bond_chk.ifname2[IFNAMSIZ - 1] = '\0';
    e->bond_chk.uptr = errstr;
    e->bond_chk.len = sz;

    if ((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr)) && (err != BCM_E_UNAVAIL))
    {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
    }

    close(skfd);
    return e->bond_chk.len;
}

#ifndef DESKTOP_LINUX


// NOTE: there is another copy of this function in bcm_ethswutils.c.  Need to keep these copies in sync.
static int getWanLanAttrList(int opcode, char *ifNameBuf, unsigned int sz, int *count)
{
   int skfd;
   struct ifreq ifr;
   struct ifreq_ext ifx;
   char nameList[(MAX_SYS_ETH_PORT * (IFNAMSIZ + 1)) + 2] = {0};

   memset((void *) &ifr, 0, sizeof(ifr));
   memset((void *) &ifx, 0, sizeof(ifx));

   if ((skfd = ethswctl_init(&ifr)) < 0)
   {
      return -1;
   }

   ifx.stringBuf = nameList;
   ifx.bufLen = sizeof(nameList);
   ifx.opcode = opcode;
   ifr.ifr_data = (void *)&ifx;
   if (ioctl(skfd, SIOCIFREQ_EXT, &ifr) < 0)
   {
      fprintf(stderr, "%s: SIOCIFREQ_EXT opcode=0x%x failed, errno=%d\n",
                      __FUNCTION__, opcode, errno);
      close(skfd);
      return -2;
   }

   if(count)
       *count = ifx.count;
	   
   close(skfd);

   // Copy returned data to user buf
   if (strlen(nameList) >= sz)
   {
      fprintf(stderr, "%s: provided buf %d too small for %s\n",
                      __FUNCTION__, sz, nameList);
      return -3;
   }
   strcpy(ifNameBuf, nameList);

   return 0;
}
#endif  // DESKTOP_LINUX

int bcm_enet_driver_getWANOnlyEthPortIfNameList(char *ifNameBuf, unsigned int sz)
{
#ifdef DESKTOP_LINUX
   strncpy(ifNameBuf, "eth0", sz-1);
   return 0;
#else
   return getWanLanAttrList(SIOCGPORTWANONLY, ifNameBuf, sz, NULL);
#endif
}

int bcm_enet_driver_getWanPreferredPortIfNameList(char *ifNameBuf, unsigned int sz)
{
#ifdef DESKTOP_LINUX
   strncpy(ifNameBuf, "eth1,eth3", sz-1);
   return 0;
#else
   return getWanLanAttrList(SIOCGPORTWANPREFERRED, ifNameBuf, sz, NULL);
#endif
}

int bcm_enet_driver_getLanWanPortIfNameList(char *ifNameBuf, unsigned int sz)
{
#ifdef DESKTOP_LINUX
   strncpy(ifNameBuf, "eth1,eth3", sz-1);
   return 0;
#else
   return getWanLanAttrList(SIOCGPORTLANWAN, ifNameBuf, sz, NULL);
#endif
}

int bcm_enet_driver_getLANOnlyEthPortIfNameList(char *ifNameBuf, unsigned int sz)
{
#ifdef DESKTOP_LINUX
   strncpy(ifNameBuf, "eth4", sz-1);
   return 0;
#else
   return getWanLanAttrList(SIOCGPORTLANONLY, ifNameBuf, sz, NULL);
#endif
}

/* Get the devname who has the port_cap of LANWAN or LANONLY */
int bcm_enet_driver_getLANEthPortIfNameList(char *ifNameBuf, unsigned int sz, int *count)
{
#ifdef DESKTOP_LINUX
   strncpy(ifNameBuf, "eth4", sz-1);
   return 0;
#else
   return getWanLanAttrList(SIOCGPORTLANALL, ifNameBuf, sz, count);
#endif
}

/* Utility function to get the port name for a given port index. */

#if defined(BRCM_XTM_UNI)
char *bcm_enet_util_get_lan_port_name(int portIdx)
{
    if (portIdx == 0)
    {
        return "ptm0";
    }
    else
    {
        fprintf(stderr, "No such port, portIdx %d\n", portIdx);
        return NULL;
    }
}
#else /* BRCM_XTM_UNI */
char *bcm_enet_util_get_lan_port_name(int portIdx)
{
    typedef struct
    {
        /* -1: to be initialized, 0: port not exist. 1: exist. */
        SINT32 flags;
        char ifName[IFNAMSIZ];
    } EthPortNameEntry;

    /* Locally cached data to avoid ioctl() calls for better performance. */
    static EthPortNameEntry ethLanPortNames[MAX_SYS_ETH_PORT];
    static UBOOL8 ethLanPortMappingDone = FALSE;
    char *ifNameP = NULL;

    /* First time access. */
    if (ethLanPortMappingDone == FALSE)
    {
        int portIdx;
        char ethLanList[MAX_SYS_ETH_PORT * IFNAMSIZ] = {0};
        int ethLanCount = 0;
        char *nextP = NULL;
        char *savedP = NULL;

        if ((bcm_enet_driver_getLANEthPortIfNameList(ethLanList,
          sizeof(ethLanList), &ethLanCount) != 0))
        {
            fprintf(stderr, "Failed to get LAN ports");
            return NULL;
        }

        for (portIdx = 0, nextP = strtok_r(ethLanList, ",", &savedP);
          (nextP != NULL) && (portIdx < ethLanCount);
          nextP = strtok_r(NULL, ",", &savedP), portIdx++)
        {
            snprintf(ethLanPortNames[portIdx].ifName, IFNAMSIZ, "%s", nextP);
            ethLanPortNames[portIdx].flags = 1;
        }

        ethLanPortMappingDone = TRUE;
    }

    if (portIdx >= MAX_SYS_ETH_PORT)
    {
        fprintf(stderr, "Invalid portIdx %d\n", portIdx);
        return NULL;
    }

    if (ethLanPortNames[portIdx].flags > 0)
    {
        ifNameP = ethLanPortNames[portIdx].ifName;
    }
    else
    {
        fprintf(stderr, "No such port, portIdx %d\n", portIdx);
    }

    return ifNameP;
}
#endif /* BRCM_XTM_UNI */

/*
 * Function:
 *    bcm_phy_autoneg_info_get
 * Description:
 *  Get autoneg info for port
 * Parameters:
 *    unit - unit
 *    port - port.
 * Returns:
 *    autoneg - 0 is disabled and 1 is enabled
 *    local_cap - local capability for port
 *    ad_cap  -  advertised capability for port
 *    BCM_E_NONE - Success.
 */

int bcm_phy_autoneg_info_get(int unit, int port, unsigned char *autoneg, unsigned short *local_cap, unsigned short* ad_cap)
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }
	
    return bcm_phy_autoneg_info_get_by_name(obj_name, autoneg, local_cap, ad_cap, 1);
}

/*
 * Function:
 *    bcm_phy_autoneg_info_get_by_name
 * Description:
 *  Get autoneg info for port
 * Parameters:
 *    ifname - interface name
 * Returns:
 *    autoneg - 0 is disabled and 1 is enabled
 *    local_cap - local capability for port
 *    ad_cap  -  advertised capability for port
 *    is_object - Indicate if ifname is name of network interface or port object name
 *    BCM_E_NONE - Success.
 */
int bcm_phy_autoneg_info_get_by_name(const char *ifname, unsigned char *autoneg, unsigned short *local_cap, unsigned short* ad_cap, int is_object)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	
    
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

	e->op = ETHSWPHYAUTONEG;
	e->type = TYPE_GET;
    
	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
		goto out;
	}

	if (e->ret_val != -1) {
		*local_cap = e->autoneg_local;
		*ad_cap = e->autoneg_ad;
		*autoneg = e->autoneg_info;
	}
out:
	close(skfd);
	return err;
}

/*
 * Function:
 *    bcm_phy_autoneg_info_set
 * Description:
 *  Set autoneg info for port
 * Parameters:
 *    unit - unit
 *    port - port.
 * Returns:
 *    autoneg - 0 is disabled and 1 is enabled
 *    BCM_E_NONE - Success.
 */
int bcm_phy_autoneg_info_set(int unit, int port, unsigned char autoneg)
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }
    
    return bcm_phy_autoneg_info_set_by_name(obj_name, autoneg, 1);
}


/*
 * Function:
 *    bcm_phy_autoneg_info_set_by_name
 * Description:
 *  Set autoneg info for port by name
 * Parameters:
 *    ifname - ifname.
 *    autoneg - 0 is disabled and 1 is enabled
 *    is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_phy_autoneg_info_set_by_name(const char *ifname, unsigned char autoneg, int is_object)
{
    int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

	e->op = ETHSWPHYAUTONEG;
	e->type = TYPE_SET;
	e->autoneg_info = autoneg;
	
	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
		goto out;
	}

out:
	close(skfd);
	return err;
}

/*
 * Function:
 *    bcm_phy_autoneg_cap_adv_set
 * Description:
 *  Set autoneg local capability
 * Parameters:
 *    unit - unit
 *    port - port.
 * Returns:
 *    autoneg - bit 0:  0  --- disable auto-negotiation
                               1  ---- enable auto-negotiation
                      bit 1:  1 --- restart auto-negotiation
 *    BCM_E_NONE - Success.
 */
int bcm_phy_autoneg_cap_adv_set(int unit, int port, unsigned char autoneg, unsigned short* ad_cap)
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }
    
    return bcm_phy_autoneg_cap_adv_set_by_name(obj_name, autoneg, ad_cap, 1);
}


/*
 * Function:
 *    bcm_phy_autoneg_cap_adv_set_by_name
 * Description:
 *  Set autoneg local capability
 * Parameters:
 *    ifname - ifname.
 *    autoneg - 0 is disabled and 1 is enabled
 *    is_object - Indicate if ifname is name of network interface or port object name
 * Returns:
 *    autoneg - bit 0:  0  --- disable auto-negotiation
                        1  ---- enable auto-negotiation
                bit 1:  1 --- restart auto-negotiation
 *    BCM_E_NONE - Success.
 */
int bcm_phy_autoneg_cap_adv_set_by_name(const char *ifname, unsigned char autoneg, unsigned short* ad_cap, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWPHYAUTONEGCAPADV;
    e->type = TYPE_SET;
    e->autoneg_info = autoneg;
    e->autoneg_ad = *ad_cap;
    
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

out:
    close(skfd);
    return err;
}



/*! 
\brief      Set auto mdix [OP-::ETHSWAUTOMDIX CLI- ]
\param[in]      unit    switch unit num
\param[in]      port    port num on switch unit
\param[in]      enable  enable/disable mdix
\return     0 on success; a negative value on failure
*/
int bcm_phy_force_auto_mdix_set(int unit, int port, int enable)
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }
    
    return bcm_phy_force_auto_mdix_set_by_name(obj_name, enable, 1);
}


/*! 
\brief      Set auto mdix by interface name
\param[in]      ifname    interface name
\param[in]      enable    enable/disable mdix
\param[in]      is_object Indicate if ifname is name of network interface or port object name
\return     0 on success; a negative value on failure
*/
int bcm_phy_force_auto_mdix_set_by_name(const char *ifname, int enable, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }

    e->op = ETHSWAUTOMDIX;
    e->type = TYPE_SET;
    e->val = enable;
	
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

out:
    close(skfd);
    return err;
}


/*! 
\brief      Get autoneg local capability [OP-::ETHSWAUTOMDIX CLI- ]
\param[in]      unit    switch unit num
\param[in]      port    port num on switch unit
\param[out]     enable  return enable/disable mdix
\return     0 on success; a negative value on failure
*/
int bcm_phy_force_auto_mdix_get(int unit, int port, int *enable)
{
    char obj_name[OBJIFNAMSIZ];
	
    if (bcm_port_obj_name_get(unit, port, obj_name) < 0) 
    {
        fprintf(stderr, "No such port, unit %d, port %d\n", unit, port);
        return -1;
    }
    
    return bcm_phy_force_auto_mdix_get_by_name(obj_name, enable, 1);
}

/*! 
\brief      Get autoneg local capability by interface name
\param[in]      ifname  interface name
\param[out]     enable  return enable/disable mdix
\param[in]      is_object Indicate if ifname is name of network interface or port object name
\return     0 on success; a negative value on failure
*/
int bcm_phy_force_auto_mdix_get_by_name(const char *ifname, int *enable, int is_object)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;
    
    memset(&ifdata, 0, sizeof(ifdata));
    if ((skfd=ethswctl_setup_ifreq(&ifr, e, ifname, is_object)) < 0) {
        fprintf(stderr, "ethswctl_setup_ifreq failed. %d (%s)\n", skfd, ifname);
        return skfd;
    }
    
    e->op = ETHSWAUTOMDIX;
    e->type = TYPE_GET;
	
    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        fprintf(stderr, "%s: ioctl command return error %d!\n", __FUNCTION__, err);
        goto out;
    }

	if (e->ret_val != -1)
    {
        *enable = e->val;
    }
    else 
    {
        *enable = -1; // -1 stands for not support
    }
    
out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_port_mirror_set/get
 * Description:
 *  Set/Get switch port mirroring configuration
 * Parameters:
 *    unit - unit
 *    enbl - enable(1)/disable(0)
 *    port - mirror port.
 *    ing_pmap - Port map of ingress mirror ports
 *    eg_pmap - Port map of egress mirror ports
 *    blk_no_mrr - switch should block all non-mirrored traffic towards mirror_port
 *    tx_port - TX packet mirror port, Optional - if not supplied all traffic is mirrored to "mirror_port"; Applicable only to Runner
 *    rx_port - RX packet mirror port, Optional - if not supplied all traffic is mirrored to "mirror_port"; Applicable only to Runner
 * Returns:
 *    0 - Success else failure.
 */
int bcm_port_mirror_set(int unit,int enbl,int port,unsigned int ing_pmap,
                        unsigned int eg_pmap, unsigned int blk_no_mrr,
                        int tx_port, int rx_port)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWMIRROR;
    e->type = TYPE_SET;
    e->unit = unit;
    e->port_mirror_cfg.enable = enbl;
    e->port_mirror_cfg.mirror_port = port;
    e->port_mirror_cfg.ing_pmap = ing_pmap;
    e->port_mirror_cfg.eg_pmap = eg_pmap;
    e->port_mirror_cfg.blk_no_mrr = blk_no_mrr;
    e->port_mirror_cfg.tx_port = tx_port;
    e->port_mirror_cfg.rx_port = rx_port;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

out:
    close(skfd);
    return err;
}
int bcm_port_mirror_get(int unit,int *enbl,int *port,unsigned int *ing_pmap,
                        unsigned int *eg_pmap, unsigned int *blk_no_mrr,
                        int *tx_port, int *rx_port)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWMIRROR;
    e->type = TYPE_GET;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
      goto out;
    }
    *enbl = e->port_mirror_cfg.enable;
    *port = e->port_mirror_cfg.mirror_port;
    *ing_pmap = e->port_mirror_cfg.ing_pmap;
    *eg_pmap = e->port_mirror_cfg.eg_pmap;
    *blk_no_mrr = e->port_mirror_cfg.blk_no_mrr;
    *tx_port = e->port_mirror_cfg.tx_port;
    *rx_port= e->port_mirror_cfg.rx_port;

out:
    close(skfd);
    return err;
}

/*
 * Function:
 *    bcm_port_trunk_set/get
 * Description:
 *  Set/Get switch port trunking configuration
 * Parameters:
 *    unit - unit
 *    enbl - enable(1)/disable(0)
 *    hash_sel - MAC hash selection criteria.
 *    grp0_pmap - Port map of group_0 trunk ports
 *    grp1_pmap - Port map of group_1 trunk ports
 * Returns:
 *    0 - Success else failure.
 */
int bcm_port_trunk_set(int unit,unsigned int hash_sel)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWPORTTRUNK;
    e->type = TYPE_SET;
    e->unit = unit;
    e->port_trunk_cfg.hash_sel = hash_sel;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }

out:
    close(skfd);
    return err;
}
int bcm_port_trunk_get(int unit,int *enbl,unsigned int *hash_sel,unsigned int *grp0_pmap,unsigned int *grp1_pmap)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op   = ETHSWPORTTRUNK;
    e->type = TYPE_GET;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }
    *enbl = e->port_trunk_cfg.enable;
    *hash_sel = e->port_trunk_cfg.hash_sel;
    *grp0_pmap = e->port_trunk_cfg.grp0_pmap;
    *grp1_pmap = e->port_trunk_cfg.grp1_pmap;

out:
    close(skfd);
    return err;
}

#if defined(HAS_SF2_CFP)
int bcm_cfp_op(cfpArg_t *cfpArg)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;
    memcpy(&e->cfpArgs, cfpArg, sizeof(e->cfpArgs));

    e->op   = ETHSWCFP;
    e->unit = cfpArg->unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
      goto out;
    }
    memcpy(cfpArg, &e->cfpArgs, sizeof(e->cfpArgs));

out:
    close(skfd);
    return err;
}
#endif //HAS_SF2_CFP

#if defined(SUPPORT_ETH_PWRSAVE)
/*
 * Function:
 *    bcm_phy_apd_get
 * Description:
 *    Gets global PHY auto-power-down setting
 * Parameters:
 *    none
 * Returns:
 *    apd_en - 0 disabled, 1 enabled
 *    BCM_E_NONE - Success.
 */
int bcm_phy_apd_get(unsigned int* apd_en)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

	if ((skfd=ethswctl_init(&ifr)) < 0) {
		fprintf(stderr, "ethswctl_init failed. \n");
		return skfd;
	}
	ifr.ifr_data = (char *)&ifdata;

	e->op = ETHSWPHYAPD;
	e->type = TYPE_GET;

	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "ioctl command return error!\n");
		goto out;
	}

	if (e->ret_val != -1) {
		*apd_en = e->val;
	}
out:
	close(skfd);
	return err;
}

/*
 * Function:
 *    bcm_phy_apd_set
 * Description:
 *    Globally sets auto-power-down on all PHYs
 * Parameters:
 *    apd_en - 0 disabled, 1 enabled
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_phy_apd_set(unsigned int apd_en)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

	if ((skfd=ethswctl_init(&ifr)) < 0) {
		fprintf(stderr, "ethswctl_init failed. \n");
		return skfd;
	}
	ifr.ifr_data = (char *)&ifdata;

	e->op = ETHSWPHYAPD;
	e->type = TYPE_SET;
	e->val = apd_en;

	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "ioctl command return error!\n");
		goto out;
	}

out:
	close(skfd);
	return err;
}
#endif

#if defined(SUPPORT_ENERGY_EFFICIENT_ETHERNET)
/*
 * Function:
 *    bcm_phy_eee_get
 * Description:
 *    Gets global PHY Energy Efficient Ethernet setting
 * Parameters:
 *    none
 * Returns:
 *    apd_en - 0 disabled, 1 enabled
 *    BCM_E_NONE - Success.
 */
int bcm_phy_eee_get(unsigned int* eee_en)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

	if ((skfd=ethswctl_init(&ifr)) < 0) {
		fprintf(stderr, "ethswctl_init failed. \n");
		return skfd;
	}
	ifr.ifr_data = (char *)&ifdata;

	e->op = ETHSWPHYEEE;
	e->type = TYPE_GET;

	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "ioctl command return error!\n");
		goto out;
	}

	if (e->ret_val != -1) {
		*eee_en = e->val;
	}
out:
	close(skfd);
	return err;
}

/*
 * Function:
 *    bcm_phy_eee_set
 * Description:
 *    Globally sets Energy Efficient Ethernet on all PHYs
 * Parameters:
 *    apd_en - 0 disabled, 1 enabled
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_phy_eee_set(unsigned int eee_en)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

	if ((skfd=ethswctl_init(&ifr)) < 0) {
		fprintf(stderr, "ethswctl_init failed. \n");
		return skfd;
	}
	ifr.ifr_data = (char *)&ifdata;

	e->op = ETHSWPHYEEE;
	e->type = TYPE_SET;
	e->val = eee_en;

	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "ioctl command return error!\n");
		goto out;
	}

out:
	close(skfd);
	return err;
}
#endif

#if defined(SUPPORT_ETH_DEEP_GREEN_MODE)
/*
 * Function:
 *    bcm_DeepGreenMode_get
 * Description:
 *    Gets the SF2 Switch Deep Green Mode setting
 * Parameters:
 *    dgm_en - 0 to get whether DGM Feature is enabled/disabled, 1 to get whether DGM Feature is activated/deactivated
 * Returns:
 *    dgm_en - 0 disabled/deactivated, 1 enabled/activated
 *    BCM_E_NONE - Success.
 */
int bcm_DeepGreenMode_get(unsigned int* dgm_en)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

	if ((skfd=ethswctl_init(&ifr)) < 0) {
		fprintf(stderr, "ethswctl_init failed. \n");
		return skfd;
	}
	ifr.ifr_data = (char *)&ifdata;

	e->op = ETHSWDEEPGREENMODE;
	e->type = TYPE_GET;
	e->val = *dgm_en;

	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "ioctl command return error!\n");
		goto out;
	}

	if (e->ret_val != -1) {
		*dgm_en = e->val;
	}
out:
	close(skfd);
	return err;
}

/*
 * Function:
 *    bcm_DeepGreenMode_set
 * Description:
 *    Enables/disables the SF2 Switch Deep Green Mode
 * Parameters:
 *    dgm_en - 0 disabled, 1 enabled
 * Returns:
 *    BCM_E_NONE - Success.
 */
int bcm_DeepGreenMode_set(unsigned int dgm_en)
{
	int skfd, err = 0;
	struct ifreq ifr;
	struct ethswctl_data ifdata;
	struct ethswctl_data *e = &ifdata;	

	if ((skfd=ethswctl_init(&ifr)) < 0) {
		fprintf(stderr, "ethswctl_init failed. \n");
		return skfd;
	}
	ifr.ifr_data = (char *)&ifdata;

	e->op = ETHSWDEEPGREENMODE;
	e->type = TYPE_SET;
	e->val = dgm_en;

	if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
		fprintf(stderr, "ioctl command return error!\n");
		goto out;
	}

out:
	close(skfd);
	return err;
}
#endif

#if defined(HAS_SF2)
/*
 * Function:
 *  bcm_port_storm_ctrl_set
 * Purpose:
 *  Set ingress storm control rate limiting parameters
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  pkt_msk = 0x1(unicast lookup hit), 0x2(multicast lookup hit), 0x4(reserved mac addr 01-80-c2-00-00- 00~2f)
 *            0x8(broadcast), 0x10(multicast lookup fail), 0x20(unicast lookup fail)
 *  rate = 1~28   (Bit rate = rate*8*1024/125, that is 64Kb~1.792Mb with resolution 64Kb)
 *         29~127 (Bit rate = (rate-27)1024, that is 2Mb~100Mb with resolution 1Mb)
 *         128~240 (Bit rate = (rate-115)*8*1024, that is 104Mb~1000Mb with resolution 8Mb)
 *  bucket_size = 0(4K), 1(8K), 2(16K), 3(32K), 4(64K), others(488K) bytes
 * Returns:
 *  BCM_E_XXX
 * Note :
 *  Robo Switch support 2 ingress buckets for different packet type. Only use bucket 0 here.
 *  PKT_MSK1 and PKT_MSK0 shouldn't have any overlaps on packet type selection. Otherwise, the accuracy of rate would be affected.
 */
int bcm_port_storm_ctrl_set(int unit,bcm_port_t port, int pkt_msk, unsigned int rate, unsigned int bucket_size)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTSTORMCTRL;
    e->port = port;
    e->type = TYPE_SET;
    e->unit = unit;
    e->pkt_type_mask = pkt_msk;
    e->limit = rate;
    e->burst_size = bucket_size;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

int bcm_port_storm_ctrl_get(int unit,bcm_port_t port,int *pkt_msk, unsigned int *rate,unsigned int *bucket_size)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWPORTSTORMCTRL;
    e->port = port;
    e->type = TYPE_GET;
    e->unit = unit;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }
    close(skfd);

    *pkt_msk = e->pkt_type_mask;
    *rate = e->limit;
    *bucket_size = e->burst_size;
    return err;
}

/*
 * Function:
 *  bcm_port_maclmt_set
 * Purpose:
 *  Set port/global learned MAC limit and actions
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  type = following are port specific types requiring valid port
 *          PORT_LIMIT_ENT              1-enable, 0-disable
 *          PORT_LIMIT                  0-4096 MACs
 *          PORT_ACTION                 action > limit: increment PORT_OVER_LIMIT_PKT_COUNT and 
 *                                      0-normal forward, 1-drop, 2-copy to cpu, 3-forward to cpu only
 *          PORT_RST_OVER_LIMIT_PKT_COUNT   clear port over limit packet counter
 *         following are global types port will be ignored
 *          GLOBAL_LIMIT                0-49096 MACs
 *          GLOBAL_RST_OVER_LIMIT_PKT_COUNT  clear all over limit packet counters
 * Returns:
 *  BCM_E_XXX
 */
int bcm_port_maclmt_set(int unit,bcm_port_t port,int type, int *val)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWMACLMT;
    e->port = port;
    e->type = TYPE_SET;
    e->unit = unit;
    e->sw_ctrl_type = type;
    e->val = *val;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }

    close(skfd);
    return err;
}

/*
 * Function:
 *  bcm_port_maclmt_get
 * Purpose:
 *  Get port/global learned MAC limit, actions, and stats.
 * Parameters:
 *  unit - Device number
 *  port - Port number
 *  type = following are port specific types requiring valid port
 *          PORT_LIMIT_ENT              1-enable, 0-disable
 *          PORT_LIMIT                  0-4096 MACs
 *          PORT_ACTION                 action > limit: increment PORT_OVER_LIMIT_PKT_COUNT and 
 *                                      0-normal forward, 1-drop, 2-copy to cpu, 3-forward to cpu only
 *          PORT_LEARNED_COUNT          
 *          PORT_OVER_LIMIT_PKT_COUNT   
 *         following are global types port will be ignored
 *          GLOBAL_LIMIT                0-49096 MACs
 *          GLOBAL_LEARNED_COUNT
 *
 * Returns:
 *  BCM_E_XXX
 */
int bcm_port_maclmt_get(int unit,bcm_port_t port,int type, int *val)
{
    int skfd, err = 0;
    struct ifreq ifr;
    struct ethswctl_data ifdata;
    struct ethswctl_data *e = &ifdata;

    if ((skfd=ethswctl_init(&ifr)) < 0) {
      fprintf(stderr, "ethswctl_init failed. \n");
        return skfd;
    }
    ifr.ifr_data = (char *)&ifdata;

    e->op = ETHSWMACLMT;
    e->port = port;
    e->type = TYPE_GET;
    e->unit = unit;
    e->sw_ctrl_type = type;
    e->val = *val;

    if((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr))) {
        if (err == -EOPNOTSUPP)
            fprintf(stderr, "ioctl not supported!\n");
        else
            fprintf(stderr, "ioctl command return error %d!\n", err);
    }
    *val = e->val;
    
    close(skfd);
    return err;
}
#endif //HAS_SF2

#if defined(HAS_RUNNER)
/*
 * Function:
 *    bcm_KeepAlive
 * Description:
 *    Check if runner is responsive
 * Parameters:
 *    timeout      - seconds to wait until declaring unresponsive
 * Returns:
 *    BCM_E_NONE - Success.
 *    ETIME      - Timed out without receiving looped back packet
 *    Other errors as defined in errno.h
 */
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
int bcm_KeepAlive(unsigned int timeout)
{
	int rc, fd_idx, fd_send, fd_recv;
	struct ifreq ifr;
	struct sockaddr_ll addr_send, addr_recv;
	struct timeval tv;
	fd_set readFds;
	socklen_t addr_len = sizeof(struct sockaddr_ll);
	int pktlen = 255;
	char buf_send[1500];
	char buf_recv[1500];
	int i;

	for (i=0; i<pktlen; i++)
		buf_send[i] = i;

	/* Get loopback device */
	if ((fd_idx = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return errno;
	}

	strcpy(ifr.ifr_name, "bcmswlpbk0");

	if (ioctl(fd_idx, SIOCGIFFLAGS, &ifr) < 0) {
		close(fd_idx);
		return errno;
	}

	if ((ifr.ifr_flags & IFF_UP) == 0)
	{
		close(fd_idx);
		return -ENETDOWN;
	}

	if ((ifr.ifr_flags & IFF_RUNNING) == 0)
	{
		close(fd_idx);
		return -ENOLINK;
	}

	if (ioctl(fd_idx, SIOCGIFINDEX, &ifr) < 0 ) {
		close(fd_idx);
		return errno;
	}
	close(fd_idx);

	/* Open raw sockets */
	if ((fd_send = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		return errno;
	}

	if ((fd_recv = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		close(fd_send);
		return errno;
	}

	/* Send packet */
	addr_send.sll_ifindex = ifr.ifr_ifindex;
	addr_send.sll_pkttype = PACKET_OUTGOING;
	addr_send.sll_halen = ETH_ALEN;
	memset(&addr_send.sll_addr, 0, ETH_ALEN);

	if (sendto(fd_send, buf_send, pktlen, 0, (struct sockaddr*)&addr_send, (int)addr_len) < 0) {
		close(fd_send);
		close(fd_recv);
		return errno;
	}
	close(fd_send);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	/* Receive packet */
	do
	{
		FD_ZERO(&readFds);
		FD_SET(fd_recv, &readFds);
		rc = select(fd_recv + 1, &readFds, NULL, NULL, &tv);
		switch (rc) {
			case -1:
				close(fd_recv);
				return errno;
			case 0:
				close(fd_recv);
				return -ETIME;
			default:
				if (recvfrom(fd_recv, buf_recv, pktlen, 0,
							(struct sockaddr*)&addr_recv, &addr_len) <= 0) {
					close(fd_recv);
					return errno;
				}
				if (!memcmp(buf_recv, buf_send, pktlen) &&
					addr_recv.sll_pkttype != PACKET_OUTGOING) {
					close(fd_recv);
					return BCM_E_NONE;
				}
		}
	} while (tv.tv_sec || tv.tv_usec);

	close(fd_recv);
	return -ETIME;
}
#endif //HAS_RUNNER

#ifdef DESKTOP_LINUX

static int fake_ethSwCtlOps(void *data)
{
    struct ifreq *ifr = (struct ifreq*)data;
    struct ethswctl_data *e = (ifr != NULL) ? ifr->ifr_data : NULL;
    int ret = 0;

    if (e == NULL)
    {
        return -EINVAL;
    }

#if 1

    switch (e->op)
    {
        case ETHSWGETIFNAME:
        case ETHSWGETOBJNAME:
            if (e->type == TYPE_GET)
            {
#ifndef G9991_COMMON
                sprintf(e->ifname, "eth%d", e->port);
#else
                sprintf(e->ifname, "sid%d", e->port);
#endif
            }
            break;
        default:
            /* Ignore silently. */
            break;
    }
#endif

    return ret;
}

static int fake_ethsw_ioctl(int fd __attribute__((unused)),
                            int cmd,
                            void *data)
{
    int ret = 0;

    switch (cmd)
    {
        case SIOCETHSWCTLOPS:
            ret = fake_ethSwCtlOps(data);
            break;
        default:
            fprintf(stderr, "Unsupported cmd %u\n", cmd);
            ret = -EOPNOTSUPP;
            break;
    }

    return ret;
}
#endif

void port_to_Intf(int port, char *intf)
{
    snprintf(intf, 6, "eth%u", port);
}

/* get interface statistics based on device name.  struct with long longs, allocated by caller
   Basic Statistics                                                                                     |   Extended Statistics
Inter-|   Receive                                       |  Transmit                                     |multicast           |unicast    |broadcast  |unkn
 face |  bytes    pckts errs drop fifo frame  comp multi|  bytes    pckts errs drop fifo coll carr  comp|txpckt rxbyte txbyte|   rx    tx|   rx    tx|rxerr
eth4.0:     632       7    0    0    0     0     0     0    23066     323    0  996    0    0    0     0      0      0      0     7   323     0     0     0
*/

#define BUFLEN_512      512

int getIntfStats(const char *devName, struct IntfStats_s *s)
{
    int devNameLen, ret = -1;
    char *pcDevNameStart;
    FILE *fs;
    char line[BUFLEN_512];

    if (devName == NULL)
    {
        return -2;
    }
    memset(s, 0, sizeof(*s));
    devNameLen = strlen(devName);

    fs = fopen("/proc/net/dev_extstats", "r");
    if (fs == NULL)
    {
        return -3;
    }

    while (fgets(line, sizeof(line), fs))
    {
        pcDevNameStart = strstr(line, devName);
        if ( (pcDevNameStart != NULL) && (*(pcDevNameStart + devNameLen) == ':') )
        {
            sscanf(pcDevNameStart + devNameLen + 1,
                " %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                &s->byteRx, &s->packetRx, &s->packetErrRx, &s->packetDropRx, &s->packetFifoRx, &s->packetFrameRx, &s->packetCompRx, &s->packetMultiRx,
                &s->byteTx, &s->packetTx, &s->packetErrTx, &s->packetDropTx, &s->packetFifoTx, &s->packetCollTx, &s->packetCarrTx, &s->packetCompTx,
                &s->packetMultiTx, &s->byteMultiRx, &s->byteMultiTx, &s->packetUniRx, &s->packetUniTx, &s->packetBcastRx, &s->packetBcastTx, &s->packetUnknownerrRx);
            ret = 0;
            break;
        }
    }
    fclose(fs);
    return ret;
}

static int bcm_port(int is_add, struct net_port_t *net_port)
{
    struct ifreq ifr;
    int err, skfd;
    struct ethswctl_data ethswctl = {};

    memcpy(&ethswctl.net_port, net_port, sizeof(net_port_t));

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("socket open error\n");
        return -1;
    }

    strcpy(ifr.ifr_name, BASE_IF_NAME);
    if ((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCGIFINDEX, &ifr)) < 0 )
    {
        printf("%s interface does not exist\n", BASE_IF_NAME);
        goto exit;
    }

    ifr.ifr_data = (void *)&ethswctl;
    ethswctl.op = is_add ? ETHSWPORTCREATE : ETHSWPORTDELETE;
    err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr);
    if (err < 0 )
        printf("Error %d bcmenet port\n", err);

    if (is_add)
        strncpy(net_port->ifname, ethswctl.net_port.ifname, IFNAMSIZ);

exit:
    close(skfd);
    return err;
}

/*! 
\brief      Create dynamic xPON port. [OP-::ETHSWPORTCREATE CLI-::CMD_createport]
\details    ethswctl -c createport -p \<port\> -w \<is_wan\> -i \<name\> \n 
            Note: for EPON, GPON dynamic port creation
\param[in]  net_port net_port_t struct contains dynamic port configuration parameters
\return     0 on success; a negative value on failure
*/
int bcm_port_create(struct net_port_t *net_port)
{
    return bcm_port(1, net_port);
}

/*! 
\brief      Delete dynamic xPON port. [OP-::ETHSWPORTDELETE CLI-::CMD_deleteport]
\details    ethswctl -c deleteport -p \<port\> \n 
            Note: for EPON, GPON dynamic port delete
\param[in]  port port number
\return     0 on success; a negative value on failure
*/
int bcm_port_delete(int port)
{
    struct net_port_t net_port =
    {
        .port = port,
    };

    return bcm_port(0, &net_port);
}

/*! 
\brief      Set GPON GEM multicast index. [OP-::ETHSWPORTMCASTGEMSET CLI-::CMD_gponmcastgemset]
\details    ethswctl -c gponmcastgemset -p \<mcast_gem_idx\> \n 
\param[in]  mcast_idx port number
\return     0 on success; a negative value on failure
*/
int ethswctl_gpon_mcast_gem_set(int mcast_idx)
{
    struct ifreq ifr;
    int err, skfd;
    struct ethswctl_data ethswctl = {};

    ethswctl.val = mcast_idx;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("socket open error\n");
        return -1;
    }

    strcpy(ifr.ifr_name, BASE_IF_NAME);
    if ((err = ETHSW_IOCTL_WRAPPER(skfd, SIOCGIFINDEX, &ifr)) < 0 )
    {
        printf("%s interface does not exist\n", BASE_IF_NAME);
        goto exit;
    }

    ifr.ifr_data = (void *)&ethswctl;
    ethswctl.op = ETHSWPORTMCASTGEMSET;
    err = ETHSW_IOCTL_WRAPPER(skfd, SIOCETHSWCTLOPS, &ifr);
    if (err < 0 )
        printf("Error %d bcmenet gpon_mcast_gem_set\n", mcast_idx);

exit:
    close(skfd);
    return err;
}

/*! @} end of ethswctl group */
