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
#include "rdp_drv_qm.h"

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
#include "rdd_dscp_to_pbit.h"
#ifdef G9991_PRV
#include "rdp_drv_bbh_tx.h"
#endif
#ifndef G9991_PRV
#include "rdd_service_queues.h"
#endif

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

#ifndef G9991_PRV
rdd_vport_id_t rx_flow_to_vport[RX_FLOW_CONTEXTS_NUMBER] = {
    [0 ... RDD_MAX_RX_WAN_FLOW]  = RDD_WAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_0] = RDD_LAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_1] = RDD_LAN1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_2] = RDD_LAN2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_3] = RDD_LAN3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_4] = RDD_LAN4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_5] = RDD_LAN5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_6] = RDD_LAN6_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU0] = RDD_CPU0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU1] = RDD_CPU1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU2] = RDD_CPU2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU3] = RDD_CPU3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU4] = RDD_CPU4_VPORT,
};
#else
rdd_vport_id_t rx_flow_to_vport[RX_FLOW_CONTEXTS_NUMBER] = {
    [0 ... RDD_MAX_RX_WAN_FLOW]  = RDD_WAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_0] = RDD_LAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_1] = RDD_LAN1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_2] = RDD_LAN2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_3] = RDD_LAN3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_4] = RDD_LAN4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_5] = RDD_LAN5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_6] = RDD_LAN6_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_7] = RDD_LAN7_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_8] = RDD_LAN8_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_9] = RDD_LAN9_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_10] = RDD_LAN10_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_11] = RDD_LAN11_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_12] = RDD_LAN12_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_13] = RDD_LAN13_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_14] = RDD_LAN14_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_15] = RDD_LAN15_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_16] = RDD_LAN16_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_17] = RDD_LAN17_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_18] = RDD_LAN18_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_19] = RDD_LAN19_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_20] = RDD_LAN20_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_21] = RDD_LAN21_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_22] = RDD_LAN22_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_23] = RDD_LAN23_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_24] = RDD_LAN24_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_25] = RDD_LAN25_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_26] = RDD_LAN26_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_27] = RDD_LAN27_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + SID_28] = RDD_SYSTEM_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_5] = RDD_WAN1_VPORT,
};
#endif
rdpa_emac bbh_id_to_rdpa_emac[BBH_ID_NUM] = {
    rdpa_emac0,
    rdpa_emac1,
    rdpa_emac2,
    rdpa_emac3,
    rdpa_emac4,
    rdpa_emac5,
    rdpa_emac6,
    rdpa_emac7,
    rdpa_emac_none
};

extern FPM_GLOBAL_CFG_STRUCT g_fpm_hw_cfg;

static void image_0_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][32];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

#ifdef G9991_PRV
    /* G9991 FLOW CONTROL: thread 0 */
    local_regs[IMAGE_0_DS_TM_FLOW_CONTROL_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, g9991_flow_control_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_FLOW_CONTROL_THREAD_NUMBER][reg_id[15]] = IMAGE_0_DS_TM_FLOW_CONTROL_THREAD_NUMBER;
    local_regs[IMAGE_0_DS_TM_FLOW_CONTROL_THREAD_NUMBER][reg_id[14]] = BB_ID_SBPM;
    local_regs[IMAGE_0_DS_TM_FLOW_CONTROL_THREAD_NUMBER][reg_id[13]] = BB_ID_DISPATCHER_REORDER;
    local_regs[IMAGE_0_DS_TM_FLOW_CONTROL_THREAD_NUMBER][reg_id[12]] = IMAGE_0_G9991_FLOW_CONTROL_PACKET_HEDAER_ADDRESS;
#endif

    /* Budget allocation: 6858-thread 0  G9991-thread 1 */
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, budget_allocator_1st_wakeup_request) << 16;
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_BUDGET_ALLOCATION_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;
#endif
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

    /* UPDATE_FIFO_READ: 6858-thread 2  G9991-thread 2 */
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_ds_start(0) & 0x1f;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, update_fifo_ds_read_1st_wakeup_request) << 16;
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS | (IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16;
#endif
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

    /* FLUSH TASK: 6858-thread 3  G9991-thread 3 */
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[18]] = ((((drv_qm_get_ds_end(0) - (drv_qm_get_ds_start(0) & ~0x1F)) + 8) / 8) << 16) + drv_qm_get_ds_start(0);
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[12]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_0_DS_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_DS_TM_FLUSH << 6));
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_0_DS_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_0_DS_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        ((drv_qm_get_ds_start(0) & ~0x1f) / 8); /* Offset to first word with DS queue aggregation indication */

    /* SCHEDULING LAN 0: thread 4 */
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[12]] = (IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[12]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS +
        (0 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)) + BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_OFFSET);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_0 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_0;
