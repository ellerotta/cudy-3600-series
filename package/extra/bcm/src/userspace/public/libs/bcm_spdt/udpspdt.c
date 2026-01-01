/*
<:copyright-BRCM:2018:DUAL/GPL:standard

   Copyright (c) 2018 Broadcom 
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/
#ifdef BCM_XRDP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bdmf_api.h>
#include <user_api.h>
#include <spdt_api.h>
#include "spdt_int.h"
#include <tcpspdtest_defs.h>

typedef struct {
    int is_own_sock_fd;
} udpspdt_stream_t;

static udpspdt_stream_t udp_streams[SPDT_NUM_OF_STREAMS] = {};

void ipv4_mapped_ipv6_to_ipv4(struct sockaddr_storage *sa)
{
    struct sockaddr_in6 * sa_in6 = (struct sockaddr_in6 *)sa;
    struct sockaddr_in * sa_in = (struct sockaddr_in *)sa;

    if (sa_in6->sin6_family != AF_INET6)
        return;

    if ((sa_in6->sin6_addr.__in6_u.__u6_addr32[0] | sa_in6->sin6_addr.__in6_u.__u6_addr32[1] |
        (sa_in6->sin6_addr.__in6_u.__u6_addr32[2] ^ htonl(0x0000ffff))) == 0UL)
    {
        sa_in->sin_addr.s_addr = sa_in6->sin6_addr.__in6_u.__u6_addr32[3];
        sa_in->sin_family = AF_INET;
    }
}

static rdpa_udpspdtest_proto_t spdt_proto_to_rdpa_udpspd_proto(spdt_proto_t proto)
{
    rdpa_udpspdtest_proto_t rdpa_proto = rdpa_udpspdtest_proto_basic;

    if (proto == SPDT_IPERF3_UDP) /* No more protocols are currently supported */
        rdpa_proto = rdpa_udpspdtest_proto_iperf3;

    return rdpa_proto;
}

static void udpspdt_obj_uninit(void)
{
}

static int udpspdt_init(spdt_proto_t proto, uint8_t stream_idx)
{
    spdt_stream_params_t stream_params = {};
    uint8_t num_udp_streams;
    rdpa_udpspdtest_cfg_t udpspdtest_cfg = {};
    bdmf_object_handle udpspdt_obj;
    int rc;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
    {
        printf("%s:%d Failed to retrieve RDPA UDP Speed Test management object, rc %d\n",
            __FUNCTION__, __LINE__, rc);
        return -1;
    }

    rc = tcpspdtest_num_streams(stream_idx, NULL, &num_udp_streams);
    if (rc)
    {
        printf("%s:%d Failed to get num of streams, rc:%d\n", __FUNCTION__, __LINE__, rc);
        goto exit;
    }

    udpspdtest_cfg.proto = spdt_proto_to_rdpa_udpspd_proto(proto);
    rc = rdpa_udpspdtest_cfg_set(udpspdt_obj, stream_idx, &udpspdtest_cfg);
    if (rc)
        goto exit;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        goto exit;

    stream_params.protocol = proto;
    stream_params.dir = SPDT_DIR_NONE;
    stream_params.sock_fd = -1;
    udp_streams[stream_idx].is_own_sock_fd = 1;
    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);

exit:
    if (rc)
    {
        printf("%s:%d Failed to propagate test protocol to RDPA UDP Speed Test management object,"
            "rc %d\n", __FUNCTION__, __LINE__, rc);
    }
    bdmf_put(udpspdt_obj);
    return rc;
}

static int udpspdt_uninit(uint8_t stream_idx)
{
    spdt_stream_params_t stream_params;
    uint8_t num_udp_streams;
    int rc;

    rc = tcpspdtest_num_streams(stream_idx, NULL, &num_udp_streams);
    if (rc)
    {
        printf("%s:%d Failed to get num of streams, rc:%d\n", __FUNCTION__, __LINE__, rc);
        return rc;
    }

    if (1 == num_udp_streams)
        udpspdt_obj_uninit();

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (!rc)
    {
        stream_params.protocol = SPDT_NONE;
        stream_params.dir = SPDT_DIR_NONE;
        stream_params.sock_fd = -1;
        udp_streams[stream_idx].is_own_sock_fd = 1;
        rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    }

    return rc;
}

