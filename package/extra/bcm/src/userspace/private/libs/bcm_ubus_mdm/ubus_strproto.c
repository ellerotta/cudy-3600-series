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

#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "cms_lck.h"
#include "cms_util.h"
#include "cms_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "bcm_zbus_mdm.h"
#include "bcm_ubus_intf.h"

// this file uses defines associated with the BCM Generic HAL,
// but does not use BCM Generic HAL directly.
#include "bcm_generic_hal_defs.h"
#include "bcm_generic_hal_utils.h"

#include "libubus.h"
#include <libubox/blobmsg_json.h>

/*!\file ubus_strproto.c
 * \brief U-Bus handlers for inbound generic string protocol methods.  The
 *        functions in this file just unwraps the UBus specific stuff off
 *        of the actual parameters and calls the zbus strproto functions in
 *        bcm_zbus_mdm to process them.  If there is return data, these
 *        functions wrap the UBus stuff back on the data.
 *
 */

extern const struct blobmsg_policy get_param_name_in_policy[3];
extern const struct blobmsg_policy get_param_value_in_policy[3];
extern const struct blobmsg_policy set_param_value_in_policy[2];
extern const struct blobmsg_policy get_param_attribute_in_policy[3];
extern const struct blobmsg_policy set_param_attribute_in_policy[2];
extern const struct blobmsg_policy adddel_object_in_policy[2];
extern const struct blobmsg_policy get_next_object_in_policy[3];

static const struct blobmsg_policy setParamValue_innerPolicy[3] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "type", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "value", .type = BLOBMSG_TYPE_STRING},
};

/*innner data type: table: { "fullpath": string, "setAccess": boolean, "access":int16,
* "setNotif":boolean, "notif": int16,
* "setAltNotif":boolean, "altNotif": int16, "clearAltValue": boolean}
*/
static const struct blobmsg_policy setParamAttr_innerPolicy[8] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "setAccess", .type = BLOBMSG_TYPE_BOOL},
   [2] = { .name = "access", .type = BLOBMSG_TYPE_INT16},
   [3] = { .name = "setNotif", .type = BLOBMSG_TYPE_BOOL},
   [4] = { .name = "notif", .type = BLOBMSG_TYPE_INT16},
   [5] = { .name = "setAltNotif", .type = BLOBMSG_TYPE_BOOL},
   [6] = { .name = "altNotif", .type = BLOBMSG_TYPE_INT16},
   [7] = { .name = "clearAltValue", .type = BLOBMSG_TYPE_BOOL},
};

/* ---- Private Functions -------------------------------------------------- */

static bool is_pathNameEndwithInstNbrNodot(const char *fullpath)
{
   bool ret = FALSE;
   uint32_t len = 0;

   len = strlen(fullpath);
   if (len == 0)
   {
      printf("NULL or zero length fullpath.\n");
      return ret;
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
         ret = TRUE;
   }

   return ret;
}

static char *getErrorResp(BcmRet ret)
{
   char *resp;
   switch(ret)
   {
      case BCMRET_INVALID_ARGUMENTS:
         resp = "invalid arguments";
         break;
      case BCMRET_RESOURCE_EXCEEDED:
         resp = "not enough memory";
         break;
      case BCMRET_INVALID_PARAM_NAME:
         resp = "invalid parameter name";
         break;
      case CMSRET_SUCCESS_REBOOT_REQUIRED:
         resp = "need reboot";
         break;
      default:
         resp = "unknown error";
         break;
   }

   return resp;
}

/*
* Convert "fullpath", "type", "value", "profile", "writable" and "isPassword" in BcmGenericParamInfo into a table of blobmsg.
* Note this must be a BcmGenericParamInfo as returned by getParameterValues with the value field filled in.
* It is up to the caller to decide the the table's final position in blob message. if the caller opens an
* arry in blob buffer, before calling this method, the table will be one the entry of the array; otherwise,
* the table will be the first level of nested blob attribute in blob buffer
*/
static void fillParameterInfoIntoTable(const BcmGenericParamInfo *paramInfo,
                                        struct blob_buf *blobBuf)
{
   void * blobTable = blobmsg_open_table(blobBuf, NULL);

   blobmsg_add_string(blobBuf, "fullpath", paramInfo->fullpath);
   blobmsg_add_string(blobBuf, "type", paramInfo->type);
   if (paramInfo->value == NULL)
   {
      bcmuLog_error("paramInfo of %s has null value!", paramInfo->fullpath);
   }
   else
   {
      blobmsg_add_string(blobBuf, "value", paramInfo->value);
   }
   blobmsg_add_string(blobBuf, "profile", paramInfo->profile);
   blobmsg_add_u8(blobBuf, "writable", paramInfo->writable);
   blobmsg_add_u8(blobBuf, "isPassword", paramInfo->isPassword);

