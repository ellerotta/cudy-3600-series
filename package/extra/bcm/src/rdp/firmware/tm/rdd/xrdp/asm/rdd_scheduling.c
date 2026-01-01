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


#include "rdd_scheduling.h"
#include "rdp_drv_rnr.h"
#include "rdd_ghost_reporting.h"
#include "rdp_drv_dqm.h"
#include "rdp_drv_qm.h"


#define SCHED_WAKE_UP_DONE 0xff
uint32_t exponent_list[] = {0, 3, 6, 9};

#ifdef G9991_COMMON
    rdpa_emac vport_to_emac[RDD_LAN_VPORT_LAST] = {};
#endif

/* It is impossible to reset counters safely. Use base value instead. Used for codel/flush drop */
static PACKETS_AND_BYTES_STRUCT tx_queue_drop_counters[MAX_TX_QUEUES__NUM_OF];

extern bdmf_error_t rdd_complex_scheduler_rate_set(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, uint32_t block_bit_mask, uint8_t tm_id, uint8_t is_complex_rl);
extern bdmf_error_t rdd_basic_scheduler_rate_set(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, uint8_t queue_bit_mask, uint8_t tm_id);


void rdd_bbh_queue_init(void)
{
    uint32_t *rdd_ds_tm_bbh_queue_table_ptr = RDD_DS_TM_BBH_QUEUE_TABLE_ADDRESS_ARR;
#ifndef XRDP_BBH_PER_LAN_PORT
    uint32_t i;
#endif

    RDD_BTRACE("\n");

#ifndef XRDP_BBH_PER_LAN_PORT
    for (i = 0; i < DS_TM_BBH_QUEUE_TABLE_SIZE; ++i)
    {
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_LAN, rdd_ds_tm_bbh_queue_table_ptr, i);
    }
    rdd_ag_ds_tm_bb_destination_table_set(BB_ID_TX_LAN);
#else
    /* init lan0 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_0, rdd_ds_tm_bbh_queue_table_ptr, 0);

    /* init lan1 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_1, rdd_ds_tm_bbh_queue_table_ptr, 1);

    /* init lan2 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_2, rdd_ds_tm_bbh_queue_table_ptr, 2);

    /* init lan3 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_3, rdd_ds_tm_bbh_queue_table_ptr, 3);

    /* init lan4 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_4, rdd_ds_tm_bbh_queue_table_ptr, 4);

    /* init lan5 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_5, rdd_ds_tm_bbh_queue_table_ptr, 5);

    /* init lan6 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_6, rdd_ds_tm_bbh_queue_table_ptr, 6);

#ifdef BCM6858
    /* init lan7 */
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE_G(BB_ID_TX_BBH_7, rdd_ds_tm_bbh_queue_table_ptr, 7);
#endif
#endif
}

bdmf_error_t rdd_tm_epon_cfg(void)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t llid;
    rdd_scheduling_queue_descriptor_t cfg = {};
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;

    RDD_BTRACE("\n");
    for (llid = 0; (llid < MAX_NUM_OF_LLID); ++llid)
    {
        cfg.bbh_queue_index = llid;
        rc = rdd_scheduling_scheduler_block_cfg(rdpa_dir_us, 0, llid + drv_qm_get_us_epon_start(), &cfg, 0, 0, 1, rdpa_port_epon);
        if (rc)
            break;
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + llid;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_PON_ETH_STAT, entry);
        RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_WRITE_G(llid, RDD_US_TM_BBH_QUEUE_TABLE_ADDRESS_ARR, llid);
    }
    return rc;
}

