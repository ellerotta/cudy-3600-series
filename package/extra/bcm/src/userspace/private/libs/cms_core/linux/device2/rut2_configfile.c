/***********************************************************************
 *
 *
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom
   All Rights Reserved

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


#include "cms_core.h"
#include "cms_util.h"
#include "mdm.h"

#ifdef DMP_DEVICE2_BASELINE_1

#include "rut2_configfile.h"


/***************************************************************************
 *
 * This is a brief description of how Device.DeviceInfo.VendorConfigFile.{i}.
 * is implemented.  5.04L.03 introduces a significant change in this area.
 *
 *
 * PRIOR TO 504L03 release:
 * tr69c was the only app that creates/deletes instances and writes to the
 * VendorConfigFile.{i}. table.  When a config file is downloaded via the
 * tr69c RPC, a new instance will be created.  Thus, the largest instance
 * number is the last one downloaded and in use.  If the maximum number of
 * VendorConfigFile.{i}. instances are created, then the oldest one is
 * deleted.  When a new config file is downloaded, some information about the
 * config file is written to the scratchpad.  When the system reboots (as
 * the result of downloading and switching to the new config file, tr69c will
 * read the info from the scratchpad and write it to the last instance of
 * the VendorConfigFile.{i}. table.  tr69c cannot write the data to the
 * VendorConfigFile.{i}. table (the MDM) before the reboot because that table
 * and the entire MDM will be overwritten with the new config file on the
 * next reboot.  After the info has been transfered from the scratchpad to
 * the VendorConfigFile.{i}. table, the scratchpad entry is deleted.
 *
 * Because tr69c is the only app that touches the VendorConfigFile.{i}. table,
 * if no config file is downloaded via tr69c RPC, no instances will be created.
 *
 * 504L03 release:
 * Management of the VendorConfigFile.{i}. table is now centralized in this
 * file.  When the system boots, mdm_adjustForHardware_dev2() will call
 * mdm_initVendorConfigFileObjects_dev2() to create the initial
 * VendorConfigFile instance at instance 1.  This entry represents the "system
 * config file".  Once the system is running, if tr69c, obuspa, httpd, or
 * any other app downloads a new config file, the new config file is still
 * represented by instance 1.  (Unlike pre-504L03, no additional
 * instances will be created.)  When applications download a new config file,
 * they should call rutConfigFile_saveInfoToScratchPad() to save some
 * info about the config file into the scratchpad.  This info will be written
 * to instance 1 on reboot and the scratchpad entry will be deleted.  (Read
 * the previous section for why it is necessary to hold the info in scratchpad.)
 *
 * Instances 1-999 are reserved for Broadcom use.  If an OEM or carrier
 * wants to create additional instances in the  VendorConfigFile.{i}. table,
 * they should start at instance 1000.  (Also, this code currently assumes
 * there is only 1 instance, so some changes to this code will be needed to
 * accomodate multiple instances).
 *
 * On bootup, if multiple instances or instance 1 is not detected by
 * mdm_initVendorConfigFileObjects_dev2(), then all instances will be deleted
 * and instance 1 will be created.  This will "convert" pre-504L03 
 * VendorConfigFile.{i}. table layout to the new layout.
 *
 ***************************************************************************
 */


void rutConfigFile_saveInfoToScratchPad(const char *name, const char *version, const char *desc)
{
   CmsRet ret;
   DownloadVendorConfigInfo vendorConfig;

   cmsLog_notice("Entered: name=%s version=%s desc=%s", name, version, desc);

   memset(&vendorConfig,0, sizeof(vendorConfig));

   if (IS_EMPTY_STRING(name))
   {
      snprintf(vendorConfig.name, sizeof(vendorConfig.name), "%s", CONFIGFILE_SYSTEM_1);
   }
   else
   {
      snprintf(vendorConfig.name, sizeof(vendorConfig.name), "%s", name);
   }

   // If caller provided a version string, use it, otherwise, leave it blank.
   if (!IS_EMPTY_STRING(version))
   {
      snprintf(vendorConfig.version, sizeof(vendorConfig.version), "%s", version);
   }

   // Set the first use date as now, even though technically, it is not used
   // until after we reboot, which should be soon after this.
   cmsTms_getXSIDateTime(0, vendorConfig.date, sizeof(vendorConfig.date));

   // If caller provided a description, use it, otherwise, leave it blank.
   if (!IS_EMPTY_STRING(desc))
   {
      snprintf(vendorConfig.description, sizeof(vendorConfig.description), "%s", desc);
   }

   printf("saving vendorConfig info into scratchpad %s: %s/%s/%s/%s\n",
          CMSPSP_VENDOR_CFG_TOKEN,
          vendorConfig.name,
          vendorConfig.version,
          vendorConfig.date,
          vendorConfig.description);

   ret = cmsPsp_set(CMSPSP_VENDOR_CFG_TOKEN,
                    &vendorConfig, (UINT32)sizeof(vendorConfig));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("error saving vendorConfig to scratchpad (key=%s ret=%d)",
                   CMSPSP_VENDOR_CFG_TOKEN, ret);
   }

   return;
}


