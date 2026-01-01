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
#include "rdd_iptv_processing.h"
#include "rdd_qos_mapper.h"
#include "XRDP_AG.h"
#include "rdd_map_auto.h"
#include "rdd_ghost_reporting.h"
#include "rdd_scheduling.h"
#include "rdd_common.h"
#include "rdd_init_pon_common.h"
#include "rdp_common.h"
#include "rdd_runner_proj_defs.h"
#include "rdp_drv_rnr.h"
#include "rdp_drv_qm.h"
#include "rdd_service_queues.h"
#include "rdd_debug.h"


extern uintptr_t xrdp_virt2phys(const ru_block_rec *ru_block, uint8_t addr_idx);
static void rdd_actions_proj_init(void);

extern int reg_id[32];
DEFINE_BDMF_FASTLOCK(int_lock);
DEFINE_BDMF_FASTLOCK(iptv_lock);
DEFINE_BDMF_FASTLOCK(int_lock_irq);

#ifdef USE_BDMF_SHELL
extern int rdd_make_shell_commands(void);
#endif /* USE_BDMF_SHELL */

rdd_vport_id_t rx_flow_to_vport[RX_FLOW_CONTEXTS_NUMBER] = {
    [0 ... RDD_MAX_RX_WAN_FLOW]  = RDD_WAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_0] = RDD_LAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_1] = RDD_LAN1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_2] = RDD_LAN2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_3] = RDD_LAN3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_4] = RDD_LAN4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_5] = RDD_LAN5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU0] = RDD_CPU0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU1] = RDD_CPU1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU2] = RDD_CPU2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU3] = RDD_CPU3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU4] = RDD_CPU4_VPORT
};

extern FPM_GLOBAL_CFG_STRUCT g_fpm_hw_cfg;

static void image_0_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
     MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* Budget allocation: thread 0 */
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, budget_allocator_1st_wakeup_request) << 16;
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
#endif
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN;
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[10]] = IMAGE_0_RATE_LIMITER_VALID_TABLE_DS_ADDRESS;
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

#ifdef XRDP_PI2
    /* PI2 probability calculator: thread 1 */
    local_regs[IMAGE_0_DS_TM_PI2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, pi2_calc_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_PI2_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_0_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_PI2_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif

    /* UPDATE_FIFO_READ: thread 2 */
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_ds_start(0) & 0x1f;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, update_fifo_ds_read_1st_wakeup_request) << 16;
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS | (IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
#endif
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

    /* SCHEDULING LAN: thread 3 */
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[12]] = (IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN;
#endif    
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[8]]  = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_TX_TASK_THREAD_NUMBER][reg_id[8]]  = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif    

    /* FLUSH TASK: thread 4 */
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[18]] = ((((drv_qm_get_ds_end(0) - (drv_qm_get_ds_start(0) & ~0x1F)) + 8) / 8) << 16) + drv_qm_get_ds_start(0);
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DS_TM_FLUSH << 6));
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_0_DS_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        ((drv_qm_get_ds_start(0) & ~0x1f) / 8); /* Offset to first word with DS queue aggregation indication */

    /* REPORTING : thread 5 */
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[18]] = drv_qm_get_us_end();
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ghost_reporting_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[15]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, TOTAL_VALID_COUNTER_COUNTER);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[14]] = BB_ID_TX_PON_ETH_STAT;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[13]] = IMAGE_0_REPORTING_QUEUE_DESCRIPTOR_TABLE_ADDRESS + (IMAGE_0_REPORTING_QUEUE_COUNTER_TABLE_ADDRESS<<16);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[12]] = IMAGE_0_REPORTING_QUEUE_ACCUMULATED_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[11]] = IMAGE_0_GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[10]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_COUNTER);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[9]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_QUEUE_STATUS);
    local_regs[IMAGE_0_DS_TM_REPORTING_THREAD_NUMBER][reg_id[8]] = IMAGE_0_REPORTING_COUNTER_TABLE_ADDRESS;

     rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_1_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* CPU_RX_METER_BUDGET_ALLOCATOR: thread 0 */
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] =
        ADDRESS_OF(image_1, cpu_rx_meter_budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[11]] = CPU_RX_METER_TIMER_PERIOD_IN_USEC;

    /* CPU_INTERRUPT_COALESCING: thread 1 */
    local_regs[IMAGE_1_IMAGE_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_1_CPU_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] =
        IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS | (IMAGE_1_CPU_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);


    /* CPU_RX_READ: thread 2 */
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[13]] = IMAGE_1_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[12]] = IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_THREAD_NUMBER][reg_id[11]] = IMAGE_1_PD_FIFO_TABLE_ADDRESS;

    /* CPU_RECYCLE: thread 3 */
    local_regs[IMAGE_1_IMAGE_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_1_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* CPU_RX_COPY_READ: thread 4 */
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_copy_wakeup_request) << 16;
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[14]] = IMAGE_1_CPU_RX_SCRATCHPAD_ADDRESS | (IMAGE_1_CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_ADDRESS << 16);
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[13]] =
        IMAGE_1_CPU_RX_COPY_UPDATE_FIFO_TABLE_ADDRESS | (IMAGE_1_WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[12]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS) | (IMAGE_1_CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_IMAGE_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[11]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS + (sizeof(PROCESSING_CPU_RX_DESCRIPTOR_STRUCT) * 4));

    /* PROCESSING : thread 7-14 */
    for (task = IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER; task <= IMAGE_1_IMAGE_1_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_1_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_1, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_1_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER));
    }

     rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_2_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

