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


#include "rdd_basic_rate_limiter.h"
#include "rdd_scheduling.h"
#include "rdp_drv_qm.h"

extern uint32_t exponent_list[EXPONENT_LIST_LEN];

#if !defined(COMPLEX_SCHEDULER_IN_DS) && !defined(COMPLEX_SCHED_ON_BOTH_TM)
/* we cant use auto generated function as it writes for both US and DS tables */
int rdd_us_tm_complex_scheduler_rl_cfg_set(uint32_t _entry, rdd_complex_scheduler_rl_cfg_t *complex_scheduler_rl_cfg)
{
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;

    if (!complex_scheduler_rl_cfg || _entry >= RDD_COMPLEX_SCHEDULER_TABLE_US_SIZE || complex_scheduler_rl_cfg->rate_limiter_index >= 128)
        return BDMF_ERR_PARM;

    entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + _entry;
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(complex_scheduler_rl_cfg->is_positive_budget, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(complex_scheduler_rl_cfg->rate_limiter_index, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(complex_scheduler_rl_cfg->rate_limit_enable, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_DEFICIT_COUNTER_WRITE(complex_scheduler_rl_cfg->deficit_counter, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE(complex_scheduler_rl_cfg->quantum_number, entry);

    return BDMF_ERR_OK;
}
#endif

uint32_t rdd_basic_rate_limiter_size_get(rdpa_traffic_dir dir)
{
    if (dir == rdpa_dir_ds)
        return RDD_BASIC_RATE_LIMITER_TABLE_DS_SIZE;
    else
        return RDD_BASIC_RATE_LIMITER_TABLE_US_SIZE;
}

/* API to RDPA level */
bdmf_error_t rdd_basic_rate_limiter_cfg(rdpa_traffic_dir dir, int16_t basic_rl_index, uint8_t tm_id, rdd_basic_rl_cfg_t *rl_cfg)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t basic_rl_vec_en, alloc_rate, limit_rate, budget_residue;
    rdd_rl_float_t basic_rl_float;
    uint32_t rate_limiter_timer_period;
    uint8_t *basic_rate_limiter_p;
    uint8_t *scheduling_queue_table_p;
    uint8_t *rate_limiter_valid_table_p;
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    uint8_t *complex_scheduler_table_p;
#endif
    int tm_core = drv_qm_get_tm_core(tm_id, dir);

    RDD_BTRACE("dir = %d, basic_rl_index = %d, rl_cfg %px = { rate = %d, block type = %d, block_index = %d  }\n",
        dir, basic_rl_index, rl_cfg, rl_cfg->rate, rl_cfg->type, rl_cfg->block_index);

    if (basic_rl_index >= rdd_basic_rate_limiter_size_get(dir))
    {
        return BDMF_ERR_PARM;
    }

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Illegal tm_core value %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (dir == rdpa_dir_ds)
    {
        basic_rate_limiter_p =  (uint8_t *)RDD_BASIC_RATE_LIMITER_TABLE_DS_PTR(tm_core);
        scheduling_queue_table_p = (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_core);
        rate_limiter_valid_table_p = (uint8_t *)RDD_RATE_LIMITER_VALID_TABLE_DS_PTR(tm_core);
        rate_limiter_timer_period = DS_RATE_LIMITER_TIMER_PERIOD_IN_USEC;
    }
    else
    {
        basic_rate_limiter_p =  (uint8_t *)RDD_BASIC_RATE_LIMITER_TABLE_US_PTR(tm_core);
        scheduling_queue_table_p = (uint8_t *)RDD_US_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_core);
        rate_limiter_valid_table_p = (uint8_t *)RDD_RATE_LIMITER_VALID_TABLE_US_PTR(tm_core);
        rate_limiter_timer_period = US_RATE_LIMITER_TIMER_PERIOD_IN_USEC;
    }



    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_CURRENT_BUDGET_WRITE(BASIC_RATE_LIMITER_INIT_RATE, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_BLOCK_TYPE_WRITE(rl_cfg->type, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    if (rl_cfg->type == rdd_basic_rl_queue)
    {
        if (dir == rdpa_dir_ds)
            RDD_BASIC_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_WRITE(
                (rl_cfg->block_index - drv_qm_get_ds_start(rl_cfg->block_index)), basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
        else
            RDD_BASIC_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_WRITE(
                (rl_cfg->block_index - drv_qm_get_us_start()), basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    }
    else
        RDD_BASIC_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_WRITE(rl_cfg->block_index, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_RL_TYPE_WRITE(RDD_RL_TYPE_BASIC, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    alloc_rate = rdd_rate_to_alloc_unit(rl_cfg->rate, rate_limiter_timer_period, &budget_residue);
    basic_rl_float = rdd_rate_limiter_get_floating_point_rep(alloc_rate, exponent_list);

    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_ALLOC_EXPONENT_WRITE(basic_rl_float.exponent, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_ALLOC_MANTISSA_WRITE(basic_rl_float.mantissa, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_ALLOC_RESIDUE_WRITE(budget_residue, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    limit_rate = rl_cfg->limit;

    if (!limit_rate)
        limit_rate = alloc_rate;
    if (limit_rate > RL_MAX_BUCKET_SIZE)
        limit_rate = RL_MAX_BUCKET_SIZE;
    basic_rl_float = rdd_rate_limiter_get_floating_point_rep(limit_rate, exponent_list);

    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_LIMIT_EXPONENT_WRITE(basic_rl_float.exponent, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_LIMIT_MANTISSA_WRITE(basic_rl_float.mantissa, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    switch ((uint32_t)rl_cfg->type)
    {
        case rdd_basic_rl_queue:
            if (dir == rdpa_dir_us)
            {
                rc = rdd_ag_us_tm_scheduling_queue_table_rate_limiter_index_set(rl_cfg->block_index - drv_qm_get_us_start(), basic_rl_index);
                rc = rc ? rc : rdd_ag_us_tm_scheduling_queue_table_rate_limit_enable_set(rl_cfg->block_index - drv_qm_get_us_start(), 1);
            }
            else
            {
                RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(
                    basic_rl_index, scheduling_queue_table_p +  ((rl_cfg->block_index - drv_qm_get_ds_start(rl_cfg->block_index))) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT));
                RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(
                    1, scheduling_queue_table_p +  ((rl_cfg->block_index - drv_qm_get_ds_start(rl_cfg->block_index))) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT));
            }
            break;
        case rdd_basic_rl_basic_scheduler:
            rc = rdd_basic_scheduler_rate_limiter_cfg(dir, rl_cfg->block_index, basic_rl_index, tm_id);
            break;
        case rdd_basic_rl_complex_scheduler:
            {
                rdd_complex_scheduler_rl_cfg_t cs_rl_cfg =
                {
                    .rate_limit_enable = 1,
                    .is_positive_budget = 1,
                    .rate_limiter_index = basic_rl_index
                };

#if !defined(COMPLEX_SCHEDULER_IN_DS) && !defined(COMPLEX_SCHED_ON_BOTH_TM)
                if (dir == rdpa_dir_ds)
                    return BDMF_ERR_NOT_SUPPORTED;
                else
                    rc = rdd_us_tm_complex_scheduler_rl_cfg_set(rl_cfg->block_index, &cs_rl_cfg);
#else
                if (dir == rdpa_dir_ds)
                    complex_scheduler_table_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(tm_core);
                else
                    complex_scheduler_table_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(tm_core);
                complex_scheduler_table_p += (rl_cfg->block_index) * sizeof(COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT);

                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(cs_rl_cfg.is_positive_budget, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(cs_rl_cfg.rate_limiter_index, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(cs_rl_cfg.rate_limit_enable, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_DEFICIT_COUNTER_WRITE(cs_rl_cfg.deficit_counter, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE(cs_rl_cfg.quantum_number, complex_scheduler_table_p);
#endif
            }
            break;
    }

    /* enable the rate limiter */
    RDD_BYTES_4_BITS_READ(basic_rl_vec_en, rate_limiter_valid_table_p + ((basic_rl_index / 32) * sizeof(BYTES_4_STRUCT)));
    basic_rl_vec_en |= (1 << (basic_rl_index & 0x1f));
    RDD_BYTES_4_BITS_WRITE(basic_rl_vec_en, rate_limiter_valid_table_p + ((basic_rl_index / 32) * sizeof(BYTES_4_STRUCT)));
    return rc;
}

bdmf_error_t rdd_basic_rate_limiter_remove(rdpa_traffic_dir dir, int16_t basic_rl_index, uint8_t tm_id)
{
    uint32_t block_index, type, queue_index;
    uint8_t *scheduling_queue_table_p;
    uint32_t q_sched_index, q_shed_type;
    bdmf_error_t rc = BDMF_ERR_OK;
    uint8_t *basic_rate_limiter_p;

#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    uint8_t *complex_scheduler_table_p;
#endif

    uint32_t basic_rl_vec_en;
    uint8_t *rate_limiter_valid_table_p;
    int tm_core = drv_qm_get_tm_core(tm_id, dir);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Illegal tm_core value %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (dir == rdpa_dir_ds)
    {
        basic_rate_limiter_p =  (uint8_t *)RDD_BASIC_RATE_LIMITER_TABLE_DS_PTR(tm_core);
        rate_limiter_valid_table_p = (uint8_t *)RDD_RATE_LIMITER_VALID_TABLE_DS_PTR(tm_core);
    }
    else
    {
        basic_rate_limiter_p =  (uint8_t *)RDD_BASIC_RATE_LIMITER_TABLE_US_PTR(tm_core);
        rate_limiter_valid_table_p = (uint8_t *)RDD_RATE_LIMITER_VALID_TABLE_US_PTR(tm_core);
    }

    RDD_BTRACE("dir = %d, basic_rl_index = %d\n", dir, basic_rl_index);

    if (basic_rl_index >= rdd_basic_rate_limiter_size_get(dir))
    {
        return BDMF_ERR_PARM;
    }

    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_BLOCK_TYPE_READ(type, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_READ(block_index, basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    RDD_BYTES_4_BITS_READ(basic_rl_vec_en, rate_limiter_valid_table_p + ((basic_rl_index / 32) * sizeof(BYTES_4_STRUCT)));
    basic_rl_vec_en &= ~(1 << (basic_rl_index & 0x1f));
    RDD_BYTES_4_BITS_WRITE(basic_rl_vec_en, rate_limiter_valid_table_p + ((basic_rl_index / 32) * sizeof(BYTES_4_STRUCT)));

    switch (type)
    {
        case rdd_basic_rl_queue:
            if (dir == rdpa_dir_ds)
                block_index += drv_qm_get_first_ds_queue_for_tm_index(tm_id);
            rc = rdd_scheduling_queue_rate_limiter_remove(dir, block_index, 0);

            if (dir == rdpa_dir_ds)
            {
                scheduling_queue_table_p = (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_core);
                queue_index = block_index - drv_qm_get_ds_start(block_index);
            }
            else
            {
                scheduling_queue_table_p = (uint8_t *)RDD_US_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_core);
                queue_index = block_index - drv_qm_get_us_start();
            }

            RDD_SCHEDULING_QUEUE_DESCRIPTOR_BLOCK_TYPE_READ(q_shed_type, scheduling_queue_table_p + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_READ(q_sched_index,  scheduling_queue_table_p + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
            rdd_scheduler_wake_up_bbh(dir, q_sched_index, q_shed_type, tm_core, 0);

            break;
        case rdd_basic_rl_basic_scheduler:
            rc = rdd_basic_scheduler_rate_limiter_remove(dir, block_index, tm_id);
            rdd_scheduler_wake_up_bbh(dir, block_index, 0, tm_core, 0);
            break;
        case rdd_basic_rl_complex_scheduler:
            {
                rdd_complex_scheduler_rl_cfg_t rl_cfg =
                {
                    .rate_limit_enable = 0,
                    .is_positive_budget = 1,
                    .rate_limiter_index = 0
                };
#if !defined(COMPLEX_SCHEDULER_IN_DS) && !defined(COMPLEX_SCHED_ON_BOTH_TM)
                if (dir == rdpa_dir_ds)
                    return BDMF_ERR_NOT_SUPPORTED;
                else
                {
                    rc = rdd_us_tm_complex_scheduler_rl_cfg_set(block_index, &rl_cfg);
                    rdd_scheduler_wake_up_bbh(dir, block_index, 1, tm_core, 0);
                }
#else
                if (dir == rdpa_dir_ds)
                    complex_scheduler_table_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(tm_core);
                else
                    complex_scheduler_table_p = (uint8_t *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(tm_core);
                complex_scheduler_table_p += (block_index) * sizeof(COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT);

                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(rl_cfg.is_positive_budget, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(rl_cfg.rate_limiter_index, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(rl_cfg.rate_limit_enable, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_DEFICIT_COUNTER_WRITE(rl_cfg.deficit_counter, complex_scheduler_table_p);
                RDD_COMPLEX_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE(rl_cfg.quantum_number, complex_scheduler_table_p);

                rdd_scheduler_wake_up_bbh(dir, block_index, 1, tm_core, 0);
#endif
            }
            break;
        default:
            return BDMF_ERR_INTERNAL;
    }
    RDD_BASIC_RATE_LIMITER_DESCRIPTOR_CURRENT_BUDGET_WRITE(BASIC_RATE_LIMITER_INIT_RATE,  basic_rate_limiter_p + (basic_rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    return rc;
}

