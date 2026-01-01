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

#include "bdmf_shell.h"
#include "rdd.h"
#include "rdd_pi2.h"
#include "xrdp_drv_qm_ag.h"
#include "rdd_scheduling.h"
#include "rdd_pi2.h"
#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
#include "rdd_laqm.h"
#include "rdp_drv_qm.h"
#endif

/* Group mode allows configuring PI2 differently for DS/US/SQ. 
 * When disabled, it means the configuration is the same for all.
 * This switch only controls the API. The actual configuration requires some work.
 */
#undef PI2_GROUP_CONFIGURATION

#define ROUND_AND_DIVIDE(a, b) ((b) ? ((a) + (b)/2) / (b) : 0)
#define PI2_TICKS_TO_MS(ticks)  ROUND_AND_DIVIDE((ticks)*1024UL, 1000)  /* PI2 ticks are in units of 1.024 ms */
#define LAQM_TICKS_TO_MS(ticks) ROUND_AND_DIVIDE((ticks)*128UL, 1000)   /* LAQM ticks are in units of 128 us */

extern int get_tm_identity_from_qm_queue(uint32_t qm_queue);

#if defined(PI2_GROUP_CONFIGURATION) || defined(RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE)
static struct bdmfmon_enum_val pi2_group_enum_table[] = {
    {"pon", RDD_PI2_GROUP_PON},
    {"eth", RDD_PI2_GROUP_ETH},
    {"sq", RDD_PI2_GROUP_SQ},
    {NULL, 0},
};
#endif

static struct bdmfmon_enum_val pi2_rtt_max_enum_table[] = {
    {"30ms", RDD_PI2_RTT_MAX_30MS},
    {"50ms", RDD_PI2_RTT_MAX_50MS},
    {"100ms", RDD_PI2_RTT_MAX_100MS},
    {NULL, 0},
};

static struct bdmfmon_enum_val pi2_target_delay_enum_table[] = {
    {"5ms", RDD_PI2_TARGET_DELAY_5MS},
    {"10ms", RDD_PI2_TARGET_DELAY_10MS},
    {"15ms", RDD_PI2_TARGET_DELAY_15MS},
    {"20ms", RDD_PI2_TARGET_DELAY_20MS},
    {NULL, 0},
};

#define RTT_MAX_PARAM_NAME          "rtt_max"
#define TARGET_DELAY_PARAM_NAME     "target_delay"
#if defined(XRDP_DUALQ_L4S)
#define COUPLING_FACTOR_PARAM_NAME  "coupling_factor"
#define LQ_ECN_MASK_PARAM_NAME      "lq_ecn_mask"
#endif

#if defined(XRDP_DUALQ_L4S)
static struct bdmfmon_enum_val lq_ecn_mask_enum_table[] = {
    { "ECT1", 1 },
    { "ECT0_ECT1", 3 },
};
#endif


bdmf_error_t drv_tx_counters_read(uint16_t queue_index, uint32_t *p_pkt_cntr, uint64_t *p_byte_cntr);

bdmf_error_t rdd_pi2_check_group(rdd_pi2_group_t group)
{
    if ((group == RDD_PI2_GROUP_PON) || (group == RDD_PI2_GROUP_ETH))
    {
        return BDMF_ERR_OK;
    }
#ifdef PI2_ON_SERVICE_QUEUES
    else if (group == RDD_PI2_GROUP_SQ)
    {
        return BDMF_ERR_OK;
    }
#endif

    return BDMF_ERR_NOT_SUPPORTED;
}

#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE
/* Calculate the percentile of a given histogram.
 * percentile = 0..100
 * The value is returned x10 in order to print a fractional number.
 */
