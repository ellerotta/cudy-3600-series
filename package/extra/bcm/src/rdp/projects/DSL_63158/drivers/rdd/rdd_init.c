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
#include "rdd_defs.h"
#include "rdd_init.h"
#include "rdd_tunnels_parsing.h"
#include "rdd_iptv_processing.h"
#include "XRDP_AG.h"
#include "rdd_map_auto.h"
#include "rdd_ghost_reporting.h"
#include "rdd_scheduling.h"
#include "rdd_common.h"
#include "rdd_cpu.h"
#include "rdp_common.h"
#include "rdd_tuple_lkp.h"
#include "rdd_runner_proj_defs.h"
#include "rdd_tcam_ic.h"
#include "rdd_ingress_filter.h"
#include "rdd_iptv.h"
#include "rdd_multicast_whitelist.h"
#include "rdp_drv_rnr.h"
#include "rdp_drv_qm.h"
#include "rdd_ag_natc.h"
#ifdef CONFIG_DHD_RUNNER
#include "rdd_dhd_helper.h"
#include "rdp_drv_dhd.h"
#endif
#include "rdd_debug.h"
#include "rdd_service_queues.h"

extern uintptr_t xrdp_virt2phys(const ru_block_rec *ru_block, uint8_t addr_idx);
static void rdd_proj_init(rdd_init_params_t *init_params);
static void rdd_actions_proj_init(void);

#ifdef RDP_SIM
extern uint8_t *soc_base_address;
extern uint32_t natc_lkp_table_ptr;
extern int rdd_sim_alloc_segments(void);
extern void rdd_sim_free_segments(void);
#endif

extern int reg_id[32];
#define PER_CONTEXT_WORD_COUNT ((RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) / NUM_OF_MAIN_RUNNER_THREADS)
DEFINE_BDMF_FASTLOCK(int_lock);
DEFINE_BDMF_FASTLOCK(iptv_lock);
DEFINE_BDMF_FASTLOCK(int_lock_irq);

#ifdef USE_BDMF_SHELL
extern int rdd_make_shell_commands(void);
#endif /* USE_BDMF_SHELL */

rdd_vport_id_t rx_flow_to_vport[RX_FLOW_CONTEXTS_NUMBER] = {
    [RDD_WAN_FLOW_PON_START ... RDD_WAN_FLOW_PON_END]  = RDD_PON_WAN_VPORT,
    [RDD_WAN_FLOW_DSL_START ... RDD_WAN_FLOW_DSL_END]  = RDD_DSL_WAN_VPORT,
    [RDD_WAN_FLOW_AE10_START ... RDD_WAN_FLOW_AE10_END]  = RDD_ETH_WAN_VPORT,
    [RDD_WAN_FLOW_AE2P5_START ... RDD_WAN_FLOW_AE2P5_END]  = RDD_ETH_WAN_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN0_VPORT] = RDD_LAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN1_VPORT] = RDD_LAN1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN2_VPORT] = RDD_LAN2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN3_VPORT] = RDD_LAN3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN4_VPORT] = RDD_LAN4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN5_VPORT] = RDD_LAN5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + RDD_LAN6_VPORT] = RDD_LAN6_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_0] = RDD_LAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_1] = RDD_LAN1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_2] = RDD_LAN2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_PON] = RDD_LAN3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_10G] = RDD_LAN4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_2P5] = RDD_LAN5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_DSL] = RDD_LAN6_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU0] = RDD_CPU0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU1] = RDD_CPU1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU2] = RDD_CPU2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU3] = RDD_CPU3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU4] = RDD_CPU4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU5] = RDD_CPU5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU6] = RDD_CPU6_VPORT,
};

uint8_t vport_switch_port_mapping[] = {0, 1, 2, 3, 4, 6};

extern FPM_GLOBAL_CFG_STRUCT g_fpm_hw_cfg;

#if defined(XRDP_PI2) && defined(PI2_ON_SERVICE_QUEUES)
int pi2_shares_ds_and_sq(void);
#endif

int rdd_init(void)
{
#ifdef RDP_SIM
    if (rdd_sim_alloc_segments())
        return -1;
#endif
    return 0;
}

void rdd_exit(void)
{
#ifdef RDP_SIM
    rdd_sim_free_segments();
#endif
}

void rdp_rnr_write_context(void *__to, void *__from, unsigned int __n);

static void rdd_global_registers_init(uint32_t core_index)
{
    static uint32_t global_regs[8] = {};
    uint32_t i;

    /********** Reserved global registers **********/
    /* R5 - ingress qos don't drop counter in processing cores */

    /********** common to all threads on the given core **********/
    global_regs[1] = 1; /* R1 = 1 */
#ifdef BCM6858_A0
    global_regs[3] |= (1 << GLOBAL_CFG_REG_IS_6858A0);
#endif

    for (i = 0; i < 8; ++i)
    {
        RDD_BYTES_4_BITS_WRITE(global_regs[i], (uint8_t *)RDD_RUNNER_GLOBAL_REGISTERS_INIT_PTR(core_index) + (sizeof(BYTES_4_STRUCT) * i));
    }
}

static void image_0_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

    /* Budget allocation */
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] =
        IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN;
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[10]] = IMAGE_0_RATE_LIMITER_VALID_TABLE_DS_ADDRESS;
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);

#ifdef BUFFER_CONGESTION_MGT
    /* BUFFER_CONG_MGT */
    local_regs[IMAGE_0_DS_TM_BUFFER_CONG_MGT_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, buffer_cong_mgt_1st_wakeup_request) << 16;
#endif

