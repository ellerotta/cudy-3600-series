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


#include "bdmf_shell.h"
#include "rdd_map_auto.h"
#include "rdd_runner_reg_dump.h"
#include "rdd_runner_reg_dump_addrs.h"
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_COALESCING_TABLE =
{
	16,
	{
		{ dump_RDD_CPU_INTERRUPT_COALESCING_ENTRY, 0x2b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REASON_TO_METER_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_METER_TABLE =
{
	8,
	{
		{ dump_RDD_CPU_RX_METER_ENTRY, 0x300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_SCRATCHPAD =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_COMPLETE_COMMON_RADIO_DATA =
{
	192,
	{
		{ dump_RDD_DHD_COMPLETE_COMMON_RADIO_ENTRY, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REASON_AND_VPORT_TO_METER_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xa40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_CACHE_TABLE =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_INT_INTERRUPT_SCRATCH =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xbd4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_ID_DDR_ADDR =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0xbd8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_RSV_TABLE =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0xbe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING0_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xe00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION =
{
	8,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0xf68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_DESCRIPTOR_TABLE =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0xf70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xf80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING_8_TASKS_PACKET_BUFFER =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING1_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2168 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_AUX_INT_SCRATCHPAD =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2170 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RXQ_SCRATCH_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING2_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_POST_RING_SIZE =
{
	2,
	{
		{ dump_RDD_DHD_RING_SIZE, 0x2368 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DOS_DROP_REASONS_CFG =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x236e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_GEN_PARAM =
{
	4,
	{
		{ dump_RDD_SPDTEST_GEN_CFG, 0x2370 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER =
{
	8,
	{
		{ dump_RDD_GENERAL_TIMER_ENTRY, 0x2380 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING3_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_COMPLETE_RING_SIZE =
{
	2,
	{
		{ dump_RDD_DHD_RING_SIZE, 0x2568 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_ACTION_VEC =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x256e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_COMPLETE_FLOW_RING_BUFFER =
{
	32,
	{
		{ dump_RDD_DHD_RX_COMPLETE_DESCRIPTOR, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPV4_HOST_ADDRESS_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING4_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_COMPLETE_RING_SIZE =
{
	2,
	{
		{ dump_RDD_DHD_RING_SIZE, 0x2768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MCAST_MAX_REPLICATION_FLAG_TABLE =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x276e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_POST_FLOW_RING_BUFFER =
{
	32,
	{
		{ dump_RDD_DHD_RX_POST_DESCRIPTOR, 0x2780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x27e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING5_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE =
{
	8,
	{
		{ dump_RDD_CPU_RING_INTERRUPT_COUNTER_ENTRY, 0x2968 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_DISPATCHER_CREDIT_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2970 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RXQ_BUF_GLOBAL_CFG_TABLE =
{
	4,
	{
		{ dump_RDD_CPU_RXQ_BUF_GLOBAL_CFG_ENTRY, 0x297c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2980 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x29d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MUTEX_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x29e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING6_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_INDEX_DDR_ADDR_TABLE =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2b68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FPM_THRESHOLDS =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_HOST_CNT_DISABLE_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x2b80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_DOORBELL_TX_COMPLETE_VALUE =
{
	4,
	{
		{ dump_RDD_DHD_DOORBELL, 0x2bd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_BAD_IPV4_HDR_CSUM_PACKETS =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2bdc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_DESCRIPTOR_TABLE =
{
	2,
	{
		{ dump_RDD_BUFMNG_DESCRIPTOR_ENTRY, 0x2be0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING7_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_GENERIC_FIELDS =
{
	2,
	{
		{ dump_RDD_TCAM_GENERIC, 0x2d68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_DOORBELL_RX_COMPLETE_VALUE =
{
	4,
	{
		{ dump_RDD_DHD_DOORBELL, 0x2d70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_SCRATCH =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2dc8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_DOORBELL_RX_POST_VALUE =
{
	4,
	{
		{ dump_RDD_DHD_DOORBELL, 0x2dd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TASK_IDX =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ddc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_STATUS_TABLE =
{
	32,
	{
		{ dump_RDD_BUFMNG_STATUS_ENTRY, 0x2de0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SRAM_PD_FIFO =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RING_INTERRUPT_COUNTER_TABLE =
{
	8,
	{
		{ dump_RDD_CPU_RING_INTERRUPT_COUNTER_ENTRY, 0x2f00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPV6_HOST_ADDRESS_CRC_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RING_DESCRIPTORS_TABLE =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_PD_FIFO_TABLE =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x3200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_COMPLETE_0_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x32c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_CONTEXT_TABLE =
{
	160,
	{
		{ dump_RDD_CSO_CONTEXT_ENTRY, 0x3300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_COMPLETE_1_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x3400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x34a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_COMPLETE_0_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x34c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x3500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_COMPLETE_1_STACK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TR471_SPDSVC_RX_PKT_ID_TABLE =
{
	12,
	{
		{ dump_RDD_TR471_SPDSVC_RX_PKT_ID, 0x35c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_INT_ID =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x35f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x35fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_NUM_OF_RX_FLOWS =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x35fe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x35ff },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_RECYCLE_RING_DESCRIPTOR_TABLE =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x3600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_WLAN_GEN_PARAMS_TABLE =
{
	10,
	{
		{ dump_RDD_SPDSVC_WLAN_GEN_PARAMS, 0x3630 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FORCE_DSCP =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x363a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x363b },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LLQ_SELECTOR_ECN_MASK =
{
	1,
	{
		{ dump_RDD_LLQ_SELECTOR_ECN, 0x363c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_CACHE_OFFSET =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x363d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x363e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_COMPLETE_FLOW_RING_BUFFER =
{
	16,
	{
		{ dump_RDD_DHD_TX_COMPLETE_DESCRIPTOR, 0x3640 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_DESCRIPTOR_TABLE =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x3670 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_RX_COMPLETE_DISPATCHER_CREDIT_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x36b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_COMPLETE_DISPATCHER_CREDIT_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x36c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_INTERRUPT_COALESCING_TABLE =
{
	16,
	{
		{ dump_RDD_CPU_INTERRUPT_COALESCING_ENTRY, 0x36f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_TABLE =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x3700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x3728 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_SCRATCH =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3730 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x3738 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x3740 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_LOCAL_SCRATCH =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x3768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG =
{
	7,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x3778 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x377f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_RD_IDX =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x37a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x37aa },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_DISABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x37ac },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x37ad },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_TABLE_CFG_TABLE =
{
	1,
	{
		{ dump_RDD_TCAM_TABLE_CFG, 0x37ae },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x37af },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x37b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE =
{
	2,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x37c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REASON_TO_TC =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x37e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TC_TO_CPU_RXQ =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3828 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT EXC_TC_TO_CPU_RXQ =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3868 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_WR_IDX =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x38a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x38aa },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x38ab },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x38ac },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x38b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RXQ_DATA_BUF_TYPE_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x38c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_HW_CFG =
{
	20,
	{
		{ dump_RDD_DHD_HW_CONFIGURATION, 0x38e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_KEY0_MASK =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x38f8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_TABLE =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x3900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG =
{
	28,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x3920 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_UPDATE_FIFO_TABLE =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x3940 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3960 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_VLAN_KEY_MASK =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3968 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_NEXT_PTR_TABLE =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3970 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x3978 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PD_FIFO_TABLE =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x3980 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x39a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_TOS_MASK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x39d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x39e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FPM_REPLY =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x3a40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_VPORT_TO_METER_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3a80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3ac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3b00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_CPU_OBJ =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3b40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_STREAM_TABLE_1 =
{
	376,
	{
		{ dump_RDD_TCPSPDTEST_STREAM, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x5e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING0_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_ID_DDR_ADDR_1 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_GEN_PARAM_1 =
{
	4,
	{
		{ dump_RDD_SPDTEST_GEN_CFG, 0x770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_SCRATCHPAD_1 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_1 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_1 =
{
	8,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0xaa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_ANALYZER_PARAMS_TABLE_1 =
{
	16,
	{
		{ dump_RDD_SPDSVC_ANALYZER_PARAMS, 0xab0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT COMMON_REPROCESSING_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_ANALYZER_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_TX_STREAM_TABLE_1 =
{
	136,
	{
		{ dump_RDD_PKTGEN_TX_STREAM_ENTRY, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_1 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xe20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_GEN_PARAMS_TABLE_1 =
{
	60,
	{
		{ dump_RDD_SPDSVC_GEN_PARAMS, 0xe40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_NO_SBPM_HDRS_CNTR_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xe7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_GEN_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xe80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_DOWNLOAD_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xf00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING_8_TASKS_PACKET_BUFFER_1 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_1 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING1_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_ABS_RECYCLE_COUNTERS_1 =
{
	8,
	{
		{ dump_RDD_TX_ABS_RECYCLE_COUNTERS_ENTRY, 0x2368 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_1 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x2370 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_1 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x2380 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_DESCRIPTOR_TABLE_1 =
{
	2,
	{
		{ dump_RDD_BUFMNG_DESCRIPTOR_ENTRY, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING2_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_CURR_SBPM_HDR_PTR_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2568 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_GEN_DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MUTEX_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x257c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_ANALYZER_PD_FIFO_TABLE_1 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TR471_SPDSVC_RX_PKT_ID_TABLE_1 =
{
	12,
	{
		{ dump_RDD_TR471_SPDSVC_RX_PKT_ID, 0x25c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_HOST_CNT_DISABLE_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING3_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_NUM_OF_AVAIL_SBPM_HDRS_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT COMMON_REPROCESSING_DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_BAD_IPV4_HDR_CSUM_PACKETS_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x277c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_SBPM_HDR_BNS_1 =
{
	2,
	{
		{ dump_RDD_PKTGEN_SBPM_HDR_BN, 0x2780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_TABLE_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x27c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_SBPM_END_PTR_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x27e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_GEN_DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x27f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_BAD_GET_NEXT_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x27fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING4_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2968 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_GENERIC_FIELDS_1 =
{
	2,
	{
		{ dump_RDD_TCAM_GENERIC, 0x29e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_1 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x29f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_MAX_UT_PKTS_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING5_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_1 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2b68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FPM_THRESHOLDS_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_UT_TRIGGER_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT COMMON_REPROCESSING_PD_FIFO_TABLE_1 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2b80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2bc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_1 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2be8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UDPSPDT_SCRATCH_TABLE_1 =
{
	8,
	{
		{ dump_RDD_UDPSPDT_SCRATCH_IPERF3_RX, 0x2bf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_1 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x2bf8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING6_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_SBPM_EXTS_1 =
{
	4,
	{
		{ dump_RDD_PKTGEN_SBPM_EXT, 0x2d68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_1 =
{
	7,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2d78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_NUM_OF_RX_FLOWS_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d7f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_PD_FIFO_TABLE_1 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TASK_IDX_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2de8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DOS_DROP_REASONS_CFG_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2dec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MCAST_MAX_REPLICATION_FLAG_TABLE_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2dee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_BBMSG_REPLY_SCRATCH_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2df0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2df8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dfa },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dfb },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dfc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_TCPSPDTEST_COMMON_TABLE_1 =
{
	1,
	{
		{ dump_RDD_SPDSVC_TCPSPDTEST_COMMON_ENTRY, 0x2dfd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LLQ_SELECTOR_ECN_MASK_1 =
{
	1,
	{
		{ dump_RDD_LLQ_SELECTOR_ECN, 0x2dfe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_1 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x2dff },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING7_STACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_KEY0_MASK_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2f68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2f6c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2f6e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_1 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x2f70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_1 =
{
	2,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_VLAN_KEY_MASK_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2fa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_DISABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2faa },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fab },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_TABLE_CFG_TABLE_1 =
{
	1,
	{
		{ dump_RDD_TCAM_TABLE_CFG, 0x2fac },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fad },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_1 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x2fae },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2faf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_1 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x2fb0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fbc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_SESSION_DATA_1 =
{
	32,
	{
		{ dump_RDD_PKTGEN_TX_PARAMS, 0x2fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_STATUS_TABLE_1 =
{
	32,
	{
		{ dump_RDD_BUFMNG_STATUS_ENTRY, 0x2fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_1 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_1 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x30d8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x30e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_1 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x3100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_ENGINE_GLOBAL_TABLE_1 =
{
	20,
	{
		{ dump_RDD_TCPSPDTEST_ENGINE_GLOBAL_INFO, 0x31a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_TOS_MASK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x31b8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT COMMON_REPROCESSING_UPDATE_FIFO_TABLE_1 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x31c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_1 =
{
	28,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x31e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UDPSPDT_STREAM_RX_STAT_TABLE_1 =
{
	48,
	{
		{ dump_RDD_UDPSPDT_STREAM_RX_STAT, 0x3200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_ANALYZER_UPDATE_FIFO_TABLE_1 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x32c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x32e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_1 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x3300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCPSPDTEST_UPDATE_FIFO_TABLE_1 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x3380 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x33a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_1 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x33e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UDPSPDT_STREAM_TX_STAT_TABLE_1 =
{
	40,
	{
		{ dump_RDD_UDPSPDT_STREAM_TX_STAT, 0x3400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_FPM_UG_MGMT_1 =
{
	20,
	{
		{ dump_RDD_PKTGEN_FPM_UG_MGMT_ENTRY, 0x34c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UDPSPDT_TX_PARAMS_TABLE_1 =
{
	24,
	{
		{ dump_RDD_UDPSPDT_TX_PARAMS, 0x3500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_1 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x3580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x35c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PKTGEN_TX_STREAM_SCRATCH_TABLE_1 =
{
	8,
	{
		{ dump_RDD_PKTGEN_TX_STREAM_SCRATCH_ENTRY, 0x3680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_PD_FIFO_TABLE_2 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_SCRATCHPAD_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_2 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_SCRATCHPAD_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xaa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_RATE_LIMITER_BUDGET_VALID_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xab0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_SQ_TABLE_2 =
{
	2,
	{
		{ dump_RDD_AQM_SQ_ENTRY, 0xac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_2 =
{
	8,
	{
		{ dump_RDD_RATE_LIMITER_BUDGET_DESCRIPTOR, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_2 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING0_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xe00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_2 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0xf68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_2 =
{
	16,
	{
		{ dump_RDD_CPU_INTERRUPT_COALESCING_ENTRY, 0xf70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_RATE_LIMITER_PROFILE_TABLE_2 =
{
	4,
	{
		{ dump_RDD_RATE_LIMITER_PROFILE, 0xf80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING_8_TASKS_PACKET_BUFFER_2 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING1_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_ID_DDR_ADDR_2 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2168 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_GEN_PARAM_2 =
{
	4,
	{
		{ dump_RDD_SPDTEST_GEN_CFG, 0x2170 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_2 =
{
	8,
	{
		{ dump_RDD_GENERAL_TIMER_ENTRY, 0x2180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING2_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_2 =
{
	8,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x2368 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFFER_ALLOC_REPLY_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2370 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SG_DESC_TABLE_2 =
{
	48,
	{
		{ dump_RDD_SG_DESC_ENTRY, 0x2380 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_2 =
{
	1,
	{
		{ dump_RDD_RATE_LIMITER_PROFILE_RESIDUE, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING3_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_SQ_BITMAP_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2568 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_INT_INTERRUPT_SCRATCH_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x256c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_2 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_2 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_CFG_CURRENT_TABLE_2 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x25d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING4_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_2 =
{
	8,
	{
		{ dump_RDD_CPU_RING_INTERRUPT_COUNTER_ENTRY, 0x2768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_2 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x2770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_RING_INDICES_VALUES_TABLE_2 =
{
	4,
	{
		{ dump_RDD_CPU_TX_RING_INDICES, 0x27c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_2 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x27e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING5_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_AQM_QUEUE_TABLE_2 =
{
	4,
	{
		{ dump_RDD_AQM_QUEUE_DESCRIPTOR, 0x2968 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DDR_LATENCY_DBG_USEC_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DDR_LATENCY_DBG_USEC_MAX_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29ec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_SECONDARY_SCHEDULER_TABLE_2 =
{
	12,
	{
		{ dump_RDD_SECONDARY_SCHEDULER_DESCRIPTOR, 0x29f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MUTEX_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING6_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_GENERIC_FIELDS_2 =
{
	2,
	{
		{ dump_RDD_TCAM_GENERIC, 0x2be8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_DISPATCHER_CREDIT_TABLE_2 =
{
	12,
	{
		{ dump_RDD_DISPATCHER_CREDIT_DESCRIPTOR, 0x2bf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_HOST_CNT_DISABLE_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2bfc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING7_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_2 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2d68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_AQM_TIMER_DISPATCHER_CREDIT_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_BAD_IPV4_HDR_CSUM_PACKETS_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_2 =
{
	52,
	{
		{ dump_RDD_PI2_PROBABILITY_CALC_DESCRIPTOR, 0x2d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TASK_IDX_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2db4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_2 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2db8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TR471_SPDSVC_RX_PKT_ID_TABLE_2 =
{
	12,
	{
		{ dump_RDD_TR471_SPDSVC_RX_PKT_ID, 0x2dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_EGRESS_DISPATCHER_CREDIT_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2df0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DOS_DROP_REASONS_CFG_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2dfc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_ACTION_VEC_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2dfe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_SCHEDULING_QUEUE_TABLE_2 =
{
	10,
	{
		{ dump_RDD_SCHEDULING_QUEUE_DESCRIPTOR, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_TABLE_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2f40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_2 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x2f68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_INGRESS_DISPATCHER_CREDIT_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2f70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_2 =
{
	2,
	{
		{ dump_RDD_FLUSH_CFG_ENABLE_ENTRY, 0x2f7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MCAST_MAX_REPLICATION_FLAG_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f7e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_2 =
{
	7,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2fa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_NUM_OF_RX_FLOWS_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2faf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_2 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x2fb0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2fbc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_FIRST_QUEUE_MAPPING_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2fbe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_RD_IDX_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2fe8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fea },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2feb },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LLQ_SELECTOR_ECN_MASK_2 =
{
	1,
	{
		{ dump_RDD_LLQ_SELECTOR_ECN, 0x2fed },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SCHEDULING_FLUSH_GLOBAL_CFG_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_2 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x2fef },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FPM_THRESHOLDS_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ff0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2ffc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2ffe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_2 =
{
	8,
	{
		{ dump_RDD_RATE_LIMITER_PARAMS_DESCRIPTOR, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SRAM_PD_FIFO_2 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x3200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_2 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0x3300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_DISABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33d4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33d5 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_TABLE_CFG_TABLE_2 =
{
	1,
	{
		{ dump_RDD_TCAM_TABLE_CFG, 0x33d6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33d7 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_WR_IDX_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x33d8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_2 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x33da },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33db },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x33dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_DESCRIPTOR_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BUFMNG_DESCRIPTOR_ENTRY, 0x33e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_0_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_2 =
{
	2,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x34c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_KEY0_MASK_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x34e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_DESCRIPTOR_TABLE_2 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x34f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_1_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_UPDATE_FIFO_TABLE_2 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x35c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_STATUS_TABLE_2 =
{
	32,
	{
		{ dump_RDD_BUFMNG_STATUS_ENTRY, 0x35e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_2 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x3600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_TX_QUEUE_DROP_TABLE_2 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x36a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_BUFMNG_STATUS_TABLE_2 =
{
	32,
	{
		{ dump_RDD_BUFMNG_STATUS_ENTRY, 0x37a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_STACK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x37c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x37e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SG_CONTEXT_TABLE_2 =
{
	64,
	{
		{ dump_RDD_SG_CONTEXT_ENTRY, 0x3800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RING_CPU_TX_DESCRIPTOR_DATA_TABLE_2 =
{
	16,
	{
		{ dump_RDD_RING_CPU_TX_DESCRIPTOR, 0x3880 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_SCHEDULER_TABLE_2 =
{
	24,
	{
		{ dump_RDD_SCHEDULER_DESCRIPTOR, 0x38a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x38b8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE_2 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x38e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_VLAN_KEY_MASK_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x38f8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_2 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x3900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_2 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x3980 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_SCHEDULER_POOL_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3998 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_2 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x39b8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x39c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SQ_TM_AQM_QUEUE_TIMER_TABLE_2 =
{
	1,
	{
		{ dump_RDD_AQM_QUEUE_TIMER_DESCRIPTOR, 0x39d8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_TOS_MASK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x39f8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT QUEUE_THRESHOLD_VECTOR_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_RING_DESCRIPTOR_TABLE_2 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x3a20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3a40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_SCRATCH_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3a50 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_2 =
{
	28,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x3a60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_TX_SYNC_FIFO_TABLE_2 =
{
	8,
	{
		{ dump_RDD_CPU_TX_SYNC_FIFO_ENTRY, 0x3a80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_2 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x3a90 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_2 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x3aa0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_NEXT_PTR_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3ab0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3ac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_2 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x3ae0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FLOW_RING_CACHE_CTX_TABLE_3 =
{
	16,
	{
		{ dump_RDD_DHD_FLOW_RING_CACHE_CTX_ENTRY, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_DOORBELL_TX_POST_VALUE_3 =
{
	4,
	{
		{ dump_RDD_DHD_DOORBELL, 0x300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_AUX_INFO_CACHE_TABLE_3 =
{
	16,
	{
		{ dump_RDD_DHD_AUX_INFO_CACHE_ENTRY, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_3 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0x700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MUTEX_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x7d4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_3 =
{
	8,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x7d8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x7e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_3 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MIRRORING_SCRATCH_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xaa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_GEN_PARAM_3 =
{
	4,
	{
		{ dump_RDD_SPDTEST_GEN_CFG, 0xab0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TR471_SPDSVC_RX_PKT_ID_TABLE_3 =
{
	12,
	{
		{ dump_RDD_TR471_SPDSVC_RX_PKT_ID, 0xac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xaf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_RING_CFG_TABLE_3 =
{
	24,
	{
		{ dump_RDD_FPM_RING_CFG_ENTRY, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CODEL_BIAS_SLOPE_TABLE_3 =
{
	4,
	{
		{ dump_RDD_DHD_CODEL_BIAS_SLOPE, 0xbc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_HOST_CNT_DISABLE_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xbec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_3 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0xbf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_POST_COMMON_RADIO_DATA_3 =
{
	200,
	{
		{ dump_RDD_DHD_POST_COMMON_RADIO_ENTRY, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_3 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0xe58 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_3 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xe60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_FPM_RINGS_AND_BUFMNG_REFILL_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xe80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_3 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0xf00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_DESCRIPTOR_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BUFMNG_DESCRIPTOR_ENTRY, 0xfa0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_TABLE_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0xfc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_GENERIC_FIELDS_3 =
{
	2,
	{
		{ dump_RDD_TCAM_GENERIC, 0xfe8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xff0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING_8_TASKS_PACKET_BUFFER_3 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_3 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING0_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_3 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2368 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_3 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x2370 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_BAD_IPV4_HDR_CSUM_PACKETS_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x237c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_BACKUP_INDEX_CACHE_3 =
{
	32,
	{
		{ dump_RDD_DHD_BACKUP_IDX_CACHE_TABLE, 0x2380 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_STATUS_TABLE_3 =
{
	32,
	{
		{ dump_RDD_BUFMNG_STATUS_ENTRY, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING1_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_3 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2568 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FPM_THRESHOLDS_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TASK_IDX_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x257c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_3 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_INT_ID_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DOS_DROP_REASONS_CFG_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x25dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MCAST_MAX_REPLICATION_FLAG_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x25de },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING2_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_3 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x2768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_MIRRORING_DISPATCHER_CREDIT_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x277c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDSVC_WLAN_TXPOST_PARAMS_TABLE_3 =
{
	2,
	{
		{ dump_RDD_SPDSVC_WLAN_TXPOST_PARAMS, 0x277e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_L2_HEADER_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_3 =
{
	7,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x27c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_NUM_OF_RX_FLOWS_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27cf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_3 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x27d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TIMER_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING3_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2968 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_KEY0_MASK_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x29ec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x29ed },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x29ee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LLQ_SELECTOR_ECN_MASK_3 =
{
	1,
	{
		{ dump_RDD_LLQ_SELECTOR_ECN, 0x29ef },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_3 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x29f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x29fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x29fe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING4_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_VLAN_KEY_MASK_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2b68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_3 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x2b6a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_DISABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2b6b },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2b6c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_TABLE_CFG_TABLE_3 =
{
	1,
	{
		{ dump_RDD_TCAM_TABLE_CFG, 0x2b6d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2b6e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_3 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x2b6f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_3 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x2b70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2b72 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2b73 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_TOS_MASK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2b78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_TX_POST_RING_DESCRIPTOR_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x2b80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2bc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING5_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_CPU_RECYCLE_RING_DESCRIPTOR_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x2d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING6_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_3 =
{
	2,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_POST_UPDATE_FIFO_TABLE_3 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x2fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_POST_UPDATE_FIFO_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING7_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3168 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_HW_CFG_3 =
{
	20,
	{
		{ dump_RDD_DHD_HW_CONFIGURATION, 0x31a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_3 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x31c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_3 =
{
	28,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x31e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_POST_FLOW_RING_BUFFER_3 =
{
	48,
	{
		{ dump_RDD_DHD_TX_POST_DESCRIPTOR, 0x3200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x32a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x32c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_3 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x32e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_POST_0_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x33c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_POST_1_STACK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_3 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x3500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_TX_POST_PD_FIFO_TABLE_3 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x3600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FLOW_RING_CACHE_LKP_TABLE_3 =
{
	2,
	{
		{ dump_RDD_DHD_FLOW_RING_CACHE_LKP_ENTRY, 0x3700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_PD_FIFO_TABLE_4 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_TASK_0_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING0_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xa00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_TX_WAKE_UP_DATA_TABLE_4 =
{
	8,
	{
		{ dump_RDD_BBH_TX_WAKE_UP_DATA_ENTRY, 0xb68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_RATE_LIMITER_BUDGET_VALID_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xb70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_RATE_LIMITER_PROFILE_TABLE_4 =
{
	4,
	{
		{ dump_RDD_RATE_LIMITER_PROFILE, 0xb80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_SCHEDULING_QUEUE_TABLE_4 =
{
	10,
	{
		{ dump_RDD_SCHEDULING_QUEUE_DESCRIPTOR, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_GEN_PARAM_4 =
{
	4,
	{
		{ dump_RDD_SPDTEST_GEN_CFG, 0xf70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_4 =
{
	8,
	{
		{ dump_RDD_GENERAL_TIMER_ENTRY, 0xf80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING_4_TASKS_PACKET_BUFFER_4 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REPORTING_QUEUE_DESCRIPTOR_TABLE_4 =
{
	8,
	{
		{ dump_RDD_REPORTING_QUEUE_DESCRIPTOR, 0x1800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_SCHEDULER_POOL_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1c08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUDGET_ALLOCATION_TIMER_VALUE_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x1cc2 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MUTEX_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1cc4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_4 =
{
	8,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x1cc8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_CPU_TABLE_4 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x1cd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_4 =
{
	1,
	{
		{ dump_RDD_RATE_LIMITER_PROFILE_RESIDUE, 0x1ce0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REPORTING_QUEUE_ACCUMULATED_TABLE_4 =
{
	16,
	{
		{ dump_RDD_QM_QUEUE_COUNTER_DATA, 0x1d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING1_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MIRRORING_SCRATCH_4 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x1f68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_FW_TABLE_4 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x1f70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_AQM_QUEUE_TIMER_TABLE_4 =
{
	1,
	{
		{ dump_RDD_AQM_QUEUE_TIMER_DESCRIPTOR, 0x1f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REPORT_BBH_TX_QUEUE_ID_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1fd8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_4 =
{
	8,
	{
		{ dump_RDD_RATE_LIMITER_BUDGET_DESCRIPTOR, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_TM_FLOW_CNTR_TABLE_4 =
{
	1,
	{
		{ dump_RDD_TM_FLOW_CNTR_ENTRY, 0x22c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_4 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_HOST_CNT_DISABLE_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x23d4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_GENERIC_FIELDS_4 =
{
	2,
	{
		{ dump_RDD_TCAM_GENERIC, 0x23d8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_4 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_4 =
{
	8,
	{
		{ dump_RDD_RATE_LIMITER_PARAMS_DESCRIPTOR, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x26c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_SCHEDULER_TABLE_4 =
{
	24,
	{
		{ dump_RDD_SCHEDULER_DESCRIPTOR, 0x2700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_4 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x27a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_CURRENT_TABLE_4 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x27b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFFER_CONG_SCRATCHPAD_4 =
{
	8,
	{
		{ dump_RDD_BUFFER_CONG_Q_OCCUPANCY, 0x27c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_4 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_BAD_IPV4_HDR_CSUM_PACKETS_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2aa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GHOST_REPORTING_GLOBAL_CFG_4 =
{
	4,
	{
		{ dump_RDD_GHOST_REPORTING_GLOBAL_CFG_ENTRY, 0x2aac },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_4 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x2ab0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_4 =
{
	52,
	{
		{ dump_RDD_PI2_PROBABILITY_CALC_DESCRIPTOR, 0x2ac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TASK_IDX_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2af4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_4 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2af8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_4 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2b00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_DESCRIPTOR_TABLE_4 =
{
	2,
	{
		{ dump_RDD_BUFMNG_DESCRIPTOR_ENTRY, 0x2ba0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TR471_SPDSVC_RX_PKT_ID_TABLE_4 =
{
	12,
	{
		{ dump_RDD_TR471_SPDSVC_RX_PKT_ID, 0x2bc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2bf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_4 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING2_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_4 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x2f68 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_DISPATCHER_CREDIT_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2f70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DOS_DROP_REASONS_CFG_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f7c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_ACTION_VEC_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f7e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_SECONDARY_SCHEDULER_TABLE_4 =
{
	12,
	{
		{ dump_RDD_SECONDARY_SCHEDULER_DESCRIPTOR, 0x2f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_ENABLE_TABLE_4 =
{
	2,
	{
		{ dump_RDD_FLUSH_CFG_ENABLE_ENTRY, 0x2fd4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MCAST_MAX_REPLICATION_FLAG_TABLE_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2fd6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BBH_TX_EGRESS_REPORT_COUNTER_TABLE_4 =
{
	8,
	{
		{ dump_RDD_BBH_TX_EGRESS_COUNTER_ENTRY, 0x2fd8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_STATUS_TABLE_4 =
{
	32,
	{
		{ dump_RDD_BUFMNG_STATUS_ENTRY, 0x2fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PROCESSING3_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BBH_TX_INGRESS_COUNTER_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3168 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_4 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x3170 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x317c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_BB_DESTINATION_TABLE_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x317e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_4 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x3180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DHD_FPM_THRESHOLDS_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x31d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SPDTEST_NUM_OF_RX_FLOWS_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x31dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x31dd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_FIRST_QUEUE_MAPPING_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x31de },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x31e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_AQM_QUEUE_TABLE_4 =
{
	4,
	{
		{ dump_RDD_AQM_QUEUE_DESCRIPTOR, 0x3200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_TX_QUEUE_DROP_TABLE_4 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x3360 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_4 =
{
	7,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x3568 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x356f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_DISPATCHER_CREDIT_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x357c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LLQ_SELECTOR_ECN_MASK_4 =
{
	1,
	{
		{ dump_RDD_LLQ_SELECTOR_ECN, 0x357d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SCHEDULING_FLUSH_GLOBAL_CFG_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x357e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_EXCEPTION_4 =
{
	1,
	{
		{ dump_RDD_TX_EXCEPTION_ENTRY, 0x357f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NAT_CACHE_KEY0_MASK_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x35c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x35cc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x35ce },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_4 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x35d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REPORTING_STACK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x35e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REPORTING_QUEUE_COUNTER_TABLE_4 =
{
	2,
	{
		{ dump_RDD_REPORTING_QUEUE_COUNTER, 0x3600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_4 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x3702 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CSO_DISABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3703 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3704 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TCAM_TABLE_CFG_TABLE_4 =
{
	1,
	{
		{ dump_RDD_TCAM_TABLE_CFG, 0x3705 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3706 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BACKUP_BBH_INGRESS_COUNTERS_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3707 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_VLAN_KEY_MASK_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3708 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BACKUP_BBH_EGRESS_COUNTERS_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x370a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_4 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x370b },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x370c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x370d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_4 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x3710 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFFER_CONG_DQM_NOT_EMPTY_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3720 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CODEL_BIAS_SLOPE_TABLE_4 =
{
	4,
	{
		{ dump_RDD_CODEL_BIAS_SLOPE, 0x3740 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT XGPON_REPORT_ZERO_SENT_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_BUFFER_CONG_MGT_CFG_4 =
{
	68,
	{
		{ dump_RDD_BUFFER_CONG_MGT, 0x3780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x37c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_4 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x37f8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REPORTING_COUNTER_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_CODEL_DROP_DESCRIPTOR_4 =
{
	20,
	{
		{ dump_RDD_CODEL_DROP_DESCRIPTOR, 0x38a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT NATC_L2_TOS_MASK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x38b8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_TABLE_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x38c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_4 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x3900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3980 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_QUEUE_TABLE_4 =
{
	8,
	{
		{ dump_RDD_BBH_QUEUE_DESCRIPTOR, 0x3a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_CPU_TX_ABS_COUNTERS_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3a40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_TABLE_4 =
{
	40,
	{
		{ dump_RDD_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_ENTRY, 0x3ac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3b20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x3b40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT ETH_TM_TX_TRUNCATE_MIRRORING_TABLE_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x3b80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3bc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_4 =
{
	2,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x3c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_TABLE_4 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x3c40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT QUEUE_TO_REPORT_BIT_VECTOR_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3c60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_4 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x3c80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_4 =
{
	28,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x3ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3cc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3ce0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_4 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x3d20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3d40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_PD_FIFO_TABLE_5 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_RATE_LIMITER_PROFILE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_RATE_LIMITER_PROFILE, 0x1080 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT WAN_STACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_SECONDARY_SCHEDULER_TABLE_5 =
{
	12,
	{
		{ dump_RDD_SECONDARY_SCHEDULER_DESCRIPTOR, 0x1200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_TM_FLOW_CNTR_TABLE_5 =
{
	1,
	{
		{ dump_RDD_TM_FLOW_CNTR_ENTRY, 0x1380 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_SCHEDULER_TABLE_5 =
{
	24,
	{
		{ dump_RDD_SCHEDULER_DESCRIPTOR, 0x1400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_5 =
{
	8,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x1718 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_5 =
{
	1,
	{
		{ dump_RDD_RATE_LIMITER_PROFILE_RESIDUE, 0x1720 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_STACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1740 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_5 =
{
	8,
	{
		{ dump_RDD_GENERAL_TIMER_ENTRY, 0x1780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_SCHEDULING_QUEUE_TABLE_5 =
{
	10,
	{
		{ dump_RDD_SCHEDULING_QUEUE_DESCRIPTOR, 0x1800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_5 =
{
	8,
	{
		{ dump_RDD_BBH_TX_WAKE_UP_DATA_ENTRY, 0x1d28 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_RATE_LIMITER_BUDGET_VALID_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1d30 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFFER_CONG_SCRATCHPAD_5 =
{
	8,
	{
		{ dump_RDD_BUFFER_CONG_Q_OCCUPANCY, 0x1d40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_STACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_BBH_QUEUE_TABLE_5 =
{
	8,
	{
		{ dump_RDD_BBH_QUEUE_DESCRIPTOR, 0x1e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_5 =
{
	52,
	{
		{ dump_RDD_PI2_PROBABILITY_CALC_DESCRIPTOR, 0x1f40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MUTEX_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1f74 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BBH_TX_EPON_WAKE_UP_DATA_TABLE_5 =
{
	8,
	{
		{ dump_RDD_BBH_TX_WAKE_UP_DATA_ENTRY, 0x1f78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_5 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x1f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_CPU_TABLE_5 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x1fd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_5 =
{
	8,
	{
		{ dump_RDD_RATE_LIMITER_BUDGET_DESCRIPTOR, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_5 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x2420 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CODEL_BIAS_SLOPE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_CODEL_BIAS_SLOPE, 0x2440 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFMNG_HOST_CNT_DISABLE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x246c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_FW_TABLE_5 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x2470 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_STACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2480 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MIRRORING_SCRATCH_5 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x24c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_CURRENT_TABLE_5 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x24d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT EPON_UPDATE_FIFO_STACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT WAN_EPON_STACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_SCHEDULER_POOL_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TASK_IDX_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2738 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_TX_TRUNCATE_MIRRORING_TABLE_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x273c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUDGET_ALLOCATION_TIMER_VALUE_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x273e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2740 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_TIMER_ACTION_VEC_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2768 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_ENABLE_TABLE_5 =
{
	2,
	{
		{ dump_RDD_FLUSH_CFG_ENABLE_ENTRY, 0x276a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_PAUSE_QUANTA_5 =
{
	2,
	{
		{ dump_RDD_PAUSE_QUANTA_ENTRY, 0x276c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_WAN_0_BB_DESTINATION_TABLE_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x276e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_PD_TABLE_5 =
{
	16,
	{
		{ dump_RDD_PROCESSING_RX_DESCRIPTOR, 0x2770 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_BUFFER_CONG_MGT_CFG_5 =
{
	68,
	{
		{ dump_RDD_BUFFER_CONG_MGT, 0x2780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_BBH_TX_WAN_0_FIFO_BYTES_USED_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x27c4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT QEMU_SYNC_MEM_5 =
{
	1,
	{
		{ dump_RDD_QEMU_DATA, 0x27c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_WAN_VIQ_EXCLUSIVE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27c9 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_VPORT_TO_RL_OVERHEAD_TABLE_5 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x27ca },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27cb },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_FIRST_QUEUE_MAPPING_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x27cc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_TX_PAUSE_NACK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27ce },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SCHEDULING_FLUSH_GLOBAL_CFG_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27cf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_DISPATCHER_CREDIT_TABLE_5 =
{
	12,
	{
		{ dump_RDD_DISPATCHER_CREDIT_DESCRIPTOR, 0x27d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BBH_TX_US_WAN_0_FIFO_BYTES_THRESHOLD_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x27dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_EXCEPTION_5 =
{
	1,
	{
		{ dump_RDD_TX_EXCEPTION_ENTRY, 0x27de },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27df },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_CNTR_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_5 =
{
	8,
	{
		{ dump_RDD_RATE_LIMITER_PARAMS_DESCRIPTOR, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2c20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BUFFER_CONG_DQM_NOT_EMPTY_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2cc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2ce8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2cea },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT MAC_TYPE_5 =
{
	1,
	{
		{ dump_RDD_MAC_TYPE_ENTRY, 0x2ced },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT WAN_0_BBH_TX_FIFO_SIZE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cef },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_DISPATCHER_CREDIT_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2cf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_RX_MIRRORING_SCRATCHPAD_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d88 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_5 =
{
	2,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DISPATCHER_CREDIT_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2df0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_AQM_QUEUE_TIMER_TABLE_5 =
{
	1,
	{
		{ dump_RDD_AQM_QUEUE_TIMER_DESCRIPTOR, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_CPU_TX_ABS_COUNTERS_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2e88 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_5 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x2f10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_EPON_CONTROL_SCRATCH_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2f20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BBH_TX_EPON_INGRESS_COUNTER_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2f40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_PAUSE_DEBUG_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2f70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT BBH_TX_EPON_EGRESS_COUNTER_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_CODEL_DROP_DESCRIPTOR_5 =
{
	20,
	{
		{ dump_RDD_CODEL_DROP_DESCRIPTOR, 0x2fa0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_TABLE_5 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x2fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_5 =
{
	28,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x2fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT PON_TM_AQM_QUEUE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_AQM_QUEUE_DESCRIPTOR, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_TX_QUEUE_DROP_TABLE_5 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x3210 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_DISPATCHER_CREDIT_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3620 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_5 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x3630 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT EPON_UPDATE_FIFO_TABLE_5 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x3640 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_TX_OCTETS_COUNTERS_TABLE_5 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x3660 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x36a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT US_TM_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_TABLE_5 =
{
	40,
	{
		{ dump_RDD_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_ENTRY, 0x36c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6855
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_5 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x3800 },
		{ 0, 0 },
	}
};
#endif

TABLE_STRUCT RUNNER_TABLES[NUMBER_OF_TABLES] =
{
#if defined BCM6855
	{ "RX_FLOW_TABLE", 1, CORE_0_INDEX, &RX_FLOW_TABLE, 340, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR", 1, CORE_0_INDEX, &CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_INTERRUPT_COALESCING_TABLE", 1, CORE_0_INDEX, &CPU_RECYCLE_INTERRUPT_COALESCING_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REASON_TO_METER_TABLE", 1, CORE_0_INDEX, &CPU_REASON_TO_METER_TABLE, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_METER_TABLE", 1, CORE_0_INDEX, &CPU_RX_METER_TABLE, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_SCRATCHPAD", 1, CORE_0_INDEX, &CPU_RX_SCRATCHPAD, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_COMPLETE_COMMON_RADIO_DATA", 1, CORE_0_INDEX, &DHD_COMPLETE_COMMON_RADIO_DATA, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REASON_AND_VPORT_TO_METER_TABLE", 1, CORE_0_INDEX, &CPU_REASON_AND_VPORT_TO_METER_TABLE, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_FEED_RING_CACHE_TABLE", 1, CORE_0_INDEX, &CPU_FEED_RING_CACHE_TABLE, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_FLOW_TABLE", 1, CORE_0_INDEX, &TX_FLOW_TABLE, 212, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_INT_INTERRUPT_SCRATCH", 1, CORE_0_INDEX, &CPU_INT_INTERRUPT_SCRATCH, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_INTERRUPT_ID_DDR_ADDR", 1, CORE_0_INDEX, &CPU_RX_INTERRUPT_ID_DDR_ADDR, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_FEED_RING_RSV_TABLE", 1, CORE_0_INDEX, &CPU_FEED_RING_RSV_TABLE, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_0_INDEX, &RUNNER_PROFILING_TRACE_BUFFER, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING0_STACK", 1, CORE_0_INDEX, &PROCESSING0_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "SYSTEM_CONFIGURATION", 1, CORE_0_INDEX, &SYSTEM_CONFIGURATION, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_FEED_RING_DESCRIPTOR_TABLE", 1, CORE_0_INDEX, &CPU_FEED_RING_DESCRIPTOR_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_COPY_STACK", 1, CORE_0_INDEX, &CPU_RX_COPY_STACK, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING_8_TASKS_PACKET_BUFFER", 1, CORE_0_INDEX, &PROCESSING_8_TASKS_PACKET_BUFFER, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING1_STACK", 1, CORE_0_INDEX, &PROCESSING1_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR", 1, CORE_0_INDEX, &DHD_CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_AUX_INT_SCRATCHPAD", 1, CORE_0_INDEX, &CPU_RX_AUX_INT_SCRATCHPAD, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RXQ_SCRATCH_TABLE", 1, CORE_0_INDEX, &CPU_RXQ_SCRATCH_TABLE, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING2_STACK", 1, CORE_0_INDEX, &PROCESSING2_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_POST_RING_SIZE", 1, CORE_0_INDEX, &DHD_RX_POST_RING_SIZE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DOS_DROP_REASONS_CFG", 1, CORE_0_INDEX, &DOS_DROP_REASONS_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_GEN_PARAM", 1, CORE_0_INDEX, &SPDTEST_GEN_PARAM, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER", 1, CORE_0_INDEX, &GENERAL_TIMER, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING3_STACK", 1, CORE_0_INDEX, &PROCESSING3_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_COMPLETE_RING_SIZE", 1, CORE_0_INDEX, &DHD_RX_COMPLETE_RING_SIZE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_ACTION_VEC", 1, CORE_0_INDEX, &GENERAL_TIMER_ACTION_VEC, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BRIDGE_CFG_TABLE", 1, CORE_0_INDEX, &BRIDGE_CFG_TABLE, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_COMPLETE_FLOW_RING_BUFFER", 1, CORE_0_INDEX, &DHD_RX_COMPLETE_FLOW_RING_BUFFER, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "IPV4_HOST_ADDRESS_TABLE", 1, CORE_0_INDEX, &IPV4_HOST_ADDRESS_TABLE, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING4_STACK", 1, CORE_0_INDEX, &PROCESSING4_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_COMPLETE_RING_SIZE", 1, CORE_0_INDEX, &DHD_TX_COMPLETE_RING_SIZE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "MCAST_MAX_REPLICATION_FLAG_TABLE", 1, CORE_0_INDEX, &MCAST_MAX_REPLICATION_FLAG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_0_INDEX, &INGRESS_FILTER_L2_REASON_TABLE, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_POST_FLOW_RING_BUFFER", 1, CORE_0_INDEX, &DHD_RX_POST_FLOW_RING_BUFFER, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_0_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING5_STACK", 1, CORE_0_INDEX, &PROCESSING5_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE", 1, CORE_0_INDEX, &CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_COPY_DISPATCHER_CREDIT_TABLE", 1, CORE_0_INDEX, &CPU_RX_COPY_DISPATCHER_CREDIT_TABLE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RXQ_BUF_GLOBAL_CFG_TABLE", 1, CORE_0_INDEX, &CPU_RXQ_BUF_GLOBAL_CFG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_STACK", 1, CORE_0_INDEX, &CPU_RX_STACK, 80, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_0_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MUTEX_TABLE", 1, CORE_0_INDEX, &MUTEX_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_0_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING6_STACK", 1, CORE_0_INDEX, &PROCESSING6_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_FEED_RING_INDEX_DDR_ADDR_TABLE", 1, CORE_0_INDEX, &CPU_FEED_RING_INDEX_DDR_ADDR_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FPM_THRESHOLDS", 1, CORE_0_INDEX, &DHD_FPM_THRESHOLDS, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_HOST_CNT_DISABLE_TABLE", 1, CORE_0_INDEX, &BUFMNG_HOST_CNT_DISABLE_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_TABLE", 1, CORE_0_INDEX, &VPORT_CFG_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_DOORBELL_TX_COMPLETE_VALUE", 1, CORE_0_INDEX, &DHD_DOORBELL_TX_COMPLETE_VALUE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_BAD_IPV4_HDR_CSUM_PACKETS", 1, CORE_0_INDEX, &CSO_BAD_IPV4_HDR_CSUM_PACKETS, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_DESCRIPTOR_TABLE", 1, CORE_0_INDEX, &BUFMNG_DESCRIPTOR_TABLE, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING7_STACK", 1, CORE_0_INDEX, &PROCESSING7_STACK, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_GENERIC_FIELDS", 1, CORE_0_INDEX, &TCAM_GENERIC_FIELDS, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_DOORBELL_RX_COMPLETE_VALUE", 1, CORE_0_INDEX, &DHD_DOORBELL_RX_COMPLETE_VALUE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_INTERRUPT_SCRATCH", 1, CORE_0_INDEX, &CPU_RX_INTERRUPT_SCRATCH, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_STACK", 1, CORE_0_INDEX, &GENERAL_TIMER_STACK, 72, 1, 1 },
#endif
#if defined BCM6855
	{ "TUNNELS_PARSING_CFG", 1, CORE_0_INDEX, &TUNNELS_PARSING_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_DOORBELL_RX_POST_VALUE", 1, CORE_0_INDEX, &DHD_DOORBELL_RX_POST_VALUE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "TASK_IDX", 1, CORE_0_INDEX, &TASK_IDX, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_STATUS_TABLE", 1, CORE_0_INDEX, &BUFMNG_STATUS_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_SRAM_PD_FIFO", 1, CORE_0_INDEX, &CPU_RECYCLE_SRAM_PD_FIFO, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RING_INTERRUPT_COUNTER_TABLE", 1, CORE_0_INDEX, &CPU_RING_INTERRUPT_COUNTER_TABLE, 24, 1, 1 },
#endif
#if defined BCM6855
	{ "IPV6_HOST_ADDRESS_CRC_TABLE", 1, CORE_0_INDEX, &IPV6_HOST_ADDRESS_CRC_TABLE, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RING_DESCRIPTORS_TABLE", 1, CORE_0_INDEX, &CPU_RING_DESCRIPTORS_TABLE, 24, 1, 1 },
#endif
#if defined BCM6855
	{ "REGISTERS_BUFFER", 1, CORE_0_INDEX, &REGISTERS_BUFFER, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_COPY_PD_FIFO_TABLE", 1, CORE_0_INDEX, &CPU_RX_COPY_PD_FIFO_TABLE, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_COMPLETE_0_STACK", 1, CORE_0_INDEX, &DHD_RX_COMPLETE_0_STACK, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_CONTEXT_TABLE", 1, CORE_0_INDEX, &CSO_CONTEXT_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_STACK", 1, CORE_0_INDEX, &CPU_RECYCLE_STACK, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_COMPLETE_1_STACK", 1, CORE_0_INDEX, &DHD_RX_COMPLETE_1_STACK, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "POLICER_PARAMS_TABLE", 1, CORE_0_INDEX, &POLICER_PARAMS_TABLE, 80, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_ENABLE_TABLE", 1, CORE_0_INDEX, &AQM_ENABLE_TABLE, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_COMPLETE_0_STACK", 1, CORE_0_INDEX, &DHD_TX_COMPLETE_0_STACK, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_0_INDEX, &INGRESS_FILTER_PROFILE_TABLE, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_COMPLETE_1_STACK", 1, CORE_0_INDEX, &DHD_TX_COMPLETE_1_STACK, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "TR471_SPDSVC_RX_PKT_ID_TABLE", 1, CORE_0_INDEX, &TR471_SPDSVC_RX_PKT_ID_TABLE, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_INT_ID", 1, CORE_0_INDEX, &DHD_CPU_INT_ID, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_NUM_QUEUES", 1, CORE_0_INDEX, &AQM_NUM_QUEUES, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_NUM_OF_RX_FLOWS", 1, CORE_0_INDEX, &SPDTEST_NUM_OF_RX_FLOWS, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_0_INDEX, &LOOPBACK_WAN_FLOW_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_RECYCLE_RING_DESCRIPTOR_TABLE", 1, CORE_0_INDEX, &DHD_CPU_RECYCLE_RING_DESCRIPTOR_TABLE, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_WLAN_GEN_PARAMS_TABLE", 1, CORE_0_INDEX, &SPDSVC_WLAN_GEN_PARAMS_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FORCE_DSCP", 1, CORE_0_INDEX, &FORCE_DSCP, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CORE_ID_TABLE", 1, CORE_0_INDEX, &CORE_ID_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LLQ_SELECTOR_ECN_MASK", 1, CORE_0_INDEX, &LLQ_SELECTOR_ECN_MASK, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_FEED_RING_CACHE_OFFSET", 1, CORE_0_INDEX, &CPU_FEED_RING_CACHE_OFFSET, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_0_INDEX, &RX_MIRRORING_CONFIGURATION, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_COMPLETE_FLOW_RING_BUFFER", 1, CORE_0_INDEX, &DHD_TX_COMPLETE_FLOW_RING_BUFFER, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_RING_DESCRIPTOR_TABLE", 1, CORE_0_INDEX, &CPU_RECYCLE_RING_DESCRIPTOR_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_RX_COMPLETE_DISPATCHER_CREDIT_TABLE", 1, CORE_0_INDEX, &DHD_RX_COMPLETE_DISPATCHER_CREDIT_TABLE, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE", 1, CORE_0_INDEX, &CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_COMPLETE_DISPATCHER_CREDIT_TABLE", 1, CORE_0_INDEX, &DHD_TX_COMPLETE_DISPATCHER_CREDIT_TABLE, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_INTERRUPT_COALESCING_TABLE", 1, CORE_0_INDEX, &CPU_INTERRUPT_COALESCING_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_TABLE", 1, CORE_0_INDEX, &RX_MIRRORING_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CFG_TABLE", 1, CORE_0_INDEX, &IPTV_CFG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_INTERRUPT_SCRATCH", 1, CORE_0_INDEX, &CPU_RECYCLE_INTERRUPT_SCRATCH, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_0_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_0_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_LOCAL_SCRATCH", 1, CORE_0_INDEX, &CPU_RX_LOCAL_SCRATCH, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_CFG", 1, CORE_0_INDEX, &NAT_CACHE_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REDIRECT_MODE", 1, CORE_0_INDEX, &CPU_REDIRECT_MODE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_0_INDEX, &LOOPBACK_QUEUE_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_SHADOW_RD_IDX", 1, CORE_0_INDEX, &CPU_RECYCLE_SHADOW_RD_IDX, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_0_INDEX, &TX_MIRRORING_CONFIGURATION, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_DISABLE", 1, CORE_0_INDEX, &CSO_DISABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_0_INDEX, &DEBUG_PRINT_CORE_LOCK, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_TABLE_CFG_TABLE", 1, CORE_0_INDEX, &TCAM_TABLE_CFG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SRAM_DUMMY_STORE", 1, CORE_0_INDEX, &SRAM_DUMMY_STORE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_TABLE", 1, CORE_0_INDEX, &DEBUG_PRINT_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_EX_TABLE", 1, CORE_0_INDEX, &VPORT_CFG_EX_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REASON_TO_TC", 1, CORE_0_INDEX, &CPU_REASON_TO_TC, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "TC_TO_CPU_RXQ", 1, CORE_0_INDEX, &TC_TO_CPU_RXQ, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "EXC_TC_TO_CPU_RXQ", 1, CORE_0_INDEX, &EXC_TC_TO_CPU_RXQ, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_SHADOW_WR_IDX", 1, CORE_0_INDEX, &CPU_RECYCLE_SHADOW_WR_IDX, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_0_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_0_INDEX, &INGRESS_FILTER_1588_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_0_INDEX, &RX_MIRRORING_DIRECT_ENABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GDX_PARAMS_TABLE", 1, CORE_0_INDEX, &GDX_PARAMS_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RXQ_DATA_BUF_TYPE_TABLE", 1, CORE_0_INDEX, &CPU_RXQ_DATA_BUF_TYPE_TABLE, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_HW_CFG", 1, CORE_0_INDEX, &DHD_HW_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_KEY0_MASK", 1, CORE_0_INDEX, &NAT_CACHE_KEY0_MASK, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "UPDATE_FIFO_TABLE", 1, CORE_0_INDEX, &UPDATE_FIFO_TABLE, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_GLOBAL_CFG", 1, CORE_0_INDEX, &FPM_GLOBAL_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_COPY_UPDATE_FIFO_TABLE", 1, CORE_0_INDEX, &CPU_RX_COPY_UPDATE_FIFO_TABLE, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_0_INDEX, &FW_ERROR_VECTOR_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_VLAN_KEY_MASK", 1, CORE_0_INDEX, &NATC_L2_VLAN_KEY_MASK, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_NEXT_PTR_TABLE", 1, CORE_0_INDEX, &CPU_RECYCLE_NEXT_PTR_TABLE, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_CFG", 1, CORE_0_INDEX, &INGRESS_FILTER_CFG, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PD_FIFO_TABLE", 1, CORE_0_INDEX, &PD_FIFO_TABLE, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_SCRATCHPAD", 1, CORE_0_INDEX, &DEBUG_SCRATCHPAD, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_TOS_MASK", 1, CORE_0_INDEX, &NATC_L2_TOS_MASK, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MULTICAST_KEY_MASK", 1, CORE_0_INDEX, &MULTICAST_KEY_MASK, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FPM_REPLY", 1, CORE_0_INDEX, &DHD_FPM_REPLY, 24, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_0_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_VPORT_TO_METER_TABLE", 1, CORE_0_INDEX, &CPU_VPORT_TO_METER_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_0_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_0_INDEX, &BITS_CALC_MASKS_TABLE, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_CPU_OBJ", 1, CORE_0_INDEX, &VPORT_TO_CPU_OBJ, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_STREAM_TABLE", 1, CORE_1_INDEX, &TCPSPDTEST_STREAM_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_1_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_1, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING0_STACK", 1, CORE_1_INDEX, &PROCESSING0_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_INTERRUPT_ID_DDR_ADDR", 1, CORE_1_INDEX, &CPU_RX_INTERRUPT_ID_DDR_ADDR_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_GEN_PARAM", 1, CORE_1_INDEX, &SPDTEST_GEN_PARAM_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_SCRATCHPAD", 1, CORE_1_INDEX, &TCPSPDTEST_SCRATCHPAD_1, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_FLOW_TABLE", 1, CORE_1_INDEX, &RX_FLOW_TABLE_1, 340, 1, 1 },
#endif
#if defined BCM6855
	{ "SYSTEM_CONFIGURATION", 1, CORE_1_INDEX, &SYSTEM_CONFIGURATION_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_ANALYZER_PARAMS_TABLE", 1, CORE_1_INDEX, &SPDSVC_ANALYZER_PARAMS_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "COMMON_REPROCESSING_STACK", 1, CORE_1_INDEX, &COMMON_REPROCESSING_STACK_1, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_ANALYZER_STACK", 1, CORE_1_INDEX, &SPDSVC_ANALYZER_STACK_1, 256, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_TX_STREAM_TABLE", 1, CORE_1_INDEX, &PKTGEN_TX_STREAM_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_1_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_1, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_GEN_PARAMS_TABLE", 1, CORE_1_INDEX, &SPDSVC_GEN_PARAMS_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_NO_SBPM_HDRS_CNTR", 1, CORE_1_INDEX, &PKTGEN_NO_SBPM_HDRS_CNTR_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_GEN_STACK", 1, CORE_1_INDEX, &SPDSVC_GEN_STACK_1, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_DOWNLOAD_STACK", 1, CORE_1_INDEX, &TCPSPDTEST_DOWNLOAD_STACK_1, 256, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING_8_TASKS_PACKET_BUFFER", 1, CORE_1_INDEX, &PROCESSING_8_TASKS_PACKET_BUFFER_1, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_1_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_1, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING1_STACK", 1, CORE_1_INDEX, &PROCESSING1_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_ABS_RECYCLE_COUNTERS", 1, CORE_1_INDEX, &TX_ABS_RECYCLE_COUNTERS_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BRIDGE_CFG_TABLE", 1, CORE_1_INDEX, &BRIDGE_CFG_TABLE_1, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_TABLE", 1, CORE_1_INDEX, &VPORT_CFG_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_1_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_1, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_DESCRIPTOR_TABLE", 1, CORE_1_INDEX, &BUFMNG_DESCRIPTOR_TABLE_1, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING2_STACK", 1, CORE_1_INDEX, &PROCESSING2_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_CURR_SBPM_HDR_PTR", 1, CORE_1_INDEX, &PKTGEN_CURR_SBPM_HDR_PTR_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_GEN_DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &SPDSVC_GEN_DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "MUTEX_TABLE", 1, CORE_1_INDEX, &MUTEX_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_ANALYZER_PD_FIFO_TABLE", 1, CORE_1_INDEX, &SPDSVC_ANALYZER_PD_FIFO_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "TR471_SPDSVC_RX_PKT_ID_TABLE", 1, CORE_1_INDEX, &TR471_SPDSVC_RX_PKT_ID_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &TCPSPDTEST_DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_HOST_CNT_DISABLE_TABLE", 1, CORE_1_INDEX, &BUFMNG_HOST_CNT_DISABLE_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING3_STACK", 1, CORE_1_INDEX, &PROCESSING3_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_NUM_OF_AVAIL_SBPM_HDRS", 1, CORE_1_INDEX, &PKTGEN_NUM_OF_AVAIL_SBPM_HDRS_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "COMMON_REPROCESSING_DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &COMMON_REPROCESSING_DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_BAD_IPV4_HDR_CSUM_PACKETS", 1, CORE_1_INDEX, &CSO_BAD_IPV4_HDR_CSUM_PACKETS_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_SBPM_HDR_BNS", 1, CORE_1_INDEX, &PKTGEN_SBPM_HDR_BNS_1, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_TABLE", 1, CORE_1_INDEX, &RX_MIRRORING_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_SBPM_END_PTR", 1, CORE_1_INDEX, &PKTGEN_SBPM_END_PTR_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_GEN_DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &TCPSPDTEST_GEN_DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_BAD_GET_NEXT", 1, CORE_1_INDEX, &PKTGEN_BAD_GET_NEXT_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING4_STACK", 1, CORE_1_INDEX, &PROCESSING4_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "REGISTERS_BUFFER", 1, CORE_1_INDEX, &REGISTERS_BUFFER_1, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_GENERIC_FIELDS", 1, CORE_1_INDEX, &TCAM_GENERIC_FIELDS_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_1_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_MAX_UT_PKTS", 1, CORE_1_INDEX, &PKTGEN_MAX_UT_PKTS_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING5_STACK", 1, CORE_1_INDEX, &PROCESSING5_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "TUNNELS_PARSING_CFG", 1, CORE_1_INDEX, &TUNNELS_PARSING_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FPM_THRESHOLDS", 1, CORE_1_INDEX, &DHD_FPM_THRESHOLDS_1, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_UT_TRIGGER", 1, CORE_1_INDEX, &PKTGEN_UT_TRIGGER_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "COMMON_REPROCESSING_PD_FIFO_TABLE", 1, CORE_1_INDEX, &COMMON_REPROCESSING_PD_FIFO_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_1_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CFG_TABLE", 1, CORE_1_INDEX, &IPTV_CFG_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "UDPSPDT_SCRATCH_TABLE", 1, CORE_1_INDEX, &UDPSPDT_SCRATCH_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_1_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING6_STACK", 1, CORE_1_INDEX, &PROCESSING6_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_SBPM_EXTS", 1, CORE_1_INDEX, &PKTGEN_SBPM_EXTS_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_CFG", 1, CORE_1_INDEX, &NAT_CACHE_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_NUM_OF_RX_FLOWS", 1, CORE_1_INDEX, &SPDTEST_NUM_OF_RX_FLOWS_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_PD_FIFO_TABLE", 1, CORE_1_INDEX, &TCPSPDTEST_PD_FIFO_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_1_INDEX, &LOOPBACK_QUEUE_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "TASK_IDX", 1, CORE_1_INDEX, &TASK_IDX_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DOS_DROP_REASONS_CFG", 1, CORE_1_INDEX, &DOS_DROP_REASONS_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MCAST_MAX_REPLICATION_FLAG_TABLE", 1, CORE_1_INDEX, &MCAST_MAX_REPLICATION_FLAG_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_BBMSG_REPLY_SCRATCH", 1, CORE_1_INDEX, &PKTGEN_BBMSG_REPLY_SCRATCH_1, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_NUM_QUEUES", 1, CORE_1_INDEX, &AQM_NUM_QUEUES_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_1_INDEX, &LOOPBACK_WAN_FLOW_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FORCE_DSCP", 1, CORE_1_INDEX, &FORCE_DSCP_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CORE_ID_TABLE", 1, CORE_1_INDEX, &CORE_ID_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_TCPSPDTEST_COMMON_TABLE", 1, CORE_1_INDEX, &SPDSVC_TCPSPDTEST_COMMON_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LLQ_SELECTOR_ECN_MASK", 1, CORE_1_INDEX, &LLQ_SELECTOR_ECN_MASK_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REDIRECT_MODE", 1, CORE_1_INDEX, &CPU_REDIRECT_MODE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING7_STACK", 1, CORE_1_INDEX, &PROCESSING7_STACK_1, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_KEY0_MASK", 1, CORE_1_INDEX, &NAT_CACHE_KEY0_MASK_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_1_INDEX, &RX_MIRRORING_CONFIGURATION_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_1_INDEX, &TX_MIRRORING_CONFIGURATION_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_TABLE", 1, CORE_1_INDEX, &DEBUG_PRINT_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_EX_TABLE", 1, CORE_1_INDEX, &VPORT_CFG_EX_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_VLAN_KEY_MASK", 1, CORE_1_INDEX, &NATC_L2_VLAN_KEY_MASK_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_DISABLE", 1, CORE_1_INDEX, &CSO_DISABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_1_INDEX, &DEBUG_PRINT_CORE_LOCK_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_TABLE_CFG_TABLE", 1, CORE_1_INDEX, &TCAM_TABLE_CFG_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SRAM_DUMMY_STORE", 1, CORE_1_INDEX, &SRAM_DUMMY_STORE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_1_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_1_INDEX, &INGRESS_FILTER_1588_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GDX_PARAMS_TABLE", 1, CORE_1_INDEX, &GDX_PARAMS_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_1_INDEX, &RX_MIRRORING_DIRECT_ENABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_SESSION_DATA", 1, CORE_1_INDEX, &PKTGEN_SESSION_DATA_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_STATUS_TABLE", 1, CORE_1_INDEX, &BUFMNG_STATUS_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_FLOW_TABLE", 1, CORE_1_INDEX, &TX_FLOW_TABLE_1, 212, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_CFG", 1, CORE_1_INDEX, &INGRESS_FILTER_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_ENABLE_TABLE", 1, CORE_1_INDEX, &AQM_ENABLE_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "POLICER_PARAMS_TABLE", 1, CORE_1_INDEX, &POLICER_PARAMS_TABLE_1, 80, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_ENGINE_GLOBAL_TABLE", 1, CORE_1_INDEX, &TCPSPDTEST_ENGINE_GLOBAL_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_TOS_MASK", 1, CORE_1_INDEX, &NATC_L2_TOS_MASK_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "COMMON_REPROCESSING_UPDATE_FIFO_TABLE", 1, CORE_1_INDEX, &COMMON_REPROCESSING_UPDATE_FIFO_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_GLOBAL_CFG", 1, CORE_1_INDEX, &FPM_GLOBAL_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "UDPSPDT_STREAM_RX_STAT_TABLE", 1, CORE_1_INDEX, &UDPSPDT_STREAM_RX_STAT_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_ANALYZER_UPDATE_FIFO_TABLE", 1, CORE_1_INDEX, &SPDSVC_ANALYZER_UPDATE_FIFO_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_1_INDEX, &FW_ERROR_VECTOR_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_1_INDEX, &INGRESS_FILTER_PROFILE_TABLE_1, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "TCPSPDTEST_UPDATE_FIFO_TABLE", 1, CORE_1_INDEX, &TCPSPDTEST_UPDATE_FIFO_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_SCRATCHPAD", 1, CORE_1_INDEX, &DEBUG_SCRATCHPAD_1, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "MULTICAST_KEY_MASK", 1, CORE_1_INDEX, &MULTICAST_KEY_MASK_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "UDPSPDT_STREAM_TX_STAT_TABLE", 1, CORE_1_INDEX, &UDPSPDT_STREAM_TX_STAT_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_FPM_UG_MGMT", 1, CORE_1_INDEX, &PKTGEN_FPM_UG_MGMT_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "UDPSPDT_TX_PARAMS_TABLE", 1, CORE_1_INDEX, &UDPSPDT_TX_PARAMS_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_1_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_1_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_1, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_1_INDEX, &BITS_CALC_MASKS_TABLE_1, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "PKTGEN_TX_STREAM_SCRATCH_TABLE", 1, CORE_1_INDEX, &PKTGEN_TX_STREAM_SCRATCH_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_PD_FIFO_TABLE", 1, CORE_2_INDEX, &SQ_TM_PD_FIFO_TABLE_2, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_SCRATCHPAD", 1, CORE_2_INDEX, &CPU_TX_SCRATCHPAD_2, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_FLOW_TABLE", 1, CORE_2_INDEX, &RX_FLOW_TABLE_2, 340, 1, 1 },
#endif
#if defined BCM6855
	{ "SERVICE_QUEUES_SCRATCHPAD", 1, CORE_2_INDEX, &SERVICE_QUEUES_SCRATCHPAD_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_RATE_LIMITER_BUDGET_VALID", 1, CORE_2_INDEX, &SQ_TM_RATE_LIMITER_BUDGET_VALID_2, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_SQ_TABLE", 1, CORE_2_INDEX, &AQM_SQ_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &SQ_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_2_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_2, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING0_STACK", 1, CORE_2_INDEX, &PROCESSING0_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR", 1, CORE_2_INDEX, &CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_INTERRUPT_COALESCING_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_RATE_LIMITER_PROFILE_TABLE", 1, CORE_2_INDEX, &SQ_TM_RATE_LIMITER_PROFILE_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING_8_TASKS_PACKET_BUFFER", 1, CORE_2_INDEX, &PROCESSING_8_TASKS_PACKET_BUFFER_2, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING1_STACK", 1, CORE_2_INDEX, &PROCESSING1_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RX_INTERRUPT_ID_DDR_ADDR", 1, CORE_2_INDEX, &CPU_RX_INTERRUPT_ID_DDR_ADDR_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_GEN_PARAM", 1, CORE_2_INDEX, &SPDTEST_GEN_PARAM_2, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER", 1, CORE_2_INDEX, &GENERAL_TIMER_2, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING2_STACK", 1, CORE_2_INDEX, &PROCESSING2_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "SYSTEM_CONFIGURATION", 1, CORE_2_INDEX, &SYSTEM_CONFIGURATION_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFFER_ALLOC_REPLY", 1, CORE_2_INDEX, &BUFFER_ALLOC_REPLY_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "SG_DESC_TABLE", 1, CORE_2_INDEX, &SG_DESC_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE", 1, CORE_2_INDEX, &SQ_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING3_STACK", 1, CORE_2_INDEX, &PROCESSING3_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_SQ_BITMAP", 1, CORE_2_INDEX, &AQM_SQ_BITMAP_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_INT_INTERRUPT_SCRATCH", 1, CORE_2_INDEX, &CPU_INT_INTERRUPT_SCRATCH_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SERVICE_QUEUES_FLUSH_CFG_FW_TABLE", 1, CORE_2_INDEX, &SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_TABLE", 1, CORE_2_INDEX, &VPORT_CFG_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "SERVICE_QUEUES_FLUSH_CFG_CURRENT_TABLE", 1, CORE_2_INDEX, &SERVICE_QUEUES_FLUSH_CFG_CURRENT_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_2_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_2, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING4_STACK", 1, CORE_2_INDEX, &PROCESSING4_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BRIDGE_CFG_TABLE", 1, CORE_2_INDEX, &BRIDGE_CFG_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_STACK", 1, CORE_2_INDEX, &GENERAL_TIMER_STACK_2, 72, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_RING_INDICES_VALUES_TABLE", 1, CORE_2_INDEX, &CPU_TX_RING_INDICES_VALUES_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_2_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_2_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING5_STACK", 1, CORE_2_INDEX, &PROCESSING5_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_AQM_QUEUE_TABLE", 1, CORE_2_INDEX, &SQ_TM_AQM_QUEUE_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "DDR_LATENCY_DBG_USEC", 1, CORE_2_INDEX, &DDR_LATENCY_DBG_USEC_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DDR_LATENCY_DBG_USEC_MAX", 1, CORE_2_INDEX, &DDR_LATENCY_DBG_USEC_MAX_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_SECONDARY_SCHEDULER_TABLE", 1, CORE_2_INDEX, &SQ_TM_SECONDARY_SCHEDULER_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MUTEX_TABLE", 1, CORE_2_INDEX, &MUTEX_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING6_STACK", 1, CORE_2_INDEX, &PROCESSING6_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "REGISTERS_BUFFER", 1, CORE_2_INDEX, &REGISTERS_BUFFER_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_GENERIC_FIELDS", 1, CORE_2_INDEX, &TCAM_GENERIC_FIELDS_2, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_DISPATCHER_CREDIT_TABLE", 1, CORE_2_INDEX, &SQ_TM_DISPATCHER_CREDIT_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_HOST_CNT_DISABLE_TABLE", 1, CORE_2_INDEX, &BUFMNG_HOST_CNT_DISABLE_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING7_STACK", 1, CORE_2_INDEX, &PROCESSING7_STACK_2, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "TUNNELS_PARSING_CFG", 1, CORE_2_INDEX, &TUNNELS_PARSING_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SERVICE_QUEUES_AQM_TIMER_DISPATCHER_CREDIT_TABLE", 1, CORE_2_INDEX, &SERVICE_QUEUES_AQM_TIMER_DISPATCHER_CREDIT_TABLE_2, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_BAD_IPV4_HDR_CSUM_PACKETS", 1, CORE_2_INDEX, &CSO_BAD_IPV4_HDR_CSUM_PACKETS_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR", 1, CORE_2_INDEX, &SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TASK_IDX", 1, CORE_2_INDEX, &TASK_IDX_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CFG_TABLE", 1, CORE_2_INDEX, &IPTV_CFG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TR471_SPDSVC_RX_PKT_ID_TABLE", 1, CORE_2_INDEX, &TR471_SPDSVC_RX_PKT_ID_TABLE_2, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_EGRESS_DISPATCHER_CREDIT_TABLE", 1, CORE_2_INDEX, &CPU_TX_EGRESS_DISPATCHER_CREDIT_TABLE_2, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DOS_DROP_REASONS_CFG", 1, CORE_2_INDEX, &DOS_DROP_REASONS_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_ACTION_VEC", 1, CORE_2_INDEX, &GENERAL_TIMER_ACTION_VEC_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_SCHEDULING_QUEUE_TABLE", 1, CORE_2_INDEX, &SQ_TM_SCHEDULING_QUEUE_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_TABLE", 1, CORE_2_INDEX, &RX_MIRRORING_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_2_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_INGRESS_DISPATCHER_CREDIT_TABLE", 1, CORE_2_INDEX, &CPU_TX_INGRESS_DISPATCHER_CREDIT_TABLE_2, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE", 1, CORE_2_INDEX, &SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MCAST_MAX_REPLICATION_FLAG_TABLE", 1, CORE_2_INDEX, &MCAST_MAX_REPLICATION_FLAG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_2_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_CFG", 1, CORE_2_INDEX, &NAT_CACHE_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_NUM_OF_RX_FLOWS", 1, CORE_2_INDEX, &SPDTEST_NUM_OF_RX_FLOWS_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_2_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_NUM_QUEUES", 1, CORE_2_INDEX, &AQM_NUM_QUEUES_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_FIRST_QUEUE_MAPPING", 1, CORE_2_INDEX, &SQ_TM_FIRST_QUEUE_MAPPING_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_2_INDEX, &LOOPBACK_QUEUE_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_SHADOW_RD_IDX", 1, CORE_2_INDEX, &CPU_RECYCLE_SHADOW_RD_IDX_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_2_INDEX, &LOOPBACK_WAN_FLOW_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FORCE_DSCP", 1, CORE_2_INDEX, &FORCE_DSCP_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CORE_ID_TABLE", 1, CORE_2_INDEX, &CORE_ID_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LLQ_SELECTOR_ECN_MASK", 1, CORE_2_INDEX, &LLQ_SELECTOR_ECN_MASK_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SCHEDULING_FLUSH_GLOBAL_CFG", 1, CORE_2_INDEX, &SCHEDULING_FLUSH_GLOBAL_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REDIRECT_MODE", 1, CORE_2_INDEX, &CPU_REDIRECT_MODE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FPM_THRESHOLDS", 1, CORE_2_INDEX, &DHD_FPM_THRESHOLDS_2, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_2_INDEX, &RX_MIRRORING_CONFIGURATION_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_2_INDEX, &TX_MIRRORING_CONFIGURATION_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &SQ_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "SERVICE_QUEUES_STACK", 1, CORE_2_INDEX, &SERVICE_QUEUES_STACK_2, 256, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_SRAM_PD_FIFO", 1, CORE_2_INDEX, &CPU_RECYCLE_SRAM_PD_FIFO_2, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_FLOW_TABLE", 1, CORE_2_INDEX, &TX_FLOW_TABLE_2, 212, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_DISABLE", 1, CORE_2_INDEX, &CSO_DISABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_2_INDEX, &DEBUG_PRINT_CORE_LOCK_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_TABLE_CFG_TABLE", 1, CORE_2_INDEX, &TCAM_TABLE_CFG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SRAM_DUMMY_STORE", 1, CORE_2_INDEX, &SRAM_DUMMY_STORE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_SHADOW_WR_IDX", 1, CORE_2_INDEX, &CPU_RECYCLE_SHADOW_WR_IDX_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_2_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_2_INDEX, &INGRESS_FILTER_1588_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_2_INDEX, &RX_MIRRORING_DIRECT_ENABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &BUFMNG_DESCRIPTOR_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_0_STACK", 1, CORE_2_INDEX, &CPU_TX_0_STACK_2, 192, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_EX_TABLE", 1, CORE_2_INDEX, &VPORT_CFG_EX_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_KEY0_MASK", 1, CORE_2_INDEX, &NAT_CACHE_KEY0_MASK_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_RING_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_RING_DESCRIPTOR_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_1_STACK", 1, CORE_2_INDEX, &CPU_TX_1_STACK_2, 192, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_UPDATE_FIFO_TABLE", 1, CORE_2_INDEX, &SQ_TM_UPDATE_FIFO_TABLE_2, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_STATUS_TABLE", 1, CORE_2_INDEX, &BUFMNG_STATUS_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "POLICER_PARAMS_TABLE", 1, CORE_2_INDEX, &POLICER_PARAMS_TABLE_2, 80, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_TX_QUEUE_DROP_TABLE", 1, CORE_2_INDEX, &SQ_TM_TX_QUEUE_DROP_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_BUFMNG_STATUS_TABLE", 1, CORE_2_INDEX, &SQ_TM_BUFMNG_STATUS_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_STACK", 1, CORE_2_INDEX, &CPU_RECYCLE_STACK_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_ENABLE_TABLE", 1, CORE_2_INDEX, &AQM_ENABLE_TABLE_2, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "SG_CONTEXT_TABLE", 1, CORE_2_INDEX, &SG_CONTEXT_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "RING_CPU_TX_DESCRIPTOR_DATA_TABLE", 1, CORE_2_INDEX, &RING_CPU_TX_DESCRIPTOR_DATA_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_SCHEDULER_TABLE", 1, CORE_2_INDEX, &SQ_TM_SCHEDULER_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_SCRATCHPAD", 1, CORE_2_INDEX, &DEBUG_SCRATCHPAD_2, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_VLAN_KEY_MASK", 1, CORE_2_INDEX, &NATC_L2_VLAN_KEY_MASK_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_2_INDEX, &INGRESS_FILTER_PROFILE_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_2_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_SCHEDULER_POOL", 1, CORE_2_INDEX, &SQ_TM_SCHEDULER_POOL_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_CFG", 1, CORE_2_INDEX, &INGRESS_FILTER_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_2_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_2, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "SQ_TM_AQM_QUEUE_TIMER_TABLE", 1, CORE_2_INDEX, &SQ_TM_AQM_QUEUE_TIMER_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_TOS_MASK", 1, CORE_2_INDEX, &NATC_L2_TOS_MASK_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "QUEUE_THRESHOLD_VECTOR", 1, CORE_2_INDEX, &QUEUE_THRESHOLD_VECTOR_2, 5, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_RING_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &CPU_TX_RING_DESCRIPTOR_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_2_INDEX, &BITS_CALC_MASKS_TABLE_2, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_INTERRUPT_SCRATCH", 1, CORE_2_INDEX, &CPU_RECYCLE_INTERRUPT_SCRATCH_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_GLOBAL_CFG", 1, CORE_2_INDEX, &FPM_GLOBAL_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_TX_SYNC_FIFO_TABLE", 1, CORE_2_INDEX, &CPU_TX_SYNC_FIFO_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_TABLE", 1, CORE_2_INDEX, &DEBUG_PRINT_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GDX_PARAMS_TABLE", 1, CORE_2_INDEX, &GDX_PARAMS_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_RECYCLE_NEXT_PTR_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_NEXT_PTR_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_2_INDEX, &FW_ERROR_VECTOR_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MULTICAST_KEY_MASK", 1, CORE_2_INDEX, &MULTICAST_KEY_MASK_2, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FLOW_RING_CACHE_CTX_TABLE", 1, CORE_3_INDEX, &DHD_FLOW_RING_CACHE_CTX_TABLE_3, 48, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_DOORBELL_TX_POST_VALUE", 1, CORE_3_INDEX, &DHD_DOORBELL_TX_POST_VALUE_3, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_AUX_INFO_CACHE_TABLE", 1, CORE_3_INDEX, &DHD_AUX_INFO_CACHE_TABLE_3, 48, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_FLOW_TABLE", 1, CORE_3_INDEX, &TX_FLOW_TABLE_3, 212, 1, 1 },
#endif
#if defined BCM6855
	{ "MUTEX_TABLE", 1, CORE_3_INDEX, &MUTEX_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SYSTEM_CONFIGURATION", 1, CORE_3_INDEX, &SYSTEM_CONFIGURATION_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_3_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_3, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_FLOW_TABLE", 1, CORE_3_INDEX, &RX_FLOW_TABLE_3, 340, 1, 1 },
#endif
#if defined BCM6855
	{ "MIRRORING_SCRATCH", 1, CORE_3_INDEX, &MIRRORING_SCRATCH_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_GEN_PARAM", 1, CORE_3_INDEX, &SPDTEST_GEN_PARAM_3, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "TR471_SPDSVC_RX_PKT_ID_TABLE", 1, CORE_3_INDEX, &TR471_SPDSVC_RX_PKT_ID_TABLE_3, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "NULL_BUFFER", 1, CORE_3_INDEX, &NULL_BUFFER_3, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_RING_CFG_TABLE", 1, CORE_3_INDEX, &FPM_RING_CFG_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CODEL_BIAS_SLOPE_TABLE", 1, CORE_3_INDEX, &DHD_CODEL_BIAS_SLOPE_TABLE_3, 11, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_HOST_CNT_DISABLE_TABLE", 1, CORE_3_INDEX, &BUFMNG_HOST_CNT_DISABLE_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BRIDGE_CFG_TABLE", 1, CORE_3_INDEX, &BRIDGE_CFG_TABLE_3, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_POST_COMMON_RADIO_DATA", 1, CORE_3_INDEX, &DHD_POST_COMMON_RADIO_DATA_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR", 1, CORE_3_INDEX, &DHD_CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_3_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_3, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_FPM_RINGS_AND_BUFMNG_REFILL_STACK", 1, CORE_3_INDEX, &CPU_FPM_RINGS_AND_BUFMNG_REFILL_STACK_3, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "POLICER_PARAMS_TABLE", 1, CORE_3_INDEX, &POLICER_PARAMS_TABLE_3, 80, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_DESCRIPTOR_TABLE", 1, CORE_3_INDEX, &BUFMNG_DESCRIPTOR_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_TABLE", 1, CORE_3_INDEX, &RX_MIRRORING_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_GENERIC_FIELDS", 1, CORE_3_INDEX, &TCAM_GENERIC_FIELDS_3, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_3_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING_8_TASKS_PACKET_BUFFER", 1, CORE_3_INDEX, &PROCESSING_8_TASKS_PACKET_BUFFER_3, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_3_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_3, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING0_STACK", 1, CORE_3_INDEX, &PROCESSING0_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "TUNNELS_PARSING_CFG", 1, CORE_3_INDEX, &TUNNELS_PARSING_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_3_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_BAD_IPV4_HDR_CSUM_PACKETS", 1, CORE_3_INDEX, &CSO_BAD_IPV4_HDR_CSUM_PACKETS_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_BACKUP_INDEX_CACHE", 1, CORE_3_INDEX, &DHD_BACKUP_INDEX_CACHE_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_STATUS_TABLE", 1, CORE_3_INDEX, &BUFMNG_STATUS_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING1_STACK", 1, CORE_3_INDEX, &PROCESSING1_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CFG_TABLE", 1, CORE_3_INDEX, &IPTV_CFG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FPM_THRESHOLDS", 1, CORE_3_INDEX, &DHD_FPM_THRESHOLDS_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "TASK_IDX", 1, CORE_3_INDEX, &TASK_IDX_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_TABLE", 1, CORE_3_INDEX, &VPORT_CFG_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_INT_ID", 1, CORE_3_INDEX, &DHD_CPU_INT_ID_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DOS_DROP_REASONS_CFG", 1, CORE_3_INDEX, &DOS_DROP_REASONS_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MCAST_MAX_REPLICATION_FLAG_TABLE", 1, CORE_3_INDEX, &MCAST_MAX_REPLICATION_FLAG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_ENABLE_TABLE", 1, CORE_3_INDEX, &AQM_ENABLE_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING2_STACK", 1, CORE_3_INDEX, &PROCESSING2_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_3_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_MIRRORING_DISPATCHER_CREDIT_TABLE", 1, CORE_3_INDEX, &DHD_MIRRORING_DISPATCHER_CREDIT_TABLE_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_NUM_QUEUES", 1, CORE_3_INDEX, &AQM_NUM_QUEUES_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDSVC_WLAN_TXPOST_PARAMS_TABLE", 1, CORE_3_INDEX, &SPDSVC_WLAN_TXPOST_PARAMS_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_L2_HEADER", 1, CORE_3_INDEX, &DHD_L2_HEADER_3, 72, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_CFG", 1, CORE_3_INDEX, &NAT_CACHE_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_NUM_OF_RX_FLOWS", 1, CORE_3_INDEX, &SPDTEST_NUM_OF_RX_FLOWS_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_TABLE", 1, CORE_3_INDEX, &DEBUG_PRINT_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TIMER_STACK", 1, CORE_3_INDEX, &DHD_TIMER_STACK_3, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING3_STACK", 1, CORE_3_INDEX, &PROCESSING3_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "REGISTERS_BUFFER", 1, CORE_3_INDEX, &REGISTERS_BUFFER_3, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_KEY0_MASK", 1, CORE_3_INDEX, &NAT_CACHE_KEY0_MASK_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_3_INDEX, &LOOPBACK_WAN_FLOW_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FORCE_DSCP", 1, CORE_3_INDEX, &FORCE_DSCP_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CORE_ID_TABLE", 1, CORE_3_INDEX, &CORE_ID_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LLQ_SELECTOR_ECN_MASK", 1, CORE_3_INDEX, &LLQ_SELECTOR_ECN_MASK_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GDX_PARAMS_TABLE", 1, CORE_3_INDEX, &GDX_PARAMS_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_3_INDEX, &RX_MIRRORING_CONFIGURATION_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_3_INDEX, &TX_MIRRORING_CONFIGURATION_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING4_STACK", 1, CORE_3_INDEX, &PROCESSING4_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_VLAN_KEY_MASK", 1, CORE_3_INDEX, &NATC_L2_VLAN_KEY_MASK_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REDIRECT_MODE", 1, CORE_3_INDEX, &CPU_REDIRECT_MODE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_DISABLE", 1, CORE_3_INDEX, &CSO_DISABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_3_INDEX, &DEBUG_PRINT_CORE_LOCK_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_TABLE_CFG_TABLE", 1, CORE_3_INDEX, &TCAM_TABLE_CFG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SRAM_DUMMY_STORE", 1, CORE_3_INDEX, &SRAM_DUMMY_STORE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_3_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_CFG", 1, CORE_3_INDEX, &INGRESS_FILTER_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_3_INDEX, &INGRESS_FILTER_1588_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_3_INDEX, &RX_MIRRORING_DIRECT_ENABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_TOS_MASK", 1, CORE_3_INDEX, &NATC_L2_TOS_MASK_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_TX_POST_RING_DESCRIPTOR_TABLE", 1, CORE_3_INDEX, &DHD_CPU_TX_POST_RING_DESCRIPTOR_TABLE_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_3_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING5_STACK", 1, CORE_3_INDEX, &PROCESSING5_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_CPU_RECYCLE_RING_DESCRIPTOR_TABLE", 1, CORE_3_INDEX, &DHD_CPU_RECYCLE_RING_DESCRIPTOR_TABLE_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_3_INDEX, &LOOPBACK_QUEUE_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING6_STACK", 1, CORE_3_INDEX, &PROCESSING6_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_EX_TABLE", 1, CORE_3_INDEX, &VPORT_CFG_EX_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_POST_UPDATE_FIFO_TABLE", 1, CORE_3_INDEX, &DHD_TX_POST_UPDATE_FIFO_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_POST_UPDATE_FIFO_STACK", 1, CORE_3_INDEX, &DHD_TX_POST_UPDATE_FIFO_STACK_3, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING7_STACK", 1, CORE_3_INDEX, &PROCESSING7_STACK_3, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_SCRATCHPAD", 1, CORE_3_INDEX, &DEBUG_SCRATCHPAD_3, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_HW_CFG", 1, CORE_3_INDEX, &DHD_HW_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_3_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_GLOBAL_CFG", 1, CORE_3_INDEX, &FPM_GLOBAL_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_POST_FLOW_RING_BUFFER", 1, CORE_3_INDEX, &DHD_TX_POST_FLOW_RING_BUFFER_3, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_3_INDEX, &FW_ERROR_VECTOR_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_3_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_3, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "MULTICAST_KEY_MASK", 1, CORE_3_INDEX, &MULTICAST_KEY_MASK_3, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_POST_0_STACK", 1, CORE_3_INDEX, &DHD_TX_POST_0_STACK_3, 144, 1, 1 },
#endif
#if defined BCM6855
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_3_INDEX, &BITS_CALC_MASKS_TABLE_3, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_POST_1_STACK", 1, CORE_3_INDEX, &DHD_TX_POST_1_STACK_3, 144, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_3_INDEX, &INGRESS_FILTER_PROFILE_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_TX_POST_PD_FIFO_TABLE", 1, CORE_3_INDEX, &DHD_TX_POST_PD_FIFO_TABLE_3, 6, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FLOW_RING_CACHE_LKP_TABLE", 1, CORE_3_INDEX, &DHD_FLOW_RING_CACHE_LKP_TABLE_3, 48, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_PD_FIFO_TABLE", 1, CORE_4_INDEX, &DS_TM_PD_FIFO_TABLE_4, 144, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_TASK_0_STACK", 1, CORE_4_INDEX, &TX_TASK_0_STACK_4, 256, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING0_STACK", 1, CORE_4_INDEX, &PROCESSING0_STACK_4, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_BBH_TX_WAKE_UP_DATA_TABLE", 1, CORE_4_INDEX, &DS_TM_BBH_TX_WAKE_UP_DATA_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_RATE_LIMITER_BUDGET_VALID", 1, CORE_4_INDEX, &ETH_TM_RATE_LIMITER_BUDGET_VALID_4, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_RATE_LIMITER_PROFILE_TABLE", 1, CORE_4_INDEX, &ETH_TM_RATE_LIMITER_PROFILE_TABLE_4, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_SCHEDULING_QUEUE_TABLE", 1, CORE_4_INDEX, &ETH_TM_SCHEDULING_QUEUE_TABLE_4, 88, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_GEN_PARAM", 1, CORE_4_INDEX, &SPDTEST_GEN_PARAM_4, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER", 1, CORE_4_INDEX, &GENERAL_TIMER_4, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING_4_TASKS_PACKET_BUFFER", 1, CORE_4_INDEX, &PROCESSING_4_TASKS_PACKET_BUFFER_4, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "REPORTING_QUEUE_DESCRIPTOR_TABLE", 1, CORE_4_INDEX, &REPORTING_QUEUE_DESCRIPTOR_TABLE_4, 129, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_SCHEDULER_POOL", 1, CORE_4_INDEX, &ETH_TM_SCHEDULER_POOL_4, 186, 1, 1 },
#endif
#if defined BCM6855
	{ "BUDGET_ALLOCATION_TIMER_VALUE", 1, CORE_4_INDEX, &BUDGET_ALLOCATION_TIMER_VALUE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MUTEX_TABLE", 1, CORE_4_INDEX, &MUTEX_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SYSTEM_CONFIGURATION", 1, CORE_4_INDEX, &SYSTEM_CONFIGURATION_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_FLUSH_CFG_CPU_TABLE", 1, CORE_4_INDEX, &DS_TM_FLUSH_CFG_CPU_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE", 1, CORE_4_INDEX, &ETH_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_4, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "REPORTING_QUEUE_ACCUMULATED_TABLE", 1, CORE_4_INDEX, &REPORTING_QUEUE_ACCUMULATED_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING1_STACK", 1, CORE_4_INDEX, &PROCESSING1_STACK_4, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "MIRRORING_SCRATCH", 1, CORE_4_INDEX, &MIRRORING_SCRATCH_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_FLUSH_CFG_FW_TABLE", 1, CORE_4_INDEX, &DS_TM_FLUSH_CFG_FW_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_AQM_QUEUE_TIMER_TABLE", 1, CORE_4_INDEX, &ETH_TM_AQM_QUEUE_TIMER_TABLE_4, 88, 1, 1 },
#endif
#if defined BCM6855
	{ "REPORT_BBH_TX_QUEUE_ID_TABLE", 1, CORE_4_INDEX, &REPORT_BBH_TX_QUEUE_ID_TABLE_4, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_4_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE", 1, CORE_4_INDEX, &ETH_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_4, 88, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_TM_FLOW_CNTR_TABLE", 1, CORE_4_INDEX, &DS_TM_TM_FLOW_CNTR_TABLE_4, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_FLOW_TABLE", 1, CORE_4_INDEX, &TX_FLOW_TABLE_4, 212, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_HOST_CNT_DISABLE_TABLE", 1, CORE_4_INDEX, &BUFMNG_HOST_CNT_DISABLE_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_GENERIC_FIELDS", 1, CORE_4_INDEX, &TCAM_GENERIC_FIELDS_4, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_4_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_4, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE", 1, CORE_4_INDEX, &ETH_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_4, 88, 1, 1 },
#endif
#if defined BCM6855
	{ "UPDATE_FIFO_STACK", 1, CORE_4_INDEX, &UPDATE_FIFO_STACK_4, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_SCHEDULER_TABLE", 1, CORE_4_INDEX, &ETH_TM_SCHEDULER_TABLE_4, 7, 1, 1 },
#endif
#if defined BCM6855
	{ "TUNNELS_PARSING_CFG", 1, CORE_4_INDEX, &TUNNELS_PARSING_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_FLUSH_CFG_CURRENT_TABLE", 1, CORE_4_INDEX, &DS_TM_FLUSH_CFG_CURRENT_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFFER_CONG_SCRATCHPAD", 1, CORE_4_INDEX, &BUFFER_CONG_SCRATCHPAD_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_FLOW_TABLE", 1, CORE_4_INDEX, &RX_FLOW_TABLE_4, 340, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_BAD_IPV4_HDR_CSUM_PACKETS", 1, CORE_4_INDEX, &CSO_BAD_IPV4_HDR_CSUM_PACKETS_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GHOST_REPORTING_GLOBAL_CFG", 1, CORE_4_INDEX, &GHOST_REPORTING_GLOBAL_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BRIDGE_CFG_TABLE", 1, CORE_4_INDEX, &BRIDGE_CFG_TABLE_4, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR", 1, CORE_4_INDEX, &DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TASK_IDX", 1, CORE_4_INDEX, &TASK_IDX_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CFG_TABLE", 1, CORE_4_INDEX, &IPTV_CFG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "POLICER_PARAMS_TABLE", 1, CORE_4_INDEX, &POLICER_PARAMS_TABLE_4, 80, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_DESCRIPTOR_TABLE", 1, CORE_4_INDEX, &BUFMNG_DESCRIPTOR_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "TR471_SPDSVC_RX_PKT_ID_TABLE", 1, CORE_4_INDEX, &TR471_SPDSVC_RX_PKT_ID_TABLE_4, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_4_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_4_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_4, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING2_STACK", 1, CORE_4_INDEX, &PROCESSING2_STACK_4, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_4_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_FLUSH_DISPATCHER_CREDIT_TABLE", 1, CORE_4_INDEX, &DS_TM_FLUSH_DISPATCHER_CREDIT_TABLE_4, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DOS_DROP_REASONS_CFG", 1, CORE_4_INDEX, &DOS_DROP_REASONS_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_ACTION_VEC", 1, CORE_4_INDEX, &GENERAL_TIMER_ACTION_VEC_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_SECONDARY_SCHEDULER_TABLE", 1, CORE_4_INDEX, &ETH_TM_SECONDARY_SCHEDULER_TABLE_4, 7, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_FLUSH_CFG_ENABLE_TABLE", 1, CORE_4_INDEX, &DS_TM_FLUSH_CFG_ENABLE_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MCAST_MAX_REPLICATION_FLAG_TABLE", 1, CORE_4_INDEX, &MCAST_MAX_REPLICATION_FLAG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BBH_TX_EGRESS_REPORT_COUNTER_TABLE", 1, CORE_4_INDEX, &BBH_TX_EGRESS_REPORT_COUNTER_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_STATUS_TABLE", 1, CORE_4_INDEX, &BUFMNG_STATUS_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PROCESSING3_STACK", 1, CORE_4_INDEX, &PROCESSING3_STACK_4, 360, 1, 1 },
#endif
#if defined BCM6855
	{ "BBH_TX_INGRESS_COUNTER_TABLE", 1, CORE_4_INDEX, &BBH_TX_INGRESS_COUNTER_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_4_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_NUM_QUEUES", 1, CORE_4_INDEX, &AQM_NUM_QUEUES_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_BB_DESTINATION_TABLE", 1, CORE_4_INDEX, &DS_TM_BB_DESTINATION_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_TABLE", 1, CORE_4_INDEX, &VPORT_CFG_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "DHD_FPM_THRESHOLDS", 1, CORE_4_INDEX, &DHD_FPM_THRESHOLDS_4, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "SPDTEST_NUM_OF_RX_FLOWS", 1, CORE_4_INDEX, &SPDTEST_NUM_OF_RX_FLOWS_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_4_INDEX, &LOOPBACK_WAN_FLOW_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_FIRST_QUEUE_MAPPING", 1, CORE_4_INDEX, &DS_TM_FIRST_QUEUE_MAPPING_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "AQM_ENABLE_TABLE", 1, CORE_4_INDEX, &AQM_ENABLE_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_AQM_QUEUE_TABLE", 1, CORE_4_INDEX, &ETH_TM_AQM_QUEUE_TABLE_4, 88, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_TX_QUEUE_DROP_TABLE", 1, CORE_4_INDEX, &DS_TM_TX_QUEUE_DROP_TABLE_4, 65, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_CFG", 1, CORE_4_INDEX, &NAT_CACHE_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "FORCE_DSCP", 1, CORE_4_INDEX, &FORCE_DSCP_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_DISPATCHER_CREDIT_TABLE", 1, CORE_4_INDEX, &TX_MIRRORING_DISPATCHER_CREDIT_TABLE_4, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "CORE_ID_TABLE", 1, CORE_4_INDEX, &CORE_ID_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "LLQ_SELECTOR_ECN_MASK", 1, CORE_4_INDEX, &LLQ_SELECTOR_ECN_MASK_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SCHEDULING_FLUSH_GLOBAL_CFG", 1, CORE_4_INDEX, &SCHEDULING_FLUSH_GLOBAL_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_EXCEPTION", 1, CORE_4_INDEX, &TX_EXCEPTION_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_STACK", 1, CORE_4_INDEX, &GENERAL_TIMER_STACK_4, 72, 1, 1 },
#endif
#if defined BCM6855
	{ "NAT_CACHE_KEY0_MASK", 1, CORE_4_INDEX, &NAT_CACHE_KEY0_MASK_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_4_INDEX, &RX_MIRRORING_CONFIGURATION_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_4_INDEX, &TX_MIRRORING_CONFIGURATION_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_TABLE", 1, CORE_4_INDEX, &DEBUG_PRINT_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "REPORTING_STACK", 1, CORE_4_INDEX, &REPORTING_STACK_4, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "REPORTING_QUEUE_COUNTER_TABLE", 1, CORE_4_INDEX, &REPORTING_QUEUE_COUNTER_TABLE_4, 129, 1, 1 },
#endif
#if defined BCM6855
	{ "CPU_REDIRECT_MODE", 1, CORE_4_INDEX, &CPU_REDIRECT_MODE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CSO_DISABLE", 1, CORE_4_INDEX, &CSO_DISABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_4_INDEX, &DEBUG_PRINT_CORE_LOCK_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TCAM_TABLE_CFG_TABLE", 1, CORE_4_INDEX, &TCAM_TABLE_CFG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SRAM_DUMMY_STORE", 1, CORE_4_INDEX, &SRAM_DUMMY_STORE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BACKUP_BBH_INGRESS_COUNTERS_TABLE", 1, CORE_4_INDEX, &BACKUP_BBH_INGRESS_COUNTERS_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_VLAN_KEY_MASK", 1, CORE_4_INDEX, &NATC_L2_VLAN_KEY_MASK_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BACKUP_BBH_EGRESS_COUNTERS_TABLE", 1, CORE_4_INDEX, &BACKUP_BBH_EGRESS_COUNTERS_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_4_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_4_INDEX, &INGRESS_FILTER_1588_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_4_INDEX, &RX_MIRRORING_DIRECT_ENABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GDX_PARAMS_TABLE", 1, CORE_4_INDEX, &GDX_PARAMS_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFFER_CONG_DQM_NOT_EMPTY", 1, CORE_4_INDEX, &BUFFER_CONG_DQM_NOT_EMPTY_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "CODEL_BIAS_SLOPE_TABLE", 1, CORE_4_INDEX, &CODEL_BIAS_SLOPE_TABLE_4, 11, 1, 1 },
#endif
#if defined BCM6855
	{ "XGPON_REPORT_ZERO_SENT_TABLE", 1, CORE_4_INDEX, &XGPON_REPORT_ZERO_SENT_TABLE_4, 10, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_BUFFER_CONG_MGT_CFG", 1, CORE_4_INDEX, &DS_BUFFER_CONG_MGT_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_SCRATCHPAD", 1, CORE_4_INDEX, &DEBUG_SCRATCHPAD_4, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_CFG", 1, CORE_4_INDEX, &INGRESS_FILTER_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "REPORTING_COUNTER_TABLE", 1, CORE_4_INDEX, &REPORTING_COUNTER_TABLE_4, 40, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_CODEL_DROP_DESCRIPTOR", 1, CORE_4_INDEX, &DS_TM_CODEL_DROP_DESCRIPTOR_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "NATC_L2_TOS_MASK", 1, CORE_4_INDEX, &NATC_L2_TOS_MASK_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_TABLE", 1, CORE_4_INDEX, &RX_MIRRORING_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_4_INDEX, &INGRESS_FILTER_PROFILE_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "REGISTERS_BUFFER", 1, CORE_4_INDEX, &REGISTERS_BUFFER_4, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_BBH_QUEUE_TABLE", 1, CORE_4_INDEX, &DS_TM_BBH_QUEUE_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_CPU_TX_ABS_COUNTERS", 1, CORE_4_INDEX, &DS_TM_CPU_TX_ABS_COUNTERS_4, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_TABLE", 1, CORE_4_INDEX, &DS_TM_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_TABLE_4, 2, 1, 1 },
#endif
#if defined BCM6855
	{ "GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE", 1, CORE_4_INDEX, &GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_4, 5, 1, 1 },
#endif
#if defined BCM6855
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_4_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "ETH_TM_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_4_INDEX, &ETH_TM_TX_TRUNCATE_MIRRORING_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_4_INDEX, &LOOPBACK_QUEUE_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_EX_TABLE", 1, CORE_4_INDEX, &VPORT_CFG_EX_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "UPDATE_FIFO_TABLE", 1, CORE_4_INDEX, &UPDATE_FIFO_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "QUEUE_TO_REPORT_BIT_VECTOR", 1, CORE_4_INDEX, &QUEUE_TO_REPORT_BIT_VECTOR_4, 5, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_4_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_GLOBAL_CFG", 1, CORE_4_INDEX, &FPM_GLOBAL_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_4_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_4, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_4_INDEX, &FW_ERROR_VECTOR_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_4_INDEX, &BITS_CALC_MASKS_TABLE_4, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "MULTICAST_KEY_MASK", 1, CORE_4_INDEX, &MULTICAST_KEY_MASK_4, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DS_TM_BBH_TX_EGRESS_COUNTER_TABLE", 1, CORE_4_INDEX, &DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_PD_FIFO_TABLE", 1, CORE_5_INDEX, &US_TM_PD_FIFO_TABLE_5, 264, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_RATE_LIMITER_PROFILE_TABLE", 1, CORE_5_INDEX, &PON_TM_RATE_LIMITER_PROFILE_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "WAN_STACK", 1, CORE_5_INDEX, &WAN_STACK_5, 256, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_SECONDARY_SCHEDULER_TABLE", 1, CORE_5_INDEX, &PON_TM_SECONDARY_SCHEDULER_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_TM_FLOW_CNTR_TABLE", 1, CORE_5_INDEX, &US_TM_TM_FLOW_CNTR_TABLE_5, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_SCHEDULER_TABLE", 1, CORE_5_INDEX, &PON_TM_SCHEDULER_TABLE_5, 33, 1, 1 },
#endif
#if defined BCM6855
	{ "SYSTEM_CONFIGURATION", 1, CORE_5_INDEX, &SYSTEM_CONFIGURATION_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE", 1, CORE_5_INDEX, &PON_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "UPDATE_FIFO_STACK", 1, CORE_5_INDEX, &UPDATE_FIFO_STACK_5, 64, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER", 1, CORE_5_INDEX, &GENERAL_TIMER_5, 16, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_SCHEDULING_QUEUE_TABLE", 1, CORE_5_INDEX, &PON_TM_SCHEDULING_QUEUE_TABLE_5, 132, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE", 1, CORE_5_INDEX, &US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_RATE_LIMITER_BUDGET_VALID", 1, CORE_5_INDEX, &PON_TM_RATE_LIMITER_BUDGET_VALID_5, 4, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFFER_CONG_SCRATCHPAD", 1, CORE_5_INDEX, &BUFFER_CONG_SCRATCHPAD_5, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_STACK", 1, CORE_5_INDEX, &DIRECT_FLOW_STACK_5, 128, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_BBH_QUEUE_TABLE", 1, CORE_5_INDEX, &US_TM_BBH_QUEUE_TABLE_5, 40, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR", 1, CORE_5_INDEX, &US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MUTEX_TABLE", 1, CORE_5_INDEX, &MUTEX_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BBH_TX_EPON_WAKE_UP_DATA_TABLE", 1, CORE_5_INDEX, &BBH_TX_EPON_WAKE_UP_DATA_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_TABLE", 1, CORE_5_INDEX, &VPORT_CFG_TABLE_5, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_FLUSH_CFG_CPU_TABLE", 1, CORE_5_INDEX, &US_TM_FLUSH_CFG_CPU_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_5_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_5, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE", 1, CORE_5_INDEX, &PON_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_5, 132, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_5_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "CODEL_BIAS_SLOPE_TABLE", 1, CORE_5_INDEX, &CODEL_BIAS_SLOPE_TABLE_5, 11, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFMNG_HOST_CNT_DISABLE_TABLE", 1, CORE_5_INDEX, &BUFMNG_HOST_CNT_DISABLE_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_FLUSH_CFG_FW_TABLE", 1, CORE_5_INDEX, &US_TM_FLUSH_CFG_FW_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_STACK", 1, CORE_5_INDEX, &GENERAL_TIMER_STACK_5, 72, 1, 1 },
#endif
#if defined BCM6855
	{ "MIRRORING_SCRATCH", 1, CORE_5_INDEX, &MIRRORING_SCRATCH_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_FLUSH_CFG_CURRENT_TABLE", 1, CORE_5_INDEX, &US_TM_FLUSH_CFG_CURRENT_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "EPON_UPDATE_FIFO_STACK", 1, CORE_5_INDEX, &EPON_UPDATE_FIFO_STACK_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "WAN_EPON_STACK", 1, CORE_5_INDEX, &WAN_EPON_STACK_5, 256, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_SCHEDULER_POOL", 1, CORE_5_INDEX, &PON_TM_SCHEDULER_POOL_5, 312, 1, 1 },
#endif
#if defined BCM6855
	{ "TASK_IDX", 1, CORE_5_INDEX, &TASK_IDX_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_5_INDEX, &PON_TM_TX_TRUNCATE_MIRRORING_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BUDGET_ALLOCATION_TIMER_VALUE", 1, CORE_5_INDEX, &BUDGET_ALLOCATION_TIMER_VALUE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE", 1, CORE_5_INDEX, &US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_5, 40, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_TIMER_ACTION_VEC", 1, CORE_5_INDEX, &GENERAL_TIMER_ACTION_VEC_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_FLUSH_CFG_ENABLE_TABLE", 1, CORE_5_INDEX, &US_TM_FLUSH_CFG_ENABLE_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_PAUSE_QUANTA", 1, CORE_5_INDEX, &DIRECT_FLOW_PAUSE_QUANTA_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_WAN_0_BB_DESTINATION_TABLE", 1, CORE_5_INDEX, &US_TM_WAN_0_BB_DESTINATION_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_PD_TABLE", 1, CORE_5_INDEX, &DIRECT_FLOW_PD_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_BUFFER_CONG_MGT_CFG", 1, CORE_5_INDEX, &US_BUFFER_CONG_MGT_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_BBH_TX_WAN_0_FIFO_BYTES_USED", 1, CORE_5_INDEX, &US_TM_BBH_TX_WAN_0_FIFO_BYTES_USED_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "QEMU_SYNC_MEM", 1, CORE_5_INDEX, &QEMU_SYNC_MEM_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_WAN_VIQ_EXCLUSIVE", 1, CORE_5_INDEX, &DIRECT_FLOW_WAN_VIQ_EXCLUSIVE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_5_INDEX, &PON_TM_VPORT_TO_RL_OVERHEAD_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "CORE_ID_TABLE", 1, CORE_5_INDEX, &CORE_ID_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_FIRST_QUEUE_MAPPING", 1, CORE_5_INDEX, &US_TM_FIRST_QUEUE_MAPPING_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_TX_PAUSE_NACK", 1, CORE_5_INDEX, &US_TM_TX_PAUSE_NACK_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SCHEDULING_FLUSH_GLOBAL_CFG", 1, CORE_5_INDEX, &SCHEDULING_FLUSH_GLOBAL_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_DISPATCHER_CREDIT_TABLE", 1, CORE_5_INDEX, &PON_TM_DISPATCHER_CREDIT_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "BBH_TX_US_WAN_0_FIFO_BYTES_THRESHOLD", 1, CORE_5_INDEX, &BBH_TX_US_WAN_0_FIFO_BYTES_THRESHOLD_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_EXCEPTION", 1, CORE_5_INDEX, &TX_EXCEPTION_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_5_INDEX, &DEBUG_PRINT_CORE_LOCK_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_CNTR_TABLE", 1, CORE_5_INDEX, &DIRECT_FLOW_CNTR_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE", 1, CORE_5_INDEX, &PON_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_5, 132, 1, 1 },
#endif
#if defined BCM6855
	{ "REGISTERS_BUFFER", 1, CORE_5_INDEX, &REGISTERS_BUFFER_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "BUFFER_CONG_DQM_NOT_EMPTY", 1, CORE_5_INDEX, &BUFFER_CONG_DQM_NOT_EMPTY_5, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_5_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_5, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_5_INDEX, &RX_MIRRORING_CONFIGURATION_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_5_INDEX, &TX_MIRRORING_CONFIGURATION_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "SRAM_DUMMY_STORE", 1, CORE_5_INDEX, &SRAM_DUMMY_STORE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "MAC_TYPE", 1, CORE_5_INDEX, &MAC_TYPE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "WAN_0_BBH_TX_FIFO_SIZE", 1, CORE_5_INDEX, &WAN_0_BBH_TX_FIFO_SIZE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_5_INDEX, &RX_MIRRORING_DIRECT_ENABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_FLUSH_DISPATCHER_CREDIT_TABLE", 1, CORE_5_INDEX, &US_TM_FLUSH_DISPATCHER_CREDIT_TABLE_5, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_RX_MIRRORING_SCRATCHPAD", 1, CORE_5_INDEX, &DIRECT_FLOW_RX_MIRRORING_SCRATCHPAD_5, 136, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_SCRATCHPAD", 1, CORE_5_INDEX, &DEBUG_SCRATCHPAD_5, 12, 1, 1 },
#endif
#if defined BCM6855
	{ "VPORT_CFG_EX_TABLE", 1, CORE_5_INDEX, &VPORT_CFG_EX_TABLE_5, 20, 1, 1 },
#endif
#if defined BCM6855
	{ "DISPATCHER_CREDIT_TABLE", 1, CORE_5_INDEX, &DISPATCHER_CREDIT_TABLE_5, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_AQM_QUEUE_TIMER_TABLE", 1, CORE_5_INDEX, &PON_TM_AQM_QUEUE_TIMER_TABLE_5, 132, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_CPU_TX_ABS_COUNTERS", 1, CORE_5_INDEX, &US_TM_CPU_TX_ABS_COUNTERS_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_5_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_EPON_CONTROL_SCRATCH", 1, CORE_5_INDEX, &DIRECT_FLOW_EPON_CONTROL_SCRATCH_5, 22, 1, 1 },
#endif
#if defined BCM6855
	{ "BBH_TX_EPON_INGRESS_COUNTER_TABLE", 1, CORE_5_INDEX, &BBH_TX_EPON_INGRESS_COUNTER_TABLE_5, 40, 1, 1 },
#endif
#if defined BCM6855
	{ "DIRECT_FLOW_PAUSE_DEBUG", 1, CORE_5_INDEX, &DIRECT_FLOW_PAUSE_DEBUG_5, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "BBH_TX_EPON_EGRESS_COUNTER_TABLE", 1, CORE_5_INDEX, &BBH_TX_EPON_EGRESS_COUNTER_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_CODEL_DROP_DESCRIPTOR", 1, CORE_5_INDEX, &US_TM_CODEL_DROP_DESCRIPTOR_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "UPDATE_FIFO_TABLE", 1, CORE_5_INDEX, &UPDATE_FIFO_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "FPM_GLOBAL_CFG", 1, CORE_5_INDEX, &FPM_GLOBAL_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "PON_TM_AQM_QUEUE_TABLE", 1, CORE_5_INDEX, &PON_TM_AQM_QUEUE_TABLE_5, 132, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_TX_QUEUE_DROP_TABLE", 1, CORE_5_INDEX, &US_TM_TX_QUEUE_DROP_TABLE_5, 129, 1, 1 },
#endif
#if defined BCM6855
	{ "TX_MIRRORING_DISPATCHER_CREDIT_TABLE", 1, CORE_5_INDEX, &TX_MIRRORING_DISPATCHER_CREDIT_TABLE_5, 3, 1, 1 },
#endif
#if defined BCM6855
	{ "DEBUG_PRINT_TABLE", 1, CORE_5_INDEX, &DEBUG_PRINT_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "EPON_UPDATE_FIFO_TABLE", 1, CORE_5_INDEX, &EPON_UPDATE_FIFO_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_TX_OCTETS_COUNTERS_TABLE", 1, CORE_5_INDEX, &US_TM_TX_OCTETS_COUNTERS_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6855
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_5_INDEX, &FW_ERROR_VECTOR_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "US_TM_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_TABLE", 1, CORE_5_INDEX, &US_TM_BBH_QUEUE_TO_BBH_QUEUE_DESC_MAPPING_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6855
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_5_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_5, 128, 1, 1 },
#endif
};

TABLE_STACK_STRUCT RUNNER_STACK_TABLES[NUMBER_OF_STACK_TABLES] =
{
    { "PROCESSING0_STACK", CORE_0_INDEX, 0xe00, 360},
    { "CPU_RX_COPY_STACK", CORE_0_INDEX, 0xf80, 128},
    { "PROCESSING1_STACK", CORE_0_INDEX, 0x2000, 360},
    { "PROCESSING2_STACK", CORE_0_INDEX, 0x2200, 360},
    { "PROCESSING3_STACK", CORE_0_INDEX, 0x2400, 360},
    { "PROCESSING4_STACK", CORE_0_INDEX, 0x2600, 360},
    { "PROCESSING5_STACK", CORE_0_INDEX, 0x2800, 360},
    { "CPU_RX_STACK", CORE_0_INDEX, 0x2980, 80},
    { "PROCESSING6_STACK", CORE_0_INDEX, 0x2a00, 360},
    { "PROCESSING7_STACK", CORE_0_INDEX, 0x2c00, 360},
    { "GENERAL_TIMER_STACK", CORE_0_INDEX, 0x2d80, 72},
    { "DHD_RX_COMPLETE_0_STACK", CORE_0_INDEX, 0x32c0, 64},
    { "CPU_RECYCLE_STACK", CORE_0_INDEX, 0x33a0, 32},
    { "DHD_RX_COMPLETE_1_STACK", CORE_0_INDEX, 0x33c0, 64},
    { "DHD_TX_COMPLETE_0_STACK", CORE_0_INDEX, 0x34c0, 64},
    { "DHD_TX_COMPLETE_1_STACK", CORE_0_INDEX, 0x3580, 64},
    { "PROCESSING0_STACK", CORE_1_INDEX, 0x600, 360},
    { "COMMON_REPROCESSING_STACK", CORE_1_INDEX, 0xac0, 64},
    { "SPDSVC_ANALYZER_STACK", CORE_1_INDEX, 0xb00, 256},
    { "SPDSVC_GEN_STACK", CORE_1_INDEX, 0xe80, 128},
    { "TCPSPDTEST_DOWNLOAD_STACK", CORE_1_INDEX, 0xf00, 256},
    { "PROCESSING1_STACK", CORE_1_INDEX, 0x2200, 360},
    { "PROCESSING2_STACK", CORE_1_INDEX, 0x2400, 360},
    { "PROCESSING3_STACK", CORE_1_INDEX, 0x2600, 360},
    { "PROCESSING4_STACK", CORE_1_INDEX, 0x2800, 360},
    { "PROCESSING5_STACK", CORE_1_INDEX, 0x2a00, 360},
    { "PROCESSING6_STACK", CORE_1_INDEX, 0x2c00, 360},
    { "PROCESSING7_STACK", CORE_1_INDEX, 0x2e00, 360},
    { "PROCESSING0_STACK", CORE_2_INDEX, 0xe00, 360},
    { "PROCESSING1_STACK", CORE_2_INDEX, 0x2000, 360},
    { "PROCESSING2_STACK", CORE_2_INDEX, 0x2200, 360},
    { "PROCESSING3_STACK", CORE_2_INDEX, 0x2400, 360},
    { "PROCESSING4_STACK", CORE_2_INDEX, 0x2600, 360},
    { "GENERAL_TIMER_STACK", CORE_2_INDEX, 0x2780, 72},
    { "PROCESSING5_STACK", CORE_2_INDEX, 0x2800, 360},
    { "PROCESSING6_STACK", CORE_2_INDEX, 0x2a00, 360},
    { "PROCESSING7_STACK", CORE_2_INDEX, 0x2c00, 360},
    { "SERVICE_QUEUES_STACK", CORE_2_INDEX, 0x3100, 256},
    { "CPU_TX_0_STACK", CORE_2_INDEX, 0x3400, 192},
    { "CPU_TX_1_STACK", CORE_2_INDEX, 0x3500, 192},
    { "CPU_RECYCLE_STACK", CORE_2_INDEX, 0x37c0, 32},
    { "CPU_FPM_RINGS_AND_BUFMNG_REFILL_STACK", CORE_3_INDEX, 0xe80, 128},
    { "PROCESSING0_STACK", CORE_3_INDEX, 0x2200, 360},
    { "PROCESSING1_STACK", CORE_3_INDEX, 0x2400, 360},
    { "PROCESSING2_STACK", CORE_3_INDEX, 0x2600, 360},
    { "DHD_TIMER_STACK", CORE_3_INDEX, 0x27e0, 32},
    { "PROCESSING3_STACK", CORE_3_INDEX, 0x2800, 360},
    { "PROCESSING4_STACK", CORE_3_INDEX, 0x2a00, 360},
    { "PROCESSING5_STACK", CORE_3_INDEX, 0x2c00, 360},
    { "PROCESSING6_STACK", CORE_3_INDEX, 0x2e00, 360},
    { "DHD_TX_POST_UPDATE_FIFO_STACK", CORE_3_INDEX, 0x2fe0, 32},
    { "PROCESSING7_STACK", CORE_3_INDEX, 0x3000, 360},
    { "DHD_TX_POST_0_STACK", CORE_3_INDEX, 0x3300, 144},
    { "DHD_TX_POST_1_STACK", CORE_3_INDEX, 0x3400, 144},
    { "TX_TASK_0_STACK", CORE_4_INDEX, 0x900, 256},
    { "PROCESSING0_STACK", CORE_4_INDEX, 0xa00, 360},
    { "PROCESSING1_STACK", CORE_4_INDEX, 0x1e00, 360},
    { "UPDATE_FIFO_STACK", CORE_4_INDEX, 0x26c0, 64},
    { "PROCESSING2_STACK", CORE_4_INDEX, 0x2e00, 360},
    { "PROCESSING3_STACK", CORE_4_INDEX, 0x3000, 360},
    { "GENERAL_TIMER_STACK", CORE_4_INDEX, 0x3580, 72},
    { "REPORTING_STACK", CORE_4_INDEX, 0x35e0, 32},
    { "WAN_STACK", CORE_5_INDEX, 0x1100, 256},
    { "UPDATE_FIFO_STACK", CORE_5_INDEX, 0x1740, 64},
    { "DIRECT_FLOW_STACK", CORE_5_INDEX, 0x1d80, 128},
    { "GENERAL_TIMER_STACK", CORE_5_INDEX, 0x2480, 72},
    { "EPON_UPDATE_FIFO_STACK", CORE_5_INDEX, 0x24e0, 32},
    { "WAN_EPON_STACK", CORE_5_INDEX, 0x2500, 256}
};
