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

#ifndef __BCM_ZBUS_INTF_H__
#define __BCM_ZBUS_INTF_H__

/*!\file bcm_zbus_intf.h
 * \brief Header file for libbcm_zbus_intf.so, which is the top level wrapper
 *        lib for D-Bus and U-Bus.  The bcm_zbus_intf lib is MDM independent,
 *        while bcm_zbus_mdm contains the MDM dependent functions.  Most of
 *        the documentation associated with the various BDK bus libraries is
 *        here so please read the block below.
 *
 */

/*
 * There are 3 ways you can use the ZBus libraries, listed below in increasing
 * order of complexity.
 *
 * 1. Your app just wants to get and set key/value pairs in the BDK system
 *    directory or read/write to other BDK Distributed MDM's.  (make "outbound"
 *    calls).
 * 2. Your app wants to do (1) and also get asynchronous notifiations when
 *    a key, which your app subscribed to, changes.
 * 3. Your app wants to implement ("inbound" or "server") methods on the bus
 *    which other apps can call.
 *
 * For 1: your app must link against libbcm_zbus_intf and one of:
 * libbcm_dbus_intf or libbcm_ubus_intf (but not both).  To get a key-value
 * from the BDK system directory server, call zbus_out_getKey().  To publish a
 * key-value, call zbus_out_publishKeyValue().  There is no need to call
 * zbusIntf_init or zbusIntf_mainLoop().  Your app does not give up its control
 * of execution.  Your app does not link with (does not have its own)
 * Distributed MDM, but it can get/set read/write to other Distributed MDM's
 * using zbus_out_getParameterValues and zbus_out_setParameterValues.
 *
 * For 2: same linking requirements as (1).  However, you will need to
 * call zbusIntf_init().  ZbusConfig must be filled in, especially the
 * processNotifyEventFp.  You must create a method context and call
 * zbusIntf_addNotifyMethod() to set the bus dependent method which will
 * receive the async notification.  The bus dependent method will then call
 * the processNotifyEventFp.  Then call zbusIntf_mainLoop().  Note that by
 * calling zbusIntf_mainLoop(), your app gives up the control of its execution.
 * It cannot run unless an event comes in (or a CMS msg, if you have
 * configured that and set processBcmMsgFp).  Your app is not dependent on
 * the MDM.
 *
 * TODO: even for simple cases of 1 and 2, there is some use of CMS message
 * header structure.  Even though it is just a data structure (not dependent
 * on CMS msg code), it would be good to remove it.
 *
 * For 3: your app must link against libbcm_zbus_intf and libbcm_zbus_mdm
 * and either {libbcm_dbus_intf+libbcm_dbus_mdm} or
 * {libbcm_ubus_intf+libbcm_ubus_mdm}, but not both sets.  You must fill in
 * ZbusConfig and MethodContext and call zbusIntf_init() and
 * zbus_intfMainLoop().  Your app gives up control of
 * its execution and it is dependent on the MDM.
 *
 * Technical details:
 * On the "outbound" or "client" direction, the API's start with zbusIntf_ or
 * zbus_out_.  These functions do the bus independent stuff and then call
 * the bus dependent functions, which start with busIntf_ or bus_out_.
 * These bus dependent functions must be implemented in the bus dependent libs.
 * Note that function pointers are not used in this path.  The bus dependent
 * libs must provide all functions that the zbus lib call.
 * On the "inbound" or "server" direction, the bus dependent methods plug
 * into their bus and handle the initial call.  After unwrapping all the bus
 * dependent stuff off of the data, the bus dependent functions call
 * zbus_in_xyz to do the bus independent stuff.  Any return data is again
 * wrapped by the bus dependent functions and returned on the bus.  Note that
 * function pointers are also not used in this path.  The zbus libraries must
 * provide all functions that the bus dependent libs call.
 */

#include "bdk.h"
#include "bcm_retcodes.h"
#include "cms_msg.h"  // only need structure definition, not any code
#include "cms_msg_pubsub.h"
#include "bcm_generic_hal_defs.h"


#define ZBUS_INTF_NAME_LEN       256


/** Bus-independent address.  Geared towards DBus.  // TODO: add UBus fields
 */
