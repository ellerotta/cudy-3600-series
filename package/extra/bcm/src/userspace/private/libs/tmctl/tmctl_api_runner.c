/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "os_defs.h"
#include "board.h"
#include "tmctl_api.h"
#include "tmctl_plat.h"
#include "tmctl_api_trace.h"
#include "tmctl_ethsw.h"
#if defined(BUILD_DSL)
#include "tmctl_xtm.h"
#endif

#include <bp3_license.h>
#include "user_api.h"
#include "tmctl_bdmf_rdpa.h"
#include "tmctl_api_runner.h"
#include <net/if.h>
#include <linux/ethtool.h>


/* Do not store return value, used for prints only*/
char *ifp_to_name(tmctl_devType_e devType, tmctl_if_t *if_p)
{
    static char name[IFNAMSIZ+1] = {0};
    switch (devType)
    {
    case TMCTL_DEV_ETH:
        strncpy(name, if_p->ethIf.ifname, IFNAMSIZ);
        break;
    case TMCTL_DEV_GPON:
        snprintf(name, IFNAMSIZ,  "tcont.%d", if_p->gponIf.tcontid);
        break;
    case TMCTL_DEV_EPON:
        snprintf(name, IFNAMSIZ,  "llid.%d", if_p->eponIf.llid);
        break;
    case TMCTL_DEV_XTM:
        strncpy(name, if_p->ethIf.ifname, IFNAMSIZ);
        break;
    case TMCTL_DEV_SVCQ:
        strncpy(name, "srvcq", IFNAMSIZ);
        break;
    default:
        snprintf(name, IFNAMSIZ,  "unknown.%d", devType);
    }
    return name;
}

static int is_valid_wlan_prefix(char *ifname)
{
    if (strncmp(ifname, "wlan", 4)) {
        tmctl_error("invalid wlan interface name[%s], use wlan* where * is radio index!!\n", ifname);
        return 0;
    }
    return 1;
}

static int is_switch_embedded(void)
{
#if defined(BCM_DSL_RDP) || defined(CONFIG_BCM963158)
    return 1;
#else
    return 0;
#endif
}

int is_switch_intf(tmctl_devType_e devType, tmctl_if_t *if_p)
{
    bdmf_object_handle port = BDMF_NULL;
    rdpa_port_type type;
    int rc;

    if ((!is_switch_embedded()) || (devType != TMCTL_DEV_ETH))
        return 0;

    rc = get_tm_owner(devType, if_p, &port);
    if (rc)
        return 0;

    rc = rdpa_port_type_get(port, &type);
    if (rc)
        return 0;

    return (type == rdpa_port_sf2_emac);
}


static int is_supported_rdpa_api(tmctl_devType_e devType, tmctl_if_t *if_p)
{
   int is_supported = 0;

   switch (devType)
   {
      case TMCTL_DEV_ETH:
      {
         if (is_lan(devType, if_p))
         {
            is_supported = 1;
            goto exit;
         }

         is_supported = is_supported_wan_mode();
         break;
      }
      case TMCTL_DEV_EPON:
      {
         is_supported = is_supported_wan_mode();
         break;
      }
      case TMCTL_DEV_XTM:
        goto exit;

      case TMCTL_DEV_SVCQ:
      default:
        is_supported = 1;
   }

exit:
    return is_supported;
}

/* ----------------------------------------------------------------------------
 * This function initializes the basic TM settings for a port/tcont/llid based
 * on TM capabilities.
 *
 * Note that if the port had already been initialized, all its existing
 * configuration will be deleted before re-initialization.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port/TCONT/LLID identifier.
 *    cfgFlags (IN) Port TM initialization config flags.
 *                  See bit definitions in tmctl_api.h
 *    numQueues   (IN) Number of queues to be set for TM.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_portTmInit_plat(tmctl_devType_e devType,
                                  tmctl_if_t*     if_p,
                                  uint32_t        cfgFlags,
                                  int             numQueues)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s cfgFlags=0x%04x numQueues=%d", ifp_to_name(devType, if_p),
           cfgFlags, numQueues);

    /* No init required for xtm */
    if (devType == TMCTL_DEV_XTM)
        return 0;

   if (is_supported_rdpa_api(devType, if_p))
       return tmctl_RdpaTmInit(devType, if_p, cfgFlags, numQueues);

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
           ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_portTmInit() */


/* ----------------------------------------------------------------------------
 * This function un-initializes all TM configurations of a port. This
 * function may be called when the port is down.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_portTmUninit_plat(tmctl_devType_e devType,
                                    tmctl_if_t*     if_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s", ifp_to_name(devType, if_p));

    /* No uninit required for xtm */
    if (devType == TMCTL_DEV_XTM)
        return 0;

    if (is_supported_rdpa_api(devType, if_p))
        return tmctl_RdpaTmUninit(devType, if_p);

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
           ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_portTmUninit() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of a software queue. If the
 * configuration is not found, qid in the config structure will be
 * returned as -1.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID must be in the range of [0..maxQueues-1].
 *    qcfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueCfg_plat(tmctl_devType_e   devType,
                                   tmctl_if_t*       if_p,
                                   int               queueId,
                                   tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s qid=%d", ifp_to_name(devType, if_p), queueId);


#if defined(BUILD_DSL)
   if(devType == TMCTL_DEV_XTM)
       return tmctl_xtm_getQueueCfg(if_p->xtmIf.ifname, queueId, qcfg_p);
#endif

   if (is_supported_rdpa_api(devType, if_p))
   {
       ret = tmctl_RdpaQueueCfgGet(devType, if_p, queueId, qcfg_p);

       if (is_switch_intf(devType, if_p))
       {
           /* Retrieve Queue Shaper info from Ethernet Switch
            */
           ret = tmctlEthSw_getQueueShaper(if_p->ethIf.ifname,
                   qcfg_p->qid,
                   &(qcfg_p->shaper));
       }
       return ret;
   }

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
           ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_getQueueCfg() */

/* ----------------------------------------------------------------------------
 * This function updates the switch port scheduer based on the current queue
 * configuration. Port scheduler is updated only when all the queues are
 * configured in runner.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e ethSw_update_port_sched(tmctl_devType_e         devType,
                                    tmctl_if_t              *if_p)
{
#if defined(BCM_DSL_XRDP) || defined(BCM_DSL_RDP)
   /* Must only be called for Ethernet Switch LAN Port*/
   tmctl_portQcfg_t portQcfg;
   tmctl_portTmParms_t tmParms;
   tmctl_queueCfg_t* qcfg_p;
   int qidx;
   tmctl_ret_e ret = TMCTL_ERROR;

   if (!is_switch_intf(devType, if_p))
   {
       tmctl_error("not a switch lan port");
       return ret;
   }
   /* get the port tm parameters */
   ret = tmctl_getPortTmParms(devType, if_p, &tmParms);
   if (ret == TMCTL_ERROR)
   {
      tmctl_error("tmctl_getPortTmParms returns error");
      return ret;
   }

   portQcfg.numQueues = 0;

   qcfg_p = &(portQcfg.qcfg[0]);
   for (qidx = 0; qidx < tmParms.maxQueues; qidx++, qcfg_p++)
   {
      ret = tmctl_RdpaQueueCfgGet(devType, if_p, qidx, qcfg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_RdpaQueueCfgGet ERROR for qidx = %d!", qidx);
         return ret;
      }
      else if (ret == TMCTL_SUCCESS)
      {
         portQcfg.numQueues++;
      }
   }

   if (portQcfg.numQueues == tmParms.maxQueues)
   {
      ret = tmctlEthSw_setPortSched(if_p->ethIf.ifname, &portQcfg);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlEthSw_setPortSched ERROR!");
         return ret;
      }
   }

   /* Force return success since the "ret" variable may not contain success.
      It can be TMCTL_NOT_FOUND when calling tmctl_RdpaQueueCfgGet earlier
      We bail out for error cases before reaching this point anyway */
   return TMCTL_SUCCESS;
