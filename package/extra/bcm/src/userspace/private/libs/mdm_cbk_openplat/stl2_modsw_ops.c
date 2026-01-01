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

#ifdef DMP_DEVICE2_SM_BASELINE_1

#include <sys/syscall.h>
#include <sys/stat.h>
#include "cms_core.h"
#include "cms_fil.h"
#include "cms_util.h"
#include "cms_msg_modsw.h"
#include "rcl.h"
#include "rut_util.h"
#include "qdm_modsw_ee.h"
#include "openplat.h"
#include "beep_common.h"

/*!\file stl_modsw.c
 * \brief This file contains generic modular sw functions (not specific to
 *        any specific execution env.)
 *
 */

static UBOOL8 isStandaloneApp(const char *eeFullPath)
{
   UBOOL8 ret = FALSE;
   ExecEnvObject *eeObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   while (cmsObj_getNextFlags(MDMOID_EXEC_ENV, &iidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&eeObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, OPS_HOSTEE_NAME))
      {
         MdmPathDescriptor pathDesc;
         char *path = NULL;

         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_EXEC_ENV;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &path);

         ret = cmsUtl_strcmp(path, eeFullPath)?FALSE:TRUE;
         CMSMEM_FREE_BUF_AND_NULL_PTR(path);
         
         cmsObj_free((void **)&eeObj);
         break;
      }
      cmsObj_free((void **)&eeObj);
   }

   return ret;
}

CmsRet stl_swModulesObject(_SwModulesObject *obj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dUObject(_DUObject *obj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
/*
 * In the "classical" architecture of CMS, when the STL handler function is
 * called, the function will do an ioctl or send a message to a daemon to
 * get the latest up-to-date info for this object.  However, in the case
 * of modular software, osgid and linmosd will always update the DUstatus
 * object with the latest info, so the data in this object is always
 * up-to-date.  So no need to do anything in this function.
 */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
}


#ifdef SUPPORT_OPENPLAT
static UBOOL8 isEefullpathHostEe(const char *eeFullPath)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;
   ExecEnvObject *eeObj = NULL;
   UBOOL8 isHostEe = FALSE;

   cmsLog_debug("Enter: eeFullPath=%s", eeFullPath);

   ret = cmsMdm_fullPathToPathDescriptor(eeFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("invalid fullPath<%s>", eeFullPath);
      return isHostEe;
   }

   ret = cmsObj_get(MDMOID_EXEC_ENV, &pathDesc.iidStack, 0, (void **)&eeObj);
   if (ret == CMSRET_SUCCESS)
   {
      if (strcmp(eeObj->name, OPS_HOSTEE_NAME) == 0)
      {
         isHostEe = TRUE;
      }
      cmsObj_free((void **)&eeObj);
   }
   else
   {
      cmsLog_error("cannot get EEObj: fullPath<%s> ret<%d>", eeFullPath, ret);
   }

   return isHostEe;

}  /* End of isEefullpathHostEe() */


static BcmRet getEeBusInfoByFullPathLocked(const char *eeFullPath,
                                           char *busInfo, int busInfoLen)
{
   BcmRet ret = BCMRET_SUCCESS;
   ExecEnvObject *eeObj = NULL;
   EeBusObject *busObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   ret = (BcmRet) qdmModsw_getExecEnvObjectByFullPathLocked(eeFullPath,
                                                   &eeObj,
                                                   &iidStack);
   if (ret == BCMRET_SUCCESS)
   {
      ret = (BcmRet) cmsObj_get(MDMOID_EE_BUS, &iidStack, OGF_NO_VALUE_UPDATE,
                       (void **)&busObj);
      if (ret == BCMRET_SUCCESS)
      {
         cmsUtl_strncpy(busInfo, busObj->busInfo, busInfoLen);
         cmsObj_free((void **)&busObj);
      }
      cmsObj_free((void **)&eeObj);
   }

   return ret;

}  /* End of getEeBusInfoByFullPathLocked() */


/* this routine uses lxc-info command to find of memory used by container.
 * return memoryInUse -1 or in Kbytes 
 */
