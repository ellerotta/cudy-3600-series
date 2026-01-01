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

#include "bdmf_shell.h"
#include "rdd.h"
#include "rdd_codel.h"
#include "rdd_scheduling.h"
#include "rdp_drv_cntr.h"
#include "rdd_ag_ds_tm.h"
#if !(defined(TM_C_CODE))
#include "rdd_ag_service_queues.h"
#endif
#include "rdd_ag_us_tm.h"

static int _rdd_print_codel_debug_counters(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    uint16_t codel_back_to_drop = 0;
    uint16_t codel_recover = 0;
    uint16_t codel_empty_queue_recover = 0;
    uint16_t codel_max_seq_drops_us = 0;
    uint16_t codel_max_seq_drops_ds = 0;
#if !(defined(TM_C_CODE))
    uint16_t codel_max_seq_drops_sq = 0;
#endif

    drv_cntr_various_counter_get(COUNTER_CODEL_QUEUE_BACK_TO_DROP, &codel_back_to_drop);
    drv_cntr_various_counter_get(COUNTER_CODEL_QUEUE_RECOVER, &codel_recover);
    drv_cntr_various_counter_get(COUNTER_CODEL_EMPTY_QUEUE_RECOVER, &codel_empty_queue_recover);
    rdd_ag_us_tm_codel_drop_descriptor_max_seq_drops_get(&codel_max_seq_drops_us);
    rdd_ag_us_tm_codel_drop_descriptor_max_seq_drops_set(0);
    /* TODO DS_MULTI_CORE fix when enable codel */
    rdd_ag_ds_tm_codel_drop_descriptor_max_seq_drops_get(&codel_max_seq_drops_ds);
    rdd_ag_ds_tm_codel_drop_descriptor_max_seq_drops_set(0);
    /* TODO! Remove BCM_DSL_XRDP when move service queue to another implementation */
#if !defined(BCM_DSL_XRDP) && !(defined(TM_C_CODE))
    rdd_ag_service_queues_codel_drop_descriptor_max_seq_drops_get(&codel_max_seq_drops_sq);
    rdd_ag_service_queues_codel_drop_descriptor_max_seq_drops_set(0);
#endif

    bdmf_session_print(session, "CoDel debug counters:\n");
    bdmf_session_print(session, "\tnumber of times left dropping state briefly and returned: %u\n", codel_back_to_drop);
    bdmf_session_print(session, "\tnumber of times recovered from the dropping state: recent packet:%u  empty_queue:%u\n",
        codel_recover, codel_empty_queue_recover);
#if !(defined(TM_C_CODE))
    bdmf_session_print(session, "\tmax number of continuous drops: US=%u  DS=%u  SQ=%u\n", codel_max_seq_drops_us, codel_max_seq_drops_ds, codel_max_seq_drops_sq);
#endif

    return BDMF_ERR_OK;
}

static int _rdd_print_codel_queue_descriptor(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    uint32_t queue_id = (uint32_t)parm[0].value.unumber;
    uint16_t window_ts, drop_interval;
    int rc;
    rc = rdd_codel_queue_qd_get(queue_id, &window_ts, &drop_interval);
    if (rc != BDMF_ERR_OK)
        return rc;

    bdmf_session_print(session, "CoDel queue %u descriptor: drops: %u  next drop interval timestamp: %u\n",
        queue_id, drop_interval, window_ts);

    return BDMF_ERR_OK;
}

static int _rdd_codel_flush_enable(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    int enable = (int)parm[0].value.number;
    /* TODO DS_MULTI_CORE fix when enable codel */
    rdd_ag_ds_tm_codel_drop_descriptor_flush_enable_set(enable);
    rdd_ag_us_tm_codel_drop_descriptor_flush_enable_set(enable);
    /* TODO! Remove BCM_DSL_XRDP when move service queue to another implementation */
#if !defined(BCM_DSL_XRDP) && !(defined(TM_C_CODE))
    rdd_ag_service_queues_codel_drop_descriptor_flush_enable_set(enable);
#endif
    return BDMF_ERR_OK;
}

void rdd_codel_shell_cmds_init(bdmfmon_handle_t aqm_dir)
{
    BDMFMON_MAKE_CMD_NOPARM(aqm_dir, "cdldbc", "print Codel Debug Counters", _rdd_print_codel_debug_counters);
    BDMFMON_MAKE_CMD(aqm_dir, "cdlqd", "print Codel Queue Descriptor", _rdd_print_codel_queue_descriptor,
        BDMFMON_MAKE_PARM("queue_id", "QM queue id", BDMFMON_PARM_NUMBER, 0));
    BDMFMON_MAKE_CMD(aqm_dir, "cdlf", "Codel Flush enable: Yes/No", _rdd_codel_flush_enable,
        BDMFMON_MAKE_PARM_ENUM("enable", "Enable", bdmfmon_enum_bool_table, 0));
}
#endif /* XRDP_CODEL */
