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

/*
*******************************************************************************
* File Name  : tcpspdtest_genl.c
*
* Description: This file contains the Broadcom Tcp Speed Test Generic Netlink Library Implementation.
*******************************************************************************
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
#include <signal.h>
#include <pthread.h>
#include <linux/genetlink.h>
#include <spdt_defs.h>
#include "tcpspdtest_defs.h"
#include "spdt_int.h"

/******************************************** Defines ********************************************/

/* GENL request/response messages */
typedef struct
{
    struct nlmsghdr   nl;
    struct genlmsghdr genl;
    char buf[TCPSPDTEST_GENL_MAX_MSG_LEN];
} tcpspdtest_genl_family_msg_t;

#define GENLMSG_DATA(glh)     ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh)  (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na)          ((void *)((char*)(na) + NLA_HDRLEN))

#define GENLMSG_INIT_SEQ      60

/**************************************** Global / Static  ***************************************/
static int nl_family_id = -1;
static uint32_t nlmsg_seq[SPDT_NUM_OF_STREAMS] = { 0 };
static int nl_fd[SPDT_NUM_OF_STREAMS] = { [0 ... SPDT_NUM_OF_STREAMS-1] = -1 };
static pthread_mutex_t streams_lock =  PTHREAD_MUTEX_INITIALIZER;
static uint32_t streams_num_cnt = 0;

/**************************************** Implementation *****************************************/
static inline int tcpspdtest_genl_get_family_id(int nl_fd, struct sockaddr_nl *nl_address)
{
    tcpspdtest_genl_family_msg_t genl_request_msg, genl_response_msg;
    struct nlattr *nl_attr;
    int len;

    /* Resolve the family ID, fill netlink &&  genl && nl attr headers */
    genl_request_msg.nl.nlmsg_type = GENL_ID_CTRL;
    genl_request_msg.nl.nlmsg_flags = NLM_F_REQUEST;
    genl_request_msg.nl.nlmsg_seq = GENLMSG_INIT_SEQ;
    genl_request_msg.nl.nlmsg_pid = getpid();
    genl_request_msg.nl.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    genl_request_msg.genl.cmd = CTRL_CMD_GETFAMILY;
    genl_request_msg.genl.version = 0x1;
    nl_attr = (struct nlattr *) GENLMSG_DATA(&genl_request_msg);
    nl_attr->nla_type = CTRL_ATTR_FAMILY_NAME;
    nl_attr->nla_len = strlen(TCPSPDTEST_GENL_FAMILY_NAME) + 1 + NLA_HDRLEN;
    strcpy(NLA_DATA(nl_attr), TCPSPDTEST_GENL_FAMILY_NAME);

    genl_request_msg.nl.nlmsg_len += NLMSG_ALIGN(nl_attr->nla_len);

    /* Send Family ID request message to netlink controller */
    len = sendto(nl_fd, (char *) &genl_request_msg, genl_request_msg.nl.nlmsg_len, 0, (struct sockaddr *)nl_address, sizeof(*nl_address));
    if (len != genl_request_msg.nl.nlmsg_len)
    {
        perror("Failed ! to sendto TCPSPDTEST GENL Family ID request\n");
        return -1;
    }

    /* Wait for kernel response message */
    len = recv(nl_fd, &genl_response_msg, sizeof(genl_response_msg), 0);
    if (len < 0)
    {
        perror("Failed ! to recv TCPSPDTEST GENL Family ID response\n");
        return -1;
    }

    /* Validate response message */
    if (!NLMSG_OK((&genl_response_msg.nl), len))
    {
        printf("Error ! Received TCPSPDTST GENL Family ID response Invalid message\n");
        return -1;
    }
    if (NLMSG_ERROR == genl_response_msg.nl.nlmsg_type)
    {
        printf("Error ! Received TCPSPDTST GENL Family ID response Error\n");
        return -1;
    }

    /* Extract family ID */
    nl_attr = (struct nlattr *) GENLMSG_DATA(&genl_response_msg);
    nl_attr = (struct nlattr *) ((char *) nl_attr + NLA_ALIGN(nl_attr->nla_len));
    if (CTRL_ATTR_FAMILY_ID != nl_attr->nla_type)
    {
        printf("Error ! Received TCPSPDTST GENL Family ID wrong msg type\n");
        return -1;
    }

    nl_family_id = (int)*(__u16 *) NLA_DATA(nl_attr);

    return 0;
}

