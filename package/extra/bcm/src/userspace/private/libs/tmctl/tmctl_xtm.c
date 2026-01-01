/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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
#include <ctype.h>
#include <net/if.h>
#include <unistd.h>
#include "os_defs.h"

#include "tmctl_api.h"
#include "tmctl_xtm.h"
#include "devctl_xtm.h"
#include "bcm_retcodes.h"
#include "bcmnet.h"


/*************************************************************************************
 * Function Name: getConnAddrCfg
 * Description  : Returns XTM_ADDR and XTM_CONN_CFG structure by Linux interface name.
 * Returns      : 0 - success, non-0 - error
 *************************************************************************************/
static int getConnAddrCfg(const char* ifname, PXTM_ADDR pAddr, PXTM_CONN_CFG pCfg)
{
   int i;
   PXTM_ADDR pAddrs = NULL;
   PXTM_ADDR pAddrsMem = NULL;
   uint32_t ulNumAddrs = 0;
   BOOL addrFound = FALSE;

   devCtl_xtmGetConnAddrs( NULL, &ulNumAddrs );
   if(!ulNumAddrs)
   {
      tmctl_debug("Failed to get connection address.");
      return TMCTL_ERROR;
   }

   pAddrsMem = (PXTM_ADDR) malloc(ulNumAddrs * sizeof(XTM_ADDR));
   if(pAddrsMem)
   {
      devCtl_xtmGetConnAddrs(pAddrsMem, &ulNumAddrs );
      for( i = 0, pAddrs = pAddrsMem; i < ulNumAddrs; i++, pAddrs++ )
      {
         if(pAddrs && (BCMRET_SUCCESS == (BcmRet) devCtl_xtmGetConnCfg(pAddrs, pCfg)))
         {
            if (!strcmp(ifname, pCfg->netDeviceName))
            {
               memcpy(pAddr, pAddrs, sizeof(XTM_ADDR));
               addrFound = TRUE;
               break;
            }
         }
   
      }
      free(pAddrsMem);
   }

   if(!addrFound)
   {
      tmctl_debug("Failed to get connection configuration for %s.", ifname);
      return TMCTL_ERROR;   
   }

   return TMCTL_SUCCESS;

} /* End of getConnAddrCfg */

/*************************************************************************************
 * Function Name: getConnIfname
 * Description  : Get first Linux interface name from the XTM connection config table.
 * Returns      : 0 - success, non-0 - error
 *************************************************************************************/
