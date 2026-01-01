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

#ifndef __SSK_UTIL_H__
#define __SSK_UTIL_H__

/*!\file ssk_util.h
 * \brief Header file for libssk_util.so.  This lib contains common functions
 *        for all variants of ssk (esp dsl_ssk and intfstack).
 *
 */

#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_mdm.h"
#include "cms_core.h"
#include "cms_qdm.h"

/** Interval (in seconds) between periodic task execution.
 *
 * Hopefully we will not need to run more than once every 60 seconds to
 * keep data model stuff up-to-date.
 */
#define PERIODIC_TASK_INTERVAL 15

/** Max number of milliseconds to wait for a MDM read lock.
 *
 * Some apps, most notably wireless and voice, sometimes hold the lock for 
 * a lot longer than CMSLCK_MAX_HOLDTIME.  ssk runs in the background,
 * so it can afford to wait longer for the lock.
 */
#define SSK_LOCK_TIMEOUT  (15*MSECS_IN_SEC)

/** Increase the lock hold time warning threshold for link status change events.
 */
#define SSK_LINK_LOCK_WARNTHRESH (12*MSECS_IN_SEC)


/* In TR98 mode, we need to build our own list of layer 2 interfaces (not
 * easily figured out from data model.
 */
extern struct dlist_node wanLinkInfoHead;

/** Need to keep track of the previous link status for each WAN link
 *  so I know whether their state has changed or not.
 *  This is used by TR98 WAN side code only.
 */
typedef struct
{
   DlistNode dlist;   
   UBOOL8 isUp;                /**< Is the wan layer 2 link up */
   UBOOL8 isATM;               /**< Is this an ATM wan link */
   UBOOL8 isPTM;               /**< Is this a PTM wan link */
   UBOOL8 isEth;               /**< Is this an ethernet wan link */
   UBOOL8 isGpon;              /**< Is this a gpon wan link */
   UBOOL8 isEpon;              /**< Is this a epon wan link */   
   UBOOL8 isWifi;              /**< Is this an wifi wan link */   
   InstanceIdStack iidStack;   /**< Instance Id Stack for this wan link */
} WanLinkInfo;

/** Create a list of wanLinkInfo structs for TR98 only.
 *  This is not needed in Pure181 mode.
 */
void initWanLinkInfo(void);
void initWanLinkInfo_igd(void);

#if defined(SUPPORT_DM_LEGACY98)
#define initWanLinkInfo()           initWanLinkInfo_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define initWanLinkInfo()           initWanLinkInfo_igd()
#elif defined(SUPPORT_DM_PURE181)
#define initWanLinkInfo()
#elif defined(SUPPORT_DM_DETECT)
#define initWanLinkInfo()      do { if (!cmsMdm_isDataModelDevice2()) \
                                          initWanLinkInfo_igd(); } while(0)
#endif


/** Free all wanLinkInfo structs (free allocations for mem leak checker).
 *  This is not needed in Pure181 mode.
 */
void freeWanLinkInfoList(void);
void freeWanLinkInfoList_igd(void);

#if defined(SUPPORT_DM_LEGACY98)
#define freeWanLinkInfoList()           freeWanLinkInfoList_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define freeWanLinkInfoList()           freeWanLinkInfoList_igd()
#elif defined(SUPPORT_DM_PURE181)
#define freeWanLinkInfoList()
#elif defined(SUPPORT_DM_DETECT)
#define freeWanLinkInfoList()      do { if (!cmsMdm_isDataModelDevice2()) \
                                          freeWanLinkInfoList_igd(); } while(0)
#endif


/** Possible states of LAN interface temporarily moved to another bridge.
  * Used in SskLinkStatusRecord.
  */
typedef enum
{
   tmpMoveNo,      /**< no tmpmove action on this intreface */
   tmpMoveDoing,   /**< intreface is moving to another bridge, but have't been up*/
   tmpMoveDone     /**< intreface have moved to another bridge, and have been up*/
}TmpMoveState;

/** This struct is used by the TR98 LAN side code and TR181 (WAN and LAN side)
 *  for link status tracking.
 */
