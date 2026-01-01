/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom
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

#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "bcm_ulog.h"
#include "sysutil.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "bcm_boardutils.h"
#include "bcm_flashutil.h"
#include "bcm_hwdefs.h"

#include "rut_system.h"
#include "rut2_configfile.h"
#include "rut_util.h"

CmsRet mdm_adjustForHardware_dev2_devinfo(void);
CmsRet mdm_addDefaultProcessorObjects_dev2(void);
CmsRet mdm_addDefaultSupportedDataModelObjects_dev2(void);
CmsRet mdm_initFirmwareImageObjects_dev2(void);


/** This is the "weak" mdm_adjustForHardware_dev2.
 *  It will only be called in a distributed MDM build in the devinfo component.
 */
#pragma weak mdm_adjustForHardware_dev2
CmsRet mdm_adjustForHardware_dev2()
{
   return (mdm_adjustForHardware_dev2_devinfo());
}

// This function is shared by this file (BDK) and also the CMS181 case (mdm2_init_core.c).
CmsRet mdm_adjustForHardware_dev2_devinfo()
{
   CmsRet ret=CMSRET_SUCCESS;
   
   bcmuLog_notice("Distributed MDM: adjustForHardware for Device Info only");

   /* TR69/TR181 these 2 parameters have forced active notification */
   ret = mdm_setForcedActiveNotification("Device.DeviceInfo.SoftwareVersion");
   RETURN_IF_NOT_SUCCESS(ret);
   ret = mdm_setForcedActiveNotification("Device.DeviceInfo.ProvisioningCode");
   RETURN_IF_NOT_SUCCESS(ret);   

   // Update SoftwareVersion only if it does not match the current image version.
   // Even though it is a read-only param, we always write SoftwareVersion to
   // config file.  This way, during bootup, if we detect that the software
   // version changed, we will generate an active notification.  But if the 
   // software version did not change, then no change and not active notification.
   {
      Dev2DeviceInfoObject *devInfoObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      mdm_getObject(MDMOID_DEV2_DEVICE_INFO, &iidStack, (void **) &devInfoObj);
      if (cmsUtl_strcmp(devInfoObj->softwareVersion, CMS_RELEASE_VERSION))
      {
         CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->softwareVersion,
                                     CMS_RELEASE_VERSION,
                                     mdmLibCtx.allocFlags);

         cmsLog_notice("Update SoftwareVersion to %s", devInfoObj->softwareVersion);
         ret = mdm_setObject((void **)&devInfoObj, &iidStack, TRUE);
         if (ret != CMSRET_SUCCESS)
         {
            bcmuLog_error("Failed to set SoftwareVersion, ret=%d", ret);
         }
      }

      mdm_freeObject((void **)&devInfoObj);
      RETURN_IF_NOT_SUCCESS(ret);
   }

   ret = mdm_initFirmwareImageObjects_dev2();
   RETURN_IF_NOT_SUCCESS(ret);

   ret = mdm_initVendorConfigFileObjects_dev2();
   RETURN_IF_NOT_SUCCESS(ret);

#ifdef DMP_DEVICE2_SUPPORTEDDATAMODEL_1
   ret = mdm_addDefaultSupportedDataModelObjects_dev2();
   RETURN_IF_NOT_SUCCESS(ret);
#endif

#ifdef DMP_DEVICE2_PROCESSORS_1
   ret = mdm_addDefaultProcessorObjects_dev2();
   RETURN_IF_NOT_SUCCESS(ret);
#endif

   return ret;
}

static void setBootFirmwareToActiveFirmware(const MdmPathDescriptor *activePathDesc,
                                            Dev2DeviceInfoObject *devInfoObj)
{
   char nameBuf[IMAGE_VERSION_TAG_SIZE+1] = {0};
   char versionBuf[IMAGE_VERSION_TAG_SIZE+1] = {0};
   char *fullpath = NULL;
   UINT32 part = 0;

   if ((cmsMdm_pathDescriptorToFullPathNoEndDot(activePathDesc, &fullpath)) == CMSRET_SUCCESS)
   {
       CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->bootFirmwareImage, fullpath,
                                   mdmLibCtx.allocFlags);
       CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
   }

   part = PEEK_INSTANCE_ID(&(activePathDesc->iidStack));
   cmsImg_getFirmwareNameVersion((SINT32)part, nameBuf, versionBuf);
   CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->X_BROADCOM_COM_PrevBootName, nameBuf,
                               mdmLibCtx.allocFlags);

   return;
}

static void setFirmwareObjectAliasAndPartition(Dev2FirmwareImageObject *fwObj,
                                               UINT32 part)
{
   char aliasBuf[64] = {0};

   snprintf(aliasBuf, sizeof(aliasBuf), "cpe-firmware-%d", part);
   CMSMEM_REPLACE_STRING_FLAGS(fwObj->alias, aliasBuf, mdmLibCtx.allocFlags);
   fwObj->X_BROADCOM_COM_FLASH_PARTITION = part;
   return;
}