static int getConnIfname(char* ifname)
{
   int i;
   uint32_t ulNumAddrs = 0;
   PXTM_ADDR pAddrs = NULL;
   PXTM_ADDR pAddrsMem = NULL;
   XTM_CONN_CFG connCfg;

   devCtl_xtmGetConnAddrs( NULL, &ulNumAddrs );
   if(!ulNumAddrs)
   {
      tmctl_debug("Failed to get connection address.");
      return TMCTL_ERROR;
   }

   pAddrsMem = (PXTM_ADDR) malloc(ulNumAddrs * sizeof(XTM_ADDR));
   if(pAddrsMem)
   {
      devCtl_xtmGetConnAddrs(pAddrsMem, &ulNumAddrs );
      for( i = 0, pAddrs = pAddrsMem; i < ulNumAddrs; i++, pAddrs++ )
      {
         if(pAddrs && (BCMRET_SUCCESS == (BcmRet) devCtl_xtmGetConnCfg(pAddrs, &connCfg)))
         {
            strncpy(ifname, connCfg.netDeviceName, IFNAMSIZ);
            break;            
         }
      }
      free(pAddrsMem);
   }

   return TMCTL_SUCCESS;

} /* End of getConnIfname */


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
tmctl_ret_e tmctl_xtm_getTxChannelByQid(const char* ifname, int qid, int* txChannel_p)
{
   int i;
   BOOL qFound;
   int ulMaxQueues;
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;

   if((qid < XTM_MIN_QOS_QUEUE_IDX) || (qid > XTM_MAX_QOS_QUEUE_IDX))
   {
      tmctl_error("qid is out of range and invalid");
      return TMCTL_ERROR;
   }

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_error("Failed to get connection address and config.");
      return TMCTL_ERROR;
   }

   if(xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM)
   {
      ulMaxQueues = MAX_ATM_TRANSMIT_QUEUES;
   }
   else
   {
      ulMaxQueues = MAX_PTM_TRANSMIT_QUEUES;
   }

   if((qid < XTM_MIN_QOS_QUEUE_IDX) || (qid >= ulMaxQueues))
   {
      tmctl_error("qid %d out of range [%d-%d].", qid, XTM_MIN_QOS_QUEUE_IDX, ulMaxQueues - 1);
      return TMCTL_ERROR;
   }

   qFound = FALSE;
   for (i = 0, pTxQ = conn_cfg.TransmitQParms; i < conn_cfg.ulTransmitQParmsSize; i++, pTxQ++)
   {
      if (pTxQ->ucQosQId == qid)
      {
         qFound = TRUE;
         break;
      }
   }

   if(!qFound)
   {
      return TMCTL_ERROR;
   }

   *txChannel_p = pTxQ->ulTxQueueIdx;

   tmctl_debug("ifname=%s, qid=%d, txChannel=%d", ifname, qid, *txChannel_p);

   return TMCTL_SUCCESS;

}  /* End of tmctl_xtm_getTxChannelByQid() */


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
tmctl_ret_e tmctl_xtm_getTmParms(const char* ifname, tmctl_portTmParms_t* tmParms_ps)
{
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_debug("Failed to get connection address and config. Please confirm DSL link is up.");
      return TMCTL_ERROR;
   }

   if(xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM)
   {
      tmParms_ps->maxQueues   = MAX_ATM_TRANSMIT_QUEUES;
      tmParms_ps->maxSpQueues = MAX_ATM_TRANSMIT_QUEUES;
      tmParms_ps->queueShaper = FALSE;
   }
   else
   {
      tmParms_ps->maxQueues   = MAX_PTM_TRANSMIT_QUEUES;
      tmParms_ps->maxSpQueues = MAX_PTM_TRANSMIT_QUEUES;
      tmParms_ps->queueShaper = TRUE;
   }

#ifdef XTM_PORT_SHAPING
   tmParms_ps->portShaper  = TRUE;
#else
   tmParms_ps->portShaper  = FALSE;
#endif

   tmParms_ps->dualRate    = TRUE;
   tmParms_ps->cfgFlags    = TMCTL_INIT_DEFAULT_QUEUES | \
                             TMCTL_QIDPRIO_MAP_Q7P7 | \
                             TMCTL_SCHED_TYPE_SP;
   tmParms_ps->schedCaps   = TMCTL_SP_CAPABLE | \
                             TMCTL_WRR_CAPABLE | \
                             TMCTL_SP_WRR_CAPABLE | \
                             TMCTL_WFQ_CAPABLE;

   return TMCTL_SUCCESS;

}  /* End of tmctl_xtm_getTmParms() */


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
                                    tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   
#ifdef XTM_PORT_SHAPING
   uint32_t ulPortId;
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;
   XTM_INTERFACE_CFG intf_cfg;

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));
   memset(&intf_cfg, 0, sizeof(XTM_INTERFACE_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_error("Failed to get connection address and config.");
      return TMCTL_ERROR;
   }   

   if( xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM )
   {
      ulPortId = xtm_addr.u.Vcc.ulPortMask;
   }
   else
   {
      ulPortId = xtm_addr.u.Flow.ulPortMask;
   }

   if(BCMRET_SUCCESS == (BcmRet) devCtl_xtmGetInterfaceCfg(ulPortId, &intf_cfg))
   {
      if(intf_cfg.ulPortShaping == XTM_PORT_SHAPING_ON)
      {
         shaper_p->shapingRate = intf_cfg.ulShapeRate / 1000;
      }
      else
      {
         shaper_p->shapingRate = 0;
      }
      shaper_p->shapingBurstSize = intf_cfg.usMbs;
   }
   else
   {
      tmctl_error("Failed to get xtm interface config.");
      return TMCTL_ERROR;
   }
#else
   ret = TMCTL_UNSUPPORTED;
#endif

   return ret;
   
}  /* End of tmctl_xtm_getPortShaper() */


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
                                    tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   