static CmsRet getMemoryUseOfContainer(const char *containerName, SINT32 *memoryInUse)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_128]={0};
   char buf[BUFLEN_256]={0};
   char *ptr = NULL;
   UINT32 byteUsed = 0;
   FILE *fp = NULL;
   *memoryInUse = -1;

   if (IS_EMPTY_STRING(containerName) || (!strcmp(containerName,"")))
   {
      return ret;
   }

   sprintf(cmd, "lxc-info -n %s --stats -H", containerName);
   
   if ((fp = popen(cmd, "r")) == NULL)
   {
      cmsLog_error("Error opening pipe! cmd=%s", cmd);
      return CMSRET_INTERNAL_ERROR;
   }

   while (fgets(buf, BUFLEN_256, fp) != NULL)
   {
      /* the stats commnands will return something like this for the container:
       * CPU use: xx seconds
       * Memory use: xxx
       * Kmem use: xxx
       * And we are only interested in the memory used
       */
      ptr = strstr(buf, "Memory");
      if (ptr == NULL)
      {
         continue;
      }
      sscanf(buf, "%*[^:]%*c %u",&byteUsed);
      *memoryInUse = (SINT32)(byteUsed/1024);
      break;
   }
   pclose(fp);
   return (ret);
}


static CmsRet getAvailableDiskSpaceByEe(_ExecEnvObject *obj, SINT32 *availableDiskSpace)
{
   char eeDir[BUFLEN_256]={0};
   char buf[BUFLEN_256]={0};
   FILE *fp;

   /* Path to EE lxc rootfs directory.
    * e.g. /BEE-6.0/lxc/rootfs
    */
   snprintf(eeDir, sizeof(eeDir), "/%s-%s/lxc/rootfs", obj->name, obj->version);

   *availableDiskSpace = -1;  /* size in KiB */

   if ((fp = popen("df -k", "r")) == NULL)
   {
      cmsLog_error("Error opening pipe! cmd=df -k");
      return CMSRET_INTERNAL_ERROR;
   }

   /* Example of "df -k" output:
    *
    * Filesystem           1024-blocks    Used Available Use% Mounted on
    * ubi:rootfs2              23740     23740         0 100% /
    * ubi:data                  5848        52      5464   1% /data
    * ubi:defaults              5848        24      5492   0% /mnt/defaults
    * ubi0:local               73856     17640     52408  25% /local
    * /dev/loop0               12023     10496      1527  87% /local/modsw/tr157du/Broadcom/ExampleEE2-1.0/lxc/rootfs
    */

   while (fgets(buf, sizeof(buf), fp) != NULL)
   {
      cmsLog_debug("buf=%s", buf);

      /* look for eeDir */
      if (cmsUtl_strstr(buf, eeDir))
      {
         char dontcare[BUFLEN_256];

         sscanf(buf, "%s %s %s %d %s %s",
                dontcare, dontcare, dontcare, availableDiskSpace, dontcare, dontcare);
         break;
      }
   }

   pclose(fp);
   cmsLog_debug("availableDiskSpace=%d", *availableDiskSpace);
   
   return CMSRET_SUCCESS;

}  /* End of getAvailableDiskSpaceByEe() */


static CmsRet getMemoryInUseByEe(_ExecEnvObject *obj, SINT32 *memoryInUse)
{
   return(getMemoryUseOfContainer(obj->X_BROADCOM_COM_ContainerName, memoryInUse));
}

static void getHostEeDir(char *dir, int len)
{
   const char *dataDirName;

   /*
    * DU of host EE is located at:
    * /local/modsw/tr157du/Broadcom/OPS_HOSTEE/du
    */
   beepParam_getPersistentDirName(&dataDirName);
   snprintf(dir, len-1, "%s%s/%s", dataDirName, MODSW_DU_DIR, OPS_HOSTEE_DU_DIR);

}  /* End of getHostEeDir() */


static CmsRet getMemoryInUseByEu(_EUObject *obj, SINT32 *memoryInUse)
{
   return (getMemoryUseOfContainer(obj->EUID, memoryInUse));
}


/* Openplat has no access to Device.DeviceInfo.ProcessStatus.
 * EUObject->associatedProcessList is currently not supported.
 */
#if 0    //#ifdef DMP_DEVICE2_PROCESSSTATUS_1
static UINT32 getParentPid(UINT32 pid)
{
   UINT32   ppid = 0;
   char     *ptr;
   char     buf[81];
   FILE     *fp = NULL;
   
   /* open /proc/pid/status file */
   snprintf(buf, sizeof(buf), "/proc/%d/status", pid);
     
   if ((fp = fopen(buf, "r")) == NULL)
   {
      cmsLog_notice("fopen %s failed", buf);
      return ppid;
   }

   while (fgets(buf, sizeof(buf), fp))
   {
      /* strip eol character */
      ptr = strchr(buf, 0xa);
      if (ptr)
      {
         *ptr = '\0';
      }

      if ((ptr = strstr(buf, "PPid:")))
      {
         ptr += sizeof("PPid:");
         ppid = strtol(ptr, NULL, 10);
         break;
      }
   }

   fclose(fp);
   return ppid;

}  /* End of getParentPid() */


