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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "os_defs.h"
#include "tmctl_ethsw.h"
#include "tmctl_archer.h"

#include "archer_api.h"

/* ----------------------------------------------------------------------------
 * This function initializes the Archer QoS configuration for a port.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_TmInit(const char* ifname)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   tmctl_shaper_t shaper;
   int i;

   tmctl_debug("Enter: ifname=%s", ifname);

   /* Reset port sched mode to sp */
   ret = tmctlEthSw_resetPortSched(ifname);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_resetPortSched ERROR!");
      return ret;
   }

   /* Reset SysPort hardware arbiter mode to sp */
   ret = tmctlArcher_resetSysPortArbiter(ifname);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_resetSysPortArbiter ERROR!");
      return ret;
   }

   /* Reset port shaper */
   memset(&shaper, 0, sizeof(tmctl_shaper_t));

   ret = tmctlEthSw_setPortShaper(ifname, &shaper);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_setPortShaper ERROR! ret=%d", ret);
      return ret;
   }

   /* Reset queue shaper */
   for(i = 0; i < BCM_COS_COUNT; i++)
   {
      ret = tmctlEthSw_setQueueShaper(ifname, i, &shaper);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_setQueueShaper ERROR!");
         return ret;
      }
   }   
   
   return ret;
}


/* ----------------------------------------------------------------------------
 * This function un-initializes the Archer QoS configuration for a port.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_TmUninit(const char* ifname)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;

   tmctl_debug("Enter: ifname=%s", ifname);

   ret = tmctlArcher_TmInit(ifname);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_TmInit ERROR!");
      return ret;
   }
   
   return ret;
}


/* ----------------------------------------------------------------------------
 * This function gets the configuration of a Archer QoS queue.
 *
 * Parameters:
 *    ifname (IN)  Linux interface name.
 *    qid (IN)     queue id.
 *    qcfg_p (OUT) structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_getQueueCfg(const char* ifname, int qid,
                                    tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   tmctl_portQcfg_t portQcfg;
   uint32_t qSize;

   tmctl_debug("Enter: ifname=%s qid=%d", ifname, qid);

   if (qid >= BCM_COS_COUNT)
   {
      tmctl_error("qid should be smaller than %d, input qid=%d", BCM_COS_COUNT, qid);
      return TMCTL_ERROR;
   }
   
   ret = tmctlEthSw_getPortSched(ifname, &portQcfg);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_getPortSched ERROR!");
      return ret;
   }
   if (ret == TMCTL_UNSUPPORTED)
      return ret;

   ret = tmctlArcher_getDefQSize(TMCTL_DEV_ETH, &qSize);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_getDefQSize ERROR!");
      return ret;
   }
   
   memset(qcfg_p, 0, sizeof(tmctl_queueCfg_t));
   qcfg_p->schedMode = portQcfg.qcfg[qid].schedMode;
   qcfg_p->priority = portQcfg.qcfg[qid].priority;
   qcfg_p->weight = portQcfg.qcfg[qid].weight;
   qcfg_p->qid = qid;
   qcfg_p->qsize = qSize;

   ret = tmctlEthSw_getQueueShaper(ifname, qid, &(qcfg_p->shaper));
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_getQueueShaper ERROR!");
      return ret;
   }

   tmctl_debug("Done: ifname=%s qid=%d priority=%d schedMode=%d qsize=%d wt=%d shapingRate=%d burstSize=%d minRate=%d",
               ifname, qcfg_p->qid, qcfg_p->priority, qcfg_p->schedMode, qcfg_p->qsize, qcfg_p->weight,
               qcfg_p->shaper.shapingRate, qcfg_p->shaper.shapingBurstSize, qcfg_p->shaper.minRate);

   return ret;

}


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
tmctl_ret_e tmctlArcher_setQueueCfg(const char* ifname,
                                    tmctl_portTmParms_t* tmParms_p,
                                    tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   tmctl_portQcfg_t portQcfg;
   tmctl_portQcfg_t newPortQcfg;

   tmctl_debug("Enter: ifname=%s qid=%d priority=%d schedMode=%d qsize=%d wt=%d shapingRate=%d burstSize=%d minRate=%d",
               ifname, qcfg_p->qid, qcfg_p->priority, qcfg_p->schedMode, qcfg_p->qsize, qcfg_p->weight,
               qcfg_p->shaper.shapingRate, qcfg_p->shaper.shapingBurstSize, qcfg_p->shaper.minRate);

   ret = tmctlEthSw_getPortSched(ifname, &portQcfg);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_getPortSched ERROR!");
      return ret;
   }
   if (ret == TMCTL_UNSUPPORTED)
      return ret;

   if((qcfg_p->schedMode == TMCTL_SCHED_SP) && (qcfg_p->qid != qcfg_p->priority))
   {
      tmctl_error("qid and priority of SP queue should be the same, qid=%d, priority=%d", qcfg_p->qid, qcfg_p->priority);
      return TMCTL_ERROR;
   }   

   ret = tmctlEthSw_determineNewPortQcfg(tmParms_p, qcfg_p, &portQcfg, &newPortQcfg);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_determineNewPortQcfg ERROR! ifname=%s ret=%d", ifname, ret);
      return ret;
   }

   ret = tmctlEthSw_setPortSched(ifname, &newPortQcfg);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_setPortSched ERROR!");
      return ret;
   }

   ret = tmctlArcher_setSysPortArbiter(ifname, &newPortQcfg);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_setSysPortArbiter ERROR!");
      return ret;
   }

   ret = tmctlEthSw_setQueueShaper(ifname, qcfg_p->qid, &(qcfg_p->shaper));
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlEthSw_setQueueShaper ERROR!");
      return ret;
   }

   return ret;
}

/* ----------------------------------------------------------------------------
 * This function sets the SysPort hardware arbiter.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *    portQcfg_p (IN) The port queue configurations.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_setSysPortArbiter(const char*       ifname,
                                          tmctl_portQcfg_t* portQcfg_p)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;

#if defined(CHIP_6756) || defined(CHIP_6765) || defined(CHIP_6766)
   int i, credit;
   int numSpQ = 0;
   int wrrType = 0;
   int weights[BCM_COS_COUNT] = {0};
   tmctl_queueCfg_t* qcfg_p;
   sysport_hw_tm_arbiter_t arbiter;
   
   tmctl_debug("Enter: ifname=%s numQueues=%d", ifname, portQcfg_p->numQueues);

   if (portQcfg_p->numQueues != BCM_COS_COUNT)
   {
      tmctl_error("number of configured queues (%d) is less than port queues (%d)",
                  portQcfg_p->numQueues, BCM_COS_COUNT);
      return TMCTL_ERROR;
   }

   for (i = 0; i < portQcfg_p->numQueues; i++)
   {
      qcfg_p = &(portQcfg_p->qcfg[i]);
      
      if (qcfg_p->qid < 0)
      {
         tmctl_error("queue #%d is not configured. qid=%d", i, qcfg_p->qid);
         ret = TMCTL_ERROR;
         break;
      }
      
      if (qcfg_p->schedMode == TMCTL_SCHED_SP)
      {
         numSpQ++;
      }
      else
      {
         weights[i] = qcfg_p->weight;
         wrrType = qcfg_p->schedMode;
      }
   }
   
   if (ret != TMCTL_SUCCESS)
      return ret;

   if (numSpQ == 0)
   {
      if (wrrType == TMCTL_SCHED_WRR)
      {
         arbiter = SYSPORT_HW_TM_ARBITER_WRR;
      }
      else
      {
         arbiter = SYSPORT_HW_TM_ARBITER_DRR;
      }
   }
   else if (numSpQ == portQcfg_p->numQueues)
   {
      arbiter = SYSPORT_HW_TM_ARBITER_SP;
   }
   else
   {
      /* This is SPWRR case, but Sysport Arbiter does not
      support SPWRR mode. We use SP mode instead. */
      arbiter = SYSPORT_HW_TM_ARBITER_SP;
   }

   ret = archer_sysport_hw_tm_port_arbiter_set(ifname, arbiter);
   if (ret)
   {
       tmctl_error("archer_sysport_hw_tm_port_arbiter_set ERROR! ifname=%s ret=%d", ifname, ret);
       return ret;
   }

   for (i = portQcfg_p->numQueues - 1; i >= 0; i--)
   {
      switch(arbiter)
      {
         case SYSPORT_HW_TM_ARBITER_SP:
            credit = i+1;
            break;
   
         case SYSPORT_HW_TM_ARBITER_WRR:
            credit = weights[i];
            break;
   
         case SYSPORT_HW_TM_ARBITER_DRR:
            credit = weights[i] * 32;
            break;
   
         case SYSPORT_HW_TM_ARBITER_RR:
            credit = 0;
            break;
     
         default:
            tmctl_error("Invalid arbiterMode: %d", arbiter);
            return TMCTL_UNSUPPORTED;
      }
      ret = archer_sysport_hw_tm_txq_credit_set(ifname, i, credit);
      if (ret)
      {
          tmctl_error("archer_sysport_hw_tm_txq_credit_set ERROR! ifname=%s ret=%d", ifname, ret);
          return ret;
      }
   }
