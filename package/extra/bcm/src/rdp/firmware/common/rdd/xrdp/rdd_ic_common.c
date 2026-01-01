/*
* <:copyright-BRCM:2014:proprietary:standard
* 
*    Copyright (c) 2014 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/


#include "rdd.h"
#include "rdd_tcam_ic.h"
#include "rdd_data_structures_auto.h"
#include "rdd_ic_common.h"
#include "rdd.h"

#if defined(RDP_UFC) || defined(XRDP_RGEN6)
void rdd_ic_result_entry_compose(uint16_t index, const rdd_ic_context_t *ctx, uint8_t *entry)
{
    TCAM_CONTEXT_STRUCT context_entry = {};

    context_entry.cntr_id = ctx->cntr_id;
    context_entry.cntr_disable = ctx->cntr_disable;
    context_entry.bytes_cntr_id = ctx->bytes_cntr_id;
    context_entry.bytes_cntr_disable = ctx->bytes_cntr_disable;
    context_entry.iq_priority = ctx->iq_priority;
#if defined(RDP_UFC) && defined(CONFIG_RNR_HW_FIREWALL)
    context_entry.ct_override = ctx->ct_override;
#endif
    
    switch (ctx->action)
    {
    case    rdpa_forward_action_forward:
    case    rdpa_forward_action_none:
    case    rdpa_forward_action_skip:
        context_entry.action = ACTION_FORWARD;
#if defined(BCM_DSL_XRDP)
        context_entry.trap_reason = ctx->trap_reason;
#endif        
        break;
    case    rdpa_forward_action_host:
        context_entry.action = ACTION_TRAP;
        context_entry.trap_reason = ctx->trap_reason;
        break;
    case    rdpa_forward_action_drop:
    default:
        context_entry.action = ACTION_DROP;
        break;
    }

    memcpy(entry, &context_entry, sizeof(TCAM_CONTEXT_STRUCT));
}

#else
void rdd_ic_result_entry_compose(uint16_t index, const rdd_ic_context_t *ctx, uint8_t *entry)
{
    RULE_BASED_CONTEXT_ENTRY_STRUCT context_entry = {};

    context_entry.queue = ctx->priority;
    context_entry.to_eth = ctx->to_eth;
#if !defined(BCM_DSL_XRDP)
    context_entry.port =  ctx->egress_port;
#endif /* !BCM_DSL_XRDP */
    context_entry.flow =  context_entry.to_eth ? ctx->egress_port : ctx->tx_flow;

    context_entry.rule = index;
    context_entry.pbit_to_q = ctx->qos_method == rdpa_qos_method_pbit;
    context_entry.tc_to_q = (ctx->forw_mode != rdpa_forwarding_mode_flow) && (ctx->qos_method == rdpa_qos_method_flow);
    context_entry.pbit_to_gem = ctx->gem_mapping_mode;
    context_entry.flow_based_forward = 
        (ctx->forw_mode == rdpa_forwarding_mode_flow);
    if (context_entry.pbit_to_gem == rdpa_qos_method_pbit)
        context_entry.gem_mapping_table = ctx->gem_mapping_table;

    context_entry.pbit_remark = (ctx->opbit_remark | ctx->ipbit_remark);
    context_entry.cntr_id = ctx->cntr_id;
    context_entry.cntr_disable = ctx->cntr_disable;
    context_entry.policer_id = ctx->policer;
    context_entry.qos_rule_wan_flow_overrun = ctx->qos_rule_wan_flow_overrun;
    context_entry.opbit_remark_mode = ctx->opbit_remark;
    context_entry.ipbit_remark_mode = ctx->ipbit_remark;
    context_entry.outer_pbit = ctx->opbit_val;	 	 
    context_entry.inner_pbit = ctx->ipbit_val;
#if !defined(BCM_DSL_XRDP)
    context_entry.tunnel_id = ctx->tunnel;
    context_entry.ttl = ctx->ttl;
    context_entry.loopback = ctx->loopback;
    context_entry.include_mcast = ctx->include_mcast;
    context_entry.bytes_cntr_id = ctx->bytes_cntr_id;
    context_entry.bytes_cntr_disable = ctx->bytes_cntr_disable;

    /* action is added dynamically according to policer params */    
    context_entry.dei_color = 0;
#ifdef OPERATION_MODE_PRV
    context_entry.pbit_color = 0;
#endif
#else
    /* action is added dynamically according to policer params */    
    context_entry.dei = 0;
#endif

    if (-1 == ctx->policer)
        context_entry.policer_enable = 0;
    else
        context_entry.policer_enable = 1;

