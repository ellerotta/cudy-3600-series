/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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


#include "cms.h"
#include "cms_params_modsw.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "osgid.h"
#include "beep_common.h"
#include "openplat.h"

/*!\file mdm_initmodsw.c
 * \brief This file calls other Exe Env functions (osgi, linux) to initialize
 *        the MDM.
 *
 */


#ifdef DMP_DEVICE2_SM_BASELINE_1

CmsRet mdm_addDefaultModSwObjects(void);

#ifdef SUPPORT_BEEP_HOST_EE
CmsRet mdm_addDefaultModSwOpsEeObjects(UINT32 *eeAddedCount);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1
CmsRet mdm_addDefaultModSwOsgiEeObjects(UINT32 *eeAddedCount);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1
CmsRet mdm_addDefaultModSwDockerEeObjects(UINT32 *eeAddedCount);
#endif

#if !defined(SUPPORT_BEEP_HOST_EE) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1)
static CmsRet mdm_delModSwEeObjects(const char *eeName);
#endif



/** This is the "weak" mdm_adjustForHardware_dev2.
 *  It will only be called in a distributed MDM build in the openplat component.
 */
#pragma weak mdm_adjustForHardware_dev2
CmsRet mdm_adjustForHardware_dev2()
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Distributed MDM build: adjustForHardware for openplat component only");

   ret = mdm_addDefaultModSwObjects();
   RETURN_IF_NOT_SUCCESS(ret);

   return ret;
}


CmsRet mdm_addDefaultModSwObjects(void)
{
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 eeDefinedCount=0;
   UINT32 eeAddedCount=0;
   UBOOL8 updateNeeded = FALSE;
   SwModulesObject *swObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

   ret = mdm_getObject(MDMOID_SW_MODULES, &iidStack, (void **) &swObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get SW_MODULES object, ret=%d", ret);
   }
   else
   {
#ifdef SUPPORT_OPENPLAT
      /* Openplat handles preinstall mdm in ops_ee and ops_app */
#else
      /* Should execute purely based on control flags */
      {
         UINT32 eeDeletedCount=0;
         UINT32 duDeletedCount=0;
         UINT32 euDeletedCount=0;

         if (swObj->X_BROADCOM_COM_PreinstallNeeded == TRUE)
         {
            if (swObj->X_BROADCOM_COM_PreinstallOverwrite == TRUE)
            {
               /* 
                * go through all the EEs that are preinstalled and 
                * remove them from MDM 
                */
               mdm_delModSwPreinstalledEeObjects(&eeDeletedCount,
                                              &duDeletedCount, &euDeletedCount);
               if (eeDeletedCount)
               {
                  swObj->execEnvNumberOfEntries -= eeDeletedCount;
                  updateNeeded = TRUE;
               }
               if (duDeletedCount)
               {
                  swObj->deploymentUnitNumberOfEntries -= duDeletedCount;
                  updateNeeded = TRUE;
               }
               if (euDeletedCount)
               {
                  swObj->executionUnitNumberOfEntries -= euDeletedCount;
                  updateNeeded = TRUE;
               }

               /* 
                * go through all the DUs that are preinstalled and 
                * remove them from MDM 
                */
               duDeletedCount = euDeletedCount = 0;
               mdm_delModSwPreinstalledDuObjects(&duDeletedCount,
                                                 &euDeletedCount);
               if (duDeletedCount)
               {
                  swObj->deploymentUnitNumberOfEntries -= duDeletedCount;
                  updateNeeded = TRUE;
               }
               if (euDeletedCount)
               {
                  swObj->executionUnitNumberOfEntries -= euDeletedCount;
                  updateNeeded = TRUE;
               }
            }
         }
      }
#endif
   }

#ifdef SUPPORT_BEEP_HOST_EE
   eeDefinedCount++;
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwOpsEeObjects(&eeAddedCount);
   }
#else
   mdm_delModSwEeObjects(BEEP_HOSTEE_NAME);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1
   eeDefinedCount++;
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwOsgiEeObjects(&eeAddedCount);
   }
#else
   mdm_delModSwEeObjects(OSGI_NAME);
#endif

#ifndef DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1
   mdm_delModSwEeObjects(DOCKEREE_NAME);
#endif

   if (eeDefinedCount == 0)
   {
      cmsLog_notice("No Exec Envs were built into the system.");
   }

   if (eeAddedCount > 0)
   {
      swObj->execEnvNumberOfEntries += eeAddedCount;
      cmsLog_notice("setting exe env num entries=%d", swObj->execEnvNumberOfEntries);
      updateNeeded = TRUE;
   }

   if (updateNeeded)
   {
      ret = mdm_setObject((void **) &swObj, &iidStack, FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set SW_MODULES object, ret=%d", ret);
      }
   }

   return ret;

}  /* End of mdm_addDefaultModSwObjects() */


