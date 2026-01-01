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

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "os_defs.h"
#include "board.h"
#include "tmctl_api.h"
#include "tmctl_api_trace.h"

#include "tmctl_archer.h"
#include "tmctl_ethsw.h"
#include "tmctl_archer.h"
#include "tmctl_sysporttm.h"
#if defined(BUILD_DSL)
#include "tmctl_xtm.h"
#endif

/* ----------------------------------------------------------------------------
 * This function initializes the basic TM settings for a port/tcont/llid based
 * on TM capabilities.
 *
 * Note that if the port had already been initialized, all its existing
 * configuration will be deleted before re-initialization.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    if_p (IN)      Port/TCONT/LLID identifier.
 *    cfgFlags (IN)  Port TM initialization config flags.
 *                   See bit definitions in tmctl_api.h
 *    numQueues (IN)   Number of queues to be set for TM.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_portTmInit_plat(tmctl_devType_e devType,
                                  tmctl_if_t*     if_p,
                                  uint32_t        cfgFlags,
                                  int             numQueues)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d cfgFlags=0x%04x numQueues=%d", devType, cfgFlags, numQueues);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      ret = tmctlSysportTm_portTmInit(if_p->ethIf.ifname);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_portTmInit ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlArcher_TmInit(if_p->ethIf.ifname);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_TmInit ERROR! ret=%d", ret);
         return ret;
      }
#endif
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      ret = tmctl_xtm_TmInit();
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqTmInit();
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqTmInit ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of tmctl_portTmInit_plat() */


/* ----------------------------------------------------------------------------
 * This function un-initializes all TM configurations of a port. This
 * function may be called when the port is down.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_portTmUninit_plat(tmctl_devType_e devType,
                                    tmctl_if_t*     if_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d", devType);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      /* Do the same thing with tmctlSysportTm_portTmInit */
      ret = tmctlSysportTm_portTmInit(if_p->ethIf.ifname);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_portTmInit ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlArcher_TmUninit(if_p->ethIf.ifname);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_TmUninit ERROR! ret=%d", ret);
         return ret;
      }
#endif
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      ret = tmctl_xtm_TmUnInit();
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqTmUninit();
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqTmUninit ERROR! ret=%d", ret);
         return ret;
      }
   }
   
   return ret;

}  /* End of tmctl_portTmUninit_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of a software queue. If the
 * configuration is not found, qid in the config structure will be
 * returned as -1.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *    queueId (IN) Queue ID must be in the range of [0..maxQueues-1].
 *    qcfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueCfg_plat(tmctl_devType_e   devType,
                                   tmctl_if_t*       if_p,
                                   int               queueId,
                                   tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      ret = tmctlSysportTm_getQueueCfg(if_p->ethIf.ifname, queueId, qcfg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_getQueueCfg ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlArcher_getQueueCfg(if_p->ethIf.ifname, queueId, qcfg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_getQueueCfg ERROR! ret=%d", ret);
         return ret;
      }
#if defined(CHIP_6765) || defined(CHIP_6766)
      // ports on external switch will use crossbow TxQ
      if (ret == TMCTL_UNSUPPORTED)
      {
          ret = tmctlSysportTm_getQueueCfg(if_p->ethIf.ifname, queueId, qcfg_p);
          if (ret == TMCTL_ERROR)
          {
             tmctl_error("tmctlSysportTm_getQueueCfg ERROR! ret=%d", ret);
             return ret;
          }
      }
#endif
#endif
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      uint32_t qSize;
      ret = tmctl_xtm_getQueueCfg(if_p->xtmIf.ifname, queueId, qcfg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_xtm_getQueueCfg ERROR!");
         return ret;
      }
      ret = tmctlArcher_getDefQSize(TMCTL_DEV_XTM, &qSize);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_getDefQSize ERROR!");
         return ret;
      }
      qcfg_p->qsize = qSize;
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqGetQueueCfg(queueId, qcfg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqGetQueueCfg ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of tmctl_getQueueCfg_plat() */