#endif

   return ret;
       
}  /* End of tmctlArcher_setSysPortArbiter() */


/* ----------------------------------------------------------------------------
 * This function resets the SysPort hardware arbiter to Strict Priority.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_resetSysPortArbiter(const char* ifname)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;

   int i;
   tmctl_portQcfg_t portQcfg;
   tmctl_queueCfg_t* qcfg_p;

   tmctl_debug("Enter: ifname=%s", ifname);

   memset(&portQcfg, 0, sizeof(tmctl_portQcfg_t));
   portQcfg.numQueues = BCM_COS_COUNT;
   for(i = 0; i < BCM_COS_COUNT; i++)
   {
      qcfg_p = &(portQcfg.qcfg[i]);
      qcfg_p->qid = i;
      qcfg_p->schedMode = TMCTL_SCHED_SP;
      qcfg_p->priority = i;         
      qcfg_p->weight = 0;
   }

   ret = tmctlArcher_setSysPortArbiter(ifname, &portQcfg);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_setSysPortArbiter ERROR!");
      return ret;
   }

   return ret;
       
}  /* End of tmctlArcher_resetSysPortArbiter() */


/* ----------------------------------------------------------------------------
 * This function gets the drop algorithm of a Archer QoS queue.
 *
 * Parameters:
 *    devType (IN)    tmctl device type.
 *    qid (IN)        queue id.
 *    dropAlg_p (OUT) structure to receive the drop algorithm parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_getQueueDropAlg(int devType, const char* ifname, int qid,
                                        tmctl_queueDropAlg_t* dropAlg_p)
{
    archer_drop_ioctl_t drop_ioctl;
    archer_drop_config_t *config_p = &drop_ioctl.config;
    archer_ioctl_cmd_t ioctl_cmd;

    switch(devType)
    {
        case TMCTL_DEV_XTM:
            strncpy(drop_ioctl.if_name, "xtm", ARCHER_IFNAMSIZ);
            ioctl_cmd = ARCHER_IOC_XTMDROPALG_GET;
            break;

        case TMCTL_DEV_ETH:
            strncpy(drop_ioctl.if_name, ifname, sizeof(drop_ioctl.if_name)-1);
            drop_ioctl.if_name[sizeof(drop_ioctl.if_name)-1] = '\0';
            ioctl_cmd = ARCHER_IOC_ENETDROPALG_GET;
            break;

        default:
            tmctl_error("Invalid devType: %d", devType);
            return TMCTL_UNSUPPORTED;
    }

    drop_ioctl.queue_id = qid;

    if(archer_cmd_send(ioctl_cmd, (unsigned long)&drop_ioctl))
    {
        tmctl_error("Could not archer_cmd_send\n");
        return TMCTL_ERROR;
    }

    dropAlg_p->dropAlgorithm = config_p->algorithm;
    dropAlg_p->priorityMask0 = config_p->priorityMask_0;
    dropAlg_p->priorityMask1 = config_p->priorityMask_1;
    dropAlg_p->dropAlgLo.redMinThreshold = config_p->profile[ARCHER_DROP_PROFILE_LOW].minThres;
    dropAlg_p->dropAlgLo.redMaxThreshold = config_p->profile[ARCHER_DROP_PROFILE_LOW].maxThres;
    dropAlg_p->dropAlgLo.redPercentage = config_p->profile[ARCHER_DROP_PROFILE_LOW].dropProb;
    dropAlg_p->dropAlgHi.redMinThreshold = config_p->profile[ARCHER_DROP_PROFILE_HIGH].minThres;
    dropAlg_p->dropAlgHi.redMaxThreshold = config_p->profile[ARCHER_DROP_PROFILE_HIGH].maxThres;
    dropAlg_p->dropAlgHi.redPercentage = config_p->profile[ARCHER_DROP_PROFILE_HIGH].dropProb;

    return TMCTL_SUCCESS;
}


/* ----------------------------------------------------------------------------
 * This function sets the drop algorithm of a Archer QoS queue.
 *
 * Parameters:
 *    devType (IN)   tmctl device type.
 *    qid (IN)       queue id.
 *    dropAlg_p (IN) structure to receive the drop algorithm parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_setQueueDropAlg(int devType, const char* ifname, int qid,
                                        tmctl_queueDropAlg_t* dropAlg_p)
{
    archer_drop_ioctl_t drop_ioctl;
    archer_drop_config_t *config_p = &drop_ioctl.config;
    archer_ioctl_cmd_t ioctl_cmd;

    switch(devType)
    {
        case TMCTL_DEV_XTM:
            strncpy(drop_ioctl.if_name, "xtm", sizeof(drop_ioctl.if_name)-1);
            drop_ioctl.if_name[sizeof(drop_ioctl.if_name)-1] = '\0';
            ioctl_cmd = ARCHER_IOC_XTMDROPALG_SET;
            break;

        case TMCTL_DEV_ETH:
            strncpy(drop_ioctl.if_name, ifname, sizeof(drop_ioctl.if_name)-1);
            drop_ioctl.if_name[sizeof(drop_ioctl.if_name)-1] = '\0';
            ioctl_cmd = ARCHER_IOC_ENETDROPALG_SET;
            break;

        default:
            tmctl_error("Invalid devType: %d", devType);
            return TMCTL_UNSUPPORTED;
    }

    drop_ioctl.queue_id = qid;

    config_p->algorithm = dropAlg_p->dropAlgorithm;
    config_p->priorityMask_0 = dropAlg_p->priorityMask0;
    config_p->priorityMask_1 = dropAlg_p->priorityMask1;
    config_p->profile[ARCHER_DROP_PROFILE_LOW].minThres = dropAlg_p->dropAlgLo.redMinThreshold;
    config_p->profile[ARCHER_DROP_PROFILE_LOW].maxThres = dropAlg_p->dropAlgLo.redMaxThreshold;
    config_p->profile[ARCHER_DROP_PROFILE_LOW].dropProb = dropAlg_p->dropAlgLo.redPercentage;
    config_p->profile[ARCHER_DROP_PROFILE_HIGH].minThres = dropAlg_p->dropAlgHi.redMinThreshold;
    config_p->profile[ARCHER_DROP_PROFILE_HIGH].maxThres = dropAlg_p->dropAlgHi.redMaxThreshold;
    config_p->profile[ARCHER_DROP_PROFILE_HIGH].dropProb = dropAlg_p->dropAlgHi.redPercentage;

    if(archer_cmd_send(ioctl_cmd, (unsigned long)&drop_ioctl))
    {
        tmctl_error("Could not archer_cmd_send\n");
        return TMCTL_ERROR;
    }

    return TMCTL_SUCCESS;
}

/* ----------------------------------------------------------------------------
 * This function gets the queue statistics of an Archer tx queue.
 * ARCHER_IOC_ENETTXQSTATS_GET gets the stats based on queue remapping.
 * So the stats of remapped queues will be the same.
 *
 * Parameters:
 *    devType (IN)  tmctl device type.
 *    ifname (IN)   Linux interface name.
 *    qid (IN)      Queue ID.
 *    stats_p (OUT) structure to receive the queue statistics.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_getQueueStats(int devType, const char* ifname, int qid,
                                      tmctl_queueStats_t* stats_p)
{
    archer_txq_stats_ioctl_t stats_ioctl;
    archer_ioctl_cmd_t ioctl_cmd;

    switch(devType)
    {
        case TMCTL_DEV_XTM:
            strncpy(stats_ioctl.if_name, "xtm", sizeof(stats_ioctl.if_name)-1);        
            stats_ioctl.if_name[sizeof(stats_ioctl.if_name)-1] = '\0';          
            ioctl_cmd = ARCHER_IOC_XTMTXQSTATS_GET;
            break;

        case TMCTL_DEV_ETH:
            strncpy(stats_ioctl.if_name, ifname, sizeof(stats_ioctl.if_name)-1);
            stats_ioctl.if_name[sizeof(stats_ioctl.if_name)-1] = '\0'; 
            ioctl_cmd = ARCHER_IOC_ENETTXQSTATS_GET;
            break;

        default:
            tmctl_error("Invalid devType: %d", devType);
            return TMCTL_UNSUPPORTED;
    }

    stats_ioctl.queue_id = qid;

    if(archer_cmd_send(ioctl_cmd, (unsigned long)&stats_ioctl))
    {
        tmctl_error("Could not archer_cmd_send\n");
        return TMCTL_ERROR;
    }

    stats_p->txPackets = stats_ioctl.stats.txPackets;
    stats_p->txBytes = stats_ioctl.stats.txBytes;
    stats_p->droppedPackets = stats_ioctl.stats.droppedPackets;
    stats_p->droppedBytes = stats_ioctl.stats.droppedBytes;

    return TMCTL_SUCCESS;
    
} /* End of tmctlArcher_getQueueStats() */

