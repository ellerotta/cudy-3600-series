/***********************************************************************
 *
 *  Copyright (c) 2018 Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

/*****************************************************************************
*    Description:
*
*      Constants and Macros related to OMCI initial configuration in CMS.
*
*****************************************************************************/

#ifndef _CMSOMCI_H
#define _CMSOMCI_H

/* ---- Include Files ----------------------------------------------------- */

#include "os_defs.h"


/* ---- Constants and Types ----------------------------------------------- */

typedef enum
{
    OMCI_CONN_UNCONFIGURE = 0,
    OMCI_CONN_CONNECTING,
    OMCI_CONN_CONNECTED,
    OMCI_CONN_PENDING_DISCONNECT,
    OMCI_CONN_DISCONNECTING,
    OMCI_CONN_DISCONNECTED
} OmciConnectType;

typedef struct
{
    union
    {
        UINT32 all;
        struct
        {
            UINT32 omci:1;
            UINT32 model:1;
            UINT32 vlan:1;
            UINT32 mib:1;
            UINT32 flow:1;
            UINT32 rule:1;
            UINT32 mcast:1;
            UINT32 voice:1;
            UINT32 file:1;
            UINT32 handler:1;
            UINT32 pm:1;
            UINT32 pal:1;
            UINT32 ext:1;
            UINT32 util:1;
            UINT32 unused:18;
        } bits;
    } flags;
} omciDebug_t;

/*
 * Ethernet port type definition in a Home Gateway Unit (HGU) profile.
 * Reference: BBF TR-142.
 *
 * ONT:
 *      An ETH port set to ONT type is owned by the OMCI domain, it may be
 *      configured by OMCI as if the port works in the SFU mode. The ETH port
 *      is reported as a PPTP ETH UNI ME instance in the OMCI MIB.
 * RG:
 *      An ETH port set to RG type is owned by the Gateway domain, it may be
 *      managed via CWMP/USP or WEB-UI. The ETH port is not reported in the
 *      OMCI MIB. The OMCI L2-OCM model is based on VEIP.
 * RG_ONT:
 *      An ETH port set to RG_ONT type is visible in both OMCI domain and
 *      Gateway domain. The ETH port is reported in the OMCI MIB, but only
 *      in a read-only sense, e.g., for ETH port alarms and PM collection.
 *      The OMCI L2-OCM model is still based on VEIP, and the software data
 *      path is identical to the RG mode.
 */

#define OMCI_ETH_PORT_TYPE_MASK 0x3

typedef enum
{
    OMCI_ETH_PORT_TYPE_NONE = 0,
    OMCI_ETH_PORT_TYPE_RG_ONT,
    OMCI_ETH_PORT_TYPE_ONT,
    OMCI_ETH_PORT_TYPE_RG
} OmciEthPortType;

typedef struct
{
    union
    {
        UINT32 all;
    } types;
} OmciEthPortType_t;

/*
 * OmciEthPortMgmt_t defines whether an Ethernet port is owned AND has been
 * already controlled/provisioned by OMCI. When the ONU works in the SFU mode,
 * in simple scenarios, OMCI owns all Ethernet LAN ports. However, in some
 * other scenarios, a non-OMCI management protocol may try to configure an 
 * Ethernet LAN port if it is not already taken by OMCI. For those scenarios,
 * when an Ethernet LAN port is already provisioned by OMCI, OMCISDK updates
 * the Ethernet management status ("it is taken") at run-time.
 * Note this is different from the static/pre-configured ownership defined in
 * OmciEthPortType.
 *
 * Note OMCI_ETH_PORT_MGMT_MASK can be defined as 0x1. Make it identical to
 * OMCI_ETH_PORT_TYPE_MASK just for simplicity.
 */
#define OMCI_ETH_PORT_MGMT_MASK 0x3

typedef struct
{
    union
    {
        UINT32 all;
    } omciManaged;
} OmciEthPortMgmt_t;


/* ---- Macro API definitions --------------------------------------------- */

#define omciSetEthPortType(_data, _portIdx, _portType) \
{ \
  if ((_portIdx) < 16) \
  { \
    (_data) &= ~(OMCI_ETH_PORT_TYPE_MASK << (_portIdx * 2)); \
    (_data) |= (((_portType) & OMCI_ETH_PORT_TYPE_MASK) << (_portIdx * 2)); \
  } \
}

#define omciGetEthPortType(_data, _portIdx) \
  (((_portIdx) < 16) ? ((((_data)) >> (_portIdx * 2)) & OMCI_ETH_PORT_TYPE_MASK) : \
  OMCI_ETH_PORT_TYPE_NONE)

#define omciSetEthPortOmciManaged(_data, _portIdx, _isTaken) \
{ \
  if ((_portIdx) < 16) \
  { \
    (_data) &= ~(OMCI_ETH_PORT_MGMT_MASK << (_portIdx * 2)); \
    (_data) |= (((_isTaken) & OMCI_ETH_PORT_MGMT_MASK) << (_portIdx * 2)); \
  } \
}

#define omciIsEthPortOmciManaged(_data, _portIdx) \
  (((_portIdx) < 16) ? ((((_data)) >> (_portIdx * 2)) & OMCI_ETH_PORT_MGMT_MASK) : \
  0)


/* ---- Variable Externs -------------------------------------------------- */


/* ---- Function Prototypes ----------------------------------------------- */


#endif /* _CMSOMCI_H */