/* ----------------------------------------------------------------------------
 * This function configures a software queue for a port. The qeueu ID shall
 * be specified in the configuration parameter structure. If the queue
 * already exists, its configuration will be modified. Otherwise, the queue
 * will be added.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *    qcfg_p (IN)  Queue config parameters.
 *                 Notes:
 *                 - qid must be in the range of [0..maxQueues-1].
 *                 - For 63178 TMCTL_DEV_ETH device type,
 *                    -- the priority of SP queue must be set to qid.
 *                    -- the priority of WRR/WDRR/WFQ queue must be set to 0.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueCfg_plat(tmctl_devType_e   devType,
                                   tmctl_if_t*       if_p,
                                   tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d qid=%d priority=%d qsize=%d schedMode=%d wt=%d minRate=%d kbps=%d mbs=%d",
               devType, qcfg_p->qid, qcfg_p->priority, qcfg_p->qsize, qcfg_p->schedMode,
               qcfg_p->weight, qcfg_p->shaper.minRate,
               qcfg_p->shaper.shapingRate, qcfg_p->shaper.shapingBurstSize);

   if (devType == TMCTL_DEV_ETH)
   {
      tmctl_portTmParms_t tmParms;

      /* get the port tm parameters */
      ret = tmctl_getPortTmParms(TMCTL_DEV_ETH, if_p, &tmParms);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_getPortTmParms returns error");
         return ret;
      }

      if (((qcfg_p->schedMode == TMCTL_SCHED_SP) &&
           (tmParms.schedCaps & (TMCTL_SP_CAPABLE | TMCTL_SP_WRR_CAPABLE))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WRR) &&
           (tmParms.schedCaps & (TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WDRR) &&
           (tmParms.schedCaps & (TMCTL_WDRR_CAPABLE | TMCTL_SP_WDRR_CAPABLE))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WFQ) &&
           (tmParms.schedCaps & (TMCTL_WFQ_CAPABLE))))
      {
#if defined(CHIP_47622)
         ret = tmctlSysportTm_setQueueCfg(if_p->ethIf.ifname, &tmParms, qcfg_p);
         if (ret == TMCTL_ERROR)
         {
            tmctl_error("tmctlArcher_setQueueCfg ERROR! ret=%d", ret);
            return ret;
         }
#else
         ret = tmctlArcher_setQueueCfg(if_p->ethIf.ifname, &tmParms, qcfg_p);
         if (ret == TMCTL_ERROR)
         {
            tmctl_error("tmctlArcher_setQueueCfg ERROR! ret=%d", ret);
            return ret;
         }
#if defined(CHIP_6765) || defined(CHIP_6766)
          // ports on external switch will use crossbow TxQ
          if (ret == TMCTL_UNSUPPORTED)
          {
              ret = tmctlSysportTm_setQueueCfg(if_p->ethIf.ifname, &tmParms, qcfg_p);
              if (ret == TMCTL_ERROR)
              {
                 tmctl_error("tmctlSysportTm_setQueueCfg ERROR! ret=%d", ret);
                 return ret;
              }
          }
#endif
#endif
      }
      else
      {
         tmctl_error("Queue sched mode %d is not supported", qcfg_p->schedMode);
         return TMCTL_ERROR;
      }
      tmctlEthSw_setQueueState(if_p->ethIf.ifname, qcfg_p->qid, 1 /*enable*/);
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      tmctl_portTmParms_t tmParms;

      /* get the port tm parameters */
      ret = tmctl_getPortTmParms(TMCTL_DEV_XTM, if_p, &tmParms);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_getPortTmParms returns error");
         return ret;
      }

      if (((qcfg_p->schedMode == TMCTL_SCHED_SP) &&
           (tmParms.schedCaps & (TMCTL_SP_CAPABLE | TMCTL_SP_WRR_CAPABLE))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WRR) &&
           (tmParms.schedCaps & (TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WDRR) &&
           (tmParms.schedCaps & (TMCTL_WDRR_CAPABLE | TMCTL_SP_WDRR_CAPABLE))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WFQ) &&
           (tmParms.schedCaps & (TMCTL_WFQ_CAPABLE))))
      {
         /* To let XTM queue shaper config work, remove the XTM queue first before adding it. */
         ret = tmctl_xtm_remQueueCfg(if_p->xtmIf.ifname, qcfg_p->qid);
         if (ret == TMCTL_ERROR)
         {
            tmctl_error("tmctl_xtm_remQueueCfg ERROR! ret=%d", ret);
            return ret;
         }
         ret = tmctl_xtm_setQueueCfg(if_p->xtmIf.ifname, &tmParms, qcfg_p);
         if (ret == TMCTL_ERROR)
         {
            tmctl_error("tmctl_xtm_setQueueCfg ERROR! ret=%d", ret);
            return ret;
         }
      }
      else
      {
         tmctl_error("Queue sched mode %d is not supported", qcfg_p->schedMode);
         return TMCTL_ERROR;
      }
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqSetQueueCfg(qcfg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqSetQueueCfg ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of tmctl_setQueueCfg_plat() */


/* ----------------------------------------------------------------------------
 * This function deletes a software queue from a port.
 *
 * Note that for Ethernet port with an external Switch, the corresponding
 * Switch queue will not be deleted.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *    queueId (IN) The queue ID.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_delQueueCfg_plat(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   int             queueId)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);
   if (devType == TMCTL_DEV_ETH)
   {
      /* Archer Ethernet LAN/WAN does not support queue deleting. Return success
         to avoid error stop if user always delete a queue before set it. */
      ret = TMCTL_SUCCESS;
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      ret = tmctl_xtm_remQueueCfg(if_p->xtmIf.ifname, queueId);
#endif
   }

   return ret;

}  /* End of tmctl_delQueueCfg_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the port shaper configuration.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    if_p (IN)      Port identifier.
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPortShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d", devType);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      ret = tmctlSysportTm_getPortShaper(if_p->ethIf.ifname, shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_getPortShaper ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlEthSw_getPortShaper(if_p->ethIf.ifname, shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_getPortShaper ERROR! ret=%d", ret);
         return ret;
      }
#if defined(CHIP_6765) || defined(CHIP_6766)
      // ports on external switch will use crossbow TxQ
      if (ret == TMCTL_UNSUPPORTED)
      {
          ret = tmctlSysportTm_getPortShaper(if_p->ethIf.ifname, shaper_p);
          if (ret == TMCTL_ERROR)
          {
             tmctl_error("tmctlSysportTm_getPortShaper ERROR! ret=%d", ret);
             return ret;
          }
      }
#endif
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqGetPortShaper(shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqGetPortShaper ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of tmctl_getPortShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function configures the port shaper for shaping rate, shaping burst
 * size and minimum rate. If port shaping is to be done by the external
 * Switch, the corresponding Switch port shaper will be configured.
 *
 * Parameters:
 *    devType (IN)  tmctl device type.
 *    if_p (IN)     Port identifier.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPortShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d portKbps=%d portMbs=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      ret = tmctlSysportTm_setPortShaper(if_p->ethIf.ifname, shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_setPortShaper ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlEthSw_setPortShaper(if_p->ethIf.ifname, shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_setPortShaper ERROR! ret=%d", ret);
         return ret;
      }
#if defined(CHIP_6765) || defined(CHIP_6766)
      // ports on external switch will use crossbow TxQ
      if (ret == TMCTL_UNSUPPORTED)
      {
          ret = tmctlSysportTm_setPortShaper(if_p->ethIf.ifname, shaper_p);
          if (ret == TMCTL_ERROR)
          {
             tmctl_error("tmctlSysportTm_setPortShaper ERROR! ret=%d", ret);
             return ret;
          }
      }
#endif
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqSetPortShaper(shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqSetPortShaper ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;
}  /* End of tmctl_setPortShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the port rx rate control configuration.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    if_p (IN)      Port identifier.
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPortRxRate_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d", devType);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      tmctl_error("tmctlSysportTm_getPortRxRate ERROR! ret=%d", ret);
      return ret;
#else
      ret = tmctlEthSw_getPortRxRate(if_p->ethIf.ifname, shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_getPortRxRate ERROR! ret=%d", ret);
         return ret;
      }
#endif
   }

   return ret;

}  /* End of tmctl_getPortRxRate_plat() */


