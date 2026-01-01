/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/if.h>
#include "bcmnet.h"
#include "bcm/bcmswapitypes.h"
#include "pwrctl_api.h"
#ifdef BRCM_WLAN
#include "bcm_wlan_defs.h"
#endif /* BRCM_WLAN */
#include "bcm/flexrmdefs.h"
#ifdef BRCM_VOICE_SUPPORT
#include "cms_msg.h"
#include "cms_eid.h"
#endif
#include "bdmf_chrdev.h"
#include <stdbool.h>
#include <limits.h>


#define SCS_UTIL "/bin/bs"

#define IS_VALID_STRTOL(val, str, endptr) !((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || ((endptr) == (str)))
#define __SAFE_STR(str, rep) (NULL != (str)) ? (str) : (rep)
#define SAFE_STR(str) __SAFE_STR(str, "N/A")

#define APPEND_STR(str, offset, max_size, fmt, arg ...) snprintf((str) + offset, max_size - offset, fmt, ##arg)
#define APPEND_CHR_ARR(arr, offset, fmt, arg ...) APPEND_STR(arr, offset, sizeof(arr), fmt, ##arg)

#define system_run(cmd) ___system_check(__FUNCTION__, system(__SAFE_STR(cmd, "")), SAFE_STR(cmd))
#define is_resource_resident(resource) ___is_resource_resident(__FUNCTION__, SAFE_STR(resource))

#define MUTE_OUTPUT "> /dev/null"

static pwr_drv_t *pwr_driver_get(pwr_type_t type);
static int pwr_status_show_one(pwr_drv_t *pwr_drv, char *param);

static int ___system_check(const char *caller, int status, const char *sys_cmd);
static bool ___is_resource_resident(const char * caller, const char * resource);


static inline bool read_strtol(long * val, const char * line) {

    bool rc  = false;

    if ((NULL != val) && (NULL != line)) {    
        char * pend;

        *val  = strtol(line, &pend, 0);
        rc = IS_VALID_STRTOL(*val, line, pend);
    }

    return rc;
}

static int file_exist(char *filename)
{
    struct stat buffer;   
    return (stat(filename, &buffer) == 0);
}

static int pwr_api_file_read_line(char *filename, char *line, int line_num)
{
    int ret = 0;
    int i;
    FILE *file;

    if (!file_exist(filename))
        return -1;

    file = fopen(filename, "r");
    if (file == NULL)
        return -1;

    for (i = 0; i < line_num; i++)
    {
        if (fgets(line, 256, file) == NULL)
            ret = -1;
    }

    fclose(file);
    return ret;
}

static int pwr_api_file_write_line(char *filename, char *line)
{
    char command[256];

    if (!file_exist(filename))
        return -1;

    snprintf(command, sizeof(command), "echo %s > %s 2>/dev/null", line, filename);
    return system(command);
}

static int pwr_api_file_read_int(char *filename, int *num)
{
    char line[256];

    if (pwr_api_file_read_line(filename, line, 1))
        return -1;

    *num = atoi(line);

    return 0;
}

static int pwr_api_file_write_int(char *filename, int num)
{
    char line[256];

    snprintf(line, sizeof(line), "%d", num);
    return pwr_api_file_write_line(filename, line);
}

static int sock_init(char *ifname, int *skfd, struct ifreq *ifr)
{
    int ret = -1;

    if ((*skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        goto Exit;

    if (ifname)
    {
        strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name)-1);
        ifr->ifr_name[sizeof(ifr->ifr_name)-1] = '\0';
    }
    else
        strcpy(ifr->ifr_name, "bcmsw");

    if (ioctl(*skfd, SIOCGIFINDEX, ifr) < 0)
        goto Exit;

    ret = 0;

Exit:

    return ret;
}
/* WIFI Suspend */
#define WIFI_SCRIPT                 "/etc/init.d/wifi.sh"
#define TMP_WIFI_MODULES            "/tmp/wifi_modules"
int pwr_api_wifi_suspend_get(__attribute__ ((unused)) char *param, int *enable)
{
    char command[256];
    char line[256];

    if (!file_exist(WIFI_SCRIPT))
        return -1;

    snprintf(command, sizeof(command), "%s %s > %s", WIFI_SCRIPT, "modules", TMP_WIFI_MODULES);
    if (system(command) == -1)
    {
        printf("Error: failed to execute command=[%s]\n", command);
    }

    *enable = pwr_api_file_read_line(TMP_WIFI_MODULES, line, 1) ? 1 : 0;
    return 0;
}

int pwr_api_wifi_suspend_set( __attribute__ ((unused)) char *param, int enable)
{
    int en;
    char command[256];

    if (pwr_api_wifi_suspend_get(NULL, &en))
        return -1;

    if (enable == en)
        return -1;

    snprintf(command, sizeof(command), "%s %s " MUTE_OUTPUT, WIFI_SCRIPT, enable ? "suspend" : "resume");
    return system(command);
}

/* WLDPD Power Down */
#if defined(WL_MAX_NUM_RADIO)
#define WLDPD_MAXIF                 WL_MAX_NUM_RADIO
#else   
/* For non-wlan builds */   
#define WLDPD_MAXIF                 4
#endif /* */    
#define WLDPD_SCRIPT                "/etc/init.d/wifi.sh"
#define TMP_WLDPD_PWRSTS            "/tmp/wlan_dpdsts"
#define WLDPD_RC_FEAT_DISABLED      (0)
#define WLDPD_RC_NVRM_MISSING       (-1)
#define WLDPD_RC_SCRPT_MISSING      (-2)
#define WLDPD_RC_FEAT_MISSING       (-3)

static const char wldpd_err_str[][32] = {
     "feature disabled",
     "nvram missing",
     "script missing",
     "feature missing"
};

/*
 * Check if WLAN deep power down feature is enabled
 *
 * en:  enable feature control (unused)
 *
 * return
 *   1: enabled, number of interfaces
 *   0: feature control disabled
 *  -1: feature control missing
 *  -2: script missing
 *  -3: feature missing
 */
