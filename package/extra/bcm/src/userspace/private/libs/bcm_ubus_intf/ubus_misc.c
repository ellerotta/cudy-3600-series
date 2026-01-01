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

/*!\file  dbus_misc.c
 * \brief Various MDM independent methods for ubus.
 */

#include <stdio.h>
#include <errno.h>

#include "number_defs.h"
#include "os_defs.h"
#include "cms_mem.h"
#include "bdk_dbus.h"
#include "bcm_zbus_intf.h"
#include "bcm_ubus_intf.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"

#include "blobmsg_json.h"
#include "libubus.h"

enum
{
   SU_RET_STRDATA,
   SU_RET_BCMRET,
   SU_RET_MAX
};

enum
{
   U_RET_DATA,
   U_RET_MAX
};

enum
{
   NOTIFY_EVENT_TYPE,
   NOTIFY_EVENT_EID,
   NOTIFY_EVENT_STRDATA,
   NOTIFY_EVENT_MAX
};

enum
{
   FIREWALL_RESULT,
   FIREWALL_ERRSTR,
   FIREWALL_MAX
};

enum
{
   MDM_OP_DATA,
   MDM_OP_RESULT,
   MDM_OP_MAX
};


typedef struct
{
   char *strDataOut;
   uint32_t result;
}SUResult;

typedef struct
{
   uint32_t result;
   char *errorStr;
}FireWallCtlRet;

/*Policies of output parameters */
static const struct blobmsg_policy su_result_policy[SU_RET_MAX] =
{
   [SU_RET_STRDATA] = { .name = "strDataOut", .type = BLOBMSG_TYPE_STRING },
   [SU_RET_BCMRET] = { .name = "result", .type = BLOBMSG_TYPE_INT32 },
};

static const struct blobmsg_policy u_result_policy[1] =
{
   [0] = { .name = "result", .type = BLOBMSG_TYPE_INT32 },
};

static const struct blobmsg_policy notify_event_policy[NOTIFY_EVENT_MAX] =
{
   [NOTIFY_EVENT_TYPE] = { .name = "eventType", .type = BLOBMSG_TYPE_INT32 },
   [NOTIFY_EVENT_EID] = { .name = "eid", .type = BLOBMSG_TYPE_INT32 },
   [NOTIFY_EVENT_STRDATA] = { .name = "strData", .type = BLOBMSG_TYPE_STRING },
};

static const struct blobmsg_policy firewallCtl_policy[FIREWALL_MAX] =
{
   [FIREWALL_RESULT] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
   [FIREWALL_ERRSTR] = { .name = "errorStr", .type = BLOBMSG_TYPE_STRING},
};

static const struct blobmsg_policy mdmOp_policy[MDM_OP_MAX] =
{
   [MDM_OP_DATA] = { .name = "retData", .type = BLOBMSG_TYPE_STRING},
   [MDM_OP_RESULT] = { .name = "result", .type = BLOBMSG_TYPE_INT32},
};


static void ubus_in_notifyEvent(const void *ctx, const void *req, void *method, void *msg);

static ZbusMethodInfo notifyMethod =
{
   .name = "notifyEvent",
   .policy = (void *)notify_event_policy,
   .intfHandler = ubus_in_notifyEvent,
   .policyEntries = ARRAY_SIZE(notify_event_policy),
};

/* Method call handler. result is [string, u32] */
static void callback_handle_su(struct ubus_request *req, int type __attribute__((unused)), struct blob_attr *msg)
{
   struct blob_attr *table[SU_RET_MAX];
   SUResult *result = req->priv;

   if(!result)
   {
      bcmuLog_error("no private data to hold response");
      return;
   }

   blobmsg_parse(su_result_policy, ARRAY_SIZE(su_result_policy), table, blob_data(msg), blob_len(msg));
   result->strDataOut = (table[SU_RET_STRDATA])? blobmsg_get_string(table[SU_RET_STRDATA]) : NULL;
   result->result = (table[SU_RET_BCMRET])? blobmsg_get_u32(table[SU_RET_BCMRET]) : BCMRET_INTERNAL_ERROR;
}

/* Method call handler. result is u32 only */
static void callback_handle_u(struct ubus_request *req, int type __attribute__((unused)), struct blob_attr *msg)
{
   struct blob_attr *table[U_RET_MAX];
   uint32_t *result = req->priv;

   if(!result)
   {
      bcmuLog_error("no private data to hold response");
      return;
   }

   blobmsg_parse(u_result_policy, ARRAY_SIZE(u_result_policy), table, blob_data(msg), blob_len(msg));
   *result = (table[U_RET_DATA])? blobmsg_get_u32(table[U_RET_DATA]) : BCMRET_INTERNAL_ERROR;
}