static CmsRet getAssociatedProcessList(_EUObject *obj)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;
   InstanceIdStack   iidStack, iidStackSave;
   Dev2ProcessStatusObject       *processStatusObj      = NULL;
   Dev2ProcessStatusEntryObject  *processStatusEntryObj = NULL;
   char   *buf, *fullStr, *ptr;
   UINT32 size;
   UINT32 pid = 0, ppid = 0;
   UBOOL8 found = FALSE;      

   /* get DeviceInfo.ProcessStatus object to update the
    * DeviceInfo.ProcessStatus.Process.{i}. table
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_DEV2_PROCESS_STATUS, &iidStack, 0,
                         (void **)&processStatusObj)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **)&processStatusObj);
   }
   else
   {
      cmsLog_error("cmsObj_get MDMOID_DEV2_PROCESS_STATUS failed, ret=%d", ret);
      return ret;
   }

   /* first find the application process, then its parents if any */
   iidStackSave = iidStack;
   while ((ret = cmsObj_getNext(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iidStack,
                        (void **)&processStatusEntryObj)) == CMSRET_SUCCESS)
   {
      if (!found)
      {
         if (processStatusEntryObj->command)
         {
            if ((ptr = strrchr(processStatusEntryObj->command, '/')))
               ptr++;
            else
               ptr = processStatusEntryObj->command;

            if (cmsUtl_strcmp(obj->name, ptr) == 0)
            {
               /* found the application process */
               found = TRUE;
               /* free the associatedProcessList before update */
               CMSMEM_FREE_BUF_AND_NULL_PTR(obj->associatedProcessList);
            }
         }

         if (!found)
         {
            cmsObj_free((void **)&processStatusEntryObj);
            continue;
         }
      }
      else
      {
         /* the application process had been found. Now we look
          * for its parent processes.
          */
         if (processStatusEntryObj->PID != ppid)
         {
            cmsObj_free((void **)&processStatusEntryObj);
            continue;
         }
      }

      pid = processStatusEntryObj->PID;
      cmsObj_free((void **)&processStatusEntryObj);

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid      = MDMOID_DEV2_PROCESS_STATUS_ENTRY;
      pathDesc.iidStack = iidStack;
      fullStr = NULL;
      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullStr);

      /* allocate buffer for the associatedProcessList */
      size = cmsUtl_strlen(obj->associatedProcessList) +
             cmsUtl_strlen(fullStr) +
             2;  /* plus comma and null terminator */

      if ((buf = cmsMem_alloc(size, ALLOC_ZEROIZE)) == NULL)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullStr);
         cmsLog_error("cmsMem_alloc failed. size=%d", size);
         ret = CMSRET_RESOURCE_EXCEEDED;
         break;
      }
      
      if (obj->associatedProcessList == NULL)
         sprintf(buf, "%s", fullStr);
      else
         sprintf(buf, "%s,%s", fullStr, obj->associatedProcessList);

      CMSMEM_REPLACE_STRING_FLAGS(obj->associatedProcessList, buf, ALLOC_SHARED_MEM);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullStr);
      CMSMEM_FREE_BUF_AND_NULL_PTR(buf);

      /* Look for the parent processes */
      if ((ppid = getParentPid(pid)) > 1)
         iidStack = iidStackSave;
      else
         break;   /* done */
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
      ret = CMSRET_SUCCESS;
   return ret;

}  /* End of getAssociatedProcessList() */
#endif