#endif
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[9]] |= IMAGE_0_G9991_SCHEDULING_INFO_TABLE_ADDRESS + 0 * sizeof(G9991_SCHEDULING_INFO_ENTRY_STRUCT);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN0_THREAD_NUMBER][reg_id[8]] += (0 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT));
#endif

    /* SCHEDULING LAN 1: thread 5 */
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        1 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[12]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS +
        (1 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)) + BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_OFFSET);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_1 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_1;
#endif
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[9]] |= IMAGE_0_G9991_SCHEDULING_INFO_TABLE_ADDRESS + 1 * sizeof(G9991_SCHEDULING_INFO_ENTRY_STRUCT);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN1_THREAD_NUMBER][reg_id[8]] += (1 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT));
#endif

    /* SCHEDULING LAN 2: thread 6 */
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        2 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[12]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS +
        (2 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)) + BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_OFFSET);
#endif

#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_2 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_2;
#endif
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[9]] |= IMAGE_0_G9991_SCHEDULING_INFO_TABLE_ADDRESS + 2 * sizeof(G9991_SCHEDULING_INFO_ENTRY_STRUCT);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN2_THREAD_NUMBER][reg_id[8]] += (2 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT));
#endif

    /* SCHEDULING LAN 3: thread 7 */
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        3 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[12]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS +
        (3 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)) + BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_OFFSET);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_3 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_3;
#endif
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[9]] |= IMAGE_0_G9991_SCHEDULING_INFO_TABLE_ADDRESS + 3 * sizeof(G9991_SCHEDULING_INFO_ENTRY_STRUCT);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN3_THREAD_NUMBER][reg_id[8]] += (3 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT));
#endif

    /* SCHEDULING LAN 4: thread 8 */
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        4 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[12]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS +
        (4 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)) + BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_OFFSET);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_4 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_4;
#endif
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[9]] |= IMAGE_0_G9991_SCHEDULING_INFO_TABLE_ADDRESS + 4 * sizeof(G9991_SCHEDULING_INFO_ENTRY_STRUCT);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN4_THREAD_NUMBER][reg_id[8]] += (4 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT));
#endif

    /* SCHEDULING LAN 5: thread 9 */
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        5 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[12]] |= (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS +
        (5 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)) + BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_OFFSET);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_5 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_5;
#endif
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[9]] |= IMAGE_0_G9991_SCHEDULING_INFO_TABLE_ADDRESS + 5 * sizeof(G9991_SCHEDULING_INFO_ENTRY_STRUCT);
#endif
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS;
#else
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[8]] = IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif
#ifndef G9991_PRV
    local_regs[IMAGE_0_DS_TM_LAN5_THREAD_NUMBER][reg_id[8]] += (5 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT));
#endif

#ifndef G9991_PRV
    /* SCHEDULING LAN 6: thread 10 */
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        6 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_6 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_6;
#endif
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[8]] = (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS + (6 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)));
#else
    local_regs[IMAGE_0_DS_TM_LAN6_THREAD_NUMBER][reg_id[8]] =
        (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS + (6 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT))) | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

    /* SCHEDULING LAN 7: thread 11 */
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_0, ds_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[12]] = ((IMAGE_0_DS_TM_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS +
        7 * sizeof(BBH_TX_EGRESS_COUNTER_ENTRY_STRUCT)) << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_7 | (IMAGE_0_COMPLEX_SCHEDULER_TABLE_DS_ADDRESS << 16);
#else
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[11]] = BB_ID_TX_BBH_7;
#endif
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[10]] = IMAGE_0_DS_TM_PD_FIFO_TABLE_ADDRESS | (IMAGE_0_BASIC_RATE_LIMITER_TABLE_DS_ADDRESS << 16);
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[9]] = (IMAGE_0_DS_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
#ifdef COMPLEX_SCHEDULER_IN_DS
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[8]] = (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS + (7 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT)));
#else
    local_regs[IMAGE_0_DS_TM_LAN7_THREAD_NUMBER][reg_id[8]] =
        (IMAGE_0_DS_TM_BBH_QUEUE_TABLE_ADDRESS + (7 * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT))) | (IMAGE_0_BASIC_SCHEDULER_TABLE_DS_ADDRESS << 16);