/* Method call handler. result is u32 only */
static void callback_handle_firewallCtl(struct ubus_request *req, int type __attribute__((unused)), struct blob_attr *msg)
{
   struct blob_attr *table[FIREWALL_MAX];
   FireWallCtlRet *ret = req->priv;

   if(!ret)
   {
      bcmuLog_error("no private data to hold response");
      return;
   }

   blobmsg_parse(firewallCtl_policy, ARRAY_SIZE(firewallCtl_policy), table, blob_data(msg), blob_len(msg));
   ret->result = (table[FIREWALL_RESULT])? blobmsg_get_u32(table[FIREWALL_RESULT]) : BCMRET_INTERNAL_ERROR;
   ret->errorStr = (table[FIREWALL_ERRSTR])? blobmsg_get_string(table[FIREWALL_ERRSTR]) : NULL;
}

/* Method call handler. result is [string, u32] */
static void callback_handle_mdmOp(struct ubus_request *req, int type __attribute__((unused)), struct blob_attr *msg)
{
   struct blob_attr *table[SU_RET_MAX];
   SUResult *result = req->priv;	//use same structure to get result

   if(!result)
   {
      bcmuLog_error("no private data to hold response");
      return;
   }

   blobmsg_parse(mdmOp_policy, ARRAY_SIZE(mdmOp_policy), table, blob_data(msg), blob_len(msg));
   result->strDataOut = (table[SU_RET_STRDATA])? blobmsg_get_string(table[SU_RET_STRDATA]) : NULL;
   result->result = (table[SU_RET_BCMRET])? blobmsg_get_u32(table[SU_RET_BCMRET]) : BCMRET_INTERNAL_ERROR;
}


