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
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU0] = RDD_CPU0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU1] = RDD_CPU1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU2] = RDD_CPU2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU4] = RDD_CPU4_VPORT,
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

    /* DIRECT PROCESSING : thread 0 */
    local_regs[IMAGE_0_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[15]] = IMAGE_0_DIRECT_FLOW_CNTR_TABLE_ADDRESS << 16 | IMAGE_0_TM_DIRECT_FLOW_THREAD_NUMBER;
    local_regs[IMAGE_0_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[10]] = BB_ID_DISPATCHER_REORDER;
    local_regs[IMAGE_0_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[11]] = IMAGE_0_US_TM_WAN_0_BB_DESTINATION_TABLE_ADDRESS << 16 | QM_QUEUE_CPU_RX_COPY_EXCLUSIVE;
    local_regs[IMAGE_0_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, gpon_control_wakeup_request) << 16 | IMAGE_0_DIRECT_FLOW_PD_TABLE_ADDRESS;

    /* EPON UPDATE_FIFO_READ: thread 1 */
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, epon_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_0_EPON_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* EPON SCHEDULING WAN: thread 2 */
    local_regs[IMAGE_0_TM_WAN_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_epon_start();
    local_regs[IMAGE_0_TM_WAN_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, epon_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_WAN_EPON_THREAD_NUMBER][reg_id[12]] = BB_ID_TX_PON_ETH_PD | (IMAGE_0_BBH_TX_EPON_EGRESS_COUNTER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_TM_WAN_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_0_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_WAN_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_0_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_TM_WAN_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BBH_TX_EPON_INGRESS_COUNTER_TABLE_ADDRESS;

    /* UPDATE_FIFO_READ: thread 3 */
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, update_fifo_us_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_0_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[12]] = (IMAGE_0_US_TM_PD_FIFO_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[11]] = IMAGE_0_US_TM_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_0_US_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_TM_UPDATE_FIFO_US_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* SCHEDULING WAN_0: thread 4 */
    local_regs[IMAGE_0_TM_WAN_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, us_tx_task_1st_wakeup_request) << 16 | IMAGE_0_US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_WAN_THREAD_NUMBER][reg_id[12]] = (IMAGE_0_US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_0_WAN_0_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_0_TM_WAN_THREAD_NUMBER][reg_id[11]] = (IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16); /* low register used dynamically */
    local_regs[IMAGE_0_TM_WAN_THREAD_NUMBER][reg_id[10]] = IMAGE_0_US_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_0_TM_WAN_THREAD_NUMBER][reg_id[9]]  = (IMAGE_0_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | IMAGE_0_US_TM_WAN_0_BB_DESTINATION_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_WAN_THREAD_NUMBER][reg_id[8]]  = IMAGE_0_US_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* REPORTING : thread 5 */
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[18]] = drv_qm_get_us_end();
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ghost_reporting_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[15]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, TOTAL_VALID_COUNTER_COUNTER);
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[14]] = BB_ID_TX_PON_ETH_STAT;
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[13]] = IMAGE_0_REPORTING_QUEUE_DESCRIPTOR_TABLE_ADDRESS + (IMAGE_0_REPORTING_QUEUE_COUNTER_TABLE_ADDRESS<<16);
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[12]] = IMAGE_0_REPORTING_QUEUE_ACCUMULATED_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[11]] = IMAGE_0_GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[10]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_COUNTER);
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[9]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_QUEUE_STATUS);
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[8]] = IMAGE_0_REPORTING_COUNTER_TABLE_ADDRESS;
#if defined(XRDP_PI2) && defined(PI2_IN_REPORTING_TASK)
    /* PI2 probability calculator */
    local_regs[IMAGE_0_TM_REPORTING_THREAD_NUMBER][reg_id[11]] |= (IMAGE_0_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
#endif

    /* FLUSH TASK: thread 6 */
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[19]] = drv_qm_get_ds_start(0);
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[18]] =
        (((drv_qm_get_us_end() - drv_qm_get_us_start() + 8) / 8) << 16) +  ((((drv_qm_get_ds_end(0) - (drv_qm_get_ds_start(0) & ~0x1F)) + 8) / 8));
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[17]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_TM_FLUSH << 6));
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[15]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[14]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[13]] = ((drv_qm_get_ds_start(0) - (drv_qm_get_ds_start(0) % 32)) / 8);

    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_0_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = IMAGE_0_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_0_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    /* (US is always 0, DS is used by pointer to correct location in r13 */
    local_regs[IMAGE_0_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID);

    /* Budget allocation US : thread 7 */
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, budget_allocator_us_1st_wakeup_request) << 16;
    /* The task use ffi32 so the size is 32 * entry */
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[12]] = IMAGE_0_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[10]] = IMAGE_0_RATE_LIMITER_VALID_TABLE_US_ADDRESS;
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BASIC_RATE_LIMITER_TABLE_US_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* Budget allocation DS : thread 8 */
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, budget_allocator_ds_1st_wakeup_request) << 16;
    /* The task use ffi32 so the size is 32 * entry */
