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


#include "rdd_complex_rate_limiter.h"
#include "rdd_scheduling.h"
#include "rdd_complex_scheduler.h"
#include "rdp_drv_qm.h"
#include "rdd_basic_rate_limiter.h"

extern uint32_t exponent_list[EXPONENT_LIST_LEN];

/* API to RDPA level */
bdmf_error_t rdd_complex_rate_limiter_cfg(rdpa_traffic_dir dir, int16_t rl_index, uint8_t tm_id, rdd_complex_rl_cfg_t *rl_cfg)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t rl_vec_en, sir_alloc, pir_alloc, pir_limit, budget_residue;
    rdd_rl_float_t complex_rl_float;
    uint32_t rate_limiter_timer_period;
    uint8_t *basic_rate_limiter_p;
    uint8_t *scheduling_queue_table_p;
    uint8_t *rate_limiter_valid_table_p;
    int tm_core = drv_qm_get_tm_core(tm_id, dir);

    
    RDD_BTRACE("dir = %d, rl_index = %d, rl_cfg %px = { sustain_budget = %d, peak_limit = %d, peak_rate = %d, "
        "block type =%d, block_index = %d }\n",
        dir, rl_index, rl_cfg, rl_cfg->sustain_budget, rl_cfg->peak_limit, rl_cfg->peak_rate, rl_cfg->type,
        rl_cfg->block_index);




    if ((rl_index >= rdd_basic_rate_limiter_size_get(dir)) ||
        (rl_index % 2 != 0) ||
        (rl_cfg->type >= num_of_rdd_complex_rl_block))
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

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_SIR_CURRENT_BUDGET_WRITE(
        COMPLEX_RATE_LIMITER_SIR_INIT_RATE, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_PIR_CURRENT_BUDGET_WRITE(
        COMPLEX_RATE_LIMITER_PIR_INIT_RATE, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    
    #ifndef COMPLEX_SCHEDULER_IN_DS
        RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_WRITE(rl_cfg->block_index, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    #else
        if ((rl_cfg->type == rdd_complex_rl_queue) && (dir == rdpa_dir_ds))
            RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_WRITE(
                (rl_cfg->block_index - drv_qm_get_ds_start(rl_cfg->block_index)) , basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
        else
            RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_WRITE(rl_cfg->block_index, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    #endif

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_BLOCK_TYPE_WRITE(rl_cfg->type, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_RL_TYPE_WRITE(RDD_RL_TYPE_COMPLEX, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    sir_alloc = rdd_rate_to_alloc_unit(rl_cfg->sustain_budget, rate_limiter_timer_period, &budget_residue);
    complex_rl_float = rdd_rate_limiter_get_floating_point_rep(sir_alloc, exponent_list);

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_SIR_ALLOC_EXPONENT_WRITE(complex_rl_float.exponent, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_SIR_ALLOC_MANTISSA_WRITE(complex_rl_float.mantissa, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_ALLOC_RESIDUE_WRITE(budget_residue, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    complex_rl_float = rdd_rate_limiter_get_floating_point_rep(sir_alloc * 10, exponent_list);

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_SIR_LIMIT_EXPONENT_WRITE(complex_rl_float.exponent, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_SIR_LIMIT_MANTISSA_WRITE(complex_rl_float.mantissa, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    pir_alloc = rdd_rate_to_alloc_unit(rl_cfg->peak_rate, rate_limiter_timer_period, &budget_residue);
    complex_rl_float = rdd_rate_limiter_get_floating_point_rep(pir_alloc, exponent_list);
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_PIR_ALLOC_EXPONENT_WRITE(complex_rl_float.exponent, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_PIR_ALLOC_MANTISSA_WRITE(complex_rl_float.mantissa, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
   
    pir_limit = rl_cfg->peak_limit;
    if (!pir_limit)
        pir_limit = pir_alloc;
    if (pir_limit > RL_MAX_BUCKET_SIZE)
        pir_limit = RL_MAX_BUCKET_SIZE;
    complex_rl_float = rdd_rate_limiter_get_floating_point_rep(pir_limit, exponent_list);
    if ((!complex_rl_float.exponent) && (!complex_rl_float.mantissa))
        return BDMF_ERR_PARM;

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_PIR_LIMIT_EXPONENT_WRITE(complex_rl_float.exponent, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_PIR_LIMIT_MANTISSA_WRITE(complex_rl_float.mantissa, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    switch ((uint32_t)rl_cfg->type)
    {
        case rdd_complex_rl_queue:
            if (dir == rdpa_dir_us)
            {
                rc = rdd_ag_us_tm_scheduling_queue_table_rate_limiter_index_set(rl_cfg->block_index - drv_qm_get_us_start(), rl_index);
                rc = rc ? rc : rdd_ag_us_tm_scheduling_queue_table_rate_limit_enable_set(rl_cfg->block_index - drv_qm_get_us_start(), 1);
            }
            else
            {
                RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(
                    rl_index, scheduling_queue_table_p + sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT) * (rl_cfg->block_index - drv_qm_get_ds_start(rl_cfg->block_index)));
                RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(
                    1, scheduling_queue_table_p + sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT) * (rl_cfg->block_index - drv_qm_get_ds_start(rl_cfg->block_index)));
            }
            break;
        case rdd_complex_rl_basic_scheduler:
            rc = rdd_basic_scheduler_rate_limiter_cfg(dir, rl_cfg->block_index, rl_index, tm_id);
            break;
        case rdd_complex_rl_complex_scheduler:
            rc = rdd_complex_scheduler_rate_limiter_cfg(dir, rl_cfg->block_index, rl_index, tm_id);
            break;
        default:
            return BDMF_ERR_INTERNAL;
    }

    /* enable the rate limiter */
    
    RDD_BYTES_4_BITS_READ(rl_vec_en, rate_limiter_valid_table_p + ((rl_index / 32) * sizeof(BYTES_4_STRUCT)));
    rl_vec_en |= (1 << (rl_index & 0x1f));
    RDD_BYTES_4_BITS_WRITE(rl_vec_en, rate_limiter_valid_table_p + ((rl_index / 32) * sizeof(BYTES_4_STRUCT)));

    return rc;
}

bdmf_error_t rdd_complex_rate_limiter_remove(rdpa_traffic_dir dir, int16_t rl_index, uint8_t tm_id)
{
    uint32_t q_sched_index, q_shed_type, queue_index;
    uint8_t *scheduling_queue_table_p;
    bdmf_error_t rc;
    uint32_t type, block_index;
    uint8_t *basic_rate_limiter_p;
    uint32_t rl_vec_en;
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

    RDD_BTRACE("dir = %d, rl_index = %d\n", dir, rl_index);

    if (rl_index >= rdd_basic_rate_limiter_size_get(dir))
    {
        return BDMF_ERR_PARM;
    }

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_BLOCK_TYPE_READ(type, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_BLOCK_INDEX_READ(block_index, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    RDD_BYTES_4_BITS_READ(rl_vec_en, rate_limiter_valid_table_p + ((rl_index / 32) * sizeof(BYTES_4_STRUCT)));
    rl_vec_en &= ~(1 << (rl_index & 0x1f));
    RDD_BYTES_4_BITS_WRITE(rl_vec_en, rate_limiter_valid_table_p + ((rl_index / 32) * sizeof(BYTES_4_STRUCT)));

    switch (type)
    {
        case rdd_complex_rl_queue:
#ifdef COMPLEX_SCHEDULER_IN_DS 
            if (dir == rdpa_dir_ds)
                block_index += drv_qm_get_first_ds_queue_for_tm_index(tm_id);
#endif
            rc = rdd_scheduling_queue_rate_limiter_remove(dir, block_index, 1);

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
            rdd_scheduler_wake_up_bbh(dir, q_sched_index, q_shed_type, tm_core, 1);
            
            break;
        case rdd_complex_rl_basic_scheduler:
            rc = rdd_basic_scheduler_rate_limiter_remove(dir, block_index, tm_id);
            rdd_scheduler_wake_up_bbh(dir, block_index, 0, tm_core, 1);

            break;
        case rdd_complex_rl_complex_scheduler:
            rc = rdd_complex_scheduler_rate_limiter_remove(dir, block_index, tm_id);
            rdd_scheduler_wake_up_bbh(dir, block_index, 1, tm_core, 1);
            break;
        default:
            return BDMF_ERR_INTERNAL;
    }

    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_SIR_CURRENT_BUDGET_WRITE(
        COMPLEX_RATE_LIMITER_SIR_INIT_RATE, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));
    RDD_COMPLEX_RATE_LIMITER_DESCRIPTOR_PIR_CURRENT_BUDGET_WRITE(
        COMPLEX_RATE_LIMITER_PIR_INIT_RATE, basic_rate_limiter_p + (rl_index)*sizeof(BASIC_RATE_LIMITER_DESCRIPTOR_STRUCT));

    return rc;
}

