// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright 2019 Broadcom Ltd.
 *	Ido Brezel <ido.brezel@broadcom.com>
 */

#include <common.h>
#include <dm.h>
#include <net.h>
#include <environment.h>
#include <command.h>
#include <dm/lists.h>
#include <dm/device-internal.h>

#ifdef CONFIG_BCMBCA_PMC_XRDP
#include "pmc_xrdp.h"
#ifdef CONFIG_SMC_BASED
#include "smc_rccore.h"
#endif
#endif
#ifdef CONFIG_BCM6858
#include "pmc_lport.h"
#endif
#ifdef CONFIG_BCMBCA_UBUS4
#include "bcm_ubus4.h"
#endif

#include "bca_common.h"
#include "bus_drv.h"
#include "mac_drv.h"
#include "phy_drv.h"
#include "bcmbca_xrdp_api.h"


/* Min frame ethernet frame size without FCS */
#define ETH_ZLEN 60

static int active_port = -1;

#ifdef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
static int env_active_port = -1;
#define ENV_ACTIVE_PORT     "active_port"

static int get_env_active_port(void)
{
#ifndef CONFIG_BCMBCA_IKOS
	char *env = env_get(ENV_ACTIVE_PORT);

	if (!env)
		return -1;

	return simple_strtoul(env, NULL, 10);
#else
	return -1;
#endif
}

int bcmbca_xrdp_eth_active_port_get(void)
{
	return active_port;
}

int bcmbca_xrdp_eth_active_port_set(int port)
{
	phy_dev_t *phy_dev;

	if (port < 0 || port >= dt_get_ports_num()) {
		printf("port %d is out of range\n", port);
		return -1;
	}

	phy_dev = dt_get_phy_dev(port);
	if (!phy_dev) {
		printf("port %d is not availabe\n", port);
		return -1;
	}

#ifndef CONFIG_BCMBCA_IKOS
	if (!phy_dev->link) {
		printf("port %d can not be set to active when the link is down"\
			"\n", port);
		return -1;
	}
#endif

	active_port = port;

	return 0;
}

int bcmbca_xrdp_eth_env_active_port_set(int port)
{
	env_active_port = -1;

	if (port == -1)
		return 0;

	if (bcmbca_xrdp_eth_active_port_set(port))
		return -1;

	env_active_port = port;

	return 0;
}

#ifndef CONFIG_BCMBCA_IKOS
static int active_port_handle_poll(int port, int link)
{
	return env_active_port != -1; /* skip */
}
#endif

static void env_active_port_init(void)
{
	active_port = env_active_port = get_env_active_port();
	if (active_port != -1)
		printf("Active port was set to %d according to the environment "
		       "variable\n", active_port);
}

#endif /* CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE */

static void mac_set_cfg_by_phy(phy_dev_t *phy_dev, mac_dev_t *mac_dev)
{
	mac_cfg_t mac_cfg = {};

	if (!mac_dev || !phy_dev)
		return;

	mac_dev_disable(mac_dev);
	mac_dev_cfg_get(mac_dev, &mac_cfg);

	if (phy_dev->speed == PHY_SPEED_10)
		mac_cfg.speed = MAC_SPEED_10;
	else if (phy_dev->speed == PHY_SPEED_100)
		mac_cfg.speed = MAC_SPEED_100;
	else if (phy_dev->speed == PHY_SPEED_1000)
		mac_cfg.speed = MAC_SPEED_1000;
	else if (phy_dev->speed == PHY_SPEED_2500)
		mac_cfg.speed = MAC_SPEED_2500;
	else if (phy_dev->speed == PHY_SPEED_5000)
		mac_cfg.speed = MAC_SPEED_5000;
	else if (phy_dev->speed == PHY_SPEED_10000)
		mac_cfg.speed = MAC_SPEED_10000;
	else
		mac_cfg.speed = MAC_SPEED_UNKNOWN;

	mac_cfg.duplex = phy_dev->duplex == PHY_DUPLEX_FULL ? MAC_DUPLEX_FULL :
		MAC_DUPLEX_HALF;
    mac_cfg.flag &= ~MAC_FLAG_XGMII;
    mac_cfg.flag |= phy_dev_is_xgmii_mode(phy_dev) ? MAC_FLAG_XGMII : 0;
	mac_dev_cfg_set(mac_dev, &mac_cfg);

	if (phy_dev->link) {
		char eth_addr[ETH_ALEN];

		eth_env_get_enetaddr("ethaddr", (uchar *)eth_addr);
		// XXX: TODO
		//struct eth_pdata *pdata = dev_get_platdata(dev);
		//memcpy(port->dev_addr, pdata->enetaddr, ETH_ALEN);
		mac_dev_pause_set(mac_dev, phy_dev->pause_rx, phy_dev->pause_tx,
			eth_addr);
		mac_dev_enable(mac_dev);
	}
}

