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

#ifndef _RDPACTL_API_H_
#define _RDPACTL_API_H_

#include "bcmtypes.h"
#include "rdpa_types.h"
#include "rdpa_drv.h"
#include "tmctl_api.h"

#define RDPACTL_IFNAME_INVALID_STR "NULL"
#define RDPACTL_IS_IFNAME_VALID(_ifName)                            \
    strncmp((_ifName), RDPACTL_IFNAME_INVALID_STR, IFNAMSIZ)


int rdpaCtl_GetTmMemoryInfo(int  *fpmPoolMemorySize);

int rdpaCtl_add_classification_rule(rdpactl_classification_rule_t *rule, uint8_t *prty);
int rdpaCtl_del_classification_rule(rdpactl_classification_rule_t *rule);
int rdpaCtl_add_classification(rdpactl_classification_rule_t *rule);
int rdpaCtl_del_classification(rdpactl_classification_rule_t *rule);
int rdpaCtl_find_classification(rdpactl_classification_rule_t *rule);

int rdpaCtl_get_in_tpid(uint16_t *inTpid);
int rdpaCtl_set_in_tpid(uint16_t inTpid);
int rdpaCtl_get_out_tpid(uint16_t *outTpid);
int rdpaCtl_set_out_tpid(uint16_t outTpid);
int rdpaCtl_set_epon_mode(rdpa_epon_mode eponMode);
int rdpaCtl_get_epon_mode(rdpa_epon_mode *eponMode);
int rdpaCtl_set_always_tpid(uint16_t alwaysTpid);
int rdpaCtl_set_detect_tpid(uint16_t tpid, BOOL is_inner);
int rdpaCtl_get_force_dscp(uint16_t dir, BOOL *enable);
int rdpaCtl_set_force_dscp(uint16_t dir, BOOL enable);
int rdpaCtl_set_sys_car_mode(BOOL enable);

int rdpaCtl_set_port_sal_miss_action(const char *ifName, rdpa_forward_action act);
int rdpaCtl_get_port_sal_miss_action(const char *ifName, rdpa_forward_action *act);
int rdpaCtl_set_port_dal_miss_action(const char *ifname, rdpa_forward_action act);
int rdpaCtl_get_port_dal_miss_action(const char *ifName, rdpa_forward_action *act);
int rdpaCtl_set_port_param(const char *ifName, rdpa_drv_ioctl_port_param_t *portParam);
int rdpaCtl_get_port_param(const char *ifName, rdpa_drv_ioctl_port_param_t *portParam);

int rdpaCtl_BrExist(uint8_t brId, BOOL *found);
int rdpaCtl_BrLocalSwitchSet(uint8_t brId, BOOL local_switch);


int rdpaCtl_LlidCreate(uint8_t llid_index);

int rdpaCtl_DsWanUdpFilterAdd(rdpactl_ds_wan_udp_filter_t *filter_p);
int rdpaCtl_DsWanUdpFilterDelete(long index);
int rdpaCtl_DsWanUdpFilterGet(rdpactl_ds_wan_udp_filter_t *filter_p);

int rdpaCtl_RdpaMwMCastSet(int dscp_val);
int rdpaCtl_time_sync_init(void);

/* rdpa_user based APIs */
int rdpactl_has_rdpa_port_type_gpon(char* gpon_name);
int rdpactl_has_rdpa_port_type_epon(char* epon_name);
int rdpactl_set_sys_car_mode(int enable);
int rdpactl_set_learning_ind(char *name, unsigned char learningInd);
rdpa_port_type rdpactl_get_port_type(const char *name);

/* This function needs to be used instead of rdpa_port_get 
*  for HGU plarforms to translate veip name which does not
*  have port object to underlaying physical interface name
*/
int rdpactl_get_port_by_name(const char *ifname, bdmf_object_handle *port);

/**
 * @brief gets network interface device type and fixes up interface name as displayed towards OS
 * @param[io] dev_type 
 * @param[o]  oifname 
 * @param[i]  ifname 
 */
void rdpactl_get_dev_type_by_ifname(tmctl_devType_e *dev_type, char **oifname, const char *ifname);

#endif /* _RDPACTL_API_H_ */

