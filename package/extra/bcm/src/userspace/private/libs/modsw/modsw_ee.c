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

#ifdef DMP_DEVICE2_SM_BASELINE_1

/*!\file modsw_ee.c
 * \brief This file contains functions for dealing with OSGI and Linux
 *        Deployment Units, Execution Units, etc.
 *
 */

#include "cms.h"
#include "cms_params_modsw.h"
#include "cms_util.h"
#include "cms_obj.h"
#include "cms_lck.h"
#include "cms_mgm.h"
#include "modsw.h"
#include "modsw_private.h"
#include "qdm_modsw_ee.h"
#include "beep_common.h"

CmsRet modsw_setExecEnvEnableLocked(const char *name,
                                    const char *vendor,
                                    const char *version,
                                    UBOOL8 enable)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   ExecEnvObject *execEnvObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if (name == NULL || vendor == NULL || version == NULL)
   {
      cmsLog_error("Invalid arguments!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   while (!found && cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                                        &iidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&execEnvObj) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp(name, execEnvObj->name) == 0 &&
               cmsUtl_strcmp(vendor, execEnvObj->vendor) == 0 &&
               cmsUtl_strcmp(version, execEnvObj->version) == 0);

      if (found)
      {
         if ((!cmsUtl_strcmp(execEnvObj->status, MDMVS_UP) && enable) ||
             (!cmsUtl_strcmp(execEnvObj->status, MDMVS_DISABLED) && !enable))
         {
            cmsLog_error("enable running or disable stopped EE, do nothing");
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
         else
         {
            execEnvObj->enable = enable;

            ret = cmsObj_set(execEnvObj, &iidStack);

            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not set Exec Env enable to %d (%s)", enable, name);
            }
         }
      }

      cmsObj_free((void **)&execEnvObj);
   }

   return ret;
}


CmsRet modsw_setExecEnvEnable(const char *name,
                              const char *vendor,
                              const char *version,
                              UBOOL8 enable)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }

   ret = modsw_setExecEnvEnableLocked(name, vendor, version, enable);

   cmsLck_releaseLock();

   return ret;
}


CmsRet modsw_setExecEnvStatusLocked(const char *name, const char *status)
{
   UBOOL8 found = FALSE;
   ExecEnvObject *execEnvObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   if (name == NULL)
   {
      cmsLog_error("Invalid argument!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   while (!found && cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                                        &iidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&execEnvObj) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp(name, execEnvObj->name) == 0);

      if (found)
      {
         REPLACE_STRING_IF_NOT_EQUAL(execEnvObj->status, status);

         ret = cmsObj_set(execEnvObj, &iidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not set Exec Env status to %s (%s)", status, name);
         }
      }

      cmsObj_free((void **)&execEnvObj);
   }
   
   return ret;
}


CmsRet modsw_setExecEnvStatus(const char *name, const char *status)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }

   ret = modsw_setExecEnvStatusLocked(name, status);

   cmsLck_releaseLock();

   return ret;
}


CmsRet modsw_setExecEnvVersion(const char *eeName, const char *eeVersion)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   ExecEnvObject   *eeObj   = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }

   while (!found && 
          cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&eeObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, eeName))
      {
         found = TRUE;
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->version, eeVersion);

         if ((ret = cmsObj_set(eeObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set MDMOID_EXEC_ENV returns error. ret=%d", ret);
         }
      }

      cmsObj_free((void **)&eeObj);      
   }
  
   cmsLck_releaseLock();

   return ret;
}


UBOOL8 modsw_findExecEnvByNameVendor(const char *name,
                                     const char *vendor)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   ExecEnvObject *eeObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }

   while (!found && cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                                        &iidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&eeObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(eeObj->name, name) == 0 &&
          cmsUtl_strcmp(eeObj->vendor, vendor) == 0)
      {
         found = TRUE;
      }

      cmsObj_free((void **)&eeObj);      
   }

   cmsLck_releaseLock();

   return found;
}


CmsRet modsw_addEeEntry(const char *name,
                        const char *alias,
                        const char *version,
                        const char *vendor,
                        const char *appName,
                        const char *description,
                        const char *containerName,
                        int isPreinstall,
                        int diskSpace,
                        int memory,
                        int runLevel,
                        CmsEntityId eeEid)
{
   CmsRet ret = CMSRET_UNKNOWN_EE;
   ExecEnvObject *eeObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char eeFullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
    }

   /* add a EE entry into the EE table */
   cmsLog_debug("Creating ExecEnvObject pkgEeName=%s, pkgEeVendor=%s", name, vendor);

   if ((ret = cmsObj_addInstance(MDMOID_EXEC_ENV, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("add instance of Execution Environment returned %d", ret);
   }
   else
   {
      ret = cmsObj_get(MDMOID_EXEC_ENV, &iidStack, 0, (void **) &eeObj);
      if (ret == CMSRET_SUCCESS)
      {
         /* all EE's parent EE is the HOSTEE-- this is what we support */
         qdmModsw_getExecEnvFullPathByNameLocked(BEEP_HOSTEE_NAME, eeFullPath, sizeof(eeFullPath));
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->parentExecEnv, eeFullPath);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->name, name);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->vendor, vendor);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->version, version);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->alias, alias);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->X_BROADCOM_COM_MngrAppName, appName);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->X_BROADCOM_COM_ContainerName, containerName);
         REPLACE_STRING_IF_NOT_EQUAL(eeObj->type, description);
         eeObj->X_BROADCOM_COM_IsPreinstall = isPreinstall;
         eeObj->allocatedDiskSpace = diskSpace;
         eeObj->allocatedMemory    = memory;
         if (runLevel < 0)
         {
            /* runLevel is irrelevant. */
            eeObj->initialRunLevel = 0;
            eeObj->currentRunLevel = -1;
            eeObj->initialExecutionUnitRunLevel = -1;
         }
         else
         {
            eeObj->initialRunLevel = runLevel;
            eeObj->currentRunLevel = runLevel;
            eeObj->initialExecutionUnitRunLevel = runLevel;
         }
         eeObj->X_BROADCOM_COM_MngrEid = eeEid;

         if ((ret = cmsObj_set(eeObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of Execution Environment entry failed, ret=%d, du->uuid %s", ret, eeObj->name);
         }
         else
         {
            cmsLog_debug("Execution Environment entry %s is added", eeObj->name);
         }

         cmsObj_free((void **) &eeObj);
      }
      else
      {
         cmsLog_error("Could not get newly created Execution Environment object! ret=%d", ret);
      }
   }

   cmsLck_releaseLock();
   
   return ret;
}