static int pwr_api_wldpd_check(__attribute__ ((unused)) int en)
{

#if !defined(BUILD_BCM_WLAN_DPDCTL)
    return WLDPD_RC_FEAT_MISSING;
#else
    int  ret;
    char command[256];
    char line[256];
    /* Check if script is available */
    if (!file_exist(WLDPD_SCRIPT))
        return WLDPD_RC_SCRPT_MISSING;

    snprintf(command, sizeof(command), "nvram kget %s > %s", "wl_dpdctl_enable", TMP_WLDPD_PWRSTS);
    if (system(command) == -1)
    {
        printf("Error: failed to execute command=[%s]\n", command);
    }

    /* Return
        -1: nvram doesn't exists 
         0: nvram exists & disabled
         1: nvram exists & enabled
    */
    if ((ret = pwr_api_file_read_line(TMP_WLDPD_PWRSTS, line, 1)) == 0) {
        ret = atoi(line);
    }

    return ret;
#endif /* !BUILD_BCM_WLAN_DPDCTL */
}

/*
 * Get power down status of given wlan interface
 */
static int pwr_api_wldpd_get_one(char *ifname, int *enable)
{
    char command[256];
    char line[256];
    int  ret;


    snprintf(command, sizeof(command), "%s %s | grep %s | cut -d '|' -f3 > %s",
        WLDPD_SCRIPT, "dpdsts", ifname, TMP_WLDPD_PWRSTS);
    if (system(command) == -1)
    {
        printf("Error: failed to execute command=[%s]\n", command);
    }

    if ((ret = pwr_api_file_read_line(TMP_WLDPD_PWRSTS, line, 1)) == 0)
        *enable = atoi(line);

    return ret;
}

/*
 * Set power down (enable/disable) of given wlan interface
 */
static int pwr_api_wldpd_set_one(char *ifname, int enable)
{
    char command[256];
    char iflist[32] = "\"";
    char ifn[4];
    int i, en, change = 0;
    int ret = 0;

    if (!file_exist(WLDPD_SCRIPT))
        return -1;

    /* Get the current power down status of all ports */
    for (i=0; i < WLDPD_MAXIF; i++)
    {
        snprintf(ifn, sizeof(ifn), "wl%d", i);
        if ((ret = pwr_api_wldpd_get_one(ifn, &en)) < 0)
            break;
        if (strncmp(ifname, ifn, strlen(ifname)) == 0) {
            if (en == enable)    /* No change */
                break;
            else {
                strncat(iflist, ifn, sizeof(iflist) - strlen(iflist) - 1);
                strncat(iflist, " ", sizeof(iflist) - strlen(iflist) - 1);
                change = 1;
            }
        }

        /* build the pwrup or pwrdn interface list */
        if (en == enable) {
            strncat(iflist, ifn, sizeof(iflist) - strlen(iflist) - 1);
            strncat(iflist, " ", sizeof(iflist) - strlen(iflist) - 1);
        }
    }
    strncat(iflist, "\"", sizeof(iflist) - strlen(iflist) - 1);

    if (change) {
        snprintf(command, sizeof(command), "%s %s %s > %s",
            WLDPD_SCRIPT, (enable) ? "dpddn" : "dpdup", iflist, "/dev/null");
        if (system(command) == -1)
        {
            printf("Error: failed to execute command=[%s]\n", command);
        }
        ret = 0;
    }

    return ret;
}

/*
 * Get power down status of all wlan interfaces
 */
static int pwr_api_wldpd_get_global(int *enable)
{
    int i, en;
    char ifname[5];
    int  ret;

    *enable = 1;

    for (i = 0; i < WLDPD_MAXIF; i++)
    {
        snprintf(ifname, sizeof(ifname), "wl%d", i);
        if ((ret = pwr_api_wldpd_get_one(ifname, &en)) < 0)
            break;

        if (!en)
            *enable = 0;
    }

    return 0;
}

/*
 * Set power down (enable/disable) of all wlan interfaces
 */
static int pwr_api_wldpd_set_global(int enable)
{
    char command[256];
    char ifn[4];
    int i, en, ret;
    int change = 0;

    /* Get the current power down status of all ports */
    for (i=0; i < WLDPD_MAXIF; i++)
    {
        snprintf(ifn, sizeof(ifn), "wl%d", i);
        if ((ret = pwr_api_wldpd_get_one(ifn, &en)) < 0)
            break;

        if (en != enable)    /* No change */
            change = 1;
    }

    if (change) {
        snprintf(command, sizeof(command), "%s %s %s > %s",
            WLDPD_SCRIPT, (enable) ? "dpdup" : "dpddn", "\"\"", "/dev/null");
        if (system(command) == -1)
        {
            printf("Error: failed to execute command=[%s]\n", command);
        }
        ret = 0;
    } else if (i) {
        /* Some interfaces are present */
        ret = 0;
    }

    return ret;
}

/*
 * Get power down status of wlan interface(s)
 */
int pwr_api_wldpd_get(char *param, int *enable)
{
    if (pwr_api_wldpd_check(0) < 0)
        return -1;

    if (param)
        return pwr_api_wldpd_get_one(param, enable);
    else
        return pwr_api_wldpd_get_global(enable);
}

/*
 * Enable/disable power down of wlan interface(s)
 */
int pwr_api_wldpd_set(char *param, int enable)
{
    int rc;

    rc = pwr_api_wldpd_check(1);
    if (rc > 0) {
        if (param)
            rc = pwr_api_wldpd_set_one(param, enable);
        else
            rc = pwr_api_wldpd_set_global(enable);
    } else {
        printf("wldpd set FAILED [%s]\n", wldpd_err_str[-rc]);
        rc = -1;
    }

    return rc;
}

/*
 * Show status of all wlan interfaces power down status
 */
int pwr_api_wldpd_show(__attribute__ ((unused)) char *param)
{
    int i;
    char ifn[4];

    for (i=0; i < WLDPD_MAXIF; i++)
    {
        snprintf(ifn, sizeof(ifn), "wl%d", i);
        pwr_status_show_one(pwr_driver_get(PWR_TYPE_WLDPD), ifn);
    }

    return 0;
}


