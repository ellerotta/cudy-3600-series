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

/*!\file  ubus_mdm.c
 * \brief Various MDM dependent functions.
 */

#include <stdio.h>
#include <errno.h>

#include "bcm_zbus_intf.h"
#include "bcm_zbus_mdm.h"
#include "bcm_ubus_intf.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"
#include "cms_mem.h"

#include "libubus.h"

const struct blobmsg_policy get_param_name_in_policy[3] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING },
   [1] = { .name = "nextlevel", .type = BLOBMSG_TYPE_BOOL},
   [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy get_param_value_in_policy[3] =
{
   /* inner data type: "fullpath": string */
   [0] = { .name = "fullpath_array", .type = BLOBMSG_TYPE_ARRAY},
   [1] = { .name = "nextlevel", .type = BLOBMSG_TYPE_BOOL},
   [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy set_param_value_in_policy[2] =
{
   /*innner data type: table: { "fullpath": string, "type": string, "value": string } */
   [0] = { .name = "fullpath_type_value_array", .type = BLOBMSG_TYPE_ARRAY},
   [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy get_param_attribute_in_policy[3] =
{
   /* inner data type: "fullpath": string */
   [0] = { .name = "fullpath_array", .type = BLOBMSG_TYPE_ARRAY},
   [1] = { .name = "nextlevel", .type = BLOBMSG_TYPE_BOOL},
   [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy set_param_attribute_in_policy[2] =
{
   /*innner data type: table: { "fullpath": string, "setAccess": boolean, "access":int16,
   * "setNotif":boolean, "notif": int16,
   * "setAltNotif":boolean, "altNotif": int16, "clearAltValue": boolean}
   */
   [0] = { .name = "fullpath_setAccess_access_setNotif_notif_setAltNotif_altNotif_clearAltValue_array", .type = BLOBMSG_TYPE_ARRAY},
   [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy adddel_object_in_policy[2] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy get_next_object_in_policy[3] =
{
   [0] = { .name = "fullpath", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "limitSubtree", .type = BLOBMSG_TYPE_STRING},
   [2] = { .name = "flags", .type = BLOBMSG_TYPE_INT32},
};

const struct blobmsg_policy mdm_op_in_policy[2] =
{
   [0] = { .name = "op", .type = BLOBMSG_TYPE_STRING},
   [1] = { .name = "arg", .type = BLOBMSG_TYPE_STRING},
};

// All component MD's support these methods.
static ZbusMethodInfo compMdMethods[] =
{
   {
      .name = "getParameterNames",
      .intfHandler = ubus_in_getParameterNames,
      .policy = get_param_name_in_policy,
      .policyEntries = ARRAY_SIZE(get_param_name_in_policy),
   },

   {
      .name = "getParameterValues",
      .intfHandler = ubus_in_getParameterValues,
      .policy = get_param_value_in_policy,
      .policyEntries = ARRAY_SIZE(get_param_value_in_policy),
   },

   {
      .name = "setParameterValues",
      .intfHandler = ubus_in_setParameterValues,
      .policy = set_param_value_in_policy,
      .policyEntries = ARRAY_SIZE(set_param_value_in_policy),
   },

   {
      .name = "getParameterAttributes",
      .intfHandler = ubus_in_getParameterAttributes,
      .policy = get_param_attribute_in_policy,
      .policyEntries = ARRAY_SIZE(get_param_attribute_in_policy),
   },

   {
      .name = "setParameterAttributes",
      .intfHandler = ubus_in_setParameterAttributes,
      .policy = set_param_attribute_in_policy,
      .policyEntries = ARRAY_SIZE(set_param_attribute_in_policy),
   },

   {
      .name = "addObject",
      .intfHandler = ubus_in_addObject,
      .policy = adddel_object_in_policy,
      .policyEntries = ARRAY_SIZE(adddel_object_in_policy),
   },

   {
      .name = "deleteObject",
      .intfHandler = ubus_in_deleteObject,
      .policy = adddel_object_in_policy,
      .policyEntries = ARRAY_SIZE(adddel_object_in_policy),
   },

   {
      .name = "getNextObject",
      .intfHandler = ubus_in_getNextObject,
      .policy = get_next_object_in_policy,
      .policyEntries = ARRAY_SIZE(get_next_object_in_policy),
   },

   {
      .name = "mdmOp",
      .intfHandler = ubus_in_mdmOp,
      .policy = mdm_op_in_policy,
      .policyEntries = ARRAY_SIZE(mdm_op_in_policy),
   },

};


BcmRet busIntf_addCompMdMethods(ZbusMethodContext *ctx)
{
   int numMethods = sizeof(compMdMethods)/sizeof(ZbusMethodInfo);
   int i;
   BcmRet ret;

   // Go through our array and use the zbusIntf to add one-by-one.
   for (i = 0; i < numMethods; i++)
   {
      ret = zbusIntf_addMethod(ctx, &(compMdMethods[i]));
      BCM_RETURN_IF_NOT_SUCCESS(ret);
   }

   // Also add the notify method, which is in bcm_dbus_intf (non-MDM)
   ret = zbusIntf_addNotifyMethod(ctx);

   bcmuLog_notice("added %d methods, num=%d (max=%d)",
                  numMethods+1, ctx->numMethods, ctx->maxMethods);
   return ret;
}


void ubus_in_mdmOp(const void *ctx, const void *req, void *method __attribute__((unused)), void *msg)
{
   struct blob_buf blobBuffer = {};
   struct blob_attr *blogMsg = (struct blob_attr *)msg;
   struct blob_attr *argsTable[ARRAY_SIZE(mdm_op_in_policy)] = {};

   char *op;
   char *args;
   BcmRet ret = BCMRET_INVALID_ARGUMENTS;
   char *retData = NULL;

   bcmuLog_debug("Entered:");

   if(UBUS_STATUS_OK != blob_buf_init(&blobBuffer, 0))
   {
      bcmuLog_error("Failed to init blobBuffer!");
      return;  //Just return, can't send ubus reply
   }

   if(0 != blobmsg_parse(mdm_op_in_policy, ARRAY_SIZE(mdm_op_in_policy),
            argsTable,
            blob_data(blogMsg),
            blob_len(blogMsg)))
   {
      goto EXIT_MDMOP;
   }

   op = blobmsg_get_string(argsTable[0]);
   args = blobmsg_get_string(argsTable[1]);
   bcmuLog_notice("mdmOp=%s args=%s", op, args);

   /*
   * Call Z-Bus generic method to do the real work.
   */
   ret = zbus_in_mdmOp(op, args, &retData);

EXIT_MDMOP:
   blobmsg_add_string(&blobBuffer, "retData", retData? retData : "");
   blobmsg_add_u32(&blobBuffer, "result", ret);
   ubus_send_reply((struct ubus_context *)ctx, (struct ubus_request_data *)req, blobBuffer.head);

   blob_buf_free(&blobBuffer);
   if(retData)
      cmsMem_free(retData);

   bcmuLog_debug("Exit, ret=%d", ret);

   return;
}
