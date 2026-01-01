/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

#ifndef _BCM_DIAG_DRV_H
#define _BCM_DIAG_DRV_H

#define CONFIG_BCM_PHY_DIAG_DRV

#include <bcmtypes.h>
#if defined(__KERNEL__)
#include <linux/version.h>
#include <linux/skbuff.h>
#endif

#define  DIAGDRV_MAJOR            334 /* arbitrary unused value, see targets/fs.src/etc/make_static_devnodes.sh */

#define  DIAGIOCTL_DIAG_COMMAND \
   _IOR(DIAGDRV_MAJOR, 0, DIAGDRV_DIAG)
#define  DIAGIOCTL_OPEN_PROCIF \
	_IOR(DIAGDRV_MAJOR, 1, DIAGDRV_PROC_IF)
#define  DIAGIOCTL_CLOSE_PROCIF \
	_IOR(DIAGDRV_MAJOR, 2, DIAGDRV_PROC_IF)
#define  MAX_DIAGDRV_IOCTL_COMMANDS   3


#define BCM_DIAG_MAX_PROC_IFNAME_LEN   32
#define BCM_DIAG_PROC_SKB_MSG          0


// Return status values
typedef enum BcmDiagStatus
{
   BCMDIAG_STATUS_SUCCESS = 0,
   BCMDIAG_STATUS_ERROR
} BCMDIAG_STATUS;


typedef struct
{
   int diagCmd;
   BCM_IOC_PTR(unsigned long, diagMap);
   int logTime;
   int srvIpAddr;
   int gwIpAddr;
   BCMDIAG_STATUS bvStatus;
   BCM_IOC_PTR(void *, skbModel);
} DIAGDRV_DIAG, *PDIAGDRV_DIAG;

typedef struct
{
    BCM_IOC_PTR(char *, ifName);
    int  ifNameLen;
    int  procMsgType;
    BCMDIAG_STATUS bvStatus;
} DIAGDRV_PROC_IF, *PDIAGDRV_PROC_IF;

#if defined(__KERNEL__)

typedef struct BcmDiagDrvSkbPool {
   struct sk_buff **skbPool;
   struct sk_buff *skbModel;
   int            numOfSkbs;
   int            numOfShortSkbs;
   int            skbLengh;
   int            shortSkbLengh;
   int            skbBufIndex;
   int            shortSkbBufIndex;
   int            frameHeaderLen;
   int            dataAlignMask;
   unsigned int   extraSkb;
   int            skbHeadRoomReserve;
} BcmDiagDrvSkbPool;

#define  dslDrvSkbPool  BcmDiagDrvSkbPool

#ifndef USE_DEV_TRANSMIT   /* In case the definition is coming from outside, we want to use that */
#define USE_DEV_TRANSMIT 0
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
/* Note: Linux4.04L.02 and older don't have "netdev_ops->ndo_start_xmit" */
#define	DEV_TRANSMIT_(x)	(x)->dev->netdev_ops->ndo_start_xmit (x, (x)->dev)
#else
#define	DEV_TRANSMIT_(x)	dev_queue_xmit(x)
#endif

#if defined(USE_DEV_TRANSMIT) && USE_DEV_TRANSMIT
#define DEV_TRANSMIT(x) DEV_TRANSMIT_(x)
#else
#define DEV_TRANSMIT(x) BcmDiagPendingSkbAdd(x)
#endif

#ifdef __GNUC__
#define	ALIGN_PACKED __attribute__ ((packed))
#else
#define	ALIGN_PACKED
#endif

typedef int DiagCommandCode;

typedef     struct __diagCommandStruct
{
   DiagCommandCode   command;
   union
   {
      int            value;
      unsigned char  flag;
      struct
      {
         unsigned int   type;
         unsigned int   imagePtr0;
         unsigned int   imageSize0;
         unsigned int   imagePtr;
         unsigned int   imageSize;
      } diagImageTestSpec;
   } param ALIGN_PACKED;
} diagCommandStruct;

#define UN_INIT_HDR_MARK    0x80

typedef struct {
   char           *pSdram;
   unsigned int   size;
   unsigned int   cnt;
   unsigned int   frameCnt;
} diagImageTestData;

void BcmDiagImageTestAck (unsigned int clientType, diagImageTestData *pImageData, uint32_t frNum, uint32_t frRcv);
int BcmDiagPendingSkbAdd(struct sk_buff* skb);

BcmDiagDrvSkbPool * BcmDiagDevSkbAllocate(
   struct sk_buff *model,
   int skbMaxBufSize, int numOfSkbsInPool,
   int shortSkbMaxBufSize, int numOfShortSkbsInPool,
   int dataAlignMask, int frameHeaderLength, int dmaZone, int skbHeadRoomReserve);

int BcmDiagDevSkbFree(BcmDiagDrvSkbPool *skbDev, int enabeWA);
struct sk_buff * BcmDiagGetSkb(BcmDiagDrvSkbPool *skbDev, int len);
struct sk_buff * BcmDiagGetClientSkb(unsigned int clientType, int len);

#define  DevSkbAllocate    BcmDiagDevSkbAllocate
#define  DevSkbFree        BcmDiagDevSkbFree
#define  GetSkb            BcmDiagGetSkb

#endif   // defined(__KERNEL__)

#endif

