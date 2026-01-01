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
#ifdef CONFIG_DHD_RUNNER
#include "rdd_dhd_helper.h"
#include "rdp_drv_dhd.h"
#endif
#include "XRDP_AG.h"
#include "rdd_map_auto.h"
#include "rdd_ghost_reporting.h"
#include "rdd_scheduling.h"
#include "rdd_common.h"
#include "rdd_init_pon_common.h"
#include "rdp_common.h"
#include "rdd_runner_proj_defs.h"
#ifdef CONFIG_DHD_RUNNER
#include "rdd_dhd_helper.h"
#include "rdp_drv_dhd.h"
#endif
#include "rdp_drv_rnr.h"
#include "rdp_drv_qm.h"
#include "rdd_debug.h"
#include "rdd_spdsvc.h"
#include "rdd_scheduling.h"


extern uintptr_t xrdp_virt2phys(const ru_block_rec *ru_block, uint8_t addr_idx);

extern int reg_id[32];
DEFINE_BDMF_FASTLOCK(int_lock);
DEFINE_BDMF_FASTLOCK(iptv_lock);
DEFINE_BDMF_FASTLOCK(int_lock_irq);

#ifdef USE_BDMF_SHELL
extern int rdd_make_shell_commands(void);
#endif /* USE_BDMF_SHELL */

extern FPM_GLOBAL_CFG_STRUCT g_fpm_hw_cfg;

static void image_0_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    rdd_global_registers_init(core_index, local_regs, IMAGE_0_IMAGE_0_LAST);
    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_0_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

    /* GENERAL_TIMER: thread 0 */
    local_regs[IMAGE_0_IMAGE_0_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, general_timer_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_0_GENERAL_TIMER_STACK_ADDRESS + IMAGE_0_GENERAL_TIMER_STACK_BYTE_SIZE;

    /* CPU_RX_READ: thread 1 */
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_cpu_rx_wakeup_request);
    /* Remove after disabling binary builder on C */
#if !defined(CONFIG_CPU_RX_FROM_XPM)    
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[17]] = IMAGE_0_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[18]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[8]] = IMAGE_0_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[9]] = IMAGE_0_UPDATE_FIFO_TABLE_ADDRESS;
#endif
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_THREAD_NUMBER][reg_id[30]] = IMAGE_0_CPU_RX_STACK_ADDRESS + IMAGE_0_CPU_RX_STACK_BYTE_SIZE;

#if !defined(CONFIG_CPU_RX_FROM_XPM)
    /* CPU_RECYCLE: thread 2 */
    local_regs[IMAGE_0_IMAGE_0_CPU_RECYCLE_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_cpu_recycle_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_CPU_RECYCLE_THREAD_NUMBER][reg_id[30]] = IMAGE_0_CPU_RECYCLE_STACK_ADDRESS + IMAGE_0_CPU_RECYCLE_STACK_BYTE_SIZE;

    /* CPU_RX_COPY_READ: thread 3 */
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_COPY_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_cpu_rx_copy_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_CPU_RX_COPY_THREAD_NUMBER][reg_id[30]] = IMAGE_0_CPU_RX_COPY_STACK_ADDRESS + IMAGE_0_CPU_RX_COPY_STACK_BYTE_SIZE;
#endif