int tcpspdtest_genl_init(uint8_t *stream_idx)
{
    struct sockaddr_nl nl_address;
    int nl_fd_local;
    uint32_t nlmsg_seq_local;
    uint8_t num_streams;
    uint8_t is_alloc_family_id = 0;
    int rc = -1;

    /* Open the netlink socket */
    nl_fd_local = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (nl_fd_local < 0)
    {
        perror("Failed ! to open TCPSPDTEST GENL socket\n");
        return -1;
    }

    /* Bind the netlink socket */
    memset(&nl_address, 0, sizeof(nl_address));
    nl_address.nl_family = AF_NETLINK;
    nl_address.nl_groups = 0;
    if (bind(nl_fd_local, (struct sockaddr *) &nl_address, sizeof(nl_address)) < 0)
    {
        perror("Failed ! to bind TCPSPDTEST GENL socket\n");
        goto exit;
    }

    if (-1 == nl_family_id)
    {
        rc = tcpspdtest_genl_get_family_id(nl_fd_local, &nl_address);
        if (rc)
            goto exit;
        is_alloc_family_id = 1;
    }

    nlmsg_seq_local = GENLMSG_INIT_SEQ;

    rc = tcpspdtest_stream_idx(stream_idx, &num_streams, nl_fd_local, &nlmsg_seq_local, TCPSPDTEST_GENL_CMD_PARAM_ALLOC);
    if (rc)
    {
        printf("Error ! Failed to allocate stream_idx\n");
        goto exit;
    }
    //printf("Allocate stream_idx:%d, num_streams:%d\n", *stream_idx, num_streams);

    nl_fd[*stream_idx] = nl_fd_local;
    nlmsg_seq[*stream_idx] = nlmsg_seq_local;

exit:
    if (rc)
    {
        close(nl_fd_local);
        if (is_alloc_family_id)
            nl_family_id = -1;
    }

    return rc;
}

int tcpspdtest_genl_shutdown(uint8_t stream_idx, uint8_t *num_streams)
{
    int rc;

    VALIDATE_IDX(stream_idx);

    if (nl_fd[stream_idx] == -1)
    {
        printf("Error ! genetlink for stream_idx:%u already shutdown\n", stream_idx);
        return -1;
    }

    rc = tcpspdtest_stream_idx(&stream_idx, num_streams, -1, NULL, TCPSPDTEST_GENL_CMD_PARAM_FREE);
    if (rc)
    {
        printf("Error ! Failed to free stream_idx\n");
        return -1;
    }

    //printf("Free stream_idx:%u, num_streams:%u\n", stream_idx, *num_streams);

    if (0 == *num_streams)
        nl_family_id = -1;

    close(nl_fd[stream_idx]);
    nl_fd[stream_idx] = -1;
    nlmsg_seq[stream_idx] = 0;

    return 0;
}