#else
   return TMCTL_ERROR;
#endif
}


int set_min_buf_size(tmctl_queueCfg_t* qcfg_p, tmctl_devType_e  dev_type, const tmctl_if_t *if_p)
{
#if (defined(BCM_PON_XRDP))
    uint32_t calculated_min_buff = 1;

    if (SPEED_10000 == tmctl_getIfaceLinkSpeed(dev_type, if_p))
    {
#if !defined(CHIP_6858) 
        calculated_min_buff = 64;
#else
        calculated_min_buff = 16; /* equal to 16 * 512 bytes*/
#endif
    }
if (qcfg_p->minBufs < calculated_min_buff)
    qcfg_p->minBufs = calculated_min_buff;
#endif
   return 0;
}

static tmctl_ret_e validate_sched_mode(tmctl_devType_e   devType,
                                   tmctl_if_t*       if_p,
                                   tmctl_queueCfg_t* qcfg_p)
{
   tmctl_portTmParms_t tmParms;
   tmctl_ret_e ret = TMCTL_SUCCESS;

  /* get the port tm parameters */
   ret = tmctl_getPortTmParms(devType, if_p, &tmParms);
   if (ret)
   {
       tmctl_error("tmctl_getPortTmParms returns error");
       return ret;
   }

   if (((qcfg_p->schedMode == TMCTL_SCHED_SP) &&
           !((tmParms.schedCaps & (TMCTL_SP_CAPABLE | TMCTL_SP_WRR_CAPABLE)))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WRR) &&
           !((tmParms.schedCaps & (TMCTL_WRR_CAPABLE | TMCTL_SP_WRR_CAPABLE)))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WDRR) &&
           !((tmParms.schedCaps & (TMCTL_WDRR_CAPABLE | TMCTL_SP_WDRR_CAPABLE)))) ||
          ((qcfg_p->schedMode == TMCTL_SCHED_WFQ) &&
           !(tmParms.schedCaps & (TMCTL_WFQ_CAPABLE))))
   {

       tmctl_error("Queue sched mode: %d is not supported, mode configured on port: 0x%x",
                       qcfg_p->schedMode, tmParms.schedCaps);
       ret =  TMCTL_ERROR;

   }
   return ret;
}

/* ----------------------------------------------------------------------------
 * This function configures a software queue for a port. The qeueu ID shall
 * be specified in the configuration parameter structure. If the queue
 * already exists, its configuration will be modified. Otherwise, the queue
 * will be added.
 *
 * Note that for Ethernet port with an external Switch, the new queue
 * configuration may not be applied immediately to the Switch. For instance,
 * SF2 only supports one of the following priority queuing options:
 *
 *    Q0  Q1  Q2  Q3  Q4  Q5  Q6  Q7
 * 1) SP  SP  SP  SP  SP  SP  SP  SP
 * 2) WRR WRR WRR WRR WRR WRR WRR SP
 * 3) WRR WRR WRR WRR WRR WRR SP  SP
 * 4) WRR WRR WRR WRR WRR SP  SP  SP
 * 5) WRR WRR WRR WRR SP  SP  SP  SP
 * 6) WRR WRR WRR WRR WRR WRR WRR WRR
 *
 * This function will commit the new queue configuration to SF2 only when
 * all the queue configurations of the port match one of the priority
 * queuing options supported by the Switch.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    qcfg_p (IN) Queue config parameters.
 *                Notes:
 *                - qid must be in the range of [0..maxQueues-1].
 *                - For 63138, 63148 or 63158 TMCTL_DEV_ETH device type,
 *                  -- the priority of SP queue must be set to qid.
 *                  -- the priority of WRR/WDRR/WFQ queue must be set to 0.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueCfg_plat(tmctl_devType_e   devType,
                                   tmctl_if_t*       if_p,
                                   tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s qid=%d priority=%d qsize=%d schedMode=%d wt=%d minBufs=%d minRate=%d kbps=%d mbs=%d",
               ifp_to_name(devType,if_p), qcfg_p->qid, qcfg_p->priority, qcfg_p->qsize, qcfg_p->schedMode,
               qcfg_p->weight, qcfg_p->minBufs, qcfg_p->shaper.minRate,
               qcfg_p->shaper.shapingRate, qcfg_p->shaper.shapingBurstSize);

   ret = validate_sched_mode(devType, if_p, qcfg_p);
   if (ret) return ret;

#if defined(BUILD_DSL)
   if(devType == TMCTL_DEV_XTM)
   {
       tmctl_portTmParms_t tmParms;
       /* To let XTM queue shaper config work, remove the XTM queue first before adding it. */
       ret = tmctl_xtm_remQueueCfg(if_p->xtmIf.ifname, qcfg_p->qid);
       if (ret == TMCTL_ERROR)
       {
           tmctl_error("tmctl_xtm_remQueueCfg ERROR! ret=%d", ret);
           return ret;
       }
       ret = tmctl_xtm_setQueueCfg(if_p->xtmIf.ifname, &tmParms, qcfg_p);
       if (ret == TMCTL_ERROR)
           tmctl_error("tmctl_xtm_setQueueCfg ERROR! ret=%d", ret);
       return ret;
   }