   blobmsg_close_table(blobBuf, blobTable);
}

/**
* APIs to handle incoming ubus method calls
* Parameters
*  @ctx: struct ubus_context *
*  @req: struct ubus_request_data *
*  @method: pointer of method name
*  @msg: struct blob_attr *
*/
void ubus_in_setParameterValues(const void *ctx, const void *req, void *method, void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(set_param_value_in_policy)] = {NULL};

   /*argsTable[0] is an array of argument table, each table carries info of a paramter, looks like
   * { "fullpath": "String", "type": "String", "value": "String" }.
   * innerArgsTable is used to parse argsTable[0]
   */
   struct blob_attr *innerArgsTable[ARRAY_SIZE(setParamValue_innerPolicy)] = {};
   uint32_t numParamValues = 0;
   uint32_t i = 0;
   uint32_t flags;
   BcmGenericParamInfo *paramInfoArray = NULL;
   struct blob_attr *attr;
   unsigned int len = 0;
   void *blobArray;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char tmpResp[BUFLEN_128] = {0};
   char *resp = NULL;

   bcmuLog_debug("Entered");

   /*
   * Prepare the return values in case we return early. Here inserting an array.
   * if the method call returns early, an arry is created with 0 entries so that we
   * can make sure that number of arguments returned matches the number of entries
   * of output policy
   */
   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }
   /*
   * The array name matches the key of the first field of struct
   * blobmsg_policy set_param_value_out_policy
   */
   blobArray = blobmsg_open_array(&blobBuffer, "fullpath_error_array");

   /* parse and verify input arguments */
   if(!parseAndVerifyParameters(set_param_value_in_policy,
            ARRAY_SIZE(set_param_value_in_policy),
            (struct blob_attr *)msg,
            argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_SETPARAM;
   }

   /* get number of parameter tables in the array */
   numParamValues = getNumberOfArrayEntries(argsTable[0]);
   if(0 == numParamValues)
   {
      bcmuLog_error("%s is called with 0 parameters", (char *)method);
      goto EXIT_SETPARAM;
   }

   /* allocate memory for SetParamValue array */
   paramInfoArray = (BcmGenericParamInfo *) cmsMem_alloc(sizeof(BcmGenericParamInfo) * numParamValues, ALLOC_ZEROIZE);
   if(NULL == paramInfoArray)
   {
      bcmuLog_error("not enough memory for array of %d BcmGenericParamInfos", numParamValues);
      ret = BCMRET_RESOURCE_EXCEEDED;
      goto EXIT_SETPARAM;
   }

   /* iterate the parameter table array to get {fullpath, type, value} of each parameter */
   len = blobmsg_data_len(argsTable[0]);
   __blob_for_each_attr(attr, blobmsg_data(argsTable[0]), len)
   {
      char *fullpath, *type, *value;

      if(BLOBMSG_TYPE_TABLE != blobmsg_type(attr))
      {
         bcmuLog_error("invalid parameters(inner parameter not a tabe)");
         goto EXIT_SETPARAM;
      }

      blobmsg_parse(setParamValue_innerPolicy, ARRAY_SIZE(setParamValue_innerPolicy),
                  innerArgsTable,
                  blobmsg_data(attr),
                  blobmsg_data_len(attr));
      if(!innerArgsTable[0] || !innerArgsTable[1] || !innerArgsTable[2] )
      {
         bcmuLog_error("missing fullpath, type or value");
         goto EXIT_SETPARAM;
      }

      fullpath = blobmsg_get_string(innerArgsTable[0]);
      type = blobmsg_get_string(innerArgsTable[1]);
      value = blobmsg_get_string(innerArgsTable[2]);
      bcmuLog_debug("[%d] fullpath: %s type: %s value: %s", i, fullpath, type, value);

      paramInfoArray[i].fullpath = cmsMem_strdup(fullpath);
      paramInfoArray[i].type = cmsMem_strdup(type);
      paramInfoArray[i].value = cmsMem_strdup(value);
      i++;
   }

   flags = blobmsg_get_u32(argsTable[1]);

   /*
   * Call Z-Bus generic method.
   */
   ret = zbus_in_setParameterValues(paramInfoArray, numParamValues, flags);
   if(ret == BCMRET_SUCCESS)
   {

      snprintf(tmpResp, sizeof(tmpResp), "%s is performed completely", (char *)method);
      resp = tmpResp;
   }
   else
   {
      /* fill the fullpath_error_array with {fullpath, status} tables */
      for(i = 0; i < numParamValues; i++)
      {
         if (paramInfoArray[i].errorCode != BCMRET_SUCCESS)
         {
            void *blobTable;

            /*keys used here match comments for innner data type: set_param_value_out_policy */
            blobTable = blobmsg_open_table(&blobBuffer, NULL);
            blobmsg_add_string(&blobBuffer, "fullpath", paramInfoArray[i].fullpath);
            blobmsg_add_u32(&blobBuffer, "status", paramInfoArray[i].errorCode);
            blobmsg_close_table(&blobBuffer, blobTable);
         }
      }

      snprintf(tmpResp, sizeof(tmpResp), "Errors are occured during %s", (char *)method);
      resp = tmpResp;
   }

EXIT_SETPARAM:
   blobmsg_close_array(&blobBuffer, blobArray);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);

   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);
   cmsUtl_freeParamInfoArray(&paramInfoArray, numParamValues);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}

