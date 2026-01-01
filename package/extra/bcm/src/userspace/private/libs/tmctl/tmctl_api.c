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
#include <sys/stat.h>
#include "os_defs.h"
#include "tmctl_api.h"
#include "tmctl_plat.h"
#include "tmctl_api_trace.h"
#include "bcmctl_syslog.h"
#include <json-c/json.h>
#include <linux/ethtool.h>
#include "ethswctl_api.h"
#include <bp3_license.h>

/************* M A C R O S  &  D E F I N I T I O N S **************/

#define QUEUE_SIZE_FACTOR                1536
#define TMCTL_DEF_Q_SIZE_1_MB            ((1024 * 1024) / QUEUE_SIZE_FACTOR)
#define TMCTL_DEF_Q_SIZE_4_MB            4 * (TMCTL_DEF_Q_SIZE_1_MB)
#define TMCTL_DEF_Q_SIZE_400KB           ((400 * 1024) / QUEUE_SIZE_FACTOR)

#define TMCTL_DEF_ETH_Q_SZ_LE_2_5G_EPON  (130) /* ~200K bytes */

#if defined(BCM_PON_XRDP) || defined(BCM_DSL_XRDP)
#define TMCTL_DEF_ETH_Q_SZ_10G_ETH       TMCTL_DEF_Q_SIZE_4_MB
#define TMCTL_DEF_ETH_Q_SZ_5G_ETH        TMCTL_DEF_Q_SIZE_4_MB
#define TMCTL_DEF_ETH_Q_SZ_2_5G_ETH      TMCTL_DEF_Q_SIZE_1_MB
#define TMCTL_DEF_ETH_Q_SZ_1G_ETH        TMCTL_DEF_Q_SIZE_1_MB
#define TMCTL_DEF_ETH_Q_SZ_100M_ETH      TMCTL_DEF_Q_SIZE_1_MB
#define TMCTL_DEF_ETH_Q_SZ_10M_ETH       TMCTL_DEF_Q_SIZE_1_MB
#else
#if defined(SUPPORT_ARCHERCTL)
#define TMCTL_DEF_ETH_Q_SZ_10G_ETH       1024
#define TMCTL_DEF_ETH_Q_SZ_5G_ETH        1024
#define TMCTL_DEF_ETH_Q_SZ_2_5G_ETH      1024
#define TMCTL_DEF_ETH_Q_SZ_1G_ETH        1024
#define TMCTL_DEF_ETH_Q_SZ_100M_ETH      1024
#define TMCTL_DEF_ETH_Q_SZ_10M_ETH       1024
#else
#define TMCTL_DEF_ETH_Q_SZ_10G_ETH       1024
#define TMCTL_DEF_ETH_Q_SZ_5G_ETH        1024
#define TMCTL_DEF_ETH_Q_SZ_2_5G_ETH      1024
#define TMCTL_DEF_ETH_Q_SZ_1G_ETH        1024
#define TMCTL_DEF_ETH_Q_SZ_100M_ETH      1024
#define TMCTL_DEF_ETH_Q_SZ_10M_ETH       1024
#endif
#endif

/** JASON TAGS **/
#define JSK_PROFILES_LIST                "profiles"
#define JSK_LINK_SPEEDS_LIST             "link_speeds"
#define JSK_LINK_SPEED                   "link_speed"
#define JSK_DEVICES_QUEUE_SIZES          "devices_queue_sizes"
#define JSK_DEVICE_TYPE                  "device_type"
#define JSK_PRIORITY_QUEUES_LIST         "priority_queues"
#define JSK_QUEUE_SIZE                   "queue_size"

/****************** T Y P E S  D E F I N I T I O N ****************/
typedef enum {
   __TMCTL_PE_DFT_PROFILE_ENTRY = 0x00,
   __TMCTL_PE_CFG_PROFILE_ENTRY,
   __TMCTL_PE_PROFILE_ENTRY_SIZE //End sentinel
} tmctl_profile_entry_t;

typedef struct {
   tmctl_devType_e dev_type;
   uint32_t *queue_sizes;
   uint16_t count;
}tmctl_dev_qsize_t;

typedef struct {
   uint32_t  link_speed;
   tmctl_dev_qsize_t *devices_list;
   uint16_t  dev_count;
} tmctl_link_speed_t;

typedef struct {
   tmctl_link_speed_t *link_speeds;
   uint16_t count;
} tmctl_profile_t;

typedef struct {
   tmctl_profile_t *list;
   uint16_t count;
} tmctl_profiles_t;

typedef struct  {
   uint32_t profile_idx;
   uint32_t device_idx;
   uint32_t speed_idx;
   uint32_t priority_idx;
} tmctl_profile_idexer_t; 

#if defined(BCMCTL_SYSLOG_SUPPORTED)
IMPL_setSyslogLevel(tmctl);
IMPL_getSyslogLevel(tmctl);
IMPL_isSyslogLevelEnabled(tmctl);
IMPL_setSyslogMode(tmctl);
IMPL_isSyslogEnabled(tmctl);
#endif /* BCMCTL_SYSLOG_SUPPORTED */