CmsRet modsw_updateEeEntry(const char *fullPath,
                           const char *version,
                           const char *vendor,
                           const char *appName,
                           const char *description,
                           const char *containerName,
                           int diskSpace,
                           int memory,
                           int runLevel,
                           CmsEntityId eeEid)
{
   CmsRet ret = CMSRET_UNKNOWN_EE;
   MdmPathDescriptor pathDesc;
   ExecEnvObject *eeObj = NULL;

   cmsLog_debug("fullPath: %s, version: %s, vendor: %s, appName: %s, description: %s",
                fullPath, version, vendor, appName, description);

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
    }

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));

   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot convert full path to path descriptor. Ret = %d", ret);
      goto out;
   }

   ret = cmsObj_get(pathDesc.oid,
                    &(pathDesc.iidStack),
                    OGF_NO_VALUE_UPDATE, 
                    (void **)&eeObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get ExecEnvObject by full path %s. Ret = %d",
                   fullPath, ret);
      goto out;
   }

   /* update a EE entry into the EE table */
   cmsLog_debug("Updating ExecEnvObject name=%s, vendor=%s", eeObj->name, vendor);

   REPLACE_STRING_IF_NOT_EQUAL(eeObj->version, version);
   REPLACE_STRING_IF_NOT_EQUAL(eeObj->vendor, vendor);
   REPLACE_STRING_IF_NOT_EQUAL(eeObj->X_BROADCOM_COM_MngrAppName, appName);
   REPLACE_STRING_IF_NOT_EQUAL(eeObj->type, description);
   REPLACE_STRING_IF_NOT_EQUAL(eeObj->X_BROADCOM_COM_ContainerName, containerName);
   eeObj->allocatedDiskSpace = diskSpace;
   eeObj->allocatedMemory    = memory;
   if (runLevel < 0)
   {
      /* runLevel is irrelevant. set initalRunLevel to 0 as default. */
      eeObj->initialRunLevel = 0;
      eeObj->currentRunLevel = -1;
      eeObj->initialExecutionUnitRunLevel = -1;
   }
   else
   {
      eeObj->initialRunLevel = runLevel;
      eeObj->currentRunLevel = runLevel;
      eeObj->initialExecutionUnitRunLevel = runLevel;
   }
   eeObj->X_BROADCOM_COM_MngrEid = eeEid;

   if ((ret = cmsObj_set(eeObj, &(pathDesc.iidStack))) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of Execution Environment entry failed, ret=%d, du->uuid %s", ret, eeObj->name);
   }
   else
   {
      cmsLog_debug("Execution Environment entry %s is updated", eeObj->name);
   }

   cmsObj_free((void **) &eeObj);

out:

   cmsLck_releaseLock();
   
   return ret;
}


CmsRet modsw_enableEe(const char *name, const char *version)
{
   CmsRet ret = CMSRET_UNKNOWN_EE;
   ExecEnvObject *eeObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }
   
   while (!found &&
          cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                              &iidStack, 
                              OGF_NO_VALUE_UPDATE,
                              (void **) &eeObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("name %s, version %s, eeObj->name %s", name, version, eeObj->name);
      
      if (!cmsUtl_strcmp(eeObj->name, name) && !cmsUtl_strcmp(eeObj->version, version))
      {
         found = TRUE;
         eeObj->enable = TRUE;

         if ((ret = cmsObj_set(eeObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of Execution Environment entry failed, ret=%d, du->uuid %s", ret, eeObj->name);
         }
         else
         {
            cmsLog_debug("Execution Environment entry %s enabled", eeObj->name);
            ret = CMSRET_SUCCESS;
         }
      }

      cmsObj_free((void **) &eeObj);
   }

   cmsLck_releaseLock();

   return ret;
   
}


CmsRet modsw_getEeContainerName(const char *name, const char *version,
                                char *containerName, UINT32 nameLen)
{
   CmsRet ret = CMSRET_UNKNOWN_EE;
   ExecEnvObject *eeObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   *containerName = '\0';
   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }
   
   while (!found &&
          cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &eeObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("name %s, version %s, eeObj->name %s", name, version, eeObj->name);
      
      if (!cmsUtl_strcmp(eeObj->name, name) && !cmsUtl_strcmp(eeObj->version, version))
      {
         found = TRUE;
         cmsUtl_strncpy(containerName, eeObj->X_BROADCOM_COM_ContainerName, nameLen);
         ret = CMSRET_SUCCESS;
      }

      cmsObj_free((void **) &eeObj);
   }

   cmsLck_releaseLock();

   return ret;
}


static CmsRet getDuObjectByUuidVersionEeFullPathLocked
   (const char *uuid,
    const char *version,
    const char *eeFullPath,
    DUObject **duObject,
    InstanceIdStack *iidStack)
{
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(iidStack);

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DU, iidStack, (void **)duObject)) == CMSRET_SUCCESS)
   {
      if (version == NULL || version[0] == '\0')
      {
         found = (cmsUtl_strcmp((*duObject)->UUID, uuid) == 0 &&
                  cmsUtl_strcmp((*duObject)->executionEnvRef, eeFullPath) == 0);
      }
      else
      {
         found = (cmsUtl_strcmp((*duObject)->UUID, uuid) == 0 &&
                  cmsUtl_strcmp((*duObject)->version, version) == 0 &&
                  cmsUtl_strcmp((*duObject)->executionEnvRef, eeFullPath) == 0);
      }

      if (found == FALSE)
      {
         /* only free object if not a match.  if match, duObject is returned
          * to the caller, who is responsible for freeing.
          */
         cmsObj_free((void **)duObject);
      }
   }

   if (found == FALSE)
   {
      ret = CMSRET_OBJECT_NOT_FOUND;
   }

   return ret;
}


static CmsRet getDuObjectByDuidLocked(const char *duid,
                                      DUObject **duObject,
                                      InstanceIdStack *iidStack)
{
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(iidStack);

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DU, iidStack, (void **)duObject)) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp((*duObject)->DUID, duid) == 0);

      if (!found)
      {
         /* only free object if not a match.  if match, duObject is returned
          * to the caller, who is responsible for freeing.
          */
         cmsObj_free((void **)duObject);
      }
   }

   if (!found)
   {
      ret = CMSRET_OBJECT_NOT_FOUND;
   }

   return ret;
}


/** dup the given string, but if orig string ends with {i}, replace with
 *  instance id.
 */
static char *dupStringSpecialReplace(const char *orig, UINT32 iid)
{
   UINT32 len;
   char *dest;

   if (orig == NULL)
   {
      cmsLog_error("orig string is NULL?!");
      return NULL;
   }

   len = cmsUtl_strlen(orig);
   if (len > 3 &&
       orig[len-3] == '{' &&
       orig[len-2] == 'i' &&
       orig[len-1] == '}')
   {
      dest = cmsMem_alloc(len+16, ALLOC_ZEROIZE);
      if (dest == NULL)
      {
         cmsLog_error("failed to allocate %d bytes", len+16);
         return NULL;
      }

      sprintf(dest, "%s", orig);
      /* replace the {i} with iid */
      sprintf(&(dest[len-3]), "%d", iid);
      return dest;
   }

   /* if we are still here, then its just a normal strdup */
   dest = cmsMem_strdup(orig);

   return dest;
}


CmsRet modsw_addDuEntryLocked(const char *uuid, const char *duVerion,
                              const char *duid, const char *url,
                              const char *username, const char *password,
                              const char *execEnvPath, int isPreinstall,
                              char *outDuid, UINT32 outDuidLen)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if (cmsUtl_strlen(execEnvPath) == 0)
   {
      cmsLog_error("No execEnvPath, there must be one by now!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* first try to find the UUID, must not already exist */
   if (cmsUtl_strlen(uuid) != 0)
   {
      ret = getDuObjectByUuidVersionEeFullPathLocked(uuid,
                                                     duVerion,
                                                     execEnvPath,
                                                     &duObject,
                                                     &iidStack);
      if (ret == CMSRET_SUCCESS)
      {
         cmsLog_error("DUObject uuid=%s already exists!", uuid);
         cmsObj_free((void **) &duObject);
         cmsLck_releaseLock();
         return CMSRET_DU_DUPLICATE;
      }
   }

   /* add a DU entry into the DU table */
   cmsLog_debug("Creating DUObject UUID=%s execEnvPath=%s", uuid, execEnvPath);


   if ((ret = cmsObj_addInstance(MDMOID_DU, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("add instance of Deployment Unit returned %d", ret);
   }
   else
   {
      ret = cmsObj_get(MDMOID_DU, &iidStack, 0, (void **) &duObject);
      if (ret == CMSRET_SUCCESS)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(duObject->DUID);
         duObject->DUID = dupStringSpecialReplace(duid, PEEK_INSTANCE_ID(&iidStack));
         if (cmsUtl_strlen(uuid) != 0)
         {
            REPLACE_STRING_IF_NOT_EQUAL(duObject->UUID, uuid);
         }
         REPLACE_STRING_IF_NOT_EQUAL(duObject->URL, url);
         REPLACE_STRING_IF_NOT_EQUAL(duObject->X_BROADCOM_COM_Username, username);
         REPLACE_STRING_IF_NOT_EQUAL(duObject->X_BROADCOM_COM_Password, password);
         REPLACE_STRING_IF_NOT_EQUAL(duObject->executionEnvRef, execEnvPath);
         REPLACE_STRING_IF_NOT_EQUAL(duObject->version, duVerion);
         duObject->X_BROADCOM_COM_IsPreinstall = isPreinstall;

         if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of DU entry failed, ret=%d, du->uuid %s", ret, duObject->UUID);
         }
         else
         {
            cmsLog_debug("DU entry added");
            cmsUtl_strncpy(outDuid, duObject->DUID, outDuidLen);
         }
         cmsObj_free((void **) &duObject);
      }
      else
      {
         cmsLog_error("Could not get newly created DU object! ret=%d", ret);
      }
   }
   
   return ret;
}


CmsRet modsw_addDuEntry(const char *uuid, const char *duVerion,
                        const char *duid, const char *url,
                        const char *username, const char *password,
                        const char *execEnvPath, int isPreinstall,
                        char *outDuid, UINT32 outDuidLen)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
   }

   ret = modsw_addDuEntryLocked(uuid, duVerion, duid, url,
                                username, password, execEnvPath,
                                isPreinstall, outDuid, outDuidLen);

   cmsLck_releaseLock();

   return ret;
}