#ifdef XTM_PORT_SHAPING
   uint32_t ulPortId;
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;
   XTM_INTERFACE_CFG intf_cfg;

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));
   memset(&intf_cfg, 0, sizeof(XTM_INTERFACE_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_error("Failed to get connection address and config.");
      return TMCTL_ERROR;
   }   

   if( xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM )
   {
      ulPortId = xtm_addr.u.Vcc.ulPortMask;
   }
   else
   {
      ulPortId = xtm_addr.u.Flow.ulPortMask;
   }

   if(BCMRET_SUCCESS == (BcmRet) devCtl_xtmGetInterfaceCfg(ulPortId, &intf_cfg))
   {
      if(shaper_p->shapingRate > 0)
      {
         if(shaper_p->shapingBurstSize < 0 || shaper_p->shapingBurstSize > 65535)
         {
            tmctl_error("shapingBurstSize %d is out of range [0-65535].", shaper_p->shapingBurstSize);
            return TMCTL_ERROR;
         }
         intf_cfg.ulPortShaping = XTM_PORT_SHAPING_ON;
         intf_cfg.ulShapeRate = shaper_p->shapingRate * 1000;
         intf_cfg.usMbs = shaper_p->shapingBurstSize;
      }
      else
      {
         intf_cfg.ulPortShaping = XTM_PORT_SHAPING_OFF;
         intf_cfg.ulShapeRate = 0;
         intf_cfg.usMbs = 0;
      }
      if(BCMRET_SUCCESS != (BcmRet) devCtl_xtmSetInterfaceCfg(ulPortId, &intf_cfg))
      {
         tmctl_error("Failed to set xtm interface config.");
         return TMCTL_ERROR;
      }
   }
   else
   {
      tmctl_error("Failed to get xtm interface config.");
      return TMCTL_ERROR;
   }
#else
   ret = TMCTL_UNSUPPORTED;
#endif

   return ret;
   
}  /* End of tmctl_xtm_setPortShaper() */


/* ----------------------------------------------------------------------------
 * This function Remove the XTM SAR queue from the XTM driver.
 *
 * Parameters:
 *    ifname (IN) Linux interface name.
 *    qid (IN) Queue ID.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_remQueueCfg(const char* ifname, int qid)
{
   int i;
   BOOL qFound;
   int ulMaxQueues;
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_error("Failed to get connection address and config.");
      return TMCTL_ERROR;
   }

   if(xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM)
   {
      ulMaxQueues = MAX_ATM_TRANSMIT_QUEUES;
   }
   else
   {
      ulMaxQueues = MAX_PTM_TRANSMIT_QUEUES;
   }

   if((qid < XTM_MIN_QOS_QUEUE_IDX) || (qid >= ulMaxQueues))
   {
      tmctl_error("qid %d out of range [%d-%d].", qid, XTM_MIN_QOS_QUEUE_IDX, ulMaxQueues - 1);
      return TMCTL_ERROR;
   }

   qFound = FALSE;
   for (i = 0, pTxQ = conn_cfg.TransmitQParms; i < conn_cfg.ulTransmitQParmsSize; i++, pTxQ++)
   {
      if (pTxQ->ucQosQId == qid)
      {
         qFound = TRUE;
         break;
      }
   }

   if(!qFound)
   {
      /* Queue does not exist. Do not print error message. */
      return TMCTL_NOT_FOUND;
   }

   conn_cfg.ulTransmitQParmsSize--;
   memmove(pTxQ, pTxQ + 1,
           (conn_cfg.ulTransmitQParmsSize - i) *
           sizeof(XTM_TRANSMIT_QUEUE_PARMS));

   if (BCMRET_SUCCESS != (BcmRet) devCtl_xtmSetConnCfg(&xtm_addr,&conn_cfg))
   {
      tmctl_error("Failed to set the connection configuration");
      return TMCTL_ERROR;
   }

   return TMCTL_SUCCESS;
}


