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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "os_defs.h"

#include "bcmtypes.h"
#include "bcmnet.h"
#include "rdpa_types.h"
#include "rdpa_drv.h"
#include "rdpactl_api.h"

/*
 * Macros
 */

#define RDPACTL_IOCTL_FILE_NAME "/dev/bcmrdpa"

/* #define CC_RDPACTL_DEBUG */

#ifdef CC_RDPACTL_DEBUG
#define rdpaCtl_debug(fmt, arg...) {printf(">>> %s.%u: " fmt "\n", __FUNCTION__, __LINE__, ##arg); fflush(stdout);}
#define rdpaCtl_error(fmt, arg...) {printf("ERROR[%s.%u]: " fmt, __FUNCTION__, __LINE__, ##arg); fflush(stdout);}
#else
#define rdpaCtl_debug(fmt, arg...)
#define rdpaCtl_error(fmt, arg...)
#endif

/*
 * Static functions
 */

static inline int __rdpa_ioctl(int code __attribute__ ((unused)), uintptr_t ctx __attribute__ ((unused)))
{
#ifndef DESKTOP_LINUX
    int fd, ret = 0;
    fd = open(RDPACTL_IOCTL_FILE_NAME, O_RDWR);
    if (fd < 0)
    {
        rdpaCtl_error("%s: %s\n", RDPACTL_IOCTL_FILE_NAME, strerror(errno));
        return -EINVAL;
    }

    ret = ioctl(fd, code, ctx);
    if (ret)
        rdpaCtl_error("ioctl failed, Errno[%s] ret=%d\n", strerror(errno), ret);

    close(fd);
    return ret;
#else
	rdpaCtl_error("rdpa ioctl %d unsupported in desktop\n", code);
	return 0;
#endif
}

static int __sendIcCommand(rdpa_drv_ioctl_ic_t *ic_p)
{
    return __rdpa_ioctl(RDPA_IOC_IC, (uintptr_t)ic_p);
}

static int __sendPortCommand(rdpa_drv_ioctl_port_t *port_p)
{
    return __rdpa_ioctl(RDPA_IOC_PORT, (uintptr_t)port_p);
}

static int __sendBrCommand(rdpa_drv_ioctl_br_t *br_p)
{
    return __rdpa_ioctl(RDPA_IOC_BRIDGE, (uintptr_t)br_p);
}

static int __sendSysCommand(rdpa_drv_ioctl_sys_t *sys_p)
{
    return __rdpa_ioctl(RDPA_IOC_SYS, (uintptr_t)sys_p);
}

static int __sendLlidCommand(rdpa_drv_ioctl_llid_t *llid_p)
{
    return __rdpa_ioctl(RDPA_IOC_LLID, (uintptr_t)llid_p);
}

static int __sendDsWanUdpFilterCommand(rdpa_drv_ioctl_ds_wan_udp_filter_t *filter_p)
{
    return __rdpa_ioctl(RDPA_IOC_DS_WAN_UDP_FILTER, (uintptr_t)filter_p);
}

static int __sendDscpToPbitCommand(rdpa_drv_ioctl_dscp_to_pbit_t *dscp_to_pbit_p)
{
    return __rdpa_ioctl(RDPA_IOC_DSCP_TO_PBIT, (uintptr_t)dscp_to_pbit_p);
}

static int __sendMiscCommand(rdpa_drv_ioctl_misc_t *misc_cfg_p)
{
    return __rdpa_ioctl(RDPA_IOC_MISC, (uintptr_t)misc_cfg_p);
}

/*
 * Public functions
 */

int rdpaCtl_GetTmMemoryInfo(int  *fpmPoolMemorySize)
{
   int rc = 0;
   rdpa_drv_ioctl_misc_t misc;
   memset(&misc, 0, sizeof(rdpa_drv_ioctl_misc_t));

   misc.cmd = RDPA_IOCTL_MISC_CMD_GET_TM_MEMORY_INFO;
   rc = __sendMiscCommand(&misc);
   *fpmPoolMemorySize = misc.fpm_pool_memory_size;
   
   /* 
    Note: Do not remove if-else braces. If CC_RDPACTL_DEBUG is not defined (as in DESKTOP profile) 
    these lines will be empty causing warnings treated like errors in rdpactl lib.
   */
   if (rc)
   {
      rdpaCtl_debug("Error!");
   }
   else 
   {
      rdpaCtl_debug("Success!");
   }

   return rc;
}


