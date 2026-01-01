/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_qos.h"
#include "cms_dal.h"
#include "cms_boardcmds.h"
#include "qdm_intf.h"
#include "rcl.h"
#include "odl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_atm.h"
#include "rut_dsl.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_xtmlinkcfg.h"
#include "devctl_xtm.h"
#include <linux/bcm_skb_defines.h>

#ifdef SUPPORT_QOS

#ifdef SUPPORT_POLICING

#ifdef TC_POLICING
typedef enum
{
   CONFORM        = 1,
   PARTIALCONFORM = 2,
   NONCONFORM     = 3

} PolicerConformType;

/** Local functions **/
static void policerClassCommand(QosCommandType cmdType, UINT32 flowid, const char *action);
static void policerFilterCommand(QosCommandType cmdType, UINT32 prio, UINT32 handle,
                                 char *policeStr, UINT32 flowid);
static CmsRet policerAddQdisc(const char *ifname, UINT32 handle);
static CmsRet policerDelQdisc(const char *ifname, UINT32 handle, UINT32 clsKey);


/***************************************************************************
// Function Name: rutQos_policer
// Description  : execute tc commands to add or remove rate limit for a traffic class.
// Parameters   : cmdType - command type either config or unconfig.
//                queueIfname - egress queue layer 2 interface name.
//                cInfo - QoS class information.
//                pInfo - QoS policer information.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_policer(QosCommandType cmdType,
                      const char *queueIfname,
                      const CmsQosClassInfo *cInfo,
                      const CmsQosPolicerInfo *pInfo)
{
   char cmd[BUFLEN_1024]={0};
   char policeStr[BUFLEN_64]={0};
   UINT32 clsKeyMark = 0;
   UINT32 clsKey = 0;
   UINT32 tcFlowId;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: cmd=%d queueIfname=%s clsKey=%d policer->meterType=%s",
                cmdType, queueIfname, cInfo->key, pInfo->meterType);

   clsKey = cInfo->key;

   if (!pInfo->enable)
   {
      cmsLog_debug("class policer is not enabled. No action is taken.");
      return CMSRET_SUCCESS;
   }

   clsKeyMark = SKBMARK_SET_FLOW_ID(clsKeyMark, clsKey); 

   /* Note that clsKey is always greater than 0. It starts from 1. */
   tcFlowId = (clsKey - 1) * 3;

   rut_doSystemAction("rutQos_policer", "ifconfig ifb0 up 2>/dev/null");

   if (cmdType == QOS_COMMAND_CONFIG)
   {
      snprintf(cmd, sizeof(cmd), "tc qdisc add dev ifb0 root handle 10: dsmark indices %d 2>/dev/null",
              QOS_DSMARK_QDISC_INDICES);
      rut_doSystemAction("rutQos_policer", cmd);

      /* add conforming class */
      policerClassCommand(cmdType, tcFlowId+CONFORM, pInfo->conformingAction);

      if (cmsUtl_strcmp(MDMVS_SIMPLETOKENBUCKET, pInfo->meterType) == 0)
      {
         /* add nonconforming class */
         policerClassCommand(cmdType, tcFlowId+NONCONFORM, pInfo->nonConformingAction);

         if (cmsUtl_strcmp(MDMVS_DROP, pInfo->nonConformingAction) == 0)
         {
            /* add conforming filter and policer to drop if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
         }
         else
         {
            /* add conforming filter and policer to continue if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);

            /* add nonconforming filter */               
            policeStr[0] = '\0';
            policerFilterCommand(cmdType, NONCONFORM, clsKeyMark, policeStr, tcFlowId+NONCONFORM);
         }
      }
      else if (cmsUtl_strcmp(MDMVS_SINGLERATETHREECOLOR, pInfo->meterType) == 0)
      {
         /* add partialconforming class */
         policerClassCommand(cmdType, tcFlowId+PARTIALCONFORM, pInfo->partialConformingAction);

         if (cmsUtl_strcmp(MDMVS_DROP, pInfo->partialConformingAction) == 0)
         {
            /* add conforming filter and policer to drop if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
         }
         else
         {
            /* add conforming filter and policer to continue if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
            
            /* add nonconforming class */
            policerClassCommand(cmdType, tcFlowId+NONCONFORM, pInfo->nonConformingAction);

            if (cmsUtl_strcmp(MDMVS_DROP, pInfo->nonConformingAction) == 0)
            {
               /* add partialconforming filter and policer to drop if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                       pInfo->committedRate/1000, pInfo->excessBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);
            }
            else
            {
               /* add partialconforming filter and policer to continue if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                       pInfo->committedRate/1000, pInfo->excessBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);

               /* add nonconforming filter */               
               policeStr[0] = '\0';
               policerFilterCommand(cmdType, NONCONFORM, clsKeyMark, policeStr, tcFlowId+NONCONFORM);
            }
         }
      }
      else  /* MDMVS_TWORATETHREECOLOR */
      {
         /* add partialconforming class */
         policerClassCommand(cmdType, tcFlowId+PARTIALCONFORM, pInfo->partialConformingAction);

         if (cmsUtl_strcmp(MDMVS_DROP, pInfo->partialConformingAction) == 0)
         {
            /* add conforming filter and policer to drop if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
         }
         else
         {
            /* add conforming filter and policer to continue if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
            
            /* add nonconforming class */
            policerClassCommand(cmdType, tcFlowId+NONCONFORM, pInfo->nonConformingAction);

            if (cmsUtl_strcmp(MDMVS_DROP, pInfo->nonConformingAction) == 0)
            {
               /* add partialconforming filter and policer to drop if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                       (pInfo->peakRate - pInfo->committedRate)/1000, pInfo->peakBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);
            }
            else
            {
               /* add partialconforming filter and policer to continue if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                       (pInfo->peakRate - pInfo->committedRate)/1000, pInfo->peakBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);

               /* add nonconforming filter */               
               policeStr[0] = '\0';
               policerFilterCommand(cmdType, NONCONFORM, clsKeyMark, policeStr, tcFlowId+NONCONFORM);
            }
         }
      }

      /* add qdisc to interface */
      policerAddQdisc(queueIfname, clsKeyMark);
   }
   else
   {
      if (cmsUtl_strcmp(MDMVS_SIMPLETOKENBUCKET, pInfo->meterType) == 0)
      {
         if (cmsUtl_strcmp(MDMVS_DROP, pInfo->nonConformingAction) == 0)
         {
            /* delete conforming filter and policer to drop if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
         }
         else
         {
            /* delete conforming filter and policer to continue if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);

            /* delete nonconforming filter */               
            policeStr[0] = '\0';
            policerFilterCommand(cmdType, NONCONFORM, clsKeyMark, policeStr, tcFlowId+NONCONFORM);
         }
         /* delete nonconforming class */
         policerClassCommand(cmdType, tcFlowId+NONCONFORM, pInfo->nonConformingAction);
      }
      else if (cmsUtl_strcmp(MDMVS_SINGLERATETHREECOLOR, pInfo->meterType) == 0)
      {
         if (cmsUtl_strcmp(MDMVS_DROP, pInfo->partialConformingAction) == 0)
         {
            /* delete conforming filter and policer to drop if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
         }
         else
         {
            /* delete conforming filter and policer to continue if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
            
            if (cmsUtl_strcmp(MDMVS_DROP, pInfo->nonConformingAction) == 0)
            {
               /* delete partialconforming filter and policer to drop if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                       pInfo->committedRate/1000, pInfo->excessBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);
            }
            else
            {
               /* delete partialconforming filter and policer to continue if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                       pInfo->committedRate/1000, pInfo->excessBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);

               /* delete nonconforming filter */               
               policeStr[0] = '\0';
               policerFilterCommand(cmdType, NONCONFORM, clsKeyMark, policeStr, tcFlowId+NONCONFORM);
            }
            /* delete nonconforming class */
            policerClassCommand(cmdType, tcFlowId+NONCONFORM, pInfo->nonConformingAction);
         }
         /* delete partialconforming class */
         policerClassCommand(cmdType, tcFlowId+PARTIALCONFORM, pInfo->partialConformingAction);
      }
      else  /* MDMVS_TWORATETHREECOLOR */
      {
         if (cmsUtl_strcmp(MDMVS_DROP, pInfo->partialConformingAction) == 0)
         {
            /* delete conforming filter and policer to drop if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
         }
         else
         {
            /* delete conforming filter and policer to continue if overlimit */
            sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                    pInfo->committedRate/1000, pInfo->committedBurstSize);
            policerFilterCommand(cmdType, CONFORM, clsKeyMark, policeStr, tcFlowId+CONFORM);
            
            if (cmsUtl_strcmp(MDMVS_DROP, pInfo->nonConformingAction) == 0)
            {
               /* delete partialconforming filter and policer to drop if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 drop",
                       (pInfo->peakRate - pInfo->committedRate)/1000, pInfo->peakBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);
            }
            else
            {
               /* delete partialconforming filter and policer to continue if overlimit */
               sprintf(policeStr, "police rate %lldkbit burst %d mtu 2000 continue",
                       (pInfo->peakRate - pInfo->committedRate)/1000, pInfo->peakBurstSize);
               policerFilterCommand(cmdType, PARTIALCONFORM, clsKeyMark, policeStr, tcFlowId+PARTIALCONFORM);

               /* delete nonconforming filter */               
               policeStr[0] = '\0';
               policerFilterCommand(cmdType, NONCONFORM, clsKeyMark, policeStr, tcFlowId+NONCONFORM);
            }
            /* delete nonconforming class */
            policerClassCommand(cmdType, tcFlowId+NONCONFORM, pInfo->nonConformingAction);
         }
         /* delete partialconforming class */
         policerClassCommand(cmdType, tcFlowId+PARTIALCONFORM, pInfo->partialConformingAction);
      }
      /* delete conforming class */
      policerClassCommand(cmdType, tcFlowId+CONFORM, pInfo->conformingAction);
      
      /* delete qdisc from interface */
      policerDelQdisc(queueIfname, clsKeyMark, clsKey);
   }

   return ret;

}  /* End of rutQos_policer() */