#ifdef CONFIG_DHD_RUNNER
    /* DHD_RX_COMPLETE_0 */
    local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_dhd_rx_complete_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[30]] = IMAGE_0_DHD_RX_COMPLETE_0_STACK_ADDRESS+IMAGE_0_DHD_RX_COMPLETE_0_STACK_BYTE_SIZE;

    /* DHD_RX_COMPLETE_1 */
    local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_dhd_rx_complete_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[8]] = 1;
    local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[30]] = IMAGE_0_DHD_RX_COMPLETE_1_STACK_ADDRESS+IMAGE_0_DHD_RX_COMPLETE_1_STACK_BYTE_SIZE;

    /* DHD_TX_COMPLETE_0 */
    local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_dhd_tx_complete_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[30]] = IMAGE_0_DHD_TX_COMPLETE_0_STACK_ADDRESS+IMAGE_0_DHD_TX_COMPLETE_0_STACK_BYTE_SIZE;

    /* DHD_TX_COMPLETE_1 */
    local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_0, c_dhd_tx_complete_wakeup_request);
    local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[8]] = 1;
    local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[30]] = IMAGE_0_DHD_TX_COMPLETE_1_STACK_ADDRESS+IMAGE_0_DHD_TX_COMPLETE_1_STACK_BYTE_SIZE;

    if (g_dhd_hw_config.offload_config == 1)
    {
        /* radios wl1+wl2 offload configuration */
        local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_0_THREAD_NUMBER][reg_id[8]] = 1;
        local_regs[IMAGE_0_IMAGE_0_DHD_RX_COMPLETE_1_THREAD_NUMBER][reg_id[8]] = 2;
        local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_0_THREAD_NUMBER][reg_id[8]] = 1;
        local_regs[IMAGE_0_IMAGE_0_DHD_TX_COMPLETE_1_THREAD_NUMBER][reg_id[8]] = 2;
    }
#endif

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_0_IMAGE_0_PROCESSING0_THREAD_NUMBER; task <= IMAGE_0_IMAGE_0_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_0, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_0_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_0_IMAGE_0_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_0_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_0_dynamic_code_section_proc1-image_0_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_0_IMAGE_0_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING4_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING4_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING5_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING5_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING6_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING6_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_0_IMAGE_0_PROCESSING7_THREAD_NUMBER][reg_id[30]] = IMAGE_0_PROCESSING7_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_1_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));
    rdd_global_registers_init(core_index, local_regs, IMAGE_1_IMAGE_1_LAST);
    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_1_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

    /* Common reprocessing task is used for pktgen_reprocessing for tcp/udp speed test and for AQM.
     * XXX: functionality to be added:
     *      - tx_abs_recycle for speed service
     *      - recycle host buffers from CPU TX once CPU TX from ABS address is implemented */
    local_regs[IMAGE_1_IMAGE_1_COMMON_REPROCESSING_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, c_common_reprocessing_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_COMMON_REPROCESSING_THREAD_NUMBER][reg_id[30]] = IMAGE_1_COMMON_REPROCESSING_STACK_ADDRESS+IMAGE_1_COMMON_REPROCESSING_STACK_BYTE_SIZE;

#if defined(CONFIG_BCM_SPDSVC_SUPPORT)
    /* SPDSVC_ANALYZER */
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_ANALYZER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, c_spdsvc_analyzer_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_ANALYZER_THREAD_NUMBER][reg_id[30]] = IMAGE_1_SPDSVC_ANALYZER_STACK_ADDRESS + IMAGE_1_SPDSVC_ANALYZER_STACK_BYTE_SIZE;

/* SPDSVC_GEN - UDP Speed test generator (iperf3 / udp_spdt) */
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_GEN_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, c_spdsvc_gen_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_SPDSVC_GEN_THREAD_NUMBER][reg_id[30]] = IMAGE_1_SPDSVC_GEN_STACK_ADDRESS+IMAGE_1_SPDSVC_GEN_STACK_BYTE_SIZE;

    /* TcpSpdTest + pktgen_tx */
