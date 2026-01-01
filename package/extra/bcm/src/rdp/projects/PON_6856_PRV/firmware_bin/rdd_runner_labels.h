/* IMAGE 0 LABELS */
#ifndef IMAGE_0_CODE_ADDRESSES
#define IMAGE_0_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_0_budget_allocator_1st_wakeup_request        (0x790)
#define image_0_debug_routine        (0x9c)
#define image_0_debug_routine_handler        (0x4)
#define image_0_ds_tx_task_wakeup_request        (0x300)
#define image_0_flush_task_1st_wakeup_request        (0xd44)
#define image_0_ghost_reporting_1st_wakeup_request        (0xe6c)
#define image_0_initialization_task        (0x48)
#define image_0_start_task_budget_allocator_1st_wakeup_request        (0x790)
#define image_0_start_task_debug_routine        (0x9c)
#define image_0_start_task_ds_tx_task_wakeup_request        (0x300)
#define image_0_start_task_flush_task_1st_wakeup_request        (0xd44)
#define image_0_start_task_ghost_reporting_1st_wakeup_request        (0xe6c)
#define image_0_start_task_initialization_task        (0x48)
#define image_0_start_task_update_fifo_ds_read_1st_wakeup_request        (0x12dc)
#define image_0_update_fifo_ds_read_1st_wakeup_request        (0x12dc)

#else

#define image_0_budget_allocator_1st_wakeup_request        (0x1e4)
#define image_0_debug_routine        (0x27)
#define image_0_debug_routine_handler        (0x1)
#define image_0_ds_tx_task_wakeup_request        (0xc0)
#define image_0_flush_task_1st_wakeup_request        (0x351)
#define image_0_ghost_reporting_1st_wakeup_request        (0x39b)
#define image_0_initialization_task        (0x12)
#define image_0_start_task_budget_allocator_1st_wakeup_request        (0x1e4)
#define image_0_start_task_debug_routine        (0x27)
#define image_0_start_task_ds_tx_task_wakeup_request        (0xc0)
#define image_0_start_task_flush_task_1st_wakeup_request        (0x351)
#define image_0_start_task_ghost_reporting_1st_wakeup_request        (0x39b)
#define image_0_start_task_initialization_task        (0x12)
#define image_0_start_task_update_fifo_ds_read_1st_wakeup_request        (0x4b7)
#define image_0_update_fifo_ds_read_1st_wakeup_request        (0x4b7)

#endif


#endif

/* IMAGE 1 LABELS */
#ifndef IMAGE_1_CODE_ADDRESSES
#define IMAGE_1_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_1_cpu_recycle_wakeup_request        (0x4f2c)
#define image_1_cpu_rx_copy_wakeup_request        (0x3ee4)
#define image_1_cpu_rx_meter_budget_allocator_1st_wakeup_request        (0x4d4c)
#define image_1_cpu_rx_wakeup_request        (0x4a24)
#define image_1_debug_routine        (0x3dd8)
#define image_1_debug_routine_handler        (0x4)
#define image_1_gpe_cmd_copy_bits_16        (0x3478)
#define image_1_gpe_cmd_replace_16        (0x3438)
#define image_1_gpe_cmd_replace_32        (0x3440)
#define image_1_gpe_cmd_replace_bits_16        (0x3458)
#define image_1_gpe_cmd_skip_if        (0x34a8)
#define image_1_gpe_vlan_action_cmd_drop        (0x34c8)
#define image_1_gpe_vlan_action_cmd_dscp        (0x34cc)
#define image_1_gpe_vlan_action_cmd_mac_hdr_copy        (0x3520)
#define image_1_initialization_task        (0x8)
#define image_1_interrupt_coalescing_1st_wakeup_request        (0x5088)
#define image_1_processing_wakeup_request        (0xb20)
#define image_1_start_task_cpu_recycle_wakeup_request        (0x4f2c)
#define image_1_start_task_cpu_rx_copy_wakeup_request        (0x3ee4)
#define image_1_start_task_cpu_rx_meter_budget_allocator_1st_wakeup_request        (0x4d4c)
#define image_1_start_task_cpu_rx_wakeup_request        (0x4a24)
#define image_1_start_task_debug_routine        (0x3dd8)
#define image_1_start_task_initialization_task        (0x8)
#define image_1_start_task_interrupt_coalescing_1st_wakeup_request        (0x5088)
#define image_1_start_task_processing_wakeup_request        (0xb20)
#define image_1_tcam_cmd_bc        (0x2b20)
#define image_1_tcam_cmd_dst_ip        (0x2ccc)
#define image_1_tcam_cmd_dst_ipv6_masked        (0x2d20)
#define image_1_tcam_cmd_dst_mac        (0x2db0)
#define image_1_tcam_cmd_dst_port        (0x2d6c)
#define image_1_tcam_cmd_ethertype        (0x2aa8)
#define image_1_tcam_cmd_gem_flow        (0x2c00)
#define image_1_tcam_cmd_generic_l2        (0x2ddc)
#define image_1_tcam_cmd_generic_l3        (0x2e10)
#define image_1_tcam_cmd_generic_l4        (0x2e44)
#define image_1_tcam_cmd_ic_submit        (0x2a2c)
#define image_1_tcam_cmd_ingress_port        (0x2be8)
#define image_1_tcam_cmd_inner_pbit        (0x2b08)
#define image_1_tcam_cmd_inner_tpid        (0x2a90)
#define image_1_tcam_cmd_inner_vid        (0x2ad8)
#define image_1_tcam_cmd_ip_protocol        (0x2b80)
#define image_1_tcam_cmd_ipv6_label        (0x2c18)
#define image_1_tcam_cmd_l3_protocol        (0x2b98)
#define image_1_tcam_cmd_mc        (0x2b38)
#define image_1_tcam_cmd_mc_l3        (0x2b50)
#define image_1_tcam_cmd_network_layer        (0x2bc8)
#define image_1_tcam_cmd_outer_pbit        (0x2af0)
#define image_1_tcam_cmd_outer_tpid        (0x2a78)
#define image_1_tcam_cmd_outer_vid        (0x2ac0)
#define image_1_tcam_cmd_src_ip        (0x2c44)
#define image_1_tcam_cmd_src_ipv6_masked        (0x2c98)
#define image_1_tcam_cmd_src_mac        (0x2d84)
#define image_1_tcam_cmd_src_port        (0x2d54)
#define image_1_tcam_cmd_tos        (0x2bb0)
#define image_1_tcam_cmd_vlan_num        (0x2b68)

#else