#endif

   ret = set_min_buf_size(qcfg_p, devType, if_p);
   if (ret != TMCTL_SUCCESS)
       return ret;

   if (is_supported_rdpa_api(devType, if_p))
   {
       if (is_switch_intf(devType, if_p))
       {
           /* Set Queue Shaper info in Ethernet Switch */
           ret = tmctlEthSw_setQueueShaper(if_p->ethIf.ifname,
                   qcfg_p->qid,
                   &(qcfg_p->shaper));
           /* Runner does not perform shaping towards switch */
           memset(&(qcfg_p->shaper), 0, sizeof(qcfg_p->shaper));
       }
       /* Intentionally RDPA is called after switch, to wipe-out shaper config */
       ret = tmctl_RdpaTmQueueSet(devType, if_p, qcfg_p);
       /* Update port scheduler if needed */
       if (is_switch_intf(devType, if_p))
           ret = ethSw_update_port_sched(devType, if_p);
       /* always set port queue states so enet driver know which ones are enabled */
       if (devType == TMCTL_DEV_ETH)
               tmctlEthSw_setQueueState(if_p->ethIf.ifname, qcfg_p->qid, 1 /*enable*/);
       tmctl_debug("Exit: %d", ret);
       return ret;
   }

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
           ifp_to_name(devType, if_p), ret);
   return TMCTL_ERROR;
}  /* End of tmctl_setQueueCfg() */


/* ----------------------------------------------------------------------------
 * This function deletes a software queue from a port.
 *
 * Note that for Ethernet port with an external Switch, the corresponding
 * Switch queue will not be deleted.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) The queue ID.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_delQueueCfg_plat(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   int             queueId)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s qid=%d", ifp_to_name(devType, if_p),  queueId);

#if defined(BUILD_DSL)
   if (devType == TMCTL_DEV_XTM)
       return tmctl_xtm_remQueueCfg(if_p->xtmIf.ifname, queueId);
#endif

   if (is_supported_rdpa_api(devType, if_p))
   {
       ret = tmctl_RdpaTmQueueDel(devType, if_p, queueId);
       if (is_switch_intf(devType, if_p))
       {
           tmctl_shaper_t shaper;
           /* Disable Eth Switch queue shaper */
           memset(&shaper, 0, sizeof(tmctl_shaper_t));

           ret = tmctlEthSw_setQueueShaper(if_p->ethIf.ifname, queueId, &shaper);
       }

       if (devType == TMCTL_DEV_ETH)
           tmctlEthSw_setQueueState(if_p->ethIf.ifname, queueId, 0 /*enable*/);
       return ret;
   }

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
           ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_delQueueCfg() */


/* ----------------------------------------------------------------------------
 * This function gets the port shaper configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPortShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s", ifp_to_name(devType, if_p));

#if defined(BUILD_DSL)
   if(devType == TMCTL_DEV_XTM)
   {
      return tmctl_xtm_getPortShaper(if_p->xtmIf.ifname, shaper_p);
   }
#endif

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (is_switch_intf(devType, if_p))
            return tmctlEthSw_getPortShaper(if_p->ethIf.ifname, shaper_p);
        return tmctl_RdpaGetPortShaper(devType, if_p, shaper_p);
    }
    tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_getPortShaper() */


/* ----------------------------------------------------------------------------
 * This function configures the port shaper for shaping rate, shaping burst
 * size and minimum rate. If port shaping is to be done by the external
 * Switch, the corresponding Switch port shaper will be configured.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPortShaper_plat(tmctl_devType_e devType,
        tmctl_if_t*     if_p,
        tmctl_shaper_t* shaper_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dev=%s portKbps=%d portMbs=%d", ifp_to_name(devType, if_p),
            shaper_p->shapingRate, shaper_p->shapingBurstSize);

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
        return tmctl_xtm_setPortShaper(if_p->xtmIf.ifname, shaper_p);
#endif

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (is_switch_intf(devType, if_p))
            return tmctlEthSw_setPortShaper(if_p->ethIf.ifname, shaper_p);
        return tmctl_RdpaSetPortShaper(devType, if_p, shaper_p);
    }
    tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_setPortShaper() */


/* ----------------------------------------------------------------------------
 * This function gets the port rx rate configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPortRxRate_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s", ifp_to_name(devType, if_p));

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (is_switch_intf(devType, if_p))
            return tmctlEthSw_getPortRxRate(if_p->ethIf.ifname, shaper_p);
        return tmctl_RdpaGetPortRxRate(devType, if_p, shaper_p);
    }
    tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_getPortRxRate() */


/* ----------------------------------------------------------------------------
 * This function configures the port rx rate, shaping burst
 * size and minimum rate. If port shaping is to be done by the external
 * Switch, the corresponding Switch port shaper will be configured.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPortRxRate_plat(tmctl_devType_e devType,
        tmctl_if_t*     if_p,
        tmctl_shaper_t* shaper_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dev=%s portKbps=%d portMbs=%d", ifp_to_name(devType, if_p),
            shaper_p->shapingRate, shaper_p->shapingBurstSize);

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (is_switch_intf(devType, if_p))
            return tmctlEthSw_setPortRxRate(if_p->ethIf.ifname, shaper_p);
        return tmctl_RdpaSetPortRxRate(devType, if_p, shaper_p);
    }
    tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_setPortRxRate() */