#ifdef TCP_SPDTEST_C_CODE
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_DOWNLOAD_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, tcpspdtest_download_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_DOWNLOAD_THREAD_NUMBER][reg_id[30]] = IMAGE_1_TCPSPDTEST_DOWNLOAD_STACK_ADDRESS+IMAGE_1_TCPSPDTEST_DOWNLOAD_STACK_BYTE_SIZE;

    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_UPLOAD_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, tcpspdtest_upload_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_UPLOAD_THREAD_NUMBER][reg_id[30]] = IMAGE_1_TCPSPDTEST_UPLOAD_STACK_ADDRESS+IMAGE_1_TCPSPDTEST_UPLOAD_STACK_BYTE_SIZE;

    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_UPLOAD_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, tcpspdtest_upload_timer_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_UPLOAD_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_1_TCPSPDTEST_UPLOAD_TIMER_STACK_ADDRESS+IMAGE_1_TCPSPDTEST_UPLOAD_TIMER_STACK_BYTE_SIZE;
#else
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, tcpspdtest_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_THREAD_NUMBER][reg_id[14]] = IMAGE_1_TCPSPDTEST_SCRATCHPAD_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_THREAD_NUMBER][reg_id[13]] = IMAGE_1_TCPSPDTEST_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_THREAD_NUMBER][reg_id[12]] = (IMAGE_1_TCPSPDTEST_PD_FIFO_TABLE_ADDRESS) | (IMAGE_1_TCPSPDTEST_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);

    /* TCP SPEED GEN task: thread 6 */
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_GEN_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_1, tcpspdtest_gen_wakeup_request);
    local_regs[IMAGE_1_IMAGE_1_TCPSPDTEST_GEN_THREAD_NUMBER][reg_id[12]] = (IMAGE_1_TCPSPDTEST_GEN_DISPATCHER_CREDIT_TABLE_ADDRESS << 16);
#endif
#endif

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER; task <= IMAGE_1_IMAGE_1_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_1, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_1_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_1_IMAGE_1_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_1_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_1_dynamic_code_section_proc1-image_1_dynamic_code_section_proc0));
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

     rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_2_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));
    rdd_global_registers_init(core_index, local_regs, IMAGE_2_IMAGE_2_LAST);
    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_2_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

/* Strict priority order tasks: */

    /* GENERAL_TIMER: thread 0 */
    local_regs[IMAGE_2_IMAGE_2_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, general_timer_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_2_GENERAL_TIMER_STACK_ADDRESS+IMAGE_2_GENERAL_TIMER_STACK_BYTE_SIZE;

   /* GENERAL_TIMER: thread */
    local_regs[IMAGE_2_IMAGE_2_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, general_timer_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_2_GENERAL_TIMER_STACK_ADDRESS+IMAGE_2_GENERAL_TIMER_STACK_BYTE_SIZE;

    /* SERVICE_QUEUES: thread */
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, sq_tx_task_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_THREAD_NUMBER][reg_id[8]] = IMAGE_2_SQ_TM_PD_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_THREAD_NUMBER][reg_id[9]] = IMAGE_2_SQ_TM_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_2_IMAGE_2_SERVICE_QUEUES_THREAD_NUMBER][reg_id[30]] = IMAGE_2_SERVICE_QUEUES_STACK_ADDRESS + C_DEFS_SQ_TX_TASK_STACK_SIZE;
    /* CPU_RECYCLE: thread 1 */
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_cpu_recycle_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_CPU_RECYCLE_THREAD_NUMBER][reg_id[30]] = IMAGE_2_CPU_RECYCLE_STACK_ADDRESS+IMAGE_2_CPU_RECYCLE_STACK_BYTE_SIZE;

    /* CPU Tx threads */
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_cpu_tx_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[8]] = IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_0_THREAD_NUMBER][reg_id[30]] = IMAGE_2_CPU_TX_0_STACK_ADDRESS+IMAGE_2_CPU_TX_0_STACK_BYTE_SIZE;

    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_2, c_cpu_tx_wakeup_request);
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[8]] = IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER;
    local_regs[IMAGE_2_IMAGE_2_CPU_TX_1_THREAD_NUMBER][reg_id[30]] = IMAGE_2_CPU_TX_1_STACK_ADDRESS+IMAGE_2_CPU_TX_1_STACK_BYTE_SIZE;


    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER; task <= IMAGE_2_IMAGE_2_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_2, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_2_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_2_IMAGE_2_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_2_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_2_dynamic_code_section_proc1-image_2_dynamic_code_section_proc0));
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

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_3_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    rdd_global_registers_init(core_index, local_regs, IMAGE_3_IMAGE_3_LAST);
    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_3_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

