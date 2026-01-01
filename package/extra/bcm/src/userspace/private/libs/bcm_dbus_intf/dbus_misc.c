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
 * \brief Various MDM independent methods for D-Bus.
 */

// TODO: most of the functions in this file deals with events.  Rename file
// to dbus_events.c?


#include <stdio.h>
#include <errno.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include "number_defs.h"
#include "os_defs.h"
#include "cms_mem.h"
#include "bdk_dbus.h"
#include "bcm_zbus_intf.h"
#include "bcm_dbus_intf.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"


BcmRet bus_out_subscribeEvent(const char *compName, CmsEntityId eid,
                              UINT32 eventType, UINT32 flags,
                              const char *strReq, char **strResp)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    const ZbusAddr *destAddr = NULL;
    GError *error = NULL;
    BcmRet ret = BCMRET_SUCCESS;

    if ((compName == NULL) || (strReq == NULL))
    {
        bcmuLog_error("one or more input args is null %p/%p", compName, strReq);
        return BCMRET_INVALID_ARGUMENTS;
    }

    bcmuLog_notice("Entered: compName=%s flags=0x%x", compName, flags);

    destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INTERNAL_ERROR;
    }

    // call sys_directory on D-Bus
    gresult = g_dbus_proxy_call_sync(outProxy,
                  "subscribeEvent",
                  g_variant_new("(usuus)", eventType, compName, eid, flags, strReq),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,  //TODO: set timeout to be consistent with UBus
                  NULL,
                  &error);
    if (error != NULL)
    {
        bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
        g_error_free(error);
        ret = BCMRET_INTERNAL_ERROR;
    }
    else
    {
        gchar *strDataIn = NULL;
        guint32 gret = 0;
        g_variant_get(gresult, "(su)", &strDataIn, &gret);
        bcmuLog_notice("dbus method returned %d: %s", gret, strDataIn);

        if (strResp)
            *strResp = cmsMem_strdup(strDataIn);
        g_free(strDataIn);
        g_variant_unref(gresult);
        ret = gret;
    }

    dbusIntf_freeOutboundHandle(outProxy);
    return ret;
}


BcmRet bus_out_getEventStatus(UINT32 eventType, const char *strReq, char **strResp)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    const ZbusAddr *destAddr = NULL;
    GError *error = NULL;
    BcmRet ret = BCMRET_INTERNAL_ERROR;

    bcmuLog_notice("Entered:");

    destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INTERNAL_ERROR;
    }

    // call sys_directory on D-Bus (see sysdir_dbus.h)
    gresult = g_dbus_proxy_call_sync(outProxy,
                  "getEventStatus",
                  g_variant_new("(uus)", eventType, 0, strReq),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,
                  NULL,
                  &error);

    if (error != NULL)
    {
        bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
        g_error_free(error);
        ret = BCMRET_INTERNAL_ERROR;
    }
    else
    {
        gchar *strDataIn = NULL;
        guint32 gret = 0;
        g_variant_get(gresult, "(su)", &strDataIn, &gret);
        bcmuLog_debug("dbus method returned %d: %s", gret, strDataIn);

        *strResp = cmsMem_strdup(strDataIn);
        g_free(strDataIn);
        g_variant_unref(gresult);
        ret = gret;
    }

    dbusIntf_freeOutboundHandle(outProxy);
    return ret;
}

BcmRet bus_out_queryEventStatus(UINT32 eventType, UINT32 flags,
                                const char *strReq, char **strResp)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    const ZbusAddr *destAddr = NULL;
    GError *error = NULL;
    BcmRet ret = BCMRET_INTERNAL_ERROR;

    if((NULL == strResp) || (NULL == strReq))
    {
       bcmuLog_error("one or more NULL input args");
       return BCMRET_INVALID_ARGUMENTS;
    }
    *strResp = NULL;

    bcmuLog_notice("Entered: eventType=%d request=%s", eventType, strReq);

    destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INTERNAL_ERROR;
    }

    // call sys_directory on D-Bus (see sysdir_dbus.h)
    gresult = g_dbus_proxy_call_sync(outProxy,
                  "queryEventStatus",
                  g_variant_new("(uus)", eventType, flags, strReq),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,
                  NULL,
                  &error);

    if (error != NULL)
    {
        bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
        g_error_free(error);
        ret = BCMRET_INTERNAL_ERROR;
    }
    else
    {
        const gchar *strDataIn = NULL;
        guint32 gret = 0;
        g_variant_get(gresult, "(&su)", &strDataIn, &gret);
        bcmuLog_debug("dbus method returned %d: %s", gret, strDataIn);

        *strResp = cmsMem_strdup(strDataIn);
        g_variant_unref(gresult);
        ret = gret;
    }

    dbusIntf_freeOutboundHandle(outProxy);
    return ret;
}

