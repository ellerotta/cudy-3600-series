/***********************************************************************
 *
 * <:copyright-BRCM:2015:DUAL/GPL:standard
 *
 *    Copyright (c) 2015 Broadcom
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net_port.h>
#include "wanconf.h"
#include <bcmnet.h>
#include <bcm/bcmswapitypes.h>
#include "wan_drv.h"

/* wanconf_mgmt */
void wanconf_brcm_notify(struct net_port_t *net_port, int is_create);

/* wanconf_psp */
int get_parameters_from_scratchpad(net_port_t *net_port_p, int *is_auto);
int set_parameters_to_scratchpad(char *wan_type, char *wan_speed);
int dump_scratchpad(void);

/* libwanconf */
extern int load_trx_modules(net_port_t *net_port);
extern int unload_trx_modules(void);
extern int wanconf_interface_create(net_port_t *net_port_p);
extern int wanconf_interface_destroy(net_port_t *net_port_p, char *ifname);


static int bcm_ifname_get(struct net_port_t *net_port)
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
    ethswctl.op = ETHSWPORTSERDESNAME;
    err = ioctl(skfd, SIOCETHSWCTLOPS, &ifr);
    if (err < 0 )
        wc_log_err("Error %d bcmenet port\n", err);

    strncpy(net_port->ifname, ethswctl.net_port.ifname, IFNAMSIZ);

exit:
    close(skfd);
    return err;
}

#if defined(CONFIG_BCM963158)
static int get_sfp_info(sfp_type_t *sfp_type)
{
    struct ifreq ifr;
    int err, skfd;
    struct ethctl_data ethctl;

    memset(&ethctl, 0x0, sizeof(struct ethctl_data));
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("socket open error\n");
        return -1;
    }

    strcpy(ifr.ifr_name, "bcmsw");
    if ((err = ioctl(skfd, SIOCGIFINDEX, &ifr)) < 0 )
    {
        printf("bcmsw interface does not exist");
        goto exit;
    }

    ethctl.op = ETHGETSFPINFO;
    ifr.ifr_data = (void *)&ethctl;
    err = ioctl(skfd, SIOCETHCTLOPS, &ifr);
    if (err < 0 )
        printf("Error %d bcmenet gbe wan port\n", err);

    *sfp_type = ethctl.ret_val;

exit:
    close(skfd);
    return err;

}

static int waiting_gpon_module(void)
{
    int err;
    sfp_type_t sfp_type = SFP_TYPE_NO_MODULE;

    for (;;) {
        if ((err = get_sfp_info(&sfp_type))) {
            return err;
        }

        /* If the module is KNOWN XPON SFP or 10GAE port is not defined in
            board parameters, thus this is GPON only board design
            we quit waiting */
        if (sfp_type == SFP_TYPE_XPON || sfp_type == SFP_TYPE_NOT_ETHERNET)
            break;

        sleep(1);
    }

    return 0;
}
#endif

static int do_detect_wan_port(int signal_detect_required __attribute__((unused)),
    int scratchpad_set_required __attribute__((unused)))
{
#if !defined(CONFIG_BCM963158)
    int rc;

    rc = try_wan_type_detect_and_set(signal_detect_required,
        scratchpad_set_required);
    if (DETECTION_OK != rc)
    {
        wc_log_err("Failed to detect optical\n");
        return -1;
    }

    return 0;
#endif
    return -1;
}

static serdes_wan_type_t net_port_to_serdes_wan_type(int port, int sub_type, int speed)
{
    if (port == NET_PORT_GPON)
    {
        switch (sub_type)
        {
            case NET_PORT_SUBTYPE_GPON:
                return SERDES_WAN_TYPE_GPON;
            case NET_PORT_SUBTYPE_XGPON:
                return SERDES_WAN_TYPE_XGPON_10G_2_5G;
            case NET_PORT_SUBTYPE_NGPON:
                if (speed == NET_PORT_SPEED_1025)
                    return SERDES_WAN_TYPE_NGPON_10G_2_5G;
                /* fall through */
            case NET_PORT_SUBTYPE_XGS:
                return SERDES_WAN_TYPE_NGPON_10G_10G;
            default:
                return SERDES_WAN_TYPE_NONE;
        }
    }
    else if (port == NET_PORT_EPON)
    {
        switch (speed)
        {
            case NET_PORT_SPEED_1010:
                return SERDES_WAN_TYPE_EPON_10G_SYM;
            case NET_PORT_SPEED_1001:
                return SERDES_WAN_TYPE_EPON_10G_ASYM;
            case NET_PORT_SPEED_0101:
                return SERDES_WAN_TYPE_EPON_1G;
            case NET_PORT_SPEED_2501:
                return SERDES_WAN_TYPE_EPON_2G;
            default:
                return SERDES_WAN_TYPE_NONE;
        }
    }
    else
    {
        return SERDES_WAN_TYPE_NONE;
    }
}