#ifdef CONFIG_DHD_RUNNER
    local_regs[IMAGE_3_IMAGE_3_DHD_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_timer_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TIMER_STACK_ADDRESS+IMAGE_3_DHD_TIMER_STACK_BYTE_SIZE;

    /* DHD_TX_POST_UPDATE_FIFO */
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_tx_post_update_fifo_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_3_DHD_TX_POST_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TX_POST_UPDATE_FIFO_STACK_ADDRESS + IMAGE_3_DHD_TX_POST_UPDATE_FIFO_STACK_BYTE_SIZE;

    /* DHD_TX_POST_0 */
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_tx_post_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_0_THREAD_NUMBER][reg_id[8]] = 0;
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_0_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TX_POST_0_STACK_ADDRESS + IMAGE_3_DHD_TX_POST_0_STACK_BYTE_SIZE;

    /* DHD_TX_POST_1 */
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_1_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_dhd_tx_post_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_1_THREAD_NUMBER][reg_id[8]] = 1;
    local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_1_THREAD_NUMBER][reg_id[30]] = IMAGE_3_DHD_TX_POST_1_STACK_ADDRESS + IMAGE_3_DHD_TX_POST_1_STACK_BYTE_SIZE;

    if (g_dhd_hw_config.offload_config == 1)
    {
        /* radios wl1+wl2 offload configuration */
        local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_0_THREAD_NUMBER][reg_id[8]] = 1;
        local_regs[IMAGE_3_IMAGE_3_DHD_TX_POST_1_THREAD_NUMBER][reg_id[8]] = 2;
    }
#endif

#if defined(CONFIG_CPU_TX_FROM_XPM) || defined(CONFIG_CPU_RX_FROM_XPM)
    /* FPM rings and buffer management REFILL 5 */
    local_regs[IMAGE_3_IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_3, c_cpu_fpm_rings_and_bufmng_refill_wakeup_request);
    local_regs[IMAGE_3_IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL_THREAD_NUMBER][reg_id[8]] = IMAGE_3_IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL_THREAD_NUMBER;
    local_regs[IMAGE_3_IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL_THREAD_NUMBER][reg_id[30]] =
        IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL_STACK_ADDRESS + IMAGE_3_CPU_FPM_RINGS_AND_BUFMNG_REFILL_STACK_BYTE_SIZE;
#endif

    /* PROCESSING : thread 8-15 */
    for (task = IMAGE_3_IMAGE_3_PROCESSING0_THREAD_NUMBER; task <= IMAGE_3_IMAGE_3_PROCESSING7_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_3, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_3_PROCESSING_8_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_3_IMAGE_3_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_3_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_3_dynamic_code_section_proc1-image_3_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_3_IMAGE_3_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING4_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING4_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING5_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING5_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING6_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING6_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_3_IMAGE_3_PROCESSING7_THREAD_NUMBER][reg_id[30]] = IMAGE_3_PROCESSING7_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_4_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;
    uint32_t task;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
     MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));
    rdd_global_registers_init(core_index, local_regs, IMAGE_4_DS_TM_LAST);
    ag_drv_rnr_regs_cfg_ext_acc_cfg_set(core_index, PACKET_BUFFER_PD_PTR(IMAGE_4_PROCESSING_4_TASKS_PACKET_BUFFER_ADDRESS, 0), 8, 8, 8);

#ifdef BUFFER_CONGESTION_MGT
    /* BUFFER_CONG_MGT: thread 1 */
    local_regs[IMAGE_4_DS_TM_BUFFER_CONG_MGT_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, buffer_cong_mgt_1st_wakeup_request);