static int _net_init(void)
{
	int rc = 0;
	static int is_initialized;
#ifdef CONFIG_SMC_BASED	
	unsigned int pmc_rccore_zone = 0;
#endif	

	if (is_initialized)
		return -1;

#ifdef CONFIG_BCMBCA_UBUS4
	bcm_ubus_config();
#endif

#ifdef CONFIG_BCM6858
	rc = pmc_lport_init();
	if (rc) {
		printf("pmc lport init failed\n");
		goto error;
	}
#endif

#ifdef CONFIG_BCMBCA_PMC_XRDP
	rc = pmc_xrdp_init();
	if (rc) {
		printf("pmc xrdp init failed\n");
		goto error;
	}
	
#ifdef CONFIG_SMC_BASED	
	for (pmc_rccore_zone = 0; pmc_rccore_zone < PMC_RCCORE_MAX; pmc_rccore_zone++)
	{
		rc = smc_rccore_power_up(pmc_rccore_zone);
		if (rc)
		{
			printf("[%s] rc[%d] pmc_rccore_zone[%d]\n",__FUNCTION__, rc, pmc_rccore_zone);
			goto error;
		}
	}
#endif	
#endif

	rc = bcmbca_xrdp_init();
	if (rc) {
		printf("XRDP init failed\n");
		goto error;
	}

	rc = dt_probe();
	if (rc) {
		printf("device tree probe failed\n");
		goto error;
	}

	is_initialized = 1;

error:
	return rc;
}

static int _net_uninit(void)
{
	int i;
	int rc = 0;
#ifdef CONFIG_SMC_BASED	
	unsigned int pmc_rccore_zone = 0;
#endif	

	for (i = 0; i < dt_get_ports_num(); i++) {
		mac_dev_t *mac_dev = dt_get_mac_dev(i);
		phy_dev_t *phy_dev = dt_get_phy_dev(i);

		if (phy_dev) {
			phy_dev_power_set(phy_dev, 0);
			phy_dev_del(phy_dev);
		}

		if (mac_dev) {
			mac_dev_disable(mac_dev);
			mac_dev_del(mac_dev);
		}
	}

#ifdef CONFIG_BCMBCA_PMC_XRDP
	rc = pmc_xrdp_shutdown();
	if (rc) {
		printf("pmc xrdp shutdown failed\n");
		goto exit;
	}
	
#ifdef CONFIG_SMC_BASED		
	for (pmc_rccore_zone = 0; pmc_rccore_zone < PMC_RCCORE_MAX; pmc_rccore_zone++)
	{
		rc = smc_rccore_power_down(pmc_rccore_zone);
		if (rc)
		{
			printf("[%s] rc[%d] pmc_rccore_zone[%d]\n",__FUNCTION__, rc, pmc_rccore_zone);
			goto exit;
		}
	}
#endif

	rc = pmc_xrdp_reset();
	if (rc) {
		printf("pmc xrdp reset failed\n");
		goto exit;
	}
#endif

#ifdef CONFIG_BCM6858
	rc = pmc_lport_shutdown();
	if (rc) {
		printf("pmc lport shutdown failed\n");
		goto exit;
	}
#endif

#if defined(CONFIG_BCM6858) || defined(CONFIG_BCMBCA_PMC_XRDP)
exit:
#endif
	return rc;
}

static void phy_poll(void)
{
#ifndef CONFIG_BCMBCA_IKOS
	int i;

	for (i = 0; i < dt_get_ports_num(); i++) {
		int link;
		mac_dev_t *mac_dev = dt_get_mac_dev(i);
		phy_dev_t *phy_dev = dt_get_phy_dev(i);

		if (!phy_dev)
			continue;

		link = phy_dev->link;
		phy_dev_read_status(phy_dev);

		if (link != phy_dev->link) {
			mac_set_cfg_by_phy(phy_dev, mac_dev);
			phy_dev_print_status(phy_dev);
		}

#ifdef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
		if (active_port_handle_poll(i, phy_dev->link))
			continue;
#endif

		if (!phy_dev->link && active_port == i)
			active_port = -1;

		if (phy_dev->link && active_port < 0) {
			active_port = i;
			printf("active port %d\n", i);
		}
	}
#endif
}

