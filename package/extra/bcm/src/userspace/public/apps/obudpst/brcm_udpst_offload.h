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

#if defined(BRCM_UDPST_OFFLOAD_PROFILING)
#define MAX_PROFILE_POINTS 10
#define MAX_PROFILE_BINS   19

typedef enum _UdpstProfilePts {

    UDPST_PROFILE_SEND_LD_TMR = 0,
    UDPST_PROFILE_SEND_LD_EXE,
    UDPST_PROFILE_RECV_DATA,
} UdpStProfilePts;

typedef struct  {
    int active;
    int total_cnt;
    char name[256];
    unsigned int start_time;
    unsigned int stop_time;
    int num_of_bins;
    int bin[MAX_PROFILE_BINS+1]; // numbers in us in decending order
    unsigned int count[MAX_PROFILE_BINS+1];
} brcm_profile_point_t;

unsigned int brcm_gettime(int is_nsec);
int brcm_profile_point_init(int index, char *name, int *bins, int num_of_bins);
void brcm_profile_print_results();
int brcm_profile_point_start(int connindex, int index, int is_nsec);
int brcm_profile_point_stop(int connindex, int index, int is_nsec);
#else
#define brcm_profile_point_init(index,name,bins,num_of_bins)
#define brcm_profile_point_start(connindex,index,is_nsec)
#define brcm_profile_point_stop(connindex,index,is_nsec)
#define brcm_profile_print_results()
#endif


/* Wrapper function defined in udpst source code */
void brcm_populate_header(struct loadHdr *lHdr, struct connection *c);
void brcm_sendmsg_burst(int connindex, int totalburst, int burstsize, unsigned int payload, unsigned int addon);
void brcm_sendmmsg_burst(int connindex, int totalburst, int burstsize, unsigned int payload, unsigned int addon);

/* Functions provided by BRCM */
int brcm_udpst_offload_init(void);
int brcm_udpst_offload_exit(void);
int brcm_udpst_offload_event(struct epoll_event *epoll_event);
void brcm_udpst_offload_cleanup(int connindex);
int brcm_udpst_offload_config(struct sockaddr_storage *sas, struct connection *c, int is_local);
int brcm_udpst_offload_analyzer_enable(int connindex);
int brcm_udpst_offload_send(int connindex, int totalburst, int burstsize, unsigned int payload, unsigned int addon);
void brcm_sendmsg_burst_update_result(int connindex, int count_inc, int error_inc);
int brcm_udpst_offload_validate_params(void);

/* Variables provided by BRCM */
//extern spdsvc_tr471_rx_queue_t *spdsvc_tr471_rx_queue_p;
extern struct loadHdr *offload_rx_queue_lHdr;
//
// Convert timespec to ns
//
#define tspecnsec(a) ((a)->tv_sec * NSECINSEC) + (a)->tv_nsec

// Wait for burst completion; timeout = 10ms (for production cases.
// Wait for burst completion; timeout = 25sec (for debug)
#define BURST_COMPLETE_NO_WAIT        0
#if 1
#define BURST_COMPLETE_WAIT           10
#else
#define BURST_COMPLETE_WAIT           25000
#endif

#endif
//----------------------------------------------------------------------------
