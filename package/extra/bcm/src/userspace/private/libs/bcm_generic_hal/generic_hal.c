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

/*!\file generic_hal.c
 * \brief Implementation of the BCM Generic HAL.  This basically wraps up
 *        our existing CMS PHL, OBJ, and MGM interfaces in a generic way.
 */

#include <stddef.h>
#include <string.h>

#include "cms_phl.h"
#include "cms_obj.h"
#include "cms_mgm.h"
#include "cms_mem.h"
#include "cms_image.h"
#include "cms_msg.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"
#include "bcm_generic_hal.h"
#include "bcm_generic_hal_utils.h"   // in libcms_util.h


// TODO: Move to cms_util/bcm_generic_hal_utils?
int isPathNameEndWithInstanceNumberNoDot(const char *fullpath)
{
   int b = FALSE;
   UINT32 len = 0;

   if (fullpath == NULL)
      return FALSE;

   len = strlen(fullpath);
   if (len == 0)
   {
      return FALSE;
   }

   if (isdigit(fullpath[--len]))
   {
      for (len--;
           len != 0 &&
           fullpath[len] != '.' &&
           isdigit(fullpath[len]);
           len--)
         ;

      if (fullpath[len] == '.')
         b = TRUE;
   }

   return b;
}


BcmRet bcm_generic_getParameterNames(const char            *fullpath,
                                     UBOOL8                 nextLevel,
                                     UINT32                 flags,
                                     BcmGenericParamInfo  **paramInfoArray,
                                     UINT32                *numParamInfos)
{
   SINT32 snum = 0;
   BcmRet ret;

   if ((fullpath == NULL) || (paramInfoArray == NULL) || (numParamInfos == NULL))
   {
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: fullpath=%s nextLevel=%d flags=0x%x",
                  fullpath, nextLevel, flags);

   ret = (BcmRet) bcmGeneric_getParameterNamesFlags(fullpath, nextLevel, flags,
                                    paramInfoArray, &snum);
   if (ret == BCMRET_SUCCESS)
   {
      *numParamInfos = (UINT32) snum;
   }

   bcmuLog_debug("Exit: ret=%d num=%d", ret, *numParamInfos);
   return ret;
}


BcmRet bcm_generic_getParameterValues(const char          **fullpathArray,
                                      UINT32                numFullpaths,
                                      UBOOL8                nextLevel,
                                      UINT32                flags,
                                      BcmGenericParamInfo **paramInfoArray,
                                      UINT32               *numParamInfos)
{
   SINT32              numGetVals = 0;
   BcmRet ret;

   bcmuLog_notice("Entered: numFullpaths=%d nextLevel=%d flags=0x%x",
                  numFullpaths, nextLevel, flags);

   ret = (BcmRet) bcmGeneric_getParameterValuesFlags(fullpathArray, (SINT32) numFullpaths,
                                        nextLevel, flags,
                                        paramInfoArray, &numGetVals);

   *numParamInfos = (UINT32) numGetVals;

   bcmuLog_debug("Exit: ret=%d num=%d", ret, *numParamInfos);
   return ret;
}


BcmRet bcm_generic_setParameterValues(BcmGenericParamInfo *paramInfoArray,
                                      UINT32               numParamInfos,
                                      UINT32               flags)
{
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered: numParamInfos=%d flags=0x%x",
                  numParamInfos, flags);

   ret = (BcmRet) bcmGeneric_setParameterValuesFlags(paramInfoArray,
                                              (SINT32) numParamInfos, flags);

   return ret;
}


