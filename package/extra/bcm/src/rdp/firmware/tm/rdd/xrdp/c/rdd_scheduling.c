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

#define BURST_SIZE_MULTIPLIER       100
#define RDD_RATE_UNLIMITED          0x260000000
#define BITS_IN_BYTE                8
#define BITS_IN_WORD                32
#define BUDGET_NOT_VALID            63
#define RATE_LIMIT_CLOSE_TIMEOUT    1 /* 1 ms, there is error in function doing ms, i want in final 1 ms*/
#define EMPTY_QUANTOM_NUMBER 50
#define EMPTY_DEFICIT_COUNTER 100

/* taken from fw_defs.h, to be used by fw to wake up task async*/
#define THREAD_ASYNC_WAKEUP_REQUEST(x) (((x) << 4) + 9) 

typedef struct
{
    bdmf_boolean rl_en;
    uint8_t rl_index;
    uint32_t rl_rate;
} rdd_tm_rl_info;

rdd_tm_entity_info schedulers_info[TM_NUM_OF_IDENTITY];

#define SCHEDULER_MAX_NUM_OF_QUEUES   32

uint32_t exponent_list[] = {0, 3, 6, 9};

#ifdef G9991_FC
    rdpa_emac vport_to_emac[RDD_LAN_VPORT_LAST] = {};
#endif

/* It is impossible to reset counters safely. Use base value instead. Used for codel/flush drop */
static PACKETS_AND_BYTES_STRUCT tx_queue_drop_counters[MAX_TX_QUEUES__NUM_OF];

extern rdd_bb_id rdpa_emac_to_bb_id_rx[rdpa_emac__num_of];
extern rdd_bb_id rdpa_emac_to_bb_id_tx[rdpa_emac__num_of];
extern rdd_bb_id rdpa_emac_to_bb_id_tx[rdpa_emac__num_of];
extern bdmf_error_t rdd_scheduling_scheduler_block_cfg(tm_identifier_e tm_identity, int16_t channel_id, uint16_t qm_queue_index, 
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *scheduler_cfg, bdmf_boolean type, uint8_t dwrr_offset, bdmf_boolean enable);

extern uint16_t get_num_sched_ids(void);
extern bdmf_error_t sched_id_in_use(sched_id_t sched_id);
extern rdpa_rdd_sched_type_t get_scheduler_type(sched_id_t sched_id);
extern uint8_t get_scheduler_index(sched_id_t sched_id);
extern int _rdpa_rate_limit_set_budget_id(bdmf_object_handle _obj_, int16_t budget_id, uint8_t sir_pir);

/* mappping between scheduler to bbh queue */
static uint8_t scheduler_to_bbh_queue[TM_NUM_OF_IDENTITY][RDD_SCHEDULER_TABLE_MAX_SIZE];


static bdmf_error_t scheduler_index_type_invalid(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type)
{
    if (((scheduler_type == RDD_SCHED_TYPE_PRIMARY) && (scheduler_index > schedulers_info[tm_identity].scheduler_table_size)) ||
       ((scheduler_type == RDD_SCHED_TYPE_SECONDARY) && (scheduler_index > schedulers_info[tm_identity].secondary_scheduler_table_size)))
        return BDMF_ERR_PARM;

    return BDMF_ERR_OK;
}

static uint16_t get_scheduler_size(rdpa_rdd_sched_type_t scheduler_type)
{
    if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
        return sizeof(SCHEDULER_DESCRIPTOR_STRUCT);
    else
        return sizeof(SECONDARY_SCHEDULER_DESCRIPTOR_STRUCT);
}

uint8_t *get_scheduler_address(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type)
{
    uint8_t *table_base;
    uint16_t table_size;

    if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
        table_base = (uint8_t *)schedulers_info[tm_identity].scheduler_table_p;
    else /* RDD_SCHED_TYPE_SECONDARY */
        table_base = (uint8_t *)schedulers_info[tm_identity].secondary_scheduler_table_p;

    table_size = get_scheduler_size(scheduler_type);

    return table_base + (scheduler_index * table_size);
}

uint32_t rdd_primary_scheduler_descriptor_get_secondary_scheduler_vector(tm_identifier_e tm_identity, uint8_t scheduler_index)
{
    uint32_t secondary_scheduler_vector;
    uint8_t *scheduler_address;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_PRIMARY);

    RDD_SCHEDULER_DESCRIPTOR_SECONDARY_SCHEDULER_VECTOR_READ(secondary_scheduler_vector, scheduler_address);

    return secondary_scheduler_vector;
}

void rdd_primary_scheduler_descriptor_set_secondary_scheduler_vector(tm_identifier_e tm_identity, uint8_t scheduler_index, uint32_t secondary_scheduler_vector)
{
    uint8_t *scheduler_address;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_PRIMARY);

    RDD_SCHEDULER_DESCRIPTOR_SECONDARY_SCHEDULER_VECTOR_WRITE(secondary_scheduler_vector, scheduler_address);
}

uint32_t rdd_primary_scheduler_descriptor_get_slot_budget_bit_vector_1(tm_identifier_e tm_identity, uint8_t scheduler_index)
{
    uint32_t slot_budget_bit_vector_1;
    uint8_t *scheduler_address;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_PRIMARY);

    RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_READ(slot_budget_bit_vector_1, scheduler_address);

    return slot_budget_bit_vector_1;
}

void rdd_common_scheduler_descriptor_set_sir_dwrr_offset(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type, uint8_t sir_dwrr_offset)
{
    uint8_t *scheduler_address;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

    RDD_SCHEDULER_DESCRIPTOR_SIR_DWRR_OFFSET_WRITE(sir_dwrr_offset, scheduler_address);
}

void rdd_common_scheduler_descriptor_set_pir_dwrr_offset(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type, uint8_t pir_dwrr_offset)
{
    uint8_t *scheduler_address;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

    RDD_SCHEDULER_DESCRIPTOR_PIR_DWRR_OFFSET_WRITE(pir_dwrr_offset, scheduler_address);
}

void rdd_common_scheduler_descriptor_set_dwrr_offsets(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type,
                                                      rdpa_tm_rl_rate_mode rl_rate_mode, uint8_t dwrr_offset)
{
    if (rl_rate_mode == rdpa_tm_rl_single_rate)
    {
        /* Configure only SIR since in single rate mode there's no meaning for the pir offset, it isn't used */
        rdd_common_scheduler_descriptor_set_sir_dwrr_offset(tm_identity, scheduler_index, scheduler_type, dwrr_offset);
    }
    else /* rdpa_tm_rl_dual_rate */
    {
        /* It is assumed that in dual rate the SIR is always serviced before the PIR. Force it to SP.
         * The user controls only the PIR.
         */
        rdd_common_scheduler_descriptor_set_sir_dwrr_offset(tm_identity, scheduler_index, scheduler_type, scheduler_full_sp);
        rdd_common_scheduler_descriptor_set_pir_dwrr_offset(tm_identity, scheduler_index, scheduler_type, dwrr_offset);
    }
}

uint16_t rdd_common_scheduler_descriptor_get_queue_offset(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type)
{
    uint8_t *scheduler_address;
    uint16_t q_offset;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

    RDD_SCHEDULER_DESCRIPTOR_QUEUE_OFFSET_READ(q_offset, scheduler_address);

    return q_offset;
}

void rdd_secondary_scheduler_descriptor_set_quatnum_number(tm_identifier_e tm_identity, uint8_t scheduler_index, uint8_t quatnum_number)
{
    uint8_t *scheduler_address;

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_SECONDARY);

    RDD_SECONDARY_SCHEDULER_DESCRIPTOR_QUANTUM_NUMBER_WRITE(quatnum_number, scheduler_address);
}

static void rdd_scheduler_reset(uint8_t *scheduler_address, rdpa_rdd_sched_type_t scheduler_type)
{
    uint16_t scheduler_size = get_scheduler_size(scheduler_type);

    MEMSET_8(scheduler_address, 0x00, scheduler_size);
}

/* API to RDPA level */
bdmf_error_t rdd_scheduler_init(tm_identifier_e tm_identity, int16_t channel_id, uint8_t scheduler_index, 
    rdpa_rdd_sched_type_t scheduler_type, scheduler_cfg_t *cfg, rdpa_port_type port_type)
{
    uint8_t *scheduler_address;

    if (scheduler_index_type_invalid(tm_identity, scheduler_index, scheduler_type) ||
        (cfg->dwrr_offset >= scheduler_num_of_dwrr_offset) ||
        ((scheduler_type == RDD_SCHED_TYPE_PRIMARY) && (cfg->bbh_queue_index >= RDD_BBH_QUEUE_TABLE_SIZE) && (tm_identity != TM_ETH_SQ)))
    {
        BDMF_TRACE_ERR("cant init scheduler with scheduler_index=%d ,scheduler_type=%d, dwrr_offset=%d, bbh_queue_index=%d\n",
                       scheduler_index, scheduler_type, cfg->dwrr_offset, cfg->bbh_queue_index);
        return BDMF_ERR_PARM;
    }

    RDD_BTRACE("tm_identity = %d, scheduler_index = %d, scheduler_type = %d, cfg %px = { dwrr_offset = %d, bbh_queue_index = %d,  hw_bbh_qid = %d }\n",
        tm_identity, scheduler_index, scheduler_type, cfg, cfg->dwrr_offset, cfg->bbh_queue_index, cfg->hw_bbh_qid);

    /* It is a good practice to clean up before configuring */
    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);
    rdd_scheduler_reset(scheduler_address, scheduler_type);

    /* Common scheduler configurations */
    RDD_COMMON_SCHEDULER_DESCRIPTOR_QUEUE_OFFSET_WRITE(cfg->first_q_index, scheduler_address);
    RDD_COMMON_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, scheduler_address);

    rdd_common_scheduler_descriptor_set_dwrr_offsets(tm_identity, scheduler_index, scheduler_type, cfg->rl_rate_mode, cfg->dwrr_offset);

    /* Specific scheduler configurations */
    if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
    {
        RDD_SCHEDULER_DESCRIPTOR_BBH_QUEUE_DESC_ID_WRITE(cfg->bbh_queue_index, scheduler_address);
        RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(0xFFFFFFFF, scheduler_address);
        RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_WRITE(0, scheduler_address);
        RDD_SCHEDULER_DESCRIPTOR_AQM_STATS_ENABLE_WRITE(0, scheduler_address);
        rdd_primary_scheduler_descriptor_set_secondary_scheduler_vector(tm_identity, scheduler_index, 0);
    }
    else
    {
        RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PRIMARY_SCHEDULER_INDEX_WRITE(cfg->primary_scheduler_index, scheduler_address);
        RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PRIMARY_SCHEDULER_SLOT_INDEX_WRITE(cfg->primary_scheduler_slot_index, scheduler_address);
    }

    /* Secondary schedulers are not associated with BBH TX since they're subsidiary schedulers */
    if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
    {
        if (schedulers_info[tm_identity].bbh_bound)
        {
            BBH_QUEUE_DESCRIPTOR_STRUCT bbh_queue_descriptor;
            rdd_bbh_queue_descriptor_get(tm_identity, cfg->bbh_queue_index, &bbh_queue_descriptor);
            bbh_queue_descriptor.scheduler_index = scheduler_index;
            rdd_bbh_queue_descriptor_set(tm_identity, cfg->bbh_queue_index, &bbh_queue_descriptor);

            /* init bbh-queue */
#ifndef G9991_FC
            scheduler_to_bbh_queue[tm_identity][scheduler_index] = cfg->bbh_queue_index;
#else
            scheduler_to_bbh_queue[tm_identity][scheduler_index] = channel_id;
#endif
        }
    }

    return BDMF_ERR_OK;
}

