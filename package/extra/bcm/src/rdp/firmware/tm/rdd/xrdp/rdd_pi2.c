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
#ifdef XRDP_PI2

#include "rdd.h"
#include "rdd_pi2.h"
#include "rdd_ag_ds_tm.h"
#ifdef PI2_ON_SERVICE_QUEUES
#include "rdd_ag_service_queues.h"
#endif
#include "rdd_ag_us_tm.h"
#include "rdd_ag_processing.h"
#include "xrdp_drv_rnr_regs_ag.h"
#include "rdp_drv_qm.h"
#ifdef L4S_LAQM_DEBUG
#include "rdd_laqm.h"
#endif

extern void *get_schedulers_info_probability_calc_desc_table_p(int tm_identity);
extern void *get_schedulers_info_laqm_desc_p(int tm_identity);
extern int get_tm_identity_from_qm_queue(uint32_t qm_queue);

#ifdef TM_C_CODE
#define RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR         RDD_SQ_TM_AQM_QUEUE_TABLE_ADDRESS_ARR
#define RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_PTR                 RDD_SQ_TM_AQM_QUEUE_TABLE_PTR
#define RDD_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR RDD_SQ_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR

#define RDD_SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_PTR           RDD_SQ_TM_AQM_QUEUE_TIMER_TABLE_PTR
#endif

/* PI2 parameter configuration */
typedef struct rdd_pi2_cfg_s
{
    rdd_pi2_rtt_max_t rtt_max;
    rdd_pi2_target_delay_t target_delay;
} rdd_pi2_cfg_t;

/* ALPHA, BETA multipliers */
typedef struct rdd_pi2_alpha_beta_s
{
    uint16_t alpha;
    uint16_t beta;
} rdd_pi2_alpha_beta_t;

static rdd_pi2_cfg_t rdd_pi2_cfg_per_group[] = {
    [RDD_PI2_GROUP_PON] = { RDD_PI2_RTT_MAX_DEFAULT, RDD_PI2_TARGET_DELAY_DEFAULT},
    [RDD_PI2_GROUP_ETH] = { RDD_PI2_RTT_MAX_DEFAULT, RDD_PI2_TARGET_DELAY_DEFAULT},
    [RDD_PI2_GROUP_SQ] = { RDD_PI2_RTT_MAX_DEFAULT, RDD_PI2_TARGET_DELAY_DEFAULT}
};

static const rdd_pi2_alpha_beta_t rdd_pi2_coeff_per_rtt_max[] = {
    /* The following numbers must match PI2_TIMER_PERIOD_US. They are currently set for Tupdate of 3.2msec */
    /* Both HW and SW insert timestamps in 1ms resolution */
    [RDD_PI2_RTT_MAX_30MS] = {  24, 671 },
    [RDD_PI2_RTT_MAX_50MS] = {  9, 403 },
    [RDD_PI2_RTT_MAX_100MS] = {  2, 201 }
};

/* convert from target delay enum to target delay msec */
static const uint8_t rdd_pi2_target_delay_msec[] = {
    [RDD_PI2_TARGET_DELAY_5MS]  = 5,
    [RDD_PI2_TARGET_DELAY_10MS] = 10,
    [RDD_PI2_TARGET_DELAY_15MS] = 15,
    [RDD_PI2_TARGET_DELAY_20MS] = 20
};

static int is_initialized;
static int ds_q_group_idx;
#ifdef PI2_ON_SERVICE_QUEUES
static int sq_q_group_idx;
#endif

#ifndef RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR
#define RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR RDD_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR
#endif
#ifndef RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR
#define RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR RDD_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR
#endif

int pi2_shares_ds_and_sq(void)
{
#if defined(PI2_ON_SERVICE_QUEUES)
    return (get_runner_idx(ds_tm_runner_image) == get_runner_idx(service_queues_runner_image));
#else
    return 0;
#endif
}

int pi2_shares_us_and_sq(void)
{
#if defined(PI2_ON_SERVICE_QUEUES)
    return (get_runner_idx(us_tm_runner_image) == get_runner_idx(service_queues_runner_image));
#else
    return 0;
#endif
}

int pi2_shares_ds_and_us(void)
{
    return (get_runner_idx(ds_tm_runner_image) == get_runner_idx(us_tm_runner_image));
}

static int _pi2_queue_to_group_core(uint16_t queue_index, rdd_pi2_group_t *p_group, uint8_t *p_rnr)
{
    *p_rnr = drv_qm_get_runner_idx(queue_index);
    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        *p_group = RDD_PI2_GROUP_ETH;
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        *p_group = RDD_PI2_GROUP_PON;
    }
#ifdef PI2_ON_SERVICE_QUEUES
    else if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        *p_group = RDD_PI2_GROUP_SQ;
    }
#endif
    else
    {
        return BDMF_ERR_NOT_SUPPORTED;
    }
    return BDMF_ERR_OK;
}

#ifndef TM_C_CODE
/* In some platforms the PI2_PROBABILITY_CALC_DESCRIPTOR is shared between pi2_groups.
 * These are the valid options:
 *    1. three instances: DS, US, SQ
 *    2. two instances: DS+SQ, US
 *    3. single instance: DS+SQ+US
 */
