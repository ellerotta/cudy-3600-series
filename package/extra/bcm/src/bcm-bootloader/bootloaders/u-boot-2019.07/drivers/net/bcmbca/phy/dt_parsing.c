// SPDX-License-Identifier: GPL-2.0+
/*
   Copyright (c) 2019 Broadcom Corporation
   All Rights Reserved


*/

/*
 *  Created on: Oct 2019
 *      Author: ido.brezel@broadcom.com
 */

#include "phy_drv.h"
#include "mac_drv.h"
#include "brcm_rgmii.h"
#include "dt_access.h"

#include <dm/device.h>
#include <dm/uclass.h>
#if defined(CONFIG_BCM_BCA_LED)
#include "bcm_bca_leds.h"
#endif

extern phy_drv_t *phy_drivers[PHY_TYPE_MAX];

static phy_type_t phy_type_get_by_str(const char *phy_type_str)
{
    phy_drv_t *phy_drv;
    int i;

    for (i = 0; i < PHY_TYPE_MAX; i++)
    {
        phy_drv = phy_drivers[i];
        if (phy_drv && !strcmp(phy_drv->name, phy_type_str))
            return i;
    }

    return PHY_TYPE_UNKNOWN;
}

static int phy_drv_dt_priv(phy_type_t phy_type, const dt_handle_t handle, uint32_t addr, uint32_t phy_mode, void **_priv)
{
    phy_drv_t *phy_drv = phy_drivers[phy_type];

    if (!phy_drv)
    {
        printk("Failed to find phy driver: phy_type=%d\n", phy_type);
        return -1;
    }

    if (!phy_drv->dt_priv)
        return 0;

    return phy_drv->dt_priv(handle, addr, phy_mode, _priv);
}

static int parse_caps_mask(phy_mii_type_t mii_type)
{
#if defined(DSL_DEVICES)  /* caps mask does not apply to DSL platform, which uses static private array for each type */
    return PHY_CAP_ALL;
#else
    int caps_mask = PHY_CAP_ALL;

    if (mii_type < PHY_MII_TYPE_XFI)
        caps_mask &= ~(PHY_CAP_10000 | PHY_CAP_5000);

    if (mii_type < PHY_MII_TYPE_HSGMII)
        caps_mask &= ~PHY_CAP_2500;

    if (mii_type < PHY_MII_TYPE_GMII)
        caps_mask &= ~(PHY_CAP_1000_HALF | PHY_CAP_1000_FULL);

    return caps_mask;
#endif
}

static int parse_inter_phy_types(const dt_handle_t handle)
{
    int inter_phy_types = 0;

#if !defined(DSL_DEVICES)
    if (dt_property_read_bool(handle, "1000-Base-X"))
        inter_phy_types |= INTER_PHY_TYPE_1GBASE_X_M;
    if (dt_property_read_bool(handle, "1000-Base-R"))
        inter_phy_types |= INTER_PHY_TYPE_1GBASE_R_M;
    if (dt_property_read_bool(handle, "2500-Base-X"))
        inter_phy_types |= INTER_PHY_TYPE_2P5GBASE_X_M;
    if (dt_property_read_bool(handle, "2500-Base-R"))
        inter_phy_types |= INTER_PHY_TYPE_2P5GBASE_R_M;
    if (dt_property_read_bool(handle, "5000-Base-X"))
        inter_phy_types |= INTER_PHY_TYPE_5GBASE_X_M;
    if (dt_property_read_bool(handle, "5000-Base-R"))
        inter_phy_types |= INTER_PHY_TYPE_5GBASE_R_M;
    if (dt_property_read_bool(handle, "10000-Base-X"))
        inter_phy_types |= INTER_PHY_TYPE_10GBASE_X_M;
    if (dt_property_read_bool(handle, "10000-Base-R"))
        inter_phy_types |= INTER_PHY_TYPE_10GBASE_R_M;
    if (dt_property_read_bool(handle, "USXGMII-S"))
        inter_phy_types |= INTER_PHY_TYPE_USXGMII_M;
    if (dt_property_read_bool(handle, "USXGMII-M"))
        inter_phy_types |= INTER_PHY_TYPE_USXGMII_MP_M;

#if defined(CONFIG_BCM96858)
    inter_phy_types &= ~INTER_PHY_TYPE_5GBASE_X_M;
    inter_phy_types |= INTER_PHY_TYPE_5GIDLE_M;
#endif
#endif

    return inter_phy_types;
}