typedef struct
{
   char compName[BDK_COMP_NAME_LEN+1];  // standard component name from bdk.h
   char busName[ZBUS_INTF_NAME_LEN];  // dbus
   char intfName[ZBUS_INTF_NAME_LEN]; // dbus
   char objPath[ZBUS_INTF_NAME_LEN];  // dbus
} ZbusAddr;

/** Given a standard component name as defined in bdk.h, return the ZbusAddr
  * structure, or NULL if the name is not recognized.  The returned ZbusAddr
  * MUST NOT be freeded (it is a pointer to internal const array).
 */
const ZbusAddr *zbusIntf_componentNameToZbusAddr(const char *compName);


/** Structure used to configure Zbus.  Caller must fill in all info needed by
 * both D-Bus and Z-Bus since caller does not know what bus will be used.
 */
typedef struct {
   char busName[ZBUS_INTF_NAME_LEN];  // dbus
   char intfName[ZBUS_INTF_NAME_LEN]; // dbus
   char objPath[ZBUS_INTF_NAME_LEN];  // dbus
   char ubusName[ZBUS_INTF_NAME_LEN]; // can add ubus specific config params
   int msgFd;                         // BCM msg fd
   int (*processBcmMsgFp)(void *);    // BCM msg handler
   void *msgData;                     // data passed to processBcmMsgFp and processNotifyEventFp
   void (*processNotifyEventFp)(void *, CmsMsgHeader *);  // handle notification on subscribed event
   int monitorFd;                     // fd to be monitored
   int (*processMonitorFp)(void *);   // monitor fd handler
   int (*busNameAcquiredFp)(void *);   // bus name acquired handler
   UINT32 periodicTimeoutMs;           // timeout interval in milliseconds
   int (*periodicTimeoutFp)(void *);  // this function pointer will be called every periodicTimeoutMs.
} ZbusConfig;



/*
 * These Event related methods send and receive CMS_MSG_PUBSUB messages over the
 * Z-Bus.  In BDK, sys_directory is the manager of the event state so there
 * is not need to pass in the destAddr.
 */

// Can handle subscribe and unsubscribe (depends on msg->type)
// Handles keyValue and interface, but not namespace
// If unsubscribe, msgResp can be NULL; if provided, it will be unchanged (
// should remain NULL).
BcmRet zbus_out_subscribeEvent(const char *compName, const CmsMsgHeader *msgReq, CmsMsgHeader **msgResp);

// Can handle subscribe and unsubscribe, depends on flags: 0 means subscribe,
// ZBUS_FLAG_UNSUBSCRIBE means unsubscribe.
// If unsubsribe, strResult can be NULL; if provided, it will be 
// unchanged (should remain NULL).
BcmRet zbus_out_subscribeNamespaces(const char *compName, CmsEntityId eid,
                                    UINT32 flags, char **strResult);

BcmRet zbus_out_getNamespaces(const char *strReq, char **strResult);

BcmRet zbus_out_getEventStatus(const CmsMsgHeader *msgReq, CmsMsgHeader **msgResp);
BcmRet zbus_out_getEventStatusGeneric(UINT32 eventType,
                                      const char *strReq, char **strResult);

BcmRet zbus_out_queryEventStatus(UINT32 flags, const CmsMsgHeader *msgReq,
                                 CmsMsgHeader **msgResp);
BcmRet zbus_out_queryEventStatusGeneric(UINT32 eventType, UINT32 flags,
                                 const char *strReq, char **strResult);

// Can handle publish and unpublish events (depends on msg->type)
BcmRet zbus_out_publishEvent(const char *compName, const CmsMsgHeader *msg);
BcmRet zbus_out_publishEventGeneric(const char *compName, UINT32 src,
                                    UINT32 eventType, UINT32 flags,
                                    const char *strDataOut);

// Called by sysdir_notifier to notify a subscriber of an event change.
// sysdir_notifier should be the only app calling this function.
BcmRet zbus_out_notifyEvent(const char *compName, UINT32 eventType, UINT32 eid,
                            const char *strDataOut);

// Request firewall control operation.  msg must include FirewallCtlMsgBody.
BcmRet zbus_out_firewallCtl(const char *compName, const CmsMsgHeader *msgReq);