static void setFirmwareMgmtOwner(Dev2DeviceInfoObject *devInfoObj,
                                 UBOOL8 *isOwnedByTr69)
{
#if defined(DMP_X_BROADCOM_COM_GPON_1) || defined(DMP_X_BROADCOM_COM_EPON_1)
   CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->X_BROADCOM_COM_FirmwareMgmtOwner,
                               MDMVS_OTHER, mdmLibCtx.allocFlags);
   *isOwnedByTr69 = FALSE;
#else
   // If the firmware images are owned by TR69, then this code can commit
   // the firmware images.  Otherwise, firmware images are owned by epon or
   // gpon, so only update the data model, but do not commit the images.
   // For SQA: httpd in EPON/GPON mode is also allowed to commit firmware
   // image.  Detect this by looking at bootFirmwareImage (eponapp and omcid
   // do not set it, but httpd will)
   CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->X_BROADCOM_COM_FirmwareMgmtOwner,
                               MDMVS_TR69, mdmLibCtx.allocFlags);
   *isOwnedByTr69 = TRUE;
#endif /* DMP_X_BROADCOM_COM_GPON_1 || DMP_X_BROADCOM_COM_EPON_1 */
   printf("FirmwareSelect: firmware mgmt owner is %s\n",
          devInfoObj->X_BROADCOM_COM_FirmwareMgmtOwner);
   return;
}


