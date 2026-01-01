/***********************************************************************
 *
 *  Copyright (c) 2023  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2023:proprietary:standard

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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "fpi_ioctl.h"
#include "fpictl_api.h"

const char *flow_mode_name[] = {
    "L2",
    "L3 + L4",
    NULL
};

const char *esp_mode_name[] = {
    "Ignored",
    "ESP in IP",
    "ESP in UDP",
    NULL
};

const char *gre_flow_mode_name[] = {
    "Standard",
    "Proprietary",
    NULL
};

const char *l2hdr_mode_name[] = {
    "disabled",
    "14 bytes",
    "18 bytes",
    "22 bytes",
    NULL
};

/*
 *------------------------------------------------------------------------------
 * Function Name: fpictl_open
 * Description  : Opens the Flow Provisioning Interface device.
 * Returns      : device handle if successsful or -1 if error
 *------------------------------------------------------------------------------
 */
static int fpictl_open(void)
{
    int nFd = open(FPI_DRV_DEV_NAME, O_RDWR);
    if (nFd == -1) {
        fprintf(stderr, "open <%s> error no %d\n",
                FPI_DEV_NAME, errno);
        return -1;
    }
    return nFd;
} /* fpictl_open */

/*
 *------------------------------------------------------------------------------
 * Function Name: fpictl_ioctl
 * Description  : Ioctls into Flow Provisioning driver passing the IOCTL command,
 * Returns      : 0 - success, non-0 - error
 *------------------------------------------------------------------------------
 */
static int fpictl_ioctl(fpictl_ioctl_t ioctl_cmd, void *arg)
{
    int devFd, ret = -1;

    if ((devFd = fpictl_open()) == -1)
        return -1;

    if ((ret = ioctl(devFd, ioctl_cmd, (uintptr_t)arg)) == -1)
        fprintf(stderr, "fpictl_ioctl <%d> error\n", ioctl_cmd);

    close(devFd);
    return ret;
}

int bcm_fpictl_set_mode(fpi_mode_t mode)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_mode;
    fpidata.op = fpictl_op_set;
    fpidata.mode = mode;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_get_mode(fpi_mode_t *pMode)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_mode;
    fpidata.op = fpictl_op_get;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    *pMode = fpidata.mode;
    return err;
}

int bcm_fpictl_set_default_priority(uint8_t prio)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_def_prio;
    fpidata.op = fpictl_op_set;
    fpidata.def_prio = prio;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_get_default_priority(uint8_t *pPrio)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_def_prio;
    fpidata.op = fpictl_op_get;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    *pPrio = fpidata.def_prio;
    return err;
}

int bcm_fpictl_add_flow(fpictl_data_t *fpi)
{
    int err = 0;

    if (fpi == NULL)
        return -1;

    fpi->subsys = fpictl_subsys_flow;
    fpi->op = fpictl_op_add;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)fpi))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_delete_flow_by_handle(uint32_t handle)
{
    fpictl_data_t fpidata;
    int err = 0;

    memset(&fpidata, 0, sizeof(fpidata));
    fpidata.subsys = fpictl_subsys_flow;
    fpidata.op = fpictl_op_del_by_handle;
    fpidata.handle = handle;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }

    if (fpidata.rc != 0) {
        fprintf(stderr, "failed to delete the flow. %s (rc=%d)!\n",
                bcm_fpictl_err_message(fpidata.rc), fpidata.rc);
        return fpidata.rc;
    }

    return err;
}

int bcm_fpictl_delete_flow_by_key(fpictl_data_t *fpi)
{
    int err = 0;

    if (fpi == NULL)
        return -1;

    fpi->subsys = fpictl_subsys_flow;
    fpi->op = fpictl_op_del_by_key;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)fpi))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }

    if (fpi->rc != 0) {
        fprintf(stderr, "failed to delete the flow. %s (rc=%d)!\n",
                bcm_fpictl_err_message(fpi->rc), fpi->rc);
        return fpi->rc;
    }

    return err;
}

int bcm_fpictl_get_flow(uint32_t handle, fpictl_data_t *fpi)
{
    int err = 0;

    if (fpi == NULL)
        return -1;

    fpi->subsys = fpictl_subsys_flow;
    fpi->op = fpictl_op_get;
    fpi->handle = handle;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)fpi))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

