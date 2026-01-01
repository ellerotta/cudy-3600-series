/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1


#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_firmware.h"
#include "bcm_flashutil.h"
#include "bcm_boardutils.h"
#include "bcm_hwdefs.h"


// A small local helper function.  Caller is responsible for freeing the obj.
static Dev2FirmwareImageObject *getFirmwareImageObj(const char *bootFirmwareImage)
{
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   Dev2FirmwareImageObject *fwImageObj = NULL;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(bootFirmwareImage, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid bootFirmwareImage fullpath %s",
                   bootFirmwareImage);
      return NULL;
   }

   if ((ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0,
                         (void **) &fwImageObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Dev2FirmwareImageObject at %s, ret=%d",
                   bootFirmwareImage, ret);
      return NULL;
   }

   return fwImageObj;
}


CmsRet rcl_dev2DeviceInfoObject( _Dev2DeviceInfoObject *newObj,
                const _Dev2DeviceInfoObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      Dev2FirmwareImageObject *fwImageObj = NULL;
      UBOOL8 isChangeDetected = FALSE;
      UBOOL8 isChangeRestricted = FALSE;

      // If the caller is TR69 or TR69C, and the FirmwareMgmtOwner is not TR69,
      // then we not allowed to set bootFirmwareState.  That is, the params
      // that tr69 is allowed to change is RESTRICTED (the only
      // params tr69 is allowed to change is provisioningCode).
      if ((mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_TR69C_2) &&
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_FirmwareMgmtOwner, MDMVS_TR69))
      {
         isChangeRestricted = TRUE;
      }

      if (cmsUtl_strcmp(currObj->provisioningCode, newObj->provisioningCode))
      {
         isChangeDetected = TRUE;
      }

#ifdef DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1
      if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_E2E_ProvisioningCode,
                        newObj->X_BROADCOM_COM_E2E_ProvisioningCode))
      {
         isChangeDetected = TRUE;
      }
