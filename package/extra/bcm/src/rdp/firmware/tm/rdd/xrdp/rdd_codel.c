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

#ifdef XRDP_CODEL

#include "rdd.h"
#include "rdd_codel.h"
#include "rdd_ag_ds_tm.h"
#ifdef CODEL_ON_SERVICE_QUEUES
#include "rdd_ag_service_queues.h"
#endif
#include "rdd_ag_us_tm.h"
#include "rdd_ag_processing.h"
#include "rdp_drv_qm.h"

/* New projects use AQM_QUEUE_TABLE instead of CODEL_QUEUE_TABLE */
/* A few defines for legacy support */
#if !defined(RDD_US_TM_AQM_QUEUE_TABLE_SIZE) && !defined(RDD_PON_TM_AQM_QUEUE_TABLE_SIZE)
#define RDD_US_TM_AQM_QUEUE_TABLE_ADDRESS_ARR  RDD_US_TM_CODEL_QUEUE_TABLE_ADDRESS_ARR
#define RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_READ_G RDD_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_READ_G
#define RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_WRITE_G RDD_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_WRITE_G
#define RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_READ_G RDD_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_READ_G
#define RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_WRITE_G RDD_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_WRITE_G
#endif

#if !defined(RDD_DS_TM_AQM_QUEUE_TABLE_SIZE) && !defined(RDD_ETH_TM_AQM_QUEUE_TABLE_SIZE)
#define RDD_DS_TM_AQM_QUEUE_TABLE_ADDRESS_ARR  RDD_DS_TM_CODEL_QUEUE_TABLE_ADDRESS_ARR
#endif

#if !defined(RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_SIZE)
#define RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR  RDD_SERVICE_QUEUES_CODEL_QUEUE_TABLE_ADDRESS_ARR
#endif

#if !defined(RDD_AQM_ENABLE_TABLE_SIZE)
#define rdd_ag_processing_aqm_enable_table_set rdd_ag_processing_codel_enable_table_set
#define rdd_ag_processing_aqm_enable_table_get rdd_ag_processing_codel_enable_table_get
#define rdd_ag_processing_aqm_num_queues_get rdd_ag_processing_codel_num_queues_get
#define rdd_ag_processing_aqm_num_queues_set rdd_ag_processing_codel_num_queues_set
#endif

#define THREAD_ASYNC_WAKEUP_REQUEST(x)                          (((x) << 4) + 9)

static int rdd_codel_initialized;

static void _rdd_codel_ds_qd_clear(uint8_t core, uint8_t rel_queue_index)
{
    AQM_QUEUE_DESCRIPTOR_STRUCT *ptr = &((RDD_DS_TM_AQM_QUEUE_TABLE_PTR(core))->entry[rel_queue_index]);
    RDD_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_WRITE(0, ptr);
    RDD_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_WRITE(0, ptr);
}

static void _rdd_codel_ds_qd_get(uint8_t core, uint8_t rel_queue_index,
    uint16_t *p_drop_interval, uint16_t *p_window_ts)
{
    AQM_QUEUE_DESCRIPTOR_STRUCT *ptr = &((RDD_DS_TM_AQM_QUEUE_TABLE_PTR(core))->entry[rel_queue_index]);
    RDD_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_READ(*p_window_ts, ptr);
    RDD_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_READ(*p_drop_interval, ptr);
}

/* Initialize CODEL */
static void _rdd_codel_init(void)
{
    /* Initialize { bias, slope } table */
    /* interval 2 */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_0, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 0);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_0, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 0);
    /* interval [3-4] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_1, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 1);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_1, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 1);
    /* interval [5-8] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_2, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 2);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_2, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 2);
    /* interval [9-16] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_3, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 3);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_3, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 3);
    /* interval [17-32] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_4, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 4);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_4, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 4);
    /* interval [33-64] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_5, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 5);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_5, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 5);
    /* interval [65-128] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_6, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 6);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_6, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 6);
    /* interval [129-256] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_7, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 7);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_7, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 7);
    /* interval [257-512] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_8, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 8);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_8, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 8);
    /* interval [513-1024] */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_9, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 9);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_9, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 9);
    /* interval >1024 */
    RDD_CODEL_BIAS_SLOPE_BIAS_WRITE_G(CODEL_BIAS_10, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 10);
    RDD_CODEL_BIAS_SLOPE_SLOPE_WRITE_G(CODEL_SLOPE_10, RDD_CODEL_BIAS_SLOPE_TABLE_ADDRESS_ARR, 10);

    /* flush ptr  us/ds/sq */
