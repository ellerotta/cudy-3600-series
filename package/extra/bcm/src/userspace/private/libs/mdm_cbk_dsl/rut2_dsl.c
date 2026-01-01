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

#ifdef DMP_DEVICE2_DSL_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "genutil_hexbinary.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "rut_wan.h"

#include "adslctlapi.h"
#include "AdslMibDef.h"
#include "DslCommonDef.h"
#include "bcmxdsl.h"
#include "devctl_xtm.h"
#include "rut_dsl.h"
#include "rut_system.h"
#include "rut_wanlayer2.h"
#include "devctl_adsl.h"
#include "adsl_api_trace.h"
#include "qdm_intf.h"
#include "rut2_fast.h"
#include "bcmadsl.h"
#include "DiagDef.h"

extern int GetXmtRate(adslMibInfo *pMib, int pathId);
extern int GetRcvRate(adslMibInfo *pMib, int pathId);
extern int f2DecI(int val, int q);
extern int GetAdsl2Sq(adsl2DataConnectionInfo *p2, int q);
extern int f2DecF(int val, int q);

void rutUtil_modifyNumDslNumOfEntries(MdmObjectId oid, SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DslObject *dslObj = NULL;

   if (mdmShmCtx->inMdmInit)
   {
      /*
       * During system startup, we might have loaded from a config file.
       * The config file already contains the correct count of these objects.
       * So don't update the count in the parent object.
       */
      cmsLog_debug("don't update count for new object oid %d (delta=%d)",
                   oid, delta);
      return;
   }

   if ((cmsObj_get(MDMOID_DEV2_DSL, &iidStack, 0, (void *) &dslObj)) == CMSRET_SUCCESS)
   {
      if (oid == MDMOID_DEV2_DSL_LINE)
      {
         dslObj->lineNumberOfEntries += delta;
      }
      else if (oid == MDMOID_DEV2_DSL_CHANNEL)
      {
         dslObj->channelNumberOfEntries += delta;
      }
#ifdef DMP_DEVICE2_BONDEDDSL_1
      else if (oid == MDMOID_DEV2_DSL_BONDING_GROUP)
      {
         dslObj->bondingGroupNumberOfEntries += delta;
      }
#endif
      if ((cmsObj_set(dslObj, &iidStack)) != CMSRET_SUCCESS)
          cmsLog_debug("cmsObj_set failure!");
      cmsObj_free((void **) &dslObj);
   }
}

CmsRet rutdsl_getLineIdByLineIidStack_dev2(const InstanceIdStack *iidStack, UINT32 *lineId)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   Dev2DslLineObject *dslLineObj = NULL;

   if ((ret=cmsObj_get(MDMOID_DEV2_DSL_LINE, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslLineObj)) == CMSRET_SUCCESS)
   {
      *lineId = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
      cmsObj_free((void **)&dslLineObj);
   }
   else
   {
      cmsLog_error("Fail to get cmsObj_get(MDMOID_DEV2_DSL_LINE). ret=%d", ret);
      *lineId = 0;
   }

   return ret;
}

CmsRet rutdsl_getAdslMibByLineIidStack_dev2(const InstanceIdStack *iidStack, adslMibInfo *adslMib, UINT32 *line)
{
   long size = sizeof(adslMibInfo);
   Dev2DslLineObject *dslLineObj;
   UINT32 lineId = 0;

   if (cmsObj_get(MDMOID_DEV2_DSL_LINE, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslLineObj) == CMSRET_SUCCESS)
   {
      lineId = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
      cmsObj_free((void **)&dslLineObj);
   }

   if (line)
      *line = lineId;

   return ((CmsRet) xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)adslMib, &size));
}

CmsRet rutdsl_getLineIdByChannelIidStack_dev2(const InstanceIdStack *iidStack, UINT32 *lineId)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   MdmPathDescriptor pathDesc;
   Dev2DslChannelObject *dslChannelObj = NULL;
   Dev2DslLineObject *dslLineObj = NULL;

   *lineId = 0;
   if ((ret=cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,OGF_NO_VALUE_UPDATE,(void **)&dslChannelObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get cmsObj_get(MDMOID_DEV2_DSL_CHANNEL). ret=%d", ret);
      goto out;
   }
   INIT_PATH_DESCRIPTOR(&pathDesc);
   if ((ret=cmsMdm_fullPathToPathDescriptor(dslChannelObj->lowerLayers, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", dslChannelObj->lowerLayers, ret);
      goto release_channel;
   }
   if ((ret = cmsObj_get(MDMOID_DEV2_DSL_LINE,&pathDesc.iidStack,OGF_NO_VALUE_UPDATE,(void **)&dslLineObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get line object, ret=%d", ret);
      goto release_channel;
   }
   *lineId = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
   cmsObj_free((void **) &dslLineObj);

release_channel:
   cmsObj_free((void **) &dslChannelObj);
out:
   return ret;
}

CmsRet rutdsl_getAdslMibByChannelIidStack_dev2(const InstanceIdStack *iidStack, adslMibInfo *adslMib, UINT32 *line)
{
   long size = sizeof(adslMibInfo);
   UINT32 lineId = 0;

   rutdsl_getLineIdByChannelIidStack_dev2(iidStack, &lineId);
   if (line)
      *line = lineId;

   return ((CmsRet) xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)adslMib, &size));
}


void xdslUtil_CfgProfileInit_dev2(adslCfgProfile * pAdslCfg,  Dev2DslLineObject *pDslLineObj)
{
    long dslCfgParam = pDslLineObj->X_BROADCOM_COM_DslCfgParam;

    pAdslCfg->adslHsModeSwitchTime = pDslLineObj->X_BROADCOM_COM_DslHsModeSwitchTime;
    pAdslCfg->adslLOMTimeThldSec = pDslLineObj->X_BROADCOM_COM_DslLOMTimeThldSec;
    pAdslCfg->adslPwmSyncClockFreq = pDslLineObj->X_BROADCOM_COM_DslPwmSyncClockFreq;
    pAdslCfg->adslShowtimeMarginQ4 = pDslLineObj->X_BROADCOM_COM_DslShowtimeMarginQ4;
    pAdslCfg->adslTrainingMarginQ4 = pDslLineObj->X_BROADCOM_COM_DslTrainingMarginQ4;
    pAdslCfg->adslDemodCapMask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Mask;
    pAdslCfg->adslDemodCapValue = pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Value;
    pAdslCfg->adslDemodCap2Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Mask;
    pAdslCfg->adslDemodCap2Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Value;
    pAdslCfg->adsl2Param = pDslLineObj->X_BROADCOM_COM_DslParam;
#ifdef SUPPORT_CFG_PROFILE
    pAdslCfg->xdslAuxFeaturesMask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Mask;
    pAdslCfg->xdslAuxFeaturesValue = pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Value;
    pAdslCfg->vdslCfgFlagsMask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Mask;
    pAdslCfg->vdslCfgFlagsValue = pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Value;
    pAdslCfg->xdslCfg1Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Mask;
    pAdslCfg->xdslCfg1Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Value;
    pAdslCfg->xdslCfg2Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Mask;
    pAdslCfg->xdslCfg2Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Value;
    pAdslCfg->xdslCfg3Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Mask;
    pAdslCfg->xdslCfg3Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Value;
    pAdslCfg->xdslCfg4Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Mask;
    pAdslCfg->xdslCfg4Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Value;
#endif
    pAdslCfg->maxUsDataRateKbps = pDslLineObj->X_BROADCOM_COM_DslPhyUsDataRateKbps;
    pAdslCfg->maxDsDataRateKbps = pDslLineObj->X_BROADCOM_COM_DslPhyDsDataRateKbps;
    pAdslCfg->maxAggrDataRateKbps = pDslLineObj->X_BROADCOM_COM_DslPhyAggrDataRateKbps;
    pAdslCfg->xdslMiscCfgParam = pDslLineObj->X_BROADCOM_COM_DslPhyMiscCfgParam;

    cmsLog_debug("AdslModulationCfg=%s\n", pDslLineObj->X_BROADCOM_COM_AdslModulationCfg);

    /* Modulation type */
    dslCfgParam &= ~kAdslCfgModMask;
    if(cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg, MDMVS_ADSL_MODULATION_ALL)) {
        /* Note: MDMVS_ADSL_MODULATION_ALL does not include AnnexM */
        dslCfgParam |= kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly | kAdslCfgModAdsl2Only | kAdslCfgModAdsl2pOnly | kAdslCfgModT1413Only;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        dslCfgParam |= kDslCfgModVdsl2Only;
#ifdef SUPPORT_DSL_GFAST
        dslCfgParam |= kDslCfgModGfastOnly;
#endif
#ifdef CONFIG_MGFAST_SUPPORT
        // dslCfgParam |= kDslCfgModMgfastOnly;   /* TO DO: Uncomment this when MG.fast is officially supported */
#endif
#endif
        pAdslCfg->adsl2Param |= kAdsl2CfgReachExOn;
    }
    else {
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_DMT))
            dslCfgParam |= kAdslCfgModGdmtOnly;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_LITE))
            dslCfgParam |= kAdslCfgModGliteOnly;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_DMT_BIS))
            dslCfgParam |= kAdslCfgModAdsl2Only;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_RE_ADSL))
            pAdslCfg->adsl2Param |= kAdsl2CfgReachExOn;
        else
            pAdslCfg->adsl2Param &= ~kAdsl2CfgReachExOn;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_2PLUS))
            dslCfgParam |= kAdslCfgModAdsl2pOnly;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_ANSI_T1_413))
            dslCfgParam |= kAdslCfgModT1413Only;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_VDSL2))
            dslCfgParam |= kDslCfgModVdsl2Only;
#ifdef SUPPORT_DSL_GFAST
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_G_FAST))
            dslCfgParam |= kDslCfgModGfastOnly;
#endif
#ifdef CONFIG_MGFAST_SUPPORT
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_MG_FAST))
            dslCfgParam |= kDslCfgModMgfastOnly;
#endif
#endif
    }

    if ( (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg, MDMVS_ANNEXM)) ||
        (pDslLineObj->X_BROADCOM_COM_ADSL2_AnnexM == TRUE) )
        pAdslCfg->adsl2Param |= kAdsl2CfgAnnexMEnabled;
    else
        pAdslCfg->adsl2Param &= ~(kAdsl2CfgAnnexMEnabled | kAdsl2CfgAnnexMOnly | kAdsl2CfgAnnexMpXMask);

    /* Phone line pair */
    dslCfgParam &= ~kAdslCfgLinePairMask;
    if (pDslLineObj->lineNumber == ADSL_LINE_INNER_PAIR)
        dslCfgParam |= kAdslCfgLineInnerPair;
    else
        dslCfgParam |= kAdslCfgLineOuterPair;

    /* Bit swap */
    pAdslCfg->adslDemodCapMask |= kXdslBitSwapEnabled;
    if (!strcmp(pDslLineObj->X_BROADCOM_COM_Bitswap, MDMVS_ON))
        pAdslCfg->adslDemodCapValue |= kXdslBitSwapEnabled;
    else
        pAdslCfg->adslDemodCapValue &= ~kXdslBitSwapEnabled;

    /* SRA */
    pAdslCfg->adslDemodCapMask |= kXdslSRAEnabled;
    if (!strcmp(pDslLineObj->X_BROADCOM_COM_SRA, MDMVS_ON))
        pAdslCfg->adslDemodCapValue |= kXdslSRAEnabled;
    else
        pAdslCfg->adslDemodCapValue &= ~kXdslSRAEnabled;

    if(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) {
        if(kAdslCfgModAny == (dslCfgParam & kAdslCfgModMask))
            pAdslCfg->adsl2Param |= kAdsl2CfgAnnexMOnly;
        else
            pAdslCfg->adsl2Param &= ~kAdsl2CfgAnnexMOnly;
    }
#ifdef DMP_VDSL2WAN_1
    pAdslCfg->vdslParam = 0;
    pAdslCfg->vdslParam1 = 0;
    if( dslCfgParam & kDslCfgModVdsl2Only ){
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8a)
            pAdslCfg->vdslParam |= kVdslProfile8a;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8b)
            pAdslCfg->vdslParam |= kVdslProfile8b;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8c)
            pAdslCfg->vdslParam |= kVdslProfile8c;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8d)
            pAdslCfg->vdslParam |= kVdslProfile8d;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_12a)
            pAdslCfg->vdslParam |= kVdslProfile12a;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_12b)
            pAdslCfg->vdslParam |= kVdslProfile12b;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_17a)
            pAdslCfg->vdslParam |= kVdslProfile17a;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_30a)
            pAdslCfg->vdslParam |= kVdslProfile30a;
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
        if(pDslLineObj->X_BROADCOM_COM_VDSL_35b)
            pAdslCfg->vdslParam |= kVdslProfile35b;
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
        if(pDslLineObj->X_BROADCOM_COM_VDSL_BrcmPriv2)
            pAdslCfg->vdslParam |= kVdslProfileBrcmPriv2;
#endif
        if(pDslLineObj->X_BROADCOM_COM_VDSL_US0_8a)
            pAdslCfg->vdslParam |= kVdslUS0Mask;
    }
#ifdef SUPPORT_DSL_GFAST
    if((dslCfgParam & kDslCfgModGfastOnly)
#ifdef CONFIG_MGFAST_SUPPORT
        || (dslCfgParam & kDslCfgModMgfastOnly)
#endif
        )
    {
       rutfast_cfgProfileInit_dev2(pAdslCfg,pDslLineObj->X_BROADCOM_COM_BondingLineNumber);
    }
#endif
#endif

    dslCfgParam &= ~kAdslCfgDemodCapMask;
    if(pAdslCfg->adslDemodCapMask)
        dslCfgParam |= kAdslCfgDemodCapOn;

    dslCfgParam &= ~kAdslCfgDemodCap2Mask;
    if(pAdslCfg->adslDemodCap2Mask)
        dslCfgParam |= kAdslCfgDemodCap2On;

    dslCfgParam &= ~kAdslCfgTrellisMask;
    if(pAdslCfg->adslDemodCapValue&kXdslTrellisEnabled)
        dslCfgParam |= kAdslCfgTrellisOn;
    else
        dslCfgParam |= kAdslCfgTrellisOff;

#ifdef ANNEX_C
    pAdslCfg->adslAnnexCParam = dslCfgParam;
#else
    pAdslCfg->adslAnnexAParam = dslCfgParam;
#endif

#ifdef CONFIG_MGFAST_SUPPORT
  {
    BcmRet ret;
    ret = devCtl_getBaseMacAddress((UINT8 *)&pAdslCfg->p2mp_ntid[0]);
    if(BCMRET_SUCCESS != ret)
        cmsLog_error("*** devCtl_getBaseMacAddress() failed(%d)! ***\n", ret);
  }
#endif

    cmsLog_debug("*** adslCfgParam=0x%X vdslParam=0x%X ***\n", dslCfgParam, pAdslCfg->vdslParam);
}

