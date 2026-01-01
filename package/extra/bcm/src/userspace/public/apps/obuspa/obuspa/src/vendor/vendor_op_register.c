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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "usp_err_codes.h"
#include "vendor_defs.h"
#include "vendor_api.h"
#include "usp_api.h"
#include "usp_log.h"
#include "data_model.h"
#include "msg_handler.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "bcm_fsutils.h"
#include "bcm_boardutils.h"
#include "bcm_ulog.h"
#include "bcm_generic_hal.h"
#include "bcm_retcodes.h"
#include "bdk_usp.h"
#include "cms_mem.h"
#include "cms_util.h"
#include "vendor_op_diag.h"
#include "vendor_op_modsw.h"

/* registration of functions that requires #ifdef for data model compatibility in releases */

int registerDiagTraceRoute(void);
int registerDiagPing(void);
int registerDiagDownload(void);
int registerDiagUpload(void);
int registerDiagUdpecho(void);
int registerDiagIpLayerCap(void);
int registerDiagServerSelection(void);
int registerDiagAdslLineTest(void);
int registerDiagAtmF5Loopback(void);

/* vendor_init calls this func to register supported diagnostic operations to obuspa 
 * When this command is received from the controller, obuspa will call the registered callback
 * with request along with the input_args to handle the operation.
 */

int vendorRegisterOpIpDiag(void)
{
   int err = USP_ERR_OK;
#ifdef DMP_DEVICE2_TRACEROUTE_1
   err |= registerDiagTraceRoute();
#endif
#ifdef DMP_DEVICE2_IPPING_1
   err |= registerDiagPing();
#endif
#ifdef DMP_DEVICE2_DOWNLOAD_1
   err |= registerDiagDownload();
#endif
#ifdef DMP_DEVICE2_UPLOAD_1
   err |= registerDiagUpload();
#endif
#ifdef DMP_DEVICE2_UDPECHODIAG_1
   err |= registerDiagUdpecho();
#endif
#ifdef DMP_DEVICE2_IPLAYERCAPACITYTEST_1
   err |= registerDiagIpLayerCap();
#endif
#ifdef DMP_DEVICE2_SERVERSELECTIONDIAG_1
   err |= registerDiagServerSelection();
#endif
   
   return err;
}

int vendorRegisterOpDslDiag(void)
{
   int err = USP_ERR_OK;

   /* register all the DSL diagnostic operations */
#ifdef DMP_DEVICE2_DSLDIAGNOSTICS_1
   err |= registerDiagAdslLineTest();
#endif
#ifdef DMP_DEVICE2_ATMLOOPBACK_1
   err |= registerDiagAtmF5Loopback();
#endif
   
   return err;
}