static CmsRet getDiskSpaceInUseByEu(_EUObject *obj, SINT32 *diskSpaceInUse)
{
   CmsRet ret = CMSRET_SUCCESS;
   char euFullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char duName[BUFLEN_32+1]={0};
   char duVer[BUFLEN_16+1]={0};
   char directory[BEEP_FULLPATH_LEN_MAX*2+16]={0};
   char hostEeDir[BEEP_FULLPATH_LEN_MAX]={0};
   char euDir[BEEP_FULLPATH_LEN_MAX]={0};
   char cmd[BEEP_FULLPATH_LEN_MAX*2+32]={0};
   char buf[BUFLEN_256]={0};
   char used[BUFLEN_8];
   struct stat st;
   FILE *fp=NULL;
   DUObject *duObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;

   cmsLog_debug("Enter: euid=%s euName=%s", obj->EUID, obj->name);
   *diskSpaceInUse = -1;

   getHostEeDir(hostEeDir, sizeof(hostEeDir));

   ret = qdmModsw_getExecUnitFullPathByEuidLocked(obj->EUID,
                                                  euFullPath,
                                                  sizeof(euFullPath)-1);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed get euFullPath by EUID=%s, ret=%d", obj->EUID, ret);
      return ret;
   }

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DU, &iidStack,
                                (void **) &duObj)) == CMSRET_SUCCESS)
   {
      if ((cmsUtl_strcmp(duObj->status, MDMVS_INSTALLED) == 0) &&
          (cmsUtl_isFullPathInCSL(euFullPath, duObj->executionUnitList) == TRUE))
      {
         found = TRUE;
         cmsUtl_strncpy(duName, duObj->name, sizeof(duName));
         cmsUtl_strncpy(duVer, duObj->version, sizeof(duVer));
      }
      cmsObj_free((void **)&duObj);
   }

   if (ret != CMSRET_SUCCESS)
   {
      if (ret == CMSRET_NO_MORE_INSTANCES)
         cmsLog_notice("Failed get duName/duVer/duInstance from euFullPath=%s",
                       euFullPath);
      else
         cmsLog_error("Failed get duName/duVer/duInstance from euFullPath=%s",
                      euFullPath);
      return ret;
   }

   snprintf(euDir, sizeof(euDir), "%s-%s/app_%s", duName, duVer, obj->name);
   cmsLog_debug("euDir=%s", euDir);

   /* check if this eu has a loop device */
   sprintf(directory, "%s/%s/lxc/loop", hostEeDir, euDir);
   cmsLog_debug("directory=%s", directory);

   if (stat(directory, &st) != 0)
   {
      SINT32 inUse1 = -1;
      SINT32 inUse2 = -1;

      /* loop device for this eu does not exist. flash usage is un-limited. */

      /* find the size of euDir/lxc directory.
       * e.g. /du/spTestSuite-2.3/app_spMaster/lxc
       */ 
      sprintf(cmd, "du -k -s %s/%s/lxc", hostEeDir, euDir);
      if ((fp = popen(cmd, "r")) == NULL)
      {
         cmsLog_error("Error opening pipe! cmd=%s", cmd);
         return CMSRET_INTERNAL_ERROR;
      }

      if (fgets(buf, sizeof(buf), fp) != NULL)
      {
         memset(used, 0, sizeof(used));
         memset(directory, 0, sizeof(directory));

         cmsLog_debug("buf=%s", buf);
         sscanf(buf, "%s %s", used, directory);

         inUse1 = strtol(used, NULL, 10);
      }

      pclose(fp);

      if (inUse1 <= 0)
      {
         return CMSRET_INTERNAL_ERROR;
      } 

      /* find the size of euDir/lxc/rootfs directory.
       * e.g. /du/spTestSuite-1/app_spMaster/lxc/rootfs 
       */
      sprintf(cmd, "du -k -s %s/%s/lxc/rootfs", hostEeDir, euDir);
      if ((fp = popen(cmd, "r")) == NULL)
      {
         cmsLog_error("Error opening pipe! cmd=%s", cmd);
         return CMSRET_INTERNAL_ERROR;
      }

      if (fgets(buf, sizeof(buf), fp) != NULL)
      {
         memset(used, 0, sizeof(used));
         memset(directory, 0, sizeof(directory));

         cmsLog_debug("buf=%s", buf);
         sscanf(buf, "%s %s", used, directory);

         if ((inUse2 = strtol(used, NULL, 10)))
         {
            if (inUse1 > inUse2)
            {
               *diskSpaceInUse = inUse1 - inUse2;   /* size in KiB */
            }
         }
      }

      pclose(fp);
   }
   else
   {
      char filesystem[BUFLEN_16];
      char blocks[BUFLEN_16];
      char available[BUFLEN_8];
      char usePercent[BUFLEN_8];
      char mountedOn[BUFLEN_128];

      /* this eu has a loop device. i.e. flash usage is limited. */
      sprintf(directory, "%s/%s/lxc/rootfs", hostEeDir, euDir);
      cmsLog_debug("directory=%s", directory);

      strcpy(cmd, "df -k");
      if ((fp = popen(cmd, "r")) == NULL)
      {
         cmsLog_error("Error opening pipe! cmd=%s", cmd);
         return CMSRET_INTERNAL_ERROR;
      }

      while (fgets(buf, sizeof(buf), fp) != NULL)
      {
         memset(filesystem, 0, sizeof(filesystem));
         memset(blocks, 0, sizeof(blocks));
         memset(used, 0, sizeof(used));
         memset(available, 0, sizeof(available));
         memset(usePercent, 0, sizeof(usePercent));
         memset(mountedOn, 0, sizeof(mountedOn));

         cmsLog_debug("buf=%s", buf);
         sscanf(buf, "%s %s %s %s %s %s",
                filesystem, blocks, used, available, usePercent, mountedOn);

         /* look for overlay file system mounted on directory */
         if (cmsUtl_strcmp(filesystem, "overlay") == 0)
         {
            cmsLog_debug("mountedOn=%s", mountedOn);
            if (cmsUtl_strcmp(mountedOn, directory) == 0)
            {
               SINT32 inUse = -1;

               if ((inUse = strtol(used, NULL, 10)))
               {
                  *diskSpaceInUse = inUse;   /* size in KiB */
               }

               break;
            }
         }
      }

      pclose(fp);
   }

   cmsLog_debug("diskSpaceInUse=%d", *diskSpaceInUse);

   return ret;

}  /* End of getDiskSpaceInUseByEu() */


