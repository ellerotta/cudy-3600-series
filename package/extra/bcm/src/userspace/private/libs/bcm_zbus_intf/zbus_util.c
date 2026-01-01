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


/*!\file zbus_util.c
 * \brief Zbus utility functions
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "json_tokener.h"
#include "json_util.h"

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_util.h"

BcmRet zbusUtil_parseKeyValueString(const char *kvStr,
                                    char *keyBuf, UINT32 keyBufLen,
                                    char **valuePtr)
{
   UINT32 i=0;

   if (kvStr == NULL || keyBuf == NULL)
   {
      bcmuLog_error("NULL input args");
      return BCMRET_INVALID_ARGUMENTS;
   }

   while (kvStr[i] != '=' && kvStr[i] != '\0' && i < keyBufLen-1)
   {
      keyBuf[i] = kvStr[i];
      i++;
   }

   if (kvStr[i] == '\0')
   {
      // kvStr only contains the key part, which is allowed.
      keyBuf[i] = '\0';
      if (valuePtr)
         *valuePtr = (char *) &(kvStr[i]);
      return BCMRET_SUCCESS;
   }
   else if (kvStr[i] == '=')
   {
      keyBuf[i] = '\0';
      // skip over = and continue processing
      i++;
   }
   else if (i >= keyBufLen-1)
   {
      bcmuLog_error("key too long for buf, key=%s keyBufLen=%d",
                    kvStr, keyBufLen);
      memset(keyBuf, 0, keyBufLen);
      return BCMRET_INVALID_ARGUMENTS;
   }
   else
   {
      bcmuLog_error("Unknown condition, fail parsing");
      memset(keyBuf, 0, keyBufLen);
      return BCMRET_INTERNAL_ERROR;
   }

   if (valuePtr)
      *valuePtr = (char *) &(kvStr[i]);

   return BCMRET_SUCCESS;
}

// Convert msg to a string suitable to transmiting over bus.
// Caller is responsible for freeing the string (dataStr).
char *zbusUtil_createOutboundEventDataStr(const CmsMsgHeader *msgReq)
{
   UINT32 eventType = msgReq->wordData;
   char *dataStr=NULL;

   bcmuLog_debug("Entered: msgType=0x%x eventType=%d", msgReq->type, eventType);

   if (eventType == PUBSUB_EVENT_KEY_VALUE)
   {
      const PubSubKeyValueMsgBody *kvBody=(PubSubKeyValueMsgBody *)(msgReq+1);
      UINT32 len = sizeof(kvBody->key) + kvBody->valueLen + 1; // for '='

      dataStr = cmsMem_alloc(len, ALLOC_ZEROIZE);
      if (dataStr == NULL)
      {
         bcmuLog_error("cmsMem_alloc failed, len=%d", len);
      }
      else
      {
         if (kvBody->valueLen == 0)
         {
            // In some cases (e.g. SUBSCRIBE_EVENT or GET_EVENT_STATUS),
            // only key is needed.
            snprintf(dataStr, len, "%s", kvBody->key);
         }
         else
         {
            snprintf(dataStr, len, "%s=%s", kvBody->key, (char *) (kvBody+1));
         }
      }
   }
   else if (eventType == PUBSUB_EVENT_NAMESPACE)
   {
      const PubSubNamespaceMsgBody *nsBody=(PubSubNamespaceMsgBody *)(msgReq+1);
      UINT32 len = sizeof(PubSubNamespaceMsgBody) + 1; // for '='

      dataStr = cmsMem_alloc(len, ALLOC_ZEROIZE);
      if (dataStr == NULL)
      {
         bcmuLog_error("cmsMem_alloc failed, len=%d", len);
      }
      else
      {
         snprintf(dataStr, len, "%s=%s", nsBody->namespc, nsBody->ownerCompName);
      }
   }
   else if (eventType == PUBSUB_EVENT_INTERFACE)
   {
      dataStr = zbusUtil_encodePubSubInterfaceMsgBodyToJson((PubSubInterfaceMsgBody *)(msgReq+1));
      if (dataStr == NULL)
      {
         bcmuLog_error("Could not encode PubSubInterfaceMsgBody to JSON!");
      }
   }
   else
   {
      bcmuLog_error("Unsupported eventType %d", eventType);
   }

   return dataStr;
}


// Convert strData to a response msg.
// Caller is responsible for freeing the returned msg buffer (msgResp)
CmsMsgHeader *zbusUtil_createEventRespMsg(const CmsMsgHeader *msgReq,
                                          const char *strData)
{
   CmsMsgHeader *msgResp=NULL;
   UINT32 eventType=0;
   UINT32 len=0;

   if (msgReq == NULL || strData == NULL)
   {
      bcmuLog_error("NULL input params");
      return NULL;
   }
   eventType = msgReq->wordData;

   bcmuLog_debug("Entered: eventType=%d strData=%s", eventType, strData);

   if (eventType == PUBSUB_EVENT_KEY_VALUE)
   {
      PubSubKeyValueMsgBody *kvBody=NULL;
      char *valuePtr=NULL;
      BcmRet ret;

      // len is more than what is strictly needed, but easier to calculate
      len = sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody) + strlen(strData);
      msgResp = (CmsMsgHeader *) cmsMem_alloc(len, ALLOC_ZEROIZE);
      if (msgResp == NULL)
      {
         bcmuLog_error("Could not alloc msg buf of %d bytes", len);
         return NULL;
      }

      kvBody = (PubSubKeyValueMsgBody *)(msgResp+1);
      ret = zbusUtil_parseKeyValueString(strData,
                                kvBody->key, sizeof(kvBody->key), &valuePtr);
      if (ret != BCMRET_SUCCESS)
      {
         cmsMem_free(msgResp);
         return NULL;
      }
      if (valuePtr != NULL)
      {
         strcpy((char *)(kvBody+1), valuePtr);  // value follows kvBody
         kvBody->valueLen = strlen(valuePtr) + 1;  // valueLen includes null term char
      }
      else
      {
         kvBody->valueLen = 1;  // just null term
      }
      msgResp->dataLength = sizeof(PubSubKeyValueMsgBody) + kvBody->valueLen;
   }
   else if (eventType == PUBSUB_EVENT_NAMESPACE)
   {
      PubSubNamespaceMsgBody *nsBody=NULL;
      char *valuePtr=NULL;
      BcmRet ret;

      len = sizeof(CmsMsgHeader) + sizeof(PubSubNamespaceMsgBody);
      msgResp = (CmsMsgHeader *) cmsMem_alloc(len, ALLOC_ZEROIZE);
      if (msgResp == NULL)
      {
         bcmuLog_error("Could not alloc msg buf of %d bytes", len);
         return NULL;
      }


      // TODO: Not clear if this is really the right thing to do, but oks
      // for now.  See comments in createNotificationMsg below.
      nsBody=(PubSubNamespaceMsgBody *)(msgResp+1);
      ret = zbusUtil_parseKeyValueString(strData,
                         nsBody->namespc, sizeof(nsBody->namespc), &valuePtr);
      if (ret != BCMRET_SUCCESS)
      {
         cmsMem_free(msgResp);
         return NULL;
      }
      strncpy(nsBody->ownerCompName, valuePtr, sizeof(nsBody->ownerCompName)-1);
      msgResp->dataLength = sizeof(PubSubNamespaceMsgBody);
   }
   else if ((eventType == PUBSUB_EVENT_INTERFACE) &&
            (msgReq->type == CMS_MSG_QUERY_EVENT_STATUS))
   {
      // Queries are handled a bit differently.  The return data is a string
      // that follows the CMS msg header, no additional structures or encodings.
      len = sizeof(CmsMsgHeader) + strlen(strData) + 1;
      msgResp = (CmsMsgHeader *) cmsMem_alloc(len, ALLOC_ZEROIZE);
      if (msgResp == NULL)
      {
         bcmuLog_error("Could not alloc msg buf of %d bytes", len);
         return NULL;
      }

      strcpy((char *)(msgResp+1), strData);
      msgResp->dataLength = strlen(strData) + 1;
   }
   else if (eventType == PUBSUB_EVENT_INTERFACE)
   {
      UINT32 totalLen = 0;
      PubSubInterfaceMsgBody *intfMsgBody = NULL;

      // TODO: this function is similar to createEventNotificationMsg.  Merge?
      // Decode the JSON string into the C struct.
      intfMsgBody = zbusUtil_decodeJsonToPubSubInterfaceMsgBody(strData);
      if (intfMsgBody == NULL)
      {
          bcmuLog_error("Could not parse InterfaceMsgBody JSON string %s",
                        strData);
          return NULL;
      }

      // Allocate a buffer for the entire msg.
      totalLen = sizeof(CmsMsgHeader) + sizeof(PubSubInterfaceMsgBody) +
                 intfMsgBody->additionalDataLen;
      msgResp = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
      if (msgResp == NULL)
      {
         bcmuLog_error("message buf allocation failed, len=%d", totalLen);
         CMSMEM_FREE_BUF_AND_NULL_PTR(intfMsgBody);
         return NULL;
      }

      // Copy the InterfaceMsgBody into the returned msgBody, including
      // any additional data, which is included in the msg returned by
      // zbusUtil_decodeJsonToPubSubInterfaceMsgBody.
      msgResp->dataLength = totalLen - sizeof(CmsMsgHeader);
      memcpy((void *) (msgResp+1), (void *) intfMsgBody, msgResp->dataLength);

      CMSMEM_FREE_BUF_AND_NULL_PTR(intfMsgBody);
   }
   else
   {
      bcmuLog_error("Unsupported eventType %d", eventType);
      return NULL;
   }

   // Flip src and dest around in the response msg header
   msgResp->type = msgReq->type;
   msgResp->src = msgReq->dst;
   msgResp->dst = msgReq->src;
   msgResp->flags_response = 1;
   msgResp->wordData = msgReq->wordData;
   return msgResp;
}


// Allocate and return a CMS Msg based on input data.
// Caller is responsible for freeing the message.
CmsMsgHeader *zbusUtil_createEventNotificationMsg(UINT32 eventType, UINT32 eid,
                                                  const char *strData)
{
   CmsMsgHeader *msg=NULL;
   BcmRet ret;

   if (strData == NULL)
   {
      bcmuLog_error("NULL strData!");
      return NULL;
   }

   if (eventType == PUBSUB_EVENT_KEY_VALUE)
   {
      PubSubKeyValueMsgBody *kvBody=NULL;
      char *valuePtr=NULL;
      UINT32 valueLen = strlen(strData)+1;  // being generous, strData includes the key
      UINT32 totalLen = sizeof(CmsMsgHeader) + sizeof(PubSubKeyValueMsgBody) + valueLen;

      msg = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
      if (msg == NULL)
      {
         bcmuLog_error("Could not alloc msg buf of %d bytes", totalLen);
         return NULL;
      }

      // Fill in header
      msg->type = CMS_MSG_NOTIFY_EVENT;
      msg->src = EID_SMD;
      msg->dst = eid;
      msg->flags_event = 1;
      msg->wordData = eventType;

      kvBody = (PubSubKeyValueMsgBody *) (msg+1);
      ret = zbusUtil_parseKeyValueString(strData,
                                 kvBody->key, sizeof(kvBody->key), &valuePtr);
      if (ret != BCMRET_SUCCESS)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
         return NULL;
      }

      // copy the value
      if (valuePtr != NULL)
      {
         snprintf((char *)(kvBody+1), valueLen, "%s", valuePtr);
         kvBody->valueLen = strlen(valuePtr)+1;
      }
      else
      {
         kvBody->valueLen = 1;  // just null term
      }
      msg->dataLength = sizeof(PubSubKeyValueMsgBody) + kvBody->valueLen;

      return msg;
   }
   else if (eventType == PUBSUB_EVENT_NAMESPACE)
   {
      // Just send out a cms message notifying that namespace has changed.
      // The receiver should update its namespaces with a get operation.
      msg = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
      if (msg == NULL)
      {
         bcmuLog_error("Could not alloc msg buf");
         return NULL;
      }

      msg->type = CMS_MSG_NOTIFY_EVENT;
      msg->src = EID_SMD;
      msg->dst = eid;
      msg->flags_event = 1;
      msg->wordData = eventType;
      msg->dataLength = 0;
      return msg;
   }
   else if (eventType == PUBSUB_EVENT_INTERFACE)
   {
      UINT32 totalLen = 0;
      PubSubInterfaceMsgBody *intfMsgBody = NULL;

      // Decode the JSON string into the C struct.
      intfMsgBody = zbusUtil_decodeJsonToPubSubInterfaceMsgBody(strData);
      if (intfMsgBody == NULL)
      {
          bcmuLog_error("Could not parse InterfaceMsgBody JSON string %s",
                        strData);
          return NULL;
      }

      // Allocate a buffer for the entire msg.
      totalLen = sizeof(CmsMsgHeader) + sizeof(PubSubInterfaceMsgBody) +
                 intfMsgBody->additionalDataLen;
      msg = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
      if (msg == NULL)
      {
         bcmuLog_error("Could not alloc msg buf of %d bytes", totalLen);
         CMSMEM_FREE_BUF_AND_NULL_PTR(intfMsgBody);
         return NULL;
      }

      // Fill in header
      msg->type = CMS_MSG_NOTIFY_EVENT;
      msg->src = EID_SMD;
      msg->dst = eid;
      msg->flags_event = 1;
      msg->wordData = eventType;
      msg->dataLength = totalLen - sizeof(CmsMsgHeader);

      // Copy the InterfaceMsgBody into the returned msgBody.
      memcpy((void *) (msg+1), (void *) intfMsgBody, msg->dataLength);

      CMSMEM_FREE_BUF_AND_NULL_PTR(intfMsgBody);
      return msg;
   }
   else
   {
      bcmuLog_error("Unsupported eventType %d", eventType);
   }

   return NULL;
}


// Caller is responsible for freeing the returned msg buffer (msgResp)
CmsMsgHeader *zbusUtil_createProxyGetParamRespMsg(const CmsMsgHeader *msgReq,
                                                  const char *fullpath __attribute__((unused)),
                                                  const char *type __attribute__((unused)),
                                                  const char *value)
{
   CmsMsgHeader *msgResp = NULL;
   char *body = NULL;
   UINT32 len = 0;

   if (msgReq == NULL)
   {
      bcmuLog_error("NULL input params");
      return NULL;
   }

   len = sizeof(CmsMsgHeader) + strlen(value) + 1;
   msgResp = (CmsMsgHeader *) cmsMem_alloc(len, ALLOC_ZEROIZE);
   if (msgResp == NULL)
   {
       bcmuLog_error("Could not alloc msg buf of %d bytes", len);
       return NULL;
   }

   msgResp->dataLength = len - sizeof(CmsMsgHeader);
   body = (char *)(msgResp + 1);
   strcpy(body, value);

   // Flip src and dest around in the response msg header
   msgResp->type = msgReq->type;
   msgResp->src = msgReq->dst;
   msgResp->dst = msgReq->src;
   msgResp->flags_response = 1;
   return msgResp;
}


static void addObjToJsonObj(json_object *allParamsObj,
                            const char *paramName, json_object *paramObj)
{
   int rc;

   if (paramObj == NULL)
   {
      bcmuLog_error("given null obj to add for param %s", paramName);
      return;
   }

   // the add operation will transfer ownership of paramObj to allParamsObj
   rc = json_object_object_add(allParamsObj, paramName, paramObj);
   if (rc)
   {
      bcmuLog_error("object_add failed for param %s", paramName);
   }

   return;
}

char *zbusUtil_encodePubSubInterfaceMsgBodyToJson(const PubSubInterfaceMsgBody *intfMsgBody)
{
   json_object *allParamsObj;
   char *jsonStr=NULL;

   // Allocate the top level container object for all params.
   allParamsObj = json_object_new_object();

   // The JSON string should use the EXACT field names as PubSubInterfaceMsgBody
   // Empty or null strings, 0 int, and false booleans are omitted from the
   // JSON string, which means their values should be set to empty/null, 0, or
   // false on the receiving side.  Lack of param does NOT mean the reciever
   // can keep the current value on his side.
   if (!IS_EMPTY_STRING(intfMsgBody->intfName))
   {
      addObjToJsonObj(allParamsObj, "intfName",
                      json_object_new_string(intfMsgBody->intfName));
   }
   else
   {
      bcmuLog_error("intfMsgBody has empty intfName!");
      json_object_put(allParamsObj);
      return NULL;
   }

   if (intfMsgBody->isUpstream)
      addObjToJsonObj(allParamsObj, "isUpstream",
                      json_object_new_boolean(intfMsgBody->isUpstream));
   if (intfMsgBody->isLayer2)
      addObjToJsonObj(allParamsObj, "isLayer2",
                      json_object_new_boolean(intfMsgBody->isLayer2));
   if (intfMsgBody->isLayer3)
      addObjToJsonObj(allParamsObj, "isLayer3",
                      json_object_new_boolean(intfMsgBody->isLayer3));

   if (!IS_EMPTY_STRING(intfMsgBody->bridgeType))
   {
      addObjToJsonObj(allParamsObj, "bridgeType",
                      json_object_new_string(intfMsgBody->bridgeType));
      addObjToJsonObj(allParamsObj, "bridgeIndex",
                      json_object_new_int(intfMsgBody->bridgeIndex));
   }

   if (intfMsgBody->isIpv4Enabled)
      addObjToJsonObj(allParamsObj, "isIpv4Enabled",
                      json_object_new_boolean(intfMsgBody->isIpv4Enabled));
   if (intfMsgBody->isIpv4Up)
      addObjToJsonObj(allParamsObj, "isIpv4Up",
                      json_object_new_boolean(intfMsgBody->isIpv4Up));
   if (intfMsgBody->isIpv6Enabled)
      addObjToJsonObj(allParamsObj, "isIpv6Enabled",
                      json_object_new_boolean(intfMsgBody->isIpv6Enabled));
   if (intfMsgBody->isIpv6Up)
      addObjToJsonObj(allParamsObj, "isIpv6Up",
                      json_object_new_boolean(intfMsgBody->isIpv6Up));
   if (intfMsgBody->isLinkUp)
      addObjToJsonObj(allParamsObj, "isLinkUp",
                      json_object_new_boolean(intfMsgBody->isLinkUp));

   if (!IS_EMPTY_STRING(intfMsgBody->ipv4Addr))
      addObjToJsonObj(allParamsObj, "ipv4Addr",
                      json_object_new_string(intfMsgBody->ipv4Addr));
   if (!IS_EMPTY_STRING(intfMsgBody->ipv4Netmask))
      addObjToJsonObj(allParamsObj, "ipv4Netmask",
                      json_object_new_string(intfMsgBody->ipv4Netmask));
   if (!IS_EMPTY_STRING(intfMsgBody->ipv4Gateway))
      addObjToJsonObj(allParamsObj, "ipv4Gateway",
                      json_object_new_string(intfMsgBody->ipv4Gateway));
   if (!IS_EMPTY_STRING(intfMsgBody->ipv6GlobalAddr))
      addObjToJsonObj(allParamsObj, "ipv6GlobalAddr",
                      json_object_new_string(intfMsgBody->ipv6GlobalAddr));
   if (!IS_EMPTY_STRING(intfMsgBody->ipv6NextHop))
      addObjToJsonObj(allParamsObj, "ipv6NextHop",
                      json_object_new_string(intfMsgBody->ipv6NextHop));
   if (!IS_EMPTY_STRING(intfMsgBody->dnsServers))
      addObjToJsonObj(allParamsObj, "dnsServers",
                      json_object_new_string(intfMsgBody->dnsServers));
   if (!IS_EMPTY_STRING(intfMsgBody->childrenIntfNames))
      addObjToJsonObj(allParamsObj, "childrenIntfNames",
                      json_object_new_string(intfMsgBody->childrenIntfNames));
   addObjToJsonObj(allParamsObj, "wlBrIndex",
                   json_object_new_int(intfMsgBody->wlBrIndex));
   if (!IS_EMPTY_STRING(intfMsgBody->pubCompName))
      addObjToJsonObj(allParamsObj, "pubCompName",
                      json_object_new_string(intfMsgBody->pubCompName));
   if (!IS_EMPTY_STRING(intfMsgBody->fullpath))
      addObjToJsonObj(allParamsObj, "fullpath",
                      json_object_new_string(intfMsgBody->fullpath));
   if (!IS_EMPTY_STRING(intfMsgBody->tags))
      addObjToJsonObj(allParamsObj, "tags",
                      json_object_new_string(intfMsgBody->tags));

   if (intfMsgBody->additionalDataLen > 0)
   {
      addObjToJsonObj(allParamsObj, "additionalDataLen",
                      json_object_new_int((int32_t) intfMsgBody->additionalDataLen));
      addObjToJsonObj(allParamsObj, "additionalData",
                      json_object_new_string((char *)(intfMsgBody+1)));
   }

   // s points to internal buffer of allParamsObj, and will be freed when
   // allParamsObj is freed.
   {
      const char *s = json_object_to_json_string(allParamsObj);
      // jsonStr is passed back to the user, who is responsible for freeing.
      jsonStr = cmsMem_strdup(s);
      json_object_put(allParamsObj);
   }

   bcmuLog_notice("returning %s", jsonStr);
   return jsonStr;
}


static json_bool getBoolParamFromJsonObj(const json_object *allParamsObj,
                                         const char *paramName)
{
   json_object *valObj=NULL;
   json_bool b=0;

   if (json_object_object_get_ex(allParamsObj, paramName, &valObj))
   {
      json_type valType = json_object_get_type(valObj);
      if (valType == json_type_boolean)
      {
         b = json_object_get_boolean(valObj); 
         bcmuLog_debug("got %s=%d", paramName, b);
      }
      else
      {
         bcmuLog_error("type mismatch on %s, expected boolean, got %s (%d)",
                       paramName, json_type_to_name(valType), valType);
      }
   }

   return b;
}


static int32_t getIntParamFromJsonObj(const json_object *allParamsObj,
                                  const char *paramName)
{
   json_object *valObj=NULL;
   int32_t i=0;

   if (json_object_object_get_ex(allParamsObj, paramName, &valObj))
   {
      json_type valType = json_object_get_type(valObj);
      if (valType == json_type_int)
      {
         i = json_object_get_int(valObj); 
         bcmuLog_debug("got %s=%d", paramName, i);
      }
      else
      {
         bcmuLog_error("type mismatch on %s, expected int, got %s (%d)",
                       paramName, json_type_to_name(valType), valType);
      }
   }

   return i;
}

static const char *getStringParamFromJsonObj(const json_object *allParamsObj,
                                             const char *paramName)
{
   json_object *valObj=NULL;
   const char *s=NULL;

   if (json_object_object_get_ex(allParamsObj, paramName, &valObj))
   {
      json_type valType = json_object_get_type(valObj);
      if (valType == json_type_string)
      {
         s = json_object_get_string(valObj);
         bcmuLog_debug("got %s=%s", paramName, s);
      }
      else
      {
         bcmuLog_error("type mismatch on %s, expected string, got %s (%d)",
                       paramName, json_type_to_name(valType), valType);
      }
   }

   return s;
}

PubSubInterfaceMsgBody *zbusUtil_decodeJsonToPubSubInterfaceMsgBody(const char *jsonStr)
{
   json_object *allParamsObj;
   PubSubInterfaceMsgBody *intfMsgBody;
   const char *s;
   UINT32 totalLen=0;
   UINT32 additionalDataLen=0;

   bcmuLog_notice("Entered: jsonStr %s", jsonStr);

   allParamsObj = json_tokener_parse(jsonStr);
   if (allParamsObj == NULL)
   {
      bcmuLog_error("unable to parse jsonStr %s", jsonStr);
      return NULL;
   }

   // Calc the size of intfMsgBody (may include additionalData) and allocate.
   additionalDataLen = (UINT32) getIntParamFromJsonObj(allParamsObj, "additionalDataLen");
   totalLen = sizeof(PubSubInterfaceMsgBody) + additionalDataLen;
   intfMsgBody = (PubSubInterfaceMsgBody *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (intfMsgBody == NULL)
   {
      bcmuLog_error("Alloc of %d bytes failed", totalLen);
      json_object_put(allParamsObj);
      return NULL;
   }

   // Look for each param in the intfMsgBody struct.
   if ((s = getStringParamFromJsonObj(allParamsObj, "intfName")) != NULL)
      snprintf(intfMsgBody->intfName, sizeof(intfMsgBody->intfName), "%s", s);

   intfMsgBody->isUpstream = getBoolParamFromJsonObj(allParamsObj, "isUpstream");
   intfMsgBody->isLayer2 = getBoolParamFromJsonObj(allParamsObj, "isLayer2");
   intfMsgBody->isLayer3 = getBoolParamFromJsonObj(allParamsObj, "isLayer3");
   intfMsgBody->isIpv4Enabled = getBoolParamFromJsonObj(allParamsObj, "isIpv4Enabled");
   intfMsgBody->isIpv4Up = getBoolParamFromJsonObj(allParamsObj, "isIpv4Up");
   intfMsgBody->isIpv6Enabled = getBoolParamFromJsonObj(allParamsObj, "isIpv6Enabled");
   intfMsgBody->isIpv6Up = getBoolParamFromJsonObj(allParamsObj, "isIpv6Up");
   intfMsgBody->isLinkUp = getBoolParamFromJsonObj(allParamsObj, "isLinkUp");

   intfMsgBody->bridgeIndex = -1;
   if ((s = getStringParamFromJsonObj(allParamsObj, "bridgeType")) != NULL)
   {
      strncpy(intfMsgBody->bridgeType, s, sizeof(intfMsgBody->bridgeType)-1);
      intfMsgBody->bridgeIndex = getIntParamFromJsonObj(allParamsObj, "bridgeIndex");
   }

   if ((s = getStringParamFromJsonObj(allParamsObj, "ipv4Addr")) != NULL)
      snprintf(intfMsgBody->ipv4Addr, sizeof(intfMsgBody->ipv4Addr), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "ipv4Netmask")) != NULL)
      snprintf(intfMsgBody->ipv4Netmask, sizeof(intfMsgBody->ipv4Netmask), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "ipv4Gateway")) != NULL)
      snprintf(intfMsgBody->ipv4Gateway, sizeof(intfMsgBody->ipv4Gateway), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "ipv6GlobalAddr")) != NULL)
      snprintf(intfMsgBody->ipv6GlobalAddr, sizeof(intfMsgBody->ipv6GlobalAddr), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "ipv6NextHop")) != NULL)
      snprintf(intfMsgBody->ipv6NextHop, sizeof(intfMsgBody->ipv6NextHop), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "dnsServers")) != NULL)
      snprintf(intfMsgBody->dnsServers, sizeof(intfMsgBody->dnsServers), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "childrenIntfNames")) != NULL)
      snprintf(intfMsgBody->childrenIntfNames, sizeof(intfMsgBody->childrenIntfNames), "%s", s);
   intfMsgBody->wlBrIndex = getIntParamFromJsonObj(allParamsObj, "wlBrIndex");
   if ((s = getStringParamFromJsonObj(allParamsObj, "pubCompName")) != NULL)
      snprintf(intfMsgBody->pubCompName, sizeof(intfMsgBody->pubCompName), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "fullpath")) != NULL)
      snprintf(intfMsgBody->fullpath, sizeof(intfMsgBody->fullpath), "%s", s);
   if ((s = getStringParamFromJsonObj(allParamsObj, "tags")) != NULL)
      snprintf(intfMsgBody->tags, sizeof(intfMsgBody->tags), "%s", s);

   // Additional data will follow the intfMsgBody struct.
   intfMsgBody->additionalDataLen = additionalDataLen;
   if (additionalDataLen > 0)
   {
      const char *s = getStringParamFromJsonObj(allParamsObj, "additionalData");
      if (s != NULL)
      {
         memcpy((void *) (intfMsgBody+1), (void *) s, additionalDataLen);
      }
      else
      {
         bcmuLog_error("additionalDataLen=%d but could not find additionalData field in JSON str, set len to 0",
                       additionalDataLen);
         // Do not return an inconsistent object which may cause caller to crash.
         intfMsgBody->additionalDataLen = 0;
      }
   }

   json_object_put(allParamsObj);
   return intfMsgBody;
}

