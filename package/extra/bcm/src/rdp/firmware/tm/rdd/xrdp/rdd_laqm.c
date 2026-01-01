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
#ifdef XRDP_DUALQ_L4S

#include "rdd.h"
#include "rdd_laqm.h"
#include "rdd_ag_ds_tm.h"
#ifdef PI2_ON_SERVICE_QUEUES
#include "rdd_ag_service_queues.h"
#endif
#include "rdd_ag_us_tm.h"
#include "rdp_drv_qm.h"

#ifdef TM_C_CODE
extern int get_tm_identity_from_qm_queue(uint32_t qm_queue);
extern rdd_tm_entity_info schedulers_info[TM_NUM_OF_IDENTITY];
#define RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR         RDD_SQ_TM_AQM_QUEUE_TABLE_ADDRESS_ARR
#endif

#define LLQ_SELECTOR_ECN_MASK_DEFAULT   LLQ_SELECTOR_ECN_ECT0_ECT1
#define COUPLING_FACTOR_DEFAULT         16      /* units of 0.125 i.e. a value of 16 means K=2 */

static bdmf_boolean _rdd_laqm_enable_ds(uint16_t queue_index)
{
    uint8_t rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);
    int rc = 0;

#ifdef TM_C_CODE
    void *queue_ptr, *aqm_queue_ptr;
    tm_identifier_e tm_identity;
    
    tm_identity = get_tm_identity_from_qm_queue(queue_index);
    if (tm_identity == TM_ERROR)
    {
        return BDMF_ERR_INTERNAL;
    }

    aqm_queue_ptr = schedulers_info[tm_identity].aqm_queue_descriptor_table_p;
    aqm_queue_ptr += rel_queue_index * sizeof(AQM_QUEUE_DESCRIPTOR_STRUCT);
    RDD_LAQM_QUEUE_DESCRIPTOR_PROBABILITY_WRITE(0, aqm_queue_ptr);
    RDD_LAQM_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_WRITE(0, aqm_queue_ptr);

    queue_ptr = schedulers_info[tm_identity].queue_descriptor_table_p;
    queue_ptr += rel_queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT);
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_LAQM_ENABLE_WRITE(1, queue_ptr);
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_AQM_ENABLE_WRITE(1, queue_ptr);
#else
    uint8_t core = drv_qm_get_runner_idx(queue_index);
    AQM_QUEUE_DESCRIPTOR_STRUCT *ptr = &((RDD_DS_TM_AQM_QUEUE_TABLE_PTR(core))->entry[rel_queue_index]);

    RDD_LAQM_QUEUE_DESCRIPTOR_PROBABILITY_WRITE(0, ptr);
    RDD_LAQM_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_WRITE(0, ptr);

    rc = rdd_ag_ds_tm_scheduling_queue_table_laqm_enable_set(rel_queue_index, 1);
    rc = rc ? rc : rdd_ag_ds_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
#endif

    return rc;
}

static bdmf_boolean _rdd_laqm_disable_ds(uint16_t queue_index)
{
    uint8_t rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);

#ifdef TM_C_CODE
    void *queue_ptr;
    tm_identifier_e tm_identity;

    tm_identity = get_tm_identity_from_qm_queue(queue_index);
    if (tm_identity == TM_ERROR)
    {
        return BDMF_ERR_INTERNAL;
    }

    queue_ptr = schedulers_info[tm_identity].queue_descriptor_table_p;
    queue_ptr += rel_queue_index * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT);
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_AQM_ENABLE_WRITE(0, queue_ptr);
    RDD_SCHEDULING_QUEUE_DESCRIPTOR_LAQM_ENABLE_WRITE(0, queue_ptr);
#else
    rdd_ag_ds_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 0);
    rdd_ag_ds_tm_scheduling_queue_table_laqm_enable_set(rel_queue_index, 0);
#endif

    return BDMF_ERR_OK;
}