BcmRet bus_out_publishEvent(const char *compName, UINT32 src,
                            UINT32 eventType, UINT32 flags,
                            const char *strDataOut)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    const ZbusAddr *destAddr = NULL;
    GError *error = NULL;
    BcmRet ret=BCMRET_INVALID_ARGUMENTS;

    if (compName == NULL || strDataOut == NULL)
    {
        bcmuLog_error("NULL input args");
        return BCMRET_INVALID_ARGUMENTS;
    }

    bcmuLog_notice("Entered: compName=%s src=%d eventType=%d flags=0x%x strDataOut=%s",
                   compName, src, eventType, flags, strDataOut);

    destAddr = zbusIntf_componentNameToZbusAddr(BDK_COMP_SYS_DIRECTORY);
    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INVALID_ARGUMENTS;
    }

    // call sys_directory on D-Bus (see sysdir_dbus.h)
    gresult = g_dbus_proxy_call_sync(outProxy,
                  "publishEvent",
                  g_variant_new("(usuus)",
                                eventType, compName, src, flags, strDataOut),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,
                  NULL,
                  &error);

    dbusIntf_freeOutboundHandle(outProxy);

    if (error != NULL)
    {
        bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
        g_error_free(error);
        ret = BCMRET_INTERNAL_ERROR;
    }
    else
    {
        g_variant_get(gresult, "(u)", &ret);
        bcmuLog_debug("dbus method returned %d", ret);
        g_variant_unref(gresult);
    }

    return ret;
}


BcmRet bus_out_notifyEvent(const ZbusAddr *destAddr,
                           UINT32 eventType, UINT32 eid, const char *strData)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    GError *error = NULL;
    BcmRet ret=BCMRET_INVALID_ARGUMENTS;
    int tries=0;

    if (destAddr == NULL || strData == NULL)
    {
        bcmuLog_error("NULL input args");
        return BCMRET_INVALID_ARGUMENTS;
    }

    bcmuLog_debug("Entered:");

    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INTERNAL_ERROR;
    }

    // During system startup, there is a small race condition between
    // sysmgmt registering for notification and actually being ready to
    // receive it.
    while (tries < 3)
    {
       // call sys_directory on D-Bus (see sysdir_dbus.h)
       gresult = g_dbus_proxy_call_sync(outProxy,
                  "notifyEvent",
                  g_variant_new("(uus)", eventType, eid, strData),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,
                  NULL,
                  &error);
       if (error != NULL)
       {
           if (tries > 0)
           {
             bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
             bcmuLog_error("[%d] destAddr %s eventType %d eid %d data %s",
                           tries, destAddr->compName, eventType, eid, strData);
           }
           g_error_free(error);
           error = NULL;
           ret = BCMRET_INTERNAL_ERROR;
           sleep(1);
           tries++;
           dbusIntf_freeOutboundHandle(outProxy);
           outProxy = dbusIntf_getOutboundHandle(destAddr);
           continue;
       }
       else
       {
           guint32 gret = 0;
           g_variant_get(gresult, "(u)", &gret);
           bcmuLog_debug("dbus method returned %d", gret);
           ret = gret;
           g_variant_unref(gresult);
           break;
       }
    }

    dbusIntf_freeOutboundHandle(outProxy);
    return ret;
}


void dbus_in_notifyEvent(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    guint32 eventType = 0;
    guint32 eid = 0;
    gchar *strDataIn = NULL;

    bcmuLog_debug("Entered:");

    /* retrieve all input parameters */
    g_variant_get (parameters, "(uus)", &eventType, &eid, &strDataIn);
    bcmuLog_notice("eventType=%d eid=%d strDataIn=%s", eventType, eid, strDataIn);

    if (_zbusConfig.processNotifyEventFp != NULL)
    {
        CmsMsgHeader *msg=NULL;
        // convert notification data to a CMS message which is sent to subscriber.
        msg = zbusUtil_createEventNotificationMsg(eventType, eid, strDataIn);
        if (msg != NULL)
        {
            // This function will free the msg.
            (*_zbusConfig.processNotifyEventFp)(_zbusConfig.msgData, msg);
        }
    }
    else
    {
         bcmuLog_error("processNotifyEventFp is NULL, cannot pass along "
                    "notification event %d strData=%s", eventType, strDataIn);
    }

    g_free (strDataIn);
    *result = g_variant_new ("(u)", 0);
    bcmuLog_notice("exit");
    return;
}