void xdslUtil_IntfCfgInit_dev2(adslCfgProfile *pAdslCfg,  Dev2DslLineObject *pDslLineObj)
{
    int len;
    char    cfgModType[BUFLEN_128];
#ifdef ANNEX_C
    ulong   dslCfgParam = pAdslCfg->adslAnnexCParam;
#else
    ulong   dslCfgParam = pAdslCfg->adslAnnexAParam;
#endif
    pDslLineObj->X_BROADCOM_COM_DslCfgParam = dslCfgParam;
    pDslLineObj->X_BROADCOM_COM_DslHsModeSwitchTime = pAdslCfg->adslHsModeSwitchTime;
    pDslLineObj->X_BROADCOM_COM_DslLOMTimeThldSec = pAdslCfg->adslLOMTimeThldSec;
    pDslLineObj->X_BROADCOM_COM_DslPwmSyncClockFreq = pAdslCfg->adslPwmSyncClockFreq;
    pDslLineObj->X_BROADCOM_COM_DslShowtimeMarginQ4 = pAdslCfg->adslShowtimeMarginQ4;
    pDslLineObj->X_BROADCOM_COM_DslTrainingMarginQ4 = pAdslCfg->adslTrainingMarginQ4;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Mask = pAdslCfg->adslDemodCapMask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Value = pAdslCfg->adslDemodCapValue;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Mask = pAdslCfg->adslDemodCap2Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Value = pAdslCfg->adslDemodCap2Value;
    pDslLineObj->X_BROADCOM_COM_DslParam = pAdslCfg->adsl2Param;
#ifdef SUPPORT_CFG_PROFILE
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Mask = pAdslCfg->xdslAuxFeaturesMask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Value = pAdslCfg->xdslAuxFeaturesValue;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Mask = pAdslCfg->vdslCfgFlagsMask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Value = pAdslCfg->vdslCfgFlagsValue;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Mask = pAdslCfg->xdslCfg1Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Value = pAdslCfg->xdslCfg1Value;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Mask = pAdslCfg->xdslCfg2Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Value = pAdslCfg->xdslCfg2Value;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Mask = pAdslCfg->xdslCfg3Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Value = pAdslCfg->xdslCfg3Value;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Mask = pAdslCfg->xdslCfg4Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Value = pAdslCfg->xdslCfg4Value;
#endif

    pDslLineObj->X_BROADCOM_COM_DslPhyUsDataRateKbps = pAdslCfg->maxUsDataRateKbps;
    pDslLineObj->X_BROADCOM_COM_DslPhyDsDataRateKbps = pAdslCfg->maxDsDataRateKbps;
    pDslLineObj->X_BROADCOM_COM_DslPhyAggrDataRateKbps = pAdslCfg->maxAggrDataRateKbps;
    pDslLineObj->X_BROADCOM_COM_DslPhyMiscCfgParam = pAdslCfg->xdslMiscCfgParam;

    /* Modulations */
    cmsMem_free(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg);
    if((kAdslCfgModAny == (dslCfgParam & kAdslCfgModMask)) && !(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled)) {
        pDslLineObj->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdup(MDMVS_ADSL_MODULATION_ALL);
    }
    else {
        memset(cfgModType, 0, BUFLEN_128);

        if(dslCfgParam & kAdslCfgModGdmtOnly) {
            strcat(cfgModType,MDMVS_ADSL_G_DMT);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModGliteOnly) {
            strcat(cfgModType,MDMVS_ADSL_G_LITE);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModT1413Only) {
            strcat(cfgModType,MDMVS_ADSL_ANSI_T1_413);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModAdsl2Only) {
            strcat(cfgModType,MDMVS_ADSL_G_DMT_BIS);
            strcat(cfgModType,", ");
        }
        if(pAdslCfg->adsl2Param & kAdsl2CfgReachExOn) {
            strcat(cfgModType,MDMVS_ADSL_RE_ADSL);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModAdsl2pOnly) {
            strcat(cfgModType,MDMVS_ADSL_2PLUS);
            strcat(cfgModType,", ");
        }
        if(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) {
            strcat(cfgModType,MDMVS_ANNEXM);
            strcat(cfgModType,", ");
        }
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        if(dslCfgParam & kDslCfgModVdsl2Only) {
            strcat(cfgModType,MDMVS_VDSL2);
            strcat(cfgModType,", ");
        }
#ifdef SUPPORT_DSL_GFAST
        if(dslCfgParam & kDslCfgModGfastOnly) {
            strcat(cfgModType,MDMVS_G_FAST);
            strcat(cfgModType,", ");
        }
#endif
#ifdef CONFIG_MGFAST_SUPPORT
        if(dslCfgParam & kDslCfgModMgfastOnly) {
            strcat(cfgModType,MDMVS_MG_FAST);
            strcat(cfgModType,", ");
        }
#endif
#endif
        /* take out the last ", " */
        len = strlen(cfgModType);
        if (len > 2) {
           cfgModType[len-2] = '\0';
           pDslLineObj->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdup(cfgModType);
        }
        else {
           /* default will be all */
           pDslLineObj->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdup(MDMVS_ADSL_MODULATION_ALL);
        }
    }
    pDslLineObj->X_BROADCOM_COM_ADSL2_AnnexM = ((pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) != 0);

    /* VDSL2 profile */
#ifdef  DMP_VDSL2WAN_1
    pDslLineObj->X_BROADCOM_COM_VDSL_8a = ((pAdslCfg->vdslParam & kVdslProfile8a) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_8b = ((pAdslCfg->vdslParam & kVdslProfile8b) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_8c = ((pAdslCfg->vdslParam & kVdslProfile8c) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_8d = ((pAdslCfg->vdslParam & kVdslProfile8d) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_12a = ((pAdslCfg->vdslParam & kVdslProfile12a) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_12b = ((pAdslCfg->vdslParam & kVdslProfile12b) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_17a = ((pAdslCfg->vdslParam & kVdslProfile17a) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_30a = ((pAdslCfg->vdslParam & kVdslProfile30a) != 0);
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
    pDslLineObj->X_BROADCOM_COM_VDSL_35b = ((pAdslCfg->vdslParam & kVdslProfile35b) != 0);
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
    pDslLineObj->X_BROADCOM_COM_VDSL_BrcmPriv2 = ((pAdslCfg->vdslParam & kVdslProfileBrcmPriv2) != 0);
#endif
    pDslLineObj->X_BROADCOM_COM_VDSL_US0_8a = ((pAdslCfg->vdslParam & kVdslUS0Mask) != 0);
#endif

    /* Capability */
   /* sra */
    cmsMem_free(pDslLineObj->X_BROADCOM_COM_SRA);
    if (pAdslCfg->adslDemodCapValue & kXdslSRAEnabled)
        pDslLineObj->X_BROADCOM_COM_SRA = cmsMem_strdup(MDMVS_ON);
    else
        pDslLineObj->X_BROADCOM_COM_SRA = cmsMem_strdup(MDMVS_OFF);
    /* bitswap */
    cmsMem_free(pDslLineObj->X_BROADCOM_COM_Bitswap);
    if (pAdslCfg->adslDemodCapValue & kXdslBitSwapEnabled)
        pDslLineObj->X_BROADCOM_COM_Bitswap = cmsMem_strdup(MDMVS_ON);
    else
        pDslLineObj->X_BROADCOM_COM_Bitswap = cmsMem_strdup(MDMVS_OFF);

    if(kAdslCfgLineOuterPair == (dslCfgParam & kAdslCfgLinePairMask))
       pDslLineObj->lineNumber = ADSL_LINE_OUTER_PAIR;
    else
       pDslLineObj->lineNumber = ADSL_LINE_INNER_PAIR;
}

UBOOL8 rutdsl_isDslConfigChanged_dev2(const _Dev2DslLineObject *newObj, const _Dev2DslLineObject *currObj)
{
   UBOOL8 changed=FALSE;

   if (!POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      return FALSE;
   }

   /*
    * need to check for a lot more parameter than this.
    * But these will be enough to detect run-time changes of VDSL profiles.
    * The POTENTIAL_CHANGE_OF_EXISTING macro guarantees both pointers are not null.
    */
   if (
       (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_AdslModulationCfg, currObj->X_BROADCOM_COM_AdslModulationCfg)) ||
       (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_Bitswap, currObj->X_BROADCOM_COM_Bitswap)) ||
       (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_SRA, currObj->X_BROADCOM_COM_SRA)) ||
#ifdef DMP_X_RDK_DSL_1
       // These params will be standardized in the future, so for now, use
       // them only in RDK builds.
       (0 != cmsUtl_strcmp(newObj->XTURSystemCountry, currObj->XTURSystemCountry)) ||
       (0 != cmsUtl_strcmp(newObj->XTURSystemVendor, currObj->XTURSystemVendor)) ||
       (0 != cmsUtl_strcmp(newObj->XTURSystemVendorSpecific, currObj->XTURSystemVendorSpecific)) ||
       (0 != cmsUtl_strcmp(newObj->XTURVersion, currObj->XTURVersion)) ||
       (0 != cmsUtl_strcmp(newObj->XTURSerial, currObj->XTURSerial)) ||
#endif
       (newObj->X_BROADCOM_COM_DslPhyAggrDataRateKbps != currObj->X_BROADCOM_COM_DslPhyAggrDataRateKbps) ||
       (newObj->X_BROADCOM_COM_DslPhyDsDataRateKbps != currObj->X_BROADCOM_COM_DslPhyDsDataRateKbps) ||
       (newObj->X_BROADCOM_COM_DslPhyUsDataRateKbps != currObj->X_BROADCOM_COM_DslPhyUsDataRateKbps) ||
       (newObj->lineNumber != currObj->lineNumber) ||
       (newObj->X_BROADCOM_COM_ADSL2_AnnexM != currObj->X_BROADCOM_COM_ADSL2_AnnexM) ||
       (newObj->X_BROADCOM_COM_DslHsModeSwitchTime != currObj->X_BROADCOM_COM_DslHsModeSwitchTime) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg1Mask != currObj->X_BROADCOM_COM_DslPhyCfg1Mask) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg1Value != currObj->X_BROADCOM_COM_DslPhyCfg1Value) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg2Mask != currObj->X_BROADCOM_COM_DslPhyCfg2Mask) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg2Value != currObj->X_BROADCOM_COM_DslPhyCfg2Value) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg3Mask != currObj->X_BROADCOM_COM_DslPhyCfg3Mask) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg3Value != currObj->X_BROADCOM_COM_DslPhyCfg3Value) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg4Mask != currObj->X_BROADCOM_COM_DslPhyCfg4Mask) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg4Value != currObj->X_BROADCOM_COM_DslPhyCfg4Value) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg5Mask != currObj->X_BROADCOM_COM_DslPhyCfg5Mask) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg5Value != currObj->X_BROADCOM_COM_DslPhyCfg5Value) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg6Mask != currObj->X_BROADCOM_COM_DslPhyCfg6Mask) ||
       (newObj->X_BROADCOM_COM_DslPhyCfg6Value != currObj->X_BROADCOM_COM_DslPhyCfg6Value)
#ifdef DMP_VDSL2WAN_1
       || (newObj->X_BROADCOM_COM_VDSL_8a != currObj->X_BROADCOM_COM_VDSL_8a) ||
       (newObj->X_BROADCOM_COM_VDSL_8b != currObj->X_BROADCOM_COM_VDSL_8b) ||
       (newObj->X_BROADCOM_COM_VDSL_8c != currObj->X_BROADCOM_COM_VDSL_8c) ||
       (newObj->X_BROADCOM_COM_VDSL_8d != currObj->X_BROADCOM_COM_VDSL_8d) ||
       (newObj->X_BROADCOM_COM_VDSL_12a != currObj->X_BROADCOM_COM_VDSL_12a) ||
       (newObj->X_BROADCOM_COM_VDSL_12b != currObj->X_BROADCOM_COM_VDSL_12b) ||
       (newObj->X_BROADCOM_COM_VDSL_17a != currObj->X_BROADCOM_COM_VDSL_17a) ||
       (newObj->X_BROADCOM_COM_VDSL_30a != currObj->X_BROADCOM_COM_VDSL_30a) ||
       (newObj->X_BROADCOM_COM_VDSL_US0_8a != currObj->X_BROADCOM_COM_VDSL_US0_8a)
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
       || (newObj->X_BROADCOM_COM_VDSL_35b != currObj->X_BROADCOM_COM_VDSL_35b)
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
       || (newObj->X_BROADCOM_COM_VDSL_BrcmPriv2 != currObj->X_BROADCOM_COM_VDSL_BrcmPriv2) 
#endif
#endif
       )
   {
      changed = TRUE;
   }
   return (changed);
}


#ifdef DMP_X_RDK_DSL_1
CmsRet rutdsl_configDslOemEoc(Dev2DslLineObject *dslLineObj)
{
   CmsRet ret = CMSRET_SUCCESS;

   if (dslLineObj == NULL)
   {
      cmsLog_error("dslLineObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Cram 3 fixed length HexBinary params into a single HexBinary buffer then convert to binary.
   // <XTURSystemCountry><XTURSystemVendor><XTURSystemVendorSpecific>
   // hexBinary len= 4         8                      4
   // binary len   = 2         4                      2
   // params should either have the exact length, or NULL/empty..  If null/empty,
   // substitute 0 in its place.  The final string should be hexBinary len 16,
   // binary len 8.
   {
      UINT32 i=0;
      UINT32 j=0;
      char hexBuf[17]={0};
      UINT8 *sysVendorId = NULL;
      SINT32 hexRet;

      // first fill hexBuf with '0' in case we advance over any slots
      for (i=0; i < 16; i++)
         hexBuf[i] = '0';

      if (!IS_EMPTY_STRING(dslLineObj->XTURSystemCountry))
      {
         for (i=0; i < 4; i++,j++)
            hexBuf[j] = dslLineObj->XTURSystemCountry[i];
      }
      j = 4;  // even if XTURSystemCountry was null/empty, we advance
      if (!IS_EMPTY_STRING(dslLineObj->XTURSystemVendor))
      {
         for (i=0; i < 8; i++,j++)
            hexBuf[j] = dslLineObj->XTURSystemVendor[i];
      }
      j = 12; // even if XTURSystemVendor was null/empty, we advance
      if (!IS_EMPTY_STRING(dslLineObj->XTURSystemVendorSpecific))
      {
         for (i=0; i < 4; i++,j++)
            hexBuf[j] = dslLineObj->XTURSystemVendorSpecific[i];
      }

      hexRet = genUtl_hexStringToBinaryBufMalloc(hexBuf, &sysVendorId);
      if (hexRet == HEXRET_SUCCESS)
      {
         cmsLog_debug("8 octet binary buf (in hex) %02x%02x%02x%02x%02x%02x%02x%02x",
                sysVendorId[0], sysVendorId[1],
                sysVendorId[2], sysVendorId[3],
                sysVendorId[4], sysVendorId[5],
                sysVendorId[6], sysVendorId[7]);

         ret = (CmsRet) xdslCtl_SetOemParam(dslLineObj->lineNumber-1,
                                   ADSL_OEM_EOC_VENDOR_ID,
                                   sysVendorId, 8);
         free(sysVendorId);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ADSL_OEM_EOC_VENDOR_ID to %s failed, ret=%d",
                         hexBuf, ret);
            return ret;
         }
         else
         {
            cmsLog_debug("set ADSL_OEM_EOC_VENDOR_ID to %s SUCCESS!", hexBuf);
         }
      }
      else
      {
         cmsLog_error("hexBinary convert of %s failed, hexRet=%d",
                      hexBuf, hexRet);
         return CMSRET_INVALID_ARGUMENTS;
      }
   }


   if (!IS_EMPTY_STRING(dslLineObj->XTURVersion))
   {
      ret = (CmsRet) xdslCtl_SetOemParam(dslLineObj->lineNumber-1,
                                         ADSL_OEM_EOC_VERSION,
                                         dslLineObj->XTURVersion,
                                         strlen(dslLineObj->XTURVersion));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of ADSL_OEM_EOC_VERSION to %s failed, ret=%d",
                      dslLineObj->XTURVersion, ret);
         return ret;
      }
      else
      {
         cmsLog_debug("set ADSL_OEM_EOC_VERSION to %s SUCCESS!",
                      dslLineObj->XTURVersion);
      }

   }

   if (!IS_EMPTY_STRING(dslLineObj->XTURSerial))
   {
      ret = (CmsRet) xdslCtl_SetOemParam(dslLineObj->lineNumber-1,
                                         ADSL_OEM_EOC_SERIAL_NUMBER,
                                         dslLineObj->XTURSerial,
                                         strlen(dslLineObj->XTURSerial));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of ADSL_OEM_EOC_SERIAL_NUMBER to %s failed, ret=%d",
                      dslLineObj->XTURSerial, ret);
         return ret;
      }
      else
      {
         cmsLog_debug("set ADSL_OEM_EOC_SERIAL_NUMBER to %s SUCCESS!",
                      dslLineObj->XTURSerial);
      }
   }

   return ret;
}
#endif  /* DMP_X_RDK_DSL_1 */