static bdmf_error_t _rdd_laqm_init(void)
{
    static int initialized;

    if (!initialized)
    {
        RDD_LLQ_SELECTOR_ECN_MASK_WRITE_G(LLQ_SELECTOR_ECN_MASK_DEFAULT, RDD_LLQ_SELECTOR_ECN_MASK_ADDRESS_ARR, 0);
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_WRITE_G(COUPLING_FACTOR_DEFAULT, RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_WRITE_G(COUPLING_FACTOR_DEFAULT, RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_WRITE_G(COUPLING_FACTOR_DEFAULT, RDD_SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);    

        initialized = 1;
    }

    return BDMF_ERR_OK;
}

/* Enable codel on queue */
bdmf_error_t rdd_laqm_enable(uint16_t queue_index)
{
    uint8_t rel_queue_index;
#ifdef PD_TS_INSERTION_BY_HW
    qm_q_context q_context = {};
#endif
    bdmf_error_t rc;

    _rdd_laqm_init();

    RDD_BTRACE("qm_queue_index = %d\n", queue_index);

#ifdef PD_TS_INSERTION_BY_HW
    /* Timestamp is inserted by the h/w. Set timestamp resolution profile */
    rc = ag_drv_qm_q_context_get(queue_index, &q_context);
    q_context.timestamp_res_profile = QM_TIMESTAMP_RES_PROFILE_128US;
    rc = rc ? rc : ag_drv_qm_q_context_set(queue_index, &q_context);
    if (rc != BDMF_ERR_OK)
        return rc;
#endif

    /* Enable on the scheduler side */
#ifdef PI2_ON_SERVICE_QUEUES
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        RDD_LAQM_QUEUE_DESCRIPTOR_PROBABILITY_WRITE_G(0, RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_LAQM_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_WRITE_G(0, RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        rc = rdd_ag_service_queues_scheduling_queue_table_laqm_enable_set(rel_queue_index, 1);
        rc = rc ? rc : rdd_ag_service_queues_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
    }
    else
#endif
    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        rc = _rdd_laqm_enable_ds(queue_index);
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_us_start();
        RDD_LAQM_QUEUE_DESCRIPTOR_PROBABILITY_WRITE_G(0, RDD_PON_TM_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_LAQM_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_WRITE_G(0, RDD_PON_TM_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        rc = rdd_ag_us_tm_scheduling_queue_table_laqm_enable_set(rel_queue_index, 1);
        rc = rc ? rc : rdd_ag_us_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
    }
    else
    {
        RDD_BTRACE("LAQM is not supported on queue %d\n", queue_index);
        rc = BDMF_ERR_NOT_SUPPORTED;
    }

    if (rc != BDMF_ERR_OK)
    {
        rdd_laqm_disable(queue_index);
        return rc;
    }

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_laqm_disable(uint16_t queue_index)
{
    uint8_t rel_queue_index;

    RDD_BTRACE("qm_queue_index = %d\n", queue_index);

    /* Disable everything unconditionally. It really shouldn't fail.
       Parameter errors will be detected by rdd_pi2_enable */
    /* Disable on the scheduler side */
#ifdef PI2_ON_SERVICE_QUEUES
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        rdd_ag_service_queues_scheduling_queue_table_aqm_enable_set(rel_queue_index, 0);
        rdd_ag_service_queues_scheduling_queue_table_laqm_enable_set(rel_queue_index, 0);
    }
    else
#endif
    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        _rdd_laqm_disable_ds(queue_index);
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_us_start();
        rdd_ag_us_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 0);
        rdd_ag_us_tm_scheduling_queue_table_laqm_enable_set(rel_queue_index, 0);
    }
    else
    {
        RDD_BTRACE("LAQM is not supported on queue %d\n", queue_index);
    }

    return BDMF_ERR_OK;
}

void rdd_laqm_cfg_get(int *coupling_factor, int *lq_ecn_mask)
{
    _rdd_laqm_init();

    RDD_LLQ_SELECTOR_ECN_MASK_READ_G(*lq_ecn_mask, RDD_LLQ_SELECTOR_ECN_MASK_ADDRESS_ARR, 0);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_READ_G(*coupling_factor, RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);
}

/* Set low-latency queue packet selector:
   use 1 of LLQ_SELECTOR_ECN_DCTCP_COMPAT, LLQ_SELECTOR_ECN_DCTCP_COMPAT constants */
bdmf_error_t rdd_laqm_cfg(int coupling_factor, int lq_ecn_mask)
{
    if ((lq_ecn_mask != LLQ_SELECTOR_ECN_ECT1) && (lq_ecn_mask != LLQ_SELECTOR_ECN_ECT0_ECT1))
        return BDMF_ERR_PARM;

    /* units of 0.125 give 0, 0.125, ..., 31.875 */
    if ((coupling_factor < 0) || (coupling_factor > 255))
        return BDMF_ERR_PARM;

    _rdd_laqm_init();

    RDD_LLQ_SELECTOR_ECN_MASK_WRITE_G(lq_ecn_mask, RDD_LLQ_SELECTOR_ECN_MASK_ADDRESS_ARR, 0);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_WRITE_G(coupling_factor, RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_WRITE_G(coupling_factor, RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_LAQM_COUPLING_FACTOR_WRITE_G(coupling_factor, RDD_SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR, 0);

    return BDMF_ERR_OK;
}

#ifdef L4S_LAQM_DEBUG
static void _laqm_reset_debug_descriptor(LAQM_DEBUG_DESCRIPTOR_STRUCT *descr)
{
    int i;

    RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_TOTAL_WRITE(0, descr);
    RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_WRITE(0, descr);
    RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_BY_CQ_WRITE(0, descr);
    RDD_LAQM_DEBUG_DESCRIPTOR_MAX_LATENCY_WRITE(0, descr);
    RDD_LAQM_DEBUG_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE(0, descr);

    for (i = 0; i < AQM_LQ_HISTOGRAM_SIZE; i++)
    {
        RDD_LAQM_DEBUG_DESCRIPTOR_HISTOGRAM_WRITE(0, descr, i);
    }
}

void rdd_pi2_reset_lq_histogram(void *laqm_desc_ptr)
{
    int i;

    for (i = 0; i < AQM_LQ_HISTOGRAM_SIZE; i++)
    {
        RDD_LAQM_DEBUG_DESCRIPTOR_HISTOGRAM_WRITE(0, laqm_desc_ptr, i);
    }
}

static bdmf_error_t _rdd_laqm_debug_descriptor_get(LAQM_DEBUG_DESCRIPTOR_STRUCT *debug_table, int clear, LAQM_DEBUG_DESCRIPTOR_STRUCT *descr)
{
    int i;

    RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_TOTAL_READ(descr->packets_total, debug_table);
    RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_READ(descr->packets_remarked, debug_table);
    RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_BY_CQ_READ(descr->packets_remarked_by_cq, debug_table);
    RDD_LAQM_DEBUG_DESCRIPTOR_MAX_LATENCY_READ(descr->max_latency, debug_table);
    RDD_LAQM_DEBUG_DESCRIPTOR_CUMULATIVE_LATENCY_READ(descr->cumulative_latency, debug_table);

    for (i = 0; i < AQM_LQ_HISTOGRAM_SIZE; i++)
    {
        RDD_LAQM_DEBUG_DESCRIPTOR_HISTOGRAM_READ(descr->histogram[i], debug_table, i);
    }

    if (clear)
    {
        _laqm_reset_debug_descriptor(debug_table);
    }

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_laqm_debug_descriptor_get(tm_identifier_e tm_identity, int clear, LAQM_DEBUG_DESCRIPTOR_STRUCT *descr)
{
    LAQM_DEBUG_DESCRIPTOR_STRUCT *debug_table =
        (LAQM_DEBUG_DESCRIPTOR_STRUCT *)schedulers_info[tm_identity].laqm_debug_table_p;

    if (debug_table)
        return _rdd_laqm_debug_descriptor_get(debug_table, clear, descr);
    else
    {
        memset(descr, 0x00, sizeof(LAQM_DEBUG_DESCRIPTOR_STRUCT));
        return BDMF_ERR_PARM;
    }
}

void rdd_laqm_debug_init(void)
{
    LAQM_DEBUG_DESCRIPTOR_STRUCT *descr;
    tm_identifier_e tm_identity;

    for (tm_identity = TM_START; tm_identity <= TM_MAX; tm_identity++)
    {
        descr = (LAQM_DEBUG_DESCRIPTOR_STRUCT *)schedulers_info[tm_identity].laqm_debug_table_p;
        if (descr)
        {
            _laqm_reset_debug_descriptor(descr);
        }
    }
}
#endif

#endif