int vendorRegisterOpModsw(void)
{
   int err=USP_ERR_OK;

#ifdef SUPPORT_OPENPLAT
   static char *installDUInputArgs[]=
      {"URL", "UUID", "Username", "Password", "ExecutionEnvRef"};
   static char *updateDUInputArgs[]=
      {"URL", "Username", "Password"};
   static char *duStateChangeEventArg[]=
      {"UUID", "DeploymentUnitRef", "Version", "CurrentState", "Resolved",
       "ExecutionUnitRefList", "StartTime", "CompleteTime", "OperationPerformed",
       "Fault.FaultCode", "Fault.FaultString"};
   static char *setRunLevelInputArgs[]=
      {"RequestedRunLevel"};
   static char *setRequestedStateInputArgs[]=
      {"RequestedState"};

   /* Register InstallDU() command */
   err |= USP_REGISTER_AsyncOperation(MODSW_INSTALL_DU_COMMAND_PATH,
                                      vendorInstallDUCb, NULL);
   err |= USP_REGISTER_OperationArguments(MODSW_INSTALL_DU_COMMAND_PATH,
                                          installDUInputArgs,
                                          NUM_ELEM(installDUInputArgs),
                                          NULL, 0);

   /* Register DU Update() command */
   err |= USP_REGISTER_AsyncOperation(MODSW_UPDATE_DU_COMMAND_PATH,
                                      vendorUpdateDUCb, NULL);
   err |= USP_REGISTER_OperationArguments(MODSW_UPDATE_DU_COMMAND_PATH,
                                          updateDUInputArgs,
                                          NUM_ELEM(updateDUInputArgs),
                                          NULL, 0);

   /* Register DU Uninstall() command */
   err |= USP_REGISTER_AsyncOperation(MODSW_UNINSTALL_DU_COMMAND_PATH,
                                      vendorUninstallDUCb, NULL);
   err |= USP_REGISTER_OperationArguments(MODSW_UNINSTALL_DU_COMMAND_PATH,
                                          NULL, 0, NULL, 0);

   /* Register DUStateChange! event */
   err |= USP_REGISTER_Event(MODSW_DU_STATE_CHANGE_EVENT_PATH);
   err |= USP_REGISTER_EventArguments(MODSW_DU_STATE_CHANGE_EVENT_PATH,
                                      duStateChangeEventArg,
                                      NUM_ELEM(duStateChangeEventArg));

   /* Register EE SetRunLevel() command */
   err |= USP_REGISTER_SyncOperation(MODSW_EE_SETRUNLEVEL_COMMAND_PATH, eeSetRunLevel);
   err |= USP_REGISTER_OperationArguments(MODSW_EE_SETRUNLEVEL_COMMAND_PATH,
                                          setRunLevelInputArgs,
                                          NUM_ELEM(setRunLevelInputArgs),
                                          NULL, 0);

   /* Register EU SetRequestedState() command */
   err |= USP_REGISTER_SyncOperation(MODSW_EU_SETREQUESTEDSTATE_COMMAND_PATH, euSetRequestedState);
   err |= USP_REGISTER_OperationArguments(MODSW_EU_SETREQUESTEDSTATE_COMMAND_PATH,
                                          setRequestedStateInputArgs,
                                          NUM_ELEM(setRequestedStateInputArgs),
                                          NULL, 0);
#endif
   return err;
}

int registerDiagTraceRoute(void)
{
   int err = USP_ERR_OK;
   
   static char *traceRouteDiagInputArgs[] =
      {"Interface", "ProtocolVersion", "Host", "NumberOfTries", "Timeout",
       "DataBlockSize", "DataBlockSize", "DSCP", "MaxHopCount"};
   /* RouteHops.{i} */
   static char *traceRouteDiagOutputArgs[] =
      {"Status", "IPAddressUsed", "ResponseTime", "RouteHops.{i}.Host", "RouteHops.{i}.HostAddress", 
       "RouteHops.{i}.ErrorCode", "RouteHops.{i}.RTTimes"};

   err |= USP_REGISTER_AsyncOperation(DIAG_IP_TRACEROUTE_COMMAND_PATH,
                                      vendorDiagTraceRouteCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_TRACEROUTE_COMMAND_PATH,
                                          traceRouteDiagInputArgs,
                                          NUM_ELEM(traceRouteDiagInputArgs),
                                          traceRouteDiagOutputArgs,
                                          NUM_ELEM(traceRouteDiagOutputArgs));
   return (err);
}

int registerDiagPing(void)
{
   int err = USP_ERR_OK;
   
   static char *ipPingDiagInputArgs[] =
      {"Interface", "ProtocolVersion", "Host", "NumberOfRepetitions", "Timeout",
       "DataBlockSize", "DSCP"};
   static char *ipPingDiagOutputArgs[] =
      {"Status", "IPAddressUsed", "SuccessCount", "FailureCount", "AverageResponseTime",
       "MinimumResponseTime", "MaximumResponseTime", "AverageResponseTimeDetailed",
       "MinimumResponseTimeDetailed", "MaximumResponseTimeDetailed"};

   /* register all the IP diagnostic operations */
   /* TODO: restartcb */
   err |= USP_REGISTER_AsyncOperation(DIAG_IP_PING_COMMAND_PATH,
                                      vendorDiagIpPingCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_PING_COMMAND_PATH,
                                          ipPingDiagInputArgs,
                                          NUM_ELEM(ipPingDiagInputArgs),
                                          ipPingDiagOutputArgs,
                                          NUM_ELEM(ipPingDiagOutputArgs));
   return (err);
}

