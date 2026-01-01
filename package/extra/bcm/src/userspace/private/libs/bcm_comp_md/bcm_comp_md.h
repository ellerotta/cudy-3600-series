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

#ifndef __BCM_COMP_MD_H__
#define __BCM_COMP_MD_H__

/*!\file bcm_comp_md.h
 * \brief Header file for libbcm_comp_md.so.  Useful functions for the various
 *        component manager daemons.
 *
 */

#include "bcm_retcodes.h"
#include "number_defs.h"
#include "cms_eid.h"
#include "cms_msg.h"

BcmRet compMd_startMsgBus(const char *args);

BcmRet compMd_connectToMsgBus(CmsEntityId eid, const char *busName,
                              UBOOL8 isBusManager, void **msgHandle);

BcmRet compMd_setBusManager(void *msgHandle, CmsEntityId eid);

BcmRet compMd_launchApp(const char *prefix, const char *exe, const char *args, int *pid);

BcmRet compMd_sendTerminateMsg(void *msgHandle, CmsEntityId dst, UINT32 timeoutMs);

void compMd_collectChild(SINT32 pid, UINT32 timeoutMs);


/** Indicates MDM (including intfStack) is initialized.  Should not be needed
  * by external code, but make it available anyways (for now).
  */
extern SINT32 mdmInitialized;


/** Holds a bunch of ZBus function pointers for use by bcm_comp_md code.
 *  bcm_comp_md code cannot call zbus directly because it cannot have a
 *  hard dependency to the ZBus/DBus/UBus libs.
 */
typedef struct {
   BcmRet (*processPublishEventFp)(const char *, const CmsMsgHeader *);
   BcmRet (*processSubscribeEventFp)(const char *, const CmsMsgHeader *, CmsMsgHeader **);
   BcmRet (*processGetEventStatusFp)(const CmsMsgHeader *, CmsMsgHeader **);
   BcmRet (*processQueryEventStatusFp)(UINT32, const CmsMsgHeader *, CmsMsgHeader **);
   BcmRet (*processProxyGetParamFp)(const CmsMsgHeader *, CmsMsgHeader **);
} CompMdZbusFpConfig;


/** Data/state needed by compMd_processMsg.
 *
 *  If a processSpecificCmsMsgFp is set, it will be given the first chance to
 *  process a message before the common message processing code
 *  (compMd_processMsg).
 *  If processSpecificCmsMsgFp has processed the message, it should return 1 so
 *  the common message processing code will not process it again.  Otherwise,
 *  return 0 so the common message processing code will try to process it.
 *  In all cases, processSpecificCmsMsgFp should not free the msg pointer,
 *  that will be done in compMd_processMsgWithTimeout.
 */
typedef struct {
   void   *msgHandle;          // mandatory
   SINT32 *shmIdPtr;           // optional
   SINT32 *mdmInitializedPtr;  // optional
   int   (*processSpecificCmsMessageFp)(CmsMsgHeader *msg);  // optional
   CompMdZbusFpConfig zbusConfig; // optional
} CompMdProcessMsgConfig;

/** Common CMS message message handler for all the Management Daemons (MD's)
 *  Any unhandled messages can be sent to the MD via the
 *  processSpecificCmsMessageFp function pointer in the CompMdProcessMsgConfig.
 *
 *  @param timeout (IN) Timeout in milliseconds. 0 means do not block,
 *                      < 0 means block until a message is received.
 *                      otherwise, block for sepecified number of milliseconds.
 *
 * The real prototype of this function is:
 * BcmRet compMd_processMsgWithTimeout(CompMdProcessMsgConfig *config, SINT32 timeout);
 *
 * But it was made very generic so it can be easily passed to the zbus intf lib.
 */
int compMd_processMsgWithTimeout(void *config, SINT32 timeout);
int compMd_processMsgWith35msTimeout(void *config);

/** Common CMS message message handler for all the Management Daemons (MD's)
 *  A wrapper function for blocking mode.
 */
int compMd_processMsg(void *config);


/** Handler for receiving an event notification from the ZBus.
 *
 * @param config (IN) pointer to CompMdProcessMsgConfig passed in during init.
 */
void compMd_processNotifyEvent(void *config, CmsMsgHeader *msg);


/** Send a simple reply message.
 */
void compMd_sendReply(void *msgHandle, CmsMsgHeader *msg, CmsRet rv);

/** Do standard cleanup and initialization in the newly forked daemon process.
 */
void compMd_initDaemon();

/** Register namespace with sys_directory.
 */
BcmRet compMd_registerNamespace(void *msgHandle, UINT32 eid, const char *namespc);

/** Subscribe or unsubscribe to a key in sys_directory.
 */
BcmRet compMd_subscribeKey(void *msgHandle, UBOOL8 sub,
                           UINT32 srcEid, UINT32 dstEid, const char *key);

