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
#ifndef __TMCTL_BDMF_RDPA_H_
#define __TMCTL_BDMF_RDPA_H_

#include "user_api.h"
#include "tmctl_api.h"
#include "rdpa_drv.h"

#if defined(BCM_DSL_RDP)
#define MAX_Q_PER_TM_MULTI_LEVEL 8
#else
#define MAX_Q_PER_TM_MULTI_LEVEL 32
#endif
#define TMCTL_ORL_TM_id          -1
#define TMCTL_ALL_TCONT_ID       -1

/* high priorty number get lowset idx */
#define HIGH_P_LOW_IDX 1

#if !defined(BCM_PON_XRDP) && !defined(BCM_DSL_XRDP)
#define MAX_Q_PER_SID 4
#else
#define MAX_Q_PER_SID 8
#endif /* !defined(BCM_PON_XRDP) && !defined(BCM_DSL_XRDP */

#define MAX_Q_PER_LAN_TM 8

#if defined(CHIP_6846) || defined(BCM_DSL_RDP)
#define MAX_Q_PER_WAN_TM 8
#define MAX_Q_SERVICE_QUEUE_TM 8
#else
#define MAX_Q_PER_WAN_TM 32
#define MAX_Q_SERVICE_QUEUE_TM 32
#endif

#ifdef RDPA_XRDP_US_DS_AGNOSTIC
#define MAX_Q_PER_TM MAX_Q_PER_WAN_TM
#endif
#define BDMF_NULL (bdmf_object_handle)0

#ifdef TM_C_CODE
#define RDPA_TM_SECONDARY_SERVICE_QUEUE_ENABLE 0
#define RDPA_TM_SECONDARY_LEVEL rdpa_tm_level_secondary
#define RDPA_TM_SECONDARY_SCHEDULER_MODE rdpa_tm_sched_sp
#define RDPA_TM_SECONDARY_NUM_QUEUES 4
#undef RDPA_TM_SECONDARY_SP_ELEMENTS 
#else
#define RDPA_TM_SECONDARY_SERVICE_QUEUE_ENABLE 1
#define RDPA_TM_SECONDARY_LEVEL rdpa_tm_level_queue
#define RDPA_TM_SECONDARY_SCHEDULER_MODE rdpa_tm_sched_sp_wrr
#define RDPA_TM_SECONDARY_NUM_QUEUES 8
#define RDPA_TM_SECONDARY_SP_ELEMENTS 8
#endif
#define RDPA_TM_SECONDARY_DEFAULT_WEIGHT 1

#define PACKET_SIZE 1536 /* this is value is used only to convert 'tmctl packet based api' into XRDP api which is in bytes */

#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
/* On XRDP platforms queue size is in bytes*/
#define getDeviceQueueSize(usrSize) (usrSize * PACKET_SIZE)
#define getUsrQueueSize(devSize) (devSize / PACKET_SIZE)
#else
/* On RDP platforms queue size is in packets*/
#define getDeviceQueueSize(usrSize) (usrSize)
#define getUsrQueueSize(devSize) (devSize)
#endif /* BCM_PON_XRDP || defined(BCM_DSL_XRDP */

#define convertDropAlg(tmDropAlg) \
    ((tmDropAlg == TMCTL_DROP_DT) ? rdpa_tm_drop_alg_dt :\
    ((tmDropAlg == TMCTL_DROP_RED) ? rdpa_tm_drop_alg_red :\
    ((tmDropAlg == TMCTL_DROP_WRED) ? rdpa_tm_drop_alg_wred :\
    ((tmDropAlg == TMCTL_DROP_CODEL) ? rdpa_tm_drop_alg_codel :\
    ((tmDropAlg == TMCTL_DROP_PI2) ? rdpa_tm_drop_alg_pi2 :\
    ((tmDropAlg == TMCTL_DROP_L4S_CQ) ? rdpa_tm_drop_alg_dualq_pi2 :\
    ((tmDropAlg == TMCTL_DROP_L4S_LQ) ? rdpa_tm_drop_alg_dualq_laqm :\
    ((tmDropAlg == TMCTL_DROP_FLOW_CTRL) ? rdpa_tm_drop_alg_flow_ctrl :\
    rdpa_tm_drop_alg_dt))))))))

#define IS_ACTIV_Q(q_cfg) \
    (q_cfg.queue_id != BDMF_INDEX_UNASSIGNED && q_cfg.drop_threshold)

#define IS_SINGLE_LEVEL(level) \
    (level == rdpa_tm_level_queue)

#define KBPS_TO_BPS(kbps) (kbps) * 1000ULL
#define BPS_TO_KBPS(kbps) (kbps) / 1000ULL

tmctl_ret_e tmctl_RdpaTmInit(tmctl_devType_e dev_Type,
                                  tmctl_if_t *if_p,
                                  uint32_t cfg_flags,
                                  int num_queues);

tmctl_ret_e tmctl_RdpaTmUninit( tmctl_devType_e dev_type,
                               tmctl_if_t *if_p);

tmctl_ret_e tmctl_RdpaQueueCfgGet(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  tmctl_queueCfg_t* qcfg_p);

tmctl_ret_e tmctl_RdpaTmQueueSet(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_queueCfg_t* qcfg_p);

tmctl_ret_e tmctl_RdpaTmQueueDel(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id);

