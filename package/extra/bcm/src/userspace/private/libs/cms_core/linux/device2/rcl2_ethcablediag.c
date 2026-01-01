/***********************************************************************
 *
 *  Copyright (c) 2020-2023  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2023:proprietary:standard

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

#ifdef DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1


#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "sysutil_net.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_util.h"
#include "rut2_ethernet.h"
#include "rut2_ethcablediag.h"
#include "rut_ethintf.h"
#include "rut_qos.h"


#include "bcmnet.h"  // for ETHERNET_ROOT_DEVICE_NAME

/*!\file rcl2_ethcablediag.c
 * \brief This file contains Device2 ethernet cable diagnostics related functions.
 *
 */

static void clearPerInterfaceResults(void)
{
   const _Dev2CableDiagPerInterfaceResultObject *perIntfResultObj = NULL;
   InstanceIdStack perIntfResultIidStack = EMPTY_INSTANCE_ID_STACK;


   /** delete all previous result objects. */
   while (cmsObj_getNextFlags(MDMOID_DEV2_CABLE_DIAG_PER_INTERFACE_RESULT, 
                              &perIntfResultIidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&perIntfResultObj) == CMSRET_SUCCESS)
   {
      cmsObj_deleteInstance(MDMOID_DEV2_CABLE_DIAG_PER_INTERFACE_RESULT, &perIntfResultIidStack);
      cmsObj_free((void **) &perIntfResultObj);
      INIT_INSTANCE_ID_STACK(&perIntfResultIidStack);
   }
}


CmsRet rcl_dev2CableDiagObject( _Dev2CableDiagObject *newObj,
      const _Dev2CableDiagObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("Enter==>");

   if ((mdmLibCtx.eid != EID_SSK))
   {
      // Normal apps such as tr69c and httpd can only write requested or
      // none to the diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      // diagnosticsState is still MDMVS_NONE so return CMSRET_SUCCESS
      // but record other params which may have changed.
      else if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_NONE) == 0)
      {
         return CMSRET_SUCCESS;
      }
   }

   // Perform a manual cabel diagnostics
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      void *msgHandle = cmsMdm_getThreadMsgHandle();
      CmsEntityId eid = GENERIC_EID(cmsMsg_getHandleEid(msgHandle));

      /** delete all previous result objects. */
      clearPerInterfaceResults();

      if (!IS_EMPTY_STRING(newObj->interface))
      {
         if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->interface, ifName)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfname(%s) returns error ret=%d", newObj->interface,ret);
            *errorParam = cmsMem_strdupFlags("Interface",mdmLibCtx.allocFlags);
            *errorCode = CMSRET_INVALID_PARAM_VALUE;
            return CMSRET_INVALID_ARGUMENTS;
         }

         ret = rutEthCableDiag_Run(ifName, 1);
      }
      else // empty string means all ethernet interfaces
      {
         _Dev2EthernetInterfaceObject *ethInftObj = NULL;
         InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

         cmsLog_debug("Initiate cable diagonostics for all Ethernet Interfaces");
         while (cmsObj_getNextFlags(MDMOID_DEV2_ETHERNET_INTERFACE, &iidStack,
                  OGF_NO_VALUE_UPDATE,
                  (void **) &ethInftObj) == CMSRET_SUCCESS)
         {
            cmsLog_debug("Run [%s] cable diagonostics", ethInftObj->name);
            snprintf(ifName, sizeof(ifName)-1, ethInftObj->name); 
            cmsObj_free((void **) &ethInftObj);

            if ((ret = rutEthCableDiag_Run(ifName, 1)) != CMSRET_SUCCESS)
               break;
         }
      }

      if (ret == CMSRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->diagnosticsState, MDMVS_COMPLETE, mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->diagnosticsState, MDMVS_ERROR, mdmLibCtx.allocFlags);
      }

      // send diag complete event. TR69C should send out an inform to ACS.
      rut_sendEventMsgToSsk(CMS_MSG_ETHCABLE_DIAG_COMPLETE, eid, NULL, 0);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2CableDiagPerInterfaceResultObject( _Dev2CableDiagPerInterfaceResultObject *newObj,
      const _Dev2CableDiagPerInterfaceResultObject *currObj,
      const InstanceIdStack *iidStack,
      char **errorParam,
      CmsRet *errorCode)
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumCdPerInterfaceResult_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumCdPerInterfaceResult_dev2(iidStack, -1);
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2CableDiagBrownPairObject( _Dev2CableDiagBrownPairObject *newObj,
      const _Dev2CableDiagBrownPairObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2CableDiagBluePairObject( _Dev2CableDiagBluePairObject *newObj,
      const _Dev2CableDiagBluePairObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2CableDiagGreenPairObject( _Dev2CableDiagGreenPairObject *newObj,
      const _Dev2CableDiagGreenPairObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2CableDiagOrangePairObject( _Dev2CableDiagOrangePairObject *newObj,
      const _Dev2CableDiagOrangePairObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif /* DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1
#error "Device2 ethernet interface objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif

#endif  /* DMP_DEVICE2_BASELINE_1 */
