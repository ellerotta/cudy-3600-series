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


/*!\file ubus_out_strproto.c
 * \brief Actual U-Bus access methods for outbound (zbus_out_xxx) functions.
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
#include "bcm_ubus_intf.h"
#include "cms_mem.h"

#include "blobmsg_json.h"
#include "libubus.h"


typedef struct
{
   uint32_t result;
   char resp[BUFLEN_128];
   BcmGenericParamInfo *paramInfoArray;
   uint32_t numParamInfos;
   char *nextFullpath;
}GenericParamInfoRet;

typedef struct
{
   uint32_t result;
   char resp[BUFLEN_128];
   BcmGenericParamAttr *paramAttrArray;
   uint32_t numParamAttrs;
}GenericParamAttrRet;

typedef struct
{
   uint32_t result;
   char resp[BUFLEN_128];
   uint32_t instanceNumber;
}AddDelObjectRet;

const struct blobmsg_policy get_param_name_out_policy[3] =
{
  /*innner data type: table: { "fullpath": string, "type": string,"profile": string,
   * "writable": boolean, "isPassword": boolean }
   */
   [0] = { .name = "fullpath_type_profile_writable_isPassword_array", .type = BLOBMSG_TYPE_ARRAY },
   [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy get_param_value_out_policy[3] =
{
   /*innner data type: table: { "fullpath": string, "type": string, "value": string,
   * "profile":string, "writable":boolean, "isPassword":boolean }
   */
   [0] = { .name = "fullpath_type_value_profile_writable_isPassword_array", .type = BLOBMSG_TYPE_ARRAY },

   [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy set_param_value_out_policy[3] =
{
   /*innner data type: table: { "fullpath": string, "status": int32 }*/
   [0] = { .name = "fullpath_error_array", .type = BLOBMSG_TYPE_ARRAY },

   [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy get_param_attribute_out_policy[3] =
{
   /*
   * innner data type: table: { "fullpath": string, "access": int16, "notif":int16,
   * "valueChanged":int16, "altNotif": int16,  "altValue": int16}
   */
   [0] = { .name = "fullpath_access_notif_valueChanged_altNotif_altValue_array", .type = BLOBMSG_TYPE_ARRAY },

   [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy set_param_attribute_out_policy[3] =
{
   [0] = { .name = "fullpath_error_array", .type = BLOBMSG_TYPE_ARRAY},
   [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy add_object_out_policy[3] =
{
   [0] = { .name = "instancenumber", .type = BLOBMSG_TYPE_INT32},
   [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy delete_object_out_policy[2] =
{
   [0] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy get_next_object_out_policy[4] =
{
   /*
   * innner data type: table: { "fullpath": string, "type": string, "value": string,
   * "profile":string, "writable":boolean, "isPassword":boolean }
   */
   [0] = { .name = "fullpath_type_value_profile_writable_isPassword_array", .type = BLOBMSG_TYPE_ARRAY},
   [1] = { .name = "nextFullpath", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
   [3] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy getParamName_out_innerPolicy[5] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "type", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "profile", .type = BLOBMSG_TYPE_STRING},
   [3] = { .name = "writable", .type = BLOBMSG_TYPE_BOOL},
   [4] = { .name = "isPassword", .type = BLOBMSG_TYPE_BOOL},
};

const struct blobmsg_policy getParamValues_out_innerPolicy[6] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "type", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "value", .type = BLOBMSG_TYPE_STRING},
   [3] = { .name = "profile", .type = BLOBMSG_TYPE_STRING},
   [4] = { .name = "writable", .type = BLOBMSG_TYPE_BOOL},
   [5] = { .name = "isPassword", .type = BLOBMSG_TYPE_BOOL},
};

const struct blobmsg_policy setParamValues_out_innerPolicy[2] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "status", .type = BLOBMSG_TYPE_INT32},
};

/*
* innner data type: table: { "fullpath": string, "access": int16, "notif":int16,
* "valueChanged":int16, "altNotif": int16,  "altValue": int16}
*/
const struct blobmsg_policy getParamAttrs_out_innerPolicy[6] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "access", .type = BLOBMSG_TYPE_INT16},
   [2] = { .name = "notif", .type = BLOBMSG_TYPE_INT16},
   [3] = { .name = "valueChanged", .type = BLOBMSG_TYPE_INT16},
   [4] = { .name = "altNotif", .type = BLOBMSG_TYPE_INT16},
   [5] = { .name = "altValue", .type = BLOBMSG_TYPE_INT16},
};

/* parse blob attr based on getParamValues_out_innerPolicy */
static BcmRet parseParamValueArray(struct blob_attr *blobAttr,
            GenericParamInfoRet *paramInfoRet)
{
   struct blob_attr *retInnerTable[ARRAY_SIZE(getParamValues_out_innerPolicy)] = {};
   uint32_t numParams;
   void *attr;
   uint32_t len, i = 0;

   numParams = getNumberOfArrayEntries(blobAttr);
   if((paramInfoRet->paramInfoArray= cmsMem_alloc(numParams * sizeof(BcmGenericParamInfo), ALLOC_ZEROIZE)) != NULL)
   {
      /* iterate the parameter table array to get {fullpath, type, value, profile, writable, isPassword} of each parameter */
      len = blobmsg_data_len(blobAttr);
      __blob_for_each_attr(attr, blobmsg_data(blobAttr), len)
      {
         char *fullpath, *type, *value, *profile;
         uint8_t writable, isPassword;

         blobmsg_parse(getParamValues_out_innerPolicy, ARRAY_SIZE(getParamValues_out_innerPolicy),
                     retInnerTable,
                     blobmsg_data(attr),
                     blobmsg_data_len(attr));

         fullpath = blobmsg_get_string(retInnerTable[0]);
         type = blobmsg_get_string(retInnerTable[1]);
         value = blobmsg_get_string(retInnerTable[2]);
         profile = blobmsg_get_string(retInnerTable[3]);
         writable = blobmsg_get_u8(retInnerTable[4]);
         isPassword = blobmsg_get_u8(retInnerTable[5]);

         bcmuLog_debug("  fullpath: %s. type: %s. value: %s. profile=%s. writable: %s. isPassword:%s",
                     fullpath, type, value, profile,
                     writable? "yes" : "no",
                     isPassword? "yes": "no");

         paramInfoRet->paramInfoArray[i].fullpath= cmsMem_strdup(fullpath);
         paramInfoRet->paramInfoArray[i].type = cmsMem_strdup(type);
         paramInfoRet->paramInfoArray[i].value = cmsMem_strdup(value);
         paramInfoRet->paramInfoArray[i].profile = cmsMem_strdup(profile);
         paramInfoRet->paramInfoArray[i].writable = writable;
         paramInfoRet->paramInfoArray[i].isPassword = isPassword;

          i++;
      }
      paramInfoRet->numParamInfos = numParams;
      return BCMRET_SUCCESS;
   }

   return BCMRET_RESOURCE_EXCEEDED;
}

/*
* output parameter policy defined in get_param_name_out_policy
*  innner data type: table: { "fullpath": string, "type": string,"profile": string,
* "writable": boolean, "isPassword": boolean }
*  [0] = { .name = "fullpath_type_profile_writable_isPassword_array", .type = BLOBMSG_TYPE_ARRAY },
*  [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
*  [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleGetParameterNames(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(get_param_name_out_policy)] = {};
   struct blob_attr *retInnerTable[ARRAY_SIZE(getParamName_out_innerPolicy)] = {};
   GenericParamInfoRet *paramInfoRet = req->priv;
   struct blob_attr *attr;
   uint32_t numParams = 0;
   uint32_t len, i = 0;

   if(!paramInfoRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(get_param_name_out_policy,
               ARRAY_SIZE(get_param_name_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   snprintf(paramInfoRet->resp, sizeof(paramInfoRet->resp), "%s", blobmsg_get_string(retArgsTable[1]));
   paramInfoRet->result = blobmsg_get_u32(retArgsTable[2]);

   /* iterate the parameter table array(retArgsTable[0]) to get { "fullpath": string, "writable": boolean }
   *  of each parameter
   */
   if(BCMRET_SUCCESS == paramInfoRet->result )
   {
      numParams = getNumberOfArrayEntries(retArgsTable[0]);
      if((paramInfoRet->paramInfoArray= cmsMem_alloc(numParams * sizeof(BcmGenericParamInfo),
                  ALLOC_ZEROIZE)) != NULL)
      {
         len = blobmsg_data_len(retArgsTable[0]);
         __blob_for_each_attr(attr, blobmsg_data(retArgsTable[0]), len)
         {
            char *fullpath, *type, *profile;
            uint8_t writable, isPassword;

            blobmsg_parse(getParamName_out_innerPolicy, ARRAY_SIZE(getParamName_out_innerPolicy),
                        retInnerTable,
                        blobmsg_data(attr),
                        blobmsg_data_len(attr));
            /*innner data type: table:
            * { "fullpath": string, "type": string,"profile": string,
            * "writable": boolean, "isPassword": boolean }
            */
            fullpath = blobmsg_get_string(retInnerTable[0]);
            type= blobmsg_get_string(retInnerTable[1]);
            profile = blobmsg_get_string(retInnerTable[2]);
            writable = blobmsg_get_u8(retInnerTable[3]);
            isPassword= blobmsg_get_u8(retInnerTable[4]);

            bcmuLog_debug("  fullpath: %s. type: %s. profile: %s. writable: %d, isPassword: %d",
                        fullpath, type, profile,
                        writable, isPassword);

            paramInfoRet->paramInfoArray[i].fullpath= cmsMem_strdup(fullpath);
            paramInfoRet->paramInfoArray[i].type = cmsMem_strdup(type);
            paramInfoRet->paramInfoArray[i].profile = cmsMem_strdup(profile);
            paramInfoRet->paramInfoArray[i].writable = writable;
            paramInfoRet->paramInfoArray[i].isPassword = isPassword;

            i++;
         }

         paramInfoRet->numParamInfos = numParams;
      }
      else
      {
         /* overwrite the returned result */
         paramInfoRet->result = BCMRET_RESOURCE_EXCEEDED;
         bcmuLog_error("no memory to hold array of BcmGenericParamInfo");
      }
   }
}

/*
* output parameter policy defined in get_param_value_out_policy
* innner data type: table: { "fullpath": string, "type": string, "value": string,
* "profile":string, "writable":boolean, "isPassword":boolean }
*  [0] = { .name = "fullpath_type_value_profile_writable_isPassword_array", .type = BLOBMSG_TYPE_ARRAY },
*  [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
*  [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleGetParameterValues(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(get_param_value_out_policy)] = {};
   GenericParamInfoRet *paramInfoRet = req->priv;

   if(!paramInfoRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(get_param_value_out_policy,
               ARRAY_SIZE(get_param_value_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   snprintf(paramInfoRet->resp, sizeof(paramInfoRet->resp), "%s", blobmsg_get_string(retArgsTable[1]));
   paramInfoRet->result = blobmsg_get_u32(retArgsTable[2]);

   if(BCMRET_SUCCESS == paramInfoRet->result )
   {
      paramInfoRet->result = parseParamValueArray(retArgsTable[0], paramInfoRet);
   }
}

/* output parameter policy defined in set_param_value_out_policy
* innner data type: table: { "fullpath": string, "status": int32 }
*  [0] = { .name = "fullpath_error_array", .type = BLOBMSG_TYPE_ARRAY },
*  [1] = { .name ="response", .type = BLOBMSG_TYPE_STRING},
*  [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleSetParameterValues(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(set_param_value_out_policy)] = {};
   struct blob_attr *retInnerTable[ARRAY_SIZE(setParamValues_out_innerPolicy)] = {};
   GenericParamInfoRet *paramInfoRet = req->priv;
   BcmGenericParamInfo *paramInfo = NULL;
   struct blob_attr *attr;
   uint32_t i = 0;
   uint32_t len;
   uint32_t numParamInfos = 0;

   if(!paramInfoRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(set_param_value_out_policy,
               ARRAY_SIZE(set_param_value_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   numParamInfos = paramInfoRet->numParamInfos;

   snprintf(paramInfoRet->resp, sizeof(paramInfoRet->resp), "%s", blobmsg_get_string(retArgsTable[1]));
   paramInfoRet->result = blobmsg_get_u32(retArgsTable[2]);

   paramInfo = paramInfoRet->paramInfoArray;

   if(BCMRET_SUCCESS != paramInfoRet->result )
   {
      /* iterate the parameter table array to get {fullpath, status} of each parameter */
      len = blobmsg_data_len(retArgsTable[0]);
      __blob_for_each_attr(attr, blobmsg_data(retArgsTable[0]), len)
      {
         char *fullpath;
         uint32_t status;

         blobmsg_parse(setParamValues_out_innerPolicy,
                        ARRAY_SIZE(setParamValues_out_innerPolicy),
                        retInnerTable,
                        blobmsg_data(attr),
                        blobmsg_data_len(attr));

         fullpath = blobmsg_get_string(retInnerTable[0]);
         status   = blobmsg_get_u32(retInnerTable[1]);

         for(i = 0; i < numParamInfos; i++)
         {
            if(!strcmp(paramInfo[i].fullpath, fullpath))
               paramInfo[i].errorCode = status;
         }
      }
   }
}

/* output parameter policy defined in get_param_attribute_out_policy
* innner data type: table: { "fullpath": string, "access": int16, "notif":int16,
* "valueChanged":int16, "altNotif": int16,  "altValue": int16}
*  [0] = { .name = "fullpath_access_notif_valueChanged_altNotif_altValue_array", .type = BLOBMSG_TYPE_ARRAY },
*  [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
*  [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleGetParameterAttrs(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(get_param_attribute_out_policy)] = {};
   struct blob_attr *retInnerTable[ARRAY_SIZE(getParamAttrs_out_innerPolicy)] = {};
   GenericParamAttrRet *paramAttrRet = req->priv;
   struct blob_attr *attr;
   uint32_t numParams = 0;
   uint32_t len, i = 0;

   if(!paramAttrRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(get_param_attribute_out_policy,
               ARRAY_SIZE(get_param_attribute_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   snprintf(paramAttrRet->resp, sizeof(paramAttrRet->resp), "%s", blobmsg_get_string(retArgsTable[1]));
   paramAttrRet->result = blobmsg_get_u32(retArgsTable[2]);

   if(BCMRET_SUCCESS == paramAttrRet->result )
   {
      numParams = getNumberOfArrayEntries(retArgsTable[0]);

      if((paramAttrRet->paramAttrArray= cmsMem_alloc(numParams * sizeof(BcmGenericParamAttr), ALLOC_ZEROIZE)) != NULL)
      {
         /*
         * innner data type: table: { "fullpath": string, "access": int16, "notif":int16,
         * "valueChanged":int16, "altNotif": int16,  "altValue": int16}
         */
         len = blobmsg_data_len(retArgsTable[0]);
         __blob_for_each_attr(attr, blobmsg_data(retArgsTable[0]), len)
         {
            char *fullpath = NULL;
            uint16_t access = 0, notification = 0;
            uint16_t valueChanged = 0, altNotif = 0, altValue = 0;

            blobmsg_parse(getParamAttrs_out_innerPolicy, ARRAY_SIZE(getParamAttrs_out_innerPolicy),
                        retInnerTable,
                        blobmsg_data(attr),
                        blobmsg_data_len(attr));
            fullpath = blobmsg_get_string(retInnerTable[0]);
            access = blobmsg_get_u16(retInnerTable[1]);
            notification = blobmsg_get_u16(retInnerTable[2]);
            valueChanged = blobmsg_get_u16(retInnerTable[3]);
            altNotif = blobmsg_get_u16(retInnerTable[4]);
            altValue = blobmsg_get_u16(retInnerTable[5]);

            bcmuLog_debug("  fullpath=%s. access=%d. notification=%d, valueChanged=%d, alNotif=%d, altValue=%d",
                        fullpath,
                        access, notification, valueChanged,
                        altNotif, altValue);

            paramAttrRet->paramAttrArray[i].fullpath = cmsMem_strdup(fullpath);
            paramAttrRet->paramAttrArray[i].access = access;
            paramAttrRet->paramAttrArray[i].notif = notification;
            paramAttrRet->paramAttrArray[i].valueChanged = valueChanged;
            paramAttrRet->paramAttrArray[i].altNotif = altNotif;
            paramAttrRet->paramAttrArray[i].altNotifValue = altValue;

            i++;
         }

         paramAttrRet->numParamAttrs = i;
      }
      else
      {
         paramAttrRet->result = BCMRET_RESOURCE_EXCEEDED;
         bcmuLog_error("no memory to hold array of BcmGenericParamInfo");
      }
   }
}

