/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom
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

#include "cms_phl.h"
#include "cms_mgm.h"
#include "cms_mem.h"
#include "cms_image.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"
#include "bcm_zbus_intf.h"
#include "bcm_generic_hal.h"
#include "cms_obj.h"  // for OGF_LOCAL_MDM_ONLY


/*!\file zbus_strproto.c
 * \brief Z-Bus inbound handlers for string protocol functions.
 *        DBus and UBus inbound method handlers will unwrap the bus related
 *        stuff from the actual data and then call these functions
 *        with BCM Generic data structures.  These functions will call the
 *        BCM Generic HAL to do the work.  If there is return data, the DBus
 *        and UBus handlers will wrap bus stuff back onto the return data and
 *        send back on the bus.
 *
 */


/** Handle inbound getParameterNames call.
 */
BcmRet zbus_in_getParameterNames(const char *fullpath,
              UBOOL8             nextLevelOnly,
              UINT32             flags,
              BcmGenericParamInfo **paramInfoArray,
              UINT32               *numParamInfos)
{
   bcmuLog_notice("Entered: %s nextLevelOnly=%d flags=0x%x",
                 fullpath, (int)nextLevelOnly, flags);

   // When we get this request from the bus, we only want to get the
   // parameter names from our local MDM.  Do not trigger the remote_objd
   // to get all the (remote) parameter names.
   if ((flags & OGF_LOCAL_MDM_ONLY) == 0)
   {
      bcmuLog_error("Automatically adding OGF_LOCAL_MDM_ONLY to flags (%s)", fullpath);
      flags |= OGF_LOCAL_MDM_ONLY;
   }

   return (bcm_generic_getParameterNames(fullpath, nextLevelOnly, flags,
                                         paramInfoArray, numParamInfos));
}


/** Handle inbound getParameterValues method call.
 */
BcmRet zbus_in_getParameterValues(const char          **fullpathArray,
                                  UINT32                numFullpaths,
                                  UBOOL8                nextLevel,
                                  UINT32                flags,
                                  BcmGenericParamInfo **paramInfoArray,
                                  UINT32               *numParamInfos)
{
   bcmuLog_notice("Entered: numFullpaths=%d nextLevel=%d flags=0x%x",
                 numFullpaths, (int)nextLevel, flags);

   // When we get this request from the bus, we only want to get the
   // parameter values from our local MDM.  Do not trigger the remote_objd
   // to get all the (remote) parameter values.
   if ((flags & OGF_LOCAL_MDM_ONLY) == 0)
   {
      bcmuLog_error("Automatically adding OGF_LOCAL_MDM_ONLY to flags");
      bcmuLog_error("first fullpath of %d: %s", numFullpaths, fullpathArray[0]);
      flags |= OGF_LOCAL_MDM_ONLY;
   }

   return (bcm_generic_getParameterValues(fullpathArray,
                                          numFullpaths,
                                          nextLevel,
                                          flags,
                                          paramInfoArray,
                                          numParamInfos));

}


/** Handle inbound setParameterValues call.
 */
BcmRet zbus_in_setParameterValues(BcmGenericParamInfo *paramInfoArray,
                                  UINT32               numParamInfos,
                                  UINT32               flags)
{
   BcmRet ret;
   UINT32 mdmFlags = flags & ~(ZBUS_FLAGS_MASK);  // strip off zbus flags

   bcmuLog_notice("Entered: numParamInfos=%d flags=0x%x", numParamInfos, flags);

   ret = bcm_generic_setParameterValues(paramInfoArray, numParamInfos,
                                        mdmFlags);
   if ((ret == BCMRET_SUCCESS) && (flags & ZBUS_FLAG_SAVE_ON_SUCCESS))
   {
      bcmuLog_debug("setParamValues was successful, save config");
      bcm_generic_databaseOp(BCM_DATABASEOP_SAVECONFIG_LOCAL, NULL, NULL);
   }
   return ret;
}


BcmRet zbus_in_getParameterAttributes(const char **fullpathArray,
                                      UINT32        numFullpaths,
                                      UBOOL8        nextLevel,
                                      UINT32        flags,
                                      BcmGenericParamAttr **paramAttrArray,
                                      UINT32       *numParamAttrs)
{
   bcmuLog_notice("Entered: numFullpaths=%d nextLevel=%d flags=0x%x",
                  numFullpaths, (int)nextLevel, flags);

   // When we get this request from the bus, we only want to get the
   // parameter attrs from our local MDM.  Do not trigger the remote_objd
   // to get all the (remote) parameter attrs.
   if ((flags & OGF_LOCAL_MDM_ONLY) == 0)
   {
      bcmuLog_error("Automatically adding OGF_LOCAL_MDM_ONLY to flags");
      bcmuLog_error("first fullpath of %d: %s", numFullpaths, fullpathArray[0]);
      flags |= OGF_LOCAL_MDM_ONLY;
   }

   return (bcm_generic_getParameterAttributes(fullpathArray,
                                              numFullpaths,
                                              nextLevel,
                                              flags,
                                              paramAttrArray,
                                              numParamAttrs));
}


BcmRet zbus_in_setParameterAttributes(BcmGenericParamAttr *paramAttrArray,
                                      SINT32               numParamAttrs,
                                      UINT32               flags)
{
   BcmRet ret;

   bcmuLog_notice("Entered: numParamAttrs=%d flags=0x%x",
                  numParamAttrs, flags);
   ret = bcm_generic_setParameterAttributes(paramAttrArray, numParamAttrs, flags);
   if (flags & ZBUS_FLAG_SAVE_ON_SUCCESS)
   {
      if (ret == BCMRET_SUCCESS)
      {
         bcmuLog_debug("setParamAttr was successful, save config");
         bcm_generic_databaseOp(BCM_DATABASEOP_SAVECONFIG_LOCAL, NULL, NULL);
      }
   }
   return ret;
}


BcmRet zbus_in_addObject(const char *fullpath, UINT32 flags, UINT32 *newInstance)
{
   UINT32 mdmFlags = flags & ~(ZBUS_FLAGS_MASK);
   BcmRet ret;

   bcmuLog_notice("Entered: fullpath=%s, flags=0x%x", fullpath, flags);

   ret = bcm_generic_addObject(fullpath, mdmFlags, newInstance);
   if (flags & ZBUS_FLAG_SAVE_ON_SUCCESS)
   {
      if ((ret == BCMRET_SUCCESS) || (ret == BCMRET_SUCCESS_REBOOT_REQUIRED))
      {
         bcmuLog_debug("addObj was successful, save config");
         bcm_generic_databaseOp(BCM_DATABASEOP_SAVECONFIG_LOCAL, NULL, NULL);
      }
   }
   return ret;
}


BcmRet zbus_in_deleteObject(const char *fullpath, UINT32 flags)
{
   UINT32 mdmFlags = flags & ~(ZBUS_FLAGS_MASK);
   BcmRet ret;

   bcmuLog_notice("Entered: fullpath=%s, flags=0x%x", fullpath, flags);

   ret = bcm_generic_deleteObject(fullpath, mdmFlags);
   if (flags & ZBUS_FLAG_SAVE_ON_SUCCESS)
   {
      if ((ret == BCMRET_SUCCESS) || (ret == BCMRET_SUCCESS_REBOOT_REQUIRED))
      {
         bcmuLog_debug("delObj was successful, save config");
         bcm_generic_databaseOp(BCM_DATABASEOP_SAVECONFIG_LOCAL, NULL, NULL);
      }
   }
   return ret;
}

