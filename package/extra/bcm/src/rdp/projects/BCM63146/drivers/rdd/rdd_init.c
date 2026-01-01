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
#include "rdp_drv_rnr.h"
#include "rdp_drv_qm.h"
#include "rdd_ag_natc.h"
#ifdef CONFIG_DHD_RUNNER
#include "rdd_dhd_helper.h"
#include "rdp_drv_dhd.h"
#endif
#include "rdd_debug.h"


extern uintptr_t xrdp_virt2phys(const ru_block_rec *ru_block, uint8_t addr_idx);

extern int reg_id[32];
#define PER_CONTEXT_WORD_COUNT ((RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) / NUM_OF_MAIN_RUNNER_THREADS)
DEFINE_BDMF_FASTLOCK(int_lock);
DEFINE_BDMF_FASTLOCK(iptv_lock);
DEFINE_BDMF_FASTLOCK(int_lock_irq);

#ifdef USE_BDMF_SHELL
extern int rdd_make_shell_commands(void);
#endif /* USE_BDMF_SHELL */

uint8_t rdpa_bbh_queue_to_hw_bbh_qid[rdpa_emac__num_of] = {
    0, 1, 2, 3, 4, 5, 6, 7};

rdd_vport_id_t rx_flow_to_vport[RX_FLOW_CONTEXTS_NUMBER] = {
    [RDD_WAN_FLOW_DSL_START ... RDD_WAN_FLOW_DSL_END] = RDD_DSL_WAN_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_0] = RDD_LAN0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_1] = RDD_LAN1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_2] = RDD_LAN2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_3] = RDD_LAN3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_4] = RDD_LAN4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_5] = RDD_LAN5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_6] = RDD_LAN6_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_RX_BBH_7] = RDD_LAN7_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU0] = RDD_CPU0_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU1] = RDD_CPU1_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU2] = RDD_CPU2_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU3] = RDD_CPU3_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU4] = RDD_CPU4_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU5] = RDD_CPU5_VPORT,
    [RDD_NUM_OF_RX_WAN_FLOWS + BB_ID_CPU6] = RDD_CPU6_VPORT,
};

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

/* Copied from BCM6878 */
static void rdd_global_registers_init(uint32_t core_index, uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS], uint32_t last_thread)
{
     uint32_t i;


    /********** common to all runners **********/
    /* in this project we don't have really global, so will set all registers that should be global for all threads */
    for (i = 0; i <= last_thread; ++i)
    {
        local_regs[i][31] = 1; /* CONST_1 is in r31 */
    }
}

