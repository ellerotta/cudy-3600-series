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

#ifndef __BCM_GENERIC_DSL_HAL_H__
#define __BCM_GENERIC_DSL_HAL_H__

/*!\file bcm_generic_dsl_hal.h
 * \brief Header file for the Broadcom Generic DSL HAL.  The core MDM
 *        access functions of this HAL simply use the Broadcom Generic HAL.
 *        This file only implements a DSL spcific init and notification
 *        function.
 */

#include "bcm_generic_hal.h"


// Return codes used by the callback functions.  RETURN_OK (0) is success.
// All other codes are errors.  (These are a bit of leftovers from the RDK HAL,
// maybe remove one day?)
#ifndef RETURN_OK
#define RETURN_OK   0
#endif

#ifndef RETURN_ERR
#define RETURN_ERR   -1
#endif

/** Interface status and info used in publishInterfaceStatusFp.
 */
typedef struct {
   char intfName[32];  /**< interface name */
   int  isLinkUp;      /**< 1 for UP, 0 for down. */
   char fullpath[256]; /**< TR181 fullpath of the interface. */
} DslHalInterfaceStatus;


/** DSL HAL config struct, to be passed into dsl_hal_init.
 *  For now, this contains only callback functions, but we could add other
 *  config params later.
 *
 * publishInterfaceStatus allows the DSL HAL to notify the upper layer agent
 * of changes to interface status.  pub=1 means publish the interface data.
 * pub=0 means unpublish the data (used when interface is deleted).  This
 * callback is optional.
 *
 * publishKeyValue allows the DSL HAL to publish various key value pairs to
 * the system.  Current uses are for DSL diag complete and TR69 active
 * notification.  This callback is optional.  (pub=0 means unpublish)
 *
 * subscribeKey allows the DSL HAL to subscribe to changes to any key/value
 * pair in the system.  This is needed for the PPPXTM lock feature.  This
 * callback is optional, so if this function pointer is NULL, all subscribe
 * requests will fail.  sub=1 means subscribe, sub=0 means unsubscribe.
 * Optional: on successful subscribe, and current value is non-null, it
 * will be returned.  On entry, valueLen indicates number of bytes pointed
 * to by value, on return, the actual number of bytes copied (including
 * null termination) is returned.
 */
typedef struct {
   unsigned int serverEid;  // EID of the upper layer agent (dsl_md or json_hal_server), must be filled in by caller
   void *serverMsgHandle;   // Created and returned by dsl_hal_init.
   int (*publishInterfaceStatusFp)(int pub, const DslHalInterfaceStatus *status);
   int (*publishKeyValueFp)(int pub, const char *key, const char *value);
   int (*subscribeKeyFp)(int sub, const char *key, char *value, int *valueLen);
   int (*queryInterfaceFp)(const char *queryStr, char *value, int *valueLen);
} DslHalConfig;


/** Initialize DSL HAL
 *
 *  @param config (IN/OUT) Config info, including callbacks, for DSL HAL.
 *           On successful return serverMsgHandle will point to a valid
 *           CMS msgHandle, which the upper layer agent may use.
 *  @param verbosity (IN) 0 for Error only, 1 for Notice+errors, and 3 for
 *                        debug,notice, and errors.
 *  @return BcmRet code
 */
BcmRet dsl_hal_init(DslHalConfig *config, int verbosity);

// This is really an internal variable, but more convenient to put it here.
// We expect the DSL HAL to fully initialize in under 30 seconds.
#define DSL_HAL_INIT_TIMEOUT   30


/** This is called by the system agent (dsl_md or ccsp-dsl-agent) when a
 *  subscribed key/value pair changes value.
 *  See subscribeKeyFp in DslHalCallbacks.
 */
BcmRet dsl_hal_notifyKeyValueChange(const char *key, const char *value);

#endif /* __BCM_GENERIC_DSL_HAL_H__ */