/*********************** S T A T I C  D A T A *********************/
static tmctl_profiles_t configuration_profiles = {
   .list = (tmctl_profile_t []) {
      /* Load Time Default Configuration Profile */
      {
         .link_speeds = (tmctl_link_speed_t[]) {
            {
               .link_speed = TMCTL_UNDEFINED_LINK_SPEED,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_SVCQ,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_1_MB,
                     },
                     .count = 1,
                  },
               },
               .dev_count = 1,
            },
            {
               .link_speed = SPEED_10,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_ETH,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_10M_ETH,
                     },
                     .count = 1,
                  },
               },
               .dev_count = 1,
            },
            {
               .link_speed = SPEED_100,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_ETH,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_100M_ETH,
                     },
                     .count = 1,
                  },
               },
               .dev_count = 2,
            },
            {
               .link_speed = SPEED_1000,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_EPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_LE_2_5G_EPON,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_GPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_1_MB,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_ETH,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_1G_ETH,
                     },
                     .count = 1,
                  },
               },
               .dev_count = 3,
            }, 
            {
               .link_speed = SPEED_2500,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_EPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_LE_2_5G_EPON,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_GPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_1_MB,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_ETH,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_2_5G_ETH,
                     },
                     .count = 1,
                  },
               },
               .dev_count  = 3,
            },
            {
               .link_speed = SPEED_5000,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_EPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_4_MB,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_GPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_4_MB,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_ETH,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_5G_ETH,
                     },
                     .count = 1,
                  },
               },
               .dev_count = 3,
            }, 
            {
               .link_speed = SPEED_10000,
               .devices_list = (tmctl_dev_qsize_t []) {
                  {
                     .dev_type = TMCTL_DEV_EPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_4_MB,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_GPON,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_Q_SIZE_4_MB,
                     },
                     .count = 1,
                  },
                  {
                     .dev_type = TMCTL_DEV_ETH,
                     .queue_sizes = (uint32_t []) {
                        TMCTL_DEF_ETH_Q_SZ_10G_ETH,
                     },
                     .count = 1,
                  },
               },
               .dev_count = 3,
            },
         }, /* End of link_speeds */
         .count = __TS_LAST + 1
      },
      /* Dynamically (run-time) Loaded Configuration Profile */
      {
         .link_speeds = NULL,
         .count = 0
      }
   },
   .count = 2,
};

/************** F A R W A R D  D E C L A R A T I O N S ************/
static BOOL         json_parse                     (tmctl_profiles_t *profiles,
                                                    tmctl_profile_idexer_t *p_indexer, 
                                                    uint16_t profile_idx, 
                                                    json_object *obj);
static BOOL         json_parse_array               (tmctl_profiles_t *profiles,
                                                    tmctl_profile_idexer_t *p_indexer, 
                                                    uint16_t profile_idx, 
                                                    json_object *obj, 
                                                    char *key);
static BOOL         allocate_pqueue_sublist_idx    (const char *key, 
                                                    const uint16_t sublist_len, 
                                                    const uint16_t profile_idx, 
                                                    tmctl_profiles_t *profiles, 
                                                    tmctl_profile_idexer_t *p_indexer,
                                                    uint32_t **duty_ptr);
static uint32_t     read_queue_profile_config      (const tmctl_profiles_t * profiles, 
                                                    const tmctl_profile_entry_t profile_idx,
                                                    const tmctl_devType_e dev_type,
                                                    const uint32_t link_speed, 
                                                    const uint16_t priority_idx);
static void         clean_profile                  (tmctl_profile_t *profile);                                                 
static BOOL         file_exist                     (const char *filename);

