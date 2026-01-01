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

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"
#include "os_defs.h"

#include "libubus.h"
#include "uloop.h"

#include "bcm_ubus_intf.h"

#define SOCKET_IDLE  (0)
#define SOCKE_INUSE  (1)

#define MAGIC_UBUS   (0x75627573)

/*zbus supports md to add two kinds of socket into bus mainloop
* one is for cms message, the other one is for monitor(openplatform).
* the entries would be changed according to zbus structure ZbusConfig
*/
typedef enum{
   USER_SOCKET_CMSMSG = 1,
   USER_SOCKET_MONITOR,
   USER_SOCKET_MAX
}user_socket_type;

struct user_socket
{
   struct uloop_fd fd;  //this must be the first member
   void *user_data;
   int (*handler)(void *);
   user_socket_type type;
   int status;	//0: idle, 1: in use
};

struct ubus_context *bus_context = NULL;
struct blob_buf blob_buffer;


/*Now only support one object for each ubus client*/
static struct ubus_object bus_object;

static struct ubus_object_type bus_object_type;

static struct ubus_method *bus_methods = NULL;

static struct user_socket cmsmsg_socket =
{
   .type = USER_SOCKET_CMSMSG,
};

static struct user_socket monitor_socket =
{
   .type = USER_SOCKET_MONITOR,
};


static int ubus_method_handler(struct ubus_context *ctx,
      struct ubus_object *obj  __attribute__((unused)),
      struct ubus_request_data *req,
      const char *method,
      struct blob_attr *msg)
{
   int i;
   ZbusMethodInfo *m;

   bcmuLog_notice("handle method call: [method=%s]", method);

   for(i = 0; i < _zbusMethodCtx->numMethods; i++)
   {
      m = &(_zbusMethodCtx->methodInfos[i]);
      if(0 == strcmp(method, m->name))
      {
         if(m->intfHandler)
         m->intfHandler((void *)ctx, (void *)req, (void *)method, (void *)msg);

         break;
      }
   }
   return 0;
}

static void ubus_fd_handle(struct uloop_fd *u, unsigned int events __attribute__((unused)))
{
   struct user_socket *socket = container_of(u, struct user_socket,fd);

   if((USER_SOCKET_CMSMSG == socket->type) ||(USER_SOCKET_MONITOR == socket->type))
   {
      if(socket->handler)
         socket->handler(socket->user_data);
   }

   /*TODO: handle other event*/
}

static void stop_user_socket(user_socket_type type)
{
   if((USER_SOCKET_CMSMSG == type) && (SOCKE_INUSE == cmsmsg_socket.status))
      uloop_fd_delete((struct uloop_fd *)&cmsmsg_socket);
   else if((USER_SOCKET_MONITOR == type) && (SOCKE_INUSE == monitor_socket.status))
      uloop_fd_delete((struct uloop_fd *)&monitor_socket);
}


