#if defined(CONFIG_BCM_OVS_MCAST)
/* 
 * <:label-BRCM:2019:NONE/NONE:standard
 * 
 :>   
 * Copyright 2019 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include "bcm_mcast.h"
#include "bcm_mcast_api_public.h"
#include "vswitch-idl.h"
#include "ovs_bcmmcast.h"
#include "poll-loop.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(ovs_bcmmcast);

/* Data structure to store OVS<->MCPD related info */
typedef struct ovs_mcpd_info_struct{
    int  sock_conn;
    int  sock;
    char *sock_buff;
    char *send_buff;
    t_ovs_mcpd_msg brcfg;
    int tx_sock;
}t_ovs_mcpd_info;

t_ovs_mcpd_info g_ovs_mcpd_info;

bridge_manage_mcastsnoopentry_hook_t bridge_manage_mcastsnoopentry_hook = NULL;

/* Send message to MCPD */
static int ovs_mcpd_send_msg(void *pBuf, int buf_size)
{
    int ret;
    if ( g_ovs_mcpd_info.tx_sock <= 0 )
    {
        g_ovs_mcpd_info.tx_sock = bcm_mcast_api_stream_client_sock_connect(OVS_2_MCPD_SOCK_PORT);
        if (g_ovs_mcpd_info.tx_sock <= 0) 
        {
            VLOG_ERR("%s bcm_mcast_api_stream_client_sock_connect() failed\\n", __func__);
            return -1;
        }
    }

    ret = bcm_mcast_api_stream_sock_send(g_ovs_mcpd_info.tx_sock, pBuf, buf_size);
    if ( ret < buf_size )
    {
        if ( ret == -1 )
        {
            if ( errno != EAGAIN )
            {
                VLOG_ERR("%s bcm_mcast_api_stream_sock_send() failed %s\\n",
                         __func__, strerror(errno));
                close(g_ovs_mcpd_info.tx_sock);
                g_ovs_mcpd_info.tx_sock = -1;
            }
            return -1;
        }
        VLOG_ERR("%s sent length %d less than buf_size %d\n", __func__, ret, buf_size);
        return -1;
    }
    return 0;
}

/* Initialize the socket used for MCPD to OVS communtication */
int ovs_mcpd_socket_init(void)
{
    struct sockaddr_in sa;
    int             sd;
    int             flags;
    int             optval = 1;
    socklen_t       optlen = sizeof(optval);
  
    g_ovs_mcpd_info.sock_conn = -1;
    g_ovs_mcpd_info.sock = -1;
    g_ovs_mcpd_info.tx_sock = -1;

    g_ovs_mcpd_info.sock_buff = (char *)malloc(OVS_MCPD_RX_BUF_SIZE);
    if ( NULL == g_ovs_mcpd_info.sock_buff )
    {
       VLOG_ERR("%s mcpd socket buffer allocation failed\n", __func__);
    }

    g_ovs_mcpd_info.send_buff = (char *)malloc(OVS_MCPD_SEND_BUF_SIZE);
    if ( NULL == g_ovs_mcpd_info.send_buff )
    {
       VLOG_ERR("%s mcpd send buffer allocation failed\n", __func__);
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        VLOG_ERR("%s socket() error, %s", __func__, strerror(errno));
        return -1;
    }

    /* Allow reusing the socket immediately when application is restarted */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen))
    {
        VLOG_ERR("%s setsockopt error %s", __func__, strerror(errno));
    }

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port        = htons( (unsigned short)MCPD_2_OVS_SOCK_PORT);
    if((bind(sd, (struct sockaddr *)&sa, sizeof(sa))) == -1)
    {
        VLOG_ERR("%s bind() to port %d error, %s", __func__, 
                   MCPD_2_OVS_SOCK_PORT, strerror(errno));
        close(sd);
        return -1;
    }

    /* Support a maximum of 3 connections. MCPD is the only client
       today. Do not see a scenario that would require more than 1
       connection from MCPD but following MCPCTL model to support 3
       connections */
    if ((listen(sd, 3)) == -1)
    {
        VLOG_ERR("%s listen() to port %d error, %s", __func__, 
                   MCPD_2_OVS_SOCK_PORT, strerror(errno));
        close(sd);
        return -1;
    }

    flags = fcntl(sd, F_GETFL, 0);
    if(flags < 0)
    {
        VLOG_ERR("%s cannot retrieve socket flags. error=%s", 
                   __func__, strerror(errno));
    }
    if ( fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0 )
    {
        VLOG_ERR("%s cannot set socket to non-blocking. error=%s", 
                   __func__, strerror(errno));
    }

    g_ovs_mcpd_info.sock = sd;

    VLOG_INFO("%s successful\n", __func__);
    return(0);
}