// Helper function to:
// -- add default set of firmwareObjects if they don't already exist.
// -- detect old config file or old version of firmware objs and overwrite them.
// -- write some basic info to the firmware objects.
// -- gets the deviceInfo, activeFwObj, and alternFwObj for the calling
//    function, which is responsible for freeing them.
static CmsRet addDefaultFirmwareImageObjects_dev2(
       InstanceIdStack *devInfoIidStack, Dev2DeviceInfoObject **devInfoObj,
       MdmPathDescriptor *activePathDesc, Dev2FirmwareImageObject **activeFwObj,
       MdmPathDescriptor *alternPathDesc, Dev2FirmwareImageObject **alternFwObj,
       UBOOL8 *isOwnedByTr69)
{
   char nameBuf[IMAGE_VERSION_TAG_SIZE+1] = {0};
   char versionBuf[IMAGE_VERSION_TAG_SIZE+1] = {0};
   UINT32 activePart = 0;
   UINT32 alternPart = 0;
   CmsRet ret;

   activePart = (UINT32) getBootPartition();
   if (activePart < 1 || activePart > 2)
   {
      bcmuLog_error("getBootPartition returned %d", activePart);
      return CMSRET_INTERNAL_ERROR;
   }
   alternPart = (activePart == 1) ? 2 : 1;

   // Get the DeviceInfo object.
   if ((ret = mdm_getObject(MDMOID_DEV2_DEVICE_INFO,
                            devInfoIidStack,
                            (void **) devInfoObj)) != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to get Dev2DeviceInfoObject, ret=%d", ret);
      return ret;
   }

   // Always update the firmware management owner in case the config file
   // says one thing, but the currently running image is something else.
   setFirmwareMgmtOwner(*devInfoObj, isOwnedByTr69);

   // Check if we need to create the firmware objects.
   {
      MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
      Dev2FirmwareImageObject *fwObj = NULL;

      pathDesc.oid = MDMOID_DEV2_FIRMWARE_IMAGE;
      PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
      ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &fwObj);
      if (ret != CMSRET_SUCCESS)
      {
         // If we go from BDK to CMS Pure181, we will lose the config file,
         // so we need to COMMIT this current image.  Technically, we only
         // need to do this extra commit if the CMS Pure181 is a GPON image,
         // but simplify the logic by doing it in more cases -- extra commit
         // does not hurt.
         if (bcmUtl_isUbootNoCommitImage())
         {
            printf("FirmwareSelect: uboot no_commit_image set, user must manually commit image\n");
         }
         else
         {
            printf("FirmwareSelect: COMMIT partition %d "
                   "(due to new MDM/no config file)\n", activePart);
            setBootImageState((activePart == 1) ?
                              BOOT_SET_PART1_IMAGE : BOOT_SET_PART2_IMAGE);
         }

         cmsLog_notice("creating default FirmwareImage objs");
         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            bcmuLog_error("mdm_addObjectInstance for first MDMOID_DEV2_FIRMWARE_IMAGE failed, ret=%d", ret);
            mdm_freeObject((void **)devInfoObj);
            return ret;
         }

         // create a second one
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_FIRMWARE_IMAGE;
         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            bcmuLog_error("mdm_addObjectInstance for second MDMOID_DEV2_FIRMWARE_IMAGE failed, ret=%d", ret);
            mdm_freeObject((void **)devInfoObj);
            return ret;
         }

         (*devInfoObj)->firmwareImageNumberOfEntries = 2;
      }
      else
      {
         // firmwareImageObjs exist, free it and get it again below.
         mdm_freeObject((void **)&fwObj);
      }
   }

   // Firmware objects are now present in MDM, load them into activeFwObj and
   // alternFwObj.
   activePathDesc->oid = MDMOID_DEV2_FIRMWARE_IMAGE;
   PUSH_INSTANCE_ID(&(activePathDesc->iidStack), activePart);
   if ((ret = mdm_getObject(activePathDesc->oid, &(activePathDesc->iidStack),
                               (void **)activeFwObj)) != CMSRET_SUCCESS)
   {
       bcmuLog_error("mdm_getObject error, ret=%d", ret);
       mdm_freeObject((void **)devInfoObj);
       return ret;
   }

   alternPathDesc->oid = MDMOID_DEV2_FIRMWARE_IMAGE;
   PUSH_INSTANCE_ID(&(alternPathDesc->iidStack), alternPart);
   if ((ret = mdm_getObject(alternPathDesc->oid, &(alternPathDesc->iidStack),
                               (void **)alternFwObj)) != CMSRET_SUCCESS)
   {
       bcmuLog_error("mdm_getObject error, ret=%d", ret);
       mdm_freeObject((void **)devInfoObj);
       mdm_freeObject((void **)activeFwObj);
       return ret;
   }

   // Set alias and X_BROADCOM_COM_Partition if not set (new MDM objects).
   if (IS_EMPTY_STRING((*activeFwObj)->alias) ||
       (*activeFwObj)->X_BROADCOM_COM_FLASH_PARTITION == 0)
   {
      setFirmwareObjectAliasAndPartition(*activeFwObj, activePart);
   }

   if (IS_EMPTY_STRING((*alternFwObj)->alias) ||
       (*alternFwObj)->X_BROADCOM_COM_FLASH_PARTITION == 0)
   {
      setFirmwareObjectAliasAndPartition(*alternFwObj, alternPart);
   }

   // Check bootFirmwareImage.
   if (!IS_EMPTY_STRING((*devInfoObj)->bootFirmwareImage))
   {
      // bootFirmwareImage is set, but need to make sure it is "valid".
      // Valid means the image name that bootFirmwareImage points to has not
      // changed between when we set bootFirmwareImage in
      // rcl_dev2DeviceInfoObject and now.  If it has changed, that means
      // somebody uploaded an old config file or is uploading firmware image
      // using bootloader or some other app (wget, ftp, tftp) which do not
      // update the bootFirmwareImage and prevBootName.
      const char *s = (*devInfoObj)->bootFirmwareImage;
      int part = 0;
      UBOOL8 bootFullpathValid = FALSE;

      // Get the last char of the bootFirmwareImage string, which should be
      // the instance number, which can only be 1 or 2.
      part = atoi(&(s[strlen(s)-1]));
      cmsLog_debug("bootFirmwareImage=%s part=%d", s, part);
      if (part == 1 || part == 2)
      {
         cmsImg_getFirmwareNameVersion(part, nameBuf, versionBuf);
         if (!cmsUtl_strcmp((*devInfoObj)->X_BROADCOM_COM_PrevBootName, nameBuf))
         {
            cmsLog_debug("bootFirmwareImage is valid (%s : %s)",
                         s, nameBuf);
            bootFullpathValid = TRUE;
         }
      }

      if (!bootFullpathValid)
      {
         // bootFirmwareImage is not valid, then set bootFirmwareImage to
         // the current active image and null out the (prev)activeFirmwareImage.
         // Also null out (prev)activeFirmwareImage, which will force a
         // COMMIT of the current image to ensure we don't get into a bad
         // bootstate.
         printf("FirmwareSelect: invalid bootFirmwareImage (%s : %s<>%s), overwrite!\n",
                s, (*devInfoObj)->X_BROADCOM_COM_PrevBootName, nameBuf);
         setBootFirmwareToActiveFirmware(activePathDesc, *devInfoObj);
         CMSMEM_FREE_BUF_AND_NULL_PTR((*devInfoObj)->activeFirmwareImage);
      }
   }
   else
   {
      // bootFirmwareImage is not set.  This can be due to a new MDM, or
      // because eponapp/omcid did not set it when they did firmware upgrade.
      // Only set if FirmwareMgmtOwner is tr69.
      if (*isOwnedByTr69)
      {
         setBootFirmwareToActiveFirmware(activePathDesc, *devInfoObj);
      }
   }

   // Check for old version of fwObj.  If so, overwrite old values (includes
   // devInfo activeImage and bootImage)
   if (!cmsUtl_strcmp((*activeFwObj)->alias, "cpe-firmware-A") ||
       !cmsUtl_strcmp((*activeFwObj)->alias, "cpe-firmware-B") ||
       !cmsUtl_strcmp((*alternFwObj)->alias, "cpe-firmware-A") ||
       !cmsUtl_strcmp((*alternFwObj)->alias, "cpe-firmware-B") ||
       ((*activeFwObj)->X_BROADCOM_COM_FLASH_PARTITION != activePart) ||
       ((*alternFwObj)->X_BROADCOM_COM_FLASH_PARTITION != alternPart))
   {
      printf("FirmwareSelect: old TR181 FirmwareImage objects detected, overwrite!\n");
      setFirmwareObjectAliasAndPartition(*activeFwObj, activePart);
      setFirmwareObjectAliasAndPartition(*alternFwObj, alternPart);

      // Since we have swapped around the firmware objects, don't know if
      // activeFirmwareImage and bootFirmwareImage are valid anymore.
      // Set the bootFirmwareImage to the current active image and null out the
      // (prev)activeFirmwareImage, which will force a COMMIT of this image.
      setBootFirmwareToActiveFirmware(activePathDesc, *devInfoObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR((*devInfoObj)->activeFirmwareImage);
   }

   // Since name and version are read-only params, they are never written
   // to config file, so we have to fill them in on every boot.  It is
   // better this way, since it ensures data model is in sync with what
   // is actually in the firmware partition.
   cmsImg_getFirmwareNameVersion((SINT32)activePart, nameBuf, versionBuf);
   CMSMEM_REPLACE_STRING_FLAGS((*activeFwObj)->name, nameBuf, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS((*activeFwObj)->version, versionBuf, mdmLibCtx.allocFlags);
   if (IS_EMPTY_STRING(nameBuf))
   {
      cmsLog_error("Could not get versionString from active image (%d)!!",
                   activePart);
   }
   else
   {
      // This is the image we are running on now, so definitely good and available.
      CMSMEM_REPLACE_STRING_FLAGS((*activeFwObj)->status, MDMVS_AVAILABLE,
                                  mdmLibCtx.allocFlags);
      (*activeFwObj)->available = TRUE;
      (*activeFwObj)->X_BROADCOM_COM_SEQ_NUM = (UINT32) getSequenceNumber(activePart);
   }

   cmsImg_getFirmwareNameVersion((SINT32)alternPart, nameBuf, versionBuf);
   CMSMEM_REPLACE_STRING_FLAGS((*alternFwObj)->name, nameBuf, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS((*alternFwObj)->version, versionBuf, mdmLibCtx.allocFlags);
   if (IS_EMPTY_STRING(nameBuf))
   {
      CMSMEM_REPLACE_STRING_FLAGS((*alternFwObj)->status, MDMVS_NOIMAGE,
                                  mdmLibCtx.allocFlags);
      (*alternFwObj)->available = FALSE;
   }
   else
   {
      UINT32 seqNum = (UINT32) getSequenceNumber(alternPart);
      if ((*alternFwObj)->X_BROADCOM_COM_SEQ_NUM != seqNum)
      {
         // Only write to alternFwObj->{status, available} if seqNum has changed.
         // If seqNum has not changed, this might be a firmware image that
         // has been marked as ACTIVATION_FAILED, so leave it as is.
         CMSMEM_REPLACE_STRING_FLAGS((*alternFwObj)->status, MDMVS_AVAILABLE,
                                     mdmLibCtx.allocFlags);
         (*alternFwObj)->available = TRUE;
         (*alternFwObj)->X_BROADCOM_COM_SEQ_NUM = seqNum;
      }
   }

   // Eventhough we updated the objects in this function, do not set yet.
   // the calling function may do additional updates and it will do the
   // cmsObj_set, free, and save to config.
   return ret;
}

// This function always runs on system bootup.
// This function does more than "init", it's main purpose is to detect 
// whether we have successfully booted on a new image and possibly COMMIT that
// image.
CmsRet mdm_initFirmwareImageObjects_dev2(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2DeviceInfoObject *devInfoObj = NULL;
   Dev2FirmwareImageObject *activeFwObj = NULL;
   Dev2FirmwareImageObject *alternFwObj = NULL;
   InstanceIdStack devInfoIidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor activePathDesc = EMPTY_PATH_DESCRIPTOR;
   MdmPathDescriptor alternPathDesc = EMPTY_PATH_DESCRIPTOR;
   char *activeFullpath = NULL;
   char prevActiveFirmwareImageBuf[CMS_MAX_FULLPATH_LENGTH] = {0};
   UBOOL8 isOwnedByTr69 = FALSE;

   bcmuLog_notice("Entered:");

#ifdef SUPPORT_FW_UPGRADE_WDT
   // Ping watchdog one more time before we do some setup work.
   system("wdtctl ping");
#endif

   ret = addDefaultFirmwareImageObjects_dev2(&devInfoIidStack, &devInfoObj,
                                &activePathDesc, &activeFwObj,
                                &alternPathDesc, &alternFwObj,
                                &isOwnedByTr69);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create default FirmwareObjects, ret=%d", ret);
      return ret;
   }

#ifdef SUPPORT_FW_UPGRADE_WDT
   // Declare the boot a success at this point (although technically, you
   // could argue we should not declare success until sysmgmt component has
   // started.  But this is close enough for now.)
   printf("FirmwareSelect: successful boot on partition %d (%s), "
          "resume normal watchdog operation (wdtd)\n",
          activeFwObj->X_BROADCOM_COM_FLASH_PARTITION, activeFwObj->name);
   system("wdtctl -d -t 30 start");
#endif

   // save the previous activeFirmwareImage
   cmsUtl_strncpy(prevActiveFirmwareImageBuf,
                  devInfoObj->activeFirmwareImage,
                  (SINT32) sizeof(prevActiveFirmwareImageBuf));


   // Always update activeFirmwareImage
   if ((cmsMdm_pathDescriptorToFullPathNoEndDot(&activePathDesc, &activeFullpath)) == CMSRET_SUCCESS)
   {
       CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->activeFirmwareImage,
                                   activeFullpath, mdmLibCtx.allocFlags);
       CMSMEM_FREE_BUF_AND_NULL_PTR(activeFullpath);
   }

   bcmuLog_debug("prevActiveFirmwareImage: %s", prevActiveFirmwareImageBuf);
   bcmuLog_debug("activeFimwareImage=%s", devInfoObj->activeFirmwareImage);
   bcmuLog_debug("bootFirmwareImage=%s", devInfoObj->bootFirmwareImage);

   bcmuLog_debug("active instance %d", PEEK_INSTANCE_ID(&activePathDesc.iidStack));
   bcmuLog_debug("active->FLASH_PARTITION=%d", activeFwObj->X_BROADCOM_COM_FLASH_PARTITION);
   bcmuLog_debug("active->alias=%s", activeFwObj->alias);
   bcmuLog_debug("active->name/version=%s/%s", activeFwObj->name, activeFwObj->version);
   bcmuLog_debug("active->status=%s", activeFwObj->status);
   bcmuLog_debug("active->available=%d", activeFwObj->available);

   bcmuLog_debug("altern instance %d", PEEK_INSTANCE_ID(&alternPathDesc.iidStack));
   bcmuLog_debug("altern->FLASH_PARTITION=%d", alternFwObj->X_BROADCOM_COM_FLASH_PARTITION);
   bcmuLog_debug("altern->alias=%s", alternFwObj->alias);
   bcmuLog_debug("altern->name/version=%s/%s", alternFwObj->name, alternFwObj->version);
   bcmuLog_debug("altern->status=%s", alternFwObj->status);
   bcmuLog_debug("altern->available=%d", alternFwObj->available);

   // Since ftpd and tftpd cannot write to MDM, they write a hint to the
   // scratchpad.  We pick up the hint here and set BootFirmwareImage appropriately.
   {
      char fullpath[1024]={0};
      CmsImageBootFirmwareImageHint hint;
      CmsRet r2;

      r2 = cmsImg_getBootFirmwareImageHint(&hint);
      if (r2 == CMSRET_SUCCESS)
      {
         printf("FirmwareSelect: detected BootFirmwareImageHint from %s (partition=%d timestamp=%s)\n",
                hint.appName, hint.part, hint.timestamp);

         if (hint.part == 1 || hint.part == 2)
         {
            snprintf(fullpath, sizeof(fullpath),
                     "Device.DeviceInfo.FirmwareImage.%d", hint.part);
            CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->bootFirmwareImage,
                                        fullpath, mdmLibCtx.allocFlags);
            cmsImg_deleteBootFirmwareImageHint();
         }
         else
         {
            cmsLog_error("bad partition number %d in BootFirmwareImageHint (must be 1 or 2), ignored", hint.part);
         }
      }
   }

   /*
    * The real work of this function:
    * Figure out what kind of boot this is, and whether we need to COMMIT the
    * image.  First big question to ask is: is FirmwareMgmtOwner TR69 or
    * is bootFirmwareImage set?
    */
   if (isOwnedByTr69 || !IS_EMPTY_STRING(devInfoObj->bootFirmwareImage))
   {
      // FirmwareMgmOwner is TR69, or
      // FirmwareMgmtOwner is not TR69, but SQA has used httpd to update the
      // image, in this case, bootFirmwareImage is correctly set.
      // So the next level question to ask: is bootFirmwareImage (the image
      // we want to boot to) and activeFirmwareImage (the image we actually
      // booted to) the same?
      if (!cmsUtl_strcmp(devInfoObj->bootFirmwareImage, devInfoObj->activeFirmwareImage))
      {
         // Yes, we have booted to the image we wanted to boot to
         // (bootFirmwareImage == activeFirmwareImage).  Final question: is
         // the current activeFirmwareImage the same as the
         // prev activeFirmwareImage?
         if (!cmsUtl_strcmp(prevActiveFirmwareImageBuf, devInfoObj->activeFirmwareImage))
         {
            // This is the most common case, nothing has changed,
            // bootFirmwareImage, activeFirmwareImage, (prev)activeFirmwareImage
            // all point to the same image.
            printf("FirmwareSelect: normal boot on partition %d (%s)\n",
                   activeFwObj->X_BROADCOM_COM_FLASH_PARTITION,
                   activeFwObj->name);
         }
         else
         {
           // bootFirmwareImage == activeFirmwareImage AND
           // (prev)activeFirmwareImage != activeFirmwareImage
           // We are up and running on a new image, or the config file is new
           // or partially overwritten due to "old config file detection" in
           // addDefaultFirmwareImageObjects_dev2, COMMIT this image.
           // Strictly speaking, we do not need to commit if this is a new
           // or overwritten config file, but it does not hurt (and adds some
           // insurance against bad bootstate).
            printf("FirmwareSelect: successful boot on new partition %d (%s) "
                   "or new/overwritten config file\n",
                   activeFwObj->X_BROADCOM_COM_FLASH_PARTITION,
                   activeFwObj->name);
            if (bcmUtl_isUbootNoCommitImage())
            {
               printf("FirmwareSelect: uboot no_commit_image set, user must manually commit image\n");
            }
            else
            {
               printf("FirmwareSelect: COMMIT partition %d\n",
                      activeFwObj->X_BROADCOM_COM_FLASH_PARTITION);
               setBootImageState((activeFwObj->X_BROADCOM_COM_FLASH_PARTITION == 1) ?
                                 BOOT_SET_PART1_IMAGE : BOOT_SET_PART2_IMAGE);
            }
         }
      }
      else
      {
         // activeFirmwareImage != bootFirmwareImage
         // this means we tried the bootFirmwareImage and it failed, so we
         // are back running on the original (old) image.
         // TODO: if updating firmware using bootloader, the code will
         // incorrectly get into this case.  bootstate will be correct, but
         // the alternate partition will be incorrectly marked as invalid, and
         // we print the wrong message below.  bootloader needs to leave a
         // hint in the scratchpad (see cmsImg_writeBootFirmwareImageHint)
         // to let this code know that it updated the firmware.  ftp and tftp
         // does this already, only bootloader does not.
         printf("FirmwareSelect: recovery boot (to orig good partition %d) "
                "after firmware update failure (on new bad partition %d)\n",
                activeFwObj->X_BROADCOM_COM_FLASH_PARTITION,
                alternFwObj->X_BROADCOM_COM_FLASH_PARTITION);

         // the alternFwObj was the new image we tried to boot but it failed,
         // so set status to MDMVS_ACTIVATIONFAILED.
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(alternFwObj->status,
                                           MDMVS_ACTIVATIONFAILED,
                                           mdmLibCtx.allocFlags);
         alternFwObj->available = FALSE;

         // Commit the current active image in case ftpd or tftpd updated
         // the image and marked it as BOOT_ONCE.
         if (bcmUtl_isUbootNoCommitImage())
         {
            printf("FirmwareSelect: uboot no_commit_image set, user must manually commit image\n");
         }
         else
         {
            printf("FirmwareSelect: COMMIT partition %d\n",
                   activeFwObj->X_BROADCOM_COM_FLASH_PARTITION);
            setBootImageState((activeFwObj->X_BROADCOM_COM_FLASH_PARTITION == 1) ?
                                 BOOT_SET_PART1_IMAGE : BOOT_SET_PART2_IMAGE);
         }
      }

      if (!isOwnedByTr69)
      {
         // This happens if we are running a GPON or EPON image, but SQA
         // uses httpd to update the image.  BootFirmwareImage was set and
         // we have to do the processing above, but now we want to null it
         // out here so that if they use eponapp or omcid to update the
         // image, bootFirmwareImage won't end up pointing to the wrong thing
         // (since eponapp/omcid do not set bootFirmwareImage at all).
         // But if SQA uses httpd to update the image again, the code
         // in rcl_dev2DeviceInfoObject will set bootFirmwareImage and
         // everything still works as expected.
         CMSMEM_FREE_BUF_AND_NULL_PTR(devInfoObj->bootFirmwareImage);
         CMSMEM_FREE_BUF_AND_NULL_PTR(devInfoObj->X_BROADCOM_COM_PrevBootName);
      }
   }
   else
   {
      // TR69 is not the owner of the firmware images, and epon and gpon did
      // a firmware upgrade, and they did not set bootFirmwareImage.
      printf("FirmwareSelect: (non-tr69 mgmt) boot on partition %d (%s)\n",
             activeFwObj->X_BROADCOM_COM_FLASH_PARTITION,
             activeFwObj->name);
   }

   /*
    * Set all the objects, which were modified in this function and
    * addDefaultFirmwareImageObjects.
    */
   ret = mdm_setObject((void **)&activeFwObj, &activePathDesc.iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to set active Dev2FirmwareImageObject, ret=%d", ret);
   }

   ret = mdm_setObject((void **)&alternFwObj, &alternPathDesc.iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to set altern Dev2FirmwareImageObject, ret=%d", ret);
   }

   ret = mdm_setObject((void **)&devInfoObj, &devInfoIidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to set Dev2DeviceInfoObject. ret=%d", ret);
   }

   mdm_freeObject((void **)&activeFwObj);
   mdm_freeObject((void **)&alternFwObj);
   mdm_freeObject((void **)&devInfoObj);

   bcmuLog_debug("Save config to flash.");
   cmsMgm_saveConfigToFlash();

   bcmuLog_debug("Exit.");
   return ret;
}