static void image_0_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);
    rdd_global_registers_init(core_index, local_regs, IMAGE_0_IMAGE_0_LAST);

    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_0_PROCESSING_7_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

    /* CPU_RX_READ */
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_cpu_rx_wakeup_request);
    /* Remove after disabling binary builder on C */
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[17]] = IMAGE_0_CPU_RX_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[18]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
    /* ---------------------------------- */
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[8]] = IMAGE_0_CPU_RX_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[9]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[30]] = IMAGE_0_CPU_RX_STACK_ADDRESS+IMAGE_0_CPU_RX_STACK_BYTE_SIZE;

    /* CPU_RX_COPY_READ */
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_COPY_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_cpu_rx_copy_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_COPY_THREAD_NUMBER][reg_id[30]] = IMAGE_0_CPU_RX_COPY_STACK_ADDRESS+IMAGE_0_CPU_RX_COPY_STACK_BYTE_SIZE;

    /* GENERAL_TIMER */
    local_regs[IMAGE_0_IMAGE_0_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, general_timer_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_0_GENERAL_TIMER_STACK_ADDRESS+IMAGE_0_GENERAL_TIMER_STACK_BYTE_SIZE;

    /* CPU_RECYCLE */
    local_regs[IMAGE_0_IMAGE_0_CPU_RECYCLE_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_cpu_recycle_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_CPU_RECYCLE_THREAD_NUMBER][reg_id[30]] = IMAGE_0_CPU_RECYCLE_STACK_ADDRESS+IMAGE_0_CPU_RECYCLE_STACK_BYTE_SIZE;

    /* SERVICE_QUEUES: thread */
    local_regs[IMAGE_0_IMAGE_0_SERVICE_QUEUES_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, sq_tx_task_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_SERVICE_QUEUES_THREAD_NUMBER][reg_id[8]] = IMAGE_0_SQ_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_SERVICE_QUEUES_THREAD_NUMBER][reg_id[9]] = IMAGE_0_SQ_TM_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_SERVICE_QUEUES_THREAD_NUMBER][reg_id[30]] = IMAGE_0_SERVICE_QUEUES_STACK_ADDRESS + C_DEFS_SQ_TX_TASK_STACK_SIZE;

    /* PROCESSING */
    for (task = IMAGE_0_IMAGE_0_PROCESSING0_THREAD_NUMBER; task <= IMAGE_0_IMAGE_0_PROCESSING6_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_0, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_0_PROCESSING_7_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_0_IMAGE_0_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_0_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_0_dynamic_code_section_proc1 - image_0_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_0_IMAGE_0_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING4_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING4_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING5_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING5_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING6_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING6_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_1_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);
    rdd_global_registers_init(core_index, local_regs, IMAGE_1_IMAGE_1_LAST);

    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_1_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

    /* GENERAL_TIMER */
    local_regs[IMAGE_1_IMAGE_1_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, general_timer_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_1_GENERAL_TIMER_STACK_ADDRESS+IMAGE_1_GENERAL_TIMER_STACK_BYTE_SIZE;

    /* CPU_RECYCLE */
    local_regs[IMAGE_1_IMAGE_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, c_cpu_recycle_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_CPU_RECYCLE_THREAD_NUMBER][reg_id[30]] = IMAGE_1_CPU_RECYCLE_STACK_ADDRESS+IMAGE_1_CPU_RECYCLE_STACK_BYTE_SIZE;

    /* CPU Tx threads */
    local_regs[IMAGE_1_IMAGE_1_CPU_TX_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, c_cpu_tx_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_CPU_TX_0_THREAD_NUMBER][reg_id[8]] = IMAGE_1_IMAGE_1_CPU_TX_0_THREAD_NUMBER;
    local_regs[IMAGE_1_IMAGE_1_CPU_TX_0_THREAD_NUMBER][reg_id[30]] = IMAGE_1_CPU_TX_0_STACK_ADDRESS+IMAGE_1_CPU_TX_0_STACK_BYTE_SIZE;

    local_regs[IMAGE_1_IMAGE_1_CPU_TX_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, c_cpu_tx_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_CPU_TX_1_THREAD_NUMBER][reg_id[8]] = IMAGE_1_IMAGE_1_CPU_TX_1_THREAD_NUMBER;
    local_regs[IMAGE_1_IMAGE_1_CPU_TX_1_THREAD_NUMBER][reg_id[30]] = IMAGE_1_CPU_TX_1_STACK_ADDRESS+IMAGE_1_CPU_TX_1_STACK_BYTE_SIZE;

    for (task = IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER; task <= IMAGE_1_IMAGE_1_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_1, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_1_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_1_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_1_dynamic_code_section_proc1 - image_1_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING4_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING4_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING5_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING5_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING6_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING6_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_1_IMAGE_1_PROCESSING7_THREAD_NUMBER][reg_id[30]] = IMAGE_1_PROCESSING7_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_2_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)DEVICE_ADDRESS(RU_BLK(RNR_CNTXT).addr[core_index] + RU_REG_OFFSET(RNR_CNTXT, MEM_ENTRY));
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);
    rdd_global_registers_init(core_index, local_regs, IMAGE_2_IMAGE_2_LAST);

    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_2_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