void ubus_in_getParameterValues(const void *ctx, const void *req, void *method, void *msg)
{
   struct blob_attr *argsTable[ARRAY_SIZE(get_param_value_in_policy)] = {};
   char *fullpath;
   uint32_t numFullpaths = 0;
   uint8_t nextLevel;
   uint32_t flags;
   char               **fullpathArray=NULL;
   BcmGenericParamInfo *paramInfoArray=NULL;
   UINT32               numParamInfos=0;
   UINT32 i = 0;
   void *attr;
   unsigned int len = 0;
   struct blob_buf blobBuffer = {};
   void *blobArray;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char tmpResp[BUFLEN_128] = {0};
   char *resp = NULL;

   bcmuLog_debug("Entered");

   /*
   * Prepare the return values in case we return early. Here inserting an array
   * if the method call returns early, an arry is created with 0 entries so that we
   * can make sure that number of arguments returned matches the number of entries
   * of output policy
   */
   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }
   /*
   * The array name matches the key of the first field of struct
   * blobmsg_policy get_param_value_out_policy
   */
   blobArray = blobmsg_open_array(&blobBuffer,
               "fullpath_type_value_profile_writable_isPassword_array");

   /* parse and verify input arguments
   * example: '{"fullpath_array":["Device.DeviceInfo."], "nextLevel":false,"flags":0}'
   */
   if(!parseAndVerifyParameters(get_param_value_in_policy,
               ARRAY_SIZE(get_param_value_in_policy),
               (struct blob_attr *)msg,
               argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_GETPARAM;
   }

   /* Get number of fullpaths in the request (fullpath_array) */
   numFullpaths = getNumberOfArrayEntries(argsTable[0]);
   if(0 == numFullpaths)
   {
      bcmuLog_error("%s is called with 0 fullpath", (char *)method);
      goto EXIT_GETPARAM;
   }

   nextLevel = blobmsg_get_bool(argsTable[1]);
   flags = blobmsg_get_u32(argsTable[2]);

   // Allocate array of char * to hold the fullpaths
   fullpathArray = cmsMem_alloc(numFullpaths * sizeof(char *), ALLOC_ZEROIZE);
   if(NULL == fullpathArray)
   {
      bcmuLog_error("not enough memory to hold array of %d char *", numFullpaths);
      ret = BCMRET_RESOURCE_EXCEEDED;
      goto EXIT_GETPARAM;
   }

   /* iterate the fullpath array to get fullpath of each entry */
   len = blobmsg_data_len(argsTable[0]);
   __blob_for_each_attr(attr, blobmsg_data(argsTable[0]), len)
   {
      fullpath = blobmsg_get_string(attr);
        /*
        * object name of object instance should have
        * Dot (.) at the end, otherwise this function
        * should return CMSRET_INVALID_PARAM_NAME.
        * Since cmsMdm_fullPathToPathDescriptor accepts
        * full path that does not have Dot (.) at the end,
        * isPathNameEndWithInstanceNumberNoDot() is used
        * to validate this format.
        * TODO: this check should be done at the TR69 level since it
        * is a TR69 requirement.  Other callers might not care.
        */
         if (is_pathNameEndwithInstNbrNodot(fullpath) == TRUE)
         {
            bcmuLog_error("invalid parameter name: %s", fullpath);
            ret = BCMRET_INVALID_PARAM_NAME;
            goto EXIT_GETPARAM;
         }

      fullpathArray[i] = cmsMem_strdup(fullpath);
      i++;
   }

   /*
   * Call Z-Bus generic method.
   */
   ret = zbus_in_getParameterValues((const char **) fullpathArray, numFullpaths,
                                    nextLevel, flags,
                                    &paramInfoArray, &numParamInfos);

   if (ret == BCMRET_SUCCESS)
   {
      for (i = 0; i < numParamInfos; i++)
       {
           fillParameterInfoIntoTable(&(paramInfoArray[i]), &blobBuffer);
       }

      snprintf(tmpResp, sizeof(tmpResp), "%s: returning %d parameters",
               (char *)method, numParamInfos);
      resp = tmpResp;
      cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
   }
   else
   {
      bcmuLog_error("zbus_in_getParameterValues returned %d", ret);
   }

EXIT_GETPARAM:
   blobmsg_close_array(&blobBuffer, blobArray);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);
   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req,
            blobBuffer.head);

   cmsUtl_freeArrayOfStrings(&fullpathArray, numFullpaths);
   blob_buf_free(&blobBuffer);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}