#ifdef XRDP_PI2
    /* PI2 probability calculator: thread 2 */
    local_regs[IMAGE_0_DS_TM_PI2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, pi2_calc_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_PI2_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_0_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_PI2_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif

    /* UPDATE_FIFO_READ */
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_ds_start(0) & 0x1f;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, update_fifo_ds_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[13]] = IMAGE_0_DS_TM_EGRESS_PORT_RR_TABLE_ADDRESS | (IMAGE_0_EGRESS_PORT_TO_IMP_PORT_MAPPING_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);

    /* SCHEDULING LAN */
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_EGRESS_PORT_RR_TABLE_ADDRESS | (IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN | (IMAGE_0_ACB_QUEUE_INFLIGHT_DROPCNT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[8]]  = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);

    /* FLUSH TASK */
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[18]] = ((((drv_qm_get_ds_end(0) - (drv_qm_get_ds_start(0) & ~0x1F)) + 8) / 8) << 16) + drv_qm_get_ds_start(0);
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DS_TM_FLUSH << 6));
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_0_DS_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        ((drv_qm_get_ds_start(0) - (drv_qm_get_ds_start(0) % 32)) / 8);

    /* REPORTING  */
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[18]] = drv_qm_get_us_end();
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ghost_reporting_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[15]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, TOTAL_VALID_COUNTER_COUNTER);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[13]] = IMAGE_0_REPORTING_QUEUE_DESCRIPTOR_TABLE_ADDRESS + (IMAGE_0_REPORTING_QUEUE_COUNTER_TABLE_ADDRESS<<16);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[12]] = IMAGE_0_REPORTING_QUEUE_ACCUMULATED_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[11]] = IMAGE_0_GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[10]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_COUNTER);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[9]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_QUEUE_STATUS);

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_1_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

    /* CPU_RX_METER_BUDGET_ALLOCATOR */
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] =
        ADDRESS_OF(image_1, cpu_rx_meter_budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[11]] = CPU_RX_METER_TIMER_PERIOD;

    /* CPU_INTERRUPT_COALESCING */
    local_regs[IMAGE_1_IMAGE_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_1_CPU_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] =
        IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS | (IMAGE_1_CPU_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_RX_READ */
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[13]] = IMAGE_1_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[12]] = IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[11]] = IMAGE_1_PD_FIFO_TABLE_ADDRESS;

    /* Common reprocessing task is used tx_abs_recycle for speed service, or pktgen_reprocessing for tcp speed test.
     * It will be also used to recycle host buffers from CPU TX once CPU TX from ABS address is implemented */

    /* COMMON_REPROCESSING */
    local_regs[IMAGE_1_IMAGE_1_COMMON_REPROCESSING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, common_reprocessing_wakeup_request) << 16 |
        IMAGE_1_COMMON_REPROCESSING_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_COMMON_REPROCESSING_THREAD_NUMBER][reg_id[10]] = IMAGE_1_COMMON_REPROCESSING_DISPATCHER_CREDIT_TABLE_ADDRESS |
        ((BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_COMMON_REPROCESSING << 6)) << 16);

    /* CPU_RECYCLE */
    local_regs[IMAGE_1_IMAGE_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_1_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* CPU_RX_COPY_READ + (TcpSpdTest + pktgen_tx) */
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_copy_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[14]] = IMAGE_1_CPU_RX_SCRATCHPAD_ADDRESS | (IMAGE_1_CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_ADDRESS << 16);
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[13]] = IMAGE_1_CPU_RX_COPY_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[12]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS) | (IMAGE_1_CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[11]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS + (sizeof(PROCESSING_CPU_RX_DESCRIPTOR_STRUCT) * 4));

#if defined(CONFIG_BCM_SPDSVC_SUPPORT)
    /* SPEED SERVICE */
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_GEN_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, spdsvc_gen_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_GEN_THREAD_NUMBER][reg_id[9]] = ((BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SPDSVC << 6)) << 16);
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_GEN_THREAD_NUMBER][reg_id[12]] = (IMAGE_1_SPDSVC_GEN_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
#endif

    /* PROCESSING  */
    for (task = IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER; task <= IMAGE_1_IMAGE_1_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_1_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_1, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_1_DS_PACKET_BUFFER_ADDRESS, (task - IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER));
    }

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_2_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

    /* CPU_INTERRUPT_COALESCING */
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] = IMAGE_2_CPU_RECYCLE_RING_DESCRIPTOR_TABLE_ADDRESS |
        (IMAGE_2_CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_TX_RECYCLE */
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_2_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* CPU_TX_EGRESS */
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[10]] = IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* CPU_TX_INGRESS */
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[10]] = IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* SERVICE QUEUES BUDGET ALLOCATOR */
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, budget_allocator_sq_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[12]] =
        IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_2_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[10]] = IMAGE_2_SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[9]]  = IMAGE_2_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS;
#if defined(XRDP_PI2) && defined(PI2_ON_SERVICE_QUEUES)
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[15]] =
        IMAGE_2_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_2_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[10]] |= IMAGE_2_SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif

    /* SERVICE QUEUES UPDATE_FIFO_READ */
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_sq_start() & 0x1f;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, service_queues_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] =
        IMAGE_2_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS | (IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[13]] = (((IMAGE_2_IMAGE_2_SERVICE_QUEUES_TX_THREAD_NUMBER) << 4) + 1);
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* SERVICE_QUEUES FLUSH TASK: thread 2 */
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[18]] =
        ((((drv_qm_get_sq_end() - (drv_qm_get_sq_start() & ~0x1F)) + 8) / 8) << 16) + drv_qm_get_sq_start();
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[12]] =
        ((IMAGE_2_AQM_ENABLE_TABLE_ADDRESS + ((drv_qm_get_sq_start() & ~0x1f) / 8)) << 16) |
        IMAGE_2_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES_FLUSH << 6));
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_2_SERVICE_QUEUES_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        ((drv_qm_get_sq_start() & ~0x1f) / 8); /* Offset to first word with DS queue aggregation indication */

    /* SERVICE QUEUES SCHEDULING TX */
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, service_queues_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[11]] =
        (IMAGE_2_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16) | IMAGE_2_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[10]] = (IMAGE_2_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[9]] =
        (IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES << 6));

    /* PROCESSING  */
    for (task = IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER; task <= IMAGE_2_IMAGE_2_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_2_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_2, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_2_DS_PACKET_BUFFER_ADDRESS, (task - IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER));
    }

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_3_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)DEVICE_ADDRESS(RU_BLK(RNR_CNTXT).addr[core_index] + RU_REG_OFFSET(RNR_CNTXT, MEM_ENTRY));
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

    /* DIRECT PROCESSING  */
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[15]] = IMAGE_3_DIRECT_FLOW_CNTR_TABLE_ADDRESS << 16 | IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER;
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[10]] = BB_ID_DISPATCHER_REORDER;
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[11]] = QM_QUEUE_CPU_RX_COPY_EXCLUSIVE | (IMAGE_3_DIRECT_FLOW_EPON_CONTROL_SCRATCH_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[16]] =
        ADDRESS_OF(image_3, gpon_control_wakeup_request) << 16 | IMAGE_3_DIRECT_FLOW_PD_TABLE_ADDRESS;

    /* Budget allocation */
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[10]] = IMAGE_3_RATE_LIMITER_VALID_TABLE_US_ADDRESS;
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* Overall budget allocation (for ovlr rl):
     * Same as US TM WAN DSL initialization.
     * Reg 8 - BBH Queue table for DSL
     * Reg 11 - DSL BBH and Complex schedule table address
     *
     */
    local_regs[IMAGE_3_US_TM_OVL_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[8]] =
        (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS + sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT) * 32) | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_OVL_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_DSL | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_OVL_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, ovl_budget_allocator_1st_wakeup_request) << 16;