#define image_1_cpu_recycle_wakeup_request        (0x13cb)
#define image_1_cpu_rx_copy_wakeup_request        (0xfb9)
#define image_1_cpu_rx_meter_budget_allocator_1st_wakeup_request        (0x1353)
#define image_1_cpu_rx_wakeup_request        (0x1289)
#define image_1_debug_routine        (0xf76)
#define image_1_debug_routine_handler        (0x1)
#define image_1_gpe_cmd_copy_bits_16        (0xd1e)
#define image_1_gpe_cmd_replace_16        (0xd0e)
#define image_1_gpe_cmd_replace_32        (0xd10)
#define image_1_gpe_cmd_replace_bits_16        (0xd16)
#define image_1_gpe_cmd_skip_if        (0xd2a)
#define image_1_gpe_vlan_action_cmd_drop        (0xd32)
#define image_1_gpe_vlan_action_cmd_dscp        (0xd33)
#define image_1_gpe_vlan_action_cmd_mac_hdr_copy        (0xd48)
#define image_1_initialization_task        (0x2)
#define image_1_interrupt_coalescing_1st_wakeup_request        (0x1422)
#define image_1_processing_wakeup_request        (0x2c8)
#define image_1_start_task_cpu_recycle_wakeup_request        (0x13cb)
#define image_1_start_task_cpu_rx_copy_wakeup_request        (0xfb9)
#define image_1_start_task_cpu_rx_meter_budget_allocator_1st_wakeup_request        (0x1353)
#define image_1_start_task_cpu_rx_wakeup_request        (0x1289)
#define image_1_start_task_debug_routine        (0xf76)
#define image_1_start_task_initialization_task        (0x2)
#define image_1_start_task_interrupt_coalescing_1st_wakeup_request        (0x1422)
#define image_1_start_task_processing_wakeup_request        (0x2c8)
#define image_1_tcam_cmd_bc        (0xac8)
#define image_1_tcam_cmd_dst_ip        (0xb33)
#define image_1_tcam_cmd_dst_ipv6_masked        (0xb48)
#define image_1_tcam_cmd_dst_mac        (0xb6c)
#define image_1_tcam_cmd_dst_port        (0xb5b)
#define image_1_tcam_cmd_ethertype        (0xaaa)
#define image_1_tcam_cmd_gem_flow        (0xb00)
#define image_1_tcam_cmd_generic_l2        (0xb77)
#define image_1_tcam_cmd_generic_l3        (0xb84)
#define image_1_tcam_cmd_generic_l4        (0xb91)
#define image_1_tcam_cmd_ic_submit        (0xa8b)
#define image_1_tcam_cmd_ingress_port        (0xafa)
#define image_1_tcam_cmd_inner_pbit        (0xac2)
#define image_1_tcam_cmd_inner_tpid        (0xaa4)
#define image_1_tcam_cmd_inner_vid        (0xab6)
#define image_1_tcam_cmd_ip_protocol        (0xae0)
#define image_1_tcam_cmd_ipv6_label        (0xb06)
#define image_1_tcam_cmd_l3_protocol        (0xae6)
#define image_1_tcam_cmd_mc        (0xace)
#define image_1_tcam_cmd_mc_l3        (0xad4)
#define image_1_tcam_cmd_network_layer        (0xaf2)
#define image_1_tcam_cmd_outer_pbit        (0xabc)
#define image_1_tcam_cmd_outer_tpid        (0xa9e)
#define image_1_tcam_cmd_outer_vid        (0xab0)
#define image_1_tcam_cmd_src_ip        (0xb11)
#define image_1_tcam_cmd_src_ipv6_masked        (0xb26)
#define image_1_tcam_cmd_src_mac        (0xb61)
#define image_1_tcam_cmd_src_port        (0xb55)
#define image_1_tcam_cmd_tos        (0xaec)
#define image_1_tcam_cmd_vlan_num        (0xada)

#endif


#endif

/* IMAGE 2 LABELS */
#ifndef IMAGE_2_CODE_ADDRESSES
#define IMAGE_2_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_2_cpu_recycle_wakeup_request        (0x4560)
#define image_2_cpu_tx_read_ring_indices        (0x3ee4)
#define image_2_cpu_tx_wakeup_request        (0x3ee4)
#define image_2_debug_routine        (0x48)
#define image_2_debug_routine_handler        (0x4)
#define image_2_gpe_cmd_copy_bits_16        (0x3584)
#define image_2_gpe_cmd_replace_16        (0x3544)
#define image_2_gpe_cmd_replace_32        (0x354c)
#define image_2_gpe_cmd_replace_bits_16        (0x3564)
#define image_2_gpe_cmd_skip_if        (0x35b4)
#define image_2_gpe_vlan_action_cmd_drop        (0x35d4)
#define image_2_gpe_vlan_action_cmd_dscp        (0x35d8)
#define image_2_gpe_vlan_action_cmd_mac_hdr_copy        (0x362c)
#define image_2_initialization_task        (0x8)
#define image_2_interrupt_coalescing_1st_wakeup_request        (0x4980)
#define image_2_processing_wakeup_request        (0xc2c)
#define image_2_start_task_cpu_recycle_wakeup_request        (0x4560)
#define image_2_start_task_cpu_tx_wakeup_request        (0x3ee4)
#define image_2_start_task_debug_routine        (0x48)
#define image_2_start_task_initialization_task        (0x8)
#define image_2_start_task_interrupt_coalescing_1st_wakeup_request        (0x4980)
#define image_2_start_task_processing_wakeup_request        (0xc2c)
#define image_2_start_task_timer_common_task_wakeup_request        (0x46bc)
#define image_2_tcam_cmd_bc        (0x2c2c)
#define image_2_tcam_cmd_dst_ip        (0x2dd8)
#define image_2_tcam_cmd_dst_ipv6_masked        (0x2e2c)
#define image_2_tcam_cmd_dst_mac        (0x2ebc)
#define image_2_tcam_cmd_dst_port        (0x2e78)
#define image_2_tcam_cmd_ethertype        (0x2bb4)
#define image_2_tcam_cmd_gem_flow        (0x2d0c)
#define image_2_tcam_cmd_generic_l2        (0x2ee8)
#define image_2_tcam_cmd_generic_l3        (0x2f1c)
#define image_2_tcam_cmd_generic_l4        (0x2f50)
#define image_2_tcam_cmd_ic_submit        (0x2b38)
#define image_2_tcam_cmd_ingress_port        (0x2cf4)
#define image_2_tcam_cmd_inner_pbit        (0x2c14)
#define image_2_tcam_cmd_inner_tpid        (0x2b9c)
#define image_2_tcam_cmd_inner_vid        (0x2be4)
#define image_2_tcam_cmd_ip_protocol        (0x2c8c)
#define image_2_tcam_cmd_ipv6_label        (0x2d24)
#define image_2_tcam_cmd_l3_protocol        (0x2ca4)
#define image_2_tcam_cmd_mc        (0x2c44)
#define image_2_tcam_cmd_mc_l3        (0x2c5c)
#define image_2_tcam_cmd_network_layer        (0x2cd4)
#define image_2_tcam_cmd_outer_pbit        (0x2bfc)
#define image_2_tcam_cmd_outer_tpid        (0x2b84)
#define image_2_tcam_cmd_outer_vid        (0x2bcc)
#define image_2_tcam_cmd_src_ip        (0x2d50)
#define image_2_tcam_cmd_src_ipv6_masked        (0x2da4)
#define image_2_tcam_cmd_src_mac        (0x2e90)
#define image_2_tcam_cmd_src_port        (0x2e60)
#define image_2_tcam_cmd_tos        (0x2cbc)
#define image_2_tcam_cmd_vlan_num        (0x2c74)
#define image_2_timer_common_task_wakeup_request        (0x46bc)

#else