BcmRet bcm_generic_getParameterAttributes(const char **fullpathArray,
                                          UINT32       numFullpaths,
                                          UBOOL8       nextLevel,
                                          UINT32       flags,
                                          BcmGenericParamAttr **paramAttrArray,
                                          UINT32      *numParamAttrs)
{
   SINT32 numPhlAttrs = 0;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered: numFullpaths=%d nextLevel=%d flags=0x%x",
                  numFullpaths, nextLevel, flags);

   ret = (BcmRet) bcmGeneric_getParameterAttributesFlags(fullpathArray, (SINT32) numFullpaths,
                                       nextLevel, flags,
                                       paramAttrArray, &numPhlAttrs);

   if (ret == BCMRET_SUCCESS)
   {
      *numParamAttrs = (UINT32) numPhlAttrs;
   }

   bcmuLog_debug("Exit: ret=%d num=%d", ret, *numParamAttrs);
   return ret;
}


BcmRet bcm_generic_setParameterAttributes(BcmGenericParamAttr *paramAttrArray,
                                          UINT32               numParamAttrs,
                                          UINT32               flags)
{
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered: num=%d flags=0x%x", numParamAttrs, flags);

   ret = (BcmRet) bcmGeneric_setParameterAttributesFlags(paramAttrArray,
                                            (SINT32) numParamAttrs, flags);

   return ret;
}



void bcm_generic_freeParamInfoArray(BcmGenericParamInfo  **paramInfoArray,
                                    UINT32                 numParamInfos)
{
   cmsUtl_freeParamInfoArray(paramInfoArray, numParamInfos);
}

void bcm_generic_freeParamAttrArray(BcmGenericParamAttr  **paramAttrArray,
                                    UINT32                 numParamAttrs)
{
   cmsUtl_freeParamAttrArray(paramAttrArray, numParamAttrs);
}

void bcm_generic_freeArrayOfStrings(char ***array, UINT32 len)
{
   cmsUtl_freeArrayOfStrings(array, len);
}

void bcm_generic_freeDatabaseOutput(char **output)
{
   if (output == NULL)
      return;

   CMSMEM_FREE_BUF_AND_NULL_PTR(*output);
   return;
}


BcmRet bcm_generic_addObject(const char *fullpath,
                             UINT32 flags,
                             UINT32 *newInstance)
{
   return ( (BcmRet) cmsPhl_addObjInstanceByFullPathFlags(fullpath, flags, newInstance));
}


BcmRet bcm_generic_deleteObject(const char *fullpath,
                                UINT32 flags)
{
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   BcmRet ret;

   if (fullpath == NULL)
   {
      return ((BcmRet)CMSRET_INVALID_PARAM_NAME);
   }

   ret = (BcmRet) cmsMdm_fullPathToPathDescriptor(fullpath, &pathDesc);
   if (ret != BCMRET_SUCCESS)
   {
      return BCMRET_INVALID_PARAM_NAME;
   }

   ret = (BcmRet) cmsPhl_delObjInstanceFlags(&pathDesc, flags);
   return ret;
}