#endif

      if (isChangeRestricted)
      {
         if (isChangeDetected)
         {
            // Assume TR69c is not trying to change one of the provisioning code
            // params AND the BootFirmwareImage param at the same time.  So if
            // we detect a change in one of the provisioning code params, just
            // accept that and return.
            return CMSRET_SUCCESS;
         }
         else
         {
            // The only possible change at this point is BootFirmwareImage,
            // and we are not allowed to do change it if the FirmwareMgmtOnwer
            // is not TR69.
            cmsLog_error("FirmwareMgmtOwner is %s, reject setting",
                         newObj->X_BROADCOM_COM_FirmwareMgmtOwner);
            return CMSRET_REQUEST_DENIED;
         }
      }

      // If we get here, that means that:
      // 1. FirmwareMgmtOwner is TR69, so we are not "restricted".
      // 2. FirmwareMgmtOwner is not TR69, meaning EPON or GPON build, and
      //    httpd is doing firmware update.  httpd will write to
      //    BootFirmwareState even in EPON/GPON mode.  But eponapp/omcid will
      //    not write to bootFirmwareState.
      cmsLog_debug("possible change bootFirmwareImage %s => %s",
                   currObj->bootFirmwareImage, newObj->bootFirmwareImage);

      // Detect setFirmwareUpdatedHint setting magic hint string in
      // PrevBootName.  Just accept it and return.
      if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_PrevBootName, FW_UPDATE_HINT_STR) &&
          !cmsUtl_strcmp(newObj->X_BROADCOM_COM_PrevBootName, FW_UPDATE_HINT_STR))
      {
         cmsLog_debug("detected change PrevBootName %s => %s",
                      currObj->X_BROADCOM_COM_PrevBootName,
                      newObj->X_BROADCOM_COM_PrevBootName);
         return CMSRET_SUCCESS;
      }

      // In GPON/EPON configs, bootFirmwareImage will normally be NULL, except
      // if someone tries to update the image using httpd.
      if (IS_EMPTY_STRING(newObj->bootFirmwareImage))
      {
         cmsLog_debug("detected NULL bootfirmwareimage, return");
         return CMSRET_SUCCESS;
      }

      fwImageObj = getFirmwareImageObj(newObj->bootFirmwareImage);
      if (fwImageObj == NULL)
      {
         return CMSRET_INVALID_PARAM_VALUE;
      }

      cmsLog_debug("currObj->prevBootname=%s new fwImageName=%s",
                   currObj->X_BROADCOM_COM_PrevBootName, fwImageObj->name);

      // We have to check for change in bootFirmwareImage AND the image name that
      // bootFirmwareImage points to because if bootFirmwareImage is pointing
      // to instance 2, try to boot it and fail, we will end up running on
      // instance 1 but bootFirmwareImage will still be pointing to 2.  Now
      // we download a new (good) image to instance 2, and set
      // bootFirmwareImage to 2, but it is already pointing to 2!!  So the
      // only way we can detect a change in this scenario is to also track
      // the last image name that bootFirmwareImage pointed to.
      // (Surprisingly, cannot use sequence number because in the above
      // scenario, (by desgin) seqNum does not change.)
      if (cmsUtl_strcmp(newObj->bootFirmwareImage, currObj->bootFirmwareImage) ||
          cmsUtl_strcmp(currObj->X_BROADCOM_COM_PrevBootName, fwImageObj->name))
      {
         /* The firmware image instance referenced by this parameter must also
          * have an Available parameter value of 'true'. Attempting to set this
          * parameter to point to a non-enabled firmware image MUST result in
          * the CPE responding with a CWMP fault (9007).
          */
         if (fwImageObj->available)
         {
            int bootState = -1;
            int rc = -1;

            if (bcmUtl_isBootloaderUboot())
            {
               printf("FirmwareSelect: set partition %d to BOOT_ONCE\n",
                      fwImageObj->X_BROADCOM_COM_FLASH_PARTITION);
               bootState = (fwImageObj->X_BROADCOM_COM_FLASH_PARTITION == 1) ?
                           BOOT_SET_PART1_IMAGE_ONCE : BOOT_SET_PART2_IMAGE_ONCE;
            }
            else
            {
               // CFE does not support BOOT_ONCE, so just COMMIT new image now.
               printf("FirmwareSelect: COMMIT partition %d (due to CFE)\n",
                      fwImageObj->X_BROADCOM_COM_FLASH_PARTITION);
               bootState = (fwImageObj->X_BROADCOM_COM_FLASH_PARTITION == 1) ?
                           BOOT_SET_PART1_IMAGE : BOOT_SET_PART2_IMAGE;
            }

            rc = setBootImageState(bootState);
            if (rc != 0)
            {
               cmsLog_error("setBootImageState(%d) failed, rc=%d", bootState, rc);
               ret = CMSRET_INTERNAL_ERROR;
            }
            else
            {
               // Record the fact that we are now pointing to a firmwareImage
               // obj with this seqnum.
               CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_PrevBootName,
                                           fwImageObj->name,
                                           mdmLibCtx.allocFlags);
            }
         }
         else
         {
            cmsLog_error("cannot set bootFirmwareImage to %s (not available).",
                         newObj->bootFirmwareImage);
            ret = CMSRET_INVALID_PARAM_VALUE;  // (9007)
         }
      }

      cmsObj_free((void **)&fwImageObj);
   }

   cmsLog_debug("Exit: ret=%d", ret);
   return ret;
}

#ifdef DMP_DEVICE2_PROCESSORS_1

CmsRet rcl_dev2ProcessorObject( _Dev2ProcessorObject *newObj __attribute__((unused)),
                const _Dev2ProcessorObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* Nothing to implement here yet */

   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_PROCESSORS_1 */


#endif    /* DMP_DEVICE2_BASELINE_1 */