/* ----------------------------------------------------------------------------
 * This function gets the overall shaper configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p     (OUT) The portMap which linked to the overall shaper
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p,
                                     tmctl_shaper_t* shaper_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;
    tmctl_debug(" Enter: dev=%s \n", ifp_to_name(devType, if_p));
#if defined(BCM_PON)
    if (devType == TMCTL_DEV_GPON)
    {
        ret = tmctl_RdpaGetOverAllShaper(shaper_p, &(if_p->portId.portMap));
        if (ret == TMCTL_ERROR)
        {
            tmctl_error("tmctl_RdpaGetOverAllShaper ERROR! ret=%d", ret);
            return ret;
        }
    }
#endif
    return ret;
}  /* End of tmctl_getOverAllShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function configures a overall shaper for shaping rate, shaping burst
 * size.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_shaper_t* shaper_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: devType=%d Rate=%d BurstSize=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);
#if defined(BCM_PON)
    if (devType == TMCTL_DEV_GPON)
    {
        ret = tmctl_RdpaSetOverAllShaper(shaper_p);
        if (ret)
        {
            tmctl_error("tmctl_RdpaConfigOverAllShaper ERROR! ret=%d", ret);
            return ret;
        }
    }
#endif
    return ret;
}  /* End of tmctl_setOverAllShaper_plat() */


/* ----------------------------------------------------------------------------
 * This function link port to the overall shaper
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_linkOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: dev=%s \n", ifp_to_name(devType, if_p));
#if defined(BCM_PON)
    if (devType == TMCTL_DEV_GPON)
    {
        ret = tmctl_RdpaLinkOverAllShaper(if_p->gponIf.tcontid, 1);
        if (ret == TMCTL_ERROR)
        {
            tmctl_error("tmctl_RdpaLinkOverAllShaper ERROR! ret=%d", ret);
            return ret;
        }
    }
#endif
    return ret;
}  /* End of tmctl_linkOverAllShaper_plat() */

/* ----------------------------------------------------------------------------
 * This function unlink port from the overall shaper
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_unlinkOverAllShaper_plat(tmctl_devType_e devType,
                                     tmctl_if_t*     if_p)
{
    tmctl_ret_e ret  = TMCTL_UNSUPPORTED;

    tmctl_debug(" Enter: dev=%s", ifp_to_name(devType, if_p));
#if defined(BCM_PON)
    if (devType == TMCTL_DEV_GPON)
    {
        ret = tmctl_RdpaLinkOverAllShaper(if_p->gponIf.tcontid, 0);
        if (ret == TMCTL_ERROR)
        {
            tmctl_error("tmctl_RdpaLinkOverAllShaper ERROR! ret=%d", ret);
            return ret;
        }
    }
#endif
    return ret;
}  /* End of tmctl_unLinkOverAllShaper_plat() */

/* ----------------------------------------------------------------------------
 * This function gets the drop algorithm of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    dropAlg_p (OUT) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueDropAlg_plat(tmctl_devType_e       devType,
                                       tmctl_if_t*           if_p,
                                       int                   queueId,
                                       tmctl_queueDropAlg_t* dropAlg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s qid=%d", ifp_to_name(devType, if_p), queueId);

#if defined(BUILD_DSL)
   if(devType == TMCTL_DEV_XTM)
   {
      int channelId = 0;
      ret = tmctl_xtm_getTxChannelByQid(ifp_to_name(devType, if_p), queueId, &channelId);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_xtm_getTxChannelByQid returns error");
         return ret;
      }
      return tmctl_RdpaGetQueueDropAlg(devType, if_p, channelId, dropAlg_p);
   }
#endif

   if (is_supported_rdpa_api(devType, if_p))
          return tmctl_RdpaGetQueueDropAlg(devType, if_p, queueId, dropAlg_p);

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
                      ifp_to_name(devType, if_p), ret);
   return ret;
}  /* End of tmctl_getQueueDropAlg() */


/* ----------------------------------------------------------------------------
 * This function sets the drop algorithm of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    dropAlg_p (IN) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueDropAlg_plat(tmctl_devType_e          devType,
                                       tmctl_if_t*              if_p,
                                       int                      queueId,
                                       tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

#if defined(BUILD_DSL)
   tmctl_debug("Enter: dev=%s qid=%d dropAlgorithm=%d "
               "priorityMask0=0x%x priorityMask1=0x%x",
               ifp_to_name(devType, if_p), queueId, dropAlg_p->dropAlgorithm,
               dropAlg_p->priorityMask0, dropAlg_p->priorityMask1);

   if(devType == TMCTL_DEV_XTM)
   {
      int channelId = 0;
      ret = tmctl_xtm_getTxChannelByQid(ifp_to_name(devType, if_p), queueId, &channelId);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctl_xtm_getTxChannelByQid returns error");
         return ret;
      }
      
      return tmctl_RdpaSetQueueDropAlg(devType, if_p, channelId, dropAlg_p);
   }
#endif

   if(((dropAlg_p->dropAlgorithm == TMCTL_DROP_CODEL) || (dropAlg_p->dropAlgorithm == TMCTL_DROP_PI2)) &&  !bcm_license_check(BP3_FEATURE_AQM))
   {
      tmctl_error("No license for AQM feature");
      return TMCTL_ERROR;
   }

   if (is_supported_rdpa_api(devType, if_p))
       return tmctl_RdpaSetQueueDropAlg(devType, if_p, queueId, dropAlg_p);

   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
                  ifp_to_name(devType, if_p), ret);

   return ret;

}  /* End of tmctl_setQueueDropAlg() */

/* ----------------------------------------------------------------------------
 * This function gets the drop algorithm of a XTM channel.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    channelId (IN) Channel ID.
 *    dropAlg_p (OUT) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getXtmChannelDropAlg_plat(tmctl_devType_e       devType,
                                            int                   channelId,
                                            tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;
#if defined(FIXME)
#if defined(BCM_DSL_RDP) || defined(BCM_DSL_XRDP) /* What to do for XTM */
   tmctl_debug("Enter: dev=%s channelId=%d", ifp_to_name(devType, if_p), channelId);

   if (devType == TMCTL_DEV_XTM)
   {
      ret = tmctlRdpa_getQueueDropAlg(devType, if_p, channelId, dropAlg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlRdpa_getQueueDropAlg ERROR! ret=%d", ret);
         return ret;
      }
   }
#endif
#endif
   return ret;

}  /* End of tmctl_getXtmChannelDropAlg() */