/* ----------------------------------------------------------------------------
 * This function gets the configuration of a xtm connection configuration.
 *
 * Parameters:
 *    ifname (IN)  Linux interface name.
 *    qid (IN)     Queue ID.
 *    qcfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getQueueCfg(const char* ifname, int qid, tmctl_queueCfg_t* qcfg_p)
{
   int i;
   BOOL qFound;
   int ulMaxQueues;
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;

   if((qid < XTM_MIN_QOS_QUEUE_IDX) || (qid > XTM_MAX_QOS_QUEUE_IDX))
   {
      tmctl_error("qid is out of range and invalid");
      return TMCTL_ERROR;
   }

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_error("Failed to get connection address and config.");
      return TMCTL_ERROR;
   }

   if(xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM)
   {
      ulMaxQueues = MAX_ATM_TRANSMIT_QUEUES;
   }
   else
   {
      ulMaxQueues = MAX_PTM_TRANSMIT_QUEUES;
   }

   if((qid < XTM_MIN_QOS_QUEUE_IDX) || (qid >= ulMaxQueues))
   {
      tmctl_error("qid %d out of range [%d-%d].", qid, XTM_MIN_QOS_QUEUE_IDX, ulMaxQueues - 1);
      return TMCTL_ERROR;
   }

   qFound = FALSE;
   for (i = 0, pTxQ = conn_cfg.TransmitQParms; i < conn_cfg.ulTransmitQParmsSize; i++, pTxQ++)
   {
      if (pTxQ->ucQosQId == qid)
      {
         qFound = TRUE;
         break;
      }
   }

   if(!qFound)
   {
      /* tmctl_cmds will getqcfg when setqcfg.
         Do not print error message when queue is not found. */
      return TMCTL_NOT_FOUND;
   }

   qcfg_p->qid = qid;
   qcfg_p->priority = pTxQ->ucSubPriority;
   qcfg_p->qsize = pTxQ->usSize;
   qcfg_p->weight = pTxQ->ulWeightValue;
   switch(pTxQ->ucWeightAlg)
   {
   case WA_DISABLED:
      qcfg_p->schedMode = TMCTL_SCHED_SP;
      break;
   case WA_WFQ:
      qcfg_p->schedMode = TMCTL_SCHED_WFQ;
      break;
   default:
      qcfg_p->schedMode = TMCTL_SCHED_WRR;
      break;
   }
   qcfg_p->shaper.shapingRate = pTxQ->ulShapingRate / 1000;
   qcfg_p->shaper.shapingBurstSize = pTxQ->usShapingBurstSize;
   qcfg_p->shaper.minRate = pTxQ->ulMinBitRate / 1000;
   qcfg_p->bestEffort = FALSE;
   /* For 158 minimum reserved buffer size is based on priority. */
   qcfg_p->minBufs = (pTxQ->ucSubPriority * 16);

   return TMCTL_SUCCESS;

}  /* End of tmctl_xtm_getQueueCfg() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of a xtm queue.
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
                                  tmctl_portTmParms_t* tmParms_p,
                                  tmctl_queueCfg_t* qcfg_p)
{
   int i;
   int qid;
   BOOL qFound;
   int ulMaxQueues;
   XTM_ADDR xtm_addr;
   XTM_CONN_CFG conn_cfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;

   tmctl_debug("Enter: ifname=%s qid=%d priority=%d schedMode=%d qsize=%d "
               "wt=%d minBufs=%d shapingRate=%d burstSize=%d minRate=%d bestEffort=%d",
               ifname, qcfg_p->qid, qcfg_p->priority, qcfg_p->schedMode,
               qcfg_p->qsize, qcfg_p->weight, qcfg_p->minBufs, qcfg_p->shaper.shapingRate,
               qcfg_p->shaper.shapingBurstSize, qcfg_p->shaper.minRate,
               qcfg_p->bestEffort ? 1 : 0);

   qid = qcfg_p->qid;

   memset(&xtm_addr, 0, sizeof(XTM_ADDR));
   memset(&conn_cfg, 0, sizeof(XTM_CONN_CFG));

   if(getConnAddrCfg(ifname, &xtm_addr, &conn_cfg) != TMCTL_SUCCESS)
   {
      tmctl_error("Failed to get connection address and config.");
      return TMCTL_ERROR;
   }

   if(xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM)
   {
      ulMaxQueues = MAX_ATM_TRANSMIT_QUEUES;
   }
   else
   {
      ulMaxQueues = MAX_PTM_TRANSMIT_QUEUES;
   }

   if((qid < XTM_MIN_QOS_QUEUE_IDX) || (qid >= ulMaxQueues))
   {
      tmctl_error("qid %d out of range [%d-%d].", qid, XTM_MIN_QOS_QUEUE_IDX, ulMaxQueues - 1);
      return TMCTL_ERROR;
   }

   tmctl_debug("TransmitQParmsSize:%d ulMaxQueues:%d ucQosQId:%d qid:%d",
               conn_cfg.ulTransmitQParmsSize, ulMaxQueues,
               conn_cfg.TransmitQParms[qid].ucQosQId, qid);

   qFound = FALSE;
   for (i = 0, pTxQ = conn_cfg.TransmitQParms; i < conn_cfg.ulTransmitQParmsSize; i++, pTxQ++)
   {
      if (pTxQ->ucQosQId == qid)
      {
         qFound = TRUE;
         break;
      }
   }

   if(!qFound)
   {
      /* add queue */
      if(conn_cfg.ulTransmitQParmsSize >= ulMaxQueues)
      {
         tmctl_error("Too many queues for this connection.");
         return TMCTL_ERROR;
      }
      /* pTxQ is already pointed to last unused TxQ. */
      memset(pTxQ, 0, sizeof(XTM_TRANSMIT_QUEUE_PARMS));
      conn_cfg.ulTransmitQParmsSize++;
      pTxQ->ucDropAlg = WA_DT;
      pTxQ->ucLoMinThresh = 0;
      pTxQ->ucLoMaxThresh = 0;
      pTxQ->ucHiMinThresh = 0;
      pTxQ->ucHiMaxThresh = 0;
   }

   /* add or edit queue */
   if (qcfg_p->schedMode == TMCTL_SCHED_WFQ)
   {
      pTxQ->ucWeightAlg = WA_WFQ;
   }
   else /* WRR or SP */
   {
      /* WRR in tmctl definition is packet weighted round robin. */
      pTxQ->ucWeightAlg = WA_PWRR;
   }
   pTxQ->usSize = qcfg_p->qsize;
   pTxQ->ucSubPriority = qcfg_p->priority;
   pTxQ->ucQosQId = qcfg_p->qid;
   pTxQ->ulWeightValue = qcfg_p->weight;
   pTxQ->ulMinBitRate = 1000 * qcfg_p->shaper.minRate;
   pTxQ->ulShapingRate = 1000 * qcfg_p->shaper.shapingRate;
   pTxQ->usShapingBurstSize = qcfg_p->shaper.shapingBurstSize;

   if( xtm_addr.ulTrafficType == TRAFFIC_TYPE_ATM )
   {
      pTxQ->ulPortId = xtm_addr.u.Vcc.ulPortMask;
   }
   else
   {
      pTxQ->ulPortId = xtm_addr.u.Flow.ulPortMask;
      /* If the flow-PtmPriority is either HIGH-only or Low-only,
       *    set the queue-PtmPriority to the flow-PtmPriority value.
       * If the flow-PtmPriority is HIGH-LOW, this must be the first queue.
       *    set the queue-PtmPriority to PTM_PRI_LOW.
       */
      if( xtm_addr.u.Flow.ulPtmPriority == PTM_PRI_HIGH )
      {
         /* The flow-PtmPriority is HIGH-only. */
         pTxQ->ulPtmPriority = PTM_PRI_HIGH;
      }
      else
      {
         /* The flow-PtmPriority is either HIGH-LOW or LOW-only. */
         pTxQ->ulPtmPriority = PTM_PRI_LOW;
      }
   }

   tmctl_debug("TransmitQParmsSize:%d ucQosQId:%d qid:%d "
               "usSize:%d, ucSubPriority:%d ucWeightAlg:%d "
               "ucDropAlg:%d ulPortId:%d ulPtmPriority:%d",
               conn_cfg.ulTransmitQParmsSize,
               pTxQ->ucQosQId, qid,
               pTxQ->usSize, pTxQ->ucSubPriority,
               pTxQ->ucWeightAlg, pTxQ->ucDropAlg,
               pTxQ->ulPortId, pTxQ->ulPtmPriority);

   if (BCMRET_SUCCESS != (BcmRet) devCtl_xtmSetConnCfg(&xtm_addr,&conn_cfg))
   {
      tmctl_error("Failed to set the connection configuration");
      return TMCTL_ERROR;
   }

   return TMCTL_SUCCESS;

}  /* End of tmctl_xtm_setQueueCfg() */


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
tmctl_ret_e tmctl_xtm_getQueueStats(int devType, tmctl_if_t* if_p, int q_id,
                                    tmctl_queueStats_t* queueStats_p)
{
   return TMCTL_UNSUPPORTED;
}

