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
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "tr143_defs.h"
#include "tr143_private.h"

extern unsigned int incremental_rxcnt;
extern struct timeval incremental_startTime;
extern bool forceStop;

#ifdef BRCM_SPDTEST
#include <spdt_api.h>

int spdt_httpDownloadDiag(testParams_t *testParams, connectionResult_t *connResult)
{
   spdt_conn_params_t params = {};
   uint8_t stream_idx = 0;
   int rc, errCode = Complete;
   spdt_rx_params_t rx_params = {};
   spdt_stat_t spd_stat = {};
   tcp_spdt_rep_t *spd_report;
   spdt_stats_get_mode_t mode = SPDT_STATS_GET_BLOCKING_END_OF_TEST;

   cmsLog_notice("\nHTTP download with %s offloading enabled:\n", diag_offload_mode ? "HW" : "SW");

   if ((rc = tcpspeedtest_setup(SPDT_HTTP, &stream_idx)))
   {
      cmsLog_error("TCPSPEEDTEST init error, rc:%d\n", rc);
      errCode = Error_InitConnectionFailed;
      goto errOut3;
   }

   if (rc = tcpspeedtest_set_duration(stream_idx, time_based_test_duration))
   {
      cmsLog_error("TCPSPEEDTEST set test duration failed, rc:%d\n", rc);
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   strncpy(params.if_name, if_name, (sizeof(params.if_name) - 1));

   if (server_port == 0) server_port = 80;

   memcpy(&params.server_addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&params.server_addr)->sin_port = htons(server_port);
   params.tos = dscp << 2;
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   params.force_swoffl_mode = (diag_offload_mode == SW_OFFLOAD_MODE) ? 1 : 0;
   rc = spdt_connect(stream_idx, SPDT_DIR_RX, &params);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (rc) 
   {
      cmsLog_error("[%u]open socket error, rc:%d\n", stream_idx, rc);
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }
   cmsLog_debug("====>[%u]connected to server", stream_idx);

   rx_params.proto.tcp.size = 0;
   rx_params.proto.tcp.file_name = uri[0] != '/' ? uri : uri + 1;
   rx_params.proto.tcp.host_name = server_name;

   if ((rc = spdt_recv_start(stream_idx, &rx_params)))
   {
      cmsLog_error("[%u]download error, rc:%d\n", stream_idx, rc);
      errCode = Error_TransferFailed;
      goto errOut1;
   }

   sleep(time_based_test_measurement_offset);

   connResult->TotalBytesReceivedBegin = connResult->TotalBytesSentBegin = 0;

   if (time_based_test_measurement_interval)
      mode = SPDT_STATS_GET_CURRENT;

   do {
        if ((rc = spdt_stats_get(stream_idx, mode, &spd_stat)))
        {
           cmsLog_error("[%u]HTTP speed report error, rc:%d\n", stream_idx, rc);
           errCode = Error_TransferFailed;
           goto errOut1;
        }

        spd_report = &(spd_stat.proto_ext.tcp_speed_rep);

        if (mode == SPDT_STATS_GET_CURRENT && TCPSPDTEST_GENL_CMD_STATUS_IN_PROCESS == spd_report->status)
        {
            printf("[%u]Downloaded %lld bytes\n", stream_idx, spd_report->num_bytes);
            sleep(time_based_test_measurement_interval);
        }
   } while (TCPSPDTEST_GENL_CMD_STATUS_IN_PROCESS == spd_report->status);

   if (TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status || TCPSPDTEST_GENL_CMD_STATUS_INTERRUPTED == spd_report->status ||
         TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
   {
      connResult->TestBytes = spd_report->num_bytes;
      connResult->TotalBytesReceivedEnd = spd_report->num_bytes;
      connResult->TotalBytesSentEnd = 0;
      connResult->ROMTime.tv_sec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_ROM_TIME].tv_sec;
      connResult->ROMTime.tv_usec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_ROM_TIME].tv_usec;
      connResult->BOMTime.tv_sec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_sec;
      connResult->BOMTime.tv_usec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_usec;
      connResult->EOMTime.tv_sec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_sec;
      connResult->EOMTime.tv_usec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_usec;

      if (TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
            errCode = Error_TransferFailed;

      cmsLog_notice("[%u]Download completed %s with %llu bytes at %u ms GoodPut=%d Mbps\n", stream_idx,
         TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status ? "OK" : TCPSPDTEST_GENL_CMD_STATUS_INTERRUPTED == spd_report->status ?  "INTERRUPTED" : "BAD",
         spd_report->num_bytes, spd_report->time_ms, spd_report->rate);
   }

errOut1:
   pthread_mutex_lock(&g_stream_disconnect_lock[stream_idx]);
   spdt_disconnect(stream_idx);
   pthread_mutex_unlock(&g_stream_disconnect_lock[stream_idx]);

errOut2:
   pthread_mutex_lock(&g_stream_disconnect_lock[stream_idx]);
   spdt_uninit(stream_idx);
   pthread_mutex_unlock(&g_stream_disconnect_lock[stream_idx]);

errOut3:
   g_stream_active[stream_idx] = 0;

   cmsLog_debug("[%u]return:%d", stream_idx, errCode);

   return errCode;	
}

int spdt_httpUploadDiag(testParams_t *testParams, connectionResult_t *connResult)
{
   spdt_conn_params_t params = {};
   uint8_t stream_idx = 0;
   int rc, errCode = Complete;
   spdt_tx_params_t tx_params = {};
   unsigned long long fileLength = testFileLength;
   spdt_stat_t spd_stat = {};
   tcp_spdt_rep_t *spd_report;
   spdt_stats_get_mode_t mode = SPDT_STATS_GET_BLOCKING_END_OF_TEST;

   cmsLog_notice("\nHTTP upload with %s offloading enabled:\n", diag_offload_mode ? "HW" : "SW");

   if ((rc = tcpspeedtest_setup(SPDT_HTTP, &stream_idx)))
   {
      cmsLog_error("TCPSPEEDTEST init error, rc:%d\n", rc);
      errCode = Error_InitConnectionFailed;
      goto errOut3;
   }

   if ((rc = tcpspeedtest_set_duration(stream_idx, time_based_test_duration)))
   {
      cmsLog_error("TCPSPEEDTEST set test duration failed, rc:%d\n", rc);
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   strncpy(params.if_name, if_name, (sizeof(params.if_name) - 1));

   if (server_port == 0)
      server_port = 80;

   memcpy(&params.server_addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&params.server_addr)->sin_port = htons(server_port);
   params.tos = dscp << 2;
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   params.force_swoffl_mode = (diag_offload_mode == SW_OFFLOAD_MODE) ? 1 : 0;
   rc = spdt_connect(stream_idx, SPDT_DIR_TX, &params);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (rc) 
   {
      cmsLog_error("[%u]open socket error, rc:%d\n", stream_idx, rc);
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }
   cmsLog_debug("====>[%u]connected to server", stream_idx);
   
   connResult->TotalBytesReceivedBegin = connResult->TotalBytesSentBegin = 0;
   cmsLog_debug("[%u]start transmision", stream_idx);

   tx_params.proto.tcp.size = fileLength;
   tx_params.proto.tcp.uri = uri[0] != '/' ? uri : uri + 1;

   if ((rc = spdt_send_start(stream_idx, &tx_params)))
   {
      cmsLog_error("[%u]upload error, rc:%d\n", stream_idx, rc);
      errCode = Error_TransferFailed;
      goto errOut1;
   }

   sleep(time_based_test_measurement_offset);

   if (time_based_test_measurement_interval)
       mode = SPDT_STATS_GET_CURRENT;

   do {
        if ((rc = spdt_stats_get(stream_idx, mode, &spd_stat)))
        {
           cmsLog_error("[%u]HTTP Speed upload report error, rc:%d\n", stream_idx, rc);
           errCode = Error_TransferFailed;
           goto errOut1;
        }

        spd_report = &(spd_stat.proto_ext.tcp_speed_rep);

        if (mode == SPDT_STATS_GET_CURRENT && TCPSPDTEST_GENL_CMD_STATUS_IN_PROCESS == spd_report->status)
        {
            printf("[%u]Uploaded %lld bytes\n", stream_idx, spd_report->num_bytes);
            sleep(time_based_test_measurement_interval);
        }

   } while (TCPSPDTEST_GENL_CMD_STATUS_IN_PROCESS == spd_report->status);

   if (TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status || TCPSPDTEST_GENL_CMD_STATUS_INTERRUPTED == spd_report->status ||
         TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
   {
      connResult->TestBytes = spd_report->num_bytes;
      connResult->TotalBytesSentEnd = spd_report->num_bytes;
      connResult->TotalBytesReceivedEnd = 0;
      connResult->ROMTime.tv_sec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_ROM_TIME].tv_sec;
      connResult->ROMTime.tv_usec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_ROM_TIME].tv_usec;
      connResult->BOMTime.tv_sec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_sec;
      connResult->BOMTime.tv_usec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_usec;
      connResult->EOMTime.tv_sec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_sec;
      connResult->EOMTime.tv_usec = spd_report->msg.tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_usec;

      if (TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
            errCode = Error_TransferFailed;

      cmsLog_notice("[%u]Upload completed %s with %llu bytes at %u ms GoodPut=%d Mbps", stream_idx,
         TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status ? "OK" : TCPSPDTEST_GENL_CMD_STATUS_INTERRUPTED == spd_report->status ?  "INTERRUPTED" : "BAD",
         spd_report->num_bytes, spd_report->time_ms, spd_report->rate);
   }

   cmsLog_debug("[%u]end transmision", stream_idx);

errOut1:
   pthread_mutex_lock(&g_stream_disconnect_lock[stream_idx]);
   spdt_disconnect(stream_idx);
   pthread_mutex_unlock(&g_stream_disconnect_lock[stream_idx]);

errOut2:
   pthread_mutex_lock(&g_stream_disconnect_lock[stream_idx]);
   spdt_uninit(stream_idx);
   pthread_mutex_unlock(&g_stream_disconnect_lock[stream_idx]);

errOut3:
   g_stream_active[stream_idx] = 0;

   cmsLog_debug("[%u]return:%d", stream_idx, errCode);

   return errCode;
}

#endif //#ifdef BRCM_SPDTEST

/*
return:
1) actual size,  if content-length is not specified
2) content-length, if content-length is > 0
3) -1, if timeout or socket close
*/
static int64_t http_readLengthMsg(tProtoCtx *pc, uint64_t readLth, int doFlushStream)
{
   int64_t bufCnt = 0, readCnt = 0;
   int64_t bufLth = readLth;
   /* The coverity set default stack size to 1KB, it is configurable.
    * To avoid error when user doesn't specific it, just ingore the error here.*/
   /* coverity[stack_use_local_overflow] */
   char buf[16384];
   int (*readfn)(tProtoCtx *, char *, int);
   int buflen;

   set_sockopt_nocopy(pc->fd);


   if (pc->discardData)
   {
       readfn = proto_Readn_discard;
       buflen = 128 * 1024; /* no need to allocate buffer */
   }
   else
   {
       readfn = proto_Readn;
       buflen = sizeof(buf);
   }

   cmsLog_notice("Payload read started");

   while ((readLth <= 0 || bufCnt < readLth) && !forceStop)
   {
      if ((readCnt = readfn(pc, buf, (bufLth > buflen || readLth <= 0 ) ? buflen : bufLth)) > 0)
      {
          //cmsLog_debug("readCnt: %d\n",readCnt);
          bufCnt += readCnt;
          incremental_rxcnt += readCnt;
          bufLth -= readCnt;
      }
      else
      {
         cmsLog_error("proto_Readn timeout");
         break;
      }
   }

   cmsLog_notice("Payload read ended :read=%lld bytes", bufCnt);

   if(readCnt <= 0 && readLth > 0)
   {
      cmsLog_error("http_readLengthMsg error");
      return -1;
   }

   if (doFlushStream) proto_Skip(pc);         

   return bufCnt;
}

static int64_t  http_readChunkedMsg(tProtoCtx *pc) 
{
   char chunkedBuf[512];   
   int64_t  chunkedSz = 0, readSz = 0, totSz = 0;

   while (1)
   {
      do
      {
         chunkedBuf[0] = '\0';
         readSz = proto_Readline(pc, chunkedBuf, sizeof(chunkedBuf));
         if (readSz <= 0) 
         {
            cmsLog_error("read chunked size error");
            return -1;
         }
      }
      while ((readSz > 0 && isxdigit(chunkedBuf[0]) == 0) && !forceStop);

      if (forceStop) break;

      totSz += readSz;
      sscanf(chunkedBuf, "%llx", &chunkedSz);

      if (chunkedSz <= 0) break;
      if ((readSz = http_readLengthMsg(pc, chunkedSz, FALSE)) < 0)
      {
         cmsLog_error("http_readLengthMsg error, chunked size = %lld, readSz = %lld", chunkedSz, readSz);
         return -1;
      }

      totSz += readSz;
   }

   proto_Skip(pc);         
   return totSz;
}

static int send_http_get(tProtoCtx *pc)
{
   if (proto_SendRequest(pc, "GET", uri) < 0)
      return -1;

   proto_SendHeader(pc,  "Host", server_name);
   proto_SendHeader(pc,  "User-Agent", TR143_AGENT_NAME);
   proto_SendHeader(pc,  "Connection", "keep-alive");
   proto_SendRaw(pc, "\r\n", 2);

   return 0;
}  /* End of send_get_request() */


static int http_GetData(tProtoCtx *pc, connectionResult_t *connResult)
{
   tHttpHdrs *hdrs;
   int errCode = Complete;
   int ret = 0;
   int64_t len = 0;

   if ((hdrs = proto_NewHttpHdrs()) == NULL)
   {
      cmsLog_error("http hdr alloc error");
      return Error_Internal;
   }

   ret = proto_ParseResponse(pc, hdrs);
   if (ret < 0)  // not a "successful response" means no response
   {
      cmsLog_error("error: read response failure");
      errCode = Error_NoResponse;
      goto errOut1;
   }
   connResult->TestBytes = 0;

   connResult->TestBytes += ret;
   connResult->TestBytes += proto_ParseHdrs(pc, hdrs);

   if (hdrs->status_code != 200 )
   {
      cmsLog_error("http reponse %d", hdrs->status_code);
      if (hdrs->status_code == 401)
         errCode = Error_LoginFailed;
      else  // not a "successful response" means no response
         errCode = Error_NoResponse;
      goto errOut1;
   }

   incremental_startTime = connResult->BOMTime;

   if (hdrs->TransferEncoding && !strcasecmp(hdrs->TransferEncoding,"chunked"))
   {
      alarm(TR143_INCREMENTAL_RESULT_INTERVAL);
      if(pc->discardData)
      {
          set_sockopt_discard(pc->fd, 0);
          pc->discardData = false;
          cmsLog_error("HTTP chunked transfer, disabling dataDiscard");
      }
      len = http_readChunkedMsg(pc);
      if (len < 0)
      {
         cmsLog_error("calling http_readChunkedMsg error");
         errCode = Error_TransferFailed;
      }
      connResult->TestBytes += len;
   }
   else if (hdrs->content_length > 0)
   {
      alarm(TR143_INCREMENTAL_RESULT_INTERVAL);
      cmsLog_notice("calling readLengthMsg, content_length=%llu", hdrs->content_length);
      len = http_readLengthMsg(pc, hdrs->content_length, FALSE);
      if ( len < 0 )
      {
         cmsLog_error("calling readLengthMsg error, content_length=%llu", hdrs->content_length);
         errCode = Error_TransferFailed;
      }
      connResult->TestBytes += len;
   }

errOut1:
   proto_FreeHttpHdrs(hdrs);
   return errCode;
}

int default_httpDownloadDiag(testParams_t *testParams, connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   int errCode = Complete;
   int sfd = -1;
   tProtoCtx *pc = NULL;

   cmsLog_notice("\nHTTP download without offloading:\n");

   if (server_port == 0) server_port = 80;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr, connResult->ipAddr,sizeof(connResult->ipAddr));
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);

   if (sfd <= 0)
   {
      cmsLog_error("open socket error");
      errCode = Error_InitConnectionFailed;
      goto errOut0;
   }
   cmsLog_notice("====>connected to server");

   if ((pc = proto_NewCtx(sfd)) == NULL)
   {
      cmsLog_error("proto_NewCtx error");
      errCode = Error_Internal;
      goto errOut1;
   }
   if(testParams->discardData)
   {
       set_sockopt_discard(sfd, 1);
       pc->discardData = true;
   }

   gettimeofday(&connResult->ROMTime, NULL);

   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin, &connResult->TotalPacketsReceivedBegin, &connResult->TotalPacketsSentBegin);
   if (send_http_get(pc) < 0)
   {
      cmsLog_error("send GET request error errno:%d(%s)", errno, strerror(errno));
      if (errno == ECONNREFUSED)
         errCode = Error_InitConnectionFailed;
      else if (errno == EHOSTUNREACH)
         errCode = Error_NoRouteToHost;
      else
         errCode = Error_Other;
      goto errOut2;
   }
   cmsLog_notice("====>http get sent");

   if (select_with_timeout(sfd, 1))
   {
      cmsLog_error("http get reponse error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   gettimeofday(&connResult->BOMTime, NULL);

   errCode = http_GetData(pc, connResult);

   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd, &connResult->TotalPacketsReceivedEnd, &connResult->TotalPacketsSentEnd);
   gettimeofday(&connResult->EOMTime, NULL);

   cmsLog_debug("before downloading RxBytes:%llu, TxBytes:%llu, RxPackets:%llu, TxPackets:%llu", connResult->TotalBytesReceivedBegin, connResult->TotalBytesSentBegin,
                                                                         connResult->TotalPacketsReceivedBegin, connResult->TotalPacketsSentBegin);

   cmsLog_debug("finish downloading RxBytes:%llu, TxBytes:%llu, RxPackets:%llu, TxPacktes:%llu", connResult->TotalBytesReceivedEnd, connResult->TotalBytesSentEnd,
                                                                         connResult->TotalPacketsReceivedEnd, connResult->TotalPacketsSentEnd);
   generate_report(connResult, 0);

errOut2:
   proto_FreeCtx(pc);
errOut1:
   close(sfd);
errOut0:
   cmsLog_debug("return:%d", errCode);
   return errCode;
}