#endif

#endif
    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_1_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][32];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

    /* CPU_RX_READ: thread 2 */
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_wakeup_request) << 16;
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_THREAD_NUMBER][reg_id[13]] = IMAGE_1_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_THREAD_NUMBER][reg_id[12]] = IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS;
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_THREAD_NUMBER][reg_id[11]] = IMAGE_1_PD_FIFO_TABLE_ADDRESS;

    /* CPU_RX_INTERRUPT_COALESCING: thread 3 */
    local_regs[IMAGE_1_CPU_IF_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_CPU_IF_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_1_CPU_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_1_CPU_IF_1_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] =
        IMAGE_1_CPU_RING_DESCRIPTORS_TABLE_ADDRESS | (IMAGE_1_CPU_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_RX_METER_BUDGET_ALLOCATOR: thread 4 */
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] =
        ADDRESS_OF(image_1, cpu_rx_meter_budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[11]] = CPU_RX_METER_TIMER_PERIOD;

    /* CPU_RECYCLE: thread 12 */
    local_regs[IMAGE_1_CPU_IF_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_1_CPU_IF_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_1_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* CPU_RX_COPY_READ: thread 14 */
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_1, cpu_rx_copy_wakeup_request) << 16;
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[14]] = IMAGE_1_CPU_RX_SCRATCHPAD_ADDRESS | (IMAGE_1_CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_ADDRESS << 16);
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[13]] =
        IMAGE_1_CPU_RX_COPY_UPDATE_FIFO_TABLE_ADDRESS | (IMAGE_1_WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[12]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS) | (IMAGE_1_CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_1_CPU_IF_1_CPU_RX_COPY_THREAD_NUMBER][reg_id[11]] = (IMAGE_1_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS + (sizeof(PROCESSING_CPU_RX_DESCRIPTOR_STRUCT) * 4));

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}
#ifdef UNDEF /* Tal Meged */
/* **************************************************************************************/
/*    image_2_context_set - prepare global variables for image 2 tasks for 6858 G9991   */
/*****************************************************************************************/
static void image_2_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][32];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

    /* REPORTING : thread 0 */
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[18]] = drv_qm_get_us_end();
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, ghost_reporting_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[15]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, TOTAL_VALID_COUNTER_COUNTER);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[14]] = BB_ID_TX_PON_ETH_STAT;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_REPORTING_QUEUE_DESCRIPTOR_TABLE_ADDRESS + (IMAGE_2_REPORTING_QUEUE_COUNTER_TABLE_ADDRESS<<16);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[12]] = IMAGE_2_REPORTING_QUEUE_ACCUMULATED_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[11]] = IMAGE_2_GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[10]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_COUNTER);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[9]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_QUEUE_STATUS);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[8]] = IMAGE_2_REPORTING_COUNTER_TABLE_ADDRESS;

    /* CPU_RX_METER_BUDGET_ALLOCATOR: thread 1 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] =
        ADDRESS_OF(image_2, cpu_rx_meter_budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_METER_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[11]] = CPU_RX_METER_TIMER_PERIOD;

    /* TIMER_COMMON: thread 2  */
    local_regs[IMAGE_2_CPU_IF_2_TIMER_COMMON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, timer_common_task_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_TIMER_COMMON_THREAD_NUMBER][reg_id[14]] = CNTR_MAX_VAL;

    /* CPU_TX_INTERRUPT_COALESCING: thread 3 */
    local_regs[IMAGE_2_CPU_IF_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_CPU_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] =
        IMAGE_2_CPU_RING_DESCRIPTORS_TABLE_ADDRESS | (IMAGE_2_CPU_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* CPU_RECYCLE: thread 4 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_2_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

    /* CPU_RX_COPY_READ: thread 5 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_COPY_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_rx_copy_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_COPY_THREAD_NUMBER][reg_id[14]] = IMAGE_2_CPU_RX_SCRATCHPAD_ADDRESS | (IMAGE_2_CPU_RX_PSRAM_GET_NEXT_SCRATCHPAD_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_COPY_THREAD_NUMBER][reg_id[13]] =
        IMAGE_2_CPU_RX_COPY_UPDATE_FIFO_TABLE_ADDRESS | (IMAGE_2_WAN_LOOPBACK_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_COPY_THREAD_NUMBER][reg_id[12]] = (IMAGE_2_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS) | (IMAGE_2_CPU_RX_COPY_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_COPY_THREAD_NUMBER][reg_id[11]] = (IMAGE_2_CPU_RX_COPY_PD_FIFO_TABLE_ADDRESS + (sizeof(PROCESSING_CPU_RX_DESCRIPTOR_STRUCT) * 4));

    /* CPU_RX_READ: thread 6 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_rx_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_THREAD_NUMBER][reg_id[13]] = IMAGE_2_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_THREAD_NUMBER][reg_id[12]] = IMAGE_2_CPU_RING_DESCRIPTORS_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RX_THREAD_NUMBER][reg_id[11]] = IMAGE_2_PD_FIFO_TABLE_ADDRESS;

    /* CPU_TX_EGRESS: thread 8 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_0_THREAD_NUMBER][reg_id[10]] = IMAGE_2_CPU_IF_2_CPU_TX_0_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* CPU_TX_INGRESS: thread 9 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_1_THREAD_NUMBER][reg_id[10]] = IMAGE_2_CPU_IF_2_CPU_TX_1_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}
#endif /* Tal Meged */

/* *********************************************************************************/
/*    image_2_context_set - prepare global variables for image 2 tasks for 6858    */
/***********************************************************************************/
static void image_2_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][32];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