#ifdef CONFIG_DHD_RUNNER
    /* DHD_TIMER_THREAD_NUMBER */
    local_regs[IMAGE_2_IMAGE_2_DHD_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_dhd_timer_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_DHD_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_2_DHD_TIMER_STACK_ADDRESS+IMAGE_2_DHD_TIMER_STACK_BYTE_SIZE;

    /* DHD_TX_POST_UPDATE_FIFO */
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_dhd_tx_post_update_fifo_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_2_DHD_TX_POST_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_2_DHD_TX_POST_UPDATE_FIFO_STACK_ADDRESS + IMAGE_2_DHD_TX_POST_UPDATE_FIFO_STACK_BYTE_SIZE;

    /* DHD_TX_POST_0 */
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_dhd_tx_post_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_0_THREAD_NUMBER][reg_id[30]] = IMAGE_2_DHD_TX_POST_0_STACK_ADDRESS + IMAGE_2_DHD_TX_POST_0_STACK_BYTE_SIZE;

    /* DHD_TX_POST_1 */
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_dhd_tx_post_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_1_THREAD_NUMBER][reg_id[8]] = 1;
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_1_THREAD_NUMBER][reg_id[30]] = IMAGE_2_DHD_TX_POST_1_STACK_ADDRESS + IMAGE_2_DHD_TX_POST_1_STACK_BYTE_SIZE;

    /* DHD_TX_POST_2 */
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_2_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_dhd_tx_post_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_2_THREAD_NUMBER][reg_id[8]] = 2;
    local_regs[IMAGE_2_IMAGE_2_DHD_TX_POST_2_THREAD_NUMBER][reg_id[30]] = IMAGE_2_DHD_TX_POST_2_STACK_ADDRESS + IMAGE_2_DHD_TX_POST_2_STACK_BYTE_SIZE;
#endif

    /* FPM_RINGS AND BUFMNG REFILL 5 */
#if defined(CONFIG_CPU_TX_FROM_XPM)
    local_regs[IMAGE_2_IMAGE_2_CPU_FPM_RINGS_REFILL_AND_BUFMNG_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_cpu_fpm_rings_and_bufmng_refill_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_CPU_FPM_RINGS_REFILL_AND_BUFMNG_THREAD_NUMBER][reg_id[8]] = IMAGE_2_IMAGE_2_CPU_FPM_RINGS_REFILL_AND_BUFMNG_THREAD_NUMBER;
    local_regs[IMAGE_2_IMAGE_2_CPU_FPM_RINGS_REFILL_AND_BUFMNG_THREAD_NUMBER][reg_id[30]] =
        IMAGE_2_CPU_FPM_RINGS_REFILL_AND_BUFMNG_STACK_ADDRESS + IMAGE_2_CPU_FPM_RINGS_REFILL_AND_BUFMNG_STACK_BYTE_SIZE;