int tcpspdtest_genl_send_request_own_msg(tcpspdtest_genl_req_msg_t *req_msg, int nl_fd_local, uint32_t *nlmsg_seq_local)
{
    tcpspdtest_genl_family_msg_t genl_request_msg;
    struct sockaddr_nl nl_address;
    struct nlattr *nl_attr;
    int len = sizeof(*req_msg);
    int genl_fd = (nl_fd_local == -1 ? nl_fd[req_msg->stream_idx] : nl_fd_local);

    if (-1 == genl_fd)
        return -1;

    if (TCPSPDTEST_GENL_MAX_MSG_LEN < len)
    {
        printf("Error !, Try to send too long GENL msg len:%d. Max:%d\n", len, TCPSPDTEST_GENL_MAX_MSG_LEN);
        return -1;
    }

    /* Send request message to kernel */
    memset(&genl_request_msg, 0, sizeof(genl_request_msg));

    genl_request_msg.nl.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    genl_request_msg.nl.nlmsg_type = nl_family_id;
    genl_request_msg.nl.nlmsg_flags = NLM_F_REQUEST;
    genl_request_msg.nl.nlmsg_seq = (nlmsg_seq_local == NULL ?  nlmsg_seq[req_msg->stream_idx] : *nlmsg_seq_local);
    genl_request_msg.nl.nlmsg_pid = getpid();
    genl_request_msg.genl.cmd = TCPSPDTEST_GENL_FAMILY_MSG_OWN_MSG;

    nl_attr = (struct nlattr *) GENLMSG_DATA(&genl_request_msg);
    nl_attr->nla_type = TCPSPDTEST_GENL_POLICY_OWN_MSG;
    nl_attr->nla_len = len + NLA_HDRLEN; /* Message length */
    memcpy(NLA_DATA(nl_attr), req_msg, len);
    genl_request_msg.nl.nlmsg_len += NLMSG_ALIGN(nl_attr->nla_len);

    memset(&nl_address, 0, sizeof(nl_address));
    nl_address.nl_family = AF_NETLINK;

    /* Send request message to kernel */
    len = sendto(genl_fd, (char *) &genl_request_msg, genl_request_msg.nl.nlmsg_len, 0, (struct sockaddr *) &nl_address, sizeof(nl_address));
    if (len != genl_request_msg.nl.nlmsg_len)
    {
        perror("sendto()");
        printf("Failed ! to sendto TCPSPDTEST GENL request message to kernel\n");
        return -1;
    }

    return 0;
}

//#ib#todo#: recv with timeout
int tcpspdtest_genl_recv_response_own_msg(tcpspdtest_genl_resp_msg_t *resp_msg, int nl_fd_local, uint32_t *nlmsg_seq_local)
{
    tcpspdtest_genl_family_msg_t genl_response_msg;
    struct nlattr *nl_attr;
    int len;
    int genl_fd = (nl_fd_local == -1 ? nl_fd[resp_msg->stream_idx] : nl_fd_local);

    if (-1 == genl_fd)
        return -1;

    memset(&genl_response_msg, 0, sizeof(genl_response_msg));

    /* Wait for response kernel message */
    len = recv(genl_fd, &genl_response_msg, sizeof(genl_response_msg), 0);
    if (len < 0)
    {
        perror("Failed ! to recv TCPSPDTEST GENL kernel response\n");
        return -1;
    }

    /* Validate response message */
    if (NLMSG_ERROR == genl_response_msg.nl.nlmsg_type)
    {
        printf("Error ! receiving TCPSPDTEST GENL kernel response. NACK Received\n");
        return -1;
    }

    if (!NLMSG_OK((&genl_response_msg.nl), len))
    {
        printf("Error ! receiving TCPSPDTEST GENL kernel response. Invalid Message\n");
        return -1;
    }

    /* Parse reply message */
    len = GENLMSG_PAYLOAD(&genl_response_msg.nl);
    nl_attr = (struct nlattr *) GENLMSG_DATA(&genl_response_msg);

    memcpy(resp_msg, NLA_DATA(nl_attr),sizeof(*resp_msg));

    if (nlmsg_seq_local == NULL)
        nlmsg_seq[resp_msg->stream_idx] = genl_response_msg.nl.nlmsg_seq + 1;
    else
        *nlmsg_seq_local = genl_response_msg.nl.nlmsg_seq + 1;

    return 0;
}