/* Disk Suspend: USB, SATA etc. */
#define DISK_SCRIPT                 "/etc/init.d/disk.sh"
#define TMP_DISK_MODULES            "/tmp/disk_modules"
int pwr_api_disk_suspend_get(__attribute__ ((unused)) char *param, int *enable)
{
    char command[256];
    char line[256];

    if (!file_exist(DISK_SCRIPT))
        return -1;

    snprintf(command, sizeof(command), "%s %s > %s", DISK_SCRIPT, "modules", TMP_DISK_MODULES);
    if (system(command) == -1)
    {
        printf("Error: failed to execute command=[%s]\n", command);
    }

    *enable = pwr_api_file_read_line(TMP_DISK_MODULES, line, 1) ? 1 : 0;
    return 0;
}

int pwr_api_disk_suspend_set(__attribute__ ((unused)) char *param, int enable)
{
    int en;
    char command[256];

    if (pwr_api_disk_suspend_get(NULL, &en))
        return -1;

    if (enable == en)
        return -1;

    snprintf(command, sizeof(command), "%s %s " MUTE_OUTPUT, DISK_SCRIPT, enable ? "suspend" : "resume");
    return system(command);
}

/* PCI ASPM: Active State Power Management */
#define MODULE_PCIE_ASPM            "/sys/module/pcie_aspm/parameters/policy"
int pwr_api_pcie_aspm_get(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int *enable)
{
#ifdef WL_IDLE_PWRSAVE
    char policy[256];
    char *p1, *p2;

    if (pwr_api_file_read_line(MODULE_PCIE_ASPM, policy, 1))
        return -1;

    p1 = strstr(policy, "[");
    p2 = strstr(policy, "]");

    if (!p1 || !p2)
        return -1;

    p1++;
    *p2 = 0;
    *enable = strstr(p1, "l1_powersave") ? 1 : 0;

    return 0;
#else
    return -1;
#endif
}

int pwr_api_pcie_aspm_set(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int enable)
{
#ifdef WL_IDLE_PWRSAVE
    return pwr_api_file_write_line(MODULE_PCIE_ASPM, enable ? "l1_powersave" : "default");
#else
    printf("PCI ASPM control is disabled in this image\n");
    return 0;
#endif
}

/* UBUS DCM */
#define MODULE_UBUS_DCM             "/sys/module/ubus4_dcm/parameters/enable"
int pwr_api_ubus_dcm_get(__attribute__ ((unused)) char *param, int *enable)
{
    return pwr_api_file_read_int(MODULE_UBUS_DCM, enable);
}

int pwr_api_ubus_dcm_set(__attribute__ ((unused)) char *param, int enable)
{
    return pwr_api_file_write_int(MODULE_UBUS_DCM, enable);
}

/* XRDP Clock Gating */
#define TMP_CLOCK_GATE              "/tmp/clock_gate"
int pwr_api_xrdp_clock_gate_get(__attribute__ ((unused)) char *param, int *enable)
{
    char line[256];
    char *p;

    if (system(SCS_UTIL " /b/e system clock_gate > "TMP_CLOCK_GATE" 2>&1"))
        return -1;

    if (pwr_api_file_read_line(TMP_CLOCK_GATE, line, 3))
        return -1;

    p = strstr(line, ":");
    if (!p)
        return -1;

    *enable = !strncmp(p + 2, "yes", 3) ? 1 : 0;

    return 0;
}

int pwr_api_xrdp_clock_gate_set(__attribute__ ((unused)) char *param, int enable)
{
    char command[256];

    if (false == is_resource_resident(SCS_UTIL)) {
        return -1;
    }
    snprintf(command, sizeof(command), SCS_UTIL " /b/c system clock_gate=%s", enable ? "yes" : "no");
    return system(command);
}

int pwr_api_xrdp_clock_divider_get(__attribute__ ((unused)) char *param, int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));

    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWXRDPDIV; 
    ethswctl.type = TYPE_GET;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    *enable = ethswctl.val;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_xrdp_clock_divider_set(__attribute__ ((unused)) char *param, int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));

    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWXRDPDIV; 
    ethswctl.type = TYPE_SET;
    ethswctl.val = enable;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