/* ----------------------------------------------------------------------------
 * This function sets the drop algorithm of a XTM channel.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    channelId (IN) Channel ID.
 *    dropAlg_p (IN) The drop algorithm configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setXtmChannelDropAlg_plat(tmctl_devType_e       devType,
                                            int                   channelId,
                                            tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;
#if defined(FIXME)
#if defined(BCM_DSL_RDP) || defined(BCM_DSL_XRDP) /* XTM ?? */
   tmctl_debug("Enter: dev=%s channelId=%d dropAlgorithm=%d "
               "priorityMask0=0x%x priorityMask1=0x%x",
               ifp_to_name(devType, if_p), channelId, dropAlg_p->dropAlgorithm,
               dropAlg_p->priorityMask0, dropAlg_p->priorityMask1);

   if (devType == TMCTL_DEV_XTM)
   {
      ret = tmctlRdpa_setQueueDropAlg(RDPA_IOCTL_DEV_XTM, rdpa_wan_type_to_if(rdpa_wan_dsl), channelId, dropAlg_p);
      if (ret == TMCTL_ERROR)
      {
         tmctl_error("tmctlRdpa_setQueueDropAlg ERROR! ret=%d", ret);
         return ret;
      }
   }
#endif
#endif
   return ret;

}  /* End of tmctl_setXtmChannelDropAlg() */

/* ----------------------------------------------------------------------------
 * This function configures the flow control of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    flowCtrl_p (IN) The flow control configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueFlowCtrl_plat(tmctl_devType_e        devType,
                                        tmctl_if_t*            if_p,
                                        int                    queueId,
                                        tmctl_queueFlowCtrl_t* flowCtrl_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;
   tmctl_queueDropAlg_t dropAlg;

   memset(&dropAlg, 0, sizeof(tmctl_queueDropAlg_t));
   tmctl_debug("Enter: devType=%d ifname=%s qid=%d",
               devType, if_p->ethIf.ifname, queueId);

   if (is_switch_intf(devType, if_p))
   {
      /* TODO! see if we are going to support this from tmctl */
      return ret;
   }

   if (!is_supported_rdpa_api(devType, if_p))
      return ret;

   ret = tmctl_RdpaGetQueueDropAlg(devType, if_p, queueId, &dropAlg);
   if (ret)
      return ret;

   /* FlowCtrl does not coexist with other AQM drop algorithm.
    * Return error if user is configuring FlowCtrl on top of queue with
    * AQM algorithm already. */
   if ((dropAlg.dropAlgorithm != TMCTL_DROP_DT) &&
       (dropAlg.dropAlgorithm != TMCTL_DROP_FLOW_CTRL))
   {
      tmctl_error("%s:can only configure flow control on "
                  "drop-tail/flow-control-enabled queue\n");
      return TMCTL_ERROR;
   }

   if (flowCtrl_p->enable == 0)
   {
      dropAlg.dropAlgorithm = TMCTL_DROP_DT;
   }
   else
   {
      dropAlg.dropAlgorithm = TMCTL_DROP_FLOW_CTRL;
      /* dropAlgHi.redMinThreshold is used for Pause Threshold. */
      /* dropAlgLo.redMinThreshold is used for Hysteresis Threshold */
      dropAlg.dropAlgHi.redMinThreshold = flowCtrl_p->pauseThr;
      dropAlg.dropAlgLo.redMinThreshold = flowCtrl_p->hystThr;
   }

   return tmctl_RdpaSetQueueDropAlg(devType, if_p, queueId, &dropAlg);
} /* End of tmctl_setQueueFlowCtrl_plat */

/* ----------------------------------------------------------------------------
 * This function gets the flow control configuration of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    flowCtrl_p (OUT) The flow control configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueFlowCtrl_plat(tmctl_devType_e        devType,
                                        tmctl_if_t*            if_p,
                                        int                    queueId,
                                        tmctl_queueFlowCtrl_t* flowCtrl_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;
   tmctl_queueDropAlg_t dropAlg;

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d",
               devType, if_p->ethIf.ifname, queueId);

   if (is_switch_intf(devType, if_p))
   {
      /* TODO! see if we are going to support this from tmctl */
   }
   else if (is_supported_rdpa_api(devType, if_p))
      ret = tmctl_RdpaGetQueueDropAlg(devType, if_p, queueId, &dropAlg);

   if (ret)
      return ret;

   if (dropAlg.dropAlgorithm == TMCTL_DROP_FLOW_CTRL)
   {
      flowCtrl_p->enable = 1;
      /* dropAlgHi.redMinThreshold is used for Pause Threshold. */
      /* dropAlgLo.redMinThreshold is used for Hysteresis Threshold */
      flowCtrl_p->pauseThr = dropAlg.dropAlgHi.redMinThreshold;
      flowCtrl_p->hystThr = dropAlg.dropAlgLo.redMinThreshold;
   }
   else
   {
      flowCtrl_p->enable = 0;
      flowCtrl_p->pauseThr = 0;
      flowCtrl_p->hystThr = 0;
   }

   return ret;
} /* End of tmctl_getQueueFlowCtrl_plat */

/* ----------------------------------------------------------------------------
 * This function gets the statistics of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    stats_p (OUT) The queue stats.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getQueueStats_plat(tmctl_devType_e     devType,
                                     tmctl_if_t*         if_p,
                                     int                 queueId,
                                     tmctl_queueStats_t* stats_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dev=%s qi=%d", ifp_to_name(devType, if_p), queueId);

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
        int channelId = 0;
        ret = tmctl_xtm_getTxChannelByQid(ifp_to_name(devType, if_p), queueId, &channelId);
        if (ret == TMCTL_ERROR)
        {
            tmctl_error("tmctl_xtm_getTxChannelByQid returns error");
            return ret;
        }
        return tmctl_RdpaGetQueueStats(devType, if_p, channelId, stats_p);
    }
#endif
    if (is_supported_rdpa_api(devType, if_p))
        return tmctl_RdpaGetQueueStats(devType, if_p, queueId, stats_p);
    tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
    return ret;

}  /* End of tmctl_getQueueStats() */