void ubus_in_getParameterNames(const void *ctx, const void *req, void *method, void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(get_param_name_in_policy)] = {};
   char *fullpath = NULL;
   MdmPathDescriptor strPathDesc = EMPTY_PATH_DESCRIPTOR;
   uint8_t nextLevel;
   uint32_t flags;
   BcmGenericParamInfo *info = NULL;
   BcmGenericParamInfo *paramInfoArray = NULL;
   UINT32 numParamInfos=0;
   void *blobArray;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char tmpResp[BUFLEN_128] = {0};
   char *resp = NULL;

   bcmuLog_debug("Entered");

   /*
   * Prepare the return values in case we return early. Here inserting an array.
   * if the method call returns early, an arry is created with 0 entries so that we
   * can make sure that number of arguments returned matches the number of entries
   * of output policy
   */
   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }
   /*
   * The array name matches the key of the first field of struct
   * blobmsg_policy get_param_name_out_policy
   */
   blobArray = blobmsg_open_array(&blobBuffer, "fullpath_type_profile_writable_isPassword_array");

   /* parse and verify input arguments */
   if(!parseAndVerifyParameters(get_param_name_in_policy,
               ARRAY_SIZE(get_param_name_in_policy),
               (struct blob_attr *)msg,
               argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_GETPARAMNAMES;
   }

   fullpath = blobmsg_get_string(argsTable[0]);
   nextLevel = blobmsg_get_bool(argsTable[1]);
   flags = blobmsg_get_u32(argsTable[2]);


   ret = (BcmRet)cmsMdm_fullPathToPathDescriptor(fullpath, &strPathDesc);
   if(ret == BCMRET_SUCCESS)
   {
      /*
      * Call Z-Bus generic method.
      */
      ret = zbus_in_getParameterNames(fullpath, nextLevel, flags,
                                      &paramInfoArray, &numParamInfos);
      if(ret == BCMRET_SUCCESS)
      {
         UINT32 p = 0;

         // insert paramNameArray elements into blob table.
         for (p=0; p < numParamInfos; p++)
         {
            void *blobTable = NULL;
            info = (BcmGenericParamInfo *) &(paramInfoArray[p]);
            /*innner data type: table:
            * { "fullpath": string, "type": string,"profile": string,
            * "writable": boolean, "isPassword": boolean }
            */
            blobTable = blobmsg_open_table(&blobBuffer, NULL);
            blobmsg_add_string(&blobBuffer, "fullpath", info->fullpath);
            blobmsg_add_string(&blobBuffer, "type", info->type);
            blobmsg_add_string(&blobBuffer, "profile", info->profile);
            blobmsg_add_u8(&blobBuffer, "writable", info->writable);
            blobmsg_add_u8(&blobBuffer, "isPassword", info->isPassword);
            blobmsg_close_table(&blobBuffer, blobTable);
         }
         cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);

         snprintf(tmpResp, sizeof(tmpResp), "%s: is performed completely", (char *)method);
         resp = tmpResp;
      }
      else
      {
         snprintf(tmpResp, sizeof(tmpResp), "call into zbus_in_getParameterNames failed, ret=%d", ret);
         resp = tmpResp;
      }
   }
   else
   {
      snprintf(tmpResp, sizeof(tmpResp), "%s: is invalid path", fullpath);
      resp = tmpResp;
   }

EXIT_GETPARAMNAMES:
   blobmsg_close_array(&blobBuffer, blobArray);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);

   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);
   bcmuLog_debug("Exit, ret=%d", ret);

   return;
}


