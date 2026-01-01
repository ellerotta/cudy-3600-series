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

#ifndef _SCHEDULING_H
#define _SCHEDULING_H

#include "rdd.h"
#include "rdd_defs.h"
#include "rdd_basic_scheduler.h"
#include "rdp_platform.h"
#include "rdpa_rate_limit.h"

#define SCHEDULING

#define ALL_CORES 0xff

typedef struct
{
    bdmf_boolean rl_en;
    uint8_t rl_index;
    uint32_t rl_rate;
} rdd_tm_rl_info;

typedef struct
{
    uint16_t queue_index;
    uint8_t queue_weight;
    rdd_tm_rl_info queue_rl;
    uint8_t queue_bit_mask;
} rdd_tm_queue_info;

typedef struct
{
    bdmf_boolean enable;
    rdd_tm_rl_info sched_rl;
    uint8_t sched_index;
    uint8_t dwrr_offset;
    uint32_t cs_scheduler_slot;
    uint32_t cs_scheduler_basic;
    rdd_tm_queue_info queue_info[MAX_NUM_OF_QUEUES_IN_SCHED];
} rdd_tm_info;

/* API to RDPA level */
void rdd_set_queue_enable(uint32_t qm_queue_index, bdmf_boolean enable);

/*XXX: change to AUTO */
/* API to scheduler module */
bdmf_error_t rdd_scheduling_scheduler_block_cfg(rdpa_traffic_dir dir, int16_t channel_id, uint16_t qm_queue_index, 
    rdd_scheduling_queue_descriptor_t *scheduler_cfg, bdmf_boolean type, uint8_t dwrr_offset, bdmf_boolean enable, rdpa_port_type port_type);

/*XXX: change to AUTO */
/* API to rate limiter module */
bdmf_error_t rdd_scheduling_queue_rate_limiter_remove(rdpa_traffic_dir dir, uint16_t qm_queue_index, uint8_t is_complex_rl);

/* Internal RDD init function */
void rdd_bbh_queue_init(void);
bdmf_error_t rdd_ds_budget_allocation_timer_set(void);
bdmf_error_t rdd_us_budget_allocation_timer_set(void);
bdmf_error_t rdd_tm_epon_cfg(void);
bdmf_error_t rdd_scheduling_flush_timer_set(void);
bdmf_error_t rdd_tx_queue_drop_count_get(uint16_t queue_index, uint32_t *p_packets, uint32_t *p_bytes, bdmf_boolean reset);

/* TM debug */
void rdd_tm_debug_get(rdpa_traffic_dir dir, uint8_t bbh_queue, rdd_tm_info *info);
void rdd_tm_debug_bs_get(rdpa_traffic_dir dir, uint8_t basic_scheduler_index, rdd_tm_info *info);
bdmf_boolean rdd_tm_is_cs_exist(rdpa_traffic_dir dir, uint8_t bbh_queue);
void rdd_scheduler_wake_up_bbh(rdpa_traffic_dir dir, uint8_t scheduler_index, uint8_t block_type, uint8_t tm_core, uint8_t complex_rate_limit);
void rdd_scheduler_wake_up_bbh_init_data_structure(void);


#if defined(OPERATION_MODE_PRV) || defined(G9991_FC)
/* API for g9991 vport to physical port mapping */
int rdd_g9991_vport_to_emac_mapping_cfg(rdd_rdd_vport vport, rdpa_emac emac);
int rdd_g9991_control_sid_set(rdd_rdd_vport vport, rdpa_emac emac);
#ifdef OPERATION_MODE_PRV
void rdd_g9991_system_port_set(rdpa_emac emac);
int rdd_g9991_ingress_congestion_flow_control_enable(bbh_id_e bbh_id, bdmf_boolean enable);
void rdd_g9991_single_fragment_enable_cfg(bdmf_boolean enable);
#endif
#endif

#endif