int bcmbca_link_status(void)
{
	phy_dev_t *phy_dev;

	phy_poll();

	if (active_port >= 0) {
	    phy_dev = dt_get_phy_dev(active_port);
	    if (phy_dev)
	        return phy_dev->link;
	}

	return 0;
}

__weak void xrdp_eth_init(void)
{
#ifdef CONFIG_BCMBCA_IKOS
	int i;

	for (i = 0; i < dt_get_ports_num(); i++) {
		mac_dev_t *mac_dev = dt_get_mac_dev(i);
		phy_dev_t *phy_dev = dt_get_phy_dev(i);

		if (!phy_dev)
			continue;

		phy_dev->link = 1;
		phy_dev->speed = PHY_SPEED_1000;
		phy_dev->duplex = PHY_DUPLEX_FULL;
		phy_dev->pause_rx = 0;
		phy_dev->pause_tx = 0;
		mac_set_cfg_by_phy(phy_dev, mac_dev);
	}
	active_port = 0;
#else
	register_cli_job_cb(1000, phy_poll);
#endif
}

static int xrdp_eth_inited = 0;
void bcmbca_xrdp_eth_init(void)
{
	if (xrdp_eth_inited)
		return;

	printf("XRDP Ethernet driver initialize\n");
	xrdp_eth_inited++;

	unregister_cli_job_cb(bcmbca_xrdp_eth_init);
	if (!_net_init())
		xrdp_eth_init();

#ifdef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
	env_active_port_init();
#endif
}

void bcmbca_xrdp_eth_uninit(void)
{
	printf("XRDP Ethernet driver un-initialize\n");

	_net_uninit();
}

extern int mac_proc_cmd_stats(int id);
extern int phy_cmd_list(void);
extern int mac_cmd_list(void);
extern int mac_devices_internal_index(mac_dev_t *mac_dev);
extern int phy_devices_internal_index(phy_dev_t *phy_dev);

static int bcmbca_xrdp_eth_phy_status(void)
{
	int i;

	printf("|================================================|\n");
	printf("| Port  |          Name          |  MAC  |  PHY  |\n");
	printf("|================================================|\n");

	for (i = 0; i < dt_get_ports_num(); i++) {
		char *port_name = dt_get_port_name(i);
		mac_dev_t *mac_dev = dt_get_mac_dev(i);
		phy_dev_t *phy_dev = dt_get_phy_dev(i);
        int mac_index = -1, phy_index = -1;

        if (mac_dev)
            mac_index = mac_devices_internal_index(mac_dev);
        if (phy_dev)
            phy_index = phy_devices_internal_index(phy_dev);

        if (!mac_dev && !phy_dev)
            continue;

		printf("| %s ", i == active_port ? "*" : " ");
		printf("%2d  ", i);
		printf("| %-21s  ", port_name);
        if (mac_index >= 0)
            printf("|   %2d  ", mac_index);
        else
            printf("|       ");
        if (phy_index >= 0)
            printf("|   %2d  ", phy_index);
        else
            printf("|       ");
		printf("|\n");
	}

	printf("|================================================|\n");

	return 0;
}
#define atoi(s) ((int)(strtoul((s), NULL, 10)))
int do_eth_print_status(cmd_tbl_t *cmdtp, int flag, int argc,
	char *const argv[])
{
	char *cmd = NULL;

	if (argc >= 2)
		cmd = argv[1];

	if (argc > 3)
		return cmd_usage(cmdtp);

    if (cmd && !strcmp(cmd, "stats") && argc >= 3)
    {
        mac_proc_cmd_stats(atoi(argv[2]));
        return 0;
    }

    bcmbca_xrdp_eth_phy_status();
    printf("\n");

	if (!cmd || !strcmp(cmd, "phy"))
    {
        phy_cmd_list();
        printf("\n");
    }

	if (!cmd || !strcmp(cmd, "mac"))
    {
        mac_cmd_list();
        printf("\n");
    }

    return 0;
}

U_BOOT_CMD(
		xrdpeth_status, 7, 0, do_eth_print_status,
		"Broadcom BCA XRDP eth controller management",
		"xrdp_status [type]\n"
		" - Print the XRDP Ethernet network driver status tables\n"
		" - Argument can be phy or mac. If no argument provided,"
		"both tables will be printed\n"
		);

static int get_ethaddr(struct udevice *dev)
{
	uchar v_mac[ETH_ALEN] = {};

	if (eth_env_get_enetaddr("ethaddr", v_mac)) {
		eth_env_set_enetaddr_by_index("eth", dev->seq, v_mac);
		printf("Using MAC Address %pM\n", v_mac);
	}

	return 0;
}

static int bcmbca_eth_probe(struct udevice *dev)
{
	get_ethaddr(dev);

	return 0;
}