static uint32_t calc_percentile_x10(uint32_t histogram[], uint32_t histogram_size, uint8_t percentile)
{
    uint64_t total, subtotal, target;
    int i;

    total = 0;
    for (i = 0; i < histogram_size; i++)
        total += histogram[i];

    /* multiply with percentile with ceiling */
    target = (percentile * total + (100 - 1)) / 100;

    if (target == 0)
        return 0;

    subtotal = 0;
    for (i = 0; i < histogram_size; i++)
    {
        /* If there are more samples than required then assume that the samples are evenly spaced in the bin, to avoid taking the entire bin */
        if ((subtotal + histogram[i]) >= target)
            return (10 * (i + 1)) - (((subtotal + histogram[i] - target) * 10) / histogram[i]);

        subtotal += histogram[i];
    }

    return histogram_size;
}

static void get_cq_histogram(void *pi2_calc_desc_ptr[], uint32_t histogram[])
{
    uint32_t value, i, j;

    for (i = 0; i < TM_MAX_GROUP; i++)
    {
        if (!pi2_calc_desc_ptr[i])
            continue;

        for (j = 0; j < AQM_CQ_HISTOGRAM_SIZE; j++)
        {
            RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_HISTOGRAM_READ(value, pi2_calc_desc_ptr[i], j);
            histogram[j] += value;
        }
    }
}

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
static void get_lq_histogram(void *laqm_desc_ptr[], uint32_t histogram[])
{
    uint32_t value, i, j;

    for (i = 0; i < TM_MAX_GROUP; i++)
    {
        if (!laqm_desc_ptr[i])
            continue;

        for (j = 0; j < AQM_LQ_HISTOGRAM_SIZE; j++)
        {
            RDD_LAQM_DEBUG_DESCRIPTOR_HISTOGRAM_READ(value, laqm_desc_ptr[i], j);
            histogram[j] += value;
        }
    }
}
#endif

static void print_histogram(bdmf_session_handle session, uint32_t histogram[], uint32_t histogram_size)
{
    uint32_t i;

    for (i = 0; i < histogram_size; i++)
    {
        bdmf_session_print(session, " %d", histogram[i]);
    }
    bdmf_session_print(session, "\n");
}

#define RDD_PI2_PRINT_CQ_STATS  (1)
#define RDD_PI2_PRINT_LQ_STATS  (2)

