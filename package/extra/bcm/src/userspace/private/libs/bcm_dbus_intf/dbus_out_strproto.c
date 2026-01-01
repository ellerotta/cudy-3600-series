/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
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


/*!\file dbus_out_strproto.c
 * \brief Actual D-Bus access methods for outbound (zbus_out_xxx) functions.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>
#include <glib-unix.h>

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "bcm_dbus_intf.h"
#include "cms_mem.h"



// Copy getParameterValue data from D-Bus iterator to a paramInfoArray.
static BcmGenericParamInfo *copyIterToParamInfoArray(GVariantIter *iter,
                                                     UINT32 *numParamInfos)
{
   const gchar *fullpath=NULL, *paramType=NULL, *paramValue=NULL, *profile=NULL;
   gboolean writable=FALSE, isPassword=FALSE;
   BcmGenericParamInfo *paramInfoArray=NULL;
   UINT32 count;

   count = g_variant_iter_n_children(iter);
   bcmuLog_debug("iter has %d elements", count);

   paramInfoArray = cmsMem_alloc(count * sizeof(BcmGenericParamInfo), ALLOC_ZEROIZE);
   if (paramInfoArray == NULL)
   {
      bcmuLog_error("Failed to alloc %d BcmGenericParamInfos", count);
   }
   else
   {
      UINT32 i = 0;
      // copy data to return array
      while (g_variant_iter_loop(iter, "(&s&s&s&sbb)",
                                 &fullpath, &paramType, &paramValue, &profile,
                                 &writable, &isPassword))
      {
         bcmuLog_debug("[%d]%s : type=%s value=%s profile=%s writable=%d isPassword=%d",
                       i, fullpath, paramType, paramValue, profile,
                       writable, isPassword);
         paramInfoArray[i].fullpath = cmsMem_strdup(fullpath);
         paramInfoArray[i].type = cmsMem_strdup(paramType);
         paramInfoArray[i].value = cmsMem_strdup(paramValue);
         paramInfoArray[i].profile = cmsMem_strdup(profile);
         paramInfoArray[i].writable = writable;
         paramInfoArray[i].isPassword = isPassword;
         i++;
      }
      *numParamInfos = count;
   }

   return paramInfoArray;
}


BcmRet bus_out_getParameterNames(const ZbusAddr *destAddr,
                const char *fullpath, UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   GDBusProxy *proxy = NULL;
   GError *gerror = NULL;
   GVariant *gresult = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered: fullpath=%s dest compName=%s",
                  fullpath, destAddr->compName);

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
       bcmuLog_error("Failed to create proxy!");
       return BCMRET_INTERNAL_ERROR;
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                    "getParameterNames",
                    g_variant_new("(sbu)", fullpath, nextLevel, flags),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,  //TODO: set timeout to be consistent with UBus
                    NULL,
                    &gerror);

   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
      bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                    destAddr->compName, gerror->message);
      g_error_free(gerror);
      ret = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      GVariantIter *iter = NULL;
      const gchar *response = NULL;

      g_variant_get(gresult, "(a*&su)", &iter, &response, &ret);
      bcmuLog_notice("dbus method returned %d (%s)", ret, response);
      if (ret == BCMRET_SUCCESS)
      {
         const gchar *fullpath=NULL, *paramType=NULL, *profile=NULL;
         gboolean writable=FALSE, isPassword=FALSE;
         BcmGenericParamInfo *retArray=NULL;
         UINT32 count;

         count = g_variant_iter_n_children(iter);
         bcmuLog_debug("got %d in iterator", count);

         retArray = cmsMem_alloc(count * sizeof(BcmGenericParamInfo), ALLOC_ZEROIZE);
         if (retArray == NULL)
         {
            bcmuLog_error("Failed to alloc %d BcmGenericParamInfos", count);
            ret = BCMRET_RESOURCE_EXCEEDED;
         }
         else
         {
            // copy data to return array
            UINT32 i = 0;
            while (g_variant_iter_loop(iter,
                                       "(&s&s&sbb)",
                                       &fullpath, &paramType, &profile,
                                       &writable, &isPassword))
            {
               bcmuLog_debug("[%d]%s : paramType=%s profile=%s writable=%d isPassword=%d",
                             i, fullpath, paramType, profile,
                             writable, isPassword);
               retArray[i].fullpath = cmsMem_strdup(fullpath);
               retArray[i].type = cmsMem_strdup(paramType);
               retArray[i].profile = cmsMem_strdup(profile);
               retArray[i].writable = writable;
               retArray[i].isPassword = isPassword;
               i++;
            }
         }
         *paramInfoArray = retArray;
         *numParamInfos = count;
      }

      g_variant_iter_free(iter);
      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet bus_out_getParameterValues(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   GVariantBuilder *builder;
   GDBusProxy *proxy = NULL;
   GError *gerror = NULL;
   GVariant *gresult = NULL;
   UINT32 i;
   BcmRet ret;

   bcmuLog_notice("Entered: numEntries=%d nextLevel=%d flags=0x%x",
                 numEntries, nextLevel, flags);

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
       bcmuLog_error("Failed to create proxy!");
       return BCMRET_INTERNAL_ERROR;
    }

   // put input fullpath array into builder
   builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
   for (i = 0; i < numEntries; i++)
   {
      bcmuLog_debug("[%d]: %s", i, fullpathArray[i]);
      g_variant_builder_add(builder, "s", fullpathArray[i]);
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                          "getParameterValues",
                          g_variant_new("(asbu)", builder, nextLevel, flags),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          &gerror);

   g_variant_builder_unref(builder);
   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
      bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                    destAddr->compName, gerror->message);
      g_error_free(gerror);
      ret = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      GVariantIter *iter = NULL;
      const gchar *response = NULL;

      g_variant_get(gresult, "(a*&su)", &iter, &response, &ret);
      bcmuLog_notice("dbus method returned %d (%s)", ret, response);
      if (ret == BCMRET_SUCCESS)
      {
         *paramInfoArray = copyIterToParamInfoArray(iter, numParamInfos);
         if (*paramInfoArray == NULL)
            ret = BCMRET_RESOURCE_EXCEEDED;
      }

      g_variant_iter_free(iter);
      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d numParamInfos=%d", ret, *numParamInfos);
   return ret;
}


BcmRet bus_out_setParameterValues(const ZbusAddr *destAddr,
                BcmGenericParamInfo *paramInfoArray, UINT32 numParamInfos,
                UINT32 flags)
{
   GDBusProxy *proxy = NULL;
   GVariantBuilder *builder;
   GError *gerror = NULL;
   GVariant *gresult = NULL;
   BcmGenericParamInfo *paramInfo = NULL;
   UINT32 i;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered:");

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
      bcmuLog_error("Failed to create proxy!");
      return BCMRET_INTERNAL_ERROR;
   }

   // build outbound array(name_type_value)
   builder = g_variant_builder_new(G_VARIANT_TYPE("a(sss)"));
   for (i = 0; i < numParamInfos; i++)
   {
      paramInfo = &(paramInfoArray[i]);

      if ((paramInfo->fullpath == NULL) ||
          (paramInfo->type == NULL) ||
          (paramInfo->value == NULL) ||
          (g_utf8_validate(paramInfo->value, -1, NULL) == FALSE))
      {
         bcmuLog_error("Invalid input params [%d] %s %s %s", i,
                       paramInfo->fullpath, paramInfo->type, paramInfo->value);
         g_variant_builder_unref(builder);
         dbusIntf_freeOutboundHandle(proxy);
         paramInfo->errorCode = BCMRET_INVALID_ARGUMENTS;
         return BCMRET_INVALID_ARGUMENTS;
      }
      bcmuLog_debug("[%d] %s %s %s", i,
                    paramInfo->fullpath, paramInfo->type, paramInfo->value);
      g_variant_builder_add(builder, "(sss)",
                    paramInfo->fullpath, paramInfo->type, paramInfo->value);
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                                    "setParameterValues",
                                    g_variant_new("(a*u)", builder, flags),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &gerror);

   g_variant_builder_unref(builder);
   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
      bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                    destAddr->compName, gerror->message);
      g_error_free(gerror);
      ret = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      GVariantIter *iter = NULL;
      const gchar *response = NULL;
      guint32 resultCode = 0;
      const gchar *fullpath = NULL;
      UINT32 errorCode = 0;

      g_variant_get(gresult, "(a*&su)", &iter, &response, &resultCode);
      bcmuLog_notice("dbus method returned %d (%s)", resultCode, response);
      ret = resultCode;

      // iter contains fullpaths that had errors.  This is a sparse list.
      // Find the fullpath in the paramInfo input array and set errorCode.
      while (g_variant_iter_loop(iter, "(&su)", &fullpath, &errorCode))
      {
         bcmuLog_error("got specific error on %s (%d)", fullpath, errorCode);
         for (i = 0; i < numParamInfos; i++)
         {
            if (!strcmp(paramInfoArray[i].fullpath, fullpath))
            {
               paramInfoArray[i].errorCode = errorCode;
               break;
            }
         }
         if (i >= numParamInfos)
         {
            bcmuLog_error("Could not find %s in paramInfoArray", fullpath);
            ret = BCMRET_INTERNAL_ERROR;
         }
      }
      g_variant_iter_free(iter);
      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet bus_out_getParameterAttributes(const ZbusAddr *destAddr,
                const char **fullpathArray, UINT32 numEntries,
                UBOOL8 nextLevel, UINT32 flags,
                BcmGenericParamAttr **paramAttrArray, UINT32 *numParamAttrs)
{
   GDBusProxy *proxy = NULL;
   GVariantBuilder *builder = NULL;
   GVariant *gresult = NULL;
   GError *gerror = NULL;
   UINT32 i;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered:");

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
      bcmuLog_error("Failed to create proxy!");
      return BCMRET_INTERNAL_ERROR;
   }

   // build outbound fullpath array
   builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
   for (i = 0; i < numEntries; i++)
   {
      bcmuLog_debug("[%d]: %s", i, fullpathArray[i]);
      g_variant_builder_add(builder, "s", fullpathArray[i]);
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                          "getParameterAttributes",
                          g_variant_new("(asbu)", builder, nextLevel, flags),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          &gerror);

   g_variant_builder_unref(builder);
   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
      bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                    destAddr->compName, gerror->message);
      g_error_free(gerror);
   }
   else
   {
      GVariantIter *iter = NULL;
      const gchar *response = NULL;
      BcmGenericParamAttr *retArray = NULL;

      g_variant_get(gresult, "(a*&su)", &iter, &response, &ret);
      bcmuLog_notice("dbus method returned %d (%s)", ret, response);
      if (ret == BCMRET_SUCCESS)
      {
         const gchar *fullpath = NULL;
         guint16 access=0, notif=0, valueChanged=0, altNotif=0, altValue=0;
         UINT32 count;

         count = g_variant_iter_n_children(iter);
         bcmuLog_debug("got %d elements in iterator", count);

         retArray = cmsMem_alloc(count * sizeof(BcmGenericParamAttr), ALLOC_ZEROIZE);
         if (retArray == NULL)
         {
            bcmuLog_error("Failed to alloc %d BcmGenericParamAttrs", count);
         }
         else
         {
            // copy data to return array
            i = 0;
            while (g_variant_iter_loop(iter, "(&sqqqqq)", &fullpath,
                                             &access, &notif, &valueChanged,
                                             &altNotif, &altValue))
            {
               bcmuLog_debug("[%d]%s : access=0x%x notif=0x%x valueChanged=0x%x "
                             "altNotif=0x%x altValue=0x%x",
                             i, fullpath, access, notif, valueChanged,
                             altNotif, altValue);
               retArray[i].fullpath = cmsMem_strdup(fullpath);
               retArray[i].access = access;
               retArray[i].notif = notif;
               retArray[i].valueChanged = valueChanged;
               retArray[i].altNotif = altNotif;
               retArray[i].altNotifValue = altValue;
               i++;
            }
            *paramAttrArray = retArray;
            *numParamAttrs = count;
         }
      }

      g_variant_iter_free(iter);
      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d numParamAttrs=%d", ret, *numParamAttrs);
   return ret;
}


BcmRet bus_out_setParameterAttributes(const ZbusAddr *destAddr,
                BcmGenericParamAttr *paramAttrArray, UINT32 numParamAttrs,
                UINT32 flags)
{
   GDBusProxy *proxy = NULL;
   GVariantBuilder *builder = NULL;
   GVariant *gresult = NULL;
   GError *gerror = NULL;
   UINT32 i;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered:");

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
      bcmuLog_error("Failed to create proxy!");
      return BCMRET_INTERNAL_ERROR;
   }

   // build outbound fullpath array
   // (sbqbqbqb) fullpath_setAccess_access_setNotif_notif_setAltNotif_altNotif_clearAltValue
   builder = g_variant_builder_new(G_VARIANT_TYPE("a(sbqbqbqb)"));
   for (i = 0; i < numParamAttrs; i++)
   {
      bcmuLog_debug("[%d]: %s %d(0x%x) %d(0x%x) %d(0x%x) %d",
                    i, paramAttrArray[i].fullpath,
                    paramAttrArray[i].setAccess, paramAttrArray[i].access,
                    paramAttrArray[i].setNotif, paramAttrArray[i].notif,
                    paramAttrArray[i].setAltNotif, paramAttrArray[i].altNotif,
                    paramAttrArray[i].clearAltNotifValue);
      g_variant_builder_add(builder, "(sbqbqbqb)",
                    paramAttrArray[i].fullpath,
                    paramAttrArray[i].setAccess, paramAttrArray[i].access,
                    paramAttrArray[i].setNotif, paramAttrArray[i].notif,
                    paramAttrArray[i].setAltNotif, paramAttrArray[i].altNotif,
                    paramAttrArray[i].clearAltNotifValue);
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                                    "setParameterAttributes",
                                    g_variant_new("(a*u)", builder, flags),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &gerror);

   g_variant_builder_unref(builder);
   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
      bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                    destAddr->compName, gerror->message);
      g_error_free(gerror);
   }
   else
   {
      GVariantIter *iter = NULL;
      const gchar *response = NULL;

      g_variant_get(gresult, "(a*&su)", &iter, &response, &ret);
      bcmuLog_notice("dbus method returned %d (%s)", ret, response);

      // TODO: the other side does not return any individual error codes yet,
      // so we know the fullpath_error iterator is empy.  When it is
      // implemented, we will need to match the returned fullpath to the 
      // paramAttrArray and set the individual error codes just like in
      // setParameterValues.
      g_variant_iter_free(iter);
      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet bus_out_addObject(const ZbusAddr *destAddr,
                          const char *fullpath, UINT32 flags,
                          UINT32 *instanceNumber)
{
   GDBusProxy *proxy = NULL;
   GVariant *gresult = NULL;
   GError *gerror = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered:");

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
      bcmuLog_error("Could not get outbound handle for %s",
                     destAddr->compName);
      return BCMRET_INTERNAL_ERROR;
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                                    "addObject",
                                    g_variant_new("(su)", fullpath, flags),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &gerror);

   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
       bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                     destAddr->compName, gerror->message);
       g_error_free(gerror);
       ret = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      const gchar *response = NULL;

      g_variant_get(gresult, "(u&su)", instanceNumber, &response, &ret);
      bcmuLog_notice("method returned: instanceNumber %d, ret %d (%s)",
                     *instanceNumber, ret, response);

      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet bus_out_deleteObject(const ZbusAddr *destAddr,
                             const char *fullpath, UINT32 flags)
{
   GDBusProxy *proxy = NULL;
   GVariant *gresult = NULL;
   GError *gerror = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered:");

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
      bcmuLog_error("Could not get outbound handle for %s",
                     destAddr->compName);
      return BCMRET_INTERNAL_ERROR;
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                                    "deleteObject",
                                    g_variant_new("(su)", fullpath, flags),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &gerror);

   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
       bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                     destAddr->compName, gerror->message);
       g_error_free(gerror);
       ret = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      const gchar *response = NULL;

      g_variant_get(gresult, "(&su)", &response, &ret);
      bcmuLog_notice("method returned: ret %d (%s)", ret, response);

      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d", ret);
   return ret;
}


BcmRet bus_out_getNextObject(const ZbusAddr *destAddr,
                const char *fullpath, const char *limitSubtree, UINT32 flags,
                char **nextFullpath,
                BcmGenericParamInfo **paramInfoArray, UINT32 *numParamInfos)
{
   GDBusProxy *proxy = NULL;
   GVariant *gresult = NULL;
   GError *gerror = NULL;
   BcmRet ret = BCMRET_INTERNAL_ERROR;

   bcmuLog_notice("Entered: fullpath=%s limitSubtree=%s", fullpath, limitSubtree);

   proxy = dbusIntf_getOutboundHandle(destAddr);
   if (proxy == NULL)
   {
      bcmuLog_error("Could not get outbound handle for %s",
                     destAddr->compName);
      return BCMRET_INTERNAL_ERROR;
   }

   gresult = g_dbus_proxy_call_sync(proxy,
                        "getNextObject",
                        g_variant_new("(ssu)", fullpath, limitSubtree, flags),
                        G_DBUS_CALL_FLAGS_NONE,
                        -1,
                        NULL,
                        &gerror);

   dbusIntf_freeOutboundHandle(proxy);

   if (gerror != NULL)
   {
      bcmuLog_error("g_dbus_proxy_call_sync error for %s: %s",
                    destAddr->compName, gerror->message);
      g_error_free(gerror);
      ret = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      const gchar *cstFullpath = NULL;
      const gchar *response = NULL;
      GVariantIter *iter = NULL;

      g_variant_get(gresult, "(&sa*&su)", &cstFullpath, &iter, &response, &ret);
      bcmuLog_notice("dbus method returned: %d (%s) nextFullpath=%s",
                     ret, response, cstFullpath);
      // Note that getNextObject can return BCMRET_NO_MORE_INSTANCES, which
      // is technically an error, but also expected at the end of a walk.
      if (ret == BCMRET_SUCCESS)
      {
         *paramInfoArray = copyIterToParamInfoArray(iter, numParamInfos);
         if (*paramInfoArray == NULL)
         {
            ret = BCMRET_RESOURCE_EXCEEDED;
         }
         else
         {
            *nextFullpath = cmsMem_strdup(cstFullpath);
         }
      }

      g_variant_iter_free(iter);
      g_variant_unref(gresult);
   }

   bcmuLog_debug("ret=%d numParamInfos=%d", ret, *numParamInfos);
   return ret;
}

