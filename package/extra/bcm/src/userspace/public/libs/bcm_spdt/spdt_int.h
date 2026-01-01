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

#ifndef __SPDT_INT_H__
#define __SPDT_INT_H__

#define VALIDATE_IDX(stream_idx) \
do { \
    if ((stream_idx) >= SPDT_NUM_OF_STREAMS || (stream_idx) < 0) \
    { \
        printf("%s:%d stream_idx %d is out of bound\n", __FUNCTION__, __LINE__, (stream_idx)); \
        return -1; \
    } \
} while(0)

typedef struct {
    int (*init)(spdt_proto_t proto, uint8_t stream_idx);
    int (*uninit)(uint8_t stream_idx);
    int (*connect)(uint8_t stream_idx, spdt_stream_dir_t dir, spdt_proto_t protocol, spdt_conn_params_t *params);
    int (*set_options)(uint8_t stream_idx, spdt_stream_options_t *options);
    int (*disconnect)(uint8_t stream_idx);
    int (*msg_send)(uint8_t stream_idx, char *buf, uint32_t len);
    int (*msg_receive)(uint8_t stream_idx, char *buf, uint32_t len, uint32_t *received);
    int (*stats_get)(uint8_t stream_idx, int blocking, spdt_stat_t *stat);
    int (*recv_start)(uint8_t stream_idx, spdt_proto_t protocol, spdt_rx_params_t *rx_params);
    int (*recv_stop)(uint8_t stream_idx);
    int (*send_start)(uint8_t stream_idx, spdt_proto_t protocol, spdt_tx_params_t *tx_params);
    void (*send_stop)(uint8_t stream_idx);
} spdt_proto_ops_t;

int tcpspdtest_genl_init(uint8_t *stream_idx);
int tcpspdtest_genl_shutdown(uint8_t stream_idx, uint8_t *num_streams);
int tcpspdtest_stream_idx(uint8_t *stream_idx, uint8_t *num_stream, int nl_fd_local, uint32_t *nlmsg_seq_local, tcpspdtest_genl_cmd_param_t cmd_param);
int tcpspdtest_stream_params(uint8_t stream_idx, spdt_stream_params_t *stream_params, tcpspdtest_genl_cmd_param_t cmd_param);
int tcpspdtest_num_streams(uint8_t stream_idx, uint8_t *num_streams, uint8_t *num_udp_streams);

extern spdt_proto_ops_t spdt_udp_ops;
extern spdt_proto_ops_t spdt_tcp_ops;

#endif /* __SPDT_INT_H__ */