/* ----------------------------------------------------------------------------
 * This function configures the port Rx  rate, shaping burst
 * size and minimum rate. If port shaping is to be done by the external
 * Switch, the corresponding Switch port shaper will be configured.
 *
 * Parameters:
 *    devType (IN)  tmctl device type.
 *    if_p (IN)     Port identifier.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPortRxRate_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d portKbps=%d portMbs=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      tmctl_error("tmctlSysportTm_setPortRxRate ERROR! ret=%d", ret);
      return ret;
#else
      ret = tmctlEthSw_setPortRxRate(if_p->ethIf.ifname, shaper_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_setPortRxRate ERROR! ret=%d", ret);
         return ret;
      }
#endif
   }

   return ret;
}  /* End of tmctl_setPortRxRate_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the overall shaper configuration.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    if_p     (OUT) The portMap which linked to the overall shaper
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{

    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: devType=%d \n", devType);

    return ret;

}  /* End of tmctl_getOverAllShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function configures a overall shaper for shaping rate, shaping burst
 * size.
 *
 * Parameters:
 *    devType (IN)  tmctl device type.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_shaper_t* shaper_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: devType=%d Rate=%d BurstSize=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);

    return ret;

}  /* End of tmctl_setOverAllShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function link port to the overall shaper
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_linkOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: devType=%d \n", devType);

    return ret;
}  /* End of tmctl_linkOverAllShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function unlink port from the overall shaper
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_unlinkOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: devType=%d", devType);

    return ret;
}  /* End of tmctl_unlinkOverAllShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the drop algorithm of a queue.
 *
 * Parameters:
 *    devType (IN)    tmctl device type.
 *    if_p (IN)       Port identifier.
 *    queueId (IN)    Queue ID.
 *    dropAlg_p (OUT) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueDropAlg_plat(tmctl_devType_e       devType,
                                       tmctl_if_t*           if_p,
                                       int                   queueId,
                                       tmctl_queueDropAlg_t* dropAlg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);

   if (devType == TMCTL_DEV_ETH)
   {
      ret = tmctlArcher_getQueueDropAlg(devType, if_p->ethIf.ifname, queueId, dropAlg_p);
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      int channelId;
      ret = tmctl_xtm_getTxChannelByQid(if_p->xtmIf.ifname, queueId, &channelId);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_xtm_getTxChannelByQid returns error");
         return ret;
      }
      ret = tmctlArcher_getQueueDropAlg(devType, NULL, channelId, dropAlg_p);
#endif
   }

   return ret;

}  /* End of tmctl_getQueueDropAlg_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the drop algorithm of a queue.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    if_p (IN)      Port identifier.
 *    queueId (IN)   Queue ID.
 *    dropAlg_p (IN) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueDropAlg_plat(tmctl_devType_e       devType,
                                       tmctl_if_t*           if_p,
                                       int                   queueId,
                                       tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d dropAlgorithm=%d "
               "priorityMask0=0x%x priorityMask1=0x%x "
               "Lo(minThr=%d maxThr=%d pct=%d) "
               "Hi(minThr=%d maxThr=%d pct=%d)",
               devType, if_p->ethIf.ifname, queueId, dropAlg_p->dropAlgorithm,
               dropAlg_p->priorityMask0, dropAlg_p->priorityMask1,
               dropAlg_p->dropAlgLo.redMinThreshold, dropAlg_p->dropAlgLo.redMaxThreshold,
               dropAlg_p->dropAlgLo.redPercentage,
               dropAlg_p->dropAlgHi.redMinThreshold, dropAlg_p->dropAlgHi.redMaxThreshold,
               dropAlg_p->dropAlgHi.redPercentage);

   if (devType == TMCTL_DEV_ETH)
   {
      ret = tmctlArcher_setQueueDropAlg(devType, if_p->ethIf.ifname, queueId, dropAlg_p);
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      int channelId;
      ret = tmctl_xtm_getTxChannelByQid(if_p->xtmIf.ifname, queueId, &channelId);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_xtm_getTxChannelByQid returns error");
         return ret;
      }
      ret = tmctlArcher_setQueueDropAlg(devType, NULL, channelId, dropAlg_p);
#endif
   }

   return ret;

}  /* End of tmctl_setQueueDropAlg_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the drop algorithm of a XTM channel.
 *
 * Parameters:
 *    devType (IN)    tmctl device type.
 *    channelId (IN)  Channel ID.
 *    dropAlg_p (OUT) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getXtmChannelDropAlg_plat(tmctl_devType_e       devType,
                                            int                   channelId,
                                            tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d channelId=%d", devType, channelId);

   ret = tmctlArcher_getQueueDropAlg(devType, NULL, channelId, dropAlg_p);

   return ret;

}  /* End of tmctl_getXtmChannelDropAlg_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the drop algorithm of a XTM channel.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    channelId (IN) Channel ID.
 *    dropAlg_p (IN) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setXtmChannelDropAlg_plat(tmctl_devType_e       devType,
                                            int                   channelId,
                                            tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d channelId=%d dropAlgorithm=%d "
               "priorityMask0=0x%x priorityMask1=0x%x",
               devType, channelId, dropAlg_p->dropAlgorithm, 
               dropAlg_p->priorityMask0, dropAlg_p->priorityMask1);

   ret = tmctlArcher_setQueueDropAlg(devType, NULL, channelId, dropAlg_p);

   return ret;

}  /* End of tmctl_setXtmChannelDropAlg_plat() */