/* UnInitialize the socket used for MCPD to OVS communtication */
int ovs_mcpd_socket_uninit(void)
{
    struct sockaddr_in sa;
    int             sd;
    int             flags;
    int             optval = 1;
    socklen_t       optlen = sizeof(optval);
  
    free(g_ovs_mcpd_info.sock_buff);
    free(g_ovs_mcpd_info.send_buff);

    if ( g_ovs_mcpd_info.sock_conn != -1 )
    {
        close(g_ovs_mcpd_info.sock_conn);
    }
    g_ovs_mcpd_info.sock_conn = -1;

    if ( g_ovs_mcpd_info.sock != -1 )
    {
        close(g_ovs_mcpd_info.sock);
    }
    g_ovs_mcpd_info.sock = -1;

    if ( g_ovs_mcpd_info.tx_sock != -1 )
    {
        close(g_ovs_mcpd_info.tx_sock);
    }
    g_ovs_mcpd_info.tx_sock = -1;

    VLOG_INFO("%s successful\n", __func__);
    return(0);
}

/* Initialize objects */
int ovs_mcpd_init(void)
{
    ovs_mcpd_socket_init();

    g_ovs_mcpd_info.brcfg.msghdr.msgtype = OVS_MCPD_MSG_BRIDGE_CONFIG_UPDATE;
    g_ovs_mcpd_info.brcfg.msghdr.msglen = sizeof(g_ovs_mcpd_info.brcfg);
    strncpy(g_ovs_mcpd_info.brcfg.msgmarker, 
            OVS_MCPD_MSG_MARKER,
            sizeof(g_ovs_mcpd_info.brcfg.msgmarker));

    return 0;
}

int ovs_mcpd_exit(void)
{
    ovs_mcpd_socket_uninit();

    return 0;
}

/* Accept incoming connection request from MCPD */
int ovs_mcpd_socket_accept(void)
{
    struct sockaddr clientAddr;
    unsigned int sockAddrSize;
    int sd;
    int flags;

    if ( g_ovs_mcpd_info.sock_conn != -1 )
    {
        VLOG_ERR("%s Only one connection available\n", __func__);
        return -1;
    }

    sockAddrSize = sizeof(clientAddr);
    if ((sd = accept(g_ovs_mcpd_info.sock, (struct sockaddr *)&clientAddr, &sockAddrSize)) < 0) {
        if (errno != EAGAIN) {
            VLOG_ERR("%s accept connection failed. errno=%d %s\n", __func__, errno, strerror(errno));
        }
        return -1;
    }
  
    flags = fcntl(sd, F_GETFL, 0);
    if(flags < 0) {
        VLOG_ERR("%s cannot retrieve socket flags. errno=%d\n", __func__, errno);
    }
    if ( fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0 ) {
        VLOG_ERR("%s cannot set socket to non-blocking. errno=%d\n", __func__, errno);
    }

    g_ovs_mcpd_info.sock_conn = sd;

    return 0;
}