#ifdef DMP_DEVICE2_SUPPORTEDDATAMODEL_1
CmsRet mdm_addDefaultSupportedDataModelObjects_dev2(void)
{
   char uuid[BUFLEN_48]={0}, features[BUFLEN_64]={0};
   CmsRet ret = CMSRET_SUCCESS;
   Dev2SupportedDataModelObject *supportedDataModelObj = NULL;
   Dev2DeviceInfoObject *deviceInfoObj = NULL;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

   bcmuLog_debug("Adding default supported data model objects (enter)");

   /* Check if there is a Device.DeviceInfo.SupportedDataModel.{i}. instanace already in MDM. */
   ret = mdm_getNextObject(MDMOID_DEV2_SUPPORTED_DATA_MODEL,
                           &pathDesc.iidStack,
                           (void **) &supportedDataModelObj);

   // TODO: Peter Tran: double check this logic.  If already in MDM, we still
   // continue in this function and increment upportedDataModelNumberOfEntries?

   /* if Dev2SupportedDataModelObject cannot be found then add and get it */
   if (ret != CMSRET_SUCCESS)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_SUPPORTED_DATA_MODEL;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         bcmuLog_error("mdm_addObjectInstance for MDMOID_DEV2_SUPPORTED_DATA_MODEL failed, ret=%d", ret);
         return ret;
      }

      if ((ret = mdm_getObject(pathDesc.oid,
                               &pathDesc.iidStack,
                               (void **) &supportedDataModelObj)) != CMSRET_SUCCESS)
      {
         bcmuLog_error("Failed to get Dev2SupportedDataModelObject, ret=%d", ret);
         return ret;
      }
   }

   /* URL */
   CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->URL,
                               "http://localhost/data-model/bbf-data-model-2.xml",
                               mdmLibCtx.allocFlags);

   /* UUID */
   if (rutSys_getUuidFromBbfDataModel(sizeof(uuid)-1, uuid) == CMSRET_SUCCESS)
   {
      CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->UUID,
                                  uuid,
                                  mdmLibCtx.allocFlags);
   }

   /* URN */
   CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->URN,
                               "urn:broadband-forum-org:tr-181-2-10-0",
                               mdmLibCtx.allocFlags);

   /* features */
   if (rutSys_getFeaturesFromDataModel(sizeof(features)-1, features) == CMSRET_SUCCESS)
   {
      CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->features,
                                  features,
                                  mdmLibCtx.allocFlags);
   }

   bcmuLog_debug("Before set Dev2ProcessorObject, URL='%s', UUID='%s', URN='%s', features='%s'",
                supportedDataModelObj->URL, supportedDataModelObj->UUID,
                supportedDataModelObj->URN, supportedDataModelObj->features);

   ret = mdm_setObject((void **) &supportedDataModelObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **)&supportedDataModelObj);

   if (ret != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to set Dev2SupportedDataModelObject. ret=%d", ret);
      return ret;
   }

   /* Increment supportedDataModelNumberOfEntries (use generic function?) */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DEVICE_INFO;

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&deviceInfoObj)) != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to get Dev2DeviceInfoObject, ret=%d", ret);
      return ret;
   }

   deviceInfoObj->supportedDataModelNumberOfEntries++;
   bcmuLog_debug("Setting supportedDataModelNumberOfEntries to %d",
                deviceInfoObj->supportedDataModelNumberOfEntries++);

   ret = mdm_setObject((void **)&deviceInfoObj, &(pathDesc.iidStack), FALSE);
   mdm_freeObject((void **)&deviceInfoObj);

   if (ret != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to set Dev2DeviceInfoObject. ret=%d", ret);
      return ret;
   }

   return ret;
}
#endif  /* DMP_DEVICE2_SUPPORTEDDATAMODEL_1 */