/* ----------------------------------------------------------------------------
 * This function gets port TM parameters (capabilities).
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    tmParms_p (OUT) Structure to return port TM parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPortTmParms_plat(tmctl_devType_e      devType,
                                      tmctl_if_t*          if_p,
                                      tmctl_portTmParms_t* tmParms_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: dev=%s", ifp_to_name(devType, if_p));

#if defined(BUILD_DSL)
   if(devType == TMCTL_DEV_XTM)
      return  tmctl_xtm_getTmParms(if_p->xtmIf.ifname, tmParms_p);
#endif

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (is_switch_intf(devType, if_p))
            return tmctlEthSw_getPortTmParms(if_p->ethIf.ifname, tmParms_p);
        return tmctl_RdpaGetPortTmParms(devType, if_p, tmParms_p);
    }
    tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
    return ret;
}  /* End of tmctl_getPortTmParms() */


#if defined(CONFIG_BCM_RUNNER_QOS_MAPPER)
/* ----------------------------------------------------------------------------
 * This function gets the configuration of dscp to pbit table. If the
 * configuration is not found, ....
 *
 * Parameters:
 *    cfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getDscpToPbit_plat(tmctl_dscpToPbitCfg_t* cfg_p)
{
   tmctl_debug("Enter: ");

   return tmctl_RdpaGetDscpToPbit(cfg_p->devType, cfg_p->if_p, cfg_p);

}  /* End of tmctl_getDscpToPbit() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of dscp to pbit table.
 *
 * Parameters:
 *    cfg_p (IN) config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setDscpToPbit_plat(tmctl_dscpToPbitCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    int i = 0;

    tmctl_debug("Enter: ");
    for (i = 0; i < TOTAL_DSCP_NUM; i++)
    {
        tmctl_debug("dscp[%d]=%d", i, cfg_p->dscp[i]);
    }

    ret = tmctl_RdpaSetDscpToPbit(cfg_p->devType, cfg_p->if_p, cfg_p);

    return ret;

}  /* End of tmctl_setDscpToPbit() */

/* ----------------------------------------------------------------------------
 * This function sets the configuration of traffic class to queue table.
 *
 * Parameters:
 *    dir (IN) direction.
 *    cfg_p (IN) config parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
#if defined(CONFIG_BCM_PLATFORM_RDP_PRV)
tmctl_ret_e tmctl_setTcToQueue_plat(int table_index, tmctl_tcToQCfg_t* cfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_debug("Enter: ");

   ret = tmctl_RdpaSetTc2Queue(table_index, cfg_p);
   
   return ret;

}  /* End of tmctl_setTcToQueue_plat() */
#endif
/* ----------------------------------------------------------------------------
 * This function gets the configuration of pbit to q table. If the
 * configuration is not found, ....
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    cfg_p (OUT) Structure to receive configuration parameters.
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPbitToQ_plat(tmctl_devType_e devType,
                                  tmctl_if_t* if_p,
                                  tmctl_pbitToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dev=%s", ifp_to_name(devType, if_p));


    ret = tmctlRdpa_getPbitToQ(devType, if_p, cfg_p);

    return ret;

}  /* End of tmctl_getPbitToQ() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of pbit to q table.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    cfg_p (IN) config parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPbitToQ_plat(tmctl_devType_e devType,
                                  tmctl_if_t* if_p,
                                  tmctl_pbitToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    ret = tmctlRdpa_setPbitToQ(devType, if_p, cfg_p);

   return ret;

}  /* End of tmctl_setPbitToQ() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of dscp to pbit feature.
 *
 * Parameters:
 *    dir (IN) direction.
 *    enable_p (OUT) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getForceDscpToPbit_plat(tmctl_dir_e dir, BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d", dir);

    ret = tmctlRdpa_getForceDscpToPbit(dir, enable_p);
    if (ret == TMCTL_ERROR)
    {
        tmctl_error("tmctlRdpa_getForceDscpToPbit ERROR! ret=%d", ret);
    }

    return ret;

}  /* End of tmctl_getForceDscpToPbit() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of dscp to pbit feature.
 *
 * Parameters:
 *    dir (IN) direction.
 *    enable_p (IN) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setForceDscpToPbit_plat(tmctl_dir_e dir, BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d enable=%d", dir, (*enable_p));

    ret = tmctlRdpa_setForceDscpToPbit(dir, enable_p);
    if (ret == TMCTL_ERROR)
    {
        tmctl_error("tmctlRdpa_setForceDscpToPbit ERROR! ret=%d", ret);
    }


    return ret;

}  /* End of tmctl_setForceDscpToPbit() */


/* ----------------------------------------------------------------------------
 * This function gets the configuration of packet based qos.
 *
 * Parameters:
 *    dir (IN) direction.
 *    type (IN) qos type
 *    enable_p (OUT) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getPktBasedQos_plat(tmctl_dir_e dir,
                                      tmctl_qosType_e type,
                                      BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d type=%d", dir, type);

    if (dir >= TMCTL_DIR_MAX)
    {
        tmctl_error("dir our of range. dir=%d", dir);
        return TMCTL_ERROR;
    }

    if (type >= TMCTL_QOS_MAX)
    {
        tmctl_error("type our of range. type=%d", type);
        return TMCTL_ERROR;
    }

    ret = tmctlRdpa_getPktBasedQos(dir, type, enable_p);
    if (ret == TMCTL_ERROR)
    {
        tmctl_error("tmctlRdpa_getPktBasedQos ERROR! ret=%d", ret);
    }

    return ret;

}  /* End of tmctl_getPktBasedQos() */


/* ----------------------------------------------------------------------------
 * This function sets the configuration of packet based qos.
 *
 * Parameters:
 *    dir (IN) direction.
 *    type (IN) qos type
 *    enable_p (IN) enable or disable
 *
 * Return:
 *    tmctl_return_e enum value.
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPktBasedQos_plat(tmctl_dir_e dir,
                                      tmctl_qosType_e type,
                                      BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_debug("Enter: dir=%d type=%d enable=%d", dir, type, (*enable_p));

    if (dir >= TMCTL_DIR_MAX)
    {
        tmctl_error("dir our of range. dir=%d", dir);
        return TMCTL_ERROR;
    }

    if (type >= TMCTL_QOS_MAX)
    {
        tmctl_error("type our of range. type=%d", type);
        return TMCTL_ERROR;
    }

    ret = tmctlRdpa_setPktBasedQos(dir, type, enable_p);
    if (ret == TMCTL_ERROR)
    {
        tmctl_error("tmctlRdpa_setPktBasedQos ERROR! ret=%d", ret);
    }

    return ret;

}  /* End of tmctl_setPktBasedQos() */
#endif