void policerClassCommand(QosCommandType cmdType, UINT32 flowid, const char *action)
{
   char cmd[BUFLEN_1024];
   UINT32 mask, dscp;

   if (cmsUtl_strcmp(MDMVS_DROP, action) == 0)
   {
      /* drop */
      return;
   }
   else if (cmsUtl_strcmp(MDMVS_NULL, action) == 0)
   {
      /* continue without dscp re-mark */
      mask = 0xff;   /* preserve all bits */
      dscp = 0;
   }
   else
   {
      /* continue with dscp re-mark */
      mask = 0x3;    /* preserve ECN bits */
      dscp = atoi(action);
   }

   snprintf(cmd, sizeof(cmd), "tc class %s dev ifb0 classid 10:%d dsmark mask 0x%x value 0x%x 2>/dev/null",
           (cmdType == QOS_COMMAND_CONFIG)? "change" : "del",
           flowid, mask, dscp << 2);
   rut_doSystemAction("policerClassCommand", cmd);

   return;

}  /* End of policerClassCommand() */

void policerFilterCommand(QosCommandType cmdType, UINT32 prio, UINT32 handle,
                          char *policeStr, UINT32 flowid)
{
   char cmd[BUFLEN_1024];

   snprintf(cmd, sizeof(cmd), "tc filter %s dev ifb0 parent 10: protocol 0x%x prio %d handle 0x%x/0x%x fw %s flowid :%d 2>/dev/null",
           (cmdType == QOS_COMMAND_CONFIG)? "add" : "del",
           ETH_P_ALL, prio, handle, SKBMARK_FLOW_ID_M, policeStr, flowid);
   rut_doSystemAction("policerFilterCommand", cmd);

   return;

}  /* End of policerFilterCommand() */

