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


#include "bdmf_shell.h"
#include "rdd.h"
#include "rdd_buffer_congestion_mgt.h"
#include "rdp_drv_qm.h"
#ifdef BUFFER_CONGESTION_MGT
static int _rdd_buffer_cong_mgt_enable(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    fpm_ug_id_e ug = (fpm_ug_id_e)parm[0].value.number;
    int enable = (int)parm[1].value.number;

    return rdd_buffer_congestion_mgt_enable(ug, enable);
}

static int _rdd_buffer_cong_mgt_print_stats(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    fpm_ug_id_e ug = (fpm_ug_id_e)parm[0].value.number;
    rdd_buffer_congestion_mgt_stat stats;
    int rc;

    rc = rdd_buffer_congestion_mgt_stat_get(ug, &stats);
    if (rc != BDMF_ERR_OK)
        return rc;

    bdmf_session_print(session, "Buffer congestion management stats: UG=%d\n", ug);
    bdmf_session_print(session, "\tenabled: %s  congested: %s\n",
        stats.is_enabled ? "YES" : "NO", stats.is_congested ? "YES" : "NO");
    bdmf_session_print(session, "\tCongestions detected: %u  cleared: %u  run_time: %uus\n",
        stats.congestion_detected, stats.congestion_cleared, stats.run_time);
    bdmf_session_print(session, "\tLast congested queue: index: %u  occupancy: %u\n",
        stats.cong_queue_idx, stats.cong_queue_occupancy);
    if (ug == FPM_DS_UG)
    {
#ifdef MULTIPLE_BBH_TX_LAN
        bdmf_session_print(session, "\tERR: need to add support for multiple BBH_TX");
#else
        int runner_idx = get_runner_idx(ds_tm_runner_image);
#ifndef TM_C_CODE /* TODO_TM_C_CODE*/
        int sq_runner_idx = get_runner_idx(service_queues_runner_image);

        bdmf_session_print(session,
            "\tcfg_area:%x@%d  flush_cfg:%x  flush_enable:%x  sq_flush_cfg:%x@%d  sq_flush_enable:%x\n",
            RDD_DS_BUFFER_CONG_MGT_CFG_ADDRESS_ARR[runner_idx], runner_idx,
            RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR[runner_idx],
            RDD_DS_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS_ARR[runner_idx],
            RDD_SERVICE_QUEUES_FLUSH_CFG_FW_TABLE_ADDRESS_ARR[sq_runner_idx], sq_runner_idx,
            RDD_SERVICE_QUEUES_FLUSH_CFG_ENABLE_TABLE_ADDRESS_ARR[sq_runner_idx]);
#else
        bdmf_session_print(session,
            "\tcfg_area:%x@%d  flush_cfg:%x  flush_enable:%x\n",
            RDD_DS_BUFFER_CONG_MGT_CFG_ADDRESS_ARR[runner_idx], runner_idx,
            RDD_DS_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR[runner_idx],
            RDD_DS_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS_ARR[runner_idx]);
#endif
#endif            
    }
    else
    {
        int runner_idx = get_runner_idx(us_tm_runner_image);
        bdmf_session_print(session,
            "\tcfg_area:%x@%d  flush_cfg:%x  flush_enable:%x\n",
            RDD_US_BUFFER_CONG_MGT_CFG_ADDRESS_ARR[runner_idx], runner_idx,
            RDD_US_TM_FLUSH_CFG_FW_TABLE_ADDRESS_ARR[runner_idx],
            RDD_US_TM_FLUSH_CFG_ENABLE_TABLE_ADDRESS_ARR[runner_idx]);
    }
    bdmf_session_print(session, "\tus_queue:%u-%u  ds_queue:%u-%u  sq:%u-%u\n",
        drv_qm_get_us_start(), drv_qm_get_us_end(),
#if !defined(MULTIPLE_BBH_TX_LAN) || defined(MULTIPLE_TM_ON_RNR_CORE)      
        drv_qm_get_ds_start(0), drv_qm_get_ds_end(0),
#else
        QM_QUEUE_DS_START, QM_QUEUE_DS_END,
#endif        
        drv_qm_get_sq_start(), drv_qm_get_sq_end());

    return BDMF_ERR_OK;
}

static int _rdd_buffer_cong_mgt_thresholds(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    fpm_ug_id_e ug = (fpm_ug_id_e)parm[0].value.number;
    uint32_t high_threshold;
    uint32_t low_threshold;
    uint8_t num_pds;
    int rc;

    if (bdmfmon_parm_is_set(session, 1))
    {
        if (!bdmfmon_parm_is_set(session, 2))
        {
            bdmf_session_print(session, "Can't set just 1 threshold, must set neither or both\n");
            return BDMF_ERR_PARM;
        }
        high_threshold = (uint16_t)parm[1].value.number;
        low_threshold = (uint16_t)parm[2].value.number;
        rc = rdd_buffer_congestion_mgt_thresholds_set(ug, high_threshold, low_threshold);
        if (rc != BDMF_ERR_OK)
            return rc;
    }
    if (bdmfmon_parm_is_set(session, 3))
    {
        num_pds = (uint8_t)parm[3].value.number;
        rc = rdd_buffer_congestion_mgt_num_pds_set(ug, num_pds);
        if (rc != BDMF_ERR_OK)
            return rc;
    }

    rc = rdd_buffer_congestion_mgt_thresholds_get(ug, &high_threshold, &low_threshold);
    rc = rc ? rc : rdd_buffer_congestion_mgt_num_pds_get(ug, &num_pds);
    if (rc != BDMF_ERR_OK)
        return rc;
    bdmf_session_print(session, "Buffer congestion management thresholds: UG=%d  high_threshold=%u  low_threshold=%u  num_PDs=%u\n",
        ug, high_threshold, low_threshold, num_pds);
    return BDMF_ERR_OK;
}

void rdd_buffer_cong_mgt_shell_cmds_init(bdmfmon_handle_t rdd_dir)
{
    static struct bdmfmon_enum_val ug_id_enum_table[] = {
        {"US", FPM_US_UG},
        {"DS", FPM_DS_UG},
        {NULL, 0},
    };

    BDMFMON_MAKE_CMD(rdd_dir, "bufcms", "Buffer Congestion Management: Print Stats", _rdd_buffer_cong_mgt_print_stats,
        BDMFMON_MAKE_PARM_ENUM("ug", "User Group", ug_id_enum_table, 0));
    BDMFMON_MAKE_CMD(rdd_dir, "bufcme", "Buffer Congestion Management: Enable/Disable", _rdd_buffer_cong_mgt_enable,
        BDMFMON_MAKE_PARM_ENUM("ug", "User Group", ug_id_enum_table, 0),
        BDMFMON_MAKE_PARM_ENUM("enable", "Enable", bdmfmon_enum_bool_table, 0));
    BDMFMON_MAKE_CMD(rdd_dir, "bufcmthr", "Buffer Congestion Management: Set/Get Thresholds", _rdd_buffer_cong_mgt_thresholds,
        BDMFMON_MAKE_PARM_ENUM("ug", "User Group", ug_id_enum_table, 0),
        BDMFMON_MAKE_PARM("high_threshold", "high_threshold", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL),
        BDMFMON_MAKE_PARM("low_threshold", "low_threshold", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL),
        BDMFMON_MAKE_PARM("num_pds_to_flush", "num_pds_to_flush", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
}
#endif