/* Strict priority order tasks: */
    /* CPU_INTERRUPT_COALESCING: thread 0 */
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] = IMAGE_2_CPU_RECYCLE_RING_DESCRIPTOR_TABLE_ADDRESS |
        (IMAGE_2_CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_RECYCLE: thread 1 */
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_2_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* TIMER_COMMON: thread 2 */
    local_regs[IMAGE_2_IMAGE_2_TIMER_COMMON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, timer_common_task_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_TIMER_COMMON_THREAD_NUMBER][reg_id[14]] = CNTR_MAX_VAL;

/* Round Robin order tasks: */

    /* CPU_TX_EGRESS: thread 6 */
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[10]] = IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* CPU_TX_INGRESS: thread 7 */
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[10]] = IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER; task <= IMAGE_2_IMAGE_2_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_2_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_2, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_2_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER));
    }

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_3_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* DIRECT PROCESSING : thread 0 */
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[15]] = IMAGE_3_DIRECT_FLOW_CNTR_TABLE_ADDRESS << 16 | IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER;
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[10]] = BB_ID_DISPATCHER_REORDER;
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_WAN_0_BB_DESTINATION_TABLE_ADDRESS << 16 | QM_QUEUE_CPU_RX_COPY_EXCLUSIVE;
    local_regs[IMAGE_3_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, gpon_control_wakeup_request) << 16 | IMAGE_3_DIRECT_FLOW_PD_TABLE_ADDRESS;

    /* Budget allocation: thread 1 */
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[10]] = IMAGE_3_RATE_LIMITER_VALID_TABLE_US_ADDRESS;
    local_regs[IMAGE_3_US_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* Overall budget allocation (for ovlr rl): thread 2 */
    local_regs[IMAGE_3_US_TM_OVL_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[8]] = IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_OVL_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[11]] = (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_OVL_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, ovl_budget_allocator_1st_wakeup_request) << 16;