#if !defined(HW_AGGREGATION_FLUSH)
bdmf_error_t rdd_scheduling_flush_timer_set(void)
{
    SCHEDULING_FLUSH_GLOBAL_CFG_STRUCT *entry;
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t core_index;

    RDD_BTRACE("\n");
    /* flush task - ds core */

    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
    {
        if (IS_DS_TM_RUNNER_IMAGE(core_index))
        {
            entry = RDD_SCHEDULING_FLUSH_GLOBAL_CFG_PTR(core_index);
            RDD_BYTE_1_BITS_WRITE(FLUSH_TASK_TIMER_INTERVAL, entry);
        }
    }

    /* flush task - us core */
    entry = RDD_SCHEDULING_FLUSH_GLOBAL_CFG_PTR(get_runner_idx(us_tm_runner_image));
    RDD_BYTE_1_BITS_WRITE(FLUSH_TASK_TIMER_INTERVAL, entry);

#ifdef SERVICE_QUEUES_FLUSH_THREAD_NUMBER
#ifndef SQ_FLUSH_TASK_TIMER_INTERVAL_IN_USEC
#define SQ_FLUSH_TASK_TIMER_INTERVAL_IN_USEC FLUSH_TASK_TIMER_INTERVAL_IN_USEC
#endif
    /* flush task - sq core */
    entry = RDD_SCHEDULING_FLUSH_GLOBAL_CFG_PTR(get_runner_idx(service_queues_runner_image));
    RDD_BYTE_1_BITS_WRITE(SQ_FLUSH_TASK_TIMER_INTERVAL_IN_USEC, entry);
#endif

    /* Make sure the timer configured before the cpu weakeup */
    WMB();

#if !defined(BCM6846) && !defined(BCM6878)

    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), US_TM_FLUSH_THREAD_NUMBER);

    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
        if (IS_DS_TM_RUNNER_IMAGE(core_index))
           rc = rc ? rc : ag_drv_rnr_regs_cfg_cpu_wakeup_set(core_index, DS_TM_FLUSH_THREAD_NUMBER);
#else
    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), TM_FLUSH_THREAD_NUMBER);
#endif
#ifdef SERVICE_QUEUES_FLUSH_THREAD_NUMBER
    if (drv_qm_get_sq_start() != QM_ILLEGAL_QUEUE) {
        rc = rc ? rc : ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(service_queues_runner_image),
            SERVICE_QUEUES_FLUSH_THREAD_NUMBER);
    }
#endif
    return rc;
}
#endif

bdmf_error_t rdd_us_budget_allocation_timer_set(void)
{
    BUDGET_ALLOCATION_TIMER_VALUE_STRUCT *entry;
    bdmf_error_t rc = BDMF_ERR_OK;
    static uint8_t first_time = 1;

    RDD_BTRACE("\n");

    if (!first_time)
        return rc;

    first_time = 0;

    /* budget allocation task - us core */
    entry = RDD_BUDGET_ALLOCATION_TIMER_VALUE_PTR(get_runner_idx(us_tm_runner_image));
    RDD_BYTES_2_BITS_WRITE(US_RATE_LIMITER_TIMER_PERIOD, entry);

    /* Make sure the timer configured before the cpu weakeup */
    WMB();

    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), TM_BUDGET_ALLOCATION_US_THREAD_NUMBER);
    rc = rc ? rc : ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), TM_OVL_BUDGET_ALLOCATION_US_THREAD_NUMBER);
    return rc;
}

bdmf_error_t rdd_ds_budget_allocation_timer_set(void)
{
    BUDGET_ALLOCATION_TIMER_VALUE_STRUCT *entry;
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t core_index;

    RDD_BTRACE("\n");
    /* budget allocation task - ds core */
    for (core_index = 0; core_index < NUM_OF_RUNNER_CORES; core_index++)
    {
        if (IS_DS_TM_RUNNER_IMAGE(core_index))
        {
            entry = RDD_BUDGET_ALLOCATION_TIMER_VALUE_PTR(core_index);
            RDD_BYTES_2_BITS_WRITE(DS_RATE_LIMITER_TIMER_PERIOD, entry);

            /* Make sure the timer configured before the cpu weakeup */
            WMB();
            rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(core_index, drv_qm_get_budget_allocator_task(core_index));
        }
    }
    return rc;
}

/* API to RDPA level */
void rdd_set_queue_enable(uint32_t qm_queue_index, bdmf_boolean enable)
{
    uint32_t q_vec;

    RDD_BTRACE("set queue %d threshold\n", qm_queue_index);

    /* set/clear the queue in the threshold vector */
    RDD_BYTES_4_BITS_READ(q_vec, (((BYTES_4_STRUCT *)RDD_QUEUE_THRESHOLD_VECTOR_PTR(get_runner_idx(cpu_tx_runner_image))) +
        (qm_queue_index >> 5)));
    if (enable)
        q_vec |= (1 << (qm_queue_index & 0x1f));
    else
        q_vec &= (~(1 << (qm_queue_index & 0x1f)));
    RDD_BYTES_4_BITS_WRITE_G(q_vec, RDD_QUEUE_THRESHOLD_VECTOR_ADDRESS_ARR, (qm_queue_index >> 5));
}

