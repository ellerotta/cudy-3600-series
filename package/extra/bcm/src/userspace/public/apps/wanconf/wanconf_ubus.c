/***********************************************************************
 *
 * <:copyright-BRCM:2020:DUAL/GPL:standard
 *
 *    Copyright (c) 2020 Broadcom
 *    All Rights Reserved
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 *
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 *
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 *
 * :>
 *
 ************************************************************************/

/*!\file wanconf_ubus.c
 * \brief wanconf specific U-Bus methods.
 *
 */


#ifdef MESSAGE_BUS_UBUS

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "wanconf.h"
#include "libubus.h"
#include "bdk.h"
/* Some definitions in bdk_dbus.h are actually common to dbus and ubus. */
#include "bdk_dbus.h"

#define WANCONF_TIMEOUT_METHOD_CALL     (3000)

static struct ubus_context *wanconf_context = NULL;
static struct blob_buf wanconf_bb;
static uint32_t wanconf_objId;

/* Note this should be provided by zbus api.
   Since zbus is private, let's define the function here
   until a better solution
*/
static char* get_dest_bus_name(const char *destCompName)
{
    if (!strncmp(destCompName, BDK_COMP_GPON, strlen(BDK_COMP_GPON)))
        return GPON_MD_BUS_NAME;
    else if (!strncmp(destCompName, BDK_COMP_EPON, strlen(BDK_COMP_EPON)))
        return EPON_MD_BUS_NAME;
    else if (!strcmp(destCompName, BDK_APP_SYSMGMT_NB))
        return SYSMGMT_NB_BUS_SVR_BUS_NAME;
    else if (!strcmp(destCompName, BDK_COMP_SYSMGMT))  // Note: wanconf now sends to sysmgmt_nb, so this entry is unused
        return SYSMGMT_MD_BUS_NAME;
    else
    {
        bcmuLog_error("Unknown destCompName %s", destCompName);
        return NULL;
    }
}
void bus_out_wanConf(const char *destCompName, const char *cmd, const char *arg)
{
    int ret = 0;

    if ((destCompName == NULL) || (cmd == NULL))
    {
        bcmuLog_error("one or more NULL input args");
        return;
    }

    uloop_init();

    /* Connect to ubusd and get context. */
    wanconf_context = ubus_connect(NULL);
    if (wanconf_context == NULL)
    {
        bcmuLog_error("Failed to connect to ubusd.");
        return;
    }

    ubus_add_uloop(wanconf_context);

    if ((ret = ubus_lookup_id(wanconf_context, get_dest_bus_name(destCompName), &wanconf_objId)) == 0)
    {
        blob_buf_init(&wanconf_bb, 0);
        blobmsg_add_string(&wanconf_bb, "cmd", cmd);
        blobmsg_add_string(&wanconf_bb, "arg", arg);

        ret = ubus_invoke(wanconf_context, wanconf_objId, "wanConf",
          wanconf_bb.head, NULL, NULL, WANCONF_TIMEOUT_METHOD_CALL);
        if (ret == UBUS_STATUS_OK)
        {
            bcmuLog_notice("wanConf invoked");
        }
        else
        {
            bcmuLog_error("wanConf invoke failed ret=%d:%s", ret, ubus_strerror(ret));
        }

        blob_buf_free(&wanconf_bb);
    }
    else
    {
        bcmuLog_error("ubus_lookup_id() failed, ret=%d:%s", ret, ubus_strerror(ret));
    }

    ubus_free(wanconf_context);
    uloop_done();
}

#endif // MESSAGE_BUS_UBUS

