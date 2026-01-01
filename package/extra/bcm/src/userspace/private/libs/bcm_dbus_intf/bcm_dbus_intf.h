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

#ifndef __BCM_DBUS_INTF_H__
#define __BCM_DBUS_INTF_H__

/*!\file bcm_dbus_intf.h
 * \brief Public header file for most of the BDK DBus implementation, even if
 *        the actual code/implementation is not in bcm_dbus_intf (might be in
 *        bcm_dbus_mdm or the apps that need it).
 *
 */

#include <gio/gio.h>
#include <glib-unix.h>

#include "cms_mdm.h"
#include "bcm_zbus_intf.h"


#define DBUS_INTROSPECTION_BEGIN   "<node> <interface name='"

// important! After xxx_MD_INTERNFACE_NAME, close with '>
// Then add all methods in between.

#define DBUS_INTROSPECTION_END    "</interface></node>"


// Real prototype is:
// void dbus_in_getParameterNames(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
void dbus_in_getParameterNames(void *input, void **output);

#define DBUS_GETPARAMETERNAMES_METHOD_INTROSPECTION \
  "    <method name='getParameterNames'>" \
  "      <arg direction='in' type='s' name='fullpath'/>" \
  "      <arg direction='in' type='b'  name='nextlevel'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='a(sssbb)' name='fullpath_type_profile_writable_isPassword_array'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_getParameterValues(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
void dbus_in_getParameterValues(void *input, void **output);

#define DBUS_GETPARAMETERVALUES_METHOD_INTROSPECTION \
  "    <method name='getParameterValues'>" \
  "      <arg direction='in' type='as' name='fullpath_array'/>" \
  "      <arg direction='in' type='b'  name='nextlevel'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='a(ssssbb)' name='fullpath_type_value_profile_writable_isPassword_array'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_setParameterValues(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// fullpath_error_array: This array is empty if no errors, otherwise, contains each fullpath that had an error
void dbus_in_setParameterValues(void *input, void **output);

#define DBUS_SETPARAMETERVALUES_METHOD_INTROSPECTION \
  "    <method name='setParameterValues'>" \
  "      <arg direction='in' type='a(sss)' name='fullpath_type_value_array'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='a(su)' name='fullpath_error_array'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>" \


// Real prototype is:
// void dbus_in_getParameterAttributes(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// fullpath_access_notif_valueChanged_altNotif_altValue_array: a(sqqqqq) where q is uint16.
void dbus_in_getParameterAttributes(void *input, void **output);

#define DBUS_GETPARAMETERATTRIBUTES_METHOD_INTROSPECTION \
  "    <method name='getParameterAttributes'>" \
  "      <arg direction='in' type='as' name='fullpath_array'/>" \
  "      <arg direction='in' type='b'  name='nextlevel'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='a(sqqqqq)' name='fullpath_access_notif_valueChanged_altNotif_altValue_array'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_setParameterAttributes(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// fullpath_setAccess_access_setNotif_notif_setAltNotif_altNotif_clearAltValue_array: a(sbqbqbqb) where q is uint16;
// fullpath_error_array: This array is empty if no errors, otherwise, contains each fullpath that had an error
void dbus_in_setParameterAttributes(void *input, void **output);

#define DBUS_SETPARAMETERATTRIBUTES_METHOD_INTROSPECTION \
  "    <method name='setParameterAttributes'>" \
  "      <arg direction='in' type='a(sbqbqbqb)' name='fullpath_setAccess_access_setNotif_notif_setAltNotif_altNotif_clearAltValue_array'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='a(su)' name='fullpath_error_array'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_addObject(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// instanceNumber: instance number of the newly created object.
void dbus_in_addObject(void *input, void **output);

#define DBUS_ADDOBJECT_METHOD_INTROSPECTION \
  "    <method name='addObject'>" \
  "      <arg direction='in' type='s' name='fullpath'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='u' name='instanceNumber'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_deleteObject(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
void dbus_in_deleteObject(void *input, void **output);

#define DBUS_DELETEOBJECT_METHOD_INTROSPECTION \
  "    <method name='deleteObject'>" \
  "      <arg direction='in' type='s' name='fullpath'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_getNextObject(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// fullpath: at the beginning of the walk, this is just a generic fullpath,
//           e.g. Device.Services.VoiceService.{i}.SIP.Network.{i}.  On subsequent
//           calls, pass in the previously returned nextFullpath.
// limitSubtree: fullpath to limit the walk to this subtree.  Use Device. if no
//           limit, otherwise, specifiy a subtree, e.g. Device.Services.VoiceService.1.
// nextFullpath: on success, this contains the fullpath of the next instance found.
// Also, on success, all params of the found object is returned in the following array.
void dbus_in_getNextObject(void *input, void **output);

#define DBUS_GETNEXTOBJECT_METHOD_INTROSPECTION \
  "    <method name='getNextObject'>" \
  "      <arg direction='in' type='s' name='fullpath'/>" \
  "      <arg direction='in' type='s'  name='limitSubtree'/>" \
  "      <arg direction='in' type='u'  name='flags'/>" \
  "      <arg direction='out' type='s' name='nextFullpath'/>" \
  "      <arg direction='out' type='a(ssssbb)' name='fullpath_type_value_profile_writable_isPassword_array'/>" \
  "      <arg direction='out' type='s' name='response'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// MDM operation method.  This method is supported by all MD's.