/*******************************************************************************/
/* Ingress Classifier  API                                                                    */
/*******************************************************************************/

/*******************************************************************************
 * Function: rdpaCtl_add_classification_rule
 *   rule                                       // IN: classification rule
*******************************************************************************/

int rdpaCtl_add_classification_rule(rdpactl_classification_rule_t *rule, uint8_t *prty)
{
    rdpa_drv_ioctl_ic_t ic;
    int ret;

    rdpaCtl_debug("Rule classifier, dir: %d, type: %d, prty: %d, field_mask: 0x%x, port_mask: 0x%0x",
        rule->dir, rule->type, rule->prty, rule->field_mask, rule->port_mask);

    rdpaCtl_debug("Class key, src_ip: 0x%0x, dst_ip: 0x%0x, src_port: %d, dst_port: %d, protocol: %d, outer_vid: %d, inner_vid: %d,\
        dst_mac: %02x%02x%02x%02x%02x%02x, src_mac: %02x%02x%02x%02x%02x%02x,\
        etype: 0x%04x, dscp: %d, ingress_port: %d, opbits: %d, ipbits: %d, num_vlan: %d, ipv6_label: 0x%x, otpid: 0x%04x, itpid: 0x%04x",
        rule->src_ip.ipv4, rule->dst_ip.ipv4, rule->src_port, rule->dst_port, rule->protocol, rule->outer_vid, rule->inner_vid,
        rule->dst_mac[0], rule->dst_mac[1], rule->dst_mac[2], rule->dst_mac[3], rule->dst_mac[4], rule->dst_mac[5],
        rule->src_mac[0], rule->src_mac[1], rule->src_mac[2], rule->src_mac[3], rule->src_mac[4], rule->src_mac[5],
        rule->etype, rule->dscp, rule->ingress_port_id, rule->outer_pbits, rule->inner_pbits, rule->number_of_vlans,
        rule->ipv6_label, rule->outer_tpid, rule->inner_tpid);

    rdpaCtl_debug("Class result, qos_method: %d, wan_flow: %d, action: %d, forw_mode: %d, egress_port: %d, queue_id: %d, \
        opbit_remark: %d, ipbit_remark: %d, dscp_remark: %d, pbit_to_gem: %d",
        rule->qos_method, rule->wan_flow, rule->action, rule->forw_mode, rule->egress_port, rule->queue_id,
        rule->opbit_remark, rule->ipbit_remark, rule->dscp_remark, rule->pbit_to_gem);

    memset(&ic, 0, sizeof(rdpa_drv_ioctl_ic_t));

    ic.cmd = RDPA_IOCTL_IC_CMD_ADD_CLASSIFICATION_RULE;
    ic.param.rule = rule;

    ret = __sendIcCommand(&ic);
    *prty = ic.param.prty;
    rdpaCtl_debug("return ic priority: %d\n", *prty);

    return ret;
}

/*******************************************************************************
 * Function: rdpaCtl_del_classification_rule
 *   rule                                       // IN: classification rule
*******************************************************************************/