typedef struct
{
   DlistNode dlist;        /**< linked list header */
   char ifName[CMS_IFNAME_LENGTH]; /**< ifname */
   UBOOL8 isWan;    /**< Is this a WAN interface */
   UBOOL8 isLinkUp; /**< Is link up */
   TmpMoveState tmpMoved; /**< LAN only: is interface temporarily moved to another bridge */
   UBOOL8 moveBack; /**< LAN only: Does this interface need to be moved back to original bridge */
   char tmpBridgeIfName[CMS_IFNAME_LENGTH];  /**< LAN only: Name of bridge where this is temporarily moved to */
   SINT32 parentBrIndex; /**< Used by wifi to track which bridge (br0, br1) SSID is in */
} SskLinkStatusRecord;

extern struct dlist_node sskLinkStatusRecordHead;


/** Return TRUE if the link status of the given ifName has changed.
 *  If the given ifName is unknown, a linkStatusRecord will be created
 *  with initial state of link down.
 *  This function is used by the TR98 LAN side code and the TR181 LAN and
 *  WAN side code for tracking link status.
 *
 *  @param ifName (IN) intf name being queried
 *  @param isWan  (IN) TRUE if this is a Upstream/WAN interface
 *  @param status (IN) new link status: MDMVS_UP if UP, any other string is
 *                     considered not up.
 *
 *  @return TRUE if the link status has changed.
 */
UBOOL8 comparePreviousLinkStatus(const char *ifName, UBOOL8 isWan, const char *status);

/** Same as comparePreviousLinkStatus, except takes additional parentBrIndex arg,
 *  Used by wifi to detect change in the parent bridge assignment.
 */
UBOOL8 comparePreviousLinkInfo(const char *ifName, UBOOL8 isWan,
                               const char *status, SINT32 parentBrIndex);



SskLinkStatusRecord *getLinkStatusRecord(const char *ifName);

void cleanupLinkStatusRecords(void);



/** Broadcom netlink monitor related defines.
 */

/** A set of callback functions which is passed into processBcmNetlinkMonitor
 *  or processStdNetlinkMonitor.
 *  Currently just 1 callback function, but could be easily expaneded to more.
 */
typedef struct {
   void (*updateLinkStatus)(const char *intfName);
   void (*processXdslCfgSaveMsg)(unsigned int msgId);
   void (*xdslDriverCallback)(unsigned char lineId);
} BcmNetlinkCallbacks;

/** Open the BCM kernel monitor (netlink) fd.  Returns -1 on error. */
int initBcmNetlinkMonitorFd(void);

/** Receive and process netlink messages from Broadcom proprietary/legacy netlink socket.
 */
void processBcmNetlinkMonitor(int monitorFd, const BcmNetlinkCallbacks *callbacks);

/** Receive and process netlink messages from standard netlink socket.
 */
void processStdNetlinkMonitor(int monitorFd, const BcmNetlinkCallbacks *callbacks);


extern UBOOL8 isMdmInitDone;  // refers to full interface stack init done.


extern void sendStatusMsgToSmd(void *appMsgHandle, CmsMsgType msgType, const char *ifName);
extern void sendSubscribeKeyMsg(void *appMsgHandle, CmsEntityId dst, const char *key);
extern void sendIpInterfaceEventMsg(void *appMsgHandle, const char *ipIntfFullpath);
extern void sendIpInterfaceEventMsgEx(void *appMsgHandle, const char *ipIntfFullpath, UBOOL8 pub, const char *additionalStrData);
extern void sendIpInterfaceEvenMsgForBridge(void *appMsgHandle, const char *mgmtBridgePortFullpath);
extern void sendLayer2InterfaceEventMsg(void *appMsgHandle, const char *status,
                                        const MdmPathDescriptor *pathDesc);
extern void sendLayer2InterfaceEventMsgEx(void *appMsgHandle, const char *status,
                                  const MdmPathDescriptor *pathDesc,
                                  UBOOL8 pub, const char *additionalStrData);


