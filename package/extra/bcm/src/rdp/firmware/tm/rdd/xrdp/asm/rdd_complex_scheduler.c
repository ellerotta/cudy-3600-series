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
#include "rdd_scheduling.h"
#include "rdd_basic_rate_limiter.h"
#include "rdd_complex_scheduler.h"
#include "rdp_drv_qm.h"

extern bdmf_error_t rdd_scheduling_scheduler_block_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint16_t qm_queue_index, 
    rdd_scheduling_queue_descriptor_t *scheduler_cfg, bdmf_boolean type, uint8_t dwrr_offset, bdmf_boolean enable, rdpa_port_type port_type);

typedef struct
{
    uint8_t bbh_queue;
    complex_scheduler_block_type_t type[COMPLEX_SCHEDULER_NUM_OF_QUEUES];
} complex_scheduler_info_t;

/* mappping between complex scheduler to bbh queue */
static complex_scheduler_info_t complex_scheduler_info[2][RDD_COMPLEX_SCHEDULER_TABLE_MAX_SIZE][MAX_TM_CORES_PER_DIRECTION];

static uint32_t _rdd_complex_scheduler_cfg_bbh_queue_get(rdpa_traffic_dir dir, uint32_t index, rdpa_port_type port_type)
{
#ifdef XRDP_BBH_PER_LAN_PORT
    if (dir == rdpa_dir_ds)
        return port_mapping_bb_tx_id_get(port_type, index);
    else
        return index;
#else
    return index;
#endif
}

bdmf_error_t rdd_complex_scheduler_size(rdpa_traffic_dir dir)
{
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    if (dir == rdpa_dir_us)
        return RDD_COMPLEX_SCHEDULER_TABLE_US_SIZE;
    else
        return RDD_COMPLEX_SCHEDULER_TABLE_DS_SIZE;
#else
        return RDD_COMPLEX_SCHEDULER_TABLE_US_SIZE;
#endif
}

/* API to RDPA level */
bdmf_error_t rdd_complex_scheduler_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint8_t complex_scheduler_index, complex_scheduler_cfg_t *cfg, 
    rdpa_port_type port_type)
{
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    BBH_QUEUE_DESCRIPTOR_STRUCT *bbh_queue_entry;
    bdmf_error_t rc = 0;
    uint32_t bbh_queue_index;
    uint8_t core_index = port_mapping_get_tx_runner_core_by_channel_id(port_type, channel_id);
    uint8_t tm_index = port_mapping_get_tm_index_by_channel_id(port_type, channel_id);

    RDD_BTRACE("dir = %d, complex_scheduler_index = %d, cfg %px = { dwrr_offset_sir = %d, dwrr_offset_pir = %d, "
        "bbh_q_desc_id = %d, hw_bbh_qid = %d }\n",
        dir, complex_scheduler_index, cfg, cfg->dwrr_offset_sir, cfg->dwrr_offset_pir, cfg->bbh_q_desc_id,
        cfg->hw_bbh_qid);

    if  ((complex_scheduler_index >= rdd_complex_scheduler_size(dir)) ||
        (cfg->dwrr_offset_sir >= complex_scheduler_num_of_dwrr_offset) ||
        (cfg->dwrr_offset_pir >= complex_scheduler_num_of_dwrr_offset) ||
        (cfg->bbh_q_desc_id >= RDD_BBH_QUEUE_TABLE_SIZE))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
    {
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(core_index)) + complex_scheduler_index;
        bbh_queue_entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_DS_TM_BBH_QUEUE_TABLE_PTR(core_index)) + cfg->bbh_q_desc_id;
#else
        return BDMF_ERR_NOT_SUPPORTED;
