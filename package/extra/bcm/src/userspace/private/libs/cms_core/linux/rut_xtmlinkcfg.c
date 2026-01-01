/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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

#ifdef DMP_ADSLWAN_1


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ptm.h"
#include "rut_atm.h"
#include "rut_wanlayer2.h"
#include "rut_xtmlinkcfg.h"
#include "cms_boardcmds.h"
#include "rut_system.h"
#include "devctl_xtm.h"

#ifdef DMP_X_BROADCOM_COM_ATMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object.
 * This will be removed in the near future.
 */

void rut_getAtmIntfStats(WanAtmStatsObject *stats, UINT32 port, UINT32 reset)
{
    UINT32 ulPortId = PORTID_TO_PORTMASK(port);
    XTM_INTERFACE_STATS IntfStats;

    memset((UINT8 *) &IntfStats, 0x00, sizeof(IntfStats));
    devCtl_xtmGetInterfaceStatistics( ulPortId, &IntfStats, reset );

    stats->inOctets = IntfStats.ulIfInOctets;
    stats->outOctets = IntfStats.ulIfOutOctets;
    stats->inErrors = IntfStats.ulIfInCellErrors;
    stats->inUnknown = 0;
    stats->inHecErrors = 0;
    stats->inInvalidVpiVciErrors = 0;
    stats->inPortNotEnableErrors = 0;
    stats->inPtiError = 0;
    stats->inIdleCells = 0;
    stats->inCircuitTypeErrors = 0;
    stats->inOAMCrcErrors = 0;
    stats->inGfcError = 0;
}

void rut_getAtmAal5IntfStats(_WanAal5StatsObject *stats, UINT32 port, UINT32 reset)
{
    UINT32 ulPortId = PORTID_TO_PORTMASK(port);
    XTM_INTERFACE_STATS IntfStats;

    memset((UINT8 *) &IntfStats, 0x00, sizeof(IntfStats));
    devCtl_xtmGetInterfaceStatistics( ulPortId, &IntfStats, reset );

    stats->inOctets = IntfStats.ulIfInOctets;
    stats->outOctets = IntfStats.ulIfOutOctets;
    stats->inUcastPkts = IntfStats.ulIfInPackets;
    stats->OUtUcastPkts = IntfStats.ulIfOutPackets;
    stats->inErrors = IntfStats.ulIfInPacketErrors;
    stats->outErrors = 0;
    stats->inDiscards = 0;
    stats->outDiscards = 0;
}

void rut_getAtmAal2IntfStats(_WanAal2StatsObject *stats, UINT32 port, UINT32 reset)
{
    stats->inOctets = 0;
    stats->outOctets = 0;
    stats->inUcastPkts = 0;
    stats->OUtUcastPkts = 0;
    stats->inErrors = 0;
    stats->outErrors = 0;
    stats->inDiscards = 0;
    stats->outDiscards = 0;
}

void rut_getAtmVccStats(UINT32 vpi, UINT32 vci, UINT32 port, UINT32 *crcError, UINT32 *oversizedSdu,
                        UINT32 *shortPktErr,UINT32 *lenErr)
{
    *crcError = *oversizedSdu = *shortPktErr = *lenErr = 0;
}

void rut_clearAtmVccStats(UINT32 vpi, UINT32 vci, UINT32 port)
{
}
#endif /* DMP_X_BROADCOM_COM_ATMSTATS_1*/

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object */ 