/* Receive incoming messages from MCPD and process them */
int ovs_mcpd_socket_receive(void)
{
    int length;
    int ret = 0; 
    fd_set rfds;
    struct timeval time_out;
    int msghdrlen;
    int msgdatalen;
    t_ovs_mcpd_msg_hdr msgHdr;
    t_ovs_mcpd_msg_hdr *pMsgHdr = &msgHdr;
    char msg_marker[OVS_MCPD_MSG_MARKER_SIZE_BYTES];
    char msg_marker_buf[OVS_MCPD_MSG_MARKER_SIZE_BYTES];
    int msg_mark_idx=0;
    int msg_detected=0;
    int bufpos;
    int msg_recv_complete=0;

    FD_ZERO(&rfds);
    FD_SET(g_ovs_mcpd_info.sock_conn, &rfds);

    /* Set timeout to 0 since we are just polling for the presence of a message */
    time_out.tv_sec = 0;
    time_out.tv_usec = 0;

    strncpy(msg_marker, OVS_MCPD_MSG_MARKER, sizeof(msg_marker));
    while (select(g_ovs_mcpd_info.sock_conn+1, &rfds, NULL, NULL, &time_out) > 0) 
    {
        /* Read 1 byte at a time until we can figure out the message start */
        while ((length = recv(g_ovs_mcpd_info.sock_conn, 
                    &msg_marker_buf[msg_mark_idx], 1, 0)) >= 0)
        {
            if ( length == 0 )
            {
                close(g_ovs_mcpd_info.sock_conn);
                g_ovs_mcpd_info.sock_conn = -1;

                /* Most likely the mcpd daemon has stopped. Close the tx socket as well */
                close(g_ovs_mcpd_info.tx_sock);
                g_ovs_mcpd_info.tx_sock = -1;
                break;
            }
            else if ( msg_marker_buf[msg_mark_idx] == msg_marker[msg_mark_idx] )
            {
                /* Check if we have detected a complete pattern */ 
                if (msg_mark_idx == (OVS_MCPD_MSG_MARKER_SIZE_BYTES - 1))
                {
                    msg_detected = 1;
                }
                else
                {
                    /* Complete pattern not yet detected. Match the next byte */
                    msg_mark_idx++;
                    continue;
                }
            }
            else
            {
                /* Msg marker mismatch. Restart the matching process  */
                msg_mark_idx =0;
                continue;
            }

            if ( msg_detected )
            {
                /* Now reset the msg marker and msg_detected flag and start reading the message */
                msg_detected = 0;
                msg_mark_idx = 0;

                /* Read message header first */
                if ( (msghdrlen = recv(g_ovs_mcpd_info.sock_conn, &msgHdr,
                                       sizeof(t_ovs_mcpd_msg_hdr), 0)) >= 0 )
                {
                    if ( msghdrlen == 0 )
                    {
                        VLOG_ERR("%s ovs-mcpd connection socket %d closed\n",
                                 __func__, g_ovs_mcpd_info.sock_conn);
                        close(g_ovs_mcpd_info.sock_conn);
                        g_ovs_mcpd_info.sock_conn = -1;

                        /* Most likely the mcpd daemon has stopped. Close the tx socket as well */
                        close(g_ovs_mcpd_info.tx_sock);
                        g_ovs_mcpd_info.tx_sock = -1;
                        return;
                    }
                    else if ( msghdrlen != sizeof(t_ovs_mcpd_msg_hdr) )
                    {
                        VLOG_ERR("%s Unusual msghdrlen msghdrlen %d expected %d\n",
                                 __func__, msghdrlen, sizeof(t_ovs_mcpd_msg_hdr));
                        continue;
                    }
                    
                    VLOG_DBG("%s Received hdr msgtype %d of length %d msgdatalen %d\n",
                              __func__, pMsgHdr->msgtype, msghdrlen, pMsgHdr->msglen);

                    if ( pMsgHdr->msgtype != MCPD_OVS_MSG_MANAGE_SNOOP_ENTRY )
                    {
                        /* Invalid msgtype. Discard this message and detect next msg  */
                        VLOG_ERR("%s Invalid msgtype %d\n", __func__, pMsgHdr->msgtype);
                        continue;
                    }

                    msgdatalen = pMsgHdr->msglen - msghdrlen - OVS_MCPD_MSG_MARKER_SIZE_BYTES;
                    if ( msgdatalen < 0 || msgdatalen > OVS_MCPD_MSG_DATA_BUF_SIZE )
                    {
                        /* Unusual msg data length. Discard this message and detect next msg  */
                        VLOG_ERR("%s unusual msgdata length msgdatalen %d OVS_MCPD_RX_BUF_SIZE %d\n",
                                 __func__, msgdatalen, OVS_MCPD_RX_BUF_SIZE);
                        continue;
                    }

                    /* Now receive the msg data */
                    bufpos = 0;
                    msg_recv_complete = 0;
                    while ( !msg_recv_complete )
                    {
                        if ( (length = recv(g_ovs_mcpd_info.sock_conn,
                                            g_ovs_mcpd_info.sock_buff + bufpos,
                                            msgdatalen, 0)) >= 0 )
                        {
                            VLOG_DBG("%s Received msgdata of length %d msgdatalen %d\n",
                                      __func__,  length, pMsgHdr->msglen);

                            if ( length == 0 )
                            {
                                VLOG_ERR("%s ovs-mcpd connection socket %d closed\n",
                                 __func__, g_ovs_mcpd_info.sock_conn);
                                close(g_ovs_mcpd_info.sock_conn);
                                g_ovs_mcpd_info.sock_conn = -1;

                                /* Most likely the mcpd daemon has stopped. Close the tx socket as well */
                                close(g_ovs_mcpd_info.tx_sock);
                                g_ovs_mcpd_info.tx_sock = -1;
                                return;
                            }
                            else if ( length > msgdatalen )
                            {
                                /* Unexpected length. Discard this message and read next */
                                VLOG_ERR("Received datalength %d > expected %d\n", length, msgdatalen);
                                break;
                            }
                            else if ( length != msgdatalen )
                            {
                                VLOG_ERR("%s data recv() error length %d expected %d %d: %s\n",
                                         __func__, msgdatalen, length, errno, strerror(errno));
                                msgdatalen = msgdatalen - length;
                                bufpos = bufpos + length;
                                continue;
                            }
                            msg_recv_complete = 1;

                            switch ( pMsgHdr->msgtype )
                            {
                                case MCPD_OVS_MSG_MANAGE_SNOOP_ENTRY:
                                {
                                    t_ovs_mcpd_msg_snoopentry *pSnoopEntry =
                                        (t_ovs_mcpd_msg_snoopentry *)g_ovs_mcpd_info.sock_buff;

                                    VLOG_DBG("%s MCPD_OVS_MSG_MANAGE_SNOOP_ENTRY addentry %d\n",
                                             __func__, pSnoopEntry->addentry);

                                    if ( bridge_manage_mcastsnoopentry_hook ) 
                                    {
                                        bridge_manage_mcastsnoopentry_hook(pSnoopEntry->brname,
                                                                           pSnoopEntry->ipv4grp.s_addr,
                                                                           &pSnoopEntry->ipv6grp,
                                                                           pSnoopEntry->vlan,
                                                                           pSnoopEntry->portname,
                                                                           pSnoopEntry->is_igmp,
                                                                           pSnoopEntry->ipv4src.s_addr,
                                                                           &pSnoopEntry->ipv6src,
                                                                           pSnoopEntry->addentry);
                                    }
                                    break;
                                }
                                default:
                                    VLOG_ERR("%s Unknown msgtype %d length %d\n",
                                             __func__, pMsgHdr->msgtype, length);
                                    break;
                            }
                        } 
                        else
                        {
                            if ( errno != EAGAIN )
                            {
                                /* Error encountered. Discard this message and read next */
                                VLOG_ERR("%s data recv error\n", __func__);
                            }

                            /* break the data recv loop and go to the outer loop */
                            break; 
                        }
                    }
                }
                else
                {
                    if ( errno != EAGAIN )
                    {
                        /* Error encountered. Discard this message and read next */
                        VLOG_ERR("%s recv error\n", __func__);
                    }
                    continue;
                }
            }
        }
    }
    return ret;
}