#endif

    /* PROCESSING */
    for (task = IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER; task <= IMAGE_2_IMAGE_2_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_2, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_2_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_2_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_2_dynamic_code_section_proc1 - image_2_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING4_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING4_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING5_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING5_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING6_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING6_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_2_IMAGE_2_PROCESSING7_THREAD_NUMBER][reg_id[30]] = IMAGE_2_PROCESSING7_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_3_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)DEVICE_ADDRESS(RU_BLK(RNR_CNTXT).addr[core_index] + RU_REG_OFFSET(RNR_CNTXT, MEM_ENTRY));
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);
    rdd_global_registers_init(core_index, local_regs, IMAGE_3_IMAGE_3_LAST);

    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_3_PROCESSING_4_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

    /* GENERAL_TIMER: thread 0 */
    local_regs[IMAGE_3_IMAGE_3_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, general_timer_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_3_GENERAL_TIMER_STACK_ADDRESS + IMAGE_3_GENERAL_TIMER_STACK_BYTE_SIZE;

    /* UPDATE_FIFO_READ for US */
    local_regs[IMAGE_3_IMAGE_3_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, tm_update_fifo_first_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_3_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_IMAGE_3_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_3_UPDATE_FIFO_STACK_ADDRESS + C_DEFS_UPDATE_FIFO_STACK_SIZE;

    /* SCHEDULING Ethernet */
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, tm_tx_task_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_0_THREAD_NUMBER][reg_id[9]] = IMAGE_3_IMAGE_3_TX_TASK_US_0_THREAD_NUMBER;
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_0_THREAD_NUMBER][reg_id[30]] = IMAGE_3_TX_TASK_US_0_STACK_ADDRESS + C_DEFS_TX_TASK_STACK_SIZE;

    /* SCHEDULING DSL */
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, tm_tx_task_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_1_THREAD_NUMBER][reg_id[8]] = TM_INDEX_DSL_OR_PON;
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_1_THREAD_NUMBER][reg_id[9]] = IMAGE_3_IMAGE_3_TX_TASK_US_1_THREAD_NUMBER;
    local_regs[IMAGE_3_IMAGE_3_TX_TASK_US_1_THREAD_NUMBER][reg_id[30]] = IMAGE_3_TX_TASK_US_1_STACK_ADDRESS + C_DEFS_TX_TASK_STACK_SIZE;

#ifdef CONFIG_DHD_RUNNER
    /* DHD_RX_COMPLETE_0 */
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_rx_complete_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_RX_COMPLETE_0_STACK_ADDRESS+IMAGE_3_DHD_RX_COMPLETE_0_STACK_BYTE_SIZE;

    /* DHD_RX_COMPLETE_1 */
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_rx_complete_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[8]] = 1;
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_RX_COMPLETE_1_STACK_ADDRESS+IMAGE_3_DHD_RX_COMPLETE_1_STACK_BYTE_SIZE;

    /* DHD_RX_COMPLETE_2 */
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_rx_complete_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[8]] = 2;
    local_regs[IMAGE_3_IMAGE_3_DHD_RX_COMPLETE_2_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_RX_COMPLETE_2_STACK_ADDRESS+IMAGE_3_DHD_RX_COMPLETE_2_STACK_BYTE_SIZE;

    /* DHD_TX_COMPLETE_0 */
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_tx_complete_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TX_COMPLETE_0_STACK_ADDRESS+IMAGE_3_DHD_TX_COMPLETE_0_STACK_BYTE_SIZE;

    /* DHD_TX_COMPLETE_1 */
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_tx_complete_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[8]] = 1;
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TX_COMPLETE_1_STACK_ADDRESS+IMAGE_3_DHD_TX_COMPLETE_1_STACK_BYTE_SIZE;

    /* DHD_TX_COMPLETE_2 */
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_tx_complete_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[8]] = 2;
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_COMPLETE_2_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TX_COMPLETE_2_STACK_ADDRESS+IMAGE_3_DHD_TX_COMPLETE_2_STACK_BYTE_SIZE;
#endif

#ifdef BUFFER_CONGESTION_MGT
    /* BUFFER_CONG_MGT: thread 3 */
    local_regs[IMAGE_3_IMAGE_3_BUFFER_CONG_MGT_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, buffer_cong_mgt_1st_wakeup_request);
#endif

    /* PROCESSING */
    for (task = IMAGE_3_IMAGE_3_PROCESSING0_THREAD_NUMBER; task <= IMAGE_3_IMAGE_3_PROCESSING3_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_3, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_3_PROCESSING_4_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_3_IMAGE_3_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_3_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_3_dynamic_code_section_proc1 - image_3_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_3_IMAGE_3_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, mem_cntxt_byte_num);
}

