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

/**
 * \file vendor_op_firmware.h
 *
 * This header file is used by both vendor_op_firmware.c and vendor_op_configfile.c
 *
 */

#ifndef __VENDOR_OP_FIRMWARE_H__
#define __VENDOR_OP_FIRMWARE_H__

#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "cms_msg_usp.h"
#include "bcm_webproto.h"


#define VENDOR_DOWNLOAD_REBOOT_DELAY     60
#define VENDOR_CONFIGFILE_REBOOT_DELAY   30
#define VENDOR_DOWNLOAD_ACTIVATE_DELAY   15


// Internal state for FirmwareImage.Download() and VendorConfigFile.Restore()
typedef struct 
{
   int    magicId;  // See WEBPROTO_MAGICID_xxx in bcm_webproto.h
   int    status;   // 0: idle, 1: running, 2: thread done, check uspErr for result.
   int    uspErr;   // download thread sets this when done
   int    requestInstance;  // instance id of the LocalAgent.Request obj
   int    isFirmwareDownload;  // in use by FirmwareImage.Download()
   int    isConfigFileRestore;  // in use by VendorConfigFile.Restore()
   time_t    startTime;
   time_t    stopTime;
   pthread_t pthreadId;  // pthread id of the child download thread.
   void *    msgHandle;  // msgHandle for the child download thread
   UINT32    actualFileSize;  // actual file size downloaded
   void *    imgBuf;    // downloaded buffer
   UspTestDownloadMsgBody body;  // most args are in this in msg, so just reuse it here.
   char      errMsg[256];
} VendorDownloadState;

typedef struct 
{
   int    magicId;  // See WEBPROTO_MAGICID_xxx in bcm_webproto.h
   int    status;   // 0: idle, 1: running
   int    requestInstance;  // instance id of the LocalAgent.Request obj
   UINT32 fwInstance;       // inactive firmware instance to activate
   pthread_t pthreadId;   // pthread id of the child activate thread.
   void *    msgHandle;   // msgHandle for the child activate thread
   UspTestActivateMsgBody body;
} VendorActivateState;

// For VendorConfigFile.{i}.Backup() cmd.  Basically, upload config file.
// This will be shared with VendorLogFile.{i}.Upload(), which is not impl yet.
typedef struct 
{
   int    magicId;  // See WEBPROTO_MAGICID_xxx in bcm_webproto.h
   int    status;   // 0: idle, 1: running
   int    uspErr;   // upload thread sets this when done
   int    requestInstance;  // instance id of the LocalAgent.Request obj
   int    isConfigFileBackup;  // in use by VendorConfigFile.Backup()
   int    isLogFileUpload;     // in use by VendorLogFile.Upload()
   time_t    startTime;
   time_t    stopTime;
   pthread_t pthreadId;   // pthread id of the child backup thread.
   void *    msgHandle;   // msgHandle for the child backup thread
   char *    buf;  // data to upload, could be configfile or log file
   UspTestUploadMsgBody body;
   char      errMsg[256];
} VendorUploadState;


// Functions shared by vendor_op_firmware.c and vendor_op_configfile.c
extern const char *webproto_getAcsUrl(void);
extern eWanState getWanState(void);
extern UBOOL8 isTimeSynchronized(void);
extern int www_EstablishConnection(struct sockaddr_storage host_addr, int *sock_fd);

extern void sendTransferCompleteEvent(int requestInstance,
                               char *transferType,
                               char *url,
                               time_t startTime, time_t stopTime,
                               int uspErr, char *errMsg);

extern void signalOperationComplete(int requestInstance,
                                    int uspErr, char *errMsg);

extern void vendor_up_down_eventLoop(WebprotoCB *appCB, int *childStatusPtr);

#endif /* __VENDOR_OP_FIRMWARE_H__ */
