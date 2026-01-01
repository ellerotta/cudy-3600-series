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
#include "rdd_ag_ds_tm.h"
#include "rdd_ag_us_tm.h"
#if !defined G9991_COMMON
#include "rdd_ag_service_queues.h"
#endif
#include "rdd_basic_scheduler.h"
#include "rdd_complex_scheduler.h"
#include "rdp_drv_qm.h"
#include "rdd_basic_rate_limiter.h"
#include "rdd_scheduling.h"

#ifdef G9991_FC
extern rdpa_emac vport_to_emac[];
#endif
extern bdmf_error_t rdd_scheduling_scheduler_block_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint16_t qm_queue_index, rdd_scheduling_queue_descriptor_t *scheduler_cfg, 
    bdmf_boolean type, uint8_t dwrr_offset, bdmf_boolean enable, rdpa_port_type port_type);

/* mappping between basic scheduler to bbh queue */
static uint8_t basic_scheduler_to_bbh_queue[2][RDD_BASIC_SCHEDULER_TABLE_MAX_SIZE][MAX_TM_CORES_PER_DIRECTION];

/* bbh_q is splitted into 2 separated fields (msb and lsb) because of data structure limitation (runner and host write to same bytes creates race condition */
void write_bbh_queue_g(rdd_bb_id bb_id, uint32_t *table_ptr, uint8_t basic_scheduler_index, uint8_t channel_id, uint8_t runner_idx)
{
    RDD_BASIC_SCHEDULER_DESCRIPTOR_BBH_QUEUE_LSB_WRITE_G(bb_id & 0xF, table_ptr, basic_scheduler_index);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_BBH_QUEUE_MSB_WRITE_G((bb_id >> 4) & 0x3, table_ptr, basic_scheduler_index);
}

uint32_t rdd_basic_scheduler_table_size_get(rdpa_traffic_dir dir)
{
    if (dir == rdpa_dir_ds)
#ifdef COMPLEX_SCHEDULER_IN_DS
    {
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return 0;
    }
#else
        return RDD_BASIC_SCHEDULER_TABLE_DS_SIZE;
#endif
    else
        return RDD_BASIC_SCHEDULER_TABLE_US_SIZE;
}

static uint32_t _rdd_basic_scheduler_cfg_bb_tx_id_get(rdpa_traffic_dir dir, uint8_t index, rdpa_port_type port_type)
{
#ifdef XRDP_BBH_PER_LAN_PORT
    if (dir == rdpa_dir_ds)
        return port_mapping_bb_tx_id_get(port_type, index);
#endif
    return index;
}

/* API to RDPA level */
bdmf_error_t rdd_basic_scheduler_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint8_t basic_scheduler_index, basic_scheduler_cfg_t *cfg, rdpa_port_type port_type)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t *basic_scheduler_table_ptr;
    uint32_t *bbh_queue_table_ptr;
    uint8_t runner_idx = port_mapping_get_tx_runner_core_by_channel_id(port_type, channel_id);
    uint8_t tm_index = port_mapping_get_tm_index_by_channel_id(port_type, channel_id);
    uint8_t bb_tx_id;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, cfg %px = { dwrr_offset = %d, "
        "bbh_q_desc_id  = %d,  hw_bbh_qid = %d }\n",
        dir, basic_scheduler_index, cfg, cfg->dwrr_offset, cfg->bbh_q_desc_id,
        cfg->hw_bbh_qid);

    if ((basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir)) ||
        (cfg->dwrr_offset >= basic_scheduler_num_of_dwrr_offset) ||
        (cfg->bbh_q_desc_id >= RDD_BBH_QUEUE_TABLE_SIZE))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
    {
#ifdef COMPLEX_SCHEDULER_IN_DS
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
#else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_DS_ADDRESS_ARR;
        bbh_queue_table_ptr = RDD_DS_TM_BBH_QUEUE_TABLE_ADDRESS_ARR;
#endif
    }
    else
    {
        tm_index = 0; /* single tm_core */
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_US_ADDRESS_ARR;
        bbh_queue_table_ptr = RDD_US_TM_BBH_QUEUE_TABLE_ADDRESS_ARR;
    }

    /* initialize budget for all queues - relevent for the case no rate limiter was configured */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_WRITE_G(BASIC_SCHEDULER_FULL_BUDGET_VECTOR, basic_scheduler_table_ptr, basic_scheduler_index);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_WRITE_G(cfg->dwrr_offset, basic_scheduler_table_ptr, basic_scheduler_index);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE_G(1, basic_scheduler_table_ptr, basic_scheduler_index);
    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_WRITE_G(basic_scheduler_index, bbh_queue_table_ptr, cfg->bbh_q_desc_id);
    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_TYPE_WRITE_G(RDD_SCHED_TYPE_BASIC, bbh_queue_table_ptr, cfg->bbh_q_desc_id);
   