CmsRet stl_execEnvObject(_ExecEnvObject *obj __attribute__((unused)),
                         const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 availableDiskSpace = -1;
   SINT32 availableMemory    = -1;
   SINT32 memoryInUse        = -1;
   SINT32 count = 0;

   cmsLog_debug("Enter: obj->name=%s obj->status=%s", obj->name, obj->status);

   if (IS_EMPTY_STRING(obj->name))
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   if (cmsUtl_strcmp(obj->status, MDMVS_UP) != 0)
   {
      /* if pmd is not up, we cannot get availableDiskSpace and availableMemory. */
      obj->availableDiskSpace = -1;
      obj->availableMemory    = -1;
      obj->X_BROADCOM_COM_NumberOfActiveContainers = count;
      return CMSRET_SUCCESS;
   }

   /* update host EE's active container counts */
   if (cmsUtl_strcmp(obj->name, OPS_HOSTEE_NAME) == 0)
   {
      ExecEnvObject *eeObj = NULL;
      EUObject *euObj = NULL;
      InstanceIdStack tmpIidStack = EMPTY_INSTANCE_ID_STACK;

      while (cmsObj_getNextFlags(MDMOID_EXEC_ENV, &tmpIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&eeObj) == CMSRET_SUCCESS)
      {
         if (!IS_EMPTY_STRING(eeObj->X_BROADCOM_COM_ContainerName))
         {
            if (cmsUtl_strcmp(eeObj->status, MDMVS_DISABLED))
            {
               count++;
            }
         }

         cmsObj_free((void **)&eeObj);
      }

      INIT_INSTANCE_ID_STACK(&tmpIidStack);

      while (cmsObj_getNextFlags(MDMOID_EU, &tmpIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&euObj) == CMSRET_SUCCESS)
      {
         if (isStandaloneApp(euObj->executionEnvRef) &&
             cmsUtl_strcmp(euObj->X_BROADCOM_COM_Username, HOST_USER_STRING))
         {
            if (cmsUtl_strcmp(euObj->status, MDMVS_IDLE))
            {
               count++;
            }
         }

         cmsObj_free((void **)&euObj);
      }

      obj->X_BROADCOM_COM_NumberOfActiveContainers = count;
   }

   if (obj->allocatedDiskSpace > 0)
   {
      getAvailableDiskSpaceByEe(obj, &availableDiskSpace);
   }

   if (obj->allocatedMemory > 0)
   {
      getMemoryInUseByEe(obj, &memoryInUse);
      if (memoryInUse > 0)
      {
         availableMemory = obj->allocatedMemory - memoryInUse;
      }
   }

   if (obj->availableDiskSpace == availableDiskSpace &&
       obj->availableMemory    == availableMemory)
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      obj->availableDiskSpace = availableDiskSpace;
      obj->availableMemory    = availableMemory;      
   }

   return ret;

}  /* End of stl_execEnvObject() */