/** Publish MDM_ACTIVE_NOTIFICATION:component/timeStamp key/value pair with sys_directory.
 */
BcmRet compMd_publishMdmNotification(void *msgHandle, UINT32 eid, const char *componentName);

// CLI related functions
UBOOL8 compMd_launchedAsCli(const char *cmd);
BcmRet compMd_initCli(SINT32 keyOffset, void *shmAttachAddr, const char *busName,
                      void **msgHandle, SINT32 *shmId);
void compMd_processCliInput();



/** Init function for sysmgmt_md
 */
BcmRet compMd_initSysmgmt(const char **myNamespaces,
                          void **msgHandle, SINT32 *shmId);

void compMd_cleanupSysmgmt(void *msgHandle);


/** Init function for devinfo_md
 */
BcmRet compMd_initDevinfo(const char **myNamespaces,
                          void **msgHandle, SINT32 *shmId);

/** Init function for diag_md
 */
BcmRet compMd_initDiag(const char **myNamespaces,
                       void **msgHandle, SINT32 *shmId);

/** Init functions for DSL component.
 *  The north bound version is used by dsl_md to initialize interface to ZBus.
 *  The south bound version is used by dsl_hal_thread to initialize the
 *  entire DSL stack.  The south version is called by dsl_hal_init and should
 *  be called before the north version.
 */
BcmRet compMd_initDslNorth(const char **myNamespaces, void *msgHandle);
BcmRet compMd_initDslSouth(void **msgHandle, SINT32 *shmId, SINT32 verbosity);

/** cleanup DSL component */
void compMd_cleanupDsl(void *msgHandle);


/** Init functions for GPON component.
 *  The north bound version is used by gpon_md to initialize interface to ZBus.
 *  The south bound version is used by gpon_hal_thread to initialize the
 *  GPON/OMCI stack.  The south version is called by gpon_hal_init and should
 *  be called before the north version.
 */
BcmRet compMd_initGponNorth(const char **myNamespaces, void *msgHandle);
BcmRet compMd_initGponSouth(void **msgHandle, SINT32 *shmId);

// The omcid pid is stored in the bcm_comp_md lib, but exported to any app
// that needs to look at it.  CMS_INVALID_PID means omcid is not running.
extern int compMd_omciPid;
BcmRet compMd_launchOmcid(SINT32 shmId);
BcmRet compMd_terminateOmcid(void *msgHandle);

void compMd_cleanupGpon(void *msgHandle);


/** Init functions for EPON component.
 *  The north bound version is used by epon_md to initialize interface to ZBus.
 *  The south bound version is used by epon_hal_thread to initialize the
 *  EPON stack.  The south version is called by epon_hal_init and should
 *  be called before the north version.
 */
BcmRet compMd_initEponNorth(const char **myNamespaces, void *msgHandle);
BcmRet compMd_initEponSouth(void **msgHandle, SINT32 *shmId);

/** Launch eponapp */
BcmRet compMd_launchEponapp(SINT32 shmId);


/** Init functions for Voice component.
 *  The north bound version is used by voice_md to initialize interface to ZBus.
 *  The south bound version is used by voice_hal_thread to initialize the
 *  entire Voice stack.  The south version is called by mta_hal_InitDB and should
 *  be called before the north version.
 */
BcmRet compMd_initVoiceNorth(const char **myNamespaces, void **msgHandle);
BcmRet compMd_initVoiceSouth(void **msgHandle, SINT32 *shmId);

// The voice app pid is stored in the bcm_comp_md lib, but exported to any app
// that needs to look at it.  CMS_INVALID_PID means voice app is not running.
extern int compMd_voicePid;

/** cleanup voice component */
void compMd_cleanupVoice(void *msgHandle);


/** Init function for openplat_md
 */
BcmRet compMd_initOpenPlat(const char **myNamespaces,
                           void **msgHandle, SINT32 *shmId);


/** Init function for wifi_md
 */
BcmRet compMd_initWiFi(const char **myNamespaces,
                       void **msgHandle, SINT32 *shmId, SINT32 logLevel);

// The wlssk pid is stored in the bcm_comp_md lib, but exported to any app
// that needs to look at it.  CMS_INVALID_PID means wlssk is not running.
extern int compMd_wlsskPid;

void compMd_cleanupWiFi(void);


/** Init function for tr69_md
 */
BcmRet compMd_initTr69(const char **myNamespaces,
                       void **msgHandle, SINT32 *shmId, SINT32 *tr69cPid);

void compMd_cleanupTr69(void);


/** Init function for usp_md
 */
BcmRet compMd_initUsp(const char **myNamespaces,
                      void **msgHandle, SINT32 *shmId);

#endif /* __BCM_COMP_MD_H__ */