CmsRet modsw_deleteDuEntryLocked(const char *uuid,
                                 const char *version,
                                 const char *execEnvPath)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("uuid=%s, version=%s, execEnvPath %s", uuid, version, execEnvPath);

   if (cmsUtl_strlen(uuid) == 0)
   {
      cmsLog_error("uuid must be provided");
      return CMSRET_INVALID_ARGUMENTS;
   }

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid,
                                                  version,
                                                  execEnvPath,
                                                  &duObject,
                                                  &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject, uuid=%s", uuid);
      return ret;
   }

   if ((ret = cmsObj_deleteInstance(MDMOID_DU, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("deleteInstance failed, ret=%d", ret);
   }

   cmsObj_free((void **) &duObject);

   return ret;
}


CmsRet modsw_deleteDuEntry(const char *uuid,
                           const char *version,
                           const char *execEnvPath)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
   }

   ret = modsw_deleteDuEntryLocked(uuid, version, execEnvPath);

   cmsLck_releaseLock();

   return ret;
}


CmsRet modsw_setDuStatusLocked(const char *uuid, const char *versionStr,
                               const char *eeFullPath, const char *duStatus)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char dateTimeBuf[BUFLEN_64];
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("uuid=%s version=%s eeFullPath=%s duStatus=%s",
                uuid, versionStr, eeFullPath, duStatus);

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, versionStr, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFUllPath=%s",
                    uuid, versionStr, eeFullPath);
      return ret;
   }

   REPLACE_STRING_IF_NOT_EQUAL(duObject->status, duStatus);

   /*
    * Starttime and completetime should be tracked in the requestor, not
    * in the data model.  When we delete a DU, we still have to track
    * start and complete time elsewhere, so why not do it for all operations.
    */
   cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
   if (!cmsUtl_strcmp(duStatus, MDMVS_X_BROADCOM_COM_DOWNLOADING) ||
       !cmsUtl_strcmp(duStatus, MDMVS_INSTALLING) ||
       !cmsUtl_strcmp(duStatus, MDMVS_UPDATING) ||
       !cmsUtl_strcmp(duStatus, MDMVS_UNINSTALLING))
   {
      /* these status indicate beginning an action, so update starttime */
      CMSMEM_REPLACE_STRING(duObject->X_BROADCOM_COM_startTime, dateTimeBuf);
   }
   else
   {
      /* all other statuses indicate end of action, so update completetime */
      CMSMEM_REPLACE_STRING(duObject->X_BROADCOM_COM_completeTime, dateTimeBuf);
   }

   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, du->uuid %s", ret, duObject->UUID);
   }

   cmsObj_free((void **) &duObject);

   return ret;
}


CmsRet modsw_setDuStatus(const char *uuid, const char *versionStr, 
                         const char *eeFullPath, const char *duStatus)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
   }

   ret = modsw_setDuStatusLocked(uuid, versionStr, eeFullPath, duStatus);

   cmsLck_releaseLock();

   return ret;
}


CmsRet modsw_setDuStatusByDuidLocked(const char *duid, const char *duStatus)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char dateTimeBuf[BUFLEN_64];
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("duid=%s duStatus=%s", duid, duStatus);

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return ret;
   }

   REPLACE_STRING_IF_NOT_EQUAL(duObject->status, duStatus);

   /*
    * Starttime and completetime should be tracked in the requestor, not
    * in the data model.  When we delete a DU, we still have to track
    * start and complete time elsewhere, so why not do it for all operations.
    */
   cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
   if (!cmsUtl_strcmp(duStatus, MDMVS_X_BROADCOM_COM_DOWNLOADING) ||
       !cmsUtl_strcmp(duStatus, MDMVS_INSTALLING) ||
       !cmsUtl_strcmp(duStatus, MDMVS_UPDATING) ||
       !cmsUtl_strcmp(duStatus, MDMVS_UNINSTALLING))
   {
      /* these status indicate beginning an action, so update starttime */
      CMSMEM_REPLACE_STRING(duObject->X_BROADCOM_COM_startTime, dateTimeBuf);
   }
   else
   {
      /* all other statuses indicate end of action, so update completetime */
      CMSMEM_REPLACE_STRING(duObject->X_BROADCOM_COM_completeTime, dateTimeBuf);
   }

   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, du->uuid %s", ret, duObject->UUID);
   }

   cmsObj_free((void **) &duObject);

   return ret;
}


CmsRet modsw_setDuStatusByDuid(const char *duid, const char *duStatus)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
   }

   ret = modsw_setDuStatusByDuidLocked(duid, duStatus);

   cmsLck_releaseLock();

   return ret;
}


void modsw_setDuInfo(const char *uuid, const char *duMsgversionStr,
                     const char *name, const char *vendor,
                     const char *alias, const char *version,
                     const char *description, const char *eeFullPath)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("uuid=%s name=%s  duMsgversionStr=%s  version=%s description=%s eeFullPath=%s",
                 uuid, name, duMsgversionStr, version, description, eeFullPath);

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return;
   }

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, duMsgversionStr, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFUllPath=%s",
                    uuid, duMsgversionStr, eeFullPath);
      return;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(duObject->alias);
   duObject->alias = dupStringSpecialReplace(alias, PEEK_INSTANCE_ID(&iidStack));

   CMSMEM_REPLACE_STRING(duObject->name, name);
   CMSMEM_REPLACE_STRING(duObject->UUID, uuid);

   if (vendor != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL(duObject->vendor, vendor);
   }   
   if (version != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL(duObject->version, version);
   }
   if (description != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL(duObject->description, description);
   }
 
   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, du->uuid %s", ret, duObject->UUID);
   }

   cmsObj_free((void **) &duObject);

   cmsLck_releaseLock();

   cmsLog_debug("Exit");
   
   return;
}


void modsw_setDuInfoByDuid(const char *duid, const char *uuid,
                           const char *name, const char *vendor,
                           const char *alias, const char *version,
                           const char *description)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("duid=%s uuid=%s name=%s vendor=%s alias=%s version=%s description=%s",
                 duid, uuid, name, vendor, alias, version, description);

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return;
   }

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(duObject->alias);
   duObject->alias = dupStringSpecialReplace(alias, PEEK_INSTANCE_ID(&iidStack));

   CMSMEM_REPLACE_STRING(duObject->name, name);
   CMSMEM_REPLACE_STRING(duObject->UUID, uuid);

   if (vendor != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL(duObject->vendor, vendor);
   }   
   if (version != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL(duObject->version, version);
   }
   if (description != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL(duObject->description, description);
   }
 
   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, du->uuid %s", ret, duObject->UUID);
   }

   cmsObj_free((void **) &duObject);

   cmsLck_releaseLock();

   cmsLog_debug("Exit");
   
   return;
}