void ubus_in_setParameterAttributes(const void *ctx, const void *req, void *method, void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(set_param_attribute_in_policy)] = {};
   struct blob_attr *innerArgsTable[ARRAY_SIZE(setParamAttr_innerPolicy)] = {};
   uint32_t flags;
   uint32_t numParams = 0;
   BcmGenericParamAttr *paramAttrArray = NULL;
   void *attr;
   void *blobArray;
   unsigned int len = 0, i = 0;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char tmpResp[BUFLEN_128] = {0};
   char *resp = NULL;

   bcmuLog_debug("Entered:");

   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }
   /*
   * The array name matches the key of the first field of struct
   * blobmsg_policy set_param_attribute_out_policy
   */
   blobArray = blobmsg_open_array(&blobBuffer, "fullpath_error_array");

   /* parse and verify input arguments */
   if(!parseAndVerifyParameters(set_param_attribute_in_policy,
               ARRAY_SIZE(set_param_attribute_in_policy),
               (struct blob_attr *)msg,
               argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_SETPARAMATTRI;
   }

   numParams = getNumberOfArrayEntries(argsTable[0]);
   if(0 == numParams)
   {
      bcmuLog_error("%s is called with 0 parameters", (char *)method);
      goto EXIT_SETPARAMATTRI;
   }

   /* allocate memory for array of BcmGenericParamAttr */
   paramAttrArray = (BcmGenericParamAttr *) cmsMem_alloc(sizeof(BcmGenericParamAttr) * numParams, ALLOC_ZEROIZE);
   if (NULL == paramAttrArray)
   {
      bcmuLog_error("not enough memory to hold array of %d BcmGenericParamAttr", numParams);
      ret = BCMRET_RESOURCE_EXCEEDED;
      goto EXIT_SETPARAMATTRI;
   }

   /* parsing the first argument: array of tables */
   len = blobmsg_data_len(argsTable[0]);
   __blob_for_each_attr(attr, blobmsg_data(argsTable[0]), len)
   {
      if(BLOBMSG_TYPE_TABLE != blobmsg_type(attr))
      {
         bcmuLog_error("invalid parameters(inner parameter not a tabe)");
         goto EXIT_SETPARAMATTRI;
      }

      blobmsg_parse(setParamAttr_innerPolicy, ARRAY_SIZE(setParamAttr_innerPolicy),
                  innerArgsTable,
                  blobmsg_data(attr),
                  blobmsg_data_len(attr));
      if(!innerArgsTable[0])
      {
         bcmuLog_error("missing fullpath");
         goto EXIT_SETPARAMATTRI;
      }


      /*innner data type: table: { "fullpath": string, "setAccess": boolean, "access":int16,
      * "setNotif":boolean, "notif": int16,
      * "setAltNotif":boolean, "altNotif": int16, "clearAltValue": boolean}
      */
      paramAttrArray[i].fullpath = cmsMem_strdup(blobmsg_get_string(innerArgsTable[0]));
      paramAttrArray[i].setAccess = blobmsg_get_bool(innerArgsTable[1]);
      paramAttrArray[i].access = blobmsg_get_u16(innerArgsTable[2]);
      paramAttrArray[i].setNotif = blobmsg_get_bool(innerArgsTable[3]);
      paramAttrArray[i].notif = blobmsg_get_u16(innerArgsTable[4]);
      paramAttrArray[i].setAltNotif = blobmsg_get_bool(innerArgsTable[5]);
      paramAttrArray[i].altNotif = blobmsg_get_u16(innerArgsTable[6]);
      paramAttrArray[i].clearAltNotifValue = blobmsg_get_bool(innerArgsTable[7]);
      i++;
   }

   flags = blobmsg_get_u32(argsTable[1]);

   /*
   * Call Z-Bus generic method.
   */
   ret = zbus_in_setParameterAttributes(paramAttrArray, numParams, flags);
   if(ret == BCMRET_SUCCESS)
   {
      snprintf(tmpResp, sizeof(tmpResp), "%s: is performed completely", (char *)method);
      resp = tmpResp;
   }
   else
   {
      //TODO: fill out the array of table: {"fullpath", "error"}
      resp = getErrorResp(ret);
   }

EXIT_SETPARAMATTRI:
   blobmsg_close_array(&blobBuffer, blobArray);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);

   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);
   cmsUtl_freeParamAttrArray(&paramAttrArray, numParams);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}