static int tcpspdtest_connect(uint8_t stream_idx, spdt_stream_dir_t dir, spdt_proto_t protocol, spdt_conn_params_t *params)
{
    tcpspdtest_genl_req_msg_t req_msg = {};
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_CONNECT;
    req_msg.stream_params.dir = dir;
    req_msg.stream_params.protocol = protocol;
    req_msg.stream_idx = stream_idx;

    memcpy(&req_msg.stream_params.conn_params, params, sizeof(*params));
    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    }

    return (rc ? rc : resp_msg.status);
}

static int tcpspdtest_disconnect(uint8_t stream_idx)
{
    tcpspdtest_genl_req_msg_t   req_msg = {};
    int rc;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_DISCONNECT;
    req_msg.stream_idx = stream_idx;

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (rc)
    {
        printf("Error ! TCPSPDTEST GENL failed to Disconnect stream_idx:%u\n", stream_idx);
        return -1;
    }

    req_msg.cmd = TCPSPDTEST_GENL_CMD_RELEASE;
    req_msg.stream_idx = stream_idx;
    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (rc)
    {
        printf("Error ! TCPSPDTEST GENL failed to Release stream_idx:%u\n", stream_idx);
        return -1;
    }

    return 0;
}

static int tcpspdtest_download(uint8_t stream_idx, spdt_proto_t protocol, spdt_rx_params_t *rx_params)
{
    tcpspdtest_genl_req_msg_t   req_msg = {};
    tcpspdtest_genl_resp_msg_t  resp_msg;
    int rc;
    char *file_name = rx_params->proto.tcp.file_name, *host_str = rx_params->proto.tcp.host_name;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_DNLD;
    req_msg.stream_params.protocol = protocol;
    req_msg.stream_idx = stream_idx;
    switch (protocol)
    {
    case SPDT_FTP:
    case SPDT_IPERF3_TCP:
        req_msg.dn_up_size = rx_params->proto.tcp.size;
        break;
    case SPDT_HTTP:
        if (!file_name || strlen(file_name) > sizeof(req_msg.file_name))
        {
            printf("[%s] Wrong HTTP file name length %d\n", __func__, file_name ? strlen(file_name) : "0");
            return -1;
        }
        strcpy(req_msg.file_name, file_name);

        if (!host_str || strlen(host_str) > sizeof(req_msg.host_str))
        {
            printf("[%s] Wrong HTTP host name length %d\n", __func__, host_str ? strlen(host_str) : "0");
            return -1;
        }
        strcpy(req_msg.host_str, host_str);
        break;
    default:
        printf("[%s] Error! Not supported download protocol %d\n", __func__, protocol);
        return -1;
    }

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    }

    return (rc ? rc : resp_msg.status);
}

static int tcpspdtest_upload(uint8_t stream_idx, spdt_proto_t protocol, spdt_tx_params_t *tx_params)
{
    tcpspdtest_genl_req_msg_t   req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_UPLOAD;
    req_msg.stream_params.protocol = protocol;
    req_msg.stream_idx = stream_idx;

    if (protocol == SPDT_HTTP ||  protocol == SPDT_FTP)
    {
        if (!tx_params->proto.tcp.uri)
        {
            req_msg.file_name[0] = '*';
            req_msg.file_name[1] = '\0';
        }
        else if (strlen(tx_params->proto.tcp.uri) < sizeof(req_msg.file_name))
            strncpy(req_msg.file_name, tx_params->proto.tcp.uri, sizeof(req_msg.file_name));
        else
        {
            printf("[%s] Wrong HTTP URI length %d\n", __func__, strlen(tx_params->proto.tcp.uri));
            return -1;
        }
    }

    if (protocol == SPDT_HTTP ||  protocol == SPDT_FTP || protocol == SPDT_IPERF3_TCP)
        req_msg.dn_up_size = tx_params->proto.tcp.size;
    else
    {
        printf("Error ! upload protocol:%d currently not supported\n", protocol);
        return -1;
    }

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    }

    return (rc ? rc : resp_msg.status);
}