#define image_2_cpu_recycle_wakeup_request        (0x1158)
#define image_2_cpu_tx_read_ring_indices        (0xfb9)
#define image_2_cpu_tx_wakeup_request        (0xfb9)
#define image_2_debug_routine        (0x12)
#define image_2_debug_routine_handler        (0x1)
#define image_2_gpe_cmd_copy_bits_16        (0xd61)
#define image_2_gpe_cmd_replace_16        (0xd51)
#define image_2_gpe_cmd_replace_32        (0xd53)
#define image_2_gpe_cmd_replace_bits_16        (0xd59)
#define image_2_gpe_cmd_skip_if        (0xd6d)
#define image_2_gpe_vlan_action_cmd_drop        (0xd75)
#define image_2_gpe_vlan_action_cmd_dscp        (0xd76)
#define image_2_gpe_vlan_action_cmd_mac_hdr_copy        (0xd8b)
#define image_2_initialization_task        (0x2)
#define image_2_interrupt_coalescing_1st_wakeup_request        (0x1260)
#define image_2_processing_wakeup_request        (0x30b)
#define image_2_start_task_cpu_recycle_wakeup_request        (0x1158)
#define image_2_start_task_cpu_tx_wakeup_request        (0xfb9)
#define image_2_start_task_debug_routine        (0x12)
#define image_2_start_task_initialization_task        (0x2)
#define image_2_start_task_interrupt_coalescing_1st_wakeup_request        (0x1260)
#define image_2_start_task_processing_wakeup_request        (0x30b)
#define image_2_start_task_timer_common_task_wakeup_request        (0x11af)
#define image_2_tcam_cmd_bc        (0xb0b)
#define image_2_tcam_cmd_dst_ip        (0xb76)
#define image_2_tcam_cmd_dst_ipv6_masked        (0xb8b)
#define image_2_tcam_cmd_dst_mac        (0xbaf)
#define image_2_tcam_cmd_dst_port        (0xb9e)
#define image_2_tcam_cmd_ethertype        (0xaed)
#define image_2_tcam_cmd_gem_flow        (0xb43)
#define image_2_tcam_cmd_generic_l2        (0xbba)
#define image_2_tcam_cmd_generic_l3        (0xbc7)
#define image_2_tcam_cmd_generic_l4        (0xbd4)
#define image_2_tcam_cmd_ic_submit        (0xace)
#define image_2_tcam_cmd_ingress_port        (0xb3d)
#define image_2_tcam_cmd_inner_pbit        (0xb05)
#define image_2_tcam_cmd_inner_tpid        (0xae7)
#define image_2_tcam_cmd_inner_vid        (0xaf9)
#define image_2_tcam_cmd_ip_protocol        (0xb23)
#define image_2_tcam_cmd_ipv6_label        (0xb49)
#define image_2_tcam_cmd_l3_protocol        (0xb29)
#define image_2_tcam_cmd_mc        (0xb11)
#define image_2_tcam_cmd_mc_l3        (0xb17)
#define image_2_tcam_cmd_network_layer        (0xb35)
#define image_2_tcam_cmd_outer_pbit        (0xaff)
#define image_2_tcam_cmd_outer_tpid        (0xae1)
#define image_2_tcam_cmd_outer_vid        (0xaf3)
#define image_2_tcam_cmd_src_ip        (0xb54)
#define image_2_tcam_cmd_src_ipv6_masked        (0xb69)
#define image_2_tcam_cmd_src_mac        (0xba4)
#define image_2_tcam_cmd_src_port        (0xb98)
#define image_2_tcam_cmd_tos        (0xb2f)
#define image_2_tcam_cmd_vlan_num        (0xb1d)
#define image_2_timer_common_task_wakeup_request        (0x11af)

#endif


#endif

/* IMAGE 3 LABELS */
#ifndef IMAGE_3_CODE_ADDRESSES
#define IMAGE_3_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_3_budget_allocator_1st_wakeup_request        (0x14ec)
#define image_3_debug_routine        (0x48)
#define image_3_debug_routine_handler        (0x4)
#define image_3_epon_tx_task_wakeup_request        (0x134c)
#define image_3_epon_update_fifo_read_1st_wakeup_request        (0x2014)
#define image_3_flush_task_1st_wakeup_request        (0x1b80)
#define image_3_gpon_control_wakeup_request        (0x59c)
#define image_3_initialization_task        (0x8)
#define image_3_ovl_budget_allocator_1st_wakeup_request        (0x19c4)
#define image_3_start_task_budget_allocator_1st_wakeup_request        (0x14ec)
#define image_3_start_task_debug_routine        (0x48)
#define image_3_start_task_epon_tx_task_wakeup_request        (0x134c)
#define image_3_start_task_epon_update_fifo_read_1st_wakeup_request        (0x2014)
#define image_3_start_task_flush_task_1st_wakeup_request        (0x1b80)
#define image_3_start_task_gpon_control_wakeup_request        (0x59c)
#define image_3_start_task_initialization_task        (0x8)
#define image_3_start_task_ovl_budget_allocator_1st_wakeup_request        (0x19c4)
#define image_3_start_task_update_fifo_us_read_1st_wakeup_request        (0x1d28)
#define image_3_start_task_us_tx_task_1st_wakeup_request        (0x8f8)
#define image_3_update_fifo_us_read_1st_wakeup_request        (0x1d28)
#define image_3_us_tx_task_1st_wakeup_request        (0x8f8)
#define image_3_us_tx_task_wakeup_request        (0x8f8)

#else

#define image_3_budget_allocator_1st_wakeup_request        (0x53b)
#define image_3_debug_routine        (0x12)
#define image_3_debug_routine_handler        (0x1)
#define image_3_epon_tx_task_wakeup_request        (0x4d3)
#define image_3_epon_update_fifo_read_1st_wakeup_request        (0x805)
#define image_3_flush_task_1st_wakeup_request        (0x6e0)
#define image_3_gpon_control_wakeup_request        (0x167)
#define image_3_initialization_task        (0x2)
#define image_3_ovl_budget_allocator_1st_wakeup_request        (0x671)
#define image_3_start_task_budget_allocator_1st_wakeup_request        (0x53b)
#define image_3_start_task_debug_routine        (0x12)
#define image_3_start_task_epon_tx_task_wakeup_request        (0x4d3)
#define image_3_start_task_epon_update_fifo_read_1st_wakeup_request        (0x805)
#define image_3_start_task_flush_task_1st_wakeup_request        (0x6e0)
#define image_3_start_task_gpon_control_wakeup_request        (0x167)
#define image_3_start_task_initialization_task        (0x2)
#define image_3_start_task_ovl_budget_allocator_1st_wakeup_request        (0x671)
#define image_3_start_task_update_fifo_us_read_1st_wakeup_request        (0x74a)
#define image_3_start_task_us_tx_task_1st_wakeup_request        (0x23e)
#define image_3_update_fifo_us_read_1st_wakeup_request        (0x74a)
#define image_3_us_tx_task_1st_wakeup_request        (0x23e)
#define image_3_us_tx_task_wakeup_request        (0x23e)

#endif


#endif