CmsRet stl_eUObject(_EUObject *obj,
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char statusBuf[BUFLEN_32]={0};
   UINT32 timeoutMs = 5000;
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(EUstatusRequestMsgBody)]={0};
   CmsMsgHeader *msg = (CmsMsgHeader *)msgBuf;
   EUstatusRequestMsgBody *msgBody = (EUstatusRequestMsgBody *)(msg+1);
   char replyBuf[sizeof(CmsMsgHeader)+sizeof(EUstatusReplyMsgBody)]={0};
   CmsMsgHeader *reply = (CmsMsgHeader *)replyBuf;
   EUstatusReplyMsgBody *replyBody = (EUstatusReplyMsgBody *)(reply+1);

   cmsLog_debug("Enter: obj->EUID=%s obj->name=%s obj->status=%s",
                obj->EUID, obj->name, obj->status);
   cmsLog_debug("execEnvRef=%s execEnvLabel=%s",
                obj->executionEnvRef, obj->execEnvLabel);

   if (IS_EMPTY_STRING(obj->EUID) ||
       IS_EMPTY_STRING(obj->name) ||
       IS_EMPTY_STRING(obj->executionEnvRef) ||
       IS_EMPTY_STRING(obj->execEnvLabel))
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   if (isEefullpathHostEe(obj->executionEnvRef))
   {
      SINT32 diskSpaceInUse = -1;
      SINT32 memoryInUse    = -1;

      /* standalone eu */
      getDiskSpaceInUseByEu(obj, &diskSpaceInUse);

      if (cmsUtl_strcmp(obj->status, MDMVS_ACTIVE) == 0)
      {
         getMemoryInUseByEu(obj, &memoryInUse);
      }

      if (obj->diskSpaceInUse == diskSpaceInUse &&
          obj->memoryInUse    == memoryInUse)
      {
         ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
      }
      else
      {
         obj->diskSpaceInUse = diskSpaceInUse;
         obj->memoryInUse    = memoryInUse;      
      }
   
/* Openplat has no access to Device.DeviceInfo.ProcessStatus.
 * EUObject->associatedProcessList is currently not supported.
 */
#if 0    //#ifdef DMP_DEVICE2_PROCESSSTATUS_1
      if (cmsUtl_strcmp(obj->status, MDMVS_ACTIVE) != 0)
      {
         /* free the associatedProcessList */
         if (obj->associatedProcessList)
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(obj->associatedProcessList);
            ret = CMSRET_SUCCESS;
         }
      }
      else
      {
         CmsRet ret1;

         ret1 = getAssociatedProcessList(obj);
         if (ret1 != CMSRET_SUCCESS_OBJECT_UNCHANGED)
         {
            ret = ret1;
         }
      }   