/* ----------------------------------------------------------------------------
 * This function sets the size of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    size    (IN) The drop threshold configuration.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueSize_plat(tmctl_devType_e          devType,
                                    tmctl_if_t*        if_p,
                                    int                queueId,
                                    int                size)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;
#if defined(BUILD_DSL)
   if(devType == TMCTL_DEV_XTM)
      return tmctl_xtm_setQueueSize(size);
#endif   //BUILD_DSL

   if (is_supported_rdpa_api(devType, if_p))
   {
       /* Set the queue size in runner for Ethernet switch LAN ports as well */
       return  tmctl_RdpaSetQueueSize(devType, if_p, queueId, size);
   }
   tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
                   ifp_to_name(devType, if_p), ret);
   return ret;

}  /* End of tmctl_setQueueSize() */

/* ----------------------------------------------------------------------------
 * This function sets shaper of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    shaper  (IN) Queue Shaper configuration
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueShaper_plat(tmctl_devType_e          devType,
                                      tmctl_if_t*        if_p,
                                      int                queueId,
                                      tmctl_shaper_t     *shaper_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    if (devType == TMCTL_DEV_XTM)
        return TMCTL_UNSUPPORTED;

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (!is_switch_intf(devType, if_p))
            return  tmctl_RdpaSetQueueShaper(devType, if_p, queueId, shaper_p);
        tmctl_error("swich interface is not supported, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
    }
    else
        tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);

   return ret;
}  /* End of tmctl_setQueueShaper() */

/* ----------------------------------------------------------------------------
 * This function create shaper for queues
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    shaper  (IN) Queue Shaper configuration
 *    shaper_obj  (OUT) pointer to shaper object
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_CreateShaper_plat(tmctl_devType_e          devType,
                                      tmctl_if_t*        if_p,
                                      tmctl_shaper_t     *shaper_p,
                                      uint64_t *shaper_obj)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    if (devType == TMCTL_DEV_XTM)
        return TMCTL_UNSUPPORTED;

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (!is_switch_intf(devType, if_p))
            return  tmctl_RdpaCreateShaper(devType, if_p, shaper_p, shaper_obj);
        tmctl_error("swich interface is not supported, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
    }
    else
        tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);

   return ret;
}

/* ----------------------------------------------------------------------------
 * This function delete shaper for queues
 *
 * Parameters:
 *    shaper_obj  (IN) pointer to shaper object
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_DeleteShaper_plat(uint64_t shaper_obj)
{
    return tmctl_RdpaDeleteShaper(shaper_obj);
}

/* ----------------------------------------------------------------------------
 * This function set subset shaper for queues
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) queue to use
 *    shaper_obj (IN) shaper object created - NULL to delete shaper from queue
 * 
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_SetShaperQueue_plat(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     int queueId,
                                     uint64_t shaper_obj)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    if (devType == TMCTL_DEV_XTM)
        return TMCTL_UNSUPPORTED;

    if (is_supported_rdpa_api(devType, if_p))
    {
        if (!is_switch_intf(devType, if_p))
            return  tmctl_RdpaSetShaperQueue(devType, if_p, queueId, shaper_obj);
        tmctl_error("swich interface is not supported, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);
    }
    else
        tmctl_error("is_supported_rdpa_api returns error, dev=%s, ret%d",
            ifp_to_name(devType, if_p), ret);

   return ret;
}

#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
/*TODO read the speed from interace */
static int is_wan_type_us_rate(char *wan_type, char *rate)
{
#define RATE_STR_LEN 2
    int count;
    char buf[16] = {0};

    if (wan_type)
    {
        count = (int)devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ, SCRATCH_PAD,
                                       RDPA_WAN_TYPE_PSP_KEY, 0, sizeof(buf), (void *)buf);
        if (count <= 0)
            return 0;

        if (strcasecmp(buf, wan_type))
            return 0;
    }

    count = (int)devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ, SCRATCH_PAD,
                                   RDPA_WAN_RATE_PSP_KEY, 0, sizeof(buf), (void *)buf);
    if (count <= 0)
        return 0;

    if (!strncasecmp(&buf[RATE_STR_LEN], rate, RATE_STR_LEN))
        return 1;

    return 0;
}
#endif

tmctl_ret_e tmctl_createPolicer_plat(tmctl_policer_t *policer_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
#if defined(POLICER_SUPPORT)
    ret = tmctl_RdpaCreatePolicer(policer_p);
#endif
    return ret;
}  /* End of tmctl_createPolicer_plat() */

tmctl_ret_e tmctl_modifyPolicer_plat(tmctl_policer_t *policer_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
#if defined(POLICER_SUPPORT)
		ret = tmctl_RdpaModifyPolicer(policer_p);
#endif
    return ret;
}  /* End of tmctl_modifyPolicer_plat() */

tmctl_ret_e tmctl_deletePolicer_plat(int policerId)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
#if defined(POLICER_SUPPORT)
    ret = tmctl_RdpaDeletePolicer(policerId);
#endif
    return ret;
}    /* End of tmctl_deletePolicer_plat() */

/* ----------------------------------------------------------------------------
 * This function sets port enable/disable state.
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    if_p            (IN) Port identifier.
 *    state           (IN) Enable (TRUE) or Disable (FALSE).
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setPortState_plat(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   BOOL            state)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

#if defined(BCM_PON) || ((defined(CHIP_63158) || defined(CHIP_6813)) && defined(BRCM_OMCI))

    switch (devType)
    {

      case TMCTL_DEV_GPON:
         ret = tmctl_RdpaSetTcontState(if_p->gponIf.tcontid, state);
         if (ret != TMCTL_SUCCESS)
         {
            tmctl_error("tmctl_RdpaSetTcontState: failed for dev=%s", ifp_to_name(devType, if_p));

         }
         break;

      case TMCTL_DEV_SVCQ:
      case TMCTL_DEV_EPON:
      case TMCTL_DEV_XTM:
      case TMCTL_DEV_NONE:
      default:
        tmctl_error("tmctl_setPortState: not supported for dev=%s", ifp_to_name(devType, if_p));
        return TMCTL_ERROR;
    }
#endif
    return ret;
}    /* End of tmctl_setPortState_plat() */