/* ----------------------------------------------------------------------------
 * This function gets the size of a queue.
 *
 * Parameters:
 *    size_p (OUT) uint32_t to return the queue size.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_getQueueSize(int32_t *psize)
{
   int rc;
   tmctl_ret_e ret = TMCTL_SUCCESS;
   XTM_THRESHOLD_PARMS xtm_threshold;
   memset(&xtm_threshold,0x0,sizeof(XTM_THRESHOLD_PARMS));
   xtm_threshold.sParams.gfastParam = XTM_THRESHOLD_PARM_GET;
   rc = devCtl_xtmManageThreshold(&xtm_threshold);
   if(rc != BCMRET_SUCCESS)
   {
      tmctl_error("Failed to get xtm thresholds");
      ret = TMCTL_ERROR;
   }
   else
   {
      *psize = (int32_t)xtm_threshold.gfastThreshold;
   }
   return ret;
}

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
tmctl_ret_e tmctl_xtm_setQueueSize(int size)
{
   int rc;
   tmctl_ret_e ret = TMCTL_SUCCESS;
   XTM_THRESHOLD_PARMS xtm_threshold;
   if((size < TMCTL_MIN_XTM_DPU_Q_SZ) || (size > TMCTL_MAX_XTM_DPU_Q_SZ))
      return TMCTL_ERROR;
   memset(&xtm_threshold,0x0,sizeof(XTM_THRESHOLD_PARMS));
   xtm_threshold.sParams.gfastParam = XTM_THRESHOLD_PARM_SET;
   xtm_threshold.gfastThreshold = (uint32_t)size;
   rc = devCtl_xtmManageThreshold(&xtm_threshold);
   if(rc != BCMRET_SUCCESS)
   {
      tmctl_error("Failed to set xtm thresholds");
      ret = TMCTL_ERROR;
   }
   return ret;
}

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
tmctl_ret_e tmctl_xtm_getMirror(tmctl_mirror_op_e op)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   int  socketFd;
   struct ifreq intf;
   MirrorCfg mirrorCfg;
   char xtmIfName[IFNAMSIZ] = {0};

   if(getConnIfname(xtmIfName) != TMCTL_SUCCESS)
   {
      tmctl_debug("Failed to get the connection interface name. Please confirm the DSL link is up.");
      return TMCTL_SUCCESS;
   }

   if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
   {
      tmctl_error("port mirroring: could not open socket");
      ret = TMCTL_ERROR;
   }
   else 
   {
      memset(&mirrorCfg, 0x0, sizeof(MirrorCfg));  
      strncpy(mirrorCfg.szMonitorInterface, xtmIfName, sizeof(mirrorCfg.szMonitorInterface)-1);
      mirrorCfg.szMonitorInterface[sizeof(mirrorCfg.szMonitorInterface)-1] = '\0';      
      mirrorCfg.nDirection = (op == TMCTL_MIRROR_RX_GET) ? MIRROR_DIR_IN : MIRROR_DIR_OUT;

      strncpy(intf.ifr_name, xtmIfName, sizeof(intf.ifr_name)-1);
      intf.ifr_name[sizeof(intf.ifr_name)-1] = '\0';      
      intf.ifr_data = (char*)&mirrorCfg;

      if (ioctl(socketFd, SIOCGPORTMIRROR, &intf) == -1) 
      {
         ret = TMCTL_ERROR;
         tmctl_error( "port mirroring: IOCTL to bcmxtmrt driver to get port mirroring cfg failed");
      }
      close(socketFd);

      if(mirrorCfg.nStatus == MIRROR_ENABLED)
      {
         printf("Interface %s mirrored to interface %s\n", mirrorCfg.szMonitorInterface, mirrorCfg.szMirrorInterface);
      }
   }

   return ret;
}

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
tmctl_ret_e tmctl_xtm_clrMirror(tmctl_mirror_op_e op)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   char xtmIfName[IFNAMSIZ] = {0};
   tmctl_mirror_op_e newOp;

   if(getConnIfname(xtmIfName) != TMCTL_SUCCESS)
   {
      tmctl_debug("Failed to get the connection interface name. Please confirm the DSL link is up.");
      return TMCTL_SUCCESS;
   }

   newOp = (op == TMCTL_MIRROR_RX_CLR) ? TMCTL_MIRROR_RX_DEL : TMCTL_MIRROR_TX_DEL;

   ret = tmctl_xtm_delMirror(xtmIfName, newOp);
   if (ret != TMCTL_SUCCESS)
   {
      tmctl_error("tmctl_xtm_delMirror failed! ret=%d", ret);
   }

   return ret;
}

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
tmctl_ret_e tmctl_xtm_addMirror(const char *srcIfName, const char *destIfName, tmctl_mirror_op_e op)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   int  socketFd;
   struct ifreq intf;
   MirrorCfg mirrorCfg;

   if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
   {
      tmctl_error("port mirroring: could not open socket");
      ret = TMCTL_ERROR;
   }
   else 
   {
      memset(&mirrorCfg, 0x0, sizeof(MirrorCfg));  
      strncpy(mirrorCfg.szMonitorInterface, srcIfName, sizeof(mirrorCfg.szMonitorInterface)-1);
      mirrorCfg.szMonitorInterface[sizeof(mirrorCfg.szMonitorInterface)-1] = '\0';      
      strncpy(mirrorCfg.szMirrorInterface, destIfName, sizeof(mirrorCfg.szMirrorInterface)-1);
      mirrorCfg.szMirrorInterface[sizeof(mirrorCfg.szMirrorInterface)-1] = '\0';      
      mirrorCfg.nDirection = (op == TMCTL_MIRROR_RX_ADD) ? MIRROR_DIR_IN : MIRROR_DIR_OUT;
      mirrorCfg.nStatus = MIRROR_ENABLED;

      strncpy(intf.ifr_name, srcIfName, sizeof(intf.ifr_name)-1);
      intf.ifr_name[sizeof(intf.ifr_name)-1] = '\0';      
      intf.ifr_data = (char*)&mirrorCfg;

      if (ioctl(socketFd, SIOCPORTMIRROR, &intf) == -1) 
      {
         ret = TMCTL_ERROR;
         tmctl_error( "port mirroring: IOCTL to bcmxtmrt driver to set port mirroring cfg failed");
      }
      close(socketFd);
   }

   return ret;
}

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
tmctl_ret_e tmctl_xtm_delMirror(const char *srcIfName, tmctl_mirror_op_e op)
{
   tmctl_ret_e ret = TMCTL_SUCCESS;
   int  socketFd;
   struct ifreq intf;
   MirrorCfg mirrorCfg;

   if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
   {
      tmctl_error("port mirroring: could not open socket");
      ret = TMCTL_ERROR;
   }
   else 
   {
      memset(&mirrorCfg, 0x0, sizeof(MirrorCfg));  
      strncpy(mirrorCfg.szMonitorInterface, srcIfName, sizeof(mirrorCfg.szMonitorInterface)-1);
      mirrorCfg.szMonitorInterface[sizeof(mirrorCfg.szMonitorInterface)-1] = '\0';      
      mirrorCfg.nDirection = (op == TMCTL_MIRROR_RX_DEL) ? MIRROR_DIR_IN : MIRROR_DIR_OUT;
      mirrorCfg.nStatus = MIRROR_DISABLED;

      strncpy(intf.ifr_name, srcIfName, sizeof(intf.ifr_name)-1);
      intf.ifr_name[sizeof(intf.ifr_name)-1] = '\0';      
      intf.ifr_data = (char*)&mirrorCfg;

      if (ioctl(socketFd, SIOCPORTMIRROR, &intf) == -1) 
      {
         ret = TMCTL_ERROR;
         tmctl_error( "port mirroring: IOCTL to bcmxtmrt driver to set port mirroring cfg failed");
      }
      close(socketFd);
   }

   return ret;
}

/* ----------------------------------------------------------------------------
 * This function initializes the XTM TM configuration 
 *
 * Parameters:
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_TmInit()
{
   return TMCTL_SUCCESS;
}

/* ----------------------------------------------------------------------------
 * This function uninitializes the XTM TM configuration 
 *
 * Parameters:
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_xtm_TmUnInit()
{
   return TMCTL_SUCCESS;
}