static void bcm_fpictl_print_flow(fpictl_data_t *fpi)
{
    int is_ipv6 = 0, is_ipv4 = 0, dump_size, i;
    fpi_l2_key_t *l2_key;
    fpi_l3l4_key_t *l3l4_key = NULL;

    fprintf(stdout, "Flow: Handle#0x%x\n", fpi->handle);
    fprintf(stdout, "\tKey info:\tMode: %s (%d)\n",
            flow_mode_name[fpi->flow.key.mode], fpi->flow.key.mode);

    /* key */
    switch (fpi->flow.key.mode) {
    case fpi_mode_l2:
        l2_key = &fpi->flow.key.l2_key;

        fprintf(stdout, "\t\tIngress Device %s\n",
                l2_key->ingress_device_name);
        fprintf(stdout, "\t\tSRC MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                l2_key->src_mac[0], l2_key->src_mac[1],
                l2_key->src_mac[2], l2_key->src_mac[3],
                l2_key->src_mac[4], l2_key->src_mac[5]);
        fprintf(stdout, "\t\tDST MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                l2_key->dst_mac[0], l2_key->dst_mac[1],
                l2_key->dst_mac[2], l2_key->dst_mac[3],
                l2_key->dst_mac[4], l2_key->dst_mac[5]);
        fprintf(stdout, "\t\tEthernet Type 0x%04x\n", l2_key->eth_type);
        fprintf(stdout, "\t\tNumber of Vtag %d\n", l2_key->vtag_num);
        fprintf(stdout, "\t\tPacket Priority %d\n", l2_key->packet_priority);
        break;
    case fpi_mode_l3l4:
        l3l4_key = &fpi->flow.key.l3l4_key;

        fprintf(stdout, "\t\tIngress Device %s\n",
                l3l4_key->ingress_device_name);
        fprintf(stdout, "\t\tNumber of Vtag %d\n", l3l4_key->vtag_num);

        if (l3l4_key->is_ipv6 != 0) {
            fprintf(stdout, "\t\tIP Version 6\n");
            fprintf(stdout, "\t\tSRC IP %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
                    (l3l4_key->src_ip[0] >> 16), (l3l4_key->src_ip[0] & 0xffff),
                    (l3l4_key->src_ip[1] >> 16), (l3l4_key->src_ip[1] & 0xffff),
                    (l3l4_key->src_ip[2] >> 16), (l3l4_key->src_ip[2] & 0xffff),
                    (l3l4_key->src_ip[3] >> 16), (l3l4_key->src_ip[3] & 0xffff));
            fprintf(stdout, "\t\tDST IP %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
                    (l3l4_key->dst_ip[0] >> 16), (l3l4_key->dst_ip[0] & 0xffff),
                    (l3l4_key->dst_ip[1] >> 16), (l3l4_key->dst_ip[1] & 0xffff),
                    (l3l4_key->dst_ip[2] >> 16), (l3l4_key->dst_ip[2] & 0xffff),
                    (l3l4_key->dst_ip[3] >> 16), (l3l4_key->dst_ip[3] & 0xffff));
            is_ipv6 = 1;
        } else {
            fprintf(stdout, "\t\tIP Version 4\n");
            fprintf(stdout, "\t\tSRC IP %d.%d.%d.%d\n",
                    ((l3l4_key->src_ip[0] >> 24) & 0xff),
                    ((l3l4_key->src_ip[0] >> 16) & 0xff),
                    ((l3l4_key->src_ip[0] >> 8) & 0xff),
                    (l3l4_key->src_ip[0] & 0xff));
            fprintf(stdout, "\t\tDST IP %d.%d.%d.%d\n",
                    ((l3l4_key->dst_ip[0] >> 24) & 0xff),
                    ((l3l4_key->dst_ip[0] >> 16) & 0xff),
                    ((l3l4_key->dst_ip[0] >> 8) & 0xff),
                    (l3l4_key->dst_ip[0] & 0xff));
            is_ipv4 = 1;
        }

        fprintf(stdout, "\t\tL4 Proto %d\n", l3l4_key->l4_proto);
        fprintf(stdout, "\t\tSRC Port %d\n", l3l4_key->src_port);
        fprintf(stdout, "\t\tDST Port %d\n", l3l4_key->dst_port);

        fprintf(stdout, "\t\tESP SPI Mode %s (%d)\n",
                esp_mode_name[l3l4_key->esp_spi_mode], l3l4_key->esp_spi_mode);
        fprintf(stdout, "\t\tESP SPI 0x%08x\n", l3l4_key->esp_spi);
        fprintf(stdout, "\t\tPacket Priority %d\n", l3l4_key->packet_priority);
        break;
    default:
        return;
    }

    /* context */
    fprintf(stdout, "\tContext info:\n");

    if (fpi->flow.context.drop) {
        fprintf(stdout, "\t\tDrop is enabled.\n");

        if (fpi->flow.context.vtag_check != 0)
            fprintf(stdout, "\t\tVLAN Tag Check is enabled. VLAN Tag 0x%06x\n",
                    fpi->flow.context.vtag_value);
        else
            fprintf(stdout, "\t\tVLAN Tag Check is disabled.\n");

        return;
    }

    fprintf(stdout, "\t\tEgress Device %s\n",
            fpi->flow.context.egress_device_name);

    if (fpi->wl_info.wl_dst_type != FPI_WL_DST_TYPE_INVALID) {
        fprintf(stdout, "\t\t\tWLAN Interface User Priority %d\n",
                fpi->flow.context.wl_user_priority);
        fprintf(stdout, "\t\t\tGenerated Private Egress Info:\n");
        /* For WLAN types of egress interface */
        switch (fpi->wl_info.wl_dst_type) {
        case FPI_WL_DST_TYPE_WFD_NIC:
            fprintf(stdout, "\t\t\t\tWL DST Type WFD-NIC (%d)\n",
                    fpi->wl_info.wl_dst_type);
            fprintf(stdout, "\t\t\t\tWFD IDX %d\n", fpi->wl_info.radio_idx);
            fprintf(stdout, "\t\t\t\tWFD Key: Domain(%d), Endpoint(%d), "
                            "Incarnation(%d)\n",
                    fpi->wl_info.wfd_nic_key.domain,
                    fpi->wl_info.wfd_nic_key.endpoint,
                    fpi->wl_info.wfd_nic_key.incarnation);
            break;
        case FPI_WL_DST_TYPE_WFD_DHD:
            fprintf(stdout, "\t\t\t\tWL DST Type WFD-DHD (%d)\n",
                    fpi->wl_info.wl_dst_type);
            fprintf(stdout, "\t\t\t\tWFD IDX %d\n", fpi->wl_info.radio_idx);
            fprintf(stdout, "\t\t\t\tWFD Key: Flowring ID(%d), "
                            "Radio Index(%d)\n",
                    fpi->wl_info.flowring_id, fpi->wl_info.radio_idx);
            break;
        case FPI_WL_DST_TYPE_WDO_DIRECT:
            fprintf(stdout, "\t\t\t\tWL DST Type WDO-DIRECT (%d)\n",
                    fpi->wl_info.wl_dst_type);
            fprintf(stdout, "\t\t\t\tRadio Index %d, Flowring ID#%d\n",
                    fpi->wl_info.radio_idx, fpi->wl_info.flowring_id);
            break;
        }
    }

    if (fpi->flow.context.vtag_check != 0)
        fprintf(stdout, "\t\tVLAN Tag Check is enabled. VLAN Tag 0x%06x\n",
                fpi->flow.context.vtag_value);
    else
        fprintf(stdout, "\t\tVLAN Tag Check is disabled.\n");

    if (fpi->flow.context.vlan_8021q_remove != 0)
        fprintf(stdout, "\t\tVLAN 8021Q Header Remove is enabled.\n");
    else
        fprintf(stdout, "\t\tVLAN 8021Q Header Remove is disabled.\n");

    if (fpi->flow.context.vlan_8021q_prepend != 0)
        fprintf(stdout, "\t\tVLAN 8021Q Header Prepend is enabled. "
                "Value 0x%08x\n", fpi->flow.context.vlan_8021q_hdr);
    else
        fprintf(stdout, "\t\tVLAN 8021Q Header Prepend is disabled.\n");

    if (fpi->flow.key.mode == fpi_mode_l3l4) {
        fprintf(stdout, "\t\tSRC MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                fpi->flow.context.src_mac[0], fpi->flow.context.src_mac[1],
                fpi->flow.context.src_mac[2], fpi->flow.context.src_mac[3],
                fpi->flow.context.src_mac[4], fpi->flow.context.src_mac[5]);
        fprintf(stdout, "\t\tDST MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                fpi->flow.context.dst_mac[0], fpi->flow.context.dst_mac[1],
                fpi->flow.context.dst_mac[2], fpi->flow.context.dst_mac[3],
                fpi->flow.context.dst_mac[4], fpi->flow.context.dst_mac[5]);
    }

    fprintf(stdout, "\t\tNAPT Enable %d\n", fpi->flow.context.napt_enable);
    if (fpi->flow.context.napt_enable == 1 ||
        (l3l4_key && l3l4_key->esp_spi_mode != FPI_ESP_IGNORED)) {
        if (is_ipv6 == 1) {
            fprintf(stdout, "\t\tSRC IP %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
                    (fpi->flow.context.src_ip[0] >> 16),
                    (fpi->flow.context.src_ip[0] & 0xffff),
                    (fpi->flow.context.src_ip[1] >> 16),
                    (fpi->flow.context.src_ip[1] & 0xffff),
                    (fpi->flow.context.src_ip[2] >> 16),
                    (fpi->flow.context.src_ip[2] & 0xffff),
                    (fpi->flow.context.src_ip[3] >> 16),
                    (fpi->flow.context.src_ip[3] & 0xffff));
            fprintf(stdout, "\t\tDST IP %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
                    (fpi->flow.context.dst_ip[0] >> 16),
                    (fpi->flow.context.dst_ip[0] & 0xffff),
                    (fpi->flow.context.dst_ip[1] >> 16),
                    (fpi->flow.context.dst_ip[1] & 0xffff),
                    (fpi->flow.context.dst_ip[2] >> 16),
                    (fpi->flow.context.dst_ip[2] & 0xffff),
                    (fpi->flow.context.dst_ip[3] >> 16),
                    (fpi->flow.context.dst_ip[3] & 0xffff));
        } else if (is_ipv4 == 1) {
            fprintf(stdout, "\t\tSRC IP %d.%d.%d.%d\n",
                    ((fpi->flow.context.src_ip[0] >> 24) & 0xff),
                    ((fpi->flow.context.src_ip[0] >> 16) & 0xff),
                    ((fpi->flow.context.src_ip[0] >> 8) & 0xff),
                    (fpi->flow.context.src_ip[0] & 0xff));
            fprintf(stdout, "\t\tDST IP %d.%d.%d.%d\n",
                    ((fpi->flow.context.dst_ip[0] >> 24) & 0xff),
                    ((fpi->flow.context.dst_ip[0] >> 16) & 0xff),
                    ((fpi->flow.context.dst_ip[0] >> 8) & 0xff),
                    (fpi->flow.context.dst_ip[0] & 0xff));
        } else
            fprintf(stdout, "\t\t\tIP header is not used.\n");

        if ((is_ipv6 == 1) || (is_ipv4 == 1)) {
            fprintf(stdout, "\t\tSRC Port %d\n", fpi->flow.context.src_port);
            fprintf(stdout, "\t\tDST Port %d\n", fpi->flow.context.dst_port);
        }
    } else {
        fprintf(stdout, "\t\t\tIP headers are not used.\n");
    }

    if (fpi->flow.context.dscp_rewrite != 0)
        fprintf(stdout, "\t\tDSCP Rewrite is enabled with value = 0x%02x\n",
                fpi->flow.context.dscp);
    else
        fprintf(stdout, "\t\tDSCP Rewrite is disabled.\n");

    if (fpi->flow.context.gre_remove != 0)
        fprintf(stdout, "\t\tGRE Header Remove is enabled.\n");
    else
        fprintf(stdout, "\t\tGRE Header Remove is disabled.\n");

    if (fpi->flow.context.gre_prepend != 0) {
        fprintf(stdout, "\t\tGRE Header Prepend is enabled.\n");
        fprintf(stdout, "\t\t\tOuter SRC MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                fpi->flow.context.outer_src_mac[0],
                fpi->flow.context.outer_src_mac[1],
                fpi->flow.context.outer_src_mac[2],
                fpi->flow.context.outer_src_mac[3],
                fpi->flow.context.outer_src_mac[4],
                fpi->flow.context.outer_src_mac[5]);
        fprintf(stdout, "\t\t\tOuter DST MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                fpi->flow.context.outer_dst_mac[0],
                fpi->flow.context.outer_dst_mac[1],
                fpi->flow.context.outer_dst_mac[2],
                fpi->flow.context.outer_dst_mac[3],
                fpi->flow.context.outer_dst_mac[4],
                fpi->flow.context.outer_dst_mac[5]);
        if (is_ipv6 == 1)
            dump_size = 40;
        else
            dump_size = 20;
        fprintf(stdout, "\t\t\tEncapsulating IP Header (%d bytes)\n\t\t\t",
                dump_size);
        for (i = 0; i < dump_size; i++) {
            fprintf(stdout, "%02x", fpi->flow.context.encap_ip_hdr[i]);
        }
        fprintf(stdout, "\n");
        fprintf(stdout, "\t\t\tGRE Header 0x%08x\n", fpi->flow.context.gre_hdr);
    } else
        fprintf(stdout, "\t\tGRE Header Prepend is disabled.\n");

    fprintf(stdout, "\t\tOuter L2 Header Remove is %s.\n",
            l2hdr_mode_name[fpi->flow.context.outer_l2hdr_remove_mode]);

    fprintf(stdout, "\t\tOuter L2 Header Insert is %s.\n",
            l2hdr_mode_name[fpi->flow.context.outer_l2hdr_insert_mode]);

    if (fpi->flow.context.outer_l2hdr_insert_mode != 0) {
        dump_size = 10 + (4 * fpi->flow.context.outer_l2hdr_insert_mode);
        fprintf(stdout, "\t\t\tOuter L2 Header:\n\t\t\t");
        for (i = 0; i < dump_size; i++) {
            fprintf(stdout, "%02x", fpi->flow.context.outer_l2hdr[i]);
        }
        fprintf(stdout, "\n");
    }

    fprintf(stdout, "\t\tEgress Priority %d\n",
            fpi->flow.context.egress_priority);
    fprintf(stdout, "\t\tMax Ingress Packet Size %d\n",
            fpi->flow.context.max_ingress_packet_size);
}