int registerDiagDownload(void)
{
   int err = USP_ERR_OK;
   
   static char *downloadDiagInputArgs[] =
      {"Interface", "DownloadURL", "DSCP", "EthernetPriority", "TimeBasedTestDuration",
       "TimeBasedTestMeasurementInterval", "TimeBasedTestMeasurementOffset",
       "ProtocolVersion", "NumberOfConnections", "EnablePerConnectionResults"};
   static char *downloadDiagOutputArgs[] =
      {"Status", "IPAddressUsed", "ROMTime", "BOMTime", "EOMTime", "TestBytesReceived",
       "TotalBytesReceived", "TotalBytesSent", "TestBytesReceivedUnderFullLoading",
       "TotalBytesReceivedUnderFullLoading", "TotalBytesSentUnderFullLoading",
       "PeriodOfFullLoading",
#ifdef DMP_DEVICE2_DOWNLOADTCP_1
       "TCPOpenRequestTime", "TCPOpenResponseTime",
#endif
       "PerConnectionResult.{i}.ROMTime", "PerConnectionResult.{i}.BOMTime",
       "PerConnectionResult.{i}.EOMTime", "PerConnectionResult.{i}.TestBytesReceived",
       "PerConnectionResult.{i}.TotalBytesReceived", "PerConnectionResult.{i}.TotalBytesSent",
#ifdef DMP_DEVICE2_DOWNLOADTCP_1
       "PerConnectionResult.{i}.TCPOpenRequestTime", "PerConnectionResult.{i}.TCPOpenResponseTime",
#endif
       "IncrementalResult.{i}.TestBytesReceived", "IncrementalResult.{i}.TotalBytesReceived",
       "IncrementalResult.{i}.TotalBytesSent", "IncrementalResult.{i}.StartTime",
       "IncrementalResult.{i}.EndTime" };


   err |= USP_REGISTER_AsyncOperation(DIAG_IP_DOWNLOAD_COMMAND_PATH,
                                      vendorDiagDownloadCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_DOWNLOAD_COMMAND_PATH,
                                          downloadDiagInputArgs,
                                          NUM_ELEM(downloadDiagInputArgs),
                                          downloadDiagOutputArgs,
                                          NUM_ELEM(downloadDiagOutputArgs));
   return (err);
}

int registerDiagUpload(void)
{
   int err = USP_ERR_OK;
   
   static char *uploadDiagInputArgs[] =
      {"Interface", "UploadURL", "DSCP", "EthernetPriority", "TestFileLength",
       "TimeBasedTestDuration", "TimeBasedTestMeasurementInterval", "TimeBasedTestMeasurementOffset",
       "ProtocolVersion", "NumberOfConnections", "EnablePerConnectionResults"};
   static char *uploadDiagOutputArgs[] =
      {"Status", "IPAddressUsed", "ROMTime", "BOMTime", "EOMTime", "TestBytesSent",
       "TotalBytesReceived", "TotalBytesSent", "TestBytesSentUnderFullLoading",
       "TotalBytesReceivedUnderFullLoading", "TotalBytesSentUnderFullLoading",
       "PeriodOfFullLoading",
#ifdef DMP_DEVICE2_UPLOADTCP_1
       "TCPOpenRequestTime", "TCPOpenResponseTime",
#endif
       "PerConnectionResult.{i}.ROMTime", "PerConnectionResult.{i}.BOMTime",
       "PerConnectionResult.{i}.EOMTime", "PerConnectionResult.{i}.TestBytesSent",
       "PerConnectionResult.{i}.TotalBytesReceived", "PerConnectionResult.{i}.TotalBytesSent",
#ifdef DMP_DEVICE2_UPLOADTCP_1
       "PerConnectionResult.{i}.TCPOpenRequestTime", "PerConnectionResult.{i}.TCPOpenResponseTime",
#endif
       "IncrementalResult.{i}.TestBytesSent", "IncrementalResult.{i}.TotalBytesReceived",
       "IncrementalResult.{i}.TotalBytesSent", "IncrementalResult.{i}.StartTime",
       "IncrementalResult.{i}.EndTime" };


   err |= USP_REGISTER_AsyncOperation(DIAG_IP_UPLOAD_COMMAND_PATH,
                                      vendorDiagUploadCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_UPLOAD_COMMAND_PATH,
                                          uploadDiagInputArgs,
                                          NUM_ELEM(uploadDiagInputArgs),
                                          uploadDiagOutputArgs,
                                          NUM_ELEM(uploadDiagOutputArgs));
   return (err);
}