// Request proxy get parameterValues from inside of a BDK component to another component.
// The md process of the component would do the proxy between cms msg and zbus operations.
BcmRet zbus_out_proxyGetParameterValues(const CmsMsgHeader *msgReq, CmsMsgHeader **msgResp);

/*
 * The next set of functions are for accessing the Distributed MDM on the bus.
 * Note that the calling application does not have to link with libcms_core
 * or any MDM libs to access the Distributed MDM because all functions are
 * independent of CMS data structures, just generic strings.
 */

/** MDM operation method.
 *
 * @param destCompName (IN) standardized component name from bdk.h
 * @param op  (IN) MDM operation string (BCM_DATABASEOP_*) defined in bcm_generic_hal_defs.h
 * @param arg (IN) Optional additional data for the operation; could be null.
 * @param data (OUT) For certain ops, such as readConfig, a data string
 *                      will be returned.  Caller is responsible for freeing.
 * @return BcmRet code.
 */
BcmRet zbus_out_mdmOp(const char *destCompName, const char *op, const char *arg, char **data);



// Get all param names/info under a single input fullpath.
BcmRet zbus_out_getParameterNames(const ZbusAddr *destAddr,
                const char *fullpath, UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);

// Get values of the given array of fullpaths.
// On successful return, the paramInfoArray will be an array of numParamInfos
// BcmGenenericParamInfo structures.  The following fields in the structures
// will be filled in:
// fullpath, type, value, profile, isWritable, isPassword.
// The errorCode field is not applicable for this function.
BcmRet zbus_out_getParameterValues(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);

// Set param values in the given array of BcmGenericParamInfo structs.
// When you call this function, you must fill out the following fields:
// fullpath, type, value.  The other fields are ignored.
// On return, the overall return code is returned and individual error codes
// will be indicated in the errorCode field (0 means no error on that param).
// The profile, isWritable, and isPassword fields are not appliable to this
// function.
BcmRet zbus_out_setParameterValues(const ZbusAddr *destAddr,
                BcmGenericParamInfo *paramInfoArray, UINT32 numParamInfos,
                UINT32 flags);

// Get the attributes of the array of fullpaths.
BcmRet zbus_out_getParameterAttributes(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamAttr **paramAttrArray, UINT32 *numParamAttrs);

// Set the attributes 
BcmRet zbus_out_setParameterAttributes(const ZbusAddr *destAddr,
                BcmGenericParamAttr *paramAttrArray, UINT32 numParamAttrs,
                UINT32 flags);

// Add the specified fullpath (normally, the fullpath does not have the new
// instance.  But CMS/BDK also supports when caller specified the "requested
// new instance number".
BcmRet zbus_out_addObject(const ZbusAddr *destAddr,
                          const char *fullpath, UINT32 flags,
                          UINT32 *instanceNumber);

// Delete the specified fullpath.
BcmRet zbus_out_deleteObject(const ZbusAddr *destAddr,
                             const char *fullpath, UINT32 flags);

// This actually just calls getParameterValues for the specified fullpath
// with nextLevel=TRUE, but is so convenient to have this function.
BcmRet zbus_out_getObject(const ZbusAddr *destAddr,
                const char *fullpath, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);

BcmRet zbus_out_getNextObject(const ZbusAddr *destAddr,
                const char *fullpath, const char *limitSubtree, UINT32 flags,
                char **nextFullpath,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);


// All of the above zbus_out_ functions will call lower level bus functions
// which are provided in bcm_dbus_intf and bcm_ubus_intf, but app can only
// link against one of those libs.
BcmRet bus_out_getEventStatus(UINT32 eventType, const char *strReq,
                              char **strResp);

BcmRet bus_out_queryEventStatus(UINT32 eventType, UINT32 flags,
                                const char *strReq, char **strResp);

BcmRet bus_out_publishEvent(const char *compName, UINT32 src,
                            UINT32 eventType, UINT32 flags,
                            const char *strDataOut);

BcmRet bus_out_subscribeEvent(const char *compName, CmsEntityId eid,
                              UINT32 eventType, UINT32 flags,
                              const char *strReq, char **strResp);