#ifdef COMPLEX_SCHEDULER_IN_DS 
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else   
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
#endif
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN;
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[10]] = IMAGE_0_RATE_LIMITER_VALID_TABLE_DS_ADDRESS;
#ifdef COMPLEX_SCHEDULER_IN_DS    
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS;
#else
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

    /* Overall budget allocation (for ovlr rl): thread 9 */
    local_regs[IMAGE_0_TM_OVL_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[8]] = IMAGE_0_US_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_0_TM_OVL_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[11]] = (IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_0_TM_OVL_BUDGET_ALLOCATION_US_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ovl_budget_allocator_1st_wakeup_request) << 16;

    /* UPDATE_FIFO_READ: thread 10 */
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[17]] = drv_qm_get_ds_start(0) & 0x1f;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, update_fifo_ds_read_1st_wakeup_request) << 16;
#ifdef COMPLEX_SCHEDULER_IN_DS 
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS | (IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
#else
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
#endif
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[11]] = IMAGE_0_DS_TM_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_TM_UPDATE_FIFO_DS_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

    /* SCHEDULING LAN: thread 11 */
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[12]] = (IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_LAN;
#endif
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[8]]  = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_TM_TX_TASK_DS_THREAD_NUMBER][reg_id[8]]  = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

    /* SERVICE QUEUES UPDATE_FIFO_READ: thread 13 */
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_sq_start() & 0x1f;
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, service_queues_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] =
        IMAGE_0_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS | (IMAGE_0_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[13]] = (((IMAGE_0_TM_SERVICE_QUEUES_TX_THREAD_NUMBER) << 4) + 1);
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_0_SERVICE_QUEUES_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* SERVICE QUEUES SCHEDULING TX: thread 14 */
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, service_queues_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[11]] =
        (IMAGE_0_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16) | IMAGE_0_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[10]] = (IMAGE_0_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_TM_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[9]]  =
        (IMAGE_0_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES << 6));

    /* SERVICE QUEUES BUDGET ALLOCATOR: thread 15 */
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, budget_allocator_1st_wakeup_request) << 16;
    /* The task use ffi32 so the size is 32 * entry */
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[12]] =
        IMAGE_0_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[10]] = IMAGE_0_SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE_ADDRESS;
    local_regs[IMAGE_0_TM_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[9]]  = IMAGE_0_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS;

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

    /* 460 CPU rx is assambler only*/
    local_regs[IMAGE_1_PROCESSING_CPU_RX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_wakeup_request) << 16;
    local_regs[IMAGE_1_PROCESSING_CPU_RX_THREAD_NUMBER][reg_id[13]] = IMAGE_1_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_PROCESSING_CPU_RX_THREAD_NUMBER][reg_id[12]] = IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS;
    local_regs[IMAGE_1_PROCESSING_CPU_RX_THREAD_NUMBER][reg_id[11]] = IMAGE_1_PD_FIFO_TABLE_ADDRESS;

    /* CPU_RX_METER_BUDGET_ALLOCATOR: thread 1 */
    local_regs[IMAGE_1_PROCESSING_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] =
        ADDRESS_OF(image_1, cpu_rx_meter_budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_PROCESSING_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[11]] = CPU_RX_METER_TIMER_PERIOD_IN_USEC;

    /* CPU_RECYCLE: thread 2 */
    local_regs[IMAGE_1_PROCESSING_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_1_PROCESSING_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_1_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* CPU_INTERRUPT_COALESCING: thread 3 */
    local_regs[IMAGE_1_PROCESSING_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_PROCESSING_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_1_CPU_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_1_PROCESSING_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] =
        IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS | (IMAGE_1_CPU_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_RX_COPY_READ: thread 4 */
    local_regs[IMAGE_1_PROCESSING_CPU_RX_COPY_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_copy_wakeup_request) << 16;
    local_regs[IMAGE_1_PROCESSING_CPU_RX_COPY_THREAD_NUMBER][reg_id[14]] = IMAGE_1_CPU_RX_SCRATCHPAD_ADDRESS | (IMAGE_1_CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_ADDRESS << 16);
    local_regs[IMAGE_1_PROCESSING_CPU_RX_COPY_THREAD_NUMBER][reg_id[13]] =
        IMAGE_1_CPU_RX_COPY_UPDATE_FIFO_TABLE_ADDRESS | (IMAGE_1_WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_PROCESSING_CPU_RX_COPY_THREAD_NUMBER][reg_id[12]] =
        (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS) | (IMAGE_1_CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_PROCESSING_CPU_RX_COPY_THREAD_NUMBER][reg_id[11]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS + (sizeof(PROCESSING_CPU_RX_DESCRIPTOR_STRUCT) * 4));

    /* PROCESSING : thread 7-12 */
    for (task = IMAGE_1_PROCESSING_PROCESSING0_THREAD_NUMBER; task <= IMAGE_1_PROCESSING_PROCESSING5_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_1_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_1, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_1_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_1_PROCESSING_PROCESSING0_THREAD_NUMBER));
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

    /* CPU_INTERRUPT_COALESCING: thread 0 */
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] = IMAGE_2_CPU_RECYCLE_RING_DESCRIPTOR_TABLE_ADDRESS |
        (IMAGE_2_CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_TX_TASK_0: thread 3 */
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[10]] = IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* we will support only one task for 6846 (task #3)*/
    /* TIMER_COMMON: thread 4 */
    local_regs[IMAGE_2_IMAGE_2_TIMER_COMMON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, timer_common_task_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_TIMER_COMMON_THREAD_NUMBER][reg_id[14]] = CNTR_MAX_VAL;

    /* CPU_RECYCLE: thread 5 */
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_2_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;


    /* PROCESSING : thread 8-13 */
    for (task = IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER; task <= IMAGE_2_IMAGE_2_PROCESSING5_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[15]] = IMAGE_2_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_2, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_2_DS_PACKET_BUFFER_ADDRESS, (task-IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER));
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

        default:
            bdmf_trace("ERROR driver %s:%u| unsupported Runner image = %d\n", __FILE__, __LINE__, rdp_core_to_image_map[core_index]);
            break;
        }
    }
}