// Create a new instance at Device.DeviceInfo.VendorConfigFile.{i}.
// On successful return, output contains the fullpath to the instance that
// was created.
static BcmRet processAddVendorConfig(char **output)
{
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if (output == NULL)
   {
      bcmuLog_error("NULL output arg, must be provided");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered:");

   if (cmsMdm_isCmsClassic() || cmsMdm_isBdkDevinfo())
   {
      MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

      // This is a local MDM operation.
      // Temporarily add myself to the fullWriteAccessEid array (because
      // VendorConfigFile is a read-only multi-instance table), add the
      // instance, and remove myself from the fullWriteAccessEid array.
      cmsMdm_addEidToFullWriteAccessArray(myEid);

      pathDesc.oid = MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE;
      ret = (BcmRet) cmsPhl_addObjInstance(&pathDesc);

      cmsMdm_deleteEidFromFullWriteAccessArray(myEid);

      // put the fullpath of the newly created instance in output.
      if (ret == BCMRET_SUCCESS)
      {
         ret = (BcmRet) cmsMdm_pathDescriptorToFullPath(&pathDesc, output);
         if(ret != BCMRET_SUCCESS)
            bcmuLog_error("cmsMdm_pathDescriptorToFullPath returns error");
         else
            bcmuLog_debug("created local instance at %s", *output);
      }
   }
   else if (cmsMdm_isRemoteCapable())
   {
      CmsMsgHeader *replyMsg = NULL;
      CmsMsgHeader msg = EMPTY_MSG_HEADER;

      // fill in header.  ADD_VENDOR_CONFIG has no input args, but returns
      // the fullpath that was created.
      msg.type = CMS_MSG_REMOTE_OBJ_ADD_VENDOR_CONFIG;
      msg.src = cmsMsg_getHandleEid(msgHandle);
      msg.dst = EID_REMOTE_OBJD;
      msg.flags_request = 1;
      // send request to remote_objd (cannot call ZBus directly here);
      ret = (BcmRet) cmsMsg_sendAndGetReplyBuf(msgHandle, &msg, &replyMsg);
      if (ret == BCMRET_SUCCESS)
      {
         bcmuLog_debug("created remote instance %s (len=%d)",
                       (char *)(replyMsg+1), replyMsg->dataLength);
         *output = cmsMem_strdup((char *)(replyMsg+1));
         CMSMEM_FREE_BUF_AND_NULL_PTR(replyMsg);
      }
   }
   else
   {
      bcmuLog_error("This command cannot be run in components that do not have remote_objd");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_debug("Exit: ret=%d output=%s", ret, *output);
   return ret;
}

// Delete the specified VendorConfigFile (fullpath).
static BcmRet processDelVendorConfig(const char *input)
{
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsEntityId myEid = cmsMsg_getHandleGenericEid(msgHandle);

   if (input == NULL)
   {
      bcmuLog_error("NULL input arg, must be provided");
      return BCMRET_INVALID_ARGUMENTS;
   }
   if (strlen(input) >= CMS_MAX_FULLPATH_LENGTH)
   {
      bcmuLog_error("input fullpath (%s) too long, max=%d",
                    input, CMS_MAX_FULLPATH_LENGTH);
      return BCMRET_INVALID_ARGUMENTS;
   }
   bcmuLog_notice("Entered: delete %s", input);

   if (cmsMdm_isCmsClassic() || cmsMdm_isBdkDevinfo())
   {
      MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;

      // See same comments in the add case above
      ret = (BcmRet) cmsMdm_fullPathToPathDescriptor(input, &pathDesc);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Invalid path %s", input);
         return BCMRET_INVALID_ARGUMENTS;
      }

      cmsMdm_addEidToFullWriteAccessArray(myEid);

      ret = (BcmRet) cmsPhl_delObjInstance(&pathDesc);

      cmsMdm_deleteEidFromFullWriteAccessArray(myEid);
   }
   else if (cmsMdm_isRemoteCapable())
   {
      char msgBuf[sizeof(CmsMsgHeader) + CMS_MAX_FULLPATH_LENGTH] = {0};
      CmsMsgHeader *msg = (CmsMsgHeader *) msgBuf;

      // fill in header.  ADD_VENDOR_CONFIG has no input args, but returns
      // the fullpath that was created.
      msg->type = CMS_MSG_REMOTE_OBJ_DEL_VENDOR_CONFIG;
      msg->src = cmsMsg_getHandleEid(msgHandle);
      msg->dst = EID_REMOTE_OBJD;
      msg->flags_request = 1;
      msg->dataLength = strlen(input)+1;
      strcpy((char *)(msg+1), input);
      bcmuLog_debug("Sending delete %s (len=%d) to remote_objd",
                    (char *)(msg+1), msg->dataLength);
      ret = (BcmRet) cmsMsg_sendAndGetReply(msgHandle, msg);
   }
   else
   {
      bcmuLog_error("This command cannot be run in components that do not have remote_objd");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_debug("Exit: ret=%d", ret);
   return ret;
}


// Some functions needed from MDM (not part of official API, but maybe could be)
UINT32 local_getNumberOfParamValueChanges(void);
void local_clearAllParamValueChanges(void);
CmsRet cmsPhl_getChangedParamsLocal(char ***arrayParams, UINT32 *numParams);

UINT32 local_getAltNumberOfParamValueChanges(void);

BcmRet bcm_generic_databaseOp(const char *op, const char *input,
                              char **output)
{
   UINT32 flags = 0;
   BcmRet ret = BCMRET_SUCCESS;

   bcmuLog_notice("Entered: op=%s", op);

   if (!strcasecmp(op, BCM_DATABASEOP_SAVECONFIG))
   {
      ret = (BcmRet) cmsMgm_saveConfigToFlashEx(flags);
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_SAVECONFIG_LOCAL))
   {
      flags |= OGF_LOCAL_MDM_ONLY;
      ret = (BcmRet) cmsMgm_saveConfigToFlashEx(flags);
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_INVALIDATECONFIG))
   {
      cmsMgm_invalidateConfigFlashEx(flags);
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_INVALIDATECONFIG_LOCAL))
   {
      flags |= OGF_LOCAL_MDM_ONLY;
      cmsMgm_invalidateConfigFlashEx(flags);
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_READCONFIG) ||
            !strcasecmp(op, BCM_DATABASEOP_READCONFIG_LOCAL))
   {
      char *cfgBuf = NULL;

      if (!strcasecmp(op, BCM_DATABASEOP_READCONFIG_LOCAL))
      {
         flags |= OGF_LOCAL_MDM_ONLY;
      }

      if (output == NULL)
      {
         bcmuLog_error("output buf must be provided!");
         ret = (BcmRet) CMSRET_INVALID_ARGUMENTS;
      }
      else
      {
         ret = (BcmRet) cmsMgm_readConfigFlashToBufEx(&cfgBuf, flags);
         if (ret == BCMRET_SUCCESS)
         {
            *output=cfgBuf;
         }
      }
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_READMDM) ||
            !strcasecmp(op, BCM_DATABASEOP_READMDM_LOCAL))
   {
      char *mdmBuf = NULL;

      if (!strcasecmp(op, BCM_DATABASEOP_READMDM_LOCAL))
      {
         flags |= OGF_LOCAL_MDM_ONLY;
      }

      if (output == NULL)
      {
         bcmuLog_error("output buf must be provided!");
         ret = (BcmRet) CMSRET_INVALID_ARGUMENTS;
      }
      else
      {
         ret = (BcmRet) cmsMgm_writeMdmToBufEx(&mdmBuf, flags);
         if (ret == BCMRET_SUCCESS)
         {
            *output = mdmBuf;
         }
      }
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_VALIDATECONFIG))
   {
      // Currently, VALIDATECONFIG at this level is always a local operation.
      // When a high level app recieves a buffer containing multiple BDK distributed
      // config files, it sends it to smd, which sends it to remote_objd
      // using CMS_MSG_VALIDATE_CONFIG_FILE.  Remote_objd will break up the
      // config file and validate with the individual components.
      // TODO: for consistency, maybe we should support remote validation at this level for tr69c?

      /* input contains the config buffer to validate */
      if (input != NULL)
      {
         ret = (BcmRet) cmsMgm_validateConfigBuf(input,strlen(input)+1);
      }
      else
      {
         bcmuLog_error("No buf provided!");
         ret = BCMRET_INVALID_ARGUMENTS;
      }
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_WRITECONFIG))
   {
      /* input contains the config buffer to write to flash */
      if (input != NULL)
      {
         // Validate the config file before writing.
         ret = (BcmRet) cmsMgm_validateConfigBuf(input, strlen(input)+1);
         if (ret == BCMRET_SUCCESS)
         {
            ret = (BcmRet) cmsMgm_writeValidatedBufToConfigFlash(input, strlen(input)+1);
         }
         else
         {
            bcmuLog_error("Invalid config file!");
         }
      }
      else
      {
         bcmuLog_error("No buf provided!");
         ret = BCMRET_INVALID_ARGUMENTS;
      }
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_CLEAR_ALL_PARAM_VALUE_CHANGES_LOCAL))
   {
      local_clearAllParamValueChanges();
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_CLEAR_ALL_ALT_PARAM_VALUE_CHANGES_LOCAL))
   {
      ret = BCMRET_SUCCESS; //do nothing
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_GET_NUM_VALUE_CHANGES_LOCAL) ||
            !strcasecmp(op, BCM_DATABASEOP_GET_ALT_NUM_VALUE_CHANGES_LOCAL))
   {
      UINT32 n,size;
      char nStr[64]={0};
      char *buf;
      
      if (!strcasecmp(op, BCM_DATABASEOP_GET_NUM_VALUE_CHANGES_LOCAL))
         n = local_getNumberOfParamValueChanges();
      else
         n = local_getAltNumberOfParamValueChanges();
         
      size = sprintf(nStr,"%d",n);
      buf = cmsMem_alloc(size+1, ALLOC_ZEROIZE);
      if (buf == NULL)
      {
         ret = BCMRET_RESOURCE_EXCEEDED;
      }
      else
      {
         strcpy(buf,nStr);
         *output = buf;
      }
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_GET_CHANGED_PARAMS_LOCAL) ||
            !strcasecmp(op, BCM_DATABASEOP_GET_ALT_CHANGED_PARAMS_LOCAL))
   {
      char **array=NULL;
      UINT32 num=0;
      UINT32 i=0;
      UINT32 lenNeeded=0;
      char *buf = NULL;
      UINT32 offset=0;

      if (!strcasecmp(op, BCM_DATABASEOP_GET_CHANGED_PARAMS_LOCAL))
         ret = (BcmRet) cmsPhl_getChangedParamsLocal(&array,&num);
      else
         ret = (BcmRet) cmsPhl_getAndClearAltChangedParamsLocal(&array,&num);

      if (num)
      {
         for (i=0; i<num; i++)
         {
            /* concat all fullpath,type, value tuples into a single string: 
             * <_DELIM_>fullpath,type,value</_DELIM_><_DELIM_>fullpath,type,value</_DELIM_>
             */
            lenNeeded += BCM_DATABASEOP_DELIM_BEGIN_LEN +
                         strlen(array[i]) +
                         BCM_DATABASEOP_DELIM_END_LEN;

         }
         /* +1 for null termination */
         buf = cmsMem_alloc(lenNeeded+1,ALLOC_ZEROIZE);
         if (buf != NULL)
         {
            for (i=0; i<num; i++)
            {
               offset += sprintf(buf+offset, "%s%s%s",
                                 BCM_DATABASEOP_DELIM_BEGIN, array[i],
                                 BCM_DATABASEOP_DELIM_END);
            }
         }
         else
         {
            bcmuLog_error("Could not allocate %d bytes", lenNeeded);
            ret = BCMRET_RESOURCE_EXCEEDED;
         }
         *output=buf;
         /* free array from phl */
         bcm_generic_freeArrayOfStrings(&array, num);
      }
      else
      {
         *output = NULL;
      }
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_CLEAR_STATISTICS) ||
            !strcasecmp(op, BCM_DATABASEOP_CLEAR_STATISTICS_LOCAL))
   {
      if (!strcasecmp(op, BCM_DATABASEOP_CLEAR_STATISTICS_LOCAL))
      {
         flags |= OGF_LOCAL_MDM_ONLY;
      }

      ret = (BcmRet) cmsPhl_clearStatisticsByFullPathFlags(input, flags);
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_ADD_VENDOR_CONFIG))
   {
      ret = processAddVendorConfig(output);
   }
   else if (!strcasecmp(op, BCM_DATABASEOP_DEL_VENDOR_CONFIG))
   {
      ret = processDelVendorConfig(input);
   }
   else
   {
      ret = BCMRET_INVALID_ARGUMENTS;
      bcmuLog_error("Unsupported op %s", op);
   }

   bcmuLog_debug("Exit: ret=%d", ret);
   return ret;
}

