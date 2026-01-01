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

#ifndef __BCM_UBUS_INTF_H__
#define __BCM_UBUS_INTF_H__

/*!\file ubus_intf.h
 * \brief Private/internal header file for libbcm_ubus_intf.so.
 *
 */

#include "libubus.h"

// Since most CMS/BDK lock timeouts are in the 6-18 second range, and we
// really want these calls to succeed, set timeout to 20*1000 milliseconds.
// TODO: should put a BCM_UBUS prefix, or move to ZBUS? (BCM_ZBUS_TIMEOUT...)
#define TIMEOUT_DEFAULT_MDMACCESS   (20000)

// Use the same timeout for non-MDM method calls, since they might be blocked
// behind a MDM access call.
#define TIMEOUT_DEFAULT_METHOD_CALL (20000)



typedef struct
{
   uint32_t magic;
   struct ubus_context *ctx;
   struct blob_buf *buf;
   uint32_t id;
}UbusOutboundHandle;

/**
* Description: handle incoming ubus method calls
* Parameters
*  @ctx: struct ubus_context *
*  @req: struct ubus_request_data *
*  @method: pointer of method name
*  @msg: struct blob_attr *
*/
void ubus_in_setParameterValues(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_getParameterValues(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_getParameterNames(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_setParameterAttributes(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_getParameterAttributes(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_addObject(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_deleteObject(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_getNextObject(const void *ctx, const void *req, void *method, void *msg);
void ubus_in_mdmOp(const void *ctx, const void *req, void *method, void *msg);

void *ubusIntf_getOutboundHandle(const ZbusAddr *dest);
void ubusIntf_freeOutboundHandle(void *handle);

/*
* Get number of array entries
* the array may carry primary data types, such as string, integer, or may carry nested data type,
*  such as a table, which looks like '{"nextlevel":false, "flags":32768}'
*/
static inline uint32_t getNumberOfArrayEntries(struct blob_attr *array)
{
   void *attr;
   unsigned int len, i = 0;

   if(!array || (BLOBMSG_TYPE_ARRAY != blobmsg_type(array)))
      return 0;

   /*
   * please refer to blob.h for operation __blob_for_each_attr performs
   * Here "len" is initialized with value of total bytes of array. it decreases after each cycle of iteration.
   * "i" is increased by one after one entry is skipped and gets the value of total entries at the end
   */
   len = blobmsg_data_len(array);
   __blob_for_each_attr(attr, blobmsg_data(array), len)
   {
      i++;
   }

   return i;
}

/*
* Parse and verify input arguments of ubus call based on the policy of that ubus method
* after parsing number of arguments, argument type will be verified
*/
static inline bool parseAndVerifyParameters(const struct blobmsg_policy *policy,
         uint32_t entries,
         struct blob_attr *msg,
         struct blob_attr **tb)
{
   uint32_t i;

   if(! policy || (0 == entries) || !msg || !tb)
      return FALSE;

   blobmsg_parse(policy, entries, tb, blob_data(msg), blob_len(msg));
   for(i = 0; i < entries; i++)
   {
      if(!tb[i] || (policy[i].type != (uint32_t) blobmsg_type(tb[i])))
      {
         return FALSE;
      }
   }

   return TRUE;
}

#endif /* __DBUS_INTF_H__ */

