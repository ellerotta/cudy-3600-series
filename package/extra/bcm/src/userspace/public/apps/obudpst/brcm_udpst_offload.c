/*
<:copyright-BRCM:2023:DUAL/GPL:standard

   Copyright (c) 2023 Broadcom 
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

#if defined(BRCM_UDPST_OFFLOAD)

/* Below includes coming from udpst.c as-is */

#include "config.h"

#define UDPST
#ifdef __linux__
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#ifdef AUTH_KEY_ENABLE
#include <openssl/hmac.h>
#include <openssl/x509.h>
#endif
#else
#include "../udpst_alt1.h"
#endif
//
#include "cJSON.h"
#include "udpst_common.h"
#include "udpst_protocol.h"
#include "udpst.h"
#include "udpst_control.h"
#include "udpst_data.h"
#include "udpst_srates.h"
#ifndef __linux__
#include "../udpst_alt2.h"
#endif

/* udpst globals need access in brcm offload */
extern struct repository repo;                    // Repository of global data
extern struct configuration conf;                 // Configuration data structure
extern struct connection *conn;                   // Connection table (array)
extern char scratch[STRING_SIZE];                 // General purpose scratch buffer
int outputfd = STDOUT_FILENO;
extern int errConn, monConn;            // Error and monitoring connections


/* BRCM related/needed includes */

#include <sys/mman.h>
#include "spdsvc_api.h"
#include "brcm_udpst_offload.h"

spdsvc_tr471_rx_queue_t *spdsvc_tr471_rx_queue_p = NULL;
struct loadHdr *offload_rx_queue_lHdr = NULL;
int offload_pid = -1;
int offload_event_fd = -1;
int burst_cmpl_event_fd = -1;
int burst_epollFD = -1;
struct epoll_event burst_epollFD_events[MAX_EPOLL_EVENTS];

struct brcm_udpst_offload
{
    int burst_nbr;
    int sendmsg_count;
    int sendmsg_errors;
    int offload_in_progress;  /* spdsvc enable/disable wrapper functions controlled by 
                                 offload_in_progress flag */
};
static struct brcm_udpst_offload offload[MAX_SERVER_CONN] = { };

static spdsvc_direction_t offload_direction = SPDSVC_DIRECTION_US; /* default */
#define DEVICE_FILENAME "/dev/spdsvc"

//#define BRCM_UDPST_DEBUG
#if defined(BRCM_UDPST_DEBUG)
#define brcm_udpst_debug(fmt, arg...) printf(fmt, ##arg)
#else
#define brcm_udpst_debug(fmt, arg...)
#endif

static int udpst_spdsvc_enable(spdsvc_config_t *config_p)
{
    int ret = 0;
    ret = spdsvc_enable(config_p);
    if (!ret)
    {
        struct brcm_udpst_offload *o_p = &offload[config_p->tr471.connindex];
        o_p->offload_in_progress = 1;
        //printf ("enable offload_in_progress=%d\n", o_p->offload_in_progress);
    }
    return ret;
}

static int udpst_spdsvc_disable(int connindex)
{
    int ret = 0;
    struct brcm_udpst_offload *o_p = &offload[connindex];

    //printf ("udpst_spdsvc_disable %d offload %d\n", connindex, o_p->offload_in_progress);

    if (o_p->offload_in_progress)
    {
        spdsvc_config_t config = { };

        config.tr471.connindex = connindex;
        config.mode = SPDSVC_MODE_TR471;
        ret = spdsvc_disable(&config);
    }
    o_p->offload_in_progress = 0;
    return ret;
}

static inline int brcm_offload_in_progress(int connindex)
{
    struct brcm_udpst_offload *o_p = &offload[connindex];
    return o_p->offload_in_progress;
}

/*
 * Helper function to read event fd
 */
static inline int brcm_udpst_offload_read_event_fd(int event_fd)
{
    uint64_t event_count;
    ssize_t s;
    s = read(event_fd, &event_count, sizeof(uint64_t));
    if (s != sizeof(uint64_t))
    {
        printf("Could not read fd %d", event_fd);
        return -1;
    }
    return 0;
}

