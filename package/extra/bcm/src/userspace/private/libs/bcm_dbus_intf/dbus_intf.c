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

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <glib-unix.h>
#include "bcm_retcodes.h"
#include "genutil_rbtree.h"
#include "bcm_ulog.h"
#include "cms_core.h"
#include "bcm_zbus_intf.h"
#include "bdk_dbus.h"
#include "bcm_dbus_intf.h"


/*!\file dbus_intf.c
 * \brief D-Bus implemention of the generic bus interfaces defined in
 *        bcm_zbus_intf.h.  Also a lot of low level DBus code.
 *
 */


/* Introspection data for the service we are exporting   Can we free this
 * after it is used during init?
 */
static gchar *introspection_xml=NULL;


static GDBusNodeInfo *introspection_data = NULL;
static GDBusNodeInfo *comp_intro_data[ZBUS_MAX_COMP_INTF] = {0};
static GDBusConnection *md_connection = NULL;
static GDBusConnection *dbus_conn = NULL;
static GMainLoop *main_loop = NULL;
static guint registration_id = 0;
static guint comp_registration_id[ZBUS_MAX_COMP_INTF] = {0};

static BcmRet dbus_connect(void);
static void dbus_disconnect(void);
static void on_bus_acquired (GDBusConnection *connection,
                             const gchar     *name  __attribute__((unused)),
                             gpointer         user_data  __attribute__((unused)));
static void on_name_acquired (GDBusConnection *connection  __attribute__((unused)),
                              const gchar     *name,
                              gpointer         user_data  __attribute__((unused)));
static void on_name_lost (GDBusConnection *connection,
                          const gchar     *name,
                          gpointer         user_data  __attribute__((unused)));
static gboolean sigterm_handler(gpointer user_data  __attribute__((unused)));
static gboolean check_bcm_messages(gint fd __attribute__((unused)),
                                   GIOCondition condition __attribute__((unused)),
                                   gpointer data __attribute__((unused)));
static gboolean check_monitor_fd(gint fd __attribute__((unused)),
                                 GIOCondition condition __attribute__((unused)),
                                 gpointer data __attribute__((unused)));
static void handle_method_call (GDBusConnection       *connection  __attribute__((unused)),
                                const gchar           *sender __attribute__((unused)),
                                const gchar           *object_path __attribute__((unused)),
                                const gchar           *interface_name __attribute__((unused)),
                                const gchar           *method_name,
                                GVariant              *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer               user_data __attribute__((unused)));


/*
 * The dbusProxyRbt is needed to work around a memory leak in
 * g_dbus_proxy_new_sync().  There is one patch for it on the internet, which
 * I tried, but it did not fix the problem.  Ultimately, we need to upgrade
 * our D-Bus (+glib) version.  Maybe sync to Yocto-3.1?
 * For now, just create the D-Bus proxy to each "busName:objPath:intfName"
 * tuple and reuse it after that.
 */
#define PROXY_RBT_KEYLEN        1024
typedef struct dbus_proxy_node
{
   rbnode_t   rbtNode;
   GDBusProxy *proxy;
   char       keyBuf[PROXY_RBT_KEYLEN];
} DbusProxyNode;

static rbtree_t *dbusProxyRbt = NULL;


/* interface_vtable only contains handle_method_call()
 * that has GDBusInterfaceMethodCallFunc() type
 * handle_method_call is the D-Bus method to c function dispatcher.
 */
static const GDBusInterfaceVTable interface_vtable =
{
   handle_method_call, NULL, NULL, { NULL}
};

static void
handle_method_call (GDBusConnection       *connection  __attribute__((unused)),
                    const gchar           *sender __attribute__((unused)),
                    const gchar           *object_path __attribute__((unused)),
                    const gchar           *interface_name __attribute__((unused)),
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data __attribute__((unused)))
{
   GVariant *result = NULL;
   int i = 0;
   int found=0;

   bcmuLog_notice("entered: method_name=%s (numMethods=%d)",
                 method_name, _zbusMethodCtx->numMethods);

   for (i = 0; i < _zbusMethodCtx->numMethods; i++)
   {
      if (_zbusMethodCtx->methodInfos[i].handler != NULL &&
          g_strcmp0 (method_name, _zbusMethodCtx->methodInfos[i].name) == 0)
        {
           (*_zbusMethodCtx->methodInfos[i].handler)((void *) parameters,
                                                     (void **) &result);
           found = 1;
           break;
        }
   }

   if (found == 0)
   {
      bcmuLog_error("Could not find handler for %s", method_name);
   }

   if (result != NULL)
   {
      g_dbus_method_invocation_return_value (invocation, result);
   }

   return;
}  /* End of handle_method_call() */