/* Add MCPD->OvS communication socket in the vswitchd
   socket poll list */
void ovs_mcpd_wait(void)
{
    poll_fd_wait(g_ovs_mcpd_info.sock, POLLIN);
}

/* Function called from vswitchd daemon when MCPD->OVS
   socket is ready for I/O */
int ovs_mcpd_comm_run(void)
{
    if(g_ovs_mcpd_info.sock_conn == -1)
        ovs_mcpd_socket_accept();

    if(g_ovs_mcpd_info.sock_conn != -1)
    {
        ovs_mcpd_socket_receive();
    }
    return 0;
}

/* Notify MCPD about a received IGMP/MLD packet and
   wait for a response. MCPD responds with OK or NOK.
   OK - OvS can continue processing this IGMP/MLD pkt
   NOK - OvS ignores this pkt
   */
void ovs_mcpd_notify(unsigned char *pkt,
                     int len,
                     int rxdev_ifi,
                     int to_acceldev_ifi,
                     uint32_t v4_src,
                     struct in6_addr *v6_src,
                     uint16_t vlan,
                     char *brname,
                     int l4offset,
                     int isigmp)
{
    int mcpdresp = MCPD_OVS_RESP_NOK;
    int     buf_size;
    t_ovs_mcpd_msg *pMcpdMsg;
    t_BCM_MCAST_PKT_INFO *pInfo;
    int igmp_mld_len;

    buf_size = sizeof(t_ovs_mcpd_msg) + len;
    VLOG_DBG("%s len %d sizeof(t_ovs_mcpd_msg) %d buf_size %d l4offset %d\n", 
              __func__, len, sizeof(t_ovs_mcpd_msg), buf_size, l4offset);

    memset(g_ovs_mcpd_info.send_buff, 0, OVS_MCPD_SEND_BUF_SIZE);
    pMcpdMsg = (t_ovs_mcpd_msg *)g_ovs_mcpd_info.send_buff;
    pInfo = &(pMcpdMsg->pktInfo);

#define ETH_ALEN 6
    memcpy(pInfo->repMac, &pkt[ETH_ALEN], ETH_ALEN);
    pInfo->rxdev_ifi = rxdev_ifi;
    pInfo->to_acceldev_ifi = to_acceldev_ifi;

    if (isigmp) {
        pMcpdMsg->msghdr.msgtype = OVS_MCPD_MSG_IGMP;
        pInfo->ipv4rep.s_addr = v4_src;
    }
    else {
        pMcpdMsg->msghdr.msgtype = OVS_MCPD_MSG_MLD;
        memcpy(&pInfo->ipv6rep, v6_src, sizeof(*v6_src));
    }

    VLOG_DBG("%s src address is 0x%x vlan %d\n", __func__, pInfo->ipv4rep.s_addr, vlan);
    pInfo->lanppp = 0;
    if (vlan) {
        pInfo->tci = vlan;
    } else {
        pInfo->tci = 0;
    }
    pInfo->packetIndex = -1;
    igmp_mld_len = len - l4offset;
    buf_size -= l4offset;
    pInfo->data_len = igmp_mld_len;
    if ( igmp_mld_len <= 0 || 
         (igmp_mld_len > (OVS_MCPD_SEND_BUF_SIZE - sizeof(t_ovs_mcpd_msg) - OVS_MCPD_MSG_MARKER_SIZE_BYTES)) )
    {
        VLOG_ERR("%s ERROR igmp_mld_len %d (OVS_MCPD_SEND_BUF_SIZE - sizeof(t_ovs_mcpd_msg)) %d\n",
              __func__,  igmp_mld_len, OVS_MCPD_SEND_BUF_SIZE - sizeof(t_ovs_mcpd_msg));
        return;
    }
    if ( pInfo->data_len > 0 && buf_size <= OVS_MCPD_SEND_BUF_SIZE )
    {
        memcpy(&pInfo->pkt[0], (unsigned char *)(pkt + l4offset), igmp_mld_len);
        if ( bcm_mcast_api_ifname_to_idx(brname, &(pInfo->parent_ifi)) == 0 )
        {
           pMcpdMsg->msghdr.msglen = buf_size;
           strncpy(pMcpdMsg->msgmarker, 
                   OVS_MCPD_MSG_MARKER,
                   sizeof(pMcpdMsg->msgmarker));

           if (ovs_mcpd_send_msg(pMcpdMsg, buf_size) == -1) 
           {
               if ( errno != EAGAIN )
               {
                   VLOG_ERR("%s ovs_mcpd_send_msg error pInfo->pkt[0](igmptype) 0x%x buf_size %d igmp_mld_len %d\n",
                              __func__, pInfo->pkt[0], buf_size, igmp_mld_len);
               }
           }
        } 
        else 
        {
            VLOG_ERR("%s bcm_mcast_api_ifname_to_idx() returned error brname %s\n",
                    __func__, brname);
        }
    }
    else
    {
        VLOG_ERR("%s pInfo->data_len %d len %d l4offset %d buf_size %d OVS_MCPD_SEND_BUF_SIZE %d\n", 
                  __func__, pInfo->data_len, len, l4offset, buf_size, OVS_MCPD_SEND_BUF_SIZE);
    }

    VLOG_DBG("%s len %d mcpdresp %d l4offset %d igmp_mld_len %d buf_size %d sizeof(*pInfo) %d\n",
              __func__, len, mcpdresp, l4offset, igmp_mld_len, buf_size, sizeof(*pInfo));
    return;
}