#ifdef DMP_DEVICE2_PROCESSORS_1
CmsRet mdm_addDefaultProcessorObjects_dev2()
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 i = 0, processorNum = 0;
   UINT32 frequency=0;
   char architecture[BUFLEN_128]={0};
   Dev2ProcessorObject *processorObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;

   bcmuLog_debug("Enter:");

   // The Device.DeviceInfo.Processor objs are not written to the config file,
   // so they are regenerated on every reboot.  If we see existing objects,
   // log an error.
   ret = mdm_getNextObject(MDMOID_DEV2_PROCESSOR, &iidStack, (void **) &processorObj);
   if (ret == CMSRET_SUCCESS)
   {
      bcmuLog_error("Unexpected existing DeviceInfo.Processor object(s) found!");
      mdm_freeObject((void **) &processorObj);
      return ret;
   }

   processorNum = sysUtil_getNumCpuThreads();
   for (i = 0; i < processorNum; i++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_PROCESSOR;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         bcmuLog_error("mdm_addObjectInstance for MDMOID_DEV2_PROCESSOR failed, ret=%d", ret);
         return ret;
      }

      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &processorObj)) != CMSRET_SUCCESS)
      {
         bcmuLog_error("failed to get Dev2ProcessorObject object, ret=%d", ret);
         return ret;
      }

      if (sysUtil_getCpuInfo(i, &frequency, architecture) == BCMRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING_FLAGS(processorObj->architecture, architecture, mdmLibCtx.allocFlags);
         processorObj->X_BROADCOM_COM_Frequency = frequency;
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(processorObj->architecture, "", mdmLibCtx.allocFlags);
         processorObj->X_BROADCOM_COM_Frequency = 0;
      }

      bcmuLog_debug("Before set Dev2ProcessorObject, arch=%s, freq=%d",
                   processorObj->architecture, processorObj->X_BROADCOM_COM_Frequency);

      ret = mdm_setObject((void **) &processorObj, &pathDesc.iidStack, FALSE);
      mdm_freeObject((void **)&processorObj);

      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("Failed to set Dev2ProcessorObject. ret=%d", ret);
         return ret;
      }
   }

   printf("devinfo_md: Detected %d processors (arch=%s freq=%dMHz)\n",
          processorNum, architecture, frequency);
   return ret;
}

