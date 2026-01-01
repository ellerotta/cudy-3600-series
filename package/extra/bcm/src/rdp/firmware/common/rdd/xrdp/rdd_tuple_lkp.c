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


#include "rdd_tuple_lkp.h"
#include "rdd_ag_natc.h"
#include "rdd_crc.h"

int rdd_nat_cache_init(const rdd_module_t *module)
{
    RDD_NAT_CACHE_CFG_RES_OFFSET_WRITE_G(module->res_offset, module->cfg_ptr, 0);
    RDD_NAT_CACHE_CFG_CONTEXT_OFFSET_WRITE_G(module->context_offset, module->cfg_ptr, 0);
    RDD_NAT_CACHE_CFG_KEY_OFFSET_WRITE_G(((natc_params_t *)module->params)->connection_key_offset, module->cfg_ptr, 0);

    return BDMF_ERR_OK;
}

#if CHIP_VER >= RDP_GEN_60
#define RDD_TRACE_RGEN6_IPV4_FLOW_KEY(title, src_ip, dst_ip, src_port, dst_port, proto, tcp_pure_ack, tos, num_of_vlans, valid)\
    RDD_TRACE("%s = { src_ip = %u.%u.%u.%u, dst_ip = %u.%u.%u.%u, prot = %d, src_port = %d, dst_port = %d, tcp_pure_ack = %d, tos = %u, num_of_vlans = %d, valid = %d\n", \
        title, (uint8_t)(src_ip >> 24) & 0xff, \
        (uint8_t)(src_ip >> 16) & 0xff, \
        (uint8_t)(src_ip >> 8) & 0xff, \
        (uint8_t)(src_ip) & 0xff, \
        (uint8_t)(dst_ip >> 24) & 0xff, \
        (uint8_t)(dst_ip >> 16) & 0xff, \
        (uint8_t)(dst_ip >> 8) & 0xff, \
        (uint8_t)(dst_ip) & 0xff, \
        proto, src_port, dst_port, tcp_pure_ack, tos, num_of_vlans, valid)
#else
#define RDD_TRACE_IPV4_FLOW_KEY(title, src_ip, dst_ip, src_port, dst_port, proto, tcp_pure_ack, is_ctx_ext)\
    RDD_TRACE("%s = { src_ip = %u.%u.%u.%u, dst_ip = %u.%u.%u.%u, prot = %d, src_port = %d, dst_port = %d, tcp_pure_ack = %d, is_ctx_ext = %d\n", \
        title, (uint8_t)(src_ip >> 24) & 0xff, \
        (uint8_t)(src_ip >> 16) & 0xff, \
        (uint8_t)(src_ip >> 8) & 0xff, \
        (uint8_t)(src_ip) & 0xff, \
        (uint8_t)(dst_ip >> 24) & 0xff, \
        (uint8_t)(dst_ip >> 16) & 0xff, \
        (uint8_t)(dst_ip >> 8) & 0xff, \
        (uint8_t)(dst_ip) & 0xff, \
        proto, src_port, dst_port, tcp_pure_ack, is_ctx_ext)
#endif

#if !defined(OPERATION_MODE_PRV)
/* Map IP header protocol field to parser L4 protocol field */
static int2int_map_t l4_protocol_to_l4_protocol_code[] =
{
    {IP_PROTO_TCP, PARSER_L4_PROTOCOL_TCP},
    {IP_PROTO_UDP, PARSER_L4_PROTOCOL_UDP},
    {IP_PROTO_ICMP, PARSER_L4_PROTOCOL_ICMP},
    {IP_PROTO_IGMP, PARSER_L4_PROTOCOL_IGMP},
    {IP_PROTO_GRE, PARSER_L4_PROTOCOL_GRE},
    {IP_PROTO_ESP, PARSER_L4_PROTOCOL_ESP},
    {IP_PROTO_AH, PARSER_L4_PROTOCOL_AH},
    {IP_PROTO_L2TP, PARSER_L4_PROTOCOL_L2TPV3},
    {IP_PROTO_ICMPV6, PARSER_L4_PROTOCOL_ICMPV6},
    {IP_PROTO_IPV6, PARSER_L4_PROTOCOL_IPV6},
    {IP_PROTO_IPIP, PARSER_L4_PROTOCOL_IPV4_OVER_IPV6},
    /* The last entry is used when no matching entry is found */
    {IP_PROTO_RESERVED, PARSER_L4_PROTOCOL_OTHER}
};

