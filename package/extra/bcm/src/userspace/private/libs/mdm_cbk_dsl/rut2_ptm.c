/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
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
#ifdef DMP_DEVICE2_BASELINE_1
/* this only applies to device 2, and also us some function in rut_atm.c */

#ifdef DMP_DEVICE2_PTMLINK_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "cms_boardcmds.h"
#include "cms_qos.h"
#include "rut_diag.h"
#include "rut_system.h"
#include "devctl_xtm.h"
#include "mdm_initdsl.h"

void rutUtil_modifyNumPtmLink(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2PtmObject *ptmObj = NULL;

   if (mdmShmCtx->inMdmInit)
   {
      /*
       * During system startup, we might have loaded from a config file.
       * The config file already contains the correct count of these objects.
       * So don't update the count in the parent object.
       */
      cmsLog_debug("don't update count in oid %d (delta=%d)",
                   MDMOID_DEV2_PTM, delta);
      return;
   }

   if ((cmsObj_get(MDMOID_DEV2_PTM, &iidStack, 0, (void *) &ptmObj)) == CMSRET_SUCCESS)
   {
      ptmObj->linkNumberOfEntries += delta;
      if ((cmsObj_set(ptmObj, &iidStack)) != CMSRET_SUCCESS)
          cmsLog_debug("cmsObj_set error!");
      cmsObj_free((void **) &ptmObj);
   }
}

CmsRet rutptm_fillL2IfName_dev2(const Layer2IfNameType ifNameType, char **ifName)
{
   CmsRet ret;
   SINT32 intfArray[IFC_WAN_MAX];
   SINT32 index = 0;
   char *prefix;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char L2IfName[CMS_IFNAME_LENGTH];
   Dev2PtmLinkObject *ptmLinkObj;
   Dev2PtmObject *ptmObj;

   memset((UINT8 *) &intfArray, 0, sizeof(intfArray));

   if ((ret = cmsObj_get(MDMOID_DEV2_PTM, &iidStack, 0, (void *) &ptmObj)) == CMSRET_SUCCESS)
   {
      if (ptmObj->linkNumberOfEntries >= IFC_WAN_MAX)
      {
         cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
         cmsObj_free((void **) &ptmObj);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      cmsLog_debug("This should never happen, Device.PTM object is not found");
      return CMSRET_INTERNAL_ERROR;
   }
   /* done with the obj. Free it now */
   cmsObj_free((void **) &ptmObj);

   switch (ifNameType)
   {
   case PTM_EOA:
      while ((ret = cmsObj_getNext(MDMOID_DEV2_PTM_LINK, &iidStack, (void **) &ptmLinkObj)) == CMSRET_SUCCESS)
      {
         if (ptmLinkObj->name == NULL)
         {
            /* this is one we just created and is NULL, so break */
            cmsObj_free((void **) &ptmLinkObj);
            break;
         }
         else
         {
            index = atoi(&(ptmLinkObj)->name[strlen(PTM_IFC_STR)]);
            if (index <= IFC_WAN_MAX)
            {
               cmsLog_debug("ptmLinkObj->name=%s, index=%d", ptmLinkObj->name, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
         }
         cmsObj_free((void **) &ptmLinkObj);
      }
      prefix = PTM_IFC_STR;
      break;

   default:
         cmsLog_error("Wrong type=%d", ifNameType);
         return CMSRET_INTERNAL_ERROR;
   }

   for (index = 0; index < IFC_WAN_MAX; index++)
   {
      cmsLog_debug("intfArray[%d]=%d", index, intfArray[index]);
      if (intfArray[index] == 0)
      {
         sprintf(L2IfName, "%s%d", prefix, index);
         ret = CMSRET_SUCCESS;
         break;
      }
   }

   CMSMEM_REPLACE_STRING_FLAGS(*ifName, L2IfName, mdmLibCtx.allocFlags);
   cmsLog_debug("Get Layer2 ifName=%s", *ifName);

   return ret;
}

CmsRet rutptm_fillLowerLayer(char **lowerLayer)
{
   Dev2DslChannelObject *dslChannelObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   UBOOL8 foundDsl = FALSE;
   UBOOL8 foundFast = FALSE;
   char *channelFullPathString=NULL;
#ifdef DMP_DEVICE2_BONDEDDSL_1
   Dev2DslBondingGroupObject *dslBondingGroupObj = NULL;
   char *fullPathString=NULL;
#endif
   char allLowerLayersStringBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};
#ifdef DMP_DEVICE2_FAST_1
   Dev2FastLineObject *fastLineObj=NULL;
   char *fastLineFullPathString=NULL;
#endif

   /* in our system, the second dsl line used for bonding is not configurable,
    * so, when a ptm link is created, it is always on the top of PTM channel that is stacked on the
    * primary line.
    */
   /* in a bonding board & image, then the ATM link is stacked on the 
    * top of the ATM bonding group.
    */
#ifdef DMP_DEVICE2_BONDEDDSL_1
   /* bonding group does not have to be operational at the time PTM is created for it to stack on top of this group.
    * the bonding status can come up later.   When down, SSK code knows to stack on top of the links of this group.
    */
   while (!foundDsl &&
          cmsObj_getNext(MDMOID_DEV2_DSL_BONDING_GROUP, &iidStack, (void **) &dslBondingGroupObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dslBondingGroupObj->name, BONDING_GROUP_PTM_NAME))
      {
         foundDsl = TRUE;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);
         sprintf(allLowerLayersStringBuf,"%s",fullPathString);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
         cmsLog_debug("allLowerLayersStringBuf=%s", allLowerLayersStringBuf);
      }
      cmsObj_free((void **) &dslBondingGroupObj);
   }

#ifdef DMP_DEVICE2_BONDEDFAST_1
   /* if fast is in, the lower layer of PTM also includes FAST bonding group */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!foundFast &&
          cmsObj_getNext(MDMOID_DEV2_DSL_BONDING_GROUP, &iidStack, (void **) &dslBondingGroupObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dslBondingGroupObj->name, BONDING_GROUP_FAST_NAME))
      {
         foundFast = TRUE;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);
         cmsUtl_addFullPathToCSL(fullPathString,allLowerLayersStringBuf,sizeof(allLowerLayersStringBuf));
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
         cmsLog_debug("allLowerLayersStringBuf=%s", allLowerLayersStringBuf);
      }
      cmsObj_free((void **) &dslBondingGroupObj);
   }