#ifdef SUPPORT_BEEP_HOST_EE
CmsRet mdm_addDefaultModSwOpsEeObjects(UINT32 *eeAddedCount)
{
   ExecEnvObject *eeObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 add = TRUE;
   CmsRet ret;

   /* If there is no OPS Host Exec Env object yet, add it. */
   while (add &&
          (ret = mdm_getNextObject(MDMOID_EXEC_ENV, &iidStack,
                                   (void **)&eeObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, OPS_HOSTEE_NAME))
      {
         int cmp = beepUtil_strverscmp(eeObj->version, OPS_HOSTEE_VERSION);

         if (cmp != 0)
         {
            if (cmp > 0)
            {
               cmsLog_error("Openplat Host EE is downgraded from %s to %s. EUs might not work as expected.",
                            eeObj->version, OPS_HOSTEE_VERSION);
            }

            CMSMEM_REPLACE_STRING_FLAGS(eeObj->version, OPS_HOSTEE_VERSION,
                                        mdmLibCtx.allocFlags);
            ret = mdm_setObject((void **) &eeObj, &iidStack, FALSE);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("could not set OPS HOST EE, ret=%d", ret);
            }
         }
         /* already in there, so no need to add */
         add = FALSE;
      }

      cmsObj_free((void **) &eeObj);
   }

   if (add)
   {
      char tmpStr[BUFLEN_64]={0};

      cmsLog_notice("Adding initial OPS host Execution Environment");

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_EXEC_ENV;
      INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL))!=CMSRET_SUCCESS)
      {
         cmsLog_error("could not add OPS host Exec Env, ret=%d", ret);
         return ret;
      }
      else
      {
         (*eeAddedCount)++;
      }

      if ((ret = mdm_getObject(MDMOID_EXEC_ENV, &pathDesc.iidStack,
                               (void **) &eeObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get host Exec Env Obj, ret=%d", ret);
         return ret;
      }

      /* set any non-default initial values for the BEEP default EE object */
      eeObj->enable = TRUE;
      eeObj->X_BROADCOM_COM_IsPreinstall = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->name, OPS_HOSTEE_NAME,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->vendor, OPS_HOSTEE_VENDOR,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->version, OPS_HOSTEE_VERSION,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->X_BROADCOM_COM_ContainerName, "",
                                  mdmLibCtx.allocFlags);
      eeObj->X_BROADCOM_COM_MngrEid = EID_BBCD;

      snprintf(tmpStr, sizeof(tmpStr), "%s %s", OPS_HOSTEE_TYPE_PREFIX, OPS_HOSTEE_TYPE);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->type, tmpStr, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &eeObj, &pathDesc.iidStack,
                               FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set EXEC_ENV Object, ret = %d", ret);
      }

      cmsObj_free((void **) &eeObj);
   }

   return ret;
}
#endif


#if !defined(SUPPORT_BEEP_HOST_EE) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1)
static CmsRet mdm_delModSwEeObjects(const char *eeName)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   ExecEnvObject *eeObj = NULL;
   char path[CMS_MAX_FULLPATH_LENGTH];
   char *duDir = NULL;

   cmsLog_debug("Enter. eeName=%s", eeName);

   /* Clear out any old DU files (pkgs) that may be on the filesystem. */
   if (!cmsUtl_strcmp(eeName, OSGI_NAME))
   {
      duDir = CMS_MODSW_OSGIEE_DU_DIR;
   }
   else if (!cmsUtl_strcmp(eeName, DOCKEREE_NAME))
   {
      duDir = CMS_MODSW_DOCKEREE_DU_DIR;
   }
   else
   {
      cmsLog_error("Invalid eeName %s", eeName);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (cmsUtl_getRunTimePath(duDir, path, sizeof(path)) == CMSRET_SUCCESS)
   {
      if (cmsFil_isFilePresent(path))
      {
         cmsFil_removeDir(path);
      }
   }
   else
   {
      cmsLog_error("Could not make path for %s. ret=%d", duDir, ret);
   }

   /* Delete the EE object instance if exist */
   while ((ret = mdm_getNextObject(MDMOID_EXEC_ENV, &iidStack, (void **)&eeObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, eeName))
      {
         MdmPathDescriptor pathDesc;

         cmsLog_debug("Deleting %s Execution Environment", eeName);

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid      = MDMOID_EXEC_ENV;
         pathDesc.iidStack = iidStack;

         if ((ret = mdm_deleteObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete %s Execution Environment, ret=%d", eeName, ret);
         }
         cmsObj_free((void **)&eeObj);
         break;
      }
      cmsObj_free((void **)&eeObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;

}  /* End of mdm_delModSwEeObjects() */
#endif


#endif /* DMP_DEVICE2_SM_BASELINE_1 */