/**************************************************************************************/
void rut_getXtmIntfStats(XtmInterfaceStatsObject *stats, UINT32 port, UINT32 reset)
{
    UINT32 ulPortId = PORTID_TO_PORTMASK(port);
    XTM_INTERFACE_STATS IntfStats;
    XTM_INTERFACE_CFG Cfg;
    CmsRet ret;
    int error = 0;

    ret = devCtl_xtmGetInterfaceCfg(ulPortId,&Cfg);

    if ( (ret != CMSRET_SUCCESS) ||
         ((ret == CMSRET_SUCCESS) && (Cfg.ulIfOperStatus  == OPRSTS_DOWN)) )
    {
       error = 1;
    }
    else
    {
       memset((UINT8 *) &IntfStats, 0x00, sizeof(IntfStats));
       ret = devCtl_xtmGetInterfaceStatistics(ulPortId, &IntfStats, reset);
       if (reset)
       {
          return;
       }
       
       if (ret != CMSRET_SUCCESS)
       {
          error = 1;
       }
       else
       {
          REPLACE_STRING_IF_NOT_EQUAL_FLAGS(stats->status,MDMVS_ENABLED,mdmLibCtx.allocFlags); 
          stats->port = port + 1;
          stats->inOctets = IntfStats.ulIfInOctets;
          stats->outOctets = IntfStats.ulIfOutOctets;
          stats->inPackets = IntfStats.ulIfInPackets;
          stats->outPackets = IntfStats.ulIfOutPackets;
          stats->inOAMCells = IntfStats.ulIfInOamRmCells;
          stats->outOAMCells = IntfStats.ulIfOutOamRmCells;
          stats->inASMCells = IntfStats.ulIfInAsmCells;
          stats->outASMCells = IntfStats.ulIfOutAsmCells;
          stats->inPacketErrors = IntfStats.ulIfInPacketErrors;
          stats->inCellErrors = IntfStats.ulIfInCellErrors;
       }
    }

    if (error && !reset)
    {
       REPLACE_STRING_IF_NOT_EQUAL_FLAGS(stats->status,MDMVS_DISABLED,mdmLibCtx.allocFlags);
       stats->inOctets = 0;
       stats->outOctets = 0;
       stats->inPackets = 0;
       stats->outPackets = 0;
       stats->inOAMCells = 0;
       stats->outOAMCells = 0;
       stats->inASMCells = 0;
       stats->outASMCells = 0;
       stats->inPacketErrors = 0;
       stats->inCellErrors = 0;
    }
}

#endif /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

UBOOL8 rutXtm_isXDSLLinkUp(const InstanceIdStack *iidStack, MdmObjectId decendentOid)
{
   _WanDslIntfCfgObject *dslIntfCfg = NULL;
   InstanceIdStack dslIntfIidStack = *iidStack;
   UBOOL8 linkUp = FALSE;
   UBOOL8 isAtm __attribute__ ((unused)) = FALSE;
   UBOOL8 isPtm __attribute__ ((unused)) = FALSE;

   if (cmsObj_getAncestor(MDMOID_WAN_DSL_INTF_CFG, 
                         decendentOid,
                         &dslIntfIidStack, 
                         (void **) &dslIntfCfg) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get DslIntfCfg");
      return linkUp;
   }

   if (cmsUtl_strcmp(dslIntfCfg->status, MDMVS_UP) == 0)
   {
      linkUp = TRUE;
   }

   if (!cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
      isAtm = TRUE;
   else 
      isPtm = TRUE;

   cmsLog_debug("xDsl intf at %s has status %s (isAtm=%d, isPtm=%d)",
                cmsMdm_dumpIidStack(&dslIntfIidStack), dslIntfCfg->status,
                isAtm, isPtm);

   cmsObj_free((void **) &dslIntfCfg);

   if (linkUp)
   {
      return linkUp;
   }


#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
   {
      /*
       * On a Bonding board, the primary line at WanDevice.1/WANDevice.2 may be down, but if
       * the secondardy bonding line at WANDevice.12/WanDevice.13 is up, then we consider the
       * DSL line to to be up.
       */
      _WanDslIntfCfgObject *bondingDslIntfObj = NULL;
      CmsRet ret;

      INIT_INSTANCE_ID_STACK(&dslIntfIidStack);
      
      if (isAtm)
         ret = rutWl2_getBondingAtmDslIntfObject(&dslIntfIidStack, &bondingDslIntfObj);
      else
         ret = rutWl2_getBondingPtmDslIntfObject(&dslIntfIidStack, &bondingDslIntfObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get Bonding %s DSL Intf, ret=%d", ((isAtm) ? "ATM" : "PTM"), ret);
         return linkUp;
      }

      if (bondingDslIntfObj->X_BROADCOM_COM_EnableBonding)
      {
         linkUp = !cmsUtl_strcmp(bondingDslIntfObj->status, MDMVS_UP);
      }

      cmsObj_free((void **) &bondingDslIntfObj);
   }

#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */


   return linkUp;
}

#endif /* DMP_ADSLWAN_1 */