/* Network device down */
static int pwr_api_net_down_get_one(char *ifname, int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    if ((ret = ioctl(skfd, SIOCGIFFLAGS, &ifr)))
        goto Exit;

    *enable = ifr.ifr_flags & IFF_UP ? 0 : 1;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_net_down_set_one(char *ifname, int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    if ((ret = ioctl(skfd, SIOCGIFFLAGS, &ifr)))
        goto Exit;

    if (enable)
        ifr.ifr_flags &= ~IFF_UP;
    else
        ifr.ifr_flags |= IFF_UP;

    if ((ret = ioctl(skfd, SIOCSIFFLAGS, &ifr)))
        goto Exit;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_net_down_get_global(int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWLANPWR; 
    ethswctl.type = TYPE_GET;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    *enable = ethswctl.val == 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_net_down_set_global(int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWLANPWR; 
    ethswctl.type = TYPE_SET;
    ethswctl.val = enable ? 0 : 1;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_net_down_get(char *param, int *enable)
{
    if (param)
        return pwr_api_net_down_get_one(param, enable);
    else
        return pwr_api_net_down_get_global(enable);
}

int pwr_api_net_down_set(char *param, int enable)
{
    if (param)
        return pwr_api_net_down_set_one(param, enable);
    else
        return pwr_api_net_down_set_global(enable);
}

int pwr_api_net_down_show(__attribute__ ((unused)) char *param)
{
    int i;
    char ifname[6];

    for (i = 0; i < 16; i++)
    {
        snprintf(ifname, sizeof(ifname), "eth%d", i);
        pwr_status_show_one(pwr_driver_get(PWR_TYPE_NET), ifname);
    }

    return 0;
}

/* PHY Power Down */
static __attribute__ ((unused)) int pwr_api_phy_down_get_one(char *ifname, int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = ETHGETPHYPWR; 
    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);
    *enable = ethctl.ret_val ? 0 : 1;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static __attribute__ ((unused)) int pwr_api_phy_down_set_one(char *ifname, int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = enable ? ETHSETPHYPWROFF : ETHSETPHYPWRON; 
    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_eee_show(__attribute__ ((unused)) char *param)
{
    int i;
    char ifname[6];

    for (i = 0; i < 16; i++)
    {
        snprintf(ifname, sizeof(ifname), "eth%d", i);
        pwr_status_show_one(pwr_driver_get(PWR_TYPE_EEE), ifname);
    }

    return 0;
}

int pwr_api_apd_show(__attribute__ ((unused)) char *param)
{
    int i;
    char ifname[6];

    for (i = 0; i < 16; i++)
    {
        snprintf(ifname, sizeof(ifname), "eth%d", i);
        pwr_status_show_one(pwr_driver_get(PWR_TYPE_APD), ifname);
    }

    return 0;
}

/* PHY EEE: Energy Efficient Ethernet */
static int pwr_api_phy_eee_get_one(char *ifname, int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));

    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = ETHGETPHYEEE; 
    ethctl.sub_port = -1;
    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);
    *enable = ethctl.ret_val ? 1 : 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_phy_eee_set_one(char *ifname, int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = enable ? ETHSETPHYEEEON : ETHSETPHYEEEOFF; 
    ethctl.sub_port = -1;
    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_phy_eee_get_global(int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWPHYEEE; 
    ethswctl.type = TYPE_GET;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    *enable = ethswctl.val ? 1 : 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_phy_eee_set_global(int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWPHYEEE; 
    ethswctl.type = TYPE_SET;
    ethswctl.val = enable ? 1 : 0;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_phy_eee_get(char *param, int *enable)
{
    if (param)
        return pwr_api_phy_eee_get_one(param, enable);
    else
        return pwr_api_phy_eee_get_global(enable);
}

int pwr_api_phy_eee_set(char *param, int enable)
{
    if (param)
        return pwr_api_phy_eee_set_one(param, enable);
    else
        return pwr_api_phy_eee_set_global(enable);
}

/* PHY APD: Auto Power Down */
static int pwr_api_phy_apd_get_one(char *ifname, int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = ETHGETPHYAPD; 
    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);
    *enable = ethctl.ret_val ? 1 : 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_phy_apd_set_one(char *ifname, int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));
    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = enable ? ETHSETPHYAPDON : ETHSETPHYAPDOFF; 
    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_phy_apd_get_global(int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWPHYAPD; 
    ethswctl.type = TYPE_GET;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    *enable = ethswctl.val ? 1 : 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

static int pwr_api_phy_apd_set_global(int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWPHYAPD; 
    ethswctl.type = TYPE_SET;
    ethswctl.val = enable ? 1 : 0;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_phy_apd_get(char *param, int *enable)
{
    if (param)
        return pwr_api_phy_apd_get_one(param, enable);
    else
        return pwr_api_phy_apd_get_global(enable);
}

int pwr_api_phy_apd_set(char *param, int enable)
{
    if (param)
        return pwr_api_phy_apd_set_one(param, enable);
    else
        return pwr_api_phy_apd_set_global(enable);
}

/* SF2 DGM: Deep Green Mode */
int pwr_api_sf2_dgm_get(__attribute__ ((unused)) char *param, int *enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWDEEPGREENMODE; 
    ethswctl.type = TYPE_GET;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    *enable = ethswctl.val ? 1 : 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_sf2_dgm_set(__attribute__ ((unused)) char *param, int enable)
{
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWDEEPGREENMODE; 
    ethswctl.type = TYPE_SET;
    ethswctl.val = enable ? 1 : 0;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

/* CPU Wait */
#define SYS_CPU_BCM_CPUWAIT         "/sys/devices/system/cpu/bcm_cpuwait"
int pwr_api_cpu_wait_get(__attribute__ ((unused)) char *param, int *enable)
{
    return pwr_api_file_read_int(SYS_CPU_BCM_CPUWAIT, enable);
}

int pwr_api_cpu_wait_set(__attribute__ ((unused)) char *param, int enable)
{
    return pwr_api_file_write_int(SYS_CPU_BCM_CPUWAIT, !!enable);
}

#define SYS_CPUFREQ_SCALING_        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_"
#if defined(BCM_PON)
int pwr_api_cpu_speed_get(__attribute__ ((unused)) char *param, int *enable)
{
    char gov[256];
    int cur, max;

    *enable = 0;

    if (!pwr_api_file_read_line(SYS_CPUFREQ_SCALING_"governor", gov, 1) &&
        !strncmp(gov, "userspace", 9) &&
        !pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"cur_freq", &cur) &&
        !pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"max_freq", &max))
    {
        *enable = cur != max;
    }

    return 0;
}

int pwr_api_cpu_speed_set(char *param, int enable)
{
    int freq = param ? atoi(param) : 0;
    int max, min;

    if (!pwr_api_file_write_line(SYS_CPUFREQ_SCALING_"governor", "userspace") && 
        !pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"min_freq", &min) &&
        !pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"max_freq", &max) &&
        !pwr_api_file_write_int(SYS_CPUFREQ_SCALING_"setspeed", enable ? (freq ? : min) : max))
    {
        return 0;
    }

    return -1;
}
#else
/* CPU Speed */
#define SYS_CPU_BCM_ARM_CPUIDLE     "/sys/devices/system/cpu/bcm_arm_cpuidle/"
static int pwr_api_bcm_arm_cpuidle_attr_get(const char *name, int *value)
{
    char fn[256];
    int ret;

    ret = snprintf(fn, sizeof(fn), SYS_CPU_BCM_ARM_CPUIDLE"%s", name);
    if (ret < 0 || ret >= (int) sizeof(fn))
        return -1;
    return pwr_api_file_read_int(fn, value);
}

static int pwr_api_bcm_arm_cpuidle_attr_set(const char *name, int value)
{
    char fn[256];
    int ret;

    ret = snprintf(fn, sizeof(fn), SYS_CPU_BCM_ARM_CPUIDLE"%s", name);
    if (ret < 0 || ret >= (int) sizeof(fn))
        return -1;
    return pwr_api_file_write_int(fn, value);
}

int pwr_api_cpu_speed_get(__attribute__ ((unused)) char *param, int *enable)
{
    // Special value "256" just means "1, but with auto enabled".
    // Note: there are no special "but with auto enabled" values for 2, 4 and
    // 8.  So if auto is enabled for them, we must return the values as if the
    // auto is disabled.
    unsigned int cpuspeed = 0;
    char gov[256];
    int cur, max;


    if (!pwr_api_file_read_line(SYS_CPUFREQ_SCALING_"governor", gov, 1)) {
        if (strcmp(gov, "userspace\n"))
            cpuspeed = 256;
        else if (!pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"cur_freq",
                                &cur) &&
                 !pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"max_freq",
                                &max))
            cpuspeed = cur ? (max + cur - 1) / cur : 256;
    } else {
        int clk_divider, auto_clock_divide_enabled;

	if (pwr_api_bcm_arm_cpuidle_attr_get("admin_max_freq", &cur))
            return -1;
        if (pwr_api_bcm_arm_cpuidle_attr_get("valid_freq_list", &max))
            return -1;
        if (pwr_api_bcm_arm_cpuidle_attr_get("enable_auto_clkdiv",
                                &auto_clock_divide_enabled))
            return -1;

        clk_divider = cur ? (max + cur - 1) / cur : 1;
        if (clk_divider == 1)
	    cpuspeed = auto_clock_divide_enabled ? 256 : 1;
        else
            cpuspeed = clk_divider;
    }

    *enable = (cpuspeed==256) ? 1 : 0;
    return 0;
}

int pwr_api_cpu_speed_set(char *param, int enable)
{
    unsigned int speed = enable ? 256 : 1;
    unsigned int cpuspeed = param ? (unsigned int) atoi(param) : speed;
    int max;

    if (cpuspeed == 256) {
        if (!pwr_api_file_write_line(SYS_CPUFREQ_SCALING_"governor",
                                "schedutil") ||
            !pwr_api_file_write_line(SYS_CPUFREQ_SCALING_"governor",
                                "interactive"))
            return 0;
    } else {
        if (!pwr_api_file_write_line(SYS_CPUFREQ_SCALING_"governor",
                                "userspace") && 
            !pwr_api_file_read_int(SYS_CPUFREQ_SCALING_"max_freq",
                                &max) &&
            !pwr_api_file_write_int(SYS_CPUFREQ_SCALING_"setspeed",
                                max / (cpuspeed ? : 1)))
            return 0;
    }

    if (pwr_api_bcm_arm_cpuidle_attr_get("valid_freq_list", &max))
        return -1;
    switch (cpuspeed) {
    case 0:
        cpuspeed = 1;
        // fall through
    case 1:
        // fall through
    case 2:
        // fall through
    case 4:
        // fall through
    case 8:
        if (pwr_api_bcm_arm_cpuidle_attr_set("admin_max_freq",
                                (max+cpuspeed-1)/cpuspeed))
            return -1;
        if (pwr_api_bcm_arm_cpuidle_attr_set("enable_auto_clkdiv", 0))
            return -1;
        break;
    case 256:
        if (pwr_api_bcm_arm_cpuidle_attr_set("admin_max_freq", max))
            return -1;
        if (pwr_api_bcm_arm_cpuidle_attr_set("enable_auto_clkdiv", 1))
            return -1;
        break;
    default:
        return -1;
    }

    return 0;
}
#endif

/* DRAM SR: Self Refresh */
#define DRAM_SR_FN                  "/sys/module/bcm_memc/parameters/enable_self_refresh"
int pwr_api_dram_sr_get(__attribute__ ((unused)) char *param, int *enable)
{
    return pwr_api_file_read_int(DRAM_SR_FN, enable);
}

int pwr_api_dram_sr_set(__attribute__ ((unused)) char *param, int enable)
{
    return pwr_api_file_write_int(DRAM_SR_FN, !!enable);
}

/* AVS: Adaptive Voltage Scaling */
#define AVS_STATUS_FN               "/sys/power/bpcm/avs/status"
int pwr_api_avs_get(__attribute__ ((unused)) char *param, int *enable)
{
    return pwr_api_file_read_int(AVS_STATUS_FN, enable);
}

/* PMD: Control the reset state of the PMD chip */
#define PROC_PMD_RESET              "/proc/pmd/reset"
int pwr_api_pmd_get(__attribute__ ((unused)) char *param, int *enable)
{
    return pwr_api_file_read_int(PROC_PMD_RESET, enable);
}

int pwr_api_pmd_set(__attribute__ ((unused)) char *param, int enable)
{
    return pwr_api_file_write_int(PROC_PMD_RESET, enable);
}

/* PHY speed limit */
int pwr_api_phy_speed_get(__attribute__ ((unused)) char *param, int *enable)
{
    unsigned speed;
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWPHYSPEED; 
    ethswctl.type = TYPE_GET;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    speed = ethswctl.val;
    *enable = speed > 0;

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}

int pwr_api_phy_speed_set(char *param, int enable)
{
    unsigned speed = enable ? (param ? atoi(param) : 100) : 0;
    int ret = -1;
    int skfd;
    struct ifreq ifr;
    struct ethswctl_data ethswctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethswctl, 0, sizeof(struct ethswctl_data));
    if (sock_init(NULL, &skfd, &ifr) < 0)
        goto Exit;

    ethswctl.op = ETHSWPHYSPEED; 
    ethswctl.type = TYPE_SET;
    ethswctl.val = speed;
    ifr.ifr_data = (void *)&ethswctl;

    ret = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}
/* SPU Suspend / Resume */
#define SPU_SCRIPT                  "/etc/init.d/spu_pwr.sh"
/**
 * @brief Set 'SPU Off' (HW accelerator for ipsec de/encryption) feature on/off
 * @param enable 0 - set the feature off, set on otherwise
 * @return int 0 if the feature is supported, -1 otherwise
 */
int pwr_api_spu_set(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int enable)
{
    int rc = -1;
#ifdef SPU_CTL_SUPPORT
    char cmd[256] = {SPU_SCRIPT};

    if (true == is_resource_resident(cmd)) {
        int is_spu_on = file_exist(FLEX_CHAR_DEV_PATH);

        if ((enable && is_spu_on) || (!enable && !is_spu_on)) {
            unsigned cmd_len = strlen(cmd);

            snprintf(&cmd[cmd_len], sizeof(cmd) - cmd_len, " %s",
                     (0 != enable) ? "suspend" : "resume");
            rc = system_run(cmd);
        }
    }
#endif /*SPU_CTL_SUPPORT*/
    return rc;
}
/**
 * @brief Read 'SPU Off' (HW accelerator for ipsec de/encryption) feature suppport status
 * @param param N/A
 * @param enable 0 (false) if the feature is disabled, 1 (true) otherwise
 * @return int 0 if the feature is supported, -1 otherwise
 */
int pwr_api_spu_get(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int * enable)
{
    int rc = -1;
#ifdef SPU_CTL_SUPPORT
    *enable = !file_exist(FLEX_CHAR_DEV_PATH);
    rc = 0;
#endif /*SPU_CTL_SUPPORT*/

    return rc;
}

#define SPU_STATIC_PWM_ENABLE "/sys/devices/platform/*.spu_crypto/static_pwr"
#define SPU_STATIC_PWM_STATUS "/tmp/ssps"
/**
 * @brief set Dynamic SPU HW crypto accelerator power control status
 * @param param 
 * @param enable 
 * @return int
 */
int pwr_api_dynamic_spu_set(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int enable)
{
    int rc = -1;
#ifdef SPU_CTL_SUPPORT
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "echo %d > $(echo %s)", !!!enable, SPU_STATIC_PWM_ENABLE);
    rc = system_run(cmd);
#endif /*SPU_CTL_SUPPORT*/

    return rc;
}

/**
 * @brief get Dynamic SPU HW crypto accelerator power control status
 * @param param 
 * @param enable 
 * @return int 
 */
int pwr_api_dynamic_spu_get(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int *enable)
{
    int rc = -1;
#ifdef SPU_CTL_SUPPORT
    int spu_enable;
    char cmd[256];

    *enable = 0;
    snprintf(cmd, sizeof(cmd), "echo $(cat $(echo %s)) > %s", SPU_STATIC_PWM_ENABLE, SPU_STATIC_PWM_STATUS);
    if ((0 == (rc = system_run(cmd))) && 
        (0 == (rc = pwr_api_file_read_int(SPU_STATIC_PWM_STATUS, &spu_enable))))
        *enable = !spu_enable;
#endif /*SPU_CTL_SUPPORT*/

    return rc;
}
#define PON_PWM_UTIL SCS_UTIL       " /d/pon_pwm"
#define PON_PPWSM_STATE             "/tmp/ppwm_state"
#define PON_PPWSM_SMODE             "/tmp/ppwm_smode"

int pwr_api_pon_cond()
{
    int rc = 0;
    if ((true == is_resource_resident(SCS_UTIL)) &&
        (0 == system_run(PON_PWM_UTIL " stack_mode | grep -i mode: | cut -d'(' -f 2 | cut -d')' -f 1 > " PON_PPWSM_SMODE) &&
         (true == is_resource_resident(PON_PPWSM_SMODE)))) {
            
        char line[256]; 
        if (0 == pwr_api_file_read_line(PON_PPWSM_SMODE, line, 1)) {
             
            rc = (NULL != strstr(line, "XGS")) ? true : false;
        }
    }
    return rc;
}

int pwr_api_pon_set_inline(int enable)
{
    char cmd[256];

    snprintf(cmd, sizeof(cmd), PON_PWM_UTIL " %s", (enable) ? "lsi" : "lwi");

    return system_run(cmd);
}
/**
 * @brief En/Disable PON Power Saving Mode
 * @param[in] param 
 * @param[out] enable 
 * @return int 0 if the feature is supported, -1 otherwise 
 */
int pwr_api_pon_set(__attribute__ ((unused)) char *param, int enable)
{
    int rc = -1;

    if ((true == pwr_api_pon_cond()) && 
        (true == is_resource_resident(SCS_UTIL))) {

        char cmd[256] = {PON_PWM_UTIL};
        unsigned cmd_len = strlen(cmd);

        snprintf(&cmd[cmd_len], sizeof(cmd) - cmd_len, enable ? " lsi" : " lwi");
        rc = system_run(cmd);
    }
    return rc;
}
/**
 * @brief Check whether PON is in Power Saving Mode
 * @param[in] param 
 * @param[out] enable 0 - PON is in Active-Held state, 1 PON is in Power Saving mode
 * @return int 0 if the feature is supported, -1 otherwise
 */
int pwr_api_pon_get(__attribute__ ((unused)) char *param, int *enable)
{
    int rc = -1;

    if ((true == pwr_api_pon_cond()) && 
        (0 != file_exist(SCS_UTIL))) {

        /*Parsing string of type: 'State:  Low-Power/Aware (en/disabled)'*/
        rc = system_run(PON_PWM_UTIL " state | grep -i State: | cut -d'(' -f 2 | cut -d')' -f 1 > " PON_PPWSM_STATE);
        if ((0 == rc) && (true == is_resource_resident(PON_PPWSM_STATE))) {

            char line[256]; 
            if (0 == pwr_api_file_read_line(PON_PPWSM_STATE, line, 1)) {

                *enable = !strncmp(line, "enabled", 7) ? 1 : 0;
                rc = 0;
            }
        }
    }
    return rc;
}

#define WAN_SFP_TRX_ENABLE          "/sys/devices/platform/xfp_sfp/%s_enable"
#define PON_STACK_UTIL              SCS_UTIL " /d pon_stack"
/**
 * @brief disable WAN SFP tx/rx status
 * @param[i] tx/rx
 * @param[i] enable 0 - off, otherwise - on
 * @return int 
 */
static int _pwr_api_wan_sfp_set(char *param, int enable)
{
    int rc = -1, cmd_len  = 0;
    char dev_path[256], cmd[512] = {0};
    bool  is_tx;

    if (!param)
        return -1;

    if (false == is_resource_resident(SCS_UTIL))
        return -1;

    snprintf(dev_path, sizeof(dev_path), WAN_SFP_TRX_ENABLE, param);
    if (false == is_resource_resident(dev_path))
        return -1;

    is_tx = !strcmp(param, "tx");
    if (is_tx && enable)
        cmd_len += APPEND_CHR_ARR(cmd, cmd_len, PON_STACK_UTIL " ld " MUTE_OUTPUT ";"); /* Turn off Rx MAC */
    cmd_len += APPEND_CHR_ARR(cmd, cmd_len, "echo %d > %s" , !!!enable, dev_path);
    if (is_tx && !enable)
        cmd_len += APPEND_CHR_ARR(cmd, cmd_len, ";" PON_STACK_UTIL " la off " MUTE_OUTPUT); /* Turn on Rx MAC */
    rc = system_run(cmd);
    
    return rc;
}

int pwr_api_wan_sfp_set(char *param, int enable)
{
    int rc = 0;

    if (!param || !strcmp(param, "rx"))
        rc |= _pwr_api_wan_sfp_set("rx", enable);

    if (!param || !strcmp(param, "tx"))
        rc |= _pwr_api_wan_sfp_set("tx", enable);

    return rc;
}

/**
 * @brief get WAN SFP tx/rx disable status
 * @param[i] param tx/rx
 * @param[o] enable enable 0 - off, 1 - on 
 * @return int 0 - supported, -1 - N/A
 */
int _pwr_api_wan_sfp_get(char *param, int *enable)
{
    int trx_enable, rc = -1;
    char dev_path[256];
    *enable = 0;

    if (!param)
        return -1;

    snprintf(dev_path, sizeof(dev_path), WAN_SFP_TRX_ENABLE, param);
    if ((0 == (rc = pwr_api_file_read_int(dev_path, &trx_enable))) &&
        (0 <= trx_enable))
    {
        *enable = !((int)trx_enable);
        rc = 0;
    }

    return rc;  
}

int pwr_api_wan_sfp_get(char *param, int *enable)
{
    int rc_rx = -1, rc_tx = -1;
    int rx_en = 0, tx_en = 0;

    if (!param || !strcmp(param, "rx"))
        rc_rx = _pwr_api_wan_sfp_get("rx", &rx_en);

    if (!param || !strcmp(param, "tx"))
        rc_tx = _pwr_api_wan_sfp_get("tx", &tx_en);

    *enable = rx_en || tx_en;

    return (-1 != rc_rx || -1 != rc_tx) ? 0 : -1;
}

int pwr_api_wan_sfp_show(__attribute__ ((unused)) char *param)
{
    pwr_status_show_one(pwr_driver_get(PWR_TYPE_WAN_SFP), "rx");
    pwr_status_show_one(pwr_driver_get(PWR_TYPE_WAN_SFP), "tx");

    return 0;
}


#ifdef BRCM_VOICE_SUPPORT
#define VOICE_CONTROL_FMT    "/bin/send_cms_msg -r -v -t 1000 -c voice 0x%x %d " MUTE_OUTPUT
static int pwr_api_voip_set(__attribute__ ((unused)) char *param, int enable)
{
    char command[256];

    snprintf(command, sizeof(command), VOICE_CONTROL_FMT,
        enable ? CMS_MSG_VOICE_STOP : CMS_MSG_VOICE_START, EID_VOICE);

    return system(command);
}
#else
static int pwr_api_voip_set(__attribute__ ((unused)) char *param, __attribute__ ((unused)) int enable)
{
    return 0;
}
#endif

#define LED_SCRIPT                  "/etc/init.d/led.sh"
#define TMP_LED_STATUS              "/tmp/led"

int pwr_api_led_get(__attribute__ ((unused)) char *param, int *enable)
{
    if (!file_exist(LED_SCRIPT))
        return -1;

    *enable = file_exist(TMP_LED_STATUS);

    return 0;
}

int pwr_api_led_set(__attribute__ ((unused)) char *param, int enable)
{
    char command[256];

    if (!file_exist(LED_SCRIPT))
        return -1;

    if (enable == file_exist(TMP_LED_STATUS))
        return -1;

    snprintf(command, sizeof(command), "%s %s " MUTE_OUTPUT, LED_SCRIPT, enable ? "suspend" : "resume");
    return system(command);
}

/* Power saving drivers list */
static pwr_drv_t pwr_drivers[] = {
    { PWR_TYPE_WIFI,        "WIFI Off",        pwr_api_wifi_suspend_set,       pwr_api_wifi_suspend_get,      NULL },
    { PWR_TYPE_DISK,        "DISK Off",        pwr_api_disk_suspend_set,       pwr_api_disk_suspend_get,      NULL },
    { PWR_TYPE_PCI,         "PCI ASPM",        pwr_api_pcie_aspm_set,          pwr_api_pcie_aspm_get,         NULL },
    { PWR_TYPE_UBUS,        "UBUS DCM",        pwr_api_ubus_dcm_set,           pwr_api_ubus_dcm_get,          NULL },
    { PWR_TYPE_CPU_WAIT,    "CPU Wait",        pwr_api_cpu_wait_set,           pwr_api_cpu_wait_get,          NULL },
    { PWR_TYPE_CPU_SPEED,   "CPU Speed",       pwr_api_cpu_speed_set,          pwr_api_cpu_speed_get,         NULL },
    { PWR_TYPE_XRDP,        "XRDP Gate",       pwr_api_xrdp_clock_gate_set,    pwr_api_xrdp_clock_gate_get,   NULL },
    { PWR_TYPE_XRDP_DIV,    "XRDP Divider",    pwr_api_xrdp_clock_divider_set, pwr_api_xrdp_clock_divider_get,NULL },
    { PWR_TYPE_NET,         "NET Down",        pwr_api_net_down_set,           pwr_api_net_down_get,          pwr_api_net_down_show },
    { PWR_TYPE_EEE,         "PHY EEE",         pwr_api_phy_eee_set,            pwr_api_phy_eee_get,           pwr_api_eee_show },
    { PWR_TYPE_APD,         "PHY APD",         pwr_api_phy_apd_set,            pwr_api_phy_apd_get,           pwr_api_apd_show },
    { PWR_TYPE_DGM,         "SF2 DGM",         pwr_api_sf2_dgm_set,            pwr_api_sf2_dgm_get,           NULL },
    { PWR_TYPE_SR,          "DRAM SR",         pwr_api_dram_sr_set,            pwr_api_dram_sr_get,           NULL },
    { PWR_TYPE_AVS,         "AVS",             NULL,                           pwr_api_avs_get,               NULL },
    { PWR_TYPE_WLDPD,       "WLAN Down",       pwr_api_wldpd_set,              pwr_api_wldpd_get,             pwr_api_wldpd_show },
    { PWR_TYPE_PMD,         "PMD Off",         pwr_api_pmd_set,                pwr_api_pmd_get,               NULL },
    { PWR_TYPE_PHY_SPEED,   "PHY Speed",       pwr_api_phy_speed_set,          pwr_api_phy_speed_get,         NULL },
    { PWR_TYPE_SPU,         "SPU Off",         pwr_api_spu_set,                pwr_api_spu_get,               NULL },
    { PWR_TYPE_DYN_SPU,     "SPU Dynamic Mode",pwr_api_dynamic_spu_set,        pwr_api_dynamic_spu_get,       NULL},
    { PWR_TYPE_PON,         "PON Saving Mode", pwr_api_pon_set,                pwr_api_pon_get,               NULL },
    { PWR_TYPE_WAN_SFP,     "SFP TRX Control", pwr_api_wan_sfp_set,            pwr_api_wan_sfp_get,           pwr_api_wan_sfp_show },
    { PWR_TYPE_VOIP,        "VOIP Off",        pwr_api_voip_set,               NULL,                          NULL },
    { PWR_TYPE_LED,         "LEDs Off",        pwr_api_led_set,                pwr_api_led_get,               NULL }
};

static int pwr_status_show_one(pwr_drv_t *pwr_drv, char *param)
{
    int enable;

    printf("%-20s %-10s", pwr_drv->name, (param) ? param : "");

    if (pwr_drv->enable_get && !pwr_drv->enable_get(param, &enable))
        printf("%s\n", enable ? "Enabled" : "Disabled");
    else
        printf("%s\n", "N/A");

    return 0;
}

static pwr_drv_t *pwr_driver_get(pwr_type_t type)
{
    unsigned int i;

    for (i = 0; i < sizeof(pwr_drivers) / sizeof(pwr_drivers[0]); i++)
    {
        if (pwr_drivers[i].type == type)
            return &pwr_drivers[i];
    }

    return NULL;
}

int pwr_enable_set(pwr_type_t type, char *param, int enable)
{
    pwr_drv_t *pwr_drv;

    if (!(pwr_drv = pwr_driver_get(type)))
        return 0;

    printf("%s (%s) ==> %s\n", pwr_drv->name, param ? param : "*", enable ? "Enable" : "Disable");

    return pwr_drv->enable_set(param, enable);
}

int pwr_profile_activate(pwr_entry_t *profile)
{
    int ret = 0;

    while (profile->type != PWR_TYPE_UNKNOWN)
    {
        ret |= pwr_enable_set(profile->type, profile->param, profile->enable);
        profile++;
    }

    return ret;
}

static int pwr_status_show_global(void)
{
    unsigned int i;
    for (i = 0; i < sizeof(pwr_drivers) / sizeof(pwr_drivers[0]); i++)
    {
        pwr_status_show_one(&pwr_drivers[i], NULL);
    }

    return 0;
}

int pwr_status_show(pwr_type_t type, char *param)
{
    pwr_drv_t *pwr_drv;

    if (!(pwr_drv = pwr_driver_get(type)))
        return pwr_status_show_global();

    if (param)
        return pwr_status_show_one(pwr_drv, param);

    if (pwr_drv->enable_show)
        return pwr_drv->enable_show(param);
    else
        return pwr_status_show_one(pwr_drv, NULL);

    return -1;
}
/**
 * @brief Checks system command return code
 * @param caller calling method
 * @param status system return code
 * @param sys_cmd system command line
 * @note should be calledby it's macro wrapper system_check defined in this file header
 * @return int 0 on success, -1 otherwise
 */
static int ___system_check(const char *caller, int status, const char *sys_cmd)
{
    int rc = 0;//Assuming success

    if ((-1 == status) || (0 == WIFEXITED(status)) || (0 != WEXITSTATUS(status))) {

        printf("%s|ERROR|Failed to '%s'. Reason: WIFEXITED %d, WEXITSTATUS %d\n",
               caller, sys_cmd, WIFEXITED(status), WEXITSTATUS(status));
        rc = -1;
    }

   return rc;
}
/**
 * @brief checks if resource is residend in the system
 * @param caller calling method
 * @param resource resource location in the file system
 * @return true if resident
 * @return false if not resident
 */
static bool ___is_resource_resident(__attribute__ ((unused)) const char * caller, const char *resource)
{
    bool rc = true;//Assuming success

    if (!file_exist((char *)resource)) {

        // printf("%s|ERROR|%s is not found.\n", caller, resource);
        rc  = false;
    }
    return rc;
}

int pwr_wake_set(wake_type_t wake_type, char *param1, char *param2)
{
    int ret = -1;
    int skfd;
    char *ifname = NULL;
    struct ifreq ifr;
    struct ethctl_data ethctl;

    memset((void *)&ifr, 0, sizeof(struct ifreq));
    memset((void *)&ethctl, 0, sizeof(ethctl));

    if (wake_type == WAKE_TYPE_MAGIC || wake_type == WAKE_TYPE_ARP || wake_type == WAKE_TYPE_LINK)
    {
        ifname = param1;

        if (!ifname)
            return -1;

        if (wake_type == WAKE_TYPE_MAGIC)
            ethctl.flags |= ETHCTL_FLAG_WAKE_MPD_SET;
        else if (wake_type == WAKE_TYPE_ARP)
            ethctl.flags |= ETHCTL_FLAG_WAKE_ARD_SET;
        else if (wake_type == WAKE_TYPE_LINK)
            ethctl.flags |= ETHCTL_FLAG_WAKE_LNK_SET;

        if (param2 && !strcasecmp(param2, "int"))
            ethctl.flags |= ETHCTL_FLAG_WAKE_INT_SET;
    }

    if (wake_type == WAKE_TYPE_BUTTON)
        ethctl.flags |= ETHCTL_FLAG_WAKE_BTN_SET;

    if (wake_type == WAKE_TYPE_TIMER)
    {
        ethctl.flags |= ETHCTL_FLAG_WAKE_TME_SET;
        ethctl.val = param1 ? atoi(param1) : 0;
    }

    if (wake_type == WAKE_TYPE_WAN)
        ethctl.flags |= ETHCTL_FLAG_WAKE_WAN_SET;

    if (sock_init(ifname, &skfd, &ifr) < 0)
        goto Exit;

    ethctl.op = ETHPHYWAKEENABLE; 
    ethctl.sub_port = -1;

    ifr.ifr_data = (void *)&ethctl;

    ret = ioctl(skfd, SIOCETHCTLOPS, &ifr);

Exit:
    if (skfd >= 0)
        close(skfd);

    return ret;
}