static int parse_usxgmii_parameters(phy_dev_t *phy_dev, const dt_handle_t handle)
{
#if defined(DSL_DEVICES)
    int m_type = USXGMII_S;
    const char *m_type_str = dt_property_read_string(handle, "usxgmii-m-type");

    if (m_type_str == NULL)
        return 0;

    for (m_type = USXGMII_M_MAX-1; m_type > USXGMII_S; m_type--)
        if (!strcasecmp(m_type_str, usxgmii_m_type_strs[m_type]))
            break;
    phy_dev->usxgmii_m_type = m_type;
    phy_dev->usxgmii_m_index = dt_property_read_u32(handle, "mphy-index");
    phy_dev->addr = mphy_dev_true_addr(phy_dev);
#else
    if (phy_dev->inter_phy_types & INTER_PHY_TYPE_USXGMII_M)
        phy_dev->usxgmii_m_type = USXGMII_S;
    if (phy_dev->inter_phy_types & INTER_PHY_TYPE_USXGMII_MP_M)
        phy_dev->usxgmii_m_type = USXGMII_M_10G_Q;
#endif

    return 0;
}

static int parse_serdes_parameters(phy_dev_t *phy_dev, const dt_handle_t handle)
{
    phy_dev->core_index = dt_property_read_u32_default(handle, "serdes-core", 0);
    phy_dev->lane_index = dt_property_read_u32_default(handle, "serdes-lane", 0);
    phy_dev->inter_phy_types = parse_inter_phy_types(handle);
    parse_usxgmii_parameters(phy_dev, handle);

    return 0;
}

static int parse_gpio_parameters(phy_dev_t *phy_dev, const dt_handle_t handle)
{
    int ret = 0;

    ret |= dt_gpio_request_by_name(handle, "phy-power", 0, "PHY power", &phy_dev->gpiod_phy_power, 0);
    ret |= dt_gpio_request_by_name(handle, "phy-reset", 0, "PHY reset", &phy_dev->gpiod_phy_reset, 1);
    ret |= dt_gpio_request_by_name(handle, "tx-disable", 0, "TX disable", &phy_dev->gpiod_tx_disable, 1);

    return ret;
}

phy_mii_type_t dt_get_phy_mode_mii_type(const dt_handle_t handle)
{
    int i;
    const char *phy_mode_str = dt_property_read_string(handle, "phy-mode");

    if (!phy_mode_str)
        goto Exit;

    for (i = PHY_MII_TYPE_UNKNOWN; i < PHY_MII_TYPE_LAST; i++)
    {
        if (!strcasecmp(phy_mode_str, phy_dev_mii_type_to_str(i)))
            return i;
    }

Exit:
    return PHY_MII_TYPE_UNKNOWN;
}
EXPORT_SYMBOL(dt_get_phy_mode_mii_type);

bus_drv_t *bus_drv_get_by_str(const char *bus_type_str);

