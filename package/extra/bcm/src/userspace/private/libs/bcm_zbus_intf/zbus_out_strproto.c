/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
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


/*!\file zbus_out_strproto.c
 * \brief Wrapper functions for outbound MDM access functions.
 *        The caller of these functions does not need to link with libcms_core
 *        or directly access the MDM, therefore, these functions can be in
 *        bcm_zbus_intf and does not need to be in bcm_zbus_mdm.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "cms_mem.h"



BcmRet zbus_out_getParameterNames(const ZbusAddr *destAddr,
                const char *fullpath, UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   BcmRet ret;

   if (destAddr == NULL || fullpath == NULL)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }
   if (paramInfoArray == NULL || numParamInfos == NULL)
   {
      bcmuLog_error("NULL args for return values");
      return BCMRET_INVALID_ARGUMENTS;
   }

   // zero out return vars in case we hit error and return early
   *paramInfoArray = NULL;
   *numParamInfos = 0;

   bcmuLog_notice("Entered: destAddr=%s fullpath=%s flags=0x%x",
                  destAddr->compName, fullpath, flags);

   ret = bus_out_getParameterNames(destAddr, fullpath, nextLevel, flags,
                                   paramInfoArray, numParamInfos);

   bcmuLog_debug("ret=%d numParamInfos=%d", ret, *numParamInfos);
   return ret;
}


BcmRet zbus_out_getParameterValues(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   BcmRet ret;

   if (destAddr == NULL || fullpathArray == NULL || numEntries == 0)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }
   if (paramInfoArray == NULL || numParamInfos == NULL)
   {
      bcmuLog_error("NULL args for return values");
      return BCMRET_INVALID_ARGUMENTS;
   }

   // zero out return vars in case we hit error and return early
   *paramInfoArray = NULL;
   *numParamInfos = 0;

   bcmuLog_notice("Entered: destAddr=%s numEntries=%d fullpath[0]=%s flags=0x%x",
                  destAddr->compName, numEntries, fullpathArray[0], flags);

   ret = bus_out_getParameterValues(destAddr,
                                    fullpathArray, numEntries,
                                    nextLevel, flags,
                                    paramInfoArray, numParamInfos);

   bcmuLog_debug("ret=%d numParamInfos=%d", ret, *numParamInfos);
   return ret;
}


BcmRet zbus_out_setParameterValues(const ZbusAddr *destAddr,
                BcmGenericParamInfo *paramInfoArray, UINT32 numParamInfos,
                UINT32 flags)
{
   UINT32 i;
   BcmRet ret;

   if (destAddr == NULL || paramInfoArray == NULL || numParamInfos == 0)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: destAddr=%s numEntries=%d paramInfo[0]=%s flags=0x%x",
                  destAddr->compName, numParamInfos,
                  paramInfoArray[0].fullpath, flags);

   for (i=0; i < numParamInfos; i++)
   {
      if (paramInfoArray[i].fullpath == NULL)
      {
         bcmuLog_error("NULL fullpath detected at [%d]", i);
         return ((BcmRet) CMSRET_INVALID_ARGUMENTS);
      }

      // NULL value ptrs are not allowed by string based API's.
      if (paramInfoArray[i].value == NULL)
      {
         bcmuLog_error("NULL value detected at [%d] %s",
                       i, paramInfoArray[i].fullpath);
         return ((BcmRet) CMSRET_INVALID_ARGUMENTS);
      }

      if (paramInfoArray[i].type == NULL)
      {
         // In theory, we don't need to require the user to specify the type,
         // we can just use whatever type the MDM thinks it is.  But type
         // still needs to be at least an empty string (and for now, we
         // require the correct type to be specified).
         bcmuLog_error("NULL type detected at [%d] %s",
                       i, paramInfoArray[i].fullpath);
         return ((BcmRet) CMSRET_INVALID_ARGUMENTS);
      }
   }

   ret = bus_out_setParameterValues(destAddr,
                                    paramInfoArray, numParamInfos,
                                    flags);

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet zbus_out_getParameterAttributes(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamAttr **paramAttrArray, UINT32 *numParamAttrs)
{
   BcmRet ret;

   if (destAddr == NULL || fullpathArray == NULL || numEntries == 0)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }
   if (paramAttrArray == NULL || numParamAttrs == NULL)
   {
      bcmuLog_error("NULL output arg");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: destAddr=%s numEntries=%d flags=0x%x",
                  destAddr->compName, numEntries, flags);

   // zero out return value in case we hit error and return early
   *paramAttrArray = NULL;
   *numParamAttrs = 0;

   ret = bus_out_getParameterAttributes(destAddr,
                                        fullpathArray, numEntries,
                                        nextLevel, flags,
                                        paramAttrArray, numParamAttrs);

   bcmuLog_debug("ret=%d numParamAttrs=%d", ret, *numParamAttrs);
   return ret;
}


BcmRet zbus_out_setParameterAttributes(const ZbusAddr *destAddr,
                BcmGenericParamAttr *paramAttrArray, UINT32 numParamAttrs,
                UINT32 flags)
{
   BcmRet ret;

   if (destAddr == NULL || paramAttrArray == NULL || numParamAttrs == 0)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: destAddr=%s numParamAttrs=%d flags=0x%x",
                  destAddr->compName, numParamAttrs, flags);

   ret = bus_out_setParameterAttributes(destAddr,
                                        paramAttrArray, numParamAttrs,
                                        flags);

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet zbus_out_addObject(const ZbusAddr *destAddr,
                          const char *fullpath, UINT32 flags,
                          UINT32 *instanceNumber)
{
   BcmRet ret;

   if (destAddr == NULL || fullpath == NULL || instanceNumber == NULL)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }

   // zero out return value in case we hit error and return early
   *instanceNumber = 0;

   bcmuLog_notice("Entered: destAddr=%s fullpath=%s flags=0x%x",
                  destAddr->compName, fullpath, flags);

   ret = bus_out_addObject(destAddr, fullpath, flags, instanceNumber);

   bcmuLog_debug("ret=%d instanceNumber=%d", ret, *instanceNumber);
   return ret;
}


BcmRet zbus_out_deleteObject(const ZbusAddr *destAddr,
                             const char *fullpath, UINT32 flags)
{
   BcmRet ret;

   if (destAddr == NULL || fullpath == NULL)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: destAddr=%s fullpath=%s flags=0x%x",
                  destAddr->compName, fullpath, flags);

   ret = bus_out_deleteObject(destAddr, fullpath, flags);

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet zbus_out_getObject(const ZbusAddr *destAddr,
                const char *fullpath, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   BcmRet ret;
   const char *fullpathArray[] = {fullpath};  // array containing a single fullpath

   if (destAddr == NULL || fullpath == NULL)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }
   if (paramInfoArray == NULL || numParamInfos == NULL)
   {
      bcmuLog_error("NULL args for return values");
      return BCMRET_INVALID_ARGUMENTS;
   }

   // zero out return vars in case we hit error and return early
   *paramInfoArray = NULL;
   *numParamInfos = 0;

   bcmuLog_notice("Entered: destAddr=%s fullpath=%s flags=0x%x",
                  destAddr->compName, fullpath, flags);

   ret = bus_out_getParameterValues(destAddr,
                                    fullpathArray, 1,
                                    TRUE, flags,
                                    paramInfoArray, numParamInfos);

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet zbus_out_getNextObject(const ZbusAddr *destAddr,
                const char *fullpath, const char *limitSubtree, UINT32 flags,
                char **nextFullpath,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   BcmRet ret;
   // If user did not pass in limitSubtree, assume it is Device. (no limit).
   const char *myLimitSubtree = (limitSubtree == NULL) ?
                                "Device." : limitSubtree;

   if (destAddr == NULL || fullpath == NULL)
   {
      bcmuLog_error("NULL input arg");
      return BCMRET_INVALID_ARGUMENTS;
   }
   if (nextFullpath == NULL || paramInfoArray == NULL || numParamInfos == NULL)
   {
      bcmuLog_error("NULL args for return values");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: destAddr=%s fullpath=%s myLimitSubtree=%s flags=%d",
                  destAddr->compName, fullpath, myLimitSubtree, flags);

   // zero out return vars in case we hit error and return early
   *nextFullpath = NULL;
   *paramInfoArray = NULL;
   *numParamInfos = 0;

   ret = bus_out_getNextObject(destAddr,
                               fullpath, myLimitSubtree, flags,
                               nextFullpath,
                               paramInfoArray, numParamInfos);

   if (ret == BCMRET_SUCCESS)
   {
      bcmuLog_debug("ret=%d (nextFullpath=%s) numParamInfos=%d",
                    ret, *nextFullpath, *numParamInfos);
   }
   else
   {
      bcmuLog_debug("ret=%d", ret);
   }
   return ret;
}


