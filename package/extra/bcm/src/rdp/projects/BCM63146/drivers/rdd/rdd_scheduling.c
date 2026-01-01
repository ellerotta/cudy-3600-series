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
#include "rdp_drv_qm.h"
#if defined(BCM6813)
#include "rdd_ghost_reporting.h"
#endif
#define SCHED_WAKE_UP_DONE 0xff

uint32_t exponent_list[] = {0, 3, 6, 9};

/* It is impossible to reset counters safely. Use base value instead. Used for codel/flush drop */
static PACKETS_AND_BYTES_STRUCT tx_queue_drop_counters[MAX_TX_QUEUES__NUM_OF];

extern bdmf_error_t rdd_complex_scheduler_rate_set(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, uint32_t block_bit_mask, uint8_t tm_id, uint8_t is_complex_rl);
extern bdmf_error_t rdd_basic_scheduler_rate_set(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, uint8_t queue_bit_mask, uint8_t tm_id);

void rdd_bbh_queue_init(void)
{
    int i;
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;
#if defined(BCM63146)
    for (i = RDD_US_CHANNEL_OFFSET_DSL; i <= RDD_US_CHANNEL_OFFSET_DSL_END; i++)
    {
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + i;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_DSL, entry);
    }
#else
    for (i = RDD_US_CHANNEL_OFFSET_ETHWAN; i <= RDD_US_CHANNEL_OFFSET_ETHWAN_END; i++)
    {
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + i;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_LAN, entry);
    }

    for (i = RDD_US_CHANNEL_OFFSET_ETHWAN_1; i <= RDD_US_CHANNEL_OFFSET_ETHWAN_1_END; i++)
    {
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + i;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_LAN_1, entry);
    }

#if defined(BCM6813)
    for (i = RDD_US_CHANNEL_OFFSET_TCONT; i <= RDD_US_CHANNEL_OFFSET_TCONT_END; i++)
    {
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + i;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_PON_PD, entry);
    }
#endif

    for (i = 0; i < DS_TM_BBH_QUEUE_TABLE_SIZE; ++i)
    {
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) + i;
        if (i <= BBH_ID_5_10G)
            RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_LAN, entry);
        else
            RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_LAN_1, entry);
    }
#endif
}

#ifdef EPON
bdmf_error_t rdd_tm_epon_cfg(void)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t llid, bbh_queue_index;
    rdd_scheduling_queue_descriptor_t cfg = {};
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;

    RDD_BTRACE("\n");
    for (llid = 0; (!rc) && (llid < MAX_NUM_OF_LLID); ++llid)
    {
        bbh_queue_index = llid + RDD_US_CHANNEL_OFFSET_TCONT;
        cfg.bbh_queue_index = bbh_queue_index;
        rc = rdd_scheduling_scheduler_block_cfg(rdpa_dir_us, 0, llid + drv_qm_get_us_epon_start(), &cfg, 0, 0, 1, rdpa_port_epon);
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + bbh_queue_index;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_PON_STAT, entry);
        RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_WRITE_G(llid, RDD_US_TM_BBH_QUEUE_TABLE_ADDRESS_ARR, bbh_queue_index);
    }
    return rc;
}
#endif

bdmf_error_t rdd_scheduling_flush_timer_set(void)
{
    bdmf_error_t rc = BDMF_ERR_OK;

#if !defined(HW_QM_QUEUE_FLUSH)
    SCHEDULING_FLUSH_GLOBAL_CFG_STRUCT *entry;

    RDD_BTRACE("\n");
    /* flush task - ds core */
    entry = RDD_SCHEDULING_FLUSH_GLOBAL_CFG_PTR(get_runner_idx(ds_tm_runner_image));
    RDD_BYTE_1_BITS_WRITE(FLUSH_TASK_TIMER_INTERVAL, entry);

    /* flush task - us core */
    entry = RDD_SCHEDULING_FLUSH_GLOBAL_CFG_PTR(get_runner_idx(us_tm_runner_image));
    RDD_BYTE_1_BITS_WRITE(FLUSH_TASK_TIMER_INTERVAL, entry);

    /* Make sure the timer configured before the cpu weakeup */
    WMB();

    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), US_TM_FLUSH_THREAD_NUMBER);
    rc = rc ? rc : ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(ds_tm_runner_image), DS_TM_FLUSH_THREAD_NUMBER);
#endif

    return rc;
}