void tmctl_getPortTmState_plat(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   BOOL            *state)
{
    *state = FALSE;
#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
     tmctl_RdpaGetPortTmState(devType, if_p, state);
#endif
}


/* ----------------------------------------------------------------------------
 * This function gets tx/rx mirror config.
 *
 * Parameters:
 *    op                (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getMirror_plat(tmctl_mirror_op_e op)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

#if defined(RUNNER_SWITCH)
    ret = tmctl_RdpaMirrorPrint(op);
#else
    tmctl_mirror_t     mirror;
    // runner + sf2 use ethswctl to config sf2
    mirror.unit = 1;
    mirror.op = op;
    ret = tmctl_EthSwGetMirror(&mirror);
    tmctl_EthSwMirrorPrint(&mirror);
#endif

#if defined(BUILD_DSL) && !defined(XTM_HW_ACCEL_MIRRORING)
    ret = tmctl_xtm_getMirror(op);
    if (ret != TMCTL_SUCCESS)
    {
        tmctl_error("tmctl_xtm_getMirror failed! ret=%d", ret);
        return ret;
    }
#endif

    return ret;
}

/* ----------------------------------------------------------------------------
 * This function clears tx/rx mirror config
 *
 * Parameters:
 *    op                (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_clrMirror_plat(tmctl_mirror_op_e op)

{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

#if defined(BUILD_DSL) && !defined(XTM_HW_ACCEL_MIRRORING)
    ret = tmctl_xtm_clrMirror(op);
    if (ret != TMCTL_SUCCESS)
    {
        tmctl_error("tmctl_xtm_clrMirror failed! ret=%d", ret);
        return ret;
    }
#endif

#if defined(RUNNER_SWITCH)
    ret = tmctl_RdpaClrMirror(op);
#else
    // runner + sf2 use ethswctl to config sf2
    tmctl_mirror_t     mirror;
    mirror.op = op;
    mirror.unit = 1;
    ret = tmctl_EthSwClrMirror(&mirror);
#endif
    return ret;
}

/* ----------------------------------------------------------------------------
 * This function adds tx/rx mirror config
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    if_p            (IN) Port identifier.
 *    op              (IN) specified RX/TX operation
 *    destIf          (IN) dest port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_addMirror_plat(tmctl_devType_e        devType,
                                tmctl_if_t*         if_p,
                                tmctl_mirror_op_e   op,
                                tmctl_ethIf_t       destIf,
                                unsigned int        truncate_size)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
#if defined(XTM_HW_ACCEL_MIRRORING)
        ret = tmctl_RdpaAddMirror("dsl", destIf.ifname, op, truncate_size);
        if (ret != TMCTL_SUCCESS)        
            tmctl_error("tmctl_RdpaAddMirror failed! ret=%d", ret);
#else
        ret = tmctl_xtm_addMirror(if_p->xtmIf.ifname, destIf.ifname, op);
        if (ret != TMCTL_SUCCESS)
            tmctl_error("tmctl_xtm_addMirror failed! ret=%d", ret);
#endif
        return ret;
    }
#endif

#if defined(RUNNER_SWITCH)
    if (devType == TMCTL_DEV_WLAN && !is_valid_wlan_prefix(if_p->ethIf.ifname))
        return TMCTL_UNSUPPORTED;

    ret = tmctl_RdpaAddMirror(if_p->ethIf.ifname, destIf.ifname, op, truncate_size);
#else
    tmctl_mirror_t     mirror;
    int unit, port;
    
    if (devType != TMCTL_DEV_ETH || bcm_enet_map_ifname_to_unit_port(if_p->ethIf.ifname, &unit, &port) || unit !=1)
    {
        tmctl_error("Only support mirroring on eth ports on SF2 switch");
        return TMCTL_UNSUPPORTED;
    }
    mirror.unit = 1;
    mirror.srcPort = port;
    mirror.op = op;
    if (bcm_enet_map_ifname_to_unit_port(destIf.ifname, &unit, &port) || unit !=1)
    {
        tmctl_error("Both source and mirroring ports need to be on same SF2 switch");
        return TMCTL_UNSUPPORTED;
    }
    mirror.destPort = port;
    
    ret = tmctl_EthSwAddMirror(&mirror);
#endif
    return ret;
}

/* ----------------------------------------------------------------------------
 * This function dels tx/rx mirror config
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    op                (IN) specified RX/TX operation
 *    if_p            (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_delMirror_plat(tmctl_devType_e        devType,
                                tmctl_mirror_op_e   op,
                                tmctl_if_t*         if_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

#if defined(BUILD_DSL)
    if(devType == TMCTL_DEV_XTM)
    {
#if defined(XTM_HW_ACCEL_MIRRORING)
        ret = tmctl_RdpaDelMirror("dsl", op);
        if (ret != TMCTL_SUCCESS)        
            tmctl_error("tmctl_RdpaAddMirror failed! ret=%d", ret);
#else
        ret = tmctl_xtm_delMirror(if_p->xtmIf.ifname, op);
        if (ret != TMCTL_SUCCESS)
            tmctl_error("tmctl_xtm_delMirror failed! ret=%d", ret);
#endif
        return ret;
    }
#endif

#if defined(RUNNER_SWITCH)
    if (devType == TMCTL_DEV_WLAN && !is_valid_wlan_prefix(if_p->ethIf.ifname))
        return TMCTL_UNSUPPORTED;

    ret = tmctl_RdpaDelMirror(if_p->ethIf.ifname, op);
#else
    tmctl_mirror_t      mirror;
    int unit, port;
    
    if (devType != TMCTL_DEV_ETH || bcm_enet_map_ifname_to_unit_port(if_p->ethIf.ifname, &unit, &port) || unit !=1)
    {
        tmctl_error("Only support mirroring on eth ports on SF2 switch");
        return TMCTL_UNSUPPORTED;
    }
    memset(&mirror, 0, sizeof(tmctl_mirror_t));
    mirror.unit = 1;
    mirror.srcPort = port;
    mirror.op = op;
    
    ret = tmctl_EthSwDelMirror(&mirror);
#endif
    return ret;
}

void tmctl_getDevTypeByIfName_plat(tmctl_devType_e *devType, 
                                   char **oifname,
                                   const char *ifname)
{
    rdpactl_get_dev_type_by_ifname(devType, oifname, ifname);
}