int brcm_udpst_offload_validate_params(void)
{
    int var;

    if (conf.brcmOffload)
    {
        var = sprintf(scratch, "********Broadcom Offload mode is enabled******** \n");
        var = write(outputfd, scratch, var);
    }
    else
    {
        var = sprintf(scratch, "********Broadcom Offload mode is disabled******** \n");
        var = write(outputfd, scratch, var);
    }

    if (conf.brcmOffload)
    {
#if !defined(BRCM_UDPST_OFFLOAD_MFLOW)
        if ((repo.serverCount > 1) || (conf.minConnCount > 1) || (conf.maxConnections > 1))
        {
            var = sprintf(scratch, "ERROR: Multi-server/Multi-connection mode not supported with offload, use -O to disable offload\n");
            var = write(outputfd, scratch, var);
            return -1;
        }
#endif
        if (conf.randPayload)
        {
            var = sprintf(scratch, "ERROR: Random payload not supported with offload, use -O to disable offload\n");
            var = write(outputfd, scratch, var);
            return -1;
        }

        /* If the max connections exceed the supported number of max offload
         * connections, the offload is disabled for all flows and all the
         * connections/flows will take non-offload path.
         * If there is a need to change the max offlod connections supported,
         * please contact Broadcom with use case scenarios
         */
        if (conf.maxConnCount > BRCM_UDPST_OFFLOAD_CONNS)
        {
            //By default, brcmOffload is enabled=TRUE
            var = sprintf(scratch, "\nWARNING!! No of connections(%d) > offload connections(%d). Offload is diabled for the test.\n",
                          conf.maxConnCount, BRCM_UDPST_OFFLOAD_CONNS);
            var = write(outputfd, scratch, var);
            conf.brcmOffload = FALSE;
        }
    } /* if (brcmoffload) */

    return 0;
}

int brcm_udpst_offload_rx_queue_init(void)
{
    int fd;

    fd = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);
    if (fd >= 0)
    {
        size_t length = spdsvc_tr471_rx_queue_mem_size();
        spdsvc_tr471_rx_queue_p = mmap(NULL,
                                       length,
                                       PROT_READ | PROT_WRITE,
                                       MAP_SHARED,
                                       fd,
                                       0);
        if (spdsvc_tr471_rx_queue_p == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return -1;
        }
        else
        {
            spdsvc_tr471_rx_queue_init(spdsvc_tr471_rx_queue_p);
        }

        close(fd);
    }
    else
    {
        perror("open");
        printf("Could not open %s\n", DEVICE_FILENAME);
        return -1;
    }
    return 0;
}

int brcm_udpst_offload_init(void)
{
    int i;
    struct epoll_event event;
    int ret;
    int profile1_bins[7] = { 5000, 1000, 500, 300, 200, 100 };
    int profile2_bins[7] = { 400, 300, 200, 150, 100, 50 };

    offload_event_fd = eventfd(0, 0);
    if (offload_event_fd == -1)
    {
        perror("eventfd");
        goto handle_error;
    }
    /* Store pointer to avoid conflict with connection index (used by udpst) */
    event.data.ptr = &offload_event_fd;
    event.events = EPOLLIN | EPOLLET;
    ret = epoll_ctl(repo.epollFD, EPOLL_CTL_ADD, offload_event_fd, &event);
    if (ret == -1)
    {
        perror("EPOLL_CTL_ADD");
        goto handle_error;
    }

    //
    // Open burst completion epoll file descriptor for events
    //
    if ((burst_epollFD = epoll_create1(0)) < 0)
    {
        perror("scratch");
        goto handle_error;
    }

    burst_cmpl_event_fd = eventfd(0, 0);
    if (burst_cmpl_event_fd == -1)
    {
        perror("burst_cmpl_event_fd");
        goto handle_error;
    }
    event.data.fd = burst_cmpl_event_fd;
    event.events = EPOLLIN | EPOLLET;
    ret = epoll_ctl(burst_epollFD, EPOLL_CTL_ADD, burst_cmpl_event_fd, &event);
    if (ret == -1)
    {
        perror("EPOLL_CTL_ADD");
        goto handle_error;
    }
    offload_pid = getpid();
    /*
    printf("\tBRCM Offload: efd %d, pid %d burst_epollFD %d burst_cmpl_event_fd %d\n", 
            offload_event_fd, offload_pid, burst_epollFD, burst_cmpl_event_fd);
    */

    memset(&offload[0], 0, (conf.maxConnections * sizeof(struct brcm_udpst_offload)));

    if (brcm_udpst_offload_rx_queue_init())
    {
        perror("brcm_udpst_offload_rx_queue_init error");
        goto handle_error;
    }

    brcm_profile_point_init(UDPST_PROFILE_SEND_LD_TMR, "Send load timer periodicity", profile1_bins, 6);
    brcm_profile_point_init(UDPST_PROFILE_SEND_LD_EXE, "Send load execution time", profile2_bins, 6);
    brcm_profile_point_init(UDPST_PROFILE_RECV_DATA, "Receive data periodicity", profile1_bins, 6);

    return 0; /* Success */

handle_error:
    {
        int var;
        var = sprintf(scratch, "ERROR: brcm_udpst_offload_init failure\n");
        var = write(outputfd, scratch, var);
        brcm_udpst_offload_exit();
    }
    return -1;
}