/* ----------------------------------------------------------------------------
 * This function configures the flow control of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    flowCtrl_p (IN) The flow control configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueFlowCtrl_plat(tmctl_devType_e        devType,
                                        tmctl_if_t*            if_p,
                                        int                    queueId,
                                        tmctl_queueFlowCtrl_t* flowCtrl_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d",
               devType, if_p->ethIf.ifname, queueId);

   return ret;
} /* End of tmctl_setQueueFlowCtrl_plat */

/* ----------------------------------------------------------------------------
 * This function gets the flow control configuration of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    flowCtrl_p (OUT) The flow control configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueFlowCtrl_plat(tmctl_devType_e        devType,
                                        tmctl_if_t*            if_p,
                                        int                    queueId,
                                        tmctl_queueFlowCtrl_t* flowCtrl_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d",
               devType, if_p->ethIf.ifname, queueId);

   return ret;
} /* End of tmctl_getQueueFlowCtrl_plat */

/* ----------------------------------------------------------------------------
 * This function gets the statistics of a queue.
 *
 * Parameters:
 *    devType (IN)  tmctl device type.
 *    if_p (IN)     Port identifier.
 *    queueId (IN)  Queue ID.
 *    stats_p (OUT) The queue stats.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueStats_plat(tmctl_devType_e     devType,
                                     tmctl_if_t*         if_p,
                                     int                 queueId,
                                     tmctl_queueStats_t* stats_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      ret = tmctlSysportTm_getQueueStats(if_p->ethIf.ifname, queueId, stats_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_getQueueStats ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlArcher_getQueueStats(devType, if_p->ethIf.ifname, queueId, stats_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_getQueueStats ERROR! ret=%d", ret);
         return ret;
      }
#endif
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      int channelId;
      ret = tmctl_xtm_getTxChannelByQid(if_p->xtmIf.ifname, queueId, &channelId);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_xtm_getTxChannelByQid returns error");
         return ret;
      }
      ret = tmctlArcher_getQueueStats(devType, if_p->xtmIf.ifname, channelId, stats_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_getQueueStats ERROR! ret=%d", ret);
         return ret;
      }
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret= tmctlArcher_sqGetQueueStats(queueId, stats_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqGetQueueStats ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of tmctl_getQueueStats_plat() */


