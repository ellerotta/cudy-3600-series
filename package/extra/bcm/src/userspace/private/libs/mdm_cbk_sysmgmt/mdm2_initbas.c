/*
* <:copyright-BRCM:2011:proprietary:standard
*
*    Copyright (c) 2011 Broadcom
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

#ifdef DMP_DEVICE2_BASELINE_1
#ifdef SUPPORT_BAS2

/*!\file mdm2_initbas.c
 * \brief MDM initialization for BAS TR181 objects
 *
 */


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "rut_basdcfg.h"

CmsRet addBasClientObj(const char *clientName, const char *exeName,
                       UBOOL8 enable)
{
   MdmPathDescriptor pathDesc;
   BasClientObject *basclientObj = NULL;
   CmsRet ret=CMSRET_SUCCESS;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_BAS_CLIENT;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for MDMOID_BAS_CLIENT failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &basclientObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get basclientObj object, ret=%d", ret);
      return ret;
   }
   CMSMEM_REPLACE_STRING_FLAGS(basclientObj->name, clientName, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(basclientObj->exeName, exeName, mdmLibCtx.allocFlags);
   if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_WIFI))
   {
      /*
       * wifi client requires arguments
       * -d: debug level, 0(debug) ~ 6(critical)
       * -p: product type, 2(xDSL), 3(PON)
       */
#if defined(DMP_X_BROADCOM_COM_GPON_1)
      CMSMEM_REPLACE_STRING_FLAGS(basclientObj->exeArgs, "-d 6 -p 3", mdmLibCtx.allocFlags);
#else
      CMSMEM_REPLACE_STRING_FLAGS(basclientObj->exeArgs, "-d 6 -p 2", mdmLibCtx.allocFlags);
#endif
   }
   //basclientObj->isStatic = TRUE; /* isStatic is TRUE by default */
   basclientObj->enable = enable;
   ret = mdm_setObject((void **) &basclientObj, &pathDesc.iidStack,  FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set basclientObj, name %s. ret=%d", clientName,ret);
   }
   mdm_freeObject((void **)&basclientObj);
   return ret;
}

CmsRet deleteBasClientObj(char *name, InstanceIdStack *iidStack)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret=CMSRET_SUCCESS;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_BAS_CLIENT;
   pathDesc.iidStack = *iidStack;

   if ((ret = mdm_deleteObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_deleteObjectInstance for MDMOID_BAS_CLIENT name %s failed, ret=%d", name, ret);
   }
   return ret;
}
CmsRet mdm_addStaticBasClients(void)
{
   BasClientObject *basclientObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 addRg = TRUE;
#ifdef BRCM_VOICE_SUPPORT
   UBOOL8 addVoice = TRUE;
#endif
#ifdef SUPPORT_OPENPLAT
   UBOOL8 addOpenplat = TRUE;
#endif
#if defined(DMP_DEVICE2_DSL_1) || defined(BUILD_BRCM_XDSL_DISTPOINT)
   UBOOL8 addDsl = TRUE;
#endif
#if defined(DMP_X_BROADCOM_COM_GPON_1)
   UBOOL8 addGpon = TRUE;
#endif
#if defined(DMP_X_BROADCOM_COM_RDPA_1)
   UBOOL8 addRdpa = TRUE;
#endif
#ifdef DMP_DEVICE2_IPLAYERCAPACITYTEST_1
   UBOOL8 addTr471 = TRUE;
#endif
#ifdef DMP_DEVICE2_WIFIRADIO_1
   UBOOL8 addWifi = TRUE;
#endif
#if defined(D6S_EYE)
   UBOOL8 addEye = TRUE;
#endif
#if defined(DMP_DEVICE2_WIFIRADIO_1)
   UBOOL8 addWldataelm = TRUE;
#endif 

   /* We have to make sure all the enabled feature has its static client added. */
   while (ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextObject(MDMOID_BAS_CLIENT, &iidStack, (void **) &basclientObj);
      if (ret == CMSRET_SUCCESS)
      {
         addRg = FALSE;

         if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_VOICE))
         {
#ifdef BRCM_VOICE_SUPPORT
            addVoice = FALSE;
#else
            /* voice is not supported, and there is voice bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_OPENPLAT))
         {
#ifdef SUPPORT_OPENPLAT
            addOpenplat = FALSE;
#else
            /* OpenPlat is not supported, and there is openplat bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_XDSL))
         {
#if defined(DMP_DEVICE2_DSL_1) || defined(BUILD_BRCM_XDSL_DISTPOINT)
            addDsl = FALSE;
#else
            /* DSL is not supported, and there is xdsl bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_GPON))
         {
#if defined(DMP_X_BROADCOM_COM_GPON_1)
            addGpon = FALSE;
#else
            /* Gpon is not supported, and there is gpon bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_RDPA))
         {
#if defined(DMP_X_BROADCOM_COM_RDPA_1)
            addRdpa = FALSE;
#else
            /* Rdpa is not supported, and there is rdpa bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_TR471))
         {
#ifdef DMP_DEVICE2_IPLAYERCAPACITYTEST_1
            addTr471 = FALSE;
#else
            /* TR471 is not supported, and there is tr471 bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_WIFI))
         {
#ifdef DMP_DEVICE2_WIFIRADIO_1
            addWifi = FALSE;
#else
            /* Wifi is not supported, and there is wifi bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_EYE))
         {
#if defined(D6S_EYE)
            addEye = FALSE;
#else
            /* Eye Scope is not supported, and there is an eye bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif
         }
         else if (!cmsUtl_strcmp(basclientObj->name, BAS_CLIENT_WLDATAELM))
         {
#if defined(DMP_DEVICE2_WIFIRADIO_1)
            addWldataelm = FALSE;
#else
            /* Wifi is not supported, and there is wifi bas client in the MDM, remove it */
            deleteBasClientObj(basclientObj->name, &iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
#endif         
         }
         mdm_freeObject((void **)&basclientObj);
      }
   } /* while */

   if (addRg)
   {
      /* rgclient is always in the system */
      ret = addBasClientObj(BAS_CLIENT_RG, BAS_CLIENT_RG_EXE, TRUE);
      ret = (ret != CMSRET_SUCCESS) ? ret :
        addBasClientObj(BAS_CLIENT_RG_BDK, BAS_CLIENT_RG_BDK_EXE, TRUE);
      ret = (ret != CMSRET_SUCCESS) ? ret :
        addBasClientObj(BAS_CLIENT_RDS, BAS_CLIENT_RDS_EXE, TRUE);
      ret = (ret != CMSRET_SUCCESS) ? ret :
        addBasClientObj(BAS_CLIENT_TR143, BAS_CLIENT_TR143_EXE, TRUE);
   }
   else
   {
      ret = CMSRET_SUCCESS;
   }