static int tcpspdtest_get_speed(uint8_t stream_idx, int blocking, spdt_stat_t *stat)
{
    tcpspdtest_genl_resp_msg_t resp_msg;
    tcpspdtest_genl_req_msg_t  req_msg = {};
    int rc = 0;

    VALIDATE_IDX(stream_idx);
    req_msg.stream_idx = stream_idx;
    req_msg.cmd = TCPSPDTEST_GENL_CMD_SPEED_REPORT;
    req_msg.cmd_param = (blocking ? TCPSPDTEST_GENL_CMD_PARAM_BLOCKING_END_OF_TEST : TCPSPDTEST_GENL_CMD_PARAM_NONE);

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (rc)
    {
        printf("Error ! Failed to request speed report\n");
        return -1;
    }
    
    /* Wait for speed report */
    resp_msg.stream_idx = stream_idx;
    rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    if (rc)
    {
        printf("Error ! Failed to get speed report\n");
        return -1;
    }

    memcpy(&stat->proto_ext.tcp_speed_rep, &resp_msg.msg.spd_report, sizeof(tcp_spdt_rep_t));
    
    return rc ? rc : resp_msg.status;
}

static int tcpspdtest_oob_send(uint8_t stream_idx, char* buff, uint32_t length)
{
    tcpspdtest_genl_resp_msg_t resp_msg = {};
    tcpspdtest_genl_req_msg_t oob_msg = {};
    int rc = 0;

    VALIDATE_IDX(stream_idx);
    if (length > TCPSPDTEST_GENL_MAX_FILE_NAME_LEN)
    {
        printf("Error on %s, can send maximum of %d\n",__FUNCTION__, TCPSPDTEST_GENL_MAX_FILE_NAME_LEN);
        return -1;
    }
    oob_msg.stream_idx = stream_idx;
    oob_msg.cmd = TCPSPDTEST_GENL_CMD_OOB_SEND;
    oob_msg.dn_up_size = length;
    memcpy(oob_msg.file_name, buff, length);
    rc = tcpspdtest_genl_send_request_own_msg(&oob_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    }

    return (rc ? rc : resp_msg.status);
}

int tcpspdtest_stream_idx(uint8_t *stream_idx, uint8_t *num_streams, int nl_fd_local, uint32_t *nlmsg_seq_local, tcpspdtest_genl_cmd_param_t cmd_param)
{
    tcpspdtest_genl_req_msg_t  req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg = { 0 };
    int rc;

    if (cmd_param != TCPSPDTEST_GENL_CMD_PARAM_ALLOC)
        VALIDATE_IDX(*stream_idx);

    req_msg.cmd = TCPSPDTEST_GENL_CMD_STREAM_IDX;
    req_msg.cmd_param = cmd_param;
    req_msg.stream_idx = *stream_idx;

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, nl_fd_local, nlmsg_seq_local);
    if (0 == rc)
    {
        pthread_mutex_lock(&streams_lock);

        if (cmd_param == TCPSPDTEST_GENL_CMD_PARAM_ALLOC)
            streams_num_cnt++;
        else
            streams_num_cnt--;
        *num_streams = streams_num_cnt;

        pthread_mutex_unlock(&streams_lock);

		if (cmd_param == TCPSPDTEST_GENL_CMD_PARAM_ALLOC)
        {
            resp_msg.stream_idx = *stream_idx;
            rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, nl_fd_local, nlmsg_seq_local);
            if (0 == rc)
            {
                *stream_idx = resp_msg.stream_idx;
                *num_streams = resp_msg.num_streams;
            }
        }
    }    
    
    return (rc ? rc : resp_msg.status);
}