#if !defined(BCM_PON_XRDP)
    RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_WRITE_G(cfg->hw_bbh_qid, bbh_queue_table_ptr, cfg->bbh_q_desc_id);
#endif

    bb_tx_id = _rdd_basic_scheduler_cfg_bb_tx_id_get(dir, cfg->bbh_q_desc_id, port_type);
    write_bbh_queue_g(bb_tx_id, basic_scheduler_table_ptr, basic_scheduler_index, channel_id, runner_idx);

    basic_scheduler_to_bbh_queue[dir][basic_scheduler_index][tm_index] = cfg->bbh_q_desc_id;
    return rc;
}

bdmf_error_t rdd_basic_scheduler_dwrr_offset_cfg(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, uint8_t dwrr_offset)
{
    uint32_t *basic_scheduler_table_ptr;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, dwrr_offset = %d }\n",
        dir, basic_scheduler_index, dwrr_offset);

    if ((basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir)) ||
        (dwrr_offset >= basic_scheduler_num_of_dwrr_offset))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
#ifdef COMPLEX_SCHEDULER_IN_DS
{
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
}
#else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_DS_ADDRESS_ARR;
#endif
    else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_US_ADDRESS_ARR;

    /* initialize budget for all queues - relevent for the case no rate limiter was configured */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_WRITE_G(dwrr_offset, basic_scheduler_table_ptr, basic_scheduler_index);

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_basic_scheduler_queue_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint8_t basic_scheduler_index, 
    basic_scheduler_queue_t *queue, rdpa_port_type port_type)
{
    uint32_t *basic_scheduler_table_ptr;
    bdmf_error_t rc = BDMF_ERR_OK;
    uint16_t first_queue_index;
    rdd_scheduling_queue_descriptor_t queue_cfg = {};

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, queue %px = { qm_queue_index = %d, queue_scheduler_index = %d, "
        "quantum_number = %d }\n",
        dir, basic_scheduler_index, queue, queue->qm_queue_index, queue->queue_scheduler_index, queue->quantum_number);

    if ((basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir)) ||
        (queue->queue_scheduler_index >= BASIC_SCHEDULER_NUM_OF_QUEUES))
    {
        return BDMF_ERR_PARM;
    }
    if (dir == rdpa_dir_ds)
    {
#ifdef COMPLEX_SCHEDULER_IN_DS
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
#else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_DS_ADDRESS_ARR;
        first_queue_index = drv_qm_get_ds_start(queue->qm_queue_index);
        RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_WRITE(queue->quantum_number,
            (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(drv_qm_get_runner_idx(queue->qm_queue_index)) +
            ((queue->qm_queue_index - first_queue_index) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
#endif
    }
    else
    {
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_US_ADDRESS_ARR;
        first_queue_index = drv_qm_get_us_start();
        rc = rdd_ag_us_tm_scheduling_queue_table_quantum_number_set((queue->qm_queue_index - first_queue_index), queue->quantum_number);
    }

    /* mapping queue to basic scheduler */
    /* mapping basic scheduler to queue */

    RDD_BASIC_SCHEDULER_DESCRIPTOR_QUEUE_INDEX_WRITE_G((queue->qm_queue_index - first_queue_index), basic_scheduler_table_ptr, basic_scheduler_index, queue->queue_scheduler_index);
    queue_cfg.bbh_queue_index = basic_scheduler_to_bbh_queue[dir][basic_scheduler_index][0];

#ifdef G9991_COMMON
    if (channel_id == RDD_WAN1_VPORT)
        queue_cfg.bbh_queue_index = channel_id;
#endif
    queue_cfg.scheduler_index = basic_scheduler_index;
    queue_cfg.block_type = 0; /* basic scheduler */
    queue_cfg.queue_bit_mask = 1 << (queue->queue_scheduler_index);
    rc = rc ? rc : rdd_scheduling_scheduler_block_cfg(dir, channel_id, queue->qm_queue_index, &queue_cfg, 0, 0, 1, port_type);

    return rc;
}

bdmf_error_t rdd_basic_scheduler_queue_remove(rdpa_traffic_dir dir, int16_t channel_id, uint8_t basic_scheduler_index, 
    uint8_t queue_scheduler_index, rdpa_port_type port_type)
{
    BASIC_SCHEDULER_DESCRIPTOR_STRUCT *entry;
    rdd_scheduling_queue_descriptor_t queue_cfg = {};
    uint16_t queue_index;
    bdmf_error_t rc;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, queue_scheduler_index = %d\n",
        dir, basic_scheduler_index, queue_scheduler_index);

    if ((basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir)) ||
        (queue_scheduler_index >= BASIC_SCHEDULER_NUM_OF_QUEUES))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
#ifdef COMPLEX_SCHEDULER_IN_DS
{
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
}
#else
        entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + basic_scheduler_index;
#endif
    else
        entry = ((BASIC_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + basic_scheduler_index;

    /* write bit mask 0 to queue */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_QUEUE_INDEX_READ(queue_index, entry, queue_scheduler_index);
#ifndef COMPLEX_SCHEDULER_IN_DS
    if (dir == rdpa_dir_ds)

        queue_index += drv_qm_get_ds_start(0);
#endif
    rc = rdd_scheduling_scheduler_block_cfg(dir, channel_id, queue_index, &queue_cfg, 0, 0, 0, port_type);
    return rc;
}

/* API to complex scheduler module */
bdmf_error_t rdd_basic_scheduler_dwrr_cfg(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, uint8_t quantum_number, int16_t channel_id,
    rdpa_port_type port_type)
{
    uint32_t *basic_scheduler_table_ptr;

    if (basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
#ifdef COMPLEX_SCHEDULER_IN_DS
{
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
}
#else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_DS_ADDRESS_ARR;
#endif
    else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_US_ADDRESS_ARR;

    RDD_BASIC_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE_G(quantum_number, basic_scheduler_table_ptr, basic_scheduler_index);
    return BDMF_ERR_OK;
}

int sizeof_bitfield(void *word)
{
    uint32_t *my_word = word;
    int i, bits;

    while (*my_word == 0)
        my_word++;
    
    for (i = bits = 0; i < 32; i++)
    {
        if (*my_word & (1 << i))
            bits++;
    }

    return bits;
}

int sizeof_complex_scheduler_slot_bit_field(void)
{
    BASIC_SCHEDULER_DESCRIPTOR_STRUCT my_struct;

    memset(&my_struct, 0, sizeof(my_struct));
    my_struct.complex_scheduler_slot_index = -1;

    return sizeof_bitfield(&my_struct);
}

bdmf_error_t rdd_basic_scheduler_block_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint8_t basic_scheduler_index,
    rdd_scheduling_queue_descriptor_t *scheduler_cfg, uint8_t dwrr_offset, rdpa_port_type port_type)
{
    uint32_t *basic_scheduler_table_ptr;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, scheduler_cfg %px = { scheduler_index = %d, bit_mask = %d, "
        "bbh_queue = %d, scheduler_type = %d, dwrr_offset = %d }\n",
        dir, basic_scheduler_index, scheduler_cfg, scheduler_cfg->scheduler_index, scheduler_cfg->queue_bit_mask,
        scheduler_cfg->bbh_queue_index, scheduler_cfg->block_type, dwrr_offset);

    if ((basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir)) ||
        (dwrr_offset >= basic_scheduler_num_of_dwrr_offset) ||
        (scheduler_cfg->bbh_queue_index >= RDD_BBH_QUEUE_TABLE_SIZE) ||
        (scheduler_cfg->scheduler_index >= rdd_complex_scheduler_size(dir)) ||
        (scheduler_cfg->queue_bit_mask >= (1 << sizeof_complex_scheduler_slot_bit_field())))
    {
        return BDMF_ERR_PARM;
    }

    if (dir == rdpa_dir_ds)
#ifdef COMPLEX_SCHEDULER_IN_DS
    {
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
    }
#else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_DS_ADDRESS_ARR;
#endif
    else
        basic_scheduler_table_ptr = RDD_BASIC_SCHEDULER_TABLE_US_ADDRESS_ARR;

    /* initialize budget for all queues - relevent for the case no rate limiter was configured */
    /* BS support in DS already checked in this function and BDMF_ERR_PARM returned if flag COMPLEX_SCHEDULER_IN_DS set */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_WRITE_G(BASIC_SCHEDULER_FULL_BUDGET_VECTOR, basic_scheduler_table_ptr, basic_scheduler_index);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_DWRR_OFFSET_WRITE_G(dwrr_offset, basic_scheduler_table_ptr, basic_scheduler_index);

    /* mapping basic scheduler to complex scheduler */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_EXISTS_WRITE_G(scheduler_cfg->block_type, basic_scheduler_table_ptr, basic_scheduler_index);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_INDEX_WRITE_G(scheduler_cfg->scheduler_index, basic_scheduler_table_ptr, basic_scheduler_index);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_SLOT_INDEX_WRITE_G(scheduler_cfg->queue_bit_mask, basic_scheduler_table_ptr, basic_scheduler_index);
    basic_scheduler_to_bbh_queue[dir][basic_scheduler_index][0] = scheduler_cfg->bbh_queue_index;
    return BDMF_ERR_OK;
}

/* API to rate limiter module */
bdmf_error_t rdd_basic_scheduler_rate_limiter_cfg(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, int16_t rate_limiter_index, uint8_t tm_id)
{
    bdmf_boolean cs_exist;
    uint8_t *basic_scheduler_entry_p;
    int tm_core;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, rate_limiter_index = %d\n",
        dir, basic_scheduler_index, rate_limiter_index);

    if ((basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir)) ||
        (rate_limiter_index >= rdd_basic_rate_limiter_size_get(dir)))
    {
        return BDMF_ERR_PARM;
    }

    tm_core = drv_qm_get_tm_core(tm_id, dir);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Invalid tm_core %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (dir == rdpa_dir_ds)
    {
#ifdef COMPLEX_SCHEDULER_IN_DS
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
#else
        basic_scheduler_entry_p = (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(tm_core);
#endif
    }
    else
    {
        basic_scheduler_entry_p = (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(tm_core);
    }
    basic_scheduler_entry_p += basic_scheduler_index * sizeof(BASIC_SCHEDULER_DESCRIPTOR_STRUCT);

    RDD_BASIC_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(rate_limiter_index, basic_scheduler_entry_p);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(1, basic_scheduler_entry_p);
    RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_EXISTS_READ(cs_exist, basic_scheduler_entry_p);

    /* this "is_positive_budget" bit should always be 1 at this point, because
     * when a basic scheduler is allocated, by default we set it to 1. */
    if (cs_exist)
        RDD_BASIC_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(0, basic_scheduler_entry_p);
    else
        RDD_BASIC_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, basic_scheduler_entry_p);
    
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_basic_scheduler_rate_limiter_remove(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, uint8_t tm_id)
{
    bdmf_boolean cs_exist;
    uint8_t *basic_scheduler_entry_p;
    uint8_t cs_slot_index, cs_index;
    uint32_t budget_vector;
    bdmf_error_t rc = BDMF_ERR_OK;
    COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *entry = NULL;
    int tm_core;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d\n", dir, basic_scheduler_index);

    if (basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir))
    {
        return BDMF_ERR_PARM;
    }

    tm_core = drv_qm_get_tm_core(tm_id, dir);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Invalid tm_core %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (dir == rdpa_dir_us)
    {
        basic_scheduler_entry_p = (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(tm_core);
    }
    else
    {
#ifdef COMPLEX_SCHEDULER_IN_DS
{
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_NOT_SUPPORTED;
}
#else
        basic_scheduler_entry_p = (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(tm_core);
#endif
    }
    basic_scheduler_entry_p += basic_scheduler_index * sizeof(BASIC_SCHEDULER_DESCRIPTOR_STRUCT);

    RDD_BASIC_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(0, basic_scheduler_entry_p);
    /* when rate limiter is disabled, in both scheduler scheme cases
     * (basic-only or basic-under-complex), this bit should always be set to 1. */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, basic_scheduler_entry_p);

    /* in case the bs is under cs make sure it has rate */
    RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_EXISTS_READ(cs_exist, basic_scheduler_entry_p);

    if (cs_exist)
    {
        RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_SLOT_INDEX_READ(cs_slot_index, basic_scheduler_entry_p);
        RDD_BASIC_SCHEDULER_DESCRIPTOR_COMPLEX_SCHEDULER_INDEX_READ(cs_index, basic_scheduler_entry_p);
        if (dir == rdpa_dir_us)
            entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_US_PTR(get_runner_idx(us_tm_runner_image))) + cs_index;
#ifdef COMPLEX_SCHED_ON_BOTH_TM
        else
            entry = ((COMPLEX_SCHEDULER_DESCRIPTOR_STRUCT *)RDD_COMPLEX_SCHEDULER_TABLE_DS_PTR(get_runner_idx(ds_tm_runner_image))) + cs_index;
#endif
        if (entry != NULL)
        {
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_READ(budget_vector, entry);
            budget_vector |= (1 << cs_slot_index);
            RDD_COMPLEX_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(budget_vector, entry);
            RDD_BTRACE("dir = %d, cs_slot_index = %d, budget_vector = %d, cs_index = %d\n",
                   dir, cs_slot_index, budget_vector, cs_index);
        }
    }

    return rc;
}