/* strict priority order tasks: */

#ifndef G9991_PRV
   /* BUDGET ALLOCATOR: thread 0 */
    local_regs[IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, budget_allocator_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[12]] =
        IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_2_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[10]] = IMAGE_2_SERVICE_QUEUES_RATE_LIMITER_VALID_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[9]]  = IMAGE_2_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS;
#if defined(XRDP_PI2) && defined(PI2_ON_SERVICE_QUEUES)
    local_regs[IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[15]]
        = IMAGE_2_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_2_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER][reg_id[10]] |= IMAGE_2_SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_ADDRESS << 16;
#endif
#endif

    /* CPU_RECYCLE: thread 1 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_recycle_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[8]] = IMAGE_2_CPU_RECYCLE_SRAM_PD_FIFO_ADDRESS;

   /* REPORTING : thread 2 */
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[18]] = drv_qm_get_us_end();
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, ghost_reporting_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[15]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, TOTAL_VALID_COUNTER_COUNTER);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[14]] = BB_ID_TX_PON_ETH_STAT;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_REPORTING_QUEUE_DESCRIPTOR_TABLE_ADDRESS + (IMAGE_2_REPORTING_QUEUE_COUNTER_TABLE_ADDRESS<<16);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[12]] = IMAGE_2_REPORTING_QUEUE_ACCUMULATED_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[11]] = IMAGE_2_GHOST_REPORTING_QUEUE_STATUS_BIT_VECTOR_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[10]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_COUNTER);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[9]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_QUEUE_STATUS);
    local_regs[IMAGE_2_CPU_IF_2_REPORTING_THREAD_NUMBER][reg_id[8]] = IMAGE_2_REPORTING_COUNTER_TABLE_ADDRESS;

    /* CPU_TX_INTERRUPT_COALESCING: thread 3 */
    local_regs[IMAGE_2_CPU_IF_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, interrupt_coalescing_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[13]] = IMAGE_2_CPU_RECYCLE_INTERRUPT_COALESCING_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_INTERRUPT_COALESCING_THREAD_NUMBER][reg_id[12]] = IMAGE_2_CPU_RECYCLE_RING_DESCRIPTOR_TABLE_ADDRESS |
        (IMAGE_2_CPU_RECYCLE_RING_INTERRUPT_COUNTER_TABLE_ADDRESS << 16);

    /* TIMER_COMMON: thread 4 */
    local_regs[IMAGE_2_CPU_IF_2_TIMER_COMMON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, timer_common_task_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_TIMER_COMMON_THREAD_NUMBER][reg_id[14]] = CNTR_MAX_VAL;