int registerDiagUdpecho(void)
{
   int err = USP_ERR_OK;
   
   static char *udpechoDiagInputArgs[] =
      {"Interface", "Host", "Port", "NumberOfRepetitions", "Timeout",
       "DataBlockSize", "DSCP", "InterTransmissionTime",
       "ProtocolVersion", "EnableIndividualPacketResults"};
   static char *udpechoDiagOutputArgs[] =
      {"Status", "IPAddressUsed", "SuccessCount", "FailureCount", 
       "AverageResponseTime", "MinimumResponseTime", "MaximumResponseTime"};


   err |= USP_REGISTER_AsyncOperation(DIAG_IP_UDPECHO_COMMAND_PATH,
                                      vendorDiagUdpechoCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_UDPECHO_COMMAND_PATH,
                                          udpechoDiagInputArgs,
                                          NUM_ELEM(udpechoDiagInputArgs),
                                          udpechoDiagOutputArgs,
                                          NUM_ELEM(udpechoDiagOutputArgs));
   return (err);
}

int registerDiagIpLayerCap(void)
{
   int err = USP_ERR_OK;
   
   static char *ipLayerCapInputArgs[] =
      {"Role", "Host", "Port", "JumboFramesPermitted", "DSCP",
       "ProtocolVersion", "UDPPayloadContent", "TestType", "IPDVEnable",
       "IPRREnable", "StartSendingRate", "NumberTestSubIntervals",
       "NumberFirstModeTestSubIntervals", "TestSubInterval",
       "StatusFeedbackInterval", "SeqErrThresh", "LowerThresh", "UpperThresh",
       "HighSpeedDelta", "SlowAdjThresh"};
   static char *ipLayerCapOutputArgs[] =
      {"Status", "BOMTime", "EOMTime", "TmaxUsed", "TestInterval",
       "MaxIPLayerCapacity", "TimeOfMax", "MaxETHCapacityNoFCS",
       "MaxETHCapacityWithFCS", "MaxETHCapacityWithFCSVLAN", "LossRatioAtMax",
       "RTTRangeAtMax", "PDVRangeAtMax", "MinOnewayDelayAtMax",
       "ReorderedRatioAtMax", "ReplicatedRatioAtMax", "InterfaceEthMbpsAtMax",
       "IPLayerCapacitySummary", "LossRatioSummary", "RTTRangeSummary",
       "PDVRangeSummary", "MinOnewayDelaySummary", "MinRTTSummary",
       "ReorderedRatioSummary", "ReplicatedRatioSummary",
       "InterfaceEthMbpsSummary", "TmaxRTTUsed", "TimestampResolutionUsed",
       "ModalResult.{i}.MaxIPLayerCapacity",
       "ModalResult.{i}.TimeOfMax",
       "ModalResult.{i}.MaxETHCapacityNoFCS",
       "ModalResult.{i}.MaxETHCapacityWithFCS",
       "ModalResult.{i}.MaxETHCapacityWithFCSVLAN",
       "ModalResult.{i}.LossRatioAtMax",
       "ModalResult.{i}.RTTRangeAtMax",
       "ModalResult.{i}.PDVRangeAtMax",
       "ModalResult.{i}.MinOnewayDelayAtMax",
       "ModalResult.{i}.ReorderedRatioAtMax",
       "ModalResult.{i}.ReplicatedRatioAtMax",
       "ModalResult.{i}.InterfaceEthMbpsAtMax",
       "IncrementalResult.{i}.IPLayerCapacity",
       "IncrementalResult.{i}.TimeOfSubInterval",
       "IncrementalResult.{i}.LossRatio",
       "IncrementalResult.{i}.RTTRange",
       "IncrementalResult.{i}.PDVRange",
       "IncrementalResult.{i}.MinOnewayDelay",
       "IncrementalResult.{i}.ReorderedRatio",
       "IncrementalResult.{i}.ReplicatedRatio",
       "IncrementalResult.{i}.InterfaceEthMbps"};


   err |= USP_REGISTER_AsyncOperation(DIAG_IP_LAYERCAPACITY_COMMAND_PATH,
                                      vendorIpLayerCapCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_LAYERCAPACITY_COMMAND_PATH,
                                          ipLayerCapInputArgs,
                                          NUM_ELEM(ipLayerCapInputArgs),
                                          ipLayerCapOutputArgs,
                                          NUM_ELEM(ipLayerCapOutputArgs));
   return (err);
}

