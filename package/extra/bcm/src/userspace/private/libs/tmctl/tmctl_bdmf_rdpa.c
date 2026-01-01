/*
* <:copyright-BRCM:2017:proprietary:standard
*
*    Copyright (c) 2017 Broadcom
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
:>
*/
#include "bcmtypes.h"
#include "rdpa_types.h"
#include "tmctl_api.h"
#include "tmctl_plat.h"
#include "tmctl_bdmf_rdpa.h"
#include "rdpactl_api.h"
#if defined(SUPPORT_FLUSH)
#include "eponctl_api.h"
#endif
#include "tmctl_api_runner.h"
#if defined(SUPPORT_DPI)
#include <bcmdpi.h>
#endif
#include "ethswctl_api.h"

/* Limit the user's queue_ids in order to maintain internal queue_ids that won't intefere with the user's */
#define MAX_USER_QUEUE_ID               (1000)
#define L4S_CQ_TO_LQ_QUEUE_ID(QUEUE_ID) ((QUEUE_ID) + (MAX_USER_QUEUE_ID))

#define L4S_LQ_WEIGHT   15  /* L4S protocol constants */
#define L4S_CQ_WEIGHT   1

#define L4S_CQ_INDEX    0
#define L4S_LQ_INDEX    1

typedef struct
{
    tmctl_devType_e devType;
    tmctl_if_t *if_p;
    rdpa_tm_level_type tm_level;
    rdpa_tm_sched_mode tm_mode;
    int sched_caps;
    uint32_t sched_type;
    int num_queues;
    BOOL set_dual_rate;
    int subsidiary_idx;
    int weight;
} set_parm_t;

typedef struct
{
    bdmf_object_handle root_tm;
    rdpa_tm_sched_mode mode;
    rdpa_tm_level_type level;
    uint8_t max_queues;
} get_root_param_t;

#define SP_SUBSIDIARY_IDX 0
#define WRR_SUBSIDIARY_IDX 1
#define STATIC_CTC_NUM_Q 8
#define STATIC_CTC_SHARED_TM_IDX 6
#define DEFAULT_INIT_QUEUE_MAX 8

/**expected to be called from is_lan only not safe if
* if_p is not ethernet
*/
static bdmf_boolean is_port_wan(tmctl_if_t *if_p)
{
    const char *devName = if_p->ethIf.ifname;
    bdmf_object_handle port = BDMF_NULL;
    bdmf_boolean is_wan;
    int rc;

    rc = rdpactl_get_port_by_name(devName, &port);
    if (rc)
        goto Error;

    rc = rdpa_port_is_wan_get(port, &is_wan);
    if (rc)
        goto Error;
    tmctl_debug("dev %s is wan %d", devName, is_wan);

    bdmf_put(port);
    return is_wan;

Error:
    /*This should never happen */
    tmctl_error("failed to communicate with dev %s", devName);
    return 0;
}

int is_supported_wan_mode(void)
{
   rdpa_epon_mode eponMode = get_epon_mode();

   if (eponMode == rdpa_epon_dpoe)
   {
      bdmf_object_handle system = 0;
      rdpa_system_cfg_t cfg = {0};

      if (rdpa_system_get(&system))
      {
        tmctl_error("is_supported_wan_mode() failed");
        return TMCTL_ERROR;
      }
      rdpa_system_cfg_get(system, &cfg);
      bdmf_put(system);
      return !cfg.car_mode;
   }
   return 1;
}


bdmf_boolean is_lan(tmctl_devType_e devType, tmctl_if_t *if_p)
{

    if (devType != TMCTL_DEV_ETH)
        return 0;

    return !is_port_wan(if_p);
}

int rdpaCtl_get_epon_mode(rdpa_epon_mode *eponMode);

rdpa_epon_mode get_epon_mode(void)
{
    /* Why is this variable static? */
    static rdpa_epon_mode epon_mode = rdpa_epon_none;
#ifdef PON_INTERFACE_PRESENT
    rdpaCtl_get_epon_mode(&epon_mode);
#endif
    return epon_mode;
}

static BOOL is_static_ctc_mode(tmctl_devType_e devType, bdmf_boolean is_lan)
{
    rdpa_epon_mode epon_mode = get_epon_mode();

    if (((devType == TMCTL_DEV_EPON) ||
       (devType == TMCTL_DEV_ETH && !is_lan)) &&
       ((epon_mode == rdpa_epon_ctc) || (epon_mode == rdpa_epon_cuc)))
    {
        return 1;
    }

    return 0;
}

static int get_queue_tm(bdmf_object_handle root_tm, int qid, bdmf_object_handle *egress_tm, int *idx)
{
    rdpa_tm_queue_location_t location;
    int ret;

    ret = rdpa_egress_tm_queue_location_get(root_tm, qid, &location);
    if (ret && ret != BDMF_ERR_NOENT)
    {
        tmctl_error("rdpa_egress_tm_queue_location_get failed, ret[%d]", ret);
        ret = TMCTL_ERROR;
    }
    else if (ret == BDMF_ERR_NOENT)
        ret = TMCTL_NOT_FOUND;
    else
    {
        *idx = location.queue_idx;
        *egress_tm = location.queue_tm;
        ret = TMCTL_SUCCESS;
    }

    return ret;
}

static void get_port_tm_caps(tmctl_devType_e devType, tmctl_if_t *if_p, tmctl_portTmParms_t *tm)
{
    tm->queueShaper = TRUE;
    tm->dualRate = FALSE;
    tm->portShaper = FALSE;
    tm->schedCaps = TMCTL_SP_CAPABLE;
    tm->dpiQueueExt = FALSE;
    if (devType == TMCTL_DEV_SVCQ)
    {
        tm->portShaper = TRUE;
        tm->maxQueues = MAX_Q_SERVICE_QUEUE_TM;
        tm->maxSpQueues = MAX_Q_SERVICE_QUEUE_TM;
        tm->schedCaps |= TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE;
#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
        tm->schedCaps |= TMCTL_1LEVEL_CAPABLE;
#endif
        return;
    }

#if defined(RDPA_XRDP_US_DS_AGNOSTIC)
    /* Report the same TM capabilities for LAN and WAN interfaces. */
    tm->portShaper = devType == TMCTL_DEV_ETH ? TRUE : FALSE;
    tm->maxQueues = MAX_Q_PER_TM;
    tm->maxSpQueues = MAX_Q_PER_TM;
    tm->schedCaps |= TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE;
    tm->schedCaps |= TMCTL_1LEVEL_CAPABLE;
    tm->dualRate = TRUE;
    if (!is_lan(devType, if_p))
        tm->dpiQueueExt = TRUE;
    return;
#endif

    if (is_lan(devType, if_p))
    {
        tm->maxQueues = MAX_Q_PER_LAN_TM;
        tm->maxSpQueues = MAX_Q_PER_LAN_TM;
#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
        tm->schedCaps |= TMCTL_WRR_CAPABLE | TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE | TMCTL_1LEVEL_CAPABLE;
#else
        tm->queueShaper = FALSE;
#endif /* BCM_PON_XRDP || BCM_DSL_XRDP */
        tm->portShaper = TRUE;
        if (is_switch_intf(devType, if_p)) /* Runner port & queue shapers are not used for switch intf */
        {
            tm->queueShaper = FALSE;
            tm->portShaper = FALSE;
        }
    }
    else
    {
        if (devType == TMCTL_DEV_EPON || devType == TMCTL_DEV_ETH)
            tm->portShaper = TRUE;


        if (is_static_ctc_mode(devType, is_lan(devType, if_p)))
        {
            tm->maxQueues = MAX_TMCTL_QUEUES_BASELINE;
            tm->maxSpQueues = MAX_TMCTL_QUEUES_BASELINE;
            tm->queueShaper = FALSE;
        }
        else
        {
            tm->maxQueues = MAX_Q_PER_WAN_TM;
            tm->maxSpQueues = MAX_Q_PER_WAN_TM;
            tm->schedCaps |=  TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE;
#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
            tm->schedCaps |= TMCTL_1LEVEL_CAPABLE;
#endif  /* BCM_PON_XRDP || BCM_DSL_XRDP */
#if !defined(CHIP_6846) && !defined(CHIP_6878)
            tm->dpiQueueExt = TRUE;
#endif
            tm->dualRate = TRUE;
        }
    }

    tmctl_debug("maxQueues[%d], maxSpQueues[%d], portShaper[%d], queueShaper[%d], schedCaps[%d], dualRate[%d]",
                tm->maxQueues, tm->maxSpQueues, tm->portShaper, tm->queueShaper, tm->schedCaps, tm->dualRate );
}


tmctl_ret_e rdpaCtl_GetTmMemoryInfo(int * fpmPoolMemorySize);
tmctl_ret_e tmctlRdpa_getMemoryInfo(int * fpmPoolMemorySize)
{
   int  rc;
   if ((rc = rdpaCtl_GetTmMemoryInfo(fpmPoolMemorySize)))
   {
      tmctl_error("rdpaCtl_GetTmMemoryInfo ERROR! rc=%d", rc);
      return TMCTL_ERROR;
   }
   return TMCTL_SUCCESS;
}



int get_tm_owner(tmctl_devType_e devType, tmctl_if_t *if_p, bdmf_object_handle *owner)
{
    int ret = TMCTL_ERROR;
    const char *name = ifp_to_name(devType, if_p);

    tmctl_debug("dev[%s]", name);

    switch (devType)
    {
        case TMCTL_DEV_ETH:
        {
            ret = rdpactl_get_port_by_name(name, owner);
            break;
        }
#if defined(PON_INTERFACE_PRESENT)
        case TMCTL_DEV_GPON:
            /* For T-CONT device type, index 0 is reserved for Default ALLOC Id so we
            need to increment by 1 the index of the TCONT that comes from OMCI userspace application. */
            ret = rdpa_tcont_get(ifp_to_tcont(if_p) + 1, owner);
            break;
        case TMCTL_DEV_EPON:
            ret = rdpa_llid_get(ifp_to_llid(if_p), owner);
            break;
#endif
        case TMCTL_DEV_SVCQ:
            *owner = BDMF_NULL;
            ret = TMCTL_SUCCESS;
            break;
        default:
            tmctl_error("Device %d not supported", devType);
            break;
    }

    return ret;
}

static void ignore_negtiv_shaper(tmctl_shaper_t *shaper)
{
    if (shaper->minRate < 0)
        shaper->minRate = 0;
    if (shaper->shapingRate < 0)
        shaper->shapingRate = 0;
    if (shaper->shapingBurstSize < 0)
        shaper->shapingBurstSize = 0;
}

tmctl_ret_e tmctl_egress_tm_rl_set(bdmf_object_handle egress_tm_obj, rdpa_rl_cfg_t * rl_cfg)
{
    int ret;
    bdmf_object_handle rate_limit_obj = BDMF_NULL;
    ret = rdpa_egress_tm_rate_limit_get(egress_tm_obj, &rate_limit_obj);
    if ((ret) || (rate_limit_obj == BDMF_NULL))
    {
        /* we dont have rate limit, if try to configure to 0 dont do anything*/
        if ((rl_cfg->af_rate == 0) && (rl_cfg->be_rate == 0))
        {
            /*in this case dont do anything*/
            return TMCTL_SUCCESS;
        }

        /* need to create new rate limit object */
        bdmf_mattr_handle mattr = BDMF_NULL;
        mattr = bdmf_mattr_alloc(rdpa_rate_limit_drv());
        if (!mattr)
        {
            tmctl_error("bdmf_mattr_alloc failed");
            return TMCTL_ERROR;
        }
  
        ret = bdmf_new_and_set(rdpa_rate_limit_drv(), BDMF_NULL, mattr, &rate_limit_obj);
        bdmf_mattr_free(mattr);
        if (ret)
        {
            if (rate_limit_obj)
                bdmf_destroy(rate_limit_obj);
            tmctl_error("bdmf_new_and_set failed to create rate limit obj, ret[%d]", ret);
            return TMCTL_ERROR;
        }
    }
    else
    {
        /* we already have rate limit object*/
        if ((rl_cfg->af_rate == 0) && (rl_cfg->be_rate == 0))
        {
            bdmf_destroy(rate_limit_obj);
            return TMCTL_SUCCESS;
        }
    }
    /* here we have for sure rate limit object */
    rdpa_rate_limit_cfg_set(rate_limit_obj, rl_cfg);
    ret = rdpa_egress_tm_rate_limit_set(egress_tm_obj, rate_limit_obj);
    return TMCTL_SUCCESS;
}

static int set_queue_rl_multi_level(bdmf_object_handle root_tm, bdmf_object_handle queue_tm, tmctl_shaper_t *shaper)
{
    rdpa_tm_rl_rate_mode rl;
    int ret;
    rdpa_rl_cfg_t rl_cfg = {0};
    ignore_negtiv_shaper(shaper);
    ret = rdpa_egress_tm_rl_rate_mode_get(root_tm, &rl);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_rl_rate_mode_get failed, ret[%d]" , ret);
        return TMCTL_ERROR;
    }

    if (rl == rdpa_tm_rl_dual_rate)
    {
        if (shaper->minRate)
        {
            rl_cfg.af_rate = KBPS_TO_BPS(shaper->minRate);
        }
        else /* for dual rate mode, AF rate must be greater than 0 */
            rl_cfg.af_rate = 1;
        if (shaper->shapingRate)
        {
            rl_cfg.af_rate = KBPS_TO_BPS(shaper->shapingRate - shaper->minRate);
        }
        else /* for dual rate mode, BE must be set to max for no max rate shaping. */
            rl_cfg.be_rate = 1000000000L;
    }
    else
    {
        rl_cfg.af_rate = KBPS_TO_BPS(shaper->shapingRate);
    }

    ret = tmctl_egress_tm_rl_set(queue_tm, &rl_cfg);

    if (ret)
    {
        tmctl_error("tmctl_egress_tm_rl_set failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}


static int setup_tm(set_parm_t *set_parm, bdmf_object_handle owner, bdmf_object_handle *egress_tm)
{
    bdmf_mattr_handle mattr = BDMF_NULL;
    int ret;

    tmctl_debug("tm_level[%d], tm_mode[%d]", set_parm->tm_level, set_parm->tm_mode);

    *egress_tm = BDMF_NULL;

    mattr = bdmf_mattr_alloc(rdpa_egress_tm_drv());
    if (!mattr)
    {
        tmctl_error("bdmf_mattr_alloc failed");
        goto error;
    }

    if (set_parm->devType == TMCTL_DEV_SVCQ)
    {
        rdpa_tm_service_queue_t service_queue = {.enable = 1};

        ret = rdpa_egress_tm_service_queue_set(mattr, &service_queue);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_service_queue_set failed, ret[%d]", ret);
            goto error;
        }
    }

    ret = rdpa_egress_tm_level_set(mattr, set_parm->tm_level);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_level_set failed, ret[%d]", ret);
        goto error;
    }

    ret = rdpa_egress_tm_mode_set(mattr, set_parm->tm_mode);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_mode_set failed, ret[%d]", ret);
        goto error;
    }

    if (IS_SINGLE_LEVEL(set_parm->tm_level) && (set_parm->tm_mode != rdpa_tm_sched_disabled))
    {
        ret = rdpa_egress_tm_num_queues_set(mattr, set_parm->num_queues);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_num_queues_set failed. ret[%d]", ret);
            goto error;
        }
    }

    if (set_parm->set_dual_rate)
    {
        ret = rdpa_egress_tm_rl_rate_mode_set(mattr, rdpa_tm_rl_dual_rate);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_rl_rate_mode_set failed, ret[%d]", ret);
            goto error;
        }
    }

    if (set_parm->devType == TMCTL_DEV_SVCQ && set_parm->subsidiary_idx == BDMF_INDEX_UNASSIGNED)
    {
        ret = rdpa_egress_tm_index_set(mattr, RDPA_EGRESS_TM_SVCQ_INDEX);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_index_set (service_queue) failed, ret[%d]", ret);
            goto error;
        }
    }

    ret = bdmf_new_and_set(rdpa_egress_tm_drv(), owner, mattr, egress_tm);
    if (ret)
    {
        tmctl_error("bdmf_new_and_set failed to create egress_tm obj, ret[%d]", ret);
        goto error;
    }

#if !defined(CHIP_63146) && !defined(CHIP_4912)
    if ((set_parm->devType == TMCTL_DEV_EPON) && (set_parm->subsidiary_idx == BDMF_INDEX_UNASSIGNED))
    {
        ret = rdpa_llid_egress_tm_set(owner, *egress_tm);
        if (ret)
        {
            tmctl_error("rdpa_llid_egress_tm_set failed, ret[%d]", ret);
            goto error;
        }
    }
#endif
    if (set_parm->subsidiary_idx != BDMF_INDEX_UNASSIGNED) /* subsidiary egress_tm */
    {
        if (set_parm->weight)
        {
            ret = rdpa_egress_tm_weight_set(*egress_tm, set_parm->weight);
            if (ret)
            {
                tmctl_error("rdpa_egress_tm_weight_set failed, ret[%d]", ret);
                goto error;
            }
        }
        ret = rdpa_egress_tm_subsidiary_set(owner, set_parm->subsidiary_idx, *egress_tm);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_subsidiary_set failed, ret[%d]", ret);
            goto error;
        }
    }
    bdmf_mattr_free(mattr);
    return TMCTL_SUCCESS;
error:
    if (mattr)
        bdmf_mattr_free(mattr);
    if (*egress_tm)
        bdmf_destroy(*egress_tm);
    return TMCTL_ERROR;
}

static int get_root_tm(tmctl_devType_e devType, tmctl_if_t *if_p, bdmf_object_handle *egress_tm)
{
    bdmf_object_handle temp_tm = BDMF_NULL;
    bdmf_object_handle owner = BDMF_NULL;
    int ret = TMCTL_ERROR;

    *egress_tm = BDMF_NULL;

    if (devType == TMCTL_DEV_SVCQ)
    {
        /* find out if egress_tm service queue already created */
        bdmf_index index = RDPA_EGRESS_TM_SVCQ_INDEX;

        ret = rdpa_egress_tm_get(index, egress_tm);
        if (ret)
        {
            *egress_tm = BDMF_NULL;
        }

        return TMCTL_SUCCESS;
    }

    ret = get_tm_owner(devType, if_p, &owner);
    if (ret)
    {
        tmctl_error("Failed to get tm owner for %s, ret[%d]", ifp_to_name(devType, if_p), ret);
        return TMCTL_ERROR;
    }

    switch (devType)
    {
        case TMCTL_DEV_ETH:
            {
                rdpa_port_tm_cfg_t tm_cfg = {0};
                ret = rdpa_port_tm_cfg_get(owner, &tm_cfg);
                if (ret)
                {
                    tmctl_error("Failed to get tm_cfg, dev[%s], ret[%d]", ifp_to_name(devType,if_p), ret);
                    goto exit;
                }
                temp_tm = tm_cfg.sched;
            }
            break;
#if defined(BCM_PON) || (defined(CHIP_63158) || defined(CHIP_6813))
#ifndef DESKTOP_LINUX
        case TMCTL_DEV_GPON:
            ret = rdpa_tcont_egress_tm_get(owner, &temp_tm);
            if (ret)
            {
                tmctl_error("Failed to get egress_tm, dev[%s], ret[%d]", ifp_to_name(devType, if_p), ret);
                goto exit;
            }
            break;
        case TMCTL_DEV_EPON:
            ret = rdpa_llid_egress_tm_get(owner, &temp_tm);
            if (ret)
            {
                tmctl_error("Failed to get egress_tm, dev[%s], ret[%d]", ifp_to_name(devType,if_p), ret);
                goto exit;
            }
            break;
#endif /* DESKTOP_LINUX */
#endif /* BCM_PON || (defined(CHIP_63158) || defined(CHIP_6813)) */
        case TMCTL_DEV_SVCQ:
        default:
            tmctl_error("Device %d not supported", devType);
            goto exit;
    }

    *egress_tm = temp_tm;
    ret = TMCTL_SUCCESS;
exit:
    bdmf_put(owner);
    return ret;
}

static void print_sched_caps(uint32_t sched_caps)
{
    printf("Sched caps are:\n");

    if (sched_caps & TMCTL_SP_WRR_CAPABLE)
        printf("SP and WRR [0]\n");
    if (sched_caps & TMCTL_SP_CAPABLE)
        printf("SP [256]\n");
    if (sched_caps & TMCTL_WRR_CAPABLE)
        printf("WRR [512]\n");
}

static inline int get_shared_mode(tmctl_devType_e devType, BOOL *is_shared)
{
    rdpa_epon_mode epon_mode = rdpa_epon_none;

    if (devType != TMCTL_DEV_EPON)
    {
        *is_shared = 0;
        return TMCTL_SUCCESS;
    }

    epon_mode = get_epon_mode();
    if (epon_mode == rdpa_epon_none)
    {
        tmctl_error("get_epon_mode failed");
        return TMCTL_ERROR;
    }

    *is_shared = epon_mode == rdpa_epon_dpoe;
    return TMCTL_SUCCESS;
}