#endif

#endif /*DMP_DEVICE2_BONDEDDSL_1*/

   /* If any bonding group was found, it is done.
    * Lower layers of ptm link will not have BondingGroup mixed with
    * DSL.Channel or FAST.Line paths.
    */
   if (foundDsl || foundFast)
   {
      goto out;
   }

   /* Find the first dsl channel. If the 2nd channel exists, it will be in the bonding group */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!foundDsl &&
          cmsObj_getNext(MDMOID_DEV2_DSL_CHANNEL, &iidStack, (void **) &dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dslChannelObj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))
      {
         foundDsl = TRUE;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_DSL_CHANNEL;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &channelFullPathString);
         cmsUtl_addFullPathToCSL(channelFullPathString,allLowerLayersStringBuf,sizeof(allLowerLayersStringBuf));
         CMSMEM_FREE_BUF_AND_NULL_PTR(channelFullPathString);
         cmsLog_debug("allLowerLayersStringBuf=%s", allLowerLayersStringBuf);

#ifdef DMP_DEVICE2_FAST_1
         /* Find the first fast line. If the 2nd fast line exists, it will be in the bonding group */
         INIT_INSTANCE_ID_STACK(&iidStack);
         if (cmsObj_getNext(MDMOID_DEV2_FAST_LINE, &iidStack, (void **) &fastLineObj) == CMSRET_SUCCESS)
         {
            INIT_PATH_DESCRIPTOR(&pathDesc);
            pathDesc.oid = MDMOID_DEV2_FAST_LINE;
            pathDesc.iidStack = iidStack;
            cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fastLineFullPathString);
            cmsUtl_addFullPathToCSL(fastLineFullPathString,allLowerLayersStringBuf,sizeof(allLowerLayersStringBuf));
            CMSMEM_FREE_BUF_AND_NULL_PTR(fastLineFullPathString);
            cmsObj_free((void **) &fastLineObj);
            cmsLog_debug("allLowerLayersStringBuf=%s", allLowerLayersStringBuf);
         }
#endif /* DMP_DEVICE2_FAST_1 */

      }
      cmsObj_free((void **) &dslChannelObj);
   }