/* output parameter policy defined in set_param_attribute_out_policy
* [0] = { .name = "fullpath_error_array", .type = BLOBMSG_TYPE_ARRAY},
* [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
* [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleSetParameterAttrs(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(set_param_attribute_out_policy)] = {};
   GenericParamAttrRet *paramAttrRet = req->priv;

   if(!paramAttrRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(set_param_attribute_out_policy,
               ARRAY_SIZE(set_param_attribute_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   snprintf(paramAttrRet->resp, sizeof(paramAttrRet->resp), "%s", blobmsg_get_string(retArgsTable[1]));
   paramAttrRet->result = blobmsg_get_u32(retArgsTable[2]);
}

/* output parameter policy defined in add_object_out_policy
*  [0] = { .name = "instancenumber", .type = BLOBMSG_TYPE_INT32},
*  [1] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
*  [2] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleAddObject(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(add_object_out_policy)] = {};
   AddDelObjectRet *objectRet = req->priv;

   if(!objectRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(add_object_out_policy,
               ARRAY_SIZE(add_object_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   snprintf(objectRet->resp, sizeof(objectRet->resp), "%s", blobmsg_get_string(retArgsTable[1]));
   objectRet->result = blobmsg_get_u32(retArgsTable[2]);
   if(objectRet->result == BCMRET_SUCCESS)
   {
      objectRet->instanceNumber = blobmsg_get_u32(retArgsTable[0]);
   }
}

/* output parameter policy defined in delete_object_out_policy
*  [0] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
*  [1] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleDeleteObject(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(delete_object_out_policy)] = {};
   AddDelObjectRet *objectRet = req->priv;

   if(!objectRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(delete_object_out_policy,
               ARRAY_SIZE(delete_object_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   snprintf(objectRet->resp, sizeof(objectRet->resp), "%s", blobmsg_get_string(retArgsTable[0]));
   objectRet->result = blobmsg_get_u32(retArgsTable[1]);
}

/* output parameter policy defined in get_next_object_in_policy
* innner data type: table: { "fullpath": string, "type": string, "value": string,
* "profile":string, "writable":boolean, "isPassword":boolean }
*  [0] = { .name = "fullpath_type_value_profile_writable_isPassword_array", .type = BLOBMSG_TYPE_ARRAY},
*  [1] = { .name = "nextFullpath", .type = BLOBMSG_TYPE_STRING},
*  [2] = { .name = "response", .type = BLOBMSG_TYPE_STRING},
*  [3] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
*/
static void callback_handleGetNextObject(struct ubus_request *req,
            int type __attribute__((unused)),
            struct blob_attr *msg)
{
   struct blob_attr *retArgsTable[ARRAY_SIZE(get_next_object_out_policy)] = {};
   GenericParamInfoRet *paramInfoRet = req->priv;