CmsRet policerAddQdisc(const char *ifname, UINT32 handle)
{
   char cmd[BUFLEN_1024];

   cmsLog_debug("Enter: ifname=%s handle=0x%x", ifname, handle);

   /* add the qdisc */
   snprintf(cmd, sizeof(cmd), "tc qdisc add dev %s root handle 1: prio 2>/dev/null", ifname);
   rut_doSystemAction("policerAddQdisc", cmd);

   /* add the filter */
   snprintf(cmd, sizeof(cmd), "tc filter add dev %s parent 1: protocol 0x%x prio 1 handle 0x%x/0x%x fw flowid :1 action mirred egress redirect dev ifb0 2>/dev/null",
           ifname, ETH_P_ALL, handle, SKBMARK_FLOW_ID_M);
   rut_doSystemAction("policerAddQdisc", cmd);
   
   return CMSRET_SUCCESS;

}  /* End of policerAddQdisc() */

CmsRet policerDelQdisc(const char *ifname, UINT32 handle, UINT32 excludeClsKey)
{
   char cmd[BUFLEN_1024];

   cmsLog_debug("Enter: ifname=%s handle=0x%x excludeClsKey=%d",
                ifname, handle, excludeClsKey);

   /* delete the filter */
   snprintf(cmd, sizeof(cmd), "tc filter del dev %s parent 1: protocol 0x%x prio 1 handle 0x%x/0x%x fw flowid :1 action mirred egress redirect dev ifb0 2>/dev/null",
           ifname, ETH_P_ALL, handle, SKBMARK_FLOW_ID_M);
   rut_doSystemAction("policerDelQdisc", cmd);


   /* If this was the last one, delete the whole thing */
   if (rutQos_isAnotherClassPolicerExist(excludeClsKey, ifname) == FALSE)
   {
      snprintf(cmd, sizeof(cmd), "tc qdisc del dev %s root handle 1: prio 2>/dev/null", ifname);
      rut_doSystemAction("policerDelQdisc", cmd);
   }

   return CMSRET_SUCCESS;

}  /* End of policerDelQdisc() */
#endif  /* TC_POLICING */