#ifdef BUFFER_CONGESTION_MGT
    /* BUFFER_CONG_MGT */
    local_regs[IMAGE_3_US_TM_BUFFER_CONG_MGT_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, buffer_cong_mgt_1st_wakeup_request) << 16;
#endif

#ifdef XRDP_PI2
    /* PI2 probability calculator: thread 4 */
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, pi2_calc_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[9]]  =
        IMAGE_3_US_TM_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_3_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif

    /* UPDATE_FIFO_READ */
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, update_fifo_us_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_3_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS  << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_3_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* FLUSH TASK */
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[18]] = (drv_qm_get_us_end() - drv_qm_get_us_start() + 8) / 8;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[12]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_US_TM_FLUSH << 6));
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_3_US_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        (drv_qm_get_us_start() / 8);

#ifdef EPON
    /* EPON UPDATE_FIFO_READ */
    local_regs[IMAGE_3_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_epon_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, epon_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[12]] = BB_ID_TX_PON;
    local_regs[IMAGE_3_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_3_EPON_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;
#endif

    /* SCHEDULING WAN */
    local_regs[IMAGE_3_US_TM_WAN_AE2P5_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, tx_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_AE2P5_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_AE2P5_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_ETH_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_AE2P5_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_2P5 | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_AE2P5_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_AE2P5_THREAD_NUMBER][reg_id[9]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16 | IMAGE_3_US_TM_AE2P5_BBH_TX_QUEUE_ID_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_AE2P5_THREAD_NUMBER][reg_id[8]] =
        (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS + sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT) * 49) | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* SCHEDULING WAN */
    local_regs[IMAGE_3_US_TM_WAN_AE10_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, tx_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_AE10_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_AE10_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_ETH_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_AE10_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_10G | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_AE10_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_AE10_THREAD_NUMBER][reg_id[9]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16 | IMAGE_3_US_TM_AE10_BBH_TX_QUEUE_ID_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_AE10_THREAD_NUMBER][reg_id[8]] =
        (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS + sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT) * 48) | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* SCHEDULING WAN */
    local_regs[IMAGE_3_US_TM_WAN_DSL_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, tx_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_DSL_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_DSL_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_DSL_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_DSL_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_DSL | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_DSL_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_DSL_THREAD_NUMBER][reg_id[9]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16 | IMAGE_3_US_TM_DSL_BBH_TX_QUEUE_ID_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_DSL_THREAD_NUMBER][reg_id[8]] =
        (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS + sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT) * 32) | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* SCHEDULING WAN */
    local_regs[IMAGE_3_US_TM_WAN_PON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, tx_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_PON_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_PON_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_PON_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_PON_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_PON | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_PON_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_PON_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16 | IMAGE_3_US_TM_PON_BBH_TX_QUEUE_ID_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_PON_THREAD_NUMBER][reg_id[8]]  = IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

#ifdef EPON
    /* SCHEDULING WAN */
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_epon_start();
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, epon_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[12]] = BB_ID_TX_PON | (IMAGE_3_BBH_TX_EPON_EGRESS_COUNTER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_BBH_TX_EPON_INGRESS_COUNTER_TABLE_ADDRESS;
#endif

#ifdef CONFIG_DSL_PREFETCH_WORKAROUND
    /* WAN TX DDR Read for prefetch */
    local_regs[IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, wan_tx_ddr_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER][reg_id[12]] = 0;  /* will be overwritten with core_id */
    local_regs[IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER][reg_id[11]] = (IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER << SBPM_ALLOC_REQUEST_TASK_NUM_F_OFFSET);
    local_regs[IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER][reg_id[10]] =
        (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_WAN_TX_PSRAM_WRITE << 6)) | ((BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_WAN_TX_DDR_READ << 6)) << 16);
    local_regs[IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_DDR_PREFETCH_PD_FIFO_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_TX_DDR_READ_THREAD_NUMBER][reg_id[8]]  = IMAGE_3_DDR_PREFETCH_PINGPONG_BUFFER_ADDRESS;

    /* WAN TX PSRAM Write for prefetch */
    local_regs[IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, wan_tx_psram_write_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER][reg_id[12]] = IMAGE_3_DDR_PREFETCH_SBPM_FIFO_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER][reg_id[11]] = (IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER << SBPM_ALLOC_REQUEST_TASK_NUM_F_OFFSET);
    local_regs[IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER][reg_id[10]] = 0;  /* Will be overwritten with core_id */
    local_regs[IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER][reg_id[9]] = IMAGE_3_DDR_PREFETCH_PD_FIFO_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_TX_PSRAM_WRITE_THREAD_NUMBER][reg_id[8]] = IMAGE_3_DDR_PREFETCH_PINGPONG_BUFFER_ADDRESS;
#endif

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_4_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)DEVICE_ADDRESS(RU_BLK(RNR_CNTXT).addr[core_index] + RU_REG_OFFSET(RNR_CNTXT, MEM_ENTRY));
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

#ifdef CONFIG_DHD_RUNNER
    /* DHD_TIMER_THREAD_NUMBER */
    local_regs[IMAGE_4_IMAGE_4_DHD_TIMER_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_4, dhd_timer_1st_wakeup_request) << 16;

    /* DHD_TX_POST_UPDATE_FIFO */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_4, dhd_tx_post_update_fifo_wakeup_request) << 16;
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[12]] = (((IMAGE_4_IMAGE_4_DHD_TX_POST_0_THREAD_NUMBER) << 4) + 9);
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_4_DHD_TX_POST_UPDATE_FIFO_TABLE_ADDRESS;

#ifdef CONFIG_WLAN_MCAST
    /* DHD_MCAST */
    local_regs[IMAGE_4_IMAGE_4_DHD_MCAST_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_4, dhd_mcast_wakeup_request) << 16;
    /* high = PD fifo bit ( = 0), low = DHD TX Post update fifo task number */
    local_regs[IMAGE_4_IMAGE_4_DHD_MCAST_THREAD_NUMBER][reg_id[12]] = (((IMAGE_4_IMAGE_4_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER) << 4) + 9);
    local_regs[IMAGE_4_IMAGE_4_DHD_MCAST_THREAD_NUMBER][reg_id[11]] = IMAGE_4_DHD_MCAST_PD_FIFO_TABLE_ADDRESS | (IMAGE_4_DHD_MCAST_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_4_IMAGE_4_DHD_MCAST_THREAD_NUMBER][reg_id[10]] =
        (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_MCAST << 6)) | (IMAGE_4_DHD_MCAST_UPDATE_FIFO_TABLE_ADDRESS << 16);