static int udpspdt_set_options(uint8_t stream_idx, spdt_stream_options_t *options)
{
    int rc = 0;
    udp_spdt_stream_options_t *udp_options = &options->proto.udp_options;
    spdt_stream_params_t stream_params;

    if (udp_options->has_sock_fd)
    {
        rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
        if (!rc)
        {
            udp_streams[stream_idx].is_own_sock_fd = 0;
            stream_params.sock_fd = udp_options->sock_fd;
            rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
        }
        if (rc)
        {
            printf("Failed to set stream_params for a UDP socket, rc:%d\n", rc);
        }
    }

    return rc;
}

static int udpspdt_connect(uint8_t stream_idx, spdt_stream_dir_t dir, spdt_proto_t protocol, spdt_conn_params_t *conn_params)
{
    int sock_fd = -1, rc = 0;
    int is_v6;
    struct sockaddr *local_addr;
    struct sockaddr_in local_addr_v4;
    struct sockaddr_in6 local_addr_v6;
    socklen_t socklen;
    spdt_stream_params_t stream_params;
    uint8_t num_udp_streams;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
    {
        printf("Failed to get stream_params for a UDP socket, rc:%d\n", rc);
        return rc;
    }

    is_v6 = conn_params->server_addr.ss_family == AF_INET6 ? 1 : 0;
    socklen = is_v6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

    if (udp_streams[stream_idx].is_own_sock_fd)
    {
        /* Open a socket */
        sock_fd = socket(is_v6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
        if (sock_fd < 0)
        {
            printf("Failed to open a UDP socket, IPv%d, rc %d (errno %d)\n", is_v6 ? 6 : 4, sock_fd, errno);
            return -1;
        }
    }

    /* If local address is set (IP and port), allow to open a connection from the specified local address. Otherwise,
     * assign automatically */
    if (is_v6)
    {
        memcpy(&local_addr_v6, &conn_params->local_addr, socklen);
        local_addr = (struct sockaddr *)&local_addr_v6;
        if (local_addr_v6.sin6_port && (local_addr_v6.sin6_addr.s6_addr32[0] ||
            local_addr_v6.sin6_addr.s6_addr32[1] ||
            local_addr_v6.sin6_addr.s6_addr32[2] || local_addr_v6.sin6_addr.s6_addr32[3])) 
        {
            if (udp_streams[stream_idx].is_own_sock_fd)
            {
                rc = bind(sock_fd, local_addr, socklen);
                if (rc)
                {
                    printf("Failed to bind a UDP socket to a local address, port %d, rc %d (errno %d)\n",
                        local_addr_v6.sin6_port, rc, errno);
                    goto error;
                }
            }
        }
    }
    else
    {
        memcpy(&local_addr_v4, &conn_params->local_addr, socklen);
        local_addr = (struct sockaddr *)&local_addr_v4;
        if (local_addr_v4.sin_port && local_addr_v4.sin_addr.s_addr)
        {
            if (udp_streams[stream_idx].is_own_sock_fd)
            {
                rc = bind(sock_fd, local_addr, socklen);
                if (rc)
                {
                    printf("Failed to bind a UDP socket to a local address, port %d, rc %d (errno %d)\n",
                        local_addr_v4.sin_port, rc, errno);
                    goto error;
                }
            }
        }
    }

    if (udp_streams[stream_idx].is_own_sock_fd)
    {
        /* Connect a socket to the peer */
        rc = connect(sock_fd, (struct sockaddr *)&(conn_params->server_addr), socklen);
        if (rc)
        {
            printf("Failed to connect a UDP socket, IPv%d, rc %d (errno %d)\n", is_v6 ? 6 : 4, rc, errno);
            goto error;
        }
        stream_params.sock_fd = sock_fd;
    }
    else
    {
        sock_fd = stream_params.sock_fd;
    }

    rc = getsockname(sock_fd, local_addr, &socklen);
    if (rc)
    {
        printf("Failed to get local connection info for a UDP socket, IPv%d, rc %d (errno %d)\n", is_v6 ? 6 : 4, rc,
            errno);
        goto error;
    }
    memcpy(&conn_params->local_addr, local_addr, socklen);

    ipv4_mapped_ipv6_to_ipv4(&conn_params->server_addr);
    ipv4_mapped_ipv6_to_ipv4(&conn_params->local_addr);

    stream_params.is_v6 = is_v6;
    memcpy(&stream_params.conn_params, conn_params, sizeof(stream_params.conn_params));
    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    if (rc)
    {
        printf("Failed to set stream_params for a UDP socket, rc:%d\n", rc);
        goto error;
    }

    rc = tcpspdtest_num_streams(stream_idx, NULL, &num_udp_streams);
    if (rc)
    {
        printf("%s:%d Failed to get num of streams, rc:%d\n", __FUNCTION__, __LINE__, rc);
        goto error;
    }

    if (num_udp_streams == 1) /* First stream, init hooks */
    {
        rc = udpspdt_init_nf_hooks(stream_idx);
        if (rc)
        {
            printf("Failed register NF hooks, rc %d\n", rc);
            goto error;
        }
    }

    return 0;

error:
    if (udp_streams[stream_idx].is_own_sock_fd && sock_fd != -1)
        close(sock_fd);
    return rc;
}

static int udpspdt_disconnect(uint8_t stream_idx)
{
    spdt_stream_params_t stream_params;
    uint8_t num_udp_streams;
    bdmf_object_handle udpspdt_obj;
    int rc;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return rc;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        goto exit;

    if (stream_params.dir == SPDT_DIR_RX)
    {
        rdpa_udpspdtest_rx_params_t _rx_params = {};

        /* When invoke rx_params_set with already configured flow IDs, the */
        rc = rdpa_udpspdtest_rx_params_set(udpspdt_obj, stream_idx, &_rx_params);
        if (rc)
            printf("Failed to reset RX params, stream IDX = %d, rc = %d\n", stream_idx, rc);
    }

    if (udp_streams[stream_idx].is_own_sock_fd && stream_params.sock_fd)
        close(stream_params.sock_fd);

    stream_params.sock_fd = -1;
    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);

    rc = tcpspdtest_num_streams(stream_idx, NULL, &num_udp_streams);
    if (rc)
    {
        printf("%s:%d Failed to get num of streams, rc:%d\n", __FUNCTION__, __LINE__, rc);
        goto exit;
    }

    if (num_udp_streams == 1) /* Last stream, un-init hooks */
    {
        rc = udpspdt_uninit_nf_hooks(stream_idx);
        if (rc)
            printf("Failed to unregister NF hooks, stream IDX = %d, rc = %d\n", stream_idx, rc);
    }

exit:
    bdmf_put(udpspdt_obj);
    return rc;
}