static void image_4_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][PER_CONTEXT_WORD_COUNT];
    uint32_t mem_cntxt_byte_num;
    uint32_t *sram_context;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);
    mem_cntxt_byte_num = (RU_REG_RAM_CNT(RNR_CNTXT, MEM_ENTRY) + 1) * sizeof(uint32_t);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, mem_cntxt_byte_num);
    rdd_global_registers_init(core_index, local_regs, IMAGE_4_IMAGE_4_LAST);

    /* UPDATE_FIFO_READ for DS */
    local_regs[IMAGE_4_IMAGE_4_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, tm_update_fifo_first_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_4_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_4_IMAGE_4_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_4_UPDATE_FIFO_STACK_ADDRESS + C_DEFS_UPDATE_FIFO_STACK_SIZE;

    /* SCHEDULING LAN */
    local_regs[IMAGE_4_IMAGE_4_TX_TASK_DS_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, tm_tx_task_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_TX_TASK_DS_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_4_IMAGE_4_TX_TASK_DS_0_THREAD_NUMBER][reg_id[9]] = IMAGE_4_IMAGE_4_TX_TASK_DS_0_THREAD_NUMBER;
    local_regs[IMAGE_4_IMAGE_4_TX_TASK_DS_0_THREAD_NUMBER][reg_id[30]] = IMAGE_4_TX_TASK_DS_0_STACK_ADDRESS + C_DEFS_TX_TASK_STACK_SIZE;

    /* GENERAL_TIMER */
    local_regs[IMAGE_4_IMAGE_4_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, general_timer_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_4_GENERAL_TIMER_STACK_ADDRESS+IMAGE_4_GENERAL_TIMER_STACK_BYTE_SIZE;
    
    /* Common reprocessing task is used for pktgen_reprocessing for tcp/udp speed test and for AQM.
     * XXX: functionality to be added:
     *      - tx_abs_recycle for speed service
     *      - recycle host buffers from CPU TX once CPU TX from ABS address is implemented */
    local_regs[IMAGE_4_IMAGE_4_COMMON_REPROCESSING_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, c_common_reprocessing_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_COMMON_REPROCESSING_THREAD_NUMBER][reg_id[30]] = IMAGE_4_COMMON_REPROCESSING_STACK_ADDRESS+IMAGE_4_COMMON_REPROCESSING_STACK_BYTE_SIZE;

#if defined(CONFIG_BCM_SPDSVC_SUPPORT)
    /* SPDSVC_GEN - UDP Speed test generator (iperf3 / udp_spdt) */
    local_regs[IMAGE_4_IMAGE_4_SPDSVC_GEN_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, c_spdsvc_gen_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_SPDSVC_GEN_THREAD_NUMBER][reg_id[30]] = IMAGE_4_SPDSVC_GEN_STACK_ADDRESS+IMAGE_4_SPDSVC_GEN_STACK_BYTE_SIZE;

    /* TcpSpdTest + pktgen_tx */
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, tcpspdtest_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_THREAD_NUMBER][reg_id[14]] = IMAGE_4_TCPSPDTEST_SCRATCHPAD_ADDRESS;
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_THREAD_NUMBER][reg_id[13]] = IMAGE_4_TCPSPDTEST_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_THREAD_NUMBER][reg_id[12]] = (IMAGE_4_TCPSPDTEST_PD_FIFO_TABLE_ADDRESS) | (IMAGE_4_TCPSPDTEST_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);

    /* TCP SPEED GEN task */
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_GEN_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, tcpspdtest_gen_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_GEN_THREAD_NUMBER][reg_id[9]] = ((BB_ID_DISPATCHER_REORDER + (DISP_REOR_VIQ_TCPSPDTEST_GEN << 6)) << 16);
    local_regs[IMAGE_4_IMAGE_4_TCPSPDTEST_GEN_THREAD_NUMBER][reg_id[12]] = (IMAGE_4_TCPSPDTEST_GEN_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
#endif

#if defined(CONFIG_BCM_SPDSVC_SUPPORT)
    /* SPDSVC_ANALYZER */
    local_regs[IMAGE_4_IMAGE_4_SPDSVC_ANALYZER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, c_spdsvc_analyzer_wakeup_request);
    local_regs[IMAGE_4_IMAGE_4_SPDSVC_ANALYZER_THREAD_NUMBER][reg_id[30]] = IMAGE_4_SPDSVC_ANALYZER_STACK_ADDRESS+IMAGE_4_SPDSVC_ANALYZER_STACK_BYTE_SIZE;
#endif

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


static int rdd_cpu_proj_init(void)
{
    uint8_t def_idx = (uint8_t)BDMF_INDEX_UNASSIGNED;
    int rc = 0;

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
    }

#if defined(DEBUG_PRINTS)
    rdd_debug_prints_init();
#endif

    rdd_init_c_stack();

    /* start budget allocation timer */
    rdd_general_timer_set(get_runner_idx(ds_tm_runner_image), 0, RATE_LIMIT_TIMER_PERIOD, TIMER_ACTION_TM_BUDGET_ALLOCATOR);
    rdd_general_timer_set(get_runner_idx(us_tm_runner_image), 0, RATE_LIMIT_TIMER_PERIOD, TIMER_ACTION_TM_BUDGET_ALLOCATOR);
    rdd_general_timer_set(get_runner_idx(service_queues_runner_image), 0, RATE_LIMIT_TIMER_PERIOD, TIMER_ACTION_TM_BUDGET_ALLOCATOR);

#ifdef XRDP_PI2
    /* start PI2 timer - each tm core has a PI2 timer*/
    rdd_general_timer_set(get_runner_idx(ds_tm_runner_image), 0, PI2_TIMER_PERIOD_US, TIMER_ACTION_PI2);
    rdd_general_timer_set(get_runner_idx(us_tm_runner_image), 0, PI2_TIMER_PERIOD_US, TIMER_ACTION_PI2);
#ifdef PI2_ON_SERVICE_QUEUES
    rdd_general_timer_set(get_runner_idx(service_queues_runner_image), 0, PI2_SQ_TIMESTAMP_INJECT_PERIOD, TIMER_ACTION_PI2_SQ_TIMESTAMP_INJECT);
    rdd_general_timer_set(get_runner_idx(service_queues_runner_image), 0, PI2_TIMER_PERIOD_US, TIMER_ACTION_PI2);   
#endif 
#endif

    /* wake up the general tasks */
    ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), IMAGE_3_IMAGE_3_GENERAL_TIMER_THREAD_NUMBER);
    ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(ds_tm_runner_image), IMAGE_4_IMAGE_4_GENERAL_TIMER_THREAD_NUMBER);
    ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(service_queues_runner_image), IMAGE_0_IMAGE_0_GENERAL_TIMER_THREAD_NUMBER);

    /* init first queue mapping */
    rdd_ag_us_tm_first_queue_mapping_set(drv_qm_get_us_start());
    rdd_ag_ds_tm_first_queue_mapping_set(drv_qm_get_ds_start(0));
    rdd_ag_service_queues_first_queue_mapping_set(drv_qm_get_sq_start());

    /* init scheduler data structures */
    rdd_scheduling_conf_init();

    rdd_scheduler_wake_up_bbh_init_data_structure();

    rdd_rx_flow_init();
    rdd_ingress_qos_drop_miss_ratio_set(2);

    rc = rdd_cpu_proj_init();

    /* Setting same global mask as for other project is safe, since the NATC_16BYTE_KEY_MASK differs */
    rc = rc ? rc : rdd_ag_natc_nat_cache_key0_mask_set(NATC_KEY0_DEF_MASK);

    rc = rc ? rc : rdd_ingress_filters_init();
    rdd_multicast_filter_cfg(rdpa_mcast_filter_method_ip);

#ifdef USE_BDMF_SHELL
    /* register shell commands */
    rc = rc ? : rdd_make_shell_commands();
#endif
    return rc;
}