void processXdslCfgSaveMessage_dev2(UINT32 msgId)
{
    long    dataLen;
    char    oidStr[] = { 95 };      /* kOidAdslPhyCfg */
    adslCfgProfile  adslCfg;
    CmsRet          cmsRet;
    Dev2DslLineObject *dslLineObj = NULL;
#ifdef SUPPORT_DSL_GFAST
    Dev2FastLineObject *fastLineObj = NULL;
    InstanceIdStack         iidStack1 = EMPTY_INSTANCE_ID_STACK;
#endif
    InstanceIdStack         iidStack = EMPTY_INSTANCE_ID_STACK;

    dataLen = sizeof(adslCfgProfile);
    cmsRet = (CmsRet) xdslCtl_GetObjectValue(0, oidStr, sizeof(oidStr), (char *)&adslCfg, &dataLen);

    if( cmsRet != (CmsRet) BCMADSL_STATUS_SUCCESS) {
        cmsLog_error("Could not get adsCfg, ret=%d", cmsRet);
        return;
    }

    if ((cmsRet = cmsLck_acquireLockWithTimeout(RUTDSL_LOCK_TIMEOUT)) != CMSRET_SUCCESS) {
        cmsLog_error("Could not get lock, ret=%d", cmsRet);
        cmsLck_dumpInfo();
        /* just a kernel event, I guess we can try later. */
        return;
    }

    cmsRet = cmsObj_getNext(MDMOID_DEV2_DSL_LINE, &iidStack, (void **) &dslLineObj);
    if (cmsRet != CMSRET_SUCCESS) {
        cmsLck_releaseLock();
        cmsLog_error("Could not get dslLineObj, ret=%d", cmsRet);
        return;
    }
#ifdef SUPPORT_DSL_GFAST
    cmsRet = cmsObj_getNext(MDMOID_DEV2_FAST_LINE, &iidStack1, (void **) &fastLineObj);
    if (cmsRet != CMSRET_SUCCESS) {
        cmsLck_releaseLock();
        cmsLog_error("Could not get fastLineObj, ret=%d", cmsRet);
        cmsObj_free((void **) &dslLineObj);
        return;
    }
#endif
    if(MSG_ID_BRCM_SAVE_DSL_CFG_ALL == msgId) {
#ifdef SUPPORT_DSL_GFAST
       rutfast_intfCfgInit_dev2(&adslCfg,fastLineObj);
#endif
       xdslUtil_IntfCfgInit_dev2(&adslCfg, dslLineObj);
    }
#if defined(SUPPORT_MULTI_PHY) || defined(SUPPORT_DSL_GFAST)
    else if (MSG_ID_BRCM_SAVE_DSL_PREFERRED_LINE == msgId) {
        dslLineObj->X_BROADCOM_COM_DslPhyMiscCfgParam &= ~(BCM_PREFERREDTYPE_FOUND | BCM_MEDIATYPE_MSK);
        dslLineObj->X_BROADCOM_COM_DslPhyMiscCfgParam |= (adslCfg.xdslMiscCfgParam & (BCM_PREFERREDTYPE_FOUND | BCM_MEDIATYPE_MSK));
    }
#endif
    else {
       goto error_return;
    }

    cmsRet = cmsObj_set(dslLineObj, &iidStack);
    if (cmsRet != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not set dslLineObj, ret=%d", cmsRet);
        goto error_return;
    }
#ifdef SUPPORT_DSL_GFAST
    if((cmsRet = cmsObj_set(fastLineObj, &iidStack1)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Could not set fastLineObj, ret=%d", cmsRet);
       goto error_return;
    }
#endif

    cmsRet = cmsMgm_saveConfigToFlash();

    error_return:
    cmsObj_free((void **) &dslLineObj);
#ifdef SUPPORT_DSL_GFAST
    cmsObj_free((void **) &fastLineObj);
#endif
    cmsLck_releaseLock();

    if(cmsRet != CMSRET_SUCCESS)
        cmsLog_error("Writing  Xdsl Cfg to flash.failed!");
}


CmsRet rutdsl_configUp_dev2(Dev2DslLineObject *dslLineObj)
{
   UBOOL8 xDSLBonding = FALSE;
   adslCfgProfile adslCfg;
   CmsRet ret = CMSRET_SUCCESS;

   /* modulationType has been deprecated */
   if (!dslLineObj->X_BROADCOM_COM_Bitswap &&
       ((dslLineObj->lineNumber != 1) && (dslLineObj->lineNumber != 2)) &&
       !dslLineObj->X_BROADCOM_COM_SRA)
   {
      cmsLog_error("Invalid dslLineObj parameters");
      return CMSRET_INVALID_PARAM_VALUE;
   }

#ifdef DMP_VDSL2WAN_1
   cmsLog_debug("bitswap %s, phone line number (1=innerPair) %d, sra %s, modulation %s, profile 8a=%d 8b=%d 8c=%d 8c=%d",
                 dslLineObj->X_BROADCOM_COM_Bitswap,
                 dslLineObj->lineNumber,
                 dslLineObj->X_BROADCOM_COM_SRA,
                 dslLineObj->X_BROADCOM_COM_AdslModulationCfg,
                 dslLineObj->X_BROADCOM_COM_VDSL_8a,
                 dslLineObj->X_BROADCOM_COM_VDSL_8b,
                 dslLineObj->X_BROADCOM_COM_VDSL_8c,
                 dslLineObj->X_BROADCOM_COM_VDSL_8d);
#endif
   /* initialialize adslCfgProfile */
   memset((char *) &adslCfg, 0x00, sizeof(adslCfg));
   xdslUtil_CfgProfileInit_dev2(&adslCfg, dslLineObj);
#ifdef SUPPORT_DSL_GFAST
   rutfast_cfgProfileInit_dev2(&adslCfg,dslLineObj->X_BROADCOM_COM_BondingLineNumber);
#endif

#ifdef SUPPORT_DSL_BONDING
    xDSLBonding = rutDsl_isDslBondingEnabled();
#ifdef SUPPORT_MULTI_PHY
    if( !(adslCfg.xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) ){
        /* Image type sanity check */
        if( TRUE == xDSLBonding ) {
            if( BCM_IMAGETYPE_BONDING != (adslCfg.xdslMiscCfgParam & BCM_IMAGETYPE_MSK) ) {
                adslCfg.xdslMiscCfgParam = (adslCfg.xdslMiscCfgParam &~BCM_IMAGETYPE_MSK) | BCM_IMAGETYPE_BONDING;
                cmsLog_notice("xDSL image type is out of sync, XTM is in bonding mode");
            }
        }
        else {
            if( BCM_IMAGETYPE_SINGLELINE != (adslCfg.xdslMiscCfgParam & BCM_IMAGETYPE_MSK) ) {
                adslCfg.xdslMiscCfgParam = (adslCfg.xdslMiscCfgParam &~BCM_IMAGETYPE_MSK) | BCM_IMAGETYPE_SINGLELINE;
                cmsLog_notice("xDSL image type is out of sync, XTM is in non-bonding mode");
            }
        }
    }
#endif /* SUPPORT_MULTI_PHY */
#endif /* SUPPORT_DSL_BONDING */

#ifdef SECONDARY_AFEID_FN
  {
     CmsRet ret;
     unsigned int  afeIds[2] = {0, 0};
     int i = 0;
     char  *pToken, *pLast, line[64], tokenSeperator[] = " \n";
     FILE  *fs = fopen(SECONDARY_AFEID_FN, "r");
     if (NULL != fs) {
        if(fgets(line, 64, fs) != NULL) {
           pToken = strtok_r(line, tokenSeperator, &pLast);
           while((NULL != pToken) && (i < 2)) {
              if ((pToken[0] == '0') && ((pToken[1] == 'x') || (pToken[1] == 'X')))
                 afeIds[i] = strtoul(pToken+2, NULL, 16);
              pToken = strtok_r(NULL, tokenSeperator, &pLast);
              i++;
           }
           if((afeIds[0] != 0) && (afeIds[1] != 0)) {
              ret = (CmsRet)xdslCtl_DiagProcessDbgCommand(0,DIAG_DEBUG_CMD_OVERRIDE_2ND_AFEIDS,0,afeIds[0],afeIds[1]);
              if (ret != CMSRET_SUCCESS)
                 cmsLog_error("xdslCtl_DiagProcessDbgCommand(0,DIAG_DEBUG_CMD_OVERRIDE_2ND_AFEIDS...) failed!");
           }
           else
              cmsLog_error("No valid afeId in %s file!", SECONDARY_AFEID_FN);
        }
        else
           cmsLog_error("fgets from %s file failed!", SECONDARY_AFEID_FN);
        fclose(fs);
     }
  }
#endif

   cmsAdsl_initialize(&adslCfg);

   /*
    * It seems like this block only needs to be done once per system bootup.
    */
   if (mdmShmCtx->dslInitDone != TRUE)
   {
      XTM_INITIALIZATION_PARMS InitParms;
      XTM_INTERFACE_CFG IntfCfg;

      memset((UINT8 *)  &InitParms, 0x00, sizeof(InitParms));
      memset((UINT8 *)  &IntfCfg, 0x00, sizeof(IntfCfg));

      InitParms.bondConfig.sConfig.ptmBond = BC_PTM_BONDING_DISABLE ;
      InitParms.bondConfig.sConfig.atmBond = BC_ATM_BONDING_DISABLE ;

      if (xDSLBonding == TRUE) {
         InitParms.bondConfig.sConfig.ptmBond = BC_PTM_BONDING_ENABLE ;
         InitParms.bondConfig.sConfig.atmBond = BC_ATM_BONDING_ENABLE ;
      }

      cmsLog_debug("DSL PTM Bonding %s", InitParms.bondConfig.sConfig.ptmBond ? "Enable" : "Disable");
      cmsLog_debug("DSL ATM Bonding %s", InitParms.bondConfig.sConfig.atmBond ? "Enable" : "Disable");

#if defined(SUPPORT_DSL_BONDING)
      InitParms.bondConfig.sConfig.autoSenseAtm = BC_ATM_AUTO_SENSE_ENABLE ;
#if defined(SUPPORT_EXT_DSL_BONDING)
      /* 63268 currently has this configuration. */
      InitParms.ulPortConfig = PC_INTERNAL_EXTERNAL ;
#endif
#endif

      devCtl_xtmInitialize( &InitParms ) ;

      /* TBD. Need configuration to indicate how many ports to enable. */
      IntfCfg.ulIfAdminStatus = ADMSTS_UP;
      devCtl_xtmSetInterfaceCfg( PORT_PHY0_FAST, &IntfCfg );
      devCtl_xtmSetInterfaceCfg( PORT_PHY0_INTERLEAVED, &IntfCfg );

      mdmShmCtx->dslInitDone = TRUE;
   }

#ifdef DMP_X_RDK_DSL_1
      // These OEM_EOC params can only be configured after the driver is up.
      ret = rutdsl_configDslOemEoc(dslLineObj);
      if (ret != CMSRET_SUCCESS)
      {
         // log error but keep going
         cmsLog_error("rutdsl_configDslOemEoc failed, ret=%d", ret);
      }
#endif

   cmsAdsl_startTrace();

   cmsLog_debug("Starting line 0");
   ret = (CmsRet) xdslCtl_ConnectionStart(0);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to start xdsl conn 0, ret=%d", ret);
      return ret;
   }

   if (xDSLBonding == TRUE)
   {
      cmsLog_debug("Starting bonding line 1");
      ret = (CmsRet) xdslCtl_ConnectionStart(1);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to start xdsl conn 1, ret=%d", ret);
         return ret;
      }
   }

   return CMSRET_SUCCESS;
}

void rutdsl_configDown_dev2(Dev2DslLineObject *dslLineObj)
{
   xdslCtl_ConnectionStop(dslLineObj->X_BROADCOM_COM_BondingLineNumber);
}

/* line show time stats */
CmsRet rutdsl_getAdslShowTimeStats_dev2(Dev2DslLineStatsShowtimeObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->erroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslSES;
      return (CMSRET_SUCCESS);
   }
   else
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslShowTimeStats_dev2 */

CmsRet rutdsl_getAdslCurrentDayStats_dev2(Dev2DslLineStatsCurrentDayObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->erroredSecs = adslMib.adslPerfData.perfCurr1Day.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr1Day.adslSES;
#ifdef DMP_X_RDK_DSL_1
      obj->X_RDK_LinkRetrain = adslMib.adslPerfData.failCurDay.adslRetr;
#endif
      return (CMSRET_SUCCESS);
   }
   else
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslCurrentDayStats_dev2 */

CmsRet rutdsl_getAdslQuarterHourStats_dev2(Dev2DslLineStatsQuarterHourObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->erroredSecs = adslMib.adslPerfData.perfCurr15Min.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr15Min.adslSES;
#ifdef DMP_X_RDK_DSL_1
      obj->X_RDK_LinkRetrain = adslMib.adslPerfData.failCur15Min.adslRetr;
#endif
      return (CMSRET_SUCCESS);
   }
   else
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslQuarterHourStats_dev2 */