BcmRet bus_out_notifyEvent(const ZbusAddr *destAddr,
                           UINT32 eventType, UINT32 eid, const char *strData);
BcmRet bus_out_firewallCtl(const ZbusAddr *destAddr, const char *compName,
                           const char *iptablesStr);
BcmRet bus_out_mdmOp(const ZbusAddr *destAddr,
                     const char *op, const char *args, char **strDataOut);


BcmRet bus_out_getParameterNames(const ZbusAddr *destAddr,
                const char *fullpath, UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);

BcmRet bus_out_getParameterValues(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);

BcmRet bus_out_setParameterValues(const ZbusAddr *destAddr,
                BcmGenericParamInfo *paramInfoArray, UINT32 numParamInfos,
                UINT32 flags);

BcmRet bus_out_getParameterAttributes(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamAttr **paramAttrArray, UINT32 *numParamAttrs);

BcmRet bus_out_setParameterAttributes(const ZbusAddr *destAddr,
                BcmGenericParamAttr *paramAttrArray, UINT32 numParamAttrs,
                UINT32 flags);

BcmRet bus_out_addObject(const ZbusAddr *destAddr,
                         const char *fullpath, UINT32 flags,
                         UINT32 *instanceNumber);

BcmRet bus_out_deleteObject(const ZbusAddr *destAddr,
                            const char *fullpath, UINT32 flags);

// This actually just calls getParameterValues for the specified fullpath
// with nextLevel=FALSE, but is so convenient to have this function.
BcmRet bus_out_getObject(const ZbusAddr *destAddr,
                const char *fullpath, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);

BcmRet bus_out_getNextObject(const ZbusAddr *destAddr,
               const char *fullpath, const char *limitSubtree, UINT32 flags,
               char **nextFullpath,
               BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos);


/** Info needed to handle INBOUND method calls. */
#define ZBUS_METHOD_NAME_LEN       128

typedef struct
{
   char name[ZBUS_METHOD_NAME_LEN]; //Required. Set either method name or interface name
   const char *introspection; //Required. D-Bus introspection method or interface string

   const void *policy;	// For ubus only. array to specify method input parameters, name and type
   int policyEntries;
   /*
    * In one ZbusMethodInfo, if it is method based info (most common case),
    * handler must be set.
    * If it is interface based info, intfHandler and objPath must be set.
    * So either handler or intfHandler+objPath must be set but not both.
    */
   void (*handler)(void *input, void **output);
   void (*intfHandler)(const void *arg1, const void *arg2,
                       void *arg3, void *argv);
   char objPath[ZBUS_INTF_NAME_LEN];

} ZbusMethodInfo;


#define ZBUS_NUM_METHODS_PER_ALLOC 25

/* maximum number of interface based ZbusMethodInfo supported per application*/
#define ZBUS_MAX_COMP_INTF         16

typedef struct
{
   int numMethods;   // number of valid methods
   int maxMethods;   // number of method slots available.
   ZbusMethodInfo *methodInfos;
} ZbusMethodContext;


// The next few functions allow apps to create a list of inbound method
// handlers.  It is taylored for D-Bus.  Not sure how applicable it will
// be to U-Bus.
// The general idea is to first call getMethodBuilder, which creates an empty
// builder.
// Then fill in the builder with the standard set of BDK Component Management
// daemon functions and or any custom functions.  And finally pass the 
// method builder context into zbusIntf_init.
// Important!|  After calling zbusIntf_init, do not modify or free the method
// builder.  You can (optionally) free the methodContext right before the
// app exits.

// Get an empty builder
BcmRet zbusIntf_getMethodBuilder(ZbusMethodContext **context);

// Add custom (INBOUND) method to the existing builder.
BcmRet zbusIntf_addMethod(ZbusMethodContext *context, const ZbusMethodInfo *methodInfo);

// Add the notify (INBOUND) method to the builder.  For apps which want to 
// subscribe to events and receive notification when event status changes.
BcmRet zbusIntf_addNotifyMethod(ZbusMethodContext *ctx);

// Add the standard set of (INBOUND) methods that all component MD's support.
// includes the notifyMethod.
BcmRet zbusIntf_addCompMdMethods(ZbusMethodContext *ctx);