static int _rdd_aqm_stats(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms, int print_xq)
{
    uint32_t pi2_packets_total = 0, pi2_max_latency = 0, pi2_cumulative_latency = 0, pi2_packets_dropped = 0;
    uint32_t max_latency_x10, avg_latency_x10, pi2_p5_x10, pi2_p95_x10, value;
    void *pi2_calc_desc_ptr[TM_MAX_GROUP] = {};
    uint32_t pi2_histogram[AQM_CQ_HISTOGRAM_SIZE] = {};
    int print_lq, print_cq;
    rdd_pi2_group_t group;
    long num_secs = 0;
    int i;
#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
    uint32_t laqm_packets_total = 0, laqm_packets_remarked = 0, laqm_packets_remarked_by_cq = 0;
    uint32_t laqm_cumulative_latency = 0, laqm_max_latency = 0;
    uint32_t laqm_p5_x10, laqm_p95_x10;
    void *laqm_desc_ptr[TM_MAX_GROUP] = {};
    uint32_t laqm_histogram[AQM_LQ_HISTOGRAM_SIZE] = {};
#endif

    print_cq = !!(print_xq & RDD_PI2_PRINT_CQ_STATS);
    print_lq = !!(print_xq & RDD_PI2_PRINT_LQ_STATS);

    group = (rdd_pi2_group_t)parm[0].value.number;

    if (print_cq)
    {
        get_pi2_calc_descs(group, pi2_calc_desc_ptr);
        if (!pi2_calc_desc_ptr[0])
            return BDMF_ERR_NOT_SUPPORTED;
    }

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
    if (print_lq)
    {
        get_laqm_descs(group, laqm_desc_ptr);
        if (!laqm_desc_ptr[0])
            return BDMF_ERR_NOT_SUPPORTED;
    }
#endif

    /* Logic:
     * No samples specified              - dump stats
     * samples specified and it is zero  - zero stats (no dump)
     * samples specified and is non-zero - zero stats, wait x seconds, dump stats
     */

    /* Clear */
    if (n_parms == 2)
    {
        num_secs = parm[1].value.number;

        for (i = 0; i < TM_MAX_GROUP; i++)
        {
            if (print_cq && pi2_calc_desc_ptr[i])
            {
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PACKETS_TOTAL_WRITE(0, pi2_calc_desc_ptr[i]);
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_MAX_LATENCY_WRITE(0, pi2_calc_desc_ptr[i]);
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PACKETS_DROPPED_WRITE(0, pi2_calc_desc_ptr[i]);
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE(0, pi2_calc_desc_ptr[i]);
                rdd_pi2_reset_cq_histogram(pi2_calc_desc_ptr[i]);
            }

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
            if (print_lq && laqm_desc_ptr[i])
            {
                RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_TOTAL_WRITE(0, laqm_desc_ptr[i]);
                RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_WRITE(0, laqm_desc_ptr[i]);
                RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_BY_CQ_WRITE(0, laqm_desc_ptr[i]);
                RDD_LAQM_DEBUG_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE(0, laqm_desc_ptr[i]);
                RDD_LAQM_DEBUG_DESCRIPTOR_MAX_LATENCY_WRITE(0, laqm_desc_ptr[i]);
                rdd_pi2_reset_lq_histogram(laqm_desc_ptr[i]);
            }
#endif
        }
    }

    /* Wait */
    if (num_secs)
        bdmf_msleep(1000 * num_secs);

    /* Collect statistics and print them */
    if ((n_parms == 1) || num_secs)
    {
        /* Collect statistics */
        for (i = 0; i < TM_MAX_GROUP; i++)
        {
            if (print_cq && pi2_calc_desc_ptr[i])
            {
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PACKETS_TOTAL_READ(value, pi2_calc_desc_ptr[i]);
                pi2_packets_total += value;
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_PACKETS_DROPPED_READ(value, pi2_calc_desc_ptr[i]);
                pi2_packets_dropped += value;
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_READ(value, pi2_calc_desc_ptr[i]);
                pi2_cumulative_latency += value;
                RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_MAX_LATENCY_READ(value, pi2_calc_desc_ptr[i]);
                if (value > pi2_max_latency)
                    pi2_max_latency = value;
            }

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
            if (print_lq && laqm_desc_ptr[i])
            {
                RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_TOTAL_READ(value, laqm_desc_ptr[i]);
                laqm_packets_total += value;
                RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_READ(value, laqm_desc_ptr[i]);
                laqm_packets_remarked += value;
                RDD_LAQM_DEBUG_DESCRIPTOR_PACKETS_REMARKED_BY_CQ_READ(value, laqm_desc_ptr[i]);
                laqm_packets_remarked_by_cq += value;
                RDD_LAQM_DEBUG_DESCRIPTOR_CUMULATIVE_LATENCY_READ(value, laqm_desc_ptr[i]);
                laqm_cumulative_latency += value;
                RDD_LAQM_DEBUG_DESCRIPTOR_MAX_LATENCY_READ(value, laqm_desc_ptr[i]);
                if (value > laqm_max_latency)
                    laqm_max_latency = value;
            }
#endif
        }

        /* Print statistics */
        if (print_cq)
        {
            /* Collect PI2 histogram and convert to msec */
            get_cq_histogram(pi2_calc_desc_ptr, pi2_histogram);
            pi2_p5_x10 = calc_percentile_x10(pi2_histogram, AQM_CQ_HISTOGRAM_SIZE, 5);
            pi2_p5_x10 = PI2_TICKS_TO_MS(pi2_p5_x10);
            pi2_p95_x10 = calc_percentile_x10(pi2_histogram, AQM_CQ_HISTOGRAM_SIZE, 95);
            pi2_p95_x10 = PI2_TICKS_TO_MS(pi2_p95_x10);

            /* Calc: Latency is in 1.024 ms units, need to convert to ms */
            avg_latency_x10 = ROUND_AND_DIVIDE(ROUND_AND_DIVIDE(10ULL * pi2_cumulative_latency, pi2_packets_total) * 1024, 1000);
            max_latency_x10 = ROUND_AND_DIVIDE(pi2_max_latency * 1024 * 10, 1000);
            bdmf_session_print(session, "CQ_packets %u CQ_drops %u P5_msec %u.%u avg_msec %u.%u P95_msec %u.%u max_msec %u.%u\n",
                pi2_packets_total,
                pi2_packets_dropped,
                pi2_p5_x10/10, pi2_p5_x10%10,
                avg_latency_x10/10, avg_latency_x10%10,                
                pi2_p95_x10/10, pi2_p95_x10%10,
                max_latency_x10/10, max_latency_x10%10);
        }

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
        if (print_lq)
        {
            /* Collect laqm histogram and convert to usec */
            get_lq_histogram(laqm_desc_ptr, laqm_histogram);
            laqm_p5_x10 = calc_percentile_x10(laqm_histogram, AQM_LQ_HISTOGRAM_SIZE, 5);
            laqm_p5_x10 = LAQM_TICKS_TO_MS(laqm_p5_x10);
            laqm_p95_x10 = calc_percentile_x10(laqm_histogram, AQM_LQ_HISTOGRAM_SIZE, 95);
            laqm_p95_x10 = LAQM_TICKS_TO_MS(laqm_p95_x10);

            /* Calc: Latency is in 128 us units, need to convert to ms */
            avg_latency_x10 = ROUND_AND_DIVIDE(ROUND_AND_DIVIDE(10ULL * laqm_cumulative_latency, laqm_packets_total) * 128, 1000);
            max_latency_x10 = LAQM_TICKS_TO_MS(10ULL * laqm_max_latency);
            bdmf_session_print(session, "LQ_packets %u LQ_remarks %u CQ_remarks %u P5_msec %u.%u avg_msec %u.%u P95_msec %u.%u max_msec %u.%u\n",
                laqm_packets_total,
                laqm_packets_remarked,
                laqm_packets_remarked_by_cq,
                laqm_p5_x10/10, laqm_p5_x10%10,
                avg_latency_x10/10, avg_latency_x10%10,
                laqm_p95_x10/10, laqm_p95_x10%10,
                max_latency_x10/10, max_latency_x10%10);
        }
#endif

#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_HISTOGRAM_WRITE
        if (print_cq)
        {
            bdmf_session_print(session, "CQ_histogram_%d_usec", 1024 / AQM_CQ_HISTOGRAM_BIN_SIZE);
            print_histogram(session, pi2_histogram, AQM_CQ_HISTOGRAM_SIZE);
        }
#endif
#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
        if (print_lq)
        {
            bdmf_session_print(session, "LQ_histogram_%d_usec", 128 / AQM_LQ_HISTOGRAM_BIN_SIZE);
            print_histogram(session, laqm_histogram, AQM_LQ_HISTOGRAM_SIZE);
        }
#endif
        bdmf_session_print(session, "* Avg and Max are calculated per packet. Percentiles are calculated from the histogram and assume an even distribution.\n");
    }

    return BDMF_ERR_OK;  
}