#endif /* DMP_DEVICE2_PROCESSORS_1 */

#if 0
CmsRet tr69RetrieveTransferListFromStore(DownloadReqInfo *list, UINT16 *size)
{
   char buf[sizeof(DownloadReqInfo) * TRANSFER_QUEUE_SIZE] = {0};
   UINT32 bufSz = sizeof(DownloadReqInfo) * TRANSFER_QUEUE_SIZE;
   CmsRet ret=CMSRET_INVALID_ARGUMENTS;
   SINT32 count;

   count = getScratchPad("tr69c_transfer", buf, bufSz);
   if (count < 0)
   {
      cmsLog_error("error during scratchpad read of tr69c_transfer (buffer too small?), count=%d", count);
      *size = 0;
   }
   else if (count == 0)
   {
      /* could not find tr69c_transfer in scratch pad or other error */
      cmsLog_debug("could not find tr69c_transfer in scratch pad");
      *size = 0;
   }
   else
   {
      cmsLog_debug("read %d bytes from scratch pad for tr69c_transfer", count);
      *size = (UINT16) count;
      memcpy((void*)list, buf, *size);
      ret = CMSRET_SUCCESS;
   }

   return ret;
}

void initTransferList(void)
{
   struct sysinfo sysInfo;
   UINT16 size, i, queueEntryCount;
   DownloadReq *q;
   RPCAction *a;
   DownloadReqInfo savedList[TRANSFER_QUEUE_SIZE], *saved;

   memset((void*)&transferList,0,sizeof(TransferInfo));
   size = i = queueEntryCount = 0;

   sysinfo(&sysInfo);

   /* if retrieve fails because nothing was read or error occured, clear list */
   if (tr69RetrieveTransferListFromStore(savedList, &size) == CMSRET_SUCCESS && size > 0)
   {
      /* remove the tr69c_transfer list from scratch pad by setting it again with len=0 */
      setScratchPad("tr69c_transfer", NULL, 0);

      queueEntryCount = size / sizeof(DownloadReqInfo);
      for (i = 0; i < queueEntryCount; i++)
      {
         saved = &savedList[i];
         q = &transferList.queue[i].request;

         q->efileType = saved->efileType;
         q->commandKey = (saved->commandKey != NULL) ? cmsMem_strdup(saved->commandKey) : NULL;
         q->url = (saved->url != NULL) ? cmsMem_strdup(saved->url) : cmsMem_strdup("");
         q->user = (saved->user != NULL) ? cmsMem_strdup(saved->user) : cmsMem_strdup("");
         q->pwd = (saved->pwd != NULL) ? cmsMem_strdup(saved->pwd) : cmsMem_strdup("");
         q->fileSize = saved->fileSize;
         q->fileName = (saved->fileName != NULL) ? cmsMem_strdup(saved->fileName) : cmsMem_strdup("");

         // calculate new delay -- consider time elapsed during reboot
         q->baseTime = sysInfo.uptime;
         if ((saved->delaySec - (int)sysInfo.uptime) > 0)
            q->delaySec = saved->delaySec - (int)sysInfo.uptime;
         else
            q->delaySec = 1;

         q->state = saved->state;
         transferList.queue[i].rpcMethod = saved->rpcMethod;
         if (q->state == eTransferNotYetStarted)
         {
            a = newRPCAction(); /* need to be freed somewhere? */
            memcpy(&a->ud.downloadReq,q,sizeof(DownloadReq));
            a->rpcMethod = transferList.queue[i].rpcMethod;
            if (transferList.queue[i].rpcMethod == rpcDownload)
            {
               cmsTmr_set(tmrHandle, downloadStart, (void *)a, (1+q->delaySec)*1000, "download");
            }
            else
            {
               cmsTmr_set(tmrHandle, uploadStart, (void *)a, (1+q->delaySec)*1000, "upload");
            }
         } /* eTransferNotYetStarted */
      } /* for */
   } /* read from scratch pad */
   else
   {
#ifdef DEBUG
      printf("initTransferList(): no list read from persistent storage.\n");
#endif
   }
} /* initTransferList */
#endif


#endif  /* DMP_DEVICE2_BASELINE_1 */