#ifdef XRDP_PI2
    /* PI2 probability calculator: thread 3 */
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, pi2_calc_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_3_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif
    /* EPON UPDATE_FIFO_READ: thread 4 */
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, epon_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_3_EPON_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* EPON SCHEDULING WAN: thread 5 */
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_epon_start();
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, epon_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[12]] = BB_ID_TX_PON_ETH_PD | (IMAGE_3_BBH_TX_EPON_EGRESS_COUNTER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_BBH_TX_EPON_INGRESS_COUNTER_TABLE_ADDRESS;

    /* UPDATE_FIFO_READ: thread 6 */
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, update_fifo_us_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_3_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_3_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* SCHEDULING WAN: thread 7 */
    local_regs[IMAGE_3_US_TM_WAN_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, us_tx_task_1st_wakeup_request) << 16 | IMAGE_3_US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_WAN_0_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_THREAD_NUMBER][reg_id[11]] = (IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16); /* low register used dynamically */
    local_regs[IMAGE_3_US_TM_WAN_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_THREAD_NUMBER][reg_id[9]]  = (IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_WAN_0_BB_DESTINATION_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_THREAD_NUMBER][reg_id[8]]  = IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* FLUSH TASK: thread 8 */
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[18]] = (drv_qm_get_us_end() - drv_qm_get_us_start() + 8) / 8;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[12]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_US_TM_FLUSH << 6));
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_3_US_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        (drv_qm_get_us_start() / 8);

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_4_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_4_IMAGE_4_0_THREAD_NUMBER; task <= IMAGE_4_IMAGE_4_7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_4_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_4, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_4_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_4_IMAGE_4_0_THREAD_NUMBER));
    }

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}


static void image_5_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_5_PROCESSING_0_THREAD_NUMBER; task <= IMAGE_5_PROCESSING_7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_5_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_5, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_5_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_5_PROCESSING_0_THREAD_NUMBER));
    }


    /* SERVICE_QUEUES BUDGET_ALLOCATOR: thread 0 */
    local_regs[IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER][reg_id[12]] =
        IMAGE_5_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_5_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER][reg_id[10]] = IMAGE_5_SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE_ADDRESS;
    local_regs[IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER][reg_id[9]]  = IMAGE_5_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS;
#if defined(XRDP_PI2) && defined(PI2_ON_SERVICE_QUEUES)
    local_regs[IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER][reg_id[15]] =
        IMAGE_5_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_5_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER][reg_id[10]] |= IMAGE_5_SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif

    /* SERVICE_QUEUES UPDATE_FIFO: thread 1 */
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_sq_start() & 0x1f;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, service_queues_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] =
        IMAGE_5_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS | (IMAGE_5_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[13]] = (((IMAGE_5_PROCESSING_SERVICE_QUEUES_TX_THREAD_NUMBER) << 4) + 1);
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_5_SERVICE_QUEUES_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_5_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* SERVICE_QUEUES FLUSH TASK: thread 2 */
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[18]] =
        ((((drv_qm_get_sq_end() - (drv_qm_get_sq_start() & ~0x1F)) + 8) / 8) << 16) + drv_qm_get_sq_start();
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[12]] = ((IMAGE_5_AQM_ENABLE_TABLE_ADDRESS + ((drv_qm_get_sq_start() & ~0x1f) / 8)) << 16) |
        IMAGE_5_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_5_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES_FLUSH << 6));
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_5_SERVICE_QUEUES_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        ((drv_qm_get_sq_start() & ~0x1f) / 8); /* Offset to first word with DS queue aggregation indication */

    /* SERVICE_QUEUES SCHEDULING TX: thread 3 */
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_5, service_queues_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[11]] =
        (IMAGE_5_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16) | IMAGE_5_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[10]] = (IMAGE_5_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_5_PROCESSING_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[9]] =
        (IMAGE_5_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES << 6));

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_6_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_6_IMAGE_6_0_THREAD_NUMBER; task <= IMAGE_6_IMAGE_6_7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_6_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_6, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_6_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_6_IMAGE_6_0_THREAD_NUMBER));
    }

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_7_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_7_IMAGE_7_0_THREAD_NUMBER; task <= IMAGE_7_IMAGE_7_7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_7_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_7, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_7_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_7_IMAGE_7_0_THREAD_NUMBER));
    }

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
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

        case image_6_runner_image:
            image_6_context_set(core_index, init_params);
            break;

        case image_7_runner_image:
            image_7_context_set(core_index, init_params);
            break;

        default:
            bdmf_trace("ERROR driver %s:%u| unsupported Runner image = %d\n", __FILE__, __LINE__, rdp_core_to_image_map[core_index]);
            break;
        }
    }
}