/* ----------------------------------------------------------------------------
 * This function get the default queue size of an Archer tx queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    dir (IN) direction.
 *    qSize (OUT) queue size.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_getDefQSize(tmctl_devType_e devType, uint32_t* qSize_p)
{
    archer_ioctl_cmd_t ioctl_cmd;
    uint32_t qSize_ioctl;

    switch(devType)
    {
        case TMCTL_DEV_XTM:
            ioctl_cmd = ARCHER_IOC_XTMTXQSIZE_GET;
            break;

        case TMCTL_DEV_ETH:
            ioctl_cmd = ARCHER_IOC_ENETTXQSIZE_GET;
            break;

        default:
            tmctl_error("Invalid devType: %d", devType);
            return TMCTL_ERROR;
    }

    if(archer_cmd_send(ioctl_cmd, (unsigned long)&qSize_ioctl))
    {
        tmctl_error("Could not archer_cmd_send\n");
        return TMCTL_ERROR;
    }

    *qSize_p = qSize_ioctl;

    return TMCTL_SUCCESS;
    
} /* End of tmctlArcher_getDefQSize() */


/* ----------------------------------------------------------------------------
 * This function gets Archer DPI Service Queue capabilities.
 *
 * Parameters:
 *    tmParms_p (OUT) structure to return Archer DPI Service Queue parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqGetTmParms(tmctl_portTmParms_t* tmParms_p)
{
   memset(tmParms_p, 0, sizeof(tmctl_portTmParms_t));

   tmParms_p->schedCaps   = TMCTL_SP_CAPABLE | TMCTL_WRR_CAPABLE;
   tmParms_p->maxQueues   = TMCTL_ARCHER_SQ_DS_QUEUE_MAX;
   tmParms_p->numQueues   = TMCTL_ARCHER_SQ_DS_QUEUE_MAX;
   tmParms_p->portShaper  = TRUE;
   tmParms_p->queueShaper = TRUE;

   return TMCTL_SUCCESS;   
}  /* End of tmctlArcher_sqGetTmParms() */