/* IMAGE 4 LABELS */
#ifndef IMAGE_4_CODE_ADDRESSES
#define IMAGE_4_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_4_debug_routine        (0x48)
#define image_4_debug_routine_handler        (0x4)
#define image_4_gpe_cmd_copy_bits_16        (0x3584)
#define image_4_gpe_cmd_replace_16        (0x3544)
#define image_4_gpe_cmd_replace_32        (0x354c)
#define image_4_gpe_cmd_replace_bits_16        (0x3564)
#define image_4_gpe_cmd_skip_if        (0x35b4)
#define image_4_gpe_vlan_action_cmd_drop        (0x35d4)
#define image_4_gpe_vlan_action_cmd_dscp        (0x35d8)
#define image_4_gpe_vlan_action_cmd_mac_hdr_copy        (0x362c)
#define image_4_initialization_task        (0x8)
#define image_4_processing_wakeup_request        (0xc2c)
#define image_4_start_task_debug_routine        (0x48)
#define image_4_start_task_initialization_task        (0x8)
#define image_4_start_task_processing_wakeup_request        (0xc2c)
#define image_4_tcam_cmd_bc        (0x2c2c)
#define image_4_tcam_cmd_dst_ip        (0x2dd8)
#define image_4_tcam_cmd_dst_ipv6_masked        (0x2e2c)
#define image_4_tcam_cmd_dst_mac        (0x2ebc)
#define image_4_tcam_cmd_dst_port        (0x2e78)
#define image_4_tcam_cmd_ethertype        (0x2bb4)
#define image_4_tcam_cmd_gem_flow        (0x2d0c)
#define image_4_tcam_cmd_generic_l2        (0x2ee8)
#define image_4_tcam_cmd_generic_l3        (0x2f1c)
#define image_4_tcam_cmd_generic_l4        (0x2f50)
#define image_4_tcam_cmd_ic_submit        (0x2b38)
#define image_4_tcam_cmd_ingress_port        (0x2cf4)
#define image_4_tcam_cmd_inner_pbit        (0x2c14)
#define image_4_tcam_cmd_inner_tpid        (0x2b9c)
#define image_4_tcam_cmd_inner_vid        (0x2be4)
#define image_4_tcam_cmd_ip_protocol        (0x2c8c)
#define image_4_tcam_cmd_ipv6_label        (0x2d24)
#define image_4_tcam_cmd_l3_protocol        (0x2ca4)
#define image_4_tcam_cmd_mc        (0x2c44)
#define image_4_tcam_cmd_mc_l3        (0x2c5c)
#define image_4_tcam_cmd_network_layer        (0x2cd4)
#define image_4_tcam_cmd_outer_pbit        (0x2bfc)
#define image_4_tcam_cmd_outer_tpid        (0x2b84)
#define image_4_tcam_cmd_outer_vid        (0x2bcc)
#define image_4_tcam_cmd_src_ip        (0x2d50)
#define image_4_tcam_cmd_src_ipv6_masked        (0x2da4)
#define image_4_tcam_cmd_src_mac        (0x2e90)
#define image_4_tcam_cmd_src_port        (0x2e60)
#define image_4_tcam_cmd_tos        (0x2cbc)
#define image_4_tcam_cmd_vlan_num        (0x2c74)

#else

#define image_4_debug_routine        (0x12)
#define image_4_debug_routine_handler        (0x1)
#define image_4_gpe_cmd_copy_bits_16        (0xd61)
#define image_4_gpe_cmd_replace_16        (0xd51)
#define image_4_gpe_cmd_replace_32        (0xd53)
#define image_4_gpe_cmd_replace_bits_16        (0xd59)
#define image_4_gpe_cmd_skip_if        (0xd6d)
#define image_4_gpe_vlan_action_cmd_drop        (0xd75)
#define image_4_gpe_vlan_action_cmd_dscp        (0xd76)
#define image_4_gpe_vlan_action_cmd_mac_hdr_copy        (0xd8b)
#define image_4_initialization_task        (0x2)
#define image_4_processing_wakeup_request        (0x30b)
#define image_4_start_task_debug_routine        (0x12)
#define image_4_start_task_initialization_task        (0x2)
#define image_4_start_task_processing_wakeup_request        (0x30b)
#define image_4_tcam_cmd_bc        (0xb0b)
#define image_4_tcam_cmd_dst_ip        (0xb76)
#define image_4_tcam_cmd_dst_ipv6_masked        (0xb8b)
#define image_4_tcam_cmd_dst_mac        (0xbaf)
#define image_4_tcam_cmd_dst_port        (0xb9e)
#define image_4_tcam_cmd_ethertype        (0xaed)
#define image_4_tcam_cmd_gem_flow        (0xb43)
#define image_4_tcam_cmd_generic_l2        (0xbba)
#define image_4_tcam_cmd_generic_l3        (0xbc7)
#define image_4_tcam_cmd_generic_l4        (0xbd4)
#define image_4_tcam_cmd_ic_submit        (0xace)
#define image_4_tcam_cmd_ingress_port        (0xb3d)
#define image_4_tcam_cmd_inner_pbit        (0xb05)
#define image_4_tcam_cmd_inner_tpid        (0xae7)
#define image_4_tcam_cmd_inner_vid        (0xaf9)
#define image_4_tcam_cmd_ip_protocol        (0xb23)
#define image_4_tcam_cmd_ipv6_label        (0xb49)
#define image_4_tcam_cmd_l3_protocol        (0xb29)
#define image_4_tcam_cmd_mc        (0xb11)
#define image_4_tcam_cmd_mc_l3        (0xb17)
#define image_4_tcam_cmd_network_layer        (0xb35)
#define image_4_tcam_cmd_outer_pbit        (0xaff)
#define image_4_tcam_cmd_outer_tpid        (0xae1)
#define image_4_tcam_cmd_outer_vid        (0xaf3)
#define image_4_tcam_cmd_src_ip        (0xb54)
#define image_4_tcam_cmd_src_ipv6_masked        (0xb69)
#define image_4_tcam_cmd_src_mac        (0xba4)
#define image_4_tcam_cmd_src_port        (0xb98)
#define image_4_tcam_cmd_tos        (0xb2f)
#define image_4_tcam_cmd_vlan_num        (0xb1d)

#endif


#endif

/* IMAGE 5 LABELS */
#ifndef IMAGE_5_CODE_ADDRESSES
#define IMAGE_5_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_5_budget_allocator_1st_wakeup_request        (0x48e0)
#define image_5_debug_routine        (0x3dd8)
#define image_5_debug_routine_handler        (0x4)
#define image_5_flush_task_1st_wakeup_request        (0x47b8)
#define image_5_gpe_cmd_copy_bits_16        (0x3478)
#define image_5_gpe_cmd_replace_16        (0x3438)
#define image_5_gpe_cmd_replace_32        (0x3440)
#define image_5_gpe_cmd_replace_bits_16        (0x3458)
#define image_5_gpe_cmd_skip_if        (0x34a8)
#define image_5_gpe_vlan_action_cmd_drop        (0x34c8)
#define image_5_gpe_vlan_action_cmd_dscp        (0x34cc)
#define image_5_gpe_vlan_action_cmd_mac_hdr_copy        (0x3520)
#define image_5_initialization_task        (0x8)
#define image_5_processing_wakeup_request        (0xb20)
#define image_5_scheduling_service_queues_call_scheduler_block        (0x430c)
#define image_5_service_queues_tx_task_wakeup_request        (0x430c)
#define image_5_service_queues_update_fifo_read_1st_wakeup_request        (0x4044)
#define image_5_start_task_budget_allocator_1st_wakeup_request        (0x48e0)
#define image_5_start_task_debug_routine        (0x3dd8)
#define image_5_start_task_flush_task_1st_wakeup_request        (0x47b8)
#define image_5_start_task_initialization_task        (0x8)
#define image_5_start_task_processing_wakeup_request        (0xb20)
#define image_5_start_task_service_queues_tx_task_wakeup_request        (0x430c)
#define image_5_start_task_service_queues_update_fifo_read_1st_wakeup_request        (0x4044)
#define image_5_tcam_cmd_bc        (0x2b20)
#define image_5_tcam_cmd_dst_ip        (0x2ccc)
#define image_5_tcam_cmd_dst_ipv6_masked        (0x2d20)
#define image_5_tcam_cmd_dst_mac        (0x2db0)
#define image_5_tcam_cmd_dst_port        (0x2d6c)
#define image_5_tcam_cmd_ethertype        (0x2aa8)
#define image_5_tcam_cmd_gem_flow        (0x2c00)
#define image_5_tcam_cmd_generic_l2        (0x2ddc)
#define image_5_tcam_cmd_generic_l3        (0x2e10)
#define image_5_tcam_cmd_generic_l4        (0x2e44)
#define image_5_tcam_cmd_ic_submit        (0x2a2c)
#define image_5_tcam_cmd_ingress_port        (0x2be8)
#define image_5_tcam_cmd_inner_pbit        (0x2b08)
#define image_5_tcam_cmd_inner_tpid        (0x2a90)
#define image_5_tcam_cmd_inner_vid        (0x2ad8)
#define image_5_tcam_cmd_ip_protocol        (0x2b80)
#define image_5_tcam_cmd_ipv6_label        (0x2c18)
#define image_5_tcam_cmd_l3_protocol        (0x2b98)
#define image_5_tcam_cmd_mc        (0x2b38)
#define image_5_tcam_cmd_mc_l3        (0x2b50)
#define image_5_tcam_cmd_network_layer        (0x2bc8)
#define image_5_tcam_cmd_outer_pbit        (0x2af0)
#define image_5_tcam_cmd_outer_tpid        (0x2a78)
#define image_5_tcam_cmd_outer_vid        (0x2ac0)
#define image_5_tcam_cmd_src_ip        (0x2c44)
#define image_5_tcam_cmd_src_ipv6_masked        (0x2c98)
#define image_5_tcam_cmd_src_mac        (0x2d84)
#define image_5_tcam_cmd_src_port        (0x2d54)
#define image_5_tcam_cmd_tos        (0x2bb0)
#define image_5_tcam_cmd_vlan_num        (0x2b68)