/* ----------------------------------------------------------------------------
 * This function gets port TM parameters (capabilities).
 *
 * Parameters:
 *    devType (IN)    tmctl device type.
 *    if_p (IN)       Port identifier.
 *    tmParms_p (OUT) Structure to return port TM parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPortTmParms_plat(tmctl_devType_e      devType,
                                      tmctl_if_t*          if_p,
                                      tmctl_portTmParms_t* tmParms_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d", devType);

   if (devType == TMCTL_DEV_ETH)
   {
#if defined(CHIP_47622)
      ret = tmctlSysportTm_getPortTmParms(if_p->ethIf.ifname, tmParms_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlSysportTm_getPortTmParms ERROR! ret=%d", ret);
         return ret;
      }
#else
      ret = tmctlEthSw_getPortTmParms(if_p->ethIf.ifname, tmParms_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_getPortTmParms ERROR! ret=%d", ret);
         return ret;
      }
#if defined(CHIP_6765) || defined(CHIP_6766)
      // ports on external switch will use crossbow TxQ
      if (ret == TMCTL_UNSUPPORTED)
      {
          ret = tmctlSysportTm_getPortTmParms(if_p->ethIf.ifname, tmParms_p);
          if (ret == TMCTL_ERROR)
          {
             tmctl_error("tmctlSysportTm_getPortTmParms ERROR! ret=%d", ret);
             return ret;
          }
      }
#endif
#endif
   }
   else if(devType == TMCTL_DEV_XTM)
   {
#if defined(BUILD_DSL)
      ret = tmctl_xtm_getTmParms(if_p->xtmIf.ifname, tmParms_p);
#endif
   }
   else if(devType == TMCTL_DEV_SVCQ)
   {
      ret = tmctlArcher_sqGetTmParms(tmParms_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlArcher_sqGetTmParms ERROR! ret=%d", ret);
         return ret;
      }
   }

   return ret;

}  /* End of tmctl_getPortTmParms_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of dscp to pbit table. If the
 * configuration is not found, ....
 *
 * Parameters:
 *    cfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getDscpToPbit_plat(tmctl_dscpToPbitCfg_t* cfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: ");

   return ret;

}  /* End of tmctl_getDscpToPbit_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of dscp to pbit table.
 *
 * Parameters:
 *    cfg_p (IN) config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setDscpToPbit_plat(tmctl_dscpToPbitCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    int i = 0;

    tmctl_debug("Enter: ");
    for (i = 0; i < TOTAL_DSCP_NUM; i++)
    {
        tmctl_debug("dscp[%d]=%d", i, cfg_p->dscp[i]);
    }

    return ret;

}  /* End of tmctl_setDscpToPbit_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of traffic class to queue table.
 *
 * Parameters:
 *    dir (IN) direction.
 *    cfg_p (IN) config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setTcToQueue_plat(int table_index, tmctl_tcToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    int i = 0;

    tmctl_debug("Enter: ");
    for (i = 0; i < TOTAL_TC_NUM; i++)
    {
        tmctl_debug("tc[%d]=%d", i, cfg_p->tc[i]);
    }

    return ret;

}  /* End of tmctl_setTcToQueue_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of pbit to q table. If the
 * configuration is not found, ....
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *    cfg_p (OUT)  Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPbitToQ_plat(tmctl_devType_e devType,
                                  tmctl_if_t* if_p,
                                  tmctl_pbitToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: devType=%d", devType);

    return ret;

}  /* End of tmctl_getPbitToQ_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of pbit to q table.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN)    Port identifier.
 *    cfg_p (IN)   config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPbitToQ_plat(tmctl_devType_e devType,
                                  tmctl_if_t* if_p,
                                  tmctl_pbitToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    int i = 0;

    tmctl_debug("Enter: devType=%d", devType);

    for (i = 0; i < TOTAL_PBIT_NUM; i++)
    {
        tmctl_debug("pbit[%d]=%d", i, cfg_p->pbit[i]);
    }

   return ret;

}  /* End of tmctl_setPbitToQ_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of dscp to pbit feature.
 *
 * Parameters:
 *    dir (IN)       direction.
 *    enable_p (OUT) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getForceDscpToPbit_plat(tmctl_dir_e dir, BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d", dir);

    return ret;

}  /* End of tmctl_getForceDscpToPbit_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of dscp to pbit feature.
 *
 * Parameters:
 *    dir (IN)      direction.
 *    enable_p (IN) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setForceDscpToPbit_plat(tmctl_dir_e dir, BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d enable=%d", dir, (*enable_p));

    return ret;

}  /* End of tmctl_setForceDscpToPbit_plat() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of packet based qos.
 *
 * Parameters:
 *    dir (IN)       direction.
 *    type (IN)      qos type
 *    enable_p (OUT) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPktBasedQos_plat(tmctl_dir_e dir,
                                      tmctl_qosType_e type,
                                      BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d type=%d", dir, type);

    return ret;

}  /* End of tmctl_getPktBasedQos_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of packet based qos.
 *
 * Parameters:
 *    dir (IN)      direction.
 *    type (IN)     qos type
 *    enable_p (IN) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPktBasedQos_plat(tmctl_dir_e     dir,
                                      tmctl_qosType_e type,
                                      BOOL*           enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d type=%d enable=%d", dir, type, (*enable_p));

    return ret;

}  /* End of tmctl_setPktBasedQos_plat() */