int brcm_udpst_offload_exit(void)
{
    int i;

    brcm_profile_print_results();

    //printf ("\n offload %d maxconn %d \n", conf.brcmOffload, conf.maxConnections);

    if (conf.brcmOffload == FALSE)
    {
        return 0;
    }
    if (spdsvc_tr471_rx_queue_p)
    {
        spdsvc_tr471_rx_queue_stats(spdsvc_tr471_rx_queue_p);
    }

    for (i = 0; i < conf.maxConnections; i++) udpst_spdsvc_disable(i);

    if (offload_event_fd != -1)
    {
        int ret;
        ret = epoll_ctl(repo.epollFD, EPOLL_CTL_DEL, offload_event_fd, NULL);
        if (ret == -1)
        {
            perror("EPOLL_CTL_DEL");
            return -1;
        }
        close(offload_event_fd);
        offload_event_fd = -1;
    }
    if (burst_cmpl_event_fd != -1)
    {
        int ret;
        ret = epoll_ctl(burst_epollFD, EPOLL_CTL_DEL, burst_cmpl_event_fd, NULL);
        if (ret == -1)
        {
            perror("EPOLL_CTL_DEL");
            return -1;
        }
        close(burst_cmpl_event_fd);
        burst_cmpl_event_fd = -1;
    }
    if (burst_epollFD >= 0)
    {
        close(burst_epollFD);
        burst_epollFD = -1;
    }
    if (spdsvc_tr471_rx_queue_p)
    {
        size_t length = spdsvc_tr471_rx_queue_mem_size();
        munmap(spdsvc_tr471_rx_queue_p, length);
    }
    return 0;
}

void brcm_udpst_offload_cleanup(int connindex)
{
    if (conf.brcmOffload == FALSE)
    {
        return;
    }
    //printf ("OC C(%d), MC(%d), D(%d)\n", connindex, repo.maxConnIndex, offload_direction);
    if (SPDSVC_DIRECTION_DS == offload_direction)
    {
        spdsvc_tr471_rx_queue_stats(spdsvc_tr471_rx_queue_p);
        udpst_spdsvc_disable(connindex);
        if (connindex == repo.maxConnIndex)
        {
            spdsvc_tr471_rx_queue_init(spdsvc_tr471_rx_queue_p);
            offload_direction = SPDSVC_DIRECTION_US; /* Set it back to default */
        }
    }
    else
    {
        udpst_spdsvc_disable(connindex);
    }
}

int brcm_udpst_offload_event(struct epoll_event *epoll_event)
{
    int                           connindex;
    spdsvc_tr471_rx_queue_entry_t *offload_rx_entry_p = NULL;


    if (offload_event_fd != -1 && epoll_event->data.ptr == &offload_event_fd)
    {
        if (brcm_udpst_offload_read_event_fd(offload_event_fd))
        {
            perror("brcm_udpst_offload_event(): Could not read event_count");
            return -1;
        }
//        printf("\tevent_count %llu (0x%llx)\n", event_count, event_count);
        while ((offload_rx_entry_p =
                                     spdsvc_tr471_rx_queue_read(spdsvc_tr471_rx_queue_p)))
        {
            offload_rx_queue_lHdr = &offload_rx_entry_p->rxq_loadHdr;
            connindex = offload_rx_entry_p->connindex;
            repo.rcvDataSize = sizeof(struct loadHdr);
            service_loadpdu(connindex);
            spdsvc_tr471_rx_queue_post(spdsvc_tr471_rx_queue_p);
            brcm_profile_point_stop(connindex, UDPST_PROFILE_RECV_DATA, 0);
            brcm_profile_point_start(connindex, UDPST_PROFILE_RECV_DATA, 0);
        }
        return 1;
    }
    return 0;
}