#else

#define image_5_budget_allocator_1st_wakeup_request        (0x1238)
#define image_5_debug_routine        (0xf76)
#define image_5_debug_routine_handler        (0x1)
#define image_5_flush_task_1st_wakeup_request        (0x11ee)
#define image_5_gpe_cmd_copy_bits_16        (0xd1e)
#define image_5_gpe_cmd_replace_16        (0xd0e)
#define image_5_gpe_cmd_replace_32        (0xd10)
#define image_5_gpe_cmd_replace_bits_16        (0xd16)
#define image_5_gpe_cmd_skip_if        (0xd2a)
#define image_5_gpe_vlan_action_cmd_drop        (0xd32)
#define image_5_gpe_vlan_action_cmd_dscp        (0xd33)
#define image_5_gpe_vlan_action_cmd_mac_hdr_copy        (0xd48)
#define image_5_initialization_task        (0x2)
#define image_5_processing_wakeup_request        (0x2c8)
#define image_5_scheduling_service_queues_call_scheduler_block        (0x10c3)
#define image_5_service_queues_tx_task_wakeup_request        (0x10c3)
#define image_5_service_queues_update_fifo_read_1st_wakeup_request        (0x1011)
#define image_5_start_task_budget_allocator_1st_wakeup_request        (0x1238)
#define image_5_start_task_debug_routine        (0xf76)
#define image_5_start_task_flush_task_1st_wakeup_request        (0x11ee)
#define image_5_start_task_initialization_task        (0x2)
#define image_5_start_task_processing_wakeup_request        (0x2c8)
#define image_5_start_task_service_queues_tx_task_wakeup_request        (0x10c3)
#define image_5_start_task_service_queues_update_fifo_read_1st_wakeup_request        (0x1011)
#define image_5_tcam_cmd_bc        (0xac8)
#define image_5_tcam_cmd_dst_ip        (0xb33)
#define image_5_tcam_cmd_dst_ipv6_masked        (0xb48)
#define image_5_tcam_cmd_dst_mac        (0xb6c)
#define image_5_tcam_cmd_dst_port        (0xb5b)
#define image_5_tcam_cmd_ethertype        (0xaaa)
#define image_5_tcam_cmd_gem_flow        (0xb00)
#define image_5_tcam_cmd_generic_l2        (0xb77)
#define image_5_tcam_cmd_generic_l3        (0xb84)
#define image_5_tcam_cmd_generic_l4        (0xb91)
#define image_5_tcam_cmd_ic_submit        (0xa8b)
#define image_5_tcam_cmd_ingress_port        (0xafa)
#define image_5_tcam_cmd_inner_pbit        (0xac2)
#define image_5_tcam_cmd_inner_tpid        (0xaa4)
#define image_5_tcam_cmd_inner_vid        (0xab6)
#define image_5_tcam_cmd_ip_protocol        (0xae0)
#define image_5_tcam_cmd_ipv6_label        (0xb06)
#define image_5_tcam_cmd_l3_protocol        (0xae6)
#define image_5_tcam_cmd_mc        (0xace)
#define image_5_tcam_cmd_mc_l3        (0xad4)
#define image_5_tcam_cmd_network_layer        (0xaf2)
#define image_5_tcam_cmd_outer_pbit        (0xabc)
#define image_5_tcam_cmd_outer_tpid        (0xa9e)
#define image_5_tcam_cmd_outer_vid        (0xab0)
#define image_5_tcam_cmd_src_ip        (0xb11)
#define image_5_tcam_cmd_src_ipv6_masked        (0xb26)
#define image_5_tcam_cmd_src_mac        (0xb61)
#define image_5_tcam_cmd_src_port        (0xb55)
#define image_5_tcam_cmd_tos        (0xaec)
#define image_5_tcam_cmd_vlan_num        (0xada)

#endif


#endif

/* IMAGE 6 LABELS */
#ifndef IMAGE_6_CODE_ADDRESSES
#define IMAGE_6_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_6_debug_routine        (0x3dd8)
#define image_6_debug_routine_handler        (0x4)
#define image_6_gpe_cmd_copy_bits_16        (0x3478)
#define image_6_gpe_cmd_replace_16        (0x3438)
#define image_6_gpe_cmd_replace_32        (0x3440)
#define image_6_gpe_cmd_replace_bits_16        (0x3458)
#define image_6_gpe_cmd_skip_if        (0x34a8)
#define image_6_gpe_vlan_action_cmd_drop        (0x34c8)
#define image_6_gpe_vlan_action_cmd_dscp        (0x34cc)
#define image_6_gpe_vlan_action_cmd_mac_hdr_copy        (0x3520)
#define image_6_initialization_task        (0x8)
#define image_6_processing_wakeup_request        (0xb20)
#define image_6_start_task_debug_routine        (0x3dd8)
#define image_6_start_task_initialization_task        (0x8)
#define image_6_start_task_processing_wakeup_request        (0xb20)
#define image_6_tcam_cmd_bc        (0x2b20)
#define image_6_tcam_cmd_dst_ip        (0x2ccc)
#define image_6_tcam_cmd_dst_ipv6_masked        (0x2d20)
#define image_6_tcam_cmd_dst_mac        (0x2db0)
#define image_6_tcam_cmd_dst_port        (0x2d6c)
#define image_6_tcam_cmd_ethertype        (0x2aa8)
#define image_6_tcam_cmd_gem_flow        (0x2c00)
#define image_6_tcam_cmd_generic_l2        (0x2ddc)
#define image_6_tcam_cmd_generic_l3        (0x2e10)
#define image_6_tcam_cmd_generic_l4        (0x2e44)
#define image_6_tcam_cmd_ic_submit        (0x2a2c)
#define image_6_tcam_cmd_ingress_port        (0x2be8)
#define image_6_tcam_cmd_inner_pbit        (0x2b08)
#define image_6_tcam_cmd_inner_tpid        (0x2a90)
#define image_6_tcam_cmd_inner_vid        (0x2ad8)
#define image_6_tcam_cmd_ip_protocol        (0x2b80)
#define image_6_tcam_cmd_ipv6_label        (0x2c18)
#define image_6_tcam_cmd_l3_protocol        (0x2b98)
#define image_6_tcam_cmd_mc        (0x2b38)
#define image_6_tcam_cmd_mc_l3        (0x2b50)
#define image_6_tcam_cmd_network_layer        (0x2bc8)
#define image_6_tcam_cmd_outer_pbit        (0x2af0)
#define image_6_tcam_cmd_outer_tpid        (0x2a78)
#define image_6_tcam_cmd_outer_vid        (0x2ac0)
#define image_6_tcam_cmd_src_ip        (0x2c44)
#define image_6_tcam_cmd_src_ipv6_masked        (0x2c98)
#define image_6_tcam_cmd_src_mac        (0x2d84)
#define image_6_tcam_cmd_src_port        (0x2d54)
#define image_6_tcam_cmd_tos        (0x2bb0)
#define image_6_tcam_cmd_vlan_num        (0x2b68)