void ubus_in_getParameterAttributes(const void *ctx, const void *req, void *method, void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(get_param_attribute_in_policy)] = {};
   uint32_t numFullpaths = 0;
   uint8_t nextLevel;
   uint32_t flags;
   char *fullpath;
   char **fullpathArray = NULL;
   BcmGenericParamAttr *paramAttrArray = NULL;
   UINT32 numParamAttrs = 0;
   UINT32 i = 0;
   unsigned int remainLen = 0;
   void *attr;
   void *retTableArray=NULL;
   BcmRet ret = BCMRET_SUCCESS;
   char tmpResp[BUFLEN_128] = {0};
   char *resp = NULL;

   bcmuLog_debug("Entered:");

   /*
   * Prepare the return values in case we return early. Here inserting an array.
   * if the method call returns early, an arry is created with 0 entries so that we
   * can make sure that number of arguments returned matches the number of entries
   * of output policy
   */
   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }

   /*
   * The array name matches the key of the first field of struct
   * blobmsg_policy get_param_attribute_out_policy
   */
   retTableArray = blobmsg_open_array(&blobBuffer,
            "fullpath_access_notif_valueChanged_altNotif_altValue_array");

   /* parse and verify input arguments */
   if(!parseAndVerifyParameters(get_param_attribute_in_policy,
            ARRAY_SIZE(get_param_attribute_in_policy),
            (struct blob_attr *)msg,
            argsTable))
   {
      bcmuLog_error("invalid parameters");
      ret = BCMRET_INVALID_ARGUMENTS;
      goto EXIT_GETPARAMATTRI;
   }

   numFullpaths = getNumberOfArrayEntries(argsTable[0]);
   if(0 == numFullpaths)
   {
      bcmuLog_error("%s is called with 0 parameters", (char *)method);
      ret = BCMRET_INVALID_ARGUMENTS;
      goto EXIT_GETPARAMATTRI;
   }

   /* allocate memory for array of char ptrs */
   fullpathArray = cmsMem_alloc(numFullpaths * sizeof(char *), ALLOC_ZEROIZE);
   if (fullpathArray == NULL)
   {
      bcmuLog_error("not enough memory to hold array of %d char *", numFullpaths);
      ret = BCMRET_RESOURCE_EXCEEDED;
      goto EXIT_GETPARAMATTRI;
   }

   /* iterate the table array to get {fullpath} of each parameter */
   remainLen = blobmsg_data_len(argsTable[0]);
   __blob_for_each_attr(attr, blobmsg_data(argsTable[0]), remainLen)
   {
      fullpath = blobmsg_get_string(attr);

      if(is_pathNameEndwithInstNbrNodot(fullpath) == TRUE)
      {
         snprintf(tmpResp, sizeof(tmpResp), "%s ends with instance number no dot", fullpath);
         ret = BCMRET_INVALID_PARAM_NAME;
      }
      resp = tmpResp;

      if(ret != BCMRET_SUCCESS)
      {
         goto EXIT_GETPARAMATTRI;
      }

      fullpathArray[i] = cmsMem_strdup(fullpath);
      i++;
   }

   nextLevel = blobmsg_get_bool(argsTable[1]);
   flags = blobmsg_get_u32(argsTable[2]);

   /*
    * Call Z-Bus generic method.
    */
   ret = zbus_in_getParameterAttributes((const char **) fullpathArray, numFullpaths,
                                        nextLevel, flags,
                                        &paramAttrArray, &numParamAttrs);
   if(ret == BCMRET_SUCCESS)
   {
      /*insert tables to array "fullpath_access_notif_valueChanged_altNotif_altValue_array" */
      for (i = 0; i < numParamAttrs; i++)
      {
         void *blobTable;

         blobTable = blobmsg_open_table(&blobBuffer, NULL);

         /*
         * innner data type: table: { "fullpath": string, "access": int16, "notif":int16,
         * "valueChanged":int16, "altNotif": int16,  "altValue": int16}
         */
         blobmsg_add_string(&blobBuffer, "fullpath", paramAttrArray[i].fullpath);
         blobmsg_add_u16(&blobBuffer, "access", paramAttrArray[i].access);
         blobmsg_add_u16(&blobBuffer, "notif", paramAttrArray[i].notif);
         blobmsg_add_u16(&blobBuffer, "valueChanged", paramAttrArray[i].valueChanged);
         blobmsg_add_u16(&blobBuffer, "altNotif", paramAttrArray[i].altNotif);
         blobmsg_add_u16(&blobBuffer, "altValue", paramAttrArray[i].altNotifValue);

         blobmsg_close_table(&blobBuffer, blobTable);
      }

      snprintf(tmpResp, sizeof(tmpResp), "%s success, return %d attrs", (char *)method, numParamAttrs);
      resp = tmpResp;
   }

