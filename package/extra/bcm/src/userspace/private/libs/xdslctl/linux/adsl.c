/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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


/* Includes. */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include "DiagDef.h"
#include "devctl_adsl.h"
#include "bcm_retcodes.h"

/* Globals. */
#ifdef SUPPORT_DSL_BONDING
#define MAX_DSL_LINE    2
static char    *g_xdslDevName[MAX_DSL_LINE] = {"/dev/bcmadsl0", "/dev/bcmadsl1"}; /* Make sure "/dev/bcmadsl1" is created in makeDevs */
#else
#define MAX_DSL_LINE    1
static char    *g_xdslDevName[MAX_DSL_LINE] = {"/dev/bcmadsl0"};
#endif

static int xdslCtl_Open(unsigned char lineId)
{
#ifdef DESKTOP_LINUX
   return (-1);
#else
   if(lineId >= MAX_DSL_LINE)
      return -1;
   return (open(g_xdslDevName[lineId], O_RDWR));
#endif
} /* xdslCtl_Open */

/*
**
** Interface to external MIB objects managed outside
**
*/

#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifdef SUPPORT_DSL_BONDING
#define EXT_OBJ_TBL_SIZE		(kExtObjNum << 1)
#define EXT_OBJ_TBL_IDX(id,ln)	((id) << 1) + (ln)
#else
#define EXT_OBJ_TBL_SIZE		kExtObjNum
#define EXT_OBJ_TBL_IDX(id,ln)	(id)
#endif

static extObjInfo	extObj[EXT_OBJ_TBL_SIZE] = { {NULL, 0} };

BcmRet xdslCtl_ExtObjFree(unsigned char lineId, int objId)
{
   void *pTmp;

   int  idx = EXT_OBJ_TBL_IDX(objId, lineId);

   if ((lineId >= MAX_DSL_LINE) || (objId >= kExtObjNum) || (NULL == extObj[idx].data))
     return BCMRET_INTERNAL_ERROR;

   pTmp = extObj[idx].data;
   free(pTmp);
   extObj[idx].data = NULL;
   return BCMRET_SUCCESS;
}

BcmRet xdslCtl_ExtObjAlloc(unsigned char lineId, int objId, extObjInfo *pObj)
{
   int  idx = EXT_OBJ_TBL_IDX(objId, lineId);

   if ((lineId >= MAX_DSL_LINE) || (objId >= kExtObjNum))
     return BCMRET_INTERNAL_ERROR;

   if (extObj[idx].data != NULL) {
     if (extObj[idx].size >= pObj->size) {
         pObj->data = extObj[idx].data;
         return BCMRET_SUCCESS;
     }
     xdslCtl_ExtObjFree(lineId, objId);
   }
   
   if (NULL == (pObj->data = malloc(pObj->size)))
     return BCMRET_INTERNAL_ERROR;

   extObj[idx].size = pObj->size;
   extObj[idx].data = pObj->data;
   return BCMRET_SUCCESS;
}

BcmRet xdslCtl_ExtObjGet(unsigned char lineId, int objId, extObjInfo *pObj)
{
   int  idx = EXT_OBJ_TBL_IDX(objId, lineId);

   if ((lineId >= MAX_DSL_LINE) || (objId >= kExtObjNum))
     return BCMRET_INTERNAL_ERROR;

   pObj->data = extObj[idx].data;
   pObj->size = extObj[idx].size;
   return BCMRET_SUCCESS;
}