static void
handle_intf_method_call (GDBusConnection *connection  __attribute__((unused)),
                         const gchar *sender,
                         const gchar *object_path,
                         const gchar *interface_name,
                         const gchar *method_name,
                         GVariant    *parameters,
                         GDBusMethodInvocation *invocation,
                         gpointer    user_data __attribute__((unused)))
{
   int i = 0;
   int found=0;

   bcmuLog_notice("entered: method_name=%s (numMethods=%d)",
                 method_name, _zbusMethodCtx->numMethods);

   for (i = 0; i < _zbusMethodCtx->numMethods; i++)
   {
      if (_zbusMethodCtx->methodInfos[i].intfHandler != NULL &&
          (g_strcmp0 (object_path, _zbusMethodCtx->methodInfos[i].objPath) == 0) &&
          (g_strcmp0 (interface_name, _zbusMethodCtx->methodInfos[i].name)==0))
        {
           (*_zbusMethodCtx->methodInfos[i].intfHandler)((void *)method_name,
                                                         (void *)sender,
                                                         (void *)parameters,
                                                         (void *)invocation);
           found = 1;
           break;
        }
   }

   if (found == 0)
   {
      bcmuLog_error("Could not find handler for %s/%s", object_path,
                                                        interface_name);
   }
}

static const GDBusInterfaceVTable comp_interface_vtable =
{
   handle_intf_method_call, NULL, NULL, { NULL}
};


BcmRet generate_introspection()
{
   int n, i;
   int len;
   const ZbusMethodContext *ctx = _zbusMethodCtx;

   bcmuLog_debug("Entered: ctx=%p introspection_xml=%p",
                 ctx, introspection_xml);

   if (introspection_xml != NULL)
   {
      // We've already created the introspection string, no need to do it again.
      return BCMRET_SUCCESS;
   }

   // Calculate the size of the introspection string buffer (with some
   // generous extra buffer).
   len = 1024;
   for (i=0; i < ctx->numMethods; i++)
   {
      if (ctx->methodInfos[i].handler != NULL)
      {
         if (ctx->methodInfos[i].name[0] == '\0' ||
             ctx->methodInfos[i].intfHandler != NULL ||
             ctx->methodInfos[i].introspection == NULL)
         {
            bcmuLog_error("empty or incomplete method info at %d", i);
            return BCMRET_INVALID_ARGUMENTS;
         }
         else
         {
            len += strlen(ctx->methodInfos[i].introspection);
         }
      }
      else if (ctx->methodInfos[i].intfHandler != NULL)
      {
         if (ctx->methodInfos[i].objPath[0] == '\0' ||
             ctx->methodInfos[i].name[0] == '\0' ||
             ctx->methodInfos[i].handler != NULL ||
             ctx->methodInfos[i].introspection == NULL)
         {
            bcmuLog_error("empty or incomplete interface info at %d", i);
            return BCMRET_INVALID_ARGUMENTS;
         }
         else
         {
            bcmuLog_debug("interface<%s>", ctx->methodInfos[i].name);
         }
      }
      else
      {
         bcmuLog_error("no handler for method/interface info at %d", i);
         return BCMRET_INVALID_ARGUMENTS;
      }
   }
   bcmuLog_debug("introspection string len=%d", len);
   introspection_xml = (gchar *) malloc(len);
   if (introspection_xml == NULL)
   {
       return BCMRET_RESOURCE_EXCEEDED;
   }

   n = sprintf(introspection_xml, "%s%s'>",
               DBUS_INTROSPECTION_BEGIN, _zbusConfig.intfName);

   for (i=0; i < ctx->numMethods; i++)
   {
      if (ctx->methodInfos[i].handler != NULL)
      {
         n += sprintf(&introspection_xml[n], "%s",
                      ctx->methodInfos[i].introspection);
      }
   }

   sprintf(&introspection_xml[n], DBUS_INTROSPECTION_END);

   // printf("\nFinal introspection string===>\n%s\n<===\n\n", introspection_xml);

   return BCMRET_SUCCESS;
}