EXIT_GETPARAMATTRI:
   blobmsg_close_array(&blobBuffer, retTableArray);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);
   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);

   cmsUtl_freeArrayOfStrings(&fullpathArray, numFullpaths);
   cmsUtl_freeParamAttrArray(&paramAttrArray, numParamAttrs);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}

void ubus_in_addObject(const void *ctx, const void *req, void *method __attribute__((unused)), void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(adddel_object_in_policy)] = {};
   char *fullpath;
   uint32_t flags;
   uint32_t instance = 0;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char tmpResp[BUFLEN_128];
   char *resp = NULL;

   bcmuLog_debug("Entered:");

   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }

   /* parse and verify input arguments */
   if(!parseAndVerifyParameters(adddel_object_in_policy,
            ARRAY_SIZE(adddel_object_in_policy),
            (struct blob_attr *)msg,
            argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_ADDOBJECT;
   }

   fullpath = blobmsg_get_string(argsTable[0]);
   if(IS_EMPTY_STRING(fullpath))
   {
      goto EXIT_ADDOBJECT;
   }
   flags = blobmsg_get_u32(argsTable[1]);

   bcmuLog_notice("got fullpath=%s flags=0x%x", fullpath, flags);

   /*
   * Call Z-Bus generic method to do the real work.
   */
   ret = zbus_in_addObject(fullpath, flags, &instance);
   if (ret == BCMRET_SUCCESS)
   {
      snprintf(tmpResp, sizeof(tmpResp), "Add object of %s OK", fullpath);
   }
   else if(ret == BCMRET_SUCCESS_REBOOT_REQUIRED)
   {
      snprintf(tmpResp, sizeof(tmpResp), "Add object of %s OK, but reboot required", fullpath);
   }
   else
   {
      snprintf(tmpResp, sizeof(tmpResp), "Add object of %s failed", fullpath);
   }
   resp = tmpResp;

EXIT_ADDOBJECT:
   blobmsg_add_u32(&blobBuffer, "instancenumber", instance);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);

   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}

void ubus_in_deleteObject(const void *ctx, const void *req, void *method __attribute__((unused)), void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(adddel_object_in_policy)] = {};
   char *fullpath;
   uint32_t flags;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char tmpResp[BUFLEN_128];
   char *resp = NULL;

   bcmuLog_debug("Entered:");

   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }

   /* parse and verify input arguments: use same input arguments policy as addObject */
   if(!parseAndVerifyParameters(adddel_object_in_policy,
            ARRAY_SIZE(adddel_object_in_policy),
            (struct blob_attr *)msg,
            argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_DELOBJECT;
   }

   fullpath = blobmsg_get_string(argsTable[0]);
   if(IS_EMPTY_STRING(fullpath))
   {
      goto EXIT_DELOBJECT;
   }

   flags = blobmsg_get_u32(argsTable[1]);
   bcmuLog_notice("got fullpath=%s flags=0x%x", fullpath, flags);

   /*
    * Call Z-Bus generic method to do the real work.
    */
   ret = zbus_in_deleteObject(fullpath, flags);
   if (ret == BCMRET_SUCCESS)
   {
      snprintf(tmpResp, sizeof(tmpResp), "Delete object %s OK", fullpath);
   }
   else if (ret == BCMRET_SUCCESS_REBOOT_REQUIRED)
   {
      snprintf(tmpResp, sizeof(tmpResp), "Delete object %s OK, but reboot required", fullpath);
   }
   else
   {
      snprintf(tmpResp, sizeof(tmpResp), "Delete of %s failed", fullpath);
   }
   resp = tmpResp;

EXIT_DELOBJECT:
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);

   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}