/* Pre-init wan serdes if wan type is known */
static int do_preload(void)
{
    struct net_port_t net_port = {0};
    int rc, is_auto_detect_mode;
    serdes_wan_type_t serdes_wan_type;
    char cmd[128] = {0};

    rc = get_parameters_from_scratchpad(&net_port, &is_auto_detect_mode);
    if (rc)
        return rc;

    if (!is_auto_detect_mode && net_port.port == NET_PORT_NONE)
        return 0;

    /* for power saving, don't pre-init for AE mode */
    if (net_port.port == NET_PORT_AE)
        return 0;

    /* net_port type -> wan serdes type */
    serdes_wan_type = net_port_to_serdes_wan_type(net_port.port, net_port.sub_type, net_port.speed);
    if (serdes_wan_type == SERDES_WAN_TYPE_NONE)
        return 0;

    if (load_trx_modules(&net_port))
        return -1;

    /* wan serdes init */
    sprintf(cmd, "serdesctrl reinit %d", serdes_wan_type);
    rc = system(cmd);
    if (rc)
        wc_log_err("unable to pre-init wan serdes: %s\n", cmd);

    return rc;
}

static int cli_parse_args(int argc, char *argv[], int *is_delete, int *is_daemonize)
{
    int c, dump = 0, sense = 0;
    char *wan_type = NULL, *wan_speed = NULL;
    // detection_opts is for "x"
    // a-b: a stands signal detect is required
    //      b stands for scratchpad set is required
    char *detection_opts = 0;

    *is_delete = 0;
    *is_daemonize = 1; /* Default is daemonize */

    while ((c = getopt(argc, argv, "x:rdft:s:")) != -1)
    {
        switch(c)
        {
            case 'f':
                *is_daemonize = 0;
                break;
            case 'r':
                dump = 1;
                break;
            case 'd':
                *is_delete = 1;
                return 1;
            case 't':
                wan_type = optarg;
                break;
            case 's':
                wan_speed = optarg;
                break;
            case 'x':
               sense = 1;
               detection_opts = optarg;
               break;
            default:
                printf("%s: bad arguments, exit\n", argv[0]);
                goto Stop_program;
        }
    }

    if (dump)
    {
        dump_scratchpad();
        goto Stop_program;
    }
    if (sense)
    {
        struct net_port_t net_port = {0};
        net_port.is_wan = 1;

        if (load_trx_modules(&net_port) != 0)
            goto Stop_program;

        do_detect_wan_port(atoi(detection_opts),
            atoi(detection_opts+2));
        goto Stop_program;
    }

    if (argc == 1 || (argc == 2 && *is_daemonize == 0))
        return 1; /* No arguments (or just is_daemonize) means create interface */

    set_parameters_to_scratchpad(wan_type, wan_speed);

Stop_program:
    return 0; /* Stop program after returning */
}


static int cli_parse_long_args(int argc, char *argv[])
{
    int curarg = 1;

    if (argc == 1)
        goto Continue_program;

    /* parse long args only */
    while (argc > curarg)
    {
        if (!strcmp(argv[curarg], "--preload"))
        {
            do_preload();
            goto Stop_program;
        }
        else
        {
            goto Continue_program;
        }
        curarg++;
    }

Stop_program:
    return 0; /* Stop program after returning */

Continue_program:
    return 1; /* Continue short args parsing after returning */
}

static int handle_auto_detect(struct net_port_t *net_port)
{
    if (do_detect_wan_port(0, 1)) /* This will update scratchpad, so re-read parameters again below */
        return -1;

    if (get_parameters_from_scratchpad(net_port, NULL))
        return -1;

    return 0;
}

static int create(struct net_port_t *net_port, int is_auto_detect_mode)
{
    int rc;

    if ((rc = load_trx_modules(net_port)))
        return rc;

    if (is_auto_detect_mode && (rc = handle_auto_detect(net_port)))
        return rc;

    rc = wanconf_interface_create(net_port);
    if (!rc)
        wanconf_brcm_notify(net_port, 1);

    return rc;
}

static int delete(struct net_port_t *net_port)
{
    if (bcm_ifname_get(net_port))
    {
        wc_log_err("Cannot find an enet WAN serdes interface name\n");
        return -1;
    }

    wanconf_brcm_notify(net_port, 0);

    if (wanconf_interface_destroy(net_port, net_port->ifname))
        return -1;

    return unload_trx_modules();
}

static int do_create_delete(int is_delete)
{
    struct net_port_t net_port = {0};
    int rc, is_auto_detect_mode;

    rc = get_parameters_from_scratchpad(&net_port, &is_auto_detect_mode);
    if (rc)
        return rc;

    if (!is_auto_detect_mode && net_port.port == NET_PORT_NONE)
        return 0;

    if (!is_delete)
        rc = create(&net_port, is_auto_detect_mode);
    else
        rc = delete(&net_port);

    return rc;
}

int main(int argc, char *argv[])
{
    pid_t childPid;
    int is_delete, is_daemonize;

    if (!cli_parse_long_args(argc, argv))
        return 0; /* Exit program if non-zero */

    if (!cli_parse_args(argc, argv, &is_delete, &is_daemonize))
        return 0; /* Exit program if non-zero */

    if (is_delete) /* Override CLI options: 'delete' should always run in foreground */
        is_daemonize = 0;

    if (is_daemonize)
    {
        childPid = fork();
        if (childPid < 0) /* Failed to fork */
            return -1;

        if (childPid != 0) /* Father always exists */
            return 0;
    }

#if defined(CONFIG_BCM963158)
    /* Waiting GPON SFP module insertion for GPON/10GAE co-existence when it is not delete */
    if ((!is_delete) && waiting_gpon_module())
        return -1;
#endif

    return do_create_delete(is_delete);
}