#endif /* CONFIG_WLAN_MCAST */

    /* DHD_TX_POST_0 */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_4, dhd_tx_post_wakeup_request) << 16;
    /* high = post_common_radio_ptr , low =radio_idx */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_0_THREAD_NUMBER][reg_id[13]] = 0 | ((IMAGE_4_DHD_POST_COMMON_RADIO_DATA_ADDRESS + sizeof(DHD_POST_COMMON_RADIO_ENTRY_STRUCT)*0)) << 16;
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_0_THREAD_NUMBER][reg_id[11]] = IMAGE_4_DHD_TX_POST_PD_FIFO_TABLE_ADDRESS + sizeof(PROCESSING_TX_DESCRIPTOR_STRUCT)*0;

    /* DHD_TX_POST_1 */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_4, dhd_tx_post_wakeup_request) << 16;
    /* high = post_common_radio_ptr , low =radio_idx */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_1_THREAD_NUMBER][reg_id[13]] = 1 | ((IMAGE_4_DHD_POST_COMMON_RADIO_DATA_ADDRESS + sizeof(DHD_POST_COMMON_RADIO_ENTRY_STRUCT)*1)) << 16;
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_1_THREAD_NUMBER][reg_id[11]] = IMAGE_4_DHD_TX_POST_PD_FIFO_TABLE_ADDRESS + sizeof(PROCESSING_TX_DESCRIPTOR_STRUCT)*2;

    /* DHD_TX_POST_2 */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_4, dhd_tx_post_wakeup_request) << 16;
    /* high = post_common_radio_ptr , low =radio_idx */
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_2_THREAD_NUMBER][reg_id[13]] = 2 | ((IMAGE_4_DHD_POST_COMMON_RADIO_DATA_ADDRESS + sizeof(DHD_POST_COMMON_RADIO_ENTRY_STRUCT)*2)) << 16;
    local_regs[IMAGE_4_IMAGE_4_DHD_TX_POST_2_THREAD_NUMBER][reg_id[11]] = IMAGE_4_DHD_TX_POST_PD_FIFO_TABLE_ADDRESS + sizeof(PROCESSING_TX_DESCRIPTOR_STRUCT)*4;
#endif

    /* PROCESSING  */
    for (task = IMAGE_4_IMAGE_4_0_THREAD_NUMBER; task <= IMAGE_4_IMAGE_4_7_THREAD_NUMBER; task++)
    {
        /* PROCESSING  */
        local_regs[task][reg_id[15]] = IMAGE_4_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_4, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_4_DS_PACKET_BUFFER_ADDRESS, (task - IMAGE_4_IMAGE_4_0_THREAD_NUMBER));
    }
    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_5_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)DEVICE_ADDRESS(RU_BLK(RNR_CNTXT).addr[core_index] + RU_REG_OFFSET(RNR_CNTXT, MEM_ENTRY));
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

#ifdef CONFIG_DHD_RUNNER
    /* DHD_RX_COMPLETE_0 */
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, dhd_rx_complete_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[13]] =
        ((IMAGE_5_DHD_COMPLETE_COMMON_RADIO_DATA_ADDRESS + sizeof(DHD_COMPLETE_COMMON_RADIO_ENTRY_STRUCT)*0));
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[12]] = 0;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[11]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_RX_COMPLETE_0 << 6));
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[10]] = 0;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[9]] =
        IMAGE_5_PROCESSING_DHD_RX_COMPLETE_0_THREAD_NUMBER | (IMAGE_5_DHD_RX_COMPLETE_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);

    /* DHD_RX_COMPLETE_1 */
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, dhd_rx_complete_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[13]] =
        ((IMAGE_5_DHD_COMPLETE_COMMON_RADIO_DATA_ADDRESS + sizeof(DHD_COMPLETE_COMMON_RADIO_ENTRY_STRUCT)*1));
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[12]] = 1;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[11]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_RX_COMPLETE_1 << 6));
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[10]] = 0;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[9]] =
        IMAGE_5_PROCESSING_DHD_RX_COMPLETE_1_THREAD_NUMBER | ((IMAGE_5_DHD_RX_COMPLETE_DISPATCHER_CREDIT_TABLE_ADDRESS + 16) << 16);

    /* DHD_RX_COMPLETE_2 */
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, dhd_rx_complete_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[13]] =
        ((IMAGE_5_DHD_COMPLETE_COMMON_RADIO_DATA_ADDRESS + sizeof(DHD_COMPLETE_COMMON_RADIO_ENTRY_STRUCT)*2));
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[12]] = 2;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[11]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_RX_COMPLETE_2 << 6));
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[10]] = 0;
    local_regs[IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[9]] =
        IMAGE_5_PROCESSING_DHD_RX_COMPLETE_2_THREAD_NUMBER | ((IMAGE_5_DHD_RX_COMPLETE_DISPATCHER_CREDIT_TABLE_ADDRESS + 32) << 16);
#endif

    /* DHD_TX_COMPLETE_0 */
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, dhd_tx_complete_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[13]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_TX_COMPLETE_0 << 6));
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[12]] = (IMAGE_5_DHD_COMPLETE_COMMON_RADIO_DATA_ADDRESS +
                                                                                 sizeof(DHD_COMPLETE_COMMON_RADIO_ENTRY_STRUCT)*0) |
                                                                                 (IMAGE_5_DHD_TX_COMPLETE_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[11]] = 0;

    /* DHD_TX_COMPLETE_1 */
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, dhd_tx_complete_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[13]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_TX_COMPLETE_1 << 6));
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[12]] = (IMAGE_5_DHD_COMPLETE_COMMON_RADIO_DATA_ADDRESS +
                                                                                 sizeof(DHD_COMPLETE_COMMON_RADIO_ENTRY_STRUCT)*1) |
                                                                                 ((IMAGE_5_DHD_TX_COMPLETE_DISPATCHER_CREDIT_TABLE_ADDRESS + 16) << 16);
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[11]] = 1;

    /* DHD_TX_COMPLETE_2 */
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, dhd_tx_complete_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[13]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DHD_TX_COMPLETE_2 << 6));
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[12]] = (IMAGE_5_DHD_COMPLETE_COMMON_RADIO_DATA_ADDRESS +
                                                                                 sizeof(DHD_COMPLETE_COMMON_RADIO_ENTRY_STRUCT)*2) |
                                                                                 ((IMAGE_5_DHD_TX_COMPLETE_DISPATCHER_CREDIT_TABLE_ADDRESS + 32) << 16);
    local_regs[IMAGE_5_PROCESSING_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[11]] = 2;

    for (task = IMAGE_5_PROCESSING_0_THREAD_NUMBER; task <= IMAGE_5_PROCESSING_7_THREAD_NUMBER; task++)
    {
        /* PROCESSING  */
        local_regs[task][reg_id[15]] = IMAGE_5_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_5, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_5_DS_PACKET_BUFFER_ADDRESS, (task - IMAGE_5_PROCESSING_0_THREAD_NUMBER));
    }
    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void rdd_local_registers_init(rdd_init_params_t *init_params)
{
    uint32_t core_index;

    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
    {
        switch (rdp_core_to_image_map[core_index])
        {
        case image_0_runner_image:
            image_0_context_set(core_index, init_params);
            break;

        case image_1_runner_image:
            image_1_context_set(core_index, init_params);
            break;

        case image_2_runner_image:
            image_2_context_set(core_index, init_params);
            break;

        case image_3_runner_image:
            image_3_context_set(core_index, init_params);
            break;

        case image_4_runner_image:
            image_4_context_set(core_index, init_params);
            break;

        case image_5_runner_image:
            image_5_context_set(core_index, init_params);
            break;

        default:
            bdmf_trace("ERROR driver %s:%u| unsupported Runner image = %d\n", __FILE__, __LINE__, rdp_core_to_image_map[core_index]);
            break;
        }
    }
}