int bcm_fpictl_dump_flow(uint32_t handle)
{
    fpictl_data_t fpidata;
    uint32_t pkt;
    uint64_t byte;
    int err;

    memset(&fpidata, 0, sizeof(fpidata));
    err = bcm_fpictl_get_flow(handle, &fpidata);
    if (err)
        return err;

    bcm_fpictl_print_flow(&fpidata);

    /* stat */
    err = bcm_fpictl_get_stat(handle, &pkt, &byte);
    if (!err)
        fprintf(stdout, "\tStats:\t\tPacket: %u, byte: %llu\n", pkt, byte);

    return err;
}

int bcm_fpictl_get_stat(uint32_t handle, uint32_t *pkt_cnt, uint64_t *byte_cnt)
{
    fpictl_data_t fpidata;
    int err = 0;

    memset(&fpidata, 0, sizeof(fpidata));
    fpidata.subsys = fpictl_subsys_stat;
    fpidata.op = fpictl_op_get;
    fpidata.handle = handle;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }

    *pkt_cnt = fpidata.stat.pkt;
    *byte_cnt = fpidata.stat.byte;
    return err;
}

int bcm_fpictl_dump_flowtbl(void)
{
    int err = 0, idx = -1, f_first = 1;
    uint8_t def_prio = 0;
    fpictl_data_t fpidata;
    fpictl_data_t *fpi = &fpidata;

    err = bcm_fpictl_get_default_priority(&def_prio);
    if (err == 0)
        fprintf(stdout, "Default Priority is %d\n", def_prio);

    memset(fpi, 0x0, sizeof(fpictl_data_t));
    /* Get lkp_enable */
    fpi->subsys = fpictl_subsys_lkp_enable;
    fpi->op = fpictl_op_get;
    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
        return err;
    }
    if (fpi->enable)
        fprintf(stdout, "HW Flow Lookup is enabled.\n");
    else
        fprintf(stdout, "HW Flow Lookup is disabled.\n");

    /* Get L2Lkp on EtherType */
    fpi->subsys = fpictl_subsys_l2lkp_on_etype;
    fpi->op = fpictl_op_get;
    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
        return err;
    }
    if (fpi->enable)
        fprintf(stdout, "L2 Lookup on EtherType mode is enabled on "
                "EtherType = 0x%4x\n", fpi->flow.key.l2_key.eth_type);
    else
        fprintf(stdout, "L2 Lookup on EtherType mode is disabled.\n");

    /* Get GRE mode */
    memset(fpi, 0x0, sizeof(fpictl_data_t));
    fpi->subsys = fpictl_subsys_gre_mode;
    fpi->op = fpictl_op_get;
    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
        return err;
    }
    fprintf(stdout, "GRE Mode: %s\n\n", gre_flow_mode_name[fpi->gre_mode]);

    do {
        memset(fpi, 0x0, sizeof(fpictl_data_t));
        fpi->subsys = fpictl_subsys_flow;
        fpi->op = fpictl_op_getnext;
        fpi->next_idx = idx;

        err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata);
        if (err == -1)
        {
            fprintf(stderr, "ioctl command return error %d!\n", errno);
            return err;
        }

        if (fpi->next_idx == -1)
            break;

        err = fpi->rc;

        /* if reach here, means it has got a valid entry at the fpi->next_idx */
        if (f_first)
        {
            f_first = 0;
            fprintf(stdout, " key table:\n");
        }

        bcm_fpictl_print_flow(fpi);

        /* getnext automatically get the latest/updated stat too */
        fprintf(stdout, "\tStats:\t\tPacket: %u, byte: %llu\n",
                fpi->stat.pkt, fpi->stat.byte);

        idx = fpi->next_idx;
        fprintf(stdout, "\n");
    } while (err == 0);

    /* means no entry found */
    if (f_first)
        fprintf(stdout, "Flow table is empty.\n");

    return 0;
}