int brcm_udpst_offload_analyzer_enable(int connindex)
{
    struct connection *c = &conn[connindex];
    int ret;

    if (conf.brcmOffload == FALSE)
    {
        return 0;
    }

    // Offload is enabled.

    offload_direction = SPDSVC_DIRECTION_DS;
    c->config.tr471.connindex = connindex;
    c->config.mode = SPDSVC_MODE_TR471;
    c->config.dir = SPDSVC_DIRECTION_DS;
    c->config.tr471.pid = offload_pid;
    c->config.tr471.event_fd = offload_event_fd;
    c->config.tr471.burst_cmpl_event_fd = burst_cmpl_event_fd;
    ret = udpst_spdsvc_enable(&c->config);
    if (ret)
    {
        int var;
        var = sprintf(scratch, "ERROR: Could not spdsvc_enable for DS test\n");
        send_proc(errConn, scratch, var);
        printf("%s,%u: ERROR: Could not enable\n", __FUNCTION__, __LINE__);
    }
    return ret;
}

static int is_ipv4_mapped_ipv6_address(char *addr)
{
    int dots = 0;
    char *p;
    for (p = addr; *p; p++)
    {
        if (*p == '.')
            dots++;
    }
    if (dots == 3)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
int brcm_udpst_offload_config(struct sockaddr_storage *sas, struct connection *c, int is_local)
{
    if (sas->ss_family == AF_INET)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)sas;
        if (is_local)
        {
            c->config.socket.local_ip_addr.family = SPDSVC_FAMILY_IPV4;
            c->config.socket.local_ip_addr.ipv4.u32 = ntohl(addr->sin_addr.s_addr);
            c->config.socket.local_port_nbr = c->locPort;
            brcm_udpst_debug("\tLocal IPv4 %08x:%d\n",
                             c->config.socket.local_ip_addr.ipv4.u32,
                             c->config.socket.local_port_nbr);
        }
        else
        {
            c->config.socket.remote_ip_addr.family = SPDSVC_FAMILY_IPV4;
            c->config.socket.remote_ip_addr.ipv4.u32 = ntohl(addr->sin_addr.s_addr);
            c->config.socket.remote_port_nbr = c->remPort;
            brcm_udpst_debug("\tRemote IPv4 %08x:%d\n",
                             c->config.socket.remote_ip_addr.ipv4.u32,
                             c->config.socket.remote_port_nbr);
        }
    }
    else
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)sas;
        if (is_local)
        {
            c->config.socket.local_port_nbr = c->locPort;
            if (is_ipv4_mapped_ipv6_address(c->locAddr))
            {
                c->config.socket.local_ip_addr.family = SPDSVC_FAMILY_IPV4;
                c->config.socket.local_ip_addr.ipv4.u32 = ntohl(addr6->sin6_addr.s6_addr32[3]);
                brcm_udpst_debug("\tLocal IPv4 Mapped %08x:%d\n",
                                 c->config.socket.local_ip_addr.ipv4.u32,
                                 c->config.socket.local_port_nbr);
            }
            else
            {
                c->config.socket.local_ip_addr.family = SPDSVC_FAMILY_IPV6;
                memcpy(c->config.socket.local_ip_addr.ipv6.u8,
                       addr6->sin6_addr.s6_addr,
                       sizeof(c->config.socket.local_ip_addr.ipv6.u8));
                brcm_udpst_debug("\tLocal IPv6 %08x %08x %08x %08x:%d\n",
                                 ntohl(c->config.socket.local_ip_addr.ipv6.u32[0]),
                                 ntohl(c->config.socket.local_ip_addr.ipv6.u32[1]),
                                 ntohl(c->config.socket.local_ip_addr.ipv6.u32[2]),
                                 ntohl(c->config.socket.local_ip_addr.ipv6.u32[3]),
                                 c->config.socket.local_port_nbr);
            }
        }
        else
        {
            c->config.socket.remote_port_nbr = c->remPort;
            if (is_ipv4_mapped_ipv6_address(c->remAddr))
            {
                c->config.socket.remote_ip_addr.family = SPDSVC_FAMILY_IPV4;
                c->config.socket.remote_ip_addr.ipv4.u32 = ntohl(addr6->sin6_addr.s6_addr32[3]);
                brcm_udpst_debug("\tRemote IPv4 Mapped %08x:%d\n",
                                 c->config.socket.remote_ip_addr.ipv4.u32,
                                 c->config.socket.remote_port_nbr);
            }
            else
            {
                c->config.socket.remote_ip_addr.family = SPDSVC_FAMILY_IPV6;
                memcpy(c->config.socket.remote_ip_addr.ipv6.u8,
                       addr6->sin6_addr.s6_addr,
                       sizeof(c->config.socket.remote_ip_addr.ipv6.u8));
                brcm_udpst_debug("\tRemote IPv6 %08x %08x %08x %08x:%d\n",
                                 ntohl(c->config.socket.remote_ip_addr.ipv6.u32[0]),
                                 ntohl(c->config.socket.remote_ip_addr.ipv6.u32[1]),
                                 ntohl(c->config.socket.remote_ip_addr.ipv6.u32[2]),
                                 ntohl(c->config.socket.remote_ip_addr.ipv6.u32[3]),
                                 c->config.socket.remote_port_nbr);
            }
        }
    }
    return 0;
}