static int rdd_cpu_proj_init(void)
{
    uint8_t def_idx = (uint8_t)BDMF_INDEX_UNASSIGNED;
    int rc;

    rdd_cpu_tc_to_rqx_init(def_idx);
    rdd_cpu_vport_cpu_obj_init(def_idx);
    rdd_cpu_rx_meters_init();
    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(cpu_rx_runner_image),
        CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER);
    return rc;
}

int rdd_data_structures_init(rdd_init_params_t *init_params, HW_IPTV_CONFIGURATION_STRUCT *iptv_hw_config)
{
    bdmf_error_t rc;
    rdd_local_registers_init(init_params);

    rdd_cpu_if_init();

    rdd_fpm_pool_number_mapping_cfg(iptv_hw_config->fpm_base_token_size);
#ifdef CONFIG_DHD_RUNNER
    rdp_drv_dhd_skb_fifo_tbl_init();
#endif

    if (!init_params->is_basic)
    {
        rdd_update_global_fpm_cfg();

#ifdef CONFIG_DHD_RUNNER
        rdd_dhd_hw_cfg(&init_params->dhd_hw_config);
#endif
        rdd_iptv_processing_cfg(iptv_hw_config);
    }

#if defined(DEBUG_PRINTS)
        rdd_debug_prints_init();
#endif

    /* init first queue mapping */
    rdd_ag_us_tm_first_queue_mapping_set(drv_qm_get_us_start());
    rdd_ag_ds_tm_first_queue_mapping_set(drv_qm_get_ds_start(0));
    rdd_scheduler_wake_up_bbh_init_data_structure();

    /* init bbh-queue */
    rdd_bbh_queue_init();

    /* start flush task */
    rc = rdd_scheduling_flush_timer_set();

    /* start budget allocation task */
    rc = rc ? rc : rdd_ds_budget_allocation_timer_set();

    rdd_rx_flow_init();
    rdd_proj_init(init_params);
    rdd_actions_proj_init();
    rdd_ingress_qos_drop_miss_ratio_set(2);

    rdd_service_queues_init(IMAGE_2_IMAGE_2_SERVICE_QUEUES_BUDGET_ALLOCATOR_THREAD_NUMBER);

    rc = rc ? rc : rdd_cpu_proj_init();

    /* Setting same global mask as for other project is safe, since the NATC_16BYTE_KEY_MASK differs */
    rc = rc ? rc : rdd_ag_natc_nat_cache_key0_mask_set(NATC_KEY0_DEF_MASK);

#ifdef USE_BDMF_SHELL
    /* register shell commands */
    rc = rc ? : rdd_make_shell_commands();
#endif
    return rc;
}

int rdd_egress_port_to_broadcom_switch_port_init(rdd_vport_id_t vport)
{
    EGRESS_PORT_TO_BROADCOM_SWITCH_PORT_TABLE_STRUCT *egress_to_switch_ptr;

    if (vport < RDD_LAN0_VPORT || vport > RDD_LAN6_VPORT)
    {
        return BDMF_ERR_PARM;
    }

    egress_to_switch_ptr = RDD_EGRESS_PORT_TO_BROADCOM_SWITCH_PORT_TABLE_PTR(ds_tm_runner_image);
    egress_to_switch_ptr->entry[vport - 1].bits = vport_switch_port_mapping[vport - RDD_LAN0_VPORT];

    return BDMF_ERR_OK;
}

int rdd_broadcom_switch_ports_mapping_table_config(rdd_vport_id_t vport)
{
    if (vport < RDD_LAN0_VPORT || vport > RDD_LAN6_VPORT)
    {
        return BDMF_ERR_PARM;
    }

    RDD_BROADCOM_SWITCH_PORT_MAPPING_RX_FLOW_TABLE_INDEX_WRITE_G(vport + RDD_NUM_OF_RX_WAN_FLOWS,
                                                                 RDD_BROADCOM_SWITCH_PORT_TO_RX_FLOW_MAPPING_TABLE_ADDRESS_ARR,
                                                                 vport_switch_port_mapping[vport - RDD_LAN0_VPORT]);

    return BDMF_ERR_OK;
}

static rdd_tcam_table_parm_t tcam_ic_ip_flow_params =
{
    .module_id = TCAM_IC_MODULE_IP_FLOW,
    .scratch_offset = offsetof(PACKET_BUFFER_STRUCT, scratch) + TCAM_IC_SCRATCH2_KEY_OFFSET,
};

static rdd_module_t tcam_ic_ip_flow_module =
{
    .context_offset = offsetof(PACKET_BUFFER_STRUCT, classification_contexts_list) +
        offsetof(FLOW_BASED_CONTEXT_STRUCT, tcam_ip_flow_result),
    .res_offset = (offsetof(PACKET_BUFFER_STRUCT, classification_results) +
        (CLASSIFICATION_RESULT_INDEX_TCAM_IC_IP_FLOW * 4)),
    .cfg_ptr = RDD_TCAM_IC_CFG_TABLE_ADDRESS_ARR,  /* Instance offset will be added at init time */
    .init = rdd_tcam_module_init,
    .params = &tcam_ic_ip_flow_params
};

static rdd_tcam_table_parm_t tcam_ic_ip_flow_miss_params =
{
    .module_id = TCAM_IC_MODULE_IP_FLOW_MISS,
    .scratch_offset = offsetof(PACKET_BUFFER_STRUCT, scratch) + TCAM_IC_SCRATCH2_KEY_OFFSET,
};