int registerDiagServerSelection(void)
{
   int err = USP_ERR_OK;
   
   static char *serverSelectionDiagInputArgs[] =
      {"Interface", "HostList", "NumberOfRepetitions", "Timeout",
       "Protocol", "ProtocolVersion"};
   static char *udpechoDiagOutputArgs[] =
      {"Status", "IPAddressUsed", "AverageResponseTime",
       "MinimumResponseTime", "MaximumResponseTime"};


   err |= USP_REGISTER_AsyncOperation(DIAG_IP_SEVERSELECTION_COMMAND_PATH,
                                      vendorDiagServerSelectionCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_IP_SEVERSELECTION_COMMAND_PATH,
                                          serverSelectionDiagInputArgs,
                                          NUM_ELEM(serverSelectionDiagInputArgs),
                                          udpechoDiagOutputArgs,
                                          NUM_ELEM(udpechoDiagOutputArgs));
   return (err);

}

int registerDiagAdslLineTest(void)
{
   int err = USP_ERR_OK;
   
   static char *adslLineTestInputArgs[] =
      {"Interface"};
   static char *adslLineTestOutputArgs[] =
      {"Status", "ACTPSDds", "ACTPSDus", "ACTATPds", "ACTATPus",
       "HLINSCds", "HLINSCus", "HLINGds", "HLINGus",
       "HLOGGds", "HLOGGus", "HLOGpsds", "HLOGpsus", "HLOGMTds", "HLOGMTus",
       "LATNpbds", "LATNpbus", "SATNds", "SATNus", "HLINpsds", "HLINpsus",
       "QLNGds", "QLNGus", "QLNpsds", "QLNpsus", "QLNMTds", "QLNMTus",
       "SNRGds", "SNRGus", "SNRpsds", "SNRpsus", "SNRMTds", "SNRMTus",
       "BITSpsds", "BITSpsus"};

   err |= USP_REGISTER_AsyncOperation(DIAG_DSL_ADSLLINETEST_COMMAND_PATH,
                                      vendorDiagAdslLineTestCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_DSL_ADSLLINETEST_COMMAND_PATH,
                                          adslLineTestInputArgs,
                                          NUM_ELEM(adslLineTestInputArgs),
                                          adslLineTestOutputArgs,
                                          NUM_ELEM(adslLineTestOutputArgs));
   return (err);
}

int registerDiagAtmF5Loopback(void)
{
   int err = USP_ERR_OK;
   
   static char *f5LoopbackInputArgs[] =
      {"Interface", "NumberOfRepetitions", "Timeout"};
   static char *f5LoopbackOutputArgs[] =
      {"Status", "SuccessCount", "FailureCount", "AverageResponseTime",
       "MinimumResponseTime", "MaximumResponseTime"};

   err |= USP_REGISTER_AsyncOperation(DIAG_ATM_F5LOOPBACK_COMMAND_PATH,
                                      vendorDiagAtmF5LoopbackCb, NULL);
   err |= USP_REGISTER_OperationArguments(DIAG_ATM_F5LOOPBACK_COMMAND_PATH,
                                          f5LoopbackInputArgs,
                                          NUM_ELEM(f5LoopbackInputArgs),
                                          f5LoopbackOutputArgs,
                                          NUM_ELEM(f5LoopbackOutputArgs));
   return (err);
}