void modsw_setDuUrlByDuid(const char *duid, const char *url)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("duid=%s url=%s", duid, url);

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return;
   }

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return;
   }

   REPLACE_STRING_IF_NOT_EQUAL(duObject->URL, url);
 
   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, duid %s", ret, duid);
   }

   cmsObj_free((void **) &duObject);

   cmsLck_releaseLock();

   cmsLog_debug("Exit");
   
   return;
}


CmsRet modsw_getDuNameLocked(const char *uuid, const char *versionStr,
                             const char *eeFullPath,
                             char *name, UINT32 nameLen)
{
   DUObject *duObject = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, versionStr, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFUllPath=%s",
                    uuid, versionStr, eeFullPath);
      return ret;
   }

   /*
    * If we are trying to delete a DUObject which failed to download, then
    * DUObject->name might be NULL.  So must use cmsUtl_strncpy to protect
    * against SEGV.
    */
   cmsUtl_strncpy(name, duObject->name, nameLen);

   cmsObj_free((void **) &duObject);

   return ret;
}


void modsw_setDuResolvedLocked(const char *uuid, const char *versionStr,
                               const char *eeFullPath, UBOOL8 resolved)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("uuid=%s versionStr=%s eeFullPath=%s resolved=%d",
                uuid, versionStr, eeFullPath, resolved);

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, versionStr, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFullPath=%s",
                    uuid, versionStr, eeFullPath);
      return;
   }

   duObject->resolved = resolved;

   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d uuid=%s version=%s eeFullPath=%s",
                   ret, uuid, versionStr, eeFullPath);
   }

   cmsObj_free((void **) &duObject);

   return;
}


void modsw_setDuResolved(const char *uuid, const char *versionStr,
                         const char *eeFullPath, UBOOL8 resolved)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return;
   }

   modsw_setDuResolvedLocked(uuid, versionStr, eeFullPath, resolved);

   cmsLck_releaseLock();

   return;
}


void modsw_setDuResolvedByDuidLocked(const char *duid, UBOOL8 resolved)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("duid=%s resolved=%d", duid, resolved);

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return;
   }

   duObject->resolved = resolved;

   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d duid=%s", ret, duid);
   }

   cmsObj_free((void **) &duObject);

   return;
}


void modsw_setDuResolvedByDuid(const char *duid, UBOOL8 resolved)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return;
   }

   modsw_setDuResolvedByDuidLocked(duid, resolved);

   cmsLck_releaseLock();

   return;
}


CmsRet modsw_setDuInfoAllLocked
   (const char *uuid, const char *version, const char *eeRef,
    const char *name, const char *vendor, const char *alias,
    const char *newVersion, const char *description, const char *url,
    const char *username, const char *password, const char *status,
    const char *eeList, const UBOOL8 resolved)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("uuid=%s version=%s eeRef=%s name=%s vendor=%s alias=%s \
newVersion=%s description=%s url=%s, username=%s password=%s status=%s \
resolved=%d", uuid, version, eeRef, name, vendor, alias, newVersion,
description, url, username, password, status, resolved);

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, version, eeRef,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeRef=%s",
                    uuid, version, eeRef);
      return ret;
   }

   REPLACE_STRING_IF_NOT_EQUAL(duObject->name, name);

   REPLACE_STRING_IF_NOT_EQUAL(duObject->vendor, vendor);

   CMSMEM_FREE_BUF_AND_NULL_PTR(duObject->alias);
   duObject->alias = dupStringSpecialReplace(alias, PEEK_INSTANCE_ID(&iidStack));

   REPLACE_STRING_IF_NOT_EQUAL(duObject->version, newVersion);

   REPLACE_STRING_IF_NOT_EQUAL(duObject->description, description);

   REPLACE_STRING_IF_NOT_EQUAL(duObject->URL, url);

   REPLACE_STRING_IF_NOT_EQUAL(duObject->X_BROADCOM_COM_Username,
                               username);

   REPLACE_STRING_IF_NOT_EQUAL(duObject->X_BROADCOM_COM_Password,
                               password);

   REPLACE_STRING_IF_NOT_EQUAL(duObject->status, status);
 
   REPLACE_STRING_IF_NOT_EQUAL(duObject->X_BROADCOM_COM_ExecutionEnvList,
                               eeList);

   duObject->resolved = resolved;

   if ((ret = cmsObj_set(duObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of DU entry failed, ret=%d, uuid=%s, version=%s, eeRef=%s",
                   ret, uuid, version, eeRef);
   }

   cmsObj_free((void **) &duObject);
   
   return ret;
}


CmsRet modsw_setDuInfoAll
   (const char *uuid, const char *version, const char *eeRef,
    const char *name, const char *vendor, const char *alias,
    const char *newVersion, const char *description, const char *url,
    const char *username, const char *password, const char *status,
    const char *eeList, const UBOOL8 resolved)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
   }

   ret = modsw_setDuInfoAllLocked
      (uuid, version, eeRef, name, vendor, alias, newVersion,
       description, url, username, password, status, eeList, resolved);

   cmsLck_releaseLock();

   return ret;
}


CmsRet modsw_findDuByUuidVersionEeFullPathLocked(const char *uuid,
                                                 const char *version,
                                                 const char *eeFullPath)
{
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
   UBOOL8 found = FALSE;
   DUObject *duObject = NULL;
   InstanceIdStack iidStack;

   cmsLog_debug("uuid=%s version=%s eeFullPath=%s", uuid, version, eeFullPath);

   INIT_INSTANCE_ID_STACK(&iidStack);

   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&duObject) == CMSRET_SUCCESS)
   {
      if (version == NULL || version[0] == '\0')
      {
         found = (cmsUtl_strcmp(duObject->UUID, uuid) == 0 &&
                  cmsUtl_strcmp(duObject->executionEnvRef, eeFullPath) == 0);
      }
      else
      {
         found = (cmsUtl_strcmp(duObject->UUID, uuid) == 0 &&
                  cmsUtl_strcmp(duObject->version, version) == 0 &&
                  cmsUtl_strcmp(duObject->executionEnvRef, eeFullPath) == 0);
      }

      cmsObj_free((void **)&duObject);
   }

   if (found == TRUE)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;
}


CmsRet modsw_getDuidByUuidVersionEeFullPath(const char *uuid,
                                            const char *version,
                                            const char *eeFullPath,
                                            char *duid,
                                            UINT32 duidLen)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("uuid=%s version=%s eeFullPath=%s", uuid, version, eeFullPath);

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return ret;
   }

   ret = qdmModsw_getDuidByUuidVersionEeFullPathLocked(uuid,
                                                       version,
                                                       eeFullPath,
                                                       duid,
                                                       duidLen);

   cmsLck_releaseLock();
   
   return ret;
}


CmsRet modsw_findDuByUrlEeFullPathLocked(const char *url,
                                         const char *eeFullPath)
{
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
   UBOOL8 found = FALSE;
   DUObject *duObject = NULL;
   InstanceIdStack iidStack;
   
   cmsLog_debug("url=%s eeFullPath=%s", url, eeFullPath);
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   
   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&duObject) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp(duObject->URL, url) == 0 &&
               cmsUtl_strcmp(duObject->executionEnvRef, eeFullPath) == 0);
      
      cmsObj_free((void **)&duObject);
   }
   
   if (found == TRUE)
   {
      ret = CMSRET_SUCCESS;
   }
   
   return ret;
}