CmsRet rutdsl_getAdslTestParamsInfo_dev2(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   long len;
   char oidStr[] = { kOidAdslPrivate, 0 };
   char   oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
   UINT16 bandObjData[NUM_BAND];
   SINT16 subcarrierData[NUM_TONE_GROUP];
   UINT8 gFactor = 1;
   Dev2DslLineTestParamsObject *testParamObj = (Dev2DslLineTestParamsObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;
   char *dataStr;

   dataStr = cmsMem_alloc(MAX_PS_STRING,ALLOC_ZEROIZE);
   if (dataStr == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }
   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,&lineId) == CMSRET_SUCCESS)
   {
      /* get G-factor objects --VDSL only */
#ifdef DMP_DEVICE2_VDSL2_1
      testParamObj->HLOGGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      testParamObj->HLOGGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      testParamObj->QLNGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      testParamObj->QLNGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      testParamObj->SNRGds = adslMib.gFactors.Gfactor_MEDLEYSETds;
      testParamObj->SNRGus = adslMib.gFactors.Gfactor_MEDLEYSETus;
#endif

#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      if (adslMib.adslConnection.modType == kVdslModVdsl2)
      {
         /* per-band objects --VDSL only */ 
         oidStr[1] = kOidAdslPrivLATNdsperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d",bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->LATNds,dataStr,mdmLibCtx.allocFlags);

         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivLATNusperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d", bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->LATNus,dataStr,mdmLibCtx.allocFlags);

         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivSATNdsperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d", bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->SATNds,dataStr,mdmLibCtx.allocFlags);

         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivSATNusperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d", bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->SATNus,dataStr,mdmLibCtx.allocFlags);

      } /* vdsl only */
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */

      testParamObj->SNRMTds = adslMib.adslPhys.SNRMT;
      testParamObj->SNRMTus = adslMib.adslAtucPhys.SNRMT;
      testParamObj->QLNMTds = adslMib.adslPhys.QLNMT;
      testParamObj->QLNMTus = adslMib.adslAtucPhys.QLNMT;
      testParamObj->HLOGMTds = adslMib.adslPhys.HLOGMT;
      testParamObj->HLOGMTus = adslMib.adslAtucPhys.HLOGMT;


      /* before getting tone data, first set the gfactor */
      oidStr1[2] = kOidAdslPrivSetFlagActualGFactor;
      gFactor = 1;
      len = 1;

      ret = (CmsRet) xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS)
      {
         /* get QLNpsds */
         oidStr[1] = kOidAdslPrivQuietLineNoiseDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&dataStr,subcarrierData,(char*)"QLNpsds",MAX_QLN_STRING);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->QLNpsds,dataStr,mdmLibCtx.allocFlags);
      }

      len = 1;
      ret = (CmsRet) xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS)
      {
         /* get QLNpsus */
         oidStr[1] = kOidAdslPrivQuietLineNoiseUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&dataStr,subcarrierData,(char*)"QLNpsus",MAX_QLN_STRING);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->QLNpsus,dataStr,mdmLibCtx.allocFlags);
      }
      len = 1;
      ret = (CmsRet) xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS)
      {
         /* get SNR */
         oidStr[1] = kOidAdslPrivSNRDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&dataStr,subcarrierData,(char*)"SNRpsds",MAX_PS_STRING);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->SNRpsds,dataStr,mdmLibCtx.allocFlags);
      }
      len = 1;
      ret = (CmsRet) xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivSNRUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&dataStr,subcarrierData,(char*)"SNRpsus",MAX_PS_STRING);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->SNRpsus,dataStr,mdmLibCtx.allocFlags);
      }
      len = 1;
      ret = (CmsRet) xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS)
      {
         /* get HLOG */
         oidStr[1] = kOidAdslPrivChanCharLogDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&dataStr,subcarrierData,(char*)"HLOGpsds",MAX_LOGARITHMIC_STRING);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->HLOGpsds,dataStr,mdmLibCtx.allocFlags);
      }
      len = 1;
      ret = (CmsRet) xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len);
      if (ret == CMSRET_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivChanCharLogUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&dataStr,subcarrierData,(char*)"HLOGpsus",MAX_LOGARITHMIC_STRING);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->HLOGpsus,dataStr,mdmLibCtx.allocFlags);
      }
      ret = CMSRET_SUCCESS;
   } /* get mib statistics ok */
   cmsMem_free(dataStr);
   return ret;
} /* rutdsl_getAdslTestParamsInfo_dev2 */

/* line stats total */
CmsRet rutdsl_getAdslTotalStats_dev2(Dev2DslLineStatsTotalObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (obj == NULL)
   {
      return (CMSRET_SUCCESS);
   }

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->erroredSecs = adslMib.adslPerfData.perfTotal.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamEs = adslMib.adslTxPerfTotal.adslESs;
      obj->X_BROADCOM_COM_UpstreamSes = adslMib.adslTxPerfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamUas = adslMib.adslTxPerfTotal.adslUAS;
      obj->X_BROADCOM_COM_DownstreamUas = adslMib.adslPerfData.perfTotal.adslUAS;
   }

   return (CMSRET_SUCCESS);
} /* rutdsl_getTotalStats_dev2 */

CmsRet rutdsl_getdslLineStats_dev2(Dev2DslLineStatsObject *obj  __attribute__((unused)),
                                   const InstanceIdStack *iidStack  __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   UINT16 xdslStatus;
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 lineId=0;

   cmsLog_debug("Entered");

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   rutdsl_getLineIdByLineIidStack_dev2(iidStack, &lineId);
   ret = (CmsRet)xdslCtl_GetConnectionInfo(lineId, &adslConnInfo);
   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo[%d].LinkState=%d", lineId, xdslStatus);

   if (xdslStatus == BCM_ADSL_LINK_UP)
   {
      XTM_BOND_INFO bondInfo;
      XTM_INTERFACE_STATS intfStats;

      if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
      {
         obj->totalStart = adslMib.adslPerfData.adslSinceDrvStartedTimeElapsed;
         obj->showtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->lastShowtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
         obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;
      } /* adslMib retrieved */

      /* Check if this is DSL Bonding. */
      memset((UINT8 *)&bondInfo, 0, sizeof(XTM_BOND_INFO));
      ret = devCtl_xtmGetBondingInfo(&bondInfo);
      if (ret == CMSRET_SUCCESS || ret == CMSRET_METHOD_NOT_SUPPORTED)
      {
         if (bondInfo.ulNumGroups == 0)
         {
            /* Not DSL Bonding. Use lineId from bondInfo instead. */
            lineId = bondInfo.grpInfo[0].portInfo[0].ulInterfaceId;
            cmsLog_debug("interfaceId=%u", lineId);
         }

         /* Get the port stats. Note that portId is a bitmap of port number */
         memset((UINT8 *)&intfStats, 0x00, sizeof(intfStats));
         ret = devCtl_xtmGetInterfaceStatistics((1 << lineId), &intfStats, FALSE);
         if (ret == CMSRET_SUCCESS)
         {
            obj->bytesReceived = (UINT64) intfStats.ulIfInOctets;
            obj->bytesSent = (UINT64)intfStats.ulIfOutOctets;
            obj->packetsReceived = (UINT64)intfStats.ulIfInPackets;
            obj->packetsSent = (UINT64)intfStats.ulIfOutPackets;
            obj->errorsReceived = intfStats.ulIfInPacketErrors;
         }
      }
   }
   else
   {
      /* TR181: if down, cpe must reset statistics */
      obj->bytesSent = 0;
      obj->bytesReceived = 0;
      obj->packetsSent = 0;
      obj->packetsReceived = 0;
      obj->errorsSent = 0;
      obj->errorsReceived = 0;
      obj->discardPacketsSent = 0;
      obj->discardPacketsReceived = 0;
      obj->totalStart = 0;
      obj->showtimeStart = 0;
      obj->lastShowtimeStart = 0;
      obj->currentDayStart = 0;
      obj->quarterHourStart = 0;
   }

   return (ret);
#endif /* DESKTOP_LINUX */
} /* rutdsl_getdslLineStats_dev2 */

CmsRet rutdsl_getdslChannelStats_dev2(Dev2DslChannelStatsObject *obj  __attribute__((unused)),
                                      const InstanceIdStack *iidStack  __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   UINT16 xdslStatus;
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 lineId;

   cmsLog_debug("Entered");

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   rutdsl_getLineIdByChannelIidStack_dev2(iidStack, &lineId);
   ret = (CmsRet)xdslCtl_GetConnectionInfo(lineId, &adslConnInfo);
   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);

   if (xdslStatus == BCM_ADSL_LINK_UP)
   {
      XTM_BOND_INFO bondInfo;
      XTM_INTERFACE_STATS intfStats;

      if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
      {
         obj->totalStart = adslMib.adslPerfData.adslSinceDrvStartedTimeElapsed;
         obj->showtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->lastShowtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
         obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;
      } /* adslMib retrieved */

      /* Check if this is DSL Bonding. */
      memset((UINT8 *)&bondInfo, 0, sizeof(XTM_BOND_INFO));
      ret = devCtl_xtmGetBondingInfo(&bondInfo);
      if (ret == CMSRET_SUCCESS || ret == CMSRET_METHOD_NOT_SUPPORTED)
      {
         if (bondInfo.ulNumGroups == 0)
         {
            /* Not DSL Bonding. Use lineId from bondInfo instead. */
            lineId = bondInfo.grpInfo[0].portInfo[0].ulInterfaceId;
            cmsLog_debug("interfaceId=%u", lineId);
         }

         /* Get the port stats. Note that portId is a bitmap of port number */
         memset((UINT8 *)&intfStats, 0x00, sizeof(intfStats));
         ret = devCtl_xtmGetInterfaceStatistics((1 << lineId), &intfStats, FALSE);
         if (ret == CMSRET_SUCCESS)
         {
            obj->bytesReceived = (UINT64) intfStats.ulIfInOctets;
            obj->bytesSent = (UINT64)intfStats.ulIfOutOctets;
            obj->packetsReceived = (UINT64)intfStats.ulIfInPackets;
            obj->packetsSent = (UINT64)intfStats.ulIfOutPackets;
            obj->errorsReceived = intfStats.ulIfInPacketErrors;
         }
      }
   }
   else
   {
      /* TR181: if down, cpe must reset statistics */
      obj->bytesSent = 0;
      obj->bytesReceived = 0;
      obj->packetsSent = 0;
      obj->packetsReceived = 0;
      obj->errorsSent = 0;
      obj->errorsReceived = 0;
      obj->discardPacketsSent = 0;
      obj->discardPacketsReceived = 0;
      obj->totalStart = 0;
      obj->showtimeStart = 0;
      obj->lastShowtimeStart = 0;
      obj->currentDayStart = 0;
      obj->quarterHourStart = 0;
   }

   return (ret);
#endif /* DESKTOP_LINUX */
} /* rutdsl_getdslChannelStats_dev2 */

CmsRet rutdsl_getAdslBertInfo_dev2(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   Dev2DslLineBertTestObject *bertObj = (Dev2DslLineBertTestObject *)obj;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered");

   if ((ret = rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL)) == CMSRET_SUCCESS)
   {
      if (adslMib.adslBertStatus.bertSecCur == 0)
      {
         if (cmsUtl_strcmp(bertObj->bertTestStatus, MDMVS_RUNNING) == 0)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(bertObj->bertTestMode,MDMVS_STOP,mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(bertObj->bertTestStatus,MDMVS_NOT_RUNNING,mdmLibCtx.allocFlags);
         }
         else
         {
            ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
         }
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(bertObj->bertTestStatus,MDMVS_RUNNING,mdmLibCtx.allocFlags);
      }
      bertObj->elapsedTime = adslMib.adslBertStatus.bertSecElapsed;
      bertObj->totalTime = adslMib.adslBertStatus.bertSecTotal;
      bertObj->bitsTestedCntHigh = adslMib.adslBertStatus.bertTotalBits.cntHi;
      bertObj->bitsTestedCntLow = adslMib.adslBertStatus.bertTotalBits.cntLo;
      bertObj->errBitsCntHigh = adslMib.adslBertStatus.bertErrBits.cntHi;
      bertObj->errBitsCntLow = adslMib.adslBertStatus.bertErrBits.cntLo;
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   return (ret);
}

CmsRet rutdsl_setAdslBertInfo_dev2(void *new, const void *curr, const InstanceIdStack *iidStack)
{
   Dev2DslLineBertTestObject *newObj = (Dev2DslLineBertTestObject*)new;
   Dev2DslLineBertTestObject *currObj = (Dev2DslLineBertTestObject*)curr;
   UINT32 lineId = 0;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered");

   rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);

   if (cmsUtl_strcmp(newObj->bertTestMode, MDMVS_START) == 0)
   {
      if ((cmsUtl_strcmp(currObj->bertTestStatus, MDMVS_RUNNING) == 0) ||
          (newObj->bertTestDuration < 1))
      {
         /* if test is running or to be run without sufficient input, just return */
         return ret;
      }
      ret = (CmsRet) xdslCtl_BertStartEx(lineId,(unsigned long)newObj->bertTestDuration);
   }
   else
   {
      if (cmsUtl_strcmp(currObj->bertTestStatus, MDMVS_RUNNING) == 0)
      {
         /* stop test */
         ret = (CmsRet) xdslCtl_BertStopEx(lineId);
      }
   }
   return ret;
}