#endif
    }
    else if (dir == rdpa_dir_us)
    {
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(core_index)) + complex_scheduler_index;
        bbh_queue_entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(core_index)) + cfg->bbh_q_desc_id;
    }

    complex_scheduler_info[dir][complex_scheduler_index][tm_index].bbh_queue = cfg->bbh_q_desc_id;

    /* initialize budget for all queues - relevent for the case no rate limiter was configured */
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(COMPLEX_SCHEDULER_FULL_BUDGET_VECTOR, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_SIR_WRITE(cfg->dwrr_offset_sir, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_PIR_WRITE(cfg->dwrr_offset_pir, entry);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, entry);

    if (!cfg->parent_exists)
    {
        RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_WRITE(complex_scheduler_index, bbh_queue_entry);
        RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_TYPE_WRITE(RDD_SCHED_TYPE_COMPLEX, bbh_queue_entry);
#if !defined(BCM_PON_XRDP)
        RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_WRITE(cfg->hw_bbh_qid, bbh_queue_entry);
#endif
    }

    bbh_queue_index = _rdd_complex_scheduler_cfg_bbh_queue_get(dir, cfg->bbh_q_desc_id, port_type);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BBH_QUEUE_WRITE(bbh_queue_index, entry);
    return rc;
}

bdmf_error_t rdd_complex_scheduler_block_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint8_t complex_scheduler_index,
    complex_scheduler_block_t *block, rdpa_port_type port_type)
{
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    rdd_scheduling_queue_descriptor_t block_cfg = {};
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t is_scheduler_slot_vector;
    uint32_t is_scheduler_basic_vector;
    uint8_t tm_index = port_mapping_get_tm_index_by_channel_id(port_type, channel_id);
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    uint8_t core_index = port_mapping_get_tx_runner_core_by_channel_id(port_type, channel_id);
#endif

    RDD_BTRACE("dir = %d, complex_scheduler_index = %d, block %px = { block_index = %d, scheduler_slot_index = %d, "
        "bs_dwrr_offset = %d, quantum_number = %d }\n",
        dir, complex_scheduler_index, block, block->block_index, block->scheduler_slot_index, block->bs_dwrr_offset,
        block->quantum_number);

    if ((complex_scheduler_index >= rdd_complex_scheduler_size(dir)) || (block->scheduler_slot_index >= COMPLEX_SCHEDULER_NUM_OF_QUEUES))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
    {
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
#if !defined(COMPLEX_SCHEDULER_IN_DS)
        /* there exists only complex scheduler with the above define */
        if (block->block_type == complex_scheduler_block_bs)
            rdd_ag_ds_tm_basic_scheduler_table_ds_quantum_number_set(block->block_index, block->quantum_number);
        else
#endif
        if (block->block_type == complex_scheduler_block_cs)
        {
            entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(core_index)) + block->block_index;
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE(block->quantum_number, entry);
        }
        else
        {
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_WRITE(block->quantum_number, (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(core_index) +
                ((block->block_index - drv_qm_get_ds_start(block->block_index)) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
        }
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(core_index)) + complex_scheduler_index;
#else
        return BDMF_ERR_NOT_SUPPORTED;
#endif
    }
    else
    {
        if (block->block_type == complex_scheduler_block_bs)
            rdd_ag_us_tm_basic_scheduler_table_us_quantum_number_set(block->block_index, block->quantum_number);
        else if (block->block_type == complex_scheduler_block_cs)
        {
            entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + block->block_index;
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE(block->quantum_number, entry);
        }
        else
            rc = rdd_ag_us_tm_scheduling_queue_table_quantum_number_set((block->block_index - drv_qm_get_us_start()), block->quantum_number);
        
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + complex_scheduler_index;
    }

    /* save the block type */
    complex_scheduler_info[dir][complex_scheduler_index][tm_index].type[block->scheduler_slot_index] = block->block_type;

    /* mapping block to complex scheduler */
    if (block->block_type == complex_scheduler_block_queue)
    {
        if (dir == rdpa_dir_ds)
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_WRITE(block->block_index - drv_qm_get_ds_start(block->block_index), entry, block->scheduler_slot_index);
        else
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_WRITE(block->block_index - drv_qm_get_us_start(), entry, block->scheduler_slot_index);
    }
    else
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_WRITE(block->block_index, entry, block->scheduler_slot_index);

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_SLOT_READ(is_scheduler_slot_vector, entry);
    if (block->block_type != complex_scheduler_block_queue)
        is_scheduler_slot_vector |= (1 << block->scheduler_slot_index);
    else
        is_scheduler_slot_vector &= ~(1 << block->scheduler_slot_index);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_SLOT_WRITE(is_scheduler_slot_vector, entry);

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_BASIC_READ(is_scheduler_basic_vector, entry);
    if (block->block_type == complex_scheduler_block_bs)
        is_scheduler_basic_vector |= (1 << block->scheduler_slot_index);
    else
        is_scheduler_basic_vector &= ~(1 << block->scheduler_slot_index);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_BASIC_WRITE(is_scheduler_basic_vector, entry);

    /* mapping complex scheduler to block */
    block_cfg.bbh_queue_index = complex_scheduler_info[dir][complex_scheduler_index][tm_index].bbh_queue;
    block_cfg.scheduler_index = complex_scheduler_index;
    block_cfg.block_type = 1; /* complex scheduler */
    block_cfg.queue_bit_mask = block->scheduler_slot_index; /* in case of cs under complex scheduler bit mask is the index */

    if (block->block_type == complex_scheduler_block_cs)
    {
        COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *sub_entry;
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
        if (dir == rdpa_dir_ds)
            sub_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(core_index)) + block->block_index;
        else
#endif
        sub_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + block->block_index;

        /* mapping complex scheduler to complex scheduler */
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_EXISTS_WRITE(1, sub_entry);
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_INDEX_WRITE(complex_scheduler_index, sub_entry);
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_SLOT_INDEX_WRITE(block->scheduler_slot_index, sub_entry);
    }
    else
    {
        rc = rc ? rc : rdd_scheduling_scheduler_block_cfg(dir, channel_id, block->block_index, &block_cfg,
            block->block_type == complex_scheduler_block_bs, block->bs_dwrr_offset, 1, port_type);
    }

    return rc;
}