/* ----------------------------------------------------------------------------
 * This function initializes the Archer DPI Service Queue configuration.
 *
 * Parameters:
 *    None
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqTmInit()
{
   int ret, i;

   tmctl_debug("Enter");

   ret = archer_dpi_sq_group_enable(TMCTL_ARCHER_SQ_DEFAULT_GROUP);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_group_enable ERROR! group_index=%d, ret=%d", TMCTL_ARCHER_SQ_DEFAULT_GROUP, ret);
      return TMCTL_ERROR;
   }

   ret = archer_dpi_sq_group_set(TMCTL_ARCHER_SQ_DEFAULT_GROUP, TMCTL_ARCHER_SQ_DEFAULT_KBPS);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_group_set ERROR! group_index=%d, kbps=%d, ret=%d",
         TMCTL_ARCHER_SQ_DEFAULT_GROUP, TMCTL_ARCHER_SQ_DEFAULT_KBPS, ret);
      return TMCTL_ERROR;
   }

   ret = archer_dpi_sq_arbiter_set(TMCTL_ARCHER_SQ_DEFAULT_GROUP, ARCHER_SQ_ARBITER_RR);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_arbiter_set ERROR! group_index=%d, arbiter=%d, ret=%d",
         TMCTL_ARCHER_SQ_DEFAULT_GROUP, ARCHER_SQ_ARBITER_RR, ret);
      return TMCTL_ERROR;
   }

   for(i = 0; i < TMCTL_ARCHER_SQ_DS_QUEUE_MAX; i++)
   {
      ret = archer_dpi_sq_queue_map(i, TMCTL_ARCHER_SQ_DEFAULT_GROUP, i);
      if (ret)
      {
         tmctl_error("archer_dpi_sq_queue_map ERROR! queue_index=%d, group_index=%d, group_queue_index=%d, ret=%d",
            i, TMCTL_ARCHER_SQ_DEFAULT_GROUP, i, ret);
         return TMCTL_ERROR;
      }
   }

   for(i = 0; i < TMCTL_ARCHER_SQ_DS_QUEUE_MAX; i++)
   {
      ret = archer_dpi_sq_queue_set(i, 0, TMCTL_ARCHER_SQ_DEFAULT_KBPS);
      if (ret)
      {
         tmctl_error("archer_dpi_sq_queue_set ERROR! queue_index=%d, min_kbps=%d, max_kbps=%d, ret=%d",
            i, 0, TMCTL_ARCHER_SQ_DEFAULT_KBPS, ret);
         return TMCTL_ERROR;
      }
   }

   return TMCTL_SUCCESS;

} /* End of tmctlArcher_sqTmInit() */