#ifndef G9991_PRV
    /* SERVICE_QUEUES_UPDATE_FIFO: thread 5 */
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_sq_start() & 0x1f;
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, service_queues_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]]
        = IMAGE_2_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS | (IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[13]] = (((IMAGE_2_CPU_IF_2_SERVICE_QUEUES_TX_THREAD_NUMBER) << 4) + 1);
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_2_SERVICE_QUEUES_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* FLUSH TASK: thread 6 */
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[18]] = ((((drv_qm_get_sq_end() - (drv_qm_get_sq_start() & ~0x1F)) + 8) / 8) << 16) + drv_qm_get_sq_start();
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[12]] = ((IMAGE_2_AQM_ENABLE_TABLE_ADDRESS + ((drv_qm_get_sq_start() & ~0x1f) / 8)) << 16) |
        IMAGE_2_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES_FLUSH << 6));
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_2_SERVICE_QUEUES_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        ((drv_qm_get_sq_start() & ~0x1f) / 8); /* Offset to first word with DS queue aggregation indication */

    /* SERVICE_QUEUES_TX: thread 7 */
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, service_queues_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[11]] =
        (IMAGE_2_SERVICE_QUEUES_COMPLEX_SCHEDULER_TABLE_ADDRESS << 16) | IMAGE_2_SERVICE_QUEUES_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[10]] = (IMAGE_2_SERVICE_QUEUES_RATE_LIMITER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_2_CPU_IF_2_SERVICE_QUEUES_TX_THREAD_NUMBER][reg_id[9]] =
        (IMAGE_2_SERVICE_QUEUES_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_SERVICE_QUEUES << 6));
#endif

/* Round Robin order tasks: */

    /* CPU_TX_EGRESS: thread 8 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_0_THREAD_NUMBER][reg_id[10]] = IMAGE_2_CPU_IF_2_CPU_TX_0_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    /* CPU_TX_INGRESS: thread 9 */
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_2, cpu_tx_wakeup_request) << 16;
    local_regs[IMAGE_2_CPU_IF_2_CPU_TX_1_THREAD_NUMBER][reg_id[10]] = IMAGE_2_CPU_IF_2_CPU_TX_1_THREAD_NUMBER | (IMAGE_2_CPU_TX_SYNC_FIFO_TABLE_ADDRESS << 16);

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_3_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][32];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)DEVICE_ADDRESS(RU_BLK(RNR_CNTXT).addr[core_index] + RU_REG_OFFSET(RNR_CNTXT, MEM_ENTRY));
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);

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
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_AQM_QUEUE_TABLE_ADDRESS | (IMAGE_3_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_PI2_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS;
#endif

    /* UPDATE_FIFO_READ: thread 4 */
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, update_fifo_us_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[14]] = IMAGE_3_COMPLEX_SCHEDULER_TABLE_US_ADDRESS | (IMAGE_3_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS  << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[11]] = IMAGE_3_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

    /* EPON UPDATE_FIFO_READ: thread 5 */
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_start() & 0x1f;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, epon_update_fifo_read_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_3_EPON_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_3_US_TM_UPDATE_FIFO_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS;

    /* FLUSH TASK: thread 6 */
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[18]] = (drv_qm_get_us_end() - drv_qm_get_us_start() + 8) / 8;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, flush_task_1st_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[12]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_SCHEDULING_QUEUE_AGING_VECTOR_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[10]] = (BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_US_TM_FLUSH << 6));
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[9]] = IMAGE_3_US_TM_SCHEDULING_AGGREGATION_CONTEXT_VECTOR_ADDRESS;
    local_regs[IMAGE_3_US_TM_FLUSH_THREAD_NUMBER][reg_id[8]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_AGGREGATION_CONTEXT_VALID) +
        (drv_qm_get_us_start() / 8);

    /* SCHEDULING WAN_0: thread 7 */
    local_regs[IMAGE_3_US_TM_WAN_0_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, us_tx_task_1st_wakeup_request) << 16 | IMAGE_3_US_TM_WAN_0_BBH_TX_WAKE_UP_DATA_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_0_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_WAN_0_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_WAN_0_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_0_THREAD_NUMBER][reg_id[11]] = 0; /* register used dynamically */
    local_regs[IMAGE_3_US_TM_WAN_0_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS  | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_0_THREAD_NUMBER][reg_id[9]]  = (IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_WAN_0_BB_DESTINATION_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_0_THREAD_NUMBER][reg_id[8]]  = IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);