// Free the builder.  Important, do not free until the app is about to exit.
// Actually, app does not even need to call free.  This is only to make
// memory leak checkers happy.
void zbusIntf_freeMethodBuilder(ZbusMethodContext **context);


/** Pass in configuration data before running in main loop.
 */
BcmRet zbusIntf_init(const ZbusConfig *config, const ZbusMethodContext *ctx);

/** Connect to bus and stay in main loop until quit.  In D-Bus, the app
 *  gives up control and goes into D-Bus main loop.
 */
int zbusIntf_mainLoop();

// Actual, low level bus functions to support the above.
// These must be implemented by bcm_dbus_intf, bcm_dbus_mdm,
// bcm_ubus_intf and bcm_ubus_mdm.
BcmRet busIntf_addNotifyMethod(ZbusMethodContext *ctx);
BcmRet busIntf_addCompMdMethods(ZbusMethodContext *ctx);
int busIntf_mainLoop();


// Make local copies of the config structures passed in during init.
// These are visible to other libs such as bcm_dbus_intf, bcm_dbus_mdm,
// bcm_ubus_intf, bcm_ubus_mdm, etc.
// 
extern ZbusConfig _zbusConfig;
extern const ZbusMethodContext *_zbusMethodCtx;



/** Flags used in various Z-Bus calls.
 *  It is ok for UNSUBSCRIBE and UNPUBLISH to use the same bit because they
 *  are only used in their specific subscribe and publish methods.  No chance
 *  of collision or confusion.
 *
 * Note these flags must not conflict with the OGF flags in cms_object.h
 */
#define ZBUS_FLAG_SAVE_ON_SUCCESS  0x00010000
#define ZBUS_FLAG_UNSUBSCRIBE      0x00800000
#define ZBUS_FLAG_UNPUBLISH        0x00800000
#define ZBUS_FLAGS_MASK            0x00FF0000

/*
 * Misc helper functions.
 */

/** Parse a key value string of form key=value or key= or key.  This can
 *  also be used to parse a namespace=ownerCompName string.
 *
 * @param kvStr (IN) the entire key value string.
 * @param keyBuf (OUT) buffer to hold the key
 * @param keyBufLen (IN) to accomodate null char, the key must be less than keyBufLen bytes.
 * @param valuePtr (OUT) if provided and on success, this will point to the
 *                       location of kvStr where the value begins.
 * @return BCMRET_SUCCESS if the string was successfully parsed.
 */
BcmRet zbusUtil_parseKeyValueString(const char *kvStr,
                                    char *keyBuf, UINT32 keyBufLen,
                                    char **valuePtr);

// Caller is responsible for freeing the returned string.
char *zbusUtil_createOutboundEventDataStr(const CmsMsgHeader *msgReq);

// Caller is responsible for freeing the returned CMS Response msg.
CmsMsgHeader *zbusUtil_createEventRespMsg(const CmsMsgHeader *msgReq,
                                          const char *strDataIn);

// Allocate and return a CMS Msg based on input data.
// Caller is responsible for freeing the message.
CmsMsgHeader *zbusUtil_createEventNotificationMsg(UINT32 eventType, UINT32 eid,
                                                  const char *strDataIn);

// Encode the given PubSubInterfaceMsgBody (including any additional data) as
// a JSON string.
// Caller is responsible for freeing the string.
char *zbusUtil_encodePubSubInterfaceMsgBodyToJson(const PubSubInterfaceMsgBody *intfMsgBody);

// Allocate and return a CMS Msg based on input data.
// Caller is responsible for freeing the string.
CmsMsgHeader *zbusUtil_createProxyGetParamRespMsg(const CmsMsgHeader *msgReq,
                                                  const char *fullpath,
                                                  const char *type,
                                                  const char *value);

// Given a JSON string describing a PubSubInterfaceMsgBody, allocate and
// return a filled in PubSubInterfaceMsgBody (including any additional data).
// Caller is responsible for freeing the buffer.
PubSubInterfaceMsgBody *zbusUtil_decodeJsonToPubSubInterfaceMsgBody(const char *jsonStr);

#endif /* __BCM_ZBUS_INTF_H__ */                                     

