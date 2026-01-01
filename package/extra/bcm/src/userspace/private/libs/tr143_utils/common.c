/***********************************************************************
 *
 *  Copyright (c) 2012-2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/sendfile.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/tcp.h>
#include <netdb.h>

   /*
    * Normally, TCP_NOCOPY value will be available through the libc toolchain headers
    * when recompiled against the kernel header file. However, we want to avoid
    * changing the toolchain, so this value is redefined here.
    * Note that TCP_NOCOPY value is kernel version dependent.
    */
#define TCP_NOCOPY 27
#define IPV4_HEADER_SIZE 20
#define IPV6_HEADER_SIZE 40
#define TCP_HEADER_SIZE 32

#include "tr143_defs.h"
#include "tr143_private.h"
#include "mdm_validstrings.h"
#include "cms_msg.h"

#ifdef BRCM_SPDTEST
#include <pthread.h>
#include <spdt_api.h>
#include <signal.h>
#include <time.h>

uint8_t g_stream_active[SPDT_NUM_OF_STREAMS] = { 0 };
pthread_mutex_t g_stream_disconnect_lock[SPDT_NUM_OF_STREAMS] = { PTHREAD_MUTEX_INITIALIZER };

int tcpspeedtest_set_duration(uint8_t stream_idx, int sec);
timer_t g_timerid[SPDT_NUM_OF_STREAMS] = { 0 };

static void terminal_signal_handler_func(int sig_num)
{
   int i;

   for (i=0; i<SPDT_NUM_OF_STREAMS; i++)
   {
      if (!g_stream_active[i])
         continue;

      g_stream_active[i] = 0;
      tcpspeedtest_set_duration(i, 0);
      spdt_disconnect(i);
   }
}

static void timer_handler_func(union sigval sigev_value)
{
   uint8_t stream_idx = sigev_value.sival_int;

   tcpspeedtest_set_duration(stream_idx, 0);

   pthread_mutex_lock(&g_stream_disconnect_lock[stream_idx]);
   spdt_disconnect(stream_idx);
   pthread_mutex_unlock(&g_stream_disconnect_lock[stream_idx]);
}

int tcpspeedtest_setup(spdt_proto_t proto, uint8_t *stream_idx)
{
   struct sigaction new_action = {};
   struct sigevent sevp = {};
   int rc;

   new_action.sa_handler = terminal_signal_handler_func;
   new_action.sa_flags = SA_RESETHAND;
   if ((rc = sigaction(SIGTERM, &new_action, NULL))) return rc;

   /* reuse SIGTERM handler for SIGINT */
   if ((rc = sigaction(SIGINT, &new_action, NULL))) return rc;

   rc = spdt_init(proto, stream_idx);
   if (!rc)
      g_stream_active[*stream_idx] = 1;

   /* create timer */
   sevp.sigev_notify = SIGEV_THREAD;
   sevp.sigev_notify_function = timer_handler_func;
   sevp.sigev_value.sival_int = *stream_idx;
   if ((rc = timer_create(CLOCK_REALTIME, &sevp, &(g_timerid[*stream_idx])))) return rc;

   return rc;
}

int tcpspeedtest_set_duration(uint8_t stream_idx, int sec)
{
    int rc = 0;
    struct itimerspec duration;

    if (g_timerid[stream_idx])
    {
       duration.it_value.tv_sec = sec;
       duration.it_value.tv_nsec = 0;

       duration.it_interval.tv_sec = 0;
       duration.it_interval.tv_nsec = 0;

       rc = timer_settime(g_timerid[stream_idx], 0, &duration, 0);
    }

    return rc;
}
#endif /* BRCM_SPDTEST */

char connIfName[CMS_IFNAME_LENGTH] = {};
UBOOL8 loggingSOAP = FALSE;

char *tr143_result_fname = NULL;
char server_name[1024] = "192.168.1.100";
struct sockaddr_storage server_addr = {};