int rdpaCtl_del_classification_rule(rdpactl_classification_rule_t *rule)
{
    rdpa_drv_ioctl_ic_t ic;

    rdpaCtl_debug("Rule classifier, dir: %d, type: %d, prty: %d, field_mask: 0x%x, port_mask: 0x%0x",
        rule->dir, rule->type, rule->prty, rule->field_mask, rule->port_mask);

    rdpaCtl_debug("Class key, src_ip: 0x%0x, dst_ip: 0x%0x, src_port: %d, dst_port: %d, protocol: %d, outer_vid: %d, inner_vid: %d,\
        dst_mac: %02x%02x%02x%02x%02x%02x, src_mac: %02x%02x%02x%02x%02x%02x,\
        etype: 0x%04x, dscp: %d, ingress_port: %d, opbits: %d, ipbits: %d, num_vlan: %d, ipv6_label: 0x%x, otpid: 0x%04x, itpid: 0x%04x",
        rule->src_ip.ipv4, rule->dst_ip.ipv4, rule->src_port, rule->dst_port, rule->protocol, rule->outer_vid, rule->inner_vid,
        rule->dst_mac[0], rule->dst_mac[1], rule->dst_mac[2], rule->dst_mac[3], rule->dst_mac[4], rule->dst_mac[5],
        rule->src_mac[0], rule->src_mac[1], rule->src_mac[2], rule->src_mac[3], rule->src_mac[4], rule->src_mac[5],
        rule->etype, rule->dscp, rule->ingress_port_id, rule->outer_pbits, rule->inner_pbits, rule->number_of_vlans,
        rule->ipv6_label, rule->outer_tpid, rule->inner_tpid);

    rdpaCtl_debug("Class result, qos_method: %d, wan_flow: %d, action: %d, forw_mode: %d, egress_port: %d, queue_id: %d, \
        opbit_remark: %d, ipbit_remark: %d, dscp_remark: %d, pbit_to_gem: %d",
        rule->qos_method, rule->wan_flow, rule->action, rule->forw_mode, rule->egress_port, rule->queue_id,
        rule->opbit_remark, rule->ipbit_remark, rule->dscp_remark, rule->pbit_to_gem);

    memset(&ic, 0, sizeof(rdpa_drv_ioctl_ic_t));

    ic.cmd = RDPA_IOCTL_IC_CMD_DEL_CLASSIFICATION_RULE;
    ic.param.rule = rule;

    return __sendIcCommand(&ic);
}


/*******************************************************************************
 * Function: rdpaCtl_add_classification
 *   rule                                       // IN: classification rule
*******************************************************************************/

int rdpaCtl_add_classification(rdpactl_classification_rule_t *rule)
{
    rdpa_drv_ioctl_ic_t ic;

    rdpaCtl_debug("Rule classifier, dir: %d, type: %d, prty: %d, field_mask: 0x%x, port_mask: 0x%0x",
        rule->dir, rule->type, rule->prty, rule->field_mask, rule->port_mask);

    memset(&ic, 0, sizeof(rdpa_drv_ioctl_ic_t));

    ic.cmd = RDPA_IOCTL_IC_CMD_ADD;
    ic.param.rule = rule;

    return __sendIcCommand(&ic);
}

/*******************************************************************************
 * Function: rdpaCtl_del_classification
 *   rule                                       // IN: classification rule
*******************************************************************************/
int rdpaCtl_del_classification(rdpactl_classification_rule_t *rule)
{
    rdpa_drv_ioctl_ic_t ic;

    rdpaCtl_debug("Rule classifier, dir: %d, type: %d, prty: %d, field_mask: 0x%x, port_mask: 0x%0x",
        rule->dir, rule->type, rule->prty, rule->field_mask, rule->port_mask);

    memset(&ic, 0, sizeof(rdpa_drv_ioctl_ic_t));

    ic.cmd = RDPA_IOCTL_IC_CMD_DEL;
    ic.param.rule = rule;

    return __sendIcCommand(&ic);
}

/*******************************************************************************
 * Function: rdpaCtl_find_classification
 *   rule                                       // IN: classification rule
*******************************************************************************/

int rdpaCtl_find_classification(rdpactl_classification_rule_t *rule)
{
    rdpa_drv_ioctl_ic_t ic;
    int ret;

    rdpaCtl_debug("Rule classifier, dir: %d, type: %d, prty: %d, field_mask: 0x%x, port_mask: 0x%0x",
        rule->dir, rule->type, rule->prty, rule->field_mask, rule->port_mask);

    memset(&ic, 0, sizeof(rdpa_drv_ioctl_ic_t));

    ic.cmd = RDPA_IOCTL_IC_CMD_FIND;
    ic.param.rule = rule;

    ret = __sendIcCommand(&ic);
    rdpaCtl_debug("return ic priority: %d\n", *prty);

    return ret;
}