#if defined(BRCM_UDPST_OFFLOAD_PROFILING)
brcm_profile_point_t brcm_profile_point[MAX_SERVER_CONN][MAX_PROFILE_POINTS] = { };

unsigned int brcm_gettime(int is_nsec)
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);

    if (is_nsec)
        return (unsigned int)tspecnsec(&t);
    else
        return (unsigned int)tspecusec(&t);
}

int brcm_profile_point_init(int index, char *name, int *bins, int num_of_bins)
{
    if (conf.verbose == TRUE)
    {
        int i;

        for (i = 0; i < conf.maxConnections; i++)
        {

            if ((index > MAX_PROFILE_POINTS) || (num_of_bins > MAX_PROFILE_BINS))
            {
                brcm_udpst_debug("\nWrong profile parameters\n");
                return -1;
            }

            brcm_profile_point[i][index].active = 1;
            brcm_profile_point[i][index].num_of_bins = num_of_bins;
            strncpy(brcm_profile_point[i][index].name, name,
                    sizeof(brcm_profile_point[i][index].name));


            memcpy(&brcm_profile_point[i][index].bin, bins, num_of_bins * sizeof(int));
            brcm_profile_point[i][index].bin[num_of_bins + 1] = 0;
        } /* for (i) */
    }
    return 0;
}

void brcm_profile_print_results()
{
    if (conf.verbose == TRUE)
    {
        int connindex;

        for (connindex = 0; connindex < conf.maxConnections; connindex++)
        {

            int i, index, percent;
            char int_str[20];

            printf("\n");
            printf("Profile results Connection : %d \n", connindex);
            for (index = 0; index < MAX_PROFILE_POINTS; index++)
            {
                if (brcm_profile_point[connindex][index].active)
                {
                    printf("== %s    : ", brcm_profile_point[connindex][index].name);
                    for (i = 0; i < brcm_profile_point[connindex][index].num_of_bins + 1; i++)
                    {
                        if (brcm_profile_point[connindex][index].count[i])
                        {
                            percent = (brcm_profile_point[connindex][index].count[i] * 100) / brcm_profile_point[connindex][index].total_cnt;
                        }
                        else
                        {
                            percent = 0;
                        }
                        sprintf(int_str, "%d", brcm_profile_point[connindex][index].bin[i]);
                        printf("Above %s us:%u(%u%%)  ", int_str, brcm_profile_point[connindex][index].count[i], percent);
                    }
                    printf("\n");
                }
            }
        } /* for (connindex) */
        printf("\n");
    }
}

int brcm_profile_point_start(int connindex, int index, int is_nsec)
{
    if (conf.verbose == TRUE)
    {
        if ((connindex >= conf.maxConnections) || (index > MAX_PROFILE_POINTS))
        {
            brcm_udpst_debug("\nWrong connection(%d) and/or profile point(%d) index\n", connindex, index);
            return -1;
        }

        brcm_profile_point[connindex][index].start_time = brcm_gettime(is_nsec);
    }

    return 0;
}