int busIntf_mainLoop()
{
   guint owner_id = 0;
   GError *error = NULL;
   int i, j=0;

   bcmuLog_notice("Entered");

   if (_zbusMethodCtx != NULL)
   {
      if (generate_introspection() != BCMRET_SUCCESS)
      {
         bcmuLog_error("could not generate introspection string for DBus");
         return -2;
      }

      if (dbus_connect() != BCMRET_SUCCESS)
      {
         return -3;
      }

      /* Parse introspection_xml and returns
       * a GDBusNodeInfo that can be used to
       * lookup interface information.
       * Note that this routine is using a
       * GMarkup-based parser that only accepts
       * a subset of valid XML documents
       */
      introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, &error);
      if (error != NULL)
      {
          bcmuLog_error("Failed to parse introspection xml: %s", error->message);
          g_error_free(error);
          return BCMRET_INTERNAL_ERROR;
      }

      for (i=0; i < _zbusMethodCtx->numMethods; i++)
      {
         if (_zbusMethodCtx->methodInfos[i].intfHandler != NULL)
         {
            comp_intro_data[j] = g_dbus_node_info_new_for_xml (_zbusMethodCtx->methodInfos[i].introspection, &error);
            if (error != NULL)
            {
                bcmuLog_error("Failed to parse introspection xml: %s",
                              error->message);
                g_error_free(error);
                return BCMRET_INTERNAL_ERROR;
            }

            j++;
         }
      }
   }

   /* Acquire our bus name on the G_BUS_TYPE_SYSTEM,
    * and calls on_name_acquired and on_name_lost when
    * the name is acquired or lost respectively.
    * Callbacks will be invoked in the thread-default main
    * context of the thread you are calling this function from
    */
   bcmuLog_debug("register busName %s", _zbusConfig.busName);
   if (_zbusConfig.busName[0] != '\0')
   {
      owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
                                 _zbusConfig.busName,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 on_bus_acquired,
                                 on_name_acquired,
                                 on_name_lost,
                                 NULL,
                                 NULL);
   }

   /* Add SIGTERM handler */
   g_unix_signal_add_full (G_PRIORITY_HIGH, SIGTERM,
                           (GSourceFunc)sigterm_handler, NULL, NULL);

   /* Watch cms messages received at msgFd */
   if (_zbusConfig.msgFd != -1)
   {
      g_unix_fd_add_full (G_PRIORITY_HIGH, _zbusConfig.msgFd, G_IO_IN,
                          (GUnixFDSourceFunc)check_bcm_messages,
                          _zbusConfig.msgData, NULL);
   }

   /* Watch cms messages received at msgFd */
   if (_zbusConfig.monitorFd)
   {
      g_unix_fd_add (_zbusConfig.monitorFd, G_IO_IN,
                     (GUnixFDSourceFunc)check_monitor_fd, NULL);
   }

   /* Add periodic timeout event */
   if (_zbusConfig.periodicTimeoutMs && (_zbusConfig.periodicTimeoutFp))
   {
      g_timeout_add (_zbusConfig.periodicTimeoutMs,
                     (GSourceFunc)(_zbusConfig.periodicTimeoutFp),
                     NULL);   /* no data passed to callback function */
   }

   /* Create a new GMainLoop structure */
   main_loop = g_main_loop_new (NULL, FALSE);

   /* Enter main loop and process events from the loop */
   bcmuLog_notice("going into main D-Bus loop...");
   g_main_loop_run (main_loop);
   bcmuLog_notice("Came out of D-Bus main loop!!!");

   /* Decrease the reference count on a GMainLoop
    * object by one. If the result is zero,
    * free the loop and free all associated memory
    */
   g_main_loop_unref (main_loop);

   /* Stop owning a name */
   if (owner_id > 0)
      g_bus_unown_name (owner_id);

   /* Decrease the reference count of introspection_data.
    * When its reference count drops to 0, the memory used is freed
    */
   if (introspection_data != NULL)
      g_dbus_node_info_unref (introspection_data);

   for (i=0; i<ZBUS_MAX_COMP_INTF; i++)
   {
      if (comp_intro_data[i] != NULL)
      {
         g_dbus_node_info_unref (comp_intro_data[i]);
      }
   }

   /* exit clean up for dbus daemon communication */
   dbus_disconnect();

   return 0;

}  /* End of dbusIntf_mainLoop */