bdmf_error_t rdd_complex_scheduler_block_remove(rdpa_traffic_dir dir, int16_t channel_id, uint8_t complex_scheduler_index, 
    uint8_t scheduler_slot_index, rdpa_port_type port_type)
{
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    rdd_scheduling_queue_descriptor_t block_cfg = {};
    uint8_t block_index;
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t is_scheduler_slot_vector;
    uint32_t is_scheduler_basic_vector;
    complex_scheduler_block_type_t type;
    uint8_t core_index = port_mapping_get_tx_runner_core_by_channel_id(port_type, channel_id);
    uint8_t tm_index = port_mapping_get_tm_index_by_channel_id(port_type, channel_id);

    RDD_BTRACE("dir = %d, complex_scheduler_index = %d, scheduler_slot_index = %d\n",
        dir, complex_scheduler_index, scheduler_slot_index);

    if (complex_scheduler_index >= rdd_complex_scheduler_size(dir))
    {
        return BDMF_ERR_PARM;
    }

    type = complex_scheduler_info[dir][complex_scheduler_index][tm_index].type[scheduler_slot_index];

#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    if (dir == rdpa_dir_ds)
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(core_index)) + complex_scheduler_index;
    else /* if (dir == rdpa_dir_us) */
#endif
    entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(core_index)) + complex_scheduler_index;

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_SLOT_READ(is_scheduler_slot_vector, entry);
    is_scheduler_slot_vector &= ~(1 << scheduler_slot_index);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_SLOT_WRITE(is_scheduler_slot_vector, entry);

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_BASIC_READ(is_scheduler_basic_vector, entry);
    is_scheduler_basic_vector &= ~(1 << scheduler_slot_index);
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_SCHEDULER_BASIC_WRITE(is_scheduler_basic_vector, entry);

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_READ(block_index, entry, scheduler_slot_index);
    if (dir == rdpa_dir_ds && type == complex_scheduler_block_queue)
        /* FIXME:MULTIPLE_BBH_TX - fix when open ds complex scheduler */
        block_index += drv_qm_get_ds_start(0);

    if (type == complex_scheduler_block_cs)
    {
        COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *sub_entry;

#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_READ(block_index, entry, scheduler_slot_index);

        if (dir == rdpa_dir_ds)
            sub_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(core_index)) + block_index;
        else
#else
        if (dir == rdpa_dir_ds)
            return BDMF_ERR_NOT_SUPPORTED;

        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_BLOCK_INDEX_READ(block_index, entry, scheduler_slot_index);
#endif
        sub_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + block_index;

        /* mapping complex scheduler to complex scheduler */
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_EXISTS_WRITE(0, sub_entry);
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_INDEX_WRITE(0, sub_entry);
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_SLOT_INDEX_WRITE(0, sub_entry);
    }
    else
    {
        rc = rdd_scheduling_scheduler_block_cfg(dir, channel_id, block_index, &block_cfg,
            type == complex_scheduler_block_bs, 0, 0, port_type);
    }
    return rc;
}

