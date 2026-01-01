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


#include "cms_core.h"
#include "cms_util.h"
#include "mdm.h"

#ifdef DMP_DEVICE2_BASELINE_1

#include "bcm_flashutil.h"
#include "rut2_firmware.h"


/***************************************************************************
 *
 * This is a high level description of the "try new firmware image once,
 * and commit if booted successfully" feature that became available in
 * CMS and BDK starting with the 5.02L.07 release.  Note that a major
 * refactoring of the code was done in 5.04L.02, so these comments only apply
 * to 5.04L.02 and beyond.
 * These comments are only intended to give a high level overview of the
 * feature.  Please also read the detailed comments in
 * mdm2_init_devinfo.c, rcl2_devinfo.c, stl2_firmware.c, and rut2_firmware.c.
 *
 *
 * OVERVIEW:
 *
 * TR69 Appendix V: Multiple Firmware Images - Theory of Operation and 
 * TR181 DeviceInfo.BootFirmwareImage and DeviceInfo.FirmwareImage.{i}.
 * describe how modern CPE's can download a new image, attempt to boot it,
 * and if that boot fails, mark that firmware image as "ActivationFailed",
 * and boot to the original (good) image.  If the boot on the new firmware
 * image is successful, then the new image is "committed" and used in
 * subsequent boots.  CMS PURE181 and BDK implement this functionality in
 * these DeviceInfo related files.
 *
 * This functionality is not available in TR98 or Hybrid98+181 mode.
 *
 * This functionality is not available if the bootloader is CFE.  It is only
 * available if the bootloader is UBoot.
 *
 * This functionality is disabled on xPON images, where the
 * DeviceInfo.X_BROADCOM_COM_FirmwareMgmtOwner is not MDMVS_TR69.  omcid and
 * eponapp have their own algorithms for managing the firmware images.
 * However, if a developer or SQA uses the Broadcom WebUI to download a new
 * image, and the data model is TR181, and the bootloader is Uboot, then this
 * feature will be activated.
 *
 * To disable this feature (that is, immediately commit the new image
 * and boot it), look in rcl_dev2DeviceInfoObject() and modify the
 * logic around bcmUtl_isBootloaderUboot().
 *
 * The code will print out messages at key points to indicate what it is
 * doing.  Look for messages starting with "FirmwareSelect".
 *
 *
 * BASIC FLOW of EVENTS:
 *
 * (Here we are only talking about Type-1 firmware download, which means
 * download the image and immediately try to boot it.  There is another type
 * of download, Type-6, which is not described here.  But once you understand
 * how Type-1 works, Type-6 will be easy to understand as well.)
 *
 * Most Broadcom SoC's now have a flash layout which can hold 2 firmware images.
 * When an app downloads a new firmware image, the low level flash software
 * (public/libs/bcm_flashutil) will automatically write it to the "alternate"
 * image partition.  The system is running on the "current" image partition.
 * You can inspect the state of the firmware partitions using bcm_bootstate.
 *
 * After the firmware image is written, the app will call
 * rutFwImg_updateFirmwareObject(), which will write various parameters in
 * the data model and make various calls to the low level firmware image
 * management code so that on the next reboot, the UBoot bootloader will 
 * load the new image (just once).  Note that the system must do a "warm"
 * reboot, meaning the system does not lose power.  (Loss of power and then
 * boot is called a "cold" boot/reboot.)
 *
 * When the system boots again, mdm_initFirmwareImageObjects_dev2() will
 * get called.  This function determines if we have successfully booted
 * on the new firmware image, and if so, "commits" the new image.
 *
 * If the system fails to boot on the new image, then through
 * kernel panic and reboot, watchdog timer forced reboot, or physical power
 * off and then on reboot, the system will boot the original (good, comitted)
 * image.  mdm_initFirmwareImageObjects_dev2() will also detect this condition
 * and mark the new firmware image as "ActivationFailed".
 *
 *
 * COMPLEX SCENARIOS:
 * 
 * This section describes more complex and unusual scenarios.  They are
 * unlikely to occur in the field, but may occur during development time.
 *
 * 1. New image is written via bootloader.  This condition is detected in
 *    addDefaultFirmwareImageObjects_dev2() by comparing the imageVersion
 *    tag in the current running image with DeviceInfo.X_BROADCOM_COM_PrevBootName.
 *    If they are different, then the current running image is committed.
 *
 * 2. CMS/BDK config file does not exist.  The config file may have been
 *    deleted by a "restoredefault" operation, or the new image
 *    does not understand the config file that is written in flash.  Since
 *    the software needs the state written in the config file to determine what
 *    to do, and since that state is gone, the current running image is
 *    committed.
 *
 * 3. System is currently running a CMS TR181 or BDK image, and a new
 *    image which uses TR98 or Hybrid TR98+181 is loaded.  Point 2 above
 *    applies.  Since the new image does not understand the existing TR181
 *    based config file that is written in the flash, the config file will
 *    be invalidated and a new one started.  The new image will be committed.
 *
 ***************************************************************************
 */