int rdd_data_structures_init(rdd_init_params_t *init_params, HW_IPTV_CONFIGURATION_STRUCT *iptv_hw_config)
{
    bdmf_error_t rc;

    GROUP_MWRITE_32(RDD_ONE_VALUE_ADDRESS_ARR, 0, 1);

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

    rdd_ingress_qos_drop_miss_ratio_set(2);
    rdd_rx_flow_init();

    rdd_proj_init(init_params);
    rdd_actions_proj_init();

    rdd_service_queues_init(IMAGE_0_TM_BUDGET_ALLOCATOR_THREAD_NUMBER);

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
        [0] = ADDRESS_OF(image_1, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_1, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_1, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_1, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_1, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_1, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_1, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_1, gpe_cmd_skip_if),
    };

    uint16_t processing1_vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_2, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_2, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_2, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_2, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_2, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_2, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_2, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_2, gpe_cmd_skip_if),
    };

    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
    {
        /* setting group 0 - processing */
        if (rdp_core_to_image_map[core_index] == processing0_runner_image)
        {
             rdd_write_action(core_index, processing0_vlan_actions_arr, sizeof(processing0_vlan_actions_arr)/sizeof(processing0_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE+1);
        }
        else if (rdp_core_to_image_map[core_index] == processing1_runner_image)
        {
            rdd_write_action(core_index, processing1_vlan_actions_arr, sizeof(processing1_vlan_actions_arr)/sizeof(processing1_vlan_actions_arr[0]),
                (uint8_t *)RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_PTR(core_index), RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE);
        }
    }
}