void rdd_ip_class_key_entry_decompose(rdd_l3_flow_key_t *tuple_entry, uint8_t *sub_tbl_id, uint8_t *tuple_entry_ptr, rdd_ctx_size *ctx_size)
{
    NAT_CACHE_L3_LKP_ENTRY_STRUCT lkp_entry;

    RDD_BTRACE("tuple_entry %px, tuple_entry_ptr = %px\n", tuple_entry, tuple_entry_ptr);

    if (!tuple_entry)
    {
        return;
    }
    memcpy(&lkp_entry, tuple_entry_ptr, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
#ifdef FIRMWARE_LITTLE_ENDIAN
    SWAPBYTES(((uint8_t *)&lkp_entry), sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
#endif

#if CHIP_VER >= RDP_GEN_60
    RDD_TRACE_RGEN6_IPV4_FLOW_KEY("tuple_entry_ptr", lkp_entry.src_ip, lkp_entry.dst_ip, lkp_entry.src_port,
        lkp_entry.dst_port, lkp_entry.protocol, lkp_entry.tcp_pure_ack, lkp_entry.tos, lkp_entry.vlans_num, lkp_entry.valid);

    *ctx_size = lkp_entry.var_len_ctx;

    if (lkp_entry.ipv6)
    {
        /* TODO - need to enhance and display also ipv6 addresses */
        memset(&tuple_entry->dst_ip.addr.ipv6, 0, 16);
        memset(&tuple_entry->src_ip.addr.ipv6, 0, 16);
    }
    else
    {
        tuple_entry->dst_ip.addr.ipv4 = lkp_entry.dst_ip;
        tuple_entry->src_ip.addr.ipv4 = lkp_entry.src_ip;
    }

    tuple_entry->dst_port = lkp_entry.dst_port;
    tuple_entry->src_port = lkp_entry.src_port;
    tuple_entry->dst_ip.family = lkp_entry.ipv6 ? bdmf_ip_family_ipv6 : bdmf_ip_family_ipv4;
    tuple_entry->src_ip.family = lkp_entry.ipv6 ? bdmf_ip_family_ipv6 : bdmf_ip_family_ipv4;
    tuple_entry->lookup_port = lkp_entry.lookup_port;
    tuple_entry->tcp_pure_ack = lkp_entry.tcp_pure_ack;
    tuple_entry->vtag_num = lkp_entry.vlans_num;
    tuple_entry->tos = lkp_entry.tos;
    tuple_entry->is_ctx_ext = lkp_entry.ctx_ext;

    if (rdd_natc_pbit_key_mode_get())
    {
        /* The protocol field is re-purposed with pbit and l4_protocol */
        tuple_entry->prot = int2int_map_r(l4_protocol_to_l4_protocol_code, lkp_entry.protocol & PBIT_KEY_L4_PROTOCOL_MASK, IP_PROTO_RESERVED);
        tuple_entry->pbit = (lkp_entry.protocol >> PBIT_KEY_SHIFT) & PBIT_KEY_MASK;
    }
    else
    {
        tuple_entry->prot = lkp_entry.protocol;
        tuple_entry->pbit = 0;
    }
#if defined(CONFIG_RNR_UNKNOWN_UCAST_FLOODING)
    tuple_entry->client_idx = lkp_entry.protocol - FLOOD_CLIENT_IDX_BIAS_L3;
#endif
#else
    RDD_TRACE_IPV4_FLOW_KEY("tuple_entry_ptr", lkp_entry.src_ip, lkp_entry.dst_ip, lkp_entry.src_port,
        lkp_entry.dst_port, lkp_entry.protocol, lkp_entry.tcp_pure_ack, lkp_entry.is_context_extension);

    if (lkp_entry.key_extend == bdmf_ip_family_ipv4)
    {
        tuple_entry->dst_ip.addr.ipv4 = lkp_entry.dst_ip;
#if defined(USE_NATC_VAR_CONTEXT_LEN)
        tuple_entry->dst_ip.addr.ipv4 = (tuple_entry->dst_ip.addr.ipv4 & ~0x0f) | lkp_entry.dst_ip_3_0;
#endif
        tuple_entry->src_ip.addr.ipv4 = lkp_entry.src_ip;
    }
    else
    {
        /* TODO - need to enhance and display also ipv6 addresses */
        memset(&tuple_entry->dst_ip.addr.ipv6, 0, 16);
        memset(&tuple_entry->src_ip.addr.ipv6, 0, 16);
    }
    tuple_entry->dst_port = lkp_entry.dst_port;
    tuple_entry->src_port = lkp_entry.src_port;
    tuple_entry->dst_ip.family = lkp_entry.key_extend;
    tuple_entry->src_ip.family = lkp_entry.key_extend;
    tuple_entry->lookup_port = lkp_entry.lookup_port;
    tuple_entry->tcp_pure_ack = lkp_entry.tcp_pure_ack;
    tuple_entry->is_ctx_ext = lkp_entry.is_context_extension;

    if (rdd_natc_pbit_key_mode_get())
    {
        /* The protocol field is re-purposed with pbit and l4_protocol */
        tuple_entry->prot = int2int_map_r(l4_protocol_to_l4_protocol_code, lkp_entry.protocol & PBIT_KEY_L4_PROTOCOL_MASK, IP_PROTO_RESERVED);
        tuple_entry->pbit = (lkp_entry.protocol >> PBIT_KEY_SHIFT) & PBIT_KEY_MASK;
    }
    else
    {
        tuple_entry->prot = lkp_entry.protocol;
        tuple_entry->pbit = 0;
    }

#if defined(CONFIG_RNR_UNKNOWN_UCAST_FLOODING)
    tuple_entry->client_idx = lkp_entry.client_idx;
#endif
    if (sub_tbl_id)
        *sub_tbl_id = lkp_entry.sub_table_id;
#endif
}

int rdd_ip_class_key_entry_var_size_ctx_compose(rdd_l3_flow_key_t *tuple_entry, uint8_t *connection_entry,
    uint8_t *connection_entry_no_size, rdd_ctx_size ctx_size)
{
    NAT_CACHE_L3_LKP_ENTRY_STRUCT lkp_entry = {};
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t ipv6_src_ip_crc, ipv6_dst_ip_crc;
#if CHIP_VER < RDP_GEN_60 && !defined(OPERATION_MODE_PRV)
    uint32_t key0_mask, *key0;
#endif
    uint8_t l4_protocol;

    RDD_BTRACE("tuple_entry %px, connection_entry = %px, vport = %d\n", tuple_entry, connection_entry,
        tuple_entry->lookup_port);
#if CHIP_VER >= RDP_GEN_60
    RDD_TRACE_RGEN6_IPV4_FLOW_KEY("tuple_entry_ptr", lkp_entry.src_ip, lkp_entry.dst_ip, lkp_entry.src_port,
        lkp_entry.dst_port, lkp_entry.protocol, lkp_entry.tcp_pure_ack, lkp_entry.tos, lkp_entry.vlans_num, lkp_entry.valid);
#else
    RDD_TRACE_IPV4_FLOW_KEY("tuple_entry", tuple_entry->src_ip.addr.ipv4, tuple_entry->dst_ip.addr.ipv4,
        tuple_entry->src_port, tuple_entry->dst_port, tuple_entry->prot, tuple_entry->tcp_pure_ack, tuple_entry->is_ctx_ext);
#endif

    if (tuple_entry->dst_ip.family == bdmf_ip_family_ipv4)
    {
        lkp_entry.dst_ip = tuple_entry->dst_ip.addr.ipv4;
        lkp_entry.src_ip = tuple_entry->src_ip.addr.ipv4;
    }
    else
    {
        rdd_crc_ipv6_addr_calc(&tuple_entry->src_ip, &ipv6_src_ip_crc);
        rdd_crc_ipv6_addr_calc(&tuple_entry->dst_ip, &ipv6_dst_ip_crc);
        lkp_entry.dst_ip = ipv6_dst_ip_crc;
        lkp_entry.src_ip = ipv6_src_ip_crc;
    }

    lkp_entry.dst_port = tuple_entry->dst_port;
    lkp_entry.src_port = tuple_entry->src_port;
    if (rdd_natc_pbit_key_mode_get())
    {
        /* The protocol field is re-purposed with pbit and l4_protocol */
        l4_protocol = int2int_map(l4_protocol_to_l4_protocol_code, tuple_entry->prot, IP_PROTO_RESERVED);
        lkp_entry.protocol = l4_protocol | ((tuple_entry->pbit & PBIT_KEY_MASK) << PBIT_KEY_SHIFT);
    }
    else
    {
        lkp_entry.protocol = tuple_entry->prot;
    }
    lkp_entry.tcp_pure_ack = tuple_entry->tcp_pure_ack;

    /* XXX: Add support for ingress port to support bonding correctly */
    lkp_entry.lookup_port = tuple_entry->lookup_port;

    lkp_entry.valid = 1;

#if CHIP_VER >= RDP_GEN_60
    lkp_entry.ipv6 = (tuple_entry->dst_ip.family == bdmf_ip_family_ipv6);
    lkp_entry.vlans_num = tuple_entry->vtag_num;
    lkp_entry.tos = tuple_entry->tos;

    lkp_entry.ctx_ext = tuple_entry->is_ctx_ext;

    /* TODO! how to obtain the key mask and apply it here */

#if defined(CONFIG_RNR_UNKNOWN_UCAST_FLOODING)
    if (tuple_entry->client_idx)
    {
        lkp_entry.client_idx = tuple_entry->client_idx + FLOOD_CLIENT_IDX_BIAS_L3;
    }
    /*else: do not fill them otherwise because it is overlapped with protocol field */
#endif
    if (ctx_size != CTX_SIZE_NOT_USED && connection_entry_no_size)
    {
        /* Add ctx_size to last 4 bit position. */
        NAT_CACHE_LKP_ENTRY_VAR_SIZE_CTX_STRUCT *lkp_entry_vsc = (NAT_CACHE_LKP_ENTRY_VAR_SIZE_CTX_STRUCT *)&lkp_entry;

        lkp_entry_vsc->var_size_ctx = 0;
        memcpy(connection_entry_no_size, &lkp_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
        lkp_entry_vsc->var_size_ctx = ctx_size;
        memcpy(connection_entry, &lkp_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));

#else

    lkp_entry.key_extend = tuple_entry->dst_ip.family;
    lkp_entry.is_context_extension = tuple_entry->is_ctx_ext;
#if defined(CONFIG_RNR_UNKNOWN_UCAST_FLOODING)
    lkp_entry.client_idx = tuple_entry->client_idx;
#endif

#ifndef OPERATION_MODE_PRV
    rdd_ag_natc_nat_cache_key0_mask_get(&key0_mask);
    key0 = (uint32_t *)&lkp_entry;
    *key0 &= key0_mask;
#endif

    if (ctx_size != CTX_SIZE_NOT_USED && connection_entry_no_size)
    {
        /* Move destination IP [3:0] to another location and add variable length context size at that bit position. */
        NAT_CACHE_LKP_ENTRY_VAR_SIZE_CTX_STRUCT *lkp_entry_vsc = (NAT_CACHE_LKP_ENTRY_VAR_SIZE_CTX_STRUCT *)&lkp_entry;

        lkp_entry.dst_ip_3_0 = lkp_entry_vsc->var_size_ctx; /* dst_ip bits [3:0] */
        lkp_entry_vsc->var_size_ctx = 0;
        memcpy(connection_entry_no_size, &lkp_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
        lkp_entry_vsc->var_size_ctx = ctx_size;
        memcpy(connection_entry, &lkp_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
#endif

#ifdef FIRMWARE_LITTLE_ENDIAN
        SWAPBYTES(connection_entry_no_size, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
        SWAPBYTES(connection_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
#endif
    }
    else
    {
        memcpy(connection_entry, &lkp_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
#ifdef FIRMWARE_LITTLE_ENDIAN
        SWAPBYTES(connection_entry, sizeof(NAT_CACHE_L3_LKP_ENTRY_STRUCT));
#endif
    }

    return rc;
}

int rdd_ip_class_key_entry_compose(rdd_l3_flow_key_t *tuple_entry, uint8_t *connection_entry)
{
    return rdd_ip_class_key_entry_var_size_ctx_compose(tuple_entry, connection_entry, NULL, CTX_SIZE_NOT_USED);
}
#endif

void rdd_3_tupples_ip_flows_enable(bdmf_boolean enable)
{
    RDD_NAT_CACHE_CFG_THREE_TUPLE_ENABLE_WRITE_G(enable, RDD_NAT_CACHE_CFG_ADDRESS_ARR, 0);
}

void rdd_esp_filter_set(rdd_action action)
{
    RDD_NAT_CACHE_CFG_ESP_FILTER_ENABLE_WRITE_G(1, RDD_NAT_CACHE_CFG_ADDRESS_ARR, 0);
    RDD_NAT_CACHE_CFG_ESP_FILTER_ACTION_WRITE_G((action == ACTION_FORWARD) ? 1 : 0, RDD_NAT_CACHE_CFG_ADDRESS_ARR, 0);
}