static int udpspdt_stats_get(uint8_t stream_idx, int blocking, spdt_stat_t *stat)
{
    int rc;
    rdpa_udpspdtest_stat_t _stat;
    udp_spdt_basic_stat_t *udp_basic;
    udp_spdt_iperf3_stat_t *udp_iperf3;
    spdt_stream_params_t stream_params;
    bdmf_object_handle udpspdt_obj;

    if (blocking)
        return -1;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return rc;

    rc = rdpa_udpspdtest_stream_stat_get(udpspdt_obj, stream_idx, &_stat);
    if (rc)
        goto exit;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        goto exit;

    if (stream_params.dir == SPDT_DIR_RX)
    {
        /* Fill RX statistics. */
        udp_basic = &(stat->proto_ext.udp_basic);

        udp_basic->rx.packets = _stat.basic.rx_packets;
        udp_basic->rx.bytes = _stat.basic.rx_bytes;

        if (stream_params.protocol != SPDT_IPERF3_UDP)
        {
            udp_basic->rx_usec = _stat.basic.rx_time_usec;
            goto exit;
        }

        /* Basic and iperf3 share same structure at the beginning */
        udp_iperf3 = &(stat->proto_ext.udp_iperf3);
        udp_iperf3->out_of_order_pkts = _stat.iperf3_ext.out_of_order_pkts;
        udp_iperf3->error_cnt_pkts = _stat.iperf3_ext.error_cnt_pkts;
        udp_iperf3->jitter = _stat.iperf3_ext.jitter;
    }
    else if (stream_params.dir == SPDT_DIR_TX)
    {
        /* Fill TX statistics. */
        udp_basic = &(stat->proto_ext.udp_basic);

        udp_basic->tx.packets = _stat.basic.tx_packets;
        /* Calc bytes */
        udp_basic->tx.bytes = _stat.basic.tx_packets * stream_params.tx_data_buf_len;

        if (stream_params.protocol != SPDT_IPERF3_UDP)
            udp_basic->tx_usec = _stat.basic.tx_time_usec;

        /* Iperf3 TX drops, jitters and out-of-orders calculated at the receiver side */
    }
    else
        rc = -1;

exit:
    bdmf_put(udpspdt_obj);
    return rc;
}