#if !defined(MULTIPLE_BBH_TX_LAN) || defined(MULTIPLE_TM_ON_RNR_CORE)
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_CFG_PTR_WRITE_G(RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS, RDD_DS_TM_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_ENABLE_PTR_WRITE_G(RDD_DS_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS, RDD_DS_TM_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_TASK_WAKEUP_VALUE_WRITE_G(THREAD_ASYNC_WAKEUP_REQUEST(DS_TM_UPDATE_FIFO_THREAD_NUMBER), RDD_DS_TM_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
#else
    /* Use DS_TM_x_UPDATE_FIFO_THREAD_NUMBER to detect the existence of other TM DS cores */
#ifdef DS_TM_0_UPDATE_FIFO_THREAD_NUMBER
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_CFG_PTR_WRITE(RDD_DS_TM_0_FLUSH_CFG_FW_TABLE_ADDRESS,
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_runner_image)));
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_ENABLE_PTR_WRITE(RDD_DS_TM_0_FLUSH_CFG_ENABLE_TABLE_ADDRESS,
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_runner_image)));
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_TASK_WAKEUP_VALUE_WRITE(THREAD_ASYNC_WAKEUP_REQUEST(DS_TM_0_UPDATE_FIFO_THREAD_NUMBER),
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_runner_image)));
#endif
#ifdef DS_TM_1_UPDATE_FIFO_THREAD_NUMBER
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_CFG_PTR_WRITE(RDD_DS_TM_1_FLUSH_CFG_FW_TABLE_ADDRESS,
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_1_runner_image)));
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_ENABLE_PTR_WRITE(RDD_DS_TM_1_FLUSH_CFG_ENABLE_TABLE_ADDRESS,
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_1_runner_image)));
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_TASK_WAKEUP_VALUE_WRITE(THREAD_ASYNC_WAKEUP_REQUEST(DS_TM_1_UPDATE_FIFO_THREAD_NUMBER),
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_1_runner_image)));
#endif
#ifdef DS_TM_2_UPDATE_FIFO_THREAD_NUMBER
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_CFG_PTR_WRITE(RDD_DS_TM_2_FLUSH_CFG_FW_TABLE_ADDRESS,
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_2_runner_image)));
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_ENABLE_PTR_WRITE(RDD_DS_TM_2_FLUSH_CFG_ENABLE_TABLE_ADDRESS,
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_2_runner_image)));
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_TASK_WAKEUP_VALUE_WRITE(THREAD_ASYNC_WAKEUP_REQUEST(DS_TM_2_UPDATE_FIFO_THREAD_NUMBER),
        RDD_DS_TM_CODEL_DROP_DESCRIPTOR_PTR(get_runner_idx(ds_tm_2_runner_image)));