/************************************************************************
 *
 * This section contains internal helper funcs for mdm_initVendorConfigFileObjects_dev2()
 *
 ************************************************************************/

static UINT32 countConfigFileInstances(UBOOL8 *isInst1Found)
{
   Dev2DeviceVendorConfigFileObject *vendorObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 count = 0;

   while (CMSRET_SUCCESS == mdm_getNextObject(MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE,
                                              &iidStack,
                                              (void **) &vendorObj))
   {
      count++;
      if (PEEK_INSTANCE_ID(&iidStack) == 1)
         *isInst1Found = TRUE;

      mdm_freeObject((void **)&vendorObj);
   }

   return count;
}

// The VendorConfigFile implementation prior to 504L03 would create a new
// instance of the VendorConfigFile every time tr69c downloaded a new
// config file.  So the last instance is the most recent.  Delete all instances
// and save the info from the last one (put it in vendorConfig).
static void deleteAllInstancesAndSaveLast(DownloadVendorConfigInfo *vendorConfig)
{
   Dev2DeviceVendorConfigFileObject *vendorObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   while (ret == CMSRET_SUCCESS)
   {
      INIT_INSTANCE_ID_STACK(&iidStack);
      ret = mdm_getNextObject(MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE,
                              &iidStack, (void **) &vendorObj);
      if (ret == CMSRET_SUCCESS)
      {
         // Save the info from this object into vendorConfig
         if (!IS_EMPTY_STRING(vendorObj->name))
            snprintf(vendorConfig->name, sizeof(vendorConfig->name), "%s", vendorObj->name);
         if (!IS_EMPTY_STRING(vendorObj->version))
            snprintf(vendorConfig->version, sizeof(vendorConfig->version), "%s", vendorObj->version);
         if (!IS_EMPTY_STRING(vendorObj->date))
            snprintf(vendorConfig->date, sizeof(vendorConfig->date), "%s", vendorObj->date);
         if (!IS_EMPTY_STRING(vendorObj->description))
            snprintf(vendorConfig->description, sizeof(vendorConfig->description), "%s", vendorObj->description);

         mdm_freeObject((void **)&vendorObj);
      }
   }
   return;
}