bdmf_error_t rdd_us_budget_allocation_timer_set(void)
{
    BUDGET_ALLOCATION_TIMER_VALUE_STRUCT *entry;
    bdmf_error_t rc = BDMF_ERR_OK;

    RDD_BTRACE("\n");
    /* budget allocation task - us core */
    entry = RDD_BUDGET_ALLOCATION_TIMER_VALUE_PTR(get_runner_idx(us_tm_runner_image));
    RDD_BYTES_2_BITS_WRITE(US_RATE_LIMITER_TIMER_PERIOD, entry);

    /* Make sure the timer configured before the cpu weakeup */
    WMB();
    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), US_TM_BUDGET_ALLOCATION_US_THREAD_NUMBER);
    rc = rc ? rc : ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), US_TM_OVL_BUDGET_ALLOCATION_US_THREAD_NUMBER);
    return rc;
}

bdmf_error_t rdd_ds_budget_allocation_timer_set(void)
{
    BUDGET_ALLOCATION_TIMER_VALUE_STRUCT *entry;
    bdmf_error_t rc = BDMF_ERR_OK;

    RDD_BTRACE("\n");
    /* budget allocation task - ds core */
    entry = RDD_BUDGET_ALLOCATION_TIMER_VALUE_PTR(get_runner_idx(ds_tm_runner_image));
    RDD_BYTES_2_BITS_WRITE(DS_RATE_LIMITER_TIMER_PERIOD, entry);

    /* Make sure the timer configured before the cpu weakeup */
    WMB();
    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(ds_tm_runner_image), DS_TM_BUDGET_ALLOCATION_DS_THREAD_NUMBER);
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
    bdmf_error_t err;
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
            if (enable)
            {
                err = rdd_ag_ds_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_ds_start(0), scheduler_cfg);
                if (!err)
                    err = rdd_ag_ds_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_ds_start(0), enable);
                return err;
            }
            else
            {
                err = rdd_ag_ds_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_ds_start(0), enable);
                if (!err)
                    err = rdd_ag_ds_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_ds_start(0), scheduler_cfg);
                return err;
            }
        }
        else /* if dir == rdpa_dir_us) */
        {
#if defined(BCM6813)
            if ((scheduler_cfg->bbh_queue_index >= RDD_US_CHANNEL_OFFSET_TCONT) &&
                (scheduler_cfg->bbh_queue_index <= RDD_US_CHANNEL_OFFSET_TCONT_END))
            {
                rdd_ghost_reporting_mapping_queue_to_wan_channel((qm_queue_index - drv_qm_get_us_start()),
                    scheduler_cfg->bbh_queue_index - RDD_US_CHANNEL_OFFSET_TCONT, enable);
            }
#endif

            if (enable)
            {
                err = rdd_ag_us_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_us_start(), scheduler_cfg);
                if (!err)
                    err = rdd_ag_us_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_us_start(), enable);
                return err;
            }
            else
            {
                err = rdd_ag_us_tm_scheduling_queue_table_enable_set(qm_queue_index - drv_qm_get_us_start(), enable);
                if (!err)
                    err = rdd_ag_us_tm_scheduling_queue_descriptor_set(qm_queue_index - drv_qm_get_us_start(), scheduler_cfg);
                return err;
            }
        }
    }
}

bdmf_error_t rdd_scheduling_queue_rate_limiter_remove(rdpa_traffic_dir dir, uint8_t qm_queue_index, uint8_t is_complex_rl)
{
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *entry;
    bdmf_boolean block_type;
    uint8_t block_index, bit_mask;
    bdmf_error_t rc;

    RDD_BTRACE("dir = %d, qm_queue_index = %d\n", dir, qm_queue_index);

    if (dir == rdpa_dir_ds)
    {
        entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image))) +
            (qm_queue_index - drv_qm_get_ds_start(0));
    }
    else
    {
        entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) +
            (qm_queue_index - drv_qm_get_us_start());
    }

    RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(0, entry);

    /* make sure the queue has budget in scheduler */
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_BLOCK_TYPE_READ(block_type, entry);
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_READ(block_index, entry);
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUEUE_BIT_MASK_READ(bit_mask, entry);
    if (block_type)
        rc = rdd_complex_scheduler_rate_set(dir, block_index, (1 << bit_mask),  drv_qm_get_ds_tm_index(qm_queue_index), is_complex_rl);
    else
        rc = rdd_basic_scheduler_rate_set(dir, block_index, bit_mask,  drv_qm_get_ds_tm_index(qm_queue_index));

    return rc;
}