/* Function called from the bridge on a bridge destroy.
   Notify MCPD with the updated OvS bridge/port info */
void ovs_mcpd_brcfg_brdelete(char *brname)
{
    int i;
    int j;
    int k;
    int ifIndex;
    t_ovs_mcpd_brcfg_info *pBrCfg = &(g_ovs_mcpd_info.brcfg.brcfgmsg);

    if (bcm_mcast_api_ifname_to_idx(brname, &ifIndex) == 0)
    {
        for (i = 0; i < pBrCfg->numbrs; i++) 
        {
            if (pBrCfg->ovs_br[i] == ifIndex) 
            {
                /* Found matching bridge, delete this entry
                   by adjusting the bridge and port array values */
                for (j = i; j < (pBrCfg->numbrs - 1); j++) 
                {
                    pBrCfg->ovs_br[j] = pBrCfg->ovs_br[j+1];
                    pBrCfg->numports[j] = pBrCfg->numports[j+1];

                    for (k = 0; k < pBrCfg->numports[j+1]; k++) 
                    {
                        pBrCfg->ovs_ports[j][k] = pBrCfg->ovs_ports[j+1][k];
                    }
                }

                /* Decrement the number of bridges */
                pBrCfg->numbrs--;
            }
        }
    
        if (pBrCfg->numbrs >= 0) 
        {
            if (ovs_mcpd_send_msg(&(g_ovs_mcpd_info.brcfg), 
                                  sizeof(g_ovs_mcpd_info.brcfg)) == -1)
            {
                VLOG_ERR("%s ovs_mcpd_send_msg returned Error\n", __func__);
            }
        }
        else
        {   
            VLOG_ERR("%s Num bridges(%d) negative\n", __func__, pBrCfg->numbrs);
        }
    }
}