bdmf_error_t rdd_scheduling_scheduler_block_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint16_t qm_queue_index,
    rdd_scheduling_queue_descriptor_t *scheduler_cfg, bdmf_boolean type, uint8_t dwrr_offset, bdmf_boolean enable,
    rdpa_port_type port_type)
{
    RDD_BTRACE("dir = %d, qm_queue_index = %d, type = %d, scheduler_cfg %px = { scheduler_index = %d, bit_mask = %x, "
        "bbh_queue = %d, scheduler_type = %d }\n",
        dir, qm_queue_index, type, scheduler_cfg,
        scheduler_cfg->scheduler_index, scheduler_cfg->queue_bit_mask, scheduler_cfg->bbh_queue_index,
        scheduler_cfg->block_type);

    if (type)
    {
        return rdd_basic_scheduler_block_cfg(dir, channel_id, qm_queue_index, scheduler_cfg, dwrr_offset, port_type);
    }
    else
    {
        if (dir == rdpa_dir_ds)
        {
            bdmf_error_t err;
            if (enable)
            {
                err = rdd_ag_ds_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_ds_start(0), scheduler_cfg);
                if (!err)
                    err = rdd_ag_ds_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_ds_start(0), enable);
                return err;
            }
            else
            {
                err = rdd_ag_ds_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_ds_start(qm_queue_index), enable);
                if (!err)
                    err = rdd_ag_ds_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_ds_start(qm_queue_index), scheduler_cfg);
                return err;
            }
        }

#ifdef EPON
        rdd_ghost_reporting_mapping_queue_to_wan_channel((qm_queue_index - drv_qm_get_us_start()), scheduler_cfg->bbh_queue_index, enable);
#endif
        if (enable)
        {
            rdd_ag_us_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_us_start(), scheduler_cfg);
            return rdd_ag_us_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_us_start(), enable);
        }
        else
        {
            rdd_ag_us_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_us_start(), enable);
            return rdd_ag_us_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_us_start(), scheduler_cfg);
        }
    }
}