static BcmRet dbus_connect(void)
{
   GError *error = NULL;

   /* get dbus connection */
   if (dbus_conn == NULL)
   {
      bcmuLog_notice("connecting to D-Bus...");
      dbus_conn = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
      if (error != NULL)
      {
         bcmuLog_error("Failed to connect to bus, error: %s", error->message);
         g_error_free(error);
         return BCMRET_INTERNAL_ERROR;
      }
   }

   if (dbusProxyRbt == NULL)
   {
      dbusProxyRbt = (rbtree_t *) calloc(1, sizeof(rbtree_t));
      if (dbusProxyRbt == NULL)
      {
         bcmuLog_error("Failed to allocate dbusProxyRbt");
         return BCMRET_INTERNAL_ERROR;
      }

      // init rbtree with built in strcasecmp function.
      rbtree_initFlags(dbusProxyRbt, RBT_FLAGS_STRCASECMP, NULL, NULL);
   }

   return BCMRET_SUCCESS;
}


void rbt_inplace_delete(rbnode_t *rbnode, void *arg __attribute__((unused)))
{
   // According to the header file, I can free the node (but not delete it
   // from rbtree to avoid rebalance).
   DbusProxyNode *node = (DbusProxyNode *) rbnode;
   g_object_unref(node->proxy);
   free(node);
   return;
}

static void dbus_disconnect(void)
{
   bcmuLog_notice("Entered: rbt=%p conn=%p", dbusProxyRbt, dbus_conn);
   if (dbusProxyRbt != NULL)
   {
      traverse_postorder(dbusProxyRbt, rbt_inplace_delete, NULL);
      free(dbusProxyRbt);
      dbusProxyRbt = NULL;
   }

   if (dbus_conn != NULL)
   {
      g_object_unref (dbus_conn);
      dbus_conn = NULL;
   }
}


static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name  __attribute__((unused)),
                 gpointer         user_data  __attribute__((unused)))
{
   int i, j=0;

   bcmuLog_notice("bus_acquired: %s (objPath %s)", name, _zbusConfig.objPath);

   registration_id = g_dbus_connection_register_object (connection,
                           _zbusConfig.objPath,
                           introspection_data->interfaces[0],
                           &interface_vtable,
                           NULL,  /* user_data */
                           NULL,  /* user_data_free_func */
                           NULL); /* GError** */
   g_assert (registration_id > 0);

   for (i=0; i < _zbusMethodCtx->numMethods; i++)
   {
      if (_zbusMethodCtx->methodInfos[i].intfHandler != NULL)
      {
         comp_registration_id[j] = g_dbus_connection_register_object(connection,
                                   _zbusMethodCtx->methodInfos[i].objPath,
                                   comp_intro_data[j]->interfaces[0],
                                   &comp_interface_vtable,
                                   NULL,  /* user_data */
                                   NULL,  /* user_data_free_func */
                                   NULL); /* GError** */
         g_assert (comp_registration_id[j] > 0);
         j++;
      }
   }

   /* Save connection */
   md_connection = connection;
   return;
}


static void
on_name_acquired (GDBusConnection *connection  __attribute__((unused)),
                  const gchar     *name,
                  gpointer         user_data  __attribute__((unused)))
{
   bcmuLog_debug("name_acquired: %s", name);
   if (_zbusConfig.busNameAcquiredFp != NULL)
   {
      (*_zbusConfig.busNameAcquiredFp)(user_data);
   }
}


