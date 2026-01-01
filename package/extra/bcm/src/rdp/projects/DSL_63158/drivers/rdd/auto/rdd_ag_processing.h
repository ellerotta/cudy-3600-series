/*
   Copyright (c) 2015 Broadcom
   All Rights Reserved

    <:label-BRCM:2015:DUAL/GPL:standard

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.

    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.

:>
*/



/* This is an automated file. Do not edit its contents. */


#ifndef _RDD_AG_PROCESSING_H_
#define _RDD_AG_PROCESSING_H_

typedef struct rdd_vport_cfg_entry_s
{
    bdmf_boolean sa_lookup_en;
    bdmf_boolean da_lookup_en;
    uint8_t sa_lookup_miss_action;
    uint8_t da_lookup_miss_action;
    bdmf_boolean congestion_flow_control;
    bdmf_boolean discard_prty;
    bdmf_boolean mcast_whitelist_skip;
    uint8_t natc_tbl_id;
} rdd_vport_cfg_entry_t;

int rdd_ag_processing_mirroring_truncate_entry_get(uint32_t _entry, rdd_mirroring_truncate_entry_t *mirroring_truncate_entry);
int rdd_ag_processing_mirroring_truncate_entry_set(uint32_t _entry, rdd_mirroring_truncate_entry_t *mirroring_truncate_entry);
int rdd_ag_processing_mirroring_truncate_entry_get_core(uint32_t _entry, rdd_mirroring_truncate_entry_t *mirroring_truncate_entry, int core_id);
int rdd_ag_processing_mirroring_truncate_entry_set_core(uint32_t _entry, rdd_mirroring_truncate_entry_t *mirroring_truncate_entry, int core_id);
int rdd_ag_processing_vport_cfg_entry_get(uint32_t _entry, rdd_vport_cfg_entry_t *vport_cfg_entry);
int rdd_ag_processing_vport_cfg_entry_set(uint32_t _entry, rdd_vport_cfg_entry_t *vport_cfg_entry);
int rdd_ag_processing_vport_cfg_entry_get_core(uint32_t _entry, rdd_vport_cfg_entry_t *vport_cfg_entry, int core_id);
int rdd_ag_processing_vport_cfg_entry_set_core(uint32_t _entry, rdd_vport_cfg_entry_t *vport_cfg_entry, int core_id);
int rdd_ag_processing_aqm_enable_table_set(uint32_t _entry, uint32_t bits);
int rdd_ag_processing_aqm_enable_table_set_core(uint32_t _entry, uint32_t bits, int core_id);
int rdd_ag_processing_aqm_enable_table_get(uint32_t _entry, uint32_t *bits);
int rdd_ag_processing_aqm_enable_table_get_core(uint32_t _entry, uint32_t *bits, int core_id);
int rdd_ag_processing_aqm_num_queues_set(uint16_t bits);
int rdd_ag_processing_aqm_num_queues_set_core(uint16_t bits, int core_id);
int rdd_ag_processing_aqm_num_queues_get(uint16_t *bits);
int rdd_ag_processing_aqm_num_queues_get_core(uint16_t *bits, int core_id);
int rdd_ag_processing_rx_mirroring_table_truncate_offset_set(uint32_t _entry, uint16_t truncate_offset);
int rdd_ag_processing_rx_mirroring_table_truncate_offset_set_core(uint32_t _entry, uint16_t truncate_offset, int core_id);
int rdd_ag_processing_rx_mirroring_table_truncate_offset_get(uint32_t _entry, uint16_t *truncate_offset);
int rdd_ag_processing_rx_mirroring_table_truncate_offset_get_core(uint32_t _entry, uint16_t *truncate_offset, int core_id);
int rdd_ag_processing_spdtest_num_of_rx_flows_set(uint8_t bits);
int rdd_ag_processing_spdtest_num_of_rx_flows_set_core(uint8_t bits, int core_id);
int rdd_ag_processing_spdtest_num_of_rx_flows_get(uint8_t *bits);
int rdd_ag_processing_spdtest_num_of_rx_flows_get_core(uint8_t *bits, int core_id);
int rdd_ag_processing_tr471_spdsvc_rx_pkt_id_table_set(uint32_t _entry, uint32_t src_ipaddr, uint32_t dst_ipaddr, uint16_t src_port, uint16_t dst_port);
int rdd_ag_processing_tr471_spdsvc_rx_pkt_id_table_set_core(uint32_t _entry, uint32_t src_ipaddr, uint32_t dst_ipaddr, uint16_t src_port, uint16_t dst_port, int core_id);
int rdd_ag_processing_tr471_spdsvc_rx_pkt_id_table_get(uint32_t _entry, uint32_t *src_ipaddr, uint32_t *dst_ipaddr, uint16_t *src_port, uint16_t *dst_port);
int rdd_ag_processing_tr471_spdsvc_rx_pkt_id_table_get_core(uint32_t _entry, uint32_t *src_ipaddr, uint32_t *dst_ipaddr, uint16_t *src_port, uint16_t *dst_port, int core_id);
int rdd_ag_processing_vport_cfg_table_loopback_en_set(uint32_t _entry, bdmf_boolean loopback_en);
int rdd_ag_processing_vport_cfg_table_loopback_en_set_core(uint32_t _entry, bdmf_boolean loopback_en, int core_id);
int rdd_ag_processing_vport_cfg_table_loopback_en_get(uint32_t _entry, bdmf_boolean *loopback_en);
int rdd_ag_processing_vport_cfg_table_loopback_en_get_core(uint32_t _entry, bdmf_boolean *loopback_en, int core_id);
int rdd_ag_processing_vport_cfg_table_mirroring_en_set(uint32_t _entry, bdmf_boolean mirroring_en);
int rdd_ag_processing_vport_cfg_table_mirroring_en_set_core(uint32_t _entry, bdmf_boolean mirroring_en, int core_id);
int rdd_ag_processing_vport_cfg_table_mirroring_en_get(uint32_t _entry, bdmf_boolean *mirroring_en);
int rdd_ag_processing_vport_cfg_table_mirroring_en_get_core(uint32_t _entry, bdmf_boolean *mirroring_en, int core_id);
int rdd_ag_processing_vport_cfg_table_egress_isolation_en_set(uint32_t _entry, bdmf_boolean egress_isolation_en);
int rdd_ag_processing_vport_cfg_table_egress_isolation_en_set_core(uint32_t _entry, bdmf_boolean egress_isolation_en, int core_id);
int rdd_ag_processing_vport_cfg_table_egress_isolation_en_get(uint32_t _entry, bdmf_boolean *egress_isolation_en);
int rdd_ag_processing_vport_cfg_table_egress_isolation_en_get_core(uint32_t _entry, bdmf_boolean *egress_isolation_en, int core_id);
int rdd_ag_processing_vport_cfg_table_ingress_isolation_en_set(uint32_t _entry, bdmf_boolean ingress_isolation_en);
int rdd_ag_processing_vport_cfg_table_ingress_isolation_en_set_core(uint32_t _entry, bdmf_boolean ingress_isolation_en, int core_id);
int rdd_ag_processing_vport_cfg_table_ingress_isolation_en_get(uint32_t _entry, bdmf_boolean *ingress_isolation_en);
int rdd_ag_processing_vport_cfg_table_ingress_isolation_en_get_core(uint32_t _entry, bdmf_boolean *ingress_isolation_en, int core_id);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_ingress_lookup_method_set(uint32_t _entry, bdmf_boolean bridge_and_vlan_ingress_lookup_method);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_ingress_lookup_method_set_core(uint32_t _entry, bdmf_boolean bridge_and_vlan_ingress_lookup_method, int core_id);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_ingress_lookup_method_get(uint32_t _entry, bdmf_boolean *bridge_and_vlan_ingress_lookup_method);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_ingress_lookup_method_get_core(uint32_t _entry, bdmf_boolean *bridge_and_vlan_ingress_lookup_method, int core_id);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_egress_lookup_method_set(uint32_t _entry, bdmf_boolean bridge_and_vlan_egress_lookup_method);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_egress_lookup_method_set_core(uint32_t _entry, bdmf_boolean bridge_and_vlan_egress_lookup_method, int core_id);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_egress_lookup_method_get(uint32_t _entry, bdmf_boolean *bridge_and_vlan_egress_lookup_method);
int rdd_ag_processing_vport_cfg_table_bridge_and_vlan_egress_lookup_method_get_core(uint32_t _entry, bdmf_boolean *bridge_and_vlan_egress_lookup_method, int core_id);
int rdd_ag_processing_vport_cfg_table_protocol_filters_dis_set(uint32_t _entry, uint8_t protocol_filters_dis);
int rdd_ag_processing_vport_cfg_table_protocol_filters_dis_set_core(uint32_t _entry, uint8_t protocol_filters_dis, int core_id);
int rdd_ag_processing_vport_cfg_table_protocol_filters_dis_get(uint32_t _entry, uint8_t *protocol_filters_dis);
int rdd_ag_processing_vport_cfg_table_protocol_filters_dis_get_core(uint32_t _entry, uint8_t *protocol_filters_dis, int core_id);
int rdd_ag_processing_vport_cfg_table_congestion_flow_control_set(uint32_t _entry, bdmf_boolean congestion_flow_control);
int rdd_ag_processing_vport_cfg_table_congestion_flow_control_set_core(uint32_t _entry, bdmf_boolean congestion_flow_control, int core_id);
int rdd_ag_processing_vport_cfg_table_congestion_flow_control_get(uint32_t _entry, bdmf_boolean *congestion_flow_control);
int rdd_ag_processing_vport_cfg_table_congestion_flow_control_get_core(uint32_t _entry, bdmf_boolean *congestion_flow_control, int core_id);
int rdd_ag_processing_vport_cfg_table_ingress_filter_profile_set(uint32_t _entry, uint8_t ingress_filter_profile);
int rdd_ag_processing_vport_cfg_table_ingress_filter_profile_set_core(uint32_t _entry, uint8_t ingress_filter_profile, int core_id);
int rdd_ag_processing_vport_cfg_table_ingress_filter_profile_get(uint32_t _entry, uint8_t *ingress_filter_profile);
int rdd_ag_processing_vport_cfg_table_ingress_filter_profile_get_core(uint32_t _entry, uint8_t *ingress_filter_profile, int core_id);
int rdd_ag_processing_vport_cfg_table_mcast_whitelist_skip_set(uint32_t _entry, bdmf_boolean mcast_whitelist_skip);
int rdd_ag_processing_vport_cfg_table_mcast_whitelist_skip_set_core(uint32_t _entry, bdmf_boolean mcast_whitelist_skip, int core_id);
int rdd_ag_processing_vport_cfg_table_mcast_whitelist_skip_get(uint32_t _entry, bdmf_boolean *mcast_whitelist_skip);
int rdd_ag_processing_vport_cfg_table_mcast_whitelist_skip_get_core(uint32_t _entry, bdmf_boolean *mcast_whitelist_skip, int core_id);
int rdd_ag_processing_vport_cfg_table_natc_tbl_id_set(uint32_t _entry, uint8_t natc_tbl_id);
int rdd_ag_processing_vport_cfg_table_natc_tbl_id_set_core(uint32_t _entry, uint8_t natc_tbl_id, int core_id);
int rdd_ag_processing_vport_cfg_table_natc_tbl_id_get(uint32_t _entry, uint8_t *natc_tbl_id);
int rdd_ag_processing_vport_cfg_table_natc_tbl_id_get_core(uint32_t _entry, uint8_t *natc_tbl_id, int core_id);
int rdd_ag_processing_vport_cfg_table_ls_fc_cfg_set(uint32_t _entry, bdmf_boolean ls_fc_cfg);
int rdd_ag_processing_vport_cfg_table_ls_fc_cfg_set_core(uint32_t _entry, bdmf_boolean ls_fc_cfg, int core_id);
int rdd_ag_processing_vport_cfg_table_ls_fc_cfg_get(uint32_t _entry, bdmf_boolean *ls_fc_cfg);
int rdd_ag_processing_vport_cfg_table_ls_fc_cfg_get_core(uint32_t _entry, bdmf_boolean *ls_fc_cfg, int core_id);
int rdd_ag_processing_vport_cfg_table_egress_isolation_map_set(uint32_t _entry, uint32_t egress_isolation_map);
int rdd_ag_processing_vport_cfg_table_egress_isolation_map_set_core(uint32_t _entry, uint32_t egress_isolation_map, int core_id);
int rdd_ag_processing_vport_cfg_table_egress_isolation_map_get(uint32_t _entry, uint32_t *egress_isolation_map);
int rdd_ag_processing_vport_cfg_table_egress_isolation_map_get_core(uint32_t _entry, uint32_t *egress_isolation_map, int core_id);
int rdd_ag_processing_vport_to_lookup_port_mapping_table_set(uint32_t _entry, uint8_t bits);
int rdd_ag_processing_vport_to_lookup_port_mapping_table_set_core(uint32_t _entry, uint8_t bits, int core_id);
int rdd_ag_processing_vport_to_lookup_port_mapping_table_get(uint32_t _entry, uint8_t *bits);
int rdd_ag_processing_vport_to_lookup_port_mapping_table_get_core(uint32_t _entry, uint8_t *bits, int core_id);
int rdd_ag_processing_vport_to_rl_overhead_table_set(uint32_t _entry, uint8_t rl_overhead);
int rdd_ag_processing_vport_to_rl_overhead_table_set_core(uint32_t _entry, uint8_t rl_overhead, int core_id);
int rdd_ag_processing_vport_to_rl_overhead_table_get(uint32_t _entry, uint8_t *rl_overhead);
int rdd_ag_processing_vport_to_rl_overhead_table_get_core(uint32_t _entry, uint8_t *rl_overhead, int core_id);
int rdd_ag_processing_vport_to_rl_overhead_table_rl_overhead_set(uint32_t _entry, uint8_t rl_overhead);
int rdd_ag_processing_vport_to_rl_overhead_table_rl_overhead_set_core(uint32_t _entry, uint8_t rl_overhead, int core_id);
int rdd_ag_processing_vport_to_rl_overhead_table_rl_overhead_get(uint32_t _entry, uint8_t *rl_overhead);
int rdd_ag_processing_vport_to_rl_overhead_table_rl_overhead_get_core(uint32_t _entry, uint8_t *rl_overhead, int core_id);

#endif /* _RDD_AG_PROCESSING_H_ */