bdmf_error_t rdd_scheduling_queue_rate_limiter_remove(rdpa_traffic_dir dir, uint16_t qm_queue_index, uint8_t is_complex_rl)
{
    bdmf_boolean block_type;
    uint8_t block_index, bit_mask;
    bdmf_error_t rc;
    uint32_t queue_index;
    uint8_t *scheduling_queue_table_p;
    int tm_index, tm_core;
       
    RDD_BTRACE("dir = %d, qm_queue_index = %d\n", dir, qm_queue_index);

    tm_index = drv_qm_get_ds_tm_index(qm_queue_index);
    tm_core = drv_qm_get_tm_core(tm_index, dir);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Invalid tm_core %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (dir == rdpa_dir_ds)
    {
        scheduling_queue_table_p = (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_core);
        queue_index = qm_queue_index - drv_qm_get_ds_start(qm_queue_index);
    }
    else
    {
        scheduling_queue_table_p = (uint8_t *)RDD_US_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_core);
        queue_index = qm_queue_index - drv_qm_get_us_start();
    }

    RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(0, scheduling_queue_table_p + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    
    /* make sure the queue has budget in scheduler */
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_BLOCK_TYPE_READ(block_type, scheduling_queue_table_p + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_READ(block_index,  scheduling_queue_table_p + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUEUE_BIT_MASK_READ(bit_mask, scheduling_queue_table_p + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));

    if (block_type)
        rc = rdd_complex_scheduler_rate_set(dir, block_index, (1 << bit_mask), drv_qm_get_ds_tm_index(qm_queue_index), is_complex_rl); 
    else
        rc = rdd_basic_scheduler_rate_set(dir, block_index, bit_mask,  drv_qm_get_ds_tm_index(qm_queue_index));

    return rc;
}

static uint8_t rdd_tm_debug_sched_index_get(rdpa_traffic_dir dir, uint8_t bbh_queue)
{
    uint8_t sched_index;
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    if (dir == rdpa_dir_ds)
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + bbh_queue;

    else
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + bbh_queue;

    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_READ(sched_index, entry);

    return sched_index;
}

static uint32_t rdd_tm_debug_basic_rl_rate_get(rdpa_traffic_dir dir, uint8_t rl_index)
{
    uint32_t man, exp;
    BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT *entry;
    uint32_t rate_limiter_timer_period;

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    if (dir == rdpa_dir_ds)
    {
        entry = ((BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT *)RDD_BASIC_RATE_LIMITER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + rl_index;
        rate_limiter_timer_period = DS_RATE_LIMITER_TIMER_PERIOD_IN_USEC;
    }

    else
    {
        entry = ((BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT *)RDD_BASIC_RATE_LIMITER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + rl_index;
        rate_limiter_timer_period = US_RATE_LIMITER_TIMER_PERIOD_IN_USEC;
    }

    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_LIMIT_EXPONENT_READ(exp, entry);
    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_LIMIT_MANTISSA_READ(man, entry);

    return 8 * ((man << (exp * exponent_list[1])) * rate_limiter_timer_period);
}

bdmf_boolean rdd_tm_is_cs_exist(rdpa_traffic_dir dir, uint8_t bbh_queue)
{
    bdmf_boolean sched_type;
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    if (dir == rdpa_dir_ds)
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + bbh_queue;

    else
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + bbh_queue;

    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_TYPE_READ(sched_type, entry);

    return sched_type;
}

void rdd_scheduler_wake_up_bbh_init_data_structure(void)
{
    /* init complex */
    RDD_BYTES_4_BITS_WRITE_G(SCHED_WAKE_UP_DONE, RDD_COMPLEX_SCHEDULER_WAKEUP_CMD_US_ADDRESS_ARR, 0);
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    RDD_BYTES_4_BITS_WRITE_G(SCHED_WAKE_UP_DONE, RDD_COMPLEX_SCHEDULER_WAKEUP_CMD_DS_ADDRESS_ARR, 0);
#endif

    /*init basic */
    RDD_BYTES_4_BITS_WRITE_G(SCHED_WAKE_UP_DONE, RDD_BASIC_SCHEDULER_WAKEUP_CMD_US_ADDRESS_ARR, 0);
#if (!defined(COMPLEX_SCHEDULER_IN_DS))
    RDD_BYTES_4_BITS_WRITE_G(SCHED_WAKE_UP_DONE, RDD_BASIC_SCHEDULER_WAKEUP_CMD_DS_ADDRESS_ARR, 0);
#endif
}

void rdd_scheduler_wake_up_bbh(rdpa_traffic_dir dir, uint8_t scheduler_index, uint8_t block_type, uint8_t tm_core, uint8_t complex_rate_limit)
{
    uint8_t *complex_scheduler_wake_up_cmd_p;
    uint8_t *basic_scheduler_wake_up_cmd_p;
    uint32_t wake_up_index;
    uint32_t wake_up_command;
    int max_tries = 5;
    int has_complex = 0;
    int has_basic = 0;

    if (dir == rdpa_dir_ds)
    {
#if (!defined(COMPLEX_SCHEDULER_IN_DS))
        has_basic = 1;
        basic_scheduler_wake_up_cmd_p = (uint8_t *)RDD_BASIC_SCHEDULER_WAKEUP_CMD_DS_PTR(tm_core);
#endif
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
        has_complex = 1;
        complex_scheduler_wake_up_cmd_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_WAKEUP_CMD_DS_PTR(tm_core);
#endif
    }
    else
    {
        has_complex = 1;
        has_basic = 1;
        complex_scheduler_wake_up_cmd_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_WAKEUP_CMD_US_PTR(tm_core);
        basic_scheduler_wake_up_cmd_p = (uint8_t *)RDD_BASIC_SCHEDULER_WAKEUP_CMD_US_PTR(tm_core);
    }

    wake_up_command = (scheduler_index | complex_rate_limit << 31);
    if (block_type)
    {
        if (!has_complex)
            BDMF_TRACE_ERR("rdd_scheduler_wake_up_bbh dir=%d complex scheduler without complex\n", dir);
        else
            RDD_BYTES_4_BITS_WRITE(wake_up_command, complex_scheduler_wake_up_cmd_p);
    }
    else
    {
        if (!has_basic)
            BDMF_TRACE_ERR("rdd_scheduler_wake_up_bbh dir=%d basic scheduler without basic\n", dir);
        else
            RDD_BYTES_4_BITS_WRITE(wake_up_command, basic_scheduler_wake_up_cmd_p);
    }

    WMB();
    if (dir == rdpa_dir_ds)
        ag_drv_rnr_regs_cfg_cpu_wakeup_set(tm_core, drv_qm_get_budget_allocator_task(tm_core));
    else
        ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), TM_BUDGET_ALLOCATION_US_THREAD_NUMBER);

    do
    {
        wake_up_index = SCHED_WAKE_UP_DONE;
        if (block_type)
        {
            if (!has_complex)
                BDMF_TRACE_ERR("rdd_scheduler_wake_up_bbh dir=%d complex scheduler without complex\n", dir);
            else
                RDD_BYTES_4_BITS_READ(wake_up_index, complex_scheduler_wake_up_cmd_p);
        }
        else
        {
            if (!has_basic)
                BDMF_TRACE_ERR("rdd_scheduler_wake_up_bbh dir=%d basic scheduler without basic\n", dir);
            else
                RDD_BYTES_4_BITS_READ(wake_up_index, basic_scheduler_wake_up_cmd_p);
        }
        max_tries--;
        if (wake_up_index != SCHED_WAKE_UP_DONE) 
            bdmf_msleep(1);
    } while ((wake_up_index != SCHED_WAKE_UP_DONE) && (max_tries));
    if (max_tries == 0)
        BDMF_TRACE_ERR("internal error in remove RL\n");
}

static void rdd_tm_debug_cs_get(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, rdd_tm_info *info)
{
    uint8_t budget, i, first_queue_index = 0;
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *base_queue_entry;
    BASIC_SCHEDULER_DESCRIPTOR_STRUCT *bs_entry;
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *cs_entry;

    memset(info, 0, sizeof(rdd_tm_info));

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    if (dir == rdpa_dir_ds)
    {
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + complex_scheduler_index;
        base_queue_entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image)));
        /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
        first_queue_index = drv_qm_get_ds_start(0);