#ifdef XDSLIOCTL_SEND_HMI_MSG
BcmRet xdslCtl_SendHmiMessage(
            unsigned char lineId,
            unsigned char *header,
            unsigned short headerSize,
            unsigned char *payload,
            unsigned short payloadSize,
            unsigned char *replyMessage,
            unsigned short replyMaxMessageSize)
{
   int             fd;
   XDSLDRV_HMI_MSG Arg;
   BcmRet nRet = BCMRET_SUCCESS;

   Arg.bvStatus = BCMADSL_STATUS_ERROR;

   if(lineId >= MAX_DSL_LINE)
     return BCMRET_INTERNAL_ERROR;

   fd = xdslCtl_Open(lineId);
   if( -1 == fd )
   {
      nRet = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      Arg.header = header;
      Arg.payload = payload;
      Arg.replyMessage = replyMessage;
      Arg.headerSize = headerSize;
      Arg.payloadSize = payloadSize;
      Arg.replyMaxMessageSize = replyMaxMessageSize;
      ioctl( fd, XDSLIOCTL_SEND_HMI_MSG, &Arg );
      close(fd);
      if (Arg.bvStatus != BCMADSL_STATUS_SUCCESS)
      {
         nRet = BCMRET_INTERNAL_ERROR;
      }
   }
   return( nRet );
}
#endif

BcmRet xdslCtl_OpenEocIntf(unsigned char lineId, int eocMsgType, char *pIfName)
{
   int               fd;
   XDSLDRV_EOC_IFACE Arg;
   BcmRet nRet = BCMRET_SUCCESS;

   Arg.bvStatus = BCMADSL_STATUS_ERROR;

   if(lineId >= MAX_DSL_LINE)
     return BCMRET_INTERNAL_ERROR;

   fd = xdslCtl_Open(lineId);
   if( -1 == fd )
   {
      nRet = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      Arg.eocMsgType = eocMsgType;
      Arg.ifName = pIfName;
      Arg.ifNameLen = strlen(pIfName);
      ioctl( fd, XDSLIOCTL_OPEN_EOC_IFACE, &Arg );
      close(fd);
      if (Arg.bvStatus != BCMADSL_STATUS_SUCCESS)
      {
         nRet = BCMRET_INTERNAL_ERROR;
      }
   }
   return( nRet );
}

BcmRet xdslCtl_CloseEocIntf(unsigned char lineId, int eocMsgType, char *pIfName)
{
   int               fd;
   XDSLDRV_EOC_IFACE Arg;
   BcmRet nRet = BCMRET_SUCCESS;

   Arg.bvStatus = BCMADSL_STATUS_ERROR;

   if(lineId >= MAX_DSL_LINE)
     return BCMRET_INTERNAL_ERROR;

   fd = xdslCtl_Open(lineId);
   if( -1 == fd )
   {
      nRet = BCMRET_INTERNAL_ERROR;
   }
   else
   {
      Arg.eocMsgType = eocMsgType;
      Arg.ifName = pIfName;
      Arg.ifNameLen = strlen(pIfName);
      ioctl( fd, XDSLIOCTL_CLOSE_EOC_IFACE, &Arg );
      close(fd);
      if (Arg.bvStatus != BCMADSL_STATUS_SUCCESS)
      {
         nRet = BCMRET_INTERNAL_ERROR;
      }
   }
   return( nRet );
}

BcmRet xdslCtl_SetOemParam(unsigned char lineId, int paramId, void *buf, int len)
{
   int fd;
   ADSLDRV_SET_OEM_PARAM Arg;
   BcmRet nRet = BCMRET_SUCCESS;

   if (lineId >= MAX_DSL_LINE)
   {
     fprintf(stderr, "%s: lineId error, got %d max=%d\n",
             __FUNCTION__, lineId, MAX_DSL_LINE);
     return BCMRET_INVALID_ARGUMENTS;
   }

   if (buf == NULL)
   {
      fprintf(stderr, "%s: got NULL buf!\n", __FUNCTION__);
      return BCMRET_INVALID_ARGUMENTS;
   }

   fd = xdslCtl_Open(lineId);
   if( -1 == fd )
   {
      nRet = BCMRET_INTERNAL_ERROR;
      fprintf(stderr, "%s: open error, lineId=%d\n", __FUNCTION__, lineId);
   }
   else
   {
      memset(&Arg, 0, sizeof(Arg));
      Arg.paramId = paramId;
      Arg.buf = buf;
      Arg.len = len;
      ioctl( fd, ADSLIOCTL_SET_OEM_PARAM, &Arg );
      close(fd);
      // Unlike the other ioctls, on success, this one sets bvStatus to len.
      if (Arg.bvStatus != len)
      {
         fprintf(stderr, "%s: ADSLIOCTL_SET_OEM_PARAM (%d) returned status %d\n",
                 __FUNCTION__, paramId, Arg.bvStatus);
         nRet = BCMRET_INTERNAL_ERROR;
      }
   }
   return( nRet );
}

