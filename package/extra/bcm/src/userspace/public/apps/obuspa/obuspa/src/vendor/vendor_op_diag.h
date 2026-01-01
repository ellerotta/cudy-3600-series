/***********************************************************************
 *
 *
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom
   All Rights Reserved

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
#ifndef __VENDOR_OP_DIAG_H__
#define __VENDOR_OP_DIAG_H__

/* IP Diagnostics */
#define DIAG_IP_PING            1
#define DIAG_IP_TRACEROUTE      2
#define DIAG_IP_DOWNLOAD        3
#define DIAG_IP_UPLOAD          4
#define DIAG_IP_UDPECHO         5
#define DIAG_IP_SEVERSELECTION  6
#define DIAG_IP_LAYERCAPACITY   7

#define DIAG_IP_PING_COMMAND_PATH "Device.IP.Diagnostics.IPPing()"
#define DIAG_IP_TRACEROUTE_COMMAND_PATH "Device.IP.Diagnostics.TraceRoute()"
#define DIAG_IP_DOWNLOAD_COMMAND_PATH "Device.IP.Diagnostics.DownloadDiagnostics()"
#define DIAG_IP_UPLOAD_COMMAND_PATH "Device.IP.Diagnostics.UploadDiagnostics()"
#define DIAG_IP_UDPECHO_COMMAND_PATH "Device.IP.Diagnostics.UDPEchoDiagnostics()"
#define DIAG_IP_SEVERSELECTION_COMMAND_PATH "Device.IP.Diagnostics.ServerSelectionDiagnostics()"
#define DIAG_IP_LAYERCAPACITY_COMMAND_PATH "Device.IP.Diagnostics.IPLayerCapacity()"

typedef struct
{
   int requestInstance;
   char host[256];
   char interface[256];
   char protocolVersion[8];
   unsigned int numOfRepetitions;
   unsigned int timeout;
   unsigned int dataSize;
   unsigned int DSCP;
} ipPingDiagInput;

typedef struct
{
   int requestInstance;
   char interface[256];
   char protocolVersion[8];
   char host[256];
   unsigned int numOfTries;
   unsigned int timeout;
   unsigned int dataSize;
   unsigned int DSCP;
   unsigned int maxHopCount;
} ipTraceRouteDiagInput;

typedef struct
{
   int requestInstance;
   char interface[256];
   char downloadURL[2048];
   unsigned int DSCP;
   unsigned int ethernetPriority;
   unsigned int timeBasedTestDuration;
   unsigned int timeBasedTestMeasurementInterval;
   unsigned int timeBasedTestMeasurementOffset;
   char protocolVersion[8];
   unsigned int numberOfConnections;
   bool enablePerConnectionResults;
} ipDownloadDiagInput;

typedef struct
{
   int requestInstance;
   char interface[256];
   char uploadURL[2048];
   unsigned int DSCP;
   unsigned int ethernetPriority;
   unsigned int testFileLength;
   unsigned int timeBasedTestDuration;
   unsigned int timeBasedTestMeasurementInterval;
   unsigned int timeBasedTestMeasurementOffset;
   char protocolVersion[8];
   unsigned int numberOfConnections;
   bool enablePerConnectionResults;
} ipUploadDiagInput;

typedef struct
{
   int requestInstance;
   char interface[256];
   char host[2048];
   unsigned int port;
   unsigned int numberOfRepetitions;
   unsigned int timeout;
   unsigned int dataBlockSize;
   unsigned int DSCP;
   unsigned int interTransmissionTime;
   char protocolVersion[8];
   bool enableIndividualPacketResults;
} ipUdpechoDiagInput;

typedef struct
{
   int requestInstance;
   char interface[256];
   char hostList[2048];
   char protocolVersion[8];
   char protocol[16];
   unsigned int numberOfRepetitions;
   unsigned int timeout;
} ipServerSelectionDiagInput;

typedef struct
{
   int requestInstance;
   char role[16];
   char host[256];
   unsigned int port;
   bool jumboFramesPermitted;
   unsigned int DSCP;
   char protocolVersion[8];
   char UDPPayloadContent[16];
   char testType[8];
   bool IPDVEnable;
   bool IPRREnable;
   unsigned int startSendingRate;
   unsigned int numberTestSubIntervals;
   unsigned int numberFirstModeTestSubIntervals;
   unsigned int testSubInterval;
   unsigned int statusFeedbackInterval;
   unsigned int seqErrThresh;
   unsigned int lowerThresh;
   unsigned int upperThresh;
   unsigned int highSpeedDelta;
   unsigned int slowAdjThresh;
} ipLayerCapInput;

int sendIpDiagOutput(int type, int err, char *err_msg);
int vendorDiagIpPingCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int vendorDiagTraceRouteCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int vendorDiagDownloadCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int vendorDiagUploadCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int vendorDiagUdpechoCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int vendorDiagServerSelectionCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int vendorIpLayerCapCb(dm_req_t *req, kv_vector_t *input_args, int instance);
int requestPathToInstance(char *command);
void fetchTraceRouteStats(kv_vector_t *output_args, int commandErr, char *err_msg);
void fetchPingStats(kv_vector_t *output_args, int commandErr, char *err_msg);
void fetchDownloadStats(kv_vector_t *output_args, int commandErr, char *err_msg);
void fetchUploadStats(kv_vector_t *output_args, int commandErr, char *err_msg);
void fetchUdpechoStats(kv_vector_t *output_args, int commandErr, char *err_msg);
void fetchServerSelectionStats(kv_vector_t *output_args, int commandErr, char *err_msg);
void fetchIpCapStats(kv_vector_t *output_args, int commandErr, char *err_msg);

/* DSL,ATM Diagnostics */
#define DIAG_DSL_ADSLLINETEST   1
#define DIAG_ATM_F5LOOPBACK     2

#define DIAG_DSL_ADSLLINETEST_COMMAND_PATH "Device.DSL.Diagnostics.ADSLLineTest()"
#define DIAG_ATM_F5LOOPBACK_COMMAND_PATH   "Device.ATM.Diagnostics.F5Loopback()"

typedef struct
{
   int requestInstance;
   char interface[256];
} dslAdslLineTestInput;

typedef struct
{
   int requestInstance;
   char interface[256];
   unsigned int numberOfRepetitions;
   unsigned int timeout;
} atmF5LoopbackInput;

int sendDslDiagOutput(int type, int err, char *err_msg);

int vendorDiagAdslLineTestCb(dm_req_t *req, kv_vector_t *input_args, int instance);
void fetchAdslLineTestStats(kv_vector_t *output_args, int commandErr, char *err_msg);

int vendorDiagAtmF5LoopbackCb(dm_req_t *req, kv_vector_t *input_args, int instance);
void fetchAtmF5LoopbackStats(kv_vector_t *output_args, int commandErr, char *err_msg);

#endif /* __VENDOR_OP_DIAG_H__ */