out:
   if (!IS_EMPTY_STRING(allLowerLayersStringBuf))
   {
      cmsLog_debug("allLowerLayersStringBuf=%s", allLowerLayersStringBuf);
      CMSMEM_REPLACE_STRING_FLAGS(*lowerLayer, allLowerLayersStringBuf, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }

   return CMSRET_INTERNAL_ERROR;
}

CmsRet rutptm_setConnCfg_dev2(const _Dev2PtmLinkObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;

   cmsLog_debug("Enter");
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   memset((UINT8 *) &ConnCfg, 0x00, sizeof(ConnCfg));

   Addr.ulTrafficType     = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_PTMPortId);

   if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE &&
       newObj->X_BROADCOM_COM_PTMPriorityLow  == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH | PTM_PRI_LOW;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityLow == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_LOW;
   }
   else
   {
      cmsLog_error("PTM priority is neither LOW nor HIGH.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   ConnCfg.ulHeaderType         = HT_LLC_SNAP_ETHERNET;
   ConnCfg.ulAdminStatus        = ADMSTS_UP;
   ConnCfg.ulTransmitQParmsSize = 1;
   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_GrpScheduler, MDMVS_WRR))
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_CWRR;
   }
   else
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_DISABLED;
   }
   ConnCfg.ConnArbs[0][0].ulWeightValue = newObj->X_BROADCOM_COM_GrpWeight;
   ConnCfg.ConnArbs[0][0].ulSubPriority = XTM_QOS_LEVELS - newObj->X_BROADCOM_COM_GrpPrecedence;

   cmsLog_debug("ConnCfg.ulTransmitQParmsSize=%d, Addr.u.Flow.ulPtmPriority=%d", ConnCfg.ulTransmitQParmsSize, Addr.u.Flow.ulPtmPriority);

   cmsUtl_strncpy(ConnCfg.netDeviceName, newObj->name, sizeof(ConnCfg.netDeviceName));

   /* Configure the default queue associated with this connection */
   pTxQ = &ConnCfg.TransmitQParms[0];
   pTxQ->usSize = HOST_XTM_NR_TXBDS;

   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_SchedulerAlgorithm, MDMVS_WFQ))
   {
      pTxQ->ucWeightAlg = WA_WFQ;
   }
   else  /* WRR or SP */
   {
      pTxQ->ucWeightAlg = WA_CWRR;
   }
   pTxQ->ulWeightValue = newObj->X_BROADCOM_COM_QueueWeight;   //ConnCfg.ConnArbs[0][0].ulWeightValue;
   pTxQ->ucSubPriority = XTM_QOS_LEVELS - newObj->X_BROADCOM_COM_QueuePrecedence;
   pTxQ->ucQosQId      = 0;   /* qid of the default queue is 0 */

   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_DropAlgorithm, MDMVS_RED))
   {
      pTxQ->ucDropAlg = WA_RED;
   }
   else if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_DropAlgorithm, MDMVS_WRED))
   {
      pTxQ->ucDropAlg = WA_WRED;
   }
   else
   {
      pTxQ->ucDropAlg = WA_DT;
   }
   pTxQ->ucLoMinThresh = newObj->X_BROADCOM_COM_LowClassMinThreshold;
   pTxQ->ucLoMaxThresh = newObj->X_BROADCOM_COM_LowClassMaxThreshold;
   pTxQ->ucHiMinThresh = newObj->X_BROADCOM_COM_HighClassMinThreshold;
   pTxQ->ucHiMaxThresh = newObj->X_BROADCOM_COM_HighClassMaxThreshold;

   if (newObj->X_BROADCOM_COM_QueueMinimumRate > QOS_QUEUE_NO_SHAPING)
   {
      pTxQ->ulMinBitRate = newObj->X_BROADCOM_COM_QueueMinimumRate;
   }
   else
   {
      pTxQ->ulMinBitRate = 0;  /* no shaping */
   }
   if (newObj->X_BROADCOM_COM_QueueShapingRate > QOS_QUEUE_NO_SHAPING)
   {
      pTxQ->ulShapingRate = newObj->X_BROADCOM_COM_QueueShapingRate;
   }
   else
   {
      pTxQ->ulShapingRate = 0;  /* no shaping */
   }
   pTxQ->usShapingBurstSize = newObj->X_BROADCOM_COM_QueueShapingBurstSize;

   if (Addr.u.Flow.ulPortMask == (PORT_PHY0_PATH0 | PORT_PHY0_PATH1))
   {
      pTxQ->ulPortId = PORT_PHY0_PATH0;
   }
   else
   {
      pTxQ->ulPortId = Addr.u.Flow.ulPortMask;
   }

   /* If the flow-PtmPriority is either HIGH-only or Low-only,
    *    set the queue-PtmPriority to the flow-PtmPriority value.
    * If the flow-PtmPriority is HIGH-LOW, this must be the first queue.
    *    set the queue-PtmPriority to PTM_PRI_LOW.
    */ 
   if( Addr.u.Flow.ulPtmPriority == PTM_PRI_HIGH )
   {
      /* The flow-PtmPriority is HIGH-only. */
      pTxQ->ulPtmPriority = PTM_PRI_HIGH; /* set ptm priority of the queue */
   }
   else
   {
      /* The flow-PtmPriority is either HIGH-LOW or LOW-only. */
      pTxQ->ulPtmPriority = PTM_PRI_LOW;  /* set ptm priority of the queue */
   }

   if ((ret = devCtl_xtmSetConnCfg( &Addr, &ConnCfg )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;
}


CmsRet rutptm_createInterface_dev2(const _Dev2PtmLinkObject *newObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   UINT32 priorityLow = newObj->X_BROADCOM_COM_PTMPriorityLow;
   UINT32 priorityHigh = 0;

   /* form the ptm interface name */
   cmsLog_debug("Create PTM interface %s", newObj->name);
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));

   Addr.ulTrafficType = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_PTMPortId);
   /* 
    * ulPtmPriority is a mask for both priorityLow and priorityHigh, so just OR them
    * and X_BROADCOM_COM_PTMPriorityLow is either 0 or 1 here so no need to adjust but 
    * X_BROADCOM_COM_PTMPriorityHigh need to be adjusted.
    */
   if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      /* since PTM_PRI_HIGH  is defined as  0x02 */
      priorityHigh = PTM_PRI_HIGH; 
   }
   Addr.u.Flow.ulPtmPriority = priorityHigh | priorityLow;

   if ((ret = devCtl_xtmCreateNetworkDevice(&Addr, newObj->name)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to start ptm interface %s. error=%d", newObj->name, ret);
      return ret;
   }

   cmsLog_debug("devCtl_xtmCreateNetworkDevice ret=%d", ret);

   return ret;
}