BcmRet xdslCtl_DiagProcessCommandFrame(UINT8 lineId, void *diagBuf, int diagBufLen)
{
    int                 fd;
    ADSLDRV_DIAG        Arg;
    DiagProtoFrame      *diagCmd = diagBuf;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;
    
    fd = xdslCtl_Open(lineId);
    if( fd == -1 )
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    else
    {
        Arg.diagCmd = diagCmd->diagHdr.logCommmand;
        if (LOG_CMD_CONNECT != Arg.diagCmd)
        {
            if (LOG_CMD_DISCONNECT != Arg.diagCmd) {
                Arg.diagCmd |= (((diagCmd->diagHdr.logPartyId & DIAG_PARTY_TYPE_SEND_MASK) >> DIAG_PARTY_TYPE_SEND_SHIFT) << DIAG_TYPE_CMD_SHIFT);
                Arg.diagMap = (unsigned long) diagCmd->diagData;
                Arg.srvIpAddr = *(short *)diagCmd->diagHdr.logProtoId;
            }
            else {
                Arg.diagMap = diagCmd->diagHdr.logPartyId & 0xFF;
                Arg.srvIpAddr = *((int *)diagCmd->diagData);
            }
            Arg.logTime = diagBufLen - sizeof(LogProtoHeader);
        }
        else
        {
#define SRVIPADDR_LOCALHOST_IP      0x0100007F // 127.0.0.1
	  // To enable skb rerouting via IOCTL
	  // 1. set SKB_REROUTE_ENABLE to 1
	  // 2. set USE_DEV_TRANSMIT to 0 in BcmAdslDiagLinux.h
#define SKB_REROUTE_ENABLE 1
#define SKB_REROUTE_FORCED (SKB_REROUTE_ENABLE && 0) // to enable, toggle '0' to '1'
#define SKB_REROUTE_FORCED_GDB ((!SKB_REROUTE_FORCED) && (SKB_REROUTE_ENABLE && 0)) // to enable, disable SKB_REROUTE_FORCED and toggle '0' to '1'

            int do_reroute = 0; // must be initialized
            
            int * pConnect = (int *) diagCmd->diagData;
            Arg.diagMap   = pConnect[0];
            Arg.logTime   = pConnect[1];
            Arg.srvIpAddr = pConnect[2];
            Arg.gwIpAddr  = pConnect[3];

#if defined(SKB_REROUTE_ENABLE) && SKB_REROUTE_ENABLE
#if defined(SKB_REROUTE_FORCED_GDB) && SKB_REROUTE_FORCED_GDB
            do_reroute = (Arg.diagMap & DIAG_DATA_GDB_ID);
#elif defined(SKB_REROUTE_FORCED) && SKB_REROUTE_FORCED
            do_reroute = 1;
#else
            // we want to reroute gdb-over-tcp always
            if ( (do_reroute=(Arg.diagMap & (DIAG_DATA_GDB_ID | DIAG_DATA_TCP_ID))==(DIAG_DATA_GDB_ID | DIAG_DATA_TCP_ID)) )
                Arg.diagMap &= ~DIAG_DATA_TCP_ID;
#endif

            if (do_reroute)
            {
                /**
                 * We are preserving srvIpAddr in gwIpAddr
                 * so srvIpAddr would indicate that 
                 * we want to do skb reroute through ioctl and not DEV_TRANSMIT
                 */
                Arg.gwIpAddr = Arg.srvIpAddr;
                Arg.srvIpAddr = 0;
            }
#endif // end of #if defined(SKB_REROUTE_ENABLE) && SKB_REROUTE_ENABLE
        }
        ioctl( fd, ADSLIOCTL_DIAG_COMMAND, &Arg );
        close(fd);

        if (Arg.bvStatus != BCMADSL_STATUS_SUCCESS)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    return( nRet );
}

BcmRet devCtl_adslDiagProcessCommandFrame(void *diagBuf, int diagBufLen)
{
   return xdslCtl_DiagProcessCommandFrame(0, diagBuf, diagBufLen);
}

BcmRet xdslCtl_DiagProcessDbgCommand(UINT8 lineId, UINT16 cmd, UINT16 cmdId, UINT32 param1, UINT32 param2)
{
    int             fd;
    ADSLDRV_DIAG    Arg;
    DiagDebugData   diagCmd;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd == -1  )
   {
        return BCMRET_INTERNAL_ERROR;
   }
   else
   {
      diagCmd.cmd       = cmd;
      diagCmd.cmdId = cmdId;
      diagCmd.param1    = param1;
      diagCmd.param2    = param2;
    
      Arg.diagCmd       = LOG_CMD_DEBUG;
      Arg.diagMap       = (unsigned long) &diagCmd;
      Arg.logTime       = sizeof(diagCmd);
      Arg.srvIpAddr    = 0;
    
      ioctl( fd, ADSLIOCTL_DIAG_COMMAND, &Arg );
      close(fd);

      if (Arg.bvStatus != BCMADSL_STATUS_SUCCESS)
      {
         nRet = BCMRET_INTERNAL_ERROR;
      }
   }

   return( nRet );
}