UBOOL8 modsw_isVersionExistedByUuidVersionEeFullPath(const char *uuid,
                                                     const char *version,
                                                     const char *eeFullPath)
{
   UBOOL8 found = FALSE;
   DUObject *duObject = NULL;
   InstanceIdStack iidStack;

   cmsLog_debug("uuid=%s version=%s eeFullPath=%s", uuid, version, eeFullPath);

   if (cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock");
       cmsLck_dumpInfo();
       return found;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);

   /* Find any version of DU that matches given version */
   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&duObject) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(duObject->UUID, uuid) == 0 &&
          cmsUtl_strcmp(duObject->executionEnvRef, eeFullPath) == 0 &&
          beepUtil_strverscmp(duObject->version, version) == 0)
      {
         found = TRUE;
      }

      cmsObj_free((void **)&duObject);
   }

   cmsLck_releaseLock();

   return found;
}


UBOOL8 modsw_areMultipleDuExistedByUuidEeFullPathLocked(const char *uuid,
                                                        const char *eeFullPath)
{
   UBOOL8 existed = FALSE;
   UINT32 count = 0;
   DUObject *duObject = NULL;
   InstanceIdStack iidStack;

   cmsLog_debug("uuid=%s eeFullPath=%s", uuid, eeFullPath);

   INIT_INSTANCE_ID_STACK(&iidStack);

   /* Find any DU that matches given uuid and eeFullPath */
   while (cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&duObject) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(duObject->UUID, uuid) == 0 &&
          cmsUtl_strcmp(duObject->executionEnvRef, eeFullPath) == 0)
      {
         count++;
      }

      cmsObj_free((void **)&duObject);
   }

   if (count > 1)
   {
      existed = TRUE;
   }

   return existed;
}


CmsRet modsw_addEuPathToDuLocked(const char *uuid, const char *version,
                                 const char *eeFullPath, const char *euFullPath)
{
   char allEuFullPathStringBuf[MDM_MULTI_FULLPATH_BUFLEN];
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;;

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, version, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFullPath=%s",
                    uuid, version, eeFullPath);
      return ret;
   }

   /* Need to append euFullPath to executionUnitList */
   cmsUtl_strncpy(allEuFullPathStringBuf, duObject->executionUnitList, sizeof(allEuFullPathStringBuf));

   ret = cmsUtl_addFullPathToCSL(euFullPath, allEuFullPathStringBuf, sizeof(allEuFullPathStringBuf));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to add %s to duObject->executionUnitList. ret %d", euFullPath, ret);
   }
   else
   {
      CMSMEM_REPLACE_STRING(duObject->executionUnitList, allEuFullPathStringBuf);

      cmsLog_debug("new duExecutionUnitList (len=%d)=%s", 
                   cmsUtl_strlen(duObject->executionUnitList), 
                   duObject->executionUnitList);
      ret = cmsObj_set(duObject, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of DU entry failed, ret=%d, allEuFullPathStringBuf=%s", ret, allEuFullPathStringBuf);
      }
   }
   
   cmsObj_free((void **) &duObject);

   return ret;
}


CmsRet modsw_addEuPathToDuByDuidLocked(const char *duid, const char *euFullPath)
{
   char allEuFullPathStringBuf[MDM_MULTI_FULLPATH_BUFLEN];
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;;

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return ret;
   }

   /* Need to append euFullPath to executionUnitList */
   cmsUtl_strncpy(allEuFullPathStringBuf, duObject->executionUnitList, sizeof(allEuFullPathStringBuf));

   ret = cmsUtl_addFullPathToCSL(euFullPath, allEuFullPathStringBuf, sizeof(allEuFullPathStringBuf));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to add %s to duObject->executionUnitList. ret %d", euFullPath, ret);
   }
   else
   {
      CMSMEM_REPLACE_STRING(duObject->executionUnitList, allEuFullPathStringBuf);

      cmsLog_debug("new duExecutionUnitList (len=%d)=%s", 
                   cmsUtl_strlen(duObject->executionUnitList), 
                   duObject->executionUnitList);
      ret = cmsObj_set(duObject, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of DU entry failed, ret=%d, allEuFullPathStringBuf=%s", ret, allEuFullPathStringBuf);
      }
   }
   
   cmsObj_free((void **) &duObject);

   return ret;
}


CmsRet modsw_deleteEuPathInDuLocked(const char *uuid, const char *version,
                                    const char *eeFullPath, const char *euFullPath)
{
   char allEuFullPathStringBuf[MDM_MULTI_FULLPATH_BUFLEN];
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;;

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, version, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFullPath=%s",
                    uuid, version, eeFullPath);
      return ret;
   }
   
   /* Need to append euFullPath to executionUnitList */
   cmsUtl_strncpy(allEuFullPathStringBuf, duObject->executionUnitList, sizeof(allEuFullPathStringBuf));
   
   cmsUtl_deleteFullPathFromCSL(euFullPath, allEuFullPathStringBuf);

   CMSMEM_REPLACE_STRING(duObject->executionUnitList, allEuFullPathStringBuf);
      
   cmsLog_debug("new duExecutionUnitList (len=%d)=%s", 
                cmsUtl_strlen(duObject->executionUnitList), 
                duObject->executionUnitList);

   ret = cmsObj_set(duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, allEuFullPathStringBuf=%s", ret, allEuFullPathStringBuf);
   }
   
   cmsObj_free((void **) &duObject);
   
   return ret;
}


CmsRet modsw_deleteEuPathInDuByDuidLocked(const char *duid, const char *euFullPath)
{
   char allEuFullPathStringBuf[MDM_MULTI_FULLPATH_BUFLEN];
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;;

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return ret;
   }
   
   /* Need to append euFullPath to executionUnitList */
   cmsUtl_strncpy(allEuFullPathStringBuf, duObject->executionUnitList, sizeof(allEuFullPathStringBuf));
   
   cmsUtl_deleteFullPathFromCSL(euFullPath, allEuFullPathStringBuf);

   CMSMEM_REPLACE_STRING(duObject->executionUnitList, allEuFullPathStringBuf);
      
   cmsLog_debug("new duExecutionUnitList (len=%d)=%s", 
                cmsUtl_strlen(duObject->executionUnitList), 
                duObject->executionUnitList);

   ret = cmsObj_set(duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of DU entry failed, ret=%d, allEuFullPathStringBuf=%s", ret, allEuFullPathStringBuf);
   }
   
   cmsObj_free((void **) &duObject);
   
   return ret;
}


