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

/*!\file  dbus_mdm.c
 * \brief Various MDM dependent functions.
 */

#include <stdio.h>
#include <errno.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include "bdk_dbus.h"
#include "bcm_zbus_intf.h"
#include "bcm_zbus_mdm.h"
#include "bcm_dbus_intf.h"
#include "bcm_ulog.h"
#include "bcm_retcodes.h"
#include "cms_mem.h"


// All component MD's support these methods.
static ZbusMethodInfo compMdMethods[] =
{
    {
        .name = "notifyEvent",
        .introspection = DBUS_NOTIFYEVENT_METHOD_INTROSPECTION,
        .handler = dbus_in_notifyEvent,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "getParameterNames",
        .introspection = DBUS_GETPARAMETERNAMES_METHOD_INTROSPECTION,
        .handler = dbus_in_getParameterNames,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "getParameterValues",
        .introspection = DBUS_GETPARAMETERVALUES_METHOD_INTROSPECTION,
        .handler = dbus_in_getParameterValues,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "setParameterValues",
        .introspection = DBUS_SETPARAMETERVALUES_METHOD_INTROSPECTION,
        .handler = dbus_in_setParameterValues,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "getParameterAttributes",
        .introspection = DBUS_GETPARAMETERATTRIBUTES_METHOD_INTROSPECTION,
        .handler = dbus_in_getParameterAttributes,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "setParameterAttributes",
        .introspection = DBUS_SETPARAMETERATTRIBUTES_METHOD_INTROSPECTION,
        .handler = dbus_in_setParameterAttributes,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "addObject",
        .introspection = DBUS_ADDOBJECT_METHOD_INTROSPECTION,
        .handler = dbus_in_addObject,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "deleteObject",
        .introspection = DBUS_DELETEOBJECT_METHOD_INTROSPECTION,
        .handler = dbus_in_deleteObject,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "getNextObject",
        .introspection = DBUS_GETNEXTOBJECT_METHOD_INTROSPECTION,
        .handler = dbus_in_getNextObject,
        .intfHandler = NULL,
        .objPath = ""
    },
    {
        .name = "mdmOp",
        .introspection = DBUS_MDMOP_METHOD_INTROSPECTION,
        .handler = dbus_in_mdmOp,
        .intfHandler = NULL,
        .objPath = ""
    },
};


BcmRet busIntf_addCompMdMethods(ZbusMethodContext *ctx)
{
   int numMethods = sizeof(compMdMethods)/sizeof(ZbusMethodInfo);
   int i;
   BcmRet ret;

   // Go through our array and use the zbusIntf to add one-by-one.
   for (i = 0; i < numMethods; i++)
   {
      ret = zbusIntf_addMethod(ctx, &(compMdMethods[i]));
      BCM_RETURN_IF_NOT_SUCCESS(ret);
   }

   // Also add the notify method, which is in bcm_dbus_intf (non-MDM)
   ret = zbusIntf_addNotifyMethod(ctx);

   bcmuLog_notice("added %d methods, num=%d (max=%d)",
                  numMethods+1, ctx->numMethods, ctx->maxMethods);
   return ret;
}


void dbus_in_mdmOp(void *input, void **output)
{
    GVariant *parameters = (GVariant *) input;
    GVariant **result = (GVariant **) output;
    gchar *op = NULL;
    gchar *args = NULL;
    BcmRet ret;
    gchar *retData=NULL;

    bcmuLog_debug("Entered:");

    /* retrieve all input parameters */
    g_variant_get (parameters, "(ss)", &op, &args);
    bcmuLog_notice("mdmOp=%s args=%s", op, args);

    /*
     * Call Z-Bus generic method to do the real work.
     */
    ret = zbus_in_mdmOp(op, args, &retData);

    if (retData == NULL)
    {
       *result = g_variant_new ("(su)", "", ret);
    }
    else
    {
       *result = g_variant_new ("(su)", retData, ret);
       cmsMem_free(retData);
    }

    g_free (op);
    g_free (args);

    bcmuLog_notice("exit: ret=%d", ret);
    return;
}