static int setup_default_queues(set_parm_t *root_param,
                                 int cfg_flags)
{
    tmctl_queueCfg_t qcfg_p = {0};
    int divergence = 0;
    int ret;
    BOOL is_shared;
    int queue_to_initialize = DEFAULT_INIT_QUEUE_MAX;

    ret = get_shared_mode(root_param->devType, &is_shared);
    if (ret)
    {
        tmctl_error("get_shared_mode failed");
        return TMCTL_ERROR;
    }

    if (root_param->tm_mode == rdpa_tm_sched_wrr)
    {
        qcfg_p.weight = 1;
        qcfg_p.schedMode = TMCTL_SCHED_WRR;
    }
    else
    {
        qcfg_p.weight = 0;
        qcfg_p.schedMode = TMCTL_SCHED_SP;
    }

    if (is_shared)
        root_param->num_queues = 8;

    if (root_param->num_queues < DEFAULT_INIT_QUEUE_MAX)
        queue_to_initialize = root_param->num_queues;

    /* move queues to the top of the array if num queues is lower than max possbile queues */
    if (root_param->tm_level == rdpa_tm_level_egress_tm || is_shared)
        divergence = MAX_Q_PER_WAN_TM - root_param->num_queues;

    /* move queues to the top of the array if default queue to be initialized
     * is less than the num queues in this egress TM */
    if (queue_to_initialize < root_param->num_queues)
        divergence = root_param->num_queues - queue_to_initialize;

    /* Initialize the default queues */
    for (qcfg_p.qid = 0; qcfg_p.qid < queue_to_initialize; qcfg_p.qid++)
    {
#ifdef BCM_DSL_XRDP
        /* DSL platform will always do Q7P7 */
        qcfg_p.priority = divergence + qcfg_p.qid;
#else
        if (cfg_flags & TMCTL_QIDPRIO_MAP_Q7P7)
            qcfg_p.priority = divergence + qcfg_p.qid;
        else
            qcfg_p.priority = divergence + queue_to_initialize - (uint32)qcfg_p.qid - 1;
#endif
        qcfg_p.qsize = tmctl_getIfacePriorityQueueSize(root_param->devType, 
                                                       root_param->if_p, 
                                                       qcfg_p.qid, 
                                                       TMCTL_DEFAULT_QUEUE_SIZE);
        if (TMCTL_DEFAULT_QUEUE_SIZE == qcfg_p.qsize)
            ret = TMCTL_ERROR;
        else
            ret = tmctl_setQueueCfg(root_param->devType, root_param->if_p, &qcfg_p);
        if (ret)
            return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

#define run_or_err(expr, errlabel) {                  \
    int _ret = expr;                                  \
    if (_ret)                                         \
    {                                                 \
        tmctl_error(#expr " failed, ret %d\n", _ret); \
        goto errlabel;                                \
    } }

#if defined(SUPPORT_DPI)
static int configure_best_effort_dpi_queues(bdmf_object_handle egress_tm, tmctl_queueCfg_t *parent_queue)
{
    int ret = 0;
    bdmf_index idx;

    /* add queues under service queue scheduler */
    for (idx = 0; idx < EG_PRIO_MAX + 1; idx++)
    {
        rdpa_tm_queue_cfg_t queue_cfg = {
            .queue_id       = idx + DPI_XRDP_US_SQ_OFFSET,
            .drop_alg       = rdpa_tm_drop_alg_dt,
            .drop_threshold = getDeviceQueueSize(parent_queue->qsize),
            .stat_enable    = 1,
            .reserved_packet_buffers = parent_queue->minBufs,
        };

        ret = rdpa_egress_tm_queue_cfg_set(egress_tm, idx, &queue_cfg);

        if (ret)
            break;
    }

    return ret;
}

static int add_best_effort_dpi_queues(bdmf_object_handle owner, tmctl_queueCfg_t *parent_queue, bdmf_index idx)
{
    rdpa_tm_service_queue_t service_queue = {.enable = RDPA_TM_SECONDARY_SERVICE_QUEUE_ENABLE};
    bdmf_object_handle egress_tm;
    bdmf_mattr_handle mattr;

    if (!rdpa_egress_tm_subsidiary_get(owner, idx, &egress_tm))
        return configure_best_effort_dpi_queues(egress_tm, parent_queue);

    mattr = bdmf_mattr_alloc(rdpa_egress_tm_drv());
    if (!mattr)
    {
        tmctl_error("bdmf_mattr_alloc failed");
        goto err;
    }

    /* create queues */
    /* set level */
    run_or_err(rdpa_egress_tm_level_set(mattr, RDPA_TM_SECONDARY_LEVEL), err_free_mattr);

#if defined(BCM_XRDP)
    run_or_err(rdpa_egress_tm_mode_set(mattr, RDPA_TM_SECONDARY_SCHEDULER_MODE), err_free_mattr);
#else
    run_or_err(rdpa_egress_tm_mode_set(mattr, rdpa_tm_sched_sp), err_free_mattr);
#endif
    run_or_err(rdpa_egress_tm_service_queue_set(mattr, &service_queue), err_free_mattr);
#if defined(BCM_XRDP)
    run_or_err(rdpa_egress_tm_overall_rl_set(mattr, FALSE), err_free_mattr);
    run_or_err(rdpa_egress_tm_num_queues_set(mattr, RDPA_TM_SECONDARY_NUM_QUEUES), err_free_mattr);
    if (parent_queue->weight)
    {
        run_or_err(rdpa_egress_tm_weight_set(mattr, parent_queue->weight), err_free_mattr);
    }
    else
    {
        run_or_err(rdpa_egress_tm_weight_set(mattr, RDPA_TM_SECONDARY_DEFAULT_WEIGHT), err_free_mattr);
    }

#ifdef RDPA_TM_SECONDARY_SP_ELEMENTS 
    run_or_err(rdpa_egress_tm_num_sp_elements_set(mattr, RDPA_TM_SECONDARY_SP_ELEMENTS), err_free_mattr);
#endif
    run_or_err(rdpa_egress_tm_rl_rate_mode_set(mattr, rdpa_tm_rl_dual_rate), err_free_mattr);
#endif

    run_or_err(bdmf_new_and_set(rdpa_egress_tm_drv(), owner, mattr, &egress_tm), err_free_mattr);

    run_or_err(rdpa_egress_tm_subsidiary_set(owner, idx, egress_tm), err_free_sched);
    run_or_err(configure_best_effort_dpi_queues(egress_tm, parent_queue), err_free_sched);

    bdmf_mattr_free(mattr);
    return TMCTL_SUCCESS;

err_free_sched:
    bdmf_destroy(egress_tm);
err_free_mattr:
    bdmf_mattr_free(mattr);
err:
    return TMCTL_ERROR;
}
#endif /* defined(SUPPORT_DPI) */

static int configure_l4s_queues(bdmf_object_handle egress_tm, tmctl_queueCfg_t *parent_queue)
{
    int ret = 0;

    rdpa_tm_queue_cfg_t queue_cfg = {
        .drop_threshold = getDeviceQueueSize(parent_queue->qsize),
        .stat_enable    = 1,
        .reserved_packet_buffers = parent_queue->minBufs,
    };

    /* Configure CQ at index 0 */
    queue_cfg.queue_id = parent_queue->qid,
    queue_cfg.drop_alg = rdpa_tm_drop_alg_dualq_pi2,
    queue_cfg.weight = L4S_CQ_WEIGHT;
    ret = rdpa_egress_tm_queue_cfg_set(egress_tm, L4S_CQ_INDEX, &queue_cfg);
    if (ret)
        return ret;

    /* Configure LQ at index 1 (must be consecutive to CQ)*/
    queue_cfg.queue_id = L4S_CQ_TO_LQ_QUEUE_ID(parent_queue->qid),
    queue_cfg.drop_alg = rdpa_tm_drop_alg_dualq_laqm,
    queue_cfg.weight = L4S_LQ_WEIGHT;
    ret = rdpa_egress_tm_queue_cfg_set(egress_tm, L4S_LQ_INDEX, &queue_cfg);
    if (ret)
    {
        rdpa_egress_tm_queue_cfg_delete(egress_tm, L4S_CQ_INDEX);
    }

    return ret;
}

static int l4s_secondary_egress_tm_set(bdmf_object_handle root_tm, bdmf_object_handle owner, tmctl_queueCfg_t *qcfg_p, bdmf_index idx)
{
    rdpa_tm_service_queue_t service_queue = {.enable = RDPA_TM_SECONDARY_SERVICE_QUEUE_ENABLE};
    bdmf_object_handle egress_tm;
    bdmf_mattr_handle mattr;

    mattr = bdmf_mattr_alloc(rdpa_egress_tm_drv());
    if (!mattr)
    {
        tmctl_error("bdmf_mattr_alloc failed");
        goto err;
    }

    /* create and configure L4S egress_tm */
    run_or_err(rdpa_egress_tm_level_set(mattr, RDPA_TM_SECONDARY_LEVEL), err_free_mattr);
    run_or_err(rdpa_egress_tm_mode_set(mattr, rdpa_tm_sched_wrr), err_free_mattr);
    run_or_err(rdpa_egress_tm_service_queue_set(mattr, &service_queue), err_free_mattr);
    run_or_err(rdpa_egress_tm_num_queues_set(mattr, 2), err_free_mattr);
    run_or_err(rdpa_egress_tm_weight_set(mattr, qcfg_p->weight), err_free_mattr);
    run_or_err(rdpa_egress_tm_rl_rate_mode_set(mattr, rdpa_tm_rl_single_rate), err_free_mattr);
    run_or_err(bdmf_new_and_set(rdpa_egress_tm_drv(), owner, mattr, &egress_tm), err_free_mattr);
    run_or_err(set_queue_rl_multi_level(root_tm, egress_tm, &qcfg_p->shaper), err_free_sched);
    run_or_err(rdpa_egress_tm_subsidiary_set(owner, idx, egress_tm), err_free_sched);

    /* create and configure L4S queues */
    run_or_err(configure_l4s_queues(egress_tm, qcfg_p), err_free_sched);

    bdmf_mattr_free(mattr);

    return TMCTL_SUCCESS;

err_free_sched:
    bdmf_destroy(egress_tm);
err_free_mattr:
    bdmf_mattr_free(mattr);
err:
    return TMCTL_ERROR;
}

static int l4s_secondary_egress_tm_update(tmctl_devType_e devType, bdmf_object_handle root_tm, tmctl_queueCfg_t *qcfg_p)
{
    bdmf_object_handle subsidiary_egress_tm;
    int queue_index;
    int ret;

    /* Get secondary egress_tm */
    ret = get_queue_tm(root_tm, qcfg_p->qid, &subsidiary_egress_tm, &queue_index);
    if (ret == TMCTL_NOT_FOUND)
    {
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(root_tm);
        }    
        return TMCTL_ERROR;
    }
    else if (ret)
    {
        return TMCTL_ERROR;
    }

    /* reconfigure the secondary egress tm */
    run_or_err(rdpa_egress_tm_weight_set(subsidiary_egress_tm, qcfg_p->weight), err);
    run_or_err(set_queue_rl_multi_level(root_tm, subsidiary_egress_tm, &qcfg_p->shaper),err);

    /* reconfigure the queues */
    run_or_err(configure_l4s_queues(subsidiary_egress_tm, qcfg_p),err);

    return TMCTL_SUCCESS;

err:
    return TMCTL_ERROR;
}

static int check_caps(uint32_t sched_type, uint32_t sched_caps)
{
    if ((sched_type == TMCTL_SCHED_TYPE_SP    && !(sched_caps & TMCTL_SP_CAPABLE)) ||
       (sched_type == TMCTL_SCHED_TYPE_WRR    && !(sched_caps & TMCTL_WRR_CAPABLE)) ||
       (sched_type == TMCTL_SCHED_TYPE_SP_WRR && !(sched_caps & TMCTL_SP_WRR_CAPABLE)))
    {
       tmctl_error("Configured scheduler type 0x%x is not supported, supported on port (0x%x)",
                       sched_type, sched_caps);
       print_sched_caps(sched_caps);
       return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static int check_cfg(tmctl_portTmParms_t *tm , set_parm_t param)
{
    if (check_caps(param.sched_type, tm->schedCaps))
       return TMCTL_ERROR;

    if (param.num_queues > tm->maxQueues)
    {
        tmctl_error("Num queues wanted[%d] is higher than max queues allowed[%d]", param.num_queues, tm->maxQueues);
        return TMCTL_ERROR;
    }

    if (param.num_queues > 0 && param.num_queues != 8 && param.num_queues != 16 && param.num_queues != 32)
    {
        tmctl_error("Num queues wanted[%d] is not allowed[8, 16, 32]", param.num_queues);
        return TMCTL_ERROR;
    }

    if (param.set_dual_rate && !tm->dualRate)
    {
        tmctl_error("Dual rate is not supported");
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static int get_default_num_queues(bdmf_boolean is_lan, uint32_t sched_type)
{
#if !defined(CHIP_6846) && !defined(BCM_RDP)
#ifndef RDPA_XRDP_US_DS_AGNOSTIC
    if (!is_lan && sched_type == TMCTL_SCHED_TYPE_SP_WRR)
#else
    if (sched_type == TMCTL_SCHED_TYPE_SP_WRR)
#endif
        return MAX_TMCTL_QUEUES_EXTENDED;
#endif
    return MAX_TMCTL_QUEUES_BASELINE;
}


static int setup_subsidiary_tm(bdmf_object_handle egress_tm, tmctl_devType_e devType)
{
    set_parm_t set_parm = {0};
    bdmf_object_handle sp_kid = BDMF_NULL;
    bdmf_object_handle wrr_kid = BDMF_NULL;
    int ret;

    set_parm.tm_level = rdpa_tm_level_egress_tm;
    set_parm.tm_mode = rdpa_tm_sched_sp;
    set_parm.subsidiary_idx = SP_SUBSIDIARY_IDX;
    set_parm.devType = devType;

    ret = setup_tm(&set_parm, egress_tm, &sp_kid);
    if (ret)
    {
        tmctl_error("setup_tm failed to create SP kid TM");
        goto error;
    }

    set_parm.tm_mode = rdpa_tm_sched_wrr;
    set_parm.subsidiary_idx = WRR_SUBSIDIARY_IDX;
    ret = setup_tm(&set_parm, egress_tm, &wrr_kid);
    if (ret)
    {
        tmctl_error("setup_tm failed to create WRR kid TM");
        goto error;
    }

    return TMCTL_SUCCESS;
error:
    if (sp_kid)
        bdmf_destroy(sp_kid);
    if (wrr_kid)
        bdmf_destroy(wrr_kid);
    return TMCTL_ERROR;
}

/* remove all queues except the control queue */
static int reset_shared_root(bdmf_object_handle root_tm)
{
    int ret;
    int i;
    uint8_t num_queues;

    ret = rdpa_egress_tm_num_queues_get(root_tm, &num_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    for (i = 1; i < num_queues; ++i)
    {
        ret = rdpa_egress_tm_queue_cfg_delete(root_tm, i);
        if (ret && ret != BDMF_ERR_NOENT)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_delete failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }
    }

    return TMCTL_SUCCESS;
}

static int setup_root_tm(set_parm_t *root_param, bdmf_object_handle *root_tm, tmctl_devType_e devType)
{
    bdmf_object_handle owner = BDMF_NULL;
    BOOL is_shared;
    int ret;

    ret = get_shared_mode(devType, &is_shared);
    if (ret)
    {
        tmctl_error("get_shared_mode failed");
        return TMCTL_ERROR;
    }

    if (*root_tm && is_shared)
    {
        ret = reset_shared_root(*root_tm);
        if (ret)
        {
            tmctl_error("Faile to reset root");
            goto error;
        }
        return TMCTL_SUCCESS;
    }
    else if (*root_tm)
    {
        tmctl_debug("Destroying old Tm object, dev[%s]",
                ifp_to_name(root_param->devType, root_param->if_p));
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(*root_tm);
        }
        ret = bdmf_destroy(*root_tm);
        if (ret)
        {
            tmctl_error("bdmf_destroy failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }
        *root_tm = BDMF_NULL;
    }

    root_param->subsidiary_idx = BDMF_INDEX_UNASSIGNED;

    if (root_param->sched_caps & TMCTL_1LEVEL_CAPABLE
           || (is_lan(root_param->devType, root_param->if_p))
       )
        root_param->tm_level = rdpa_tm_level_queue;
    else
        root_param->tm_level = rdpa_tm_level_egress_tm;

    switch (root_param->sched_type)
    {
        case TMCTL_SCHED_TYPE_SP_WRR:
            root_param->tm_mode = IS_SINGLE_LEVEL(root_param->tm_level) ? rdpa_tm_sched_sp_wrr : rdpa_tm_sched_sp ;
            break;
        case TMCTL_SCHED_TYPE_SP:
            root_param->tm_mode = rdpa_tm_sched_sp;
            break;
        case TMCTL_SCHED_TYPE_WRR:
            root_param->tm_mode = rdpa_tm_sched_wrr;
            break;
        default:
            tmctl_error("Port sched type 0x%x is not supported, dev[%s]", root_param->sched_type, ifp_to_name(root_param->devType, root_param->if_p));
            goto error;
    }
	
    root_param->num_queues = (root_param->num_queues > 0) ? root_param->num_queues :
                       get_default_num_queues(is_lan(root_param->devType, root_param->if_p), root_param->sched_type);

    ret = get_tm_owner(root_param->devType, root_param->if_p, &owner);
    if (ret)
    {
        tmctl_error("Failed to get tm owner, dev[%s], ret[%d]", ifp_to_name(root_param->devType, root_param->if_p), ret);
        goto error;
    }

    ret = setup_tm(root_param, owner, root_tm);
    if (owner != BDMF_NULL)
        bdmf_put(owner);
    if (ret)
    {
        tmctl_error("Failed to setup new tm, dev[%s]", ifp_to_name(root_param->devType, root_param->if_p));
        goto error;
    }

    /* generate multi level root for SP+WRR */
    if (!IS_SINGLE_LEVEL(root_param->tm_level) &&
        (root_param->sched_type == TMCTL_SCHED_TYPE_SP_WRR))
    {
        ret = setup_subsidiary_tm(*root_tm, devType);
        if (ret)
        {
            tmctl_error("setup_subsidiary_tm failed, dev[%s]", ifp_to_name(root_param->devType, root_param->if_p));
            goto error;
        }
    }

    return TMCTL_SUCCESS;
error:
    if (*root_tm)
        bdmf_destroy(*root_tm);
    return TMCTL_ERROR;
}

/*
Currently only epon sfu and hgu support.
*/
static BOOL is_flush_required(tmctl_devType_e devType, tmctl_if_t *if_p)
{
#if defined(SUPPORT_FLUSH)

    tmctl_debug("dev %s" , ifp_to_name(devType, if_p));

    switch (devType)
    {
    case TMCTL_DEV_EPON:
        return TRUE;

    case TMCTL_DEV_ETH:
        {
        int rc;
        bdmf_object_handle port = BDMF_NULL;
        rdpa_port_type type;
        BOOL ret = FALSE;
        rc = get_tm_owner(devType, if_p, &port);
        if (rc) goto Exit;
        rc = rdpa_port_type_get(port, &type);
        if (rc) goto Exit;
        if (type == rdpa_port_epon || type == rdpa_port_xepon)
            ret = TRUE;
Exit:
        if (port)
            bdmf_put(port);
        return ret;
        }
				
    default:
        return FALSE;
    }
#endif
    return FALSE;	
}

#if defined(SUPPORT_FLUSH)
static inline int get_impacted_devices(tmctl_devType_e devType, tmctl_if_t *if_p)
{
    /* SFU and DPoE HGU*/
    if (devType == TMCTL_DEV_EPON)
    {
        return ifp_to_llid(if_p);
    }
    /* CTC HGU */
    if (devType == TMCTL_DEV_ETH)
    {
        return 0;
    }
    return 0;
}

static tmctl_ret_e enable_device_traffic(tmctl_devType_e devType,tmctl_if_t *if_p, BOOL enable)
{
    int ret = TMCTL_SUCCESS;

    bdmf_error_t rc = BDMF_ERR_OK;
    bdmf_object_handle llid_obj = BDMF_NULL;
    uint8_t llid;

    tmctl_debug("%s traffic for %s\n\r", (enable?"enable":"disable"),
            ifp_to_name(devType, if_p));

    llid = get_impacted_devices(devType, if_p);
    rc = rdpa_llid_get(llid, &llid_obj);
    if (rc)
    {
        ret = TMCTL_ERROR;
        tmctl_error("rdpa_llid_get failed: llid(%u) rc(%d)", llid, rc);
        goto ENABLE_TRAFFIC_EXIT;
    }

    rc = rdpa_llid_data_enable_set(llid_obj, enable);
    if (rc)
    {
        ret = TMCTL_ERROR;
        tmctl_error("rdpa_llid_data_enable_set failed: llid(%u) rc(%d)", llid, rc);
        goto ENABLE_TRAFFIC_EXIT;
    }

ENABLE_TRAFFIC_EXIT:
    if (llid_obj)
        bdmf_put(llid_obj);


    return ret;
}

static tmctl_ret_e mac_flush_queue(tmctl_devType_e devType, tmctl_if_t *if_p)
{
    int ret = TMCTL_SUCCESS;

    bdmf_error_t rc = BDMF_ERR_OK;
    uint8_t llid;

    tmctl_debug("dev%s", ifp_to_name(devType, if_p));

    llid = get_impacted_devices(devType, if_p);
    rc = eponStack_CtlFlushLlid(llid);
    if (rc)
    {
        ret = TMCTL_ERROR;
        tmctl_error("eponStack_CtlFlushLlid failed: link (%u) rc(%d)", llid, rc);
    }
    return ret;
}
#else
#define mac_flush_queue(a,b) 0
#define enable_device_traffic(a,b,c) 0
#endif

tmctl_ret_e tmctl_RdpaTmInit(tmctl_devType_e devType, tmctl_if_t *if_p, uint32_t cfg_flags,
                                    int num_queues)
{
    bdmf_object_handle root_tm = BDMF_NULL;
    tmctl_portTmParms_t tm = {0};
    set_parm_t root_param = {0};
    BOOL is_shared;
    int ret;

    root_param.devType = devType;
    root_param.if_p = if_p;
    root_param.num_queues = num_queues;
    get_port_tm_caps(devType, if_p, &tm);
    root_param.sched_caps = tm.schedCaps;
    root_param.sched_type = cfg_flags & TMCTL_SCHED_TYPE_MASK;
    root_param.set_dual_rate = cfg_flags & TMCTL_SET_DUAL_RATE;

    if (check_cfg(&tm , root_param))
       return TMCTL_ERROR;

    ret = get_root_tm(devType, if_p, &root_tm);
    if (ret)
    {
        tmctl_error("get_root_tm failed, dev[%s]", ifp_to_name(devType, if_p));
        return TMCTL_ERROR;
    }

    ret = get_shared_mode(devType, &is_shared);
    if (ret)
    {
        tmctl_error("get_shared_mode failed");
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(root_tm);
        }
        return TMCTL_ERROR;
    }

    ret = setup_root_tm(&root_param, &root_tm, devType);
    if (ret)
    {
        tmctl_error("setup_root_tm failed, dev[%s]", ifp_to_name(devType, if_p));
        goto error;
    }

    if (cfg_flags & TMCTL_INIT_DEFAULT_QUEUES)   /* The default queues initialization is required */
    {
        ret = setup_default_queues(&root_param, cfg_flags);
        if (ret)
        {
            tmctl_error("setup_default_queues failed, dev[%s]", ifp_to_name(devType, if_p));
            goto error;
        }
    }

    return TMCTL_SUCCESS;
error:
    if (root_tm && !is_shared)
        bdmf_destroy(root_tm);
    return TMCTL_ERROR;
}

tmctl_ret_e tmctl_RdpaTmUninit(tmctl_devType_e devType,
        tmctl_if_t *if_p)
{
    bdmf_object_handle egress_tm = BDMF_NULL;
    BOOL is_shared;
    BOOL flush_required = FALSE;
    int ret;

    flush_required = is_flush_required(devType, if_p);
    ret = get_root_tm(devType, if_p, &egress_tm);
    if (ret)
    {
        tmctl_error("Failed to get egress_tm");
        return TMCTL_ERROR;
    }
    if (!egress_tm)
        return TMCTL_SUCCESS;

    ret = get_shared_mode(devType, &is_shared);
    if (ret)
    {
        tmctl_error("get_shared_mode failed, dev[%s]", ifp_to_name(devType, if_p));
        goto error;
    }

    /* On EPON platform, the delete queue/egress_tm should follow the sequence of
      disable traffic, flush queue and enable traffic*/
    if (flush_required)
    {
        ret = enable_device_traffic(devType, if_p, FALSE);
        if (ret)
        {
            tmctl_error("enable_device_traffic disable dev[%s] failed", ifp_to_name(devType, if_p));
            goto error;
        }

        ret = mac_flush_queue(devType, if_p);
        if (ret)
        {
            tmctl_error("mac_flush_queue dev[%s] failed", ifp_to_name(devType,if_p));
            goto  UINT_EXIT;
        }
    }

    if (is_shared)
    {
        ret = reset_shared_root(egress_tm);
        if (ret)
            tmctl_error("Failed to reset root");
        goto  UINT_EXIT;
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }

    ret = bdmf_destroy(egress_tm);
    if (ret)
    {
        ret = TMCTL_ERROR;
        tmctl_error("bdmf_destroy failed, ret[%d]", ret);
        goto  UINT_EXIT;
    }

UINT_EXIT:
    if (flush_required)
        ret |= enable_device_traffic(devType, if_p, TRUE);

    return ret;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }
    return TMCTL_ERROR;
}

static int get_q_idx(bdmf_object_handle egress_tm, int id, int *idx)
{
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    uint8_t max_queues;
    int ret;
    int i;

    ret = rdpa_egress_tm_num_queues_get(egress_tm, &max_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    for (i = 0; i < max_queues ; ++i)
    {
        ret = rdpa_egress_tm_queue_cfg_get(egress_tm, i, &queue_cfg);
        if (ret && ret != BDMF_ERR_NOENT)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }

        if (!ret && (queue_cfg.queue_id == id))
        {
            *idx = i;
            return TMCTL_SUCCESS;
        }
    }

    return TMCTL_NOT_FOUND;
}

static int get_tm_mode(bdmf_object_handle root_tm, rdpa_tm_sched_mode *mode)
{
    bdmf_object_handle subsidiary_tm = BDMF_NULL;
    rdpa_tm_level_type level;
    int ret;

    ret = rdpa_egress_tm_mode_get(root_tm, mode);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_level_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    ret = rdpa_egress_tm_level_get(root_tm, &level);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_level_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    if (IS_SINGLE_LEVEL(level)) /* in level_queue 'set mode' and 'real mode' are the same */
        return TMCTL_SUCCESS;

    /* in multi level SP+WRR mode is repesented as SP so need to verify wiche one is it
     * SP+WRR root must have subsidiary set as level egress_tm*/
    if (*mode == rdpa_tm_sched_sp)
    {
        ret = rdpa_egress_tm_subsidiary_get(root_tm, SP_SUBSIDIARY_IDX, &subsidiary_tm);
        if (ret == BDMF_ERR_NOENT)
            return TMCTL_SUCCESS;
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_subsidiary_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }

        ret = rdpa_egress_tm_level_get(subsidiary_tm, &level);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_level_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }
        if (!IS_SINGLE_LEVEL(level))
            *mode = rdpa_tm_sched_sp_wrr;
    }

    return TMCTL_SUCCESS;
}

static int get_queue_shaper(bdmf_object_handle root_tm,
                            bdmf_object_handle egress_tm,
                            rdpa_tm_sched_mode tm_mode,
                            tmctl_devType_e devType,
                            tmctl_if_t *if_p,
                            rdpa_tm_queue_cfg_t *queue_cfg,
                            tmctl_queueCfg_t *qcfg_p)
{
    rdpa_tm_rl_rate_mode rl_mode;
    bdmf_object_handle rate_limit_obj = BDMF_NULL;
    rdpa_rl_cfg_t rl_cfg = {0};
    tmctl_portTmParms_t tm ={0};
    int ret;

    get_port_tm_caps(devType, if_p, &tm);
    if(!tm.queueShaper)
    {
        qcfg_p->shaper.shapingRate = -1;
        qcfg_p->shaper.minRate = -1;
        qcfg_p->shaper.shapingBurstSize =-1;
        return TMCTL_SUCCESS;
    }

    ret = rdpa_egress_tm_rl_rate_mode_get(root_tm, &rl_mode);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_rl_rate_mode_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    if (tm_mode != rdpa_tm_sched_disabled)
    {
        /* get queue rate limit */
        if (queue_cfg->rl_cfg == BDMF_NULL)
        {
            rl_cfg.af_rate = 0;
            rl_cfg.be_rate = 0;
            ret = 0; 
        }
        else
        {
            ret = rdpa_rate_limit_cfg_get(queue_cfg->rl_cfg, &rl_cfg); 
        }
        if (rl_mode == rdpa_tm_rl_single_rate)
        {
            qcfg_p->shaper.shapingRate = BPS_TO_KBPS(rl_cfg.af_rate);
        }
        else
        {
            qcfg_p->shaper.shapingRate = BPS_TO_KBPS(rl_cfg.be_rate);
            qcfg_p->shaper.minRate = BPS_TO_KBPS(rl_cfg.af_rate);
        }
        tmctl_debug("shaper , shapingRate [%d], minRate[%d] shapingBurstSize[%d]",
                qcfg_p->shaper.shapingRate, qcfg_p->shaper.minRate, qcfg_p->shaper.shapingBurstSize);
    }
    else
    {
        /* get egress_tm rate limit */
        ret = rdpa_egress_tm_rate_limit_get(egress_tm, &rate_limit_obj);
        if ((ret) || (rate_limit_obj == BDMF_NULL))
        {
            rl_cfg.af_rate = 0;
            ret = 0; 
        }
        else
        {
            ret = rdpa_rate_limit_cfg_get(rate_limit_obj, &rl_cfg); 
        }

        if (ret)
        {
            tmctl_error("rdpa_egress_tm_rl_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }

        if (rl_mode == rdpa_tm_rl_dual_rate)
        {
            qcfg_p->shaper.minRate = BPS_TO_KBPS(rl_cfg.af_rate);
            if (rl_cfg.be_rate < 1000000000L)
                qcfg_p->shaper.shapingRate =  BPS_TO_KBPS(rl_cfg.af_rate + rl_cfg.be_rate);
        }
        else
            qcfg_p->shaper.shapingRate = BPS_TO_KBPS(rl_cfg.af_rate);

    }
    return TMCTL_SUCCESS;
}

static int switch_priority_and_idx(int prio_or_idx, int max_queues, tmctl_devType_e devType)
{
    BOOL is_shared;
    int ret;

    ret = get_shared_mode(devType, &is_shared);
    if (ret)
    {
        tmctl_error("is_shared_tm failed");
        return TMCTL_ERROR;
    }

#if HIGH_P_LOW_IDX
    prio_or_idx = max_queues - prio_or_idx - 1;
    return prio_or_idx;
#else
    return (is_shared) ?  ++prio_or_idx : prio_or_idx;
#endif
}

#define get_priority_by_index(idx, max_queues, devType) switch_priority_and_idx(idx, max_queues, devType)
#define get_index_by_priority(priority, max_queues, devType) switch_priority_and_idx(priority, max_queues, devType)

tmctl_ret_e tmctl_RdpaQueueCfgGet(tmctl_devType_e devType,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  tmctl_queueCfg_t* qcfg_p)
{
    bdmf_object_handle root_tm = BDMF_NULL;
    bdmf_object_handle egress_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_tm_sched_mode tm_mode;
    rdpa_tm_level_type level;
    uint8_t num_queues;
    int idx = 0;
    int ret;
    int index = 0;

    memset(qcfg_p, 0, sizeof(tmctl_queueCfg_t));

    get_root_tm(devType, if_p, &root_tm);
    if (!root_tm)
    {
        tmctl_error("Failed to get root_tm, dev[%s], qid[%u]", ifp_to_name(devType, if_p), queue_id);
        return TMCTL_ERROR;
    }

    ret = get_queue_tm(root_tm, queue_id, &egress_tm, &index);
    if (ret == TMCTL_NOT_FOUND)
    {
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(root_tm);
        }    
        return TMCTL_NOT_FOUND;
    }
    else if (ret)
    {
        tmctl_error("get_queue_tm failed, dev[%s], qid[%d]", ifp_to_name(devType, if_p), queue_id);
        goto error;
    }
    idx = index;

    ret = rdpa_egress_tm_mode_get(egress_tm, &tm_mode);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_mode_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
        goto error;
    }

    ret = rdpa_egress_tm_level_get(root_tm, &level);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_mode_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
        goto error;
    }

    ret = rdpa_egress_tm_queue_cfg_get(egress_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
        goto error;
    }

    ret = get_queue_shaper(root_tm, egress_tm, tm_mode, devType, if_p, &queue_cfg, qcfg_p);
    if (ret)
    {
        tmctl_error("get_queue_shaper failed, dev[%s], qid[%d]", ifp_to_name(devType, if_p), queue_id);
        goto error;
    }

    qcfg_p->qid = queue_id;
    qcfg_p->qsize = getUsrQueueSize(queue_cfg.drop_threshold);
    qcfg_p->minBufs = queue_cfg.reserved_packet_buffers;
    qcfg_p->bestEffort = queue_cfg.best_effort;

    if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_pi2 || queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_laqm)
        qcfg_p->l4s = 1;

    if (IS_SINGLE_LEVEL(level))
        qcfg_p->weight = queue_cfg.weight;
    else
    {
        bdmf_number weight;

        ret = rdpa_egress_tm_weight_get(egress_tm, &weight);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_weight_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
            goto error;
        }
        qcfg_p->weight = weight;
    }

    if (qcfg_p->weight)
    {
        qcfg_p->schedMode = TMCTL_SCHED_WRR;
        qcfg_p->priority = TMCTL_INVALID_KEY;
    }
    else
    {
        qcfg_p->schedMode = TMCTL_SCHED_SP;
        if (!IS_SINGLE_LEVEL(level))
        {
            bdmf_index  tm_idx = 0;

            ret = rdpa_egress_tm_subsidiary_find(root_tm, &tm_idx, &egress_tm);
            if (ret)
            {
                tmctl_error("rdpa_egress_tm_subsidiary_find failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
                goto error;
            }
            if (is_static_ctc_mode(devType, is_lan(devType, if_p)))
            {
                num_queues = STATIC_CTC_NUM_Q;
#ifdef BCM_XRDP
                /* in static ctc mode the lowest priority will be in index 1 the rest will be in index 0 */
                if (idx == 1)
                    idx = 7;
                else
                    idx = tm_idx;
#else
                idx = tm_idx;
#endif
            }
            else
            {
                num_queues = MAX_Q_PER_TM_MULTI_LEVEL;
                idx = tm_idx;
            }
        }
        else
        {
            ret = rdpa_egress_tm_num_queues_get(egress_tm, &num_queues);
            if (ret)
            {
                tmctl_error("rdpa_egress_tm_num_queues_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
                goto error;
            }
        }
        qcfg_p->priority = get_priority_by_index(idx, num_queues, devType);
        if (qcfg_p->priority < 0)
        {
            tmctl_error("get_priority_by_index failed, dev[%s]", ifp_to_name(devType, if_p));
            goto error;
        }
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_ERROR;
}


static int get_highest_wrr_q(bdmf_object_handle egress_tm, int ignord_id)
{
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_tm_num_sp_elem num_sp_queues;
    uint8_t max_queues;
    int ret;
    int i;

    ret = rdpa_egress_tm_num_queues_get(egress_tm, &max_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    ret = rdpa_egress_tm_num_sp_elements_get(egress_tm, &num_sp_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_sp_elements_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    for (i = num_sp_queues; i < max_queues ; ++i)
    {
        ret = rdpa_egress_tm_queue_cfg_get(egress_tm, i, &queue_cfg);
        if (ret && ret != BDMF_ERR_NOENT)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }

        if (!ret && IS_ACTIV_Q(queue_cfg) && queue_cfg.queue_id != ignord_id)
            break;
    }

    return i;
}

static rdpa_tm_num_sp_elem get_num_sp_queues(int highest_sp_q)
{
    if (highest_sp_q < 0)
        return rdpa_tm_num_sp_elem_0;
    if (highest_sp_q < 2)
        return rdpa_tm_num_sp_elem_2;
    if (highest_sp_q < 4)
        return rdpa_tm_num_sp_elem_4;
    if (highest_sp_q < 8)
        return rdpa_tm_num_sp_elem_8;
    if (highest_sp_q < 16)
        return rdpa_tm_num_sp_elem_16;
    return rdpa_tm_num_sp_elem_32;
}

/*
 * return the minimum number of SP queues if current queue(idx) will be remove
 */
static int num_sp_queues_after_rm(bdmf_object_handle egress_tm, bdmf_index idx, rdpa_tm_num_sp_elem *new_num_sp_queues)
{
    rdpa_tm_num_sp_elem curr_num_sp_queues;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    int highst_sp_q = -1;
    int ret;
    int i;

    ret = rdpa_egress_tm_num_sp_elements_get(egress_tm, &curr_num_sp_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_sp_elements_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    for (i = 0; i < curr_num_sp_queues; ++i)
    {
        /* skip current queue it will be removed */
        if (i != idx)
        {
            ret = rdpa_egress_tm_queue_cfg_get(egress_tm, i, &queue_cfg);
            if (ret && ret != BDMF_ERR_NOENT)
            {
                tmctl_error("rdpa_egress_tm_queue_cfg_get failed, ret[%d]", ret);
                return TMCTL_ERROR;
            }

            if (!ret && IS_ACTIV_Q(queue_cfg))
                highst_sp_q = i;
        }
    }

    *new_num_sp_queues = get_num_sp_queues(highst_sp_q);
    return TMCTL_SUCCESS;
}

/*
 * return the minimum number of SP queues if current queue(idx) will be add
 */
static int num_sp_queues_after_add(bdmf_object_handle egress_tm, bdmf_index idx, rdpa_tm_num_sp_elem *new_num_sp_queues)
{
    rdpa_tm_num_sp_elem curr_num_sp_queues;
    int ret;

    ret = rdpa_egress_tm_num_sp_elements_get(egress_tm, &curr_num_sp_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_sp_elements_set failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    *new_num_sp_queues = idx < curr_num_sp_queues ? curr_num_sp_queues : get_num_sp_queues(idx);
    return TMCTL_SUCCESS;
}

static void print_allowed_sched_mode(int mode)
{
    switch (mode)
    {
        case rdpa_tm_sched_sp_wrr:
            printf("Allowed sched modes are: SP and WRR.\n");
            break;
        case rdpa_tm_sched_sp:
             printf("Allowed sched mode is: SP.\n");
            break;
        case rdpa_tm_sched_wrr:
            printf("Allowed sched mode is: WRR.\n");
            break;
        case rdpa_tm_sched_disabled:
            printf("Sched mode is disabled.\n");
            break;
        default:
            tmctl_error("Sched mode %d, unknown.\n", mode);
            break;
    }
}

static int check_shared_subsidiary_tm(bdmf_object_handle egress_tm, int idx)
{
#ifdef TM_C_CODE
    int ret;
    bdmf_object_handle subsidiary_tm = BDMF_NULL;

    ret = rdpa_egress_tm_subsidiary_get(egress_tm, idx, &subsidiary_tm);
    if (ret == BDMF_ERR_NOENT)
    {
        return TMCTL_NOT_FOUND;
    }
    else if (ret)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
#else
    (void)(egress_tm);
    (void)(idx);
    return TMCTL_NOT_FOUND;
#endif
}

static int get_wrr_queue_idx(bdmf_object_handle egress_tm, int max, int min, int qid, BOOL is_dpi_q_ext)
{
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_tm_queue_location_t location;
    int ret;
    int i;

    ret = rdpa_egress_tm_queue_location_get(egress_tm, qid, &location);
    if (ret && ret != BDMF_ERR_NOENT)
    {
        tmctl_error("rdpa_egress_tm_queue_location_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    if(!ret && location.queue_idx >= min)
       return location.queue_idx;

#if defined(SUPPORT_DPI) && defined(TM_C_CODE)
    /* queue_cfg index [max-1] reserved by bestEffort subsidiary, wrr queue_cfg idx range is min ~ [max-2]
       max-2 is safe because max_queues only can be [8, 16, 32] */
    if (is_dpi_q_ext)
    {
        max -= 1;
    }
#else
    (void)(is_dpi_q_ext);
#endif

    for (i = max - 1 ; i >= min ; --i)
    {
        ret = rdpa_egress_tm_queue_cfg_get(egress_tm, i, &queue_cfg);
        if (ret && ret != BDMF_ERR_NOENT)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_get failed, ret[%d]", ret);
            return -1;
        }

        if ((ret || !IS_ACTIV_Q(queue_cfg)) && (check_shared_subsidiary_tm(egress_tm, i) == TMCTL_NOT_FOUND))
            return i;
    }

    tmctl_error("No free queue index between min[%d] and max[%d]", min, max);
    return -1;
}

static int prepare_set_q_in_sp_mode_singel_level(bdmf_object_handle egress_tm,
        tmctl_queueCfg_t *qcfg_p, int current_idx, BOOL is_dpi_q_ext)
{
    rdpa_tm_queue_cfg_t new_queue_cfg = {0};
    uint8_t max_queues;
    int idx;
    int ret;

    ret = rdpa_egress_tm_num_queues_get(egress_tm, &max_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, qid[%d], ret[%d]",qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }

    idx = get_index_by_priority(qcfg_p->priority, max_queues, 0);

#if defined(SUPPORT_DPI) && defined(TM_C_CODE)
    if (is_dpi_q_ext && idx == (max_queues-1))
    {
        tmctl_error("index[%d]) already reserved for bestEffort subsidiary tm, qid[%u], priority[%d]",
                idx, qcfg_p->qid, qcfg_p->priority);
        return TMCTL_ERROR;
    }
#else
    (void)(is_dpi_q_ext);
#endif

    ret = rdpa_egress_tm_queue_cfg_get(egress_tm, idx, &new_queue_cfg);
    if (ret && ret != BDMF_ERR_NOENT)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, qid[%u], index[%d], ret [%d]",qcfg_p->qid, idx, ret);
        return TMCTL_ERROR;
    }

    if (!ret && IS_ACTIV_Q(new_queue_cfg) && new_queue_cfg.queue_id != qcfg_p->qid)
    {
        tmctl_error("Priority[%d](index[%d]) already in use, qid[%u] current_qid[%d]",
                qcfg_p->priority, idx, qcfg_p->qid, new_queue_cfg.queue_id);
        return TMCTL_ERROR;
    }

    /* if changing priority delete existing queue */
    if (idx != current_idx && (current_idx >= 0))
    {
        ret = rdpa_egress_tm_queue_cfg_delete(egress_tm, current_idx);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_delete failed, qid[%d], currnet index[%d], ret[%d]",qcfg_p->qid, current_idx, ret);
            return TMCTL_ERROR;
        }
    }

    return idx;
}

static int prepare_set_q_in_wrr_mode_singel_level(bdmf_object_handle egress_tm, 
        tmctl_queueCfg_t *qcfg_p, int idx, BOOL is_dpi_q_ext)
{
    uint8_t max_queues;
    int ret;

    /* queue exist */
    if (idx >= 0)
        goto exit;

    ret = rdpa_egress_tm_num_queues_get(egress_tm, &max_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, qid[%u], ret[%d]",qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }

    idx = get_wrr_queue_idx(egress_tm, max_queues, 0, qcfg_p->qid, is_dpi_q_ext);

exit:
    return idx;
}

static int prepare_set_wrr_q_in_sp_wrr_mode_single_level(bdmf_object_handle egress_tm,
                                                tmctl_queueCfg_t *qcfg_p,
                                                bdmf_index idx,
                                                rdpa_tm_num_sp_elem *num_sp_queues_,
                                                uint8_t max_queues,
                                                BOOL is_dpi_q_ext)
{
    rdpa_tm_num_sp_elem num_sp_queues;
    int new_idx;
    int ret;

    ret = num_sp_queues_after_rm(egress_tm, idx, &num_sp_queues);
    if (ret)
    {
        tmctl_error("Failed to get new number of SP queues, qid[%d]",  qcfg_p->qid);
        return TMCTL_ERROR;
    }

    new_idx = get_wrr_queue_idx(egress_tm, max_queues, num_sp_queues, qcfg_p->qid, is_dpi_q_ext);
    if (new_idx == -1)
    {
        tmctl_error("No place for new WRR queue, qid[%d]",  qcfg_p->qid);
        return TMCTL_ERROR;
    }

    *num_sp_queues_ = num_sp_queues;
    return new_idx;
}

static int is_queue_index_resereved(tmctl_devType_e devType, int idx)
{
    BOOL is_shared;
    int ret;

    ret = get_shared_mode(devType, &is_shared);
    if (ret)
    {
        tmctl_error("get_shared_mode failed");
        return TMCTL_ERROR;
    }

    if (is_shared && (idx == 0))
        return TRUE;

    return FALSE;
}

static int prepare_set_sp_q_in_sp_wrr_mode_single_level(bdmf_object_handle egress_tm,
                                                        tmctl_queueCfg_t *qcfg_p,
                                                        int idx,
                                                        rdpa_tm_num_sp_elem *num_sp_queues_,
                                                        uint8_t max_queues,
                                                        tmctl_devType_e devType,
                                                        BOOL is_dpi_q_ext)
{
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_tm_num_sp_elem num_sp_queues;
    int highest_wrr_q;
    int new_idx;
    int ret;

    if (is_queue_index_resereved(devType, idx))
    {
        tmctl_error("queue is resereved");
        return TMCTL_ERROR;
    }
    new_idx = get_index_by_priority(qcfg_p->priority, max_queues, devType);

#if defined(SUPPORT_DPI) && defined(TM_C_CODE)
    if (is_dpi_q_ext && new_idx == (max_queues-1))
    {
        tmctl_error("index[%d]) already reserved for bestEffort subsidiary tm, qid[%u], priority[%d]",
                new_idx, qcfg_p->qid, qcfg_p->priority);
        return TMCTL_ERROR;
    }
#else
    (void)(is_dpi_q_ext);
#endif

    ret = rdpa_egress_tm_queue_cfg_get(egress_tm, new_idx, &queue_cfg);
    if (ret && ret != BDMF_ERR_NOENT)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, qid[%d], index[%d], ret[%d]",  qcfg_p->qid, new_idx, ret);
        return TMCTL_ERROR;
    }

    if (!ret && IS_ACTIV_Q(queue_cfg) && queue_cfg.queue_id != qcfg_p->qid)
    {
        tmctl_error("No place for new SP queue, qid[%d] index[%d], priority[%d] is taken by currnet qid[%d]",
                qcfg_p->qid, new_idx, qcfg_p->priority, queue_cfg.queue_id);
        return TMCTL_ERROR;
    }

    ret = num_sp_queues_after_add(egress_tm, new_idx, &num_sp_queues);
    if (ret)
    {
        tmctl_error("Fail to get new number of SP queues, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }

    /* check that there are no WRR queues in new SP queues area */
    highest_wrr_q = get_highest_wrr_q(egress_tm, qcfg_p->qid);
    if (highest_wrr_q < 0)
    {
        tmctl_error("Fail to get highest WRR queue, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }

    if (highest_wrr_q < num_sp_queues)
    {
        tmctl_error("No place for new SP queue, qid[%d], priority[%d] highest WRR queue[%d] "
                  "num SP queues[%d], max queues[%d]",
                  qcfg_p->qid, qcfg_p->priority, highest_wrr_q, num_sp_queues, max_queues);
        return TMCTL_ERROR;
    }

    *num_sp_queues_ = num_sp_queues;
    return new_idx;
}

static int prepare_set_q_in_sp_wrr_mode_singel_level(bdmf_object_handle egress_tm,
                                                     tmctl_queueCfg_t *qcfg_p,
                                                     int current_idx,
                                                     tmctl_devType_e devType,
                                                     BOOL is_dpi_q_ext)
{
    rdpa_tm_num_sp_elem num_sp_queues;
    rdpa_tm_queue_cfg_t cur_queue_cfg = {0};
    uint8_t max_queues;
    int new_idx;
    int ret;

    ret = rdpa_egress_tm_num_queues_get(egress_tm, &max_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, qid[%d], ret[%d]",  qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }

    if (qcfg_p->schedMode == TMCTL_SCHED_WRR)
        new_idx = prepare_set_wrr_q_in_sp_wrr_mode_single_level(egress_tm, qcfg_p, current_idx, &num_sp_queues, max_queues, is_dpi_q_ext);
    else
        new_idx = prepare_set_sp_q_in_sp_wrr_mode_single_level(egress_tm, qcfg_p, current_idx, &num_sp_queues, max_queues, devType, is_dpi_q_ext);

    if (new_idx < 0)
        return TMCTL_ERROR;

    if (current_idx >= 0)
    {
        ret = rdpa_egress_tm_queue_cfg_get(egress_tm, current_idx, &cur_queue_cfg);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_get failed, qid[%d], index[%d], ret[%d]",  qcfg_p->qid, new_idx, ret);
            return TMCTL_ERROR;
        }
    }

    /* if changing position or sched mode delete queue in current position */
    if ((current_idx >= 0 && new_idx != current_idx) || ((current_idx >= 0) &&
        ((cur_queue_cfg.weight && qcfg_p->schedMode == TMCTL_SCHED_SP) ||
        (!cur_queue_cfg.weight && qcfg_p->schedMode == TMCTL_SCHED_WRR))))
    {
        ret = rdpa_egress_tm_queue_cfg_delete(egress_tm, current_idx);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_delete failed, qid[%d], currnet index[%d], ret[%d]",qcfg_p->qid, current_idx, ret);
            return TMCTL_ERROR;
        }
    }

    ret = rdpa_egress_tm_num_sp_elements_set(egress_tm, num_sp_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_sp_elements_set failed, qid[%d], index[%d], ret[%d]",qcfg_p->qid, new_idx, ret);
        return TMCTL_ERROR;
    }

    return new_idx;
}

static int check_sched_mode(int wanted_sched_mode, int allowd_sched_mode)
{
    if ((wanted_sched_mode == TMCTL_SCHED_SP &&
        allowd_sched_mode != rdpa_tm_sched_sp  && allowd_sched_mode != rdpa_tm_sched_sp_wrr) ||
        (wanted_sched_mode == TMCTL_SCHED_WRR &&
        allowd_sched_mode != rdpa_tm_sched_wrr && allowd_sched_mode != rdpa_tm_sched_sp_wrr) ||
        allowd_sched_mode == rdpa_tm_sched_disabled ||
        (wanted_sched_mode != TMCTL_SCHED_SP && wanted_sched_mode != TMCTL_SCHED_WRR))
    {
        tmctl_error("Queue sched mode %d is not supported", wanted_sched_mode);
        print_allowed_sched_mode(allowd_sched_mode);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static int is_cfg_supported(tmctl_queueCfg_t *qcfg_p, get_root_param_t *root_param, tmctl_portTmParms_t *tm)
{
    if (qcfg_p->qid >= MAX_USER_QUEUE_ID)
    {
        tmctl_error("Queue ID is %d but it must be smaller than %d\n", qcfg_p->qid, MAX_USER_QUEUE_ID);
        return FALSE;
    }

    if (qcfg_p->bestEffort && qcfg_p->l4s)
    {
        tmctl_error("Queue can be either best effort or l4s");
        return FALSE;
    }

    if (IS_SINGLE_LEVEL(root_param->level))
    {
        if (qcfg_p->schedMode != TMCTL_SCHED_WRR && (qcfg_p->priority > root_param->max_queues || qcfg_p->priority < 0))
        {
            tmctl_error("Priority of SP queue must < %d and >= 0, qid[%d] priority[%d]",
                    root_param->max_queues, qcfg_p->qid, qcfg_p->priority);
            return FALSE;
        }
    }
    else
    {
        if (qcfg_p->schedMode != TMCTL_SCHED_WRR && (qcfg_p->priority > MAX_Q_PER_TM_MULTI_LEVEL || qcfg_p->priority < 0))
        {
            tmctl_error("Priority of SP queue must < %d and >= 0, qid[%d] priority[%d]",
                    MAX_Q_PER_TM_MULTI_LEVEL, qcfg_p->qid, qcfg_p->priority);
            return FALSE;
        }

        if (qcfg_p->shaper.shapingRate < qcfg_p->shaper.minRate)
        {
            tmctl_error("Shaper configuration not allowed maxRate(%u) < minRate(%u)",
                        qcfg_p->shaper.shapingRate, qcfg_p->shaper.minRate);
            return FALSE;
        }
    }

    if ((qcfg_p->schedMode == TMCTL_SCHED_WRR) &&
            (qcfg_p->weight == 0))
    {
        tmctl_error("Weight of WRR/WFQ queue must be non-zero. qid[%d]", qcfg_p->qid);
        return FALSE;
    }

    if (check_sched_mode(qcfg_p->schedMode, root_param->mode))
        return FALSE;

    if (!tm->queueShaper && (qcfg_p->shaper.shapingRate || qcfg_p->shaper.minRate || qcfg_p->shaper.shapingBurstSize))
    {
        tmctl_error("Queue shaper is not supported on this port");
        return FALSE;
    }

    return TRUE;
}

static int setup_queue_tm(bdmf_object_handle root_tm,
                           bdmf_index index,
                           int weight,
                           tmctl_queueCfg_t *qcfg_p,
                           bdmf_object_handle *queue_tm,
                           tmctl_devType_e devType)
{
    set_parm_t set_parm = {0};
    int ret;

    set_parm.devType = devType;
    set_parm.tm_level = rdpa_tm_level_queue;
    set_parm.tm_mode = rdpa_tm_sched_disabled;
    set_parm.subsidiary_idx = index;
    set_parm.weight = weight;

    ret = setup_tm(&set_parm, root_tm, queue_tm);
    if (ret)
    {
        tmctl_error("Failed to setup queue tm");
        return TMCTL_ERROR;
    }
    return TMCTL_SUCCESS;
}

static int find_free_subsidiary_idx(bdmf_object_handle root_tm , int *idx)
{
    bdmf_object_handle subsidiary_tm = BDMF_NULL;
    int ret;
    int i;

    for (i = 0 ; i < MAX_Q_PER_TM_MULTI_LEVEL ; ++i)
    {
        ret = rdpa_egress_tm_subsidiary_get(root_tm, i, &subsidiary_tm);
        if (ret == BDMF_ERR_NOENT)
        {
            *idx = i;
            return TMCTL_SUCCESS;
        }
        else if(ret)
        {
            tmctl_error("rdpa_egress_tm_subsidiary_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }
    }

    tmctl_error("No free subsidiary idx for new queue");
    return TMCTL_NOT_FOUND;
}

static int move_tm_to_new_index(bdmf_object_handle root_tm, bdmf_object_handle queue_tm, int new_idx)
{
    bdmf_object_handle parent_tm = queue_tm;
    bdmf_index old_idx;
    int ret;

    ret = rdpa_egress_tm_subsidiary_find(root_tm, &old_idx, &parent_tm);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_find failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    ret = rdpa_egress_tm_subsidiary_set(root_tm, old_idx, BDMF_NULL);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_set failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    ret = rdpa_egress_tm_subsidiary_set(root_tm, new_idx, queue_tm);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_set failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }
    return TMCTL_SUCCESS;
}

static int prepare_set_q_wrr_multi_level(bdmf_object_handle root_tm, tmctl_queueCfg_t *qcfg_p, bdmf_object_handle *egress_tm, tmctl_devType_e devType)
{
    int idx;
    int ret = 0;

    if (!*egress_tm)
    {
        ret = get_queue_tm(root_tm, qcfg_p->qid, egress_tm, &idx);
        if (ret && ret != TMCTL_NOT_FOUND)
        {
            tmctl_error("get_queue_tm failed, qid[%u]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
    }
    if (ret == TMCTL_NOT_FOUND)
    {
        ret = find_free_subsidiary_idx(root_tm, &idx);
        if (ret == TMCTL_NOT_FOUND)
        {
            tmctl_error("No free subsidiary index for new queue, qid[%u]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
        else if (ret)
        {
            tmctl_error("find_free_subsidiary_idx failed, qid[%u]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
        return setup_queue_tm(root_tm, idx, qcfg_p->weight, qcfg_p, egress_tm, devType);
    }

    ret = rdpa_egress_tm_weight_set(*egress_tm, qcfg_p->weight);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_weight_set failed, qid[%d], ret[%d]", qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static inline int set_queue_tm_on_empty_index(bdmf_object_handle root_tm,
                                              tmctl_queueCfg_t *qcfg_p,
                                              int new_idx,
                                              bdmf_object_handle *egress_tm,
                                              tmctl_devType_e devType,
                                              BOOL is_static_ctc)
{
    int ret;

    if (*egress_tm)
    {
        if (is_static_ctc)
        {
            tmctl_error("Can't change priorty of queues in static ctc mode");
            return TMCTL_ERROR;
        }

        ret = move_tm_to_new_index(root_tm, *egress_tm, new_idx);
        if (ret)
        {
            tmctl_error("move_tm_to_new_index failed, qid[%d]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
    }
    else
        return setup_queue_tm(root_tm, new_idx, 0, qcfg_p, egress_tm, devType);

    return TMCTL_SUCCESS;
}

static int prepare_set_q_sp_multi_level(bdmf_object_handle root_tm, tmctl_queueCfg_t *qcfg_p, bdmf_object_handle *egress_tm, tmctl_devType_e devType, BOOL is_static_ctc)
{
    bdmf_object_handle current_tm = BDMF_NULL;
    int new_idx;
    int queue_idx;
    int ret;
    int max_queues;

    if (!*egress_tm)
    { /* if egress_tm wasn't given search for it */
        ret = get_queue_tm(root_tm, qcfg_p->qid, egress_tm, &queue_idx);
        if (ret && ret != TMCTL_NOT_FOUND)
        {
            tmctl_error("get_queue_tm failed, qid[%d]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
    }

    max_queues = is_static_ctc ? STATIC_CTC_NUM_Q : MAX_Q_PER_TM_MULTI_LEVEL;
    new_idx = get_index_by_priority(qcfg_p->priority, max_queues, 0);

    ret = rdpa_egress_tm_subsidiary_get(root_tm, new_idx, &current_tm);
    if (ret && ret != BDMF_ERR_NOENT)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_get failed, qid[%d], ret[%d]", qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }

    if (ret == BDMF_ERR_NOENT)
        return set_queue_tm_on_empty_index(root_tm, qcfg_p, new_idx, egress_tm, devType, is_static_ctc);

    /* if queue in wanted index not the same as configured queue priority is taken */
    if (current_tm != *egress_tm)
    {
        tmctl_error("Priority[%d] already in use", qcfg_p->priority);
        return BDMF_ERR_ALREADY;
    }

    return TMCTL_SUCCESS;
}

static int get_subsidiary_tm(bdmf_object_handle root_tm, tmctl_sched_e sched_mode, bdmf_object_handle *subsidiary_tm)
{
    int ret;
    bdmf_index idx = (sched_mode == TMCTL_SCHED_SP)? SP_SUBSIDIARY_IDX : WRR_SUBSIDIARY_IDX;

    ret = rdpa_egress_tm_subsidiary_get(root_tm, idx ,subsidiary_tm);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static int get_current_parent(bdmf_object_handle root_tm, bdmf_object_handle queue_tm, bdmf_object_handle *parent)
{
    rdpa_tm_sched_mode mode;
    bdmf_number weight = 0;
    bdmf_index sub_id;

    int ret;

    ret = get_tm_mode(root_tm, &mode);
    if (ret)
    {
        tmctl_error("get_tm_mode failed");
        return TMCTL_ERROR;
    }

    if (mode != rdpa_tm_sched_sp_wrr) /* if mode is wrr or sp parent is root */
    {
        *parent = root_tm;
        return TMCTL_SUCCESS;
    }

    ret = rdpa_egress_tm_weight_get(queue_tm, &weight);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_weight_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    /* WRR queues must have weight and SP queues can't */
    sub_id = (weight) ? WRR_SUBSIDIARY_IDX : SP_SUBSIDIARY_IDX;
    ret = rdpa_egress_tm_subsidiary_get(root_tm, sub_id, parent);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static int remove_if_switch_tm(bdmf_object_handle root_tm, bdmf_object_handle new_parent, tmctl_queueCfg_t *qcfg_p, bdmf_object_handle *queue_tm)
{
    bdmf_object_handle old_parent = BDMF_NULL;
    int idx;
    int ret;

    ret = get_queue_tm(root_tm, qcfg_p->qid, queue_tm, &idx);
    if (ret == TMCTL_ERROR)
    {
        tmctl_error("get_queue_tm failed, qid[%d], ret[%d]", qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }
    else if (!ret)
    {
        ret = get_current_parent(root_tm, *queue_tm, &old_parent);
        if (ret)
        {
            tmctl_error("get_current_parent failed, qid[%d], ret[%d]", qcfg_p->qid, ret);
            return TMCTL_ERROR;
        }

        if (new_parent != old_parent)
        {
            ret = bdmf_destroy(*queue_tm);
            if (ret)
            {
                tmctl_error("bdmf_destroy failed, qid[%d], ret[%d]", qcfg_p->qid, ret);
                return TMCTL_ERROR;
            }
            *queue_tm = BDMF_NULL;
        }
    }

    return TMCTL_SUCCESS;
}

static int prepare_set_q_in_sp_wrr_mode_level_multi_level(bdmf_object_handle root_tm, tmctl_queueCfg_t *qcfg_p, bdmf_object_handle *egress_tm, tmctl_devType_e devType)
{
    bdmf_object_handle subsidiary_tm = BDMF_NULL;
    int ret;

    ret = get_subsidiary_tm(root_tm, qcfg_p->schedMode, &subsidiary_tm);
    if (ret)
    {
        tmctl_error("get_subsidiary_tm failed, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }

    /* if queue change mode remove it before seting new mode */
    ret = remove_if_switch_tm(root_tm, subsidiary_tm, qcfg_p, egress_tm);
    if (ret)
    {
        tmctl_error("remove_if_switch_tm failed, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }

    if (qcfg_p->schedMode == TMCTL_SCHED_SP)
       ret = prepare_set_q_sp_multi_level(subsidiary_tm, qcfg_p, egress_tm, devType, FALSE);
    else
       ret = prepare_set_q_wrr_multi_level(subsidiary_tm, qcfg_p, egress_tm, devType);
    if (ret)
    {
        tmctl_error("Failed to prepare set queue, qid[%d], ret[%d]", qcfg_p->qid, ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

#ifdef BCM_XRDP
static int static_ctc_shared_tm_set(bdmf_object_handle root_tm, tmctl_queueCfg_t *qcfg_p, bdmf_object_handle *egress_tm)
{
    bdmf_object_handle shared_tm = BDMF_NULL;
    set_parm_t set_parm = {0};
    int ret;

    ret = rdpa_egress_tm_subsidiary_get(root_tm, STATIC_CTC_SHARED_TM_IDX, &shared_tm);
    if (ret && ret != BDMF_ERR_NOENT)
    {
        tmctl_error("rdpa_egress_tm_subsidiary_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }
    if (ret)
    {
        set_parm.tm_level = rdpa_tm_level_queue;
        set_parm.tm_mode = rdpa_tm_sched_sp;
        set_parm.num_queues = STATIC_CTC_NUM_Q;
        set_parm.subsidiary_idx = STATIC_CTC_SHARED_TM_IDX;

        ret = setup_tm(&set_parm, root_tm, &shared_tm);
        if (ret)
        {
           tmctl_error("setup_tm failed");
           return TMCTL_ERROR;
        }
    }
    else
    {
        int index = qcfg_p->priority == 0 ? 1 : 0;
        rdpa_tm_queue_cfg_t queue_cfg;

        ret = rdpa_egress_tm_queue_cfg_get(shared_tm, index ,&queue_cfg);
        if (ret && ret != BDMF_ERR_NOENT)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_get failed, ret[%d]", ret);
            return TMCTL_ERROR;
        }
        else if (!ret && queue_cfg.queue_id != qcfg_p->qid)
        {
            tmctl_error("Priority[%d] already taken by qid[%d]", qcfg_p->priority, queue_cfg.queue_id);
            return TMCTL_ERROR;
        }
    }
    *egress_tm = shared_tm;
    return TMCTL_SUCCESS;
}
#endif

/***************************************************************
 * XRDP static CTC egress_tm configuration.
 * Maximum 8 queues. the two lowest priority queues share one queue_tm.
 * The lowest priority(0) will be in queue index 1 and the second(1) in index 0.
 * The rest of the queues will act the same as multi level RDP,
 * each queue will get its own queue_tm and the index will be 0.
 * Hight prirty will be in the lowest subsidiart index.
 *
 *                            ______
 * ____     |--sub_index 0 ---|Q TM|-- q index 0 prio 7
 * |TM|     |                 |____|
 * |__|-----|                 ______
 *          |--sub_index 1----|Q TM|-- q index 0 prio 6
 *          |                 |____|
 *          |                 ______
 *          |--sub_index 6----|Q TM|-- q index 0 prio 1
 *                            |____|-- q index 1 prio 0
 *
 ****************************************************************/
static int static_ctc_queue_set(bdmf_object_handle root_tm, tmctl_queueCfg_t *qcfg_p, bdmf_object_handle *egress_tm, tmctl_devType_e devType, int *idx)
{
    int ret;

#ifdef BCM_XRDP
    *idx = (qcfg_p->priority == 0) ? 1 : 0;
    if (qcfg_p->priority == 1 || qcfg_p->priority == 0)
    {
        ret = static_ctc_shared_tm_set(root_tm, qcfg_p, egress_tm);
        if (ret)
        {
           tmctl_error("static_ctc_shared_tm_set failed");
           return TMCTL_ERROR;
        }
        return TMCTL_SUCCESS;
    }
#endif

    ret = prepare_set_q_sp_multi_level(root_tm, qcfg_p, egress_tm, devType, TRUE);
    if (ret)
    {
        tmctl_error("prepare_set_q_sp_multi_level failed, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }
    return TMCTL_SUCCESS;
}


static inline int prepare_set_q_in_multi_level(bdmf_object_handle root_tm,
                                        rdpa_tm_sched_mode mode,
                                        tmctl_queueCfg_t *qcfg_p,
                                        bdmf_object_handle *egress_tm,
                                        tmctl_devType_e devType)
{
    int ret;

    switch (mode)
    {
        case rdpa_tm_sched_sp:
            ret = prepare_set_q_sp_multi_level(root_tm, qcfg_p, egress_tm, devType, FALSE);
            if (ret)
            {
                tmctl_error("prepare_set_q_sp_multi_level failed, qid[%d]", qcfg_p->qid);
                return TMCTL_ERROR;
            }
            break;
        case rdpa_tm_sched_wrr:
            ret = prepare_set_q_wrr_multi_level(root_tm, qcfg_p, egress_tm, devType);
            if (ret)
            {
                tmctl_error("prepare_set_q_wrr_multi_level failed, qid[%d]", qcfg_p->qid);
                return TMCTL_ERROR;
            }
            break;
        case rdpa_tm_sched_sp_wrr:
            ret = prepare_set_q_in_sp_wrr_mode_level_multi_level(root_tm, qcfg_p, egress_tm, devType);
            if (ret)
            {
                tmctl_error("prepare_set_q_in_sp_wrr_mode_level_multi_level failed, qid[%d]", qcfg_p->qid);
                return TMCTL_ERROR;
            }
            break;
        default:
            tmctl_error("Failed to get TM mode, qid[%d]", qcfg_p->qid);
            return TMCTL_ERROR;
    }

    ret = set_queue_rl_multi_level(root_tm, *egress_tm, &qcfg_p->shaper);
    if (ret)
    {
        tmctl_error("set_queue_rl_multi_level failed, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static inline int prepare_set_q_in_singel_level(bdmf_object_handle egress_tm,
                                         tmctl_queueCfg_t *qcfg_p,
                                         rdpa_tm_sched_mode mode,
                                         int *idx,
                                         tmctl_devType_e devType,
                                         BOOL is_dpi_q_ext)
{
    int current_idx = TMCTL_INVALID_KEY;
    int ret;

    ret = get_q_idx(egress_tm, qcfg_p->qid, &current_idx);
    if (ret && ret != TMCTL_NOT_FOUND)
    {
        tmctl_error("get_q_idx failed, qid[%d]", qcfg_p->qid);
        return TMCTL_ERROR;
    }

    switch (mode)
    {
        case rdpa_tm_sched_sp:
            *idx = prepare_set_q_in_sp_mode_singel_level(egress_tm, qcfg_p, current_idx, is_dpi_q_ext);
            if (*idx < 0)
            {
                tmctl_error("prepare_set_q_in_sp_mode_singel_level failed, qid[%u]", qcfg_p->qid);
                return TMCTL_ERROR;
            }
            break;
        case rdpa_tm_sched_wrr:
            *idx = prepare_set_q_in_wrr_mode_singel_level(egress_tm, qcfg_p, current_idx, is_dpi_q_ext);
            if (*idx < 0)
            {
                tmctl_error("prepare_set_q_in_wrr_mode_singel_level failed, qid[%u]", qcfg_p->qid);
                return TMCTL_ERROR;
            }
            break;
        case rdpa_tm_sched_sp_wrr:
            *idx = prepare_set_q_in_sp_wrr_mode_singel_level(egress_tm, qcfg_p, current_idx, devType, is_dpi_q_ext);
            if (*idx < 0)
            {
                tmctl_error("prepare_set_q_in_sp_wrr_mode_singel_level failed, qid[%u]", qcfg_p->qid);
                return TMCTL_ERROR;
            }
            break;
        default:
            tmctl_error("Mode %d not supported, qid[%u]", mode, qcfg_p->qid);
            return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

static int get_root_param(tmctl_devType_e devType, tmctl_if_t *if_p, get_root_param_t *root_param)
{
    int ret;

    get_root_tm(devType, if_p, &root_param->root_tm);
    if (!root_param->root_tm)
    {
        tmctl_error("Failed to get egress_tm, dev[%s]", ifp_to_name(devType, if_p));
        return TMCTL_ERROR;
    }

    ret = get_tm_mode(root_param->root_tm, &root_param->mode);
    if (ret)
    {
        tmctl_error("get_tm_mode failed, dev[%s], ret[%d]", ifp_to_name(devType, if_p), ret);
        goto error;
    }

    ret = rdpa_egress_tm_level_get(root_param->root_tm, &root_param->level);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_level_get failed, dev[%s], ret[%d]", ifp_to_name(devType, if_p), ret);
        goto error;
    }

    if (IS_SINGLE_LEVEL(root_param->level))
    {
        ret = rdpa_egress_tm_num_queues_get(root_param->root_tm, &root_param->max_queues);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_num_queues_get failed, dev[%s], ret[%d]", ifp_to_name(devType, if_p), ret);
            goto error;
        }
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_param->root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_param->root_tm);
    }
    return TMCTL_ERROR;   
}

static int prepare_set_q(get_root_param_t *root_param,
                         tmctl_queueCfg_t *qcfg_p,
                         int *idx,
                         bdmf_object_handle *egress_tm,
                         tmctl_devType_e devType,
                         BOOL is_static_ctc,
                         BOOL is_dpi_q_ext)
{
    int ret;

    if (!IS_SINGLE_LEVEL(root_param->level))
    {
        *idx = 0;
        if (is_static_ctc)
            return static_ctc_queue_set(root_param->root_tm, qcfg_p, egress_tm, devType, idx);

        ret = prepare_set_q_in_multi_level(root_param->root_tm, root_param->mode, qcfg_p, egress_tm, devType);
        if (ret)
        {
            tmctl_error("prepare_set_q_in_multi_level, qid[%d]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
    }
    else
    {
        *egress_tm = root_param->root_tm;
        ret = prepare_set_q_in_singel_level(*egress_tm, qcfg_p, root_param->mode, idx, devType, is_dpi_q_ext);
        if (ret)
        {
            tmctl_error("prepare_set_q_in_singel_level, qid[%d]", qcfg_p->qid);
            return TMCTL_ERROR;
        }
    }

    return TMCTL_SUCCESS;
}

static inline void queue_shaper_cfg_set(rdpa_tm_rl_rate_mode rl_mode,
                                        tmctl_shaper_t *shaper_p,
                                        rdpa_tm_queue_cfg_t *queue_cfg)
{
    bdmf_object_handle rate_limit_obj = {0};
    rdpa_rl_cfg_t rl_cfg = {0};
    int ret;

    if (queue_cfg->rl_cfg == BDMF_NULL)
    {
        /* need to create new rate limit object */
        bdmf_mattr_handle mattr = BDMF_NULL;

        /* first verify it not empty rate limit*/
        if (shaper_p->shapingRate <= 0)
        {
            /* in this case, no need real rate limit, it empty or not supported */
            return;
        }
        mattr = bdmf_mattr_alloc(rdpa_rate_limit_drv());
        if (!mattr)
        {
            tmctl_error("bdmf_mattr_alloc failed");
            return;
        }
  
        ret = bdmf_new_and_set(rdpa_rate_limit_drv(), NULL, mattr, &rate_limit_obj);
        bdmf_mattr_free(mattr);
        if (ret)
        {
            tmctl_error("bdmf_new_and_set failed to create rate limit obj, ret[%d]", ret);
            return;
        }
        queue_cfg->rl_cfg = rate_limit_obj;
    }
    else
    {
        if (shaper_p->shapingRate <= 0)
        {
            /* we have rate limit and want to put zero, i will delete the rate limit object*/
            bdmf_destroy(rate_limit_obj);
        }
    }

    if (rl_mode == rdpa_tm_rl_dual_rate)
    {
        rl_cfg.be_rate = KBPS_TO_BPS(shaper_p->shapingRate);
        rl_cfg.af_rate = KBPS_TO_BPS(shaper_p->minRate);
    }
    else
    {
        /* Single mode, variable af_rate used as best effort rate*/
        rl_cfg.af_rate = KBPS_TO_BPS(shaper_p->shapingRate);
    }
    rdpa_rate_limit_cfg_set(queue_cfg->rl_cfg, &rl_cfg);
    tmctl_debug("shaper_cfg : , shapingRate [%d], minRate[%d] shapingBurstSize[%d]",
                shaper_p->shapingRate, shaper_p->minRate, shaper_p->shapingBurstSize);

}

static inline void prepare_queue_struct(tmctl_queueCfg_t *qcfg_p,
                                        rdpa_tm_queue_cfg_t *queue_cfg,
                                        rdpa_tm_rl_rate_mode rl_rate_mode,
                                        bdmf_object_handle egress_tm,
                                        int idx)
{
    rdpa_tm_queue_cfg_t saved_queue_cfg = {0};

    queue_cfg->queue_id = qcfg_p->qid;
    queue_cfg->drop_threshold = getDeviceQueueSize(qcfg_p->qsize);
#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP) /* in rdp shaper is set on queue tm */
    queue_shaper_cfg_set(rl_rate_mode, &qcfg_p->shaper, queue_cfg);
#endif
    queue_cfg->stat_enable = 1;
    /* change configured weight to 0 for SP */
    queue_cfg->weight = qcfg_p->schedMode == TMCTL_SCHED_SP ? 0 : qcfg_p->weight;
    queue_cfg->reserved_packet_buffers = qcfg_p->minBufs;
    queue_cfg->best_effort = qcfg_p->bestEffort;

    /* restore drop_alg setting from previous egress_tm queue_cfg if exists */
    if(rdpa_egress_tm_queue_cfg_get(egress_tm, idx, &saved_queue_cfg))
    {
        queue_cfg->drop_alg = rdpa_tm_drop_alg_dt;
    }
    else
    {
        queue_cfg->drop_alg = saved_queue_cfg.drop_alg;
        queue_cfg->high_class = saved_queue_cfg.high_class;
        queue_cfg->low_class = saved_queue_cfg.low_class;
        queue_cfg->priority_mask_0 = saved_queue_cfg.priority_mask_0;
        queue_cfg->priority_mask_1 = saved_queue_cfg.priority_mask_1;
    }
}

tmctl_ret_e tmctl_RdpaTmQueueSet(tmctl_devType_e devType,
        tmctl_if_t *if_p, tmctl_queueCfg_t *qcfg_p)
{
    bdmf_object_handle egress_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    get_root_param_t root_param = {0};
    tmctl_queueCfg_t saved_qcfg_p = {0};
    tmctl_portTmParms_t tm = {0};
    BOOL restore_queue = FALSE;
    rdpa_tm_rl_rate_mode rl_rate_mode = rdpa_tm_rl_single_rate;
    static int depth = 0;
    int idx;
    int ret;
    BOOL is_static_ctc = is_static_ctc_mode(devType, is_lan(devType, if_p));

    get_port_tm_caps(devType, if_p, &tm);

    ret = get_root_param(devType, if_p, &root_param);
    if (ret)
    {
        tmctl_error("get_root_param failed, dev[%s], qid[%d]",ifp_to_name(devType, if_p), qcfg_p->qid);
        return TMCTL_ERROR;
    }


    ret = tmctl_RdpaQueueCfgGet(devType, if_p, qcfg_p->qid, &saved_qcfg_p);
    if (ret && ret != TMCTL_NOT_FOUND)
    {
        tmctl_error("tmctl_RdpaQueueCfgGet failed, dev[%s], qid[%d]",
                ifp_to_name(devType, if_p), qcfg_p->qid);
        goto error;
    }
    else if (!ret)
        restore_queue = TRUE;

    ignore_negtiv_shaper(&qcfg_p->shaper);

    if ((qcfg_p->l4s) && (saved_qcfg_p.l4s))
    {
        /* When L4S is first created the configuration relates to the secondary egress_tm, and it creates two queues under it.
         * When it is updated, the same code operates on the first queue under the secondary egress_tm.
         * To avoid issues we handle L4S udpate specifically here.
         * The only parameters that can be changed are: weight and shaping for the secondary egress_tm and size for the queues.
         * The other parameters are ignored.
         */
        return l4s_secondary_egress_tm_update(devType, root_param.root_tm, qcfg_p);
    }

    ret = !is_cfg_supported(qcfg_p, &root_param, &tm);
    if (ret)
    {
        tmctl_error("is_cfg_supported failed, dev[%s], qid[%d]", ifp_to_name(devType,if_p), qcfg_p->qid);
        goto error;
    }

    /* prepare egress_tm for new queue configuration. update num_sp_queues, update egress_tm weight, create egress_tm, etc.. */
    ret = prepare_set_q(&root_param, qcfg_p, &idx, &egress_tm, devType, is_static_ctc, tm.dpiQueueExt);
    if (ret)
    {
        tmctl_error("prepare_set_q failed, dev[%s], qid[%d]", ifp_to_name(devType, if_p), qcfg_p->qid);
        goto error;
    }
    if (IS_SINGLE_LEVEL(root_param.level) && !is_lan(devType, if_p))
    {
        ret = rdpa_egress_tm_rl_rate_mode_get(root_param.root_tm, &rl_rate_mode);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_rl_rate_mode_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), qcfg_p->qid, ret);
            goto error;
        }
    }
    qcfg_p->qsize = tmctl_getIfacePriorityQueueSize(devType, if_p, qcfg_p->qid, qcfg_p->qsize);
    if (TMCTL_DEFAULT_QUEUE_SIZE == qcfg_p->qsize)
    {
        tmctl_error("Failed to read dev[%s], qid[%d] queue size",
                ifp_to_name(devType, if_p), qcfg_p->qid);
        goto error;
    }
    prepare_queue_struct(qcfg_p, &queue_cfg, rl_rate_mode, egress_tm, idx);
#ifdef TM_C_CODE
    /* best effort queues in TMC are implemented as subsidery egress tm, that why no need for best_effort bit set in the queue cfg */
    queue_cfg.best_effort = 0;
#endif

    if (qcfg_p->l4s)
    {
        /* L4S is reflected to the user as a queue within an egress tm.
         * Under the hood, this "l4s queue" translates into a secondary level egress_tm that contains
         * a couple of queues, CQ and LQ, the "l4s queue*s*".
         */
        ret = l4s_secondary_egress_tm_set(root_param.root_tm, egress_tm, qcfg_p, idx);
        if (ret)
            goto error;
    }
    else
    {
        ret = rdpa_egress_tm_queue_cfg_set(egress_tm, idx, &queue_cfg);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_set failed, dev[%s], qid[%d], index[%d], ret[%d]", ifp_to_name(devType, if_p), qcfg_p->qid, idx, ret);
            goto error;
        }
    }

#if defined(SUPPORT_DPI)
    if (qcfg_p->bestEffort && tm.dpiQueueExt)
    {
#ifdef TM_C_CODE
        uint8_t max_queues;
        rdpa_egress_tm_num_queues_get(egress_tm, &max_queues); 
        /*  Subsidiary egress TM replaces one of queues of primary egress TM. Take the last one */
        idx =  max_queues - 1;
#endif
        ret = add_best_effort_dpi_queues(egress_tm, qcfg_p, idx);
        if (ret)
            goto error;
    }
#endif

    return TMCTL_SUCCESS;

error:
    if (egress_tm && !IS_SINGLE_LEVEL(root_param.level) && !is_static_ctc)
         bdmf_destroy(egress_tm);
    /* rollback previous config if available, prevent infinit loop */
    if (restore_queue)
    {
        if (!depth)
        {
            depth = 1;
            ret = tmctl_RdpaTmQueueSet(devType, if_p, &saved_qcfg_p);
            if (ret)
                tmctl_error("Failed to set old cfg back, dev[%s], qid[%d]",
                        ifp_to_name(devType, if_p), qcfg_p->qid);
        }
        depth = 0;
    }
    return TMCTL_ERROR;
}

tmctl_ret_e tmctl_RdpaTmQueueDel(tmctl_devType_e devType,
                                 tmctl_if_t *if_p,
                                 int queue_id)
{
    bdmf_object_handle root_tm = BDMF_NULL;
    bdmf_object_handle egress_tm = BDMF_NULL;
    BOOL flush_required = FALSE;
    int idx;
    int ret;

    flush_required = is_flush_required(devType, if_p);

    if (is_static_ctc_mode(devType, is_lan(devType, if_p)))
    {
        tmctl_error("Deleting queues is not allowed in static CTC mode");
        return TMCTL_ERROR;
    }

    get_root_tm(devType, if_p, &root_tm);
    if (!root_tm)
    {
        tmctl_error("Failed to get egress_tm, qid[%d]", queue_id);
        return TMCTL_ERROR;
    }

    ret = get_queue_tm(root_tm, queue_id, &egress_tm, &idx);
    if (ret)
    {
        tmctl_error("get_queue_tm failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType, if_p), queue_id, ret);
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(root_tm);
        }        
        return ret;
    }

    if (flush_required)
    {
        ret = enable_device_traffic(devType, if_p, FALSE);
        if (ret)
        {
            tmctl_error("enable_device_traffic disable dev[%s] failed", ifp_to_name(devType,if_p));
            if (devType == TMCTL_DEV_SVCQ)
            {
                bdmf_put(root_tm);
            }            
            return TMCTL_ERROR;
        }

        ret = mac_flush_queue(devType, if_p);
        if (ret)
        {
            tmctl_error("mac_flush_queue dev[%s] failed", ifp_to_name(devType,if_p));
            goto  DEL_EXIT;
        }
    }

    if (root_tm == egress_tm) /* single level */
    {
        if (is_queue_index_resereved(devType, idx))
        {
            tmctl_error("Can't delete reserve queue, qid[%d]", queue_id);
            ret = TMCTL_ERROR;
            goto  DEL_EXIT;
        }

        {
            bdmf_object_handle subsidiary_egress_tm; 
            /* If queue is not a link to subsidiary, delete queue */
            if (rdpa_egress_tm_subsidiary_get(egress_tm, idx, &subsidiary_egress_tm) != 0)   /* Not an egress_tm*/
            {
                ret = rdpa_egress_tm_queue_cfg_delete(egress_tm, idx);
                if (ret)
                {
                    tmctl_error("rdpa_egress_tm_queue_cfg_delete failed, qid[%d], index[%d], ret[%d]", queue_id, idx, ret);
                    ret = TMCTL_ERROR;
                    goto  DEL_EXIT;
                }
            }
            else   /* Otherwise delete subsidiary egress_tm */  
            {
                bdmf_destroy(subsidiary_egress_tm); 
            }
        }
    }
    else /* multi level */
    {
        ret = bdmf_destroy(egress_tm);
        if (ret)
        {
            tmctl_error("bdmf_destroy failed, qid[%d], ret[%d]", queue_id, ret);
            ret = TMCTL_ERROR;
            goto  DEL_EXIT;
        }
    }

DEL_EXIT:
    if (flush_required)
        ret |= enable_device_traffic(devType, if_p, TRUE);
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return ret;
}

tmctl_ret_e __tmctl_RdpaGetPortShaper(tmctl_devType_e devType,
                                      tmctl_if_t *if_p,
                                      tmctl_shaper_t *shaper_p)
{
    bdmf_object_handle rate_limit_obj = BDMF_NULL;
    bdmf_object_handle egress_tm = BDMF_NULL;
    rdpa_rl_cfg_t rl;
    tmctl_portTmParms_t tm = {0};
    int ret = 0;

    get_port_tm_caps(devType, if_p, &tm);
    /*if (!tm.portShaper)
    {
        tmctl_error("Port shaper is not supported on this port");
        return TMCTL_ERROR;
    }*/

    get_root_tm(devType, if_p, &egress_tm);
    if (!egress_tm)
    {
        tmctl_error("Failed to get egress_tm dev[%s]", ifp_to_name(devType, if_p));
        return TMCTL_ERROR;
    }
    ret = rdpa_egress_tm_rate_limit_get(egress_tm, &rate_limit_obj);
    if ((ret) || (rate_limit_obj == BDMF_NULL))
    {
        rl.af_rate = 0;
        ret = 0;
    }
    else
    {
        ret = rdpa_rate_limit_cfg_get(rate_limit_obj, &rl);
    }

    if (ret)
    {
        tmctl_error("rdpa_rate_limit_cfg_get failed, dev[%s], ret[%d]", ifp_to_name(devType, if_p), ret);
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(egress_tm);
        }
        return TMCTL_ERROR;
    }

    shaper_p->shapingRate = BPS_TO_KBPS(rl.af_rate); /* Best Effort: shaping_rate is in kbit/s: 1 kilobit = 1000 bits */

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }

    return TMCTL_SUCCESS;
}

tmctl_ret_e __tmctl_RdpaSetPortShaper(tmctl_devType_e devType,
                                      tmctl_if_t *if_p,
                                      tmctl_shaper_t *shaper_p)
{
    bdmf_object_handle egress_tm = BDMF_NULL;
    rdpa_rl_cfg_t rl = {0};
    tmctl_portTmParms_t tm = {0};
    int ret;
    get_port_tm_caps(devType, if_p, &tm);
    /*if (!tm.portShaper)
    {
        tmctl_error("Port shaper is not supported on this port");
        return TMCTL_ERROR;
    }*/

    rl.af_rate = KBPS_TO_BPS(shaper_p->shapingRate);  /* Best Effort: shaping_rate is in kbit/s: 1 kilobit = 1000 bits */


    get_root_tm(devType, if_p, &egress_tm);
    if (!egress_tm)
    {
        tmctl_error("Failed to get egress_tm, dev[%s]", ifp_to_name(devType, if_p));
        return TMCTL_ERROR;
    }

    ret = tmctl_egress_tm_rl_set(egress_tm, &rl);

    if (ret)
    {
        tmctl_error("tmctl_egress_tm_rl_set failed, dev[%s], ret[%d]", ifp_to_name(devType, if_p), ret);
        if (devType == TMCTL_DEV_SVCQ)
        {
            bdmf_put(egress_tm);
        }    
        return TMCTL_ERROR;
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }
    return TMCTL_SUCCESS;
}
#if !defined(BCM_DSL_RDP)
tmctl_ret_e tmctl_RdpaGetPortShaper(tmctl_devType_e devType,
                                    tmctl_if_t *if_p,
                                    tmctl_shaper_t *shaper_p)
{
    return __tmctl_RdpaGetPortShaper(devType, if_p, shaper_p);
}

tmctl_ret_e tmctl_RdpaSetPortShaper(tmctl_devType_e devType,
                                    tmctl_if_t *if_p,
                                    tmctl_shaper_t *shaper_p)
{
    return __tmctl_RdpaSetPortShaper(devType, if_p, shaper_p);
}
#endif // !BCM_DSL_RDP

tmctl_ret_e tmctl_RdpaGetPortRxRate(tmctl_devType_e devType,
                          tmctl_if_t *if_p,
                          tmctl_shaper_t *shaper_p)
{
    bdmf_object_handle port_obj = BDMF_NULL;
    rdpa_port_flow_ctrl_t fc_cfg;
    int ret = TMCTL_ERROR;

    ret = rdpa_port_get(if_p->ethIf.ifname, &port_obj);
    if (ret)
    {
        tmctl_error("failed to get port for interface[%s], ret[%d]",if_p->ethIf.ifname, ret);
        return TMCTL_ERROR;
    }
    ret = rdpa_port_flow_control_get(port_obj, &fc_cfg);
    if (ret)
    {
        tmctl_error("failed to get rx flow control cfg for interface[%s], ret[%d]", if_p->ethIf.ifname, ret);
        return TMCTL_ERROR;
    }

    shaper_p->shapingRate = BPS_TO_KBPS(fc_cfg.rate);   // bps -> kbps 
    shaper_p->shapingBurstSize = (fc_cfg.mbs*8)/1000;   // bytes -> kbits
    
    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaSetPortRxRate(tmctl_devType_e devType,
                          tmctl_if_t *if_p,
                          tmctl_shaper_t *shaper_p)
{
    bdmf_object_handle port_obj = BDMF_NULL;
    rdpa_port_flow_ctrl_t fc_cfg;
    int ret = TMCTL_ERROR;

    ret = rdpa_port_get(if_p->ethIf.ifname, &port_obj);
    if (ret)
    {
        tmctl_error("failed to get port for interface[%s], ret[%d]",if_p->ethIf.ifname, ret);
        return TMCTL_ERROR;
    }
    ret = rdpa_port_flow_control_get(port_obj, &fc_cfg);
    if (ret)
    {
        tmctl_error("failed to get rx flow control cfg for interface[%s], ret[%d]", if_p->ethIf.ifname, ret);
        return TMCTL_ERROR;
    }
    
    fc_cfg.rate = (bdmf_rate_t)shaper_p->shapingRate*1000;   //kbps -> bps
    fc_cfg.mbs = (uint32_t)(shaper_p->shapingBurstSize>>3)*1000;   // kbits->bits-> bytes
    if (fc_cfg.mbs < 20000)
        fc_cfg.mbs = 20000;                         /* mbs/2 > jumbo packet*/
    fc_cfg.threshold = fc_cfg.mbs/2;                /* kbits->bits->bytes, a half of mbs */

    ret = rdpa_port_flow_control_set(port_obj, &fc_cfg);
    if (ret)
    {
        tmctl_error("failed to set rx flow control cfg for interface[%s], ret[%d]", if_p->ethIf.ifname, ret);
        return TMCTL_ERROR;
    }
    
    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaGetQueueDropAlg(tmctl_devType_e devType,
                          tmctl_if_t *if_p,
                          int queue_id,
                          tmctl_queueDropAlg_t *dropAlg_p)
{
    bdmf_object_handle queue_tm = BDMF_NULL;
    bdmf_object_handle root_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    int idx = 0;
    int ret;

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
        bdmf_object_handle xtm_channel = BDMF_NULL;

        ret = rdpa_xtmchannel_get(queue_id, &xtm_channel);
        if ((ret) || (xtm_channel == BDMF_NULL)) 
        {
            tmctl_error("rdpa_xtmchannel_get() failed: channel(%d) rc(%d)", queue_id, ret);
            return TMCTL_ERROR;
        }   

        ret = rdpa_xtmchannel_egress_tm_get(xtm_channel, &queue_tm);
        bdmf_put(xtm_channel);
        if (ret || (queue_tm == BDMF_NULL)) 
        {
            tmctl_error("rdpa_xtmchannel_egress_tm_get() failed: channel(%d) rc(%d)", queue_id, ret);
            return TMCTL_ERROR;
        }
    } 
    else
#endif
    {
        get_root_tm(devType, if_p, &root_tm);
        if (!root_tm)
        {
            tmctl_error("Failed to get egress_tm, dev[%s], qid[%d]", ifp_to_name(devType,if_p), queue_id);
            return TMCTL_ERROR;
        }

        ret = get_queue_tm(root_tm, queue_id, &queue_tm, &idx);
        if (ret)
        {
            tmctl_error("get_queue_tm failed, qid[%d]", queue_id);
            goto error;
        }
    }

    /* if singel level check if not reserved queue */
    if ((root_tm == queue_tm) && is_queue_index_resereved(devType, idx))
    {
        tmctl_error("queue is resereved");
        goto error;
    }

    ret = rdpa_egress_tm_queue_cfg_get(queue_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, dev[%s], qid[%d]", ifp_to_name(devType,if_p), queue_id);
        goto error;
    }


    if (queue_cfg.drop_alg == rdpa_tm_drop_alg_red)
    {
        dropAlg_p->dropAlgorithm = TMCTL_DROP_RED;

        if (queue_cfg.low_class.min_threshold)
            dropAlg_p->dropAlgLo.redMinThreshold = getUsrQueueSize(queue_cfg.low_class.min_threshold);
        if (queue_cfg.low_class.max_threshold)
            dropAlg_p->dropAlgLo.redMaxThreshold = getUsrQueueSize(queue_cfg.low_class.max_threshold);
        dropAlg_p->dropAlgLo.redPercentage = RDPA_WRED_MAX_DROP_PROBABILITY;
    }
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_wred)
    {
        dropAlg_p->dropAlgorithm = TMCTL_DROP_WRED;

        if (queue_cfg.low_class.min_threshold)
            dropAlg_p->dropAlgLo.redMinThreshold = getUsrQueueSize(queue_cfg.low_class.min_threshold);
        if (queue_cfg.low_class.max_threshold)
            dropAlg_p->dropAlgLo.redMaxThreshold = getUsrQueueSize(queue_cfg.low_class.max_threshold);
        if (queue_cfg.high_class.min_threshold)
            dropAlg_p->dropAlgHi.redMinThreshold = getUsrQueueSize(queue_cfg.high_class.min_threshold);
        if (queue_cfg.high_class.max_threshold)
            dropAlg_p->dropAlgHi.redMaxThreshold = getUsrQueueSize(queue_cfg.high_class.max_threshold);

        dropAlg_p->dropAlgLo.redPercentage = RDPA_WRED_MAX_DROP_PROBABILITY;
        dropAlg_p->dropAlgHi.redPercentage = RDPA_WRED_MAX_DROP_PROBABILITY;
        dropAlg_p->priorityMask0 = queue_cfg.priority_mask_0;
        dropAlg_p->priorityMask1 = queue_cfg.priority_mask_1;
    }
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_codel)
        dropAlg_p->dropAlgorithm = TMCTL_DROP_CODEL;
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_pi2)
        dropAlg_p->dropAlgorithm = TMCTL_DROP_PI2;
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_pi2)
        dropAlg_p->dropAlgorithm = TMCTL_DROP_L4S_CQ;
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_laqm)
        dropAlg_p->dropAlgorithm = TMCTL_DROP_L4S_LQ;
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_flow_ctrl)
    {
        dropAlg_p->dropAlgorithm = TMCTL_DROP_FLOW_CTRL;
        dropAlg_p->dropAlgHi.redMinThreshold = getUsrQueueSize(queue_cfg.high_class.min_threshold);
        dropAlg_p->dropAlgLo.redMinThreshold = getUsrQueueSize(queue_cfg.low_class.min_threshold);
    }
    else
        dropAlg_p->dropAlgorithm = TMCTL_DROP_DT;

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_ERROR;
}

tmctl_ret_e tmctl_RdpaSetQueueDropAlg(tmctl_devType_e devType,
                          tmctl_if_t *if_p,
                          int queue_id,
                          tmctl_queueDropAlg_t *dropAlg_p)
{
    bdmf_object_handle queue_tm = BDMF_NULL;
    bdmf_object_handle root_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    int idx = 0;
    int ret;

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
        bdmf_object_handle xtm_channel = BDMF_NULL;

        ret = rdpa_xtmchannel_get(queue_id, &xtm_channel);
        if ((ret) || (xtm_channel == BDMF_NULL)) 
        {
            tmctl_error("rdpa_xtmchannel_get() failed: channel(%d) rc(%d)", queue_id, ret);
            return TMCTL_ERROR;
        }   

        ret = rdpa_xtmchannel_egress_tm_get(xtm_channel, &queue_tm);
        bdmf_put(xtm_channel);
        if (ret || (queue_tm == BDMF_NULL)) 
        {
            tmctl_error("rdpa_xtmchannel_egress_tm_get() failed: channel(%d) rc(%d)", queue_id, ret);
            return TMCTL_ERROR;
        }
    } 
    else
#endif
    {
        get_root_tm(devType, if_p, &root_tm);
        if (!root_tm)
        {
            tmctl_error("Failed to get egress_tm, dev[%s], qid[%d]", ifp_to_name(devType,if_p), queue_id);
            return TMCTL_ERROR;
        }

        ret = get_queue_tm(root_tm, queue_id, &queue_tm, &idx);
        if (ret)
        {
            tmctl_error("get_queue_tm failed, qid[%d]", queue_id);
            goto error;
        }
    }

    /* if singel level check if not reserved queue */
    if ((root_tm == queue_tm) && is_queue_index_resereved(devType, idx))
    {
        tmctl_error("queue is resereved");
        goto error;
    }

    ret = rdpa_egress_tm_queue_cfg_get(queue_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType,if_p), queue_id, ret);
        goto error;
    }

    if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_pi2 || queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_laqm)
    {
        tmctl_error("dualq drop algorithms cannot be modified");
        goto error;
    }

    if (dropAlg_p->dropAlgorithm == TMCTL_DROP_L4S_LQ || dropAlg_p->dropAlgorithm == TMCTL_DROP_L4S_CQ)
    {
        tmctl_error("dualq drop algorithms are only set on l4s queue creation");
        goto error;
    }

    queue_cfg.drop_alg = convertDropAlg(dropAlg_p->dropAlgorithm);

    if (queue_cfg.drop_alg == rdpa_tm_drop_alg_red)
    {
        queue_cfg.low_class.min_threshold  = getDeviceQueueSize(dropAlg_p->dropAlgLo.redMinThreshold);
        queue_cfg.low_class.max_threshold  = getDeviceQueueSize(dropAlg_p->dropAlgLo.redMaxThreshold);
        queue_cfg.high_class.min_threshold = queue_cfg.drop_threshold;
        queue_cfg.high_class.max_threshold = queue_cfg.drop_threshold;
    }
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_wred)
    {
        queue_cfg.low_class.min_threshold  = getDeviceQueueSize(dropAlg_p->dropAlgLo.redMinThreshold);
        queue_cfg.low_class.max_threshold  = getDeviceQueueSize(dropAlg_p->dropAlgLo.redMaxThreshold);
        queue_cfg.high_class.min_threshold = getDeviceQueueSize(dropAlg_p->dropAlgHi.redMinThreshold);
        queue_cfg.high_class.max_threshold = getDeviceQueueSize(dropAlg_p->dropAlgHi.redMaxThreshold);
        queue_cfg.priority_mask_0 = dropAlg_p->priorityMask0;
        queue_cfg.priority_mask_1 = dropAlg_p->priorityMask1;
    }
    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_flow_ctrl)
    {
        queue_cfg.low_class.min_threshold = getDeviceQueueSize(dropAlg_p->dropAlgLo.redMinThreshold);
        queue_cfg.high_class.min_threshold = getDeviceQueueSize(dropAlg_p->dropAlgHi.redMinThreshold);
        queue_cfg.low_class.max_threshold = queue_cfg.drop_threshold;
        queue_cfg.high_class.max_threshold = queue_cfg.drop_threshold;
    }
    else /* DT */
    {
        queue_cfg.low_class.min_threshold  = 0;
        queue_cfg.low_class.max_threshold  = 0;
        queue_cfg.high_class.min_threshold = 0;
        queue_cfg.high_class.max_threshold = 0;
    }

    ret = rdpa_egress_tm_queue_cfg_set(queue_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_set failed, dev[%s], qid[%d], ret[%d]", ifp_to_name(devType,if_p), queue_id, ret);
        goto error;
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_ERROR;
}

static tmctl_ret_e append_l4s_lq_stats(bdmf_object_handle root_tm, int queue_id, tmctl_queueStats_t *stats_p)
{
    bdmf_object_handle queue_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_stat_1way_t queue_stat = {0};
    int lq_idx;
    int ret;

    ret = get_queue_tm(root_tm, queue_id, &queue_tm, &lq_idx);
    if (ret)
    {
        tmctl_error("get_queue_tm failed, qid[%d]", queue_id);
        return TMCTL_ERROR;
    }

    if (rdpa_egress_tm_queue_cfg_get(queue_tm, lq_idx, &queue_cfg))
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed for LQ, qid[%d], ret[%d]", lq_idx, ret);
        return TMCTL_ERROR;
    }

    if (queue_cfg.drop_alg != rdpa_tm_drop_alg_dualq_laqm)
    {
        tmctl_error("L4S CQ (queue_id %d) dosn't match to a L4S LQ (queue_id %d), ret[%d]", lq_idx, lq_idx, ret);
        return TMCTL_ERROR;
    }

    if (rdpa_egress_tm_queue_stat_get(queue_tm, lq_idx, &queue_stat))
    {
        tmctl_error("rdpa_egress_tm_queue_stat_get failed for LQ, qid[%d], ret[%d]", lq_idx, ret);
        return TMCTL_ERROR;
    }

    stats_p->txPackets += queue_stat.passed.packets;
    stats_p->txBytes += queue_stat.passed.bytes;
    stats_p->droppedPackets += queue_stat.discarded.packets;
    stats_p->droppedBytes += queue_stat.discarded.bytes;

    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaGetQueueStats(tmctl_devType_e devType,
                          tmctl_if_t *if_p,
                          int queue_id,
                          tmctl_queueStats_t *stats_p)
{
    bdmf_object_handle root_tm = BDMF_NULL;
    bdmf_object_handle queue_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_stat_1way_t queue_stat = {0};
    int ret;
    int idx = 0;


#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
        bdmf_object_handle xtm_channel = BDMF_NULL;

        ret = rdpa_xtmchannel_get(queue_id, &xtm_channel);
        if ((ret) || (xtm_channel == BDMF_NULL)) 
        {
            tmctl_error("rdpa_xtmchannel_get() failed: channel(%d) rc(%d)", queue_id, ret);
            return TMCTL_ERROR;
        }   

        ret = rdpa_xtmchannel_egress_tm_get(xtm_channel, &queue_tm);
        bdmf_put(xtm_channel);
        if (ret || (queue_tm == BDMF_NULL)) 
        {
            tmctl_error("rdpa_xtmchannel_egress_tm_get() failed: channel(%d) rc(%d)", queue_id, ret);
            return TMCTL_ERROR;
        }
    } 
    else
#endif
    {
        get_root_tm(devType, if_p, &root_tm);
        if (!root_tm)
        {
            tmctl_error("Failed to get egress_tm, dev[%s], qid[%d]", ifp_to_name(devType, if_p), queue_id);
            return TMCTL_ERROR;
        }

        ret = get_queue_tm(root_tm, queue_id, &queue_tm, &idx);
        if (ret)
        {
            tmctl_error("get_queue_tm failed, qid[%d]", queue_id);
            goto error;
        }
    }

#ifdef BCM_DSL_RDP
    /* DSL_RDP only supports reading stat from root_tm, if root_tm != queue_tm,
     * it is likely a multi-level egress_tm setting for root_tm.  Then for
     * no SVCQ case, we need to change the tm and qid to read the stat from */
    if ((root_tm != queue_tm) && (devType != TMCTL_DEV_SVCQ))
    {
        /* DSL platform will always do Q7P7 */
        int adjusted_idx = MAX_Q_PER_WAN_TM - queue_id - 1;
        ret = rdpa_egress_tm_queue_stat_get(root_tm, adjusted_idx, &queue_stat);
    }
    else
#endif
    ret = rdpa_egress_tm_queue_stat_get(queue_tm, idx, &queue_stat);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_stat_get failed, qid[%d], ret[%d]", queue_id, ret);
        goto error;
    }

    stats_p->txPackets = queue_stat.passed.packets;
    stats_p->txBytes = queue_stat.passed.bytes;
    stats_p->droppedPackets = queue_stat.discarded.packets;
    stats_p->droppedBytes = queue_stat.discarded.bytes;

    /* Append L4S LQ stats if applicable.
     * The CQ (dualq_pi2) and LQ (dualq_laqm) are two queues that are reflected to the user as one queue.
     */
    if (rdpa_egress_tm_queue_cfg_get(queue_tm, idx, &queue_cfg))
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, qid[%d], ret[%d]", queue_id, ret);
        goto error;
    }
    if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_pi2)
    {
        ret = append_l4s_lq_stats(root_tm, L4S_CQ_TO_LQ_QUEUE_ID(queue_id), stats_p);
        if (ret)
            return ret;
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_ERROR;

}

tmctl_ret_e tmctl_RdpaSetQueueSize(tmctl_devType_e devType,
                                   tmctl_if_t *if_p,
                                   int queue_id,
                                   int size)
{
    bdmf_object_handle root_tm = BDMF_NULL;
    bdmf_object_handle queue_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    int idx;
    int ret;

    get_root_tm(devType, if_p, &root_tm);
    if (!root_tm)
    {
        tmctl_error("Failed to get egress_tm, dev[%s], qid[%d]", ifp_to_name(devType,if_p), queue_id);
        return TMCTL_ERROR;
    }

    ret = get_queue_tm(root_tm, queue_id, &queue_tm, &idx);
    if (ret)
    {
        tmctl_error("get_queue_tm failed, qid[%d]", queue_id);
        goto error;
    }

    /* if singel level check if not reserved queue */
    if ((root_tm == queue_tm) && is_queue_index_resereved(devType, idx))
    {
        tmctl_error("queue is resereved");
        goto error;
    }

    ret = rdpa_egress_tm_queue_cfg_get(queue_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, qid[%d], ret[%d]", queue_id, ret);
        goto error;
    }

    queue_cfg.drop_threshold = getDeviceQueueSize(size);

    ret = rdpa_egress_tm_queue_cfg_set(queue_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_set failed, qid[%d], ret[%d]", queue_id, ret);
        goto error;
    }

    /* In case of dualq_pi2 apply configuration to the dualq_laqm.
     * No need to do it in the reverse way because the second queue isn't exposed to the user.
     */
    if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_pi2)
    {
        int dualq_laqm_queue_id = L4S_CQ_TO_LQ_QUEUE_ID(queue_id);

        ret = tmctl_RdpaSetQueueSize(devType, if_p, dualq_laqm_queue_id, size);
        if (ret)
        {
            tmctl_error("rdpa_egress_tm_queue_cfg_set failed for laqm , pi2 qid[%d], ret[%d]", dualq_laqm_queue_id, ret);
            goto error;
        }
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_ERROR;
}

static int set_queue_rl_single_level(bdmf_object_handle egress_tm,
                                     bdmf_index idx,
                                     tmctl_shaper_t *shaper_p)
{
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    rdpa_tm_rl_rate_mode rl_mode;
    int ret;

    ret = rdpa_egress_tm_queue_cfg_get(egress_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    ret = rdpa_egress_tm_rl_rate_mode_get(egress_tm, &rl_mode);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_rl_rate_mode_get failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    queue_shaper_cfg_set(rl_mode, shaper_p, &queue_cfg);
    ret = rdpa_egress_tm_queue_cfg_set(egress_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_set failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}


tmctl_ret_e tmctl_RdpaSetQueueShaper(tmctl_devType_e devType,
                                   tmctl_if_t *if_p,
                                   int queue_id,
                                   tmctl_shaper_t *shaper_p)
{
    bdmf_object_handle egress_tm = BDMF_NULL;
    bdmf_object_handle root_tm = BDMF_NULL;
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    tmctl_portTmParms_t tm ={0};
    int idx;
    int ret;

    get_port_tm_caps(devType, if_p, &tm);
    if (!tm.queueShaper)
    {
        tmctl_error("Queue shaper is not supported on this port");
        return TMCTL_ERROR;
    }

    get_root_tm(devType, if_p, &root_tm);
    if (!root_tm)
    {
        tmctl_error("Failed to get egress_tm, dev[%s], qid[%d]", ifp_to_name(devType,if_p), queue_id);
        return TMCTL_ERROR;
    }

    ret = get_queue_tm(root_tm, queue_id, &egress_tm, &idx);
    if (ret)
    {
        tmctl_error("get_queue_tm failed, qid[%d]", queue_id);
        goto error;
    }

    /* if singel level check if not reserved queue */
    if ((root_tm == egress_tm) && is_queue_index_resereved(devType, idx))
    {
        tmctl_error("queue is resereved");
        goto error;
    }

    /* L4S allows shaping the egress_tm "l4s_queue" but not the specific LQ/CQ queue.
     * From user perspective the queue_id of the CQ reflects the entire "l4s_queue".
     */
    ret = rdpa_egress_tm_queue_cfg_get(egress_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, idx[%d], ret[%d]", idx, ret);
        goto error;
    }

    else if (queue_cfg.drop_alg == rdpa_tm_drop_alg_dualq_laqm)
    {
        tmctl_error("Shaping L4S LQ is not allowed");
        goto error;
    }

    if ((root_tm == egress_tm))
            ret = set_queue_rl_single_level(egress_tm, idx, shaper_p);
    else /* l4s and other cases too */
        ret = set_queue_rl_multi_level(root_tm, egress_tm, shaper_p);
    if (ret)
    {
        tmctl_error("Failed to set queue shpaer, qid[%d]", queue_id);
        goto error;
    }

    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(root_tm);
    }
    return TMCTL_ERROR;    
}

tmctl_ret_e tmctl_RdpaCreateShaper(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_shaper_t *shaper_p,
                                  uint64_t *shaper_obj)
{

    bdmf_object_handle root_tm = BDMF_NULL;
    tmctl_portTmParms_t tm ={0};
    bdmf_object_handle rate_limit_obj = {0};
    rdpa_rl_cfg_t rl_cfg = {0};
    int ret;
    /* rate limit object */
    bdmf_mattr_handle mattr = BDMF_NULL;

    get_port_tm_caps(dev_type, if_p, &tm);
    if (!tm.queueShaper)
    {
        tmctl_error("Queue shaper is not supported on this port");
        return TMCTL_ERROR;
    }

    get_root_tm(dev_type, if_p, &root_tm);
    if (!root_tm)
    {
        tmctl_error("Failed to get egress_tm, dev[%s]", ifp_to_name(dev_type,if_p));
        return TMCTL_ERROR;
    }

    /* verify it not empty rate limit*/
    if (shaper_p->shapingRate <= 0)
    {
        return TMCTL_ERROR;
    }

    /* verify it not empty rate limit*/
    if (shaper_p->minRate > 0)
    {
        tmctl_error("dual rate limit is not supported for this API, minRate is used as 0");
        return TMCTL_ERROR;
    }

    mattr = bdmf_mattr_alloc(rdpa_rate_limit_drv());
    if (!mattr)
    {
        tmctl_error("bdmf_mattr_alloc failed");
        return TMCTL_ERROR;
    }

    ret = bdmf_new_and_set(rdpa_rate_limit_drv(), NULL, mattr, &rate_limit_obj);
    bdmf_mattr_free(mattr);
    if (ret)
    {
        tmctl_error("bdmf_new_and_set failed to create rate limit obj, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    *shaper_obj = rate_limit_obj;
    /* Shaper ia Single mode only, variable af_rate used as best effort rate*/
    rl_cfg.af_rate = KBPS_TO_BPS(shaper_p->shapingRate);
    rdpa_rate_limit_cfg_set(rate_limit_obj, &rl_cfg);
    printf("Created Shaper 0x%llx\n", *shaper_obj);
    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaDeleteShaper(uint64_t shaper_obj)
{
    int ret;
    ret = bdmf_destroy((bdmf_object_handle)shaper_obj);
    if (ret)
    {
        tmctl_error("bdmf_destroy failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }
    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaSetShaperQueue(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     int queueId,
                                     uint64_t shaper_obj)
{
    bdmf_object_handle egress_tm = BDMF_NULL;
    bdmf_object_handle root_tm = BDMF_NULL;
    tmctl_portTmParms_t tm ={0};
    rdpa_tm_queue_cfg_t queue_cfg = {0};
    int ret;
    int idx;

    get_port_tm_caps(devType, if_p, &tm);
    if (!tm.queueShaper)
    {
        tmctl_error("Queue shaper is not supported on this port");
        return TMCTL_ERROR;
    }

    get_root_tm(devType, if_p, &root_tm);
    if (!root_tm)
    {
        tmctl_error("Failed to get egress_tm, dev[%s]", ifp_to_name(devType,if_p));
        return TMCTL_ERROR;
    }

    ret = get_queue_tm(root_tm, queueId, &egress_tm, &idx);
    if (ret)
    {
        tmctl_error("get_queue_tm failed, qid[%d]", queueId);
        return TMCTL_ERROR;
    }

    /* if singel level check if not reserved queue */
    if ((root_tm == egress_tm) && is_queue_index_resereved(devType, idx))
    {
        tmctl_error("queue is resereved");
        return TMCTL_ERROR;
    }

    ret = rdpa_egress_tm_queue_cfg_get(egress_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_get failed, idx[%d], ret[%d]", idx, ret);
        return TMCTL_ERROR;
    }
    queue_cfg.rl_cfg = (bdmf_object_handle)shaper_obj;
    ret = rdpa_egress_tm_queue_cfg_set(egress_tm, idx, &queue_cfg);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_queue_cfg_set failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}
tmctl_ret_e tmctl_RdpaGetPortTmParms(tmctl_devType_e devType,
                                    tmctl_if_t *if_p,
                                    tmctl_portTmParms_t *tm_parms)
{
    bdmf_object_handle egress_tm = BDMF_NULL;
    rdpa_tm_sched_mode mode;
    uint8_t num_queues;
    int ret;

    get_port_tm_caps(devType, if_p, tm_parms);

    ret = get_root_tm(devType, if_p, &egress_tm);
    if (ret)
    {
        tmctl_error("get_root_tm failed, dev[%s], devType[%d]", ifp_to_name(devType,if_p), devType);
        return TMCTL_ERROR;
    }
    if (!egress_tm)
    {
        tmctl_debug("No egress_tm obj, dev[%s]", ifp_to_name(devType,if_p));
        tm_parms->cfgFlags = 0;
        goto success;
    }

    ret = rdpa_egress_tm_num_queues_get(egress_tm, &num_queues);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_num_queues_get failed, ret[%d]", ret);
        goto error;
    }
    tm_parms->numQueues = num_queues;

    ret = get_tm_mode(egress_tm, &mode);
    // coverity msg : ret looks like a copy-paste error. false positive.
    /* coverity[copy_paste_error] */
    if (ret)
    {
        tmctl_error("get_tm_mode failed, dev[%s]", ifp_to_name(devType,if_p));
        goto error;
    }

    switch (mode)
    {
        case rdpa_tm_sched_sp_wrr:
            tm_parms->cfgFlags = TMCTL_SCHED_TYPE_SP_WRR;
            break;
        case rdpa_tm_sched_sp:
            tm_parms->cfgFlags = TMCTL_SCHED_TYPE_SP;
            break;
        case rdpa_tm_sched_wrr:
            tm_parms->cfgFlags = TMCTL_SCHED_TYPE_WRR;
            break;
        default:
            tmctl_error("Mode %d is not supported", mode);
            goto error;
    }

success:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }
    return TMCTL_SUCCESS;

error:
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }
    return TMCTL_ERROR;    
}


#if defined(POLICER_SUPPORT)
tmctl_ret_e tmctl_RdpaCreatePolicer(tmctl_policer_t *policer_p)
{
    bdmf_mattr_handle mattr = BDMF_NULL;
    bdmf_number index= 0;
    bdmf_object_handle policer = BDMF_NULL;
    rdpa_tm_policer_cfg_t policer_cfg = {0};
    int ret;

    tmctl_debug("id[%d], type[%d]", policer_p->policerId, policer_p->type);

    index = policer_p->policerId;
    policer_cfg.type = rdpa_tm_policer_single_token_bucket;

    switch (policer_p->type)
    {
        case TMCTL_POLICER_SINGLE_TOKEN_BUCKET:
            policer_cfg.type = rdpa_tm_policer_single_token_bucket;
            break;
#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
        case TMCTL_POLICER_SINGLE_RATE_TCM:
            policer_cfg.type = rdpa_tm_policer_sr_overflow_dual_token_bucket;
            break;
        case TMCTL_POLICER_TWO_RATE_TCM:
            policer_cfg.type = rdpa_tm_policer_tr_dual_token_bucket;
            break;
        case TMCTL_POLICER_TWO_RATE_WITH_OVERFLOW:
            policer_cfg.type = rdpa_tm_policer_tr_overflow_dual_token_bucket;
            break;
#endif
        default:
            tmctl_error("policer type %d is not supported", policer_p->type);
            return TMCTL_UNSUPPORTED;
    }
    policer_cfg.commited_rate = policer_p->cir;
    policer_cfg.committed_burst_size = policer_p->cbs;
    policer_cfg.peak_rate = policer_p->pir;
    policer_cfg.peak_burst_size = (policer_p->type == TMCTL_POLICER_SINGLE_RATE_TCM)?policer_p->ebs:policer_p->pbs;

    policer_cfg.dei_mode = policer_p->dei_mode;
    policer_cfg.color_aware_enabled = policer_p->color_aware_enabled;
    policer_cfg.rl_overhead = policer_p->rl_overhead;

    mattr = bdmf_mattr_alloc(rdpa_policer_drv());
    if (!mattr)
    {
        tmctl_error("bdmf_mattr_alloc failed");
        goto error;
    }

    if (TMCTL_INVALID_KEY != index)
    {
        ret = rdpa_policer_index_set(mattr, index);
        if (ret)
        {
            tmctl_error("rdpa_policer_index_set failed, ret[%d]", ret);
            goto error;
        }
    }

    ret = rdpa_policer_cfg_set(mattr, &policer_cfg);
    if (ret)
    {
        tmctl_error("rdpa_policer_cfg_set failed, ret[%d]", ret);
        goto error;
    }

    ret = bdmf_new_and_set(rdpa_policer_drv(), (bdmf_object_handle)0, mattr, &policer);
    if (ret)
    {
        tmctl_error("bdmf_new_and_set failed to create policer obj, ret[%d]", ret);
        goto error;
    }

    if (TMCTL_INVALID_KEY == index)
    {
        ret = rdpa_policer_index_get(policer, &index);
        if (ret)
        {
            tmctl_error("rdpa_policer_index_get failed, ret[%d]", ret);
            goto error;
        }
        policer_p->policerId = index;

        tmctl_debug("return policer index as %d", policer_p->policerId);
    }

    bdmf_mattr_free(mattr);
    return TMCTL_SUCCESS;
error:
    if (mattr)
        bdmf_mattr_free(mattr);
    if (policer)
        bdmf_destroy(policer);
    return TMCTL_ERROR;
}

tmctl_ret_e tmctl_RdpaModifyPolicer(tmctl_policer_t *policer_p)
{
    bdmf_number index = 0;
    bdmf_object_handle policer = BDMF_NULL;
    rdpa_tm_policer_cfg_t policer_cfg = {0};
    int ret;

    tmctl_debug("id[%d], type[%d]", policer_p->policerId, policer_p->type);

    index = policer_p->policerId;

    ret = rdpa_policer_get(index, &policer);
    if (ret)
    {
        tmctl_error("rdpa_policer_get failed, ret[%d]", ret);
        goto error;
    }
    ret = rdpa_policer_cfg_get(policer, &policer_cfg);
    if (ret)
    {
        tmctl_error("rdpa_policer_cfg_set failed, ret[%d]", ret);
        goto error;
    }

    policer_cfg.commited_rate = (policer_p->cir == TMCTL_INVALID_KEY_U64)?policer_cfg.commited_rate:policer_p->cir;
    policer_cfg.committed_burst_size = (policer_p->cbs == TMCTL_INVALID_KEY)?policer_cfg.committed_burst_size:policer_p->cbs;
    policer_cfg.peak_rate = (policer_p->pir == TMCTL_INVALID_KEY_U64)?policer_cfg.peak_rate:policer_p->pir;
    policer_cfg.peak_burst_size = (policer_cfg.type == rdpa_tm_policer_sr_overflow_dual_token_bucket)?
                                  (policer_p->ebs == TMCTL_INVALID_KEY?policer_cfg.peak_burst_size:policer_p->ebs):
                                  (policer_p->pbs == TMCTL_INVALID_KEY?policer_cfg.peak_burst_size:policer_p->pbs);
    policer_cfg.dei_mode = (policer_p->dei_mode == (uint8_t)TMCTL_INVALID_KEY)?policer_cfg.dei_mode:policer_p->dei_mode;
    policer_cfg.color_aware_enabled = (policer_p->color_aware_enabled == (uint8_t)TMCTL_INVALID_KEY)?policer_cfg.color_aware_enabled:policer_p->color_aware_enabled;
    policer_cfg.rl_overhead = (policer_p->rl_overhead == (int8_t)TMCTL_INVALID_KEY)?policer_cfg.rl_overhead:policer_p->rl_overhead;

    ret = rdpa_policer_cfg_set(policer, &policer_cfg);
    if (ret)
    {
        tmctl_error("rdpa_policer_cfg_set failed, ret[%d]", ret);
        goto error;
    }
    tmctl_debug("cir[%llu], cbs[%llu]", policer_cfg.commited_rate, policer_cfg.commited_rate);

    bdmf_put(policer);
    return TMCTL_SUCCESS;
error:
    if (policer)
        bdmf_put(policer);
    return TMCTL_ERROR;
}

tmctl_ret_e tmctl_RdpaDeletePolicer(int policerId)
{
    bdmf_number index = 0;
    bdmf_object_handle policer = BDMF_NULL;
    int ret;

    tmctl_debug("policerId[%d]", policerId);

    index = policerId;

    ret = rdpa_policer_get(index, &policer);
    if (ret)
    {
        tmctl_debug("rdpa_policer_get failed, ret[%d]", ret);
        return TMCTL_SUCCESS;
    }

    bdmf_put(policer);

    ret = bdmf_destroy(policer);
    if (ret)
    {
        tmctl_error("bdmf_destroy failed, ret[%d]", ret);
        return TMCTL_ERROR;
    }
    policer = BDMF_NULL;
    return TMCTL_SUCCESS;
}
#endif /* POLICER_SUPPORT */

#if defined(BCM_PON) || defined(BCM_DSL_RDP)
static int get_us_orl_tm(bdmf_object_handle *egress_tm_obj)
{
    bdmf_index   index = TMCTL_ORL_TM_id;
    int ret;

    ret = rdpa_egress_tm_get(index, egress_tm_obj);
    if (ret)
    {
        return TMCTL_NOT_FOUND;
    }

    return TMCTL_SUCCESS;
}


static int setup_us_orl_tm(tmctl_shaper_t *shaper_p, bdmf_object_handle *egress_tm_obj_p)
{
    int ret;
    rdpa_rl_cfg_t rl_cfg = {};
    bdmf_mattr_handle mattr = BDMF_NULL;
    bdmf_object_handle egress_tm_obj = BDMF_NULL;

    mattr = bdmf_mattr_alloc(rdpa_egress_tm_drv());
    if (!mattr)
    {
        tmctl_error("bdmf_mattr_alloc failed");
        goto error;
    }

    ret = rdpa_egress_tm_level_set(mattr, rdpa_tm_level_egress_tm);
        if (ret)
    {
        tmctl_error("rdpa_egress_tm_level_set failed, ret[%d]", ret);
        goto error;
    }

    ret = rdpa_egress_tm_mode_set(mattr, rdpa_tm_sched_disabled);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_mode_set failed, ret[%d]", ret);
        goto error;
    }

    ret = rdpa_egress_tm_overall_rl_set(mattr, TRUE);
    if (ret)
    {
        tmctl_error("rdpa_egress_tm_overall_rl_set failed, ret[%d]", ret);
        goto error;
    }

    ret = bdmf_new_and_set(rdpa_egress_tm_drv(), BDMF_NULL, mattr, &egress_tm_obj);
    if (ret)
    {
        tmctl_error("bdmf_new_and_set failed to create egress_tm obj, ret[%d]", ret);
        goto error;
    }

    rl_cfg.af_rate = KBPS_TO_BPS(shaper_p->shapingRate); /* rate is in kbit/s: 1 kilobit = 1000 bits */
    rl_cfg.be_rate = 0;

    ret = tmctl_egress_tm_rl_set(egress_tm_obj, &rl_cfg);

    if (ret)
    {
        tmctl_error("tmctl_egress_tm_rl_set failed, ret[%d]", ret);
        goto error;
    }

    bdmf_mattr_free(mattr);
    *egress_tm_obj_p = egress_tm_obj;

    return TMCTL_SUCCESS;

error:
    if (mattr)
        bdmf_mattr_free(mattr);
    if (egress_tm_obj)
        bdmf_destroy(egress_tm_obj);
    *egress_tm_obj_p = BDMF_NULL;

    return TMCTL_ERROR;
}
#endif /* defined(BCM_PON) || defined(BCM_DSL_RDP) */

#if defined(BCM_PON)
static BOOL is_linked(bdmf_object_handle tcont_obj, bdmf_object_handle egress_tm_obj)
{
    bdmf_link_handle link = BDMF_NULL;

    while ((link = bdmf_get_next_us_link(tcont_obj, link)))
    {
        if (bdmf_us_link_to_object(link) == egress_tm_obj)
            return 1;
    }

    return 0;
}

tmctl_ret_e tmctl_RdpaSetOverAllShaper(tmctl_shaper_t *shaper_p)
{
    int ret;
    bdmf_object_handle   egress_tm_obj = BDMF_NULL;
    rdpa_rl_cfg_t     rl_cfg        = {};

    if (get_us_orl_tm(&egress_tm_obj) == TMCTL_SUCCESS)
    {
        if (!shaper_p->shapingRate) /*if rate=0 disable us overall shaper */
        {
            bdmf_put(egress_tm_obj);
            ret = bdmf_destroy(egress_tm_obj);
            if (ret)
            {
                tmctl_error("bdmf_destroy() failed ret(%d)\n",ret);
                return TMCTL_ERROR;
            }

            return TMCTL_SUCCESS;
        }
        else
        {
            rl_cfg.af_rate = KBPS_TO_BPS(shaper_p->shapingRate); /*shaping_rate is in kbit/s: 1 kilobit = 1000 bits */
            rl_cfg.be_rate = 0;

            ret = tmctl_egress_tm_rl_set(egress_tm_obj, &rl_cfg);
            if (ret)
            {
                tmctl_error("tmctl_egress_tm_rl_set() failed: af(%llu) ret(%d)\n", rl_cfg.af_rate, ret);
                bdmf_put(egress_tm_obj);
                return TMCTL_ERROR;
            }
        }

        bdmf_put(egress_tm_obj);
    }
    else
    {
        if (!shaper_p->shapingRate)
        {
            return TMCTL_SUCCESS;
        }

        ret = setup_us_orl_tm(shaper_p, &egress_tm_obj);
        if (ret)
        {
            tmctl_error("tmctl_RdpaCreateUsOrlTm() failed: af(%llu) ret(%d)\n", rl_cfg.af_rate, ret);
            return TMCTL_ERROR;
        }
    }

    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaGetOverAllShaper(tmctl_shaper_t *shaper_p, uint32_t *tcont_map)
{
    int                  rc;
    int                  ret           = TMCTL_SUCCESS;
    bdmf_object_handle   egress_tm_obj = BDMF_NULL;
    bdmf_number          tcont_id      = TMCTL_INVALID_KEY;
    rdpa_rl_cfg_t        rl_cfg        = {};
    bdmf_boolean         mgmt;
    bdmf_type_handle     tcont_drv     = rdpa_tcont_drv();
    bdmf_object_handle   tcont_obj     = BDMF_NULL;
    uint32_t             bit_map       = 0;
    bdmf_object_handle rate_limit_obj = BDMF_NULL;

    shaper_p->shapingRate = 0;
    shaper_p->shapingBurstSize = 0;
    *tcont_map = 0;

    ret = get_us_orl_tm(&egress_tm_obj);
    if (ret)
    {
        return ret;
    }


    rc = rdpa_egress_tm_rate_limit_get(egress_tm_obj, &rate_limit_obj);
    if ((rc) || (rate_limit_obj == BDMF_NULL))
    {
        rl_cfg.af_rate = 0;
        rc = 0;
    }
    else
    {
        rc = rdpa_rate_limit_cfg_get(rate_limit_obj, &rl_cfg);
    }

    if (rc)
    {
        tmctl_error("rdpa_egress_tm_rl_get() failed: rc(%d)\n", rc);
        ret = TMCTL_ERROR;
        goto exit;
    }

    shaper_p->shapingRate = BPS_TO_KBPS(rl_cfg.af_rate);

    /* get linked object */
    while ((tcont_obj = bdmf_get_next(tcont_drv, tcont_obj, NULL)))
    {
        rc = rdpa_tcont_management_get(tcont_obj, &mgmt);
        if (rc)
        {
            tmctl_error("rdpa_tcont_management_get() failed: rc(%d)\n",rc);
            ret = TMCTL_ERROR;
            continue;
        }

        if (mgmt)
            continue;

        if (is_linked(tcont_obj, egress_tm_obj))
        {
            rc = rdpa_tcont_index_get(tcont_obj, &tcont_id);
            if (rc)
            {
                tmctl_error("get_us_orl_tm()failed: rc(%d)\n",rc);
                ret = TMCTL_ERROR;
                continue;
            }

            // if rdpa_tcont_index_get() return success, tcont_id is OK
            /* coverity[large_shift] */
            bit_map |= (1U << ((uint32_t)tcont_id - 1));  /*RDPA_OMCI_TCONT_ID = 0, data tcont id begin from 1*/
        }
    }

    *tcont_map = bit_map;

exit:
    if (tcont_obj)
        bdmf_put(tcont_obj);

    if (egress_tm_obj)
        bdmf_put(egress_tm_obj);

    return ret;
}

tmctl_ret_e tmctl_RdpaLinkOverAllShaper(int tcont_id, BOOL do_link)
{
    int                  rc;
    int                  ret           = TMCTL_SUCCESS;
    bdmf_boolean         mgmt;
    bdmf_type_handle     tcont_drv     = rdpa_tcont_drv();
    bdmf_object_handle   tcont_obj     = BDMF_NULL;
    bdmf_object_handle   egress_tm_obj = BDMF_NULL;
    bdmf_number          tmp_id;

    ret = get_us_orl_tm(&egress_tm_obj);
    if (ret)
    {
        return ret;
    }

    if (tcont_id == TMCTL_ALL_TCONT_ID)
    {
          /* link all data TCONTs */
        while ((tcont_obj = bdmf_get_next(tcont_drv, tcont_obj, BDMF_NULL)))
        {
            rc = rdpa_tcont_management_get(tcont_obj, &mgmt);
            if (rc)
            {
                tmctl_error("rdpa_tcont_management_get() failed: rc(%d)\n",rc);
                ret = TMCTL_ERROR;
                continue;
            }

            if (mgmt)
                continue;

            if (is_linked(tcont_obj, egress_tm_obj) != do_link)
            {
                if (do_link)
                    rc = bdmf_link(tcont_obj, egress_tm_obj, NULL);
                else
                    rc = bdmf_unlink(tcont_obj, egress_tm_obj);

                if (rc)
                {
                    rdpa_tcont_index_get(tcont_obj, &tmp_id);
                    tmctl_error("%s tcont(%llu) failed: rc(%d)\n", do_link ? "bdmf_link()" : "bdmf_unlink()",  tmp_id, rc);
                    ret = TMCTL_ERROR;
                }
            }
        }
    }
    else         /* link the data TCONTs */
    {
        rc = rdpa_tcont_get((bdmf_number)tcont_id, &tcont_obj);
        if (rc)
        {
            tmctl_error("rdpa_tcont_get() failed: rc(%d)\n", rc);
            ret = TMCTL_ERROR;
            goto exit;
        }

        rc = rdpa_tcont_management_get(tcont_obj, &mgmt);
        if (rc)
        {
            tmctl_error("get_us_orl_tm()failed: rc(%d)\n",rc);
            ret = TMCTL_ERROR;
            goto exit;
        }

        if (!mgmt)
        {
            if (is_linked(tcont_obj, egress_tm_obj) != do_link)
            {
                if (do_link)
                    rc = bdmf_link(tcont_obj, egress_tm_obj, NULL);
                else
                    rc = bdmf_unlink(tcont_obj, egress_tm_obj);

                if (rc)
                {
                    tmctl_error("%s tcont(%d) failed: rc(%d)\n", do_link ? "bdmf_link()" : "bdmf_unlink()",  tcont_id, rc);
                    ret = TMCTL_ERROR;
                    goto exit;
                }
            }
        }
    }

exit:
    if (tcont_obj)
        bdmf_put(tcont_obj);

    if (egress_tm_obj)
        bdmf_put(egress_tm_obj);

    return ret;
}

#endif /* BCM_PON */
void tmctl_RdpaGetPortTmState(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   BOOL            *state)
{
    bdmf_object_handle egress_tm = BDMF_NULL;

    get_root_tm(devType, if_p, &egress_tm);
    *state = egress_tm ? TRUE : FALSE;
    if (devType == TMCTL_DEV_SVCQ)
    {
        bdmf_put(egress_tm);
    }    
}

#ifdef BCM_DSL_RDP
tmctl_ret_e tmctl_RdpaGetPortShaper(tmctl_devType_e dev_type,
                                   tmctl_if_t *if_p,
                                   tmctl_shaper_t *shaper_p)
{
    int rc;
    int ret = TMCTL_SUCCESS;
    tmctl_portTmParms_t tm = {0};
    bdmf_object_handle egress_tm_obj = BDMF_NULL;
    rdpa_rl_cfg_t rl_cfg = {};
    bdmf_object_handle rate_limit_obj = BDMF_NULL;

    get_port_tm_caps(dev_type, if_p, &tm);
    if (!tm.portShaper)
    {
        tmctl_error("Port shaper is not supported on this port");
        return TMCTL_ERROR;
    }

    shaper_p->shapingRate = 0;
    shaper_p->shapingBurstSize = 0;

    if (dev_type == TMCTL_DEV_SVCQ)
    {
        return __tmctl_RdpaGetPortShaper(dev_type, if_p, shaper_p);
    }

    ret = get_us_orl_tm(&egress_tm_obj);
    if (ret == TMCTL_NOT_FOUND)
    {
        /* No overall tm found means no shaping. */
        return TMCTL_SUCCESS;
    }

    rc = rdpa_egress_tm_rate_limit_get(egress_tm_obj, &rate_limit_obj);
    if ((rc) || (rate_limit_obj == BDMF_NULL))
    {
        rl_cfg.af_rate = 0;
        rc = 0;
    }
    else
    {
        rc = rdpa_rate_limit_cfg_get(rate_limit_obj, &rl_cfg);
    }
    if (rc)
    {
        tmctl_error("rdpa_egress_tm_rl_get() failed: rc(%d)\n", rc);
        ret = TMCTL_ERROR;
        goto exit;
    }

    shaper_p->shapingRate = BPS_TO_KBPS(rl_cfg.af_rate);

exit:
    if (egress_tm_obj)
        bdmf_put(egress_tm_obj);

    return ret;
}

tmctl_ret_e tmctl_RdpaSetPortShaper(tmctl_devType_e dev_type,
                                    tmctl_if_t *if_p,
                                    tmctl_shaper_t *shaper_p)
{
    bdmf_object_handle egress_tm_obj = BDMF_NULL;
    rdpa_rl_cfg_t rl_cfg = {0};
    tmctl_portTmParms_t tm = {0};
    int ret;

    get_port_tm_caps(dev_type, if_p, &tm);
    if (!tm.portShaper)
    {
        tmctl_error("Port shaper is not supported on this port");
        return TMCTL_ERROR;
    }

    if (dev_type == TMCTL_DEV_SVCQ)
    {
        return __tmctl_RdpaSetPortShaper(dev_type, if_p, shaper_p);
    }

    if (get_us_orl_tm(&egress_tm_obj) == TMCTL_SUCCESS)
    {
        if (!shaper_p->shapingRate) /*if rate=0 disable us overall shaper */
        {
            bdmf_put(egress_tm_obj);
            ret = bdmf_destroy(egress_tm_obj);
            if (ret)
            {
                tmctl_error("bdmf_destroy() failed ret(%d)\n",ret);
                return TMCTL_ERROR;
            }

            return TMCTL_SUCCESS;
        }

        rl_cfg.af_rate = KBPS_TO_BPS(shaper_p->shapingRate); /*shaping_rate is in kbit/s: 1 kilobit = 1000 bits */
        rl_cfg.be_rate = 0;

        ret = tmctl_egress_tm_rl_set(egress_tm_obj, &rl_cfg);
        if (ret)
        {
            tmctl_error("tmctl_egress_tm_rl_set() failed: af(%llu) ret(%d)\n", rl_cfg.af_rate, ret);
            bdmf_put(egress_tm_obj);
            return TMCTL_ERROR;
        }

        bdmf_put(egress_tm_obj);
    }
    else
    {
        bdmf_object_handle port = BDMF_NULL;

        if (!shaper_p->shapingRate)
        {
            return TMCTL_SUCCESS;
        }

        ret = setup_us_orl_tm(shaper_p, &egress_tm_obj);
        if (ret)
        {
            tmctl_error("setup_us_orl_tm() failed: ret(%d)\n", ret);
            return TMCTL_ERROR;
        }

        ret = rdpa_port_get(ifp_to_name(dev_type, if_p), &port);
        if (ret)
        {
            tmctl_error("failed to get port for ifname[%s], ret[%d]", ifp_to_name(dev_type, if_p), ret);
            bdmf_destroy(egress_tm_obj);
            return TMCTL_ERROR;
        }
        
        ret = bdmf_link(port, egress_tm_obj, NULL);
        if (ret)
        {
            tmctl_error("bdmf_link failed: ret(%d)\n", ret);
            ret = TMCTL_ERROR;
            bdmf_destroy(egress_tm_obj);
        }

        bdmf_put(port);

        return ret;
    }

    return TMCTL_SUCCESS;
}
#endif // BCM_DSL_RDP

#if defined(BCM_PON) || ((defined(CHIP_63158) || defined(CHIP_6813))  && defined(BRCM_OMCI))

tmctl_ret_e tmctl_RdpaSetTcontState(int tcont_id, BOOL tcont_state)
{
    int                  rc;
    int                  ret           = TMCTL_SUCCESS;
    bdmf_object_handle   tcont_obj     = BDMF_NULL;

    rc = rdpa_tcont_get((bdmf_number)tcont_id, &tcont_obj);
    if (rc)
    {
        tmctl_error("rdpa_tcont_get() failed: rc(%d)\n", rc);
        return TMCTL_ERROR;
    }

    rc = rdpa_tcont_enable_set(tcont_obj, (bdmf_boolean) tcont_state);
    if (rc)
    {
        tmctl_error("rdpa_enable_set() failed: rc(%d)\n", rc);
        ret = TMCTL_ERROR;
    }

    if (tcont_obj)
        bdmf_put(tcont_obj);

    return ret;
}

#endif /* defined(BCM_PON) || (defined(CHIP_63158) && defined(BRCM_OMCI)) */

#ifdef RUNNER_SWITCH
bdmf_object_handle get_mirror_dev(rdpa_port_mirror_cfg_t *param, tmctl_mirror_op_e op)
{
    return (op ==  TMCTL_MIRROR_RX_GET) ? param->rx_dst_port : param->tx_dst_port;
}

void print_if_params(bdmf_object_handle src, bdmf_object_handle dest)
{
    char src_name[IFNAMSIZ];
    char dest_name[IFNAMSIZ];
    rdpa_port_type src_type;
    rdpa_port_type dest_type;
    int rc; 
    rc = rdpa_port_name_get(src, src_name, IFNAMSIZ);
    if (rc)
        goto Error; 
    rc = rdpa_port_name_get(dest, dest_name, IFNAMSIZ);
    if (rc)
        goto Error; 

    rc = rdpa_port_type_get(src, &src_type);
    if (rc)
        goto Error; 

    rc = rdpa_port_type_get(src, &dest_type);
    if (rc)
        goto Error; 

    printf("Interface %s mirrored to interface %s\n", 
        src_name, dest_name);
    return;
Error:
    printf("failed to print interface params\n");
}

tmctl_ret_e tmctl_RdpaMirrorPrint(tmctl_mirror_op_e op)
{

    bdmf_object_handle obj = 0;
    rdpa_port_mirror_cfg_t param;
    printf(" Mirror %s configuration:\n", op == TMCTL_MIRROR_RX_GET ? "Rx" : "Tx" );

    while ((obj = bdmf_get_next(rdpa_port_drv(), obj, NULL)))
    {
        bdmf_object_handle mirror_dev;
        int rc;
        memset(&param, 0, sizeof(param));
        rc  = rdpa_port_mirror_cfg_get(obj, &param);
        if (rc)
            continue;
        mirror_dev = get_mirror_dev(&param, op);
        if (mirror_dev)
            print_if_params(obj, mirror_dev);
    }
    return TMCTL_SUCCESS;
}



tmctl_ret_e tmctl_RdpaClrMirror(tmctl_mirror_op_e op)
{
    bdmf_object_handle obj = BDMF_NULL;

    while ((obj = bdmf_get_next(rdpa_port_drv(), obj, NULL)))
    {
        rdpa_port_mirror_cfg_t param;
        rdpa_port_type type;
        int rc;
        rc = rdpa_port_mirror_cfg_get(obj, &param);
        if (rc)
            continue;

        rc = rdpa_port_type_get(obj, &type);
        if (rc || type == rdpa_port_cpu)
            continue;

        if ((op == TMCTL_MIRROR_RX_CLR) && (param.rx_dst_port != BDMF_NULL))
		    param.rx_dst_port = BDMF_NULL;
        else if ((op == TMCTL_MIRROR_TX_CLR) && (param.tx_dst_port != BDMF_NULL))
            param.tx_dst_port = BDMF_NULL;
        else continue;

        rdpa_port_mirror_cfg_set(obj, &param);
    }
    return TMCTL_SUCCESS;
}

tmctl_ret_e tmctl_RdpaAddMirror(const char *srcIfName, const char *destIfName, tmctl_mirror_op_e op, unsigned int truncate_size)
{
    bdmf_object_handle port_obj = BDMF_NULL;
    bdmf_object_handle destPort_obj = BDMF_NULL;
    rdpa_port_mirror_cfg_t param;
    int ret = TMCTL_ERROR;

    ret = rdpa_port_get(srcIfName, &port_obj);
    if (ret)
    {
        tmctl_error("failed to get port for interface[%s], ret[%d]",srcIfName, ret);
        goto Exit;
    }
    ret = rdpa_port_mirror_cfg_get(port_obj, &param);
    if (ret)
    {
        tmctl_error("failed to get port mirroring cfg for interface[%s], ret[%d]",srcIfName,  ret);
        goto Exit;
    }
    ret = rdpa_port_get(destIfName, &destPort_obj);
    if (ret)
    {
        tmctl_error("failed to get port for interface[%s], ret[%d]", destIfName, ret);
        goto Exit;
    }

    if (op == TMCTL_MIRROR_RX_ADD) {
        param.rx_dst_port = destPort_obj;
        param.rx_truncate_size = truncate_size;
    }
    else {
        param.tx_dst_port = destPort_obj;
        param.tx_truncate_size = truncate_size;
    }
    param.is_cntrl_disable = 0;

    ret = rdpa_port_mirror_cfg_set(port_obj, &param);
    if (ret)
        tmctl_error("failed to set port mirroring cfg for interface[%s], ret[%d]", srcIfName, ret);
Exit:        
    if (port_obj)
        bdmf_put(port_obj);
    if (destPort_obj)
        bdmf_put(destPort_obj);
    return ret;
}


tmctl_ret_e tmctl_RdpaDelMirror(const char *srcIfName, tmctl_mirror_op_e op)
{
    bdmf_object_handle port_obj = BDMF_NULL;
    rdpa_port_mirror_cfg_t param;
    int ret = TMCTL_ERROR;
    char *errString = NULL;

    ret = rdpa_port_get(srcIfName, &port_obj);
    if (ret)
    {
        errString = "failed to get port";
        goto Error;
    }
    ret = rdpa_port_mirror_cfg_get(port_obj, &param);
    if (ret)
    {
        errString = "failed to get port mirroring cfg";
        goto Error;
    }

    switch (op)
    {
    case TMCTL_MIRROR_RX_DEL:
        param.rx_dst_port = BDMF_NULL;
        break;
    case TMCTL_MIRROR_TX_DEL:
        param.tx_dst_port = BDMF_NULL;
        break;
    default:
        errString = "unsuported operation";
        ret = TMCTL_UNSUPPORTED;
        goto Error;
    }

    ret = rdpa_port_mirror_cfg_set(port_obj, &param);

Error:
    if (ret)
        tmctl_error("%s for %s, ret[%d] operation[%d]", errString, srcIfName, ret, op);
    if (port_obj) bdmf_put(port_obj);
    return ret;
}
#endif