/* Function called from the bridge on a bridge configuration
   change. Notify MCPD with the updated OvS bridge/port info */
void ovs_mcpd_brcfg_update(const struct ovsrec_open_vswitch *cfg)
{
    int i;
    int ifIndex;
    t_ovs_mcpd_brcfg_info *pBrCfg = &(g_ovs_mcpd_info.brcfg.brcfgmsg);

    pBrCfg->numbrs = 0;
    for (i = 0; i < MCPD_MAX_OVS_BRIDGES; i++) 
    {
        pBrCfg->numports[i] = 0;
    }

    if (cfg) {
        for (i = 0; i < cfg->n_bridges; i++) {
            const struct ovsrec_bridge *br_cfg = cfg->bridges[i];
            int j;

            if (bcm_mcast_api_ifname_to_idx(br_cfg->name, &ifIndex) == 0)
            {
                if (pBrCfg->numbrs < MCPD_MAX_OVS_BRIDGES) 
                {
                    pBrCfg->ovs_br[pBrCfg->numbrs] = ifIndex;
                }

                for (j = 0; j < br_cfg->n_ports; j++) {
                    struct ovsrec_port *port_cfg = br_cfg->ports[j];

                    if (bcm_mcast_api_ifname_to_idx(port_cfg->name, &ifIndex) == 0)
                    {
                        if (pBrCfg->numbrs < MCPD_MAX_OVS_BRIDGES &&
                            pBrCfg->numports[pBrCfg->numbrs] < MCPD_MAX_OVS_BRPORTS) 
                        {
                            pBrCfg->ovs_ports[pBrCfg->numbrs][pBrCfg->numports[pBrCfg->numbrs]] = ifIndex;
                        }
                        pBrCfg->numports[pBrCfg->numbrs]++;
                    }
                }
                pBrCfg->numbrs++;
            }
        }
    }
    if (pBrCfg->numbrs > 0) 
    {
        if (ovs_mcpd_send_msg(&(g_ovs_mcpd_info.brcfg), 
                              sizeof(g_ovs_mcpd_info.brcfg)) == -1)
        {
            VLOG_ERR("%s ovs_mcpd_send_msg returned Error\n", __func__);
        }
    }
}
#endif