static phy_dev_t *phy_dev_dt_probe(const dt_handle_t handle)
{
    phy_dev_t *phy_dev = NULL, *phy_dev_next = NULL;
    void *priv = NULL;
    uint32_t addr;
    const phy_type_t phy_type = phy_type_get_by_str(dt_property_read_string(handle, "phy-type"));
    dt_handle_t phy;

    if (phy_type == PHY_TYPE_UNKNOWN)
    {
#if !defined(CONFIG_BCM94908)
        printk("Missing or wrong phy-type entry %s\n", dt_property_read_string(handle, "phy-type"));
#endif        
        goto Exit;
    }

    addr = dt_property_read_u32(handle, "reg");
    if (addr == (uint32_t)(-1))
    {
        printk("Missing reg entry\n");
        goto Exit;
    }

    phy = dt_parse_phandle(handle, "phy-handle", 0);
    if (dt_is_valid(phy))
    {
        if (!(phy_dev_next = phy_drv_find_device(phy)))
        {
            printk("phy_dev probe_defer: %s waiting for phy %s\n", dt_get_name(handle), dt_get_name(phy));
            return ERR_PTR(-EPROBE_DEFER);
        }
    }

    /* XXX: phy_drv_dt_priv API can probably be depricated after removing BP support. Should be
     * called as part of probe, but no need to pass priv outside of phy_drv implementation */
    if ((phy_drv_dt_priv(phy_type, handle, addr, 0, &priv)) == -EPROBE_DEFER)
        return ERR_PTR(-EPROBE_DEFER);

    if (phy_type == PHY_TYPE_CROSSBAR)
    {
        phy_dev = (phy_dev_t *)priv;
        goto Done;
    }

    phy_dev = phy_dev_add(phy_type, addr, priv);
    if (!phy_dev)
    {
        printk("Failed to create phy device: %d:%d\n", phy_type, addr);
        goto Exit;
    }

    phy_dev->bus_drv = bus_drv_get_by_str(dt_property_read_string(dt_parent(handle), "bus-type"));
    phy_dev->swap_pair = dt_property_read_bool(handle, "enet-phy-lane-swap");
    phy_dev->xfi_tx_polarity_inverse = dt_property_read_bool(handle, "phy-xfi-tx-polarity-inverse");
    phy_dev->xfi_rx_polarity_inverse = dt_property_read_bool(handle, "phy-xfi-rx-polarity-inverse");
    phy_dev->flag |= dt_property_read_bool(handle, "phy-external") ? PHY_FLAG_EXTPHY : 0;
    phy_dev->flag |= dt_property_read_bool(handle, "phy-extswitch") ? PHY_FLAG_TO_EXTSW : 0;
    phy_dev->flag |= dt_property_read_bool(handle, "phy-fixed") ? PHY_FLAG_FIXED_CONN: 0;
    phy_dev->flag |= dt_property_read_bool(handle, "on-mezzanine") ? PHY_FLAG_ON_MEZZANINE: 0;
    phy_dev->flag |= dt_property_read_bool(handle, "wake-on-lan") ? PHY_FLAG_WAKE_ON_LAN : 0;
    phy_dev->flag |= dt_property_read_bool(handle, "force-2p5g-10gvco") ? PHY_FLAG_FORCE_2P5G_10GVCO: 0;

    if ((phy_dev->shared_ref_clk_mhz = dt_property_read_u32_default(handle, "shared-ref-clk-mhz-bootstrap", 0)))
        phy_dev->flag |= PHY_FLAG_SHARED_CLOCK_BOOTSTRAP;
    else 
    	phy_dev->shared_ref_clk_mhz = dt_property_read_u32_default(handle, "shared-ref-clk-mhz", 0);

    parse_gpio_parameters(phy_dev, handle);
    parse_serdes_parameters(phy_dev, handle);

    phy_dev->caps_mask = PHY_CAP_ALL;
    if (dt_property_read_bool(handle, "caps-no-hdx"))
        phy_dev->caps_mask &= ~(PHY_CAP_10_HALF | PHY_CAP_100_HALF | PHY_CAP_1000_HALF);
    if (dt_property_read_bool(handle, "caps-no-10000"))
        phy_dev->caps_mask &= ~(PHY_CAP_10000);
    if (dt_property_read_bool(handle, "caps-no-5000"))
        phy_dev->caps_mask &= ~(PHY_CAP_5000);
    if (dt_property_read_bool(handle, "caps-no-2500"))
        phy_dev->caps_mask &= ~(PHY_CAP_2500);
    if (dt_property_read_bool(handle, "caps-no-1000"))
        phy_dev->caps_mask &= ~(PHY_CAP_1000_HALF | PHY_CAP_1000_FULL);
    if (dt_property_read_bool(handle, "caps-no-100"))
        phy_dev->caps_mask &= ~(PHY_CAP_100_HALF | PHY_CAP_100_FULL);
    if (dt_property_read_bool(handle, "caps-no-10"))
        phy_dev->caps_mask &= ~(PHY_CAP_10_HALF | PHY_CAP_10_FULL);

    if (phy_drv_dev_add(phy_dev))
    {
        printk("Failed to add phy device to the driver: %s:%d\n", phy_dev->phy_drv->name, addr);
        phy_dev_del(phy_dev);
        phy_dev = NULL;
        goto Exit;
    }

    if (phy_dev_next)
    {
        phy_dev_next->cascade_prev = phy_dev;
        phy_dev->cascade_next = phy_dev_next;
    }

Done:
    printk("Registered phy device: %s:0x%x\n", phy_dev->phy_drv->name, addr);

Exit:
    return phy_dev;
}