static rdd_module_t tcam_ic_ip_flow_miss_module =
{
    .context_offset = offsetof(PACKET_BUFFER_STRUCT, classification_contexts_list) +
        offsetof(FLOW_BASED_CONTEXT_STRUCT, tcam_ip_flow_result),
    .res_offset = (offsetof(PACKET_BUFFER_STRUCT, classification_results) +
        (CLASSIFICATION_RESULT_INDEX_TCAM_IC_IP_FLOW * 4)),
    .cfg_ptr = RDD_TCAM_IC_CFG_TABLE_ADDRESS_ARR,  /* Instance offset will be added at init time */
    .init = rdd_tcam_module_init,
    .params = &tcam_ic_ip_flow_miss_params
};

static rdd_module_t ingress_filter_module =
{
    .res_offset = (offsetof(PACKET_BUFFER_STRUCT, classification_results) +
        (CLASSIFICATION_RESULT_INDEX_INGRESS_FILTERS * 4)),
    .cfg_ptr = RDD_INGRESS_FILTER_CFG_ADDRESS_ARR,
    .init = rdd_ingress_filter_module_init,
    .params = NULL
};

/* IPTV */
static iptv_params_t iptv_params =
{
    .key_offset = offsetof(PACKET_BUFFER_STRUCT, scratch),
    .hash_tbl_idx = 1
};

static rdd_module_t iptv_module =
{
    .init = rdd_iptv_module_init,
    .context_offset = offsetof(PACKET_BUFFER_STRUCT, classification_contexts_list),
    .res_offset = (offsetof(PACKET_BUFFER_STRUCT, classification_results) +
        (CLASSIFICATION_RESULT_INDEX_IPTV * 4)),
    .cfg_ptr = RDD_IPTV_CFG_TABLE_ADDRESS_ARR,
    .params = (void *)&iptv_params
};

/* Multicast Whitelist */
/* IP CLASS */
static natc_params_t natc_params =
{
    .connection_key_offset = offsetof(PACKET_BUFFER_STRUCT, scratch),
};

static rdd_module_t ip_flow =
{
    .init = rdd_nat_cache_init,
    /* NATC returns context size - 4 bytes (first 4B are control) */
    /* For 8B alignment, 4B are added */
    /* LD_CONTEXT macro adds 8B (control) */
    .context_offset = offsetof(PACKET_BUFFER_STRUCT, classification_contexts_list),
    .res_offset = (offsetof(PACKET_BUFFER_STRUCT, classification_results) +
        (CLASSIFICATION_RESULT_INDEX_NAT_CACHE * 4)),
    .cfg_ptr = RDD_NAT_CACHE_CFG_ADDRESS_ARR,
    .params = (void *)&natc_params
};

/* Tunnels parser */
static tunnels_parsing_params_t tunnels_parsing_params =
{
    .tunneling_enable = 1
};

rdd_module_t tunnels_parsing =
{
    .init = rdd_tunnels_parsing_init,
    .cfg_ptr = RDD_TUNNELS_PARSING_CFG_ADDRESS_ARR,
    .res_offset = (offsetof(PACKET_BUFFER_STRUCT, classification_results) +
        (CLASSIFICATION_RESULT_INDEX_TUNNELS_PARSING * 4)),
    .params = (void *)&tunnels_parsing_params
};

static void rdd_proj_init(rdd_init_params_t *init_params)
{
    /* Classification modules initialization */
    _rdd_module_init(&tunnels_parsing);
    _rdd_module_init(&ip_flow);
    if (init_params->is_basic)
        return;

    _rdd_module_init(&tcam_ic_ip_flow_module);
    _rdd_module_init(&tcam_ic_ip_flow_miss_module);
    _rdd_module_init(&ingress_filter_module);
    _rdd_module_init(&iptv_module);

#if !defined(RDP_SIM) && defined(CONFIG_DSL_PREFETCH_WORKAROUND)
    /* initialize US TM DDR prefetch FIFO PTR and PINGPONG buffer Ptr */
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_PD_FIFO_ADDRESS, RDD_TX_TASK_DDR_PREFETCH_FIFO_PTR_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_PD_FIFO_ADDRESS, RDD_DDR_READ_DDR_PREFETCH_FIFO_PTR_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_PD_FIFO_ADDRESS, RDD_PSRAM_WRITE_DDR_PREFETCH_FIFO_PTR_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_PINGPONG_BUFFER_ADDRESS, RDD_DDR_READ_DDR_PREFETCH_PINGPONG_BUFFER_PTR_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_PINGPONG_BUFFER_ADDRESS + IMAGE_3_DDR_PREFETCH_PINGPONG_BUFFER_BYTE_SIZE - 1,
                           RDD_PSRAM_WRITE_DDR_PREFETCH_PINGPONG_BUFFER_PTR_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE((BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_WAN_TX_DDR_READ << 6)), RDD_TX_TASK_DDR_READ_DISP_BB_ID_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_SBPM_FIFO_ADDRESS, RDD_WRITE_DDR_PREFETCH_SBPM_FIFO_PTR_PTR(get_runner_idx(us_tm_runner_image)));
    RDD_BYTES_2_BITS_WRITE(IMAGE_3_DDR_PREFETCH_SBPM_FIFO_ADDRESS, RDD_READ_DDR_PREFETCH_SBPM_FIFO_PTR_PTR(get_runner_idx(us_tm_runner_image)));
#endif
}

static void rdd_write_action(uint8_t core_index, uint16_t *action_arr, uint8_t size_of_array, uint8_t *ptr, uint8_t tbl_size, uint16_t exception_label)
{
    uint32_t action_index;
    for (action_index = 0; action_index < tbl_size; action_index++)
    {
        if (action_index < size_of_array)
            RDD_BYTES_2_BITS_WRITE(action_arr[action_index], ptr + (sizeof(action_arr[0]) * action_index));
        else
            RDD_BYTES_2_BITS_WRITE(exception_label, ptr + (sizeof(action_arr[0]) * action_index));
    }
}