CmsRet modsw_addEuEntryLocked(const char *euId, const char *alias,
                  const char *name, const char *mngrAppName,
                  const char *execEnvLabel, const char *execEnvRef,
                  const char *username, UINT32 bundleId,
                  const char *vendor, const char *version, const char *description,
                  UBOOL8 autoStart, UINT32 autoStartOrder, UINT32 runLevel,
                  UBOOL8 autoRelaunch, UINT32 maxRestarts, UINT32 restartInterval,
                  UINT32 successfulStartPeriod,
                  char *euFullPath, UINT32 euFullPathLen)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   EUObject *euObject=NULL;
   BusClientObject *busClientObject = NULL;
   UINT32 iid;

   cmsLog_debug("enter");

   if ((ret = cmsObj_addInstance(MDMOID_EU, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("add instance of Execution Unit returned %d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_EU, &iidStack, 0, (void **) &euObject)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get newly created EU object, ret=%d", ret);
      return ret;
   }

   /* required arguments */
   iid = PEEK_INSTANCE_ID(&iidStack);

   CMSMEM_FREE_BUF_AND_NULL_PTR(euObject->EUID);
   euObject->EUID = dupStringSpecialReplace(euId, iid);

   CMSMEM_FREE_BUF_AND_NULL_PTR(euObject->alias);
   euObject->alias = dupStringSpecialReplace(alias, iid);

   REPLACE_STRING_IF_NOT_EQUAL(euObject->name, name);
   REPLACE_STRING_IF_NOT_EQUAL(euObject->execEnvLabel, execEnvLabel);
   REPLACE_STRING_IF_NOT_EQUAL(euObject->executionEnvRef, execEnvRef);
   euObject->X_BROADCOM_COM_bundleId = bundleId;
   euObject->autoStart = autoStart;
   euObject->X_BROADCOM_COM_autoStartOrder = autoStartOrder;
   euObject->runLevel = runLevel;
   euObject->X_BROADCOM_COM_autoRelaunch = autoRelaunch;
   euObject->X_BROADCOM_COM_maxRestarts = maxRestarts;
   euObject->X_BROADCOM_COM_restartInterval = restartInterval;
   euObject->X_BROADCOM_COM_successfulStartPeriod = successfulStartPeriod;

   /* optional arguments */
   if (vendor)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->vendor, vendor);
   }
   if (version)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->version, version);
   }
   if (description)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->description, description);
   }
   if (username)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->X_BROADCOM_COM_Username, username);
   }

   if (mngrAppName)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->X_BROADCOM_COM_MngrAppName, mngrAppName);
   }
   
   if ((ret = cmsObj_set(euObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of EU entry failed, ret=%d, eu->description %s", ret, euObject->description);
   }
   else
   {
      cmsLog_debug("EU entry added, iidStack %s",cmsMdm_dumpIidStack(&iidStack));
   }

   if (ret == CMSRET_SUCCESS)
   {
      qdmModsw_getExecUnitFullPathByEuidLocked(euObject->EUID,
                                           euFullPath, euFullPathLen);
   }

   cmsObj_free((void **) &euObject);

   /* use EUObject iidStack to get BusClientObject since they have the same iidStack */
   if ((ret = cmsObj_get(MDMOID_BUS_CLIENT, &iidStack, 0, (void **) &busClientObject)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get BusClientObject, ret=%d", ret);
      return ret;
   }

   REPLACE_STRING_IF_NOT_EQUAL(busClientObject->name, name);

   if ((ret = cmsObj_set(busClientObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of BusClientObject entry failed, ret=%d, name %s", ret, name);
   }

   cmsObj_free((void **) &busClientObject);

   return ret;
}



CmsRet modsw_updateEuEntryLocked(const char *euId, const char *name,
                     const char *vendor, const char *version, const char *description,
                     UBOOL8 autoStart, UINT32 autoStartOrder, UINT32 runLevel,
                     UBOOL8 autoRelaunch, UINT32 maxRestarts, UINT32 restartInterval,
                     UINT32 successfulStartPeriod,
                     char *username, UINT32 usernameLen,
                     char *euFullPath, UINT32 euFullPathLen)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   EUObject *euObject = NULL;
   BusClientObject *busClientObject = NULL;
   UBOOL8 found = FALSE;

   cmsLog_debug("enter");

   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&euObject) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObject->EUID, euId) == 0)
      {
         found = TRUE;
         break;  // we still need this object, so don't free yet.
      }

      cmsObj_free((void **)&euObject);
   }

   if (found == FALSE)
   {
      cmsLog_error("Could not find EU object with euID = %s", euId);
      return CMSRET_OBJECT_NOT_FOUND;
   }

   REPLACE_STRING_IF_NOT_EQUAL(euObject->name, name);
   
   euObject->autoStart = autoStart;
   euObject->X_BROADCOM_COM_autoStartOrder = autoStartOrder;
   euObject->runLevel  = runLevel;
   euObject->X_BROADCOM_COM_autoRelaunch = autoRelaunch;
   euObject->X_BROADCOM_COM_maxRestarts = maxRestarts;
   euObject->X_BROADCOM_COM_restartInterval = restartInterval;
   euObject->X_BROADCOM_COM_successfulStartPeriod = successfulStartPeriod;

   /* optional arguments */
   if (vendor)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->vendor, vendor);
   }
   if (version)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->version, version);
   }
   if (description)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->description, description);
   }

   if ((ret = cmsObj_set(euObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of EU entry failed, ret=%d, eu->description %s", ret, euObject->description);
      cmsObj_free((void **)&euObject);
      return ret;
   }
   else
   {
      cmsLog_debug("EU entry updated, iidStack %s", cmsMdm_dumpIidStack(&iidStack));
   }

   cmsUtl_strncpy(username, euObject->X_BROADCOM_COM_Username, usernameLen);

   qdmModsw_getExecUnitFullPathByEuidLocked(euObject->EUID,
                                            euFullPath, euFullPathLen);

   cmsObj_free((void **) &euObject);

   /* use EUObject iidStack to get BusClientObject since they have the same iidStack */
   if ((ret = cmsObj_get(MDMOID_BUS_CLIENT,
                         &iidStack,
                         0,
                         (void **) &busClientObject)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get BusClientObject, ret=%d", ret);
      return ret;
   }

   REPLACE_STRING_IF_NOT_EQUAL(busClientObject->name, name);

   if ((ret = cmsObj_set(busClientObject, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of BusClientObject entry failed, ret=%d, name %s", ret, name);
   }

   cmsObj_free((void **) &busClientObject);

   return ret;
}


void modsw_getEuNameLocked(const char *euFullPath, char *name, UINT32 nameLen)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      cmsUtl_strncpy(name, euObject->name, nameLen);
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return;
}


UBOOL8 modsw_isEuExistedByEuNameDuidLocked(const char *euName,
                                           const char *duid)
{
   UBOOL8 found = FALSE;
   char exUnitList[MDM_MULTI_FULLPATH_BUFLEN]={0};
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("euName=%s, duid=%d", euName, duid);
   
   ret = modsw_getExcutionUnitListByDuidLocked(duid, exUnitList, sizeof(exUnitList));

   if (ret == CMSRET_SUCCESS)
   {
      char *pToken = NULL;
      char *pLast = NULL;
      char euFullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      char buf[BUFLEN_64]={0};

      /* Loop through execution unit list to get EU full path then EU name */
      for (pToken = strtok_r(exUnitList, ", ", &pLast);
           pToken != NULL && found == FALSE;
           pToken = strtok_r(NULL, ", ", &pLast))
      {
         cmsUtl_strcpy(euFullPath, pToken);

         /* Get current execution unit name */
         ret = qdmModsw_getExecutionUnitParamsByEuFullPathLocked(euFullPath,
                                                                 buf, sizeof(buf),
                                                                 NULL, 0, /* don't need euid */
                                                                 NULL, 0, /* don't need username */
                                                                 NULL, 0, /* don't need euStatus */
                                                                 NULL, 0); /* don't need mngrAppName */

         if (cmsUtl_strcmp(euName, buf) == 0)
         {
            found = TRUE;
         }
      }
   }

   return found;
}


UBOOL8 modsw_isEuExistedByEuNameDuid(const char *euName,
                                     const char *duid)
{
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return found;
   }

   found = modsw_isEuExistedByEuNameDuidLocked(euName, duid);

   cmsLck_releaseLock();

   return found;
}


UBOOL8 modsw_isEuAutoStartLocked(const char *euFullPath)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;
   UBOOL8 isAutoStart=FALSE;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return FALSE;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      isAutoStart = euObject->autoStart;
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return isAutoStart;
}


void modsw_setEuAutoStartLocked(const char *euFullPath, UBOOL8 autoStart)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   cmsLog_debug("euFullPath=%s autoStart=%d", euFullPath, autoStart);

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      euObject->autoStart = autoStart;
      ret = cmsObj_set(euObject, &(pathDesc.iidStack));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of euObject autostart failed, ret=%d", ret);
      }
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return;
}