/* rutWan_getIntfInfo is splitted into line and channel info */
CmsRet rutdsl_getLineInfo_dev2(Dev2DslLineObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret = CMSRET_SUCCESS;
   UINT16 xdslStatus=0;
   char value[BUFLEN_32]={0};
   unsigned char pwrState=0;
   int xDsl2Mode = 0;
   adsl2ConnectionInfo *pAdsl2Info = &adslMib.adsl2Info2lp[0];
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
   adsl2ConnectionInfo *pAdsl2Info2 = &adslMib.adsl2Info2lp[1];
   vdsl2ConnectionInfo *pVdsl2Info = &adslMib.vdslInfo[0];
   vdsl2ConnectionInfo *pVdsl2Info2 = &adslMib.vdslInfo[1];
   /* allowed profile */
   char   oidStr[] = { 95 };  /* kOidAdslPhyCfg */
   adslCfgProfile adslCfg;
   long   dataLen = sizeof(adslCfgProfile);
   int len=0;
   char  oidStr1[]={kOidAdslPrivate,kOidAdslPrivExtraInfo,kOidAdslPrivBandPlanDSNegDiscoveryPresentation};
   char  oidStr2[]={kOidAdslPrivate,kOidAdslPrivSNRMusperband};
   bandPlanDescriptor32 usNegBandPlanDiscPresentation,dsNegBandPlanDiscPresentation;

#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */
   adslVersionInfo adslVer;
   UBOOL8 catPhyType = FALSE;
   UBOOL8 gfast = FALSE;

   /* hexbinary of 8 */
   UINT8 XTSUsed[8]={0,0,0,0,0,0,0,0};
   char *XTSUsedStr=NULL;

   if ((obj == NULL) || (iidStack == NULL))
   {
      cmsLog_error("null inputs %p/%p", obj, iidStack);
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("Entered: iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   memset(&adslMib, 0, sizeof(adslMib));
   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   /* get link number (0 or 1) from the instance ID?  This parameter is kept for API purposes */
   ret = (CmsRet) xdslCtl_GetConnectionInfo(obj->X_BROADCOM_COM_BondingLineNumber, &adslConnInfo);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("xdslCtl_GetConnectionInfo failed, ret=%d", ret);
      return ret;
   }

   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);

   /* If the driver returns BCM_XDSL_LINK_DOWN, this is the same as NOSIGNAL and DISABLED, no change is needed.
    * If link is training, and not up yet, we have no choice but to update both instances with such
    * status since we don't know if this is PTM or ATM.
    * If the driver status is BCM_XDSL_LINK_UP, we now know that we are either PTM or ATM,
    * we update the correct instance with the status.
    * Then the other instance will have status updated to DISABLED.
    */
   if (xdslStatus == BCM_ADSL_LINK_DOWN)
   {
      cmsLog_debug("BCM_XDSL_LINK_DOWN (1)");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->linkStatus, MDMVS_NOSIGNAL, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus == BCM_ADSL_TRAINING_G994)
   {
      /* I'm just guessing that this XDSL state maps to initializing */
      cmsLog_debug("BCM_XDSL_TRAINING_G994 (5)");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->linkStatus, MDMVS_INITIALIZING, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus >= BCM_ADSL_TRAINING_G992_EXCHANGE && xdslStatus <= BCM_ADSL_TRAINING_G993_STARTED)
   {
      /* I'm just guessing that these XDSL states map to establishing link */
      cmsLog_debug("xdsl training %d", xdslStatus);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->linkStatus, MDMVS_ESTABLISHINGLINK, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus != BCM_ADSL_LINK_UP)
   {
      /* TR69 also specifies error status.  Don't know which XDSL state
       * corresponds to that. */
      cmsLog_debug("some other kind of status %d", xdslStatus);
      if (cmsUtl_strcmp(obj->status,MDMVS_UP) == 0)
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);

      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->linkStatus, MDMVS_ERROR, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }


   if (xdslStatus == BCM_ADSL_LINK_UP)
   {
      long adslMibSize=sizeof(adslMib);

      cmsLog_debug("BCM_XDSL_LINK_UP (0)");

      /* line 0 and line 1: if not bonding, bondingLineNumber is always 0 */
      ret = (CmsRet) xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, NULL, 0, (char *) &adslMib, &adslMibSize);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get MIB for line %d", obj->X_BROADCOM_COM_BondingLineNumber);
         return CMSRET_INTERNAL_ERROR;
      }

      if(adslMib.adslTrainingState != kAdslTrainingConnected)
      {
         cmsLog_debug("adslTraining state: initializing");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->linkStatus, MDMVS_INITIALIZING, mdmLibCtx.allocFlags);
         return CMSRET_SUCCESS;
      }

      /* it is up, but I need to see if it is FAST mode or not */


      /* modulation type is replaced by standardUsed */
      switch (adslMib.adslConnection.modType)
      {
      case kAdslModAdsl2:
      case kAdslModAdsl2p:
      case kAdslModReAdsl2:
      case kVdslModVdsl2:
         xDsl2Mode = 1;
         break;

#ifdef SUPPORT_DSL_GFAST
      case kXdslModGfast:
         xDsl2Mode = 1;
         gfast = TRUE;
         break;
#endif
#ifdef CONFIG_MGFAST_SUPPORT
      case kXdslModMgfast:
         xDsl2Mode = 1;
         gfast = TRUE;
         break;
#endif
      default:
         xDsl2Mode = 0;
         break;
      } /* modulationType */

      if (gfast == FALSE)
      {
         /* if it's gfast, everything will be read and updated, except for the status because the status for FAST is in FAST object */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->linkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }

      /* Get CO vendor id and country code from adslMib. */
      {
         char vendorId[16]={0};

         cmsUtl_encodeHexStr(vendorId, 16, adslMib.xdslAtucPhys.adslVendorID, 2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCCountry, vendorId, mdmLibCtx.allocFlags);

         memset(vendorId, 0, sizeof(vendorId));
         cmsUtl_encodeHexStr(vendorId, 16, &(adslMib.xdslAtucPhys.adslVendorID[2]), 4);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCVendor, vendorId, mdmLibCtx.allocFlags);

#ifdef DMP_X_RDK_DSL_1
         // The following are only available in RDK builds until they are officially
         // standardized by BBF.  See adslctl.c for code which prints this stuff out.
         memset(vendorId, 0, sizeof(vendorId));
         cmsUtl_encodeHexStr(vendorId, 16, &(adslMib.xdslAtucPhys.adslVendorID[6]), 2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCVendorSpecific, vendorId, mdmLibCtx.allocFlags);

         memset(vendorId, 0, sizeof(vendorId));
         cmsUtl_encodeHexStr(vendorId, sizeof(vendorId), adslMib.xdslAtucPhys.adslSysVendorID, 2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCSystemCountry, vendorId, mdmLibCtx.allocFlags);
         cmsLog_debug("SystemCountry=%s", obj->XTUCSystemCountry);

         memset(vendorId, 0, sizeof(vendorId));
         cmsUtl_encodeHexStr(vendorId, sizeof(vendorId), &(adslMib.xdslAtucPhys.adslSysVendorID[2]), 4);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCSystemVendor, vendorId, mdmLibCtx.allocFlags);
         cmsLog_debug("SystemVendor=%s", obj->XTUCSystemVendor);

         memset(vendorId, 0, sizeof(vendorId));
         cmsUtl_encodeHexStr(vendorId, sizeof(vendorId), &(adslMib.xdslAtucPhys.adslSysVendorID[6]), 2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCSystemVendorSpecific, vendorId, mdmLibCtx.allocFlags);
         cmsLog_debug("SystemVendorSpec=%s", obj->XTUCSystemVendorSpecific);

         // XTUCSerial: data model says string maxLength=32, so 32 real bytes + 1 null termination char
         // Note that it might take a few seconds after SHOWTIME for the CO to
         // send this info to the CPE.  From driver, there is
         // "Chipset Serial Number" from adslMib.xdslAtucPhys.adslSerialNumber, kAdslPhysSerialNumLen 32
         {
            char strBuf[33]={0};
            snprintf(strBuf, sizeof(strBuf), adslMib.xdslAtucPhys.adslSerialNumber);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCSerial, strBuf, mdmLibCtx.allocFlags);
            cmsLog_debug("ChipSet SerialNumber: %s", adslMib.xdslAtucPhys.adslSerialNumber);
            cmsLog_debug("XTUCSerial=%s", strBuf);
         }

         // XTUCVersion: data model says hexBinary, max and min len of 16 octets or 32 HexBinary digits.
         // This is reported by the driver as a string of len 32, and already in hex format, e.g.
         // ChipSet VersionNumber:	0xc1f0
         // so skip over the initial "0x" and copy the hexBinary as is until end of string,
         // and pad with hexBinary 0 to the full 32 HexBinary digits
         // In AdslXfaceData.h: uchar		eocVersion[kAdslOemVersionMaxSize];  kAdslOemVersionMaxSize		32
         {
            UINT32 sLen=strlen(adslMib.xdslAtucPhys.adslVersionNumber);
            UINT32 s=0;
            char hexBuf[33]={0};
            UINT32 h=0;

            // initialize with HexBinary '0'
            memset(hexBuf, '0', 32);

            if ((adslMib.xdslAtucPhys.adslVersionNumber[0] == '0') &&
                (adslMib.xdslAtucPhys.adslVersionNumber[1] == 'x'))
            {
               s = 2;  // skip over initial "0x" reported by driver
            }
            while ((h < 32) && (s < sLen))
            {
               hexBuf[h++] = adslMib.xdslAtucPhys.adslVersionNumber[s++];
            }
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTUCVersion, hexBuf, mdmLibCtx.allocFlags);
            cmsLog_debug("ChipSet VersionNumber: %s", adslMib.xdslAtucPhys.adslVersionNumber);
            cmsLog_debug("XTUCVersion=%s", obj->XTUCVersion);
         }
#endif  /* DMP_X_RDK_DSL_1 */
      }

      switch (adslMib.adslLine.adslLineCoding)
      {
      case kAdslLineCodingDMT:
         cmsLog_debug("kAdslLineCodingDMT");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->lineEncoding,MDMVS_DMT,mdmLibCtx.allocFlags);
         break;
      case kAdslLineCodingCAP:
         cmsLog_debug("kAdslLineCodingCAP");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->lineEncoding,MDMVS_CAP,mdmLibCtx.allocFlags);
         break;
      case kAdslLineCodingQAM:
         cmsLog_debug("kAdslLineCodingQAM");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->lineEncoding,MDMVS_QAM,mdmLibCtx.allocFlags);
         break;
      default:
         cmsLog_debug("adslLineInfo.adslLineCoding %d",adslMib.adslLine.adslLineCoding);
      } /* lineCoding */

      if( adslMib.adslConnection.modType < kAdslModAdsl2 )
      {
         if( kAdslTrellisOn == adslMib.adslConnection.trellisCoding )
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
      }
      else
      {
         if( adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled )
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
         if( adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled )
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisU, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisU, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
      }


      pwrState = adslMib.xdslInfo.pwrState;
      if ( 0 == pwrState )
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L0,mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->powerManagementState,MDMVS_L0,mdmLibCtx.allocFlags);
      }
      else if ( 2 == pwrState )
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L2,mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->powerManagementState,MDMVS_L2,mdmLibCtx.allocFlags);
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L3,mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->powerManagementState,MDMVS_L3,mdmLibCtx.allocFlags);
      }

      cmsLog_debug("Power state: %s",obj->X_BROADCOM_COM_LinkPowerState);

      obj->downstreamNoiseMargin = adslMib.adslPhys.adslCurrSnrMgn;
      obj->upstreamNoiseMargin = adslMib.adslAtucPhys.adslCurrSnrMgn;
#ifdef DMP_DEVICE2_VDSL2_1
      obj->downstreamAttenuation = adslMib.adslPhys.adslCurrAtn;
      obj->upstreamAttenuation = adslMib.adslAtucPhys.adslCurrAtn;
#endif
      obj->downstreamPower = adslMib.adslAtucPhys.adslCurrOutputPwr;
      obj->upstreamPower = adslMib.adslPhys.adslCurrOutputPwr;
      obj->downstreamMaxBitRate = adslMib.adslPhys.adslCurrAttainableRate / 1000;
      obj->upstreamMaxBitRate = adslMib.adslAtucPhys.adslCurrAttainableRate / 1000;

      /* xDSL2 framing parameters */
      if ( xDsl2Mode )
      {
         xdslFramingInfo     *pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[0];
         xdslFramingInfo     *pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];

         cmsLog_debug("xDSL2 framing");

         /* Delay and INP for path 0 */
         snprintf(value, sizeof(value), "%d", pRxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%d", pTxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%4.2f", (float)pRxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%4.2f", (float)pTxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP, value, mdmLibCtx.allocFlags);

#if !defined(DMP_VDSL2WAN_1) && !defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
         /* ADSL2/ADSL2+ specific mode */
         obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc = pAdsl2Info->rcv2Info.MSGc;
         obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc = pAdsl2Info->xmt2Info.MSGc;
         obj->X_BROADCOM_COM_ADSL2_DownstreamB = pAdsl2Info->rcv2Info.B;
         obj->X_BROADCOM_COM_ADSL2_UpstreamB = pAdsl2Info->xmt2Info.B;
         obj->X_BROADCOM_COM_ADSL2_DownstreamM = pAdsl2Info->rcv2Info.M;
         obj->X_BROADCOM_COM_ADSL2_UpstreamM = pAdsl2Info->xmt2Info.M;
         obj->X_BROADCOM_COM_ADSL2_DownstreamT = pAdsl2Info->rcv2Info.T;
         obj->X_BROADCOM_COM_ADSL2_UpstreamT = pAdsl2Info->xmt2Info.T;
         obj->X_BROADCOM_COM_ADSL2_DownstreamR = pAdsl2Info->rcv2Info.R;
         obj->X_BROADCOM_COM_ADSL2_UpstreamR = pAdsl2Info->xmt2Info.R;

         snprintf(value, BUFLEN_32, "%d.%04d",
                  (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000),
                  (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000));
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS,value,mdmLibCtx.allocFlags);
         snprintf(value, BUFLEN_32, "%d.%04d",
                  (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000),
                  (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000));
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS,value,mdmLibCtx.allocFlags);

         obj->X_BROADCOM_COM_ADSL2_DownstreamL = pAdsl2Info->rcv2Info.L;
         obj->X_BROADCOM_COM_ADSL2_UpstreamL = pAdsl2Info->xmt2Info.L;
         obj->X_BROADCOM_COM_DownstreamD = pAdsl2Info->rcv2Info.D;
         obj->X_BROADCOM_COM_UpstreamD = pAdsl2Info->xmt2Info.D;

         /* 2nd latency */
         obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamB_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamB_2= 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamM_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamM_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamT_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamT_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamR_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamR_2 = 0;

         snprintf(value, BUFLEN_32, "%d.%d", 0, 0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS_2, value,mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS_2, value,mdmLibCtx.allocFlags);

         obj->X_BROADCOM_COM_ADSL2_DownstreamL_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamL_2 = 0;
         obj->X_BROADCOM_COM_DownstreamD_2 = 0;
         obj->X_BROADCOM_COM_UpstreamD_2 = 0;

         snprintf(value, sizeof(value), "%d.%d", 0, 0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay_2, value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay_2, value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP_2, value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP_2, value, mdmLibCtx.allocFlags);