#else
        BDMF_TRACE_ERR("Complex scheduler not supported in DS\n");
        return;
#endif
    }

    else
    {
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + complex_scheduler_index;
        base_queue_entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image)));
    }

    info->sched_index = complex_scheduler_index;
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_SIR_READ(info->dwrr_offset, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_READ(info->sched_rl.rl_en, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_READ(budget, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_READ(info->sched_rl.rl_index, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_SLOT_READ(info->cs_scheduler_slot, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_BASIC_READ(info->cs_scheduler_basic, entry);
    if (budget)
        info->enable = 1;
    else
        info->enable = 0;

    info->sched_rl.rl_rate = rdd_tm_debug_basic_rl_rate_get(dir, info->sched_rl.rl_index);

    for (i = 0; i < MAX_NUM_OF_QUEUES_IN_SCHED; i++)
    {
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_READ(info->queue_info[i].queue_index, entry, i);

        if (!(info->cs_scheduler_slot & (1 << i)))
        {
            /* queue */
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_READ(info->queue_info[i].queue_rl.rl_en, (base_queue_entry + info->queue_info[i].queue_index));
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMITER_INDEX_READ(info->queue_info[i].queue_rl.rl_index, (base_queue_entry + info->queue_info[i].queue_index));
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUEUE_BIT_MASK_READ(info->queue_info[i].queue_bit_mask , (base_queue_entry + info->queue_info[i].queue_index));
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_READ(info->queue_info[i].queue_weight , (base_queue_entry + info->queue_info[i].queue_index));
            info->queue_info[i].queue_rl.rl_rate = rdd_tm_debug_basic_rl_rate_get(dir, info->queue_info[i].queue_rl.rl_index);
            info->queue_info[i].queue_index += first_queue_index;
        }
        else if (info->cs_scheduler_basic & (1 << i))
        {
            /* basic scheduler */
            /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
            if (dir == rdpa_dir_ds)
#ifdef COMPLEX_SCHEDULER_IN_DS
            {
                BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
                return;
            }
#else
                bs_entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + info->queue_info[i].queue_index;
#endif
            else
                bs_entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + info->queue_info[i].queue_index;

            RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_SLOT_INDEX_READ(info->queue_info[i].queue_bit_mask , bs_entry);
        }
        else
        {
            /* complex scheduler */
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
            /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
            if (dir == rdpa_dir_ds)
                cs_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + info->queue_info[i].queue_index;
            else
#endif
                cs_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + info->queue_info[i].queue_index;

            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_SLOT_INDEX_READ(info->queue_info[i].queue_bit_mask , cs_entry);
        }
    }
}

void rdd_tm_debug_bs_get(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, rdd_tm_info *info)
{
    uint8_t budget, i, first_queue_index = 0;
    BASIC_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *base_queue_entry;

    memset(info, 0, sizeof(rdd_tm_info));
    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    if (dir == rdpa_dir_ds)
    {
#ifdef COMPLEX_SCHEDULER_IN_DS
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return;
#else
        entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + basic_scheduler_index;
        base_queue_entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image)));
        /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
        first_queue_index = drv_qm_get_ds_start(0);
#endif
    }

    else
    {
        entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + basic_scheduler_index;
        base_queue_entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image)));
    }

    info->sched_index = basic_scheduler_index;
    RDD_BASIC_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_READ(info->dwrr_offset, entry);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_READ(info->sched_rl.rl_en, entry);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_READ(budget, entry);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_READ(info->sched_rl.rl_index, entry);
    if (budget)
        info->enable = 1;
    else
        info->enable = 0;

    info->sched_rl.rl_rate = rdd_tm_debug_basic_rl_rate_get(dir, info->sched_rl.rl_index);

    for (i = 0; i < BASIC_SCHEDULER_NUM_OF_QUEUES; i++)
    {
        RDD_BASIC_SCHEDULER_DESCRIPTOR_QUEUE_INDEX_READ(info->queue_info[i].queue_index, entry, i);
        RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_READ(info->queue_info[i].queue_rl.rl_en, (base_queue_entry + info->queue_info[i].queue_index));
        RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMITER_INDEX_READ(info->queue_info[i].queue_rl.rl_index, (base_queue_entry + info->queue_info[i].queue_index));
        RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUEUE_BIT_MASK_READ(info->queue_info[i].queue_bit_mask , (base_queue_entry + info->queue_info[i].queue_index));
        RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_READ(info->queue_info[i].queue_weight , (base_queue_entry + info->queue_info[i].queue_index));
        info->queue_info[i].queue_rl.rl_rate = rdd_tm_debug_basic_rl_rate_get(dir, info->queue_info[i].queue_rl.rl_index);
        info->queue_info[i].queue_index += first_queue_index;
    }
}