/* ----------------------------------------------------------------------------
 * This function un-initializes the Archer DPI Service Queue configuration.
 *
 * Parameters:
 *    None
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqTmUninit()
{
   tmctl_ret_e ret = TMCTL_SUCCESS;

   tmctl_debug("Enter");

   ret = tmctlArcher_sqTmInit();
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctlArcher_sqTmInit ERROR!");
   }

   return ret;

} /* End of tmctlArcher_sqTmUninit() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of an Archer DPI Service Queue.
 *
 * Parameters:
 *    qcfg_p (IN) structure containing the queue config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqSetQueueCfg(tmctl_queueCfg_t* qcfg_p)
{
   int ret;
   archer_sq_arbiter_t arbiter;
   
   tmctl_debug("Enter: qid=%d schedMode=%d shapingRate=%d minRate=%d",
               qcfg_p->qid, qcfg_p->schedMode, qcfg_p->shaper.shapingRate, qcfg_p->shaper.minRate);

   if(qcfg_p->schedMode == TMCTL_SCHED_SP)
   {
      arbiter = ARCHER_SQ_ARBITER_SP;
   }
   else
   {
      arbiter = ARCHER_SQ_ARBITER_RR;
   }

   ret = archer_dpi_sq_arbiter_set(TMCTL_ARCHER_SQ_DEFAULT_GROUP, arbiter);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_arbiter_set ERROR! group_index=%d, arbiter=%d, ret=%d",
         TMCTL_ARCHER_SQ_DEFAULT_GROUP, arbiter, ret);
      return TMCTL_ERROR;
   }

   ret = archer_dpi_sq_queue_set(qcfg_p->qid, qcfg_p->shaper.minRate, qcfg_p->shaper.shapingRate);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_queue_set ERROR! queue_index=%d, min_kbps=%d, max_kbps=%d, ret=%d",
         qcfg_p->qid, qcfg_p->shaper.minRate, qcfg_p->shaper.shapingRate, ret);
      return TMCTL_ERROR;
   }

   return TMCTL_SUCCESS;

} /* End of tmctlArcher_sqSetQueueCfg() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of an Archer DPI Service Queue.
 *
 * Parameters:
 *    qid (IN)     queue id.
 *    qcfg_p (OUT) structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqGetQueueCfg(int qid, tmctl_queueCfg_t* qcfg_p)
{
   int ret, min_kbps, max_kbps;
   archer_sq_arbiter_t arbiter;

   tmctl_debug("Enter: qid=%d", qid);

   ret = archer_dpi_sq_queue_get(qid, &min_kbps, &max_kbps);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_queue_get ERROR! queue_index=%d, ret=%d", qcfg_p->qid, ret);
      return TMCTL_ERROR;
   }

   ret = archer_dpi_sq_arbiter_get(TMCTL_ARCHER_SQ_DEFAULT_GROUP, &arbiter);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_arbiter_get ERROR! group_index=%d, ret=%d", TMCTL_ARCHER_SQ_DEFAULT_GROUP, ret);
      return TMCTL_ERROR;
   }

   memset(qcfg_p, 0, sizeof(tmctl_queueCfg_t));

   if(arbiter == ARCHER_SQ_ARBITER_SP)
   {
      qcfg_p->priority = qid;
      qcfg_p->weight = 0;
      qcfg_p->schedMode = TMCTL_SCHED_SP;
   }
   else if((arbiter == ARCHER_SQ_ARBITER_RR) || (arbiter == ARCHER_SQ_ARBITER_WRR))
   {
      qcfg_p->priority = 0;
      /* ToDo: Get weight value for WRR queue. */
      qcfg_p->weight = 1;
      qcfg_p->schedMode = TMCTL_SCHED_WRR;
   }
   else
   {
      tmctl_error("Unexpected arbiter mode=%d", arbiter);
      return TMCTL_ERROR;
   }

   qcfg_p->qid = qid;
   qcfg_p->qsize = TMCTL_ARCHER_SQ_DS_QUEUE_SIZE;   
   qcfg_p->shaper.minRate = min_kbps;
   qcfg_p->shaper.shapingBurstSize = TMCTL_ARCHER_SQ_DEFAULT_SHAPER_MBS;
   qcfg_p->shaper.shapingRate = max_kbps;

   return TMCTL_SUCCESS;

} /* End of tmctlArcher_sqGetQueueCfg() */