#endif
#endif

    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_CFG_PTR_WRITE_G(RDD_US_TM_FLUSH_CFG_FW_TABLE_ADDRESS, RDD_US_TM_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_ENABLE_PTR_WRITE_G(RDD_US_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS, RDD_US_TM_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_TASK_WAKEUP_VALUE_WRITE_G(THREAD_ASYNC_WAKEUP_REQUEST(US_TM_UPDATE_FIFO_THREAD_NUMBER), RDD_US_TM_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);

#if (!defined(BCM6846) && !defined(BCM6878) && !defined(TM_C_CODE))
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_CFG_PTR_WRITE_G(RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS, RDD_SERVICE_QUEUES_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_ENABLE_PTR_WRITE_G(RDD_SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_ADDRESS, RDD_SERVICE_QUEUES_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
    RDD_CODEL_DROP_DESCRIPTOR_FLUSH_TASK_WAKEUP_VALUE_WRITE_G(
        THREAD_ASYNC_WAKEUP_REQUEST(SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER), RDD_SERVICE_QUEUES_CODEL_DROP_DESCRIPTOR_ADDRESS_ARR, 0);
#endif
}

/* Enable codel on queue */
bdmf_error_t rdd_codel_enable(uint16_t queue_index)
{
#ifndef PD_TS_INSERTION_BY_HW
    uint32_t queue_enable_bits = 0;
    uint32_t queue_enable_entry = queue_index / 32;
    uint16_t aqm_queues;
#else
    qm_q_context q_context = {};
#endif
    uint8_t rel_queue_index;
    bdmf_error_t rc;

    RDD_BTRACE("qm_queue_index = %d\n", queue_index);

    /* Initialize on 1st call */
    if (!rdd_codel_initialized)
    {
        _rdd_codel_init();
        rdd_codel_initialized = 1;
    }

#ifndef PD_TS_INSERTION_BY_HW
    /* Enable on the processing side first. processing will start pushing timestamps in PDs, there is no harm in it */
    rc = rdd_ag_processing_aqm_enable_table_get(queue_enable_entry, &queue_enable_bits);
    if ((queue_enable_bits & (1 << (queue_index & 0x1f))) != 0)
        return BDMF_ERR_ALREADY;
    queue_enable_bits |= (1 << (queue_index & 0x1f));
    rc = rc ? rc : rdd_ag_processing_aqm_enable_table_set(queue_enable_entry, queue_enable_bits);
    if (rc != BDMF_ERR_OK)
        return rc;

    rc = rdd_ag_processing_aqm_num_queues_get(&aqm_queues);
    ++aqm_queues;
    rc = rc ? rc : rdd_ag_processing_aqm_num_queues_set(aqm_queues);
    if (rc != BDMF_ERR_OK)
    {
        queue_enable_bits &= ~(1 << (queue_index & 0x1f));
        rdd_ag_processing_aqm_enable_table_set(queue_enable_entry, queue_enable_bits);
        return rc;
    }
#else
    /* Timestamp is inserted by the h/w. Set timestamp resolution profile */
    rc = ag_drv_qm_q_context_get(queue_index, &q_context);
    q_context.timestamp_res_profile = QM_TIMESTAMP_RES_PROFILE_1MS;
    rc = rc ? rc : ag_drv_qm_q_context_set(queue_index, &q_context);
    if (rc != BDMF_ERR_OK)
        return rc;
#endif

    /* Enable on the scheduler side */
#ifdef CODEL_ON_SERVICE_QUEUES
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        RDD_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_WRITE_G(0, RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_WRITE_G(0, RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        rc = rdd_ag_service_queues_scheduling_queue_table_codel_dropping_set(rel_queue_index, 0);
        rc = rc ? rc : rdd_ag_service_queues_scheduling_queue_table_codel_enable_set(rel_queue_index, 1);
        rc = rc ? rc : rdd_ag_service_queues_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
    }
    else
#endif
    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        /* TODO DS_MULTI_CORE fix when enable codel */
        rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);
        _rdd_codel_ds_qd_clear(drv_qm_get_runner_idx(queue_index), rel_queue_index);
        rc = rdd_ag_ds_tm_scheduling_queue_table_codel_dropping_set(rel_queue_index, 0);
        rc = rc ? rc : rdd_ag_ds_tm_scheduling_queue_table_codel_enable_set(rel_queue_index, 1);
        rc = rc ? rc : rdd_ag_ds_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_us_start();
        RDD_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_WRITE_G(0, RDD_US_TM_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_WRITE_G(0, RDD_US_TM_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        rc = rdd_ag_us_tm_scheduling_queue_table_codel_dropping_set(rel_queue_index, 0);
        rc = rc ? rc : rdd_ag_us_tm_scheduling_queue_table_codel_enable_set(rel_queue_index, 1);
        rc = rc ? rc : rdd_ag_us_tm_scheduling_queue_table_aqm_enable_set(rel_queue_index, 1);
    }
    else
    {
        RDD_BTRACE("codel is not supported on queue %d\n", queue_index);
        rc = BDMF_ERR_NOT_SUPPORTED;
    }

    if (rc != BDMF_ERR_OK)
    {
        rdd_codel_disable(queue_index);
        return rc;
    }

    return BDMF_ERR_OK;
}

bdmf_error_t rdd_codel_disable(uint16_t queue_index)
{
#ifndef PD_TS_INSERTION_BY_HW
    uint32_t queue_enable_bits = 0;
    uint32_t queue_enable_entry = queue_index / 32;
    uint16_t aqm_queues;
#endif
    uint8_t first_queue_index;

    RDD_BTRACE("qm_queue_index = %d\n", queue_index);

#ifndef PD_TS_INSERTION_BY_HW
    rdd_ag_processing_aqm_enable_table_get(queue_enable_entry, &queue_enable_bits);
    if ((queue_enable_bits & (1 << (queue_index & 0x1f))) == 0)
        return BDMF_ERR_ALREADY;
#endif

    /* Disable everything unconditionally. It really shouldn't fail.
       Parameter errors will be detected by rdd_codel_enable */
    /* Disable on the scheduler side */
#ifdef CODEL_ON_SERVICE_QUEUES
    if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        first_queue_index = drv_qm_get_sq_start();
        rdd_ag_service_queues_scheduling_queue_table_codel_dropping_set(queue_index - first_queue_index, 0);
        rdd_ag_service_queues_scheduling_queue_table_aqm_enable_set(queue_index - first_queue_index, 0);
        rdd_ag_service_queues_scheduling_queue_table_codel_enable_set(queue_index - first_queue_index, 0);
    }
    else
#endif
    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        /* TODO DS_MULTI_CORE fix when enable codel */
        first_queue_index = drv_qm_get_ds_start(queue_index);
        rdd_ag_ds_tm_scheduling_queue_table_aqm_enable_set((queue_index - first_queue_index), 0);
        rdd_ag_ds_tm_scheduling_queue_table_codel_enable_set((queue_index - first_queue_index), 0);
        rdd_ag_ds_tm_scheduling_queue_table_codel_dropping_set((queue_index - first_queue_index), 0);
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        first_queue_index = drv_qm_get_us_start();
        rdd_ag_us_tm_scheduling_queue_table_aqm_enable_set((queue_index - first_queue_index), 0);
        rdd_ag_us_tm_scheduling_queue_table_codel_enable_set((queue_index - first_queue_index), 0);
        rdd_ag_us_tm_scheduling_queue_table_codel_dropping_set((queue_index - first_queue_index), 0);
    }
    else
    {
        RDD_BTRACE("codel is not supported on queue %d\n", queue_index);
    }

#ifndef PD_TS_INSERTION_BY_HW
    /* Disable on the processing side */
    rdd_ag_processing_aqm_enable_table_get(queue_enable_entry, &queue_enable_bits);
    queue_enable_bits &= ~(1 << (queue_index & 0x1f));
    rdd_ag_processing_aqm_enable_table_set(queue_enable_entry, queue_enable_bits);

    rdd_ag_processing_aqm_num_queues_get(&aqm_queues);
    --aqm_queues;
    rdd_ag_processing_aqm_num_queues_set(aqm_queues);
#endif

    return BDMF_ERR_OK;
}


bdmf_error_t rdd_codel_queue_qd_get(uint16_t queue_index, uint16_t *p_window_ts, uint16_t *p_drop_interval)
{
    uint8_t rel_queue_index;

    if (drv_qm_tx_queue_is_ds_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_ds_start(queue_index);
        _rdd_codel_ds_qd_get(drv_qm_get_runner_idx(queue_index), rel_queue_index,
            p_drop_interval, p_window_ts);
    }
    else if (drv_qm_tx_queue_is_us_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_us_start();
        RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_READ_G(*p_drop_interval,
            RDD_US_TM_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_READ_G(*p_window_ts,
            RDD_US_TM_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
    }
#ifdef CODEL_ON_SERVICE_QUEUES
    else if (drv_qm_tx_queue_is_service_queue(queue_index))
    {
        rel_queue_index = queue_index - drv_qm_get_sq_start();
        RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_DROP_INTERVAL_READ_G(*p_drop_interval,
            RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
        RDD_AQM_QUEUE_DESCRIPTOR_CODEL_QUEUE_DESCRIPTOR_WINDOW_TS_READ_G(*p_window_ts,
            RDD_SERVICE_QUEUES_AQM_QUEUE_TABLE_ADDRESS_ARR, rel_queue_index);
    }
#endif
    else
    {
        *p_window_ts = 0;
        *p_drop_interval = 0;
        return BDMF_ERR_NOT_SUPPORTED;
    }

    return BDMF_ERR_OK;
}

#endif /* XRDP_CODEL */