#endif

   /* GENERAL_TIMER: thread 0 */
    local_regs[IMAGE_4_DS_TM_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, general_timer_wakeup_request);
    local_regs[IMAGE_4_DS_TM_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_4_GENERAL_TIMER_STACK_ADDRESS+IMAGE_4_GENERAL_TIMER_STACK_BYTE_SIZE;

    /* UPDATE_FIFO_READ: thread 3 */

    local_regs[IMAGE_4_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, tm_update_fifo_first_wakeup_request);
    local_regs[IMAGE_4_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_4_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_4_DS_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_4_UPDATE_FIFO_STACK_ADDRESS + C_DEFS_UPDATE_FIFO_STACK_SIZE;

    /* SCHEDULING LAN: thread 3 */
    local_regs[IMAGE_4_DS_TM_TX_TASK_0_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, tm_tx_task_wakeup_request);
    local_regs[IMAGE_4_DS_TM_TX_TASK_0_THREAD_NUMBER][reg_id[30]] = IMAGE_4_TX_TASK_0_STACK_ADDRESS + C_DEFS_TX_TASK_STACK_SIZE;

    /* REPORTING : thread 5 */
    local_regs[IMAGE_4_DS_TM_REPORTING_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_4, ghost_reporting_wakeup_request);
    local_regs[IMAGE_4_DS_TM_REPORTING_THREAD_NUMBER][reg_id[8]] = drv_qm_get_us_end();
    local_regs[IMAGE_4_DS_TM_REPORTING_THREAD_NUMBER][reg_id[9]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_QUEUE_STATUS);
    local_regs[IMAGE_4_DS_TM_REPORTING_THREAD_NUMBER][reg_id[10]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, EPON_RPT_CNT_COUNTER);
    local_regs[IMAGE_4_DS_TM_REPORTING_THREAD_NUMBER][reg_id[11]] = xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, TOTAL_VALID_COUNTER_COUNTER);
    local_regs[IMAGE_4_DS_TM_REPORTING_THREAD_NUMBER][reg_id[30]] = IMAGE_4_REPORTING_STACK_ADDRESS + IMAGE_4_REPORTING_STACK_BYTE_SIZE;

    /* PROCESSING : thread 8-11 */
    for (task = IMAGE_4_DS_TM_PROCESSING0_THREAD_NUMBER; task <= IMAGE_4_DS_TM_PROCESSING3_THREAD_NUMBER; task++)
    {
        local_regs[task][reg_id[0]] = ADDRESS_OF(image_4, processing_wakeup_request);
        local_regs[task][reg_id[8]] = task;
        local_regs[task][reg_id[9]] = PACKET_BUFFER_PD_PTR(IMAGE_4_PROCESSING_4_TASKS_PACKET_BUFFER_ADDRESS, (task - IMAGE_4_DS_TM_PROCESSING0_THREAD_NUMBER));
        local_regs[task][reg_id[10]] =
            image_4_dynamic_code_section + ((task - PROJ_DEFS_FIRST_PROCESSING_THREAD_ID) * (image_4_dynamic_code_section_proc1-image_4_dynamic_code_section_proc0));
    }
    /* Stack init - should point to the end of stack */
    local_regs[IMAGE_4_DS_TM_PROCESSING0_THREAD_NUMBER][reg_id[30]] = IMAGE_4_PROCESSING0_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_4_DS_TM_PROCESSING1_THREAD_NUMBER][reg_id[30]] = IMAGE_4_PROCESSING1_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_4_DS_TM_PROCESSING2_THREAD_NUMBER][reg_id[30]] = IMAGE_4_PROCESSING2_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;
    local_regs[IMAGE_4_DS_TM_PROCESSING3_THREAD_NUMBER][reg_id[30]] = IMAGE_4_PROCESSING3_STACK_ADDRESS + C_DEFS_PROCESSING_STACK_SIZE;

    rdp_rnr_write_context(sram_context, local_regs, sizeof(local_regs));
}