int server_port;
testParams_t testParams_g = {};
char uri[256], proto[256];
char *url, if_name[IF_NAMESIZE];
Tr143DiagOffloadMode diag_offload_mode;
unsigned char dscp;
unsigned long long testFileLength = 0;
void *msgHandle = NULL;
int isBdkMode = 0;
int cmsNotify = 0;
CmsEntityId dstEid = EID_SSK;
struct tr143diagstats_t diagstats;
int time_based_test_duration = 0;
int time_based_test_measurement_interval = 0;
int time_based_test_measurement_offset = 0;

const char *DiagnosticsState[] = {
   MDMVS_NONE,
   MDMVS_REQUESTED,
   MDMVS_COMPLETE,
   MDMVS_ERROR_CANNOTRESOLVEHOSTNAME,
   MDMVS_ERROR_NOROUTETOHOST,
   MDMVS_ERROR_INITCONNECTIONFAILED,
   MDMVS_ERROR_NORESPONSE,
   MDMVS_ERROR_TRANSFERFAILED,
   MDMVS_ERROR_PASSWORDREQUESTEDFAILED,
   MDMVS_ERROR_LOGINFAILED,
   MDMVS_ERROR_NOTRANSFERMODE,
   MDMVS_ERROR_NOPASV,
   MDMVS_ERROR_INCORRECTSIZE,
   MDMVS_ERROR_TIMEOUT,
   MDMVS_ERROR_NOCWD,
   MDMVS_ERROR_NOSTOR,
   MDMVS_ERROR_INTERNAL,
   MDMVS_ERROR_OTHER
};

bool forceStop = 0;
//#define DEBUG 1