void phy_dev_attach(phy_dev_t *phy_dev, phy_mii_type_t mii_type, int delay_rx, int delay_tx, int instance)
{
    if (phy_dev->phy_drv->phy_type == PHY_TYPE_CROSSBAR)
        return;

    phy_dev->delay_rx = delay_rx;
    phy_dev->delay_tx = delay_tx;
    if (mii_type == PHY_MII_TYPE_RGMII)
        phy_dev->core_index = instance;

    phy_dev->mii_type = mii_type;
    phy_dev->caps_mask &= parse_caps_mask(phy_dev->mii_type);

    if (phy_dev->cascade_next)
        phy_dev_attach(phy_dev->cascade_next, mii_type, delay_rx, delay_tx, instance);
}
EXPORT_SYMBOL(phy_dev_attach);

extern mac_drv_t *mac_drivers[MAC_TYPE_MAX];

static mac_type_t dt_get_mac_type(const dt_handle_t handle)
{
    int i;
    mac_drv_t *mac_drv;
    const char *mac_type_str = dt_property_read_string(handle, "mac-type");

    if (!mac_type_str)
        goto Exit;

    for (i = 0; i < MAC_TYPE_MAX; i++)
    {
        mac_drv = mac_drivers[i];
        if (mac_drv && !strcasecmp(mac_drv->name, mac_type_str))
            return i;
    }

Exit:
    return MAC_TYPE_UNKNOWN;
}

static int mac_drv_dt_priv(mac_type_t mac_type, const dt_handle_t handle, int mac_id, void **priv)
{
    mac_drv_t *mac_drv;

    if (!(mac_drv = mac_drivers[mac_type]))
    {
        printk("Failed to find MAC driver: mac_type=%d\n", mac_type);
        return -1;
    }

    if (!mac_drv->dt_priv)
        return 0;

    return mac_drv->dt_priv(handle, mac_id, priv);
}

mac_dev_t *mac_dev_dt_probe(const dt_handle_t handle, uint32_t port_index)
{
    int ret;
    void *priv;
    mac_dev_t *mac_dev = NULL;
    const mac_type_t mac_type = dt_get_mac_type(handle);
    unsigned long mac_index;

    if (mac_type == MAC_TYPE_UNKNOWN)
        goto Exit;

    /* Fallback to default mac_id == <reg> */
    mac_index = dt_property_read_u32_default(handle, "mac-index", port_index);

    ret = mac_drv_dt_priv(mac_type, handle, mac_index, &priv);
    if (ret)
        goto Exit;

    mac_dev = mac_dev_add(mac_type, mac_index, priv);
    if (!mac_dev)
    {
        printk("Failed to create mac device: %d:%d\n", mac_type, port_index);
        goto Exit;
    }

    printk("Registered mac device: %s:0x%x\n", mac_dev->mac_drv->name, port_index);

Exit:
    return mac_dev;
}
EXPORT_SYMBOL(mac_dev_dt_probe);

#define DT_NUM_OF_PORTS_PER_SWITCH  9
#define DT_NUM_OF_SWITCHES          2
#define DT_NUM_OF_PORTS (DT_NUM_OF_SWITCHES * DT_NUM_OF_PORTS_PER_SWITCH)
static const char *port_names[DT_NUM_OF_PORTS];
static uint32_t port_regs[DT_NUM_OF_PORTS];
static phy_dev_t *phy_devices[DT_NUM_OF_PORTS];
static mac_dev_t *mac_devices[DT_NUM_OF_PORTS];
#if defined(CONFIG_BCM_BCA_LED)
static bca_leds_info_t port_leds_info[DT_NUM_OF_PORTS];
#endif

int dt_get_port_reg(int port_index)
{
    if (port_index < 0)
        return -1;

    return port_regs[port_index];
}

uint32_t dt_get_ports_num(void)
{
    return DT_NUM_OF_PORTS;
}

const char *dt_get_port_name(uint32_t port_index)
{
    if (port_index >= DT_NUM_OF_PORTS)
        return NULL;

    return port_names[port_index];
}

phy_dev_t *dt_get_phy_dev(uint32_t port_index)
{
    if (port_index >= DT_NUM_OF_PORTS)
        return NULL;

    return phy_devices[port_index];
}

mac_dev_t *dt_get_mac_dev(uint32_t port_index)
{
    if (port_index >= DT_NUM_OF_PORTS)
        return NULL;

    return mac_devices[port_index];
}

