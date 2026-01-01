/***********************************************************************
 *
 * <:copyright-BRCM:2022:DUAL/GPL:standard
 *
 *    Copyright (c) 2022 Broadcom
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

#include <net/if.h>
#include <sys/ioctl.h>
#include <net_port.h>
#include <dirent.h>
#include <sched.h>
#include <stdlib.h>
#include <errno.h>

#include "wanconf.h"
#include "sysutil_fs.h"
#include <bcmnet.h>
#include <bcm/bcmswapitypes.h>
#include "wanconf.h"
#include <net_port.h>
#include <pmd.h>

extern int rdpaCtl_time_sync_init();
extern void set_iptv_lookup_method(void);
#if defined(PMD_JSON_LIB)
extern int set_pmd_wan_type(net_port_t *net_port);
#endif

#define TRX_MOD_FILE_LOCK         "/tmp/bcm_bootflag_load_trx_modules"
#define INSMOD_EXTRA(module_name) insmod("/lib/modules/"KERNELVER"/extra/"module_name)


static int bcm_port(int is_add, struct net_port_t *net_port)
{
    struct ifreq ifr;
    int err, skfd;
    struct ethswctl_data ethswctl;

    memset(&ethswctl, 0x0, sizeof(struct ethswctl_data));
    memcpy(&ethswctl.net_port, net_port, sizeof(net_port_t));

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        wc_log_err("socket open error\n");
        return -1;
    }

    strcpy(ifr.ifr_name, "bcmsw");
    if ((err = ioctl(skfd, SIOCGIFINDEX, &ifr)) < 0 )
    {
        wc_log_err("bcmsw interface does not exist\n");
        goto exit;
    }

    ifr.ifr_data = (void *)&ethswctl;
    ethswctl.op = is_add ? ETHSWPORTCREATE : ETHSWPORTDELETE;
    err = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    if (err < 0 )
        wc_log_err("Error %d bcmenet port\n", err);

    if (is_add)
        strncpy(net_port->ifname, ethswctl.net_port.ifname, IFNAMSIZ);

exit:
    close(skfd);
    return err;
}

static int bcm_port_create(struct net_port_t *net_port)
{
    return bcm_port(1, net_port);
}

static int bcm_port_delete(int port)
{
    int rc;
    struct net_port_t net_port =
    {
        .port = port,
    };

    rc = bcm_port(0, &net_port);
    if (rc)
    {
        wc_log_err("Failed to delete (port=%d rc=%d)\n", port, rc);
    }
    else
        wc_log("Port deleted (port=%d rc=%d)\n", port, rc);

    return rc;
}

static int insmod_param(char *driver, char *param)
{
    char cmd[128];
    int ret;

    sprintf(cmd, "insmod %s %s", driver, param ? : "");
    ret = system(cmd);
    if (ret)
        wc_log_err("unable to load module: %s\n", cmd);

    return ret;
}

static int rmmod_param(char *driver, char *param)
{
    char cmd[128];
    int ret;

    sprintf(cmd, "rmmod %s %s", driver, param ? : "");
    ret = system(cmd);
    if (ret)
        wc_log_err("unable to unload module: %s\n", cmd);

    return ret;
}

static int is_module_loaded(char *mod_name)
{
    FILE *fp;
    char mod_status_str[256];
    char cmd_line[128];
    int is_loaded = 0;

    sprintf(cmd_line, "cat /proc/modules | grep \"^%s\"", mod_name);
    fp = popen(cmd_line, "r");
    if (!fp)
    {
        wc_log_err("Failed to run: %s\n", cmd_line);
        return 0;
    }

    if (fgets(mod_status_str, sizeof(mod_status_str), fp))
        is_loaded = strlen(mod_status_str) > 0;

    pclose(fp);

    return is_loaded;
}

static void trx_mod_lock(void)
{
#define TIME_OUT     120 /* sec */
#define INTERVAL     1   /* sec */
    int time = 0;

    while (time < TIME_OUT)
    {
        if (!sysUtil_isFilePresent(TRX_MOD_FILE_LOCK))
        {
            system("touch "TRX_MOD_FILE_LOCK);
            return;
        }
        sleep(INTERVAL);
        time += INTERVAL;
    }

    if (time >= TIME_OUT)
        wc_log_err("Failed to lock trx mod: timeout!\n");
}

static void trx_mod_unlock(void)
{
    system("rm -f "TRX_MOD_FILE_LOCK);
}