void ubus_in_getNextObject(const void *ctx, const void *req, void *method __attribute__((unused)), void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *argsTable[ARRAY_SIZE(get_next_object_in_policy)] = {};
   char *fullpath, *limitSubtree = NULL;
   uint32_t flags;
   void *mdmObj = NULL;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   MdmPathDescriptor parentPathDesc = EMPTY_PATH_DESCRIPTOR;
   char *nextFullpath = NULL;
   void *blobArray;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   CmsRet cret = CMSRET_SUCCESS;
   char tmpResp[BUFLEN_128];
   char *resp = NULL;

   bcmuLog_debug("Entered:");

   /*
   * Prepare the return values in case we return early. Here inserting an array.
   * if the method call returns early, an arry is created with 0 entries so that we
   * can make sure that number of arguments returned matches the number of entries
   * of output policy
   */
   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }
   /*
   * The array name matches the key of the first field of struct
   * blobmsg_policy get_next_object_out_policy
   */
   blobArray = blobmsg_open_array(&blobBuffer,
            "fullpath_type_value_profile_writable_isPassword_array");

   /* parse and verify input arguments */
   if(!parseAndVerifyParameters(get_next_object_in_policy,
            ARRAY_SIZE(get_next_object_in_policy),
            (struct blob_attr *)msg,
            argsTable))
   {
      bcmuLog_error("invalid parameters");
      goto EXIT_GETNEXTOBJECT;
   }

   fullpath = blobmsg_get_string(argsTable[0]);
   limitSubtree = blobmsg_get_string(argsTable[1]);
   flags = blobmsg_get_u32(argsTable[2]);

    // TODO: push all this direct MDM accesses down into BCM Generic HAL
    // On the first call to getNextObject, fullpath may be a generic path
   cret = cmsMdm_genericPathToOid(fullpath, &(pathDesc.oid));
   if(cret != CMSRET_SUCCESS)
   {
      cret = cmsMdm_fullPathToPathDescriptor(fullpath, &pathDesc);
      if (cret != CMSRET_SUCCESS)
      {
         snprintf(tmpResp, sizeof(tmpResp), "invalid fullpath: %s", fullpath);
         resp = tmpResp;
         goto EXIT_GETNEXTOBJECT;
      }
   }

   // ParentIidStack limits the subtree where the getNext is allowed to walk.
   cmsMdm_fullPathToPathDescriptor(limitSubtree, &parentPathDesc);

   bcmuLog_debug("oid=%d", pathDesc.oid);
   bcmuLog_debug("parentIidStack:%s", cmsMdm_dumpIidStack(&parentPathDesc.iidStack));
   bcmuLog_debug("iidStack: %s", cmsMdm_dumpIidStack(&(pathDesc.iidStack)));

   cret = cmsObj_getNextInSubTreeFlags(pathDesc.oid,
                                        &(parentPathDesc.iidStack),
                                        &(pathDesc.iidStack),
                                        flags,
                                        &mdmObj);
   if (cret == CMSRET_SUCCESS)
   {
      char *nFullpath = NULL;
      char *fullpathArray[1];
      BcmGenericParamInfo *paramInfoArray = NULL;
      UINT32 numParamInfos = 0;
      UINT32 i;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &nFullpath);
      fullpathArray[0] = nFullpath;
      bcmuLog_debug("got next fullpath %s", nFullpath);
      nextFullpath = cmsMem_strdup(nFullpath);

      /*
      * Call generic Z-Bus function to fill in values for this object.
      */
      ret = zbus_in_getParameterValues((const char **) fullpathArray, 1,
                                       TRUE, flags,
                                       &paramInfoArray, &numParamInfos);

      CMSMEM_FREE_BUF_AND_NULL_PTR(nFullpath);

      if (ret == BCMRET_SUCCESS)
      {
         for (i = 0; i < numParamInfos; i++)
         {
            fillParameterInfoIntoTable(&(paramInfoArray[i]), &blobBuffer);
         }
         snprintf(tmpResp, sizeof(tmpResp), "getNextObject returned %d params",
                                             numParamInfos);
         cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
      }
      else
      {
         bcmuLog_error("Error from zbus_in_getParameterValues, ret=%d", ret);
         snprintf(tmpResp, sizeof(tmpResp), "Error from zbus_in_getParameterValues, ret=%d", ret);
      }

      // Delay the freeing of the mdmObj to this point.  This forces MDM
      // auto-lock to hold the lock for us while we are calling
      // zbus_in_getParameterValues on this object.
      cmsObj_free(&mdmObj);
   }
   else
   {
      // Don't complain loudly about error because it could be NO_MORE_INSTANCES(?)
      bcmuLog_notice("Error from cmsObj_getNextInSubTreeFlags, ret=%d", cret);
      ret = (BcmRet) cret;
      snprintf(tmpResp, sizeof(tmpResp), "Error from cmsObj_getNextInSubTreeFlags, ret=%d", cret);
   }
   resp = tmpResp;

EXIT_GETNEXTOBJECT:
   blobmsg_close_array(&blobBuffer, blobArray);
   if(!nextFullpath)
      nextFullpath = cmsMem_strdup(" ");
   blobmsg_add_string(&blobBuffer, "nextFullpath", nextFullpath);
   blobmsg_add_string(&blobBuffer, "response", resp? resp : getErrorResp(ret));
   blobmsg_add_u32(&blobBuffer, "result", ret);

   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);
   cmsMem_free(nextFullpath);

   bcmuLog_debug("Exit, ret=%d", ret);
   return;
}