int rdd_data_structures_init(rdd_init_params_t *init_params, HW_IPTV_CONFIGURATION_STRUCT *iptv_hw_config)
{
    bdmf_error_t rc;

    rdd_local_registers_init(init_params);

    rdd_cpu_if_init();

    rdd_fpm_pool_number_mapping_cfg(iptv_hw_config->fpm_base_token_size);
    rdd_bridge_ports_init();
    if (!init_params->is_basic)
    {
        rdd_update_global_fpm_cfg();

        rdd_iptv_processing_cfg(iptv_hw_config);
        rdd_qos_mapper_init();
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

    rdd_rx_default_flow_init();

    rdd_rx_flow_init();

    rdd_proj_init(init_params);
    rdd_actions_proj_init();

    rdd_service_queues_init(IMAGE_5_PROCESSING_BUDGET_ALLOCATION_SERVICE_QUEUES_THREAD_NUMBER);

    RDD_BYTES_2_BITS_WRITE_G(16, RDD_MCAST_BBH_OVERRUN_TASKS_LIMIT_ADDRESS_ARR, 0);

    rc = rc ? rc : rdd_cpu_proj_init();

#ifdef USE_BDMF_SHELL
    /* register shell commands */
    rc = rc ? : rdd_make_shell_commands();
#endif
    return rc;
}

static void rdd_actions_proj_init(void)
{
    uint8_t core_index;

    uint16_t processing0_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_5, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_5, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_5, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_5, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_5, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_5, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_5, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_5, gpe_cmd_skip_if),
    };

    uint16_t processing1_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_1, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_1, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_1, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_1, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_1, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_1, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_1, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_1, gpe_cmd_skip_if),
    };

    uint16_t processing2_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_2, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_2, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_2, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_2, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_2, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_2, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_2, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_2, gpe_cmd_skip_if),
    };

    uint16_t processing3_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_4, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_4, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_4, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_4, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_4, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_4, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_4, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_4, gpe_cmd_skip_if),
    };

    uint16_t processing4_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_6, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_6, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_6, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_6, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_6, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_6, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_6, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_6, gpe_cmd_skip_if),
    };

    uint16_t processing5_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_7, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_7, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_7, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_7, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_7, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_7, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_7, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_7, gpe_cmd_skip_if),
    };

    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
    {
        /* setting group 0 - processing */
        if (rdp_core_to_image_map[core_index] == processing0_runner_image)
        {
            rdd_write_action(core_index, processing0_vlan_actions_arr, sizeof(processing0_vlan_actions_arr)/sizeof(processing0_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
        else if (rdp_core_to_image_map[core_index] == processing1_runner_image)
        {
            rdd_write_action(core_index, processing1_vlan_actions_arr, sizeof(processing1_vlan_actions_arr)/sizeof(processing1_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
        else if (rdp_core_to_image_map[core_index] == processing2_runner_image)
        {
            rdd_write_action(core_index, processing2_vlan_actions_arr, sizeof(processing2_vlan_actions_arr)/sizeof(processing2_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
        else if (rdp_core_to_image_map[core_index] == processing3_runner_image)
        {
            rdd_write_action(core_index, processing3_vlan_actions_arr, sizeof(processing3_vlan_actions_arr)/sizeof(processing3_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
        else if (rdp_core_to_image_map[core_index] == processing4_runner_image)
        {
            rdd_write_action(core_index, processing4_vlan_actions_arr, sizeof(processing4_vlan_actions_arr)/sizeof(processing4_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
        else if (rdp_core_to_image_map[core_index] == processing5_runner_image)
        {
            rdd_write_action(core_index, processing5_vlan_actions_arr, sizeof(processing5_vlan_actions_arr)/sizeof(processing5_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
    }
}


