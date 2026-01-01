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

/*!\file generic_dsl_hal.c
 * \brief Implementation of the BCM Generic DSL HAL.  All of the MDM access
 *        functions are implemented in bcm_generic_hal, so this lib just 
 *        "inherits" those functions.  This lib is concerned with DSL specific
 *        init, notifications, callbacks, and callback thread.
 */

#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "sysutil.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "cms_mem.h"
#include "bcm_comp_md.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"
#include "bcm_generic_hal.h"
#include "bcm_generic_dsl_hal.h"
#include "cms_mdm.h"  // for cmsMdm_registerThreadMsgHandle().


void  *_dslHalMsgHandle=NULL;  // DSL HAL thread's msgHandle, it is the bus manager
SINT32 _dslHalShmId=-1; // UNINITIALIZED_SHM_ID
SINT32 _dslHalInitialized=0;
DslHalConfig _dslHalConfig;  // my local copy, don't depend on caller's copy.

// DSL HAL pthread stuff.
void *dsl_hal_thread_main(void *context);
static pthread_t dslHalThreadId;


BcmRet dsl_hal_init(DslHalConfig *config, int logLevel)
{
   BcmRet ret = BCMRET_INTERNAL_ERROR;
   int tries = 0;
   int rc;
   long int2pointer = logLevel; // Support both 32 and 64 bit arch

   if (_dslHalInitialized)
      return BCMRET_SUCCESS;

   // Since we are a multi-threaded process, add threadId to the logs.
   bcmuLog_setHdrMask(BCMULOG_DEFAULT_HDRMASK|BCMULOG_HDRMASK_THREAD_ID);
   bcmuLog_notice("Entered:");

   // Make my private copy so don't have to depend on caller's copy.
   memcpy(&_dslHalConfig, config, sizeof(DslHalConfig));
   _dslHalConfig.serverMsgHandle = NULL;

   // Create HAL thread to handle async events and callbacks.
   // The HAL thread will also call compMd_initDslSouth.
   rc = pthread_create(&dslHalThreadId, NULL, dsl_hal_thread_main, (void *) int2pointer);
   if (rc != 0)
   {
      bcmuLog_error("pthread_create failed, errno=%d", errno);
      return BCMRET_INTERNAL_ERROR;
   }

   // Create the upper layer (dsl_md, json_hal_server_dsl) msgHandle
   // and connect to the local component bcm_msgd.
   // Nobody is servicing this msgHandle yet, so any messages sent to it
   // will be just queued up until the main thread starts processing it.
   ret = compMd_connectToMsgBus(_dslHalConfig.serverEid, DSL_MSG_BUS, FALSE,
                                &(_dslHalConfig.serverMsgHandle));
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not init serverMsghandle, ret=%d", ret);
      return BCMRET_INTERNAL_ERROR;
   }

   // Make sure the HAL thread has initialized the MDM before registering
   // my msgHandle.
   while (_dslHalInitialized == 0 && tries < DSL_HAL_INIT_TIMEOUT)
   {
      sleep(1);
      tries++;
   }

   if (_dslHalInitialized == 0)
   {
      bcmuLog_error("Could not initialize HAL!");
      return BCMRET_INTERNAL_ERROR;
   }

   bcmuLog_notice("binding main thread %d (eid=%d) to msgHandle %p",
                  sysUtl_getThreadId(),
                  _dslHalConfig.serverEid,
                  _dslHalConfig.serverMsgHandle);
   cmsMdm_registerThreadMsgHandle(TRUE, _dslHalConfig.serverMsgHandle);
   // pass serverMsgHandle back to caller so caller can use it.
   config->serverMsgHandle = _dslHalConfig.serverMsgHandle;

   return BCMRET_SUCCESS;
}


// User of the DSL HAL (dsl_md, json_hal_server_dsl) has received notification
// of a change in the subscribed key.  User pushes the new key/value into
// the HAL.  Basically, we need to package this key/value up into a CMS msg
// and send it to some app in the DSL stack.
// This function's API does not tell us the EID of the subscriber, but in the
// case of DSL, the only app that subscribes to anything is dsl_ssk.  So just
// send it to dsl_ssk.
BcmRet dsl_hal_notifyKeyValueChange(const char *key, const char *value)
{
   CmsMsgHeader *msgHdr=NULL;
   PubSubKeyValueMsgBody *body=NULL;
   UINT32 totalLen=0;
   BcmRet ret=BCMRET_INTERNAL_ERROR;;

   if (key == NULL || value == NULL)
   {
      bcmuLog_error("Null input args %p=%p", key, value);
      return BCMRET_INVALID_ARGUMENTS;
   }

   bcmuLog_notice("Entered: key=%s value=%s", key, value);

   totalLen = sizeof(CmsMsgHeader)+sizeof(PubSubKeyValueMsgBody)+strlen(value)+1;
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      bcmuLog_error("Failed to allocate %d bytes", totalLen);
      return BCMRET_RESOURCE_EXCEEDED;
   }

   // Fill in and send a CMS msg to send to the subscriber.
   msgHdr->src = cmsMsg_getHandleEid(_dslHalConfig.serverMsgHandle);
   msgHdr->dst = EID_DSL_SSK;  // assume subscriber is dsl_ssk
   msgHdr->type = CMS_MSG_NOTIFY_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody)+strlen(value)+1;

   // Fill in key body and the string value, which follows the body.
   body = (PubSubKeyValueMsgBody *)(msgHdr+1);
   snprintf(body->key, sizeof(body->key), "%s", key);
   body->valueLen = strlen(value)+1;
   strcpy((char *)(body+1), value);

   ret = (BcmRet) cmsMsg_send(_dslHalConfig.serverMsgHandle, msgHdr);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Failed to send notify msg, ret=%d", ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(msgHdr);
   return ret;
}