/***************************** A P I ******************************/
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
 *    numQueues (IN) Number of queues to be set for TM.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_portTmInit(tmctl_devType_e devType,
                             tmctl_if_t*     if_p,
                             uint32_t        cfgFlags,
                             int             numQueues)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   if ((devType == TMCTL_DEV_SVCQ) &&
       (bcm_license_check(BP3_FEATURE_SERVICE_QUEUE) <= 0))
   {
      tmctl_error("No valid license for the service queue");
      return ret;
   }

   tmctl_portTmInitTrace(devType, if_p, cfgFlags, numQueues);

   ret = tmctl_portTmInit_plat(devType, if_p, cfgFlags, numQueues);

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
tmctl_ret_e tmctl_portTmUninit(tmctl_devType_e devType,
                               tmctl_if_t*     if_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_portTmUninitTrace(devType, if_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_portTmUninit_plat(devType, if_p);

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
tmctl_ret_e tmctl_getQueueCfg(tmctl_devType_e   devType,
                              tmctl_if_t*       if_p,
                              int               queueId,
                              tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getQueueCfgTrace(devType, if_p, queueId, qcfg_p);

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);


   ret = tmctl_getQueueCfg_plat(devType, if_p, queueId, qcfg_p);

   return ret;

}  /* End of tmctl_getQueueCfg() */


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
 *                - For 63138 or 63148 TMCTL_DEV_ETH device type,
 *                  -- the priority of SP queue must be set to qid.
 *                  -- the priority of WRR/WDRR/WFQ queue must be set to 0.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueCfg(tmctl_devType_e   devType,
                              tmctl_if_t*       if_p,
                              tmctl_queueCfg_t* qcfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   if ((devType == TMCTL_DEV_SVCQ) &&
       (bcm_license_check(BP3_FEATURE_SERVICE_QUEUE) <= 0))
   {
      tmctl_error("No valid license for the service queue");
      return ret;
   }

   if ((qcfg_p->bestEffort) && (bcm_license_check(BP3_FEATURE_SERVICE_QUEUE) <= 0))
   {
      tmctl_debug("there is No valid license for best effort queue - using regular queue instead");
      qcfg_p->bestEffort = 0;
   }

   tmctl_setQueueCfgTrace(devType, if_p, qcfg_p);
   qcfg_p->qsize = tmctl_getIfacePriorityQueueSize(devType, if_p, qcfg_p->priority, qcfg_p->qsize);
   if (TMCTL_DEFAULT_QUEUE_SIZE != qcfg_p->qsize)
   {
      tmctl_debug("Enter: devType=%d qid=%d priority=%d qsize=%d schedMode=%d wt=%d minBufs=%d minRate=%d kbps=%d mbs=%d",
                  devType, qcfg_p->qid, qcfg_p->priority, qcfg_p->qsize, qcfg_p->schedMode,
                  qcfg_p->weight, qcfg_p->minBufs, qcfg_p->shaper.minRate,
                  qcfg_p->shaper.shapingRate, qcfg_p->shaper.shapingBurstSize);

      ret = tmctl_setQueueCfg_plat(devType, if_p, qcfg_p);
   }
   return ret;

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
tmctl_ret_e tmctl_delQueueCfg(tmctl_devType_e devType,
                              tmctl_if_t*     if_p,
                              int             queueId)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_delQueueCfgTrace(devType, if_p, queueId);

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);

   ret = tmctl_delQueueCfg_plat(devType, if_p, queueId);

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
tmctl_ret_e tmctl_getPortShaper(tmctl_devType_e devType,
                                tmctl_if_t*     if_p,
                                tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getPortShaperTrace(devType, if_p, shaper_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_getPortShaper_plat(devType, if_p, shaper_p);

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
tmctl_ret_e tmctl_setPortShaper(tmctl_devType_e devType,
                                tmctl_if_t*     if_p,
                                tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_setPortShaperTrace(devType, if_p, shaper_p);

   tmctl_debug("Enter: devType=%d portKbps=%d portMbs=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);

   ret = tmctl_setPortShaper_plat(devType, if_p, shaper_p);

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
tmctl_ret_e tmctl_getPortRxRate(tmctl_devType_e devType,
                                tmctl_if_t*     if_p,
                                tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getPortRxRateTrace(devType, if_p, shaper_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_getPortRxRate_plat(devType, if_p, shaper_p);

   return ret;

}  /* End of tmctl_getPortRxRate() */


/* ----------------------------------------------------------------------------
 * This function configures the port rx rate, shaping burst
 * size and minimum rate. 
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
tmctl_ret_e tmctl_setPortRxRate(tmctl_devType_e devType,
                                tmctl_if_t*     if_p,
                                tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_setPortRxRateTrace(devType, if_p, shaper_p);

   tmctl_debug("Enter: devType=%d portKbps=%d portMbs=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);

   ret = tmctl_setPortRxRate_plat(devType, if_p, shaper_p);

   return ret;

}  /* End of tmctl_setPortRxRate() */

/* ----------------------------------------------------------------------------
 * This function gets the overall shaper configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getOverAllShaper(tmctl_devType_e devType,
                                tmctl_if_t*     if_p,
                                tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getOverAllShaperTrace(devType, if_p, shaper_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_getOverAllShaper_plat(devType, if_p, shaper_p);

   return ret;

}  /* End of tmctl_getOverAllShaper() */

/* ----------------------------------------------------------------------------
 * This function configures the overall shaper for shaping rate, shaping burst
 * size and max rate.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    shaper_p (IN) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setOverAllShaper(tmctl_devType_e devType,
                                tmctl_shaper_t* shaper_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_setOverAllShaperTrace(devType, shaper_p);

   tmctl_debug("Enter: devType=%d Rate=%d BurstSize=%d", devType,
               shaper_p->shapingRate, shaper_p->shapingBurstSize);

   ret = tmctl_setOverAllShaper_plat(devType, shaper_p);

   return ret;

}  /* End of tmctl_setOverAllShaper() */

/* ----------------------------------------------------------------------------
 * This function link the port to the overall shaper.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_linkOverAllShaper(tmctl_devType_e devType,
                                tmctl_if_t*     if_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_linkOverAllShaperTrace(devType, if_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_linkOverAllShaper_plat(devType, if_p);

   return ret;

}  /* End of tmctl_getPortShaper() */

/* ----------------------------------------------------------------------------
 * This function unlink the port from the overall shaper.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_unlinkOverAllShaper(tmctl_devType_e devType,
                                tmctl_if_t*     if_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_unlinkOverAllShaperTrace(devType, if_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_unlinkOverAllShaper_plat(devType, if_p);

   return ret;

}  /* End of tmctl_getPortShaper() */


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
tmctl_ret_e tmctl_getQueueDropAlg(tmctl_devType_e       devType,
                                  tmctl_if_t*           if_p,
                                  int                   queueId,
                                  tmctl_queueDropAlg_t* dropAlg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getQueueDropAlgTrace(devType, if_p, queueId, dropAlg_p);

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);

   ret = tmctl_getQueueDropAlg_plat(devType, if_p, queueId, dropAlg_p);

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
tmctl_ret_e tmctl_setQueueDropAlg(tmctl_devType_e       devType,
                                  tmctl_if_t*           if_p,
                                  int                   queueId,
                                  tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_setQueueDropAlgTrace(devType, if_p, queueId, dropAlg_p);

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d dropAlgorithm=%d "
               "priorityMask0=0x%x priorityMask1=0x%x "
               "Lo(minThr=%d maxThr=%d pct=%d) "
               "Hi(minThr=%d maxThr=%d pct=%d)",
               devType, if_p->ethIf.ifname, queueId, dropAlg_p->dropAlgorithm,
               dropAlg_p->priorityMask0, dropAlg_p->priorityMask1,
               dropAlg_p->dropAlgLo.redMinThreshold, dropAlg_p->dropAlgLo.redMaxThreshold,
               dropAlg_p->dropAlgLo.redPercentage,
               dropAlg_p->dropAlgHi.redMinThreshold, dropAlg_p->dropAlgHi.redMaxThreshold,
               dropAlg_p->dropAlgHi.redPercentage);

   ret = tmctl_setQueueDropAlg_plat(devType, if_p, queueId, dropAlg_p);

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
tmctl_ret_e tmctl_getXtmChannelDropAlg(tmctl_devType_e       devType,
                                       int                   channelId,
                                       tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getXtmChannelDropAlgTrace(devType, channelId, dropAlg_p);

   tmctl_debug("Enter: devType=%d channelId=%d", devType, channelId);

   ret = tmctl_getXtmChannelDropAlg_plat(devType, channelId, dropAlg_p);

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
tmctl_ret_e tmctl_setXtmChannelDropAlg(tmctl_devType_e       devType,
                                       int                   channelId,
                                       tmctl_queueDropAlg_t* dropAlg_p)
{

   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_setXtmChannelDropAlgTrace(devType, channelId, dropAlg_p);

   tmctl_debug("Enter: devType=%d channelId=%d dropAlgorithm=%d  "
               "priorityMask0=0x%x priorityMask1=0x%x",
               devType, channelId, dropAlg_p->dropAlgorithm,
               dropAlg_p->priorityMask0, dropAlg_p->priorityMask1);

   ret = tmctl_setXtmChannelDropAlg_plat(devType, channelId, dropAlg_p);

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
tmctl_ret_e tmctl_setQueueFlowCtrl(tmctl_devType_e        devType,
                                   tmctl_if_t*            if_p,
                                   int                    queueId,
                                   tmctl_queueFlowCtrl_t* flowCtrl_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_setQueueFlowCtrlTrace(devType, if_p, queueId, flowCtrl_p);

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d enable=%d "
               "pauseThr=%d hysteresisThr=%d",
               devType, if_p->ethIf.ifname, queueId, flowCtrl_p->enable,
               flowCtrl_p->pauseThr, flowCtrl_p->hystThr);

   ret = tmctl_setQueueFlowCtrl_plat(devType, if_p, queueId, flowCtrl_p);

   return ret;
} /* End of tmctl_setQueueFlowCtrl */

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
tmctl_ret_e tmctl_getQueueFlowCtrl(tmctl_devType_e        devType,
                                   tmctl_if_t*            if_p,
                                   int                    queueId,
                                   tmctl_queueFlowCtrl_t* flowCtrl_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getQueueFlowCtrlTrace(devType, if_p, queueId, flowCtrl_p);

   tmctl_debug("Enter: devType=%d ifname=%s qid=%d",
               devType, if_p->ethIf.ifname, queueId);

   ret = tmctl_getQueueFlowCtrl_plat(devType, if_p, queueId, flowCtrl_p);

   return ret;
} /* End of tmctl_getQueueFlowCtrl */

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
tmctl_ret_e tmctl_getQueueStats(tmctl_devType_e     devType,
                                tmctl_if_t*         if_p,
                                int                 queueId,
                                tmctl_queueStats_t* stats_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getQueueStatsTrace(devType, if_p, queueId, stats_p);

   tmctl_debug("Enter: devType=%d qid=%d", devType, queueId);

   ret = tmctl_getQueueStats_plat(devType, if_p, queueId, stats_p);

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
tmctl_ret_e tmctl_getPortTmParms(tmctl_devType_e      devType,
                                 tmctl_if_t*          if_p,
                                 tmctl_portTmParms_t* tmParms_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getPortTmParmsTrace(devType, if_p, tmParms_p);

   tmctl_debug("Enter: devType=%d", devType);

   ret = tmctl_getPortTmParms_plat(devType, if_p, tmParms_p);

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
tmctl_ret_e tmctl_getDscpToPbit(tmctl_dscpToPbitCfg_t* cfg_p)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;

   tmctl_getDscpToPbitTrace();

   tmctl_debug("Enter: ");

   ret = tmctl_getDscpToPbit_plat(cfg_p);

   return ret;

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
tmctl_ret_e tmctl_setDscpToPbit(tmctl_dscpToPbitCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    int i = 0;

    tmctl_setDscpToPbitTrace(cfg_p);
    tmctl_debug("Enter: ");
    for (i = 0; i < TOTAL_DSCP_NUM; i++)
    {
        tmctl_debug("dscp[%d]=%d", i, cfg_p->dscp[i]);
    }
    tmctl_debug("remark %d", cfg_p->remark);

    ret = tmctl_setDscpToPbit_plat(cfg_p);

    return ret;

}  /* End of tmctl_setDscpToPbit() */

#if defined(CONFIG_BCM_PLATFORM_RDP_PRV)
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
tmctl_ret_e tmctl_setTcToQueue(int table_index, tmctl_tcToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_setTcToQueueTrace(table_index, cfg_p);

    tmctl_debug("Enter: table_index=%d", table_index);

    ret = tmctl_setTcToQueue_plat(table_index, cfg_p);
    if (ret == TMCTL_ERROR)
    {
        tmctl_error("tmctl_setTcToQueue_plat ERROR! ret=%d", ret);
    }

    return ret;
}
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
tmctl_ret_e tmctl_getPbitToQ(tmctl_devType_e devType,
                                 tmctl_if_t* if_p,
                                 tmctl_pbitToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_getPbitToQTrace(devType, if_p, cfg_p);

    tmctl_debug("Enter: devType=%d", devType);

    ret = tmctl_getPbitToQ_plat(devType, if_p, cfg_p);

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
tmctl_ret_e tmctl_setPbitToQ(tmctl_devType_e devType,
                                 tmctl_if_t* if_p,
                                 tmctl_pbitToQCfg_t* cfg_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;
    int i = 0;

    tmctl_setPbitToQTrace(devType, if_p, cfg_p);

    tmctl_debug("Enter: devType=%d", devType);

    for (i = 0; i < TOTAL_PBIT_NUM; i++)
    {
        tmctl_debug("pbit[%d]=%d", i, cfg_p->pbit[i]);
    }

    ret = tmctl_setPbitToQ_plat(devType, if_p, cfg_p);

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
tmctl_ret_e tmctl_getForceDscpToPbit(tmctl_dir_e dir, BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_getForceDscpToPbitTrace(dir, enable_p);

    tmctl_debug("Enter: dir=%d", dir);

    ret = tmctl_getForceDscpToPbit_plat(dir, enable_p);

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
tmctl_ret_e tmctl_setForceDscpToPbit(tmctl_dir_e dir, BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_setForceDscpToPbitTrace(dir, enable_p);

    tmctl_debug("Enter: dir=%d enable=%d", dir, (*enable_p));

    ret = tmctl_setForceDscpToPbit_plat(dir, enable_p);

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
tmctl_ret_e tmctl_getPktBasedQos(tmctl_dir_e dir,
                                  tmctl_qosType_e type,
                                  BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_getPktBasedQosTrace(dir, type, enable_p);

    tmctl_debug("Enter: dir=%d type=%d", dir, type);

    ret = tmctl_getPktBasedQos_plat(dir, type, enable_p);

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
tmctl_ret_e tmctl_setPktBasedQos(tmctl_dir_e dir,
                                  tmctl_qosType_e type,
                                  BOOL* enable_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_setPktBasedQosTrace(dir, type, enable_p);

    tmctl_debug("Enter: dir=%d type=%d enable=%d", dir, type, (*enable_p));

    ret = tmctl_setPktBasedQos_plat(dir, type, enable_p);

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
tmctl_ret_e tmctl_setQueueSize(tmctl_devType_e devType,
                               tmctl_if_t*     if_p,
                               int             queueId,
                               int             size)
{
   tmctl_ret_e ret = TMCTL_UNSUPPORTED;
   tmctl_queueCfg_t qcfg = {0};

   if (TMCTL_DEFAULT_QUEUE_SIZE == size)
   {
      if (TMCTL_SUCCESS == tmctl_getQueueCfg(devType, if_p, queueId, &qcfg))
      {

         size = tmctl_getIfacePriorityQueueSize(devType, if_p, qcfg.priority, size);
      }
   }

   if (TMCTL_DEFAULT_QUEUE_SIZE != size)
   {
      tmctl_setQueueSizeTrace(devType, if_p, queueId, size);
      ret = tmctl_setQueueSize_plat(devType, if_p, queueId, size);
   }
   
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
tmctl_ret_e tmctl_setQueueShaper(tmctl_devType_e          devType,
                                     tmctl_if_t*        if_p,
                                     int                queueId,
                                     tmctl_shaper_t     *shaper_p)
{
    tmctl_ret_e ret = TMCTL_UNSUPPORTED;

    tmctl_setQueueSizeShaperTrace(devType, if_p, queueId, shaper_p);

    ret = tmctl_setQueueShaper_plat(devType, if_p, queueId, shaper_p);

    return ret;
}  /* End of tmctl_setQueueShaper() */

tmctl_ret_e tmctl_CreateShaper(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     tmctl_shaper_t     *shaper_p,
                                     uint64_t *shaper_obj)
{
    tmctl_ret_e ret = TMCTL_SUCCESS;
    tmctl_CreateShaperTrace(devType, if_p, shaper_p, shaper_obj);
    ret = tmctl_CreateShaper_plat(devType, if_p, shaper_p, shaper_obj);
    return ret;
} /* End of tmctl_CreateShaper() */

tmctl_ret_e tmctl_DeleteShaper(uint64_t shaper_obj)
{
    tmctl_ret_e ret = TMCTL_SUCCESS;
    tmctl_DeleteShaperTrace(shaper_obj);
    ret = tmctl_DeleteShaper_plat(shaper_obj);
    return ret;
} /* End of tmctl_DeleteShaper() */

tmctl_ret_e tmctl_SetShaperQueue(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     int queueId,
                                     uint64_t shaper_obj)
{
    tmctl_ret_e ret = TMCTL_SUCCESS;
    tmctl_SetShaperQueueTrace(devType, if_p, queueId, shaper_obj);
    ret = tmctl_SetShaperQueue_plat(devType, if_p, queueId, shaper_obj);
    
    return ret;
} /* End of tmctl_SetShaperQueue() */

/* ----------------------------------------------------------------------------
 * This function creates a policer.
 *
 * Parameters:
 *    tmctl_policer_t (IN) tmctl policer.
 *    Note: policerId in tmctl_policer_t can be input and output parameter
 *          If policerId is expected as output, please use TMCTL_INVALID_KEY as input.
 *          Then you can get the system allocated index in policerId.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_createPolicer(tmctl_policer_t *policer_p)
{
    return tmctl_createPolicer_plat(policer_p);
}

/* ----------------------------------------------------------------------------
 * This function modify a policer.
 *
 * Parameters:
 *    tmctl_policer_t (IN) tmctl policer
 *    Note: policerId, dir and type can not be modified.
 *          Please use TMCTL_INVALID_KEY to indicate the value is not meant to be modified.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_modifyPolicer(tmctl_policer_t *policer_p)
{
    return tmctl_modifyPolicer_plat(policer_p);
}

/* ----------------------------------------------------------------------------
 * This function deletes a policer.
 *
 * Parameters:
 *    dir       (IN) direction.
 *    policerId (IN) index
 *    Note: Dir and policerId compose the key of the policer to be deleted.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_deletePolicer(int policerId)
{
    return tmctl_deletePolicer_plat(policerId);
}
/* ----------------------------------------------------------------------------
 * This function gets the tx/rx mirror configuration.
 *
 * Parameters:
 *    op                (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getMirror(tmctl_mirror_op_e op)
{
    return tmctl_getMirror_plat(op);
}

/* ----------------------------------------------------------------------------
 * This function clear the tx/rx mirror configuration.
 *
 * Parameters:
 *    op                (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_clrMirror(tmctl_mirror_op_e op)
{
    return tmctl_clrMirror_plat(op);
}

/* ----------------------------------------------------------------------------
 * This function add tx/rx mirror configuration.
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
tmctl_ret_e tmctl_addMirror(tmctl_devType_e        devType,
                                tmctl_if_t*         if_p,
                                tmctl_mirror_op_e   op,
                                tmctl_ethIf_t       destIf,
                                unsigned int        truncate_size)
{
    return tmctl_addMirror_plat(devType, if_p, op, destIf, truncate_size);
}

/* ----------------------------------------------------------------------------
 * This function del tx/rx mirror configuration.
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    if_p            (IN) Port identifier.
 *    op              (IN) specified RX/TX operation
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_delMirror(tmctl_devType_e        devType,
                                tmctl_mirror_op_e   op,
                                tmctl_if_t*         if_p)
{
    return tmctl_delMirror_plat(devType, op, if_p);
}

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
tmctl_ret_e tmctl_setPortState(tmctl_devType_e devType,
                                   tmctl_if_t*     if_p,
                                   BOOL            state)
{
#if defined(BCM_PON) || ((defined(CHIP_63158) || defined(CHIP_6813)) && defined(BRCM_OMCI))
    return tmctl_setPortState_plat(devType, if_p, state);
#else
    return TMCTL_UNSUPPORTED;
#endif
}

void tmctl_getPortTmState(tmctl_devType_e     devType,
                                   tmctl_if_t *if_p,
                                   BOOL       *state)
{
   tmctl_getPortTmState_plat(devType, if_p, state);
}

/* ----------------------------------------------------------------------------
 * This function parses JSON configuration file.
 *
 * Parameters:
 *    config_file (IN) path to JSON configuration file.
 *    profile_idx (IN) index of profile (shall be used until next function call)
 * Return:
 *    BOOL TRUE if JSON file exists and valid, FALSE otherwise
 * ----------------------------------------------------------------------------
 */
BOOL tmctl_loadQsizeConfig(const char *config_file, const uint16_t profile_idx)
{
   BOOL rc = FALSE;
   struct json_object *obj;

   errno = 0;
   if (FALSE == file_exist(config_file)) {

      tmctl_error("Failed to open %s file. Reason: %s.", config_file, strerror(errno));
   } else if (NULL == (obj = json_object_from_file(config_file))) {

      tmctl_error("Invalid JSON %s file format.", config_file);
   } else {

      rc = json_parse(&configuration_profiles, &(tmctl_profile_idexer_t){
                                                   .profile_idx = 0,
                                                   .device_idx = 0,
                                                   .speed_idx = 0,
                                                   .priority_idx = 0 }, profile_idx, obj);
   }
   return rc;   
}

int tmctl_getPriorityQueueSize(const tmctl_devType_e dev_type,
                               const uint32_t link_speed, 
                               const uint16_t priority_idx)
{                                  
   uint32_t queue_size = TMCTL_DEFAULT_QUEUE_SIZE;

   /* Trying to read dynamic cofiguration first */
   if ((TMCTL_DEFAULT_QUEUE_SIZE == (queue_size = read_queue_profile_config(&configuration_profiles, 
                                                                           __TMCTL_PE_CFG_PROFILE_ENTRY,
                                                                           dev_type,
                                                                           link_speed, 
                                                                           priority_idx)))) {

       /* Rolling back to defaults */
      if (TMCTL_DEFAULT_QUEUE_SIZE == (queue_size =  read_queue_profile_config(&configuration_profiles, 
                                                                              __TMCTL_PE_DFT_PROFILE_ENTRY,
                                                                              dev_type,
                                                                              link_speed, 
                                                                              priority_idx))) {
         /* Final rollback */                                             
         queue_size = TMCTL_DEF_Q_SIZE_1_MB;
         tmctl_error("[%u][%u][%u]Failed to roll back to defaults, setting default queue size %d", 
                                             __TMCTL_PE_DFT_PROFILE_ENTRY,
                                             link_speed, priority_idx, TMCTL_DEF_Q_SIZE_1_MB);
      }
   }

   tmctl_debug("queue_size_list[%u][%u][%u] = %u",
                                             __TMCTL_PE_DFT_PROFILE_ENTRY,
                                             link_speed,
                                             priority_idx,
                                             queue_size);

   return (int)queue_size;
}

int tmctl_getIfacePriorityQueueSize(tmctl_devType_e  dev_type,
                                    const tmctl_if_t *if_p,
                                    const uint16_t   queue_id,
                                    int              queue_size)
{

   if ((TMCTL_DEFAULT_QUEUE_SIZE == queue_size) && 
       (TMCTL_DEV_XTM != dev_type)) {

         uint32_t link_speed = tmctl_getIfaceLinkSpeed(dev_type, if_p);
         queue_size = tmctl_getPriorityQueueSize(dev_type, link_speed, queue_id);
   }
   return queue_size;
}

uint32_t tmctl_getIfaceLinkSpeed(tmctl_devType_e dev_type, const tmctl_if_t *if_p)
{
   uint32_t link_speed = TMCTL_UNDEFINED_LINK_SPEED;

   if (TMCTL_DEV_SVCQ != dev_type) {

      BOOL is_pon;
      const char *iface;

      tmctl_getDevTypeByIfName_plat(&dev_type, (char **)&iface, if_p->ethIf.ifname);
      is_pon = ((TMCTL_DEV_EPON == dev_type) || (TMCTL_DEV_GPON == dev_type));
      bcm_phy_speed_get(iface, (FALSE == is_pon), &link_speed);

#if (defined(CHIP_63158) || defined(CHIP_63148) || defined(CHIP_63138) || defined(CHIP_4908) || defined(CHIP_47622) || defined(HAS_NONBRCM_SW)) 
      /* devices with crossbar might not have link_speed yet, set to default */
      if (link_speed == TMCTL_UNDEFINED_LINK_SPEED)
         link_speed = SPEED_1000;
      
#endif
   }

   return link_speed;
}
/*********************** P R I V A T E ****************************/

/**
 * @brief 
 * @param profiles 
 * @param p_indexer 
 * @param usr_profile_idx 
 * @param obj 
 * @return BOOL 
 */
static BOOL json_parse(tmctl_profiles_t *profiles, 
                       tmctl_profile_idexer_t *p_indexer, 
                       uint16_t usr_profile_idx, 
                       json_object *obj) 
{

   BOOL rc = TRUE;
   /* Iterate over JSON */
   json_object_object_foreach(obj, key, val) {

      switch (json_object_get_type(val)) {

      case json_type_int: /* Our target case */
         if ((NULL != key) && 
             (NULL != profiles->list[__TMCTL_PE_CFG_PROFILE_ENTRY].link_speeds)) {

            tmctl_link_speed_t *link_speeds = profiles->list[__TMCTL_PE_CFG_PROFILE_ENTRY].link_speeds;
            int tmp_val = json_object_get_int(val);

            if(0 == strcmp(key, JSK_LINK_SPEED)) {
               
               link_speeds[p_indexer->speed_idx].link_speed =  tmp_val;
               tmctl_debug("profiles->list[%u].link_speeds[%u].link_speed %u",
                                 __TMCTL_PE_CFG_PROFILE_ENTRY, 
                                 p_indexer->speed_idx,
                                 link_speeds[p_indexer->speed_idx].link_speed);
            } else if (0 == strcmp(key, JSK_QUEUE_SIZE)) {

               link_speeds[p_indexer->speed_idx].devices_list[p_indexer->device_idx].queue_sizes[p_indexer->priority_idx] = tmp_val;
               tmctl_debug("profiles->list[%u].link_speeds[%u].devices_list[%u].queue_sizes[%u] %u",
                                 __TMCTL_PE_CFG_PROFILE_ENTRY, 
                                 p_indexer->speed_idx,
                                 p_indexer->device_idx,
                                 p_indexer->priority_idx,
                                 link_speeds[p_indexer->speed_idx].devices_list[p_indexer->device_idx].queue_sizes[p_indexer->priority_idx]); 
            } else if (0 == strcmp(key, JSK_DEVICE_TYPE)) {

               link_speeds[p_indexer->speed_idx].devices_list[p_indexer->device_idx].dev_type = tmp_val;
               tmctl_debug("profiles->list[%u].link_speeds[%u].devices_list[%u].dev_type %u",
                                 __TMCTL_PE_CFG_PROFILE_ENTRY, 
                                 p_indexer->speed_idx,
                                 p_indexer->device_idx,
                                 link_speeds[p_indexer->speed_idx].devices_list[p_indexer->device_idx].dev_type);
            }
            /* JSON Validation */
            rc = (0 <= tmp_val); 
         }
      break;
      case json_type_object: 
         obj = json_object_object_get(obj, key);
         rc = json_parse(profiles, p_indexer, usr_profile_idx, obj); 
      break;
      case json_type_array: 
         rc = json_parse_array(profiles, p_indexer, usr_profile_idx, obj, key);
      break;
      default:
         /*Unsupported object*/;
      break;
      }
   } //End of json_object_object_foreach(obj, key, val)

   return rc;
}

/**
 * @brief 
 * @param profiles 
 * @param p_indexer 
 * @param profile_idx 
 * @param obj 
 * @param key 
 * @return BOOL 
 */
static BOOL json_parse_array(tmctl_profiles_t *profiles, 
                             tmctl_profile_idexer_t *p_indexer, 
                             uint16_t profile_idx, 
                             json_object *obj, 
                             char *key)
{

   BOOL rc;
   json_object *js_arr = obj;
   uint32_t js_arr_len, *duty_ptr =  NULL;

   js_arr = json_object_object_get(obj, key); 
   js_arr_len = json_object_array_length(js_arr); 
   /*By checking the key deside which index (out of 4) to promote*/
   if ((TRUE == (rc = allocate_pqueue_sublist_idx(key, js_arr_len, profile_idx, profiles, p_indexer, &duty_ptr))) && 
       (NULL != duty_ptr)) {

      /* Iterate over the array */
      for ( ; (TRUE == rc) && (*duty_ptr < js_arr_len); (*duty_ptr)++) {

         json_object * js_val = json_object_array_get_idx(js_arr, *duty_ptr);
         if (json_object_get_type(js_val) == json_type_array) {
            rc = json_parse_array(profiles, p_indexer, profile_idx, js_val, NULL);
         } else {
            rc = json_parse(profiles, p_indexer, profile_idx, js_val);
         }
      }
   } 

   return rc;  
}

/**
 * @brief 
 * @param type_size 
 * @param length 
 * @param count 
 * @return void* 
 */
static void *alloc_set_data_array(const size_t type_size, 
                                  const size_t length, 
                                  uint16_t *count, 
                                  uint32_t *idx, uint32_t **idx_ptr)
{
   size_t size = type_size * length;
   void *container = malloc(size);

   if (NULL != container) {

     memset(container, 0, size);
     *count = length;
     *idx = 0;
     *idx_ptr = idx;
   }

   return container;
}

/**
 * @brief allocates a respective (as per key) array and sets the duty pointer to the former
 * @param[i] key           one of 4 supported JSON arrays names 
 * @param[i] sublist_len   sublist length
 * @param[i] profile_idx   target profile Id
 * @param[o] profiles      profiles configuration structure
 * @param[i/o] p_indexer   structure holding 4 indices of profile, speed, device types & qsizes arrays 
 * @param[o] duty_ptr      pointer to one of 4 indices being currently processed
 * @return BOOL
 */
static BOOL allocate_pqueue_sublist_idx(const char *key, 
                                       const uint16_t sublist_len, 
                                       const uint16_t profile_idx, 
                                       tmctl_profiles_t *profiles, 
                                       tmctl_profile_idexer_t *p_indexer,
                                       uint32_t **duty_ptr)
{
   BOOL rc = TRUE;

   tmctl_profile_t *profile = (NULL != profiles->list) ? &profiles->list[__TMCTL_PE_CFG_PROFILE_ENTRY] : NULL;

   if (NULL == profile)
      return FALSE;

   if (0 == strcmp(key, JSK_PROFILES_LIST)) {

      /* Boundaries Validation */
      if (sublist_len <= profile_idx) {
         tmctl_error("Profile Id %u does not exist", profile_idx);
         rc = FALSE;
      } else {
         /* Clean dynamic configuration entry, do not touch the default entry allocated in data segment */
         clean_profile(profile);
         /* Set the user defined profile Id ignoring all other profiles set in JSON */
         p_indexer->profile_idx = profile_idx;         
         *duty_ptr = &p_indexer->profile_idx;
      }
   } else if (0 == strcmp(key, JSK_LINK_SPEEDS_LIST)) {
      if ((profile_idx == p_indexer->profile_idx && 
          (NULL != profile) &&
          (NULL == profile->link_speeds))) {
         
         profile->link_speeds = alloc_set_data_array(sizeof(tmctl_link_speed_t), 
                                                      sublist_len, 
                                                      &profile->count, 
                                                      &p_indexer->speed_idx, duty_ptr);
         if (NULL == profile->link_speeds) 
            rc = FALSE;
      }
   } else if (0 == strcmp(key, JSK_DEVICES_QUEUE_SIZES)) {
      if ((profile_idx == p_indexer->profile_idx) &&
          (NULL != profile) &&
          (NULL != profile->link_speeds) &&
          (NULL == profile->link_speeds[p_indexer->speed_idx].devices_list)) {

         tmctl_link_speed_t *link_speed = &profile->link_speeds[p_indexer->speed_idx];
         link_speed->devices_list = alloc_set_data_array(sizeof(tmctl_dev_qsize_t), 
                                                         sublist_len, 
                                                         &link_speed->dev_count, 
                                                         &p_indexer->device_idx, duty_ptr);
         if (NULL == link_speed->devices_list) 
            rc = FALSE;
      }
   } else if (0 == strcmp(key, JSK_PRIORITY_QUEUES_LIST)) {
      if ((profile_idx == p_indexer->profile_idx) && 
          (NULL != profile) &&
          (NULL != profile->link_speeds) &&
          (NULL != profile->link_speeds[p_indexer->speed_idx].devices_list) && 
          (NULL == profile->link_speeds[p_indexer->speed_idx].devices_list[p_indexer->device_idx].queue_sizes)) {

         tmctl_dev_qsize_t *device_list =  &profile->link_speeds[p_indexer->speed_idx].devices_list[p_indexer->device_idx];

         device_list->queue_sizes =  alloc_set_data_array(sizeof(uint32_t), 
                                                         sublist_len, 
                                                         &device_list->count, 
                                                         &p_indexer->priority_idx, duty_ptr);
         if (NULL ==  device_list->queue_sizes) 
            rc = FALSE;
      }
   }

   return rc;
}

/**
 * @brief 
 * @param[i] profiles 
 * @param[i] profile_idx 
 * @param[i] dev_type 
 * @param[i] link_speed 
 * @param[i] priority_idx 
 * @return BOOL 
 */
static uint32_t read_queue_profile_config(const tmctl_profiles_t *profiles, 
                                      const tmctl_profile_entry_t profile_idx,
                                      const tmctl_devType_e dev_type,
                                      const uint32_t link_speed, 
                                      const uint16_t priority_idx)
{
   uint32_t queue_size = TMCTL_DEFAULT_QUEUE_SIZE;
   uint16_t speed_idx, dev_idx;

   if ((NULL == profiles->list) || (0 == profiles->count))
      return TMCTL_DEFAULT_QUEUE_SIZE;

   if ((TMCTL_INVALID_KEY == profile_idx) || /* Boundary validation */
       (profiles->count <= profile_idx))
      return TMCTL_DEFAULT_QUEUE_SIZE;

   if ((NULL == profiles->list[profile_idx].link_speeds) || 
       (0 == profiles->list[profile_idx].count))
      return TMCTL_DEFAULT_QUEUE_SIZE;

   for (speed_idx = 0; (TMCTL_DEFAULT_QUEUE_SIZE == queue_size) && (speed_idx < profiles->list[profile_idx].count); speed_idx++) {

      tmctl_link_speed_t *link_speed_list = &profiles->list[profile_idx].link_speeds[speed_idx];
      if ((link_speed_list->link_speed == link_speed) &&  
          (NULL != link_speed_list->devices_list)) {

         for (dev_idx = 0; (TMCTL_DEFAULT_QUEUE_SIZE == queue_size) && (dev_idx < link_speed_list->dev_count); dev_idx++) {

            if ((dev_type == link_speed_list->devices_list[dev_idx].dev_type) && 
               (0 != link_speed_list->devices_list[dev_idx].count) && 
               (NULL != link_speed_list->devices_list[dev_idx].queue_sizes)) {

               /* Rolling back to default queue size for respective device and link speed */
               if ((priority_idx >= link_speed_list->devices_list[dev_idx].count)) {

                  queue_size = link_speed_list->devices_list[dev_idx].queue_sizes[0];
                  tmctl_debug("[%u][%u][%u]Queue (priority) Id index exceeds link_speeds boundaries, " \
                              " setting queue size to respective link speed for queue Id 0 %u", 
                              profile_idx, link_speed, priority_idx, queue_size);
               } else {

                  queue_size = link_speed_list->devices_list[dev_idx].queue_sizes[priority_idx];
               }
            }
         }
      }
   }

   return queue_size;
}

/**
 * @brief 
 * @param profile 
 */
static void clean_profile(tmctl_profile_t *profile)
{
   uint16_t speed_idx, dev_idx;

   if ((NULL != profile) && (NULL != profile->link_speeds)) {

      for (speed_idx = 0; speed_idx < profile->count; speed_idx++) {

         if (NULL != profile->link_speeds[speed_idx].devices_list) {

            for (dev_idx = 0; dev_idx < profile->link_speeds[speed_idx].dev_count; dev_idx++) {

               free(profile->link_speeds[speed_idx].devices_list[dev_idx].queue_sizes);
               profile->link_speeds[speed_idx].devices_list[dev_idx].queue_sizes = NULL;
            }
            free(profile->link_speeds[speed_idx].devices_list);
            profile->link_speeds[speed_idx].devices_list = NULL;
         }
      }
      free(profile->link_speeds);
      profile->count = 0;
      profile->link_speeds = NULL;
   }
}

/**
 * @brief 
 * @param filename 
 * @return BOOL 
 */
static BOOL file_exist(const char *filename)
{
    struct stat buffer;

    return (0 == stat(filename, &buffer));
}