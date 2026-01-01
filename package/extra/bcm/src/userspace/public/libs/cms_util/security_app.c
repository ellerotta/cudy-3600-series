/***********************************************************************
 *
 *  Copyright (c) 2023  Broadcom
 *  All Rights Reserved
 *
 * <:label-BRCM:2023:DUAL/GPL:standard
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

/*****************************************************************************
*    Description:
*
*      Security Apps definitions.
*
*****************************************************************************/

#ifdef SUPPORT_NETAPP_RESTRICT
#include "cms_util.h"
#include "security_app.h"

#define LIBCAP_PATH            "/sbin/capsh"
#define LIBCAP_APP             "capsh"
#define LIBCAP_CAPS_ARG        "--caps="
#define LIBCAP_USER_ARG        "--user=netuser"
#define LIBCAP_KEEP_ARG        "--keep=1"
#define LIBCAP_ADDAMB_ARG      "--addamb="
#define LIBCAP_SHELL           "--"
#define LIBCAP_SHELL_CMD       "-c"

#define CAPSET_FLAGS "cap_kill,cap_net_bind_service,cap_net_broadcast,cap_net_admin,"\
        "cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_rawio,cap_sys_ptrace,cap_sys_pacct,"\
        "cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,"\
        "cap_sys_tty_config,cap_mknod,cap_lease,cap_mac_override,cap_mac_admin+eip "\
        "cap_setpcap,cap_setuid,cap_setgid+ep"
#define AMB_CAPSET_FLAGS "cap_kill,cap_net_bind_service,cap_net_broadcast,cap_net_admin,"\
        "cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_rawio,cap_sys_ptrace,cap_sys_pacct,"\
        "cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,"\
        "cap_sys_tty_config,cap_mknod,cap_lease,cap_mac_override,cap_mac_admin"

typedef enum{
    CMD_CHOWN_FILE_SMD_MESSAGE = 0,
    CMD_CHOWN_DIR_VAR = 1
} CMD_CHOWN_INDEX;

#define NUM_APPS (sizeof(g_sec_app_info)/sizeof(SecAppInfo))
#define MAX_CMP_SIZE 64
#define REQUIRE_FILE_SMD_MESSAGE    (0x1UL << CMD_CHOWN_FILE_SMD_MESSAGE)
#define REQUIRE_DIR_VAR             (0x1UL << CMD_CHOWN_DIR_VAR)

typedef struct 
{
   const char *exe;               /* name of app */           
   const char *userId;            /* user ID */
   const char *flags;             /* Capabilities  */   
   const char *ambFlags;          /* Ambient capabilities */
   const UINT32 requireFlags;
} SecAppInfo;

typedef struct
{
    int index;
    const char * command;
    int used;
}CmdUsageInfo;


CmdUsageInfo g_cmd_usage [] = {
    {CMD_CHOWN_FILE_SMD_MESSAGE, "chown netuser /var/smd_messaging_server_addr", 0}, /* To communication with smd */
    {CMD_CHOWN_DIR_VAR, "chown netuser /var", 0}, /* To have the privilege to  create dhcp6c_duid_veip0.x  */
};

static inline void SecAppExecCommand(int index)
{
    int i;
    
    for (i = 0; i < (sizeof(g_cmd_usage)/sizeof(CmdUsageInfo)); i++)
    {
        if (i == index)
        {
            if (0 == g_cmd_usage[i].used)
            {
                system(g_cmd_usage[i].command);
                g_cmd_usage[i].used = 1;
            }
        }
    }
}

static void SecAppPreInit(SecAppInfo * app)
{
    int i;
    
    for (i = 0; i < 32; i++)
    {
        if ((app->requireFlags & (0x1UL << i)) != 0)
            SecAppExecCommand(i);
    }
}

SecAppInfo g_sec_app_info[] = {
    {"//usr/sbin/dnsmasq", "netuser", CAPSET_FLAGS, AMB_CAPSET_FLAGS, REQUIRE_FILE_SMD_MESSAGE},
    {"//bin/dhcp6c", "netuser", CAPSET_FLAGS, AMB_CAPSET_FLAGS, REQUIRE_FILE_SMD_MESSAGE | REQUIRE_DIR_VAR},
};

/* strip the start slashes of the  app start command */
static const char * SecAppStripSlashHead(const char * str)
{
    int i;

    for (i = 0; i < strlen(str) && *(str + i) != '\0'; i++)
    {
        if (*(str + i) == '/')
            continue;

        return (str + i);
    }

    /* return NULL when all the characters in the string are '/', it is not a common case */
    return NULL;
}

int SecAppLoadExe(const char *path, const char *appargs)
{
    int i;
    char bufTmp1[BUFLEN_1024] = {0};
    char bufTmp2[BUFLEN_1024] = {0};
    char bufTmp3[BUFLEN_256] = {0};

    if ((strlen(path) + strlen(appargs) + 1) >= BUFLEN_256 - 1)
    {
        cmsLog_error("Buffer to store path and appargs are too small");
        return 0;
    }
    else
    {
        snprintf(bufTmp3, BUFLEN_256 - 1, "%s %s", path, appargs); 
    }

    for (i = 0; i < NUM_APPS; i++)
    {  
        /* scan if the app to run is in the sec app list */
        if ((strncmp(SecAppStripSlashHead(path), SecAppStripSlashHead(g_sec_app_info[i].exe), MAX_CMP_SIZE) == 0))
        {
            snprintf(bufTmp1, BUFLEN_1024 - 1, "%s%s", LIBCAP_CAPS_ARG, g_sec_app_info[i].flags);
            snprintf(bufTmp2, BUFLEN_1024 - 1, "%s%s", LIBCAP_ADDAMB_ARG, g_sec_app_info[i].ambFlags);
            
            char *argvtmp[] = {LIBCAP_APP, bufTmp1, LIBCAP_USER_ARG, LIBCAP_KEEP_ARG, bufTmp2, LIBCAP_SHELL, LIBCAP_SHELL_CMD, bufTmp3, NULL};   

            SecAppPreInit(&g_sec_app_info[i]);
            execv(LIBCAP_PATH, argvtmp);
            return 1;
        }
    }

    cmsLog_debug("%s is not a security application. Run by root", path);
    return 0;
}
#endif /* SUPPORT_NETAPP_RESTRICT */