/** Used by sysmgmt to "lock" or unlock ATM or PTM link from being freed while
 *  it used by a ppp interface.  This function publishes a key/value to
 *  sys_directory where key is BDK_PPPXTM_LOCK_PREFIX:xtmLinkFullPath
 *  and value is either LOCKED or ""
 *
 * @param xtmLinkFullPath (IN) someline like "Device.DSL.PTM_LINK.1"
 * @param lockStr (IN) Either "LOCKED" or "" for unlocked.
 */
void sendPppXtmLockMsg(void *appMsgHandle, const char *xtmLinkFullpath, const char *lockStr);

/** Used by dsl_ssk to subscribe/unsubscribe to ATM/PTM link lock events.
 * @param xtmLinkFullpath (IN) fullpath to the xtm link object to be locked.
 * @param sub (IN) TRUE for subscribe, FALSE for unsubscribe
 */
void sendSubscribePppXtmLockMsg(void *appMsgHandle, const char *xtmLinkFullpath, UBOOL8 sub);

#define BDK_PPPXTM_LOCK_PREFIX "BDK_PPPXTM_LOCK:"
// Max length of value.  Should be either "LOCKED" or ""
#define BDK_PPPXTM_VALUE_LEN     16
// Number of entries in pppXtmLockTable.
#define MAX_PPPXTM_LOCK_ENTRIES  8

typedef struct {
   UBOOL8 isLocked;
   UBOOL8 isLinkUp; /* true state of the xtm link */
   char fullpath[128];
} PppXtmLockEntry;

void pppXtm_delEntry(const char *xtmLinkFullpath);
void pppXtm_linkUpAddEntry(const char *xtmLinkFullpath);
UBOOL8 pppXtm_linkDownIsLocked(const char *xtmLinkFullpath);
void pppXtm_updateLockState(void *appMsgHandle, const char *xtmLinkFullpath, UBOOL8 lock);
UBOOL8 isOverXtm(const char *higherLayerFullpath, char **xtmLinkFullpath);


/* used by dsl_ssk to publish dsl diag complete event: loop diag or selt diag */
CmsRet publishDiagCompleteMsg(void *appMsgHandle, CmsMsgType type);
CmsRet getAdslLoopDiagResultAndLinkUp(void);
#define MAX_XDSL_DATA_LENGTH  61430   /* max length defined in the spec. */

// Some convience strings for matching fullpaths
#define DEV2_PPP_INTERFACE_FULLPATH   "Device.PPP.Interface."
#define DEV2_PTM_LINK_FULLPATH        "Device.PTM.Link."
#define DEV2_ATM_LINK_FULLPATH        "Device.ATM.Link."


/*
 * IP level diags.
 */
void processStartPingDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg);
void processStopPingDiag_dev2();
void processPingStateChanged_dev2(CmsMsgHeader *msg);

void processStartTracertDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg);
void processStopTracertDiag_dev2();
void processTracertStateChanged_dev2(CmsMsgHeader *msg);

void processStartDownloadDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg);
void processStopDownloadDiag_dev2();
void processDownloadDiagComplete_dev2();

void processStartUploadDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg);
void processStopUploadDiag_dev2();
void processUploadDiagComplete_dev2();

void processStartUdpechoDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg);
void processStopUdpechoDiag_dev2();
void processUdpechoDiagComplete_dev2();

void processStartUdpecho_dev2(const CmsMsgHeader *msg);
void processStopUdpecho_dev2();

void processStartUdpst_dev2(const CmsMsgHeader *msg);
void processStopUdpst_dev2();
void processUdpstDiagComplete_dev2();

void processStartServerSelection_dev2(const CmsMsgHeader *msg);
void processStopServerSelection_dev2();
void processServerSelectionDiagComplete_dev2();

void processSpeedServiceComplete();
void processEthCableDiagComplete_dev2();

CmsRet sskUtil_queryIntfName(void *appMsgHandle, CmsEntityId dstEid, const char *queryStr, char **respStr);


/** Set some info about the app that is linking with lib ssy_util
 */
void sskUtil_setMyAppInfo(const char *compName, const char *appName, SINT32 eid);

extern char _myCompName[];
extern char _myAppName[];
extern SINT32 _myEid;