int load_trx_modules(net_port_t *net_port)
{
    int rc = -1;

    trx_mod_lock();
    wc_log("%s ...\n", __FUNCTION__);

    if (!is_module_loaded("bcm_pondrv") && INSMOD_EXTRA("bcm_pondrv.ko"))
        goto exit;

#if defined(CONFIG_BCM_SMTC)
    if (!is_module_loaded("smtc"))
        INSMOD_EXTRA("smtc.ko");
#endif

#if defined(CONFIG_BCM_SYNCE_HOLDOVER)
    if (!is_module_loaded("synce_holdover") && INSMOD_EXTRA("synce_holdover.ko"))
        goto exit;
#endif

#if defined(PMD_JSON_LIB)
    int is_pmd;

    /* Must be loaded even when not in PMD module because of symbol dep. */
    /* Load here before PMD WAN type auto-detection may take place.      */
    if (!is_module_loaded("pmd") && INSMOD_EXTRA("pmd.ko"))
        goto exit;

    is_pmd = !access("/proc/pmd", 0);  /* Should we have a better method to detect PMD? */
    /* Must be called before loading laser_dev.ko, which creates /dev/laser for non PMD */
    if ((net_port->port == NET_PORT_GPON || net_port->port == NET_PORT_EPON) && is_pmd && set_pmd_wan_type(net_port))
        goto exit;
#endif

#if defined(CONFIG_BCM_LASER)
    if (!is_module_loaded("laser_dev") && INSMOD_EXTRA("laser_dev.ko"))
        goto exit;
#endif

    rc = 0;

exit:

    trx_mod_unlock();
    return rc;
}

int unload_trx_modules(void)
{
    int rc = -1;

    trx_mod_lock();
    wc_log("%s ...\n", __FUNCTION__);

#if defined(CONFIG_BCM_LASER)
    if (is_module_loaded("laser_dev") && rmmod("laser_dev.ko"))
        goto exit;
#endif

#if defined(PMD_JSON_LIB)
    if (is_module_loaded("pmd") && rmmod("pmd.ko"))
        goto exit;
#endif

#if defined(CONFIG_BCM_SYNCE_HOLDOVER)
    if (is_module_loaded("synce_holdover") && rmmod("synce_holdover.ko"))
        goto exit;
#endif

#if defined(CONFIG_BCM_SMTC)
    if (is_module_loaded("smtc"))
        rmmod("smtc.ko");
#endif

    if (is_module_loaded("bcm_pondrv") && rmmod("bcm_pondrv.ko"))
        goto exit;

    rc = 0;

exit:

    trx_mod_unlock();
    return rc;
}

static int getPidByName(const char *name)
{
    DIR *dir;
    FILE *fp;
    struct dirent *dent;
    UBOOL8 found=FALSE;
    long pid;
    int  rc, p, i;
    int rval = -1;
    char filename[256];
    char processName[256];
    char *endptr;

    if (NULL == (dir = opendir("/proc")))
    {
        wc_log_err("could not open /proc\n");
        return -1;
    }

    while ( (!found) && (NULL != (dent = readdir(dir))) )
    {
        /*
         * Each process has its own directory under /proc, the name of the
         * directory is the pid number.
         */
        if (DT_DIR != dent->d_type)
            continue;

        pid = strtol(dent->d_name, &endptr, 10);
        if (ERANGE != errno && endptr != dent->d_name)
        {
            snprintf(filename, sizeof(filename), "/proc/%ld/stat", pid);
            if (NULL == (fp = fopen(filename, "r")))
            {
                wc_log_err("could not open %s\n", filename);
            }
            else
            {
                /* Get the process name, format: 913 (consoled) */
                memset(processName, 0, sizeof(processName));
                rc = fscanf(fp, "%d (%s", &p, processName);
                fclose(fp);

                if (rc >= 2)
                {
                    i = strlen(processName);
                    if (i > 0)
                    {
                        /* strip out the trailing ) character */
                        if (')' == processName[i-1])
                            processName[i-1] = 0;
                    }
                }

                if (!strncmp(processName, name,strlen(name)))
                {
                    rval = pid;
                    found = TRUE;
                }
            }
        }
    }

    closedir(dir);

    return rval;
}

static int load_epon_modules(int launch_epon_app, int speed)
{
#define KTHREAD_NAME "EponMPCP"
    int pid;
    struct sched_param sp = { .sched_priority = 10 };
    char cmd[128];
    int rc;

    sprintf(cmd, "insmod /lib/modules/"KERNELVER"/extra/bcmepon.ko epon_usr_init=%d xepon_mode=%d",
        launch_epon_app, speed >= NET_PORT_SPEED_1001);
    printf("load_epon_modules: %s\n\n", cmd);
    rc = system(cmd);
    if (rc)
    {
        wc_log("Failed to load epon modules (rc=%d)\n", rc);
        return rc;
    }

    if (!launch_epon_app)
        return 0;

    pid = getPidByName(KTHREAD_NAME);
    if (pid > 0)
    {
        if (sched_setscheduler(pid, SCHED_RR, &sp))
        {
            wc_log_err("failed to set kthread %s with scheduler RR\n", KTHREAD_NAME);
            return -1;
        }
    }
    else
    {
        wc_log_err("unable to find pid for kthread %s\n", KTHREAD_NAME);
    }

    return rc;
}

static int remove_epon_modules(void)
{
    return rmmod("bcmepon.ko");
}