static bdmf_error_t _rdd_complex_scheduler_rate_limiter_cfg(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, uint8_t rate_limiter_index, 
    bdmf_boolean enable, uint8_t tm_id)
{
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    bdmf_boolean parent_exists;
    int tm_core = drv_qm_get_tm_core(tm_id, dir);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Illegal tm_core value %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (rate_limiter_index >= rdd_basic_rate_limiter_size_get(dir) || complex_scheduler_index >= rdd_complex_scheduler_size(dir))
    {
        return BDMF_ERR_PARM;
    }

    /* FIXME:MULTIPLE_BBH_TX - fix when open ds complex scheduler */
#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    if (dir == rdpa_dir_ds)
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(tm_core)) + complex_scheduler_index;
    else if (dir == rdpa_dir_us)
#endif
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(tm_core)) + complex_scheduler_index;

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_EXISTS_READ(parent_exists, entry);

    if (enable)
    {
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(rate_limiter_index, entry);
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(1, entry);
        if (parent_exists)
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, entry);
    }
    else
    {
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(0, entry);

        if (parent_exists)
        {
            int cs_slot_index, cs_index;
            uint32_t budget_vector;
            COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *parent_entry;

#if !defined(COMPLEX_SCHEDULER_IN_DS) && !defined(COMPLEX_SCHED_ON_BOTH_TM)
            if (dir == rdpa_dir_ds)
                return BDMF_ERR_NOT_SUPPORTED;
#endif

            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_SLOT_INDEX_READ(cs_slot_index, entry);
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_PARENT_SCHEDULER_INDEX_READ(cs_index, entry);

#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
            if (dir == rdpa_dir_ds)
                parent_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(tm_core)) + cs_index;
            else if (dir == rdpa_dir_us)
#endif
                parent_entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(tm_core)) + cs_index;

            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_READ(budget_vector, parent_entry);
            budget_vector |= (1 << cs_slot_index);
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(budget_vector, parent_entry);
        }
        else
        {
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, entry);
        }
    }
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_complex_scheduler_rate_limiter_cfg(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, uint8_t rate_limiter_index, uint8_t tm_id)
{
    RDD_BTRACE("dir = %d, complex_scheduler_index = %d, rate_limiter_index = %d\n",
        dir, complex_scheduler_index, rate_limiter_index);

    return _rdd_complex_scheduler_rate_limiter_cfg(dir, complex_scheduler_index, rate_limiter_index, 1,  tm_id);
}

bdmf_error_t rdd_complex_scheduler_rate_limiter_remove(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, uint8_t tm_id)
{
    RDD_BTRACE("dir = %d, complex_scheduler_index = %d\n",
        dir, complex_scheduler_index);

    return _rdd_complex_scheduler_rate_limiter_cfg(dir, complex_scheduler_index, 0, 0, tm_id);
}

/* API to block */
bdmf_error_t rdd_complex_scheduler_rate_set(rdpa_traffic_dir dir, uint8_t complex_scheduler_index, uint32_t block_bit_mask, uint8_t tm_id, uint8_t is_complex_rl)
{
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    uint32_t budget;
    int tm_core = drv_qm_get_tm_core(tm_id, dir);

    RDD_BTRACE("dir = %d, complex_scheduler_index = %d, block_bit_mask = %d\n",
        dir, complex_scheduler_index, block_bit_mask);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Illegal tm_core value %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (complex_scheduler_index >= rdd_complex_scheduler_size(dir))
    {
        return BDMF_ERR_PARM;
    }

#if defined(COMPLEX_SCHEDULER_IN_DS) || defined(COMPLEX_SCHED_ON_BOTH_TM)
    if (dir == rdpa_dir_ds)
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(tm_core)) + complex_scheduler_index;
    else
#endif
        entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(tm_core)) + complex_scheduler_index;

    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_READ(budget, entry);
    budget |= block_bit_mask;
    RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(budget, entry);
    if (is_complex_rl)
    {
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_READ(budget, entry);
        budget |= block_bit_mask;
        RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_WRITE(budget, entry);
    }

    return BDMF_ERR_OK;
}