/*********************************************************************************************/
/*                                     Generic System accessors                              */
/*********************************************************************************************/
int rdpaCtl_get_in_tpid(uint16_t *inTpid)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_IN_TPID_GET;

    rc = __sendSysCommand(&sys);
    if (!rc)
    {
        *inTpid = sys.param.inner_tpid;
        rdpaCtl_debug("inner tipid: 0x%04x", *inTpid);
    }

    return rc;
}


int rdpaCtl_set_in_tpid(uint16_t inTpid)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_IN_TPID_SET;
    sys.param.inner_tpid = inTpid;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set inner tpid to 0x%04x failed", inTpid);
    }

    return rc;
}


int rdpaCtl_get_out_tpid(uint16_t *outTpid)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_OUT_TPID_GET;

    rc = __sendSysCommand(&sys);
    if (!rc)
    {
        *outTpid = sys.param.outer_tpid;
        rdpaCtl_debug("outer tpid: 0x%04x", *outTpid);
    }

    return rc;
}


int rdpaCtl_set_out_tpid(uint16_t outTpid)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_OUT_TPID_SET;
    sys.param.outer_tpid= outTpid;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set outer tpid to 0x%04x failed", outTpid);
    }

    return rc;
}

int rdpaCtl_set_detect_tpid(uint16_t tpid, BOOL is_inner)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_DETECT_TPID_SET;
    sys.param.detect_tpid.is_inner = is_inner;
    sys.param.detect_tpid.tpid = tpid;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set detect tpid[%s] to 0x%04x failed",is_inner?"inner":"outer", tpid);
    }

    return rc;
}

int rdpaCtl_set_epon_mode(rdpa_epon_mode eponMode)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_EPON_MODE_SET;
    sys.param.epon_mode = eponMode;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set epon mode %u failed", eponMode);
    }

    return rc;
}

int rdpaCtl_get_epon_mode(rdpa_epon_mode *eponMode)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_EPON_MODE_GET;

    rc = __sendSysCommand(&sys);
    if (!rc)
    {
        *eponMode = sys.param.epon_mode;
        rdpaCtl_debug("get epon mode %u success", *eponMode);
    }

    return rc;
}


int rdpaCtl_set_always_tpid(uint16_t alwaysTpid)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_ALWAYS_TPID_SET;
    sys.param.always_tpid = alwaysTpid;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set always tpid to 0x%04x failed", alwaysTpid);
    }

    return rc;
}

int rdpaCtl_get_force_dscp(uint16_t dir, BOOL *enable)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_FORCE_DSCP_GET;
    sys.param.force_dscp.dir = dir;

    rc = __sendSysCommand(&sys);
    if (!rc)
    {
        *enable = sys.param.force_dscp.enable;
        rdpaCtl_debug("dir %d force_dscp: %u", dir, *enable);
    }

    return rc;
}

int rdpaCtl_set_force_dscp(uint16_t dir, BOOL enable)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_FORCE_DSCP_SET;
    sys.param.force_dscp.dir = dir;
    sys.param.force_dscp.enable = enable;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set dir %d force dscp to %u failed", dir, enable);
    }

    return rc;
}

int rdpaCtl_set_sys_car_mode(BOOL enable)
{
    int rc;
    rdpa_drv_ioctl_sys_t sys;

    memset(&sys, 0, sizeof(rdpa_drv_ioctl_sys_t));
    sys.cmd = RDPA_IOCTL_SYS_CMD_CAR_MODE_SET;
    sys.param.car_mode = enable;

    rc = __sendSysCommand(&sys);
    if (rc)
    {
        rdpaCtl_debug("set car mode %d failed", enable);
    }

    return rc;
}