int bcm_fpictl_add_ap_mac(uint8_t *mac)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_apmac;
    fpidata.op = fpictl_op_add;
    memcpy(fpidata.flow.context.src_mac, mac, ETH_ALEN);

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_delete_ap_mac(uint8_t *mac)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_apmac;
    fpidata.op = fpictl_op_del_by_key;
    memcpy(fpidata.flow.context.src_mac, mac, ETH_ALEN);

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_dump_ap_mac(void)
{
    int err = 0, idx = -1, f_first = 1;
    fpictl_data_t fpidata;
    fpictl_data_t *fpi = &fpidata;
    uint8_t *mac;

    do {
        memset(fpi, 0x0, sizeof(fpictl_data_t));
        fpi->subsys = fpictl_subsys_apmac;
        fpi->op = fpictl_op_getnext;
        fpi->next_idx = idx;

        err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata);
        if (err == -1)
        {
            fprintf(stderr, "ioctl command return error %d!\n", errno);
            return err;
        }

        if (fpi->next_idx == -1)
            break;

        err = fpi->rc;

        /* if reach here, means it has got a valid entry at the fpi->next_idx */
        if (f_first)
        {
            f_first = 0;
            fprintf(stdout, "AP MAC table:\n");
        }

        mac = fpi->flow.context.src_mac;
        idx = fpi->next_idx;
        fprintf(stdout, "\tMAC[%d] = %02x:%02x:%02x:%02x:%02x:%02x\n",
                idx, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } while (err == 0);

    /* means no entry found */
    if (f_first)
        fprintf(stdout, "AP Mac table is empty.\n");

    return 0;
}