void modsw_setEuStatusLocked(const char *euFullPath, const char *status)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->status, status);
      ret = cmsObj_setFlags(euObject, &(pathDesc.iidStack), OSF_NO_RCL_CALLBACK);
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of euObject status failed, ret=%d", ret);
      }
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return;
}


void modsw_setEuStatus(const char *euFullPath, const char *status)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   modsw_setEuStatusLocked(euFullPath, status);

   cmsLck_releaseLock();

   return;
}


void modsw_stopEuRestartLocked(const char *euFullPath)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      /* set restartCount to maxRestarts + 1 to stop eu from further restart.
       * Note that the range of maxRestarts is [0:FFFF] and restartCount is
       * [0:FFFFFFFF].
       */
      euObject->X_BROADCOM_COM_restartCount = euObject->X_BROADCOM_COM_maxRestarts + 1;

      ret = cmsObj_setFlags(euObject, &(pathDesc.iidStack), OSF_NO_RCL_CALLBACK);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of euObject status failed, ret=%d", ret);
      }
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return;
}


void modsw_resetEuRestartLocked(const char *euFullPath)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      euObject->X_BROADCOM_COM_restartCount = 0;

      ret = cmsObj_setFlags(euObject, &(pathDesc.iidStack), OSF_NO_RCL_CALLBACK);
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of euObject status failed, ret=%d", ret);
      }
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return;
}


void modsw_setEuFaultCodeByEuFullPathLocked(const char *euFullPath, const char *faultCode)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret == CMSRET_SUCCESS)
   {
      REPLACE_STRING_IF_NOT_EQUAL(euObject->executionFaultCode, faultCode);
      ret = cmsObj_setFlags(euObject, &(pathDesc.iidStack), OSF_NO_RCL_CALLBACK);
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of euObject status failed, ret=%d", ret);
      }
      cmsObj_free((void **) &euObject);
   }
   else
   {
      cmsLog_error("get eu object failed ret=%d", ret);
   }

   return;
}


void modsw_setEuFaultCodeByEuFullPath(const char *euFullPath, const char *faultCode)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   modsw_setEuFaultCodeByEuFullPathLocked(euFullPath, faultCode);

   cmsLck_releaseLock();

   return;
}


void modsw_setEuFaultCodeByEuidLocked(const char *euid, const char *faultCode)
{
   CmsRet ret;
   UBOOL8 found = FALSE;
   EUObject *euObject=NULL;
   InstanceIdStack iidStack;
   
   cmsLog_debug("euid=%s", euid);
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   
   /* Find any version of DU that matches given version */
   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&euObject) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObject->EUID, euid) == 0)
      {
         found = TRUE;
         REPLACE_STRING_IF_NOT_EQUAL(euObject->executionFaultCode, faultCode);
         ret = cmsObj_setFlags(euObject, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of euObject status failed, ret=%d", ret);
         }
      }
      
      cmsObj_free((void **)&euObject);
   }

   return;
}


void modsw_setEuFaultCodeByEuid(const char *euid, const char *faultCode)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   modsw_setEuFaultCodeByEuidLocked(euid, faultCode);

   cmsLck_releaseLock();

   return;
}


void modsw_setEuRequestedStateLocked(const char *euFullPath, const char *requestedState)
{
   MdmPathDescriptor pathDesc;
   EUObject *euObject=NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), OGF_NO_VALUE_UPDATE,
                    (void **)&euObject);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("get eu object failed ret=%d", ret);
      return;
   }

  REPLACE_STRING_IF_NOT_EQUAL(euObject->requestedState, requestedState);
  ret = cmsObj_set(euObject, &(pathDesc.iidStack));
  if (ret != CMSRET_SUCCESS)
  {
     cmsLog_error("set of euObject requestedState failed, ret=%d", ret);
  }
  cmsObj_free((void **) &euObject);
  

  return;
}


void modsw_deleteEuEntryLocked(const char *euFullPath)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("deleting %s", euFullPath);

   ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s", euFullPath);
      return;
   }

   ret = cmsObj_deleteInstance(pathDesc.oid, &(pathDesc.iidStack));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("delete object failed ret=%d", ret);
   }

   return;
}


CmsRet modsw_getExcutionUnitListFromDuLocked(const char *uuid, const char *version,
                                             const char *eeFullPath,
                                             char *excutionUnitList, UINT32 excutionUnitListLen)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = getDuObjectByUuidVersionEeFullPathLocked(uuid, version, eeFullPath,
                                                  &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject uuid=%s version=%s eeFullPath=%s",
                    uuid, version, eeFullPath);
      return ret;
   }

   if (duObject->executionUnitList == NULL || duObject->executionUnitList[0] == '\0')
   {
      /* the execution unit list is empty */
      cmsObj_free((void **) &duObject);
      return CMSRET_NO_MORE_INSTANCES;
   }

   if (cmsUtl_strlen(duObject->executionUnitList) + 1 > (int) excutionUnitListLen)
   {
      cmsLog_error("executionUnitList %s is too long for bufLen=%d", duObject->executionUnitList, excutionUnitListLen);
      cmsObj_free((void **) &duObject);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   cmsUtl_strncpy(excutionUnitList, duObject->executionUnitList, excutionUnitListLen);

   cmsObj_free((void **) &duObject);

   return ret;

}


CmsRet modsw_getExcutionUnitListByDuidLocked(const char *duid,
                                             char *excutionUnitList, UINT32 excutionUnitListLen)
{
   DUObject *duObject=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = getDuObjectByDuidLocked(duid, &duObject, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("Could not find DUObject duid=%s", duid);
      return ret;
   }

   if (duObject->executionUnitList == NULL || duObject->executionUnitList[0] == '\0')
   {
      /* the execution unit list is empty */
      cmsObj_free((void **) &duObject);
      return CMSRET_NO_MORE_INSTANCES;
   }

   if (cmsUtl_strlen(duObject->executionUnitList) + 1 > (int) excutionUnitListLen)
   {
      cmsLog_error("executionUnitList %s is too long for bufLen=%d", duObject->executionUnitList, excutionUnitListLen);
      cmsObj_free((void **) &duObject);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   cmsUtl_strncpy(excutionUnitList, duObject->executionUnitList, excutionUnitListLen);

   cmsObj_free((void **) &duObject);

   return ret;

}


void modsw_setEuStatusByEuidLocked(const char *euid, const char *status)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   EUObject *euObject = NULL;
   UBOOL8 found = FALSE;

   cmsLog_debug("enter");

   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&euObject) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObject->EUID, euid) == 0)
      {
         found = TRUE;
         REPLACE_STRING_IF_NOT_EQUAL(euObject->status, status);
         ret = cmsObj_setFlags(euObject, &iidStack, OSF_NO_RCL_CALLBACK);
      
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of euObject status failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **)&euObject);
   }
}


void modsw_setEuStatusByEuid(const char *euid, const char *status)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   modsw_setEuStatusByEuidLocked(euid, status);

   cmsLck_releaseLock();

   return;
}


void modsw_setEuIPAddrByEuidLocked(const char *euid, const char *ipaddr)
{
   CmsRet ret;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   EUObject *euObject = NULL;
   UBOOL8 found = FALSE;

   cmsLog_debug("enter");

   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&euObject) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObject->EUID, euid) == 0)
      {
         found = TRUE;
         REPLACE_STRING_IF_NOT_EQUAL(euObject->X_BROADCOM_COM_IPv4Addresses, ipaddr);
         ret = cmsObj_setFlags(euObject, &iidStack, OSF_NO_RCL_CALLBACK);
      
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set euObject IPAddress failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **)&euObject);
   }
}