static void rdd_actions_proj_init(void)
{
    uint8_t core_index;
    uint16_t processing0_flow_actions_arr[] = {
        [0] = ADDRESS_OF(image_5, action_gpe),
        [1 ... 16] = ADDRESS_OF(image_5, processing_check_wlan_llcsnap)
    };

    uint16_t processing1_flow_actions_arr[] = {
        [0] = ADDRESS_OF(image_1, action_gpe),
        [1 ... 16] = ADDRESS_OF(image_1, processing_check_wlan_llcsnap)
    };

    uint16_t processing2_flow_actions_arr[] = {
        [0] = ADDRESS_OF(image_2, action_gpe),
        [1 ... 16] = ADDRESS_OF(image_2, processing_check_wlan_llcsnap)
    };

    uint16_t processing3_flow_actions_arr[] = {
        [0] = ADDRESS_OF(image_4, action_gpe),
        [1 ... 16] = ADDRESS_OF(image_4, processing_check_wlan_llcsnap)
    };

    uint16_t processing0_gpe_actions_arr[] = {
        [0]  = 0, /* gpe_end has no label */
        [1]  = ADDRESS_OF(image_5, gpe_sop_push_replace_ddr_sram_32),
        [2]  = ADDRESS_OF(image_5, gpe_sop_push_replace_sram_32_64),
        [3]  = ADDRESS_OF(image_5, gpe_sop_push_replace_sram_64),
        [4]  = ADDRESS_OF(image_5, gpe_sop_push_replace_sram_64_32),
        [5]  = ADDRESS_OF(image_5, gpe_sop_pull_replace_ddr_sram_32),
        [6]  = ADDRESS_OF(image_5, gpe_sop_pull_replace_sram_32_64),
        [7]  = ADDRESS_OF(image_5, gpe_sop_pull_replace_sram_64),
        [8]  = ADDRESS_OF(image_5, gpe_sop_pull_replace_sram_64_32),
        [9]  = 0, /* gpe_replace_pointer_32_ddr is not supported */
        [10] = ADDRESS_OF(image_5, gpe_replace_pointer_32_sram),
        [11] = ADDRESS_OF(image_5, gpe_replace_pointer_64_sram),
        [12] = ADDRESS_OF(image_5, gpe_replace_16),
        [13] = ADDRESS_OF(image_5, gpe_replace_32),
        [14] = ADDRESS_OF(image_5, gpe_replace_bits_16),
        [15] = ADDRESS_OF(image_5, gpe_copy_add_16_cl),
        [16] = ADDRESS_OF(image_5, gpe_copy_add_16_sram),
        [17] = ADDRESS_OF(image_5, gpe_copy_bits_16_cl),
        [18] = ADDRESS_OF(image_5, gpe_copy_bits_16_sram),
        [19] = ADDRESS_OF(image_5, gpe_insert_16),
        [20] = ADDRESS_OF(image_5, gpe_delete_16),
        [21] = ADDRESS_OF(image_5, gpe_decrement_8),
        [22] = ADDRESS_OF(image_5, gpe_apply_icsum_16),
        [23] = ADDRESS_OF(image_5, gpe_apply_icsum_nz_16),
        [24] = ADDRESS_OF(image_5, gpe_compute_csum_16_cl),
        [25] = ADDRESS_OF(image_5, gpe_compute_csum_16_sram),
        [26] = ADDRESS_OF(image_5, gpe_buffer_copy_16_sram),
        [27] = ADDRESS_OF(image_5, gpe_buffer_copy_16_ddr),
        [28] = ADDRESS_OF(image_5, gpe_replace_add_packet_length_cl)
    };

    uint16_t processing1_gpe_actions_arr[] = {
        [0]  = 0, /* gpe_end has no label */
        [1]  = ADDRESS_OF(image_1, gpe_sop_push_replace_ddr_sram_32),
        [2]  = ADDRESS_OF(image_1, gpe_sop_push_replace_sram_32_64),
        [3]  = ADDRESS_OF(image_1, gpe_sop_push_replace_sram_64),
        [4]  = ADDRESS_OF(image_1, gpe_sop_push_replace_sram_64_32),
        [5]  = ADDRESS_OF(image_1, gpe_sop_pull_replace_ddr_sram_32),
        [6]  = ADDRESS_OF(image_1, gpe_sop_pull_replace_sram_32_64),
        [7]  = ADDRESS_OF(image_1, gpe_sop_pull_replace_sram_64),
        [8]  = ADDRESS_OF(image_1, gpe_sop_pull_replace_sram_64_32),
        [9]  = 0, /* gpe_replace_pointer_32_ddr is not supported */
        [10] = ADDRESS_OF(image_1, gpe_replace_pointer_32_sram),
        [11] = ADDRESS_OF(image_1, gpe_replace_pointer_64_sram),
        [12] = ADDRESS_OF(image_1, gpe_replace_16),
        [13] = ADDRESS_OF(image_1, gpe_replace_32),
        [14] = ADDRESS_OF(image_1, gpe_replace_bits_16),
        [15] = ADDRESS_OF(image_1, gpe_copy_add_16_cl),
        [16] = ADDRESS_OF(image_1, gpe_copy_add_16_sram),
        [17] = ADDRESS_OF(image_1, gpe_copy_bits_16_cl),
        [18] = ADDRESS_OF(image_1, gpe_copy_bits_16_sram),
        [19] = ADDRESS_OF(image_1, gpe_insert_16),
        [20] = ADDRESS_OF(image_1, gpe_delete_16),
        [21] = ADDRESS_OF(image_1, gpe_decrement_8),
        [22] = ADDRESS_OF(image_1, gpe_apply_icsum_16),
        [23] = ADDRESS_OF(image_1, gpe_apply_icsum_nz_16),
        [24] = ADDRESS_OF(image_1, gpe_compute_csum_16_cl),
        [25] = ADDRESS_OF(image_1, gpe_compute_csum_16_sram),
        [26] = ADDRESS_OF(image_1, gpe_buffer_copy_16_sram),
        [27] = ADDRESS_OF(image_1, gpe_buffer_copy_16_ddr),
        [28] = ADDRESS_OF(image_1, gpe_replace_add_packet_length_cl)
    };

    uint16_t processing2_gpe_actions_arr[] = {
        [0]  = 0, /* gpe_end has no label */
        [1]  = ADDRESS_OF(image_2, gpe_sop_push_replace_ddr_sram_32),
        [2]  = ADDRESS_OF(image_2, gpe_sop_push_replace_sram_32_64),
        [3]  = ADDRESS_OF(image_2, gpe_sop_push_replace_sram_64),
        [4]  = ADDRESS_OF(image_2, gpe_sop_push_replace_sram_64_32),
        [5]  = ADDRESS_OF(image_2, gpe_sop_pull_replace_ddr_sram_32),
        [6]  = ADDRESS_OF(image_2, gpe_sop_pull_replace_sram_32_64),
        [7]  = ADDRESS_OF(image_2, gpe_sop_pull_replace_sram_64),
        [8]  = ADDRESS_OF(image_2, gpe_sop_pull_replace_sram_64_32),
        [9]  = 0, /* gpe_replace_pointer_32_ddr is not supported */
        [10] = ADDRESS_OF(image_2, gpe_replace_pointer_32_sram),
        [11] = ADDRESS_OF(image_2, gpe_replace_pointer_64_sram),
        [12] = ADDRESS_OF(image_2, gpe_replace_16),
        [13] = ADDRESS_OF(image_2, gpe_replace_32),
        [14] = ADDRESS_OF(image_2, gpe_replace_bits_16),
        [15] = ADDRESS_OF(image_2, gpe_copy_add_16_cl),
        [16] = ADDRESS_OF(image_2, gpe_copy_add_16_sram),
        [17] = ADDRESS_OF(image_2, gpe_copy_bits_16_cl),
        [18] = ADDRESS_OF(image_2, gpe_copy_bits_16_sram),
        [19] = ADDRESS_OF(image_2, gpe_insert_16),
        [20] = ADDRESS_OF(image_2, gpe_delete_16),
        [21] = ADDRESS_OF(image_2, gpe_decrement_8),
        [22] = ADDRESS_OF(image_2, gpe_apply_icsum_16),
        [23] = ADDRESS_OF(image_2, gpe_apply_icsum_nz_16),
        [24] = ADDRESS_OF(image_2, gpe_compute_csum_16_cl),
        [25] = ADDRESS_OF(image_2, gpe_compute_csum_16_sram),
        [26] = ADDRESS_OF(image_2, gpe_buffer_copy_16_sram),
        [27] = ADDRESS_OF(image_2, gpe_buffer_copy_16_ddr),
        [28] = ADDRESS_OF(image_2, gpe_replace_add_packet_length_cl)
    };

    uint16_t processing3_gpe_actions_arr[] = {
        [0]  = 0, /* gpe_end has no label */
        [1]  = ADDRESS_OF(image_4, gpe_sop_push_replace_ddr_sram_32),
        [2]  = ADDRESS_OF(image_4, gpe_sop_push_replace_sram_32_64),
        [3]  = ADDRESS_OF(image_4, gpe_sop_push_replace_sram_64),
        [4]  = ADDRESS_OF(image_4, gpe_sop_push_replace_sram_64_32),
        [5]  = ADDRESS_OF(image_4, gpe_sop_pull_replace_ddr_sram_32),
        [6]  = ADDRESS_OF(image_4, gpe_sop_pull_replace_sram_32_64),
        [7]  = ADDRESS_OF(image_4, gpe_sop_pull_replace_sram_64),
        [8]  = ADDRESS_OF(image_4, gpe_sop_pull_replace_sram_64_32),
        [9]  = 0, /* gpe_replace_pointer_32_ddr is not supported */
        [10] = ADDRESS_OF(image_4, gpe_replace_pointer_32_sram),
        [11] = ADDRESS_OF(image_4, gpe_replace_pointer_64_sram),
        [12] = ADDRESS_OF(image_4, gpe_replace_16),
        [13] = ADDRESS_OF(image_4, gpe_replace_32),
        [14] = ADDRESS_OF(image_4, gpe_replace_bits_16),
        [15] = ADDRESS_OF(image_4, gpe_copy_add_16_cl),
        [16] = ADDRESS_OF(image_4, gpe_copy_add_16_sram),
        [17] = ADDRESS_OF(image_4, gpe_copy_bits_16_cl),
        [18] = ADDRESS_OF(image_4, gpe_copy_bits_16_sram),
        [19] = ADDRESS_OF(image_4, gpe_insert_16),
        [20] = ADDRESS_OF(image_4, gpe_delete_16),
        [21] = ADDRESS_OF(image_4, gpe_decrement_8),
        [22] = ADDRESS_OF(image_4, gpe_apply_icsum_16),
        [23] = ADDRESS_OF(image_4, gpe_apply_icsum_nz_16),
        [24] = ADDRESS_OF(image_4, gpe_compute_csum_16_cl),
        [25] = ADDRESS_OF(image_4, gpe_compute_csum_16_sram),
        [26] = ADDRESS_OF(image_4, gpe_buffer_copy_16_sram),
        [27] = ADDRESS_OF(image_4, gpe_buffer_copy_16_ddr),
        [28] = ADDRESS_OF(image_4, gpe_replace_add_packet_length_cl)
    };

    uint32_t size_of_array;

    size_of_array = sizeof(processing0_gpe_actions_arr) / sizeof(processing0_gpe_actions_arr[0]);

    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
    {
        if (rdp_core_to_image_map[core_index] == processing0_runner_image)
        {
            rdd_write_action(core_index, processing0_flow_actions_arr, RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, (uint8_t *)RDD_FLOW_BASED_ACTION_PTR_TABLE_PTR(core_index),
                RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, image_5_processing_check_wlan_llcsnap);

            rdd_write_action(core_index, processing0_gpe_actions_arr, size_of_array, (uint8_t *)RDD_GPE_COMMAND_PRIMITIVE_TABLE_PTR(core_index),
                RDD_GPE_COMMAND_PRIMITIVE_TABLE_SIZE, 0);
        }
        else if (rdp_core_to_image_map[core_index] == processing1_runner_image)
        {
            rdd_write_action(core_index, processing1_flow_actions_arr, RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, (uint8_t *)RDD_FLOW_BASED_ACTION_PTR_TABLE_PTR(core_index),
                RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, image_1_processing_check_wlan_llcsnap);

            rdd_write_action(core_index, processing1_gpe_actions_arr, size_of_array, (uint8_t *)RDD_GPE_COMMAND_PRIMITIVE_TABLE_PTR(core_index),
                RDD_GPE_COMMAND_PRIMITIVE_TABLE_SIZE, 0);
        }
        else if (rdp_core_to_image_map[core_index] == processing2_runner_image)
        {
            rdd_write_action(core_index, processing2_flow_actions_arr, RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, (uint8_t *)RDD_FLOW_BASED_ACTION_PTR_TABLE_PTR(core_index),
                RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, image_2_processing_check_wlan_llcsnap);

            rdd_write_action(core_index, processing2_gpe_actions_arr, size_of_array, (uint8_t *)RDD_GPE_COMMAND_PRIMITIVE_TABLE_PTR(core_index),
                RDD_GPE_COMMAND_PRIMITIVE_TABLE_SIZE, 0);
        }
        else if (rdp_core_to_image_map[core_index] == processing3_runner_image)
        {
            rdd_write_action(core_index, processing3_flow_actions_arr, RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, (uint8_t *)RDD_FLOW_BASED_ACTION_PTR_TABLE_PTR(core_index),
                RDD_FLOW_BASED_ACTION_PTR_TABLE_SIZE, image_4_processing_check_wlan_llcsnap);

            rdd_write_action(core_index, processing3_gpe_actions_arr, size_of_array, (uint8_t *)RDD_GPE_COMMAND_PRIMITIVE_TABLE_PTR(core_index),
                RDD_GPE_COMMAND_PRIMITIVE_TABLE_SIZE, 0);
        }
    }
}