#if defined(G9991_PRV)
    /* SCHEDULING WAN_1: thread 8 */
    local_regs[IMAGE_3_US_TM_WAN_1_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, us_tx_task_1st_wakeup_request) << 16 | IMAGE_3_US_TM_WAN_1_BBH_TX_WAKE_UP_DATA_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_1_THREAD_NUMBER][reg_id[12]] = (IMAGE_3_US_TM_WAN_1_BBH_TX_EGRESS_COUNTER_TABLE_ADDRESS << 16) | IMAGE_3_WAN_1_BBH_TX_FIFO_SIZE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_1_THREAD_NUMBER][reg_id[11]] = 0; /* register used dynamically */
    local_regs[IMAGE_3_US_TM_WAN_1_THREAD_NUMBER][reg_id[10]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS  | (IMAGE_3_BASIC_RATE_LIMITER_TABLE_US_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_1_THREAD_NUMBER][reg_id[9]]  = (IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16) | IMAGE_3_US_TM_WAN_1_BB_DESTINATION_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_1_THREAD_NUMBER][reg_id[8]]  =
        (IMAGE_3_US_TM_BBH_QUEUE_TABLE_ADDRESS + (BBH_QUEUE_WAN_1_ENTRY_ID * sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT))) | (IMAGE_3_BASIC_SCHEDULER_TABLE_US_ADDRESS << 16);
#endif

    /* EPON SCHEDULING WAN: thread 9 */
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[17]] = drv_qm_get_us_epon_start();
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[16]] = ADDRESS_OF(image_3, epon_tx_task_wakeup_request) << 16;
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[12]] = BB_ID_TX_PON_ETH_PD | (IMAGE_3_BBH_TX_EPON_EGRESS_COUNTER_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[11]] = IMAGE_3_US_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID | (IMAGE_3_US_TM_SCHEDULING_QUEUE_TABLE_ADDRESS << 16);
    local_regs[IMAGE_3_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[9]]  = IMAGE_3_BBH_TX_EPON_INGRESS_COUNTER_TABLE_ADDRESS;

   rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_4_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][32];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    rdd_global_registers_init(core_index);
    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);
    for (task = IMAGE_4_PROCESSING_0_THREAD_NUMBER; task <= IMAGE_4_PROCESSING_7_THREAD_NUMBER; task++)
    {
        /* PROCESSING : thread 0 */
        local_regs[task][reg_id[15]] = IMAGE_4_RX_FLOW_TABLE_ADDRESS << 16 | task;
        local_regs[task][reg_id[16]] = ADDRESS_OF(image_4, processing_wakeup_request) << 16 |
            PACKET_BUFFER_PD_PTR(IMAGE_4_DS_PACKET_BUFFER_ADDRESS, task);
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

        default:
            bdmf_trace("ERROR driver %s:%u| unsupported Runner image = %d\n", __FILE__, __LINE__, rdp_core_to_image_map[core_index]);
            break;
        }
    }
}