int tcpspdtest_protocol(uint8_t stream_idx, spdt_proto_t *protocol, tcpspdtest_genl_cmd_param_t cmd_param)
{
    tcpspdtest_genl_req_msg_t  req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_PROTOCOL;
    req_msg.cmd_param = cmd_param;
    req_msg.stream_params.protocol = *protocol;
    req_msg.stream_idx = stream_idx;
    
    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
        if (0 == rc)
            *protocol = resp_msg.msg.stream_params.protocol;
        else
            printf("[%u] fail receive\n", stream_idx);
    }
    else
        printf("[%u] fail send\n", stream_idx);
    
    return (rc ? rc : resp_msg.status);
}

int tcpspdtest_stream_params(uint8_t stream_idx, spdt_stream_params_t *stream_params, tcpspdtest_genl_cmd_param_t cmd_param)
{
    tcpspdtest_genl_req_msg_t  req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_STREAM_PARAMS;
    req_msg.cmd_param = cmd_param;
    req_msg.stream_idx = stream_idx;
    memcpy(&req_msg.stream_params, stream_params, sizeof(req_msg.stream_params));
    
    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
        if (0 == rc && TCPSPDTEST_GENL_CMD_PARAM_GET == cmd_param)
            memcpy(stream_params, &resp_msg.msg.stream_params, sizeof(*stream_params));
    }    
    
    if (rc)
        printf("Error ! Failed to %s stream_params for stream_idx:%u, rc:%d\n",
               (cmd_param == TCPSPDTEST_GENL_CMD_PARAM_GET ? "GET" : "SET"), stream_idx, rc);

    return (rc ? rc : resp_msg.status);
}

int tcpspdtest_num_streams(uint8_t stream_idx, uint8_t *num_streams, uint8_t *num_udp_streams)
{
    tcpspdtest_genl_req_msg_t  req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);
    req_msg.cmd = TCPSPDTEST_GENL_CMD_NUM_STREAMS;
    req_msg.stream_idx = stream_idx;
    
    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
        if (0 == rc)
        {
            if (num_streams)
                *num_streams = resp_msg.num_streams;
            if (num_udp_streams)
                *num_udp_streams = resp_msg.num_udp_streams;
        }
    }    
    
    if (rc)
        printf("Error ! Failed to get num of streams, rc:%d\n", rc);

    return (rc ? rc : resp_msg.status);
}

int udpspdt_init_nf_hooks(uint8_t stream_idx)
{
    tcpspdtest_genl_req_msg_t  req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);

    req_msg.cmd = UDPSPDTEST_GENL_CMD_INIT;
    req_msg.stream_idx = stream_idx;

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    }    

    if (rc)
        printf("Error ! Failed to Init udpspdt nf_hooks, rc:%d\n", rc);

    return (rc ? rc : resp_msg.status);
}

int udpspdt_uninit_nf_hooks(uint8_t stream_idx)
{
    tcpspdtest_genl_req_msg_t  req_msg;
    tcpspdtest_genl_resp_msg_t resp_msg;
    int rc;

    VALIDATE_IDX(stream_idx);

    req_msg.cmd = UDPSPDTEST_GENL_CMD_UNINIT;
    req_msg.stream_idx = stream_idx;

    rc = tcpspdtest_genl_send_request_own_msg(&req_msg, -1, NULL);
    if (0 == rc)
    {
        resp_msg.stream_idx = stream_idx;
        rc = tcpspdtest_genl_recv_response_own_msg(&resp_msg, -1, NULL);
    }    

    if (rc)
        printf("Error ! Failed to Uninit udpspdt nf_hooks, rc:%d\n", rc);

    return (rc ? rc : resp_msg.status);
}

spdt_proto_ops_t spdt_tcp_ops = {
    .init = NULL,
    .uninit = NULL,
    .connect = tcpspdtest_connect,
    .disconnect = tcpspdtest_disconnect,
    .msg_send = tcpspdtest_oob_send,
    .stats_get = tcpspdtest_get_speed,
    .recv_start = tcpspdtest_download,
    .recv_stop = NULL,
    .send_start = tcpspdtest_upload,
    .send_stop = NULL
};