extern CmsTimestamp bootTimestamp;
extern UBOOL8 isRdpaGBEAEsysLockedIn;


/** When DSL or EthWan link comes up, ssk will call this function with the
 *  wan type that came up.  This function will match against what RDPA was
 *  initialized with.  If there is a mismatch, ssk will set the correct type
 *  and reboot.
 */
void matchRdpaWanType(const char *wanTypeUp);

/*
 * TR181 Interface Stack functions, in ssk2_intfstack.c
 */
void processIntfStackLowerLayersChangedMsg(void *appMsgHandle, const CmsMsgHeader *msg);
void processIntfStackObjectDeletedMsg(void *appMsgHandle, const CmsMsgHeader *msg);
void processIntfStackAliasChangedMsg(const CmsMsgHeader *msg);
void processIntfStackStaticAddressConfigdMsg(void *appMsgHandle, const CmsMsgHeader *msg);
void processIntfStackPropagateMsg(void *appMsgHandle, const CmsMsgHeader *msg);
void processIntfStackPropagateMsgEx(void *appMsgHandle, const CmsMsgHeader *msg);
void intfStack_propagateStatusByIidLocked(void *appMsgHandle, MdmObjectId oid, const InstanceIdStack *iidStack, const char *status);
void intfStack_propagateStatusByFullPathLocked(void *appMsgHandle, const char *lowerLayerFullPath, const char *status);
void intfStack_propagateStatusOnSingleEntryLocked(void *appMsgHandle, const Dev2InterfaceStackObject *intfStackObj, const char *status);
void intfStack_setStatusByFullPathLocked(const char *fullPath, const char *status);
void intfStack_setStatusByPathDescLocked(const MdmPathDescriptor *pathDescIn, const char *status);
void intfStack_updateIpIntfStatusLocked(const InstanceIdStack *ipIntfIidStack,
                                        const char *newIpv4ServiceStatus,
                                        const char *newIpv6ServiceStatus);
UBOOL8 intfStack_optionalCheckForEnabledByFullpath(const char *fullpath);

/** ssk calls this at the end of intfStack processing.  Do an initial scan
  * of link status and forward the message to smd to let it know everything
  *  is ready.
  */
void processMdmInitDone(void *appMsgHandle, CmsEntityId srcEid, CmsEntityId dstEid, UINT32 wordData);

void processKeyValueEvent(void *appMsgHandle, const CmsMsgHeader *msg);
void processQosParamsUpdate(char *value);
void processTrafficManagementUpdate(char *value);

#ifdef DMP_DEVICE2_BONDEDDSL_1
void updateXtmLowerLayerLocked(void);
void updateAtmLowerLayerLocked(UBOOL8 trainBonded);
void updatePtmLowerLayerLocked(UBOOL8 trainBonded);
#endif

void processTrafficMismatchMessage(unsigned int msgData);
void setDslBondingTrafficType();

void processOamStripByte(UINT32 msgId);


void processXdslCfgSaveMessage(UINT32 msgId);
void processXdslCfgSaveMessage_dev2(UINT32 msgId);
void processXdslCfgSaveMessage_igd(UINT32 msgId);
#if defined(SUPPORT_DM_LEGACY98)
#define processXdslCfgSaveMessage(m)  processXdslCfgSaveMessage_igd(m)
#elif defined(SUPPORT_DM_HYBRID)
#define processXdslCfgSaveMessage(m)  processXdslCfgSaveMessage_igd(m)
#elif defined(SUPPORT_DM_PURE181)
#define processXdslCfgSaveMessage(m)  processXdslCfgSaveMessage_dev2(m)
#elif defined(SUPPORT_DM_DETECT)
#define processXdslCfgSaveMessage(m)  (cmsMdm_isDataModelDevice2() ? \
                                      processXdslCfgSaveMessage_dev2(m) : \
                                      processXdslCfgSaveMessage_igd(m))
#endif


/** Top level function to force ssk to update its knowledge of all WAN and
 *  LAN link statuses.
 *
 * @param intfName (IN) If NULL, then ssk must query/update all link statuses.
 *                      If not NULL, then ssk only needs to query/update the
 *                      link status of the specified intfName.
 */