#ifdef HW_POLICING
#define POLICER_INDEX_MASK 0x0000FFFF
#define GET_POLICER_IDX(INFO) (INFO & POLICER_INDEX_MASK)

static void printPolicer(const CmsQosPolicerInfo *pInfo)
{
    cmsLog_debug("policer->name = %s", pInfo->name);
    cmsLog_debug("policer->enable = %d", pInfo->enable);
    cmsLog_debug("policer->meterType = %s", pInfo->meterType);
    cmsLog_debug("policer->committedRate = %llu", pInfo->committedRate);
    cmsLog_debug("policer->committedBurstSize = %d", pInfo->committedBurstSize);
    cmsLog_debug("policer->excessBurstSize = %d", pInfo->excessBurstSize);
    cmsLog_debug("policer->peakRate = %llu", pInfo->peakRate);
    cmsLog_debug("policer->peakBurstSize = %d", pInfo->peakBurstSize);
    cmsLog_debug("policer->policerInfo = 0x%x", pInfo->policerInfo);
}

/***************************************************************************
// Function Name: rutQos_createPolicer
// Description  : Create a policer
// Parameters   : pInfo - QoS policer configuration.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_createPolicer(CmsQosPolicerInfo *pInfo)
{
    UINT32 policerId = TMCTL_INVALID_KEY;
    tmctl_policer_t policer_cfg = {0};
    tmctl_ret_e rc = TMCTL_SUCCESS;
    CmsRet ret = CMSRET_SUCCESS;

    printPolicer(pInfo);

    if (!pInfo->enable)
    {
        cmsLog_debug("class policer is not enabled. No action is taken.");
        return CMSRET_SUCCESS;
    }

    policer_cfg.type = TMCTL_POLICER_SINGLE_TOKEN_BUCKET;
    if (!cmsUtl_strcmp(MDMVS_SIMPLETOKENBUCKET, pInfo->meterType))
        policer_cfg.type = TMCTL_POLICER_SINGLE_TOKEN_BUCKET;
    else if (!cmsUtl_strcmp(MDMVS_TWORATETHREECOLOR, pInfo->meterType))
        policer_cfg.type = TMCTL_POLICER_TWO_RATE_TCM; 
    else if (!cmsUtl_strcmp(MDMVS_SINGLERATETHREECOLOR, pInfo->meterType))
        policer_cfg.type = TMCTL_POLICER_SINGLE_RATE_TCM; 
    policer_cfg.policerId = TMCTL_INVALID_KEY;
    policer_cfg.cir = pInfo->committedRate;
    policer_cfg.cbs = pInfo->committedBurstSize;
    policer_cfg.pir = pInfo->peakRate;
    policer_cfg.pbs = pInfo->peakBurstSize;
    if (policer_cfg.type == TMCTL_POLICER_SINGLE_RATE_TCM)
        policer_cfg.ebs = pInfo->excessBurstSize;

    rc = rutcwmp_tmctl_createPolicer(&policer_cfg);
    if (TMCTL_SUCCESS == rc)
    {
        policerId = policer_cfg.policerId + 1;
        pInfo->policerInfo = GET_POLICER_IDX(policerId);
    }
    else        
    {
        ret = CMSRET_INTERNAL_ERROR;
        cmsLog_error("rutcwmp_tmctl_createPolicer failed");
    }

    return ret;
}   /* End of rutQos_policer() */