/* API to queue */
bdmf_error_t rdd_basic_scheduler_rate_set(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, uint8_t queue_bit_mask, uint8_t tm_id)
{
    uint8_t budget;
    int tm_core;

    RDD_BTRACE("dir = %d, basic_scheduler_index = %d, queue_bit_mask = %d\n",
        dir, basic_scheduler_index, queue_bit_mask);

    if (basic_scheduler_index >= rdd_basic_scheduler_table_size_get(dir))
    {
        return BDMF_ERR_PARM;
    }

    tm_core = drv_qm_get_tm_core(tm_id, dir);

    if (tm_core < 0)
    {
        BDMF_TRACE_ERR("Invalid tm_core %d\n", tm_core);
        return BDMF_ERR_INTERNAL;
    }

    if (dir == rdpa_dir_ds)
    {
#ifdef COMPLEX_SCHEDULER_IN_DS
        BDMF_TRACE_ERR("Basic scheduler not supported in DS\n");
        return BDMF_ERR_PARM;
#else
        RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_READ(
            budget, (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(tm_core) + (basic_scheduler_index)*sizeof(BASIC_SCHEDULER_DESCRIPTOR_STRUCT));
        budget |= queue_bit_mask;
        RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_WRITE(
            budget, (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_DS_PTR(tm_core) + (basic_scheduler_index)*sizeof(BASIC_SCHEDULER_DESCRIPTOR_STRUCT));
#endif
    }
    else
    {
        RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_READ(
            budget, (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(tm_core) + (basic_scheduler_index)*sizeof(BASIC_SCHEDULER_DESCRIPTOR_STRUCT));
        budget |= queue_bit_mask;
        RDD_BASIC_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_WRITE(
            budget, (uint8_t *)RDD_BASIC_SCHEDULER_TABLE_US_PTR(tm_core) + (basic_scheduler_index)*sizeof(BASIC_SCHEDULER_DESCRIPTOR_STRUCT));
    }

    return BDMF_ERR_OK;
}

