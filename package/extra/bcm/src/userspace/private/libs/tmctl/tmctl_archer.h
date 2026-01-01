/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2019:proprietary:standard

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/


#ifndef _TMCTL_ARCHER_H_
#define _TMCTL_ARCHER_H_

/*!\file tmctl_archer.h
 * \brief This file contains declarations for tmctl archer related functions.
 *
 */

#include "tmctl_api.h"
#include "ethswctl_api.h"

#if defined(CHIP_6765) || defined(CHIP_6766)
#define TMCTL_ARCHER_SQ_DS_QUEUE_MAX 20
#else
#define TMCTL_ARCHER_SQ_DS_QUEUE_MAX 31
#endif
#define TMCTL_ARCHER_SQ_DEFAULT_GROUP 0
#define TMCTL_ARCHER_SQ_DEFAULT_KBPS 1000000
#define TMCTL_ARCHER_SQ_DEFAULT_SHAPER_MBS 2000 /* Bytes */

/*TODO: Get queue size using archerctl api.
  Before that, qsize is hardcoded to ARCHER_DPI_DS_QUEUE_SIZE(512)
  which is defined in Archer DPI driver. */
#define TMCTL_ARCHER_SQ_DS_QUEUE_SIZE 512

tmctl_ret_e tmctlArcher_TmInit(const char* ifname);
tmctl_ret_e tmctlArcher_TmUninit(const char* ifname);
tmctl_ret_e tmctlArcher_getQueueCfg(const char* ifname, int qid,
                                    tmctl_queueCfg_t* qcfg_p);
tmctl_ret_e tmctlArcher_setQueueCfg(const char* ifname,
                                    tmctl_portTmParms_t* tmParms_p,
                                    tmctl_queueCfg_t* qcfg_p);
tmctl_ret_e tmctlArcher_setSysPortArbiter(const char* ifname,
                                          tmctl_portQcfg_t* portQcfg_p);
tmctl_ret_e tmctlArcher_resetSysPortArbiter(const char* ifname);
tmctl_ret_e tmctlArcher_setQueueDropAlg(int devType, const char* ifname, int qid,
                                        tmctl_queueDropAlg_t* dropAlg_p);
tmctl_ret_e tmctlArcher_getQueueDropAlg(int devType, const char* ifname, int qid,
                                        tmctl_queueDropAlg_t* dropAlg_p);
tmctl_ret_e tmctlArcher_getQueueStats(int devType, const char* ifname, int qid,
                                      tmctl_queueStats_t* stats_p);
tmctl_ret_e tmctlArcher_getDefQSize(tmctl_devType_e devType, uint32_t* qSize_p);

tmctl_ret_e tmctlArcher_sqGetTmParms(tmctl_portTmParms_t* tmParms_p);
tmctl_ret_e tmctlArcher_sqTmInit();
tmctl_ret_e tmctlArcher_sqTmUninit();
tmctl_ret_e tmctlArcher_sqSetQueueCfg(tmctl_queueCfg_t* qcfg_p);
tmctl_ret_e tmctlArcher_sqGetQueueCfg(int qid, tmctl_queueCfg_t* qcfg_p);
tmctl_ret_e tmctlArcher_sqSetPortShaper(tmctl_shaper_t* shaper_p);
tmctl_ret_e tmctlArcher_sqGetPortShaper(tmctl_shaper_t* shaper_p);
tmctl_ret_e tmctlArcher_sqGetQueueStats(int qid, tmctl_queueStats_t* stats_p);

#endif /* _TMCTL_ARCHER_H_ */