void *pi2_probability_calc_address_from_group(rdd_pi2_group_t group)
{
    uint32_t *probability_calc_desc_array;
    uint8_t pi2_core;

    /* coverity[dead_error_begin] */
    /* coverity[dead_error_condition] */
    if (group == RDD_PI2_GROUP_PON)
    {
#ifdef RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_SIZE
        probability_calc_desc_array = RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR;
#else
        probability_calc_desc_array = RDD_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR;
#endif
        pi2_core = get_runner_idx(us_tm_runner_image);
    }
    else if ((group == RDD_PI2_GROUP_ETH) || ((pi2_shares_ds_and_sq()) && (group == RDD_PI2_GROUP_SQ)))
    {
        /* Note that in case of multiple LAN BBH_TX (e.g., BCM96888), the same DS configuration
           applies to all DS cores. */
#ifdef RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_SIZE
        probability_calc_desc_array = RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR;
#else
        probability_calc_desc_array = RDD_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR;
#endif
        pi2_core = get_runner_idx(ds_tm_runner_image);
    }
#ifdef RDD_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR
    else if (group == RDD_PI2_GROUP_SQ)
    {
        probability_calc_desc_array = RDD_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_ADDRESS_ARR;
        pi2_core = get_runner_idx(service_queues_runner_image);
    }
#endif
    else
    {
        return NULL;
    }

    return (void *)DEVICE_ADDRESS(rdp_runner_core_addr[pi2_core] + probability_calc_desc_array[pi2_core]);
}
#endif

static int pi2_probability_calc_address_from_queue(uint16_t queue_index, void **p_calc)
{
#ifdef TM_C_CODE
    int tm_identity;

    tm_identity = get_tm_identity_from_qm_queue(queue_index);
    if (tm_identity == TM_ERROR)
        return BDMF_ERR_PARM;

    if (!p_calc)
        return BDMF_ERR_PARM;
    *p_calc = get_schedulers_info_probability_calc_desc_table_p(tm_identity);
#else
    rdd_pi2_group_t group;
    uint8_t rnr;
    int rc;

    rc = _pi2_queue_to_group_core(queue_index, &group, &rnr);
    if (rc)
        return rc;

    *p_calc = pi2_probability_calc_address_from_group(group);
#endif
        
    return BDMF_ERR_OK;
}

static int _pi2_aqm_qd_address(uint16_t queue_index, void **p_qd, void **p_qtd, uint8_t *p_rel_queue_idx)
{
    rdd_pi2_group_t group;
    uint8_t rnr;
    int rc;
    rc = _pi2_queue_to_group_core(queue_index, &group, &rnr);
    if (rc)
        return rc;
    switch (group)
    {
        case RDD_PI2_GROUP_PON:
            *p_rel_queue_idx = queue_index - drv_qm_get_us_start();
            if (p_qd)
                *p_qd = &((RDD_US_TM_AQM_QUEUE_TABLE_PTR(rnr))->entry[*p_rel_queue_idx]);
            if (p_qtd)
                *p_qtd = &((RDD_US_TM_AQM_QUEUE_TIMER_TABLE_PTR(rnr))->entry[*p_rel_queue_idx]);
            break;
        case RDD_PI2_GROUP_ETH:
            *p_rel_queue_idx = queue_index - drv_qm_get_ds_start(queue_index);
            if (p_qd)
                *p_qd = &((RDD_DS_TM_AQM_QUEUE_TABLE_PTR(rnr))->entry[*p_rel_queue_idx]);
            if (p_qtd)
                *p_qtd = &((RDD_DS_TM_AQM_QUEUE_TIMER_TABLE_PTR(rnr))->entry[*p_rel_queue_idx]);
            break;
#ifdef PI2_ON_SERVICE_QUEUES
        case RDD_PI2_GROUP_SQ:
            *p_rel_queue_idx = queue_index - drv_qm_get_sq_start();
            if (p_qd)
                *p_qd = &((RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_PTR(rnr))->entry[*p_rel_queue_idx]);
            if (p_qtd)
                *p_qtd = &((RDD_SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_PTR(rnr))->entry[*p_rel_queue_idx]);
            break;
#endif
        default:
            /* Can't happen */
            return BDMF_ERR_NOT_SUPPORTED;
            break;
    }
    return BDMF_ERR_OK;
}

#ifdef L4S_LAQM_DEBUG
void rdd_pi2_reset_cq_histogram(void *pi2_calc_desc_ptr)
{
    uint32_t i;

    for (i = 0; i < AQM_CQ_HISTOGRAM_SIZE; i++)
    {
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_HISTOGRAM_WRITE(0, pi2_calc_desc_ptr, i);
    }
}
#endif