#ifdef BRCM_VOICE_SUPPORT
   if (addVoice)
   {
      ret = addBasClientObj(BAS_CLIENT_VOICE, BAS_CLIENT_VOICE_EXE, FALSE);
   }
#endif
#ifdef SUPPORT_OPENPLAT
   if (addOpenplat)
   {
      ret = addBasClientObj(BAS_CLIENT_OPENPLAT, BAS_CLIENT_OPENPLAT_EXE, TRUE);
   }
#endif
#if defined(DMP_DEVICE2_DSL_1) || defined(BUILD_BRCM_XDSL_DISTPOINT)
   if (addDsl)
   {
      ret = addBasClientObj(BAS_CLIENT_XDSL, BAS_CLIENT_XDSL_EXE, TRUE);
   }
#endif
#if defined(DMP_X_BROADCOM_COM_GPON_1)
   if (addGpon)
   {
      ret = addBasClientObj(BAS_CLIENT_GPON, BAS_CLIENT_GPON_EXE, FALSE);
   }
#endif
#if defined(DMP_X_BROADCOM_COM_RDPA_1)
   if (addRdpa)
   {
      ret = addBasClientObj(BAS_CLIENT_RDPA, BAS_CLIENT_RDPA_EXE, FALSE);
   }
#endif
#ifdef DMP_DEVICE2_IPLAYERCAPACITYTEST_1
   if (addTr471)
   {
      ret = addBasClientObj(BAS_CLIENT_TR471, BAS_CLIENT_TR471_EXE, TRUE);
   }
#endif
#ifdef DMP_DEVICE2_WIFIRADIO_1
   if (addWifi)
   {
      ret = addBasClientObj(BAS_CLIENT_WIFI, BAS_CLIENT_WIFI_EXE, FALSE);
   }
#endif
#if defined(D6S_EYE)
   if (addEye)
   {
      ret = addBasClientObj(BAS_CLIENT_EYE, BAS_CLIENT_EYE_EXE, TRUE);
   }
#endif
#if defined(DMP_DEVICE2_WIFIRADIO_1)
   if (addWldataelm)
   {
      ret = addBasClientObj(BAS_CLIENT_WLDATAELM, BAS_CLIENT_WLDATAELM_EXE, TRUE);
   }
#endif
   return ret;
}

#endif /* SUPPORT_BAS2 */
#endif  /* DMP_DEVICE2_BASELINE_1 */