static void *get_in_address(const struct sockaddr *sa) {

    if( sa->sa_family == AF_INET) // IPv4 address
        return &(((struct sockaddr_in*)sa)->sin_addr);
    // else IPv6 address
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int isMatchedAddress(struct sockaddr *ifAddr, struct sockaddr *netmask, struct sockaddr *ipaddr)
{
   int i = 0;
   if (ifAddr == NULL || netmask == NULL || ipaddr == NULL)
      return 0;

   for (i = 0 ; i < 14 ; i ++)
   {
      if ((ifAddr->sa_data[i] & netmask->sa_data[i]) != (ipaddr->sa_data[i] & netmask->sa_data[i]))
      {
         return 0;
      }
   }
   return 1;
}

void cleanup_and_notify(Tr143DiagState state, CmsEntityId srcEid)
{
   FILE * fp;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   diagstats.forceStop = forceStop;
   strncpy(diagstats.DiagnosticsState, DiagnosticsState[state], sizeof(diagstats.DiagnosticsState)-1);
   fp=fopen(tr143_result_fname,"w");
   if (fp)
   {
      fwrite(&diagstats,1,sizeof(diagstats),fp);
      fclose(fp);
   }
   else
      cmsLog_error("open %s error", tr143_result_fname);

   if (cmsNotify)  // sending DIAG_COMPLETE message to ssk (diag_ssk)  
   {
      cmsLog_notice("srcEid=%d dstEid=%d exit state = %d", srcEid, dstEid, state);
      cmsLog_notice("tcpOpen:%s, tcpResp:%s\nROM:%s, BOM:%s, EOM:%s\n"
            "TestBytesReceived:%llu, TotalBytesReceived:%llu, TotalBytesSent:%llu",
            diagstats.TCPOpenRequestTime, diagstats.TCPOpenResponseTime,
            diagstats.ROMTime, diagstats.BOMTime, diagstats.EOMTime,
            diagstats.TestBytes, diagstats.TotalBytesReceived, diagstats.TotalBytesSent);

      if (msgHandle != NULL)
      {
         if (srcEid != EID_UPLOAD_DIAG && srcEid != EID_DOWNLOAD_DIAG)
         {
            cmsLog_error("Unsupported srcEid %d", srcEid);
         }
         else
         {
            msg.type = (srcEid == EID_UPLOAD_DIAG) ? 
                       CMS_MSG_UPLOAD_DIAG_COMPLETE : CMS_MSG_DOWNLOAD_DIAG_COMPLETE;
            msg.src =  srcEid;
            msg.dst = dstEid;
            msg.flags_event = 1;

            if (cmsMsg_send(msgHandle, &msg) != CMSRET_SUCCESS)
               cmsLog_error("could not send out 0x%x event msg", msg.type);
            else
               cmsLog_debug("Send out 0x%x event msg.", msg.type);
         }
      }
      else
         cmsLog_error("No msgHandle to send!");
   }
   else 
   {
      printf("tcpOpen:%s, tcpResp:%s\nROM:%s, BOM:%s, EOM:%s\n"
            "TestBytesReceived:%llu, TotalBytesReceived:%llu, TotalBytesSent:%llu\n",
            diagstats.TCPOpenRequestTime, diagstats.TCPOpenResponseTime,
            diagstats.ROMTime, diagstats.BOMTime, diagstats.EOMTime,
            diagstats.TestBytes, diagstats.TotalBytesReceived, diagstats.TotalBytesSent);
   }

   if (msgHandle != NULL)
      cmsMsg_cleanup(&msgHandle);

   cmsLog_cleanup();
}

int safe_strtoul(char *arg, unsigned long* value)
{
   char *endptr;
   int errno_save = errno;

   if (arg ==NULL) return -1;

   errno = 0;
   *value = strtoul(arg, &endptr, 0);
   if (errno != 0 || endptr==arg) {
      return -1;
   }
   errno = errno_save;
   return 0;
}

int safe_strtoull(char *arg, uint64_t* value)
{
   char *endptr = NULL;
   int errno_save = errno;

   if (arg ==NULL) return -1;

   errno = 0;
   *value = strtoull(arg, &endptr, 0);
   if (errno != 0 || endptr==arg) {
      return -1;
   }
   errno = errno_save;
   return 0;
}

unsigned int incremental_rxcnt = 0;
unsigned int incremental_txcnt = 0;
static unsigned int incremental_rxcnt_old = 0;
static unsigned int incremental_txcnt_old = 0;
static int incr_idx = 0;
static int isDownload = 0;
static int headersize = 0;
struct timeval incremental_startTime;

static void updateIncrFile(const char *line)
{
   FILE *fp = NULL;
   char filename[32]={0};

   snprintf(filename, sizeof(filename), "%s",
            isDownload?TR143_DOWNLOAD_INCREMENTAL_FILE:TR143_UPLOAD_INCREMENTAL_FILE);

   fp = fopen(filename, "a");
   if (fp)
   {
      fputs(line, fp);
      fclose(fp);
   }
   else
   {
      cmsLog_error("cannot open file<%s>", filename);
   }
}

void getIncrementalResult(int num)
{
   char line[256] = {0};
   struct timeval incremental_endTime;
   char startTime[64]={0};
   char endTime[64]={0};

   incr_idx++;
   gettimeofday(&incremental_endTime, NULL);
   cmsTms_getXSIDateTimeMicroseconds(incremental_endTime.tv_sec,
                                     incremental_endTime.tv_usec,
                                     endTime, sizeof(endTime));
   cmsTms_getXSIDateTimeMicroseconds(incremental_startTime.tv_sec,
                                     incremental_startTime.tv_usec,
                                     startTime, sizeof(startTime));

   snprintf(line, 256, "%d %u %u %s %s\n", incr_idx,
            incremental_rxcnt - incremental_rxcnt_old,
            incremental_txcnt - incremental_txcnt_old,
            startTime, endTime);
   updateIncrFile(line);
   incremental_rxcnt_old = incremental_rxcnt;
   incremental_txcnt_old = incremental_txcnt;
   incremental_startTime = incremental_endTime;
   alarm(TR143_INCREMENTAL_RESULT_INTERVAL);
}

void sigusr2Handler(int num)
{
   forceStop = 1;
}

//flag=1 is read, flag=2 is write
int select_with_timeout(int fd, int flag)
{
   fd_set fdset;
   struct timeval tm;
   int ret;

   FD_ZERO(&fdset);
   FD_SET(fd, &fdset);
   tm.tv_sec = TR143_SESSION_TIMEOUT;
   tm.tv_usec = 0;

   if (flag)
      ret = select(fd + 1, &fdset, NULL, NULL, &tm);
   else
      ret = select(fd + 1, NULL, &fdset, NULL, &tm);

   if ( ret <= 0 || !FD_ISSET(fd, &fdset)) 
   {
      cmsLog_error("select timeout");
      return -1;
   }

   return 0;
}

size_t read_with_timeout(int fd, char *buf, int len)
{
   if (select_with_timeout(fd, 1))
      return -1;

   return read(fd, buf, len);
}

static int host_to_addr(char *name, struct sockaddr_storage *addr)
{
   struct addrinfo hints = {};
   struct addrinfo *res;

   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   cmsLog_debug("getaddrinfo(%s)", name);
   if (getaddrinfo(name, NULL, &hints, &res) || !res)
      return -1;

   addr->ss_family = res->ai_family;            /* IP/IPv6 */
   headersize = TCP_HEADER_SIZE + (res->ai_family==AF_INET ? IPV4_HEADER_SIZE : IPV6_HEADER_SIZE); 
   memcpy(addr, res->ai_addr, res->ai_addrlen); /* Copy address */
   freeaddrinfo(res);

   return 0;
}

int open_conn_socket(struct sockaddr_storage *addr, char *strSockAddr, int len)
{
   int fd;
   int flags, bflags;
   struct ifreq ifr;
   unsigned char tos;

   fd = socket(addr->ss_family, SOCK_STREAM, 0);
   if (fd < 0)
   {
      cmsLog_error("open socket error");
      perror("open socket error");
      return -1;
   }

   if (if_name[0])
   {
      strncpy(ifr.ifr_name, if_name, (sizeof(ifr.ifr_name) - 1));
      ifr.ifr_addr.sa_family = addr->ss_family;
      if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
      {
         close(fd);
         cmsLog_error("SIOCGIFADDR error");
         return -1;
      }

      if (bind(fd, (struct sockaddr *)&ifr.ifr_addr, sizeof(ifr.ifr_addr)) < 0)
      {
         close(fd);
         cmsLog_error("bind error");
         return -1;
      }
   }

   flags = (long) fcntl(fd, F_GETFL);
   bflags = flags | O_NONBLOCK; /* clear non-block flag, i.e. block */
   if (fcntl(fd, F_SETFL, bflags) < 0)
   {
      close(fd);
      cmsLog_error("fcntl error");
      return -1;
   }
   if ((connect(fd, (struct sockaddr *)addr, sizeof(*addr)) < 0 && errno != EINPROGRESS) ||
         select_with_timeout(fd, 0) < 0)
   {
      close(fd);
      cmsLog_error("connect error");
      return -1;
   }
   if (fcntl(fd, F_SETFL, flags) < 0)
   {
      close(fd);
      cmsLog_error("fcntl error");
      return -1;
   }

   if (if_name[0] == '\0')
   {
      struct sockaddr sockaddr;
      socklen_t sockaddrlen = sizeof(sockaddr);
      struct ifaddrs *ifaddr;
      struct ifaddrs *ifa;

      if (getifaddrs(&ifaddr) == -1)
      {
         perror("getifaddrs");
         return -1;
      }

      if (getsockname(fd, &sockaddr, &sockaddrlen) < 0)
      {
         cmsLog_error("getsockname: %s", strerror(errno));
         return -1;
      }

      if (strSockAddr &&
          inet_ntop(sockaddr.sa_family,
                    get_in_address(&sockaddr), strSockAddr, len) != NULL)
      {
         cmsLog_debug("Test socket binds to %s", strSockAddr);
      }

      //look which interface contains the wanted IP.
      for (ifa = ifaddr ; ifa != NULL ; ifa = ifa->ifa_next)
      {
         if (ifa->ifa_addr == NULL || ifa->ifa_netmask == NULL)
            continue;

#ifdef DEBUG
         char strIfAddr[128] = {0};
         char strNetMask[128] = {0};
         if ((inet_ntop(ifa->ifa_addr->sa_family, get_in_address(ifa->ifa_addr), strIfAddr, sizeof(strIfAddr)) != NULL) &&
             (inet_ntop(ifa->ifa_addr->sa_family, get_in_address(ifa->ifa_netmask), strNetMask, sizeof(strNetMask)) != NULL))
         {
            fprintf(stderr, "[%s]:%s mask:%s\n", ifa->ifa_name, strIfAddr, strNetMask);
         }
#endif

         if (isMatchedAddress(ifa->ifa_addr, ifa->ifa_netmask, &sockaddr))
         {
            strncpy(if_name, ifa->ifa_name, sizeof(if_name)-1);
            break;
         }
      }
      freeifaddrs(ifaddr);
   }

   if (if_name[0])
      cmsLog_debug("Test socket binds to %s", if_name);
   else
      cmsLog_error("No matched interface for opened socket!");

   tos = dscp <<  2;
   if (setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0)
      cmsLog_error("set tos error");

   return fd;
}


#define TBT_DURATION_SEC_MAX                999
#define TBT_DURATION_SEC_MIN                0

#define TBT_MEASUREMENT_INTERVAL_SEC_MAX    999
#define TBT_MEASUREMENT_INTERVAL_SEC_MIN    0

#define TBT_MEASUREMENT_OFFSET_SEC_MAX      255
#define TBT_MEASUREMENT_OFFSET_SEC_MIN      0

static int check_time_based_test_settings()
{
   if (time_based_test_duration == 0)
   {
      time_based_test_measurement_interval = 0;
      time_based_test_measurement_offset = 0;
      return 0;
   }

   if ((time_based_test_duration < TBT_DURATION_SEC_MIN) || 
       (time_based_test_duration > TBT_DURATION_SEC_MAX))
   {
      cmsLog_error("duration out of range [%d:%d]", TBT_DURATION_SEC_MIN, TBT_DURATION_SEC_MAX);
      return -1;
   }

   if ((time_based_test_measurement_interval < TBT_MEASUREMENT_INTERVAL_SEC_MIN) || 
       (time_based_test_measurement_interval > TBT_MEASUREMENT_INTERVAL_SEC_MAX))
   {
      cmsLog_error("measurement interval out of range [%d:%d]", TBT_MEASUREMENT_INTERVAL_SEC_MIN, TBT_MEASUREMENT_INTERVAL_SEC_MAX);
      return -1;
   }

   if ((time_based_test_measurement_offset < TBT_MEASUREMENT_OFFSET_SEC_MIN) || 
       (time_based_test_measurement_offset > TBT_MEASUREMENT_OFFSET_SEC_MAX))
   {
      cmsLog_error("measurement offset out of range [%d:%d]", TBT_MEASUREMENT_OFFSET_SEC_MIN, TBT_MEASUREMENT_OFFSET_SEC_MAX);
      return -1;
   }

   return 0;
}


int setupconfig()
{
   char *s_start, *s_end;
   cmsLog_notice("interface %s, dscp %d, url %s", if_name, dscp, url);

   if (url == NULL)
   {
      cmsLog_error("url is empty");
      return TR143_GENERAL_ERROR;
   }

   if (parseUrl(url, proto, server_name, &server_port, uri) < 0)
   {
      cmsLog_error("url parse error");
      return TR143_GENERAL_ERROR;
   }

   if (uri[0] == '\0' || (uri[0] == '/' && uri[1] == '\0'))
   {
      sprintf(uri, "testfile");
   }

   s_start = strstr(server_name, "[");
   s_end = strstr(server_name, "]");
   if ((s_start != NULL) && (s_end != NULL))
   {
      /* IPv6, tmp remove ] */
      s_start++;
      *s_end = '\0';
   }
   else
   {
      /* IPv4 */
      s_start = server_name;
   }
   if (host_to_addr(s_start, &server_addr))
   {
      cmsLog_error("can't resolve host name");
      return TR143_RESOLVE_HOSTNAME_ERROR;
   }
   if ((s_start != NULL) && (s_end != NULL))
   {
      /* IPv6, recover ]  */
      *s_end = ']';
   }

   if (check_time_based_test_settings())
   {
      cmsLog_error("wrong time based test settings");
      return TR143_GENERAL_ERROR;
   }

   memset(&diagstats, 0, sizeof(diagstats));
   return 0;
}

int get_if_stats(unsigned long long *rx, unsigned long long *tx, unsigned long long *rxp, unsigned long long *txp)
{
   FILE *fh;
   char buf[512], *p;
   unsigned int discard;
   int ret = -1;
   unsigned long long lrx, ltx;
   unsigned long long lrxp, ltxp;

   if (if_name[0] == '\0'){
      cmsLog_error("if_name is null");
      return -1;
   }

   fh = fopen("/proc/net/dev", "r");
   if (!fh) {
      cmsLog_error("/proc/net/dev couldn't be opened");
      return -1;
   }

   /* Skip the first two lines */
   if (fgets(buf, sizeof(buf), fh) == NULL){
      fclose(fh);
      cmsLog_error("fgets error");
      return -1;
   }
   if (fgets(buf, sizeof(buf), fh) == NULL){
      fclose(fh);
      cmsLog_error("fgets error");
      return -1;
   }

   while (fgets(buf, sizeof(buf), fh))
   {
      if (!strstr(buf, if_name) || !(p = strchr(buf, ':'))) continue;
      sscanf(p+1, "%llu%llu%u%u%u%u%u%u%llu%llu",
            &lrx, &lrxp, &discard, &discard, &discard, &discard, &discard, &discard, &ltx, &ltxp);
      ret = 0;
      break;
   }

   if (rx) *rx = lrx;
   if (tx) *tx = ltx;
   if (rxp) *rxp = lrxp;
   if (txp) *txp = ltxp;

    cmsLog_notice(" stats for %s rx = %llu, tx=%llu (rxp=%llu,  txp=%llu)\n", if_name, lrx,ltx, lrxp, ltxp);

   fclose(fh);
   return ret;
}

void set_sockopt_nocopy(int fd __attribute__((unused)))
{
#ifdef CONFIG_BCM_SPEEDYGET
   int optval=1; 
   if(setsockopt(fd,IPPROTO_TCP,TCP_NOCOPY,&optval,sizeof(optval)))
      perror("setting setsockopt\n");
   else
      cmsLog_notice("set sock opt");
#else
    cmsLog_notice("TCP_NOCOPY option not supported \n");
#endif
   return;
}

void set_sockopt_discard(int sd,int enable)
{
#ifdef HAVE_TCP_DISCARD
    if (setsockopt(sd, IPPROTO_TCP, TCP_DISCARD, &enable, sizeof(enable)) < 0)
        cmsLog_error("Error: not able to set TCP_DISCARD \n");
    else
        cmsLog_notice("Configured TCP_DISCARD option to %d \n", enable);
#else
    cmsLog_notice("TCP_DISCARD option not supported \n");
#endif
}


int compare_timestamp(struct timeval t1, struct timeval t2)
{
   if (t1.tv_sec > t2.tv_sec)
      return 1;
   else if (t2.tv_sec > t1.tv_sec)
      return -1;
   else if (t1.tv_usec > t2.tv_usec)
      return 1;
   else if (t2.tv_usec > t1.tv_usec)
      return -1;
   else
      return 0;
}


void add_connection_result(connectionResult_t *result, int index)
{
   FILE * fp;
   char connectionFileName[CMS_IFNAME_LENGTH];

   if (result == NULL)
      return;

   sprintf(connectionFileName, "%s%s%d", tr143_result_fname, TR143_CONNFILE_MIDLE_NAME, index);
   fp = fopen(connectionFileName, "w");
   if (fp)
   {
      fwrite(result, 1, sizeof(connectionResult_t), fp);
      fclose(fp);
   }
   else
      cmsLog_error("open %s error", connectionFileName);

}

int create_diagfile(void)
{
   int fd = open(TR143_UPLOAD_DIAG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644 );
   /* The coverity set default stack size to 1KB, it is configurable.
    * To avoid error when user doesn't specific it, just ingore the error here.*/
   /* coverity[stack_use_local_overflow] */
   char buf[TR143_DIAG_FILE_SIZE];

   memset(buf, 'A', TR143_DIAG_FILE_SIZE);

   int size = 0, n;

   if (fd < 0)
   {
      perror("create file");
      return -1;
   }

   while (size < TR143_DIAG_FILE_SIZE)
   {
      n = write(fd, buf, (TR143_DIAG_FILE_SIZE - size));
      if (n < 0)
      {
         perror("fill date in file");
         close(fd);
         return -1;
      }
      size += n;
   }
   close(fd);
   return 0;
}

int send_diagfile(int out_fd, unsigned long long *length)
{
   int file_fd = open(TR143_UPLOAD_DIAG_FILE, O_RDONLY);
   off_t offset = 0;

   if (file_fd < 0)
   {
      perror("open file");
      return -1;
   }

   while (*length && !forceStop)
   {
      size_t sndCnt = *length > TR143_DIAG_FILE_SIZE ? TR143_DIAG_FILE_SIZE : *length;
      int n = sendfile(out_fd, file_fd, &offset, sndCnt);
      if (n < 0)
      {
         perror("sendfile");
         close(file_fd);
         return -1;
      }
      *length -= n;
      offset = 0;
      incremental_txcnt += n;
   }
   close(file_fd);
   return 0;
}

void delete_diagfile(void)
{
   unlink(TR143_UPLOAD_DIAG_FILE);
}

uint32_t calc_delay_ms(struct timeval stop_time, struct timeval start_time)
{
    long calc; /* may be negative use singed type */

    calc = (stop_time.tv_sec - start_time.tv_sec) * 1000;
    calc += (stop_time.tv_usec - start_time.tv_usec) / 1000;

    return (uint32_t)calc;
}

void generate_report(connectionResult_t *connResult, int is_upload)
{
   uint32_t delay_ms;
   uint64_t rate_mega_bps;
   //uint64_t test_rate_mega_bps;
   uint64_t bytes;

   connResult->forceStop = forceStop;
   delay_ms = calc_delay_ms(connResult->EOMTime, connResult->ROMTime);

   bytes = connResult->TestBytes;
   if (is_upload)
      bytes += (connResult->TotalPacketsSentEnd - connResult->TotalPacketsSentBegin) * headersize;
   else
      bytes += (connResult->TotalPacketsReceivedEnd - connResult->TotalPacketsReceivedBegin) * headersize;

   /* calculation in two stages to prevent wrap around */
   rate_mega_bps = (bytes << 3) / delay_ms / 1000;

   if (is_upload)
      cmsLog_notice("Upload completed OK with %llu bytes at %u ms GoodPut=%llu Mbps\n", bytes, delay_ms, rate_mega_bps);
   else
      cmsLog_notice("Download completed OK with %llu bytes at %u ms GoodPut=%llu Mbps\n", bytes, delay_ms, rate_mega_bps);

}

static void fill_diagnostic_results(connectionResult_t *connResult)
{
    cmsTms_getXSIDateTimeMicroseconds(connResult->TCPOpenRequestTime.tv_sec,
                                      connResult->TCPOpenRequestTime.tv_usec,
                                      diagstats.TCPOpenRequestTime,
                                      sizeof(diagstats.TCPOpenRequestTime));
    cmsTms_getXSIDateTimeMicroseconds(connResult->TCPOpenResponseTime.tv_sec,
                                      connResult->TCPOpenResponseTime.tv_usec,
                                      diagstats.TCPOpenResponseTime,
                                      sizeof(diagstats.TCPOpenResponseTime));
    cmsTms_getXSIDateTimeMicroseconds(connResult->ROMTime.tv_sec,
                                      connResult->ROMTime.tv_usec,
                                      diagstats.ROMTime,
                                      sizeof(diagstats.ROMTime));
    cmsTms_getXSIDateTimeMicroseconds(connResult->BOMTime.tv_sec,
                                      connResult->BOMTime.tv_usec,
                                      diagstats.BOMTime,
                                      sizeof(diagstats.BOMTime));
    cmsTms_getXSIDateTimeMicroseconds(connResult->EOMTime.tv_sec,
                                      connResult->EOMTime.tv_usec,
                                      diagstats.EOMTime,
                                      sizeof(diagstats.EOMTime));

    diagstats.TestBytes = connResult->TestBytes;

    strncpy(diagstats.ipAddr, connResult->ipAddr, sizeof(diagstats.ipAddr)-1);

    if (isDownload && connResult->TotalPacketsReceivedEnd >= connResult->TotalPacketsReceivedBegin) // calculate download payload and header 
    {
       diagstats.TotalBytesReceived = (connResult->TotalPacketsReceivedEnd - connResult->TotalPacketsReceivedBegin) * headersize + connResult->TestBytes;
       cmsLog_debug("((PacketsEnd)%llu - (PacketsBegin)%llu) * (header)%d + (TestBytes)%llu = %llu", connResult->TotalPacketsReceivedEnd, connResult->TotalPacketsReceivedBegin, headersize, connResult->TestBytes, diagstats.TotalBytesReceived);
    }
    else if (connResult->TotalBytesReceivedEnd >= connResult->TotalBytesReceivedBegin) 
    {
       diagstats.TotalBytesReceived = connResult->TotalBytesReceivedEnd - connResult->TotalBytesReceivedBegin;
       cmsLog_debug("(BytesEnd)%llu - (BytesBegin)%llu = %llu", connResult->TotalBytesReceivedEnd, connResult->TotalBytesReceivedBegin, diagstats.TotalBytesReceived);
    }
    else
       cmsLog_error("TotalBytesReceivedBegin %llu > TotalBytesReceivedEnd:%llu", connResult->TotalBytesReceivedBegin, connResult->TotalBytesReceivedEnd);

    if (!isDownload && connResult->TotalPacketsSentEnd >= connResult->TotalPacketsSentBegin) // calculate upload payload and header
    {
       diagstats.TotalBytesSent = (connResult->TotalPacketsSentEnd - connResult->TotalPacketsSentBegin) * headersize + connResult->TestBytes;
       cmsLog_debug("((PacketsEnd)%llu - (PacketsBegin)%llu) * (header)%d + (TestBytes)%llu = %llu", connResult->TotalPacketsSentEnd, connResult->TotalPacketsSentBegin, headersize, connResult->TestBytes, diagstats.TotalBytesSent);
    }
    else if (connResult->TotalBytesSentEnd >= connResult->TotalBytesSentBegin)
    {
       diagstats.TotalBytesSent = connResult->TotalBytesSentEnd - connResult->TotalBytesSentBegin;
       cmsLog_debug("(BytesEnd)%llu - (BytesBegin)%llu = %llu", connResult->TotalBytesSentEnd, connResult->TotalBytesSentBegin, diagstats.TotalBytesSent);
    }
    else
       cmsLog_error("TotalBytesSentBegin %llu > TotalBytesSentEnd:%llu", connResult->TotalBytesSentBegin, connResult->TotalBytesSentEnd);
}

int run_diagnostic(testParams_t testParams, connectionResult_t *connResult)
{
   int ret;
   int (*diag_func)(testParams_t *, connectionResult_t *connResult);

   isDownload = (testParams.direction == DOWNLOAD_TEST)?1:0;

   if (isDownload)
   {
      unlink(TR143_DOWNLOAD_INCREMENTAL_FILE);
   }
   else
   {
      unlink(TR143_UPLOAD_INCREMENTAL_FILE);
   }

   switch(testParams.type)
   {
       case FTP_TEST:
#ifdef BRCM_SPDTEST
           if ( diag_offload_mode == SW_OFFLOAD_MODE || diag_offload_mode == HW_OFFLOAD_MODE )
              diag_func = (testParams.direction == DOWNLOAD_TEST ? spdt_ftpDownloadDiag : spdt_ftpUploadDiag);
           else
#endif
              diag_func = (testParams.direction == DOWNLOAD_TEST ? default_ftpDownloadDiag : default_ftpUploadDiag);
           break;
       case HTTP_TEST:
#ifdef BRCM_SPDTEST
           if ( diag_offload_mode == SW_OFFLOAD_MODE || diag_offload_mode == HW_OFFLOAD_MODE )
              diag_func = (testParams.direction == DOWNLOAD_TEST ? spdt_httpDownloadDiag : spdt_httpUploadDiag);
           else
#endif
              diag_func = (testParams.direction == DOWNLOAD_TEST ? default_httpDownloadDiag : default_httpUploadDiag);
           break;
       default:
           cmsLog_error("wrong Tr143DiagTestType %d specified", testParams.type);
           return -1;
   }

   if (connResult)
   {
      ret = diag_func(&testParams, connResult);
   }
   else
   {
      connectionResult_t connResultTemp = {0};
      connResultTemp.index = -1;
      ret = diag_func(&testParams, &connResultTemp);
      fill_diagnostic_results(&connResultTemp);
   }

   return ret;
}