/* ----------------------------------------------------------------------------
 * This function sets the size of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    size    (IN) The drop threshold configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueSize_plat(tmctl_devType_e devType,
                                    tmctl_if_t*     if_p,
                                    int             queueId,
                                    int             size)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d queueId=%d size=%d", devType, queueId, size);

   return ret;

}  /* End of tmctl_setQueueSize_plat() */


/* ----------------------------------------------------------------------------
 * This function sets shaper of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    shaper  (IN) Queue Shaper configuration
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueShaper_plat(tmctl_devType_e devType,
                                      tmctl_if_t*     if_p,
                                      int             queueId,
                                      tmctl_shaper_t  *shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: devType=%d queueId=%d", devType, queueId);

   return ret;
}  /* End of tmctl_setQueueShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function create shaper for queues
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    shaper  (IN) Queue Shaper configuration
 *    shaper_obj  (OUT) pointer to shaper object
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_CreateShaper_plat(tmctl_devType_e          devType,
                                      tmctl_if_t*        if_p,
                                      tmctl_shaper_t     *shaper_p,
                                      uint64_t *shaper_obj)
{
    return TMCTL_UNSUPPORTED;
}


/* ----------------------------------------------------------------------------
 * This function delete shaper for queues
 *
 * Parameters:
 *    shaper_obj  (IN) pointer to shaper object
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_DeleteShaper_plat(uint64_t shaper_obj)
{
    return TMCTL_UNSUPPORTED;
}

/* ----------------------------------------------------------------------------
 * This function set subset shaper for queues
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) queue to use
 *    shaper_obj (IN) shaper object created - NULL to delete shaper from queue
 * 
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_SetShaperQueue_plat(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     int queueId,
                                     uint64_t shaper_obj)
{
    return TMCTL_UNSUPPORTED;
}


/* ----------------------------------------------------------------------------
 * This function creates a traffic policer object.
 *
 * Parameters:
 *    policer_p (IN) traffic policer configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_createPolicer_plat(tmctl_policer_t *policer_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    return ret;
}  /* End of tmctl_createPolicer_plat() */