#endif

      return ret;
   }

   /* EU belongs to remote EE */
   qdmModsw_getExecEnvStatusByFullPathLocked(obj->executionEnvRef,
                                             statusBuf, sizeof(statusBuf)-1);
   if (cmsUtl_strcmp(statusBuf, MDMVS_UP))
   {
      return CMSRET_SUCCESS;
   }

   ret = (CmsRet) getEeBusInfoByFullPathLocked(obj->executionEnvRef,
                                      msgBody->eeBusInfo, sizeof(msgBody->eeBusInfo)-1);
   if (IS_EMPTY_STRING(msgBody->eeBusInfo))
   {
      cmsLog_error("Cannot find busInfo of eeFullPath %s", obj->executionEnvRef);
      return CMSRET_SUCCESS;
   }

   msg->type = (CmsMsgType) CMS_MSG_REQUEST_EU_STATUS;
   msg->src  = mdmLibCtx.eid;
   msg->dst  = EID_OPENPLAT_HELPER;
   msg->flags_request = 1;
   msg->dataLength = sizeof(EUstatusRequestMsgBody);

   strncpy(msgBody->execEnvLabel, obj->execEnvLabel, sizeof(msgBody->execEnvLabel)-1);

   ret = cmsMsg_sendAndGetReplyBufWithTimeout(mdmLibCtx.msgHandle, msg,
                                              &reply, timeoutMs);
   if (ret == CMSRET_SUCCESS)
   {
      if (replyBody->retcode == CMSRET_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, replyBody->status,
                                           mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->executionFaultCode, replyBody->execFaultCode,
                                           mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->executionFaultMessage, replyBody->execFaultMsg,
                                           mdmLibCtx.allocFlags);
         obj->diskSpaceInUse = replyBody->diskSpaceInUse;
         obj->memoryInUse = replyBody->memoryInUse;
         cmsLog_debug("status=%s faultCode=%s faultMsg=%s diskSpaceInUse=%d memoryInUse=%d",
                      obj->status, obj->executionFaultCode, obj->executionFaultMessage,
                      obj->diskSpaceInUse, obj->memoryInUse);
      }                                                          
   }
   else
   {
      cmsLog_error("Failed to send CMS_MSG_REQUEST_EU_STATUS, ret=%d", ret);
   }

   return ret;

}  /* End of stl_eUObject() */
#else
CmsRet stl_execEnvObject(_ExecEnvObject *obj __attribute__((unused)),
                         const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* see comments in stl_dUObject */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_eUObject(_EUObject *obj __attribute__((unused)),
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* see comments in stl_dUObject */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_extensionsObject(_ExtensionsObject *obj __attribute__((unused)),
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busObject(_BusObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busObjectPathObject(_BusObjectPathObject *obj __attribute__((unused)),
                               const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busInterfaceObject(_BusInterfaceObject *obj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busMethodObject(_BusMethodObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busSignalObject(_BusSignalObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busPropertyObject(_BusPropertyObject *obj __attribute__((unused)),
                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busClientObject(_BusClientObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busClientPrivilegeObject(_BusClientPrivilegeObject *obj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_manifestObject(_ManifestObject *obj __attribute__((unused)),
                          const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dmAccessObject(_DmAccessObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeManifestObject(_EeManifestObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeBusObject(_EeBusObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeProcessObject(_EeProcessObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeProcessArgsObject(_EeProcessArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeProcessEnvObject(_EeProcessEnvObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeMountsObject(_EeMountsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeMountsOptionObject(_EeMountsOptionObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeLinuxObject(_EeLinuxObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeResourceObject(_EeResourceObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeBlkioObject(_EeBlkioObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeCpuObject(_EeCpuObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeResDevicesObject(_EeResDevicesObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeNetworkSetupObject(_EeNetworkSetupObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeDevicesObject(_EeDevicesObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeHooksObject(_EeHooksObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePresetupObject(_EePresetupObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePresetupArgsObject(_EePresetupArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePrestartObject(_EePrestartObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePrestartArgsObject(_EePrestartArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePoststartObject(_EePoststartObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePoststartArgsObject(_EePoststartArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePoststopObject(_EePoststopObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eePoststopArgsObject(_EePoststopArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_eeNetworkObject(_EeNetworkObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euBusObject(_EuBusObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euProcessObject(_EuProcessObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euProcessArgsObject(_EuProcessArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euProcessEnvObject(_EuProcessEnvObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euMountsObject(_EuMountsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euMountsOptionObject(_EuMountsOptionObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euLinuxObject(_EuLinuxObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euResourceObject(_EuResourceObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euBlkioObject(_EuBlkioObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euCpuObject(_EuCpuObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euResDevicesObject(_EuResDevicesObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euDevicesObject(_EuDevicesObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euHooksObject(_EuHooksObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPresetupObject(_EuPresetupObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPresetupArgsObject(_EuPresetupArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPrestartObject(_EuPrestartObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPrestartArgsObject(_EuPrestartArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPoststartObject(_EuPoststartObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPoststartArgsObject(_EuPoststartArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPoststopObject(_EuPoststopObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euPoststopArgsObject(_EuPoststopArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euDependencyObject(_EuDependencyObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euSeccompObject(_EuSeccompObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euSyscallsObject(_EuSyscallsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_euSyscallArgsObject(_EuSyscallArgsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
static CmsRet getEuDiskSpaceInUse(const CmsMsgHeader *msg,
                                         SINT32 *diskSpaceInUse)
{
   CmsRet ret = CMSRET_SUCCESS;
   EUdiskSpaceInUseMsgBody *msgBody = (EUdiskSpaceInUseMsgBody *)(msg+1);
   char directory[BEEP_FULLPATH_LEN_MAX*2]={0};
   char hostEeDir[BEEP_FULLPATH_LEN_MAX]={0};
   char cmd[BEEP_FULLPATH_LEN_MAX*2]={0};
   char buf[BUFLEN_256]={0};
   char filesystem[BUFLEN_16];
   char blocks[BUFLEN_16];
   char used[BUFLEN_8];
   char available[BUFLEN_8];
   char usePercent[BUFLEN_8];
   char mountedOn[BUFLEN_128];
   struct stat st;
   FILE *fp;

   cmsLog_debug("euDir=%s", msgBody->euDir);

   *diskSpaceInUse = -1;

   getHostEeDir(hostEeDir, sizeof(hostEeDir));
   /* check if this eu has a loop device */
   sprintf(directory, "%s/%s/lxc/loop", hostEeDir, msgBody->euDir);
   if (stat(directory, &st) != 0)
   {
      SINT32 inUse1 = -1;
      SINT32 inUse2 = -1;

      /* loop device for this eu does not exist. flash usage is un-limited. */

      /* find the size of euDir/lxc directory.
       * e.g. /du/spTestSuite-1/app_spMaster/lxc
       */ 
      sprintf(cmd, "du -k -s %s/%s/lxc", hostEeDir, msgBody->euDir);
      if ((fp = popen(cmd, "r")) == NULL)
      {
         cmsLog_error("Error opening pipe! cmd=%s", cmd);
         return CMSRET_INTERNAL_ERROR;
      }

      if (fgets(buf, sizeof(buf), fp) != NULL)
      {
         memset(used, 0, sizeof(used));
         memset(directory, 0, sizeof(directory));

         cmsLog_debug("buf=%s", buf);
         sscanf(buf, "%s %s", used, directory);

         inUse1 = strtol(used, NULL, 10);
      }

      pclose(fp);

      if (inUse1 <= 0)
      {
         return CMSRET_INTERNAL_ERROR;
      } 

      /* find the size of euDir/lxc/rootfs directory.
       * e.g. /du/spTestSuite-1/app_spMaster/lxc/rootfs 
       */
      sprintf(cmd, "du -k -s %s/%s/lxc/rootfs", hostEeDir, msgBody->euDir);
      if ((fp = popen(cmd, "r")) == NULL)
      {
         cmsLog_error("Error opening pipe! cmd=%s", cmd);
         return CMSRET_INTERNAL_ERROR;
      }

      if (fgets(buf, sizeof(buf), fp) != NULL)
      {
         memset(used, 0, sizeof(used));
         memset(directory, 0, sizeof(directory));

         cmsLog_debug("buf=%s", buf);
         sscanf(buf, "%s %s", used, directory);

         if ((inUse2 = strtol(used, NULL, 10)))
         {
            if (inUse1 > inUse2)
            {
               *diskSpaceInUse = inUse1 - inUse2;   /* size in KiB */
            }
         }
      }

      pclose(fp);
   }
   else
   {
      /* this eu has a loop device. i.e. flash usage is limited. */
      sprintf(directory, "%s/%s/lxc/rootfs", hostEeDir, msgBody->euDir);
      cmsLog_debug("directory=%s", directory);

      strcpy(cmd, "df -k");
      if ((fp = popen(cmd, "r")) == NULL)
      {
         cmsLog_error("Error opening pipe! cmd=%s", cmd);
         return CMSRET_INTERNAL_ERROR;
      }

      while (fgets(buf, sizeof(buf), fp) != NULL)
      {
         memset(filesystem, 0, sizeof(filesystem));
         memset(blocks, 0, sizeof(blocks));
         memset(used, 0, sizeof(used));
         memset(available, 0, sizeof(available));
         memset(usePercent, 0, sizeof(usePercent));
         memset(mountedOn, 0, sizeof(mountedOn));

         cmsLog_debug("buf=%s", buf);
         sscanf(buf, "%s %s %s %s %s %s",
                filesystem, blocks, used, available, usePercent, mountedOn);

         /* look for overlay file system mounted on directory */
         if (cmsUtl_strcmp(filesystem, "overlay") == 0)
         {
            cmsLog_debug("mountedOn=%s", mountedOn);
            if (cmsUtl_strcmp(mountedOn, directory) == 0)
            {
               SINT32 inUse = -1;

               if ((inUse = strtol(used, NULL, 10)))
               {
                  *diskSpaceInUse = inUse;   /* size in KiB */
               }

               break;
            }
         }
      }

      pclose(fp);
   }

   cmsLog_debug("diskSpaceInUse=%d", *diskSpaceInUse);

   return ret;
}
#endif


#endif /* DMP_DEVICE2_SM_BASELINE_1 */