static Tr143DiagState send_http_put(tProtoCtx *pc, connectionResult_t *connResult)
{
   char strLength[16] = {0};
   char connUri[BUFLEN_512] = {0};
   unsigned long long fileLength = testFileLength;

   // Note: when test with multiple thread, the same uri would cause upload 
   // rejected by server. In such cases, we add postfix with uri
   if (connResult->index != -1)
      snprintf(connUri, BUFLEN_512, "%s_%d", uri, connResult->index);
   else
      strncpy(connUri, uri ,BUFLEN_512-1);


   if (create_diagfile())
   {
      cmsLog_error("create file failed");
      return Error_Internal;
   }

   cmsLog_notice("start send");
   
   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin, &connResult->TotalPacketsReceivedBegin, &connResult->TotalPacketsSentBegin);
   gettimeofday(&connResult->BOMTime, NULL);

   incremental_startTime = connResult->BOMTime;
   alarm(TR143_INCREMENTAL_RESULT_INTERVAL);

   if (proto_SendRequest(pc, "PUT", connUri) < 0)
   {
      cmsLog_error("send PUT request failed errno:%d(%s)", errno, strerror(errno));
      if (errno == ECONNREFUSED)
         return Error_InitConnectionFailed;
      else if (errno == EHOSTUNREACH)
         return Error_NoRouteToHost;
      else
         return Error_Other;
   }

   proto_SendHeader(pc,  "Host", server_name);
   proto_SendHeader(pc,  "User-Agent", TR143_AGENT_NAME);
   proto_SendHeader(pc,  "Connection", "keep-alive");
   proto_SendHeader(pc,  "Content-Type", "text/xml");
   sprintf(strLength, "%llu", testFileLength);
   proto_SendHeader(pc,  "Content-Length", strLength);
   proto_SendRaw(pc, "\r\n", 2);

   if (send_diagfile(pc->fd, &fileLength) < 0)
   {
      cmsLog_error("send file failed");
      delete_diagfile();
      return Error_TransferFailed;
   }

   if (forceStop && fileLength)
   {
      // if user stops upload, calculate total uploaded length
      fileLength = testFileLength - fileLength;
      connResult->TestBytes += fileLength;
   }
   else
   {
      connResult->TestBytes += testFileLength;
   }

   gettimeofday(&connResult->EOMTime, NULL);
   delete_diagfile();
   fsync(pc->fd);
   cmsLog_notice("end send");

   sleep(1); // wait a second and let driver update counters into net device
   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd, &connResult->TotalPacketsReceivedEnd, &connResult->TotalPacketsSentEnd);

   cmsLog_debug("before uploading RxBytes:%llu, TxBytes:%llu, RxPackets:%llu, TxPackets:%llu", connResult->TotalBytesReceivedBegin, connResult->TotalBytesSentBegin,
                                                                         connResult->TotalPacketsReceivedBegin, connResult->TotalPacketsSentBegin);

   cmsLog_debug("finish uploading RxBytes:%llu, TxBytes:%llu, RxPackets:%llu, TxPackets:%llu", connResult->TotalBytesReceivedEnd, connResult->TotalBytesSentEnd,
                                                                         connResult->TotalPacketsReceivedEnd, connResult->TotalPacketsSentEnd);

   return Complete;
}  /* End of send_get_request() */