void queue_pool_write(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type, uint8_t entry, uint8_t val)
{
    BYTE_1_STRUCT *queue_pool_entry;
    uint8_t *scheduler_address;
    uint16_t q_offset; 

    scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);
    RDD_COMMON_SCHEDULER_DESCRIPTOR_QUEUE_OFFSET_READ(q_offset, scheduler_address);
    queue_pool_entry = (BYTE_1_STRUCT *)schedulers_info[tm_identity].queue_pool_p;
    RDD_BYTE_1_BITS_WRITE(val, queue_pool_entry + q_offset + entry);
}

bdmf_error_t rdd_scheduler_queue_cfg(tm_identifier_e tm_identity, int16_t channel_id, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type, scheduler_queue_t *queue)
{
     bdmf_error_t rc;
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT queue_cfg = {};
    uint8_t entry;
    uint8_t val;

    RDD_BTRACE("tm_identity = %d, scheduler_index = %d, scheduler_type = %d, queue %px = { qm_queue_index = %d, queue_scheduler_index = %d, quantum_number = %d }\n",
               tm_identity, scheduler_index, scheduler_type, queue, queue->qm_queue_index, queue->queue_scheduler_index, queue->quantum_number);

    if (scheduler_index_type_invalid(tm_identity, scheduler_index, scheduler_type) ||
        (queue->queue_scheduler_index >= SCHEDULER_MAX_NUM_OF_QUEUES))
        return BDMF_ERR_PARM;

    if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
    {
        queue_cfg.bbh_queue_index = scheduler_to_bbh_queue[tm_identity][scheduler_index];
    }

    queue_cfg.quantum_number = queue->quantum_number;

    /* mapping scheduler to queue */
    queue_cfg.scheduler_index = scheduler_index;
    queue_cfg.scheduler_type = (scheduler_type == RDD_SCHED_TYPE_PRIMARY) ? SCHEDULER_TYPE_PRIMARY : SCHEDULER_TYPE_SECONDARY;
    queue_cfg.queue_index = queue->queue_scheduler_index;
    rc = rdd_scheduling_scheduler_block_cfg(tm_identity, channel_id, queue->qm_queue_index, &queue_cfg, 0, 0, 1);

    /* update queue pool so scheduler will be able to locate the queue */
    if (!rc)
    {
        val = queue->qm_queue_index - schedulers_info[tm_identity].qm_queue_start;
        entry = queue->queue_scheduler_index;
        queue_pool_write(tm_identity, scheduler_index, scheduler_type, entry, val);
    }

    return rc;
}

bdmf_error_t rdd_scheduler_queue_remove(tm_identifier_e tm_identity, int16_t channel_id, uint8_t scheduler_index, 
    rdpa_rdd_sched_type_t scheduler_type, uint8_t queue_scheduler_index, rdpa_port_type port_type)
{
    BYTE_1_STRUCT *queue_pool_entry;
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT queue_cfg = {};
    uint16_t queue_index, q_offset = 0;
    bdmf_error_t rc;

    RDD_BTRACE("tm_identity = %d, scheduler_index = %d, scheduler_type = %d, queue_scheduler_index = %d\n",
        tm_identity, scheduler_index, scheduler_type, queue_scheduler_index);

    if ((scheduler_index_type_invalid(tm_identity, scheduler_index, scheduler_type)) || (queue_scheduler_index >= SCHEDULER_MAX_NUM_OF_QUEUES))
        return BDMF_ERR_PARM;

    queue_pool_entry = (BYTE_1_STRUCT *)schedulers_info[tm_identity].scheduler_pool_p;

    /* write bit mask 0 to queue */

    q_offset = rdd_common_scheduler_descriptor_get_queue_offset(tm_identity, scheduler_index, scheduler_type);
    RDD_BYTE_1_BITS_READ(queue_index, (queue_pool_entry + (queue_scheduler_index + q_offset)));
    queue_index += schedulers_info[tm_identity].qm_queue_start;

    /* in delete we may choose this queue 1 time and get stuck on it in loop !!!, 0 is risky, any numbers != from 0 are ok*/
    queue_cfg.quantum_number = EMPTY_QUANTOM_NUMBER;
    queue_cfg.deficit_counter = EMPTY_DEFICIT_COUNTER;
    rc = rdd_scheduling_scheduler_block_cfg(tm_identity, channel_id, queue_index, &queue_cfg, 0, 0, 0);
    return rc;
}