#ifdef G9991_PRV
static void rdd_g9991_ds_tx_init(rdd_init_params_t *init_params)
{
    G9991_SCHEDULING_INFO_ENTRY_STRUCT *entry;
    int i;

    RDD_BYTES_4_BITS_WRITE_G(DS_BBH_TX_BYTES_FIFO_VALUE, RDD_G9991_BBH_TX_BYTES_FIFO_SIZE_ADDRESS_ARR, 0);
    RDD_BYTES_4_BITS_WRITE_G(PD_FIFO_CREDIT_INIT_VALUE, RDD_G9991_NOT_FULL_VECTOR_ADDRESS_ARR, 0);
    RDD_BYTES_4_BITS_WRITE_G(FLOW_CONTROL_INIT_VALUE, RDD_G9991_FLOW_CONTROL_VECTOR_ADDRESS_ARR, 0);
    for (i = 0; i < RDD_G9991_SCHEDULING_INFO_TABLE_SIZE; i++)
    {
        RDD_BYTE_1_BITS_WRITE_G(port_mapping_bb_tx_id_get(rdpa_port_emac, i), RDD_G9991_PHYS_PORT_BB_ID_TABLE_ADDRESS_ARR, i);

        /* init control indications registers */
        entry = ((G9991_SCHEDULING_INFO_ENTRY_STRUCT *)RDD_G9991_SCHEDULING_INFO_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + i;
        RDD_G9991_SCHEDULING_INFO_ENTRY_CONTROL_SID_WRITE(0, entry);

        entry = ((G9991_SCHEDULING_INFO_ENTRY_STRUCT *)RDD_G9991_SCHEDULING_INFO_TABLE_PTR(get_runner_idx(processing_runner_image))) + i;
        RDD_G9991_SCHEDULING_INFO_ENTRY_CONTROL_SID_WRITE(0, entry);
    }
}
#endif

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

#ifdef G9991_PRV
    /* DS: init G9991 fragmentation */
    rdd_g9991_ds_tx_init(init_params);
#endif
    /* init first queue mapping */
    rdd_ag_us_tm_first_queue_mapping_set(drv_qm_get_us_start());
    rdd_ag_ds_tm_first_queue_mapping_set(drv_qm_get_ds_start(0));
    rdd_scheduler_wake_up_bbh_init_data_structure();

    /* init bbh-queue */
    rdd_bbh_queue_init();

#ifdef G9991_PRV
    /* init ds fifo bytes limit threshold used in tx tasks */
    rc = rdd_ag_ds_tm_bbh_tx_ds_fifo_bytes_threshold_set(BBH_TX_FIFO_BYTES_LAN_LIMIT);
#endif

    /* WA for A0 - enable queue in fw */
    rdd_set_queue_enable(QM_QUEUE_CPU_RX, 1);
    rdd_set_queue_enable(QM_QUEUE_CPU_RX_COPY_NORMAL, 1);
    rdd_set_queue_enable(QM_QUEUE_CPU_RX_COPY_EXCLUSIVE, 1);

#ifdef G9991_PRV
    /* start flush task */
    rc = rc ? rc : rdd_scheduling_flush_timer_set();
#else
    rc = rdd_scheduling_flush_timer_set();
#endif

    /* start budget allocation task */
    rc = rc ? rc : rdd_ds_budget_allocation_timer_set();

    rdd_rx_default_flow_init();

    rdd_ingress_qos_drop_miss_ratio_set(2);
    rdd_rx_flow_init();

    rdd_proj_init(init_params);
    rdd_actions_proj_init();

#ifndef G9991_PRV
    rdd_service_queues_init(IMAGE_2_CPU_IF_2_BUDGET_ALLOCATOR_THREAD_NUMBER);
#endif

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
    uint32_t action_index;

    uint16_t vlan_actions_arr[] = {
        [0] = ADDRESS_OF(image_4, gpe_vlan_action_cmd_drop),
        [1] = ADDRESS_OF(image_4, gpe_vlan_action_cmd_dscp),
        [2] = ADDRESS_OF(image_4, gpe_vlan_action_cmd_mac_hdr_copy),
        [3] = ADDRESS_OF(image_4, gpe_cmd_replace_16),
        [4] = ADDRESS_OF(image_4, gpe_cmd_replace_32),
        [5] = ADDRESS_OF(image_4, gpe_cmd_replace_bits_16),
        [6] = ADDRESS_OF(image_4, gpe_cmd_copy_bits_16),
        [7 ... 16] = ADDRESS_OF(image_4, gpe_cmd_skip_if),
    };

    for (action_index = 0; action_index < RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_SIZE; action_index++)
    {
        RDD_BYTES_2_BITS_WRITE_G(vlan_actions_arr[action_index], RDD_VLAN_ACTION_GPE_HANDLER_PTR_TABLE_ADDRESS_ARR, action_index);
    }
}