#else

#define image_6_debug_routine        (0xf76)
#define image_6_debug_routine_handler        (0x1)
#define image_6_gpe_cmd_copy_bits_16        (0xd1e)
#define image_6_gpe_cmd_replace_16        (0xd0e)
#define image_6_gpe_cmd_replace_32        (0xd10)
#define image_6_gpe_cmd_replace_bits_16        (0xd16)
#define image_6_gpe_cmd_skip_if        (0xd2a)
#define image_6_gpe_vlan_action_cmd_drop        (0xd32)
#define image_6_gpe_vlan_action_cmd_dscp        (0xd33)
#define image_6_gpe_vlan_action_cmd_mac_hdr_copy        (0xd48)
#define image_6_initialization_task        (0x2)
#define image_6_processing_wakeup_request        (0x2c8)
#define image_6_start_task_debug_routine        (0xf76)
#define image_6_start_task_initialization_task        (0x2)
#define image_6_start_task_processing_wakeup_request        (0x2c8)
#define image_6_tcam_cmd_bc        (0xac8)
#define image_6_tcam_cmd_dst_ip        (0xb33)
#define image_6_tcam_cmd_dst_ipv6_masked        (0xb48)
#define image_6_tcam_cmd_dst_mac        (0xb6c)
#define image_6_tcam_cmd_dst_port        (0xb5b)
#define image_6_tcam_cmd_ethertype        (0xaaa)
#define image_6_tcam_cmd_gem_flow        (0xb00)
#define image_6_tcam_cmd_generic_l2        (0xb77)
#define image_6_tcam_cmd_generic_l3        (0xb84)
#define image_6_tcam_cmd_generic_l4        (0xb91)
#define image_6_tcam_cmd_ic_submit        (0xa8b)
#define image_6_tcam_cmd_ingress_port        (0xafa)
#define image_6_tcam_cmd_inner_pbit        (0xac2)
#define image_6_tcam_cmd_inner_tpid        (0xaa4)
#define image_6_tcam_cmd_inner_vid        (0xab6)
#define image_6_tcam_cmd_ip_protocol        (0xae0)
#define image_6_tcam_cmd_ipv6_label        (0xb06)
#define image_6_tcam_cmd_l3_protocol        (0xae6)
#define image_6_tcam_cmd_mc        (0xace)
#define image_6_tcam_cmd_mc_l3        (0xad4)
#define image_6_tcam_cmd_network_layer        (0xaf2)
#define image_6_tcam_cmd_outer_pbit        (0xabc)
#define image_6_tcam_cmd_outer_tpid        (0xa9e)
#define image_6_tcam_cmd_outer_vid        (0xab0)
#define image_6_tcam_cmd_src_ip        (0xb11)
#define image_6_tcam_cmd_src_ipv6_masked        (0xb26)
#define image_6_tcam_cmd_src_mac        (0xb61)
#define image_6_tcam_cmd_src_port        (0xb55)
#define image_6_tcam_cmd_tos        (0xaec)
#define image_6_tcam_cmd_vlan_num        (0xada)

#endif


#endif

/* IMAGE 7 LABELS */
#ifndef IMAGE_7_CODE_ADDRESSES
#define IMAGE_7_CODE_ADDRESSES

#ifndef PC_ADDRESS_INST_IND

#define image_7_debug_routine        (0x3dd8)
#define image_7_debug_routine_handler        (0x4)
#define image_7_gpe_cmd_copy_bits_16        (0x3478)
#define image_7_gpe_cmd_replace_16        (0x3438)
#define image_7_gpe_cmd_replace_32        (0x3440)
#define image_7_gpe_cmd_replace_bits_16        (0x3458)
#define image_7_gpe_cmd_skip_if        (0x34a8)
#define image_7_gpe_vlan_action_cmd_drop        (0x34c8)
#define image_7_gpe_vlan_action_cmd_dscp        (0x34cc)
#define image_7_gpe_vlan_action_cmd_mac_hdr_copy        (0x3520)
#define image_7_initialization_task        (0x8)
#define image_7_processing_wakeup_request        (0xb20)
#define image_7_start_task_debug_routine        (0x3dd8)
#define image_7_start_task_initialization_task        (0x8)
#define image_7_start_task_processing_wakeup_request        (0xb20)
#define image_7_tcam_cmd_bc        (0x2b20)
#define image_7_tcam_cmd_dst_ip        (0x2ccc)
#define image_7_tcam_cmd_dst_ipv6_masked        (0x2d20)
#define image_7_tcam_cmd_dst_mac        (0x2db0)
#define image_7_tcam_cmd_dst_port        (0x2d6c)
#define image_7_tcam_cmd_ethertype        (0x2aa8)
#define image_7_tcam_cmd_gem_flow        (0x2c00)
#define image_7_tcam_cmd_generic_l2        (0x2ddc)
#define image_7_tcam_cmd_generic_l3        (0x2e10)
#define image_7_tcam_cmd_generic_l4        (0x2e44)
#define image_7_tcam_cmd_ic_submit        (0x2a2c)
#define image_7_tcam_cmd_ingress_port        (0x2be8)
#define image_7_tcam_cmd_inner_pbit        (0x2b08)
#define image_7_tcam_cmd_inner_tpid        (0x2a90)
#define image_7_tcam_cmd_inner_vid        (0x2ad8)
#define image_7_tcam_cmd_ip_protocol        (0x2b80)
#define image_7_tcam_cmd_ipv6_label        (0x2c18)
#define image_7_tcam_cmd_l3_protocol        (0x2b98)
#define image_7_tcam_cmd_mc        (0x2b38)
#define image_7_tcam_cmd_mc_l3        (0x2b50)
#define image_7_tcam_cmd_network_layer        (0x2bc8)
#define image_7_tcam_cmd_outer_pbit        (0x2af0)
#define image_7_tcam_cmd_outer_tpid        (0x2a78)
#define image_7_tcam_cmd_outer_vid        (0x2ac0)
#define image_7_tcam_cmd_src_ip        (0x2c44)
#define image_7_tcam_cmd_src_ipv6_masked        (0x2c98)
#define image_7_tcam_cmd_src_mac        (0x2d84)
#define image_7_tcam_cmd_src_port        (0x2d54)
#define image_7_tcam_cmd_tos        (0x2bb0)
#define image_7_tcam_cmd_vlan_num        (0x2b68)