static int _rdd_pi2_stats(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    return _rdd_aqm_stats(session, parm, n_parms, RDD_PI2_PRINT_CQ_STATS);
}

static int _rdd_l4s_stats(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    return _rdd_aqm_stats(session, parm, n_parms, RDD_PI2_PRINT_CQ_STATS | RDD_PI2_PRINT_LQ_STATS);
}

static int _rdd_lq_stats(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    return _rdd_aqm_stats(session, parm, n_parms, RDD_PI2_PRINT_LQ_STATS);
}
#endif /* #ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE */

static int _rdd_pi2_cfg(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    rdd_pi2_group_t group;
    rdd_pi2_rtt_max_t rtt_max;
    rdd_pi2_target_delay_t target_delay;
    bdmfmon_cmd_parm_t *bdmf_parm;
    bdmf_error_t err;

#ifdef PI2_GROUP_CONFIGURATION
    group = (rdd_pi2_group_t)parm[0].value.number;
    err = rdd_pi2_check_group(group);
    if (err)
        return err;
#else
    /* If group mode is disabled, all groups are the same. Arbitrarily use DS to retrieve configuration */
    group = RDD_PI2_GROUP_ETH;
#endif

    /* Always read all parameters because if just a few are set, the rest will keep their previous values */
    err = rdd_pi2_cfg_get(group, &rtt_max, &target_delay);
    if (err)
        return err;

#ifdef PI2_GROUP_CONFIGURATION
    if (n_parms == 1)
    {
        /* GET-configuration request */
        bdmf_session_print(session,
            "PI2[%s]: RTT_MAX=%s TARGET_DELAY=%s\n",
            bdmfmon_enum_stringval(pi2_group_enum_table, group),
            bdmfmon_enum_stringval(pi2_rtt_max_enum_table, rtt_max),
            bdmfmon_enum_stringval(pi2_target_delay_enum_table, target_delay));
    }
#else
    if (n_parms == 0)
    {
        bdmf_session_print(session,
            "Maximum RTT  : %s\n"
            "Target delay : %s\n",
            bdmfmon_enum_stringval(pi2_rtt_max_enum_table, rtt_max),
            bdmfmon_enum_stringval(pi2_target_delay_enum_table, target_delay));
    }
#endif
    else
    {
        /* SET-configuration request */
        bdmf_parm = bdmfmon_find_named_parm(session, RTT_MAX_PARAM_NAME);
        if (bdmf_parm)
            rtt_max = bdmf_parm->value.number;
        bdmf_parm = bdmfmon_find_named_parm(session, TARGET_DELAY_PARAM_NAME);
        if (bdmf_parm)
            target_delay = bdmf_parm->value.number;
#ifdef PI2_GROUP_CONFIGURATION
        err = rdd_pi2_cfg(group, rtt_max, target_delay);
#else
        err = rdd_pi2_cfg(RDD_PI2_GROUP_ETH, rtt_max, target_delay);
        err = err ? err : rdd_pi2_cfg(RDD_PI2_GROUP_PON, rtt_max, target_delay);
#ifdef PI2_ON_SERVICE_QUEUES
        err = err ? err : rdd_pi2_cfg(RDD_PI2_GROUP_SQ, rtt_max, target_delay);
#endif
#endif
    }
    return err;
}
static int _rdd_pi2_profile_legend(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_session_print(session, "Legend:\n");
    bdmf_session_print(session, "#       - sample number;\n");
    bdmf_session_print(session, "time    - sample timestamp [usec]\n");
    bdmf_session_print(session, "P       - used to calculate drop probability, 0=drop none, 32767=drop all. Recalculated every %d msec.\n", (16 * PI2_TIMER_PERIOD_US) / 1000);
    bdmf_session_print(session, "PS      - packet select for drop. Incremented by ~P for every packet. Packet is discarded when this crosses 255\n");
    bdmf_session_print(session, "D       - queue delay [msec]. May be zero some boards\n");
    bdmf_session_print(session, "PD      - previous queue delay [msec]\n");
    bdmf_session_print(session, "Occup   - number of bytes in queue [bytes]\n");
    bdmf_session_print(session, "Passed  - number of OK packets\n");
    bdmf_session_print(session, "Dropped - number of dropped packets\n");
    bdmf_session_print(session, "LP      - (L4S only) low priority queue remark probability. Recalculated every %d msesc. 255=remark all\n", (16 * PI2_TIMER_PERIOD_US) / 1000);
    bdmf_session_print(session, "LPS     - (L4S only) low priority packet select for remark. Incremented by LP for every packet. Remark if crosses 255\n");
    bdmf_session_print(session, "LQ_Passed (L4S only) number of OK LQ packets\n");
    bdmf_session_print(session, "Marks*  - (L4S only) packets remarked due to LQ\n");
    bdmf_session_print(session, "CQ_Marks* (L4S only) packets remarked due to CQ!\n");
    bdmf_session_print(session, "* means global counter, not per queue\n");
    bdmf_session_print(session, "Note: statisitics such as average are actaully calculated on previous delay\n");
    bdmf_session_print(session, "\n");

    return BDMF_ERR_OK;
}