/***************************************************************************
// Function Name: rutQos_updatePolicer
// Description  : Update a policer.
// Parameters   : pInfo - QoS policer information.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_updatePolicer(const CmsQosPolicerInfo *pInfo)
{
    UINT32 policerInfo = 0;
    UINT32 policerId = TMCTL_INVALID_KEY;
    tmctl_policer_t policer_cfg = {0};
    tmctl_ret_e rc = TMCTL_SUCCESS;
    CmsRet ret = CMSRET_SUCCESS;

    printPolicer(pInfo);

    if (!pInfo->enable)
    {
        cmsLog_debug("class policer is not enabled. No action is taken.");
        return CMSRET_SUCCESS;
    }

    policerInfo = pInfo->policerInfo;
    policerId = GET_POLICER_IDX(policerInfo);

    policer_cfg.type = TMCTL_POLICER_SINGLE_TOKEN_BUCKET;
    if (!cmsUtl_strcmp(MDMVS_SIMPLETOKENBUCKET, pInfo->meterType))
        policer_cfg.type = TMCTL_POLICER_SINGLE_TOKEN_BUCKET;
    else if (!cmsUtl_strcmp(MDMVS_TWORATETHREECOLOR, pInfo->meterType))
        policer_cfg.type = TMCTL_POLICER_TWO_RATE_TCM; 
    else if (!cmsUtl_strcmp(MDMVS_SINGLERATETHREECOLOR, pInfo->meterType))
        policer_cfg.type = TMCTL_POLICER_SINGLE_RATE_TCM; 
    policer_cfg.policerId = policerId - 1;
    policer_cfg.cir = pInfo->committedRate;
    policer_cfg.cbs = pInfo->committedBurstSize;
    policer_cfg.pir = pInfo->peakRate;
    policer_cfg.pbs = pInfo->peakBurstSize;
    if (policer_cfg.type == TMCTL_POLICER_SINGLE_RATE_TCM)
        policer_cfg.ebs = pInfo->excessBurstSize;
            
    rc = rutcwmp_tmctl_modifyPolicer(&policer_cfg);
    if (TMCTL_SUCCESS == rc)
    {
        ret = CMSRET_SUCCESS;
    }
    else        
    {
        ret = CMSRET_INTERNAL_ERROR;
        cmsLog_error("rutcwmp_tmctl_modifyPolicer failed");
    }

    return ret;
}   /* End of rutQos_updatePolicer() */