static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data  __attribute__((unused)))
{
   int i;

   bcmuLog_error("name_lost: %s, connection %p, registration_id %d", name,connection,registration_id);

   /* During shutdown (at reboot) process, connection may have been cleaned up already
    * at some point.  Continuing here will just causes exception at g_dbus_connection_unregister_object.
    */   
   if (connection != NULL)
   {
      g_dbus_connection_unregister_object(connection, registration_id);

      for (i=0; i<ZBUS_MAX_COMP_INTF; i++)
      {
         if (comp_registration_id[i] != 0)
         {
            g_dbus_connection_unregister_object(connection,
                                                comp_registration_id[i]);
         }
      }
   }
   /* quit the main loop */
   g_main_loop_quit (main_loop);
}


static gboolean sigterm_handler(gpointer user_data __attribute__((unused)))
{
   bcmuLog_notice("received SIGTERM. Terminating.");

   /* quit the main loop */
   g_main_loop_quit (main_loop);

   return TRUE;
}


// Just a wrapper for the D-bus stuff.  Strip it off and call our main
// message handling function.
static gboolean check_bcm_messages(gint fd __attribute__((unused)),
                                   GIOCondition condition __attribute__((unused)),
                                   gpointer user_data)
{
   bcmuLog_debug("received BCM msg, fp=%p", _zbusConfig.processBcmMsgFp);
   if (_zbusConfig.processBcmMsgFp != NULL)
   {
      (*_zbusConfig.processBcmMsgFp)(user_data);  // compmd_processMsg
   }
   return TRUE;
}


static gboolean check_monitor_fd(gint fd __attribute__((unused)),
                                 GIOCondition condition __attribute__((unused)),
                                 gpointer user_data)
{
   bcmuLog_debug("received monitor fd, fp=%p", _zbusConfig.processMonitorFp);
   if (_zbusConfig.processMonitorFp != NULL)
   {
      (*_zbusConfig.processMonitorFp)(user_data);
   }
   return TRUE;
}


// Create and return an outgoing proxy handle to the specified component.
GDBusProxy *dbusIntf_getOutboundHandle(const ZbusAddr *dest)
{
   GDBusProxy *proxy=NULL;
   GError *error=NULL;
   char keyBuf[PROXY_RBT_KEYLEN]={0};
   DbusProxyNode *node = NULL;
   BcmRet rc;

   bcmuLog_notice("Entered: busName=%s", dest->busName);

   // For DBus apps that only do outbound calls, automatically connect to
   // DBus.  There is no API to disconnect though.
   rc = dbus_connect();
   if (rc != BCMRET_SUCCESS)
   {
       bcmuLog_error("Failed to connect to dbus!");
       return NULL;
   }

   // Lookup proxy in RBt
   snprintf(keyBuf, sizeof(keyBuf), "%s:%s:%s",
                                    dest->busName,
                                    dest->objPath,
                                    dest->intfName);
   node = (DbusProxyNode *) rbtree_search(dbusProxyRbt, keyBuf);
   if (node != NULL)
   {
      return node->proxy;
   }

   // Not found in RBt, create a new one.
   bcmuLog_notice("creating new proxy (key=%s)", keyBuf);
   proxy = g_dbus_proxy_new_sync (dbus_conn,
                                  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                  NULL,
                                  dest->busName,
                                  dest->objPath,
                                  dest->intfName,
                                  NULL,
                                  &error);
   if (error != NULL)
   {
       bcmuLog_error("Failed to create proxy to %s error: %s",
                     dest->busName, error->message);
       g_error_free(error);
       return NULL;
   }

   // Insert proxy into RBt
   node = calloc(1, sizeof(DbusProxyNode));
   if (node == NULL)
   {
      bcmuLog_error("alloc of DbusProxyNode failed");
   }
   else
   {
      strcpy(node->keyBuf, keyBuf);
      node->rbtNode.key = node->keyBuf;
      node->proxy = proxy;
      rbtree_insert(dbusProxyRbt, (rbnode_t *) node);
   }

   return proxy;
}

void dbusIntf_freeOutboundHandle(GDBusProxy *proxy __attribute__((unused)))
{
   // Leave the proxy in the RBt.  All proxies will be freed when
   // dbus_disconnect is called.  We could add a function to really free
   // the proxy in the future, if needed.
   return;
}