static int port_add(uint32_t port_index, const char *port_name, phy_dev_t *phy_dev, mac_dev_t *mac_dev, uint32_t reg)
{
    port_names[port_index] = port_name;
    phy_devices[port_index] = phy_dev;
    mac_devices[port_index] = mac_dev;
    port_regs[port_index] = reg;

    return 0;
}

#define RGMII_PINS_MAX 12

/* TODO: read from dts */
#if defined(CONFIG_BCM96846)
#define RGMII_PIN 42
#elif defined(CONFIG_BCM96856)
#define RGMII_PIN 48
#elif defined(CONFIG_BCM96878)
#define RGMII_PIN 40
#elif defined(CONFIG_BCM96855)
#define RGMII_PIN 56
#elif defined(CONFIG_BCM963146)
#define RGMII_PIN 53
#elif defined(CONFIG_BCM94912)
#define RGMII_PIN 42
#elif defined(CONFIG_BCM96756)
#define RGMII_PIN 56
#elif defined(CONFIG_BCM963178)
#define RGMII_PIN 60
#elif defined(CONFIG_BCM947622)
#define RGMII_PIN 56
#elif defined(CONFIG_BCM94908)
#define RGMII_PIN 68
#elif defined(CONFIG_BCM96888)
#define RGMII_PIN 47
#else
#define RGMII_PIN 0
#endif

static int mii_pins_get(dt_handle_t port, int pins[], int *num_pins)
{
    int i;

    for (i = 0; i < RGMII_PINS_MAX; i++)
        pins[i] = RGMII_PIN + i;

    return 0;
}

static int sw_probe(dt_handle_t sw, int unit);

#if defined(CONFIG_BCM94908)
extern phy_dev_t *crossbar_get_phy(int unit, int port);
#endif

static int dt_port_probe(dt_handle_t sw, dt_handle_t port, int unit)
{
    dt_handle_t phy;
    uint32_t reg;
    static uint32_t port_index = 0;
    rgmii_params params = {};
    phy_mii_type_t mii_type;
    phy_dev_t *phy_dev = NULL;
    mac_dev_t *mac_dev = NULL;
    const char *dt_name_port = dt_get_name(port);
#if defined(CONFIG_BCM_BCA_LED)
    bca_leds_info_t *led_info;
#endif

    reg = dt_property_read_u32(port, "reg");
    if (reg == (uint32_t)(-1))
    {
        printk("Missing reg entry for port %s\n", dt_name_port);
        return -1;
    }

#if defined(CONFIG_BCM94908)
    if (unit == 0) { // bypass ports on runner
        dt_handle_t link;
        link = dt_parse_phandle(port, "link", 0);
        if (dt_is_valid(link))
        {
            return sw_probe(link, unit+1);
        }
        return 0;
    }
#endif

    mii_type = dt_get_phy_mode_mii_type(port);
    params.delay_rx = dt_property_read_bool(port, "rx-delay");
    params.delay_tx = dt_property_read_bool(port, "tx-delay");
    params.is_1p8v = dt_property_read_bool(port, "rgmii-1p8v");
    params.is_3p3v = dt_property_read_bool(port, "rgmii-3p3v");
    params.is_disabled = dt_property_read_bool(port, "rgmii-disabled");

    if (mii_type == PHY_MII_TYPE_RGMII)
    {
        int rgmii_pins[RGMII_PINS_MAX], num_pins = RGMII_PINS_MAX;

        params.instance = dt_property_read_u32_default(port, "rgmii-intf", 0);

        if (mii_pins_get(port, rgmii_pins, &num_pins))
            num_pins = 0;

        params.pins = rgmii_pins;
        params.num_pins = num_pins;

        rgmii_attach(&params);
    }

    phy = dt_parse_phandle(port, "phy-handle", 0);
    if (dt_is_valid(phy))
    {
        dt_handle_t link;

        if (!(phy_dev = phy_drv_find_device(phy)))
        {
#if defined(CONFIG_BCM94908)
            phy_dev = crossbar_get_phy(unit, reg);
#endif
            if (!phy_dev) {
                printk("Failed to find phy for port %s (%s)\n", dt_name_port, dt_get_name(phy));
                return -1;
            }
        }

        phy_dev_attach(phy_dev, mii_type, !params.delay_rx, !params.delay_tx, params.instance);

        link = dt_parse_phandle(port, "link", 0);
        if (dt_is_valid(link))
        {
            phy_dev->flag |= PHY_FLAG_TO_EXTSW;
            sw_probe(link, unit+1);
        }
    }

    if (!(mac_dev = mac_dev_dt_probe(port, reg)))
    {
        printk("Failed to create mac for port %s\n", dt_name_port);
        return -1;
    }

    if (port_add(port_index, dt_name_port, cascade_phy_get_last(phy_dev), mac_dev, reg))
    {
        printk("Failed to create port %s\n", dt_name_port);
        return -1;
    }

#if defined(CONFIG_BCM_BCA_LED)
    led_info = &port_leds_info[port_index];
    led_info->sw_id = unit;
    led_info->port_id = 0xff;
    bca_led_request_network_leds(port, led_info);
#endif

    port_index++;

    return 0;
}

