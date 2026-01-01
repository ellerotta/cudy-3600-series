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
#ifdef BCM_XRDP
#include <user_api.h>
#endif
#include <spdt_api.h>
#include "spdt_int.h"

#define SPDT_API_CALL_RET(func,...) \
   do { \
        int rc = 0; \
        if (!stream[stream_idx].proto_ops) \
        { \
            printf("Called %s for uninitialized stream %d!\n", __func__, stream_idx); \
            return -1; \
        } \
        if (stream[stream_idx].proto_ops->func) \
            rc = stream[stream_idx].proto_ops->func(__VA_ARGS__); \
        if (rc) \
            printf("%s failed!\n", __func__); \
        return rc; \
    } while(0);

#define SPDT_API_CALL(func,...) \
    do { \
        if (!stream[stream_idx].proto_ops) \
        { \
            printf("Called %s for uninitialized stream %d!\n", __func__, stream_idx); \
            return; \
        } \
        if (stream[stream_idx].proto_ops->func) \
            stream[stream_idx].proto_ops->func(__VA_ARGS__); \
    } while(0);

typedef struct {
    spdt_proto_t protocol;
    spdt_proto_ops_t *proto_ops;
} spdt_proto_handler_t;

static spdt_proto_handler_t stream[SPDT_NUM_OF_STREAMS] = {};
static spdt_proto_ops_t *protocol_to_ops[SPDT_MAX] = {
    [SPDT_HTTP] = &spdt_tcp_ops, 
    [SPDT_FTP] = &spdt_tcp_ops,
    [SPDT_IPERF3_TCP] = &spdt_tcp_ops,
#ifdef BCM_XRDP
    [SPDT_UDP_BASIC] = &spdt_udp_ops,
    [SPDT_IPERF3_UDP] = &spdt_udp_ops,
#endif
};

int spdt_init(spdt_proto_t proto, uint8_t *stream_idx)
{
    int rc = 0;

    if (proto >= SPDT_MAX || !protocol_to_ops[proto])
    {
        printf("%s:%d Unsuppported protocol %d\n", __FUNCTION__, __LINE__, proto);
        return -1;
    }

    /* Init genl socket and allocate new stream_idx */
    rc = tcpspdtest_genl_init(stream_idx);
    if (-1 == rc)
    {
        printf("%s:%d Failed to initialize generic netlink, rc=%d\n", __FUNCTION__, __LINE__, rc);
        return -1;
    }

    stream[*stream_idx].protocol = proto;
    stream[*stream_idx].proto_ops = protocol_to_ops[proto];

    /* Set stream protocol */
    rc = tcpspdtest_protocol(*stream_idx, &proto, TCPSPDTEST_GENL_CMD_PARAM_SET);
    if (rc)
    {
        printf("%s:%d Failed to set stream_idx:%u protocol:%u\n", __FUNCTION__, __LINE__, *stream_idx, proto);
        return rc;
    }

    if (stream[*stream_idx].proto_ops->init)
        rc = stream[*stream_idx].proto_ops->init(proto, *stream_idx);

    return rc;
}

void spdt_uninit(uint8_t stream_idx)
{
    uint8_t num_streams;
    spdt_proto_handler_t h = stream[stream_idx];
    int rc;

    if (!h.proto_ops)
    {
        printf("Ignore second spdt_uninit() call\n");
        return;
    }
    if (h.proto_ops->uninit)
        h.proto_ops->uninit(stream_idx);

    /* Close genl socket and free stream protocol */
    rc = tcpspdtest_genl_shutdown(stream_idx, &num_streams);
    if (rc)
    {
        printf("%s:%d Failed to shutdown genl for stream_idx:%u, rc:%d\n", __FUNCTION__, __LINE__, stream_idx, rc);
        return;
    }

    stream[stream_idx].protocol = SPDT_NONE;
    stream[stream_idx].proto_ops = NULL;
}

int spdt_connect(uint8_t stream_idx, spdt_stream_dir_t dir, spdt_conn_params_t *conn_params)
{
    SPDT_API_CALL_RET(connect, stream_idx, dir, stream[stream_idx].protocol, conn_params);
}

int spdt_set_options(uint8_t stream_idx, spdt_stream_options_t *options)
{
    SPDT_API_CALL_RET(set_options, stream_idx, options);
}

void spdt_disconnect(uint8_t stream_idx)
{
    SPDT_API_CALL(disconnect, stream_idx);
}

int spdt_oob_send(uint8_t stream_idx, char *buff, uint32_t length)
{
    SPDT_API_CALL_RET(msg_send, stream_idx, buff, length);
}

int spdt_msg_receive(uint8_t stream_idx, char *buff, uint32_t length, uint32_t *received)
{
    SPDT_API_CALL_RET(msg_receive, stream_idx, buff, length, received);
}

int spdt_get_conn_params(uint8_t stream_idx, spdt_conn_params_t *conn_params)
{
    int rc;
    spdt_stream_params_t stream_params;

    rc = tcpspdtest_stream_params(stream_idx, &stream_params, TCPSPDTEST_GENL_CMD_PARAM_GET);
    if (rc)
        return rc;

    memcpy(conn_params, &stream_params.conn_params, sizeof(*conn_params));

    return rc;
}

int spdt_stats_get(uint8_t stream_idx, spdt_stats_get_mode_t mode, spdt_stat_t *stat)
{
    SPDT_API_CALL_RET(stats_get, stream_idx, (mode == SPDT_STATS_GET_BLOCKING_END_OF_TEST), stat);
}

int spdt_recv_start(uint8_t stream_idx, spdt_rx_params_t *rx_params)
{
    SPDT_API_CALL_RET(recv_start, stream_idx, stream[stream_idx].protocol, rx_params);
}

void spdt_recv_stop(uint8_t stream_idx)
{
    SPDT_API_CALL(recv_stop, stream_idx);
}

int spdt_send_start(uint8_t stream_idx, spdt_tx_params_t *tx_params)
{
    SPDT_API_CALL_RET(send_start, stream_idx, stream[stream_idx].protocol, tx_params);
}

void spdt_send_stop(uint8_t stream_idx)
{
    SPDT_API_CALL(send_stop, stream_idx);
}