void updateLinkStatus(const char *intfName);
void updateLinkStatus_igd(const char *intfName);
void updateLinkStatus_dev2(const char *intfName);

#if defined(SUPPORT_DM_LEGACY98)
#define updateLinkStatus(i)             updateLinkStatus_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define updateLinkStatus(i)             updateLinkStatus_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define updateLinkStatus(i)             updateLinkStatus_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define updateLinkStatus(i)             (cmsMdm_isDataModelDevice2() ? \
                                         updateLinkStatus_dev2((i)) : \
                                         updateLinkStatus_igd((i)))
#endif


void checkDslLinkStatusLocked_igd(void *appMsgHandle, WanLinkInfo *wanLinkInfo);
void checkDslLinkStatusLocked_dev2(void *appMsgHandle);

void updateLanHostEntryActiveStatus_dev2(const char *ifName, UBOOL8 linkUp);


extern UBOOL8 dslDiagInProgress;
void getDslDiagResults(void *appMsgHandle);

#ifdef DMP_DSLDIAGNOSTICS_1
typedef struct
{
   UBOOL8 dslLoopDiagInProgress;
   InstanceIdStack iidStack;
   int pollRetries;
   CmsEntityId src;
} dslLoopDiag;

#define DIAG_DSL_LOOPBACK_TIMEOUT_PERIOD 150 /* maximum 2.5 minutes */

/** Function to check for DSL loop diag completion
 *
 */
void processWatchDslLoopDiag(CmsMsgHeader *msg);
void processWatchDslLoopDiag_igd(CmsMsgHeader *msg);
void processWatchDslLoopDiag_dev2(CmsMsgHeader *msg);

#if defined(SUPPORT_DM_LEGACY98)
#define processWatchDslLoopDiag(m)  (processWatchDslLoopDiag_igd)(m)
#elif defined(SUPPORT_DM_HYBRID)
#define processWatchDslLoopDiag(m)  (processWatchDslLoopDiag_igd)(m)
#elif defined(SUPPORT_DM_PURE181)
#define processWatchDslLoopDiag(m)  (processWatchDslLoopDiag_dev2)(m)
#elif defined(SUPPORT_DM_DETECT)
#define processWatchDslLoopDiag(m)  (cmsMdm_isDataModelDevice2() ? \
                                     (processWatchDslLoopDiag_dev2)(m) : \
                                     (processWatchDslLoopDiag_igd)(m))
#endif


/** Report DSL Loop diag results
 *
 */
void getDslLoopDiagResults(void *appMsgHandle);
void getDslLoopDiagResults_igd(void *appMsgHandle);
void getDslLoopDiagResults_dev2(void *appMsgHandle);

#if defined(SUPPORT_DM_LEGACY98)
#define getDslLoopDiagResults(a)    getDslLoopDiagResults_igd(a)
#elif defined(SUPPORT_DM_HYBRID)
#define getDslLoopDiagResults(a)    getDslLoopDiagResults_igd(a)
#elif defined(SUPPORT_DM_PURE181)
#define getDslLoopDiagResults(a)    getDslLoopDiagResults_dev2(a)
#elif defined(SUPPORT_DM_DETECT)
#define getDslLoopDiagResults(a)    (cmsMdm_isDataModelDevice2(a) ? \
                                    getDslLoopDiagResults_dev2(a) : \
                                    getDslLoopDiagResults_igd(a))
#endif


#endif  /* DMP_DSLDIAGNOSTICS_1 */


#ifdef DMP_X_BROADCOM_COM_SELT_1
typedef struct
{
   UBOOL8 dslDiagInProgress;
   UINT32 testType;   /* SELT or whatever test */
   int pollRetries;
   CmsEntityId src;
   time_t testStartTime;
   time_t testEndTime;
} dslDiag;

void processWatchDslSeltDiag(CmsMsgHeader *msg);
void getDslSeltDiagResults(void *appMsgHandle);
#endif /* DMP_X_BROADCOM_COM_SELT_1*/