void rdd_tm_debug_get(rdpa_traffic_dir dir, uint8_t bbh_queue, rdd_tm_info *info)
{
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;
    bdmf_boolean type;
    uint8_t scheduler_index = rdd_tm_debug_sched_index_get(dir, bbh_queue);

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    if (dir == rdpa_dir_ds)
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + bbh_queue;

    else
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + bbh_queue;

    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_TYPE_READ(type, entry);
    if (type)
        rdd_tm_debug_cs_get(dir, scheduler_index, info);
    else
        rdd_tm_debug_bs_get(dir, scheduler_index, info);
}

#if defined(OPERATION_MODE_PRV) || defined(G9991_FC)

int rdd_g9991_control_sid_set(rdd_rdd_vport vport, rdpa_emac emac)
{
#if defined(G9991_COMMON) /* will fix later when XML merged */
    G9991_SCHEDULING_INFO_ENTRY_STRUCT *entry;
    uint32_t control_sid_bit_mask, i;

    RDD_BTRACE("vport = %d, emac = %d\n", vport, emac);

    if (vport >= RDD_VPORT_ID_32)
        return BDMF_ERR_NOT_SUPPORTED;

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    for (i = 0; i < RDD_G9991_SCHEDULING_INFO_TABLE_SIZE; i++) {
        entry = ((G9991_SCHEDULING_INFO_ENTRY_STRUCT *)RDD_G9991_SCHEDULING_INFO_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + i;

        RDD_G9991_SCHEDULING_INFO_ENTRY_CONTROL_SID_READ(control_sid_bit_mask, entry);

        if (i == emac)
            control_sid_bit_mask |= (1 << vport);
        else
            control_sid_bit_mask &= ~(1 << vport);

        RDD_G9991_SCHEDULING_INFO_ENTRY_CONTROL_SID_WRITE_G(control_sid_bit_mask, RDD_G9991_SCHEDULING_INFO_TABLE_ADDRESS_ARR, i);
    }

    return 0;
#else
    return 0;
#endif
}

int rdd_g9991_vport_to_emac_mapping_cfg(rdd_rdd_vport vport, rdpa_emac emac)
{
#if defined(G9991_COMMON)
    G9991_SCHEDULING_INFO_ENTRY_STRUCT *entry;
    uint32_t port_mask, i;

    RDD_BTRACE("vport = %d, emac = %d\n", vport, emac);

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    for (i = 0; i < RDD_G9991_SCHEDULING_INFO_TABLE_SIZE; i++) {
#ifdef G9991_PRV
        entry = ((G9991_SCHEDULING_INFO_ENTRY_STRUCT *)RDD_G9991_SCHEDULING_INFO_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + i;
#else
        entry = ((G9991_SCHEDULING_INFO_ENTRY_STRUCT *)RDD_G9991_SCHEDULING_INFO_TABLE_PTR(emac_port_mapping[i].tx_runner_core)) +
            emac_port_mapping[i].bbh_queue;
#endif
        RDD_G9991_SCHEDULING_INFO_ENTRY_PHYSICAL_PORT_MAPPING_READ(port_mask, entry);

        if (i == emac)
            port_mask |= (1 << vport);
        else
            port_mask &= ~(1 << vport);

        RDD_G9991_SCHEDULING_INFO_ENTRY_PHYSICAL_PORT_MAPPING_WRITE(port_mask, entry);
    }

    vport_to_emac[vport] = emac;

#ifdef G9991_PRV
    /* Make sure to setup also for System port */
    RDD_G9991_VPORT_TO_BB_ID_ENTRY_BB_ID_WRITE_G(port_mapping_bb_tx_id_get(rdpa_port_emac, emac), RDD_G9991_VPORT_TO_BB_ID_ADDRESS_ARR, vport);
    RDD_INGRESS_CONGESTION_FLOW_CTRL_ENTRY_BBH_ID_WRITE_G(port_mapping_bb_rx_id_get(rdpa_port_emac, emac), RDD_INGRESS_CONGESTION_FLOW_CTRL_TABLE_ADDRESS_ARR, vport);
#endif
#ifdef G9991_FC
    RDD_G9991_VPORT_TO_BB_ID_ENTRY_BB_ID_WRITE_G(emac_port_mapping[emac].bb_tx_id + (emac_port_mapping[emac].bbh_queue << BBMSG_RNR_TO_BBH_TX_QUEUE_F_OFFSET),
        RDD_G9991_VPORT_TO_BB_ID_ADDRESS_ARR, vport);

    RDD_G9991_EMAC_TO_BB_QUEUE_ENTRY_BB_QUEUE_WRITE_G(emac_port_mapping[emac].bbh_queue, RDD_G9991_EMAC_TO_BB_QUEUE_TABLE_ADDRESS_ARR, emac);
#endif
#endif
    return BDMF_ERR_OK;
}

int rdd_g9991_ingress_congestion_flow_control_enable(bbh_id_e bbh_id, bdmf_boolean enable)
{
#if defined(BCM6858) && defined(OPERATION_MODE_PRV)
    INGRESS_CONGESTION_FLOW_CTRL_ENTRY_STRUCT *entry;
    bbh_id_e entry_bbh_id;
    uint32_t i;

    for (i = 0; i < RDD_IMAGE_4_INGRESS_CONGESTION_FLOW_CTRL_TABLE_SIZE; i++) {
        entry = ((INGRESS_CONGESTION_FLOW_CTRL_ENTRY_STRUCT *)RDD_INGRESS_CONGESTION_FLOW_CTRL_TABLE_PTR(get_runner_idx(processing_runner_image))) + i;

        RDD_INGRESS_CONGESTION_FLOW_CTRL_ENTRY_BBH_ID_READ(entry_bbh_id, entry);

        if (entry_bbh_id == bbh_id)
            RDD_INGRESS_CONGESTION_FLOW_CTRL_ENTRY_ENABLE_WRITE_G(enable, RDD_INGRESS_CONGESTION_FLOW_CTRL_TABLE_ADDRESS_ARR, i);
    }
#endif
    return BDMF_ERR_OK;
}

void rdd_g9991_single_fragment_enable_cfg(bdmf_boolean enable)
{
#if defined(BCM6858) && defined(OPERATION_MODE_PRV)
     RDD_G9991_SINGLE_FRAGMENT_CFG_ENTRY_ENABLE_WRITE_G(enable, RDD_G9991_SINGLE_FRAGMENT_CFG_TABLE_ADDRESS_ARR, 0);
#endif
}
#endif

#ifdef VLAN_COUNTER

/* When reusing VLAN_COUNTERs counters array is indexed by absolute queue index rather than relative.
   In this case need to sum over all cores
*/
static uint32_t _tx_queue_drop_counter_cumulative_by_core(uint16_t queue_index, uint32_t i, uint32_t *addr_arr)
{
    uint32_t offset = queue_index * sizeof(PACKETS_AND_BYTES_STRUCT);
    uint32_t *entry;
    int mem_id;
    uint32_t retval = 0;

    for (mem_id = 0; mem_id < NUM_OF_RUNNER_CORES; mem_id++)
    {
        if (addr_arr[mem_id] == INVALID_TABLE_ADDRESS)
            continue;
        entry = (uint32_t *)(DEVICE_ADDRESS((rdp_runner_core_addr[mem_id] + addr_arr[mem_id]) + offset));
        retval += MGET_I_32(entry, i);
    }
    return retval;
}

#else /* of #ifdef VLAN_COUNTER */

/* Function that retrieves tx_queue drop counters (codel/flush) for case when VLAN counters are not compiled in
   and dedicated codel counters area is used instead.
*/
static bdmf_error_t _tx_queue_drop_count_by_relative_queue_index(uint16_t queue_index, uint32_t *p_packets, uint32_t *p_bytes)
{
    uint8_t rel_queue_index;

    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);
        RDD_PACKETS_AND_BYTES_PACKETS_READ_G(*p_packets, RDD_DS_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_PACKETS_AND_BYTES_BYTES_READ_G(*p_bytes, RDD_DS_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_us_start();
        RDD_PACKETS_AND_BYTES_PACKETS_READ_G(*p_packets, RDD_US_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_PACKETS_AND_BYTES_BYTES_READ_G(*p_bytes, RDD_US_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
    }
    else
    {
        *p_packets = 0;
        *p_bytes = 0;
        return BDMF_ERR_NOT_SUPPORTED;
    }

    return BDMF_ERR_OK;
}
#endif /* #ifndef VLAN_COUNTER */


bdmf_error_t rdd_tx_queue_drop_count_get(uint16_t queue_index, uint32_t *p_packets, uint32_t *p_bytes, bdmf_boolean reset)
{
    uint32_t packets, bytes;
    int rc = BDMF_ERR_OK;
#ifdef VLAN_COUNTER
    uint8_t vlan_stats_enable;
#endif
#ifndef G9991_COMMON
    uint16_t rel_queue_index;
#endif

    if (queue_index >= MAX_TX_QUEUES__NUM_OF)
        return BDMF_ERR_RANGE;

    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
#ifndef G9991_COMMON
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        RDD_PACKETS_AND_BYTES_PACKETS_READ_G(packets, RDD_SQ_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_PACKETS_AND_BYTES_BYTES_READ_G(bytes, RDD_SQ_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);

        *p_packets = packets - tx_queue_drop_counters[queue_index].packets;
        *p_bytes = bytes - tx_queue_drop_counters[queue_index].bytes;

        if (reset)
        {
            tx_queue_drop_counters[queue_index].packets = packets;
            tx_queue_drop_counters[queue_index].bytes = bytes;
        }
#endif
    }
    else
    {
#ifdef VLAN_COUNTER
        /* VLAN counters can't be enabled simultaneously with codel. So, codel reuses VLAN counters area.
        Make sure that vlan counters are not enabled first
        */
        RDD_SYSTEM_CONFIGURATION_ENTRY_VLAN_STATS_ENABLE_READ_G(vlan_stats_enable, RDD_SYSTEM_CONFIGURATION_ADDRESS_ARR, 0);
        if (vlan_stats_enable)
        {
            *p_packets = 0;
            *p_bytes = 0;
            return BDMF_ERR_OK;
        }

        packets = _tx_queue_drop_counter_cumulative_by_core(queue_index, 0, RDD_VLAN_TX_COUNTERS_ADDRESS_ARR);
        bytes = _tx_queue_drop_counter_cumulative_by_core(queue_index, 1, RDD_VLAN_TX_COUNTERS_ADDRESS_ARR);
#else
        rc = _tx_queue_drop_count_by_relative_queue_index(queue_index, &packets, &bytes);
#endif
        *p_packets = packets - tx_queue_drop_counters[queue_index].packets;
        *p_bytes = bytes - tx_queue_drop_counters[queue_index].bytes;

        if (reset)
        {
            tx_queue_drop_counters[queue_index].packets = packets;
            tx_queue_drop_counters[queue_index].bytes = bytes;
        }
    }

    return rc;
}