#else
         /* Delay and INP for path 1 */
         pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[1];
         pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[1];
         snprintf(value, sizeof(value), "%d", pRxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay_2, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%d", pTxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay_2, value, mdmLibCtx.allocFlags);

         snprintf(value, sizeof(value), "%4.2f", (float)pRxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP_2, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%4.2f", (float)pTxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP_2, value, mdmLibCtx.allocFlags);

         if ( (kVdslModVdsl2 != adslMib.adslConnection.modType)
#ifdef SUPPORT_DSL_GFAST
               && (kXdslModGfast != adslMib.adslConnection.modType)
#endif
#ifdef CONFIG_MGFAST_SUPPORT
               && (kXdslModMgfast != adslMib.adslConnection.modType)
#endif
            ) {
            /* ADSL2/ADSL2+ Mode */
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentProfile, "", mdmLibCtx.allocFlags);

            obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc = pAdsl2Info->rcv2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc = pAdsl2Info->xmt2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_DownstreamB = pAdsl2Info->rcv2Info.B;
            obj->X_BROADCOM_COM_ADSL2_UpstreamB = pAdsl2Info->xmt2Info.B;
            obj->X_BROADCOM_COM_ADSL2_DownstreamM = pAdsl2Info->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM = pAdsl2Info->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT = pAdsl2Info->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT = pAdsl2Info->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR = pAdsl2Info->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR = pAdsl2Info->xmt2Info.R;

            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS,value,mdmLibCtx.allocFlags);
            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS,value,mdmLibCtx.allocFlags);

            obj->X_BROADCOM_COM_ADSL2_DownstreamL = pAdsl2Info->rcv2Info.L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL = pAdsl2Info->xmt2Info.L;
            obj->X_BROADCOM_COM_DownstreamD = pAdsl2Info->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD = pAdsl2Info->xmt2Info.D;

            /* 2nd latency */
            obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc_2 = pAdsl2Info2->rcv2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc_2 = pAdsl2Info2->xmt2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_DownstreamB_2 = pAdsl2Info2->rcv2Info.B;
            obj->X_BROADCOM_COM_ADSL2_UpstreamB_2 = pAdsl2Info2->xmt2Info.B;
            obj->X_BROADCOM_COM_ADSL2_DownstreamM_2 = pAdsl2Info2->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM_2 = pAdsl2Info2->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT_2 = pAdsl2Info2->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT_2 = pAdsl2Info2->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR_2 = pAdsl2Info2->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR_2 = pAdsl2Info2->xmt2Info.R;

            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info2->rcv2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info2->rcv2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS_2,value,mdmLibCtx.allocFlags);
            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info2->xmt2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info2->xmt2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS_2,value,mdmLibCtx.allocFlags);

            obj->X_BROADCOM_COM_ADSL2_DownstreamL_2 = pAdsl2Info2->rcv2Info.L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL_2 = pAdsl2Info2->xmt2Info.L;
            obj->X_BROADCOM_COM_DownstreamD_2 = pAdsl2Info2->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD_2 = pAdsl2Info2->xmt2Info.D;
         }
         else {
            /* VDSL2 Mode */
            pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[0];
            pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamB = pVdsl2Info->rcv2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_UpstreamB = pVdsl2Info->xmt2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamM = pVdsl2Info->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM = pVdsl2Info->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT = pVdsl2Info->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT = pVdsl2Info->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR = pVdsl2Info->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR = pVdsl2Info->xmt2Info.R;

            snprintf(value, sizeof(value), "%3.4f", (pRxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS,value,mdmLibCtx.allocFlags);
            snprintf(value, sizeof(value), "%3.4f", (pTxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS,value,mdmLibCtx.allocFlags);

            obj->X_BROADCOM_COM_ADSL2_DownstreamL = pRxFramingParam->L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL = pTxFramingParam->L;
            obj->X_BROADCOM_COM_DownstreamD = pVdsl2Info->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD = pVdsl2Info->xmt2Info.D;
            obj->X_BROADCOM_COM_VDSL_DownstreamI = pVdsl2Info->rcv2Info.I;
            obj->X_BROADCOM_COM_VDSL_UpstreamI = pVdsl2Info->xmt2Info.I;
            obj->X_BROADCOM_COM_VDSL_DownstreamN = pVdsl2Info->rcv2Info.N;
            obj->X_BROADCOM_COM_VDSL_UpstreamN = pVdsl2Info->xmt2Info.N;

            /* 2nd latency */
            pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[0];
            pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamB_2 = pVdsl2Info2->rcv2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_UpstreamB_2 = pVdsl2Info2->xmt2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamM_2 = pVdsl2Info2->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM_2 = pVdsl2Info2->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT_2 = pVdsl2Info2->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT_2 = pVdsl2Info2->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR_2 = pVdsl2Info2->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR_2 = pVdsl2Info2->xmt2Info.R;
            snprintf(value, sizeof(value), "%3.4f", (pRxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS_2,value,mdmLibCtx.allocFlags);
            snprintf(value, sizeof(value), "%3.4f", (pTxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS_2,value,mdmLibCtx.allocFlags);

            obj->X_BROADCOM_COM_ADSL2_DownstreamL_2 = pRxFramingParam->L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL_2 = pTxFramingParam->L;
            obj->X_BROADCOM_COM_DownstreamD_2 = pVdsl2Info2->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD_2 = pVdsl2Info2->xmt2Info.D;
            obj->X_BROADCOM_COM_VDSL_DownstreamI_2 = pVdsl2Info2->rcv2Info.I;
            obj->X_BROADCOM_COM_VDSL_UpstreamI_2 = pVdsl2Info2->xmt2Info.I;
            obj->X_BROADCOM_COM_VDSL_DownstreamN_2 = pVdsl2Info2->rcv2Info.N;
            obj->X_BROADCOM_COM_VDSL_UpstreamN_2 = pVdsl2Info2->xmt2Info.N;

            /* current profile */
            cmsLog_debug("vdsl2Profile=0x%x", pVdsl2Info->vdsl2Profile);
            switch (pVdsl2Info->vdsl2Profile)
            {
            case kVdslProfile8a:
               strcpy(value,"8a");
               break;
            case kVdslProfile8b:
               strcpy(value,"8b");
               break;
            case kVdslProfile8c:
               strcpy(value,"8c");
               break;
            case kVdslProfile8d:
               strcpy(value,"8d");
               break;
            case kVdslProfile12a:
               strcpy(value,"12a");
               break;
            case kVdslProfile12b:
               strcpy(value,"12b");
               break;
            case kVdslProfile17a:
               strcpy(value,"17a");
               break;
            case kVdslProfile30a:
               strcpy(value,"30a");
               break;
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
            case kVdslProfile35b:
               strcpy(value,"35b");
               break;
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
            case kVdslProfileBrcmPriv2:
               strcpy(value,MDMVS_X_BROADCOM_COM_PRIV2);
               break;
#endif
            default:
               /* this parameter is irrelevant for gfast */
               if (!gfast)
               {
                  cmsLog_error("unrecognized profile 0x%x (set to 8a)", pVdsl2Info->vdsl2Profile);
               }
               strcpy(value,"8a");
               break;
            } /* switch vdsl2Profile */

            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentProfile, value, mdmLibCtx.allocFlags);

            memset(&adslCfg, 0, sizeof(adslCfg));
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber,oidStr,sizeof(oidStr),(char*)&adslCfg, &dataLen);
            value[0] = '\0';
            if (adslCfg.vdslParam & kVdslProfile8a)
            {
               strcat(value,"8a,");
            }
            if (adslCfg.vdslParam & kVdslProfile8b)
            {
               strcat(value,"8b,");
            }
            if (adslCfg.vdslParam & kVdslProfile8c)
            {
               strcat(value,"8c,");
            }
            if (adslCfg.vdslParam & kVdslProfile8d)
            {
               strcat(value,"8d,");
            }
            if (adslCfg.vdslParam & kVdslProfile12a)
            {
               strcat(value,"12a,");
            }
            if (adslCfg.vdslParam & kVdslProfile12b)
            {
               strcat(value,"12b,");
            }
            if (adslCfg.vdslParam & kVdslProfile17a)
            {
               strcat(value,"17a,");
            }
            if (adslCfg.vdslParam & kVdslProfile30a)
            {
               strcat(value,"30a,");
            }
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
            if (adslCfg.vdslParam & kVdslProfile35b)
            {
               strcat(value,"35b,");
            }
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
            if (adslCfg.vdslParam & kVdslProfileBrcmPriv2)
            {
               strcat(value,"X_BROADCOM_COM_Priv2,");
            }
#endif
            /* wipe out the last , */
            len = strlen(value);
            value[len-1] = '\0';
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->allowedProfiles, value, mdmLibCtx.allocFlags);

            /* The following code are based on adslctl code to gather data from driver */
            obj->ACTSNRMODEds = adslMib.xdslAtucPhys.SNRmode;
            obj->ACTSNRMODEus = adslMib.xdslPhys.SNRmode;
            obj->ACTUALCE = adslMib.xdslPhys.actualCE;

            /* TR98 defines a range value (0 to 1280) for parameter UPBOKLE.
             * The value SHALL be coded as an unsigned 16 bit number in the range
             * 0 (coded as 0) to 128 dB (coded as 1280) in steps of 0.1 dB.
             * AdslMib UPBOkle is reported by DSL/PHY driver in steps of 0.01 dB,
             * instead of 0.1 dB. That makes the UPBOkle value 10 times bigger.
             * Therefore, we need to devide the value of UPBOkle by 10. 
             */
            obj->UPBOKLE = adslMib.xdslPhys.UPBOkle / 10;
            if (obj->UPBOKLE > 1280)
            {
               /* adslMib UPBOkle exceeds max value defined by TR98.
                * Set the parameter to max so that when cmsObj_get()
                * is called to get the DslIntfCfg object, mdm will not
                * detect an out-of-range value and return error.
                */
               cmsLog_error("adslMib UPBOkle %d is out of range. Set DslIntfCfg UPBOKLE to max 1280.", obj->UPBOKLE);
               obj->UPBOKLE = 1280;
            }

            /* these are per band data */
            /* first get dsNegBandPlanDiscPresentation */
            dataLen=sizeof(bandPlanDescriptor32);
            memset(&dsNegBandPlanDiscPresentation, 0, dataLen);
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr1, sizeof(oidStr1), (char *)&dsNegBandPlanDiscPresentation, &dataLen);

            /* get usNegBandPlanDiscPresentation*/
            oidStr1[2]=kOidAdslPrivBandPlanUSNegDiscoveryPresentation;
            dataLen=sizeof(bandPlanDescriptor32);
            memset(&usNegBandPlanDiscPresentation, 0, dataLen);
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr1, sizeof(oidStr1), (char *)&usNegBandPlanDiscPresentation, &dataLen);

            {
               char dataStr[BUFLEN_64]={0};
               // Use mem alloc for data array to get best alignment for system
               short *data= (short *) cmsMem_alloc(NUM_BAND * sizeof(short), 0);
               if (data != NULL)
               {
                  /* get kOidAdslPrivSNRMusperband and format into string */
                  oidStr2[1]=kOidAdslPrivSNRMusperband;
                  dataLen = NUM_BAND * sizeof(short);
                  memset(data, 0, dataLen);
                  xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr2, sizeof(oidStr2), (char *)data, &dataLen);
                  ret = (CmsRet) cmsAdsl_formatSnrmUsString(&usNegBandPlanDiscPresentation,
                                                   data,
                                                   dataStr, sizeof(dataStr));
                  if (ret == CMSRET_SUCCESS)
                  {
                     REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->SNRMpbus,dataStr,mdmLibCtx.allocFlags);
                  }
                  else
                  {
                     cmsLog_error("format usNegBandPlan, ret=%d", ret);
                  }

                  /* get kOidAdslPrivSNRMdsperband and format into string */
                  oidStr2[1]=kOidAdslPrivSNRMdsperband;
                  dataLen = NUM_BAND * sizeof(short);
                  memset(data, 0, dataLen);
                  xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr2, sizeof(oidStr2), (char *)data, &dataLen);
                  ret = (CmsRet) cmsAdsl_formatSnrmDsString(&dsNegBandPlanDiscPresentation,
                                                   data,
                                                   dataStr, sizeof(dataStr));
                  if (ret == CMSRET_SUCCESS)
                  {
                     REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->SNRMpbds,dataStr,mdmLibCtx.allocFlags);
                  }
                  else
                  {
                     cmsLog_error("format dsNegBandPlanDisc, ret=%d", ret);
                  }

                  cmsMem_free(data);
               }
               else
               {
                  cmsLog_error("failed to allocate %d bytes",
                               NUM_BAND * sizeof(short));
               }
            }
         }

#endif /* !defined(DMP_VDSL2WAN_1) && !defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */
      } /* xDSLl2 */
      else
      {
         int Nds, Nus, Lds, Lus, Rds;
         float Sds, INPds, INPus;
         cmsLog_debug("Not xDSLl2");
         if ((!((0 == adslMib.adslConnection.rcvInfo.R) && (0 != adslMib.adslConnection.rcvInfo.D))) &&
                (255 >= adslMib.adslConnection.rcvInfo.K) && (adslMib.adslConnection.rcvInfo.S!=0))
         {
             Nds = (adslMib.adslConnection.rcvInfo.K*adslMib.adslConnection.rcvInfo.S+adslMib.adslConnection.rcvInfo.R)/adslMib.adslConnection.rcvInfo.S;
             Lds = (Nds<<3)*adslMib.adslConnection.rcvInfo.S;
             if (0 != Lds)
                 INPds = (float)(adslMib.adslConnection.rcvInfo.R*adslMib.adslConnection.rcvInfo.D<<2)/(float)Lds;
             else
                 INPds = -1;
             Sds = (float)(adslMib.adslConnection.rcvInfo.S);
             Rds = adslMib.adslConnection.rcvInfo.R;
         }
         else
         {
             /* detect S=0.5 */
             if (255 > adslMib.adslConnection.rcvInfo.K)
                 Nds = adslMib.adslConnection.rcvInfo.K+(adslMib.adslConnection.rcvInfo.R<<1);
             else
                 Nds = (adslMib.adslConnection.rcvInfo.K+adslMib.adslConnection.rcvInfo.R)>>1;
             Lds = Nds<<4;
             if (0 != Lds)
                 INPds = (float)(adslMib.adslConnection.rcvInfo.R*adslMib.adslConnection.rcvInfo.D<<1)/(float)Lds;
             else
                 INPds = -1.0;
             Sds = 0.5;
             Rds = adslMib.adslConnection.rcvInfo.R>>1;
         }
         Nus = adslMib.adslConnection.xmtInfo.K+adslMib.adslConnection.xmtInfo.R;
         Lus = (Nus<<3)*adslMib.adslConnection.xmtInfo.S;
         if (Lus!=0)
             INPus = (float)(adslMib.adslConnection.xmtInfo.R*adslMib.adslConnection.xmtInfo.D<<2)/(float)Lus;
         else
             INPus = -1.0;

         obj->X_BROADCOM_COM_DownstreamK = adslMib.adslConnection.rcvInfo.K;
         obj->X_BROADCOM_COM_UpstreamK = adslMib.adslConnection.xmtInfo.K;

         obj->X_BROADCOM_COM_DownstreamR = Rds;
         obj->X_BROADCOM_COM_UpstreamR = adslMib.adslConnection.xmtInfo.R;

         snprintf(value,BUFLEN_32,"%4.2f", Sds);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamS,value,mdmLibCtx.allocFlags);
         snprintf(value, BUFLEN_32,"%4.2f",(float)adslMib.adslConnection.xmtInfo.S);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamS,value,mdmLibCtx.allocFlags);

         obj->X_BROADCOM_COM_DownstreamD = adslMib.adslConnection.rcvInfo.D;
         obj->X_BROADCOM_COM_UpstreamD = adslMib.adslConnection.xmtInfo.D;


         snprintf(value,BUFLEN_32,"%4.2f", (Sds* (float)adslMib.adslConnection.rcvInfo.D)/4.0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay,value,mdmLibCtx.allocFlags);

         snprintf(value,BUFLEN_32,"%4.2f",(float)((adslMib.adslConnection.xmtInfo.S* adslMib.adslConnection.xmtInfo.D))/4.0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay,value,mdmLibCtx.allocFlags);

         if (-1.0 != INPds)
             snprintf(value,BUFLEN_32, "%4.2f", INPds);
         else
             snprintf(value,BUFLEN_32, "n/a");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP,value,mdmLibCtx.allocFlags);
         if (-1.0 != INPus)
             snprintf(value,BUFLEN_32, "%4.2f", INPus);
         else
             snprintf(value,BUFLEN_32, "n/a");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP,value,mdmLibCtx.allocFlags);

      } /* !adsl2 */
      /* Values pending on PHY/driver */
      obj->XTURANSIStd = 0;
      obj->XTURANSIRev = 0;
      obj->XTUCANSIStd = 0;
      obj->XTUCANSIRev = 0;