static int _pi2_calc_descriptor_cfg(rdd_pi2_group_t group, void *probability_calc_desc_ptr)
{
    const rdd_pi2_alpha_beta_t *coeff = &rdd_pi2_coeff_per_rtt_max[rdd_pi2_cfg_per_group[group].rtt_max];
    uint8_t target_delay_msec = rdd_pi2_target_delay_msec[rdd_pi2_cfg_per_group[group].target_delay];
    int ticks_per_run = 0;

    RDD_BTRACE("setting up group %d address=%p\n", group, probability_calc_desc_ptr);

    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_ALPHA_COEFF_WRITE(coeff->alpha, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_BETA_COEFF_WRITE(coeff->beta, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_TARGET_DELAY_WRITE(target_delay_msec, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CALC_ITER_WRITE(0, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_TIMER_VALUE_WRITE(PI2_TIMER_PERIOD_US, probability_calc_desc_ptr);
#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PACKETS_TOTAL_WRITE(0, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PACKETS_DROPPED_WRITE(0, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_MAX_LATENCY_WRITE(0, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE(0, probability_calc_desc_ptr);
    rdd_pi2_reset_cq_histogram(probability_calc_desc_ptr);
#endif

    /* Configure the PI2 task period, if it is called from the budget allocater or reporting task.
     * NOTE0: The two modes are mutually exclusive, or both disabled.
     * NOTE1: In case of Budget Allocator and shared cores we might configure the same table again with the same values.
     *        There's no harm in that as long as DS_RATE_LIMITER_TIMER_PERIOD_IN_USEC==US_RATE_LIMITER_TIMER_PERIOD_IN_USEC.
     *        DS_RATE_LIMITER_TIMER_PERIOD_IN_USEC is assumed for SQ too.
     *        Budget allocator has four iterations per run so we divide by 4.
     * NOTE2: Reporting task uses shared DS/US/SQ.
     */
#if defined(PI2_IN_DS_TM_BUDGET_ALLOCATOR_TASK)
    if (group == RDD_PI2_GROUP_ETH)
    {
        ticks_per_run = PI2_TIMER_PERIOD_US / (DS_RATE_LIMITER_TIMER_PERIOD_IN_USEC / 4);
    }
#endif
#if defined(PI2_IN_US_TM_BUDGET_ALLOCATOR_TASK)
    if (group == RDD_PI2_GROUP_PON)
    {
        ticks_per_run = PI2_TIMER_PERIOD_US / (US_RATE_LIMITER_TIMER_PERIOD_IN_USEC / 4);
    }
#endif
#if defined(PI2_IN_SQ_TM_BUDGET_ALLOCATOR_TASK)
    if (group == RDD_PI2_GROUP_SQ)
    {
        ticks_per_run = PI2_TIMER_PERIOD_US / (DS_RATE_LIMITER_TIMER_PERIOD_IN_USEC / 4);
    }
#endif
#ifdef PI2_IN_REPORTING_TASK
    ticks_per_run = PI2_TIMER_PERIOD_US/GHOST_REPORTING_TIMER_INTERVAL_IN_USEC;
#endif
    if (ticks_per_run)
    {
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_TICKS_PER_RUN_WRITE(ticks_per_run, probability_calc_desc_ptr);
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_TICKS_LEFT_WRITE(1, probability_calc_desc_ptr);
    }

    /* There are extra fields that need to be filled up when PI2_CALC is shared between 2 or more of US, DS, SQ */
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_DS_START_ITER_WRITE(0xff, probability_calc_desc_ptr);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_SR_START_ITER_WRITE(0xff, probability_calc_desc_ptr);

    /* The following block is relevant only in cases where PI2 probability calculator is
       shared between US and/or DS and/or service queues */
    if (pi2_shares_ds_and_us() || pi2_shares_ds_and_sq() || pi2_shares_us_and_sq())
    {
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_US_AQM_Q_TABLE_WRITE(
            RDD_US_TM_AQM_QUEUE_TABLE_ADDRESS_ARR[get_runner_idx(us_tm_runner_image)],
            probability_calc_desc_ptr);
#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_US_AQM_Q_TIMER_TABLE_WRITE
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_US_AQM_Q_TIMER_TABLE_WRITE(
            RDD_US_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS_ARR[get_runner_idx(us_tm_runner_image)],
            probability_calc_desc_ptr);
#endif
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_DS_AQM_Q_TABLE_WRITE(
            RDD_DS_TM_AQM_QUEUE_TABLE_ADDRESS_ARR[get_runner_idx(ds_tm_runner_image)],
            probability_calc_desc_ptr);
#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_DS_AQM_Q_TIMER_TABLE_WRITE
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_DS_AQM_Q_TIMER_TABLE_WRITE(
            RDD_DS_TM_AQM_QUEUE_TIMER_TABLE_ADDRESS_ARR[get_runner_idx(ds_tm_runner_image)],
            probability_calc_desc_ptr);
#endif
        if (pi2_shares_ds_and_us() || pi2_shares_ds_and_sq())
        {
            ds_q_group_idx = (drv_qm_get_us_end() - drv_qm_get_us_start() + 1 + 15) / 16; /* 16 queues per iter */
            RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_DS_START_ITER_WRITE(ds_q_group_idx, probability_calc_desc_ptr);
        }

#ifdef PI2_ON_SERVICE_QUEUES
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_SR_AQM_Q_TABLE_WRITE(
            RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR[get_runner_idx(service_queues_runner_image)],
            probability_calc_desc_ptr);
#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_DS_AQM_Q_TIMER_TABLE_WRITE
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_SR_AQM_Q_TIMER_TABLE_WRITE(
            RDD_SERVICE_QUEUES_AQM_QUEUE_TIMER_TABLE_ADDRESS_ARR[get_runner_idx(service_queues_runner_image)],
            probability_calc_desc_ptr);
#endif
        if (pi2_shares_ds_and_sq())
            sq_q_group_idx = ds_q_group_idx + (drv_qm_get_ds_end(0) - drv_qm_get_ds_start(0) + 1 + 15) / 16; /* 16 queues per iter */
        else if (pi2_shares_us_and_sq())
            sq_q_group_idx = (drv_qm_get_us_end() - drv_qm_get_us_start() + 1 + 15) / 16; /* 16 queues per iter */
        RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_SR_START_ITER_WRITE(sq_q_group_idx, probability_calc_desc_ptr);
#endif
    }

    return BDMF_ERR_OK;
}

#if defined(PI2_IN_DEDICATED_TASK) || defined(PI2_IN_REPORTING_TASK)
/* Start PI2 timer task on all cores running the image */
static bdmf_error_t _pi2_start_calc_task(rdp_runner_image_e image, int thread)
{
    bdmf_error_t rc = BDMF_ERR_OK;
    int num_cores = 0;
    int i;

    for (i = 0; i < NUM_OF_RUNNER_CORES && !rc; ++i)
    {
        if (rdp_core_to_image_map[i] == image)
        {
            ++num_cores;
            rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(i, thread);
            RDD_BTRACE("PI2: started task %d on core %d for image %d. rc=%d\n", thread, i, image, rc);
        }
    }
    return num_cores ? rc : BDMF_ERR_NOENT;
}
#endif

/* This function configures the PI2 group table */
static bdmf_error_t config_pi2_desc(void)
{
    void *probability_calc_desc_ptr;
    int rc;

    /* Init PI2 in DS core */
    probability_calc_desc_ptr = RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(get_runner_idx(ds_tm_runner_image));
    rc = _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_ETH, probability_calc_desc_ptr);

#ifdef DS_TM_1_UPDATE_FIFO_THREAD_NUMBER
    probability_calc_desc_ptr = RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(get_runner_idx(ds_tm_1_runner_image));
    rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_ETH, probability_calc_desc_ptr);
#endif
#ifdef DS_TM_2_UPDATE_FIFO_THREAD_NUMBER
    probability_calc_desc_ptr = RDD_DS_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(get_runner_idx(ds_tm_2_runner_image));
    rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_ETH, probability_calc_desc_ptr);
#endif

    /* Init PI2 in US CORE (if it isn't th same as the DS core) */
    if (!pi2_shares_ds_and_us())
    {
        probability_calc_desc_ptr = RDD_US_TM_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(get_runner_idx(us_tm_runner_image));
        rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_PON, probability_calc_desc_ptr);
    }

#if defined(PI2_ON_SERVICE_QUEUES) && defined(RDD_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR)
    /* Init PI2 in SQ CORE (if it isn't the same as the DS/US cores) */
    if (!pi2_shares_ds_and_sq() && !pi2_shares_us_and_sq())
    {
        probability_calc_desc_ptr = RDD_SERVICE_QUEUES_PI2_PROBABILITY_CALC_DESCRIPTOR_PTR(get_runner_idx(service_queues_runner_image));
        rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_SQ, probability_calc_desc_ptr);
    }