CmsRet rutptm_deleteInterface_dev2(const _Dev2PtmLinkObject *currObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   UINT32 priorityLow = currObj->X_BROADCOM_COM_PTMPriorityLow;
   UINT32 priorityHigh = 0;

   cmsLog_notice("Delete PTM interface %s", currObj->name);

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   Addr.ulTrafficType = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(currObj->X_BROADCOM_COM_PTMPortId);
   /* 
    * ulPtmPriority is a mask for both priorityLow and priorityHigh, so just OR them
    * and X_BROADCOM_COM_PTMPriorityLow is either 0 or 1 here so no need to adjust but 
    * X_BROADCOM_COM_PTMPriorityHigh need to be adjusted.
    */
   if (currObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      /* since PTM_PRI_HIGH  is defined as  0x02 */
      priorityHigh = PTM_PRI_HIGH; 
   }
   Addr.u.Flow.ulPtmPriority = priorityHigh | priorityLow;

   if ((ret = devCtl_xtmDeleteNetworkDevice(&Addr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to delete ptm interface %s. error=%d", currObj->name, ret);
   }

   return ret;
}

CmsRet rutptm_deleteConnCfg_dev2(const _Dev2PtmLinkObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;

   cmsLog_debug("Enter");
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   memset((UINT8 *) &ConnCfg, 0x00, sizeof(ConnCfg));

   Addr.ulTrafficType     = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_PTMPortId);

   if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE &&
       newObj->X_BROADCOM_COM_PTMPriorityLow  == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH | PTM_PRI_LOW;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityLow == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_LOW;
   }
   else
   {
      cmsLog_error("PTM priority is neither LOW nor HIGH.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((ret = devCtl_xtmSetConnCfg( &Addr, NULL )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;
}

void rutptm_getLinkStats_dev2(Dev2PtmLinkStatsObject *stats,
                              const char *linkName, const char *linkLowerLayers,
                              UBOOL8 reset)
{
   cmsLog_debug("Enter: linkName=%s linkLowerLayers=%s reset=%d",
                linkName, linkLowerLayers, reset);

   if (stats)
   {
      stats->errorsReceived = 0;
      stats->errorsSent = 0;
      stats->discardPacketsReceived = 0;
      stats->discardPacketsSent = 0;
      stats->unknownProtoPacketsReceived = 0;
      stats->bytesReceived = 0;
      stats->bytesSent = 0;
      stats->packetsReceived = 0;
      stats->packetsSent = 0;
      stats->discardPacketsReceived = 0;
      stats->X_BROADCOM_COM_InOAMCells = 0;
      stats->X_BROADCOM_COM_OutOAMCells = 0;
      stats->X_BROADCOM_COM_InASMCells = 0;
      stats->X_BROADCOM_COM_OutASMCells = 0;
      stats->X_BROADCOM_COM_InCellErrors = 0;
   }

   if (reset)
   {
      CmsRet ret=CMSRET_SUCCESS;
      MdmPathDescriptor pathDesc;
      Dev2DslBondingGroupObject *bondingGroupObj=NULL;
      Dev2DslChannelObject *channelObj=NULL;
      char bondingGroupList[BUFLEN_1024+1]={0};
      char channelList[BUFLEN_1024+1]={0};
      char lineList[BUFLEN_1024+1]={0};
      char *pStr=NULL;
      char *bondingGroupPath=NULL;
      char *channelPath=NULL;
      char *linePath=NULL;
      char *pLast=NULL;
      XTM_BOND_INFO bondInfo;
      UBOOL8 trainBonded=TRUE;
      UINT32 interfaceId=0;
      UINT32 portNumber=0;

      /* Check if this is DSL Bonding. */
      memset((UINT8 *)&bondInfo, 0, sizeof(XTM_BOND_INFO));
      ret = devCtl_xtmGetBondingInfo(&bondInfo);
      if (ret == CMSRET_SUCCESS || ret == CMSRET_METHOD_NOT_SUPPORTED)
      {
         if (bondInfo.ulNumGroups == 0)
         {
            /* Not DSL Bonding. Get the interfaceId from bondInfo. */
            trainBonded = FALSE;
            interfaceId = bondInfo.grpInfo[0].portInfo[0].ulInterfaceId;
            cmsLog_debug("interfaceId=%u", interfaceId);
         }
      }
      else
      {
         cmsLog_error("devCtl_xtmGetBondingInfo failed. ret=%d", ret);
         return;
      }

      /* ptm link could be bonding. We want to clear the statistics of
       * each of its lower layer lines.
       */

      /* lowerlayers of ptm link object could be one of the path lists below:
       * (a) one or two DSL.Channel paths.
       * (b) one DSL.Channel path and/or one FAST.Line path.
       * (c) one DSL Bonding group and/or one GFAST Bonding group
       */
      channelList[0] = '\0';

      cmsLog_debug("linkLowerLayers=%s", linkLowerLayers);

      if (cmsUtl_strstr(linkLowerLayers, "DSL.Channel"))
      {
         /* linkLowerLayers can contain DSL.Channel and FAST.Line paths. */
         cmsUtl_strncpy(channelList, linkLowerLayers, sizeof(channelList));
      }
      else if (cmsUtl_strstr(linkLowerLayers, "FAST.Line"))
      {
         /* Since "DSL.Channel" was checked before "FAST.Line",
          * linkLowerLayers should contain only FAST.Line paths.
          */
         cmsUtl_strncpy(lineList, linkLowerLayers, sizeof(lineList));
      }
      else if (cmsUtl_strstr(linkLowerLayers, "DSL.BondingGroup"))
      {
         cmsUtl_strncpy(bondingGroupList, linkLowerLayers, sizeof(bondingGroupList));

         pStr = bondingGroupList;
         pLast = NULL;
         while ((bondingGroupPath = strtok_r(pStr, ",", &pLast)))
         {
            pStr = NULL;   /* set pStr to NULL to get the next token */

            INIT_PATH_DESCRIPTOR(&pathDesc);
            ret = cmsMdm_fullPathToPathDescriptor(bondingGroupPath, &pathDesc);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                            bondingGroupPath, ret);
            }
            else
            {
               ret = cmsObj_get(MDMOID_DEV2_DSL_BONDING_GROUP, &pathDesc.iidStack,
                                OGF_NO_VALUE_UPDATE, (void **)&bondingGroupObj);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsObj_get BONDING GROUP object failed for %s, ret=%d",
                               bondingGroupPath, ret);
               }
               else
               {
                  if (cmsUtl_strcmp(bondingGroupObj->status, MDMVS_UP) == 0 &&
                      !IS_EMPTY_STRING(bondingGroupObj->lowerLayers))
                  {
                     /* lowerlayers of BondingGroup can be either "DSL.Channel"
                      * or "FAST.Line"
                      */
                     cmsLog_debug("bondingGroupLowerLayers=%s", bondingGroupObj->lowerLayers);
                     if (cmsUtl_strstr(bondingGroupObj->lowerLayers, "DSL.Channel"))
                     {
                        /* add to channelList */
                        if (strlen(channelList))
                           cmsUtl_strcat(channelList, ",");
                        cmsUtl_strcat(channelList, bondingGroupObj->lowerLayers);
                     }
                     else if (cmsUtl_strstr(bondingGroupObj->lowerLayers, "FAST.Line"))
                     {
                        /* add to lineList */
                        if (strlen(lineList))
                           cmsUtl_strcat(lineList, ",");
                        cmsUtl_strcat(lineList, bondingGroupObj->lowerLayers);
                     }
                  }
                  cmsObj_free((void **) &bondingGroupObj);
               }
            }
         }  /* while */
      }

      if (strlen(channelList))
      {
         /* Note that channelList may contain DSL.Channel or FAST.Line paths */
         /* find line paths in the channel list */
         cmsLog_debug("channelList=%s", channelList);
         pStr = channelList;
         pLast = NULL;
         while ((channelPath = strtok_r(pStr, ",", &pLast)))
         {
            pStr = NULL;   /* set pStr to NULL to get the next token */

            cmsLog_debug("channelPath=%s", channelPath);
            INIT_PATH_DESCRIPTOR(&pathDesc);
            ret = cmsMdm_fullPathToPathDescriptor(channelPath, &pathDesc);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                            channelPath, ret);
            }
            else
            {
               if (cmsUtl_strstr(channelPath, "DSL.Channel"))
               {
                  /* find line path of this channel */
                  ret = cmsObj_get(MDMOID_DEV2_DSL_CHANNEL, &pathDesc.iidStack,
                                   OGF_NO_VALUE_UPDATE, (void **)&channelObj);
                  if (ret != CMSRET_SUCCESS)
                  {
                     cmsLog_error("cmsObj_get CHANNEL object failed for %s, ret=%d",
                                  channelPath, ret);
                  }
                  else
                  {
                     if (cmsUtl_strcmp(channelObj->status, MDMVS_UP) == 0 &&
                         !IS_EMPTY_STRING(channelObj->lowerLayers))
                     {
                        /* add to lineList */
                        if (strlen(lineList))
                           cmsUtl_strcat(lineList, ",");
                        cmsUtl_strcat(lineList, channelObj->lowerLayers);
                     }
                     cmsObj_free((void **) &channelObj);
                  }
               }
               else if (cmsUtl_strstr(channelPath, "FAST.Line"))
               {
                  /* add to lineList */
                  if (strlen(lineList))
                     cmsUtl_strcat(lineList, ",");
                  cmsUtl_strcat(lineList, channelPath);
               }
            }
         }  /* while */
      }

      if (strlen(lineList))
      {
         Dev2DslLineObject *dslLineObj=NULL;
         Dev2FastLineObject *fastLineObj=NULL;
         XTM_INTERFACE_STATS intfStats;

         /* Note that lineList may contain DSL.Line or FAST.Line paths */
         /* get the stats of each line in the line list */
         cmsLog_debug("lineList=%s", lineList);
         pStr = lineList;
         pLast = NULL;
         while ((linePath = strtok_r(pStr, ",", &pLast)))
         {
            pStr = NULL;   /* set pStr to NULL to get the next token */

            cmsLog_debug("linePath=%s", linePath);
            INIT_PATH_DESCRIPTOR(&pathDesc);
            ret = cmsMdm_fullPathToPathDescriptor(linePath, &pathDesc);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                            linePath, ret);
            }
            else if (cmsUtl_strstr(linePath, "DSL.Line"))
            {
               ret = cmsObj_get(MDMOID_DEV2_DSL_LINE, &pathDesc.iidStack,
                                OGF_NO_VALUE_UPDATE, (void **)&dslLineObj);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsObj_get DSL LINE object failed for %s, ret=%d",
                               linePath, ret);
               }
               else
               {
                  if (cmsUtl_strcmp(dslLineObj->status, MDMVS_UP) == 0)
                  {
                     /* If this is DSL bonding, portNumber is BondingLineNumber. */
                     if (trainBonded)
                        portNumber = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
                     else
                        portNumber = interfaceId;

                     /* Now, reset the port stats. */
                     devCtl_xtmGetInterfaceStatistics((1 << portNumber), &intfStats, TRUE);
                  }
                  cmsObj_free((void **) &dslLineObj);
               }
            }
            else if (cmsUtl_strstr(linePath, "FAST.Line"))
            {
               ret = cmsObj_get(MDMOID_DEV2_FAST_LINE, &pathDesc.iidStack,
                                OGF_NO_VALUE_UPDATE, (void **)&fastLineObj);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsObj_get FAST LINE object failed for %s, ret=%d",
                               linePath, ret);
               }
               else
               {
                  if (cmsUtl_strcmp(fastLineObj->status, MDMVS_UP) == 0)
                  {
                     /* If this is DSL bonding, portNumber is BondingLineNumber. */
                     if (trainBonded)
                        portNumber = fastLineObj->X_BROADCOM_COM_BondingLineNumber;
                     else
                        portNumber = interfaceId;

                     /* Now, reset the port stats. */
                     devCtl_xtmGetInterfaceStatistics((1 << portNumber), &intfStats, TRUE);
                  }
                  cmsObj_free((void **) &fastLineObj);
               }
            }
         }  /* while */
      }

      /* clear the network interface stats */
      rut_clearIntfStats(linkName);
   }
   else
   {
      UINT64 dontCare;
      UINT64 errorsRx, errorsTx;
      UINT64 discardPacketsRx, discardPacketsTx;

      /* make sure that stats isn't NULL */
      if (!stats)
      {
         cmsLog_error("stats is NULL");
         return;
      }

      /* get the network interface stats */
      rut_getIntfStats_uint64(linkName,
                              &stats->bytesReceived, &stats->packetsReceived,
                              &dontCare/*byteMultiRx*/, &stats->multicastPacketsReceived,
                              &stats->unicastPacketsReceived, &stats->broadcastPacketsReceived,
                              &errorsRx, &discardPacketsRx,
                              &stats->bytesSent, &stats->packetsSent,
                              &dontCare/*byteMultiTx*/, &stats->multicastPacketsSent,
                              &stats->unicastPacketsSent, &stats->broadcastPacketsSent,
                              &errorsTx, &discardPacketsTx);
      stats->errorsReceived = (UINT32)errorsRx;
      stats->errorsSent = (UINT32)errorsTx;
      stats->discardPacketsReceived = (UINT32)discardPacketsRx;
      stats->discardPacketsSent = (UINT32)discardPacketsTx;
      stats->unknownProtoPacketsReceived = stats->discardPacketsReceived;

      cmsLog_debug("Rx: bytes=%llu packets=%llu multicast=%llu unitcast=%llu broadcast=%llu errors=%llu discard=%llu",
                   stats->bytesReceived, stats->packetsReceived, stats->multicastPacketsReceived, stats->unicastPacketsReceived,
                   stats->broadcastPacketsReceived, errorsRx, discardPacketsRx);
      cmsLog_debug("Tx: bytes=%llu packets=%llu muticast=%llu unitcast=%llu broadcast=%llu errors=%llu discard=%llu",
                   stats->bytesSent, stats->packetsSent, stats->multicastPacketsSent, stats->unicastPacketsSent,
                   stats->broadcastPacketsSent, errorsTx, discardPacketsTx);
   }
}

#endif   /* DMP_DEVICE2_PTMLINK_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
