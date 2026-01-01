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


#include "rdd_buffer_congestion_mgt.h"
#include "rdd_runner_proj_defs.h"
#include "rdp_drv_rnr.h"
#include "XRDP_AG.h"
#include "xrdp_drv_qm_ag.h"
#include "rdp_drv_qm.h"

extern uintptr_t xrdp_virt2phys(const ru_block_rec *ru_block, uint8_t addr_idx);

#define THREAD_ASYNC_WAKEUP_REQUEST(x)      (((x) << 4) + 9)
#define QM_FPM_USR_GRP_CNT_ENTRY_BYTE_SIZE  32

#ifdef BUFFER_CONGESTION_MGT
static bdmf_error_t _rdd_buffer_cong_mgt_get_cfg_array_address(fpm_ug_id_e ug, uint32_t **p_cfg_array)
{
    if (ug == FPM_DS_UG)
    {
        *p_cfg_array = RDD_DS_BUFFER_CONG_MGT_CFG_ADDRESS_ARR;
    }
    else if (ug == FPM_US_UG)
    {
        *p_cfg_array = RDD_US_BUFFER_CONG_MGT_CFG_ADDRESS_ARR;
    }
    else
    {
        return BDMF_ERR_NOT_SUPPORTED;
    }
    return BDMF_ERR_OK;
}