void modsw_deleteDuEntryByUrlandEeFullPath(const char *url, const char *eeFullPath)
{
   CmsRet ret;
   UBOOL8 found = FALSE;
   DUObject *duObject = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("url=%s eeFullPath=%s", url, eeFullPath);
   
   if ((ret = cmsLck_acquireLockWithTimeout(MODSW_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       return;
   }
   
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&duObject) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp(duObject->URL, url) == 0 &&
               cmsUtl_strcmp(duObject->executionEnvRef, eeFullPath) == 0);
      
      cmsObj_free((void **)&duObject);
   }
   
   if (found)
   {
      if ((ret = cmsObj_deleteInstance(MDMOID_DU, &iidStack)) !=CMSRET_SUCCESS)
      {
         cmsLog_error("deleteInstance failed, ret=%d", ret);
      }

   }
   cmsLck_releaseLock();

   return;
}


CmsRet modsw_setPreinstallNeededRuntimeLocked(UBOOL8 need, UBOOL8 runtime)
{
   CmsRet ret = CMSRET_SUCCESS;
   SwModulesObject *swModObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if ((ret = cmsObj_get(MDMOID_SW_MODULES, &iidStack, 0, (void**)&swModObj)) 
       == CMSRET_SUCCESS)
   {
      swModObj->X_BROADCOM_COM_PreinstallNeeded = need;
      swModObj->X_BROADCOM_COM_PreinstallAtRuntime = runtime;
      ret = cmsObj_set(swModObj, &iidStack);
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set SW Module PreinstallNeeded to %d, ret<%d>",
                      need, ret);
      }
      cmsObj_free((void **)&swModObj);
   }
   else
   {
      cmsLog_error("cannot get SwModulesObject, ret<%d>", ret);
   }

   return ret;
}

CmsRet modsw_getPreinstallOverwriteLocked(UBOOL8 *overwrite)
{
   CmsRet ret = CMSRET_SUCCESS;
   SwModulesObject *swModObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   *overwrite = 0;

   if ((ret = cmsObj_get(MDMOID_SW_MODULES, &iidStack, 0, (void**)&swModObj)) 
       == CMSRET_SUCCESS)
   {
      *overwrite = swModObj->X_BROADCOM_COM_PreinstallOverwrite;
      cmsObj_free((void **)&swModObj);
   }
   else
   {
      cmsLog_error("cannot get SwModulesObject, ret<%d>", ret);
   }

   return ret;
}

UBOOL8 modsw_isPreinstalledEeExistedLocked( void )
{
   UBOOL8 found = FALSE;
   ExecEnvObject *execEnvObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   while (!found && cmsObj_getNextFlags(MDMOID_EXEC_ENV,
                                        &iidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&execEnvObj) == CMSRET_SUCCESS)
   {
      if (execEnvObj->X_BROADCOM_COM_IsPreinstall == TRUE)
      {
         found = TRUE;
      }

      cmsObj_free((void **)&execEnvObj);
   }

   return found;
}

CmsRet modsw_convertToCmsRet(SpdRet spdret)
{
   CmsRet ret;

   switch (spdret)
   {
      case SPDRET_SUCCESS:
         ret = CMSRET_SUCCESS;
         break;

      case SPDRET_ACCESS_DENIED:
         ret = CMSRET_ACCESS_DENIED;
         break;

      case SPDRET_USERNAME_IN_USE:
         ret = CMSRET_USERNAME_IN_USE;
         break;

      case SPDRET_ADD_USER_ERROR:
         ret = CMSRET_ADD_USER_ERROR;
         break;

      case SPDRET_DELETE_USER_ERROR:
         ret = CMSRET_DELETE_USER_ERROR;
         break;

      case SPDRET_USER_NOT_FOUND:
         ret = CMSRET_USER_NOT_FOUND;
         break;

      case SPDRET_FILE_OPEN_ERROR:
         ret = CMSRET_OPEN_FILE_ERROR;
         break;

      case SPDRET_MANIFEST_PARSE_ERROR:
         ret = CMSRET_MANIFEST_PARSE_ERROR;
         break;

      case SPDRET_DB_EE_DUPLICATE:
         ret = CMSRET_EE_DUPLICATE;
         break;

      case SPDRET_INVALID_URL_FORMAT:
         ret = CMSRET_INVALID_URL_FORMAT;
         break;

      case SPDRET_FILE_TRANSFER_UNABLE_CONTACT_FILE_SERVER:
         ret = CMSRET_FILE_TRANSFER_UNABLE_CONTACT_FILE_SERVER;
         break;

      case SPDRET_FILE_TRANSFER_UNABLE_ACCESS_FILE:
         ret = CMSRET_FILE_TRANSFER_UNABLE_ACCESS_FILE;
         break;

      case SPDRET_FILE_TRANSFER_FILE_TIMEOUT:
         ret = CMSRET_FILE_TRANSFER_FILE_TIMEOUT;
         break;

      case SPDRET_FILE_TRANSFER_UNABLE_COMPLETE:
         ret = CMSRET_FILE_TRANSFER_UNABLE_COMPLETE;
         break;

      case SPDRET_FILE_TRANSFER_AUTH_FAILURE:
         ret = CMSRET_FILE_TRANSFER_AUTH_FAILURE;
         break;

      case SPDRET_FILE_TRANSFER_FILE_AUTHENTICATION_ERROR:
         ret = CMSRET_FILE_TRANSFER_FILE_AUTHENTICATION_ERROR;
         break;

      case SPDRET_UNSUPPORTED_FILE_TRANSFER_PROTOCOL:
         ret = CMSRET_UNSUPPORTED_FILE_TRANSFER_PROTOCOL;
         break;

      case SPDRET_EE_UPDATE_VERSION_EXISTED:
         ret = CMSRET_EE_UPDATE_VERSION_EXISTED;
         break;

      case SPDRET_EE_UPDATE_DOWNGRADE_NOT_ALLOWED:
         ret = CMSRET_EE_UPDATE_DOWNGRADE_NOT_ALLOWED;
         break;

      case SPDRET_INVALID_ARGUMENT:
         ret = CMSRET_INVALID_ARGUMENTS;
         break;

      case SPDRET_SW_MODULE_SYSTEM_RESOURCE_EXCEEDED:
         ret = CMSRET_SW_MODULE_SYSTEM_RESOURCE_EXCEEDED;
         break;

      case SPDRET_SYSTEM_RESOURCE_EXCEEDED:
         ret = CMSRET_RESOURCE_EXCEEDED;
         break;

      case SPDRET_OPERATION_NOT_PERMITTED:
         ret = CMSRET_OPERATION_NOT_PERMITTED;
         break;

      case SPDRET_UNINSTALL_IN_PROCESS:
      case SPDRET_UPGRADE_IN_PROCESS:
         ret = CMSRET_OPERATION_IN_PROCESS;
         break;

      case SPDRET_DB_DU_DUPLICATE:
         ret = CMSRET_DU_DUPLICATE;
         break;

      case SPDRET_DB_UNKNOWN_DU:
         ret = CMSRET_DU_UNKNOWN;
         break;

      case SPDRET_DU_EE_MISMATCH:
         ret = CMSRET_DU_EE_MISMATCH;
         break;

      case SPDRET_DU_UPDATE_VERSION_EXISTED:
         ret = CMSRET_DU_UPDATE_VERSION_EXISTED;
         break;

      case SPDRET_DU_UPDATE_DOWNGRADE_NOT_ALLOWED:
         ret = CMSRET_DU_UPDATE_DOWNGRADE_NOT_ALLOWED;
         break;
         
      case SPDRET_BEEP_EE_VERSION_MISMATCH:      
         ret = CMSRET_PARENTEE_EE_VERSION_MISMATCH;
         break;
         
      case SPDRET_INTERNAL_ERROR:
      default:
         ret = CMSRET_INTERNAL_ERROR;
         break;
   }

   return ret;

}  /* End of modsw_convertToCmsRet() */

#endif /* DMP_DEVICE2_SM_BASELINE_1 */