int rdpaCtl_set_port_sal_miss_action(const char *ifName, rdpa_forward_action act)
{
    int rc;
    rdpa_drv_ioctl_port_t port;

    rc = rdpaCtl_get_port_param(ifName, &port.param);
    if (rc)
    {
        rdpaCtl_debug("get port %s sal miss action failed", ifName);
    }
    else
    {
        port.param.sal_miss_action = act;
        rc = rdpaCtl_set_port_param(ifName, &port.param);
    }

    return rc;
}

int rdpaCtl_get_port_sal_miss_action(const char *ifName, rdpa_forward_action *act)
{
    int rc;
    rdpa_drv_ioctl_port_t port;

    rc = rdpaCtl_get_port_param(ifName, &port.param);
    if (rc)
    {
        rdpaCtl_debug("get port %s sal miss action failed", ifName);
    }
    else
    {
        *act = port.param.sal_miss_action;
    }

    return rc;
}

int rdpaCtl_set_port_dal_miss_action(const char *ifName, rdpa_forward_action act)
{
    int rc;
    rdpa_drv_ioctl_port_t port;

    rc = rdpaCtl_get_port_param(ifName, &port.param);
    if (rc)
    {
        rdpaCtl_debug("get port %s dal miss action failed", ifName);
    }
    else
    {
        port.param.dal_miss_action = act;
        rc = rdpaCtl_set_port_param(ifName, &port.param);
    }

    return rc;
}

int rdpaCtl_get_port_dal_miss_action(const char *ifName, rdpa_forward_action *act)
{
    int rc;
    rdpa_drv_ioctl_port_t port;

    rc = rdpaCtl_get_port_param(ifName, &port.param);
    if (rc)
    {
        rdpaCtl_debug("get port %s dal miss action failed", ifName);
    }
    else
    {
        *act = port.param.dal_miss_action;
    }

    return rc;
}

int rdpaCtl_set_port_param(const char *ifName,
  rdpa_drv_ioctl_port_param_t *portParam)
{
    int rc;
    rdpa_drv_ioctl_port_t port;

    memset(&port, 0, sizeof(rdpa_drv_ioctl_port_t));
    port.cmd = RDPA_IOCTL_PORT_CMD_PARAM_SET;
    strncpy(port.ifname, ifName, IFNAMSIZ-1);
    memcpy(&port.param, portParam, sizeof(rdpa_drv_ioctl_port_param_t));

    rc = __sendPortCommand(&port);
    if (rc)
    {
        rdpaCtl_debug("set port %s param failed", ifName);
    }

    return rc;
}

int rdpaCtl_get_port_param(const char *ifName,
  rdpa_drv_ioctl_port_param_t *portParam)
{
    int rc;
    rdpa_drv_ioctl_port_t port;

    memset(&port, 0, sizeof(rdpa_drv_ioctl_port_t));
    port.cmd = RDPA_IOCTL_PORT_CMD_PARAM_GET;
    strncpy(port.ifname, ifName, IFNAMSIZ-1);

    rc = __sendPortCommand(&port);
    if (rc)
    {
        rdpaCtl_debug("get port %s param failed", ifName);
    }
    else
    {
        memcpy(portParam, &port.param, sizeof(rdpa_drv_ioctl_port_param_t));
    }

    return rc;
}

/*******************************************************************************
 *
 * Function: rdpaCtl_BrExist
 *   brId,                                      // IN: br id
 *   found,                                     // OUT: exist or not
 *
 *******************************************************************************/
int rdpaCtl_BrExist(uint8_t brId, BOOL *found)
{
    int rc = 0;
    rdpa_drv_ioctl_br_t br;

    rdpaCtl_debug("exist or not brId %u,", brId);

    *found = FALSE;
    memset(&br, 0, sizeof(rdpa_drv_ioctl_br_t));

    br.cmd = RDPA_IOCTL_BR_CMD_FIND_OBJ;
    br.br_index = brId;

    rc = __sendBrCommand(&br);

    if(!rc)
    {
        *found = br.found;
        if(br.found)
        {
            rdpaCtl_debug("br %u exist", brId);
        }
        else
        {
            rdpaCtl_debug("br %u not exist", brId);
        }
    }
    return rc;
}