static void setInfoToInstance1(const DownloadVendorConfigInfo *vendorConfig)
{
   Dev2DeviceVendorConfigFileObject *vendorObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   // Get instance 1 obj.
   PUSH_INSTANCE_ID(&iidStack, 1);
   if ((ret = mdm_getObject(MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE,
                            &iidStack,
                            (void **) &vendorObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MDMOID_DEV2_VENDOR_CONFIG_FILE instance 1, ret=%d", ret);
      return;
   }

   // copy vendorConfig to obj
   if (!IS_EMPTY_STRING(vendorConfig->name))
      CMSMEM_REPLACE_STRING_FLAGS(vendorObj->name, vendorConfig->name, mdmLibCtx.allocFlags);
   if (!IS_EMPTY_STRING(vendorConfig->version))
      CMSMEM_REPLACE_STRING_FLAGS(vendorObj->version, vendorConfig->version, mdmLibCtx.allocFlags);
   if (!IS_EMPTY_STRING(vendorConfig->date))
      CMSMEM_REPLACE_STRING_FLAGS(vendorObj->date, vendorConfig->date, mdmLibCtx.allocFlags);
   if (!IS_EMPTY_STRING(vendorConfig->description))
      CMSMEM_REPLACE_STRING_FLAGS(vendorObj->description, vendorConfig->description, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &vendorObj, &iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
      cmsLog_error("mdm_setObj for MDMOID_DEV2_VENDOR_CONFIG_FILE failed, ret=%d", ret);

   mdm_freeObject((void **)&vendorObj);
   return;
}

// Probably a brand new MDM.  Create instance 1 with some reasonable defaults.
static void createInstance1()
{
   CmsRet ret;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   DownloadVendorConfigInfo vendorConfig;

   memset(&vendorConfig,0, sizeof(vendorConfig));

   // Create instance 1
   pathDesc.oid = MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for MDMOID_DEV2_VENDOR_CONFIG_FILE failed, ret=%d", ret);
      return;
   }
   else
   {
      cmsLog_debug("created instance 1");
   }

   // set reasonable defaults
   snprintf(vendorConfig.name, sizeof(vendorConfig.name), "%s", CONFIGFILE_SYSTEM_1);
   snprintf(vendorConfig.description, sizeof(vendorConfig.description), "default creation");
   cmsTms_getXSIDateTime(0, vendorConfig.date, sizeof(vendorConfig.date));
   setInfoToInstance1(&vendorConfig);

   return;
}

static void loadInfofromScratchPad()
{
   SINT32 rc;
   DownloadVendorConfigInfo vendorConfig;

   memset(&vendorConfig,0, sizeof(vendorConfig));

   // read vendorConfig info from scratchpad
   rc = cmsPsp_get(CMSPSP_VENDOR_CFG_TOKEN, &vendorConfig, (UINT32)sizeof(vendorConfig));

   // If we find a token, that means we just rebooted after a config file
   // upload.  But in most cases, when we reboot, we will not see the token, so
   // don't need to do anything.
   if (rc > 0)
   {
      printf("Detected %s in scratchpad (%s/%s/%s/%s), load into VendorConfigFile.1.\n", 
             CMSPSP_VENDOR_CFG_TOKEN,
             vendorConfig.name,
             vendorConfig.version,
             vendorConfig.date,
             vendorConfig.description);

      setInfoToInstance1(&vendorConfig);

      // Once we have transfered the info from scratchpad to MDM, we can
      // delete the key and value from scratchpad.
      cmsPsp_set(CMSPSP_VENDOR_CFG_TOKEN, NULL, 0);
   }
   return;
}

static void setVendorConfigFileNumberOfEntries(UINT32 num)
{
   Dev2DeviceInfoObject *deviceInfoObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = mdm_getObject(MDMOID_DEV2_DEVICE_INFO, &iidStack, (void **)&deviceInfoObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get DeviceInfo obj, ret=%d", ret);
      return;
   }

   deviceInfoObj->vendorConfigFileNumberOfEntries = num;

   ret = mdm_setObject((void **)&deviceInfoObj, &iidStack, FALSE);
   mdm_freeObject((void **)&deviceInfoObj);

   return;
}

CmsRet mdm_initVendorConfigFileObjects_dev2(void)
{
   UINT32 numObjs = 0;
   UBOOL8 isInst1Found = FALSE;

   cmsLog_notice("Entered:");

   // count number of vendorConfigFile objects, and also see if instance 1
   // is present.
   numObjs = countConfigFileInstances(&isInst1Found);

   if (numObjs == 0)
   {
      createInstance1();
      loadInfofromScratchPad();
   }
   else if ((numObjs == 1) && isInst1Found)
   {
      // There is exactly 1 obj at instance 1.  This is the most common case.
      // Just need to check for info in scratchpad.
      loadInfofromScratchPad();
   }
   else
   {
      DownloadVendorConfigInfo vendorConfig;
      memset(&vendorConfig,0, sizeof(vendorConfig));

      // 1 obj but not at instance 1, or
      // more than 1 obj
      printf("\n***WARNING: old VendorConfigFile implementation detected, please read rut2_configfile.c***\n\n");
      deleteAllInstancesAndSaveLast(&vendorConfig);
      createInstance1();
      setInfoToInstance1(&vendorConfig);
   }

   // In the end, the number of instances is always 1 (but this will be wrong
   // if a third party adds additional vendor config files).
   setVendorConfigFileNumberOfEntries(1);

   return CMSRET_SUCCESS;
}


#else

/*
 * This section contains stub functions for TR98 builds.  That way, app code
 * can just call these functions without knowing whether it is running in
 * TR98 or TR181.  However, these functions and this feature is not tested
 * in a TR98 environment.
 */
void rutConfigFile_saveInfoToScratchPad(const char *name __attribute__((unused)),
                               const char *version __attribute__((unused)),
                               const char *desc __attribute__((unused)))
{
   cmsLog_error("Not supported (to be impl) in TR98 mode");
   return;
}

#endif    /* DMP_DEVICE2_BASELINE_1 */