/* ----------------------------------------------------------------------------
 * This function sets the shaping rate of an Archer DPI Service Queue group.
 *
 * Parameters:
 *    shaper_p (IN) structure containing the shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqSetPortShaper(tmctl_shaper_t* shaper_p)
{
   int ret;

   tmctl_debug("Enter: shapingRate=%d", shaper_p->shapingRate);

   ret = archer_dpi_sq_group_set(TMCTL_ARCHER_SQ_DEFAULT_GROUP, shaper_p->shapingRate);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_group_set ERROR! group_index=%d, kbps=%d, ret=%d",
         TMCTL_ARCHER_SQ_DEFAULT_GROUP, shaper_p->shapingRate, ret);
      return TMCTL_ERROR;
   }

   return TMCTL_SUCCESS;

} /* End of tmctlArcher_sqSetPortShaper() */


/* ----------------------------------------------------------------------------
 * This function gets the shaping rate of an Archer DPI Service Queue group.
 *
 * Parameters:
 *    shaper_p (OUT) structure to receive configuration parameters.
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqGetPortShaper(tmctl_shaper_t* shaper_p)
{
   int ret, max_kbps;

   tmctl_debug("Enter");

   ret = archer_dpi_sq_group_get(TMCTL_ARCHER_SQ_DEFAULT_GROUP, &max_kbps);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_group_get ERROR! group_index=%d, ret=%d",
         TMCTL_ARCHER_SQ_DEFAULT_GROUP, ret);
      return TMCTL_ERROR;
   }

   memset(shaper_p, 0, sizeof(tmctl_shaper_t));
   shaper_p->shapingRate = max_kbps;
   shaper_p->shapingBurstSize = TMCTL_ARCHER_SQ_DEFAULT_SHAPER_MBS;

   return TMCTL_SUCCESS;

} /* End of tmctlArcher_sqGetPortShaper() */


/* ----------------------------------------------------------------------------
 * This function gets the stats of an Archer DPI Service Queue.
 *
 * Parameters:
 *    qid (IN)      queue id.
 *    stats_p (OUT) structure to receive stats.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctlArcher_sqGetQueueStats(int qid, tmctl_queueStats_t* stats_p)
{
   int ret;

   tmctl_debug("Enter: qid=%d", qid);

   ret = archer_dpi_sq_queue_stats_get(qid, &stats_p->txPackets, &stats_p->txBytes,
                                            &stats_p->droppedPackets, &stats_p->droppedBytes);
   if (ret)
   {
      tmctl_error("archer_dpi_sq_queue_stats_get ERROR! queue_index=%d, ret=%d", qid, ret);
      return TMCTL_ERROR;
   }

   return TMCTL_SUCCESS;
}