/*******************************************************************************
 *
 * Function: rdpaCtl_BrLocalSwitchSet
 *   brId,                                 // IN: br id
 *   local_switch,                      // IN: mode to set
 *
 *******************************************************************************/
int rdpaCtl_BrLocalSwitchSet(uint8_t brId, BOOL local_switch)
{
    rdpa_drv_ioctl_br_t br;

    rdpaCtl_debug("set bridge %u local switch %u", brId, (uint8_t)local_switch);

    memset(&br, 0, sizeof(rdpa_drv_ioctl_br_t));

    br.cmd = RDPA_IOCTL_BR_CMD_LOCAL_SWITCH_SET;
    br.br_index = brId;
    br.local_switch = local_switch;

    return __sendBrCommand(&br);
}


int rdpaCtl_LlidCreate(uint8_t llid_index)
{
    rdpa_drv_ioctl_llid_t llidPara;
    memset(&llidPara, 0, sizeof(rdpa_drv_ioctl_llid_t));

    llidPara.cmd = RDPA_IOCTL_LLID_CMD_NEW;
    llidPara.llid_index = llid_index;

    return __sendLlidCommand(&llidPara);
}

int rdpaCtl_DsWanUdpFilterAdd(rdpactl_ds_wan_udp_filter_t *filter_p)
{
    rdpa_drv_ioctl_ds_wan_udp_filter_t ds_wan_udp_filter;
    int ret;

    rdpaCtl_debug();

    ds_wan_udp_filter.cmd = RDPA_IOCTL_DS_WAN_UDP_FILTER_CMD_ADD;

    ds_wan_udp_filter.filter = *filter_p;

    ret = __sendDsWanUdpFilterCommand(&ds_wan_udp_filter);

    filter_p->index = ds_wan_udp_filter.filter.index;

    return ret;
}

int rdpaCtl_DsWanUdpFilterDelete(long index)
{
    rdpa_drv_ioctl_ds_wan_udp_filter_t ds_wan_udp_filter;

    rdpaCtl_debug();

    ds_wan_udp_filter.cmd = RDPA_IOCTL_DS_WAN_UDP_FILTER_CMD_DELETE;

    ds_wan_udp_filter.filter.index = index;

    return __sendDsWanUdpFilterCommand(&ds_wan_udp_filter);
}

int rdpaCtl_DsWanUdpFilterGet(rdpactl_ds_wan_udp_filter_t *filter_p)
{
    rdpa_drv_ioctl_ds_wan_udp_filter_t ds_wan_udp_filter;
    int ret;

    rdpaCtl_debug();

    ds_wan_udp_filter.cmd = RDPA_IOCTL_DS_WAN_UDP_FILTER_CMD_GET;

    ds_wan_udp_filter.filter.index = filter_p->index;

    ret = __sendDsWanUdpFilterCommand(&ds_wan_udp_filter);

    *filter_p = ds_wan_udp_filter.filter;

    return ret;
}

/* Set -1 to disble remark */
int rdpaCtl_RdpaMwMCastSet(int dscp_val)
{
    rdpaCtl_debug();
    rdpaCtl_debug("Setting rdpa_mw multicast DSCP remark with %d,", dscp_val);

    return __rdpa_ioctl(RDPA_IOC_RDPA_MW_SET_MCAST_DSCP_REMARK, dscp_val);
}

int rdpaCtl_time_sync_init(void)
{
    return __rdpa_ioctl(RDPA_IOC_TIME_SYNC, 0);
}

/*******************************************************************************
 *
 * Function: rdpaCtl_DscpToPitGet
 *   found,                                   // OUT: found or not
 *   map,                                     // OUT: dscp tp pbit map
 *
 *******************************************************************************/