#endif

#ifdef L4S_LAQM_DEBUG
    rdd_laqm_debug_init();
#endif

    return rc;
}

/* This function starts the periodic tasks that update the PI2 probability. */
static bdmf_error_t start_pi2_task(int use_reporting_timer)
{
    int rc = 0;

#ifdef PI2_IN_DEDICATED_TASK
    /* For platforms that run PI2 in a dedicated task */
    rc = _pi2_start_calc_task(ds_tm_runner_image, DS_TM_PI2_THREAD_NUMBER);
    if (rc)
        return rc;

    rc = _pi2_start_calc_task(us_tm_runner_image, US_TM_PI2_THREAD_NUMBER);
    if (rc)
        return rc;
#elif defined(PI2_IN_REPORTING_TASK)
    /* For platforms that call PI2 from the reporting task */
    if (!use_reporting_timer)
    {
        /* Ghost reporting task hasn't been started. Do it here */
        /* It is enough to enable once since the platforms that use reporting have a shared DS, US and SQ core */
        rc = _pi2_start_calc_task(us_tm_runner_image, REPORTING_THREAD_NUMBER);
        if (rc)
            return rc;
    }
#elif defined(PI2_IN_GENERIC_TIMER)
    /* TODO: In Gen6 TM C, currently the timer is enabled from the FW's TX
     *       task, so the compiler/linker won't remove the code. Once this
     *       is resolved enable it here.
     */
#else
    /* In platforms that call PI2 from generic timer or budget allocator - these tasks are always started anyway */
#endif

    return rc;
}