/*prototype of sys_directory's subscribeEvent:
* @parameters:
* input:
*  eventType: u32
*  compName: string
*  eid: u32
*  flags: u32
*  strDataIn: string
* output:
*  strDataIn: string
*  result: u32
*/
BcmRet bus_out_subscribeEvent(const char *compName, CmsEntityId eid,
      UINT32 eventType, UINT32 flags,
      const char *strReq, char **strResp)
{
   UbusOutboundHandle *handle;
   const ZbusAddr *destAddr = NULL;
   SUResult result;
   BcmRet ret = BCMRET_SUCCESS;
   int mret;

   if (compName == NULL)
   {
      bcmuLog_error("one or more NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: compName=%s[%d] event: %d flags=0x%x.",
         compName, eid,
         eventType, flags);

   destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_u32(handle->buf, "eventType", eventType);
   blobmsg_add_string(handle->buf, "compName", compName);
   blobmsg_add_u32(handle->buf, "eid", eid);
   blobmsg_add_u32(handle->buf, "flags", flags);
   blobmsg_add_string(handle->buf, "strDataIn", strReq);

   mret = ubus_invoke(handle->ctx, handle->id, "subscribeEvent",
         handle->buf->head,
         callback_handle_su,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);
   bcmuLog_notice("subscribeEvent invoked [%s]", ubus_strerror(mret));
   if(UBUS_STATUS_OK == mret)
   {
      *strResp = cmsMem_strdup(result.strDataOut);
      ret = result.result;
      bcmuLog_debug("Event subscribed: %s. %d", *strResp, ret);
   }
   else
      ret = BCMRET_INTERNAL_ERROR;

   ubusIntf_freeOutboundHandle(handle);

   return ret;
}

/*prototype of sys_directory's getEventStatus:
* @parameters:
* input:
*  eventType: u32
*  flags: u32
*  strDataIn: string
* output:
*  strDataIn: string
*  result: u32
*/
BcmRet bus_out_getEventStatus(UINT32 eventType, const char *strReq, char **strResp)
{
   UbusOutboundHandle *handle;
   const ZbusAddr *destAddr = NULL;
   SUResult result;
   BcmRet ret = BCMRET_SUCCESS;
   int mret;

   if((NULL == strResp) || (NULL == strReq))
   {
      bcmuLog_error("one or more NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: eventType=%d request=%s", eventType, strReq);

   destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_u32(handle->buf, "eventType", eventType);
   blobmsg_add_u32(handle->buf, "flags", 0);
   blobmsg_add_string(handle->buf, "strDataIn", strReq);

   mret = ubus_invoke(handle->ctx, handle->id, "getEventStatus",
         handle->buf->head,
         callback_handle_su,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);
   bcmuLog_notice("getEventStatus invoked [%s]", ubus_strerror(mret));
   if(UBUS_STATUS_OK == mret)
   {
      *strResp = cmsMem_strdup(result.strDataOut);
      ret = result.result;
      bcmuLog_debug("Event status get: %s. %d", *strResp, ret);
   }
   else
      ret = BCMRET_INTERNAL_ERROR;

   ubusIntf_freeOutboundHandle(handle);

   return ret;
}

/*prototype of sys_directory's queryEventStatus:
* @parameters:
* input:
*  eventType: u32
*  flags: u32
*  strDataIn: string
* output:
*  strDataIn: string
*  result: u32
*/
BcmRet bus_out_queryEventStatus(UINT32 eventType, UINT32 flags __attribute__((unused)),
                                const char *strReq, char **strResp)
{
   UbusOutboundHandle *handle;
   const ZbusAddr *destAddr = NULL;
   SUResult result;
   BcmRet ret = BCMRET_SUCCESS;
   int mret;

   if((NULL == strResp) || (NULL == strReq))
   {
      bcmuLog_error("one or more NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }
   *strResp = NULL;

   bcmuLog_notice("Entered: eventType=%d request=%s", eventType, strReq);

   destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_u32(handle->buf, "eventType", eventType);
   blobmsg_add_u32(handle->buf, "flags", 0);
   blobmsg_add_string(handle->buf, "strDataIn", strReq);

   // call sys_directory on U-Bus
   mret = ubus_invoke(handle->ctx, handle->id, "queryEventStatus",
         handle->buf->head,
         callback_handle_su,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);
   bcmuLog_notice("queryEventStatus mret=%s", ubus_strerror(mret));
   if(UBUS_STATUS_OK == mret)
   {
      *strResp = cmsMem_strdup(result.strDataOut);
      ret = result.result;
      bcmuLog_debug("queryEventStatus returned: %s (ret=%d)", *strResp, ret);
   }
   else
   {
      ret = BCMRET_INTERNAL_ERROR;
   }

   ubusIntf_freeOutboundHandle(handle);
   return ret;
}

/*prototype of sys_directory's publishEvent:
* @parameters:
* input:
*  eventType: u32
*  compName: string
*  eid: u32
*  flags: u32
*  strDataIn: string
* output:
*  result: u32
*/
BcmRet bus_out_publishEvent(const char *compName, UINT32 src,
                            UINT32 eventType, UINT32 flags,
                            const char *strDataOut)
{
   UbusOutboundHandle *handle;
   const ZbusAddr *destAddr = NULL;
   uint32_t result;
   BcmRet ret = BCMRET_SUCCESS;
   int mret;

   if((NULL == compName) || (NULL == strDataOut))
   {
      bcmuLog_error("one or more NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: compName=%s src=%d eventType=%d flags=0x%x strDataOut=%s",
                  compName, src, eventType, flags, strDataOut);

   destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_u32(handle->buf, "eventType", eventType);
   blobmsg_add_string(handle->buf, "compName", compName);
   blobmsg_add_u32(handle->buf, "eid", src);
   blobmsg_add_u32(handle->buf, "flags", flags);
   blobmsg_add_string(handle->buf, "strDataIn", strDataOut);

   mret = ubus_invoke(handle->ctx, handle->id, "publishEvent",
         handle->buf->head,
         callback_handle_u,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_notice("publishEvent invoked [%s]", ubus_strerror(mret));
   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("Event published: %d", result);
      ret = result;
   }
   else
      ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_debug("Exit: ret=%d", ret);
   return ret;
}

/*prototype of notifyEvent:
* @parameters:
* input:
*  eventType: u32
*  eid: u32
*  strData: string
* output:
*  result: u32
*/
BcmRet bus_out_notifyEvent(const ZbusAddr *destAddr,
                           UINT32 eventType, UINT32 eid, const char *strData)
{
   UbusOutboundHandle *handle;
   uint32_t result = 0;
   BcmRet ret = BCMRET_SUCCESS;
   int mret = 0;

   if((NULL == destAddr) || (NULL == strData))
   {
      bcmuLog_error("one or more NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: dest=%s eventType=0x%x eid=%d strData=%s",
                 destAddr->compName, eventType, eid, strData);

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_u32(handle->buf, "eventType", eventType);
   blobmsg_add_u32(handle->buf, "eid", eid);
   blobmsg_add_string(handle->buf, "strData", strData);

   mret = ubus_invoke(handle->ctx, handle->id, "notifyEvent",
         handle->buf->head,
         callback_handle_u,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);

   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("Event notified: result=%d mret=%d", result, mret);
      ret = result;
   }
   else
   {
      bcmuLog_error("notifyEvent error (result=%d mret=%d) %s ",
                    result, mret, ubus_strerror(mret));
      ret = BCMRET_INTERNAL_ERROR;
   }

   ubusIntf_freeOutboundHandle(handle);

   return ret;
}

static void ubus_in_notifyEvent(const void *ctx __attribute__((unused)), const void *req __attribute__((unused)), void *method __attribute__((unused)), void *msg)
{
   struct blob_attr *table[NOTIFY_EVENT_MAX];
   struct blob_attr *attrs = msg;
   uint32_t eventType = 0;  //please refer to PubSubEventType
   uint32_t eid = 0;
   char *strDataIn = NULL;

   bcmuLog_debug("Entered:");

   blobmsg_parse(notify_event_policy, ARRAY_SIZE(notify_event_policy), table, blob_data(attrs), blob_len(attrs));
   if(table[NOTIFY_EVENT_TYPE])
      eventType = blobmsg_get_u32(table[NOTIFY_EVENT_TYPE]);
   if(table[NOTIFY_EVENT_EID])
      eid = blobmsg_get_u32(table[NOTIFY_EVENT_EID]);
   if(table[NOTIFY_EVENT_STRDATA])
      strDataIn = blobmsg_get_string(table[NOTIFY_EVENT_STRDATA]);

   if(_zbusConfig.processNotifyEventFp != NULL)
   {
      CmsMsgHeader *cmsg=NULL;
      cmsg = zbusUtil_createEventNotificationMsg(eventType, eid, strDataIn);
      if(NULL != cmsg)
      {
         (*_zbusConfig.processNotifyEventFp)(_zbusConfig.msgData, cmsg);
      }
   }
   else
   {
      bcmuLog_error("processNotifyEventFp is NULL, cannot pass along "
            "notification event %d strData=%s", eventType, strDataIn);
   }
}

BcmRet busIntf_addNotifyMethod(ZbusMethodContext *ctx)
{
   return (zbusIntf_addMethod(ctx, &notifyMethod));
}

/*prototype of bcm_setHostIptablesRule:
* @parameters:
* input:
*  name: string
*  rule: string
* output:
*  result: u32
*  errorStr: string
*/
BcmRet bus_out_firewallCtl(const ZbusAddr *destAddr, const char *compName,
                           const char *iptablesStr)
{
   UbusOutboundHandle *handle;
   FireWallCtlRet result;
   BcmRet ret = BCMRET_SUCCESS;
   int mret;

   if((NULL == destAddr) || (NULL == iptablesStr))
   {
      bcmuLog_error("one or more NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: ");

   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_string(handle->buf, "name", compName);
   blobmsg_add_string(handle->buf, "rule", iptablesStr);

   mret = ubus_invoke(handle->ctx, handle->id, "bcm_setHostIptablesRule",
         handle->buf->head,
         callback_handle_firewallCtl,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);
   bcmuLog_notice("notifyEvent invoked [%s]", ubus_strerror(mret));
   if(UBUS_STATUS_OK == mret)
   {
      bcmuLog_debug("bcm_setIptablesRule dbus method returned %d (%s)",
            result.result, result.errorStr);
      ret = result.result;
   }
   else
      ret = BCMRET_INTERNAL_ERROR;

   ubusIntf_freeOutboundHandle(handle);

   return ret;
}


/*prototype of mdmOp:
* @parameters:
* input:
*  op: string
*  arg: string
* output:
*  retData: string
*  result: u32
*/
BcmRet bus_out_mdmOp(const ZbusAddr *destAddr,
                     const char *op, const char *arg, char **data)
{
   UbusOutboundHandle *handle;
   const char *outArg;
   SUResult result;
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int mret;

   outArg = (arg != NULL) ? arg : "";

   bcmuLog_notice("Entered: destAddr=%s op=%s outArg=%s",
         destAddr->compName, op, outArg);


   handle = ubusIntf_getOutboundHandle(destAddr);
   if(NULL == handle)
   {
      bcmuLog_error("%s offline", destAddr->busName);
      return BCMRET_INTERNAL_ERROR;
   }

   blobmsg_add_string(handle->buf, "op", op);
   blobmsg_add_string(handle->buf, "arg", outArg);

   mret = ubus_invoke(handle->ctx, handle->id, "mdmOp",
         handle->buf->head,
         callback_handle_mdmOp,
         &result,
         TIMEOUT_DEFAULT_METHOD_CALL);

   ubusIntf_freeOutboundHandle(handle);

   bcmuLog_notice("mdmOp invoked [%s]", ubus_strerror(mret));
   if(UBUS_STATUS_OK == mret)
   {
      char *retData = result.strDataOut;
      uint32_t retDataLen = strlen(retData);

      ret = result.result;
      bcmuLog_notice("returned data len=%d (data=%s)", retDataLen, retData);

      // See comments in bcm_dbus_intf/dbus_misc.c
      if (data != NULL)
      {
         *data = cmsMem_alloc(retDataLen+1,ALLOC_ZEROIZE);
         if (*data != NULL)
         {
            strcpy(*data,retData);
         }
         else
         {
            ret = BCMRET_RESOURCE_EXCEEDED;
            bcmuLog_error("Could not alloc buffer to hold config read of %d bytes", retDataLen);
         }
      }
   }

   return ret;
}