#ifdef DMP_DEVICE2_VDSL2_1
      obj->TRELLISds = (adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled) ? kAdslTrellisOn : kAdslTrellisOff;
      obj->TRELLISus = (adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled) ? kAdslTrellisOn : kAdslTrellisOff;
#endif

      memset((void*)&adslVer, 0, sizeof(adslVer));
      xdslCtl_GetVersion(0, &adslVer);
      switch (adslMib.adslConnection.modType)
      {
      case kAdslModGdmt:
         strcpy(value,"G.992.1");
         catPhyType = TRUE;
         break;
      case kAdslModT1413:
         strcpy(value,"T1.413");
         XTSUsed[0] = XTSE_REGIONAL_T1413;
         break;
      case kAdslModGlite:
         strcpy(value,"G.992.2");
         XTSUsed[1] = XTSE_G992_2_ANNEX_A_NONOVERLAPPED;
         break;
      case kAdslModAnnexI:
         strcpy(value,"G.992.3_Annex_I");
         XTSUsed[3] = XTSE_G992_3_ANNEX_I_NONOVERLAPPED;
         break;
      case kAdslModAdsl2:
         strcpy(value,"G.992.3");
         catPhyType = TRUE;
         break;
      case kAdslModAdsl2p:
         strcpy(value,"G.992.5");
         catPhyType = TRUE;
         break;
      case kAdslModReAdsl2:
         strcpy(value,"G.992.3_Annex_L");
         XTSUsed[4] = XTSE_G992_3_ANNEX_L_MODE1;
         break;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      case kVdslModVdsl2:
         strcpy(value,"G.993.2");
         switch((adslMib.adsl2Info.adsl2Mode)>>  kXdslModeAnnexShift)
         {
         case kAdslTypeAnnexB:
            strcat(value,"_Annex_B");
            XTSUsed[7] = XTSE_G993_2_REGION_B;
            break;
         case kAdslTypeAnnexC:
            strcat(value,"_Annex_C");
            XTSUsed[7] = XTSE_G993_2_REGION_C;
            break;
         default:
            strcat(value,"_Annex_A");
            XTSUsed[7] = XTSE_G993_2_REGION_A;
            break;
         } /* xdsl */
         break;
#ifdef SUPPORT_DSL_GFAST
         /* really, this does not belong here because G.997.1 is for DSL only.  XTSE and XTUsed 
          * (StandardsSupported/StandardUsed) does not have g.fast.
          * In G.997.2 (Physcial Layer Management for G.fast) doesn't have XTSE
          */
      case kXdslModGfast:
         strcpy(value,"G.9701");
         break;
#endif
#ifdef CONFIG_MGFAST_SUPPORT
      case kXdslModMgfast:
         strcpy(value,"G.9711");
         break;
#endif
#endif
      } /* modType */

      if (catPhyType == TRUE)
      {
         /* TR181 enumeration */
         if (adslVer.phyType == kAdslTypeAnnexA)
         {
            strcat(value,"_Annex_A");
            if (strstr(value,"G.992.1") != NULL)
            {
               XTSUsed[7] = XTSE_G992_1_ANNEX_A_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[5] = XTSE_G992_3_ANNEX_A_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.5") != NULL)
            {
               XTSUsed[2] = XTSE_G992_5_ANNEX_A_NONOVERLAPPED;
            }
         }
         else if (adslVer.phyType == kAdslTypeAnnexB)
         {
            strcat(value,"_Annex_B");
            if (strstr(value,"G.992.1") != NULL)
            {
               XTSUsed[7] = XTSE_G992_1_ANNEX_B_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[5] = XTSE_G992_3_ANNEX_B_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.5") != NULL)
            {
               XTSUsed[2] = XTSE_G992_5_ANNEX_B_NONOVERLAPPED;
            }
         }
         else if (adslVer.phyType == kAdslTypeAnnexC)
         {
            strcat(value,"_Annex_C");
            if (strstr(value,"G.992.1") != NULL)
            {
               XTSUsed[7] = XTSE_G992_1_ANNEX_C_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[5] = XTSE_G992_3_ANNEX_C_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.5") != NULL)
            {
               XTSUsed[2] = XTSE_G992_5_ANNEX_C_NONOVERLAPPED;
            }
         }
         else if (adslVer.phyType == kAdslTypeAnnexI)
         {
            strcat(value,"_Annex_I");
            if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[4] = XTSE_G992_3_ANNEX_I_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.5") != NULL)
            {
               XTSUsed[2] = XTSE_G992_5_ANNEX_I_NONOVERLAPPED;
            }
         }
         else if (adslVer.phyType == kAdslTypeAnnexJ)
         {
            strcat(value,"_Annex_J");
            if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[4] = XTSE_G992_3_ANNEX_J_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.5") != NULL)
            {
               XTSUsed[1] = XTSE_G992_5_ANNEX_J_NONOVERLAPPED;
            }
         }
         else if (adslVer.phyType == kAdslTypeAnnexL)
         {
            strcat(value,"_Annex_L");
            if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[3] = XTSE_G992_3_ANNEX_L_MODE2;
            }
         }
         else if (adslVer.phyType == kAdslTypeAnnexM)
         {
            strcat(value,"_Annex_M");
            if (strstr(value,"G.992.3") != NULL)
            {
               XTSUsed[3] = XTSE_G992_3_ANNEX_M_NONOVERLAPPED;
            }
            else if (strstr(value,"G.992.5") != NULL)
            {
               XTSUsed[1] = XTSE_G992_5_ANNEX_M_NONOVERLAPPED;
            }
         }
      }
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->standardUsed,value,mdmLibCtx.allocFlags);
      /* when XTSUsed is used, standardUsed MAY be empty.  Since it is not must be empty, I just keep standardUsed as is.
       * XTSE and XTUsed is defined in G.997.1 (Physical Layer Management for DSL management)
       */
      ret = cmsUtl_binaryBufToHexString((const UINT8*)XTSUsed, sizeof(XTSUsed), &XTSUsedStr);
      if (ret == CMSRET_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->XTSUsed,XTSUsedStr,mdmLibCtx.allocFlags);
         cmsMem_free(XTSUsedStr);
      }
   } /* if up */

   cmsLog_debug("Exit: ret %d", ret);
   return (ret);
} /* get dslLineInfo */

CmsRet rutdsl_getChannelInfo_dev2(Dev2DslChannelObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret = CMSRET_SUCCESS;
   UINT16 xdslStatus = 0;
   int xDsl2Mode = 0;
   MdmPathDescriptor pathDesc;
   Dev2DslLineObject *dslLineObj = NULL;
   int bondingLineNumber=0;

   if (obj == NULL)
   {
      return ret;
   }

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));


   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));

   /* first get the line object underneath this channel */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   if((ret = cmsMdm_fullPathToPathDescriptor(obj->lowerLayers, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor for %s failed, ret=%d",
                   obj->lowerLayers, ret);
      return ret;
   }
   if ((ret = cmsObj_get(MDMOID_DEV2_DSL_LINE,&pathDesc.iidStack,0,(void **)&dslLineObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get %s object for this channel failed, ret=%d",
                   obj->lowerLayers, ret);
      return ret;
   }
   bondingLineNumber = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
   cmsObj_free((void **) &dslLineObj);

   ret = (CmsRet) xdslCtl_GetConnectionInfo(bondingLineNumber, &adslConnInfo);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("xdslCtl_GetConnectionInfo failed, ret=%d", ret);
      return ret;
   }

   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);

   if (xdslStatus != BCM_ADSL_LINK_UP)
   {
      cmsLog_debug("Down, return unchanged");
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   memset(&adslMib, 0, sizeof(adslMib));
   long adslMibSize=sizeof(adslMib);

   cmsLog_debug("BCM_XDSL_LINK_UP (0)");

   ret = (CmsRet) xdslCtl_GetObjectValue(bondingLineNumber, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line %d", bondingLineNumber);
      return CMSRET_INTERNAL_ERROR;
   }
   if(adslMib.adslTrainingState != kAdslTrainingConnected) 
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   // Line is up, training is done, now can can fill in channel data
   obj->downstreamCurrRate = GetRcvRate(&adslMib, 0);
   obj->upstreamCurrRate = GetXmtRate(&adslMib, 0);
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
   if(xDsl2Mode) {
      obj->X_BROADCOM_COM_DownstreamCurrRate_2 = GetRcvRate(&adslMib, 1);
      obj->X_BROADCOM_COM_UpstreamCurrRate_2 = GetXmtRate(&adslMib, 1);
   }
   else
#endif
   {
      obj->X_BROADCOM_COM_DownstreamCurrRate_2 = 0;
      obj->X_BROADCOM_COM_UpstreamCurrRate_2 = 0;
   }

   obj->LPATH = 0;  // Always 0 since we only support single latency.
   // Since we are CPE, use US_DIRECTION
   obj->INTLVDEPTH = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].D;
   obj->INTLVBLOCK = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].N;
   obj->actualInterleavingDelay = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].delay;
   obj->ACTINP = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].INP/2;
   obj->INPREPORT = 0;  // Always 0 since we do not use erasure decoding.
   obj->NFEC = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].N;  // confirmed this is same as INTLVBLOCK.
   obj->RFEC = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].R;
   obj->LSYMB = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].L;
   obj->ACTNDR = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].dataRate;
   obj->ACTINPREIN = adslMib.xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].INPrein/2;

   return ret;
} /* get dslChannelInfo */

/* channelTotalStats */
CmsRet rutdsl_getTotalChannelStats_dev2(Dev2DslChannelStatsTotalObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->X_BROADCOM_COM_ReceiveBlocks = adslMib.xdslChanPerfData[0].perfTotal.adslChanReceivedBlks;
      obj->X_BROADCOM_COM_TransmitBlocks = adslMib.xdslChanPerfData[0].perfTotal.adslChanTransmittedBlks;
      obj->XTUCFECErrors = adslMib.adslStatSincePowerOn.xmtStat.cntRSCor;
      obj->XTURFECErrors = adslMib.adslPerfData.perfTotal.adslFECs;
      obj->X_BROADCOM_COM_RxRsUncorrectable = adslMib.xdslStat[0].rcvStat.cntRSUncor;
      obj->X_BROADCOM_COM_TxRsUncorrectable = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable = adslMib.xdslStat[0].rcvStat.cntRSCor;;
      obj->X_BROADCOM_COM_TxRsCorrectable = adslMib.xdslStat[0].xmtStat.cntRSCor;
      obj->X_BROADCOM_COM_RxRsWords = adslMib.xdslStat[0].rcvStat.cntRS;
      obj->X_BROADCOM_COM_TxRsWords = adslMib.xdslStat[0].xmtStat.cntRS;
      obj->XTURHECErrors = adslMib.atmStatSincePowerOn.rcvStat.cntHEC;
      obj->XTUCHECErrors = adslMib.atmStatSincePowerOn.xmtStat.cntHEC;
      obj->XTURCRCErrors = adslMib.adslStatSincePowerOn.rcvStat.cntSFErr;
      obj->XTUCCRCErrors = adslMib.adslStatSincePowerOn.xmtStat.cntSFErr;
      obj->X_BROADCOM_COM_DownstreamOCD = adslMib.atmStat2lp[0].rcvStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamOCD = adslMib.atmStat2lp[0].xmtStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamLCD = adslMib.atmStat2lp[0].xmtStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamLCD = adslMib.atmStat2lp[0].rcvStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamTotalCells = adslMib.atmStat2lp[0].rcvStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamTotalCells = adslMib.atmStat2lp[0].xmtStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamDataCells = adslMib.atmStat2lp[0].xmtStat.cntCellData;
      obj->X_BROADCOM_COM_DownstreamDataCells = adslMib.atmStat2lp[0].rcvStat.cntCellData;
      obj->X_BROADCOM_COM_UpstreamBitErrors = adslMib.atmStat2lp[0].xmtStat.cntBitErrs;
      obj->X_BROADCOM_COM_DownstreamBitErrors = adslMib.atmStat2lp[0].rcvStat.cntBitErrs;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      obj->X_BROADCOM_COM_ReceiveBlocks_2 = adslMib.xdslStat[1].rcvStat.cntSF;
      obj->X_BROADCOM_COM_TransmitBlocks_2 = adslMib.xdslStat[1].xmtStat.cntSF;
      obj->X_BROADCOM_COM_RxRsUncorrectable_2 = adslMib.xdslStat[1].rcvStat.cntRSUncor;
      obj->X_BROADCOM_COM_TxRsUncorrectable_2 = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable_2 = adslMib.xdslStat[1].rcvStat.cntRSCor;;
      obj->X_BROADCOM_COM_TxRsCorrectable_2 = adslMib.xdslStat[1].xmtStat.cntRSCor;
      obj->X_BROADCOM_COM_RxRsWords_2 = adslMib.xdslStat[1].rcvStat.cntRS;
      obj->X_BROADCOM_COM_TxRsWords_2 = adslMib.xdslStat[1].xmtStat.cntRS;
      obj->X_BROADCOM_COM_XTURHECErrors_2 = adslMib.atmStat2lp[1].xmtStat.cntHEC;
      obj->X_BROADCOM_COM_XTUCHECErrors_2 = adslMib.atmStat2lp[1].rcvStat.cntHEC;
      obj->X_BROADCOM_COM_DownstreamOCD_2 = adslMib.atmStat2lp[1].rcvStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamOCD_2 = adslMib.atmStat2lp[1].xmtStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamLCD_2 = adslMib.atmStat2lp[1].xmtStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamLCD_2 = adslMib.atmStat2lp[1].rcvStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamTotalCells_2 = adslMib.atmStat2lp[1].rcvStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamTotalCells_2 = adslMib.atmStat2lp[1].xmtStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamDataCells_2 = adslMib.atmStat2lp[1].xmtStat.cntCellData;
      obj->X_BROADCOM_COM_DownstreamDataCells_2 = adslMib.atmStat2lp[1].rcvStat.cntCellData;
      obj->X_BROADCOM_COM_UpstreamBitErrors_2 = adslMib.atmStat2lp[1].xmtStat.cntBitErrs;
      obj->X_BROADCOM_COM_DownstreamBitErrors_2 = adslMib.atmStat2lp[1].rcvStat.cntBitErrs;
#else
      obj->X_BROADCOM_COM_ReceiveBlocks_2 = 0;
      obj->X_BROADCOM_COM_TransmitBlocks_2 = 0;
      obj->X_BROADCOM_COM_RxRsUncorrectable_2 = 0;
      obj->X_BROADCOM_COM_TxRsUncorrectable_2 = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable_2 = 0;
      obj->X_BROADCOM_COM_TxRsCorrectable_2 = 0;
      obj->X_BROADCOM_COM_RxRsWords_2 = 0;
      obj->X_BROADCOM_COM_TxRsWords_2 = 0;
      obj->X_BROADCOM_COM_XTURHECErrors_2 = 0;
      obj->X_BROADCOM_COM_XTUCHECErrors_2 = 0;
      obj->X_BROADCOM_COM_DownstreamOCD_2 = 0;
      obj->X_BROADCOM_COM_UpstreamOCD_2 = 0;
      obj->X_BROADCOM_COM_UpstreamLCD_2 = 0;
      obj->X_BROADCOM_COM_DownstreamLCD_2 = 0;
      obj->X_BROADCOM_COM_DownstreamTotalCells_2 = 0;
      obj->X_BROADCOM_COM_UpstreamTotalCells_2 = 0;
      obj->X_BROADCOM_COM_UpstreamDataCells_2 = 0;
      obj->X_BROADCOM_COM_DownstreamDataCells_2 = 0;
      obj->X_BROADCOM_COM_UpstreamBitErrors_2 = 0;
      obj->X_BROADCOM_COM_DownstreamBitErrors_2 = 0;
#endif  /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */

#ifdef YEN_REMOVE
      /* these are in line stats total */
      obj->erroredSecs = adslMib.adslPerfData.perfTotal.adslESs;
      obj->X_BROADCOM_COM_UpstreamEs = adslMib.adslTxPerfTotal.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamSes = adslMib.adslTxPerfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamUas = adslMib.adslTxPerfTotal.adslUAS;
      obj->X_BROADCOM_COM_DownstreamUas = adslMib.adslPerfData.perfTotal.adslUAS;
#endif
   }
   else
   {
      cmsLog_error("Cannot get adslMib");
   }

   return (CMSRET_SUCCESS);
} /* rutdsl_getTotalChannelStats */