/* Internal pi2_init function.
   It can be called either explicitly from data_path_init-->rdd_pi2_use_reporting-->_pi2_init,
   or implicitly from rdd_pi2_queue_enable.
   In the explicit case it is called prior to the host reporting initialization.
   In this case pi2 piggibacks on the gost reporting time task and doesn't need its own timer.
   The two former cases are in assembly code, and there is a third one in C, as a part of the generic timer.
*/
bdmf_error_t _pi2_init(int use_reporting_timer)
{
    int rc;
    if (is_initialized)
        return 0;

    rc = config_pi2_desc();
    if (rc)
    {
        RDD_BTRACE("Failed to configure PI2 descriptor (%d)\n", rc);
        return rc;
    }

    /* Make sure the timer configured before the cpu weakeup */
    WMB();

    rc = start_pi2_task(use_reporting_timer);
    if (rc)
    {
        RDD_BTRACE("Failed to start PI2 task (%d)\n", rc);
    }

    return rc;
}

#ifdef PI2_IN_REPORTING_TASK
bdmf_error_t rdd_pi2_use_reporting(void)
{
    return _pi2_init(1);
}
#endif

#ifndef PD_TS_INSERTION_BY_HW
/* Set queue enable bit in the PROCESSING-side bitmask */
static bdmf_error_t _pi2_queue_enable_bit_set(uint16_t queue_index)
{
    uint32_t queue_enable_bits = 0;
    uint32_t queue_enable_entry = queue_index / 32;
    uint16_t aqm_queues;
    bdmf_error_t rc;

    /* set queue's bit in aqm enable queue bitmap */
    rc = rdd_ag_processing_aqm_enable_table_get(queue_enable_entry, &queue_enable_bits);
    if ((queue_enable_bits & (1 << (queue_index & 0x1f))) != 0)
        return BDMF_ERR_ALREADY;
    queue_enable_bits |= (1 << (queue_index & 0x1f));
    rc = rc ? rc : rdd_ag_processing_aqm_enable_table_set(queue_enable_entry, queue_enable_bits);
    if (rc != BDMF_ERR_OK)
        return rc;

#ifdef TM_C_CODE
    /* For SQ, set queue's bit in aqm enable SQ queue bitmap. It is limited to one 32 bit register */
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        uint32_t aqm_sq_bit, aqm_sq_bitmap;
        
        aqm_sq_bit = 1 << (queue_index - drv_qm_get_sq_start());
    
        rc = rdd_ag_service_queues_aqm_sq_bitmap_get(&aqm_sq_bitmap);
        if ((aqm_sq_bitmap & aqm_sq_bit) != 0)
            return BDMF_ERR_ALREADY;
        aqm_sq_bitmap |= aqm_sq_bit;
        rc = rc ? rc : rdd_ag_service_queues_aqm_sq_bitmap_set(aqm_sq_bitmap);
        if (rc != BDMF_ERR_OK)
            return rc;
    }
#endif

    /* increase aqm enabled queue count */
    rc = rdd_ag_processing_aqm_num_queues_get(&aqm_queues);
    ++aqm_queues;
    rc = rc ? rc : rdd_ag_processing_aqm_num_queues_set(aqm_queues);
    if (rc != BDMF_ERR_OK)
    {
        queue_enable_bits &= ~(1 << (queue_index & 0x1f));
        rdd_ag_processing_aqm_enable_table_set(queue_enable_entry, queue_enable_bits);
        return rc;
    }
    return rc;
}

/* Clear queue enable bit in the PROCESSING-side bitmask */
static bdmf_error_t _pi2_queue_enable_bit_clear(uint16_t queue_index)
{
    uint32_t queue_enable_bits = 0;
    uint32_t queue_enable_entry = queue_index / 32;
    uint16_t aqm_queues;

    rdd_ag_processing_aqm_enable_table_get(queue_enable_entry, &queue_enable_bits);
    if ((queue_enable_bits & (1 << (queue_index & 0x1f))) == 0)
        return BDMF_ERR_ALREADY;
    queue_enable_bits &= ~(1 << (queue_index & 0x1f));
    rdd_ag_processing_aqm_enable_table_set(queue_enable_entry, queue_enable_bits);

#ifdef TM_C_CODE
    /* For SQ, clear queue's bit in aqm sq bitmap. It is limited to a one 32 bit register */
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        uint32_t aqm_sq_bit, aqm_sq_bitmap;
        
        aqm_sq_bit = 1 << (queue_index - drv_qm_get_sq_start());
    
        rdd_ag_service_queues_aqm_sq_bitmap_get(&aqm_sq_bitmap);
        if ((aqm_sq_bitmap & aqm_sq_bit) == 0)
            return BDMF_ERR_ALREADY;
        aqm_sq_bitmap &= ~aqm_sq_bit;
        rdd_ag_service_queues_aqm_sq_bitmap_set(aqm_sq_bitmap);
    }
#endif

    rdd_ag_processing_aqm_num_queues_get(&aqm_queues);
    --aqm_queues;
    rdd_ag_processing_aqm_num_queues_set(aqm_queues);

    return BDMF_ERR_OK;
}
#endif /* #ifndef PD_TS_INSERTION_BY_HW */