static int _rdd_pi2_profile(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    uint16_t queue_id = (uint16_t)parm[0].value.unumber;
    uint32_t num_samples = (uint32_t)parm[1].value.unumber;
    struct pi2_sample {
        uint32_t timestamp;
        rdd_pi2_queue_descriptor_t qd;
        uint32_t occupancy;
        uint32_t packets_passed;
        uint32_t packets_dropped;
        /* LQ */
        uint32_t packets_remarked;
        uint32_t packets_remarked_by_cq;
        uint32_t lq_packets_passed;
    };
    rdd_pi2_queue_descriptor_t qd = {};
    struct pi2_sample *sample_array;
    uint32_t prev_q_delay;
    uint32_t avg_queue_delay = 0;
    uint32_t min_queue_delay = 0xFFFF;
    uint32_t max_queue_delay = 0;
    uint32_t queue_delay_var = 0;
    int rc;
    int i;
#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
    LAQM_DEBUG_DESCRIPTOR_STRUCT laqm_desc;
    uint32_t base_packets_remarked = 0;
    uint32_t base_packets_remarked_by_cq = 0;
    uint16_t lq_queue_id = queue_id + 1; /* LQ immediately follows CQ */
    uint16_t tm_identity;
#endif

    rc = rdd_pi2_queue_qd_get(queue_id, &qd);
    if (rc != BDMF_ERR_OK)
        return rc;

    sample_array = bdmf_calloc(sizeof(struct pi2_sample) * num_samples);
    if (sample_array == NULL)
        return BDMF_ERR_NOMEM;

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
    tm_identity = get_tm_identity_from_qm_queue(queue_id);
    if (tm_identity == TM_ERROR)
    {
        bdmf_free(sample_array);
        return BDMF_ERR_PARM;
    }
#endif

    for (i = 0; i < num_samples; i++)
    {
        struct pi2_sample *sample = &sample_array[i];

        sample->timestamp = (uint32_t)bdmf_time_since_epoch_usec();
        rdd_pi2_queue_qd_get(queue_id, &sample->qd);

        prev_q_delay = PI2_TICKS_TO_MS(sample->qd.prev_q_delay);
        avg_queue_delay += prev_q_delay;
        min_queue_delay = (prev_q_delay < min_queue_delay) ? prev_q_delay : min_queue_delay;
        max_queue_delay = (prev_q_delay > max_queue_delay) ? prev_q_delay : max_queue_delay;

        ag_drv_qm_total_valid_cnt_get(queue_id * (sizeof(QM_QUEUE_COUNTER_DATA_STRUCT) / 4) + 1, &sample->occupancy);
        drv_tx_counters_read(queue_id, &sample->packets_passed, NULL);
        ag_drv_qm_drop_counter_get(queue_id * 2, &sample->packets_dropped);
#if CHIP_VER < RDP_GEN_62
        {
            /* Chip versions < 6.2 use dedicated AQM drop counters in SRAM, separate from QM drop counters.
            For chip versions >= 6.2 all drops are counted by QM */
            uint32_t aqm_drop_packets = 0;
            uint32_t aqm_drop_bytes = 0;

            rdd_tx_queue_drop_count_get(queue_id, &aqm_drop_packets, &aqm_drop_bytes, 1);
            sample->packets_dropped += aqm_drop_packets;
        }
#endif

#if defined(XRDP_DUALQ_L4S) && defined(L4S_LAQM_DEBUG)
        rdd_laqm_debug_descriptor_get(tm_identity, 0, &laqm_desc);
        drv_tx_counters_read(lq_queue_id, &sample->lq_packets_passed, NULL);
        sample->packets_remarked = laqm_desc.packets_remarked - base_packets_remarked;
        sample->packets_remarked_by_cq = laqm_desc.packets_remarked_by_cq - base_packets_remarked_by_cq;
        base_packets_remarked = laqm_desc.packets_remarked;
        base_packets_remarked_by_cq = laqm_desc.packets_remarked_by_cq;
#endif

        bdmf_msleep(1);
    }

    avg_queue_delay /= num_samples;

    /* Now print all collected samples */
    bdmf_session_print(session, "#     time_us    P     PS     D   PD  Occup    Passed   Dropped  LP    LPS    LQ_Pass  Marks*   CQ_Marks*\n");
    for (i = 0; i < num_samples; i++)
    {
        struct pi2_sample *sample = &sample_array[i];
        prev_q_delay = PI2_TICKS_TO_MS(sample->qd.prev_q_delay);

        queue_delay_var += ((avg_queue_delay > prev_q_delay) ? (avg_queue_delay - prev_q_delay) : (prev_q_delay - avg_queue_delay));

        bdmf_session_print(session, "%.5d %.10u %.5u %.6u %.3u %.3u %.8u %.8u %.8u %.5u %.6u %.8u %.8u %.8u\n",
            i, sample->timestamp,
            sample->qd.probability, sample->qd.packet_select_probability,
            (uint32_t)PI2_TICKS_TO_MS(sample->qd.q_delay), prev_q_delay,
            sample->occupancy, sample->packets_passed, sample->packets_dropped,
#if defined(XRDP_DUALQ_L4S)
            sample->qd.laqm_probability, sample->qd.laqm_packet_select_probability,
            sample->lq_packets_passed, sample->packets_remarked, sample->packets_remarked_by_cq
#else
            0, 0, 0, 0, 0
#endif
        );
    }
    bdmf_free(sample_array);

    bdmf_session_print(session, "queue delay min/max/average/variance: %.3u/%.3u/%.3u/%.3u\n",
                       min_queue_delay, max_queue_delay, avg_queue_delay, queue_delay_var/num_samples);

    return BDMF_ERR_OK;
}