int default_httpUploadDiag(testParams_t *testParams, connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   int errCode = Complete, ret;
   tProtoCtx *pc = NULL;
   tHttpHdrs *hdrs;
   int sfd;

   cmsLog_notice("\nHTTP upload without offloading:\n");

   if (server_port == 0) server_port = 80;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr, connResult->ipAddr,sizeof(connResult->ipAddr));
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (sfd <= 0)
   {
      cmsLog_error("open socket error");
      errCode = Error_InitConnectionFailed;
      goto errOut0;
   }
   cmsLog_notice("====>connected to server");

   if ((pc = proto_NewCtx(sfd)) == NULL)
   {
      cmsLog_error("proto_NewCtx error");
      errCode = Error_Internal;
      goto errOut1;
   }

   gettimeofday(&connResult->ROMTime, NULL);
   errCode = send_http_put(pc, connResult);
   if (errCode != Complete)
   {
      cmsLog_error("send put request error");
      goto errOut2;
   }
   cmsLog_notice("====>http put sent");

   if (select_with_timeout(sfd, 1))
   {
      cmsLog_error("http get reponse error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   generate_report(connResult, 1);

   if ((hdrs = proto_NewHttpHdrs()) == NULL)
   {
      cmsLog_error("http hdr alloc error");
      errCode = Error_Internal;
      goto errOut2;
   }

   ret = proto_ParseResponse(pc, hdrs);
   if (ret < 0)  // not a "successful response" means no response
   {
      cmsLog_error("error: read response failure");
      errCode = Error_NoResponse;
      goto errOut3;
   }

   proto_ParseHdrs(pc, hdrs);

   if (hdrs->status_code != 100 &&  // Continue status might be returned by Microsoft-IIS/5.1
         hdrs->status_code != 201 &&   // Created status is returned by Microsoft-IIS/5.1
         hdrs->status_code != 204 &&   // No content status is returned by Apache/2.2.2
         hdrs->status_code != 200 )
   {
      cmsLog_error("http reponse %d", hdrs->status_code);
      if (hdrs->status_code == 401)
         errCode = Error_LoginFailed;
      else // not a "successful response" means no response
         errCode = Error_NoResponse;
   }
   else
   {
      errCode = Complete;
   }

errOut3:
   proto_FreeHttpHdrs(hdrs);
errOut2:
   proto_FreeCtx(pc);
errOut1:
   close(sfd);
errOut0:
   cmsLog_debug("return:%d", errCode);
   return errCode;
}