static ZbusMethodInfo notifyMethod = {
        .name = "notifyEvent",
        .introspection = DBUS_NOTIFYEVENT_METHOD_INTROSPECTION,
        .handler = dbus_in_notifyEvent,
        .intfHandler = NULL,
        .objPath = ""
};

BcmRet busIntf_addNotifyMethod(ZbusMethodContext *ctx)
{
   return (zbusIntf_addMethod(ctx, &notifyMethod));
}


BcmRet bus_out_firewallCtl(const ZbusAddr *destAddr, const char *compName,
                           const char *iptablesStr)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    GError *error = NULL;
    BcmRet ret=BCMRET_INVALID_ARGUMENTS;

    if (destAddr == NULL || iptablesStr == NULL)
    {
        bcmuLog_error("NULL input args");
        return BCMRET_INVALID_ARGUMENTS;
    }

    bcmuLog_notice("Entered: str=%s", iptablesStr);

    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INTERNAL_ERROR;
    }

    // call firewalld on D-Bus (see firewalld.xml)
    gresult = g_dbus_proxy_call_sync(outProxy,
                  "bcm_setHostIptablesRule",
                  g_variant_new("(ss)", compName, iptablesStr),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,
                  NULL,
                  &error);
    if (error != NULL)
    {
        bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
        g_error_free(error);
        ret = BCMRET_INTERNAL_ERROR;
    }
    else
    {
        guint32 gret = 0;
        gchar *errorStr = NULL;

        g_variant_get(gresult, "(us)", &gret, &errorStr);
        bcmuLog_debug("bcm_setIptablesRule dbus method returned %d (%s)",
                      gret, errorStr);
        g_free(errorStr);
        ret = gret;
        g_variant_unref(gresult);
    }

    dbusIntf_freeOutboundHandle(outProxy);
    return ret;
}


BcmRet bus_out_mdmOp(const ZbusAddr *destAddr,
                     const char *op, const char *arg, char **data)
{
    GDBusProxy *outProxy = NULL;
    GVariant *gresult = NULL;
    GError *error = NULL;
    guint32 gret = 0;
    const char *outArg;
    const gchar *retData = NULL;

    outArg = (arg != NULL) ? arg : "";
    bcmuLog_notice("Entered: destAddr=%s op=%s outArg=%s",
                   destAddr->compName, op, outArg);

    outProxy = dbusIntf_getOutboundHandle(destAddr);
    if (outProxy == NULL)
    {
        return BCMRET_INTERNAL_ERROR;
    }

    // make outbound call on D-Bus
    gresult = g_dbus_proxy_call_sync(outProxy,
                  "mdmOp",
                  g_variant_new("(ss)", op, outArg),
                  G_DBUS_CALL_FLAGS_NONE,
                  -1,
                  NULL,
                  &error);

    dbusIntf_freeOutboundHandle(outProxy);

    if (error != NULL)
    {
        bcmuLog_error("g_dbus_proxy_call_sync error: %s", error->message);
        g_error_free(error);
        return BCMRET_INTERNAL_ERROR;
    }

    g_variant_get(gresult, "(&su)", &retData, &gret);
    bcmuLog_notice("mdmop %s returned %d", op, gret);

    // Even if the op does not generate any return data, D-Bus will always
    // return at least an empty string in the retData.  Instead of checking
    // the op to see if there will be return data, which is hard to maintain
    // because everytime we add an op, we might have to update the list,
    // simply copy back data if caller provided a data ptr.  Caller should
    // know whether the op will generate return data or not.
    if (gret == BCMRET_SUCCESS)
    {
       guint32 retDataLen = strlen(retData);
       bcmuLog_notice("returned data len=%d (data=%p)", retDataLen, data);

       if (data != NULL)
       {
          /* allocate the buffer to hold the return data, caller must free */
          *data = cmsMem_alloc(retDataLen+1, ALLOC_ZEROIZE);
          if (*data != NULL)
          {
             strcpy(*data, retData);
          }
          else
          {
             gret = BCMRET_RESOURCE_EXCEEDED;
             bcmuLog_error("Could not alloc buf of %d bytes", retDataLen);
          }
       }
    }
    else
    {
       bcmuLog_notice("mdmOp %s failed, ret=%d", op, gret);
    }

    g_variant_unref(gresult);

    return ((BcmRet) gret);
}