static uint8_t rdd_tm_debug_sched_index_get(rdpa_traffic_dir dir, uint8_t bbh_queue)
{
    uint8_t sched_index;
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;

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

    if (dir == rdpa_dir_ds)
    {
#if (!defined(COMPLEX_SCHEDULER_IN_DS))
        basic_scheduler_wake_up_cmd_p = (uint8_t *)RDD_BASIC_SCHEDULER_WAKEUP_CMD_DS_PTR(tm_core);
#endif
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM) 
        complex_scheduler_wake_up_cmd_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_WAKEUP_CMD_DS_PTR(tm_core);
#endif
    }
    else
    {
        complex_scheduler_wake_up_cmd_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_WAKEUP_CMD_US_PTR(tm_core);
        basic_scheduler_wake_up_cmd_p = (uint8_t *)RDD_BASIC_SCHEDULER_WAKEUP_CMD_US_PTR(tm_core);
    }

    wake_up_command = (scheduler_index | complex_rate_limit << 31);
    if (block_type)
        RDD_BYTES_4_BITS_WRITE(wake_up_command, complex_scheduler_wake_up_cmd_p);
    else
        RDD_BYTES_4_BITS_WRITE(wake_up_command, basic_scheduler_wake_up_cmd_p);

    WMB();
    if (dir == rdpa_dir_ds)
        ag_drv_rnr_regs_cfg_cpu_wakeup_set(tm_core, drv_qm_get_budget_allocator_task(tm_core));
    else
        ag_drv_rnr_regs_cfg_cpu_wakeup_set(get_runner_idx(us_tm_runner_image), TM_BUDGET_ALLOCATION_US_THREAD_NUMBER);

    do
    {
         if (block_type)
            RDD_BYTES_4_BITS_READ(wake_up_index, complex_scheduler_wake_up_cmd_p);
        else
            RDD_BYTES_4_BITS_READ(wake_up_index, basic_scheduler_wake_up_cmd_p);
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
    if (dir == rdpa_dir_ds)
    {
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + complex_scheduler_index;
        base_queue_entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image)));
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
        first_queue_index = drv_qm_get_us_start();
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
            if (dir == rdpa_dir_ds)
                bs_entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + info->queue_info[i].queue_index;
            else
                bs_entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + info->queue_info[i].queue_index;
            RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_SLOT_INDEX_READ(info->queue_info[i].queue_bit_mask , bs_entry);
        }
        else
        {
            /* complex scheduler */
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
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
    if (dir == rdpa_dir_ds)
    {
        entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + basic_scheduler_index;
        base_queue_entry = ((SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(get_runner_idx(ds_tm_runner_image)));
        first_queue_index = drv_qm_get_ds_start(0);
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

/* Function that retrieves tx_queue drop counters (codel/flush) for case
 * and dedicated codel counters area is used instead. */
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

bdmf_error_t rdd_tx_queue_drop_count_get(uint16_t queue_index, uint32_t *p_packets, uint32_t *p_bytes, bdmf_boolean reset)
{
    uint32_t packets, bytes;
    int rc = BDMF_ERR_OK;
#if defined(CODEL_ON_SERVICE_QUEUES) || defined(PI2_ON_SERVICE_QUEUES)
    uint16_t rel_queue_index;
#endif

    if (queue_index >= MAX_TX_QUEUES__NUM_OF)
        return BDMF_ERR_RANGE;

#if defined(CODEL_ON_SERVICE_QUEUES) || defined(PI2_ON_SERVICE_QUEUES)
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        RDD_PACKETS_AND_BYTES_PACKETS_READ_G(packets, RDD_SQ_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_PACKETS_AND_BYTES_BYTES_READ_G(bytes, RDD_SQ_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index);
    }
    else
#endif
    {
        rc = _tx_queue_drop_count_by_relative_queue_index(queue_index, &packets, &bytes);
    }

    *p_packets = packets - tx_queue_drop_counters[queue_index].packets;
    *p_bytes = bytes - tx_queue_drop_counters[queue_index].bytes;

    if (reset)
    {
        tx_queue_drop_counters[queue_index].packets = packets;
        tx_queue_drop_counters[queue_index].bytes = bytes;
    }

    return rc;
}