static void image_5_context_set(uint32_t core_index, rdd_init_params_t *init_params)
{
    static uint32_t local_regs[NUM_OF_MAIN_RUNNER_THREADS][NUM_OF_LOCAL_REGS];
    uint32_t *sram_context;

    sram_context = (uint32_t *)RUNNER_CORE_CONTEXT_ADDRESS(core_index);

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32(local_regs, sram_context, sizeof(local_regs));

    rdd_global_registers_init(core_index, local_regs, IMAGE_5_US_TM_LAST);

    /* DIRECT PROCESSING : thread 0 */
    local_regs[IMAGE_5_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, direct_flow_wakeup_request);
    local_regs[IMAGE_5_US_TM_DIRECT_FLOW_THREAD_NUMBER][reg_id[30]] = IMAGE_5_DIRECT_FLOW_STACK_ADDRESS + C_DEFS_DIRECT_FLOW_STACK_SIZE;

    /* GENERAL_TIMER: thread 0 */
    local_regs[IMAGE_5_US_TM_GENERAL_TIMER_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, general_timer_wakeup_request);
    local_regs[IMAGE_5_US_TM_GENERAL_TIMER_THREAD_NUMBER][reg_id[30]] = IMAGE_5_GENERAL_TIMER_STACK_ADDRESS+IMAGE_5_GENERAL_TIMER_STACK_BYTE_SIZE;

#ifdef BUFFER_CONGESTION_MGT
    /* BUFFER_CONG_MGT: thread 3 */
    local_regs[IMAGE_5_US_TM_BUFFER_CONG_MGT_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, buffer_cong_mgt_1st_wakeup_request);
#endif

    /* US UPDATE_FIFO_READ: thread 4  */
    local_regs[IMAGE_5_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, tm_update_fifo_first_wakeup_request);
    local_regs[IMAGE_5_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_5_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_5_US_TM_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_5_UPDATE_FIFO_STACK_ADDRESS + C_DEFS_UPDATE_FIFO_STACK_SIZE;
    
    /* US tx task: */
    local_regs[IMAGE_5_US_TM_WAN_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, tm_tx_task_wakeup_request);
    local_regs[IMAGE_5_US_TM_WAN_THREAD_NUMBER][reg_id[30]] = IMAGE_5_WAN_STACK_ADDRESS + C_DEFS_TX_TASK_STACK_SIZE;

    /* EPON UPDATE FIFO:  */
    local_regs[IMAGE_5_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, epon_tm_update_fifo_first_wakeup_request);
    local_regs[IMAGE_5_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[8]] = IMAGE_5_EPON_UPDATE_FIFO_TABLE_ADDRESS;
    local_regs[IMAGE_5_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[9]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_5_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[10]] = drv_qm_get_us_epon_start();
    local_regs[IMAGE_5_US_TM_EPON_UPDATE_FIFO_THREAD_NUMBER][reg_id[30]] = IMAGE_5_EPON_UPDATE_FIFO_STACK_ADDRESS + C_DEFS_EPON_UPDATE_FIFO_STACK_SIZE;

    /* EPON WAN: */
    local_regs[IMAGE_5_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[0]] = ADDRESS_OF(image_5, epon_tm_tx_task_wakeup_request);
    local_regs[IMAGE_5_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[8]] = BB_ID_TX_PON_ETH_PD;
    local_regs[IMAGE_5_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[9]] = drv_qm_get_us_epon_start();
    local_regs[IMAGE_5_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[10]] = BB_ID_QM_RNR_GRID;
    local_regs[IMAGE_5_US_TM_WAN_EPON_THREAD_NUMBER][reg_id[30]] = IMAGE_5_WAN_EPON_STACK_ADDRESS + C_DEFS_EPON_TX_TASK_STACK_SIZE;
  
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

        default:
            bdmf_trace("ERROR driver %s:%u| unsupported Runner image = %d\n", __FILE__, __LINE__, rdp_core_to_image_map[core_index]);
            break;
        }
    }
}