/* Enable codel on queue */
bdmf_error_t rdd_pi2_enable(uint16_t queue_index, bdmf_boolean dual_queue)
{
    uint32_t queue_enable_bits;
    uint8_t rel_queue_index = 0;
    void *probability_calc_desc = NULL;
#ifdef PD_TS_INSERTION_BY_HW
    qm_q_context q_context = {};
#endif
    rdd_pi2_group_t group;
    uint8_t rnr;
    void *qd_address, *qtd_address;
    bdmf_error_t rc;

    if (!is_initialized)
    {
        rc = _pi2_init(0);
        if (rc)
            return rc;
    }

    RDD_BTRACE("qm_queue_index = %d\n", queue_index);

    rc = _pi2_queue_to_group_core(queue_index, &group, &rnr);
    rc = rc ? rc : pi2_probability_calc_address_from_queue(queue_index, &probability_calc_desc);
    if (rc != BDMF_ERR_OK)
        return rc;
    if (probability_calc_desc == NULL)
        return BDMF_ERR_INTERNAL;

#ifndef PD_TS_INSERTION_BY_HW
    /* If timestamp is inserted by the f/w, AQM queue is indicated by a bit in SRAM table.
       Set this bit here. */
    rc = _pi2_queue_enable_bit_set(queue_index);
#else
    /* Timestamp is inserted by the h/w. Set timestamp resolution profile */
    rc = ag_drv_qm_q_context_get(queue_index, &q_context);
    q_context.timestamp_res_profile = QM_TIMESTAMP_RES_PROFILE_1MS;
    rc = rc ? rc : ag_drv_qm_q_context_set(queue_index, &q_context);
#endif

    rc = rc ? rc : _pi2_aqm_qd_address(queue_index, &qd_address, &qtd_address, &rel_queue_index);
    if (rc != BDMF_ERR_OK)
        return rc;

    RDD_PI2_QUEUE_DESCRIPTOR_PROBABILITY_WRITE(0, qd_address);
    RDD_PI2_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_WRITE(0, qd_address);
#ifdef XRDP_DUALQ_L4S
    RDD_PI2_QUEUE_TIMER_DESCRIPTOR_DUAL_QUEUE_WRITE(dual_queue, qtd_address);
#endif

    /* Enable on the scheduler side */
    switch (group)
    {
        case RDD_PI2_GROUP_PON:
            rc = rdd_ag_us_tm_scheduling_queue_table_pi2_enable_set(rel_queue_index, 1);
            rc = rc ? rc : rdd_ag_us_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
            break;
        case RDD_PI2_GROUP_ETH:
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_PI2_ENABLE_WRITE(
                1, (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(drv_qm_get_runner_idx(queue_index)) + ((rel_queue_index) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
            /* NOTE: Currently always enabling AQM stats if AQM is enabled */
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_AQM_ENABLE_WRITE(
                1, (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(drv_qm_get_runner_idx(queue_index)) + ((rel_queue_index) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
            rel_queue_index += ds_q_group_idx * 16;
            break;
#ifdef PI2_ON_SERVICE_QUEUES
        case RDD_PI2_GROUP_SQ:
            rc = rdd_ag_service_queues_scheduling_queue_table_pi2_enable_set(rel_queue_index, 1);
            rc = rc ? rc : rdd_ag_service_queues_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
            rel_queue_index += sq_q_group_idx * 16;
            break;
#endif
        default:
            /* Can't happen */
            break;
    }

    if (rc != BDMF_ERR_OK)
    {
        rdd_pi2_disable(queue_index, dual_queue);
        return rc;
    }

    /* Enable periodic PI2 probability calculation for the queue */
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PI2_QUEUE_MASK_READ(queue_enable_bits,
        probability_calc_desc, rel_queue_index/16);
    queue_enable_bits |= 1 << (rel_queue_index & 0xf);
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PI2_QUEUE_MASK_WRITE(queue_enable_bits,
        probability_calc_desc, rel_queue_index/16);

#ifdef CONFIG_RUNNER_CPU_TX_INSERT_EH
    /* Enable PI2 CPU TX prepending of embedded header. Use absolute queue index */
    RDD_BYTES_2_BITS_READ_G(queue_enable_bits, RDD_CPU_TX_PI2_BITMAP_ADDRESS_ARR, queue_index/16);
    queue_enable_bits |= 1 << (queue_index & 0xf);
    RDD_BYTES_2_BITS_WRITE_G(queue_enable_bits, RDD_CPU_TX_PI2_BITMAP_ADDRESS_ARR, queue_index/16);
#endif
    return BDMF_ERR_OK;
}

bdmf_error_t rdd_pi2_disable(uint16_t queue_index, bdmf_boolean dual_queue)
{
    uint32_t queue_enable_bits;
    uint8_t rel_queue_index = 0;
    void *probability_calc_desc = NULL;
    rdd_pi2_group_t group;
    uint8_t rnr;
    int rc;

    RDD_BTRACE("qm_queue_index = %d\n", queue_index);

    rc = _pi2_queue_to_group_core(queue_index, &group, &rnr);
    rc = rc ? rc : pi2_probability_calc_address_from_queue(queue_index, &probability_calc_desc);
    rc = rc ? rc : _pi2_aqm_qd_address(queue_index, NULL, NULL, &rel_queue_index);
    if (rc != BDMF_ERR_OK)
        return rc;

#ifndef PD_TS_INSERTION_BY_HW
    /* If timestamp is inserted by the f/w, AQM queue is indicated by a bit in SRAM table.
       Clear this bit here. */
    rc = _pi2_queue_enable_bit_clear(queue_index);
    if (rc != BDMF_ERR_OK)
        return rc;
#endif

    /* Enable on the scheduler side */
    switch (group)
    {
        case RDD_PI2_GROUP_PON:
            rdd_ag_us_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 0);
            rdd_ag_us_tm_scheduling_queue_table_pi2_enable_set(rel_queue_index, 0);
            break;
        case RDD_PI2_GROUP_ETH:
            rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_AQM_ENABLE_WRITE(
                0, (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(drv_qm_get_runner_idx(queue_index)) + ((rel_queue_index) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
            RDD_SCHEDULING_QUEUE_DESCRIPTOR_PI2_ENABLE_WRITE(
                0, (uint8_t *)RDD_DS_TM_SCHEDULING_QUEUE_TABLE_PTR(drv_qm_get_runner_idx(queue_index)) + ((rel_queue_index) * sizeof(SCHEDULING_QUEUE_DESCRIPTOR_STRUCT)));
            rel_queue_index += ds_q_group_idx * 16;
            break;
#ifdef PI2_ON_SERVICE_QUEUES
        case RDD_PI2_GROUP_SQ:
            rdd_ag_service_queues_scheduling_queue_table_aqm_enable_set(rel_queue_index, 0);
            rdd_ag_service_queues_scheduling_queue_table_pi2_enable_set(rel_queue_index, 0);
            rel_queue_index += sq_q_group_idx * 16;
            break;
#endif
        default:
            /* Can't happen */
            break;
    }

    /* Disable periodic PI2 probability calculation for the queue */
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PI2_QUEUE_MASK_READ(queue_enable_bits,
        probability_calc_desc, rel_queue_index/16);
    queue_enable_bits &= ~(1 << (rel_queue_index & 0xf));
    RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PI2_QUEUE_MASK_WRITE(queue_enable_bits,
        probability_calc_desc, rel_queue_index/16);

#ifdef CONFIG_RUNNER_CPU_TX_INSERT_EH
    /* Disable PI2 CPU TX prepending of embedded header. Use absolute queue index */
    RDD_BYTES_2_BITS_READ_G(queue_enable_bits, RDD_CPU_TX_PI2_BITMAP_ADDRESS_ARR, queue_index/16);
    queue_enable_bits &= ~(1 << (queue_index & 0xf));
    RDD_BYTES_2_BITS_WRITE_G(queue_enable_bits, RDD_CPU_TX_PI2_BITMAP_ADDRESS_ARR, queue_index/16);
#endif

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_pi2_queue_qd_get(uint16_t queue_index, rdd_pi2_queue_descriptor_t *qd)
{
    uint8_t rel_queue_index;
    void *qd_address, *qtd_address;
    int rc;

    rc = _pi2_aqm_qd_address(queue_index, &qd_address, &qtd_address, &rel_queue_index);
    if (rc)
    {
        memset(qd, 0, sizeof(*qd));
        return rc;
    }

    RDD_PI2_QUEUE_DESCRIPTOR_PROBABILITY_READ(qd->probability, qd_address);
    RDD_PI2_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_READ(qd->packet_select_probability, qd_address);
    RDD_PI2_QUEUE_TIMER_DESCRIPTOR_PREV_Q_DELAY_READ(qd->prev_q_delay, qtd_address);
#ifdef RDD_PI2_QUEUE_TIMER_DESCRIPTOR_Q_DELAY_READ
    RDD_PI2_QUEUE_TIMER_DESCRIPTOR_Q_DELAY_READ(qd->q_delay, qtd_address);
#endif

#ifdef XRDP_DUALQ_L4S
    RDD_PI2_QUEUE_TIMER_DESCRIPTOR_DUAL_QUEUE_READ(qd->dual_queue, qtd_address);
    if (qd->dual_queue)
    {
        void *laqm_qd_address = (uint32_t *)qd_address + 1;
        RDD_LAQM_QUEUE_DESCRIPTOR_PROBABILITY_READ(qd->laqm_probability, laqm_qd_address);
        RDD_LAQM_QUEUE_DESCRIPTOR_PACKET_SELECT_PROBABILITY_READ(qd->laqm_packet_select_probability, laqm_qd_address);
    }
    else
    {
        qd->laqm_probability = 0;
        qd->laqm_packet_select_probability = 0;
    }
#endif

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_pi2_cfg(rdd_pi2_group_t group, rdd_pi2_rtt_max_t rtt_max, rdd_pi2_target_delay_t target_delay)
{
    rdd_pi2_cfg_t cfg_org = rdd_pi2_cfg_per_group[group];
    void *probability_calc_desc = NULL;
    bdmf_error_t rc = BDMF_ERR_OK;

#ifdef PI2_ON_SERVICE_QUEUES
    if (group != RDD_PI2_GROUP_PON && group != RDD_PI2_GROUP_ETH && group != RDD_PI2_GROUP_SQ)
#else
    if (group != RDD_PI2_GROUP_PON && group != RDD_PI2_GROUP_ETH)
#endif
    {
        return BDMF_ERR_NOT_SUPPORTED;
    }

    rdd_pi2_cfg_per_group[group].rtt_max = rtt_max;
    rdd_pi2_cfg_per_group[group].target_delay = target_delay;

    if (group == RDD_PI2_GROUP_ETH)
    {
#ifdef MULTIPLE_BBH_TX_LAN
#ifdef DS_TM_0_UPDATE_FIFO_THREAD_NUMBER
        rc = pi2_probability_calc_address_from_queue(QM_QUEUE_DS_TX_0_START, &probability_calc_desc);
        rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_ETH, probability_calc_desc);
#endif
#ifdef DS_TM_1_UPDATE_FIFO_THREAD_NUMBER
        rc = pi2_probability_calc_address_from_queue(QM_QUEUE_DS_TX_1_START, &probability_calc_desc);
        rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_ETH, probability_calc_desc);
#endif
#ifdef DS_TM_2_UPDATE_FIFO_THREAD_NUMBER
        rc = pi2_probability_calc_address_from_queue(QM_QUEUE_DS_TX_2_START, &probability_calc_desc);
        rc = rc ? rc : _pi2_calc_descriptor_cfg(RDD_PI2_GROUP_ETH, probability_calc_desc);
#endif
#else
        rc = pi2_probability_calc_address_from_queue(drv_qm_get_ds_start(0), &probability_calc_desc);
        rc = rc ? rc : _pi2_calc_descriptor_cfg(group, probability_calc_desc);
#endif
    }
    else if (group == RDD_PI2_GROUP_PON)
    {
        rc = pi2_probability_calc_address_from_queue(drv_qm_get_us_start(), &probability_calc_desc);
        rc = rc ? rc : _pi2_calc_descriptor_cfg(group, probability_calc_desc);
    }
#ifdef PI2_ON_SERVICE_QUEUES
    else if (group == RDD_PI2_GROUP_SQ)
    {
        rc = pi2_probability_calc_address_from_queue(drv_qm_get_sq_start(), &probability_calc_desc);
        rc = rc ? rc : _pi2_calc_descriptor_cfg(group, probability_calc_desc);
    }
#endif

    if (rc)
    {
        rdd_pi2_cfg_per_group[group] = cfg_org;
        return rc;
    }

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_pi2_cfg_get(rdd_pi2_group_t group, rdd_pi2_rtt_max_t *p_rtt_max, rdd_pi2_target_delay_t *p_target_delay)
{
    *p_rtt_max = rdd_pi2_cfg_per_group[group].rtt_max;
    *p_target_delay = rdd_pi2_cfg_per_group[group].target_delay;
    return BDMF_ERR_OK;
}

void get_pi2_calc_descs(rdd_pi2_group_t group, void *pi2_calc_desc_ptr[])
{
#ifndef TM_C_CODE
    memset(pi2_calc_desc_ptr, 0x00, sizeof(pi2_calc_desc_ptr[0]) * TM_MAX_GROUP);

    pi2_calc_desc_ptr[0] = pi2_probability_calc_address_from_group(group);
#else
    tm_identifier_e tm_identity;
    int i;

    memset(pi2_calc_desc_ptr, 0x00, sizeof(pi2_calc_desc_ptr[0]) * TM_MAX_GROUP);

    if (group == RDD_PI2_GROUP_PON)
    {
        pi2_calc_desc_ptr[0] = get_schedulers_info_probability_calc_desc_table_p(TM_PON_DSL_AE);
    }
    else if (group == RDD_PI2_GROUP_ETH)
    {
        for (i = 0, tm_identity = TM_ETH_START; tm_identity <= TM_ETH_END; i++, tm_identity++)
        {
            pi2_calc_desc_ptr[i] = get_schedulers_info_probability_calc_desc_table_p(tm_identity);
        }
    }
    else if (group == RDD_PI2_GROUP_SQ)
    {
        pi2_calc_desc_ptr[0] = get_schedulers_info_probability_calc_desc_table_p(TM_ETH_SQ);
    }
#endif
}

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
void get_laqm_descs(rdd_pi2_group_t group, void *laqm_desc_ptr[])
{
    tm_identifier_e tm_identity;
    int i;

    memset(laqm_desc_ptr, 0x00, sizeof(laqm_desc_ptr[0]) * TM_MAX_GROUP);

    if (group == RDD_PI2_GROUP_PON)
    {
        laqm_desc_ptr[0] = get_schedulers_info_laqm_desc_p(TM_PON_DSL_AE);
    }
    else if (group == RDD_PI2_GROUP_ETH)
    {
        for (i = 0, tm_identity = TM_ETH_START; tm_identity <= TM_ETH_END; i++, tm_identity++)
        {
            laqm_desc_ptr[i] = get_schedulers_info_laqm_desc_p(tm_identity);
        }
    }
    else if (group == RDD_PI2_GROUP_SQ)
    {
        laqm_desc_ptr[0] = get_schedulers_info_laqm_desc_p(TM_ETH_SQ);
    }
}
#endif

#endif