/* ----------------------------------------------------------------------------
 * This function modifies a traffic policer object.
 *
 * Parameters:
 *    policer_p (IN) traffic policer configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_modifyPolicer_plat(tmctl_policer_t *policer_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    return ret;
}  /* End of tmctl_modifyPolicer_plat() */


/* ----------------------------------------------------------------------------
 * This function deletes a traffic policer object.
 *
 * Parameters:
 *    policer_p (IN) traffic policer configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_deletePolicer_plat(int policerId)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    return ret;
}    /* End of tmctl_deletePolicer_plat() */


#if defined(CHIP_47622)
    #define SF2_UNIT    1
#else
    #define SF2_UNIT    0
#endif

#if defined(CHIP_6756) || defined(CHIP_6765) || defined(CHIP_6766)
    #define SF2_DUAL    1
#endif

/* ----------------------------------------------------------------------------
 * This function gets tx/rx mirror config.
 *
 * Parameters:
 *    op                (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getMirror_plat(tmctl_mirror_op_e op)
{
    tmctl_mirror_t mirror;
    memset(&mirror, 0x00, sizeof(tmctl_mirror_t));
    mirror.op = op;
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    mirror.unit = SF2_UNIT;
    ret = tmctl_EthSwGetMirror(&mirror);
    tmctl_EthSwMirrorPrint(&mirror);
#if defined(SF2_DUAL)
    mirror.unit = 1;
    if (tmctl_EthSwGetMirror(&mirror) == 0)
        tmctl_EthSwMirrorPrint(&mirror);
#endif

#if defined(BUILD_DSL)
    ret = tmctl_xtm_getMirror(op);
    if (ret != TMCTL_SUCCESS)
    {
        tmctl_error("tmctl_xtm_getMirror failed! ret=%d", ret);
        return ret;
    }
#endif

    return ret;
}

/* ----------------------------------------------------------------------------
 * This function clears tx/rx mirror config
 *
 * Parameters:
 *    op                (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_clrMirror_plat(tmctl_mirror_op_e op)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    tmctl_mirror_t mirror;

#if defined(BUILD_DSL)
    ret = tmctl_xtm_clrMirror(op);
    if (ret != TMCTL_SUCCESS)
    {
        tmctl_error("tmctl_xtm_clrMirror failed! ret=%d", ret);
        return ret;
    }
#endif
    
    memset(&mirror, 0x00, sizeof(tmctl_mirror_t));

    mirror.op = op;
    mirror.unit = SF2_UNIT;
    ret = tmctl_EthSwClrMirror(&mirror);

#if defined(SF2_DUAL)
    mirror.unit = 1;
    tmctl_EthSwClrMirror(&mirror);
#endif
    return ret;
}

/* ----------------------------------------------------------------------------
 * This function adds tx/rx mirror config
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    if_p            (IN) Port identifier.
 *    destIf          (IN) dest port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_addMirror_plat(tmctl_devType_e        devType,
                                tmctl_if_t*         if_p,
                                tmctl_mirror_op_e op,
                                tmctl_ethIf_t       destIf,
                                unsigned int        truncate_size)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    tmctl_mirror_t      mirror;
    int unit, port;

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
        ret = tmctl_xtm_addMirror(if_p->xtmIf.ifname, destIf.ifname, op);
        if (ret != TMCTL_SUCCESS)
            tmctl_error("tmctl_xtm_addMirror failed! ret=%d", ret);
        return ret;
    }
#endif

    memset(&mirror, 0x00, sizeof(tmctl_mirror_t));
    mirror.op = op;
    
#if defined(SF2_DUAL)
    if (devType != TMCTL_DEV_ETH || bcm_enet_map_ifname_to_unit_port(if_p->ethIf.ifname, &unit, &port))
#else
    if (devType != TMCTL_DEV_ETH || bcm_enet_map_ifname_to_unit_port(if_p->ethIf.ifname, &unit, &port) || unit !=SF2_UNIT)
#endif
    {
        tmctl_error("Only support mirroring on eth ports on SF2 switch");
        return TMCTL_UNSUPPORTED;
    }
    mirror.unit = unit;
    mirror.srcPort = port;
    if (bcm_enet_map_ifname_to_unit_port(destIf.ifname, &unit, &port) || unit !=mirror.unit)
    {
        tmctl_error("Both source and mirroring ports need to be on same SF2 switch");
        return TMCTL_UNSUPPORTED;
    }
    mirror.destPort = port;
    
    ret = tmctl_EthSwAddMirror(&mirror);

    return ret;
}

/* ----------------------------------------------------------------------------
 * This function dels tx/rx mirror config
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    op                (IN) specified RX/TX operation
 *    if_p            (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_delMirror_plat(tmctl_devType_e        devType,
                                tmctl_mirror_op_e   op,
                                tmctl_if_t*         if_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    tmctl_mirror_t      mirror;
    int unit, port;

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
        ret = tmctl_xtm_delMirror(if_p->xtmIf.ifname, op);
        if (ret != TMCTL_SUCCESS)
            tmctl_error("tmctl_xtm_delMirror failed! ret=%d", ret);
        return ret;
    }
#endif
    
    memset(&mirror, 0x00, sizeof(tmctl_mirror_t));
    mirror.op = op;
    
#if defined(SF2_DUAL)
    if (devType != TMCTL_DEV_ETH || bcm_enet_map_ifname_to_unit_port(if_p->ethIf.ifname, &unit, &port))
#else
    if (devType != TMCTL_DEV_ETH || bcm_enet_map_ifname_to_unit_port(if_p->ethIf.ifname, &unit, &port) || unit !=SF2_UNIT)
#endif
    {
        tmctl_error("Only support mirroring on eth ports on SF2 switch");
        return TMCTL_UNSUPPORTED;
    }
    mirror.unit = unit;
    mirror.srcPort = port;
    
    ret = tmctl_EthSwDelMirror(&mirror);

    return ret;
}

/* Not used today for non PON platforms 
 * used to determine if queues are configured on port
 */
void tmctl_getPortTmState_plat(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   BOOL            *state)
{
    *state = TRUE;
}

void tmctl_getDevTypeByIfName_plat(tmctl_devType_e *devType, 
                                   char **oifname,
                                   const char *ifname)
{
    *oifname = ifname;
}
