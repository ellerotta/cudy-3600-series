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


#ifndef _RDD_PI2_H
#define _RDD_PI2_H

#include "rdd.h"

#if defined(XRDP_PI2)

/* The PI2 timer is called each 1000 usec but it processes each time only 1/16 of the queues.
 * This means Tupdate is 16 times of the constant i.e. 16msec
 */
#define PI2_TIMER_DURATION_US    1000 /* in us */

typedef struct rdd_pi2_queue_descriptor
{
    uint32_t probability;
    uint32_t packet_select_probability;
    uint16_t q_delay;
    uint16_t prev_q_delay;
#ifdef XRDP_DUALQ_L4S
    int dual_queue;
    uint32_t laqm_probability;
    uint32_t laqm_packet_select_probability;
#endif
} rdd_pi2_queue_descriptor_t;

typedef enum
{
    RDD_PI2_RTT_MAX_30MS,
    RDD_PI2_RTT_MAX_50MS,
    RDD_PI2_RTT_MAX_100MS,
#define RDD_PI2_RTT_MAX_DEFAULT  RDD_PI2_RTT_MAX_30MS
} rdd_pi2_rtt_max_t;


typedef enum
{
    RDD_PI2_TARGET_DELAY_5MS,
    RDD_PI2_TARGET_DELAY_10MS,
    RDD_PI2_TARGET_DELAY_15MS,
    RDD_PI2_TARGET_DELAY_20MS,
#define RDD_PI2_TARGET_DELAY_DEFAULT  RDD_PI2_TARGET_DELAY_5MS
} rdd_pi2_target_delay_t;

typedef enum
{
    RDD_PI2_GROUP_PON,
    RDD_PI2_GROUP_ETH,
    RDD_PI2_GROUP_SQ,
} rdd_pi2_group_t;


#ifdef PI2_IN_REPORTING_TASK
bdmf_error_t rdd_pi2_use_reporting(void);
#endif
bdmf_error_t rdd_pi2_enable(uint16_t queue_index, bdmf_boolean dual_queue);
bdmf_error_t rdd_pi2_disable(uint16_t queue_index, bdmf_boolean dual_queue);
bdmf_error_t rdd_pi2_cfg(rdd_pi2_group_t group, rdd_pi2_rtt_max_t rtt_max, rdd_pi2_target_delay_t target_delay);
bdmf_error_t rdd_pi2_cfg_get(rdd_pi2_group_t group, rdd_pi2_rtt_max_t *p_rtt_max, rdd_pi2_target_delay_t *p_target_delay);
bdmf_error_t rdd_pi2_queue_qd_get(uint16_t queue_index, rdd_pi2_queue_descriptor_t *qd);
void rdd_pi2_reset_cq_histogram(void *pi2_calc_desc_ptr);
void rdd_pi2_reset_lq_histogram(void *laqm_desc_ptr);
void get_pi2_calc_descs(rdd_pi2_group_t group, void *pi2_calc_desc_ptr[]);
#ifdef TM_C_CODE
void *get_pi2_calc_desc_ptr(tm_identifier_e tm_identity);
#if defined(XRDP_DUALQ_L4S)
void rdd_laqm_cfg_get(int *coupling_factor, int *lq_ecn_mask);
void get_laqm_descs(rdd_pi2_group_t group, void *laqm_desc_ptr[]);
#endif
#else
void *pi2_probability_calc_address_from_group(rdd_pi2_group_t group);
#endif
#endif

#endif /* _RDD_PI2_H */