/***************************************************************************
// Function Name: rutQos_deletePolicer
// Description  : delete the policer.
// Parameters   : policerInfo - QoS policer id information.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_deletePolicer(UINT32 policerInfo)
{
    UINT32 policerId = TMCTL_INVALID_KEY;
    tmctl_ret_e rc = TMCTL_SUCCESS;
    CmsRet ret = CMSRET_SUCCESS;

    policerId = GET_POLICER_IDX(policerInfo);
    cmsLog_debug("policerId %d", policerId);

    rc= rutcwmp_tmctl_deletePolicer(policerId - 1);
    ret = (rc == TMCTL_SUCCESS)?CMSRET_SUCCESS:CMSRET_INTERNAL_ERROR;

    return ret;
}   /* End of rutQos_deletePolicer() */

/***************************************************************************
// Function Name: rutQos_policer
// Description  : Config the reference from the classification to the policer.
// Parameters   : cmdType - command type either config or unconfig.
//                queueIfname - egress queue layer 2 interface name.
//                cInfo - QoS class information.
//                pInfo - QoS policer information.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_policer(QosCommandType cmdType,
                      const char *queueIfname,
                      const CmsQosClassInfo *cInfo,
                      const CmsQosPolicerInfo *pInfo)
{
    char cmd[BUFLEN_1024]={0};
    UINT32 tc = 0;
    UINT32 policerInfo = 0;
    UINT32 policerId = TMCTL_INVALID_KEY;
    char tblStr[BUFLEN_16] = {0};
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("%s classification %d queue %s:", 
        (cmdType == QOS_COMMAND_CONFIG)? "Attach to" : "Dettach from", cInfo->key, queueIfname);
    printPolicer(pInfo);

    if (!pInfo->enable)
    {
        cmsLog_debug("class policer is not enabled. No action is taken.");
        return CMSRET_SUCCESS;
    }

    policerInfo = pInfo->policerInfo;
    policerId = GET_POLICER_IDX(policerInfo);

    //The chain rule%d(cInfo->key) should be created by the caller of rutQos_policer
    if (cmdType == QOS_COMMAND_CONFIG)
    {
        tc = SKBMARK_SET_TC_ID(tc, policerId); 
        snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -A rule%d -j mark --mark-or 0x%x --mark-target CONTINUE",
            cInfo->key, tc);
#ifdef SUPPORT_NF_NAT
        rut_doSystemAction("rutQos_policer", cmd);
#endif // SUPPORT_NF_NAT
        //The following rule is needed if ingressIntf is local or lan
        //The chain rule%d(cInfo->key) in table mangle is created in this case 
        if ((cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_LOCAL) == 0) ||
            (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_LAN) == 0) ||
            (cInfo->ingressIsSpecificLan))
        {
            if (cInfo->etherType == ETH_P_IPV6)
            {
                strcpy(tblStr, "ip6tables");
            }
            else
            {
                strcpy(tblStr, "iptables");
            }
            snprintf(cmd, sizeof(cmd), "%s -t mangle -I rule%d -j MARK --or-mark 0x%x", tblStr, cInfo->key, tc);
#ifdef SUPPORT_NF_MANGLE
            rut_doSystemAction("rutQos_policer", cmd);
#endif // SUPPORT_NF_MANGLE
        }
    }
    else
    { 
        // The rule%d(cInfo->key) created when QOS_COMMAND_CONFIG
        // should have been removed by classification.
    }

    return ret;
}   /* End of rutQos_policer() */
#endif  /* HW_POLICING */
#endif  /* SUPPORT_POLICING */

#endif  /* SUPPORT_QOS */


