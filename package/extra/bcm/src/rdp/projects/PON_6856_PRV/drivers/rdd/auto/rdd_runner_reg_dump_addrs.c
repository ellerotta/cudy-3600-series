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
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_PD_FIFO_TABLE =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_TX_COUNTERS =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_TX_WAKE_UP_DATA_TABLE =
{
	8,
	{
		{ dump_RDD_BBH_TX_WAKE_UP_DATA_ENTRY, 0xd88 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_CPU_TABLE =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0xd90 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xda0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_TM_FLOW_CNTR_TABLE =
{
	1,
	{
		{ dump_RDD_TM_FLOW_CNTR_ENTRY, 0xdc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT COMPLEX_SCHEDULER_TABLE_DS =
{
	64,
	{
		{ dump_RDD_COMPLEX_SCHEDULER_DESCRIPTOR, 0xe00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR =
{
	58,
	{
		{ dump_RDD_PI2_PROBABILITY_CALC_DESCRIPTOR, 0xfc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0xffa },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GHOST_REPORTING_GLOBAL_CFG =
{
	4,
	{
		{ dump_RDD_GHOST_REPORTING_GLOBAL_CFG_ENTRY, 0xffc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BASIC_RATE_LIMITER_TABLE_DS =
{
	16,
	{
		{ dump_RDD_BASIC_RATE_LIMITER_DESCRIPTOR, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_CPU_TX_ABS_COUNTERS =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1480 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REPORTING_QUEUE_ACCUMULATED_TABLE =
{
	16,
	{
		{ dump_RDD_QM_QUEUE_COUNTER_DATA, 0x1500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_AQM_QUEUE_TABLE =
{
	4,
	{
		{ dump_RDD_AQM_QUEUE_DESCRIPTOR, 0x1600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1720 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x17a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CODEL_BIAS_SLOPE_TABLE =
{
	4,
	{
		{ dump_RDD_CODEL_BIAS_SLOPE, 0x17c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BUDGET_ALLOCATION_TIMER_VALUE =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x17ec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_BB_DESTINATION_TABLE =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x17ee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_FW_TABLE =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x17f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REPORTING_QUEUE_DESCRIPTOR_TABLE =
{
	8,
	{
		{ dump_RDD_REPORTING_QUEUE_DESCRIPTOR, 0x1800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1c08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x1c38 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_TABLE =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x1c40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_SCHEDULING_QUEUE_AGING_VECTOR =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1c60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1c74 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MIRRORING_SCRATCH =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x1c78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1c80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SCHEDULING_FLUSH_GLOBAL_CFG =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1c94 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TM_VLAN_STATS_ENABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1c95 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FIRST_QUEUE_MAPPING =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x1c96 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REPORT_BBH_TX_QUEUE_ID_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1c98 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_EXCEPTION =
{
	1,
	{
		{ dump_RDD_TX_EXCEPTION_ENTRY, 0x1cb4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cb5 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x1cb6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BBH_TX_EGRESS_REPORT_COUNTER_TABLE =
{
	8,
	{
		{ dump_RDD_BBH_TX_EGRESS_COUNTER_ENTRY, 0x1cb8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT QUEUE_TO_REPORT_BIT_VECTOR =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1cc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_AGGREGATION_TASK_DISABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cd4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cd5 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BACKUP_BBH_INGRESS_COUNTERS_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cd6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BACKUP_BBH_EGRESS_COUNTERS_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cd7 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BBH_TX_INGRESS_COUNTER_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cd8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x1ce0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_CURRENT_TABLE =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x1cf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REPORTING_COUNTER_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RATE_LIMITER_VALID_TABLE_DS =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1da0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_DISPATCHER_CREDIT_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1db0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1dbc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_CODEL_DROP_DESCRIPTOR =
{
	12,
	{
		{ dump_RDD_CODEL_DROP_DESCRIPTOR, 0x1dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x1dd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_DISPATCHER_CREDIT_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1de0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x1df0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REPORTING_QUEUE_COUNTER_TABLE =
{
	2,
	{
		{ dump_RDD_REPORTING_QUEUE_COUNTER, 0x1e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT XGPON_REPORT_ZERO_SENT_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1f08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT COMPLEX_SCHEDULER_WAKEUP_CMD_DS =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1f18 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x1f20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_FLUSH_CFG_ENABLE_TABLE =
{
	2,
	{
		{ dump_RDD_FLUSH_CFG_ENABLE_ENTRY, 0x1f38 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_TX_EGRESS_COUNTER_TABLE =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1f40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1f60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_SCHEDULING_QUEUE_TABLE =
{
	8,
	{
		{ dump_RDD_SCHEDULING_QUEUE_DESCRIPTOR, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_AQM_QUEUE_TIMER_TABLE =
{
	2,
	{
		{ dump_RDD_AQM_QUEUE_TIMER_DESCRIPTOR, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_TM_BBH_QUEUE_TABLE =
{
	4,
	{
		{ dump_RDD_BBH_QUEUE_DESCRIPTOR, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_PD_FIFO_TABLE_1 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_TM_FLOW_CNTR_TABLE_1 =
{
	1,
	{
		{ dump_RDD_TM_FLOW_CNTR_ENTRY, 0x1080 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_RX_MIRRORING_SCRATCHPAD_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_1 =
{
	8,
	{
		{ dump_RDD_BBH_TX_WAKE_UP_DATA_ENTRY, 0x1188 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_PD_TABLE_1 =
{
	16,
	{
		{ dump_RDD_PROCESSING_RX_DESCRIPTOR, 0x1190 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x11a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_1 =
{
	58,
	{
		{ dump_RDD_PI2_PROBABILITY_CALC_DESCRIPTOR, 0x11c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_PAUSE_QUANTA_1 =
{
	2,
	{
		{ dump_RDD_PAUSE_QUANTA_ENTRY, 0x11fa },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_BBH_TX_WAN_0_FIFO_BYTES_USED_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x11fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_AQM_QUEUE_TIMER_TABLE_1 =
{
	2,
	{
		{ dump_RDD_AQM_QUEUE_TIMER_DESCRIPTOR, 0x1200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_CPU_TX_ABS_COUNTERS_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1308 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BBH_TX_EPON_WAKE_UP_DATA_TABLE_1 =
{
	8,
	{
		{ dump_RDD_BBH_TX_WAKE_UP_DATA_ENTRY, 0x1388 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_CPU_TABLE_1 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x1390 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_CNTR_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x13a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CODEL_BIAS_SLOPE_TABLE_1 =
{
	4,
	{
		{ dump_RDD_CODEL_BIAS_SLOPE, 0x13c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BUDGET_ALLOCATION_TIMER_VALUE_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x13ec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_WAN_0_BB_DESTINATION_TABLE_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x13ee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_FW_TABLE_1 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x13f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT COMPLEX_SCHEDULER_TABLE_US_1 =
{
	64,
	{
		{ dump_RDD_COMPLEX_SCHEDULER_DESCRIPTOR, 0x1400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_SCHEDULING_QUEUE_TABLE_1 =
{
	8,
	{
		{ dump_RDD_SCHEDULING_QUEUE_DESCRIPTOR, 0x1800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1c20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x1ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1cc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_1 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x1ce8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_CURRENT_TABLE_1 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x1cf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_1 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x1d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1d48 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MIRRORING_SCRATCH_1 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x1d78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BBH_TX_EPON_INGRESS_COUNTER_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BASIC_SCHEDULER_WAKEUP_CMD_US_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1da8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1dac },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT OVERALL_RATE_LIMITER_TABLE_1 =
{
	16,
	{
		{ dump_RDD_OVERALL_RATE_LIMITER_DESCRIPTOR, 0x1db0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BBH_TX_EPON_EGRESS_COUNTER_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_EPON_CONTROL_SCRATCH_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1de0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_WAN_VIQ_EXCLUSIVE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1df6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_TX_PAUSE_NACK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1df7 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT COMPLEX_SCHEDULER_WAKEUP_CMD_US_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1df8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BBH_TX_US_WAN_0_FIFO_BYTES_THRESHOLD_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x1dfc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SCHEDULING_FLUSH_GLOBAL_CFG_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1dfe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TM_VLAN_STATS_ENABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1dff },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_BBH_QUEUE_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BBH_QUEUE_DESCRIPTOR, 0x1e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_SCHEDULING_QUEUE_AGING_VECTOR_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1ea0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FIRST_QUEUE_MAPPING_1 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x1eb4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PON_TM_VPORT_TO_RL_OVERHEAD_TABLE_1 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x1eb6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_EXCEPTION_1 =
{
	1,
	{
		{ dump_RDD_TX_EXCEPTION_ENTRY, 0x1eb7 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_CFG_ENABLE_TABLE_1 =
{
	2,
	{
		{ dump_RDD_FLUSH_CFG_ENABLE_ENTRY, 0x1eb8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1eba },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_AGGREGATION_TASK_DISABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1ebb },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x1ebc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_1 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x1ebe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_TABLE_1 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x1ec0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1ee0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1ef4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MAC_TYPE_1 =
{
	1,
	{
		{ dump_RDD_MAC_TYPE_ENTRY, 0x1ef5 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT WAN_0_BBH_TX_FIFO_SIZE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1ef6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1ef7 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_1 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x1ef8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EPON_UPDATE_FIFO_TABLE_1 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x1f00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_TX_OCTETS_COUNTERS_TABLE_1 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x1f20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RATE_LIMITER_VALID_TABLE_US_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1f60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_FLUSH_DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1f70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_CODEL_DROP_DESCRIPTOR_1 =
{
	12,
	{
		{ dump_RDD_CODEL_DROP_DESCRIPTOR, 0x1f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DIRECT_FLOW_PAUSE_DEBUG_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1f90 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1fa0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_1 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x1fb0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_DISPATCHER_CREDIT_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x1fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_1 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x1fd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_1 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x1fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BASIC_RATE_LIMITER_TABLE_US_1 =
{
	16,
	{
		{ dump_RDD_BASIC_RATE_LIMITER_DESCRIPTOR, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_TX_COUNTERS_1 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_1 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT US_TM_AQM_QUEUE_TABLE_1 =
{
	4,
	{
		{ dump_RDD_AQM_QUEUE_DESCRIPTOR, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BASIC_SCHEDULER_TABLE_US_1 =
{
	16,
	{
		{ dump_RDD_BASIC_SCHEDULER_DESCRIPTOR, 0x3400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_1 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x3800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_SCRATCHPAD_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_2 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_2 =
{
	8,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_GEM_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_QUEUE_TABLE_2 =
{
	8,
	{
		{ dump_RDD_TC_TO_QUEUE_8, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_CONTEXT_DDR_ADDR_2 =
{
	8,
	{
		{ dump_RDD_DEF_FLOW_CONTEXT_DDR_ADDR, 0xa08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_DSCP_TO_PBITS_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xa10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xa20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PORT_MAC_2 =
{
	8,
	{
		{ dump_RDD_MAC_ADDRESS, 0xa40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PROCESSING_GROUP_PARAMS_GEN4_TABLE_2 =
{
	16,
	{
		{ dump_RDD_PROCESSING_GROUP_PARAMS_GEN4_ENTRY, 0xa80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DSCP_TO_PBITS_MAP_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_QUEUE_TABLE_2 =
{
	8,
	{
		{ dump_RDD_PBIT_TO_QUEUE_8, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_FPM_TOKENS_READ_FLAG_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xe08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_2 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xe10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xe20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_REPLY_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xe40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_LOCK_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xe80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_TASKS_LIMIT_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xec0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_2 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0xf00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_DSCP_TO_PBITS_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xfc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_PACKET_BUFFER_2 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_2 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CMD_TABLE_2 =
{
	4,
	{
		{ dump_RDD_TCAM_IC_CMD, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_DST_IP_LKP_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x23c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_POLICER_CBS_2 =
{
	4,
	{
		{ dump_RDD_CBS_ENTRY, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT QUEUE_THRESHOLD_VECTOR_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2540 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2564 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2566 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TIMER_COMMON_CTR_REP_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2568 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BUFFER_ALLOC_REPLY_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_2 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_ACTION_GPE_HANDLER_PTR_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x25c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x25e2 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TIMER_COMMON_TIMER_VALUE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x25e4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_ENABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x25e6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_VECTOR_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x25e7 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x25e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x25f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SRAM_PD_FIFO_2 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_POLICER_BUDGET_2 =
{
	2,
	{
		{ dump_RDD_FW_POLICER_BUDGET_ENTRY, 0x2700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_POLICER_BUDGET_REMAINDER_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_2 =
{
	16,
	{
		{ dump_RDD_CPU_INTERRUPT_COALESCING_ENTRY, 0x27f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_2 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_2 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x28a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_TABLE_2 =
{
	16,
	{
		{ dump_RDD_TUNNEL_ENTRY, 0x28c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x28e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_2 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x2900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_COUNTERS_GENERIC_CONTEXT_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2980 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_2 =
{
	16,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_ENTRY, 0x2a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2a60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2ae0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_2 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2b00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_2 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2b48 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_2 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x2b50 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CFG_TABLE_2 =
{
	16,
	{
		{ dump_RDD_TCAM_IC_CFG, 0x2b60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_2 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x2be0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2bf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_EGRESS_DISPATCHER_CREDIT_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2c30 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2c3c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_BUDGET_2 =
{
	4,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_BUDGET_ENTRY, 0x2c40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_ID_DDR_ADDR_2 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2c58 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_RING_DESCRIPTOR_TABLE_2 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x2c60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TABLE_2 =
{
	2,
	{
		{ dump_RDD_DEFAULT_VLAN_VID_ENTRY, 0x2c80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_INGRESS_DISPATCHER_CREDIT_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2cd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cdc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cdd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_VLAN_STATS_ENABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cde },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cdf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CONFIGURATION_TABLE_2 =
{
	12,
	{
		{ dump_RDD_HW_IPTV_CONFIGURATION, 0x2ce0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2cec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_2 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2cee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_2 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x2cf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IC_MCAST_ENABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cfc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cfd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_DONT_DROP_RATIO_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cfe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_WAN_UNTAGGED_PRIORITY_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2cff },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RING_CPU_TX_DESCRIPTOR_DATA_TABLE_2 =
{
	16,
	{
		{ dump_RDD_RING_CPU_TX_DESCRIPTOR, 0x2d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_POLICER_VECTOR_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_2 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x2d2c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d2d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_2 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x2d2e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d2f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_TABLE_2 =
{
	9,
	{
		{ dump_RDD_PBIT_TO_DSCP_ENTRY, 0x2d30 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d39 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_DESCRIPTOR_TABLE_2 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x2d50 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_2 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x2d60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_2 =
{
	5,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x2d78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_SYNC_FIFO_TABLE_2 =
{
	8,
	{
		{ dump_RDD_CPU_TX_SYNC_FIFO_ENTRY, 0x2d80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE_2 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2d90 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_PD_SCRATCH_2 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2da0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_PACKET_BASED_MAPPING_2 =
{
	4,
	{
		{ dump_RDD_INGRESS_PACKET_BASED_MAPPING_ENTRY, 0x2da8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_2 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x2db0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_2 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x2dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_2 =
{
	8,
	{
		{ dump_RDD_CPU_RING_INTERRUPT_COUNTER_ENTRY, 0x2dd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_TX_RING_INDICES_VALUES_TABLE_2 =
{
	4,
	{
		{ dump_RDD_CPU_TX_RING_INDICES, 0x2dd8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_HEADER_PSRAM_BUFFER_2 =
{
	4,
	{
		{ dump_RDD_TUNNEL_HEADER_PSRAM_BUFFER, 0x2de0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_2 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2de8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_SCRATCH_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2df0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_2 =
{
	8,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2df8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_2 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_2 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2e08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_2 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x2e10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_BUDGET_REMAINDER_2 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e18 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_2 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x2e20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_RD_IDX_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2e28 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_WR_IDX_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2e30 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_2 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x2e38 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_NEXT_PTR_TABLE_2 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2e40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_3 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_3 =
{
	8,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x280 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_GEM_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_SCRATCHPAD_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CMD_TABLE_3 =
{
	4,
	{
		{ dump_RDD_TCAM_IC_CMD, 0x600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PORT_MAC_3 =
{
	8,
	{
		{ dump_RDD_MAC_ADDRESS, 0x7c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_QUEUE_TABLE_3 =
{
	8,
	{
		{ dump_RDD_TC_TO_QUEUE_8, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_CONTEXT_DDR_ADDR_3 =
{
	8,
	{
		{ dump_RDD_DEF_FLOW_CONTEXT_DDR_ADDR, 0xa08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_DSCP_TO_PBITS_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xa10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xa20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_REPLY_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PROCESSING_GROUP_PARAMS_GEN4_TABLE_3 =
{
	16,
	{
		{ dump_RDD_PROCESSING_GROUP_PARAMS_GEN4_ENTRY, 0xa80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DSCP_TO_PBITS_MAP_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_QUEUE_TABLE_3 =
{
	8,
	{
		{ dump_RDD_PBIT_TO_QUEUE_8, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_FPM_TOKENS_READ_FLAG_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xe08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_3 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xe10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xe20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_LOCK_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xe40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_CACHE_TABLE_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xe80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_METER_TABLE_3 =
{
	8,
	{
		{ dump_RDD_CPU_RX_METER_ENTRY, 0xf00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_PACKET_BUFFER_3 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_3 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SRAM_PD_FIFO_3 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_3 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_TASKS_LIMIT_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x23c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RING_INTERRUPT_COUNTER_TABLE_3 =
{
	8,
	{
		{ dump_RDD_CPU_RING_INTERRUPT_COUNTER_ENTRY, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_DSCP_TO_PBITS_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_3 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_3 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x25a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_DST_IP_LKP_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_PD_FIFO_TABLE_3 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CSO_CONTEXT_TABLE_3 =
{
	108,
	{
		{ dump_RDD_CSO_CONTEXT_ENTRY, 0x2680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x26ec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x26ee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x26f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_3 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x2700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_3 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2780 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REASON_TO_METER_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x27c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_3 =
{
	16,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_ENTRY, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_COUNTERS_GENERIC_CONTEXT_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2860 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x28e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_3 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2900 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2948 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_INTERRUPT_COALESCING_ENTRY, 0x2950 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_FLOW_IDX_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2960 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPV6_HOST_ADDRESS_CRC_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2980 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x29c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CFG_TABLE_3 =
{
	16,
	{
		{ dump_RDD_TCAM_IC_CFG, 0x2a40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REASON_AND_VPORT_TO_METER_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_DESCRIPTOR_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x2af0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_ACTION_GPE_HANDLER_PTR_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2b00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2b22 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_SCRATCH_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b24 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_3 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2b28 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_INT_SCRATCHPAD_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2b30 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_TABLE_3 =
{
	16,
	{
		{ dump_RDD_TUNNEL_ENTRY, 0x2b40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_RSV_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2b60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2b80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPV4_HOST_ADDRESS_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ba0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT UPDATE_FIFO_TABLE_3 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x2bc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2be0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_UPDATE_FIFO_TABLE_3 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_3 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x2c20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2c30 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PD_FIFO_TABLE_3 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x2c40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_LOCAL_SCRATCH_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2c70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TABLE_3 =
{
	2,
	{
		{ dump_RDD_DEFAULT_VLAN_VID_ENTRY, 0x2c80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REASON_TO_TC_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ca0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_CPU_RXQ_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ce0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EXC_TC_TO_CPU_RXQ_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2d20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_3 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x2d90 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2da0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2db0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2dbc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2dc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2dd0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_ENABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ddc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ddd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dde },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ddf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CONFIGURATION_TABLE_3 =
{
	12,
	{
		{ dump_RDD_HW_IPTV_CONFIGURATION, 0x2de0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2dec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_3 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2dee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_3 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x2df0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IC_MCAST_ENABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dfc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dfd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_DONT_DROP_RATIO_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dfe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_WAN_UNTAGGED_PRIORITY_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2dff },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_VPORT_TO_METER_TABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_TABLE_3 =
{
	9,
	{
		{ dump_RDD_PBIT_TO_DSCP_ENTRY, 0x2e10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_CACHE_OFFSET_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e19 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_3 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x2e1a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CSO_DISABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e1b },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e1c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_3 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x2e1d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e1e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e1f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_3 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x2e20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RX_INTERRUPT_ID_DDR_ADDR_3 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2e38 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_CPU_OBJ_3 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2e40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_DESCRIPTOR_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x2e50 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE_3 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2e60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_INTERRUPT_COALESCING_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_INTERRUPT_COALESCING_ENTRY, 0x2e70 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_PD_SCRATCH_3 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2e80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_3 =
{
	5,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x2e88 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_3 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x2e90 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_3 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x2ea0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_PACKET_BASED_MAPPING_3 =
{
	4,
	{
		{ dump_RDD_INGRESS_PACKET_BASED_MAPPING_ENTRY, 0x2eb0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_3 =
{
	8,
	{
		{ dump_RDD_CPU_RING_INTERRUPT_COUNTER_ENTRY, 0x2eb8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_HEADER_PSRAM_BUFFER_3 =
{
	4,
	{
		{ dump_RDD_TUNNEL_HEADER_PSRAM_BUFFER, 0x2ec0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_FEED_RING_INDEX_DDR_ADDR_TABLE_3 =
{
	8,
	{
		{ dump_RDD_DDR_ADDRESS, 0x2ec8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_INTERRUPT_SCRATCH_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ed0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_3 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2ed8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_3 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2ee0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_3 =
{
	8,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2ee8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_3 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2ef0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_3 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x2ef8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_3 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x2f00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_RD_IDX_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_SHADOW_WR_IDX_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_3 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x2f18 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RECYCLE_NEXT_PTR_TABLE_3 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_RING_DESCRIPTORS_TABLE_3 =
{
	16,
	{
		{ dump_RDD_CPU_RING_DESCRIPTOR, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_4 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_4 =
{
	8,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x280 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_GEM_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_QUEUE_TABLE_4 =
{
	8,
	{
		{ dump_RDD_TC_TO_QUEUE_8, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_CONTEXT_DDR_ADDR_4 =
{
	8,
	{
		{ dump_RDD_DEF_FLOW_CONTEXT_DDR_ADDR, 0x608 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_DSCP_TO_PBITS_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x610 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x620 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PORT_MAC_4 =
{
	8,
	{
		{ dump_RDD_MAC_ADDRESS, 0x640 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PROCESSING_GROUP_PARAMS_GEN4_TABLE_4 =
{
	16,
	{
		{ dump_RDD_PROCESSING_GROUP_PARAMS_GEN4_ENTRY, 0x680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DSCP_TO_PBITS_MAP_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_QUEUE_TABLE_4 =
{
	8,
	{
		{ dump_RDD_PBIT_TO_QUEUE_8, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_FPM_TOKENS_READ_FLAG_4 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_4 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xa10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xa20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_REPLY_4 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_LOCK_4 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_TASKS_LIMIT_4 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_4 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_DSCP_TO_PBITS_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xbc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_4 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CMD_TABLE_4 =
{
	4,
	{
		{ dump_RDD_TCAM_IC_CMD, 0xe00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_DST_IP_LKP_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xfc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_PACKET_BUFFER_4 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_4 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_4 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x20a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_ACTION_GPE_HANDLER_PTR_TABLE_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e2 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_4 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x20e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x20f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_4 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x2100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_4 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_TABLE_4 =
{
	16,
	{
		{ dump_RDD_TUNNEL_ENTRY, 0x21c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x21e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_4 =
{
	16,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_ENTRY, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_COUNTERS_GENERIC_CONTEXT_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2260 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x22e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_4 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2348 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_4 =
{
	5,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x23c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_ENABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23cd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23ce },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23cf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_4 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x23d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CFG_TABLE_4 =
{
	16,
	{
		{ dump_RDD_TCAM_IC_CFG, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_4 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x2460 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2470 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_4 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2480 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x24a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CONFIGURATION_TABLE_4 =
{
	12,
	{
		{ dump_RDD_HW_IPTV_CONFIGURATION, 0x24d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x24dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_4 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x24f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IC_MCAST_ENABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24fd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x24fe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TABLE_4 =
{
	2,
	{
		{ dump_RDD_DEFAULT_VLAN_VID_ENTRY, 0x2500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_4 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x2520 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_4 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2534 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2536 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_DONT_DROP_RATIO_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2537 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_PACKET_BASED_MAPPING_4 =
{
	4,
	{
		{ dump_RDD_INGRESS_PACKET_BASED_MAPPING_ENTRY, 0x2538 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_WAN_UNTAGGED_PRIORITY_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x253c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_4 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x253d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x253e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_4 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x253f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2540 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_TABLE_4 =
{
	9,
	{
		{ dump_RDD_PBIT_TO_DSCP_ENTRY, 0x2550 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2559 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_4 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x255a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_4 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x2560 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_4 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_HEADER_PSRAM_BUFFER_4 =
{
	4,
	{
		{ dump_RDD_TUNNEL_HEADER_PSRAM_BUFFER, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_4 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2588 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_4 =
{
	8,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2590 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_4 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2598 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_4 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_4 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x25a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_4 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x25b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_4 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x25c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_RATE_LIMITER_TABLE_5 =
{
	16,
	{
		{ dump_RDD_BASIC_RATE_LIMITER_DESCRIPTOR, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x420 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PORT_MAC_5 =
{
	8,
	{
		{ dump_RDD_MAC_ADDRESS, 0x440 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_5 =
{
	8,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x480 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_GEM_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CMD_TABLE_5 =
{
	4,
	{
		{ dump_RDD_TCAM_IC_CMD, 0x600 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_REPLY_5 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x7c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_PD_FIFO_TABLE_5 =
{
	16,
	{
		{ dump_RDD_PROCESSING_TX_DESCRIPTOR, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_5 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PROCESSING_GROUP_PARAMS_GEN4_TABLE_5 =
{
	16,
	{
		{ dump_RDD_PROCESSING_GROUP_PARAMS_GEN4_ENTRY, 0xe80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DSCP_TO_PBITS_MAP_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xf00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_PACKET_BUFFER_5 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_QUEUE_TABLE_5 =
{
	8,
	{
		{ dump_RDD_TC_TO_QUEUE_8, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_CONTEXT_DDR_ADDR_5 =
{
	8,
	{
		{ dump_RDD_DEF_FLOW_CONTEXT_DDR_ADDR, 0x2208 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_DSCP_TO_PBITS_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2210 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2220 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_LOCK_5 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2240 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_TASKS_LIMIT_5 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2280 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_SQ_TABLE_5 =
{
	2,
	{
		{ dump_RDD_AQM_SQ_ENTRY, 0x22c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_5 =
{
	8,
	{
		{ dump_RDD_SCHEDULING_QUEUE_DESCRIPTOR, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_QUEUE_TABLE_5 =
{
	8,
	{
		{ dump_RDD_PBIT_TO_QUEUE_8, 0x2400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_FPM_TOKENS_READ_FLAG_5 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2608 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_5 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x2610 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_5 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x2620 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_DSCP_TO_PBITS_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2640 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_5 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_DST_IP_LKP_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x26c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_5 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0x2700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_5 =
{
	64,
	{
		{ dump_RDD_COMPLEX_SCHEDULER_DESCRIPTOR, 0x27c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_5 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0x2800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_5 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2a00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SQ_TX_QUEUE_DROP_TABLE_5 =
{
	8,
	{
		{ dump_RDD_PACKETS_AND_BYTES, 0x2aa0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ba0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CODEL_BIAS_SLOPE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_CODEL_BIAS_SLOPE, 0x2bc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2bec },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2bee },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2bf0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_5 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x2c00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_COUNTERS_GENERIC_CONTEXT_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2c80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_5 =
{
	16,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_ENTRY, 0x2d00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2d60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x2de0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_5 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2e00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_AQM_QUEUE_TABLE_5 =
{
	4,
	{
		{ dump_RDD_AQM_QUEUE_DESCRIPTOR, 0x2e48 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_5 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x2ec8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_5 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x2ed0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CFG_TABLE_5 =
{
	16,
	{
		{ dump_RDD_TCAM_IC_CFG, 0x2ee0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2f60 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f74 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_BUDGET_ALLOCATION_TIMER_VALUE_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f76 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_SCRATCHPAD_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2f78 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_ACTION_GPE_HANDLER_PTR_TABLE_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2f80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_CONTEXT_5 =
{
	2,
	{
		{ dump_RDD_SERVICE_QUEUES_CONTEXT_DATA, 0x2fa2 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2fa4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_5 =
{
	5,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x2fa8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_ENABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fad },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SCHEDULING_FLUSH_GLOBAL_CFG_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2fae },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2faf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_CFG_CURRENT_TABLE_5 =
{
	16,
	{
		{ dump_RDD_FLUSH_CFG_ENTRY, 0x2fb0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_TABLE_5 =
{
	16,
	{
		{ dump_RDD_TUNNEL_ENTRY, 0x2fc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2fe0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ff4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2ff5 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FIRST_QUEUE_MAPPING_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2ff6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_PACKET_BASED_MAPPING_5 =
{
	4,
	{
		{ dump_RDD_INGRESS_PACKET_BASED_MAPPING_ENTRY, 0x2ff8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2ffc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_5 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2ffe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_5 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x3000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_5 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x3020 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_5 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x3030 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_UPDATE_FIFO_TABLE_5 =
{
	4,
	{
		{ dump_RDD_UPDATE_FIFO_ENTRY, 0x3040 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3060 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x3070 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TABLE_5 =
{
	2,
	{
		{ dump_RDD_DEFAULT_VLAN_VID_ENTRY, 0x3080 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_5 =
{
	2,
	{
		{ dump_RDD_AQM_QUEUE_TIMER_DESCRIPTOR, 0x30a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x30e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_DISPATCHER_CREDIT_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3110 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_AGGREGATION_TASK_DISABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x311c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IC_MCAST_ENABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x311d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x311e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_DONT_DROP_RATIO_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x311f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3120 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CONFIGURATION_TABLE_5 =
{
	12,
	{
		{ dump_RDD_HW_IPTV_CONFIGURATION, 0x3130 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_WAN_UNTAGGED_PRIORITY_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x313c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_5 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x313d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x313e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_5 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x313f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3140 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_5 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x3150 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x315c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_5 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x315d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_DISPATCHER_CREDIT_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3160 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_CODEL_DROP_DESCRIPTOR_5 =
{
	12,
	{
		{ dump_RDD_CODEL_DROP_DESCRIPTOR, 0x3170 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_5 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x3180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_5 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x3198 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_TABLE_5 =
{
	9,
	{
		{ dump_RDD_PBIT_TO_DSCP_ENTRY, 0x31a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_5 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x31b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_5 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x31c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_5 =
{
	8,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x31d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_5 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x31d8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_HEADER_PSRAM_BUFFER_5 =
{
	4,
	{
		{ dump_RDD_TUNNEL_HEADER_PSRAM_BUFFER, 0x31e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_5 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x31e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_5 =
{
	2,
	{
		{ dump_RDD_FLUSH_CFG_ENABLE_ENTRY, 0x31f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_5 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x31f8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_5 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x3200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_5 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x3220 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_6 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_6 =
{
	8,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x280 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_GEM_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_QUEUE_TABLE_6 =
{
	8,
	{
		{ dump_RDD_TC_TO_QUEUE_8, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_CONTEXT_DDR_ADDR_6 =
{
	8,
	{
		{ dump_RDD_DEF_FLOW_CONTEXT_DDR_ADDR, 0x608 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_DSCP_TO_PBITS_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x610 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x620 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PORT_MAC_6 =
{
	8,
	{
		{ dump_RDD_MAC_ADDRESS, 0x640 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PROCESSING_GROUP_PARAMS_GEN4_TABLE_6 =
{
	16,
	{
		{ dump_RDD_PROCESSING_GROUP_PARAMS_GEN4_ENTRY, 0x680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DSCP_TO_PBITS_MAP_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_QUEUE_TABLE_6 =
{
	8,
	{
		{ dump_RDD_PBIT_TO_QUEUE_8, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_FPM_TOKENS_READ_FLAG_6 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_6 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xa10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xa20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_REPLY_6 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_LOCK_6 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_TASKS_LIMIT_6 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_6 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_DSCP_TO_PBITS_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xbc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_6 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CMD_TABLE_6 =
{
	4,
	{
		{ dump_RDD_TCAM_IC_CMD, 0xe00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_DST_IP_LKP_TABLE_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xfc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_PACKET_BUFFER_6 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_6 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_6 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x20a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_ACTION_GPE_HANDLER_PTR_TABLE_6 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_6 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e2 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_6 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_6 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_6 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x20e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x20f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_6 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x2100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_6 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_TABLE_6 =
{
	16,
	{
		{ dump_RDD_TUNNEL_ENTRY, 0x21c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x21e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_6 =
{
	16,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_ENTRY, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_COUNTERS_GENERIC_CONTEXT_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2260 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_6 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x22e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_6 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2348 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_6 =
{
	5,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x23c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_ENABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23cd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23ce },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23cf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_6 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x23d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CFG_TABLE_6 =
{
	16,
	{
		{ dump_RDD_TCAM_IC_CFG, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_6 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x2460 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2470 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_6 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2480 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x24a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CONFIGURATION_TABLE_6 =
{
	12,
	{
		{ dump_RDD_HW_IPTV_CONFIGURATION, 0x24d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x24dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_6 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x24f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IC_MCAST_ENABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24fd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_6 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x24fe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TABLE_6 =
{
	2,
	{
		{ dump_RDD_DEFAULT_VLAN_VID_ENTRY, 0x2500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_6 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x2520 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_6 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2534 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2536 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_DONT_DROP_RATIO_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2537 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_PACKET_BASED_MAPPING_6 =
{
	4,
	{
		{ dump_RDD_INGRESS_PACKET_BASED_MAPPING_ENTRY, 0x2538 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_WAN_UNTAGGED_PRIORITY_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x253c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_6 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x253d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x253e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_6 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x253f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2540 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_TABLE_6 =
{
	9,
	{
		{ dump_RDD_PBIT_TO_DSCP_ENTRY, 0x2550 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2559 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_6 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x255a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_6 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x2560 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_6 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_HEADER_PSRAM_BUFFER_6 =
{
	4,
	{
		{ dump_RDD_TUNNEL_HEADER_PSRAM_BUFFER, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_6 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2588 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_6 =
{
	8,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2590 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_6 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2598 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_6 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_6 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x25a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_6 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x25b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_6 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x25c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_TABLE_7 =
{
	2,
	{
		{ dump_RDD_RX_FLOW_ENTRY, 0x0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_TABLE_7 =
{
	8,
	{
		{ dump_RDD_VPORT_CFG_ENTRY, 0x280 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_GEM_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TC_TO_QUEUE_TABLE_7 =
{
	8,
	{
		{ dump_RDD_TC_TO_QUEUE_8, 0x400 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_FLOW_CONTEXT_DDR_ADDR_7 =
{
	8,
	{
		{ dump_RDD_DEF_FLOW_CONTEXT_DDR_ADDR, 0x608 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_DSCP_TO_PBITS_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x610 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_ENABLE_TABLE_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x620 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PORT_MAC_7 =
{
	8,
	{
		{ dump_RDD_MAC_ADDRESS, 0x640 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PROCESSING_GROUP_PARAMS_GEN4_TABLE_7 =
{
	16,
	{
		{ dump_RDD_PROCESSING_GROUP_PARAMS_GEN4_ENTRY, 0x680 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DSCP_TO_PBITS_MAP_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x700 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT PBIT_TO_QUEUE_TABLE_7 =
{
	8,
	{
		{ dump_RDD_PBIT_TO_QUEUE_8, 0x800 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_FPM_TOKENS_READ_FLAG_7 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa08 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_POOL_NUMBER_MAPPING_TABLE_7 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0xa10 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_GLOBAL_REGISTERS_INIT_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xa20 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_REPLY_7 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa40 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_PROCESSING_TASKS_LOCK_7 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xa80 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_TASKS_LIMIT_7 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0xac0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_FLOW_TABLE_7 =
{
	1,
	{
		{ dump_RDD_TX_FLOW_ENTRY, 0xb00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_DSCP_TO_PBITS_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0xbc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RUNNER_PROFILING_TRACE_BUFFER_7 =
{
	4,
	{
		{ dump_RDD_TRACE_EVENT, 0xc00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CMD_TABLE_7 =
{
	4,
	{
		{ dump_RDD_TCAM_IC_CMD, 0xe00 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_DST_IP_LKP_TABLE_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0xfc0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DS_PACKET_BUFFER_7 =
{
	512,
	{
		{ dump_RDD_PACKET_BUFFER, 0x1000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT POLICER_PARAMS_TABLE_7 =
{
	2,
	{
		{ dump_RDD_POLICER_PARAMS_ENTRY, 0x2000 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_7 =
{
	1,
	{
		{ dump_RDD_FPM_POOL_NUMBER, 0x20a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VLAN_ACTION_GPE_HANDLER_PTR_TABLE_7 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_7 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e2 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_7 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e4 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT AQM_NUM_QUEUES_7 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x20e6 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NULL_BUFFER_7 =
{
	8,
	{
		{ dump_RDD_BYTES_8, 0x20e8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_QUEUE_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x20f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_PROFILE_TABLE_7 =
{
	8,
	{
		{ dump_RDD_INGRESS_FILTER_CTRL, 0x2100 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_CFG_EX_TABLE_7 =
{
	4,
	{
		{ dump_RDD_VPORT_CFG_EX_ENTRY, 0x2180 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_TABLE_7 =
{
	16,
	{
		{ dump_RDD_TUNNEL_ENTRY, 0x21c0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x21e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT EMAC_FLOW_CTRL_7 =
{
	16,
	{
		{ dump_RDD_EMAC_FLOW_CTRL_ENTRY, 0x2200 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_COUNTERS_GENERIC_CONTEXT_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2260 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT HOST_TX_TRUNCATE_MIRRORING_TABLE_7 =
{
	2,
	{
		{ dump_RDD_MIRRORING_TRUNCATE_ENTRY, 0x22e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NATC_TBL_CFG_7 =
{
	24,
	{
		{ dump_RDD_NATC_TBL_CONFIGURATION, 0x2300 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT REGISTERS_BUFFER_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2348 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SYSTEM_CONFIGURATION_7 =
{
	5,
	{
		{ dump_RDD_SYSTEM_CONFIGURATION_ENTRY, 0x23c8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_ENABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23cd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT LOOPBACK_WAN_FLOW_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23ce },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FORCE_DSCP_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x23cf },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BRIDGE_CFG_TABLE_7 =
{
	8,
	{
		{ dump_RDD_BRIDGE_CFG, 0x23d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TCAM_IC_CFG_TABLE_7 =
{
	16,
	{
		{ dump_RDD_TCAM_IC_CFG, 0x23e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_TO_RL_OVERHEAD_TABLE_7 =
{
	1,
	{
		{ dump_RDD_VPORT_TO_RL_OVERHEAD_ENTRY, 0x2460 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_L2_REASON_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2470 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_7 =
{
	2,
	{
		{ dump_RDD_BYTES_2, 0x2480 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_SCRATCHPAD_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x24a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CONFIGURATION_TABLE_7 =
{
	12,
	{
		{ dump_RDD_HW_IPTV_CONFIGURATION, 0x24d0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TASK_IDX_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x24dc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24e0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GENERAL_QUEUE_DYNAMIC_MNG_TABLE_7 =
{
	12,
	{
		{ dump_RDD_QUEUE_DYNAMIC_MNG_ENTRY, 0x24f0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CORE_ID_TABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24fc },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IC_MCAST_ENABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x24fd },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_CONFIGURATION_7 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x24fe },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEFAULT_VLAN_VID_TABLE_7 =
{
	2,
	{
		{ dump_RDD_DEFAULT_VLAN_VID_ENTRY, 0x2500 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FPM_GLOBAL_CFG_7 =
{
	20,
	{
		{ dump_RDD_FPM_GLOBAL_CFG, 0x2520 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TX_MIRRORING_CONFIGURATION_7 =
{
	2,
	{
		{ dump_RDD_MIRRORING_DESCRIPTOR, 0x2534 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT SRAM_DUMMY_STORE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2536 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_DONT_DROP_RATIO_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2537 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_PACKET_BASED_MAPPING_7 =
{
	4,
	{
		{ dump_RDD_INGRESS_PACKET_BASED_MAPPING_ENTRY, 0x2538 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_QOS_WAN_UNTAGGED_PRIORITY_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x253c },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT CPU_REDIRECT_MODE_7 =
{
	1,
	{
		{ dump_RDD_CPU_REDIRECT_MODE_ENTRY, 0x253d },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_CORE_LOCK_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x253e },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CLASSIFICATION_CFG_TABLE_7 =
{
	1,
	{
		{ dump_RDD_IPTV_CLASSIFICATION_CFG_ENTRY, 0x253f },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT BITS_CALC_MASKS_TABLE_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x2540 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GLOBAL_PBIT_TO_DSCP_TABLE_7 =
{
	9,
	{
		{ dump_RDD_PBIT_TO_DSCP_ENTRY, 0x2550 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_1588_CFG_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x2559 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT RX_MIRRORING_DIRECT_ENABLE_7 =
{
	1,
	{
		{ dump_RDD_BYTE_1, 0x255a },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT DEBUG_PRINT_TABLE_7 =
{
	16,
	{
		{ dump_RDD_DEBUG_PRINT_INFO, 0x2560 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT GDX_PARAMS_TABLE_7 =
{
	12,
	{
		{ dump_RDD_GDX_PARAMS, 0x2570 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNEL_HEADER_PSRAM_BUFFER_7 =
{
	4,
	{
		{ dump_RDD_TUNNEL_HEADER_PSRAM_BUFFER, 0x2580 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT TUNNELS_PARSING_CFG_7 =
{
	8,
	{
		{ dump_RDD_TUNNELS_PARSING_CFG, 0x2588 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT NAT_CACHE_CFG_7 =
{
	8,
	{
		{ dump_RDD_NAT_CACHE_CFG, 0x2590 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_CFG_TABLE_7 =
{
	8,
	{
		{ dump_RDD_IPTV_CFG, 0x2598 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT FW_ERROR_VECTOR_TABLE_7 =
{
	4,
	{
		{ dump_RDD_BYTES_4, 0x25a0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT IPTV_DDR_CTX_TABLE_ADDRESS_7 =
{
	8,
	{
		{ dump_RDD_IPTV_DDR_CTX_TABLE_ADDRESS, 0x25a8 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT INGRESS_FILTER_CFG_7 =
{
	2,
	{
		{ dump_RDD_INGRESS_FILTER_CFG, 0x25b0 },
		{ 0, 0 },
	}
};
#endif
#if defined BCM6856
static DUMP_RUNNERREG_STRUCT MULTICAST_KEY_MASK_7 =
{
	4,
	{
		{ dump_RDD_MULTICAST_KEY_MASK_ENTRY, 0x25c0 },
		{ 0, 0 },
	}
};
#endif

TABLE_STRUCT RUNNER_TABLES[NUMBER_OF_TABLES] =
{
#if defined BCM6856
	{ "DS_TM_PD_FIFO_TABLE", 1, CORE_0_INDEX, &DS_TM_PD_FIFO_TABLE, 144, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_TX_COUNTERS", 1, CORE_0_INDEX, &VLAN_TX_COUNTERS, 145, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_BBH_TX_WAKE_UP_DATA_TABLE", 1, CORE_0_INDEX, &DS_TM_BBH_TX_WAKE_UP_DATA_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FLUSH_CFG_CPU_TABLE", 1, CORE_0_INDEX, &DS_TM_FLUSH_CFG_CPU_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_0_INDEX, &AQM_ENABLE_TABLE, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_TM_FLOW_CNTR_TABLE", 1, CORE_0_INDEX, &DS_TM_TM_FLOW_CNTR_TABLE, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "COMPLEX_SCHEDULER_TABLE_DS", 1, CORE_0_INDEX, &COMPLEX_SCHEDULER_TABLE_DS, 7, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR", 1, CORE_0_INDEX, &DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_0_INDEX, &AQM_NUM_QUEUES, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GHOST_REPORTING_GLOBAL_CFG", 1, CORE_0_INDEX, &GHOST_REPORTING_GLOBAL_CFG, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BASIC_RATE_LIMITER_TABLE_DS", 1, CORE_0_INDEX, &BASIC_RATE_LIMITER_TABLE_DS, 72, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_CPU_TX_ABS_COUNTERS", 1, CORE_0_INDEX, &DS_TM_CPU_TX_ABS_COUNTERS, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "REPORTING_QUEUE_ACCUMULATED_TABLE", 1, CORE_0_INDEX, &REPORTING_QUEUE_ACCUMULATED_TABLE, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_AQM_QUEUE_TABLE", 1, CORE_0_INDEX, &DS_TM_AQM_QUEUE_TABLE, 72, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_0_INDEX, &REGISTERS_BUFFER, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_0_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "CODEL_BIAS_SLOPE_TABLE", 1, CORE_0_INDEX, &CODEL_BIAS_SLOPE_TABLE, 11, 1, 1 },
#endif
#if defined BCM6856
	{ "BUDGET_ALLOCATION_TIMER_VALUE", 1, CORE_0_INDEX, &BUDGET_ALLOCATION_TIMER_VALUE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_BB_DESTINATION_TABLE", 1, CORE_0_INDEX, &DS_TM_BB_DESTINATION_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FLUSH_CFG_FW_TABLE", 1, CORE_0_INDEX, &DS_TM_FLUSH_CFG_FW_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "REPORTING_QUEUE_DESCRIPTOR_TABLE", 1, CORE_0_INDEX, &REPORTING_QUEUE_DESCRIPTOR_TABLE, 129, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_0_INDEX, &DEBUG_SCRATCHPAD, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_0_INDEX, &NULL_BUFFER, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "UPDATE_FIFO_TABLE", 1, CORE_0_INDEX, &UPDATE_FIFO_TABLE, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_SCHEDULING_QUEUE_AGING_VECTOR", 1, CORE_0_INDEX, &DS_TM_SCHEDULING_QUEUE_AGING_VECTOR, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_0_INDEX, &TASK_IDX, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MIRRORING_SCRATCH", 1, CORE_0_INDEX, &MIRRORING_SCRATCH, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR", 1, CORE_0_INDEX, &DS_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "SCHEDULING_FLUSH_GLOBAL_CFG", 1, CORE_0_INDEX, &SCHEDULING_FLUSH_GLOBAL_CFG, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TM_VLAN_STATS_ENABLE", 1, CORE_0_INDEX, &TM_VLAN_STATS_ENABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FIRST_QUEUE_MAPPING", 1, CORE_0_INDEX, &DS_TM_FIRST_QUEUE_MAPPING, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "REPORT_BBH_TX_QUEUE_ID_TABLE", 1, CORE_0_INDEX, &REPORT_BBH_TX_QUEUE_ID_TABLE, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE", 1, CORE_0_INDEX, &GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_EXCEPTION", 1, CORE_0_INDEX, &TX_EXCEPTION, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_0_INDEX, &CORE_ID_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_0_INDEX, &TX_MIRRORING_CONFIGURATION, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BBH_TX_EGRESS_REPORT_COUNTER_TABLE", 1, CORE_0_INDEX, &BBH_TX_EGRESS_REPORT_COUNTER_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "QUEUE_TO_REPORT_BIT_VECTOR", 1, CORE_0_INDEX, &QUEUE_TO_REPORT_BIT_VECTOR, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FLUSH_AGGREGATION_TASK_DISABLE", 1, CORE_0_INDEX, &DS_TM_FLUSH_AGGREGATION_TASK_DISABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_0_INDEX, &SRAM_DUMMY_STORE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BACKUP_BBH_INGRESS_COUNTERS_TABLE", 1, CORE_0_INDEX, &BACKUP_BBH_INGRESS_COUNTERS_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BACKUP_BBH_EGRESS_COUNTERS_TABLE", 1, CORE_0_INDEX, &BACKUP_BBH_EGRESS_COUNTERS_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BBH_TX_INGRESS_COUNTER_TABLE", 1, CORE_0_INDEX, &BBH_TX_INGRESS_COUNTER_TABLE, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_0_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FLUSH_CFG_CURRENT_TABLE", 1, CORE_0_INDEX, &DS_TM_FLUSH_CFG_CURRENT_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "REPORTING_COUNTER_TABLE", 1, CORE_0_INDEX, &REPORTING_COUNTER_TABLE, 40, 1, 1 },
#endif
#if defined BCM6856
	{ "RATE_LIMITER_VALID_TABLE_DS", 1, CORE_0_INDEX, &RATE_LIMITER_VALID_TABLE_DS, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FLUSH_DISPATCHER_CREDIT_TABLE", 1, CORE_0_INDEX, &DS_TM_FLUSH_DISPATCHER_CREDIT_TABLE, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_0_INDEX, &DEBUG_PRINT_CORE_LOCK, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_CODEL_DROP_DESCRIPTOR", 1, CORE_0_INDEX, &DS_TM_CODEL_DROP_DESCRIPTOR, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_0_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_DISPATCHER_CREDIT_TABLE", 1, CORE_0_INDEX, &TX_MIRRORING_DISPATCHER_CREDIT_TABLE, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_0_INDEX, &DEBUG_PRINT_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "REPORTING_QUEUE_COUNTER_TABLE", 1, CORE_0_INDEX, &REPORTING_QUEUE_COUNTER_TABLE, 129, 1, 1 },
#endif
#if defined BCM6856
	{ "XGPON_REPORT_ZERO_SENT_TABLE", 1, CORE_0_INDEX, &XGPON_REPORT_ZERO_SENT_TABLE, 10, 1, 1 },
#endif
#if defined BCM6856
	{ "COMPLEX_SCHEDULER_WAKEUP_CMD_DS", 1, CORE_0_INDEX, &COMPLEX_SCHEDULER_WAKEUP_CMD_DS, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_0_INDEX, &FPM_GLOBAL_CFG, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_FLUSH_CFG_ENABLE_TABLE", 1, CORE_0_INDEX, &DS_TM_FLUSH_CFG_ENABLE_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_BBH_TX_EGRESS_COUNTER_TABLE", 1, CORE_0_INDEX, &DS_TM_BBH_TX_EGRESS_COUNTER_TABLE, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_0_INDEX, &FW_ERROR_VECTOR_TABLE, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_SCHEDULING_QUEUE_TABLE", 1, CORE_0_INDEX, &DS_TM_SCHEDULING_QUEUE_TABLE, 72, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_AQM_QUEUE_TIMER_TABLE", 1, CORE_0_INDEX, &DS_TM_AQM_QUEUE_TIMER_TABLE, 72, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_0_INDEX, &RUNNER_PROFILING_TRACE_BUFFER, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_0_INDEX, &NATC_TBL_CFG, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_TM_BBH_QUEUE_TABLE", 1, CORE_0_INDEX, &DS_TM_BBH_QUEUE_TABLE, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_PD_FIFO_TABLE", 1, CORE_1_INDEX, &US_TM_PD_FIFO_TABLE_1, 264, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_TM_FLOW_CNTR_TABLE", 1, CORE_1_INDEX, &US_TM_TM_FLOW_CNTR_TABLE_1, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_RX_MIRRORING_SCRATCHPAD", 1, CORE_1_INDEX, &DIRECT_FLOW_RX_MIRRORING_SCRATCHPAD_1, 136, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE", 1, CORE_1_INDEX, &US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_PD_TABLE", 1, CORE_1_INDEX, &DIRECT_FLOW_PD_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_1_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_1, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR", 1, CORE_1_INDEX, &US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_PAUSE_QUANTA", 1, CORE_1_INDEX, &DIRECT_FLOW_PAUSE_QUANTA_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_BBH_TX_WAN_0_FIFO_BYTES_USED", 1, CORE_1_INDEX, &US_TM_BBH_TX_WAN_0_FIFO_BYTES_USED_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_AQM_QUEUE_TIMER_TABLE", 1, CORE_1_INDEX, &US_TM_AQM_QUEUE_TIMER_TABLE_1, 132, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_CPU_TX_ABS_COUNTERS", 1, CORE_1_INDEX, &US_TM_CPU_TX_ABS_COUNTERS_1, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "BBH_TX_EPON_WAKE_UP_DATA_TABLE", 1, CORE_1_INDEX, &BBH_TX_EPON_WAKE_UP_DATA_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FLUSH_CFG_CPU_TABLE", 1, CORE_1_INDEX, &US_TM_FLUSH_CFG_CPU_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_CNTR_TABLE", 1, CORE_1_INDEX, &DIRECT_FLOW_CNTR_TABLE_1, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "CODEL_BIAS_SLOPE_TABLE", 1, CORE_1_INDEX, &CODEL_BIAS_SLOPE_TABLE_1, 11, 1, 1 },
#endif
#if defined BCM6856
	{ "BUDGET_ALLOCATION_TIMER_VALUE", 1, CORE_1_INDEX, &BUDGET_ALLOCATION_TIMER_VALUE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_WAN_0_BB_DESTINATION_TABLE", 1, CORE_1_INDEX, &US_TM_WAN_0_BB_DESTINATION_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FLUSH_CFG_FW_TABLE", 1, CORE_1_INDEX, &US_TM_FLUSH_CFG_FW_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "COMPLEX_SCHEDULER_TABLE_US", 1, CORE_1_INDEX, &COMPLEX_SCHEDULER_TABLE_US_1, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_SCHEDULING_QUEUE_TABLE", 1, CORE_1_INDEX, &US_TM_SCHEDULING_QUEUE_TABLE_1, 132, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_1_INDEX, &REGISTERS_BUFFER_1, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_1_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_1, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE", 1, CORE_1_INDEX, &US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_1, 40, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_1_INDEX, &NULL_BUFFER_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FLUSH_CFG_CURRENT_TABLE", 1, CORE_1_INDEX, &US_TM_FLUSH_CFG_CURRENT_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_1_INDEX, &NATC_TBL_CFG_1, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_1_INDEX, &DEBUG_SCRATCHPAD_1, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "MIRRORING_SCRATCH", 1, CORE_1_INDEX, &MIRRORING_SCRATCH_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BBH_TX_EPON_INGRESS_COUNTER_TABLE", 1, CORE_1_INDEX, &BBH_TX_EPON_INGRESS_COUNTER_TABLE_1, 40, 1, 1 },
#endif
#if defined BCM6856
	{ "BASIC_SCHEDULER_WAKEUP_CMD_US", 1, CORE_1_INDEX, &BASIC_SCHEDULER_WAKEUP_CMD_US_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_1_INDEX, &TASK_IDX_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "OVERALL_RATE_LIMITER_TABLE", 1, CORE_1_INDEX, &OVERALL_RATE_LIMITER_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BBH_TX_EPON_EGRESS_COUNTER_TABLE", 1, CORE_1_INDEX, &BBH_TX_EPON_EGRESS_COUNTER_TABLE_1, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_EPON_CONTROL_SCRATCH", 1, CORE_1_INDEX, &DIRECT_FLOW_EPON_CONTROL_SCRATCH_1, 22, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_WAN_VIQ_EXCLUSIVE", 1, CORE_1_INDEX, &DIRECT_FLOW_WAN_VIQ_EXCLUSIVE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_TX_PAUSE_NACK", 1, CORE_1_INDEX, &US_TM_TX_PAUSE_NACK_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "COMPLEX_SCHEDULER_WAKEUP_CMD_US", 1, CORE_1_INDEX, &COMPLEX_SCHEDULER_WAKEUP_CMD_US_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BBH_TX_US_WAN_0_FIFO_BYTES_THRESHOLD", 1, CORE_1_INDEX, &BBH_TX_US_WAN_0_FIFO_BYTES_THRESHOLD_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SCHEDULING_FLUSH_GLOBAL_CFG", 1, CORE_1_INDEX, &SCHEDULING_FLUSH_GLOBAL_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TM_VLAN_STATS_ENABLE", 1, CORE_1_INDEX, &TM_VLAN_STATS_ENABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_BBH_QUEUE_TABLE", 1, CORE_1_INDEX, &US_TM_BBH_QUEUE_TABLE_1, 40, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_SCHEDULING_QUEUE_AGING_VECTOR", 1, CORE_1_INDEX, &US_TM_SCHEDULING_QUEUE_AGING_VECTOR_1, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FIRST_QUEUE_MAPPING", 1, CORE_1_INDEX, &US_TM_FIRST_QUEUE_MAPPING_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "PON_TM_VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_1_INDEX, &PON_TM_VPORT_TO_RL_OVERHEAD_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_EXCEPTION", 1, CORE_1_INDEX, &TX_EXCEPTION_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FLUSH_CFG_ENABLE_TABLE", 1, CORE_1_INDEX, &US_TM_FLUSH_CFG_ENABLE_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_1_INDEX, &CORE_ID_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FLUSH_AGGREGATION_TASK_DISABLE", 1, CORE_1_INDEX, &US_TM_FLUSH_AGGREGATION_TASK_DISABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_1_INDEX, &RX_MIRRORING_CONFIGURATION_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_1_INDEX, &TX_MIRRORING_CONFIGURATION_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "UPDATE_FIFO_TABLE", 1, CORE_1_INDEX, &UPDATE_FIFO_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR", 1, CORE_1_INDEX, &US_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_1, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_1_INDEX, &SRAM_DUMMY_STORE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MAC_TYPE", 1, CORE_1_INDEX, &MAC_TYPE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "WAN_0_BBH_TX_FIFO_SIZE", 1, CORE_1_INDEX, &WAN_0_BBH_TX_FIFO_SIZE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_1_INDEX, &DEBUG_PRINT_CORE_LOCK_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_1_INDEX, &RX_MIRRORING_DIRECT_ENABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "EPON_UPDATE_FIFO_TABLE", 1, CORE_1_INDEX, &EPON_UPDATE_FIFO_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_TX_OCTETS_COUNTERS_TABLE", 1, CORE_1_INDEX, &US_TM_TX_OCTETS_COUNTERS_TABLE_1, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "RATE_LIMITER_VALID_TABLE_US", 1, CORE_1_INDEX, &RATE_LIMITER_VALID_TABLE_US_1, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_FLUSH_DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &US_TM_FLUSH_DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_CODEL_DROP_DESCRIPTOR", 1, CORE_1_INDEX, &US_TM_CODEL_DROP_DESCRIPTOR_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DIRECT_FLOW_PAUSE_DEBUG", 1, CORE_1_INDEX, &DIRECT_FLOW_PAUSE_DEBUG_1, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_1_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_DISPATCHER_CREDIT_TABLE", 1, CORE_1_INDEX, &TX_MIRRORING_DISPATCHER_CREDIT_TABLE_1, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_1_INDEX, &DEBUG_PRINT_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_1_INDEX, &FPM_GLOBAL_CFG_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BASIC_RATE_LIMITER_TABLE_US", 1, CORE_1_INDEX, &BASIC_RATE_LIMITER_TABLE_US_1, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_TX_COUNTERS", 1, CORE_1_INDEX, &VLAN_TX_COUNTERS_1, 145, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_1_INDEX, &FW_ERROR_VECTOR_TABLE_1, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "US_TM_AQM_QUEUE_TABLE", 1, CORE_1_INDEX, &US_TM_AQM_QUEUE_TABLE_1, 132, 1, 1 },
#endif
#if defined BCM6856
	{ "BASIC_SCHEDULER_TABLE_US", 1, CORE_1_INDEX, &BASIC_SCHEDULER_TABLE_US_1, 33, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_1_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_1, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_SCRATCHPAD", 1, CORE_2_INDEX, &CPU_TX_SCRATCHPAD_2, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_TABLE", 1, CORE_2_INDEX, &RX_FLOW_TABLE_2, 320, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_TABLE", 1, CORE_2_INDEX, &VPORT_CFG_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_GEM_TABLE", 1, CORE_2_INDEX, &PBIT_TO_GEM_TABLE_2, 32, 8, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_QUEUE_TABLE", 1, CORE_2_INDEX, &TC_TO_QUEUE_TABLE_2, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_CONTEXT_DDR_ADDR", 1, CORE_2_INDEX, &RX_FLOW_CONTEXT_DDR_ADDR_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_DSCP_TO_PBITS_TABLE", 1, CORE_2_INDEX, &VPORT_TO_DSCP_TO_PBITS_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_2_INDEX, &AQM_ENABLE_TABLE_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PORT_MAC", 1, CORE_2_INDEX, &PORT_MAC_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PROCESSING_GROUP_PARAMS_GEN4_TABLE", 1, CORE_2_INDEX, &PROCESSING_GROUP_PARAMS_GEN4_TABLE_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DSCP_TO_PBITS_MAP_TABLE", 1, CORE_2_INDEX, &DSCP_TO_PBITS_MAP_TABLE_2, 4, 64, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_QUEUE_TABLE", 1, CORE_2_INDEX, &PBIT_TO_QUEUE_TABLE_2, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_FPM_TOKENS_READ_FLAG", 1, CORE_2_INDEX, &MCAST_FPM_TOKENS_READ_FLAG_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_2_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_2_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_REPLY", 1, CORE_2_INDEX, &MCAST_PROCESSING_TASKS_REPLY_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_LOCK", 1, CORE_2_INDEX, &MCAST_PROCESSING_TASKS_LOCK_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_TASKS_LIMIT", 1, CORE_2_INDEX, &MCAST_BBH_OVERRUN_TASKS_LIMIT_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_FLOW_TABLE", 1, CORE_2_INDEX, &TX_FLOW_TABLE_2, 192, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_DSCP_TO_PBITS_TABLE", 1, CORE_2_INDEX, &GLOBAL_DSCP_TO_PBITS_TABLE_2, 1, 64, 1 },
#endif
#if defined BCM6856
	{ "DS_PACKET_BUFFER", 1, CORE_2_INDEX, &DS_PACKET_BUFFER_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_2_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_2, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CMD_TABLE", 1, CORE_2_INDEX, &TCAM_IC_CMD_TABLE_2, 7, 16, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_DST_IP_LKP_TABLE", 1, CORE_2_INDEX, &TUNNEL_DST_IP_LKP_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_POLICER_CBS", 1, CORE_2_INDEX, &FW_POLICER_CBS_2, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "QUEUE_THRESHOLD_VECTOR", 1, CORE_2_INDEX, &QUEUE_THRESHOLD_VECTOR_2, 9, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT", 1, CORE_2_INDEX, &MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT", 1, CORE_2_INDEX, &MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TIMER_COMMON_CTR_REP", 1, CORE_2_INDEX, &TIMER_COMMON_CTR_REP_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BUFFER_ALLOC_REPLY", 1, CORE_2_INDEX, &BUFFER_ALLOC_REPLY_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_EX_TABLE", 1, CORE_2_INDEX, &VPORT_CFG_EX_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_ACTION_GPE_HANDLER_PTR_TABLE", 1, CORE_2_INDEX, &VLAN_ACTION_GPE_HANDLER_PTR_TABLE_2, 17, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_2_INDEX, &AQM_NUM_QUEUES_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TIMER_COMMON_TIMER_VALUE", 1, CORE_2_INDEX, &TIMER_COMMON_TIMER_VALUE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_ENABLE", 1, CORE_2_INDEX, &GLOBAL_PBIT_TO_DSCP_ENABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL_VECTOR", 1, CORE_2_INDEX, &EMAC_FLOW_CTRL_VECTOR_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_2_INDEX, &NULL_BUFFER_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_2_INDEX, &LOOPBACK_QUEUE_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_SRAM_PD_FIFO", 1, CORE_2_INDEX, &CPU_RECYCLE_SRAM_PD_FIFO_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_POLICER_BUDGET", 1, CORE_2_INDEX, &FW_POLICER_BUDGET_2, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_POLICER_BUDGET_REMAINDER", 1, CORE_2_INDEX, &FW_POLICER_BUDGET_REMAINDER_2, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_INTERRUPT_COALESCING_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "POLICER_PARAMS_TABLE", 1, CORE_2_INDEX, &POLICER_PARAMS_TABLE_2, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_2_INDEX, &FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_TABLE", 1, CORE_2_INDEX, &TUNNEL_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_2_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_2, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_2_INDEX, &INGRESS_FILTER_PROFILE_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_COUNTERS_GENERIC_CONTEXT", 1, CORE_2_INDEX, &NATC_COUNTERS_GENERIC_CONTEXT_2, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL", 1, CORE_2_INDEX, &EMAC_FLOW_CTRL_2, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_2_INDEX, &REGISTERS_BUFFER_2, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_2_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_2_INDEX, &NATC_TBL_CFG_2, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR", 1, CORE_2_INDEX, &CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BRIDGE_CFG_TABLE", 1, CORE_2_INDEX, &BRIDGE_CFG_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CFG_TABLE", 1, CORE_2_INDEX, &TCAM_IC_CFG_TABLE_2, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_2_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_2_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE", 1, CORE_2_INDEX, &DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_PBIT_TO_DISCARD_PRIO_VECTOR", 1, CORE_2_INDEX, &VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_EGRESS_DISPATCHER_CREDIT_TABLE", 1, CORE_2_INDEX, &CPU_TX_EGRESS_DISPATCHER_CREDIT_TABLE_2, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_2_INDEX, &TASK_IDX_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL_BUDGET", 1, CORE_2_INDEX, &EMAC_FLOW_CTRL_BUDGET_2, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_INTERRUPT_ID_DDR_ADDR", 1, CORE_2_INDEX, &CPU_RX_INTERRUPT_ID_DDR_ADDR_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_RING_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &CPU_TX_RING_DESCRIPTOR_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TABLE", 1, CORE_2_INDEX, &DEFAULT_VLAN_VID_TABLE_2, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_2_INDEX, &DEBUG_SCRATCHPAD_2, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_INGRESS_DISPATCHER_CREDIT_TABLE", 1, CORE_2_INDEX, &CPU_TX_INGRESS_DISPATCHER_CREDIT_TABLE_2, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_2_INDEX, &LOOPBACK_WAN_FLOW_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FORCE_DSCP", 1, CORE_2_INDEX, &FORCE_DSCP_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_VLAN_STATS_ENABLE", 1, CORE_2_INDEX, &CPU_TX_VLAN_STATS_ENABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_2_INDEX, &CORE_ID_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CONFIGURATION_TABLE", 1, CORE_2_INDEX, &IPTV_CONFIGURATION_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_2_INDEX, &RX_MIRRORING_CONFIGURATION_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_2_INDEX, &TX_MIRRORING_CONFIGURATION_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_2_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IC_MCAST_ENABLE", 1, CORE_2_INDEX, &IC_MCAST_ENABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_2_INDEX, &SRAM_DUMMY_STORE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_DONT_DROP_RATIO", 1, CORE_2_INDEX, &INGRESS_QOS_DONT_DROP_RATIO_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_WAN_UNTAGGED_PRIORITY", 1, CORE_2_INDEX, &INGRESS_QOS_WAN_UNTAGGED_PRIORITY_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RING_CPU_TX_DESCRIPTOR_DATA_TABLE", 1, CORE_2_INDEX, &RING_CPU_TX_DESCRIPTOR_DATA_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_POLICER_VECTOR", 1, CORE_2_INDEX, &FW_POLICER_VECTOR_2, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REDIRECT_MODE", 1, CORE_2_INDEX, &CPU_REDIRECT_MODE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_2_INDEX, &DEBUG_PRINT_CORE_LOCK_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_2_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_2_INDEX, &INGRESS_FILTER_1588_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_TABLE", 1, CORE_2_INDEX, &GLOBAL_PBIT_TO_DSCP_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_2_INDEX, &RX_MIRRORING_DIRECT_ENABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_2_INDEX, &BITS_CALC_MASKS_TABLE_2, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_RING_DESCRIPTOR_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_RING_DESCRIPTOR_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_2_INDEX, &FPM_GLOBAL_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SYSTEM_CONFIGURATION", 1, CORE_2_INDEX, &SYSTEM_CONFIGURATION_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_SYNC_FIFO_TABLE", 1, CORE_2_INDEX, &CPU_TX_SYNC_FIFO_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_PD_SCRATCH", 1, CORE_2_INDEX, &CPU_RECYCLE_PD_SCRATCH_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_PACKET_BASED_MAPPING", 1, CORE_2_INDEX, &INGRESS_PACKET_BASED_MAPPING_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_2_INDEX, &DEBUG_PRINT_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GDX_PARAMS_TABLE", 1, CORE_2_INDEX, &GDX_PARAMS_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_TX_RING_INDICES_VALUES_TABLE", 1, CORE_2_INDEX, &CPU_TX_RING_INDICES_VALUES_TABLE_2, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_HEADER_PSRAM_BUFFER", 1, CORE_2_INDEX, &TUNNEL_HEADER_PSRAM_BUFFER_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNELS_PARSING_CFG", 1, CORE_2_INDEX, &TUNNELS_PARSING_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_INTERRUPT_SCRATCH", 1, CORE_2_INDEX, &CPU_RECYCLE_INTERRUPT_SCRATCH_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NAT_CACHE_CFG", 1, CORE_2_INDEX, &NAT_CACHE_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_2_INDEX, &FW_ERROR_VECTOR_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CFG_TABLE", 1, CORE_2_INDEX, &IPTV_CFG_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_2_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL_BUDGET_REMAINDER", 1, CORE_2_INDEX, &EMAC_FLOW_CTRL_BUDGET_REMAINDER_2, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "MULTICAST_KEY_MASK", 1, CORE_2_INDEX, &MULTICAST_KEY_MASK_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_SHADOW_RD_IDX", 1, CORE_2_INDEX, &CPU_RECYCLE_SHADOW_RD_IDX_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_SHADOW_WR_IDX", 1, CORE_2_INDEX, &CPU_RECYCLE_SHADOW_WR_IDX_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_CFG", 1, CORE_2_INDEX, &INGRESS_FILTER_CFG_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_NEXT_PTR_TABLE", 1, CORE_2_INDEX, &CPU_RECYCLE_NEXT_PTR_TABLE_2, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_TABLE", 1, CORE_3_INDEX, &RX_FLOW_TABLE_3, 320, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_TABLE", 1, CORE_3_INDEX, &VPORT_CFG_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_GEM_TABLE", 1, CORE_3_INDEX, &PBIT_TO_GEM_TABLE_3, 32, 8, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_SCRATCHPAD", 1, CORE_3_INDEX, &CPU_RX_SCRATCHPAD_3, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CMD_TABLE", 1, CORE_3_INDEX, &TCAM_IC_CMD_TABLE_3, 7, 16, 1 },
#endif
#if defined BCM6856
	{ "PORT_MAC", 1, CORE_3_INDEX, &PORT_MAC_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_QUEUE_TABLE", 1, CORE_3_INDEX, &TC_TO_QUEUE_TABLE_3, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_CONTEXT_DDR_ADDR", 1, CORE_3_INDEX, &RX_FLOW_CONTEXT_DDR_ADDR_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_DSCP_TO_PBITS_TABLE", 1, CORE_3_INDEX, &VPORT_TO_DSCP_TO_PBITS_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_3_INDEX, &AQM_ENABLE_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_REPLY", 1, CORE_3_INDEX, &MCAST_PROCESSING_TASKS_REPLY_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PROCESSING_GROUP_PARAMS_GEN4_TABLE", 1, CORE_3_INDEX, &PROCESSING_GROUP_PARAMS_GEN4_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DSCP_TO_PBITS_MAP_TABLE", 1, CORE_3_INDEX, &DSCP_TO_PBITS_MAP_TABLE_3, 4, 64, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_QUEUE_TABLE", 1, CORE_3_INDEX, &PBIT_TO_QUEUE_TABLE_3, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_FPM_TOKENS_READ_FLAG", 1, CORE_3_INDEX, &MCAST_FPM_TOKENS_READ_FLAG_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_3_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_3_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_LOCK", 1, CORE_3_INDEX, &MCAST_PROCESSING_TASKS_LOCK_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_FEED_RING_CACHE_TABLE", 1, CORE_3_INDEX, &CPU_FEED_RING_CACHE_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_METER_TABLE", 1, CORE_3_INDEX, &CPU_RX_METER_TABLE_3, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_PACKET_BUFFER", 1, CORE_3_INDEX, &DS_PACKET_BUFFER_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_3_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_3, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_SRAM_PD_FIFO", 1, CORE_3_INDEX, &CPU_RECYCLE_SRAM_PD_FIFO_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_FLOW_TABLE", 1, CORE_3_INDEX, &TX_FLOW_TABLE_3, 192, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_TASKS_LIMIT", 1, CORE_3_INDEX, &MCAST_BBH_OVERRUN_TASKS_LIMIT_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RING_INTERRUPT_COUNTER_TABLE", 1, CORE_3_INDEX, &CPU_RING_INTERRUPT_COUNTER_TABLE_3, 24, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_DSCP_TO_PBITS_TABLE", 1, CORE_3_INDEX, &GLOBAL_DSCP_TO_PBITS_TABLE_3, 1, 64, 1 },
#endif
#if defined BCM6856
	{ "POLICER_PARAMS_TABLE", 1, CORE_3_INDEX, &POLICER_PARAMS_TABLE_3, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_3_INDEX, &FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_3, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_DST_IP_LKP_TABLE", 1, CORE_3_INDEX, &TUNNEL_DST_IP_LKP_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_COPY_PD_FIFO_TABLE", 1, CORE_3_INDEX, &CPU_RX_COPY_PD_FIFO_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "CSO_CONTEXT_TABLE", 1, CORE_3_INDEX, &CSO_CONTEXT_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT", 1, CORE_3_INDEX, &MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT", 1, CORE_3_INDEX, &MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_3_INDEX, &LOOPBACK_QUEUE_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_3_INDEX, &INGRESS_FILTER_PROFILE_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_EX_TABLE", 1, CORE_3_INDEX, &VPORT_CFG_EX_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REASON_TO_METER_TABLE", 1, CORE_3_INDEX, &CPU_REASON_TO_METER_TABLE_3, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL", 1, CORE_3_INDEX, &EMAC_FLOW_CTRL_3, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_COUNTERS_GENERIC_CONTEXT", 1, CORE_3_INDEX, &NATC_COUNTERS_GENERIC_CONTEXT_3, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_3_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_3, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_3_INDEX, &NATC_TBL_CFG_3, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_3_INDEX, &NULL_BUFFER_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_INTERRUPT_COALESCING_TABLE", 1, CORE_3_INDEX, &CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_FLOW_IDX", 1, CORE_3_INDEX, &VPORT_TO_FLOW_IDX_3, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "IPV6_HOST_ADDRESS_CRC_TABLE", 1, CORE_3_INDEX, &IPV6_HOST_ADDRESS_CRC_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_3_INDEX, &REGISTERS_BUFFER_3, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CFG_TABLE", 1, CORE_3_INDEX, &TCAM_IC_CFG_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REASON_AND_VPORT_TO_METER_TABLE", 1, CORE_3_INDEX, &CPU_REASON_AND_VPORT_TO_METER_TABLE_3, 48, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_FEED_RING_DESCRIPTOR_TABLE", 1, CORE_3_INDEX, &CPU_FEED_RING_DESCRIPTOR_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_ACTION_GPE_HANDLER_PTR_TABLE", 1, CORE_3_INDEX, &VLAN_ACTION_GPE_HANDLER_PTR_TABLE_3, 17, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_3_INDEX, &AQM_NUM_QUEUES_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_INTERRUPT_SCRATCH", 1, CORE_3_INDEX, &CPU_RX_INTERRUPT_SCRATCH_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR", 1, CORE_3_INDEX, &CPU_RECYCLE_INTERRUPT_ID_DDR_ADDR_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_COPY_INT_SCRATCHPAD", 1, CORE_3_INDEX, &CPU_RX_COPY_INT_SCRATCHPAD_3, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_TABLE", 1, CORE_3_INDEX, &TUNNEL_TABLE_3, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_FEED_RING_RSV_TABLE", 1, CORE_3_INDEX, &CPU_FEED_RING_RSV_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE", 1, CORE_3_INDEX, &DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "IPV4_HOST_ADDRESS_TABLE", 1, CORE_3_INDEX, &IPV4_HOST_ADDRESS_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "UPDATE_FIFO_TABLE", 1, CORE_3_INDEX, &UPDATE_FIFO_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_3_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_COPY_UPDATE_FIFO_TABLE", 1, CORE_3_INDEX, &CPU_RX_COPY_UPDATE_FIFO_TABLE_3, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_3_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD", 1, CORE_3_INDEX, &CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_3, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "PD_FIFO_TABLE", 1, CORE_3_INDEX, &PD_FIFO_TABLE_3, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_PBIT_TO_DISCARD_PRIO_VECTOR", 1, CORE_3_INDEX, &VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_LOCAL_SCRATCH", 1, CORE_3_INDEX, &CPU_RX_LOCAL_SCRATCH_3, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TABLE", 1, CORE_3_INDEX, &DEFAULT_VLAN_VID_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REASON_TO_TC", 1, CORE_3_INDEX, &CPU_REASON_TO_TC_3, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_CPU_RXQ", 1, CORE_3_INDEX, &TC_TO_CPU_RXQ_3, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "EXC_TC_TO_CPU_RXQ", 1, CORE_3_INDEX, &EXC_TC_TO_CPU_RXQ_3, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_3_INDEX, &DEBUG_SCRATCHPAD_3, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "BRIDGE_CFG_TABLE", 1, CORE_3_INDEX, &BRIDGE_CFG_TABLE_3, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_3_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE", 1, CORE_3_INDEX, &WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE_3, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_3_INDEX, &TASK_IDX_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_3_INDEX, &BITS_CALC_MASKS_TABLE_3, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_COPY_DISPATCHER_CREDIT_TABLE", 1, CORE_3_INDEX, &CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_3, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_ENABLE", 1, CORE_3_INDEX, &GLOBAL_PBIT_TO_DSCP_ENABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_3_INDEX, &LOOPBACK_WAN_FLOW_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FORCE_DSCP", 1, CORE_3_INDEX, &FORCE_DSCP_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_3_INDEX, &CORE_ID_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CONFIGURATION_TABLE", 1, CORE_3_INDEX, &IPTV_CONFIGURATION_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_3_INDEX, &RX_MIRRORING_CONFIGURATION_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_3_INDEX, &TX_MIRRORING_CONFIGURATION_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_3_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IC_MCAST_ENABLE", 1, CORE_3_INDEX, &IC_MCAST_ENABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_3_INDEX, &SRAM_DUMMY_STORE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_DONT_DROP_RATIO", 1, CORE_3_INDEX, &INGRESS_QOS_DONT_DROP_RATIO_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_WAN_UNTAGGED_PRIORITY", 1, CORE_3_INDEX, &INGRESS_QOS_WAN_UNTAGGED_PRIORITY_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_VPORT_TO_METER_TABLE", 1, CORE_3_INDEX, &CPU_VPORT_TO_METER_TABLE_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_TABLE", 1, CORE_3_INDEX, &GLOBAL_PBIT_TO_DSCP_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_FEED_RING_CACHE_OFFSET", 1, CORE_3_INDEX, &CPU_FEED_RING_CACHE_OFFSET_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REDIRECT_MODE", 1, CORE_3_INDEX, &CPU_REDIRECT_MODE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CSO_DISABLE", 1, CORE_3_INDEX, &CSO_DISABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_3_INDEX, &DEBUG_PRINT_CORE_LOCK_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_3_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_3_INDEX, &INGRESS_FILTER_1588_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_3_INDEX, &RX_MIRRORING_DIRECT_ENABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_3_INDEX, &FPM_GLOBAL_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RX_INTERRUPT_ID_DDR_ADDR", 1, CORE_3_INDEX, &CPU_RX_INTERRUPT_ID_DDR_ADDR_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_CPU_OBJ", 1, CORE_3_INDEX, &VPORT_TO_CPU_OBJ_3, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_RING_DESCRIPTOR_TABLE", 1, CORE_3_INDEX, &CPU_RECYCLE_RING_DESCRIPTOR_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE", 1, CORE_3_INDEX, &CPU_RECYCLE_RING_INDEX_DDR_ADDR_TABLE_3, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_INTERRUPT_COALESCING_TABLE", 1, CORE_3_INDEX, &CPU_INTERRUPT_COALESCING_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_PD_SCRATCH", 1, CORE_3_INDEX, &CPU_RECYCLE_PD_SCRATCH_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SYSTEM_CONFIGURATION", 1, CORE_3_INDEX, &SYSTEM_CONFIGURATION_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_3_INDEX, &DEBUG_PRINT_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GDX_PARAMS_TABLE", 1, CORE_3_INDEX, &GDX_PARAMS_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_PACKET_BASED_MAPPING", 1, CORE_3_INDEX, &INGRESS_PACKET_BASED_MAPPING_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE", 1, CORE_3_INDEX, &CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_HEADER_PSRAM_BUFFER", 1, CORE_3_INDEX, &TUNNEL_HEADER_PSRAM_BUFFER_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_FEED_RING_INDEX_DDR_ADDR_TABLE", 1, CORE_3_INDEX, &CPU_FEED_RING_INDEX_DDR_ADDR_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_INTERRUPT_SCRATCH", 1, CORE_3_INDEX, &CPU_RECYCLE_INTERRUPT_SCRATCH_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNELS_PARSING_CFG", 1, CORE_3_INDEX, &TUNNELS_PARSING_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_3_INDEX, &FW_ERROR_VECTOR_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NAT_CACHE_CFG", 1, CORE_3_INDEX, &NAT_CACHE_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CFG_TABLE", 1, CORE_3_INDEX, &IPTV_CFG_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_3_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MULTICAST_KEY_MASK", 1, CORE_3_INDEX, &MULTICAST_KEY_MASK_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_SHADOW_RD_IDX", 1, CORE_3_INDEX, &CPU_RECYCLE_SHADOW_RD_IDX_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_SHADOW_WR_IDX", 1, CORE_3_INDEX, &CPU_RECYCLE_SHADOW_WR_IDX_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_CFG", 1, CORE_3_INDEX, &INGRESS_FILTER_CFG_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RECYCLE_NEXT_PTR_TABLE", 1, CORE_3_INDEX, &CPU_RECYCLE_NEXT_PTR_TABLE_3, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_RING_DESCRIPTORS_TABLE", 1, CORE_3_INDEX, &CPU_RING_DESCRIPTORS_TABLE_3, 24, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_TABLE", 1, CORE_4_INDEX, &RX_FLOW_TABLE_4, 320, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_TABLE", 1, CORE_4_INDEX, &VPORT_CFG_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_GEM_TABLE", 1, CORE_4_INDEX, &PBIT_TO_GEM_TABLE_4, 32, 8, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_QUEUE_TABLE", 1, CORE_4_INDEX, &TC_TO_QUEUE_TABLE_4, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_CONTEXT_DDR_ADDR", 1, CORE_4_INDEX, &RX_FLOW_CONTEXT_DDR_ADDR_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_DSCP_TO_PBITS_TABLE", 1, CORE_4_INDEX, &VPORT_TO_DSCP_TO_PBITS_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_4_INDEX, &AQM_ENABLE_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PORT_MAC", 1, CORE_4_INDEX, &PORT_MAC_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PROCESSING_GROUP_PARAMS_GEN4_TABLE", 1, CORE_4_INDEX, &PROCESSING_GROUP_PARAMS_GEN4_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DSCP_TO_PBITS_MAP_TABLE", 1, CORE_4_INDEX, &DSCP_TO_PBITS_MAP_TABLE_4, 4, 64, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_QUEUE_TABLE", 1, CORE_4_INDEX, &PBIT_TO_QUEUE_TABLE_4, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_FPM_TOKENS_READ_FLAG", 1, CORE_4_INDEX, &MCAST_FPM_TOKENS_READ_FLAG_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_4_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_4_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_REPLY", 1, CORE_4_INDEX, &MCAST_PROCESSING_TASKS_REPLY_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_LOCK", 1, CORE_4_INDEX, &MCAST_PROCESSING_TASKS_LOCK_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_TASKS_LIMIT", 1, CORE_4_INDEX, &MCAST_BBH_OVERRUN_TASKS_LIMIT_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_FLOW_TABLE", 1, CORE_4_INDEX, &TX_FLOW_TABLE_4, 192, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_DSCP_TO_PBITS_TABLE", 1, CORE_4_INDEX, &GLOBAL_DSCP_TO_PBITS_TABLE_4, 1, 64, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_4_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_4, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CMD_TABLE", 1, CORE_4_INDEX, &TCAM_IC_CMD_TABLE_4, 7, 16, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_DST_IP_LKP_TABLE", 1, CORE_4_INDEX, &TUNNEL_DST_IP_LKP_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_PACKET_BUFFER", 1, CORE_4_INDEX, &DS_PACKET_BUFFER_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "POLICER_PARAMS_TABLE", 1, CORE_4_INDEX, &POLICER_PARAMS_TABLE_4, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_4_INDEX, &FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_4, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_ACTION_GPE_HANDLER_PTR_TABLE", 1, CORE_4_INDEX, &VLAN_ACTION_GPE_HANDLER_PTR_TABLE_4, 17, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT", 1, CORE_4_INDEX, &MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT", 1, CORE_4_INDEX, &MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_4_INDEX, &AQM_NUM_QUEUES_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_4_INDEX, &NULL_BUFFER_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_4_INDEX, &LOOPBACK_QUEUE_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_4_INDEX, &INGRESS_FILTER_PROFILE_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_EX_TABLE", 1, CORE_4_INDEX, &VPORT_CFG_EX_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_TABLE", 1, CORE_4_INDEX, &TUNNEL_TABLE_4, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_4_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_4, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL", 1, CORE_4_INDEX, &EMAC_FLOW_CTRL_4, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_COUNTERS_GENERIC_CONTEXT", 1, CORE_4_INDEX, &NATC_COUNTERS_GENERIC_CONTEXT_4, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_4_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_4_INDEX, &NATC_TBL_CFG_4, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_4_INDEX, &REGISTERS_BUFFER_4, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "SYSTEM_CONFIGURATION", 1, CORE_4_INDEX, &SYSTEM_CONFIGURATION_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_ENABLE", 1, CORE_4_INDEX, &GLOBAL_PBIT_TO_DSCP_ENABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_4_INDEX, &LOOPBACK_WAN_FLOW_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FORCE_DSCP", 1, CORE_4_INDEX, &FORCE_DSCP_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BRIDGE_CFG_TABLE", 1, CORE_4_INDEX, &BRIDGE_CFG_TABLE_4, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CFG_TABLE", 1, CORE_4_INDEX, &TCAM_IC_CFG_TABLE_4, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_4_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_4_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE", 1, CORE_4_INDEX, &DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_4_INDEX, &DEBUG_SCRATCHPAD_4, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CONFIGURATION_TABLE", 1, CORE_4_INDEX, &IPTV_CONFIGURATION_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_4_INDEX, &TASK_IDX_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_PBIT_TO_DISCARD_PRIO_VECTOR", 1, CORE_4_INDEX, &VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_4_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_4_INDEX, &CORE_ID_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IC_MCAST_ENABLE", 1, CORE_4_INDEX, &IC_MCAST_ENABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_4_INDEX, &RX_MIRRORING_CONFIGURATION_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TABLE", 1, CORE_4_INDEX, &DEFAULT_VLAN_VID_TABLE_4, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_4_INDEX, &FPM_GLOBAL_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_4_INDEX, &TX_MIRRORING_CONFIGURATION_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_4_INDEX, &SRAM_DUMMY_STORE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_DONT_DROP_RATIO", 1, CORE_4_INDEX, &INGRESS_QOS_DONT_DROP_RATIO_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_PACKET_BASED_MAPPING", 1, CORE_4_INDEX, &INGRESS_PACKET_BASED_MAPPING_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_WAN_UNTAGGED_PRIORITY", 1, CORE_4_INDEX, &INGRESS_QOS_WAN_UNTAGGED_PRIORITY_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REDIRECT_MODE", 1, CORE_4_INDEX, &CPU_REDIRECT_MODE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_4_INDEX, &DEBUG_PRINT_CORE_LOCK_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_4_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_4_INDEX, &BITS_CALC_MASKS_TABLE_4, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_TABLE", 1, CORE_4_INDEX, &GLOBAL_PBIT_TO_DSCP_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_4_INDEX, &INGRESS_FILTER_1588_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_4_INDEX, &RX_MIRRORING_DIRECT_ENABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_4_INDEX, &DEBUG_PRINT_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GDX_PARAMS_TABLE", 1, CORE_4_INDEX, &GDX_PARAMS_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_HEADER_PSRAM_BUFFER", 1, CORE_4_INDEX, &TUNNEL_HEADER_PSRAM_BUFFER_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNELS_PARSING_CFG", 1, CORE_4_INDEX, &TUNNELS_PARSING_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NAT_CACHE_CFG", 1, CORE_4_INDEX, &NAT_CACHE_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CFG_TABLE", 1, CORE_4_INDEX, &IPTV_CFG_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_4_INDEX, &FW_ERROR_VECTOR_TABLE_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_4_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_CFG", 1, CORE_4_INDEX, &INGRESS_FILTER_CFG_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MULTICAST_KEY_MASK", 1, CORE_4_INDEX, &MULTICAST_KEY_MASK_4, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_RATE_LIMITER_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_RATE_LIMITER_TABLE_5, 66, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_5_INDEX, &AQM_ENABLE_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PORT_MAC", 1, CORE_5_INDEX, &PORT_MAC_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_TABLE", 1, CORE_5_INDEX, &VPORT_CFG_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_GEM_TABLE", 1, CORE_5_INDEX, &PBIT_TO_GEM_TABLE_5, 32, 8, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CMD_TABLE", 1, CORE_5_INDEX, &TCAM_IC_CMD_TABLE_5, 7, 16, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_REPLY", 1, CORE_5_INDEX, &MCAST_PROCESSING_TASKS_REPLY_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_PD_FIFO_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_PD_FIFO_TABLE_5, 64, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_TABLE", 1, CORE_5_INDEX, &RX_FLOW_TABLE_5, 320, 1, 1 },
#endif
#if defined BCM6856
	{ "PROCESSING_GROUP_PARAMS_GEN4_TABLE", 1, CORE_5_INDEX, &PROCESSING_GROUP_PARAMS_GEN4_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DSCP_TO_PBITS_MAP_TABLE", 1, CORE_5_INDEX, &DSCP_TO_PBITS_MAP_TABLE_5, 4, 64, 1 },
#endif
#if defined BCM6856
	{ "DS_PACKET_BUFFER", 1, CORE_5_INDEX, &DS_PACKET_BUFFER_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_QUEUE_TABLE", 1, CORE_5_INDEX, &TC_TO_QUEUE_TABLE_5, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_CONTEXT_DDR_ADDR", 1, CORE_5_INDEX, &RX_FLOW_CONTEXT_DDR_ADDR_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_DSCP_TO_PBITS_TABLE", 1, CORE_5_INDEX, &VPORT_TO_DSCP_TO_PBITS_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_5_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_LOCK", 1, CORE_5_INDEX, &MCAST_PROCESSING_TASKS_LOCK_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_TASKS_LIMIT", 1, CORE_5_INDEX, &MCAST_BBH_OVERRUN_TASKS_LIMIT_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_SQ_TABLE", 1, CORE_5_INDEX, &AQM_SQ_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_QUEUE_TABLE", 1, CORE_5_INDEX, &PBIT_TO_QUEUE_TABLE_5, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_FPM_TOKENS_READ_FLAG", 1, CORE_5_INDEX, &MCAST_FPM_TOKENS_READ_FLAG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_5_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_5_INDEX, &FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_DSCP_TO_PBITS_TABLE", 1, CORE_5_INDEX, &GLOBAL_DSCP_TO_PBITS_TABLE_5, 1, 64, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_EX_TABLE", 1, CORE_5_INDEX, &VPORT_CFG_EX_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_DST_IP_LKP_TABLE", 1, CORE_5_INDEX, &TUNNEL_DST_IP_LKP_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_FLOW_TABLE", 1, CORE_5_INDEX, &TX_FLOW_TABLE_5, 192, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_5_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_5, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "POLICER_PARAMS_TABLE", 1, CORE_5_INDEX, &POLICER_PARAMS_TABLE_5, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "SQ_TX_QUEUE_DROP_TABLE", 1, CORE_5_INDEX, &SQ_TX_QUEUE_DROP_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_5_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "CODEL_BIAS_SLOPE_TABLE", 1, CORE_5_INDEX, &CODEL_BIAS_SLOPE_TABLE_5, 11, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT", 1, CORE_5_INDEX, &MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT", 1, CORE_5_INDEX, &MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_5_INDEX, &LOOPBACK_QUEUE_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_5_INDEX, &INGRESS_FILTER_PROFILE_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_COUNTERS_GENERIC_CONTEXT", 1, CORE_5_INDEX, &NATC_COUNTERS_GENERIC_CONTEXT_5, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL", 1, CORE_5_INDEX, &EMAC_FLOW_CTRL_5, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_5_INDEX, &REGISTERS_BUFFER_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_5_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_5_INDEX, &NATC_TBL_CFG_5, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_AQM_QUEUE_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_AQM_QUEUE_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_5_INDEX, &NULL_BUFFER_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_FLUSH_CFG_FW_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CFG_TABLE", 1, CORE_5_INDEX, &TCAM_IC_CFG_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR", 1, CORE_5_INDEX, &SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_5, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_5_INDEX, &AQM_NUM_QUEUES_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_BUDGET_ALLOCATION_TIMER_VALUE", 1, CORE_5_INDEX, &SERVICE_QUEUES_BUDGET_ALLOCATION_TIMER_VALUE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_SCRATCHPAD", 1, CORE_5_INDEX, &SERVICE_QUEUES_SCRATCHPAD_5, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_ACTION_GPE_HANDLER_PTR_TABLE", 1, CORE_5_INDEX, &VLAN_ACTION_GPE_HANDLER_PTR_TABLE_5, 17, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_CONTEXT", 1, CORE_5_INDEX, &SERVICE_QUEUES_CONTEXT_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_5_INDEX, &TASK_IDX_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SYSTEM_CONFIGURATION", 1, CORE_5_INDEX, &SYSTEM_CONFIGURATION_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_ENABLE", 1, CORE_5_INDEX, &GLOBAL_PBIT_TO_DSCP_ENABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SCHEDULING_FLUSH_GLOBAL_CFG", 1, CORE_5_INDEX, &SCHEDULING_FLUSH_GLOBAL_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_5_INDEX, &LOOPBACK_WAN_FLOW_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_FLUSH_CFG_CURRENT_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_FLUSH_CFG_CURRENT_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_TABLE", 1, CORE_5_INDEX, &TUNNEL_TABLE_5, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_SCHEDULING_AGGREGATION_CONTEXT_VECTOR", 1, CORE_5_INDEX, &SERVICE_QUEUES_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_5, 5, 1, 1 },
#endif
#if defined BCM6856
	{ "FORCE_DSCP", 1, CORE_5_INDEX, &FORCE_DSCP_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_5_INDEX, &CORE_ID_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_FIRST_QUEUE_MAPPING", 1, CORE_5_INDEX, &SERVICE_QUEUES_FIRST_QUEUE_MAPPING_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_PACKET_BASED_MAPPING", 1, CORE_5_INDEX, &INGRESS_PACKET_BASED_MAPPING_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_5_INDEX, &RX_MIRRORING_CONFIGURATION_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_5_INDEX, &TX_MIRRORING_CONFIGURATION_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE", 1, CORE_5_INDEX, &DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_5_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "BRIDGE_CFG_TABLE", 1, CORE_5_INDEX, &BRIDGE_CFG_TABLE_5, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_UPDATE_FIFO_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_UPDATE_FIFO_TABLE_5, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_PBIT_TO_DISCARD_PRIO_VECTOR", 1, CORE_5_INDEX, &VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_5_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TABLE", 1, CORE_5_INDEX, &DEFAULT_VLAN_VID_TABLE_5, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_5, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_5_INDEX, &DEBUG_SCRATCHPAD_5, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_FLUSH_DISPATCHER_CREDIT_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_FLUSH_DISPATCHER_CREDIT_TABLE_5, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_FLUSH_AGGREGATION_TASK_DISABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_FLUSH_AGGREGATION_TASK_DISABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IC_MCAST_ENABLE", 1, CORE_5_INDEX, &IC_MCAST_ENABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_5_INDEX, &SRAM_DUMMY_STORE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_DONT_DROP_RATIO", 1, CORE_5_INDEX, &INGRESS_QOS_DONT_DROP_RATIO_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE_5, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CONFIGURATION_TABLE", 1, CORE_5_INDEX, &IPTV_CONFIGURATION_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_WAN_UNTAGGED_PRIORITY", 1, CORE_5_INDEX, &INGRESS_QOS_WAN_UNTAGGED_PRIORITY_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REDIRECT_MODE", 1, CORE_5_INDEX, &CPU_REDIRECT_MODE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_5_INDEX, &DEBUG_PRINT_CORE_LOCK_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_5_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_5_INDEX, &BITS_CALC_MASKS_TABLE_5, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_5_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_5_INDEX, &INGRESS_FILTER_1588_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_5_INDEX, &RX_MIRRORING_DIRECT_ENABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_DISPATCHER_CREDIT_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_DISPATCHER_CREDIT_TABLE_5, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_CODEL_DROP_DESCRIPTOR", 1, CORE_5_INDEX, &SERVICE_QUEUES_CODEL_DROP_DESCRIPTOR_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_5_INDEX, &FPM_GLOBAL_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNELS_PARSING_CFG", 1, CORE_5_INDEX, &TUNNELS_PARSING_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_TABLE", 1, CORE_5_INDEX, &GLOBAL_PBIT_TO_DSCP_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_5_INDEX, &DEBUG_PRINT_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GDX_PARAMS_TABLE", 1, CORE_5_INDEX, &GDX_PARAMS_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NAT_CACHE_CFG", 1, CORE_5_INDEX, &NAT_CACHE_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CFG_TABLE", 1, CORE_5_INDEX, &IPTV_CFG_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_HEADER_PSRAM_BUFFER", 1, CORE_5_INDEX, &TUNNEL_HEADER_PSRAM_BUFFER_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_5_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE", 1, CORE_5_INDEX, &SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_CFG", 1, CORE_5_INDEX, &INGRESS_FILTER_CFG_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_5_INDEX, &FW_ERROR_VECTOR_TABLE_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MULTICAST_KEY_MASK", 1, CORE_5_INDEX, &MULTICAST_KEY_MASK_5, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_TABLE", 1, CORE_6_INDEX, &RX_FLOW_TABLE_6, 320, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_TABLE", 1, CORE_6_INDEX, &VPORT_CFG_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_GEM_TABLE", 1, CORE_6_INDEX, &PBIT_TO_GEM_TABLE_6, 32, 8, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_QUEUE_TABLE", 1, CORE_6_INDEX, &TC_TO_QUEUE_TABLE_6, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_CONTEXT_DDR_ADDR", 1, CORE_6_INDEX, &RX_FLOW_CONTEXT_DDR_ADDR_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_DSCP_TO_PBITS_TABLE", 1, CORE_6_INDEX, &VPORT_TO_DSCP_TO_PBITS_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_6_INDEX, &AQM_ENABLE_TABLE_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PORT_MAC", 1, CORE_6_INDEX, &PORT_MAC_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PROCESSING_GROUP_PARAMS_GEN4_TABLE", 1, CORE_6_INDEX, &PROCESSING_GROUP_PARAMS_GEN4_TABLE_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DSCP_TO_PBITS_MAP_TABLE", 1, CORE_6_INDEX, &DSCP_TO_PBITS_MAP_TABLE_6, 4, 64, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_QUEUE_TABLE", 1, CORE_6_INDEX, &PBIT_TO_QUEUE_TABLE_6, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_FPM_TOKENS_READ_FLAG", 1, CORE_6_INDEX, &MCAST_FPM_TOKENS_READ_FLAG_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_6_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_6_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_REPLY", 1, CORE_6_INDEX, &MCAST_PROCESSING_TASKS_REPLY_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_LOCK", 1, CORE_6_INDEX, &MCAST_PROCESSING_TASKS_LOCK_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_TASKS_LIMIT", 1, CORE_6_INDEX, &MCAST_BBH_OVERRUN_TASKS_LIMIT_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_FLOW_TABLE", 1, CORE_6_INDEX, &TX_FLOW_TABLE_6, 192, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_DSCP_TO_PBITS_TABLE", 1, CORE_6_INDEX, &GLOBAL_DSCP_TO_PBITS_TABLE_6, 1, 64, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_6_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_6, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CMD_TABLE", 1, CORE_6_INDEX, &TCAM_IC_CMD_TABLE_6, 7, 16, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_DST_IP_LKP_TABLE", 1, CORE_6_INDEX, &TUNNEL_DST_IP_LKP_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_PACKET_BUFFER", 1, CORE_6_INDEX, &DS_PACKET_BUFFER_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "POLICER_PARAMS_TABLE", 1, CORE_6_INDEX, &POLICER_PARAMS_TABLE_6, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_6_INDEX, &FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_6, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_ACTION_GPE_HANDLER_PTR_TABLE", 1, CORE_6_INDEX, &VLAN_ACTION_GPE_HANDLER_PTR_TABLE_6, 17, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT", 1, CORE_6_INDEX, &MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT", 1, CORE_6_INDEX, &MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_6_INDEX, &AQM_NUM_QUEUES_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_6_INDEX, &NULL_BUFFER_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_6_INDEX, &LOOPBACK_QUEUE_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_6_INDEX, &INGRESS_FILTER_PROFILE_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_EX_TABLE", 1, CORE_6_INDEX, &VPORT_CFG_EX_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_TABLE", 1, CORE_6_INDEX, &TUNNEL_TABLE_6, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_6_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_6, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL", 1, CORE_6_INDEX, &EMAC_FLOW_CTRL_6, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_COUNTERS_GENERIC_CONTEXT", 1, CORE_6_INDEX, &NATC_COUNTERS_GENERIC_CONTEXT_6, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_6_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_6_INDEX, &NATC_TBL_CFG_6, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_6_INDEX, &REGISTERS_BUFFER_6, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "SYSTEM_CONFIGURATION", 1, CORE_6_INDEX, &SYSTEM_CONFIGURATION_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_ENABLE", 1, CORE_6_INDEX, &GLOBAL_PBIT_TO_DSCP_ENABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_6_INDEX, &LOOPBACK_WAN_FLOW_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FORCE_DSCP", 1, CORE_6_INDEX, &FORCE_DSCP_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BRIDGE_CFG_TABLE", 1, CORE_6_INDEX, &BRIDGE_CFG_TABLE_6, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CFG_TABLE", 1, CORE_6_INDEX, &TCAM_IC_CFG_TABLE_6, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_6_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_6_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE", 1, CORE_6_INDEX, &DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_6_INDEX, &DEBUG_SCRATCHPAD_6, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CONFIGURATION_TABLE", 1, CORE_6_INDEX, &IPTV_CONFIGURATION_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_6_INDEX, &TASK_IDX_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_PBIT_TO_DISCARD_PRIO_VECTOR", 1, CORE_6_INDEX, &VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_6_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_6_INDEX, &CORE_ID_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IC_MCAST_ENABLE", 1, CORE_6_INDEX, &IC_MCAST_ENABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_6_INDEX, &RX_MIRRORING_CONFIGURATION_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TABLE", 1, CORE_6_INDEX, &DEFAULT_VLAN_VID_TABLE_6, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_6_INDEX, &FPM_GLOBAL_CFG_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_6_INDEX, &TX_MIRRORING_CONFIGURATION_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_6_INDEX, &SRAM_DUMMY_STORE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_DONT_DROP_RATIO", 1, CORE_6_INDEX, &INGRESS_QOS_DONT_DROP_RATIO_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_PACKET_BASED_MAPPING", 1, CORE_6_INDEX, &INGRESS_PACKET_BASED_MAPPING_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_WAN_UNTAGGED_PRIORITY", 1, CORE_6_INDEX, &INGRESS_QOS_WAN_UNTAGGED_PRIORITY_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REDIRECT_MODE", 1, CORE_6_INDEX, &CPU_REDIRECT_MODE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_6_INDEX, &DEBUG_PRINT_CORE_LOCK_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_6_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_6_INDEX, &BITS_CALC_MASKS_TABLE_6, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_TABLE", 1, CORE_6_INDEX, &GLOBAL_PBIT_TO_DSCP_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_6_INDEX, &INGRESS_FILTER_1588_CFG_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_6_INDEX, &RX_MIRRORING_DIRECT_ENABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_6_INDEX, &DEBUG_PRINT_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GDX_PARAMS_TABLE", 1, CORE_6_INDEX, &GDX_PARAMS_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_HEADER_PSRAM_BUFFER", 1, CORE_6_INDEX, &TUNNEL_HEADER_PSRAM_BUFFER_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNELS_PARSING_CFG", 1, CORE_6_INDEX, &TUNNELS_PARSING_CFG_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NAT_CACHE_CFG", 1, CORE_6_INDEX, &NAT_CACHE_CFG_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CFG_TABLE", 1, CORE_6_INDEX, &IPTV_CFG_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_6_INDEX, &FW_ERROR_VECTOR_TABLE_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_6_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_CFG", 1, CORE_6_INDEX, &INGRESS_FILTER_CFG_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MULTICAST_KEY_MASK", 1, CORE_6_INDEX, &MULTICAST_KEY_MASK_6, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_TABLE", 1, CORE_7_INDEX, &RX_FLOW_TABLE_7, 320, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_TABLE", 1, CORE_7_INDEX, &VPORT_CFG_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_GEM_TABLE", 1, CORE_7_INDEX, &PBIT_TO_GEM_TABLE_7, 32, 8, 1 },
#endif
#if defined BCM6856
	{ "TC_TO_QUEUE_TABLE", 1, CORE_7_INDEX, &TC_TO_QUEUE_TABLE_7, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_FLOW_CONTEXT_DDR_ADDR", 1, CORE_7_INDEX, &RX_FLOW_CONTEXT_DDR_ADDR_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_DSCP_TO_PBITS_TABLE", 1, CORE_7_INDEX, &VPORT_TO_DSCP_TO_PBITS_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_ENABLE_TABLE", 1, CORE_7_INDEX, &AQM_ENABLE_TABLE_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PORT_MAC", 1, CORE_7_INDEX, &PORT_MAC_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "PROCESSING_GROUP_PARAMS_GEN4_TABLE", 1, CORE_7_INDEX, &PROCESSING_GROUP_PARAMS_GEN4_TABLE_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "DSCP_TO_PBITS_MAP_TABLE", 1, CORE_7_INDEX, &DSCP_TO_PBITS_MAP_TABLE_7, 4, 64, 1 },
#endif
#if defined BCM6856
	{ "PBIT_TO_QUEUE_TABLE", 1, CORE_7_INDEX, &PBIT_TO_QUEUE_TABLE_7, 65, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_FPM_TOKENS_READ_FLAG", 1, CORE_7_INDEX, &MCAST_FPM_TOKENS_READ_FLAG_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_7_INDEX, &FPM_POOL_NUMBER_MAPPING_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_GLOBAL_REGISTERS_INIT", 1, CORE_7_INDEX, &RUNNER_GLOBAL_REGISTERS_INIT_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_REPLY", 1, CORE_7_INDEX, &MCAST_PROCESSING_TASKS_REPLY_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_PROCESSING_TASKS_LOCK", 1, CORE_7_INDEX, &MCAST_PROCESSING_TASKS_LOCK_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_TASKS_LIMIT", 1, CORE_7_INDEX, &MCAST_BBH_OVERRUN_TASKS_LIMIT_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_FLOW_TABLE", 1, CORE_7_INDEX, &TX_FLOW_TABLE_7, 192, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_DSCP_TO_PBITS_TABLE", 1, CORE_7_INDEX, &GLOBAL_DSCP_TO_PBITS_TABLE_7, 1, 64, 1 },
#endif
#if defined BCM6856
	{ "RUNNER_PROFILING_TRACE_BUFFER", 1, CORE_7_INDEX, &RUNNER_PROFILING_TRACE_BUFFER_7, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CMD_TABLE", 1, CORE_7_INDEX, &TCAM_IC_CMD_TABLE_7, 7, 16, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_DST_IP_LKP_TABLE", 1, CORE_7_INDEX, &TUNNEL_DST_IP_LKP_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DS_PACKET_BUFFER", 1, CORE_7_INDEX, &DS_PACKET_BUFFER_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "POLICER_PARAMS_TABLE", 1, CORE_7_INDEX, &POLICER_PARAMS_TABLE_7, 80, 1, 1 },
#endif
#if defined BCM6856
	{ "FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE", 1, CORE_7_INDEX, &FLOODING_HEADER_FPM_POOL_NUMBER_MAPPING_TABLE_7, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "VLAN_ACTION_GPE_HANDLER_PTR_TABLE", 1, CORE_7_INDEX, &VLAN_ACTION_GPE_HANDLER_PTR_TABLE_7, 17, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT", 1, CORE_7_INDEX, &MCAST_BBH_OVERRUN_MAX_TASKS_LIMIT_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT", 1, CORE_7_INDEX, &MCAST_BBH_OVERRUN_MIN_TASKS_LIMIT_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "AQM_NUM_QUEUES", 1, CORE_7_INDEX, &AQM_NUM_QUEUES_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NULL_BUFFER", 1, CORE_7_INDEX, &NULL_BUFFER_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_QUEUE_TABLE", 1, CORE_7_INDEX, &LOOPBACK_QUEUE_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_PROFILE_TABLE", 1, CORE_7_INDEX, &INGRESS_FILTER_PROFILE_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_CFG_EX_TABLE", 1, CORE_7_INDEX, &VPORT_CFG_EX_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_TABLE", 1, CORE_7_INDEX, &TUNNEL_TABLE_7, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_LOOKUP_PORT_MAPPING_TABLE", 1, CORE_7_INDEX, &VPORT_TO_LOOKUP_PORT_MAPPING_TABLE_7, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "EMAC_FLOW_CTRL", 1, CORE_7_INDEX, &EMAC_FLOW_CTRL_7, 6, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_COUNTERS_GENERIC_CONTEXT", 1, CORE_7_INDEX, &NATC_COUNTERS_GENERIC_CONTEXT_7, 128, 1, 1 },
#endif
#if defined BCM6856
	{ "HOST_TX_TRUNCATE_MIRRORING_TABLE", 1, CORE_7_INDEX, &HOST_TX_TRUNCATE_MIRRORING_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "NATC_TBL_CFG", 1, CORE_7_INDEX, &NATC_TBL_CFG_7, 3, 1, 1 },
#endif
#if defined BCM6856
	{ "REGISTERS_BUFFER", 1, CORE_7_INDEX, &REGISTERS_BUFFER_7, 32, 1, 1 },
#endif
#if defined BCM6856
	{ "SYSTEM_CONFIGURATION", 1, CORE_7_INDEX, &SYSTEM_CONFIGURATION_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_ENABLE", 1, CORE_7_INDEX, &GLOBAL_PBIT_TO_DSCP_ENABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "LOOPBACK_WAN_FLOW_TABLE", 1, CORE_7_INDEX, &LOOPBACK_WAN_FLOW_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FORCE_DSCP", 1, CORE_7_INDEX, &FORCE_DSCP_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BRIDGE_CFG_TABLE", 1, CORE_7_INDEX, &BRIDGE_CFG_TABLE_7, 2, 1, 1 },
#endif
#if defined BCM6856
	{ "TCAM_IC_CFG_TABLE", 1, CORE_7_INDEX, &TCAM_IC_CFG_TABLE_7, 8, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_TO_RL_OVERHEAD_TABLE", 1, CORE_7_INDEX, &VPORT_TO_RL_OVERHEAD_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_L2_REASON_TABLE", 1, CORE_7_INDEX, &INGRESS_FILTER_L2_REASON_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE", 1, CORE_7_INDEX, &DEFAULT_VLAN_VID_TO_TX_VLAN_CNTR_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_SCRATCHPAD", 1, CORE_7_INDEX, &DEBUG_SCRATCHPAD_7, 12, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CONFIGURATION_TABLE", 1, CORE_7_INDEX, &IPTV_CONFIGURATION_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TASK_IDX", 1, CORE_7_INDEX, &TASK_IDX_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "VPORT_PBIT_TO_DISCARD_PRIO_VECTOR", 1, CORE_7_INDEX, &VPORT_PBIT_TO_DISCARD_PRIO_VECTOR_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "GENERAL_QUEUE_DYNAMIC_MNG_TABLE", 1, CORE_7_INDEX, &GENERAL_QUEUE_DYNAMIC_MNG_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CORE_ID_TABLE", 1, CORE_7_INDEX, &CORE_ID_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IC_MCAST_ENABLE", 1, CORE_7_INDEX, &IC_MCAST_ENABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_CONFIGURATION", 1, CORE_7_INDEX, &RX_MIRRORING_CONFIGURATION_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEFAULT_VLAN_VID_TABLE", 1, CORE_7_INDEX, &DEFAULT_VLAN_VID_TABLE_7, 16, 1, 1 },
#endif
#if defined BCM6856
	{ "FPM_GLOBAL_CFG", 1, CORE_7_INDEX, &FPM_GLOBAL_CFG_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TX_MIRRORING_CONFIGURATION", 1, CORE_7_INDEX, &TX_MIRRORING_CONFIGURATION_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "SRAM_DUMMY_STORE", 1, CORE_7_INDEX, &SRAM_DUMMY_STORE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_DONT_DROP_RATIO", 1, CORE_7_INDEX, &INGRESS_QOS_DONT_DROP_RATIO_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_PACKET_BASED_MAPPING", 1, CORE_7_INDEX, &INGRESS_PACKET_BASED_MAPPING_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_QOS_WAN_UNTAGGED_PRIORITY", 1, CORE_7_INDEX, &INGRESS_QOS_WAN_UNTAGGED_PRIORITY_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "CPU_REDIRECT_MODE", 1, CORE_7_INDEX, &CPU_REDIRECT_MODE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_CORE_LOCK", 1, CORE_7_INDEX, &DEBUG_PRINT_CORE_LOCK_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CLASSIFICATION_CFG_TABLE", 1, CORE_7_INDEX, &IPTV_CLASSIFICATION_CFG_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "BITS_CALC_MASKS_TABLE", 1, CORE_7_INDEX, &BITS_CALC_MASKS_TABLE_7, 4, 1, 1 },
#endif
#if defined BCM6856
	{ "GLOBAL_PBIT_TO_DSCP_TABLE", 1, CORE_7_INDEX, &GLOBAL_PBIT_TO_DSCP_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_1588_CFG", 1, CORE_7_INDEX, &INGRESS_FILTER_1588_CFG_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "RX_MIRRORING_DIRECT_ENABLE", 1, CORE_7_INDEX, &RX_MIRRORING_DIRECT_ENABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "DEBUG_PRINT_TABLE", 1, CORE_7_INDEX, &DEBUG_PRINT_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "GDX_PARAMS_TABLE", 1, CORE_7_INDEX, &GDX_PARAMS_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNEL_HEADER_PSRAM_BUFFER", 1, CORE_7_INDEX, &TUNNEL_HEADER_PSRAM_BUFFER_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "TUNNELS_PARSING_CFG", 1, CORE_7_INDEX, &TUNNELS_PARSING_CFG_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "NAT_CACHE_CFG", 1, CORE_7_INDEX, &NAT_CACHE_CFG_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_CFG_TABLE", 1, CORE_7_INDEX, &IPTV_CFG_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "FW_ERROR_VECTOR_TABLE", 1, CORE_7_INDEX, &FW_ERROR_VECTOR_TABLE_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "IPTV_DDR_CTX_TABLE_ADDRESS", 1, CORE_7_INDEX, &IPTV_DDR_CTX_TABLE_ADDRESS_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "INGRESS_FILTER_CFG", 1, CORE_7_INDEX, &INGRESS_FILTER_CFG_7, 1, 1, 1 },
#endif
#if defined BCM6856
	{ "MULTICAST_KEY_MASK", 1, CORE_7_INDEX, &MULTICAST_KEY_MASK_7, 1, 1, 1 },
#endif
};

TABLE_STACK_STRUCT RUNNER_STACK_TABLES[NUMBER_OF_STACK_TABLES] =
{

};