#else

#define image_7_debug_routine        (0xf76)
#define image_7_debug_routine_handler        (0x1)
#define image_7_gpe_cmd_copy_bits_16        (0xd1e)
#define image_7_gpe_cmd_replace_16        (0xd0e)
#define image_7_gpe_cmd_replace_32        (0xd10)
#define image_7_gpe_cmd_replace_bits_16        (0xd16)
#define image_7_gpe_cmd_skip_if        (0xd2a)
#define image_7_gpe_vlan_action_cmd_drop        (0xd32)
#define image_7_gpe_vlan_action_cmd_dscp        (0xd33)
#define image_7_gpe_vlan_action_cmd_mac_hdr_copy        (0xd48)
#define image_7_initialization_task        (0x2)
#define image_7_processing_wakeup_request        (0x2c8)
#define image_7_start_task_debug_routine        (0xf76)
#define image_7_start_task_initialization_task        (0x2)
#define image_7_start_task_processing_wakeup_request        (0x2c8)
#define image_7_tcam_cmd_bc        (0xac8)
#define image_7_tcam_cmd_dst_ip        (0xb33)
#define image_7_tcam_cmd_dst_ipv6_masked        (0xb48)
#define image_7_tcam_cmd_dst_mac        (0xb6c)
#define image_7_tcam_cmd_dst_port        (0xb5b)
#define image_7_tcam_cmd_ethertype        (0xaaa)
#define image_7_tcam_cmd_gem_flow        (0xb00)
#define image_7_tcam_cmd_generic_l2        (0xb77)
#define image_7_tcam_cmd_generic_l3        (0xb84)
#define image_7_tcam_cmd_generic_l4        (0xb91)
#define image_7_tcam_cmd_ic_submit        (0xa8b)
#define image_7_tcam_cmd_ingress_port        (0xafa)
#define image_7_tcam_cmd_inner_pbit        (0xac2)
#define image_7_tcam_cmd_inner_tpid        (0xaa4)
#define image_7_tcam_cmd_inner_vid        (0xab6)
#define image_7_tcam_cmd_ip_protocol        (0xae0)
#define image_7_tcam_cmd_ipv6_label        (0xb06)
#define image_7_tcam_cmd_l3_protocol        (0xae6)
#define image_7_tcam_cmd_mc        (0xace)
#define image_7_tcam_cmd_mc_l3        (0xad4)
#define image_7_tcam_cmd_network_layer        (0xaf2)
#define image_7_tcam_cmd_outer_pbit        (0xabc)
#define image_7_tcam_cmd_outer_tpid        (0xa9e)
#define image_7_tcam_cmd_outer_vid        (0xab0)
#define image_7_tcam_cmd_src_ip        (0xb11)
#define image_7_tcam_cmd_src_ipv6_masked        (0xb26)
#define image_7_tcam_cmd_src_mac        (0xb61)
#define image_7_tcam_cmd_src_port        (0xb55)
#define image_7_tcam_cmd_tos        (0xaec)
#define image_7_tcam_cmd_vlan_num        (0xada)

#endif


#endif

/* COMMON LABELS */
#ifndef COMMON_CODE_ADDRESSES
#define COMMON_CODE_ADDRESSES

#define INVALID_LABEL_ADDRESS 0xFFFFFF

#ifndef PC_ADDRESS_INST_IND

#define TCAM_CMD_BC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b20, 0x2c2c, INVALID_LABEL_ADDRESS, 0x2c2c, 0x2b20, 0x2b20, 0x2b20}
#define TCAM_CMD_DST_IP_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2ccc, 0x2dd8, INVALID_LABEL_ADDRESS, 0x2dd8, 0x2ccc, 0x2ccc, 0x2ccc}
#define TCAM_CMD_DST_IPV6_MASKED_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2d20, 0x2e2c, INVALID_LABEL_ADDRESS, 0x2e2c, 0x2d20, 0x2d20, 0x2d20}
#define TCAM_CMD_DST_MAC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2db0, 0x2ebc, INVALID_LABEL_ADDRESS, 0x2ebc, 0x2db0, 0x2db0, 0x2db0}
#define TCAM_CMD_DST_PORT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2d6c, 0x2e78, INVALID_LABEL_ADDRESS, 0x2e78, 0x2d6c, 0x2d6c, 0x2d6c}
#define TCAM_CMD_ETHERTYPE_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2aa8, 0x2bb4, INVALID_LABEL_ADDRESS, 0x2bb4, 0x2aa8, 0x2aa8, 0x2aa8}
#define TCAM_CMD_GEM_FLOW_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2c00, 0x2d0c, INVALID_LABEL_ADDRESS, 0x2d0c, 0x2c00, 0x2c00, 0x2c00}
#define TCAM_CMD_GENERIC_L2_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2ddc, 0x2ee8, INVALID_LABEL_ADDRESS, 0x2ee8, 0x2ddc, 0x2ddc, 0x2ddc}
#define TCAM_CMD_GENERIC_L3_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2e10, 0x2f1c, INVALID_LABEL_ADDRESS, 0x2f1c, 0x2e10, 0x2e10, 0x2e10}
#define TCAM_CMD_GENERIC_L4_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2e44, 0x2f50, INVALID_LABEL_ADDRESS, 0x2f50, 0x2e44, 0x2e44, 0x2e44}
#define TCAM_CMD_IC_SUBMIT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2a2c, 0x2b38, INVALID_LABEL_ADDRESS, 0x2b38, 0x2a2c, 0x2a2c, 0x2a2c}
#define TCAM_CMD_INGRESS_PORT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2be8, 0x2cf4, INVALID_LABEL_ADDRESS, 0x2cf4, 0x2be8, 0x2be8, 0x2be8}
#define TCAM_CMD_INNER_PBIT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b08, 0x2c14, INVALID_LABEL_ADDRESS, 0x2c14, 0x2b08, 0x2b08, 0x2b08}
#define TCAM_CMD_INNER_TPID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2a90, 0x2b9c, INVALID_LABEL_ADDRESS, 0x2b9c, 0x2a90, 0x2a90, 0x2a90}
#define TCAM_CMD_INNER_VID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2ad8, 0x2be4, INVALID_LABEL_ADDRESS, 0x2be4, 0x2ad8, 0x2ad8, 0x2ad8}
#define TCAM_CMD_IP_PROTOCOL_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b80, 0x2c8c, INVALID_LABEL_ADDRESS, 0x2c8c, 0x2b80, 0x2b80, 0x2b80}
#define TCAM_CMD_IPV6_LABEL_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2c18, 0x2d24, INVALID_LABEL_ADDRESS, 0x2d24, 0x2c18, 0x2c18, 0x2c18}
#define TCAM_CMD_L3_PROTOCOL_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b98, 0x2ca4, INVALID_LABEL_ADDRESS, 0x2ca4, 0x2b98, 0x2b98, 0x2b98}
#define TCAM_CMD_MC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b38, 0x2c44, INVALID_LABEL_ADDRESS, 0x2c44, 0x2b38, 0x2b38, 0x2b38}
#define TCAM_CMD_MC_L3_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b50, 0x2c5c, INVALID_LABEL_ADDRESS, 0x2c5c, 0x2b50, 0x2b50, 0x2b50}
#define TCAM_CMD_NETWORK_LAYER_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2bc8, 0x2cd4, INVALID_LABEL_ADDRESS, 0x2cd4, 0x2bc8, 0x2bc8, 0x2bc8}
#define TCAM_CMD_OUTER_PBIT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2af0, 0x2bfc, INVALID_LABEL_ADDRESS, 0x2bfc, 0x2af0, 0x2af0, 0x2af0}
#define TCAM_CMD_OUTER_TPID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2a78, 0x2b84, INVALID_LABEL_ADDRESS, 0x2b84, 0x2a78, 0x2a78, 0x2a78}
#define TCAM_CMD_OUTER_VID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2ac0, 0x2bcc, INVALID_LABEL_ADDRESS, 0x2bcc, 0x2ac0, 0x2ac0, 0x2ac0}
#define TCAM_CMD_SRC_IP_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2c44, 0x2d50, INVALID_LABEL_ADDRESS, 0x2d50, 0x2c44, 0x2c44, 0x2c44}
#define TCAM_CMD_SRC_IPV6_MASKED_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2c98, 0x2da4, INVALID_LABEL_ADDRESS, 0x2da4, 0x2c98, 0x2c98, 0x2c98}
#define TCAM_CMD_SRC_MAC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2d84, 0x2e90, INVALID_LABEL_ADDRESS, 0x2e90, 0x2d84, 0x2d84, 0x2d84}
#define TCAM_CMD_SRC_PORT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2d54, 0x2e60, INVALID_LABEL_ADDRESS, 0x2e60, 0x2d54, 0x2d54, 0x2d54}
#define TCAM_CMD_TOS_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2bb0, 0x2cbc, INVALID_LABEL_ADDRESS, 0x2cbc, 0x2bb0, 0x2bb0, 0x2bb0}
#define TCAM_CMD_VLAN_NUM_ADDR_ARR {INVALID_LABEL_ADDRESS, 0x2b68, 0x2c74, INVALID_LABEL_ADDRESS, 0x2c74, 0x2b68, 0x2b68, 0x2b68}