static void set_thresholds(fpm_ug_id_e ug, bdmf_boolean enable)
{
    uint32_t *cmcb_addr;
    uint8_t start_queue = 0, end_queue = 0;
    qm_fpm_ug_thr fpm_ug_thr = {};

#ifndef TM_C_CODE
    int runner_idx;
#endif
    uint16_t flush_cfg_addr, flush_enable_addr;
    uint16_t flush_wakeup;
    int has_service_queues = 0;
    uint8_t fw_state = 0;

    if (ug == FPM_DS_UG)
    {
        cmcb_addr = RDD_DS_BUFFER_CONG_MGT_CFG_ADDRESS_ARR;
#ifndef MULTIPLE_BBH_TX_LAN        
        start_queue = drv_qm_get_ds_start(0);
#endif        
        /* Service queues are also in DS UG */
        has_service_queues = (drv_qm_get_sq_start() != QM_ILLEGAL_QUEUE);
        /* FIXME:MULTIPLE_BBH_TX : fix when enable BUFFER_CONGESTION_MGT for 6888*/
        end_queue = has_service_queues ? drv_qm_get_sq_end() : drv_qm_get_ds_end(0);
/* TODO_TM_C_CODE in */
#ifndef TM_C_CODE
#ifndef BUFFER_CONGESTION_MGT_IN_REPORTING_TASK
        runner_idx = get_runner_idx(ds_tm_runner_image);
#else
        runner_idx = get_runner_idx(reporting_runner_image);
#endif
#endif
#ifndef MULTIPLE_BBH_TX_LAN   
        flush_cfg_addr = RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS;
        flush_enable_addr = RDD_DS_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS + FLUSH_CFG_ENABLE_ENTRY_ENABLE_FW_OFFSET;
        flush_wakeup = THREAD_ASYNC_WAKEUP_REQUEST(DS_TM_UPDATE_FIFO_THREAD_NUMBER);
#endif
    }
    else if (ug == FPM_US_UG)
    {
        cmcb_addr = RDD_US_BUFFER_CONG_MGT_CFG_ADDRESS_ARR;
        start_queue = drv_qm_get_us_start();
        end_queue = drv_qm_get_us_end();
/* TODO_TM_C_CODE in */
#ifndef TM_C_CODE
#ifndef BUFFER_CONGESTION_MGT_IN_REPORTING_TASK
        runner_idx = get_runner_idx(us_tm_runner_image);
#else
        runner_idx = get_runner_idx(reporting_runner_image);
#endif
#endif
        flush_cfg_addr = RDD_US_TM_FLUSH_CFG_FW_TABLE_ADDRESS;
        flush_enable_addr = RDD_US_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS + FLUSH_CFG_ENABLE_ENTRY_ENABLE_FW_OFFSET;
        flush_wakeup = THREAD_ASYNC_WAKEUP_REQUEST(US_TM_UPDATE_FIFO_THREAD_NUMBER);
    }
    else
        return;

    /* Get configured UG thresholds and calculate high and low thresholds */
    ag_drv_qm_fpm_ug_thr_get(ug, &fpm_ug_thr);
    if (!fpm_ug_thr.higher_thr)
        fpm_ug_thr.higher_thr = BUFFER_CONG_MGT_DEFAULT_UG_HIGH_THRESHOLD;

    RDD_BUFFER_CONG_MGT_DQM_NOT_EMPTY_ADDRESS_WRITE_G(
        xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, GLOBAL_CFG_DQM_NOT_EMPTY),
        cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_DQM_VALID_CTR_ADDRESS_WRITE_G(
        xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, DQM_VALID_COUNTER_COUNTER),
        cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_UG_COUNTER_ADDRESS_WRITE_G(
        xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, FPM_USR_GRP_CNT) + ug*QM_FPM_USR_GRP_CNT_ENTRY_BYTE_SIZE,
        cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_UG_THRESHOLD_HIGH_WRITE_G(fpm_ug_thr.higher_thr*BUFFER_CONG_MGT_HIGH_THRESHOLD_PERCENT/100,
        cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_UG_THRESHOLD_LOW_WRITE_G(fpm_ug_thr.higher_thr*BUFFER_CONG_MGT_LOW_THRESHOLD_PERCENT/100,
        cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_START_QUEUE_WRITE_G(start_queue, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_END_QUEUE_WRITE_G(end_queue, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_FLUSH_CFG_ADDRESS_WRITE_G(flush_cfg_addr, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_FLUSH_ENABLE_ADDRESS_WRITE_G(flush_enable_addr, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_FLUSH_WAKEUP_WRITE_G(flush_wakeup, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_NUM_PDS_TO_FLUSH_WRITE_G(BUFFER_CONG_MGT_NUM_PDS_TO_FLUSH, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_TIMER_DURATION_WRITE_G(BUFFER_CONG_MGT_TIMER_DURATION_US, cmcb_addr, 0);

    /* Update UG counter register address and low threshold in the flush_cfg area */
    if (ug == FPM_DS_UG)
    {
        RDD_FLUSH_CFG_ENTRY_UG_COUNTER_ADDRESS_WRITE_G(
            xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, FPM_USR_GRP_CNT) + ug*QM_FPM_USR_GRP_CNT_ENTRY_BYTE_SIZE,
            RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
        RDD_FLUSH_CFG_ENTRY_UG_THRESHOLD_LOW_WRITE_G(
            fpm_ug_thr.higher_thr*BUFFER_CONG_MGT_LOW_THRESHOLD_PERCENT/100,
            RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
    }
    else
    {
        RDD_FLUSH_CFG_ENTRY_UG_COUNTER_ADDRESS_WRITE_G(
            xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, FPM_USR_GRP_CNT) + ug*QM_FPM_USR_GRP_CNT_ENTRY_BYTE_SIZE,
            RDD_US_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
        RDD_FLUSH_CFG_ENTRY_UG_THRESHOLD_LOW_WRITE_G(
            fpm_ug_thr.higher_thr*BUFFER_CONG_MGT_LOW_THRESHOLD_PERCENT/100,
            RDD_US_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
    }

    /* Take care of service queues if necessary */
/* TODO_TM_C_CODE in */
#ifndef TM_C_CODE
    if (has_service_queues)
    {
        int sq_runner_idx = get_runner_idx(service_queues_runner_image);

        fw_state = (1 << BUFFER_CONG_MGT_FW_STATE_HAS_SQ);

        RDD_BUFFER_CONG_MGT_SQ_START_QUEUE_WRITE_G(drv_qm_get_sq_start(), cmcb_addr, 0);

        if (runner_idx != sq_runner_idx)
        {
            /* Service queue scheduler is in a different core. Use DMA to flush config area */
            fw_state |= (1 << BUFFER_CONG_MGT_FW_STATE_USE_DMA_FOR_SQ);

            RDD_BUFFER_CONG_MGT_SQ_FLUSH_CFG_ADDRESS_WRITE_G(
                (sq_runner_idx << 16) | (RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS >> 3), cmcb_addr, 0);
            RDD_BUFFER_CONG_MGT_SQ_FLUSH_ENABLE_ADDRESS_WRITE_G(
                (sq_runner_idx << 16) | (RDD_SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_ADDRESS >> 3),
                cmcb_addr, 0);
            RDD_BUFFER_CONG_MGT_SQ_FLUSH_WAKEUP_WRITE_G((sq_runner_idx << 16) | 0x8000, cmcb_addr, 0);
            RDD_BUFFER_CONG_MGT_SQ_FLUSH_WAKEUP_VALUE_WRITE_G(SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER, cmcb_addr, 0);
        }
        else
        {
            /* Service queue scheduler is in the same core. Update flush config area directly */
            RDD_BUFFER_CONG_MGT_SQ_FLUSH_CFG_ADDRESS_WRITE_G(RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS,
                cmcb_addr, 0);
            RDD_BUFFER_CONG_MGT_SQ_FLUSH_ENABLE_ADDRESS_WRITE_G(
                RDD_SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_ADDRESS + FLUSH_CFG_ENABLE_ENTRY_ENABLE_FW_OFFSET,
                cmcb_addr, 0);
            RDD_BUFFER_CONG_MGT_SQ_FLUSH_WAKEUP_WRITE_G(
                THREAD_ASYNC_WAKEUP_REQUEST(SERVICE_QUEUES_UPDATE_FIFO_THREAD_NUMBER), cmcb_addr, 0);
        }

        RDD_FLUSH_CFG_ENTRY_UG_COUNTER_ADDRESS_WRITE_G(
            xrdp_virt2phys(&RU_BLK(QM), 0) + RU_REG_OFFSET(QM, FPM_USR_GRP_CNT) + ug*QM_FPM_USR_GRP_CNT_ENTRY_BYTE_SIZE,
            RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
        RDD_FLUSH_CFG_ENTRY_UG_THRESHOLD_LOW_WRITE_G(
            fpm_ug_thr.higher_thr*BUFFER_CONG_MGT_LOW_THRESHOLD_PERCENT/100,
            RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
    }
#endif
    RDD_BUFFER_CONG_MGT_FW_STATE_WRITE_G(fw_state, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_ENABLED_WRITE_G(enable, cmcb_addr, 0);
}

/* API to RDPA level */
#ifndef BUFFER_CONGESTION_MGT_IN_REPORTING_TASK
bdmf_error_t rdd_buffer_congestion_mgt_init(fpm_ug_id_e ug)
{
    int runner_idx;
    int thread;
    int rc = BDMF_ERR_OK;

    if (ug == FPM_DS_UG)
    {
#ifdef MULTIPLE_BBH_TX_LAN        
        return BDMF_ERR_NOT_SUPPORTED;
#else
        runner_idx = get_runner_idx(ds_tm_runner_image);
        thread = DS_TM_BUFFER_CONG_MGT_THREAD_NUMBER;
#endif        
    }
    else if (ug == FPM_US_UG)
    {
        runner_idx = get_runner_idx(us_tm_runner_image);
        thread = US_TM_BUFFER_CONG_MGT_THREAD_NUMBER;
    }
    else
    {
        return BDMF_ERR_NOT_SUPPORTED;
    }

    /* disable at initialization */
    set_thresholds(ug, 0);

    /* Make sure the timer configured before the cpu weakeup */
    WMB();

    rc = ag_drv_rnr_regs_cfg_cpu_wakeup_set(runner_idx, thread);

    return rc;
}
#else /* BUFFER_CONGESTION_MGT_IN_REPORTING_TASK */
bdmf_error_t rdd_buffer_congestion_mgt_init(fpm_ug_id_e ug)
{
    int rc = BDMF_ERR_OK;

#ifdef MULTIPLE_BBH_TX_LAN        
    if (ug == FPM_DS_UG)
    {
        return BDMF_ERR_NOT_SUPPORTED;
    }
#endif        

    if (ug != FPM_DS_UG && ug != FPM_US_UG)
    {
        return BDMF_ERR_NOT_SUPPORTED;
    }

    /* disable at initialization */
    set_thresholds(ug, 0);

    return rc;
}
#endif

bdmf_error_t rdd_buffer_congestion_mgt_enable(fpm_ug_id_e ug, bdmf_boolean enable)
{
    uint32_t *cmcb_addr;
    bdmf_error_t rc;

    rc = _rdd_buffer_cong_mgt_get_cfg_array_address(ug, &cmcb_addr);
    if (rc)
        return rc;

    set_thresholds(ug, enable);

    return BDMF_ERR_OK;
}

/* RDD debug functions */
/* Read debug statistics */
bdmf_error_t rdd_buffer_congestion_mgt_stat_get(fpm_ug_id_e ug, rdd_buffer_congestion_mgt_stat *stats)
{
    uint32_t *cmcb_addr;
    uint8_t fw_state;
    bdmf_error_t rc;

    rc = _rdd_buffer_cong_mgt_get_cfg_array_address(ug, &cmcb_addr);
    if (rc)
        return rc;

    RDD_BUFFER_CONG_MGT_CONG_DETECTED_READ_G(stats->congestion_detected, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_CONG_CLEARED_READ_G(stats->congestion_cleared, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_CONG_DETECTION_TIME_READ_G(stats->run_time, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_CONG_QUEUE_IDX_READ_G(stats->cong_queue_idx, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_CONG_QUEUE_OCCUPANCY_READ_G(stats->cong_queue_occupancy, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_FW_STATE_READ_G(fw_state, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_ENABLED_READ_G(stats->is_enabled, cmcb_addr, 0);
    stats->is_congested = (fw_state & 1);
    return BDMF_ERR_OK;
}

/* Read thresholds */
bdmf_error_t rdd_buffer_congestion_mgt_thresholds_get(fpm_ug_id_e ug, uint32_t *p_high_threshold, uint32_t *p_low_threshold)
{
    uint32_t *cmcb_addr;
    bdmf_error_t rc;

    rc = _rdd_buffer_cong_mgt_get_cfg_array_address(ug, &cmcb_addr);
    if (rc)
        return rc;

    RDD_BUFFER_CONG_MGT_UG_THRESHOLD_HIGH_READ_G(*p_high_threshold, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_UG_THRESHOLD_LOW_READ_G(*p_low_threshold, cmcb_addr, 0);
    return BDMF_ERR_OK;
}

/* Set thresholds */
bdmf_error_t rdd_buffer_congestion_mgt_thresholds_set(fpm_ug_id_e ug, uint32_t high_threshold, uint32_t low_threshold)
{
    uint32_t *cmcb_addr;
    bdmf_error_t rc;

    rc = _rdd_buffer_cong_mgt_get_cfg_array_address(ug, &cmcb_addr);
    if (rc)
        return rc;
    if (high_threshold <= low_threshold)
        return BDMF_ERR_PARM;
    if (ug == FPM_DS_UG)
    {
        RDD_FLUSH_CFG_ENTRY_UG_THRESHOLD_LOW_WRITE_G(low_threshold, RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
#ifndef TM_C_CODE
        RDD_FLUSH_CFG_ENTRY_UG_THRESHOLD_LOW_WRITE_G(low_threshold, RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
#endif
    }
    else
    {
        RDD_FLUSH_CFG_ENTRY_UG_THRESHOLD_LOW_WRITE_G(low_threshold, RDD_US_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR, 0);
    }
    RDD_BUFFER_CONG_MGT_UG_THRESHOLD_HIGH_WRITE_G(high_threshold, cmcb_addr, 0);
    RDD_BUFFER_CONG_MGT_UG_THRESHOLD_LOW_WRITE_G(low_threshold, cmcb_addr, 0);
    return BDMF_ERR_OK;
}

/* Read num PDs to flush */
bdmf_error_t rdd_buffer_congestion_mgt_num_pds_get(fpm_ug_id_e ug, uint8_t *num_pds)
{
    uint32_t *cmcb_addr;
    bdmf_error_t rc;

    rc = _rdd_buffer_cong_mgt_get_cfg_array_address(ug, &cmcb_addr);
    if (rc)
        return rc;

    RDD_BUFFER_CONG_MGT_NUM_PDS_TO_FLUSH_READ_G(*num_pds, cmcb_addr, 0);
    return BDMF_ERR_OK;
}

/* Set num PDs to flush */
bdmf_error_t rdd_buffer_congestion_mgt_num_pds_set(fpm_ug_id_e ug, uint8_t num_pds)
{
    uint32_t *cmcb_addr;
    bdmf_error_t rc;

    rc = _rdd_buffer_cong_mgt_get_cfg_array_address(ug, &cmcb_addr);
    if (rc)
        return rc;
    RDD_BUFFER_CONG_MGT_NUM_PDS_TO_FLUSH_WRITE_G(num_pds, cmcb_addr, 0);
    return BDMF_ERR_OK;
}

#endif