/* rutdsl_getShowTimeChannelStats */
CmsRet rutdsl_getShowTimeChannelStats_dev2(Dev2DslChannelStatsShowtimeObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->XTUCFECErrors = adslMib.adslTxPerfSinceShowTime.adslFECs;
      obj->XTURFECErrors = adslMib.adslPerfData.perfSinceShowTime.adslFECs;
      obj->XTURHECErrors = adslMib.atmStat.rcvStat.cntHEC;
      obj->XTUCHECErrors = adslMib.atmStat.xmtStat.cntHEC;
      obj->XTURCRCErrors = adslMib.adslStat.rcvStat.cntSFErr;
      obj->XTUCCRCErrors = adslMib.adslStat.xmtStat.cntSFErr;
   }
   else
   {
      obj->XTUCFECErrors = 0;
      obj->XTURFECErrors = 0;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = 0;
      obj->XTUCCRCErrors = 0;
   }
   return (CMSRET_SUCCESS);
} /* rutdsl_getShowtimeChannelStats */

/* quarter hour stats */
CmsRet rutdsl_getQuarterHourChannelStats_dev2(Dev2DslChannelStatsQuarterHourObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->XTUCFECErrors = adslMib.adslTxPerfCur15Min.adslFECs;
      obj->XTURFECErrors = adslMib.adslPerfData.perfCurr15Min.adslFECs;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanUncorrectBlks;
      obj->XTUCCRCErrors = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanTxCRC;
#ifdef DMP_X_RDK_DSL_1
      obj->X_RDK_LinkRetrain = adslMib.adslPerfData.failCur15Min.adslRetr;
#endif
   }
   else
   {
      obj->XTUCFECErrors = 0;
      obj->XTURFECErrors = 0;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = 0;
      obj->XTUCCRCErrors = 0;
#ifdef DMP_X_RDK_DSL_1
      obj->X_RDK_LinkRetrain = 0;
#endif
   }
   return (CMSRET_SUCCESS);
} /* rutdsl_getQuarterHourChannelStats */

/* current day channel stats */
CmsRet rutdsl_getCurrentDayChannelStats_dev2(Dev2DslChannelStatsCurrentDayObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->XTUCFECErrors = adslMib.adslTxPerfCur1Day.adslFECs;
      obj->XTURFECErrors = adslMib.adslPerfData.perfCurr1Day.adslFECs;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanUncorrectBlks;
      obj->XTUCCRCErrors = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanTxCRC;
#ifdef DMP_X_RDK_DSL_1
      obj->X_RDK_LinkRetrain = adslMib.adslPerfData.failCurDay.adslRetr;
#endif
   }
   else
   {
      obj->XTUCFECErrors = 0;
      obj->XTURFECErrors = 0;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = 0;
      obj->XTUCCRCErrors = 0;
#ifdef DMP_X_RDK_DSL_1
      obj->X_RDK_LinkRetrain = 0;
#endif
   }
   return (CMSRET_SUCCESS);
} /* rutdsl_getCurrentDayChannelStats */



#ifdef DMP_DEVICE2_DSLDIAGNOSTICS_1
CmsRet rutdsl_getAdslLoopDiagStatus_dev2(void *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   adslMibInfo adslMib;
   long size = sizeof(adslMib);
   Dev2DslDiagnosticsADSLLineTestObject *dslDiagObj = (Dev2DslDiagnosticsADSLLineTestObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   int testCompleted = DIAG_DSL_LOOPBACK_ERROR;

   ret = (CmsRet)xdslCtl_GetObjectValue(dslDiagObj->X_BROADCOM_COM_LineId, NULL, 0, (char *)&adslMib, &size);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("Unable to read diagnosticsState from driver");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->diagnosticsState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
      return (ret);
   }
   else
   {
      /* reading for indication from driver that test is completed */
      testCompleted = adslMib.adslPhys.adslLDCompleted;

      cmsLog_debug(" testCompleted %d (2== completed)", testCompleted);

      if ((testCompleted == DIAG_DSL_LOOPBACK_ERROR) || (testCompleted == DIAG_DSL_LOOPBACK_ERROR_BUT_RETRY))
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->diagnosticsState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }
      else if (testCompleted == DIAG_DSL_LOOPBACK_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->diagnosticsState,MDMVS_COMPLETE,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }  /* get adslLdCompleted */
   } /* read from ADSL driver OK */
   return (ret);
}
#endif /* DMP_DEVICE2_DSLDIAGNOSTICS_1 */

#if defined(SUPPORT_DSL_BONDING)
CmsRet rutdsl_getBondingGroupStats(Dev2DslBondingGroupStatsObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   long adslMibSize;
   Dev2AtmLinkObject *pAtmLinkObj;
   Dev2PtmLinkObject *pPtmLinkObj;
   MdmPathDescriptor pathDesc;
   char *bondingGroupFullPath = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack xtmIidStack;
   UBOOL8 found = FALSE;
   UINT64 dontCare;
   UINT64 byteTx, byteRx, byteTxTotal=0, byteRxTotal=0;
   UINT64 packetTx, packetRx, packetTxTotal=0, packetRxTotal=0;
   UINT32 errorsTxTotal=0, errorsRxTotal=0;
   UINT64 errorsTx, errorsRx;
   UINT64 packetUniTx, packetUniRx, packetUniTxTotal=0, packetUniRxTotal=0;
   UINT64 packetMultiTx, packetMultiRx, packetMultiTxTotal=0, packetMultiRxTotal=0;
   UINT64 packetBcastTx, packetBcastRx, packetBcastTxTotal=0, packetBcastRxTotal=0;
   UINT32 packetDiscardTxTotal=0, packetDiscardRxTotal=0;
   UINT64 packetDiscardTx, packetDiscardRx;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   ret = (CmsRet)xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }

   obj->totalStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
   obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
   obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;

   /* aggregated counters retrieved from interface stats for all lowerlayers channels (ptmx,atmx) */
   /* get full pathname of this bonding group */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
   pathDesc.iidStack = *iidStack;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &bondingGroupFullPath)) == CMSRET_SUCCESS)
   {
      /* look for all ptm and atm interfaces with bondingGroupFullPath name, add up the stats */
      INIT_INSTANCE_ID_STACK(&xtmIidStack);
      while ((ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &xtmIidStack, (void **)&pAtmLinkObj)) == CMSRET_SUCCESS)
      {
         if ((pAtmLinkObj->enable == TRUE) &&
             (cmsUtl_strcmp(pAtmLinkObj->lowerLayers,bondingGroupFullPath) == 0))
         {
            found = TRUE;
            rut_getIntfStats_uint64(pAtmLinkObj->name,
                                    &byteRx,&packetRx,
                                    &dontCare/*byteMultiRx*/,&packetMultiRx,
                                    &packetUniRx,&packetBcastRx,
                                    &errorsRx,&packetDiscardRx,
                                    &byteTx,&packetTx,
                                    &dontCare/*byteMultiTx*/,&packetMultiTx,
                                    &packetUniTx,&packetBcastTx,
                                    &errorsTx,&packetDiscardTx);
            byteTxTotal += byteTx;
            byteRxTotal += byteRx;
            packetTxTotal += packetTx;
            packetRxTotal += packetRx;
            errorsTxTotal += (UINT32)errorsTx;
            errorsRxTotal += (UINT32)errorsRx;
            packetUniTxTotal += packetUniTx;
            packetUniRxTotal += packetUniRx;
            packetDiscardTxTotal += (UINT32)packetDiscardTx;
            packetDiscardRxTotal += (UINT32)packetDiscardRx;
            packetMultiTxTotal += packetMultiTx;
            packetMultiRxTotal += packetMultiRx;
            packetBcastTxTotal += packetBcastTx;
            packetBcastRxTotal += packetBcastRx;
         }
         cmsObj_free((void **) &pAtmLinkObj);
      } /* while ATM links */
      if (!found)
      {
         INIT_INSTANCE_ID_STACK(&xtmIidStack);
         while ((ret = cmsObj_getNext(MDMOID_DEV2_PTM_LINK, &xtmIidStack, (void **)&pPtmLinkObj)) == CMSRET_SUCCESS)
         {
            if ((pPtmLinkObj->enable == TRUE) &&
                (cmsUtl_strcmp(pPtmLinkObj->lowerLayers,bondingGroupFullPath) == 0))
            {
               found = TRUE;
               rut_getIntfStats_uint64(pPtmLinkObj->name,
                                       &byteRx,&packetRx,
                                       &dontCare/*byteMultiRx*/,&packetMultiRx,
                                       &packetUniRx,&packetBcastRx,
                                       &errorsRx,&packetDiscardRx,
                                       &byteTx,&packetTx,
                                       &dontCare/*byteMultiTx*/,&packetMultiTx,
                                       &packetUniTx,&packetBcastTx,
                                       &errorsTx,&packetDiscardTx);
               byteTxTotal += byteTx;
               byteRxTotal += byteRx;
               packetTxTotal += packetTx;
               packetRxTotal += packetRx;
               errorsTxTotal += (UINT32)errorsTx;
               errorsRxTotal += (UINT32)errorsRx;
               packetUniTxTotal += packetUniTx;
               packetUniRxTotal += packetUniRx;
               packetDiscardTxTotal += (UINT32)packetDiscardTx;
               packetDiscardRxTotal += (UINT32)packetDiscardRx;
               packetMultiTxTotal += packetMultiTx;
               packetMultiRxTotal += packetMultiRx;
               packetBcastTxTotal += packetBcastTx;
               packetBcastRxTotal += packetBcastRx;
            }
            cmsObj_free((void **) &pPtmLinkObj);
         } /* while PTM links */
      } /* try ptm link */
      CMSMEM_FREE_BUF_AND_NULL_PTR(bondingGroupFullPath);

      /* if ptm or atm is found */
      if (found)
      {
         /* store aggregated values */
         obj->bytesSent = byteTxTotal;
         obj->bytesReceived = byteRxTotal;
         obj->packetsSent += packetTx;
         obj->packetsReceived += packetRx;
         obj->errorsSent += errorsTx;
         obj->errorsReceived += errorsRx;
         obj->unicastPacketsSent += packetUniTx;
         obj->unicastPacketsReceived += packetUniRx;
         obj->discardPacketsSent += packetDiscardTx;
         obj->discardPacketsReceived += packetDiscardRx;
         obj->multicastPacketsSent += packetMultiTx;
         obj->multicastPacketsReceived += packetMultiRx;
         obj->broadcastPacketsSent += packetBcastTx;
         obj->broadcastPacketsReceived += packetBcastRx;
      }
   }

   return (CMSRET_SUCCESS);
}

CmsRet rutdsl_getBondingGroupTotalStats(Dev2DslBondingGroupStatsTotalObject *obj)
{
   adslMibInfo adslMib;
   long adslMibSize;
   CmsRet ret;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   ret = (CmsRet)xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds = adslMib.adslPerfData.perfTotal.adslESs;
   obj->severelyErroredSeconds = adslMib.adslPerfData.perfTotal.adslSES;
   obj->unavailableSeconds = adslMib.adslPerfData.perfTotal.adslUAS;

   /* line 1 */
   adslMibSize=sizeof(adslMib);
   ret = (CmsRet)xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 1");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds += adslMib.adslPerfData.perfTotal.adslESs;
   obj->severelyErroredSeconds += adslMib.adslPerfData.perfTotal.adslSES;
   obj->unavailableSeconds += adslMib.adslPerfData.perfTotal.adslUAS;

   return (CMSRET_SUCCESS);
}

CmsRet rutdsl_getBondingGroupCurrentDayStats(Dev2DslBondingGroupStatsCurrentDayObject *obj)
{
   adslMibInfo adslMib;
   long adslMibSize;
   CmsRet ret;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   ret = (CmsRet)xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds = adslMib.adslPerfData.perfCurr1Day.adslESs;
   obj->severelyErroredSeconds = adslMib.adslPerfData.perfCurr1Day.adslSES;
   obj->unavailableSeconds = adslMib.adslPerfData.perfCurr1Day.adslUAS;

   /* line 1 */
   adslMibSize=sizeof(adslMib);
   ret = (CmsRet)xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 1");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds += adslMib.adslPerfData.perfCurr1Day.adslESs;
   obj->severelyErroredSeconds += adslMib.adslPerfData.perfCurr1Day.adslSES;
   obj->unavailableSeconds += adslMib.adslPerfData.perfCurr1Day.adslUAS;

   return (CMSRET_SUCCESS);
}

CmsRet rutdsl_getBondingGroupQuarterHourStats(Dev2DslBondingGroupStatsQuarterHourObject *obj)
{
   adslMibInfo adslMib;
   long adslMibSize;
   CmsRet ret;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   ret = (CmsRet)xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds = adslMib.adslPerfData.perfCurr15Min.adslESs;
   obj->severelyErroredSeconds = adslMib.adslPerfData.perfCurr15Min.adslSES;
   obj->unavailableSeconds = adslMib.adslPerfData.perfCurr15Min.adslUAS;

   /* line 1 */
   adslMibSize=sizeof(adslMib);
   ret = (CmsRet)xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MIB for line 1");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds += adslMib.adslPerfData.perfCurr15Min.adslESs;
   obj->severelyErroredSeconds += adslMib.adslPerfData.perfCurr15Min.adslSES;
   obj->unavailableSeconds += adslMib.adslPerfData.perfCurr15Min.adslUAS;

   return (CMSRET_SUCCESS);
}

#endif /* bonding */


#endif /* DMP_DEVICE2_DSL_1 */


#endif /*  DMP_DEVICE2_BASELINE_1 */