int rdpaCtl_DscpToPitGet(BOOL *found, tmctl_dscpToPbitCfg_t *map)
{
    int i;
    int rc = 0;
    rdpa_drv_ioctl_dscp_to_pbit_t dscp_to_pbit;

    rdpaCtl_debug("rdpaCtl_DscpToPitGet");

    memset(&dscp_to_pbit, 0, sizeof(rdpa_drv_ioctl_dscp_to_pbit_t));

    dscp_to_pbit.found = FALSE;
    dscp_to_pbit.cmd = RDPA_IOCTL_D_TO_P_CMD_GET;
    rc = __sendDscpToPbitCommand(&dscp_to_pbit);

    if (!rc)
    {
        *found = dscp_to_pbit.found;
        if (dscp_to_pbit.found)
        {
            for (i = 0; i < TOTAL_DSCP_NUM; i++)
            {
                map->dscp[i] = dscp_to_pbit.dscp_pbit_map[i];
                rdpaCtl_debug(
                    "get dscp %d to pbit map %u", i, dscp_to_pbit.dscp_pbit_map[i]);
            }
        }
    }

    return rc;
}

/*******************************************************************************
 *
 * Function: rdpaCtl_DscpToPitSet
 *   map,                                     // IN: dscp tp pbit map
 *
 *******************************************************************************/
int rdpaCtl_DscpToPitSet(tmctl_dscpToPbitCfg_t map)
{
    int i;
    int rc = 0;
    rdpa_drv_ioctl_dscp_to_pbit_t dscp_to_pbit;

    rdpaCtl_debug("rdpaCtl_DscpToPitSet");

    memset(&dscp_to_pbit, 0, sizeof(rdpa_drv_ioctl_dscp_to_pbit_t));

    dscp_to_pbit.cmd = RDPA_IOCTL_D_TO_P_CMD_SET;

    for (i = 0; i < TOTAL_DSCP_NUM; i++)
    {
        dscp_to_pbit.dscp_pbit_map[i] = map.dscp[i];
    }

    rc = __sendDscpToPbitCommand(&dscp_to_pbit);

    return rc;
}

/*******************************************************************************
 *
 * Function: rdpaCtl_PktBasedQosGet
 *   dir,                                 // IN: RDPA direction
 *   type,                              // IN: RDPA qos type
 *   enable,                           // OUT: enable or not
 *
 *******************************************************************************/
int rdpaCtl_PktBasedQosGet(int dir, int type, BOOL *enable)
{
    int rc = 0;
    rdpa_drv_ioctl_misc_t misc_cfg;

    rdpaCtl_debug(
        "rdpaCtl_PktBasedQosGet dir [%d] type [%d]", dir, type);

    memset(&misc_cfg, 0, sizeof(rdpa_drv_ioctl_misc_t));

    misc_cfg.dir = dir;
    misc_cfg.type = type;
    misc_cfg.cmd = RDPA_IOCTL_MISC_CMD_PKT_BASED_QOS_GET;

    rc = __sendMiscCommand(&misc_cfg);

    if (!rc)
    {
        *enable = misc_cfg.enable;
    }

    return rc;
}

/*******************************************************************************
 *
 * Function: rdpaCtl_PktBasedQosSet
 *   dir,                                 // IN: RDPA direction
 *   type,                              // IN: RDPA qos type
 *   enable,                           // IN: enable or not
 *
 *******************************************************************************/
int rdpaCtl_PktBasedQosSet(int dir, int type, BOOL enable)
{
    int rc = 0;
    rdpa_drv_ioctl_misc_t misc_cfg;

    rdpaCtl_debug(
        "rdpaCtl_PktBasedQosSet dir [%d] type [%d] en [%d]", dir, type, enable);

    memset(&misc_cfg, 0, sizeof(rdpa_drv_ioctl_misc_t));

    misc_cfg.dir = dir;
    misc_cfg.type = type;
    misc_cfg.enable = enable;
    misc_cfg.cmd = RDPA_IOCTL_MISC_CMD_PKT_BASED_QOS_SET;

    rc = __sendMiscCommand(&misc_cfg);

    return rc;
}