// On successful return, caller must free fwObj
CmsRet rutFwImg_getFwObj(MdmPathDescriptor *pathDesc,
                         Dev2FirmwareImageObject **fwObj)
{
   UINT32 part;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   if ((pathDesc == NULL) || (fwObj == NULL))
   {
      cmsLog_error("NULL input args %p/%p", pathDesc, fwObj);
      return CMSRET_INTERNAL_ERROR;
   }

   pathDesc->oid = MDMOID_DEV2_FIRMWARE_IMAGE;
   part = PEEK_INSTANCE_ID(&(pathDesc->iidStack));
   if (part < 1 || part > 2)
   {
      cmsLog_error("invalid parttion %d", part);
      return CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("read/refresh FirmwareImage info for partition %d", part);
   ret = cmsObj_get(pathDesc->oid, &(pathDesc->iidStack), 0, (void **)fwObj);
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("fwImageObj[%d]: name=%s seqNum=%d status=%s avail=%d",
                   part, (*fwObj)->name, (*fwObj)->X_BROADCOM_COM_SEQ_NUM,
                   (*fwObj)->status, (*fwObj)->available);
   }
   else
   {
      cmsLog_error("Get of partition %d failed, ret=%d", part, ret);
   }

   return ret;
}

// On successful return, caller must free fwObj
CmsRet rutFwImg_getActiveFwObj(MdmPathDescriptor *pathDesc,
                               Dev2FirmwareImageObject **fwObj)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   UINT32 activePart = 0;

   activePart = (UINT32) getBootPartition();
   if (activePart < 1 || activePart > 2)
   {
      cmsLog_error("getBootPartition returned %d", activePart);
      return CMSRET_INTERNAL_ERROR;
   }

   pathDesc->oid = MDMOID_DEV2_FIRMWARE_IMAGE;
   PUSH_INSTANCE_ID(&(pathDesc->iidStack), activePart);

   ret = rutFwImg_getFwObj(pathDesc, fwObj);
   return ret;
}

// On successful return, caller must free fwObj
CmsRet rutFwImg_getAlternFwObj(MdmPathDescriptor *pathDesc,
                               Dev2FirmwareImageObject **fwObj)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   UINT32 activePart = 0;
   UINT32 alternPart = 0;

   /* downloaded firmware must have been written to the alternate partition */
   activePart = (UINT32) getBootPartition();
   if (activePart < 1 || activePart > 2)
   {
      cmsLog_error("getBootPartition returned %d", activePart);
      return CMSRET_INTERNAL_ERROR;
   }
   alternPart = (activePart == 1) ? 2 : 1;

   pathDesc->oid = MDMOID_DEV2_FIRMWARE_IMAGE;
   PUSH_INSTANCE_ID(&(pathDesc->iidStack), alternPart);

   ret = rutFwImg_getFwObj(pathDesc, fwObj);
   return ret;
}