int rdd_data_structures_init(rdd_init_params_t *init_params, HW_IPTV_CONFIGURATION_STRUCT *iptv_hw_config)
{
    bdmf_error_t rc;

#ifdef CONFIG_DHD_RUNNER
    rdd_dhd_get_offload_cfg();
#endif
    rdd_local_registers_init(init_params);

    rdd_cpu_if_init();

    rdd_fpm_pool_number_mapping_cfg(iptv_hw_config->fpm_base_token_size);
#ifdef CONFIG_DHD_RUNNER
    rdp_drv_dhd_skb_fifo_tbl_init();
#endif

    rdd_update_global_fpm_cfg();

#ifdef CONFIG_DHD_RUNNER
    rdd_dhd_hw_cfg(&init_params->dhd_hw_config);
#endif

#if defined(DEBUG_PRINTS)
    rdd_debug_prints_init();
#endif

    rdd_init_c_stack();

    /* init first queue mapping */
    rdd_ag_us_tm_first_queue_mapping_set(drv_qm_get_us_start());
    rdd_ag_ds_tm_first_queue_mapping_set(drv_qm_get_ds_start(0));
    rdd_ag_service_queues_first_queue_mapping_set(drv_qm_get_sq_start());
    /* init scheduler data structures */
    rdd_scheduling_conf_init();
    rdd_scheduler_wake_up_bbh_init_data_structure();

    /* init bbh-queue */

    /* start budget allocation task */
    rdd_general_timer_set(get_runner_idx(ds_tm_runner_image), 0, RATE_LIMIT_TIMER_PERIOD, TIMER_ACTION_TM_BUDGET_ALLOCATOR);
    rdd_general_timer_set(get_runner_idx(us_tm_runner_image), 0, RATE_LIMIT_TIMER_PERIOD, TIMER_ACTION_TM_BUDGET_ALLOCATOR);
    rdd_general_timer_set(get_runner_idx(service_queues_runner_image), 0, RATE_LIMIT_TIMER_PERIOD, TIMER_ACTION_TM_BUDGET_ALLOCATOR);

#ifdef XRDP_PI2
    rdd_general_timer_set(get_runner_idx(ds_tm_runner_image), 0, PI2_TIMER_PERIOD_US, TIMER_ACTION_PI2);
    rdd_general_timer_set(get_runner_idx(us_tm_runner_image), 0, PI2_TIMER_PERIOD_US, TIMER_ACTION_PI2);

#ifdef PI2_ON_SERVICE_QUEUES
    /* start seqrvice queue AQM timer task */
    rdd_general_timer_set(get_runner_idx(service_queues_runner_image), 0, PI2_SQ_TIMESTAMP_INJECT_PERIOD, TIMER_ACTION_PI2_SQ_TIMESTAMP_INJECT);
    rdd_general_timer_set(get_runner_idx(service_queues_runner_image), 0, PI2_TIMER_PERIOD_US, TIMER_ACTION_PI2);
#endif
#endif

    ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(ds_tm_runner_image), IMAGE_4_DS_TM_GENERAL_TIMER_THREAD_NUMBER);
    ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), IMAGE_5_US_TM_GENERAL_TIMER_THREAD_NUMBER);
    ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(service_queues_runner_image), IMAGE_2_IMAGE_2_GENERAL_TIMER_THREAD_NUMBER);

    rdd_rx_flow_init();
    rc = rdd_cpu_proj_init();

    rc = rc ? rc : rdd_ag_natc_nat_cache_key0_mask_set(NATC_KEY0_DEF_MASK);

    rc = rc ? rc : rdd_ingress_filters_init();

#ifdef USE_BDMF_SHELL
    /* register shell commands */
    rc = rc ? : rdd_make_shell_commands();
#endif
    return rc;
}