static int sw_probe(dt_handle_t sw, int unit)
{
    int ret = 0;
    dt_handle_t child, port;
    const char *sw_type_str = dt_property_read_string(sw, "sw-type");

    if (!sw_type_str || (strcasecmp(sw_type_str, "RUNNER_SW") && strcasecmp(sw_type_str, "SF2_SW") && strcasecmp(sw_type_str, "SYSP_SW")))
    {
        printk("Unsupported switch type: %s\n", sw_type_str);
        return -1;
    }

    dt_for_each_child_of_node(sw, child)
    {
        if (!dt_is_available(child))
            continue;

        dt_for_each_child_of_node(child, port)
        {
            if (!dt_is_available(port))
                continue;

            ret |= dt_port_probe(sw, port, unit);
        }
    }

    return 0;
}

int dt_probe(void)
{
    int i;
    struct udevice *dev;
    dt_handle_t root_switch;

    root_switch = dt_find_compatible_node(ofnode_null(), "brcm,enet");
    if (!dt_is_valid(root_switch))
    {
        printk("Failed to find the root switch node\n");
        return -1;
    }

    printk("\nFound root switch, looking for ports:\n");

    mac_drivers_set();
    phy_drivers_set();

    for (uclass_first_device_check(UCLASS_MISC, &dev); dev; uclass_next_device_check(&dev));

    if (sw_probe(root_switch, 0))
    {
        printk("Failed to probe device tree ports\n");
        return -1;
    }

    mac_drivers_init();
    phy_drivers_init();

    for (i = 0; i < dt_get_ports_num(); i++)
    {
        mac_dev_t **mac_dev = &mac_devices[i];
        phy_dev_t **phy_dev = &phy_devices[i];

        if (*mac_dev)
        {
            mac_dev_init(*mac_dev);
            if (!*phy_dev)
                mac_dev_enable(*mac_dev);
        }

        if (*phy_dev)
        {
            if ((*phy_dev)->flag & PHY_FLAG_ON_MEZZANINE &&
                (*phy_dev)->flag & PHY_FLAG_NOT_PRESENTED)
            {
                *phy_dev = NULL;
                printk("mezzanine card not present for port: %d !!\n", i);
                continue;
            }
            if (phy_dev_init(*phy_dev))
            {
                phy_dev_del(*phy_dev);
                *phy_dev = NULL;
                printk("Failed to initialize phy for port: %d. ignoring...\n", i);
                continue;
            }

            phy_dev_power_set(*phy_dev, 1);

#if defined(CONFIG_BCM_BCA_LED)
            phy_dev_leds_init(*phy_dev, &port_leds_info[i]);
#endif
        }
    }

    return 0;
}

static int bcmbca_phy_probe(struct udevice *udev)
{
    ofnode node = udev->node;
    phy_dev_t *phy_dev;

    if (!ofnode_valid(node))
        return -ENODEV;

    phy_dev = phy_dev_dt_probe(node);
    if (phy_dev == ERR_PTR(-EPROBE_DEFER))
        return -EPROBE_DEFER;
    if (!phy_dev)
        return -EINVAL;

    phy_dev->dt_handle = node;

    return 0;
}

static const struct udevice_id bcmbca_phy_ids[] = {
    { .compatible = "brcm,bcaphy" },
    { /* end of list */ },
};

U_BOOT_DRIVER(bcmbca_phy) = {
    .name	= "bcmbca_phy",
    .id	= UCLASS_MISC,
    .of_match = bcmbca_phy_ids,
    .probe = bcmbca_phy_probe,
};