// Real prototype is:
// void dbus_in_mdmOp(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
void dbus_in_mdmOp(void *input, void **output);

// Supported ops are defined in bcm_zbus_intf.h
// op: readConfig, arg: configType of PSI or MDM, retData: config buffer read
// op: writeConfig, arg: config buffer to write, retData: null
// op: saveConfig, all arguments null
// op: invalidateConfig, all arguments null
#define DBUS_MDMOP_METHOD_INTROSPECTION \
  "    <method name='mdmOp'>" \
  "      <arg direction='in' type='s' name='op'/>" \
  "      <arg direction='in' type='s' name='arg'/>" \
  "      <arg direction='out' type='s' name='retData'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"



// Methods relating to Pub/Sub.  Only sys_directory implements the inbound
// versions of publishEvent, subscribeEvent, getEventStatus, queryEventStatus.
// See sysdir_dbus.c and sysdir_ubus.c for inbound implementation.
// Most components will need to call the outbound versions of these functions,
// which is defined in bcm_zbus_intf.h

// Real prototype is:
// void dbus_in_publishEvent(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// eventType: see PubSubEventType in cms_msg_pubsub.h
void dbus_in_publishEvent(void *input, void **output);

#define DBUS_INTROSPECTION_PUBLISH_EVENT \
  "    <method name='publishEvent'>" \
  "      <arg direction='in' type='u' name='eventType'/>" \
  "      <arg direction='in' type='s' name='compName'/>" \
  "      <arg direction='in' type='u' name='eid'/>" \
  "      <arg direction='in' type='u' name='flags'/>" \
  "      <arg direction='in' type='s' name='strDataIn'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_subScribeEvent(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// eventType: see PubSubEventType in cms_msg_pubsub.h
void dbus_in_subscribeEvent(void *input, void **output);

#define DBUS_INTROSPECTION_SUBSCRIBE_EVENT \
  "    <method name='subscribeEvent'>" \
  "      <arg direction='in' type='u' name='eventType'/>" \
  "      <arg direction='in' type='s' name='compName'/>" \
  "      <arg direction='in' type='u' name='eid'/>" \
  "      <arg direction='in' type='u' name='flags'/>" \
  "      <arg direction='in' type='s' name='strDataIn'/>" \
  "      <arg direction='out' type='s' name='strDataOut'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// Real prototype is:
// void dbus_in_getEventStatus(GVariant *parameters, GVariant **result);
// But make more generic so it can be used in generic function pointers.
// eventType: see PubSubEventType in cms_msg_pubsub.h
void dbus_in_getEventStatus(void *input, void **output);

#define DBUS_INTROSPECTION_GET_EVENT_STATUS \
  "    <method name='getEventStatus'>" \
  "      <arg direction='in' type='u' name='eventType'/>" \
  "      <arg direction='in' type='u' name='flags'/>" \
  "      <arg direction='in' type='s' name='strDataIn'/>" \
  "      <arg direction='out' type='s' name='strDataOut'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// eventType: see PubSubEventType in cms_msg_pubsub.h
void dbus_in_queryEventStatus(void *input, void **output);

#define DBUS_INTROSPECTION_QUERY_EVENT_STATUS \
  "    <method name='queryEventStatus'>" \
  "      <arg direction='in' type='u' name='eventType'/>" \
  "      <arg direction='in' type='u' name='flags'/>" \
  "      <arg direction='in' type='s' name='strDataIn'/>" \
  "      <arg direction='out' type='s' name='strDataOut'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"


// This method is called when a subscribed event has a value change.
// This method is in the standard set of methods supported by all MD's.
// If a component does not subscribe to any events, this method will not be
// called.
// eventType: see PubSubEventType in cms_msg_pubsub.h
void dbus_in_notifyEvent(void *input, void **output);

#define DBUS_NOTIFYEVENT_METHOD_INTROSPECTION \
  "    <method name='notifyEvent'>" \
  "      <arg direction='in' type='u' name='eventType'/>" \
  "      <arg direction='in' type='u' name='eid'/>" \
  "      <arg direction='in' type='s' name='strData'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"



// This function is implemented by GPON, EPON, and sysmgmt components only to
// accept config info from the wanconf app.
void dbus_in_wanConf(void *input, void **output);

#define DBUS_WANCONF_METHOD_INTROSPECTION \
  "    <method name='wanConf'>" \
  "      <arg direction='in' type='s' name='cmd'/>" \
  "      <arg direction='in' type='s' name='arg'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"

// This function is implemented by sysmgmt component to accept reloadMcastCtrl
// request from another component.
void dbus_in_reloadMcastCtrl(void *input, void **output);

#define DBUS_RELOADMCASTCTRL_METHOD_INTROSPECTION \
  "    <method name='reloadMcastCtrl'>" \
  "      <arg direction='in' type='s' name='cmd'/>" \
  "      <arg direction='in' type='b' name='igmpadmission'/>" \
  "      <arg direction='in' type='b' name='joinforceforward'/>" \
  "      <arg direction='in' type='s' name='igmpmcastifnames'/>" \
  "      <arg direction='in' type='s' name='mcastbridgeifnames'/>" \
  "      <arg direction='out' type='u' name='result'/>" \
  "    </method>"



// Helper functions for outbound calls.
GDBusProxy *dbusIntf_getOutboundHandle(const ZbusAddr *destAddr);
void dbusIntf_freeOutboundHandle(GDBusProxy *proxy);


#endif /* __BCM_DBUS_INTF_H__ */