int brcm_profile_point_stop(int connindex, int index, int is_nsec)
{
    if (conf.verbose == TRUE)
    {
        int time_diff, i;
        brcm_profile_point_t *profile;

        if ((connindex >= conf.maxConnections) || (index > MAX_PROFILE_POINTS))
        {
            brcm_udpst_debug("\nWrong connection(%d) and/or profile point(%d) index\n", connindex, index);
            return -1;
        }

        profile = &brcm_profile_point[connindex][index];
        profile->stop_time = brcm_gettime(is_nsec);

        time_diff = profile->stop_time - profile->start_time;

        if ((profile->start_time == 0) || (time_diff < 0))
        {
            return 0;
        }

        for (i = 0; i < profile->num_of_bins + 1; i++)
        {
            if (time_diff >= profile->bin[i])
            {
                profile->count[i]++;
                profile->total_cnt++;
                break;
            }
        }
    }

    return 0;
}
#endif /* #if defined(BRCM_UDPST_OFFLOAD_PROFILING) */


int brcm_udpst_offload_send(int connindex, int totalburst, int burstsize, unsigned int payload, unsigned int addon)
{
    int first_burst = brcm_offload_in_progress(connindex) ? 0 : 1;
    struct connection *c = &conn[connindex];
    int max_payload_size = MAX_TPAYLOAD_SIZE - PPPOE_OVERHEAD;
#if defined(BRCM_POLLING)
    /* Legacy. Not used now as we now wait for the burst complete event. 
     * if using it, make sure to take care for multiple connections. */
    spdsvc_result_t result;
#endif
    int ret;
    int readyfds;

    brcm_profile_point_stop(connindex, UDPST_PROFILE_SEND_LD_TMR, 0);
    brcm_profile_point_start(connindex, UDPST_PROFILE_SEND_LD_TMR, 0);
    brcm_profile_point_start(connindex, UDPST_PROFILE_SEND_LD_EXE, 0);

    if (conf.brcmOffload == TRUE)
    {
        offload[connindex].burst_nbr++;
        if (SPDSVC_FAMILY_IPV6 == c->config.socket.local_ip_addr.family)
        {
            max_payload_size -= IPV6_ADDSIZE;
        }
        if (payload > max_payload_size)
        {
            int total_bytes = (burstsize * payload) + addon;
            /* printf("BURST (%uB): connindex %u, totalburst %u, burstsize %u, payload %u, addon %u\n", */
            /*        total_bytes, connindex, totalburst, burstsize, payload, addon); */
            payload = max_payload_size;
            burstsize = total_bytes / payload;
            addon = total_bytes - (burstsize * payload);
            totalburst = burstsize;
            if (addon > 0)
            {
                if (addon < sizeof(struct loadHdr))
                {
                    addon = sizeof(struct loadHdr);
                }
                totalburst++;
            }
            /* printf("\tCONVERSION (MTU %u): connindex %u, totalburst %u, burstsize %u, payload %u, addon %u\n", */
            /*        max_payload_size, connindex, totalburst, burstsize, payload, addon); */
        }
        // Make sure previous burst completed successfully
#if defined(BRCM_POLLING)
        /* if we are defining this, adopt for MULTI_FLOW also - if needed and
         * enabled. TBD */
        {
            spdsvc_config_t config = { };

            config.tr471.connindex = connindex;
            config.mode = SPDSVC_MODE_TR471;

            int retry = 100000;
            while (retry--)
            {
                ret = spdsvc_getResult(&config, &result);
                if (ret)
                {
                    printf("udpst offload: Could not get results\n");
                    return ret;
                }
                if (result.tx_setup_done && !result.running)
                {
                    break;
                }
            }
            if (retry < 0)
            {
                printf("udpst offload: Transmission timeout (burst_nbr %u, tx_setup_done %u, running %u)\n",
                       offload[connindex].burst_nbr, result.tx_setup_done, result.running);
                return -1;
            }
        }
#else

        // Clear any pending events; timeout = 0 to return immediate
        readyfds = epoll_wait(burst_epollFD, burst_epollFD_events, MAX_EPOLL_EVENTS, BURST_COMPLETE_NO_WAIT);
        if (readyfds)
        {
            printf("ERROR : Burst FD already set, unexpected %d\n", readyfds);
            /* Read and clear the FD, even though unexpected */
            brcm_udpst_offload_read_event_fd(burst_cmpl_event_fd);
        }
#endif

        c->config.mode = SPDSVC_MODE_TR471;
        c->config.dir = SPDSVC_DIRECTION_US;
        //printf("udpst offload : connindex %d\n",connindex);
        c->config.tr471.connindex = connindex;
        c->config.tr471.totalburst = totalburst;
        c->config.tr471.burstsize = burstsize;
        c->config.tr471.payload = payload;
        c->config.tr471.addon = addon;
        c->config.tr471.randPayload = c->randPayload;
        c->config.tr471.pid = offload_pid;
        c->config.tr471.event_fd = offload_event_fd;
        c->config.tr471.burst_cmpl_event_fd = burst_cmpl_event_fd;
        /* Mark first_burst if it is */
        c->config.tr471.firstBurst = first_burst;
        /* Build/populate load header to send to SPDSVC driver */
        brcm_populate_header(&c->config.tr471.lHdr, c);
        /* Increment Load SeqNo for first burst,
         * though not needed because load header is learnt from first packet */
        c->config.tr471.lHdr.lpduSeqNo = htonl((uint32_t)(c->lpduSeqNo + first_burst));
        if (payload)
        {
            c->config.tr471.lHdr.udpPayload = htons(payload);
        }
        else
        {
            c->config.tr471.lHdr.udpPayload = htons(addon);
        }
        ret = udpst_spdsvc_enable(&c->config);
        if (!ret)
        {
            offload[connindex].sendmsg_count = 0;
            offload[connindex].sendmsg_errors = 0;
            /* Only send the msg through UDP socket for first burst for learning purpose */
            if (first_burst)
                brcm_sendmsg_burst(connindex, 1, burstsize, payload, addon);
            if (offload[connindex].sendmsg_errors)
            {
                /* This faulre would be non-recoverable */
                printf("udpst offload: failed to send first burst (burst_nbr %u, sendmsg_count %u)\n",
                       offload[connindex].burst_nbr, offload[connindex].sendmsg_count);
                /* No need to decrement the seqNo as we don't know what stage the faulre occured */
                goto fallback_no_offload;
            }
            else
            {
                readyfds = epoll_wait(burst_epollFD, burst_epollFD_events, MAX_EPOLL_EVENTS, BURST_COMPLETE_WAIT);
                if (!readyfds)
                {
                    if (first_burst)
                    {
                        printf("First Burst completion timeout %d; Possibly flow not learnt\n", readyfds);
                    }
                    else
                    {
                        printf("ERROR : Burst completion timeout %d\n", readyfds);
                    }
                    goto fallback_no_offload;
                }
                else
                {
                    if (brcm_udpst_offload_read_event_fd(burst_cmpl_event_fd))
                    {
                        printf("brcm_udpst_offload_send(): Could not read burst_cmpl_event_fd %d", burst_cmpl_event_fd);
                        goto fallback_no_offload;
                    }
                }
                c->lpduSeqNo += totalburst;
                if (first_burst)
                {
                    first_burst = 0;
                }

            }
        }
        else
        {
            printf("udpst offload: spdsvc_enable failed\n");
            goto fallback_no_offload;
        }

        brcm_profile_point_stop(connindex, UDPST_PROFILE_SEND_LD_EXE, 0);
        return ret;

fallback_no_offload:
        {
           int i;
            // Offload is not working as expected. Force offload off.
            printf("udpst offload: Falling back to non-offload\n");
            for (i = 0; i < conf.maxConnections; i++)
            {
                udpst_spdsvc_disable(i);
            }
            conf.brcmOffload = FALSE;
        }

    } /* if(conf.brcmOffload == TRUE) */

#if defined(HAVE_SENDMMSG)
    brcm_sendmmsg_burst(connindex, totalburst, burstsize, payload, addon);
#else
    brcm_sendmsg_burst(connindex, totalburst, burstsize, payload, addon);
#endif /* HAVE_SENDMMSG */

    brcm_profile_point_stop(connindex, UDPST_PROFILE_SEND_LD_EXE, 0);
    return 0;
}

void brcm_sendmsg_burst_update_result(int connindex, int count_inc, int error_inc)
{
    offload[connindex].sendmsg_errors += error_inc;
    offload[connindex].sendmsg_count += count_inc;
}
#endif
//----------------------------------------------------------------------------