static int bcmbca_eth_start(struct udevice *dev)
{
#ifdef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
    if (!xrdp_eth_inited) {
        int loop;
        
        bcmbca_xrdp_eth_init();
        // poll phy up to 10 seconds to support uboot automated upgrade
        for (loop=0; loop < 100; loop++)
        {
            udelay(100000);  //100msec
            phy_poll();
            if (active_port >= 0)
                break;
        }
    }
#else
	uint32_t phyid;
	phy_dev_t *phy_dev;

	active_port = dev->seq;

	phy_dev = dt_get_phy_dev(active_port);
	if (!phy_dev) {
		netdev_err(port->dev, "cannot connect to phy\n");
		return -ENODEV;
	}

	phy_dev_phyid_get(phy_dev, &phyid);

	if (!phy_dev->link) {
		printf("%s: No link\n", phy_dev->phy_drv->name);
		return -1;
	}
#endif

	return 0;
}

static void bcmbca_eth_stop(struct udevice *dev)
{
	return;
}

static int bcmbca_eth_remove(struct udevice *dev)
{
	return 0;
}

static int bcmbca_eth_send(struct udevice *dev, void *buffer, int length)
{
	int rc;
	uint8_t txbuf[ETH_ZLEN];
	void *bufp = buffer;
	int reg = dt_get_port_reg(active_port);

	if (reg < 0)
	{
		printf("%s:Failed to send packet len %d: no active_port set\n",
			__func__, length);
		return -1;
	}

	if (length < ETH_ZLEN) {
		memcpy(txbuf, buffer, length);
		memset(txbuf + length, 0, ETH_ZLEN - length);
		bufp = txbuf;
	}

	rc = bcmbca_xrdp_send(bufp, length, reg);
	if (rc) {
		printf("%s:Failed to send packet len %d: error = %d\n",
			__func__, length, rc);
	}

	return 0;
}

static int bcmbca_eth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	uint16_t length = 0;
	uint8_t rx_port;
	int reg = dt_get_port_reg(active_port);

	*packetp = net_rx_packets[0];

	if(bcmbca_xrdp_recv(packetp, &length, &rx_port))
		return -EAGAIN;

	if (rx_port != reg)
		return -EAGAIN;

	return length;
}

static const struct eth_ops bcmbca_eth_ops = {
	.start = bcmbca_eth_start,
	.send = bcmbca_eth_send,
	.recv = bcmbca_eth_recv,
	.stop = bcmbca_eth_stop,
};

static const struct udevice_id xrdp_ids[] = {
	{
		.compatible = "brcm,enet",
		.data = 0,
	},
	{ }
};

#ifndef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
static struct driver xrdp_driver = {
#else
U_BOOT_DRIVER(xrdp_base) = {
#endif
	.name	= "xrdp_eth",
	.id	= UCLASS_ETH,
	.probe	= bcmbca_eth_probe,
	.remove = bcmbca_eth_remove,
	.ops	= &bcmbca_eth_ops,
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
#ifdef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
	.of_match = xrdp_ids,
#endif
};

#ifndef CONFIG_BCMBCA_XRDP_ETH_SWITCH_IFACE
static int xrdp_base_bind(struct udevice *parent)
{
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(parent);
	struct uclass_driver *drv;
	struct udevice *dev;
	struct eth_pdata *plat;
	int subnode;
	char *name;
	u32 id;

	/* Lookup eth driver */
	drv = lists_uclass_lookup(UCLASS_ETH);
	if (!drv) {
		puts("Cannot find eth driver\n");
		return -ENOENT;
	}

	node = fdt_first_subnode(blob, node);
	fdt_for_each_subnode(subnode, blob, node) {
		/* Skip disabled ports */
		if (!fdtdec_get_is_enabled(blob, subnode))
			continue;

		plat = calloc(1, sizeof(*plat));
		if (!plat)
			return -ENOMEM;

		id = fdtdec_get_int(blob, subnode, "reg", -1);

		name = calloc(1, 16);
		if (!name) {
			free(plat);
			return -ENOMEM;
		}
		sprintf(name, "xrdp-eth-%d", id);

		/* Create child device UCLASS_ETH and bind it */
		device_bind(parent, &xrdp_driver, name, plat, subnode, &dev);
		dev_set_of_offset(dev, subnode);
	}

	return 0;
}

U_BOOT_DRIVER(xrdp_base) = {
	.name	= "xrdp_base",
	.id	= UCLASS_MISC,
	.of_match = xrdp_ids,
	.bind	= xrdp_base_bind,
};
#endif