int busIntf_mainLoop()
{
   int ret = 0;
   int i, buf_length = 0;
   struct ubus_method *method;
   struct uloop_timeout periodicTimeout;

   uloop_init();

   signal(SIGPIPE, SIG_IGN);

   if (_zbusMethodCtx != NULL)
   {
      if('\0' == _zbusConfig.busName[0])
      {
         bcmuLog_error("invalid parameter: no valid bus name");
         return -2;
      }

      bus_context = ubus_connect(NULL);  /*use default socket address*/
      if(!bus_context)
      {
         bcmuLog_error("could not connect to ubus!");
         return -3; //follow dbus_intf style
      }
      ubus_add_uloop(bus_context);

      if(_zbusConfig.busNameAcquiredFp)
         _zbusConfig.busNameAcquiredFp(bus_context);

      buf_length = sizeof(struct ubus_method) * _zbusMethodCtx->numMethods;
      bus_methods = malloc(buf_length);
      if(NULL == bus_methods)
      {
         bcmuLog_error("not enough memeory to hold array of info of bus methods");
         goto MAINLOOP_EXIT;
      }
      memset(bus_methods, 0, buf_length);

      for(i = 0; i < _zbusMethodCtx->numMethods; i++)
      {
         method = &bus_methods[i];
         method->name = _zbusMethodCtx->methodInfos[i].name;
         method->handler = ubus_method_handler;
         method->policy  = (struct blobmsg_policy *)(_zbusMethodCtx->methodInfos[i].policy);
         method->n_policy = _zbusMethodCtx->methodInfos[i].policyEntries;
      }

      bus_object_type.methods = bus_methods;
      bus_object_type.name = _zbusConfig.busName;
      bus_object_type.n_methods = _zbusMethodCtx->numMethods;

      bus_object.name = _zbusConfig.busName;
      bus_object.path = _zbusConfig.busName;
      bus_object.type = &bus_object_type;
      bus_object.methods = bus_methods;
      bus_object.n_methods = _zbusMethodCtx->numMethods;

      ret = ubus_add_object(bus_context, &bus_object);
      if(ret)
      {
         bcmuLog_error("failed to add object: %s", ubus_strerror(ret));
         goto MAINLOOP_EXIT;
      }
   }

   if((CMS_INVALID_FD != _zbusConfig.msgFd) && (_zbusConfig.processBcmMsgFp))
   {
      cmsmsg_socket.fd.fd = _zbusConfig.msgFd;
      cmsmsg_socket.fd.cb = ubus_fd_handle;
      cmsmsg_socket.handler = _zbusConfig.processBcmMsgFp;
      cmsmsg_socket.user_data = _zbusConfig.msgData;
      /* uloop_fd_add() will set the fd to be NONBLOCKING if ULOOP_BLOCKING is not passed in
       * to it as flag. This behavior will break cmsMsg_sendAndGetReply(). So we add ULOOP_BLOCKING
       * here to make sure the fd is still in BLOCKING mode.
       */
      uloop_fd_add((struct uloop_fd *)&cmsmsg_socket, ULOOP_READ | ULOOP_BLOCKING);
      cmsmsg_socket.status = SOCKE_INUSE;
   }

   if((CMS_INVALID_FD != _zbusConfig.monitorFd) && (_zbusConfig.processMonitorFp))
   {
      monitor_socket.fd.fd = _zbusConfig.monitorFd;
      monitor_socket.fd.cb = ubus_fd_handle;
      monitor_socket.handler = _zbusConfig.processMonitorFp;
      uloop_fd_add((struct uloop_fd *)&monitor_socket, ULOOP_READ | ULOOP_EDGE_TRIGGER);
      monitor_socket.status = SOCKE_INUSE;
   }

   /* Add periodic timeout event */
   if (_zbusConfig.periodicTimeoutMs && (_zbusConfig.periodicTimeoutFp))
   {
      memset(&periodicTimeout, 0, sizeof(struct uloop_timeout));
      periodicTimeout.cb = (uloop_timeout_handler)(void *)(_zbusConfig.periodicTimeoutFp);
      uloop_timeout_set(&periodicTimeout, _zbusConfig.periodicTimeoutMs);
   }

   uloop_run();

MAINLOOP_EXIT:
   ubus_free(bus_context);
   uloop_done();

   if(bus_methods)
      free(bus_methods);

   stop_user_socket(USER_SOCKET_CMSMSG);
   stop_user_socket(USER_SOCKET_MONITOR);

   return 0;
}

// Create and return an outgoing handle to the specified component.
void *ubusIntf_getOutboundHandle(const ZbusAddr *dest)
{
   UbusOutboundHandle *handle = NULL;
   uint32_t id;

   if(NULL == dest)
      return NULL;

   bcmuLog_notice("Creating outbound handle to %s", dest->busName);

   /*Handle one-shot ubus method call.
   * in this case, the application does not connect to ubus by starting the
   * main loop, because it just want to issue the ubus method call and exit.
   * so connect to the bus if not connected
   */
   if(!bus_context)
   {
      bus_context = ubus_connect(NULL);  /*use default socket address*/
      if(!bus_context)
      {
         bcmuLog_error("could not connect to ubus!");
         return NULL;
      }
   }

   if(ubus_lookup_id(bus_context, dest->busName, &id) == UBUS_STATUS_OK)
   {
      //TODO: use cmsMem API
      handle = malloc(sizeof(UbusOutboundHandle));
      if(handle)
      {
         handle->ctx = bus_context;
         handle->id = id;
         blob_buf_init(&blob_buffer, 0);
         handle->buf = &blob_buffer;
         handle->magic = MAGIC_UBUS;
      }
   }

   return handle;
}

void ubusIntf_freeOutboundHandle(void *handle)
{
   UbusOutboundHandle *h = handle;
   if(h && (MAGIC_UBUS == h->magic))
   {
      blob_buf_free(&blob_buffer);
      free(h);
   }
}


