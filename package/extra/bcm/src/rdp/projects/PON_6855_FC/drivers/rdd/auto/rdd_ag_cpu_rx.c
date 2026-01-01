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


#include "rdd.h"

#include "rdd_ag_cpu_rx.h"

int rdd_ag_cpu_rx_cpu_rxq_buf_global_cfg_entry_get(rdd_cpu_rxq_buf_global_cfg_entry_t *cpu_rxq_buf_global_cfg_entry)
{
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_SIZE_READ_G(cpu_rxq_buf_global_cfg_entry->shared_info_size, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0);
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_OFFSET_READ_G(cpu_rxq_buf_global_cfg_entry->shared_info_offset, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_rxq_buf_global_cfg_entry_set(rdd_cpu_rxq_buf_global_cfg_entry_t *cpu_rxq_buf_global_cfg_entry)
{
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_SIZE_WRITE_G(cpu_rxq_buf_global_cfg_entry->shared_info_size, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0);
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_OFFSET_WRITE_G(cpu_rxq_buf_global_cfg_entry->shared_info_offset, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_rxq_buf_global_cfg_entry_get_core(rdd_cpu_rxq_buf_global_cfg_entry_t *cpu_rxq_buf_global_cfg_entry, int core_id)
{
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_SIZE_READ_CORE(cpu_rxq_buf_global_cfg_entry->shared_info_size, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0, core_id);
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_OFFSET_READ_CORE(cpu_rxq_buf_global_cfg_entry->shared_info_offset, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_rxq_buf_global_cfg_entry_set_core(rdd_cpu_rxq_buf_global_cfg_entry_t *cpu_rxq_buf_global_cfg_entry, int core_id)
{
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_SIZE_WRITE_CORE(cpu_rxq_buf_global_cfg_entry->shared_info_size, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0, core_id);
    RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY_SHARED_INFO_OFFSET_WRITE_CORE(cpu_rxq_buf_global_cfg_entry->shared_info_offset, RDD_CPU_RXQ_BUF_GLOBAL_CFG_TABLE_ADDRESS_ARR, 0, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_reason_to_tc_set(uint32_t _entry, uint8_t bits)
{
    if(_entry >= RDD_CPU_REASON_TO_TC_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTE_1_BITS_WRITE_G(bits, RDD_CPU_REASON_TO_TC_ADDRESS_ARR, _entry);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_reason_to_tc_set_core(uint32_t _entry, uint8_t bits, int core_id)
{
    if(_entry >= RDD_CPU_REASON_TO_TC_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTE_1_BITS_WRITE_CORE(bits, RDD_CPU_REASON_TO_TC_ADDRESS_ARR, _entry, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_reason_to_tc_get(uint32_t _entry, uint8_t *bits)
{
    if(_entry >= RDD_CPU_REASON_TO_TC_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTE_1_BITS_READ_G(*bits, RDD_CPU_REASON_TO_TC_ADDRESS_ARR, _entry);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_cpu_reason_to_tc_get_core(uint32_t _entry, uint8_t *bits, int core_id)
{
    if(_entry >= RDD_CPU_REASON_TO_TC_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTE_1_BITS_READ_CORE(*bits, RDD_CPU_REASON_TO_TC_ADDRESS_ARR, _entry, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv4_host_address_table_set(uint32_t _entry, uint32_t bits)
{
    if(_entry >= RDD_IPV4_HOST_ADDRESS_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_WRITE_G(bits, RDD_IPV4_HOST_ADDRESS_TABLE_ADDRESS_ARR, _entry);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv4_host_address_table_set_core(uint32_t _entry, uint32_t bits, int core_id)
{
    if(_entry >= RDD_IPV4_HOST_ADDRESS_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_WRITE_CORE(bits, RDD_IPV4_HOST_ADDRESS_TABLE_ADDRESS_ARR, _entry, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv4_host_address_table_get(uint32_t _entry, uint32_t *bits)
{
    if(_entry >= RDD_IPV4_HOST_ADDRESS_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_READ_G(*bits, RDD_IPV4_HOST_ADDRESS_TABLE_ADDRESS_ARR, _entry);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv4_host_address_table_get_core(uint32_t _entry, uint32_t *bits, int core_id)
{
    if(_entry >= RDD_IPV4_HOST_ADDRESS_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_READ_CORE(*bits, RDD_IPV4_HOST_ADDRESS_TABLE_ADDRESS_ARR, _entry, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv6_host_address_crc_table_set(uint32_t _entry, uint32_t bits)
{
    if(_entry >= RDD_IPV6_HOST_ADDRESS_CRC_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_WRITE_G(bits, RDD_IPV6_HOST_ADDRESS_CRC_TABLE_ADDRESS_ARR, _entry);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv6_host_address_crc_table_set_core(uint32_t _entry, uint32_t bits, int core_id)
{
    if(_entry >= RDD_IPV6_HOST_ADDRESS_CRC_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_WRITE_CORE(bits, RDD_IPV6_HOST_ADDRESS_CRC_TABLE_ADDRESS_ARR, _entry, core_id);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv6_host_address_crc_table_get(uint32_t _entry, uint32_t *bits)
{
    if(_entry >= RDD_IPV6_HOST_ADDRESS_CRC_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_READ_G(*bits, RDD_IPV6_HOST_ADDRESS_CRC_TABLE_ADDRESS_ARR, _entry);

    return BDMF_ERR_OK;
}

int rdd_ag_cpu_rx_ipv6_host_address_crc_table_get_core(uint32_t _entry, uint32_t *bits, int core_id)
{
    if(_entry >= RDD_IPV6_HOST_ADDRESS_CRC_TABLE_SIZE)
          return BDMF_ERR_PARM;

    RDD_BYTES_4_BITS_READ_CORE(*bits, RDD_IPV6_HOST_ADDRESS_CRC_TABLE_ADDRESS_ARR, _entry, core_id);

    return BDMF_ERR_OK;
}