static void rdpa_rx_ip_and_port_set(struct sockaddr_storage *addr, bdmf_ip_t *ip, uint16_t *port)
{
    if (addr->ss_family == AF_INET)
    {
        struct sockaddr_in *_addr = (struct sockaddr_in *)addr;

        ip->family = bdmf_ip_family_ipv4; 
        ip->addr.ipv4 = htonl(_addr->sin_addr.s_addr);
        *port = htons(_addr->sin_port);
    }
    else
    {
        struct sockaddr_in6 *_addr = (struct sockaddr_in6 *)addr;

        ip->family = bdmf_ip_family_ipv6; 
        memcpy(ip->addr.ipv6.data, _addr->sin6_addr.__in6_u.__u6_addr8, 16);
        *port = htons(_addr->sin6_port);
    }
}

static int do_recv_start(uint8_t stream_idx, spdt_rx_params_t *rx_params)
{
    int rc = 0;
    rdpa_udpspdtest_rx_params_t _rx_params = {};
    spdt_stream_params_t stream_params;
    bdmf_object_handle udpspdt_obj;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return rc;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        goto exit;

    /* Get connection params */
    rdpa_rx_ip_and_port_set(&stream_params.conn_params.local_addr, &_rx_params.local_ip_addr,
        &_rx_params.local_port_nbr);
    rdpa_rx_ip_and_port_set(&stream_params.conn_params.server_addr, &_rx_params.remote_ip_addr,
        &_rx_params.remote_port_nbr);

    rc = rdpa_udpspdtest_rx_params_set(udpspdt_obj, stream_idx, &_rx_params);
    rc = rc ? rc : rdpa_udpspdtest_rx_start_set(udpspdt_obj, stream_idx);
    if (rc)
        printf("Failed to configure and start receive connection, stream IDX %d, rc %d\n", stream_idx, rc);

exit:
    bdmf_put(udpspdt_obj);
    return rc;
}

static int udpspdt_recv_start(uint8_t stream_idx, spdt_proto_t protocol, spdt_rx_params_t *rx_params)
{
    int rc;
    spdt_stream_params_t stream_params;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (!rc)
    {
        stream_params.dir = SPDT_DIR_RX;
        rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    }

    if (!rc)
        rc = do_recv_start(stream_idx, rx_params);

    if (rc)
    {
        stream_params.dir = SPDT_DIR_NONE;
        rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    }
    return rc;
}

static int udpspdt_recv_stop(uint8_t stream_idx)
{
    bdmf_object_handle udpspdt_obj;
    int rc;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return rc;

    rdpa_udpspdtest_rx_stop_set(udpspdt_obj, stream_idx);
    bdmf_put(udpspdt_obj);

    return 0;
}

static int _spdt_tx_sockopt_set(uint8_t stream_idx)
{
    spdt_stream_params_t stream_params;
    bdmf_object_handle udpspdt_obj;
    int rc, val;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return rc;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        goto exit;

    /* Configuration below will do MTU path discovery and:
     * 1. Will not let to transmit a data buffer that causes MTU exceed 
     * 2. Packets will be sent with a DF flag set in the IP header */
    val = IP_PMTUDISC_DO;
    rc = setsockopt(stream_params.sock_fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
    if (rc < 0)
    {
        printf("Unable to set socket MTU Path Discovery, rc %d (errno %d)\n", rc, errno);
        goto exit;
    }

exit:
    bdmf_put(udpspdt_obj);
    return rc;
}

static int do_send_start(uint8_t stream_idx, spdt_tx_params_t *tx_params)
{
    int rc;
    int data_alloc_here = 0;
    void *data_buf = NULL;
    rdpa_udpspdtest_tx_params_t _tx_params = {};
    spdt_stream_params_t stream_params;
    bdmf_object_handle udpspdt_obj;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return rc;

    /* Mark socket so it can be recognized by the network driver */
    rc = _spdt_tx_sockopt_set(stream_idx);
    if (rc)
        goto exit;
    rc = -1;
    if (!tx_params->proto.udp.data_buf_len)
    {
        printf("Data buffer length must be set\n");
        goto exit;
    }

    data_buf = tx_params->proto.udp.data_buf;
    if (!data_buf)
    {
        data_buf = bdmf_calloc(tx_params->proto.udp.data_buf_len);
        if (!data_buf)
        {
            printf("Failed to allocate data buffer for TX, errno %d\n", errno);
            goto exit;
        }
        data_alloc_here = 1; 
    }

    /* Propagate TX parameters to Runner */
    _tx_params.kbps = tx_params->proto.udp.kbps;
    _tx_params.mbs = tx_params->proto.udp.max_burst_size;
    _tx_params.iperf3_64bit_counters = tx_params->proto.udp.iperf3_64bit_counters;
    if (tx_params->proto.udp.total_bytes_to_send)
    {
        /* If not endless test, calculate number of packets to send */
        _tx_params.total_packets_to_send = (tx_params->proto.udp.total_bytes_to_send + tx_params->proto.udp.data_buf_len - 1) / tx_params->proto.udp.data_buf_len;
    }

    rc = rdpa_udpspdtest_tx_params_set(udpspdt_obj, stream_idx, &_tx_params);
    if (rc)
    {
        printf("Failed to set UDP speed test TX params, errno %d\n", rc);
        goto exit;
    }

    /* Start sending by writing the data to the socket. The tx_start will be triggered by the network driver after the
     * whole packet will be ready */
    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (!rc)
    {
        stream_params.tx_data_buf_len = tx_params->proto.udp.data_buf_len;
        rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    }
    if (rc)
    {
        printf("Failed to set tx buffer len, rc = %d\n", rc);
        goto exit;
    }

    rc = write(stream_params.sock_fd, data_buf, tx_params->proto.udp.data_buf_len);
    if (rc < 0)
    {
        printf("Failed to send data buffer to the socket, rc = %d, errno = %d%s\n", rc, errno,
            errno == EMSGSIZE ? " (data buffer too long, MTU size is exceeded)" : "");
        goto exit;
    }

    rc = 0;

exit:
    if (data_buf && data_alloc_here)
        bdmf_free(data_buf);
    bdmf_put(udpspdt_obj);

    return rc;
}

static int udpspdt_send_start(uint8_t stream_idx, spdt_proto_t protocol, spdt_tx_params_t *tx_params)
{
    spdt_stream_params_t stream_params;
    int rc;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (!rc)
    {
        stream_params.dir = SPDT_DIR_TX;
        rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    }

    if (!rc)
        rc = do_send_start(stream_idx, tx_params);

    if (rc)
    {
        stream_params.dir = SPDT_DIR_NONE;
        tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_SET);
    }

    return rc;
}