#if defined(XRDP_DUALQ_L4S)
static int _rdd_laqm_cfg(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
     bdmfmon_cmd_parm_t *bdmf_parm;
    int lq_ecn_mask;
    int coupling_factor;

    rdd_laqm_cfg_get(&coupling_factor, &lq_ecn_mask);

    if (n_parms == 0)
    {
        /* Print current configuration */
        bdmf_session_print(session,
            "coupling factor : %d (K=%u.%02u)\n"
            "LQ ECN mask     : %s\n",
            coupling_factor,
            (unsigned int)(125 * coupling_factor) / 1000, ((unsigned int)((125 * coupling_factor)/10)) % 100,
            bdmfmon_enum_stringval(lq_ecn_mask_enum_table, lq_ecn_mask));
    }
    else
    {
        /* Configure */
        bdmf_parm = bdmfmon_find_named_parm(session, LQ_ECN_MASK_PARAM_NAME);
        if (bdmf_parm)
            lq_ecn_mask = bdmf_parm->value.number;

        bdmf_parm = bdmfmon_find_named_parm(session, COUPLING_FACTOR_PARAM_NAME);
        if (bdmf_parm)
            coupling_factor = bdmf_parm->value.number;

        return rdd_laqm_cfg(coupling_factor, lq_ecn_mask);
    }

    return BDMF_ERR_OK;
}
#endif