#else

#define TCAM_CMD_BC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xac8, 0xb0b, INVALID_LABEL_ADDRESS, 0xb0b, 0xac8, 0xac8, 0xac8}
#define TCAM_CMD_DST_IP_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb33, 0xb76, INVALID_LABEL_ADDRESS, 0xb76, 0xb33, 0xb33, 0xb33}
#define TCAM_CMD_DST_IPV6_MASKED_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb48, 0xb8b, INVALID_LABEL_ADDRESS, 0xb8b, 0xb48, 0xb48, 0xb48}
#define TCAM_CMD_DST_MAC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb6c, 0xbaf, INVALID_LABEL_ADDRESS, 0xbaf, 0xb6c, 0xb6c, 0xb6c}
#define TCAM_CMD_DST_PORT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb5b, 0xb9e, INVALID_LABEL_ADDRESS, 0xb9e, 0xb5b, 0xb5b, 0xb5b}
#define TCAM_CMD_ETHERTYPE_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xaaa, 0xaed, INVALID_LABEL_ADDRESS, 0xaed, 0xaaa, 0xaaa, 0xaaa}
#define TCAM_CMD_GEM_FLOW_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb00, 0xb43, INVALID_LABEL_ADDRESS, 0xb43, 0xb00, 0xb00, 0xb00}
#define TCAM_CMD_GENERIC_L2_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb77, 0xbba, INVALID_LABEL_ADDRESS, 0xbba, 0xb77, 0xb77, 0xb77}
#define TCAM_CMD_GENERIC_L3_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb84, 0xbc7, INVALID_LABEL_ADDRESS, 0xbc7, 0xb84, 0xb84, 0xb84}
#define TCAM_CMD_GENERIC_L4_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb91, 0xbd4, INVALID_LABEL_ADDRESS, 0xbd4, 0xb91, 0xb91, 0xb91}
#define TCAM_CMD_IC_SUBMIT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xa8b, 0xace, INVALID_LABEL_ADDRESS, 0xace, 0xa8b, 0xa8b, 0xa8b}
#define TCAM_CMD_INGRESS_PORT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xafa, 0xb3d, INVALID_LABEL_ADDRESS, 0xb3d, 0xafa, 0xafa, 0xafa}
#define TCAM_CMD_INNER_PBIT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xac2, 0xb05, INVALID_LABEL_ADDRESS, 0xb05, 0xac2, 0xac2, 0xac2}
#define TCAM_CMD_INNER_TPID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xaa4, 0xae7, INVALID_LABEL_ADDRESS, 0xae7, 0xaa4, 0xaa4, 0xaa4}
#define TCAM_CMD_INNER_VID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xab6, 0xaf9, INVALID_LABEL_ADDRESS, 0xaf9, 0xab6, 0xab6, 0xab6}
#define TCAM_CMD_IP_PROTOCOL_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xae0, 0xb23, INVALID_LABEL_ADDRESS, 0xb23, 0xae0, 0xae0, 0xae0}
#define TCAM_CMD_IPV6_LABEL_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb06, 0xb49, INVALID_LABEL_ADDRESS, 0xb49, 0xb06, 0xb06, 0xb06}
#define TCAM_CMD_L3_PROTOCOL_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xae6, 0xb29, INVALID_LABEL_ADDRESS, 0xb29, 0xae6, 0xae6, 0xae6}
#define TCAM_CMD_MC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xace, 0xb11, INVALID_LABEL_ADDRESS, 0xb11, 0xace, 0xace, 0xace}
#define TCAM_CMD_MC_L3_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xad4, 0xb17, INVALID_LABEL_ADDRESS, 0xb17, 0xad4, 0xad4, 0xad4}
#define TCAM_CMD_NETWORK_LAYER_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xaf2, 0xb35, INVALID_LABEL_ADDRESS, 0xb35, 0xaf2, 0xaf2, 0xaf2}
#define TCAM_CMD_OUTER_PBIT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xabc, 0xaff, INVALID_LABEL_ADDRESS, 0xaff, 0xabc, 0xabc, 0xabc}
#define TCAM_CMD_OUTER_TPID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xa9e, 0xae1, INVALID_LABEL_ADDRESS, 0xae1, 0xa9e, 0xa9e, 0xa9e}
#define TCAM_CMD_OUTER_VID_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xab0, 0xaf3, INVALID_LABEL_ADDRESS, 0xaf3, 0xab0, 0xab0, 0xab0}
#define TCAM_CMD_SRC_IP_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb11, 0xb54, INVALID_LABEL_ADDRESS, 0xb54, 0xb11, 0xb11, 0xb11}
#define TCAM_CMD_SRC_IPV6_MASKED_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb26, 0xb69, INVALID_LABEL_ADDRESS, 0xb69, 0xb26, 0xb26, 0xb26}
#define TCAM_CMD_SRC_MAC_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb61, 0xba4, INVALID_LABEL_ADDRESS, 0xba4, 0xb61, 0xb61, 0xb61}
#define TCAM_CMD_SRC_PORT_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xb55, 0xb98, INVALID_LABEL_ADDRESS, 0xb98, 0xb55, 0xb55, 0xb55}
#define TCAM_CMD_TOS_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xaec, 0xb2f, INVALID_LABEL_ADDRESS, 0xb2f, 0xaec, 0xaec, 0xaec}
#define TCAM_CMD_VLAN_NUM_ADDR_ARR {INVALID_LABEL_ADDRESS, 0xada, 0xb1d, INVALID_LABEL_ADDRESS, 0xb1d, 0xada, 0xada, 0xada}

#endif


#endif