tmctl_ret_e tmctl_RdpaGetPortShaper(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_shaper_t* shaper_p);

tmctl_ret_e tmctl_RdpaSetPortShaper(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_shaper_t* shaper_p);

tmctl_ret_e tmctl_RdpaGetPortRxRate(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_shaper_t* shaper_p);

tmctl_ret_e tmctl_RdpaSetPortRxRate(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_shaper_t* shaper_p);

tmctl_ret_e tmctl_RdpaGetQueueDropAlg(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  tmctl_queueDropAlg_t *dropAlg_p);

tmctl_ret_e tmctl_RdpaSetQueueDropAlg(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  tmctl_queueDropAlg_t* dropAlg_p);

tmctl_ret_e tmctl_RdpaGetQueueStats(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  tmctl_queueStats_t* stats_p);

tmctl_ret_e tmctl_RdpaSetQueueSize(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  int size);

tmctl_ret_e tmctl_RdpaSetQueueShaper(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  int queue_id,
                                  tmctl_shaper_t *shaper_p);

tmctl_ret_e tmctl_RdpaCreateShaper(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_shaper_t *shaper_p,
                                  uint64_t *shaper_obj);

tmctl_ret_e tmctl_RdpaDeleteShaper(uint64_t shaper_obj);

tmctl_ret_e tmctl_RdpaSetShaperQueue(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     int queueId,
                                     uint64_t shaper_obj);

tmctl_ret_e tmctl_RdpaGetPortTmParms(tmctl_devType_e dev_type,
                                    tmctl_if_t *if_p,
                                    tmctl_portTmParms_t *tm_parms);
void tmctl_RdpaGetPortTmState(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   BOOL            *state);

#if defined(POLICER_SUPPORT)
tmctl_ret_e tmctl_RdpaCreatePolicer(tmctl_policer_t *policer_p);
tmctl_ret_e tmctl_RdpaModifyPolicer(tmctl_policer_t *policer_p);
tmctl_ret_e tmctl_RdpaDeletePolicer(int policerId);
#endif
#if defined(CONFIG_BCM_RUNNER_QOS_MAPPER)
tmctl_ret_e tmctlRdpa_getPbitToQ(tmctl_devType_e devType,
                                 tmctl_if_t *if_p,
                                 tmctl_pbitToQCfg_t* cfg_p);


tmctl_ret_e tmctl_RdpaGetDscpToPbit(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_dscpToPbitCfg_t* cfg_p);

tmctl_ret_e tmctl_RdpaSetDscpToPbit(tmctl_devType_e dev_type,
                                  tmctl_if_t *if_p,
                                  tmctl_dscpToPbitCfg_t* cfg_p);
#if defined(CONFIG_BCM_PLATFORM_RDP_PRV)
tmctl_ret_e tmctl_RdpaSetTc2Queue(int table_index,
                                  tmctl_tcToQCfg_t* cfg_p);
#endif

tmctl_ret_e tmctlRdpa_setPbitToQ(tmctl_devType_e devType,
                                 tmctl_if_t *if_p,
                                 tmctl_pbitToQCfg_t* cfg_p);

tmctl_ret_e tmctlRdpa_getForceDscpToPbit(tmctl_dir_e dir, BOOL* enable_p);
tmctl_ret_e tmctlRdpa_setForceDscpToPbit(tmctl_dir_e dir, BOOL* enable_p);
tmctl_ret_e tmctlRdpa_getPktBasedQos(tmctl_dir_e dir, tmctl_qosType_e type, BOOL* enable_p);
tmctl_ret_e tmctlRdpa_setPktBasedQos(tmctl_dir_e dir, tmctl_qosType_e type, BOOL* enable_p);
tmctl_ret_e tmctlRdpa_setQueueSize(int devType, tmctl_if_t *if_p, int qid, int size);
#endif

#if defined(BCM_PON)
tmctl_ret_e tmctl_RdpaSetOverAllShaper(tmctl_shaper_t *shaper_p);
tmctl_ret_e tmctl_RdpaGetOverAllShaper(tmctl_shaper_t *shaper_p, uint32_t *tcont_map);
tmctl_ret_e tmctl_RdpaLinkOverAllShaper(int tcont_id, BOOL do_link);
#endif

#if defined(BCM_PON) || ((defined(CHIP_63158) || defined(CHIP_6813)) && defined(BRCM_OMCI))
tmctl_ret_e tmctl_RdpaSetTcontState(int tcont_id, BOOL tcont_state);
#endif

#if defined(RUNNER_SWITCH)
tmctl_ret_e tmctl_RdpaClrMirror(tmctl_mirror_op_e op);
tmctl_ret_e tmctl_RdpaAddMirror(const char *srcIfName, const char *destIfName, tmctl_mirror_op_e op, unsigned int truncate_size);
tmctl_ret_e tmctl_RdpaDelMirror(const char *srcIfName, tmctl_mirror_op_e op);
tmctl_ret_e tmctl_RdpaMirrorPrint(tmctl_mirror_op_e op);
#endif

rdpa_epon_mode get_epon_mode(void);
int is_supported_wan_mode(void);
tmctl_ret_e tmctlRdpa_getMemoryInfo(int * fpmPoolMemorySize);
int get_tm_owner(tmctl_devType_e devType, tmctl_if_t *if_p, bdmf_object_handle *owner);

#endif /* __TMCTL_BDMF_RDPA_H_ */