BcmRet BcmAdsl_DiagProcessDbgCommand(UINT16 cmd, UINT16 cmdId, UINT32 param1, UINT32 param2)
{
   return xdslCtl_DiagProcessDbgCommand(0, cmd, cmdId, param1, param2);
}

BcmRet xdslCtl_Check(UINT8 lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1  )
    {
        ioctl( fd, ADSLIOCTL_CHECK, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
} /* Bcmxdsl_Check */

BcmRet devCtl_adslCheck(void)
{
    return xdslCtl_Check(0);
} /* BcmAdsl_Check */

BcmRet xdslCtl_Initialize(UINT8 lineId, ADSL_FN_NOTIFY_CB pFnNotifyCb, void *pParm, adslCfgProfile *pAdslCfg)
{
    int                fd;
    ADSLDRV_INITIALIZE Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.pFnNotifyCb = pFnNotifyCb;
    Arg.pParm = pParm;
    Arg.pAdslCfg = pAdslCfg;
    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;
    
    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_INITIALIZE, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    
    return( nRet );
} /* BcmAdsl_Initialize */

BcmRet devCtl_adslInitialize(ADSL_FN_NOTIFY_CB pFnNotifyCb, void *pParm, adslCfgProfile *pAdslCfg)
{
    return xdslCtl_Initialize(0, pFnNotifyCb, pParm, pAdslCfg);
} /* BcmAdsl_Initialize */

/***************************************************************************
 * Function Name: BcmAdsl_Uninitialize
 * Description  : Clean up resources allocated during BcmAdsl_Initialize.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_Uninitialize(UINT8 lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;
    
    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_UNINITIALIZE, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    
    return( nRet );
} /* BcmAdsl_Uninitialize */

BcmRet devCtl_adslUninitialize(void)
{
    return xdslCtl_Uninitialize(0);
} /* BcmAdsl_Uninitialize */

/***************************************************************************
 * Function Name: BcmAdsl_ConnectionStart
 * Description  : Start ADSL connection.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_ConnectionStart(UINT8 lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;
    
    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_CONNECTION_START, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else 
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
} /* BcmAdsl_ConnectionStart */

BcmRet devCtl_adslConnectionStart( void )
{
    return xdslCtl_ConnectionStart(0);
} /* BcmAdsl_ConnectionStart */


/***************************************************************************
 * Function Name: BcmAdsl_ConnectionStop
 * Description  : Clean up resources allocated during BcmAdsl_Initialize.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_ConnectionStop(UINT8 lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;

    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_CONNECTION_STOP, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
} /* BcmAdsl_ConnectionStop */

BcmRet devCtl_adslConnectionStop( void )
{
    return xdslCtl_ConnectionStop(0);
} /* BcmAdsl_ConnectionStop */


/***************************************************************************
 * Function Name: BcmAdsl_GetPhyAddresses
 * Description  : Returns a ADSL PHY address.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_GetPhyAddresses(UINT8 lineId, PADSL_CHANNEL_ADDR pChannelAddr )
{
    int              fd;
    ADSLDRV_PHY_ADDR Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;
    
    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_GET_PHY_ADDR, &Arg );
        close(fd);

        pChannelAddr->usFastChannelAddr = Arg.ChannelAddr.usFastChannelAddr;
        pChannelAddr->usInterleavedChannelAddr = Arg.ChannelAddr.usInterleavedChannelAddr;
        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }

    return( nRet );
} /* BcmAdsl_GetPhyAddresses */

BcmRet devCtl_adslGetPhyAddresses( PADSL_CHANNEL_ADDR pChannelAddr )
{
    return xdslCtl_GetPhyAddresses(0, pChannelAddr);
} /* BcmAdsl_GetPhyAddresses */


/***************************************************************************
 * Function Name: BcmAdsl_SetPhyAddresses
 * Description  : Sets a ADSL PHY address.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_SetPhyAddresses(UINT8 lineId, PADSL_CHANNEL_ADDR pChannelAddr)
{
    int              fd;
    ADSLDRV_PHY_ADDR Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.ChannelAddr.usFastChannelAddr = pChannelAddr->usFastChannelAddr;
    Arg.ChannelAddr.usInterleavedChannelAddr = pChannelAddr->usInterleavedChannelAddr;
    Arg.bvStatus = BCMADSL_STATUS_ERROR;

    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1  )
    {
        ioctl( fd, ADSLIOCTL_SET_PHY_ADDR, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }

    return( nRet );
} /* BcmAdsl_SetPhyAddresses */

BcmRet devCtl_adslSetPhyAddresses( PADSL_CHANNEL_ADDR pChannelAddr )
{
    return xdslCtl_SetPhyAddresses(0, pChannelAddr);
} /* BcmAdsl_SetPhyAddresses */


/***************************************************************************
 * Function Name: BcmAdsl_MapAtmPortIDs
 * Description  : Maps ATM Port IDs to DSL PHY Utopia Addresses.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_MapAtmPortIDs(UINT8 lineId, UINT16 usAtmFastPortId, UINT16 usAtmInterleavedPortId)
{
    int                  fd;
    ADSLDRV_MAP_ATM_PORT Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.usAtmFastPortId = usAtmFastPortId;
    Arg.usAtmInterleavedPortId = usAtmInterleavedPortId;
    Arg.bvStatus = BCMADSL_STATUS_ERROR;

    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_MAP_ATM_PORT_IDS, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
} /* BcmAdsl_SetPhyAddresses */

BcmRet devCtl_adslMapAtmPortIDs(UINT16 usAtmFastPortId, UINT16 usAtmInterleavedPortId)
{
   return xdslCtl_MapAtmPortIDs(0, usAtmFastPortId, usAtmInterleavedPortId);
} /* BcmAdsl_SetPhyAddresses */


/***************************************************************************
 * Function Name: BcmAdsl_GetConnectionInfo
 * Description  : Sets a ADSL PHY address.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_GetConnectionInfo(UINT8 lineId, PADSL_CONNECTION_INFO pConnInfo )
{
    int                     fd;
    ADSLDRV_CONNECTION_INFO Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1  )
    {
        ioctl( fd, ADSLIOCTL_GET_CONNECTION_INFO, &Arg );
        close(fd);

        pConnInfo->LinkState = Arg.ConnectionInfo.LinkState;
#ifdef SUPPORT_VECTORINGD
        pConnInfo->errorSamplesAvailable = Arg.ConnectionInfo.errorSamplesAvailable;
#endif
        pConnInfo->ulFastUpStreamRate = Arg.ConnectionInfo.ulFastUpStreamRate;
        pConnInfo->ulFastDnStreamRate = Arg.ConnectionInfo.ulFastDnStreamRate;
        pConnInfo->ulInterleavedUpStreamRate = Arg.ConnectionInfo.ulInterleavedUpStreamRate;
        pConnInfo->ulInterleavedDnStreamRate = Arg.ConnectionInfo.ulInterleavedDnStreamRate;
        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
} /* BcmAdsl_GetConnectionInfo */

BcmRet devCtl_adslGetConnectionInfo( PADSL_CONNECTION_INFO pConnInfo )
{
    return xdslCtl_GetConnectionInfo(0, pConnInfo);
} /* BcmAdsl_GetConnectionInfo */

/***************************************************************************
 * Function Name: BcmAdsl_SetObjectValue
 * Description  : Sets MIB object value
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_SetObjectValue(unsigned char lineId, char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
    int                 fd;
    ADSLDRV_GET_OBJ     Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;
    
    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       Arg.objId        = objId;
       Arg.objIdLen = objIdLen;
       Arg.dataBuf      = dataBuf;
       Arg.dataBufLen   = *dataBufLen;
       ioctl( fd, ADSLIOCTL_SET_OBJ_VALUE, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslSetObjectValue(char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
    return xdslCtl_SetObjectValue(0, objId, objIdLen, dataBuf, dataBufLen);
}

/***************************************************************************
 * Function Name: BcmAdsl_GetObjectValue
 * Description  : Gets MIB object value
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_GetObjectValue(unsigned char lineId, char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
    int                 fd;
    ADSLDRV_GET_OBJ     Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    if ((objId != NULL) && (2 == objIdLen) && (kOidAdslPrivate == objId[0])) {
      int extObjId = -1;
      switch (objId[1]) {
        case kOidAdslPrivSNRmin:
          extObjId = kExtObjSnrMin;
          break;
        case kOidAdslPrivSNRmax:
          extObjId = kExtObjSnrMax;
          break;
        case kOidAdslPrivShowtimeMarginMin:
          extObjId = kExtObjSnrmMin;
          break;
        case kOidAdslPrivShowtimeMarginMax:
          extObjId = kExtObjSnrmMax;
          break;
      }
      if (extObjId >= 0) {
        extObjInfo *pObj = &extObj[EXT_OBJ_TBL_IDX(extObjId, lineId)];
        int  size;

        if (NULL == pObj->data)
          return BCMRET_INTERNAL_ERROR;

        size = MIN(pObj->size, *dataBufLen);
        memcpy(dataBuf, pObj->data, size);
        *dataBufLen = size;
        return BCMRET_SUCCESS;
      }
    }

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       Arg.objId        = objId;
       Arg.objIdLen = objIdLen;
       Arg.dataBuf      = dataBuf;
       Arg.dataBufLen   = *dataBufLen;
       ioctl( fd, ADSLIOCTL_GET_OBJ_VALUE, &Arg );
       close(fd);

       *dataBufLen = Arg.dataBufLen;
       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslGetObjectValue(char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
    return xdslCtl_GetObjectValue(0, objId, objIdLen, dataBuf, dataBufLen);
}

/***************************************************************************
 * Function Name: BcmAdsl_StartBERT
 * Description  : Starts BERT test in ADSL PHY
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_StartBERT(unsigned char lineId, UINT32  totalBits)
{
    int                 fd;
    ADSLDRV_BERT        Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       Arg.totalBits = totalBits;
       ioctl( fd, ADSLIOCTL_START_BERT, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
           nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslStartBERT(UINT32  totalBits)
{
    return xdslCtl_StartBERT(0, totalBits);
}

/***************************************************************************
 * Function Name: BcmAdsl_StopBERT
 * Description  : Stops BERT test in ADSL PHY
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/
BcmRet xdslCtl_StopBERT(UINT8 lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       ioctl( fd, ADSLIOCTL_STOP_BERT, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslStopBERT(void)
{
    return xdslCtl_StopBERT(0);
}

//**************************************************************************
// Function Name: BcmAdsl_BertStartEx
// Description  : Start BERT test
// Returns      : STS_SUCCESS if successful or error status.
//**************************************************************************
BcmRet xdslCtl_BertStartEx(unsigned char lineId, UINT32  bertSec)
{
    int                 fd;
    ADSLDRV_BERT_EX     Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        Arg.totalSec = bertSec;
        ioctl( fd, ADSLIOCTL_START_BERT_EX, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslBertStartEx(UINT32  bertSec)
{
    return xdslCtl_BertStartEx(0, bertSec);
}

//**************************************************************************
// Function Name: BcmAdsl_BertStopEx
// Description  : Stops BERT test
// Returns      : STS_SUCCESS if successful or error status.
//**************************************************************************
BcmRet xdslCtl_BertStopEx(unsigned char lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       ioctl( fd, ADSLIOCTL_STOP_BERT_EX, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }

    return( nRet );
}

BcmRet devCtl_adslBertStopEx(void)
{
    return xdslCtl_BertStopEx(0);
}

//**************************************************************************
// Function Name: BcmAdsl_Configure
// Description  : Changes ADSL current configuration
// Returns      : STS_SUCCESS if successful or error status.
//**************************************************************************
BcmRet xdslCtl_Configure(unsigned char lineId, adslCfgProfile *pAdslCfg)
{
    int                 fd;
    ADSLDRV_CONFIGURE   Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.pAdslCfg = pAdslCfg;
    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_CONFIGURE, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
           nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslConfigure(adslCfgProfile *pAdslCfg)
{
    return xdslCtl_Configure(0, pAdslCfg);
}

//**************************************************************************
// Function Name: BcmAdsl_GetVersion
// Description  : Changes ADSL version information
// Returns      : STS_SUCCESS 
//**************************************************************************
BcmRet xdslCtl_GetVersion(unsigned char lineId, adslVersionInfo *pAdslVer)
{
    int                 fd;
    ADSLDRV_GET_VERSION     Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.pAdslVer  = pAdslVer;
    Arg.bvStatus = BCMADSL_STATUS_ERROR;

    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        ioctl( fd, ADSLIOCTL_GET_VERSION, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslGetVersion(adslVersionInfo *pAdslVer)
{
    return xdslCtl_GetVersion(0, pAdslVer);
}

BcmRet xdslCtl_SetSDRAMBaseAddr(unsigned char lineId, void *pAddr)
{
    int                     fd;
    ADSLDRV_SET_SDRAM_BASE  Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       Arg.sdramBaseAddr = pAddr;
       ioctl( fd, ADSLIOCTL_SET_SDRAM_BASE, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslSetSDRAMBaseAddr(void *pAddr)
{
    return xdslCtl_SetSDRAMBaseAddr(0, pAddr);
}

BcmRet xdslCtl_ResetStatCounters(unsigned char lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       ioctl( fd, ADSLIOCTL_RESET_STAT_COUNTERS, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslResetStatCounters(void)
{
    return xdslCtl_ResetStatCounters(0);
}

//**************************************************************************
// Function Name: BcmAdsl_SetTestMode
// Description  : Sets ADSL special test mode
// Returns      : STS_SUCCESS if successful or error status.
//**************************************************************************
BcmRet xdslCtl_SetTestMode(unsigned char lineId, ADSL_TEST_MODE testMode)
{
    int             fd;
    ADSLDRV_TEST    Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;

    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        Arg.testCmd   = testMode;
        Arg.xmtStartTone    = 0;
        Arg.xmtNumTones     = 0;
        Arg.rcvStartTone    = 0;
        Arg.rcvNumTones     = 0;
        Arg.xmtToneMap      = NULL;
        Arg.rcvToneMap      = NULL;
        ioctl( fd, ADSLIOCTL_TEST, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslSetTestMode(ADSL_TEST_MODE testMode)
{
   return xdslCtl_SetTestMode(0, testMode);
}

//**************************************************************************
// Function Name: BcmAdsl_SelectTones
// Description  : Test function to set tones used by the ADSL modem
// Returns      : STS_SUCCESS if successful or error status.
//**************************************************************************
BcmRet xdslCtl_SelectTones(
    unsigned char lineId,
    int     xmtStartTone,
    int     xmtNumTones,
    int     rcvStartTone,
    int     rcvNumTones,
    char    *xmtToneMap,
    char    *rcvToneMap)
{
    int                 fd;
    ADSLDRV_TEST        Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;

    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
        Arg.testCmd         = ADSL_TEST_SELECT_TONES;
        Arg.xmtStartTone    = xmtStartTone;
        Arg.xmtNumTones     = xmtNumTones;
        Arg.rcvStartTone    = rcvStartTone;
        Arg.rcvNumTones     = rcvNumTones;
        Arg.xmtToneMap      = xmtToneMap;
        Arg.rcvToneMap      = rcvToneMap;
        ioctl( fd, ADSLIOCTL_TEST, &Arg );
        close(fd);

        if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
        {
            nRet = BCMRET_INTERNAL_ERROR;
        }
    }
    else
    {
        nRet = BCMRET_INTERNAL_ERROR;
    }
    return( nRet );
}

BcmRet devCtl_adslSelectTones(
    int     xmtStartTone,
    int     xmtNumTones,
    int     rcvStartTone,
    int     rcvNumTones,
    char    *xmtToneMap,
    char    *rcvToneMap)
{
    return xdslCtl_SelectTones(0, xmtStartTone, xmtNumTones, rcvStartTone, rcvNumTones, xmtToneMap, rcvToneMap);
}

/***************************************************************************
 * Function Name: BcmAdsl_GetConstellationPoints
 * Description  : Gets constellation point for selected tone
 * Returns      : number of points rettrieved 
 ***************************************************************************/
int xdslCtl_GetConstellationPoints (unsigned char lineId, int toneId, ADSL_CONSTELLATION_POINT *pointBuf, int numPoints)
{
    int                         fd;
    ADSLDRV_GET_CONSTEL_POINTS  Arg;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       Arg.toneId       = toneId;
       Arg.pointBuf = pointBuf;
       Arg.numPoints    = numPoints;
       ioctl( fd, ADSLIOCTL_GET_CONSTEL_POINTS, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          numPoints = 0;
       }
       else 
       {
          numPoints = Arg.numPoints;
       }
    }
    else
    {
       numPoints = 0;
    }
    return (numPoints);
}

int devCtl_adslGetConstellationPoints (int toneId, ADSL_CONSTELLATION_POINT *pointBuf, int numPoints)
{
    return xdslCtl_GetConstellationPoints(0, toneId, pointBuf, numPoints);
}

BcmRet xdslCtl_CallBackDrv(unsigned char lineId)
{
    int                 fd;
    ADSLDRV_STATUS_ONLY Arg;
    BcmRet nRet = BCMRET_SUCCESS;

    Arg.bvStatus = BCMADSL_STATUS_ERROR;
    
    if(lineId >= MAX_DSL_LINE)
        return BCMRET_INTERNAL_ERROR;

    fd = xdslCtl_Open(lineId);
    if( fd != -1 )
    {
       ioctl( fd, ADSLIOCTL_DRV_CALLBACK, &Arg );
       close(fd);

       if (Arg.bvStatus == BCMADSL_STATUS_ERROR)
       {
          nRet = BCMRET_INTERNAL_ERROR;
       }
    }
    else
    {
       nRet = BCMRET_INTERNAL_ERROR;
    }

    return( nRet );
}