bdmf_error_t rdd_rate_limiter_params_descriptor_set(tm_identifier_e tm_identity, uint8_t rl_index, RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT *rl_params_descriptor)
{
    uint8_t *RATE_LIMITER_PARAMS_TABLE_P;
    uint32_t table_offset = (rl_index) * sizeof(RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT);
    if (rl_index >= schedulers_info[tm_identity].rl_params_table_size)
    {
        return BDMF_ERR_PARM;
    }
    RATE_LIMITER_PARAMS_TABLE_P = schedulers_info[tm_identity].rl_params_table_p;
 
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_RATE_PROFILE_ID_WRITE(rl_params_descriptor->rate_profile_id, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_STATUS_UPDATE_OFFSET_WORD_WRITE(rl_params_descriptor->status_update_offset_word, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_STATUS_UPDATE_OFFSET_BIT_WRITE(rl_params_descriptor->status_update_offset_bit, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_BUDGET_DESC_ID_WRITE(rl_params_descriptor->budget_desc_id, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_WAKEUP_MODE_WRITE(rl_params_descriptor->wakeup_mode, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_WAKEUP_MESSAGE_DATA_WRITE(rl_params_descriptor->wakeup_message_data, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_rate_limiter_params_descriptor_get(tm_identifier_e tm_identity, uint8_t rl_index, RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT *rl_params_descriptor)
{
    uint8_t *RATE_LIMITER_PARAMS_TABLE_P;
    uint32_t table_offset = (rl_index) * sizeof(RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT);
    if (rl_index >= schedulers_info[tm_identity].rl_params_table_size)
    {
        return BDMF_ERR_PARM;
    }
    RATE_LIMITER_PARAMS_TABLE_P = schedulers_info[tm_identity].rl_params_table_p;

    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_RATE_PROFILE_ID_READ(rl_params_descriptor->rate_profile_id, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_STATUS_UPDATE_OFFSET_WORD_READ(rl_params_descriptor->status_update_offset_word, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_STATUS_UPDATE_OFFSET_BIT_READ(rl_params_descriptor->status_update_offset_bit, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_BUDGET_DESC_ID_READ(rl_params_descriptor->budget_desc_id, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_WAKEUP_MODE_READ(rl_params_descriptor->wakeup_mode, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    RDD_RATE_LIMITER_PARAMS_DESCRIPTOR_WAKEUP_MESSAGE_DATA_READ(rl_params_descriptor->wakeup_message_data, RATE_LIMITER_PARAMS_TABLE_P + table_offset);
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_rate_limiter_budget_descriptor_get(tm_identifier_e tm_identity, uint8_t budget_index, RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT *budget_descriptor)
{
    uint8_t *RATE_LIMITER_BUDGET_DESC_P;
    uint32_t table_offset = (budget_index)*sizeof(RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT);
    if (budget_index >= schedulers_info[tm_identity].rl_budget_desc_table_size)
    {
        return BDMF_ERR_PARM;
    }
    RATE_LIMITER_BUDGET_DESC_P = schedulers_info[tm_identity].rl_budget_desc_table_p;
   
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_CURRENT_BUDGET_READ(budget_descriptor->current_budget, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_0_READ(budget_descriptor->rl_params_desc_0, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_1_READ(budget_descriptor->rl_params_desc_1, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_2_READ(budget_descriptor->rl_params_desc_2, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_3_READ(budget_descriptor->rl_params_desc_3, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_rate_limiter_budget_descriptor_set(tm_identifier_e tm_identity, uint8_t budget_index, RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT *budget_descriptor)
{
    uint8_t *RATE_LIMITER_BUDGET_DESC_P;
    uint32_t table_offset = (budget_index)*sizeof(RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT);
    if (budget_index >= schedulers_info[tm_identity].rl_budget_desc_table_size)
    {
        return BDMF_ERR_PARM;
    }

    RATE_LIMITER_BUDGET_DESC_P = schedulers_info[tm_identity].rl_budget_desc_table_p;

    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_CURRENT_BUDGET_WRITE(budget_descriptor->current_budget, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_0_WRITE(budget_descriptor->rl_params_desc_0, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_1_WRITE(budget_descriptor->rl_params_desc_1, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_2_WRITE(budget_descriptor->rl_params_desc_2, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    RDD_RATE_LIMITER_BUDGET_DESCRIPTOR_RL_PARAMS_DESC_3_WRITE(budget_descriptor->rl_params_desc_3, RATE_LIMITER_BUDGET_DESC_P + table_offset);
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_bbh_queue_descriptor_set(tm_identifier_e tm_identity, uint8_t bbh_q_index, BBH_QUEUE_DESCRIPTOR_STRUCT *bbh_queue_descriptor)
{
    uint8_t *BBH_QUEUE_DESC_TABLE_P;
    uint32_t table_offset = (bbh_q_index)*sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT);
    BBH_QUEUE_DESC_TABLE_P = schedulers_info[tm_identity].bbh_queue_descriptor_table_p;

    if (schedulers_info[tm_identity].bbh_bound == 0)
        return BDMF_ERR_OK;

    if (bbh_q_index >= schedulers_info[tm_identity].bbh_queue_descriptor_table_size)  
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", bbh_q_index, schedulers_info[tm_identity].bbh_queue_descriptor_table_size);
    }

    RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_WRITE(bbh_queue_descriptor->hw_bbh_qid, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_MIRRORING_EN_WRITE(bbh_queue_descriptor->mirroring_en, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_WRITE(bbh_queue_descriptor->scheduler_index, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(bbh_queue_descriptor->bb_destination, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_WRITE(bbh_queue_descriptor->ingress_counter, BBH_QUEUE_DESC_TABLE_P + table_offset);
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_bbh_queue_descriptor_get(tm_identifier_e tm_identity, uint8_t bbh_q_index, BBH_QUEUE_DESCRIPTOR_STRUCT *bbh_queue_descriptor)
{
    uint8_t *BBH_QUEUE_DESC_TABLE_P;
    uint32_t table_offset = (bbh_q_index)*sizeof(BBH_QUEUE_DESCRIPTOR_STRUCT);
    BBH_QUEUE_DESC_TABLE_P = schedulers_info[tm_identity].bbh_queue_descriptor_table_p;

    if (schedulers_info[tm_identity].bbh_bound == 0)
        return BDMF_ERR_OK;

    if (bbh_q_index >= schedulers_info[tm_identity].bbh_queue_descriptor_table_size)
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", bbh_q_index, schedulers_info[tm_identity].bbh_queue_descriptor_table_size);
    }

    RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_READ(bbh_queue_descriptor->hw_bbh_qid, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_MIRRORING_EN_READ(bbh_queue_descriptor->mirroring_en, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_READ(bbh_queue_descriptor->scheduler_index, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_READ(bbh_queue_descriptor->bb_destination, BBH_QUEUE_DESC_TABLE_P + table_offset);
    RDD_BBH_QUEUE_DESCRIPTOR_INGRESS_COUNTER_READ(bbh_queue_descriptor->ingress_counter, BBH_QUEUE_DESC_TABLE_P + table_offset);
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_rate_limiter_budget_valid_set(tm_identifier_e tm_identity, uint8_t budget_index, uint32_t enable)
{
    uint8_t *RATE_LIMIT_BUDGET_VALID_TABLE_P;
    uint32_t reg_val;
    if (budget_index >= schedulers_info[tm_identity].rl_budget_valid_max)
    {
        return BDMF_ERR_PARM;
    }
    RATE_LIMIT_BUDGET_VALID_TABLE_P = schedulers_info[tm_identity].rl_budget_valid_table_p;

    RDD_BYTES_4_BITS_READ(reg_val, RATE_LIMIT_BUDGET_VALID_TABLE_P + (budget_index / 32)*sizeof(uint32_t));
    reg_val = (reg_val & (~(1 << (budget_index % 32)))) | (enable << (budget_index % 32));
    RDD_BYTES_4_BITS_WRITE(reg_val, RATE_LIMIT_BUDGET_VALID_TABLE_P + (budget_index / 32)*sizeof(uint32_t));
    return BDMF_ERR_OK;
}

#if defined(EPON)
bdmf_error_t rdd_tm_epon_cfg(void)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    uint32_t llid;
    BBH_QUEUE_DESCRIPTOR_STRUCT *entry;

    RDD_BTRACE("\n");
    for (llid = 0; (llid < MAX_NUM_OF_LLID); ++llid)
    {
        entry = ((BBH_QUEUE_DESCRIPTOR_STRUCT *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(get_runner_idx(us_tm_runner_image))) + llid;
        RDD_BBH_QUEUE_DESCRIPTOR_BB_DESTINATION_WRITE(BB_ID_TX_PON_ETH_STAT, entry);
        RDD_BBH_QUEUE_DESCRIPTOR_HW_BBH_QID_WRITE_G(llid, RDD_US_TM_BBH_QUEUE_TABLE_ADDRESS_ARR, llid);
    }
    return rc;
}
#endif

void rdd_general_timer_set(uint16_t core_index, uint16_t func_pointer, uint32_t timer_period, uint32_t timer_action)
{
    uint16_t action_vector;
    uint32_t actual_timer_period;

    GENERAL_TIMER_ENTRY_STRUCT *timer_addr;
    timer_addr = ((GENERAL_TIMER_ENTRY_STRUCT *)(RDD_GENERAL_TIMER_PTR(core_index)) + timer_action);

    RDD_BYTES_2_BITS_READ(action_vector, RDD_GENERAL_TIMER_ACTION_VEC_PTR(core_index));
    action_vector =  action_vector | (1 << timer_action);
    RDD_BYTES_2_BITS_WRITE(action_vector, RDD_GENERAL_TIMER_ACTION_VEC_PTR(core_index));

    actual_timer_period = timer_period / GENERAL_TIMER_PERIODICITY;
    RDD_GENERAL_TIMER_ENTRY_TIMEOUT_WRITE(actual_timer_period, timer_addr);
    /* TODO - when have ability to get function address */
    /*RDD_GENERAL_TIMER_ENTRY_FUNC_PTR_WRITE(func_pointer, timer_addr);
    RDD_GENERAL_TIMER_ENTRY_ENABLE_WRITE(1, timer_addr);*/
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

bdmf_error_t rdd_scheduler_queue_set_rate_limits_fields(tm_identifier_e tm_identity, uint16_t queue_index, SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *queue_descriptor)
{
    uint8_t *SCHEDULING_QUEUE_TABLE_P;

    if (queue_index >= schedulers_info[tm_identity].queue_descriptor_table_size)
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", queue_index, schedulers_info[tm_identity].queue_descriptor_table_size);
    }

    SCHEDULING_QUEUE_TABLE_P = schedulers_info[tm_identity].queue_descriptor_table_p;
   
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SIR_RATE_LIMITER_INDEX_WRITE(queue_descriptor->sir_rate_limiter_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PIR_RATE_LIMITER_INDEX_WRITE(queue_descriptor->pir_rate_limiter_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SIR_RATE_LIMIT_ENABLE_WRITE(queue_descriptor->sir_rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PIR_RATE_LIMIT_ENABLE_WRITE(queue_descriptor->pir_rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(queue_descriptor->rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_scheduler_queue_set(tm_identifier_e tm_identity, uint16_t queue_index, SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *queue_descriptor)
{
    uint8_t *SCHEDULING_QUEUE_TABLE_P;

    if (queue_index >= schedulers_info[tm_identity].queue_descriptor_table_size)
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", queue_index, schedulers_info[tm_identity].queue_descriptor_table_size);
    }

    SCHEDULING_QUEUE_TABLE_P = schedulers_info[tm_identity].queue_descriptor_table_p;
   
     /* enable is first by purpose, dont change it */
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_ENABLE_WRITE(queue_descriptor->enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SIR_RATE_LIMITER_INDEX_WRITE(queue_descriptor->sir_rate_limiter_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PIR_RATE_LIMITER_INDEX_WRITE(queue_descriptor->pir_rate_limiter_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SIR_RATE_LIMIT_ENABLE_WRITE(queue_descriptor->sir_rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PIR_RATE_LIMIT_ENABLE_WRITE(queue_descriptor->pir_rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(queue_descriptor->rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_BBH_QUEUE_INDEX_WRITE(queue_descriptor->bbh_queue_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_CODEL_ENABLE_WRITE(queue_descriptor->codel_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PI2_ENABLE_WRITE(queue_descriptor->pi2_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_AQM_ENABLE_WRITE(queue_descriptor->aqm_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_LAQM_ENABLE_WRITE(queue_descriptor->laqm_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_CODEL_DROPPING_WRITE(queue_descriptor->codel_dropping, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_WRITE(queue_descriptor->scheduler_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_TYPE_WRITE(queue_descriptor->scheduler_type, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUEUE_INDEX_WRITE(queue_descriptor->queue_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_WRITE(queue_descriptor->quantum_number, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_DEFICIT_COUNTER_WRITE(queue_descriptor->deficit_counter, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_scheduler_queue_wrr_set(tm_identifier_e tm_identity, uint16_t qm_queue_index, uint8_t quantum_number)
{
    uint8_t *SCHEDULING_QUEUE_TABLE_P;
    int queue_index;

    queue_index = qm_queue_index - schedulers_info[tm_identity].qm_queue_start;

    if (queue_index >= schedulers_info[tm_identity].queue_descriptor_table_size)
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", queue_index, schedulers_info[tm_identity].queue_descriptor_table_size);
    }

    SCHEDULING_QUEUE_TABLE_P = schedulers_info[tm_identity].queue_descriptor_table_p;
   
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_WRITE(quantum_number,
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));

    /* If the quantom number is updated, might as well reset the deficit counter and start afresh */
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_DEFICIT_COUNTER_WRITE(0,
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));

    return BDMF_ERR_OK;
}

void rdd_scheduler_aqm_stats_write(tm_identifier_e tm_identity, uint8_t scheduler_index, rdpa_rdd_sched_type_t scheduler_type, int enable)
{
    uint8_t *scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

    RDD_SCHEDULER_DESCRIPTOR_AQM_STATS_ENABLE_WRITE(!!enable, scheduler_address);
}

bdmf_error_t rdd_scheduler_queue_get(tm_identifier_e tm_identity, uint16_t queue_index, SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *queue_descriptor)
{
    uint8_t *SCHEDULING_QUEUE_TABLE_P;
    if (queue_index >= schedulers_info[tm_identity].queue_descriptor_table_size)
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", queue_index, schedulers_info[tm_identity].queue_descriptor_table_size);
    }

    SCHEDULING_QUEUE_TABLE_P = schedulers_info[tm_identity].queue_descriptor_table_p;

    RDD_SCHEDULING_QUEUE_DESCRIPTOR_CODEL_DROPPED_RECENTLY_READ(queue_descriptor->codel_dropped_recently, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_BBH_QUEUE_INDEX_READ(queue_descriptor->bbh_queue_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_ENABLE_READ(queue_descriptor->enable, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_RATE_LIMIT_ENABLE_READ(queue_descriptor->rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SIR_RATE_LIMIT_ENABLE_READ(queue_descriptor->sir_rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PIR_RATE_LIMIT_ENABLE_READ(queue_descriptor->pir_rate_limit_enable, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_CODEL_ENABLE_READ(queue_descriptor->codel_enable, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PI2_ENABLE_READ(queue_descriptor->pi2_enable, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_AQM_ENABLE_READ(queue_descriptor->aqm_enable, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_LAQM_ENABLE_READ(queue_descriptor->laqm_enable, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_CODEL_DROPPING_READ(queue_descriptor->codel_dropping, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_INDEX_READ(queue_descriptor->scheduler_index, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SCHEDULER_TYPE_READ(queue_descriptor->scheduler_type, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUEUE_INDEX_READ(queue_descriptor->queue_index, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_SIR_RATE_LIMITER_INDEX_READ(queue_descriptor->sir_rate_limiter_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_PIR_RATE_LIMITER_INDEX_READ(queue_descriptor->pir_rate_limiter_index, 
        SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_QUANTUM_NUMBER_READ(queue_descriptor->quantum_number, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_DEFICIT_COUNTER_READ(queue_descriptor->deficit_counter, SCHEDULING_QUEUE_TABLE_P + (queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_scheduling_scheduler_block_cfg(tm_identifier_e tm_identity, int16_t channel_id, uint16_t qm_queue_index,
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT *scheduler_cfg, bdmf_boolean type, uint8_t dwrr_offset, bdmf_boolean enable)
{
    int q_offset;
    RDD_BTRACE("tm_identity = %d, qm_queue_index = %d, type = %d, scheduler_cfg %px = { scheduler_index = %d, bit_mask = %x, "
        "bbh_queue_desc_id = %d}\n",
        tm_identity, qm_queue_index, type, scheduler_cfg,
        scheduler_cfg->scheduler_index, scheduler_cfg->queue_index, scheduler_cfg->bbh_queue_index);

    q_offset = qm_queue_index - schedulers_info[tm_identity].qm_queue_start;

    if (q_offset >= schedulers_info[tm_identity].queue_descriptor_table_size)
    {
        BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", q_offset, schedulers_info[tm_identity].queue_descriptor_table_size);
    }

#ifdef BCM6813
    if ((scheduler_cfg->bbh_queue_index >= RDD_US_CHANNEL_OFFSET_TCONT) &&
        (scheduler_cfg->bbh_queue_index <= RDD_US_CHANNEL_OFFSET_TCONT_END))
        rdd_ghost_reporting_mapping_queue_to_wan_channel(q_offset,
            scheduler_cfg->bbh_queue_index - RDD_US_CHANNEL_OFFSET_TCONT, enable);
#elif defined(EPON) || defined(CONFIG_BCM_TCONT)
    if (tm_identity == TM_PON_DSL_AE)
        rdd_ghost_reporting_mapping_queue_to_wan_channel(q_offset, scheduler_cfg->bbh_queue_index, enable);
#endif

    /* enable is first to written in function */

    rdd_scheduler_queue_set(tm_identity, q_offset, scheduler_cfg);
    
    if (enable)
    {
        scheduler_cfg->enable = 1;
        rdd_scheduler_queue_set(tm_identity, q_offset, scheduler_cfg);
    }
    return BDMF_ERR_OK;
}

/* Function that retrieves tx_queue drop counters (codel/flush) for case when VLAN counters are not compiled in
   and dedicated codel counters area is used instead.
*/
static bdmf_error_t _tx_queue_drop_count_by_relative_queue_index(uint16_t queue_index, uint32_t *p_packets, uint32_t *p_bytes)
{
#ifdef AQM_COUNTERS_IN_SRAM
    uint8_t rel_queue_index;

    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);
        RDD_PACKETS_AND_BYTES_PACKETS_READ_CORE(*p_packets, RDD_DS_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index, drv_qm_get_runner_idx(queue_index));
        RDD_PACKETS_AND_BYTES_BYTES_READ_CORE(*p_bytes, RDD_DS_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index, drv_qm_get_runner_idx(queue_index));
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_us_start();
        RDD_PACKETS_AND_BYTES_PACKETS_READ_CORE(*p_packets, RDD_US_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index, drv_qm_get_runner_idx(queue_index));
        RDD_PACKETS_AND_BYTES_BYTES_READ_CORE(*p_bytes, RDD_US_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index, drv_qm_get_runner_idx(queue_index));
    }
    else if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        RDD_PACKETS_AND_BYTES_PACKETS_READ_CORE(*p_packets, RDD_SQ_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index, drv_qm_get_runner_idx(queue_index));
        RDD_PACKETS_AND_BYTES_BYTES_READ_CORE(*p_bytes, RDD_SQ_TM_TX_QUEUE_DROP_TABLE_ADDRESS_ARR, rel_queue_index, drv_qm_get_runner_idx(queue_index));
    }
    else
#endif
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
#ifdef VLAN_COUNTER
    uint8_t vlan_stats_enable;
#endif

    if (queue_index >= MAX_TX_QUEUES__NUM_OF)
        return BDMF_ERR_RANGE;
   
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
        if (drv_qm_tx_queue_is_ds_queue(queue_index) || drv_qm_tx_queue_is_us_queue(queue_index))
        {
            packets = _tx_queue_drop_counter_cumulative_by_core(queue_index, 0, RDD_VLAN_TX_COUNTERS_ADDRESS_ARR);
            bytes = _tx_queue_drop_counter_cumulative_by_core(queue_index, 1, RDD_VLAN_TX_COUNTERS_ADDRESS_ARR);
        }
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

    return rc;
}

#define RATE_LIMITS_NUM_OF_PROFILES RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_SIZE
uint32_t profiles_ref_counter[RATE_LIMITS_NUM_OF_PROFILES];

void rdd_rate_limiter_profile_change_ref(int cur_profile_index, int ref_change)
{
    if ((cur_profile_index > 0) && (cur_profile_index < RATE_LIMITS_NUM_OF_PROFILES))
        profiles_ref_counter[cur_profile_index] += ref_change;
}

int rdd_rate_limiter_profile_set(rdd_rl_float_t alloc, rdd_rl_float_t limit, uint8_t alloc_residue)
{
    int cur_profile_index;
    rdd_rl_float_t current_alloc, current_limit;
    uint8_t current_alloc_residue;
    int empty_profile_idx = -1;

    BDMF_TRACE_DBG("req alloc=(%d %d) limit= (%d %d) residue=%d\n", alloc.exponent, alloc.mantissa, limit.exponent, limit.mantissa, alloc_residue);

    for (cur_profile_index = 0; cur_profile_index < RATE_LIMITS_NUM_OF_PROFILES; cur_profile_index++)
    {
        if (profiles_ref_counter[cur_profile_index] == 0)
            empty_profile_idx = cur_profile_index;
        else
        {
            RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_READ_G(current_limit.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, cur_profile_index);
            RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_READ_G(current_limit.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, cur_profile_index);
            RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_READ_G(current_alloc.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, cur_profile_index);
            RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_READ_G(current_alloc.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, cur_profile_index); 
            RDD_RATE_LIMITER_PROFILE_RESIDUE_ALLOC_RESIDUE_READ_G(current_alloc_residue, RDD_PON_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_ADDRESS_ARR, cur_profile_index);

            if ((current_alloc_residue == alloc_residue) && (alloc.exponent == current_alloc.exponent) && (limit.exponent == current_limit.exponent) && 
                     (alloc.mantissa == current_alloc.mantissa) && (limit.mantissa == current_limit.mantissa))
            {
                profiles_ref_counter[cur_profile_index]++;
                return cur_profile_index;
            }
        }
    }

    if (empty_profile_idx != -1)
    {
        RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(limit.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(limit.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(alloc.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(alloc.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx); 
        RDD_RATE_LIMITER_PROFILE_RESIDUE_ALLOC_RESIDUE_WRITE_G(alloc_residue, RDD_PON_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_ADDRESS_ARR, empty_profile_idx);

        RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(limit.exponent, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(limit.mantissa, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(alloc.exponent, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(alloc.mantissa, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx); 
        RDD_RATE_LIMITER_PROFILE_RESIDUE_ALLOC_RESIDUE_WRITE_G(alloc_residue, RDD_ETH_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_ADDRESS_ARR, empty_profile_idx);
        
        RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(limit.exponent, RDD_SQ_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(limit.mantissa, RDD_SQ_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(alloc.exponent, RDD_SQ_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx);
        RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(alloc.mantissa, RDD_SQ_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, empty_profile_idx); 
        RDD_RATE_LIMITER_PROFILE_RESIDUE_ALLOC_RESIDUE_WRITE_G(alloc_residue, RDD_SQ_TM_RATE_LIMITER_PROFILE_RESIDUE_TABLE_ADDRESS_ARR, empty_profile_idx);
        profiles_ref_counter[empty_profile_idx]++;
    }

    return empty_profile_idx;
}

void _rdd_rate_limit_scheduler_status_update_set(RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT *rl_params_descriptor,
                                                 rdpa_tm_rl_rate_mode rl_rate_mode, void *p_scheduler_address, int bit)
{
    uint32_t u_sched_address = (uint32_t)(uintptr_t)p_scheduler_address;
    uint64_t word_offset;

    rl_params_descriptor->status_update_offset_word = (u_sched_address / 4);

    if (rl_rate_mode == rdpa_tm_rl_single_rate)
    {
        word_offset =  (int)offsetof(SCHEDULER_DESCRIPTOR_STRUCT, slot_budget_bit_vector_0) / 4;
        rl_params_descriptor->status_update_offset_word += (int)word_offset /* SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WORD_OFFSET */;
    }
    else /* rdpa_tm_rl_dual_rate */
    {
        word_offset =  (int)offsetof(SCHEDULER_DESCRIPTOR_STRUCT, slot_budget_bit_vector_1) / 4;
        rl_params_descriptor->status_update_offset_word += (int)word_offset /* SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_WORD_OFFSET */;
    }
    rl_params_descriptor->status_update_offset_bit = bit;
}

uint32_t rdd_get_scheduler_descriptor_is_positive_bit_loc(void)
{
    SCHEDULER_DESCRIPTOR_STRUCT dummy_descriptor;
    uint32_t *c;
    int words;
    int bits = 0;
    int found_word = 0; /* note- for pass coverity, always find bit as we set 1 bit as 1*/
    memset(&dummy_descriptor, 0, sizeof(dummy_descriptor));
    dummy_descriptor.is_positive_budget = 1;
    c = (uint32_t *)&dummy_descriptor;

    for (words = 0; words < ((int)sizeof(dummy_descriptor) / (int)sizeof(uint32_t)); words++)
    {
        if (*c)
        {
            found_word = 1;
            break;
        }
        c++;
    }

    if (found_word == 1)
    {
        for (bits = 0; bits <= 31; bits++)
        {
            if (*c & (1 << bits))
                break;
        }
    }
    return bits;
}

int32_t rdd_rate_limiter_budget_alloc(tm_identifier_e tm_identity)
{
    uint8_t *RATE_LIMIT_BUDGET_VALID_TABLE_P;
    uint32_t reg_val;
    int budget_index;
    int budget_index_bit;

    RATE_LIMIT_BUDGET_VALID_TABLE_P = schedulers_info[tm_identity].rl_budget_valid_table_p;
    for (budget_index = 0; budget_index < schedulers_info[tm_identity].rl_budget_valid_max; budget_index++)
    {
        RDD_BYTES_4_BITS_READ(reg_val, RATE_LIMIT_BUDGET_VALID_TABLE_P + (budget_index / 32)*sizeof(uint32_t));
        budget_index_bit = reg_val & (1 << (budget_index % 32));
        if (budget_index_bit == 0)
        {
            /* we found empty budget index, return it*/
            return budget_index;
        }
    }
    return BDMF_INDEX_UNASSIGNED;
}

void rdd_rate_limiter_budget_descriptor_remove_desc(tm_identifier_e tm_identity, uint32_t budget_desc_id, uint32_t rl_id)
{
    RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT budget_descriptor;
    rdd_rate_limiter_budget_descriptor_get(tm_identity, budget_desc_id, &budget_descriptor);

    if (budget_descriptor.rl_params_desc_0 == rl_id) 
        budget_descriptor.rl_params_desc_0 = RATE_LIMIT_DESC_INVALID;
    
    if (budget_descriptor.rl_params_desc_1 == rl_id) 
        budget_descriptor.rl_params_desc_1 = RATE_LIMIT_DESC_INVALID;

    if (budget_descriptor.rl_params_desc_2 == rl_id) 
        budget_descriptor.rl_params_desc_2 = RATE_LIMIT_DESC_INVALID;

    if (budget_descriptor.rl_params_desc_3 == rl_id) 
        budget_descriptor.rl_params_desc_3 = RATE_LIMIT_DESC_INVALID;

    if ((budget_descriptor.rl_params_desc_0 == RATE_LIMIT_DESC_INVALID) && (budget_descriptor.rl_params_desc_1 == RATE_LIMIT_DESC_INVALID) &&
        (budget_descriptor.rl_params_desc_2 == RATE_LIMIT_DESC_INVALID) && (budget_descriptor.rl_params_desc_3 == RATE_LIMIT_DESC_INVALID))
    {
            /* budget become non active, do it before erally set the budget vector*/
            rdd_rate_limiter_budget_valid_set(tm_identity, budget_desc_id, 0);
    }
    else
    {
        /* 0 cant stay null as it used in budget allocator so in case it happenes, change order*/
        if (budget_descriptor.rl_params_desc_0 == RATE_LIMIT_DESC_INVALID) 
        {
            if (budget_descriptor.rl_params_desc_1 != RATE_LIMIT_DESC_INVALID)
            {
                budget_descriptor.rl_params_desc_0 = budget_descriptor.rl_params_desc_1;
                budget_descriptor.rl_params_desc_1 = RATE_LIMIT_DESC_INVALID;
            }
            else if (budget_descriptor.rl_params_desc_2 != RATE_LIMIT_DESC_INVALID)
            {
                budget_descriptor.rl_params_desc_0 = budget_descriptor.rl_params_desc_2;
                budget_descriptor.rl_params_desc_2 = RATE_LIMIT_DESC_INVALID;
            }
            else if (budget_descriptor.rl_params_desc_3 != RATE_LIMIT_DESC_INVALID)
            {
                budget_descriptor.rl_params_desc_0 = budget_descriptor.rl_params_desc_3;
                budget_descriptor.rl_params_desc_3 = RATE_LIMIT_DESC_INVALID;
            }
        } 
    }

    rdd_rate_limiter_budget_descriptor_set(tm_identity, budget_desc_id, &budget_descriptor);
}

bdmf_error_t _rdd_rate_limiter_cfg(const rdpa_tm_rl_cfg_t *rl_object, rdpa_tm_rl_rate_mode supported_rl_rate_mode, rdpa_tm_rl_rate_mode rl_rate_mode, 
    tm_identifier_e tm_identity, int16_t rl_index, rdd_basic_rl_cfg_t *rl_cfg, int new_conf, uint8_t sir_pir)
{
#if defined(RDP_SIM)
    /* rate limiter isn't support in simulator (it requires exact timer timing) */
    return BDMF_ERR_OK;
#else
    bdmf_error_t rc;
    uint32_t alloc_rate, limit_rate, budget_residue;
    rdd_rl_float_t basic_rl_float_limit, basic_rl_float_alloc;
    uint32_t rate_limiter_timer_period;
    uint16_t queue_index_offset;
    int profile_id;
    RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT rl_params_descriptor;
    BBH_QUEUE_DESCRIPTOR_STRUCT bbh_queue_descriptor;
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT queue_descriptor;
    RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT budget_descriptor;
    BBMSG_RNR_TO_BBH_TX_STRUCT bbmsg_ack = {};
    rdpa_rdd_sched_type_t scheduler_type;
    uint8_t bbh_queue_desc_id;
    uint8_t scheduler_index;
    sched_id_t sched_id;
    uint8_t *primary_scheduler_address;
    uint8_t slot_index;
    int32_t budget_desc_id;
    int16_t cur_budget_id;

    rate_limiter_timer_period = RATE_LIMIT_TIMER_PERIOD;
    rc  = BDMF_ERR_OK;

    BDMF_TRACE_DBG("tm_identity = %d, rl_index = %d, rl_cfg %px = { rate = %d, block type = %d, block_index = %d  }\n",
        tm_identity, rl_index, rl_cfg, rl_cfg->rate, rl_cfg->type, rl_cfg->block_index);
    if (rl_index >= RDD_ETH_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_SIZE)
    {
        BDMF_TRACE_RET(BDMF_ERR_RANGE, "error configure rate limit, out of range");
    }

    alloc_rate = rdd_rate_to_alloc_unit(rl_cfg->rate, rate_limiter_timer_period, &budget_residue);
    basic_rl_float_alloc = rdd_rate_limiter_get_floating_point_rep(alloc_rate, exponent_list);
    
    limit_rate = rl_cfg->limit;

    if (!limit_rate)
        limit_rate = alloc_rate;

    limit_rate += (budget_residue * 10);

    if (limit_rate > RL_MAX_BUCKET_SIZE)
        limit_rate = RL_MAX_BUCKET_SIZE;

    basic_rl_float_limit = rdd_rate_limiter_get_floating_point_rep(limit_rate, exponent_list);

    /* remove old profile */
    rdd_rate_limiter_params_descriptor_get(tm_identity, rl_index, &rl_params_descriptor);

    rdd_rate_limiter_profile_change_ref(rl_params_descriptor.rate_profile_id, -1);

    profile_id = rdd_rate_limiter_profile_set(basic_rl_float_alloc, basic_rl_float_limit, budget_residue);
    if (profile_id == -1)
    {
        /* revert the remove of profile */
        rdd_rate_limiter_profile_change_ref(rl_params_descriptor.rate_profile_id, 1);
        BDMF_TRACE_RET(BDMF_ERR_NORES, "no more rate limits profiles exist\n");
    }

    rl_params_descriptor.rate_profile_id = profile_id;


    /* set object budget_id for reuse */
    if (sir_pir == RL_SIR) 
        cur_budget_id = rl_object->budget_id_sir;
    else 
        cur_budget_id = rl_object->budget_id_pir;
     
    

    if ((!new_conf) || (cur_budget_id != BDMF_INDEX_UNASSIGNED))
    {
        rl_params_descriptor.budget_desc_id = cur_budget_id;
    }
    else
    {
        budget_desc_id = rdd_rate_limiter_budget_alloc(tm_identity);
        if (budget_desc_id == BDMF_INDEX_UNASSIGNED)
        {
            BDMF_TRACE_RET(BDMF_ERR_NORES, "rate limit budget has no more resources\n");
        }
        rl_params_descriptor.budget_desc_id = budget_desc_id;
        _rdpa_rate_limit_set_budget_id(rl_object->_obj_, budget_desc_id, sir_pir);
    }

   /* get the assiciated BBH_QUEUE_DESCRIPTOR*/

    if (rl_cfg->type == rdd_basic_rl_basic_scheduler)
    {
        sched_id = rl_cfg->block_index;
        scheduler_index = get_scheduler_index(sched_id);
        scheduler_type = get_scheduler_type(sched_id);

        if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
        {
            uint32_t sched_address;

            primary_scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_PRIMARY);
            sched_address = (uint32_t)(uintptr_t)primary_scheduler_address;
            rl_params_descriptor.status_update_offset_word = sched_address / 4;
            rl_params_descriptor.status_update_offset_bit = rdd_get_scheduler_descriptor_is_positive_bit_loc(); /* SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_F_OFFSET */;
        }
        else /* RDD_SCHED_TYPE_SECONDARY */
        {
            uint8_t primary_scheduler_index;
            uint8_t *secondary_scheduler_address;

            secondary_scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_SECONDARY);
            RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PRIMARY_SCHEDULER_INDEX_READ(primary_scheduler_index, secondary_scheduler_address);
            primary_scheduler_address = get_scheduler_address(tm_identity, primary_scheduler_index, RDD_SCHED_TYPE_PRIMARY);
            RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PRIMARY_SCHEDULER_SLOT_INDEX_READ(slot_index, secondary_scheduler_address);
            _rdd_rate_limit_scheduler_status_update_set(&rl_params_descriptor, rl_rate_mode, primary_scheduler_address, slot_index);
        }

        if (schedulers_info[tm_identity].bbh_bound == 1)
        {
              RDD_SCHEDULER_DESCRIPTOR_BBH_QUEUE_DESC_ID_READ(bbh_queue_desc_id, primary_scheduler_address);
              rc = rc ? rc : rdd_bbh_queue_descriptor_get(tm_identity, bbh_queue_desc_id, &bbh_queue_descriptor);
        }
    }
    else
    {
        uint8_t *scheduler_address;

        rc = rc ? rc : rdd_scheduler_queue_get(tm_identity, rl_cfg->block_index - schedulers_info[tm_identity].qm_queue_start, &queue_descriptor);
        if (schedulers_info[tm_identity].bbh_bound == 1)
            rc = rc ? rc : rdd_bbh_queue_descriptor_get(tm_identity, queue_descriptor.bbh_queue_index, &bbh_queue_descriptor);

        queue_index_offset = rl_cfg->block_index - schedulers_info[tm_identity].qm_queue_start;
        if (queue_index_offset >= schedulers_info[tm_identity].queue_descriptor_table_size)
        {
            BDMF_TRACE_RET(BDMF_ERR_PARM, "queue index is invalid (index=%d, max=%d)\n", queue_index_offset, schedulers_info[tm_identity].queue_descriptor_table_size);
        }

        rc = rc ? rc : rdd_scheduler_queue_get(tm_identity, queue_index_offset, &queue_descriptor);

        if (queue_descriptor.scheduler_type == SCHEDULER_TYPE_SECONDARY)
            BDMF_TRACE_RET(BDMF_ERR_PARM, "rate limit is not supported on secondary scheduler queues\n");

        scheduler_type = queue_descriptor.scheduler_type;
        scheduler_index = queue_descriptor.scheduler_index;
        scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

        _rdd_rate_limit_scheduler_status_update_set(&rl_params_descriptor, rl_rate_mode, scheduler_address, queue_descriptor.queue_index);
    }

    if (schedulers_info[tm_identity].bbh_bound == 1)
    {
        rl_params_descriptor.wakeup_mode = TM_WAKEUP_MODE_ACK;
        bbmsg_ack.bb_id = bbh_queue_descriptor.bb_destination;
        bbmsg_ack.queue = bbh_queue_descriptor.hw_bbh_qid;
        rl_params_descriptor.wakeup_message_data = bbmsg_ack.word_16[0];
    }
    else
    {
        rl_params_descriptor.wakeup_mode = TM_WAKEUP_MODE_MSG;
        rl_params_descriptor.wakeup_message_data = THREAD_ASYNC_WAKEUP_REQUEST(SERVICE_QUEUES_THREAD_NUMBER);
    }

    rc = rc ? rc : rdd_rate_limiter_params_descriptor_set(tm_identity, rl_index, &rl_params_descriptor);

    rdd_rate_limiter_budget_descriptor_get(tm_identity, rl_params_descriptor.budget_desc_id, &budget_descriptor);

    if ((budget_descriptor.rl_params_desc_0 != rl_index) && (budget_descriptor.rl_params_desc_1 != rl_index) &&
        (budget_descriptor.rl_params_desc_2 != rl_index) && (budget_descriptor.rl_params_desc_3 != rl_index))
    {
        if ((supported_rl_rate_mode == rdpa_tm_rl_dual_rate) && 
            ((budget_descriptor.rl_params_desc_0 != RATE_LIMIT_DESC_INVALID) || (budget_descriptor.rl_params_desc_1 != RATE_LIMIT_DESC_INVALID) ||
            (budget_descriptor.rl_params_desc_2 != RATE_LIMIT_DESC_INVALID) || (budget_descriptor.rl_params_desc_3 != RATE_LIMIT_DESC_INVALID)))
            {
                BDMF_TRACE_RET(BDMF_ERR_NOT_SUPPORTED, "dual rate limit is not support reuse of rate limit for few queues\n");
            }

        if (budget_descriptor.rl_params_desc_0 == RATE_LIMIT_DESC_INVALID)
            budget_descriptor.rl_params_desc_0 = rl_index;
        else if (budget_descriptor.rl_params_desc_1 == RATE_LIMIT_DESC_INVALID)
            budget_descriptor.rl_params_desc_1 = rl_index;
        else if (budget_descriptor.rl_params_desc_2 == RATE_LIMIT_DESC_INVALID)
            budget_descriptor.rl_params_desc_2 = rl_index;
        else if (budget_descriptor.rl_params_desc_3 == RATE_LIMIT_DESC_INVALID)
            budget_descriptor.rl_params_desc_3 = rl_index;
        else
        {
            BDMF_TRACE_RET(BDMF_ERR_NORES, "rate limits budget is full\n");
        }

        rdd_rate_limiter_budget_descriptor_set(tm_identity, rl_params_descriptor.budget_desc_id, &budget_descriptor);
    }

    if (rl_cfg->type == rdd_basic_rl_basic_scheduler)
    {
        uint8_t *scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

        RDD_COMMON_SCHEDULER_DESCRIPTOR_IS_POSITIVE_BUDGET_WRITE(1, scheduler_address);

        if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
        {
            RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(1, scheduler_address);
            RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(rl_index, scheduler_address);
        }
        else /* RDD_SCHED_TYPE_SECONDARY */
        {
            if (rl_rate_mode == rdpa_tm_rl_single_rate)
            {
                RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(0xFFFFFFFF, primary_scheduler_address);
                RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_WRITE(rl_index, scheduler_address);
                RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(1, scheduler_address);
            }
            else /* rdpa_tm_rl_dual_rate */
            {
                RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_WRITE(0xFFFFFFFF, primary_scheduler_address);
                RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PIR_RATE_LIMITER_INDEX_WRITE(rl_index, scheduler_address);
                RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PIR_RATE_LIMIT_ENABLE_WRITE(1, scheduler_address);
            }
        }
    }
    else
    {
        uint8_t *scheduler_address;

        scheduler_address = get_scheduler_address(tm_identity, scheduler_index, RDD_SCHED_TYPE_PRIMARY);

        if (rl_rate_mode == rdpa_tm_rl_single_rate)
        {
            queue_descriptor.sir_rate_limiter_index = rl_index;
            queue_descriptor.sir_rate_limit_enable = 1;
            RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_0_WRITE(0xFFFFFFFF, scheduler_address);
        }
        else
        {
            queue_descriptor.pir_rate_limiter_index = rl_index;
            queue_descriptor.sir_rate_limit_enable = 1;
            queue_descriptor.pir_rate_limit_enable = 1;
            RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_WRITE(0xFFFFFFFF, scheduler_address);
        }
        queue_descriptor.rate_limit_enable = 1;

        rc = rc ? rc : rdd_scheduler_queue_set_rate_limits_fields(tm_identity, queue_index_offset, &queue_descriptor);
    }
    rc = rc ? rc : rdd_rate_limiter_budget_valid_set(tm_identity, rl_params_descriptor.budget_desc_id, 1);

    return rc;
#endif
}

void rdd_scheduler_wake_up_bbh_init_data_structure(void)
{
    uint32_t i, tm_core_id;
    
    RATE_LIMITER_BUDGET_DESCRIPTOR_STRUCT budget_descriptor;
    RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT rl_params;
    /* 1. init budget descriptor*/

    budget_descriptor.current_budget = 0;
    budget_descriptor.rl_params_desc_0 = RATE_LIMIT_DESC_INVALID;
    budget_descriptor.rl_params_desc_1 = RATE_LIMIT_DESC_INVALID;
    budget_descriptor.rl_params_desc_2 = RATE_LIMIT_DESC_INVALID;
    budget_descriptor.rl_params_desc_3 = RATE_LIMIT_DESC_INVALID;
    for (tm_core_id = 0; tm_core_id < TM_NUM_OF_IDENTITY; tm_core_id++)
    {
        for (i = 0; i < schedulers_info[tm_core_id].rl_budget_desc_table_size; i++)
        {
            rdd_rate_limiter_budget_descriptor_set(tm_core_id, i, &budget_descriptor);
        }
    }

    rl_params.rate_profile_id = BUDGET_NOT_VALID;
    for (tm_core_id = 0; tm_core_id < TM_NUM_OF_IDENTITY; tm_core_id++)
    {
        for (i = 0; i < schedulers_info[tm_core_id].rl_params_table_size; i++)
        {
            rdd_rate_limiter_params_descriptor_set(tm_core_id, i, &rl_params);
        }
    }
}

bdmf_error_t rdd_rate_limiter_remove(tm_identifier_e tm_identity, int16_t rl_index)
{
    SCHEDULING_QUEUE_DESCRIPTOR_STRUCT queue_descriptor;
    RATE_LIMITER_PARAMS_DESCRIPTOR_STRUCT rl_params;
    rdd_rl_float_t current_alloc, current_limit;
    rdd_rl_float_t alloc, limit;
    uint8_t rate_limiter_index;
    int rate_limit_enable;
    sched_id_t sched_id;
    int i;

    /* clear slot_budget_vector_1 mechanism*/
    uint8_t *clear_sched_address[RATE_LIMIT_MAX_DESCRIPTORS];
    uint8_t clear_sched_slot[RATE_LIMIT_MAX_DESCRIPTORS];
    int clear_index = 0;

    /* Find all rate limiter elements by iterating over *all* of the schedulers.
     * This is a bit cumbersome but easy to code and debug (could've used brcm_ffsll() on the bitmap)
     */
    for (sched_id = 0; sched_id < get_num_sched_ids(); sched_id++)
    {
        rdpa_rdd_sched_type_t scheduler_type;
        uint8_t scheduler_index;
        uint8_t *scheduler_address;

        if (!sched_id_in_use(sched_id))
            continue;

        scheduler_index = get_scheduler_index(sched_id);
        scheduler_type = get_scheduler_type(sched_id);
        scheduler_address = get_scheduler_address(tm_identity, scheduler_index, scheduler_type);

        RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMITER_INDEX_READ(rate_limiter_index, scheduler_address);
        RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_READ(rate_limit_enable, scheduler_address);

        if (scheduler_type == RDD_SCHED_TYPE_PRIMARY)
        {
            if (rate_limit_enable && (rate_limiter_index == rl_index))
            {
                RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(0, scheduler_address);
            }
        }
        else if (scheduler_type == RDD_SCHED_TYPE_SECONDARY)
        {
            /* Check CIR */
            if (rate_limit_enable && (rate_limiter_index == rl_index))
            {
                RDD_COMMON_SCHEDULER_DESCRIPTOR_RATE_LIMIT_ENABLE_WRITE(0, scheduler_address);
            }

            /* Check PIR */
            RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PIR_RATE_LIMIT_ENABLE_READ(rate_limit_enable, scheduler_address);
            RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PIR_RATE_LIMITER_INDEX_READ(rate_limiter_index, scheduler_address);
            if (rate_limit_enable && (rate_limiter_index == rl_index))
            {
                uint8_t pri_scheduler_index;

                RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PIR_RATE_LIMIT_ENABLE_WRITE(0, scheduler_address);

                if (clear_index < RATE_LIMIT_MAX_DESCRIPTORS)
                {
                    RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PRIMARY_SCHEDULER_SLOT_INDEX_READ(clear_sched_slot[clear_index], scheduler_address);
                    RDD_SECONDARY_SCHEDULER_DESCRIPTOR_PRIMARY_SCHEDULER_INDEX_READ(pri_scheduler_index, scheduler_address);
                    clear_sched_address[clear_index] = get_scheduler_address(tm_identity, pri_scheduler_index, RDD_SCHED_TYPE_PRIMARY);
                    clear_index++;
                }
            }
        }
    }

    /* go over all queues*/
    for (i = 0; i < schedulers_info[tm_identity].queue_descriptor_table_size; i++)
    {
        rdd_scheduler_queue_get(tm_identity, i, &queue_descriptor);
        if ((queue_descriptor.sir_rate_limiter_index == rl_index) && (queue_descriptor.rate_limit_enable == 1) && (queue_descriptor.sir_rate_limit_enable == 1))
        {
            queue_descriptor.sir_rate_limit_enable = 0;
            queue_descriptor.rate_limit_enable = queue_descriptor.sir_rate_limit_enable | queue_descriptor.pir_rate_limit_enable;
            rdd_scheduler_queue_set_rate_limits_fields(tm_identity, i, &queue_descriptor);
        }

        if ((queue_descriptor.pir_rate_limiter_index == rl_index) && (queue_descriptor.rate_limit_enable == 1) && (queue_descriptor.pir_rate_limit_enable == 1))
        {
            queue_descriptor.pir_rate_limit_enable = 0;
            queue_descriptor.rate_limit_enable = queue_descriptor.sir_rate_limit_enable | queue_descriptor.pir_rate_limit_enable;
            rdd_scheduler_queue_set_rate_limits_fields(tm_identity, i, &queue_descriptor);

            if ((queue_descriptor.scheduler_type == SCHEDULER_TYPE_PRIMARY) && (clear_index < RATE_LIMIT_MAX_DESCRIPTORS))
            {
                clear_sched_address[clear_index] = get_scheduler_address(tm_identity, queue_descriptor.scheduler_index, RDD_SCHED_TYPE_PRIMARY);
                clear_sched_slot[clear_index] = queue_descriptor.queue_index;
                clear_index++;
            }
        }
    }

    /* at this stage queue and scheduler with this rate limit are disabled, so budget wont go down*/
    /* give it profile with temporary max size rate limit to have positive budget and fw will wake up associated tx task*/

    /* get rate limit*/
    rdd_rate_limiter_params_descriptor_get(tm_identity, rl_index, &rl_params);

    RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_READ_G(current_limit.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_READ_G(current_limit.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_READ_G(current_alloc.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_READ_G(current_alloc.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id); 

    limit.exponent = 0xFF;
    limit.mantissa = 0xFFFF;
    alloc.exponent = 0xFF;
    alloc.mantissa = 0xFFFF;
    
    RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(limit.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(limit.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(alloc.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(alloc.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);

    RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(limit.exponent, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(limit.mantissa, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(alloc.exponent, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(alloc.mantissa, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id); 

    bdmf_msleep(RATE_LIMIT_CLOSE_TIMEOUT);
    /* now return original profile */
    
    RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(current_limit.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(current_limit.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(current_alloc.exponent, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(current_alloc.mantissa, RDD_PON_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);

    RDD_RATE_LIMITER_PROFILE_LIMIT_EXPONENT_WRITE_G(current_limit.exponent, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_LIMIT_MANTISSA_WRITE_G(current_limit.mantissa, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_EXPONENT_WRITE_G(current_alloc.exponent, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id);
    RDD_RATE_LIMITER_PROFILE_ALLOC_MANTISSA_WRITE_G(current_alloc.mantissa, RDD_ETH_TM_RATE_LIMITER_PROFILE_TABLE_ADDRESS_ARR, rl_params.rate_profile_id); 

    /* release it profile */
    rdd_rate_limiter_profile_change_ref(rl_params.rate_profile_id, -1);

    /* remove the link in budget descriptor*/
    rdd_rate_limiter_budget_descriptor_remove_desc(tm_identity, rl_params.budget_desc_id, rl_index);

    /* At this phase we are sure the FW's budget allocater is disabled and we can clear the slot
     * budget bit without risk of it being set again.
     */
    while (clear_index--)
    {
        uint32_t slot_budget_bit_vector_1;

        RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_READ(slot_budget_bit_vector_1, clear_sched_address[clear_index]);
        slot_budget_bit_vector_1 &= ~(1 << clear_sched_slot[clear_index]);
        RDD_SCHEDULER_DESCRIPTOR_SLOT_BUDGET_BIT_VECTOR_1_WRITE(slot_budget_bit_vector_1, clear_sched_address[clear_index]);
    }

    /* wait and disable profile id*/
    bdmf_msleep(RATE_LIMIT_CLOSE_TIMEOUT);
    rl_params.rate_profile_id = BUDGET_NOT_VALID;
    rdd_rate_limiter_params_descriptor_set(tm_identity, rl_index, &rl_params);

    return 0;
}

void rdd_scheduling_conf_init(void)
{
    int tm_identity;
    schedulers_info[TM_PON_DSL_AE].scheduler_table_size = RDD_PON_TM_SCHEDULER_TABLE_SIZE;
    schedulers_info[TM_PON_DSL_AE].scheduler_table_p = (uint8_t *)RDD_PON_TM_SCHEDULER_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));
    schedulers_info[TM_PON_DSL_AE].secondary_scheduler_table_size = RDD_PON_TM_SECONDARY_SCHEDULER_TABLE_SIZE;
    schedulers_info[TM_PON_DSL_AE].secondary_scheduler_table_p = (uint8_t *)RDD_PON_TM_SECONDARY_SCHEDULER_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));
    schedulers_info[TM_PON_DSL_AE].scheduler_pool_p = (uint8_t *)RDD_PON_TM_SCHEDULER_POOL_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));
    schedulers_info[TM_PON_DSL_AE].rl_params_table_size = RDD_PON_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_SIZE;
    schedulers_info[TM_PON_DSL_AE].rl_params_table_p = (uint8_t *)RDD_PON_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));  
    schedulers_info[TM_PON_DSL_AE].rl_budget_valid_max = RDD_PON_TM_RATE_LIMITER_BUDGET_VALID_SIZE * BITS_IN_WORD;
    schedulers_info[TM_PON_DSL_AE].rl_budget_valid_table_p = (uint8_t *)RDD_PON_TM_RATE_LIMITER_BUDGET_VALID_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));  
    schedulers_info[TM_PON_DSL_AE].rl_budget_desc_table_size = RDD_PON_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_SIZE;
    schedulers_info[TM_PON_DSL_AE].rl_budget_desc_table_p = (uint8_t *)RDD_PON_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));
    schedulers_info[TM_PON_DSL_AE].bbh_queue_descriptor_table_size = RDD_US_TM_BBH_QUEUE_TABLE_SIZE;
    schedulers_info[TM_PON_DSL_AE].bbh_queue_descriptor_table_p =  (uint8_t *)RDD_US_TM_BBH_QUEUE_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));  
    schedulers_info[TM_PON_DSL_AE].probability_calc_desc_table_p = (uint8_t *)RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));
#ifdef L4S_LAQM_DEBUG
    schedulers_info[TM_PON_DSL_AE].laqm_debug_table_p = (uint8_t *)RDD_LAQM_DEBUG_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));
#endif
    schedulers_info[TM_PON_DSL_AE].queue_descriptor_table_size = RDD_PON_TM_SCHEDULING_QUEUE_TABLE_SIZE;
    schedulers_info[TM_PON_DSL_AE].queue_descriptor_table_p = (uint8_t *)RDD_PON_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));  
    schedulers_info[TM_PON_DSL_AE].aqm_queue_descriptor_table_p = (uint8_t *)RDD_PON_TM_AQM_QUEUE_TABLE_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));  
    schedulers_info[TM_PON_DSL_AE].queue_pool_p = (uint8_t *)RDD_PON_TM_SCHEDULER_POOL_PTR(tm_get_core_for_tm(TM_PON_DSL_AE));  
    schedulers_info[TM_PON_DSL_AE].qm_queue_start = drv_qm_get_us_start();
    schedulers_info[TM_PON_DSL_AE].qm_queue_end = drv_qm_get_us_end();
    schedulers_info[TM_PON_DSL_AE].bbh_bound = 1;

    for (tm_identity = TM_ETH_START; tm_identity <= TM_ETH_END; tm_identity++)
    {
        schedulers_info[tm_identity].scheduler_table_size = RDD_ETH_TM_SCHEDULER_TABLE_SIZE;
        schedulers_info[tm_identity].scheduler_table_p = (uint8_t *)RDD_ETH_TM_SCHEDULER_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].secondary_scheduler_table_size = RDD_ETH_TM_SECONDARY_SCHEDULER_TABLE_SIZE;
        schedulers_info[tm_identity].secondary_scheduler_table_p = (uint8_t *)RDD_ETH_TM_SECONDARY_SCHEDULER_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].scheduler_pool_p = (uint8_t *)RDD_ETH_TM_SCHEDULER_POOL_PTR(tm_get_core_for_tm(tm_identity));
        schedulers_info[tm_identity].rl_params_table_size = RDD_ETH_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_SIZE;
        schedulers_info[tm_identity].rl_params_table_p = (uint8_t *)RDD_ETH_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].rl_budget_valid_max = RDD_ETH_TM_RATE_LIMITER_BUDGET_VALID_SIZE * BITS_IN_WORD;
        schedulers_info[tm_identity].rl_budget_valid_table_p = (uint8_t *)RDD_ETH_TM_RATE_LIMITER_BUDGET_VALID_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].rl_budget_desc_table_size = RDD_ETH_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_SIZE;
        schedulers_info[tm_identity].rl_budget_desc_table_p = (uint8_t *)RDD_ETH_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].bbh_queue_descriptor_table_size = RDD_DS_TM_BBH_QUEUE_TABLE_SIZE;
        schedulers_info[tm_identity].bbh_queue_descriptor_table_p =  (uint8_t *)RDD_DS_TM_BBH_QUEUE_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].probability_calc_desc_table_p = (uint8_t *)RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(tm_get_core_for_tm(tm_identity));
#ifdef L4S_LAQM_DEBUG
        schedulers_info[tm_identity].laqm_debug_table_p = (uint8_t *)RDD_LAQM_DEBUG_TABLE_PTR(tm_get_core_for_tm(tm_identity));
#endif
        schedulers_info[tm_identity].queue_descriptor_table_size = RDD_ETH_TM_SCHEDULING_QUEUE_TABLE_SIZE;
        schedulers_info[tm_identity].queue_descriptor_table_p = (uint8_t *)RDD_ETH_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_get_core_for_tm(tm_identity)); 
        schedulers_info[tm_identity].aqm_queue_descriptor_table_p = (uint8_t *)RDD_ETH_TM_AQM_QUEUE_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].queue_pool_p = (uint8_t *)RDD_ETH_TM_SCHEDULER_POOL_PTR(tm_get_core_for_tm(tm_identity));  
        schedulers_info[tm_identity].qm_queue_start = drv_qm_get_first_ds_queue_for_tm_index(tm_identity - TM_ETH_START);
        schedulers_info[tm_identity].qm_queue_end = drv_qm_get_last_ds_queue_for_tm_index(tm_identity - TM_ETH_START);
        schedulers_info[tm_identity].bbh_bound = 1;
    }

    schedulers_info[TM_ETH_SQ].scheduler_table_size = RDD_SQ_TM_SCHEDULER_TABLE_SIZE;
    schedulers_info[TM_ETH_SQ].scheduler_table_p = (uint8_t *)RDD_SQ_TM_SCHEDULER_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
    schedulers_info[TM_ETH_SQ].secondary_scheduler_table_size = RDD_SQ_TM_SECONDARY_SCHEDULER_TABLE_SIZE;
    schedulers_info[TM_ETH_SQ].secondary_scheduler_table_p = (uint8_t *)RDD_SQ_TM_SECONDARY_SCHEDULER_TABLE_PTR(tm_get_core_for_tm(tm_identity));  
    schedulers_info[TM_ETH_SQ].scheduler_pool_p = (uint8_t *)RDD_SQ_TM_SCHEDULER_POOL_PTR(tm_get_core_for_tm(TM_ETH_SQ));
    schedulers_info[TM_ETH_SQ].rl_params_table_size = RDD_SQ_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_SIZE;
    schedulers_info[TM_ETH_SQ].rl_params_table_p = (uint8_t *)RDD_SQ_TM_RATE_LIMITER_PARAMS_DESCRIPTOR_TABLE_PTR(tm_get_core_for_tm(TM_ETH_SQ));  
    schedulers_info[TM_ETH_SQ].rl_budget_valid_max = RDD_SQ_TM_RATE_LIMITER_BUDGET_VALID_SIZE * BITS_IN_WORD;
    schedulers_info[TM_ETH_SQ].rl_budget_valid_table_p = (uint8_t *)RDD_SQ_TM_RATE_LIMITER_BUDGET_VALID_PTR(tm_get_core_for_tm(TM_ETH_SQ));  
    schedulers_info[TM_ETH_SQ].rl_budget_desc_table_size = RDD_SQ_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_SIZE;
    schedulers_info[TM_ETH_SQ].rl_budget_desc_table_p = (uint8_t *)RDD_SQ_TM_RATE_LIMITER_BUDGET_DESCRIPTOR_TABLE_PTR(tm_get_core_for_tm(TM_ETH_SQ));  
    schedulers_info[TM_ETH_SQ].bbh_queue_descriptor_table_size = 0;
    schedulers_info[TM_ETH_SQ].bbh_queue_descriptor_table_p = NULL;  
    schedulers_info[TM_ETH_SQ].probability_calc_desc_table_p = (uint8_t *)RDD_SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(tm_get_core_for_tm(TM_ETH_SQ));
#ifdef L4S_LAQM_DEBUG
    schedulers_info[TM_ETH_SQ].laqm_debug_table_p = (uint8_t *)RDD_LAQM_DEBUG_TABLE_PTR(tm_get_core_for_tm(TM_ETH_SQ));
#endif
    schedulers_info[TM_ETH_SQ].queue_descriptor_table_size = RDD_SQ_TM_SCHEDULING_QUEUE_TABLE_SIZE;
    schedulers_info[TM_ETH_SQ].queue_descriptor_table_p = (uint8_t *)RDD_SQ_TM_SCHEDULING_QUEUE_TABLE_PTR(tm_get_core_for_tm(TM_ETH_SQ));  
    schedulers_info[TM_ETH_SQ].aqm_queue_descriptor_table_p = (uint8_t *)RDD_SQ_TM_AQM_QUEUE_TABLE_PTR(tm_get_core_for_tm(TM_ETH_SQ));  
    schedulers_info[TM_ETH_SQ].queue_pool_p = (uint8_t *)RDD_SQ_TM_SCHEDULER_POOL_PTR(tm_get_core_for_tm(TM_ETH_SQ));  
    schedulers_info[TM_ETH_SQ].qm_queue_start = drv_qm_get_sq_start();
    schedulers_info[TM_ETH_SQ].qm_queue_end = drv_qm_get_sq_end();
    schedulers_info[TM_ETH_SQ].bbh_bound = 0;
}

void *get_schedulers_info_probability_calc_desc_table_p(int tm_identity)
{
    if (tm_identity >= TM_NUM_OF_IDENTITY)
        return NULL;

    return schedulers_info[tm_identity].probability_calc_desc_table_p;
}

#ifdef L4S_LAQM_DEBUG
void *get_schedulers_info_laqm_desc_p(int tm_identity)
{
    if (tm_identity >= TM_NUM_OF_IDENTITY)
        return NULL;

    return schedulers_info[tm_identity].laqm_debug_table_p;
}
#endif

tm_identifier_e get_tm_identity_from_qm_queue(uint32_t qm_queue)
{
    int tm_identity;

    for (tm_identity = TM_START; tm_identity <= TM_MAX; tm_identity++)
    {
        if ((qm_queue >= schedulers_info[tm_identity].qm_queue_start) &&
            (qm_queue <= schedulers_info[tm_identity].qm_queue_end))
            return (tm_identifier_e)tm_identity;
    }

    return TM_ERROR;
}

void *get_pi2_calc_desc_ptr(tm_identifier_e tm_identity)
{
    if (tm_identity >= TM_NUM_OF_IDENTITY)
        return NULL;

    return (uint8_t *)schedulers_info[tm_identity].probability_calc_desc_table_p;
}

#if defined(G9991_FC)
int rdd_g9991_vport_to_emac_mapping_cfg(rdd_rdd_vport vport, rdpa_emac emac)
{
    G9991_SCHEDULING_INFO_ENTRY_STRUCT *entry;
    uint32_t port_mask, i;

    RDD_BTRACE("vport = %d, emac = %d\n", vport, emac);

    /* FIXME:MULTIPLE_BBH_TX - fix correct runner image */
    for (i = 0; i < RDD_G9991_SCHEDULING_INFO_TABLE_SIZE; i++)
    {
        entry = ((G9991_SCHEDULING_INFO_ENTRY_STRUCT *)RDD_G9991_SCHEDULING_INFO_TABLE_PTR(emac_port_mapping[i].tx_runner_core)) +
            emac_port_mapping[i].bbh_queue;

        RDD_G9991_SCHEDULING_INFO_ENTRY_PHYSICAL_PORT_MAPPING_READ(port_mask, entry);

        if (i == emac)
            port_mask |= (1 << vport);
        else
            port_mask &= ~(1 << vport);

        RDD_G9991_SCHEDULING_INFO_ENTRY_PHYSICAL_PORT_MAPPING_WRITE(port_mask, entry);
    }

    vport_to_emac[vport] = emac;

    RDD_G9991_VPORT_TO_BB_ID_ENTRY_BB_ID_WRITE_G(emac_port_mapping[emac].bb_tx_id + (emac_port_mapping[emac].bbh_queue << BBMSG_RNR_TO_BBH_TX_QUEUE_F_OFFSET),
        RDD_G9991_VPORT_TO_BB_ID_ADDRESS_ARR, vport);

    RDD_G9991_EMAC_TO_BB_QUEUE_ENTRY_BB_QUEUE_WRITE_G(emac_port_mapping[emac].bbh_queue, RDD_G9991_EMAC_TO_BB_QUEUE_TABLE_ADDRESS_ARR, emac);

    return BDMF_ERR_OK;
}

int rdd_g9991_control_sid_set(rdd_rdd_vport vport, rdpa_emac emac)
{
    G9991_SCHEDULING_INFO_ENTRY_STRUCT *entry;
    uint32_t control_sid_bit_mask, i;

    RDD_BTRACE("vport = %d, emac = %d\n", vport, emac);

    if (vport >= RDD_VPORT_ID_32)
        return BDMF_ERR_NOT_SUPPORTED;

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
}
#endif