char *bcm_fpictl_err_message(int err)
{
    switch (err) {
    case 0:
        return "OK";
    case -ENOSPC:
        return "Out of resource";
    case -EEXIST:
        return "Duplicate entry";
    case -EKEYREJECTED:
        return "Collision occurs";
    case -EINVAL:
        return "Invalid parameters";
    case -ENOENT:
        return "Can't find the flow";
    case -EPERM:
        return "HW accelerator is not ready";
    }
    return "Unknown";
}

int bcm_fpictl_set_gre_mode(fpi_gre_mode_t mode)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_gre_mode;
    fpidata.op = fpictl_op_set;
    fpidata.gre_mode = mode;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_set_l2lkp_on_etype(uint8_t enable, uint16_t etype)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_l2lkp_on_etype;
    fpidata.op = fpictl_op_set;
    fpidata.enable = enable;
    fpidata.flow.key.l2_key.eth_type = etype;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

int bcm_fpictl_set_lkp_enable(uint8_t enable)
{
    fpictl_data_t fpidata;
    int err = 0;

    fpidata.subsys = fpictl_subsys_lkp_enable;
    fpidata.op = fpictl_op_set;
    fpidata.enable = enable;

    if ((err = fpictl_ioctl(fpictl_ioctl_sys, (void *)&fpidata))) {
        if (err == -1)
            fprintf(stderr, "ioctl command return error %d!\n", errno);
    }
    return err;
}

