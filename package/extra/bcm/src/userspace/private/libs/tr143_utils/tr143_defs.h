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

#ifndef _TR143_DEFS_H_
#define _TR143_DEFS_H_

#include <sys/socket.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <net/if.h>
#include <stdbool.h>

#include "cms.h"
#include "cms_util.h"




#define TR143_AGENT_NAME        "BCM_TR143_DIAG_04_06"
#define TR143_SESSION_TIMEOUT   10
#define TR143_LOCK_TIMEOUT      (5*1000)
#define TR143_BUF_SIZE_MAX      4096
#define TR143_MAX_CONNECTION    8

#define TR143_UPLOAD_DIAG_FILE         "/tmp/ulDiagFile"
#define TR143_UPLOAD_RESULT_FILE       "/tmp/ulDiagResult"
#define TR143_DOWNLOAD_RESULT_FILE     "/tmp/dlDiagResult"
#define TR143_UDPECHO_RESULT_FILE      "/tmp/udpEchoDiagResult"
#define TR143_SERVERSELECT_RESULT_FILE "/tmp/serverSelectDiagResult"
#define TR143_CONNFILE_MIDLE_NAME   "_conn"

#define TR143_DOWNLOAD_INCREMENTAL_FILE    "/tmp/incrementalinfodn"
#define TR143_UPLOAD_INCREMENTAL_FILE      "/tmp/incrementalinfoup"
#define TR143_UDPECHO_INCREMENTAL_FILE     "/tmp/incrementalinfoudp"

//the interval in second to query the diag result to return to BAS server
#define TR143_INCREMENTAL_RESULT_INTERVAL  1
#define TR143_GENERAL_ERROR            -1
#define TR143_RESOLVE_HOSTNAME_ERROR   -2
/*
 * These variables are used to pass args between app and tr143_utils lib.
 * All variables are declared in the tr143_utils lib, common.c
 */
typedef enum{
   NO_OFFLOAD       = -1,
   SW_OFFLOAD_MODE  = 0,
   HW_OFFLOAD_MODE  = 1
}Tr143DiagOffloadMode;

extern char *tr143_result_fname;
extern  struct tr143diagstats_t diagstats;
extern const char *DiagnosticsState[];
extern char server_name[1024];
extern struct sockaddr_storage server_addr;
extern int server_port;
extern char uri[256], proto[256];
extern char *url, if_name[IF_NAMESIZE];
extern Tr143DiagOffloadMode diag_offload_mode;
extern unsigned char dscp;
extern unsigned long long testFileLength;
extern void *msgHandle;
extern int isBdkMode;
extern int cmsNotify;
extern CmsEntityId dstEid;
extern int time_based_test_duration;
extern int time_based_test_measurement_interval;
extern int time_based_test_measurement_offset;
extern void getIncrementalResult(int num);
extern void sigusr2Handler(int num);


struct tr143diagstats_t
{
   char DiagnosticsState[64];
   char ROMTime[64];
   char BOMTime[64];
   char EOMTime[64];
   unsigned long long TestBytes;
   unsigned long long TotalBytesReceived;
   unsigned long long TotalBytesSent;
   char TCPOpenRequestTime[64];
   char TCPOpenResponseTime[64];
   char ipAddr[48];
   bool forceStop;
};

typedef struct connectionResult
{
   int index;
   int state;
   struct timeval ROMTime;
   struct timeval BOMTime;
   struct timeval EOMTime;
   unsigned long long TestBytes;
   unsigned long long TotalBytesReceivedBegin;
   unsigned long long TotalBytesReceivedEnd;
   unsigned long long TotalBytesSentBegin;
   unsigned long long TotalBytesSentEnd;
   unsigned long long TotalPacketsReceivedBegin;
   unsigned long long TotalPacketsReceivedEnd;
   unsigned long long TotalPacketsSentBegin;
   unsigned long long TotalPacketsSentEnd;
   struct timeval TCPOpenRequestTime;
   struct timeval TCPOpenResponseTime;
   char ipAddr[48];
   bool forceStop;
}connectionResult_t;

typedef enum{
   FTP_TEST,
   HTTP_TEST
}Tr143DiagTestType;

typedef enum{
   DOWNLOAD_TEST,
   UPLOAD_TEST
}Tr143DiagTestDirection;

typedef struct testParams
{
   Tr143DiagTestType type; 
   Tr143DiagTestDirection direction;
   bool discardData;
}testParams_t;

extern testParams_t testParams_g;
typedef enum{
   Complete=2,
   Error_CannotResolveHostName,
   Error_NoRouteToHost,
   Error_InitConnectionFailed,
   Error_NoResponse,
   Error_TransferFailed,
   Error_PasswordRequestFailed,
   Error_LoginFailed,
   Error_NoTransferMode,
   Error_NoPASV,
   Error_IncorrectSize,
   Error_Timeout,
   Error_NoCWD,
   Error_NoSTOR,
   Error_Internal,
   Error_Other
} Tr143DiagState;



/*
 * These are the publicly available functions from libtr143_utils
 */
int setupconfig(void);

/*
 * Set this temp data buffer size to 128KB. That could fill into 2 large SKB of 64KB data.
 * The goal is to reduce data copy and generate large size SKB, since low-level GSO works more efficently with large SKB 
 */
#define TR143_DIAG_FILE_SIZE (128*1024)  // 128K size 

int create_diagfile(void);
int send_diagfile(int outfd, unsigned long long *length);
void delete_diagfile(void);

int run_diagnostic(testParams_t testParams, connectionResult_t *result);

int compare_timestamp(struct timeval t1, struct timeval t2);
void cleanup_and_notify(Tr143DiagState state, CmsEntityId srcEid);
void add_connection_result(connectionResult_t *result, int index);


#endif /* _TR143_DEFS_H_ */
