/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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


#ifndef _TMCTL_XTM_H_
#define _TMCTL_XTM_H_

#define TMCTL_DEF_XTM_DPU_Q_SZ    (1024)
#define TMCTL_MIN_XTM_DPU_Q_SZ    (512)
#define TMCTL_MAX_XTM_DPU_Q_SZ    (2048)

#define XTM_MIN_QOS_QUEUE_IDX       0
#define XTM_MAX_QOS_QUEUE_IDX       15

/* ----------------------------------------------------------------------------
 * This function gets XTM transmit channel index by Queue ID.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *    qid (IN) Queue ID.
 *    txChannel_p (OUT) XTM transmit channel index.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getTxChannelByQid(const char* ifname, int qid, int* txChannel_p);


/* ----------------------------------------------------------------------------
 * This function gets port TM parameters (capabilities) from xtmctl driver.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *    tmParms_p (OUT) Structure to return port TM parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getTmParms(const char* ifname, tmctl_portTmParms_t* tmParms_ps);


/* ----------------------------------------------------------------------------
 * This function gets the port shaper configuration of a xtm port.
 *
 * Parameters:
 *    ifname (IN)    Linux interface name.
 *    shaper_p (OUT) The port shaper configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getPortShaper(const char*     ifname,
                                    tmctl_shaper_t* shaper_p);


/* ----------------------------------------------------------------------------
 * This function sets the port shaper configuration of a xtm port.
 *
 * Parameters:
 *    ifname (IN)   Linux interface name.
 *    shaper_p (IN) The port shaper configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_setPortShaper(const char*     ifname,
                                    tmctl_shaper_t* shaper_p);


/* ----------------------------------------------------------------------------
 * This function Remove the XTM SAR queue from the XTM driver.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *    devType (IN) rdpactl device type.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_remQueueCfg(const char* ifname, int qid);

/* ----------------------------------------------------------------------------
 * This function gets the configuration of a xtm connection configuration.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *    qid (IN) Queue ID.
 *    qcfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getQueueCfg(const char* ifname, int qid, tmctl_queueCfg_t* qcfg_p);

/* ----------------------------------------------------------------------------
 * This function sets the configuration of a queue.
 *
 * Parameters:
 *    ifname (IN)    Linux interface name.
 *    tmParms_p (IN) port tm parameters.
 *    qcfg_p (IN)    structure containing the queue config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_setQueueCfg(const char* ifname,
                                  tmctl_portTmParms_t* tm_Parms_p,
                                  tmctl_queueCfg_t* qcfg_p);


/* ----------------------------------------------------------------------------
 * This function gets the statistics of a queue.
 *
 * Parameters:
 *    devType (IN) rdpactl device type.
*    if_p (IN) Port identifier.
 *    qid (IN) Queue ID.
 *    queueStats_p (OUT) Structure to return the queue stats.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getQueueStats(int devType, tmctl_if_t* if_p, int qid,
                                    tmctl_queueStats_t* queueStats_p);

/* ----------------------------------------------------------------------------
 * This function sets the size of all queues.
 *
 * Parameters:
 *    size_p (OUT) uint32_t to return the queue size.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_setQueueSize(int size);
tmctl_ret_e tmctl_xtm_getQueueSize(int32_t *psize);

/* ----------------------------------------------------------------------------
 * This function prints the mirror configuration of the xtm interfaces.
 *
 * Parameters:
 *    op (IN) tmctl mirror operation type.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getMirror(tmctl_mirror_op_e op);

/* ----------------------------------------------------------------------------
 * This function clears the mirror configuration of the xtm interfaces.
 *
 * Parameters:
 *    op (IN) tmctl mirror operation type.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_clrMirror(tmctl_mirror_op_e op);

/* ----------------------------------------------------------------------------
 * This function adds a mirror configuration of the xtm interface.
 *
 * Parameters:
 *    srcIfName (IN) Name of monitor interface.
 *    destIfName (IN) Name of mirror interface.
 *    op (IN) tmctl mirror operation type.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_addMirror(const char *srcIfName, const char *destIfName, tmctl_mirror_op_e op);

/* ----------------------------------------------------------------------------
 * This function deletes a mirror configuration of the xtm interface.
 *
 * Parameters:
 *    srcIfName (IN) Name of monitor interface.
 *    op (IN) tmctl mirror operation type.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_delMirror(const char *srcIfName, tmctl_mirror_op_e op);

/* ----------------------------------------------------------------------------
 * This function initializes the XTM TM configuration
 *
 * Parameters:
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_TmInit();

/* ----------------------------------------------------------------------------
 * This function uninitializes the XTM TM configuration
 *
 * Parameters:
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_TmUnInit();
#endif //_TMCTL_XTM_H_