static void do_send_stop(uint8_t stream_idx)
{
    bdmf_object_handle udpspdt_obj;
    int rc;

    rc = rdpa_udpspdtest_get(&udpspdt_obj);
    if (rc)
        return;

    rdpa_udpspdtest_tx_stop_set(udpspdt_obj, stream_idx);
    bdmf_put(udpspdt_obj);
}

static int udpspdt_msg_send(uint8_t stream_idx, char *buf, uint32_t len)
{
    spdt_stream_params_t stream_params;
    int rc;
    
    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        return rc;

    rc = write(stream_params.sock_fd, buf, len);
    if (rc < 0)
    {
        printf("Failed to send data buffer to the socket, rc = %d, errno = %d%s\n", rc, errno,
            errno == EMSGSIZE ? " (data buffer too long, MTU size is exceeded)" : "");
        goto exit;
    }
    rc = 0;

exit:
    return rc;
}

static int udpspdt_msg_receive(uint8_t stream_idx, char *buf, uint32_t len, uint32_t *received)
{
    spdt_stream_params_t stream_params;
    int rc;
    int sz;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        return rc;


    sz = recv(stream_params.sock_fd, buf, len, 0);
    if (sz < 0)
    {
        rc = errno;
        printf("Failed to receie data from the socket, rc = %d%s\n", errno,
            errno == EMSGSIZE ? " (data buffer too long, MTU size is exceeded)" : "");
        goto exit;
    }

    rc = 0;
    *received = sz;

exit:
    return rc;
}

#define UDPSPDTEST_PURGE_BUF_SIZE 512
static void udpspdt_sock_purge(uint8_t stream_idx)
{
    spdt_stream_params_t stream_params;
    int rc;
    char buf[UDPSPDTEST_PURGE_BUF_SIZE];
    
    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        return;
    do {
        rc = recv(stream_params.sock_fd, buf, UDPSPDTEST_PURGE_BUF_SIZE, MSG_DONTWAIT);
    } while (rc >= 0);
}

static void udpspdt_send_stop(uint8_t stream_idx)
{
     do_send_stop(stream_idx);
     udpspdt_sock_purge(stream_idx);
}

spdt_proto_ops_t spdt_udp_ops = {
    .init = udpspdt_init,
    .uninit = udpspdt_uninit,
    .connect = udpspdt_connect,
    .set_options = udpspdt_set_options,
    .disconnect = udpspdt_disconnect,
    .msg_send = udpspdt_msg_send,
    .msg_receive = udpspdt_msg_receive,
    .stats_get = udpspdt_stats_get,
    .recv_start = udpspdt_recv_start,
    .recv_stop = udpspdt_recv_stop,
    .send_start = udpspdt_send_start,
    .send_stop = udpspdt_send_stop
};

#endif /* BCM_XRDP */