   if(!paramInfoRet)
   {
      bcmuLog_error("no private data to hold result of method call");
      return;
   }

   if(!parseAndVerifyParameters(get_next_object_out_policy,
               ARRAY_SIZE(get_next_object_out_policy),
               msg,
               retArgsTable))
   {
      bcmuLog_error("returned arguments don't match the policy");
      return;
   }

   paramInfoRet->nextFullpath = cmsMem_strdup(blobmsg_get_string(retArgsTable[1]));
   snprintf(paramInfoRet->resp, sizeof(paramInfoRet->resp), "%s", blobmsg_get_string(retArgsTable[2]));
   paramInfoRet->result = blobmsg_get_u32(retArgsTable[3]);

   bcmuLog_debug("nextFullpath: %s. resp: %s. result=%d", paramInfoRet->nextFullpath,
               paramInfoRet->resp,
               paramInfoRet->result);

   if(BCMRET_SUCCESS == paramInfoRet->result )
   {
      paramInfoRet->result = parseParamValueArray(retArgsTable[0], paramInfoRet);
   }

   bcmuLog_debug("Exit. ret=%d", paramInfoRet->result);
}

BcmRet bus_out_getParameterNames(const ZbusAddr *destAddr,
                const char *fullpath, UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   GenericParamInfoRet getParamNamesRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   int mret;

   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_debug("Entered:");

   if(!destAddr || !fullpath || !numParamInfos || !paramInfoArray)
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   *numParamInfos = 0;
   *paramInfoArray = NULL;

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method getParameterNames
   * input parameter defined int get_param_name_in_policy
   * [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING },
   * [1] = { .name = "nextlevel", .type = BLOBMSG_TYPE_BOOL},
   * [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   blobmsg_add_string(handle->buf, "fullpath", fullpath);
   blobmsg_add_u8(handle->buf, "nextlevel", nextLevel);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "getParameterNames",
         handle->buf->head,
         callback_handleGetParameterNames,
         &getParamNamesRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("getParameterNames invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("getParameterNames method returned %d (%s)", getParamNamesRet.result, getParamNamesRet.resp);

      if(BCMRET_SUCCESS == getParamNamesRet.result)
      {
         /*parse returned array of table to fill paramInfoArray and numParamInfos */
         *numParamInfos = getParamNamesRet.numParamInfos;
         *paramInfoArray = getParamNamesRet.paramInfoArray;
      }
      ret = getParamNamesRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit: ret =%d", ret);
   return ret;
}


BcmRet bus_out_getParameterValues(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   GenericParamInfoRet getParamValuesRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   void *fullpathBlobArray;
   int mret;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   uint32_t i;

   if(!destAddr || !fullpathArray ||(0 == numEntries)
               || !paramInfoArray || !numParamInfos)
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_debug("Entered: numEntries=%d nextLevel=%d flags=0x%x",
                 numEntries, nextLevel, flags);

   *numParamInfos = 0;
   *paramInfoArray = NULL;

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method getParameterValues
   *  inner data type: "fullpath": string
   *  [0] = { .name = "fullpath_array", .type = BLOBMSG_TYPE_ARRAY},
   *  [1] = { .name = "nextlevel", .type = BLOBMSG_TYPE_BOOL},
   *  [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   fullpathBlobArray = blobmsg_open_array(handle->buf, "fullpath_array");
   for(i = 0; i < numEntries; i++)
   {
      if(!fullpathArray[i])
      {
         bcmuLog_error("Emtpy fullpath in entry %d", i);
         ubusIntf_freeOutboundHandle(handle);

         return BCMRET_INVALID_ARGUMENTS;
      }

      blobmsg_add_string(handle->buf, "fullpath", fullpathArray[i]);
   }
   blobmsg_close_array(handle->buf, fullpathBlobArray);
   blobmsg_add_u8(handle->buf, "nextlevel", nextLevel);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "getParameterValues",
         handle->buf->head,
         callback_handleGetParameterValues,
         &getParamValuesRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("getParameterValues invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("getParameterValues method returned %d (%s)", getParamValuesRet.result, getParamValuesRet.resp);

      if(BCMRET_SUCCESS == getParamValuesRet.result)
      {
         /*parse returned array of table to fill paramInfoArray and numParamInfos */
         *numParamInfos = getParamValuesRet.numParamInfos;
         *paramInfoArray = getParamValuesRet.paramInfoArray;
      }
      ret = getParamValuesRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}


BcmRet bus_out_setParameterValues(const ZbusAddr *destAddr,
                BcmGenericParamInfo *paramInfoArray, UINT32 numParamInfos,
                UINT32 flags)
{
   GenericParamInfoRet setParamValuesRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   void *ptvBlobArray;  /* blob array of fullpath, type and value*/
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int mret;
   uint32_t i;

   bcmuLog_notice("Entered:");

   if(!destAddr || !paramInfoArray || (0 == numParamInfos) )
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_debug("Entered: numEntries=%d flags=0x%x", numParamInfos, flags);

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* reuse this buffer */
   setParamValuesRet.paramInfoArray = paramInfoArray;
   setParamValuesRet.numParamInfos = numParamInfos;

   /* prepare arguments to call ubus method setParameterValues
   *  inner data type: { "fullpath": string, "type": string, "value": string }
   *  [0] = { .name = "fullpath_type_value_array", .type = BLOBMSG_TYPE_ARRAY},
   *  [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   ptvBlobArray = blobmsg_open_array(handle->buf, "fullpath_type_value_array");
   for(i = 0; i< numParamInfos; i++)
   {
      void * blobTable = blobmsg_open_table(handle->buf, NULL);

      if(!paramInfoArray[i].fullpath || !paramInfoArray[i].type || !paramInfoArray[i].value)
      {
         bcmuLog_error("NULL input param at [%d]: fullpath: %s type: %s value: %s",
                     i,
                     paramInfoArray[i].fullpath,
                     paramInfoArray[i].type,
                     paramInfoArray[i].value);

         ubusIntf_freeOutboundHandle(handle);
         paramInfoArray[i].errorCode = BCMRET_INVALID_ARGUMENTS;
         return BCMRET_INVALID_ARGUMENTS;
      }

      blobmsg_add_string(handle->buf, "fullpath", paramInfoArray[i].fullpath);
      blobmsg_add_string(handle->buf, "type", paramInfoArray[i].type);
      blobmsg_add_string(handle->buf, "value", paramInfoArray[i].value);

      blobmsg_close_table(handle->buf, blobTable);
   }
   blobmsg_close_array(handle->buf, ptvBlobArray);

   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "setParameterValues",
         handle->buf->head,
         callback_handleSetParameterValues,
         &setParamValuesRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("setParameterValues invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      /*status code are set into input paramInfo array (reuse) */
      bcmuLog_debug("setParameterValues method returned result=%d, resp=%s", setParamValuesRet.result, setParamValuesRet.resp);
      ret = setParamValuesRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}


BcmRet bus_out_getParameterAttributes(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamAttr **paramAttrArray, UINT32 *numParamAttrs)
{
   GenericParamAttrRet getParamAttrsRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   void *fullpathBlobArray;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int mret;
   uint32_t i;

   bcmuLog_notice("Entered:");

   if(!destAddr || !fullpathArray ||(0 == numEntries)
               || !paramAttrArray || !numParamAttrs)
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   *paramAttrArray = NULL;
   *numParamAttrs = 0;

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method getParameterAttributes
   *   inner data type: "fullpath": string
   *  [0] = { .name = "fullpath_array", .type = BLOBMSG_TYPE_ARRAY},
   *  [1] = { .name = "nextlevel", .type = BLOBMSG_TYPE_BOOL},
   *  [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   fullpathBlobArray = blobmsg_open_array(handle->buf, "fullpath_array");
   for(i = 0; i < numEntries; i++)
   {
      if(!fullpathArray[i])
      {
         bcmuLog_error("Emtpy fullpath in entry %d", i);
         ubusIntf_freeOutboundHandle(handle);

         return BCMRET_INVALID_ARGUMENTS;
      }

      blobmsg_add_string(handle->buf, "fullpath", fullpathArray[i]);
   }
   blobmsg_close_array(handle->buf, fullpathBlobArray);
   blobmsg_add_u8(handle->buf, "nextlevel", nextLevel);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "getParameterAttributes",
         handle->buf->head,
         callback_handleGetParameterAttrs,
         &getParamAttrsRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("getParameterAttributes invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("getParameterAttributes method returned %d (%s)", getParamAttrsRet.result, getParamAttrsRet.resp);

      if(BCMRET_SUCCESS == getParamAttrsRet.result)
      {
         /*parse returned array of table to fill paramInfoArray and numParamInfos */
         *paramAttrArray = getParamAttrsRet.paramAttrArray;
         *numParamAttrs = getParamAttrsRet.numParamAttrs;
      }
      ret = getParamAttrsRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}


BcmRet bus_out_setParameterAttributes(const ZbusAddr *destAddr,
                BcmGenericParamAttr *paramAttrArray, UINT32 numParamAttrs,
                UINT32 flags)
{
   GenericParamAttrRet setParamAttrsRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   void *paramAttrBlobArray;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   uint32_t i;
   int mret;

   bcmuLog_notice("Entered:");

   if(!destAddr || !paramAttrArray ||(0 == numParamAttrs) )
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method setParameterAttributes
   *  innner data type: table:
   * { "fullpath": string, "setAccess": boolean, "access":int16,
   * "setNotif":boolean, "notif": int16,
   * "setAltNotif":boolean, "altNotif": int16, "clearAltValue": boolean}
   *  [0] = { .name = "fullpath_setAccess_access_setNotif_notif_setAltNotif_altNotif_clearAltValue_array", .type = BLOBMSG_TYPE_ARRAY},
   *  [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   paramAttrBlobArray = blobmsg_open_array(handle->buf,
               "fullpath_setAccess_access_setNotif_notif_setAltNotif_altNotif_clearAltValue_array");
   for(i = 0; i < numParamAttrs; i++)
   {
      void * blobTable;

      if(!paramAttrArray[i].fullpath)
      {
         bcmuLog_error("Emtpy fullpath in entry %d", i);
         ubusIntf_freeOutboundHandle(handle);

         return BCMRET_INVALID_ARGUMENTS;
      }

      blobTable = blobmsg_open_table(handle->buf, NULL);

      blobmsg_add_string(handle->buf, "fullpath", paramAttrArray[i].fullpath);
      blobmsg_add_u8(handle->buf, "setAccess", paramAttrArray[i].setAccess);
      blobmsg_add_u16(handle->buf, "access", paramAttrArray[i].access);
      blobmsg_add_u8(handle->buf, "setNotif", paramAttrArray[i].setNotif);
      blobmsg_add_u16(handle->buf, "notif", paramAttrArray[i].notif);
      blobmsg_add_u8(handle->buf, "setAltNotif", paramAttrArray[i].setAltNotif);
      blobmsg_add_u16(handle->buf, "altNotif", paramAttrArray[i].altNotif);
      blobmsg_add_u8(handle->buf, "clearAltValue", paramAttrArray[i].clearAltNotifValue);

      blobmsg_close_table(handle->buf, blobTable);
   }
   blobmsg_close_array(handle->buf, paramAttrBlobArray);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "setParameterAttributes",
         handle->buf->head,
         callback_handleSetParameterAttrs,
         &setParamAttrsRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("setParameterAttributes invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("setParameterAttributes method returned %d (%s)", setParamAttrsRet.result, setParamAttrsRet.resp);
      ret = setParamAttrsRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}


BcmRet bus_out_addObject(const ZbusAddr *destAddr,
                          const char *fullpath, UINT32 flags,
                          UINT32 *instanceNumber)
{
   AddDelObjectRet addObjRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int mret;

   bcmuLog_notice("Entered:");

   if(!destAddr || !fullpath || !instanceNumber)
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   *instanceNumber = 0;

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method addObject
   *  [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   *  [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   blobmsg_add_string(handle->buf, "fullpath", fullpath);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "addObject",
         handle->buf->head,
         callback_handleAddObject,
         &addObjRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("addObject invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("addObject method returned %d (%s)", addObjRet.result, addObjRet.resp);

      if(BCMRET_SUCCESS == addObjRet.result)
      {
         *instanceNumber = addObjRet.instanceNumber;
      }
      ret = addObjRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}


BcmRet bus_out_deleteObject(const ZbusAddr *destAddr,
                             const char *fullpath, UINT32 flags)
{
   AddDelObjectRet deleteObjRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int mret;

   bcmuLog_notice("Entered:");

   if(!destAddr || !fullpath)
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method deleteObject
   *  [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   *  [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   blobmsg_add_string(handle->buf, "fullpath", fullpath);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "deleteObject",
         handle->buf->head,
         callback_handleDeleteObject,
         &deleteObjRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("deleteObject invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("deleteObject method returned %d (%s)", deleteObjRet.result, deleteObjRet.resp);
      ret = deleteObjRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}


BcmRet bus_out_getNextObject(const ZbusAddr *destAddr,
                const char *fullpath, const char *limitSubtree, UINT32 flags,
                char **nextFullpath,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   GenericParamInfoRet getNextObjectRet = { .result = BCMRET_INTERNAL_ERROR, };
   UbusOutboundHandle *handle = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int mret;

   if(!destAddr || !fullpath || !limitSubtree
               || ! nextFullpath || !paramInfoArray
               || !numParamInfos)
   {
      bcmuLog_error("invalid arguments");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_debug("Entered: fullpath=%s. flags:0x%x", fullpath, flags);

   *nextFullpath = NULL;
   *numParamInfos = 0;
   *paramInfoArray = NULL;

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   /* prepare arguments to call ubus method getNextObject
   *  inner data type: "fullpath": string
   *  [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   *  [1] = { .name = "limitSubtree", .type = BLOBMSG_TYPE_STRING},
   *  [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
   */
   blobmsg_add_string(handle->buf, "fullpath", fullpath);
   blobmsg_add_string(handle->buf, "limitSubtree", limitSubtree);
   blobmsg_add_u32(handle->buf, "flags", flags);

   mret = ubus_invoke(handle->ctx, handle->id, "getNextObject",
         handle->buf->head,
         callback_handleGetNextObject,
         &getNextObjectRet,
         TIMEOUT_DEFAULT_MDMACCESS);
   bcmuLog_notice("getNextObject invoked [%s]", ubus_strerror(mret));

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("getNextObject method returned %d (%s)", getNextObjectRet.result, getNextObjectRet.resp);

      if(BCMRET_SUCCESS == getNextObjectRet.result)
      {
         /*parse returned array of table to fill paramInfoArray and numParamInfos */
         *numParamInfos = getNextObjectRet.numParamInfos;
         *paramInfoArray = getNextObjectRet.paramInfoArray;
         *nextFullpath = getNextObjectRet.nextFullpath;
      }
      else
      {
          cmsMem_free(getNextObjectRet.nextFullpath);
          getNextObjectRet.nextFullpath = NULL;
      }
      ret = getNextObjectRet.result;
   }

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_debug("Exit. ret=%d\n", ret);
   return ret;
}