static CmsRet setFirmwareUpdatedHint()
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2FirmwareImageObject *fwObj = NULL; 
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

   // Set the isUpdated param in the altern firmware object which was just
   // updated so that stl_dev2FirmwareImageObject will recognize this is
   // a new firmware image, even if the imageName did not change.
   // (Developers and SQA will sometimes load the same image into the same
   // partition.)
   ret = rutFwImg_getAlternFwObj(&pathDesc, &fwObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not find FirmwareObj for partition %d",
                   PEEK_INSTANCE_ID(&(pathDesc.iidStack)));
      return CMSRET_INTERNAL_ERROR;
   }

   fwObj->X_BROADCOM_COM_isUpdated = TRUE;

   ret = cmsObj_setFlags(fwObj, &(pathDesc.iidStack), 0);
   cmsObj_free((void **)&fwObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of isUpdated hint failed, ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }


   // Clear the PrevBootName in the deviceInfo obj so rcl_dev2DeviceInfoObject
   // will detect this is a new image, even if the imageName did not change.
   // (Developers and SQA will sometimes load the same image into the same
   // partition.)
   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2DeviceInfoObject *devInfoObj = NULL;
      ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO, &iidStack, 0,
                       (void **) &devInfoObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get DeviceInfo obj! ret=%d", ret);
         return ret;
      }

      // TODO: workaround issue in remote_objd, cannot set a param to NULL?
      CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->X_BROADCOM_COM_PrevBootName,
                                  FW_UPDATE_HINT_STR, mdmLibCtx.allocFlags);

      ret = cmsObj_setFlags(devInfoObj, &iidStack, 0);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of DeviceInfo obj failed, ret=%d", ret);
      }
      cmsObj_free((void **) &devInfoObj);
   }

   return ret;
}

CmsRet rutFwImg_updateFirmwareObject_dev2(eFileType efileType)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2FirmwareImageObject *fwObj = NULL; 
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

   cmsLog_notice("Entered: efileType=%d", efileType);

   // defined in cms_image.h: eFirmwareUpgrade (1), eFirmwareStored (6)
   if (efileType != eFirmwareUpgrade && efileType != eFirmwareStored)
      return CMSRET_SUCCESS;

   // We have just updated an image, set/clear some params so we will
   // definitely detect this updated image and try to boot it.
   ret = setFirmwareUpdatedHint();
   if (ret != CMSRET_SUCCESS)
      return ret;

   // Must get the altern FwObj again after setting the update hint.
   rutFwImg_getAlternFwObj(&pathDesc, &fwObj);
   cmsObj_free((void **)&fwObj);

   // If this is a type-1 firmware download, we automatically set the
   // bootFirmwareImage for the caller.  If this is a type-6, then ACS/TR69
   // will set bootFirmwareImage via setParameterValues RPC later.
   if (efileType == eFirmwareUpgrade)
   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2DeviceInfoObject *devInfoObj = NULL;
      char *bootFullpath = NULL;

      ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO, &iidStack, 0,
                       (void **) &devInfoObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get DeviceInfo obj! ret=%d", ret);
         return ret;
      }

      // form new bootFirmwareImage fullpath
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &bootFullpath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot bootFullpath returns error. ret=%d", ret);
         return ret;
      }
       
      CMSMEM_REPLACE_STRING_FLAGS(devInfoObj->bootFirmwareImage, bootFullpath,
                                  mdmLibCtx.allocFlags);
      CMSMEM_FREE_BUF_AND_NULL_PTR(bootFullpath);

      // set deviceInfoObj and free
      ret = cmsObj_setFlags(devInfoObj, &iidStack, 0);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("set of DeviceInfo obj failed, ret=%d", ret);
      }
      cmsObj_free((void **) &devInfoObj);
   }

   cmsMgm_saveConfigToFlash();
   return ret;
}


CmsRet rutFwImg_updateFirmwareObject(eFileType efileType)
{
   if (cmsMdm_isDataModelDevice2())
      return rutFwImg_updateFirmwareObject_dev2(efileType);
   else
      return CMSRET_SUCCESS;
}

#else

CmsRet rutFwImg_updateFirmwareObject(eFileType efileType __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif    /* DMP_DEVICE2_BASELINE_1 */