static int load_gpon_modules(int sub_type)
{
    int rc;

    rc = INSMOD_EXTRA("gpontrx.ko");

    if (sub_type == NET_PORT_SUBTYPE_GPON)
        rc = rc ? : INSMOD_EXTRA("gponstack.ko");
    else
        rc = rc ? : INSMOD_EXTRA("ngponstack.ko");

    rc = rc ? : INSMOD_EXTRA("bcmgpon.ko");

    rc = rc ? : system("cat /dev/rgs_logger &");

   return rc;
}

static int remove_gpon_modules(int sub_type)
{
    int rc;

    rc = rmmod("bcmgpon.ko");

    if (sub_type == NET_PORT_SUBTYPE_GPON)
        rc = rc ? : rmmod("gponstack.ko");
    else
        rc = rc ? : rmmod("ngponstack.ko");

    rc = rc ? : rmmod("gpontrx.ko");
    if (rc)
        wc_log("gpon module removed\n");

    return (rc) ? -1: 0;
}

static int load_pon_modules(net_port_t *net_port)
{
    switch (net_port->port)
    {
        case NET_PORT_GPON:
            if (load_gpon_modules(net_port->sub_type))
                return -1;
            break;
        case NET_PORT_AE:
#if defined(SUPPORT_EPON_AE)
            if (load_epon_modules(net_port->port == NET_PORT_EPON, net_port->speed))
                return -1;
#endif
            break;
        case NET_PORT_EPON:
            if (load_epon_modules(net_port->port == NET_PORT_EPON, net_port->speed))
                return -1;
            break;
    }

    return 0;
}

int wanconf_interface_create(net_port_t *net_port_p)
{
    int rc = 0;

    if (load_pon_modules(net_port_p))
    {
        return -1;
    }

    if ((rc = bcm_port_create(net_port_p)))
    {
        wc_log_err("Failed to create port (port=%d sub_type=%d rc=%d)\n", net_port_p->port, net_port_p->sub_type, rc);
        return -1;
    }

    switch (net_port_p->port)
    {
        case NET_PORT_GPON:
            set_iptv_lookup_method();
            break;
    }

    if ((rc = rdpaCtl_time_sync_init()))
    {
        wc_log_err("Failed to call rdpaCtl_time_sync_init ioctl (rc=%d)\n", rc);
        return -1;
    }

    return 0;
}

static int pon_logger_dump_pid(void)
{
    FILE *p;
    int pid;
    char buf[128];

    p = popen("ps | grep r[g]s_logger", "r");
    fscanf(p, "%s", &buf[0]);
    pclose(p);
    sscanf(buf,"%d", &pid);

    return pid;
}

static int stop_pon_logger_dump(void)
{
    char kill_cmd[128];
    int pid = pon_logger_dump_pid();
    int rc = -1;

    if (pid > 0)
    {
        /* Send kill signal; process will not die since it is still blocked */
        snprintf(kill_cmd, sizeof(kill_cmd), "kill -9 %d", pid);
        rc = system(kill_cmd);

        snprintf(kill_cmd, sizeof(kill_cmd), "bs /d/o testlogger 2>&1  > /dev/null");
        /*
            activate logger by 'testlogger' command, this will awake 'cat /dev/rgs_logger' process
            and it will be possible to kill it
        */
        rc = system(kill_cmd);
    }
    else
    {
        wc_log_err(" [cat /dev/rgs_logger] pid is zero\n");
    }

    return rc;
}

static int take_interface_down(char *if_name)
{
    int err;
    char cmd[128];

    sprintf(cmd, "ifconfig %s down", if_name);
    if ((err = system(cmd)))
    {
        wc_log_err("system command failed: '%s' err=%d\n", cmd, err);
        return -1;
    }

    wc_log("system command: '%s' \n", cmd);
    sleep(2);

    return 0;
}

static int remove_pon_modules(net_port_t *net_port)
{
    switch (net_port->port)
    {
        case NET_PORT_GPON:
            if (remove_gpon_modules(net_port->sub_type))
                return -1;
            break;
        case NET_PORT_AE:
#if defined(SUPPORT_EPON_AE)
           if (remove_epon_modules())
                return -1; 
#endif
            break;
        case NET_PORT_EPON:
            if (remove_epon_modules())
                return -1;
            break;
    }

    return 0;
}

int wanconf_interface_destroy(net_port_t *net_port_p, char *ifname)
{
    /* before deleting the interface, bring it down and wait until depended interfaces are down too.
     * if the interface is up and has vlan or ppp interfaces on top of it, the operation might get stuck
     * until the base interface is freed */
    if (take_interface_down(ifname))
    {
        wc_log_err("Failed to disable WAN network interface\n");
        return -1;
    }

    switch (net_port_p->port)
    {
        /* XXX: move this to remove_gpon_modules() somehow: but removing the port removes the cli api */
        case NET_PORT_GPON:
            stop_pon_logger_dump();
    }

    if (bcm_port_delete(net_port_p->port))
        return -1;

    return remove_pon_modules(net_port_p);
}

