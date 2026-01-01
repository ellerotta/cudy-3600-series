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


#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"



CmsRet rcl_dev2FirmwareImageObject( _Dev2FirmwareImageObject *newObj,
                const _Dev2FirmwareImageObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Entered: instance=%d", PEEK_INSTANCE_ID(iidStack));

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("Add new instance.");
      /* FirmwareImage object is a Multiple Instance type 2 object.
       * It can only be added during mdm init.  There will be one instance
       * per image partition, so usually, there will be exactly 2 instances
       * on the system.  After the instances are created during mdm_init,
       * they will not be deleted or added to again.
       */
      /* firmwareImageNumberOfEntries is manually updated by mdm init. */
      return CMSRET_SUCCESS;
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_error("Cannot delete FirmwareImage instance!");

      /* Per tr181, firmware image instances cannot be deleted. */
      return CMSRET_INVALID_PARAM_VALUE;
   }

   // At this point, we know this is a "change existing" operation.
   // The only params that can be set in this obj are:
   // alias, available (boolean), isUpdated (hint), and
   // checkSumAlgorithm, expectedChecksum, and calculatedChecksum (used during
   // obuspa image download, set here and checked in stl_dev2FirmwareImageObject.
   if (newObj->available != currObj->available)
   {
      if (newObj->available == TRUE)
      {
          if (cmsUtl_strcmp(currObj->status, MDMVS_AVAILABLE))
          {
             cmsLog_error("Cannot set FirmwareImage available=TRUE because status is %s",
                          currObj->status);
             ret = CMSRET_INVALID_PARAM_VALUE;
          }
      }
      else
      {
         Dev2DeviceInfoObject *devInfoObj = NULL;
         InstanceIdStack devInfoIidStack = EMPTY_INSTANCE_ID_STACK;
         MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
         char *fullpath = NULL;

         if ((ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO,
                               &devInfoIidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&devInfoObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Dev2DeviceInfoObject, ret=%d", ret);
            return ret;
         }

         // caller wants to set newObj->available to FALSE.
         // Per tr181, available cannot be set to false if the firmware image
         // is active or is referenced by the BootFirmwareImage parameter.
         pathDesc.oid = MDMOID_DEV2_FIRMWARE_IMAGE;
         pathDesc.iidStack = *iidStack;
         if((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullpath)) !=  CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
            cmsObj_free((void **)&devInfoObj);
            return CMSRET_INVALID_PARAM_VALUE;
         }

         if (cmsUtl_strcmp(devInfoObj->activeFirmwareImage, fullpath) == 0)
         {
            cmsLog_error("Available cannot be set to false if the firmware image is active.");
            ret = CMSRET_INVALID_PARAM_VALUE; 
         }
         else if (cmsUtl_strcmp(devInfoObj->bootFirmwareImage, fullpath) == 0)
         {
            cmsLog_error("Available cannot be set to false if the firmware image is referenced by the BootFirmwareImage parameter.");
            ret = CMSRET_INVALID_PARAM_VALUE; 
         }

         cmsMem_free(fullpath);
         cmsObj_free((void **)&devInfoObj);
      }
   }

   // For the other param changes, no action needed, just accept the value
   if (newObj->X_BROADCOM_COM_isUpdated != currObj->X_BROADCOM_COM_isUpdated)
      cmsLog_debug("isUpdated: %d => %d", currObj->X_BROADCOM_COM_isUpdated, newObj->X_BROADCOM_COM_isUpdated);

   if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_checkSumAlgorithm, currObj->X_BROADCOM_COM_checkSumAlgorithm) ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_expectedCheckSum, currObj->X_BROADCOM_COM_expectedCheckSum) ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_actualCheckSum, currObj->X_BROADCOM_COM_actualCheckSum))
   {
      cmsLog_debug("new CheckSumAlgo %s", newObj->X_BROADCOM_COM_checkSumAlgorithm);
      cmsLog_debug("new expectedCheckSum %s", newObj->X_BROADCOM_COM_expectedCheckSum);
      cmsLog_debug("new actualCheckSum   %s", newObj->X_BROADCOM_COM_actualCheckSum);
   }

   cmsLog_debug("Exit: ret=%d", ret);
   return ret;
}

#endif    /* DMP_DEVICE2_BASELINE_1 */