#if CHIP_VER < RDP_GEN_62
static int _rdd_print_aqm_drop_counters(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    uint32_t queue_id = (uint32_t)parm[0].value.unumber;
    bdmf_boolean reset = (bdmf_boolean)parm[1].value.number;
    uint32_t packets, bytes;
    int rc;

    rc = rdd_tx_queue_drop_count_get(queue_id, &packets, &bytes, reset);
    if (rc != BDMF_ERR_OK)
        return rc;

    bdmf_session_print(session, "Head drops by AQM: queue %u: packets %u  bytes %u\n", queue_id, packets, bytes);
    return BDMF_ERR_OK;
}
#endif

void rdd_pi2_shell_cmds_init(bdmfmon_handle_t aqm_dir)
{
    BDMFMON_MAKE_CMD(aqm_dir, "pi2cfg", "configure PI2", _rdd_pi2_cfg,
#ifdef PI2_GROUP_CONFIGURATION
        BDMFMON_MAKE_PARM_ENUM("group", "PI2 configuration group", pi2_group_enum_table, 0),
#endif
        BDMFMON_MAKE_PARM_ENUM(RTT_MAX_PARAM_NAME, "Maximum RTT", pi2_rtt_max_enum_table, BDMFMON_PARM_FLAG_OPTIONAL),
        BDMFMON_MAKE_PARM_ENUM(TARGET_DELAY_PARAM_NAME, "Target delay", pi2_target_delay_enum_table, BDMFMON_PARM_FLAG_OPTIONAL));

#ifdef RDD_PI2_PROBABILITY_CALC_DESCRIPTOR_CUMULATIVE_LATENCY_WRITE
    BDMFMON_MAKE_CMD(aqm_dir, "pi2stats", "PI2/CQ statistics", _rdd_pi2_stats,
        BDMFMON_MAKE_PARM_ENUM("group", "PI2 configuration group", pi2_group_enum_table, 0),
        BDMFMON_MAKE_PARM("num_seconds", "Number of seconds (none-only dump, 0-only zero, !0-zero,wait and dump", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));

    BDMFMON_MAKE_CMD(aqm_dir, "l4sstats", "L4S (PI2/CQ+LQ) statistics", _rdd_l4s_stats,
        BDMFMON_MAKE_PARM_ENUM("group", "PI2 configuration group", pi2_group_enum_table, 0),
        BDMFMON_MAKE_PARM("num_seconds", "Number of seconds (none-only dump, 0-only zero, !0-zero,wait and dump", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));

    BDMFMON_MAKE_CMD(aqm_dir, "lqstats", "LQ statistics", _rdd_lq_stats,
        BDMFMON_MAKE_PARM_ENUM("group", "PI2 configuration group", pi2_group_enum_table, 0),
        BDMFMON_MAKE_PARM("num_seconds", "Number of seconds (none-only dump, 0-only zero, !0-zero,wait and dump", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
#endif

    BDMFMON_MAKE_CMD(aqm_dir, "prf_legend", "PI2/L4S profiling legend", _rdd_pi2_profile_legend, BDMFMON_PARM_LIST_TERMINATOR);
    BDMFMON_MAKE_CMD(aqm_dir, "prf", "PI2/L4S profiling", _rdd_pi2_profile,
        BDMFMON_MAKE_PARM("queue_id", "QM queue id", BDMFMON_PARM_NUMBER, 0),
        BDMFMON_MAKE_PARM("num_samples", "Number of samples", BDMFMON_PARM_NUMBER, 0));

#if CHIP_VER < RDP_GEN_62
    BDMFMON_MAKE_CMD(aqm_dir, "aqmdrc", "Print Codel/PI2 Drop Counters", _rdd_print_aqm_drop_counters,
        BDMFMON_MAKE_PARM("queue_id", "QM queue id", BDMFMON_PARM_NUMBER, 0),
        BDMFMON_MAKE_PARM_ENUM_DEFVAL("reset", "Reset counters", bdmfmon_enum_bool_table, 0, "no"));
#endif

#if defined(XRDP_DUALQ_L4S)
    BDMFMON_MAKE_CMD(aqm_dir, "l4scfg", "configure L4S", _rdd_laqm_cfg,
        BDMFMON_MAKE_PARM(COUPLING_FACTOR_PARAM_NAME, "coupling factor (K) in units of 0.125 (0...255)", BDMFMON_PARM_UNUMBER, BDMFMON_PARM_FLAG_OPTIONAL),
        BDMFMON_MAKE_PARM_ENUM(LQ_ECN_MASK_PARAM_NAME, "LQ ECN mask", lq_ecn_mask_enum_table, BDMFMON_PARM_FLAG_OPTIONAL));
#endif
}

#endif