#if !defined(BCM_DSL_XRDP)
    if (-1 == ctx->tunnel)
        context_entry.tunnel_enable = 0;
    else
        context_entry.tunnel_enable = 1;
#endif

    context_entry.dscp_value = ctx->dscp_val;
    context_entry.dscp_remark = ctx->dscp_remark;
    context_entry.vlan_action_or_iptv_common_actions = ctx->is_vlan_action; /* avoid fw vlan action incase of transparnet for all egress ports */

    context_entry.service_queue = ctx->service_queue;
    context_entry.service_queue_mode = ctx->service_queue_mode;
    
    switch (ctx->action)
    {
    case    rdpa_forward_action_forward:
        context_entry.no_fwd = 0;
        break;
    case    rdpa_forward_action_host:
        context_entry.no_fwd = 1;
        context_entry.no_fwd_action = 1;
        context_entry.trap_reason = ctx->trap_reason;
        break;
#if !defined(BCM_DSL_XRDP)
    case    rdpa_forward_action_skip:
        context_entry.no_fwd = 1;
        context_entry.skip = 1;
        break;
#endif
    case    rdpa_forward_action_drop:
    case    rdpa_forward_action_none:
    default:
        context_entry.no_fwd = 1;
        context_entry.no_fwd_action = 0;
        break;
    }

    memcpy(entry, &context_entry, sizeof(RULE_BASED_CONTEXT_ENTRY_STRUCT));
}

void rdd_ic_result_entry_decompose(uint8_t *entry, uint16_t *index, rdd_ic_context_t *ctx)
{
    RULE_BASED_CONTEXT_ENTRY_STRUCT context_entry = {};

    memcpy(&context_entry, entry, sizeof(RULE_BASED_CONTEXT_ENTRY_STRUCT));

    if (ctx)
    {
        ctx->to_eth = context_entry.to_eth;
        ctx->priority = context_entry.queue;
#if defined(G9991_PRV)
        ctx->tx_flow = ctx->to_eth ? RDD_NUM_OF_TX_WAN_FLOWS : context_entry.flow;
        ctx->egress_port = ctx->to_eth ? context_entry.port : RDD_WAN0_VPORT;
#else
        ctx->tx_flow = ctx->to_eth ? 0 : context_entry.flow;
        ctx->egress_port = ctx->to_eth ? context_entry.flow : RDD_WAN0_VPORT;
#endif
        ctx->dscp_val = context_entry.dscp_value;
        ctx->dscp_remark = context_entry.dscp_remark;
        ctx->qos_method = (context_entry.pbit_to_q == 1)
            ? rdpa_qos_method_pbit : rdpa_qos_method_flow;
        ctx->gem_mapping_mode = context_entry.pbit_to_gem;
        ctx->gem_mapping_table = context_entry.gem_mapping_table;
        ctx->forw_mode = (ctx->forw_mode == 1) ? rdpa_forwarding_mode_flow : rdpa_forwarding_mode_pkt;

        ctx->policer = context_entry.policer_id;
        ctx->cntr_id = context_entry.cntr_id;
        ctx->cntr_disable = context_entry.cntr_disable;
        ctx->qos_rule_wan_flow_overrun = context_entry.qos_rule_wan_flow_overrun;
        ctx->opbit_remark = context_entry.opbit_remark_mode;
        ctx->ipbit_remark = context_entry.ipbit_remark_mode;
        ctx->opbit_val = context_entry.outer_pbit;	 	 
        ctx->ipbit_val = context_entry.inner_pbit;
        ctx->service_queue = context_entry.service_queue;
        ctx->service_queue_mode = context_entry.service_queue_mode;

#if !defined(BCM_DSL_XRDP)
        ctx->tunnel = context_entry.tunnel_id;
        ctx->ttl = context_entry.ttl;
        ctx->loopback = context_entry.loopback;
        ctx->include_mcast = context_entry.include_mcast;
        ctx->bytes_cntr_id = context_entry.bytes_cntr_id;
        ctx->bytes_cntr_disable = context_entry.bytes_cntr_disable;
#endif

        if (context_entry.no_fwd)
        {
            if (context_entry.no_fwd_action)
                ctx->action = rdpa_forward_action_host;
            else
                ctx->action = rdpa_forward_action_drop;
        }
        else
        {
#if !defined(BCM_DSL_XRDP)
            if (context_entry.skip)
                ctx->action = rdpa_forward_action_skip;
            else
#endif
                ctx->action = rdpa_forward_action_forward;
        }
    }
    *index = context_entry.rule;
}
#endif