/*
 * TR181 Connection status functions (IPv4), in ssk2_iputil.c
 */
void setIpv4ServiceStatusByFullPathLocked(const char *ipIntfFullPath, const char *serviceStatus);
void setIpv4ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack, const char *serviceStatus);
void getIpv4ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack,
                                     char *serviceStatus, UINT32 bufLen);


void sskConn_setPppConnStatusByFullPathLocked(const char *pppIntfFullPath, const char *connStatus);
void sskConn_setPppConnStatusByIidLocked(const InstanceIdStack *pppIntfIidStack, const char *connStatus);
void sskConn_setPppConnStatusByObjLocked(const InstanceIdStack *iidStack,
                      Dev2PppInterfaceObject *pppIntfObj,
                      const char *connStatus);

UBOOL8 sskConn_hasStaticIpv4AddressLocked(const InstanceIdStack *ipIntfIidStack);
UBOOL8 sskConn_hasAnyIpv4AddressLocked(const InstanceIdStack *ipIntfIidStack);
UBOOL8 sskConn_getAnyIpv4AddressLocked(const InstanceIdStack *ipIntfIidStack,
                                       char *ipv4AddrBuf, char *ipv4SubnetMask);

CmsRet sskConn_getIpv4GatewayForIpIntfLocked(const char *ipIntfFullpath, char *ipv4Gateway);
CmsRet sskConn_getDnsServersForIpIntfLocked(const char *ipIntfName, char *dnsServers);

void setIpv6ServiceStatusByFullPathLocked(const char *ipIntfFullPath, const char *serviceStatus);
void setIpv6ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack, const char *serviceStatus);
void getIpv6ServiceStatusByIidLocked(const InstanceIdStack *ipIntfIidStack,
                                     char *serviceStatus, UINT32 bufLen);

UBOOL8 sskConn_hasStaticIpv6AddressLocked(const InstanceIdStack *ipIntfIidStack);
UBOOL8 sskConn_hasAnyIpv6AddressLocked(const InstanceIdStack *ipIntfIidStack);
UBOOL8 sskConn_getAnyIpv6AddressLocked(const InstanceIdStack *ipIntfIidStack,
                                       char *ipv6AddrBuf);

CmsRet sskConn_getIpv6NextHopForIpIntfLocked(const char *ipIntfFullpath, char *ipv6NextHop);

/* In conn_util.c */
void updateWanConnStatusInSubtreeLocked(void *appMsgHandle, const InstanceIdStack *iidStack, UBOOL8 isLinkUp);
void updateSingleWanConnStatusLocked(void *appMsgHandle, const InstanceIdStack *iidStack, void *wanConnObj, UBOOL8 wanLinkUp);

/* in connstatus_n.c */
void processAutoDetectTask(void *appMsgHandle);
void updateWanConnStatusInSubtreeLocked_n(void *appMsgHandle,
                                          const InstanceIdStack *parentIidStack, 
                                          UBOOL8  isLinkUp, 
                                          UBOOL8 startNewConnection);
void addNewWanConnObj(const InstanceIdStack *iidStack, void *wanConnObj);
void updateAutoDetectWanConnListForDeletion(const InstanceIdStack *iidStack, MdmObjectId wanConnOid);
void  stopAllWanConn(void *appMsgHandle, const InstanceIdStack *parentIidStack, UBOOL8 newAutoDetectEnabled);
extern UBOOL8 isAutoDetectionEnabled;


/** A set of voice related callback functions.  These are only used in the
  * main ssk in CMS Classic build.  dsl_ssk does not handle voice at all.
 */
typedef struct {
   void (*initVoiceOnWanIntf)(const char *ifName, const char *ipAddr);
   void (*initVoiceOnIntfUp)(UINT32 ipvx, const char *intfName, UBOOL8 isWan);
} SskVoiceCallbacks;

/* defined in ssk2_intfstack.c */
extern SskVoiceCallbacks sskVoiceCallbacks;

extern UBOOL8 isVoiceOnLanSide;
extern UBOOL8 isVoiceOnAnyWan;
extern char * voiceWanIfName;

#endif /* __SSK_UTIL_H__ */
