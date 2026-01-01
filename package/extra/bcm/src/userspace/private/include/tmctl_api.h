/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
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


#ifndef _TMCTL_API_H_
#define _TMCTL_API_H_

/*!\file tmctl_api.h
 * \brief This file contains declarations for QoS related functions.
 *
 */

/* ----------------------------------------------------------------------------
 * Version  Description
 *
 * 1.0      Initial release
 * 1.1      Integrated TMCTL with FAP TM.
 *          Added support for Switch LAG.
 *          Added queue priority field to data structure tmctl_queueCfg_t.
 *          Updated tmctl_portTmInit()  with qcfgFlag.
 *          Added redMaxThreshold field to data structure tmctl_queueDropAlg_t.
 *          Added T-CONT and LLID default queue sizes.
 * 1.2      Added support for Runner WAN queue CIR shaping.
 *          Added port TM initialization config flags.
 * 1.3      Added 4th parameter numQueues to tmctl_portTmInit().
 *          Added dualRate field to data structure tmctl_portTmParms_t.
 *          Added bestEffort field to data structure tmctl_queueCfg_t.
 *          Added policer: tmctl_policerType_e, tmctl_policer_t, tmctl_createPolicer(), tmctl_modifyPolicer(), tmctl_deletePolicer().
 *          Extended TMCTL API to support configure overall rate limit per CPE: tmctl_portMap_t, tmctl_getOverAllShaper(), tmctl_setOverAllShaper(),
 *                  tmctl_linkOverAllShaper(), tmctl_unlinkOverAllShaper().
 * 1.4      Added  minBufs field to tmctl_queueCfg_t.
 * 1.5      Added default & configurable priority queue sizes per iface speed support
 *             i.    Default queue sizes configuration support
 *             ii.   Load & parse queue sizes configuration from JSON file support
 *             iii.  In case JSON configuration is missing / invalid, the system rollbacks to defaults
 *             iv.   Set all TMCTL_DEF_ETH_Q_SZ_DS to -1 to activate the abpve mechanism
 * 1.6      Limited queue_id to be smaller than 1000
 *          Added support for L4S queue via "--l4s"
 *             L4S is refelected to the user as a queue within an egress tm but under the hood, this "l4s queue"
 *             translates into a secondary level egress_tm that contains a couple of queues, CQ and LQ, each with its unique drop algorithm.
 *             Internally, the CQ is associated with the original queue_id and the LQ with the same queue_id plus
 *             an offset. Rate limiter is set only on the "l4s queue". Statistics are retrieved only for the
 *             aggregated "l4s queue". L4S and best effort are mutualy exclusive.
 * 1.7      Default & configurable priority queues refactoring
 *             i.    Removed getDefaultQueueSize
 *             ii.   Removed "api" from tmctl_api_plat.h  name
 *             iii.  Moved analyze_if to rdpa_bdmf and all name related adaptations are kept in a single place
 *             iv.   Replaced all TMCTL_DEF_XXX_Q_SZ with TMCTL_DEFAULT_QUEUE_SIZE
 *             v.    Removed tmctl_getDefQSize
 *             vi.   Removed tmctl_getAutoQSize
 *             vii.  Handled correctly exceptions like old EPON platforms and service queues.
 *             viii. Removed any other leftovers while keeping a single entry point for default queue size i.e. 
 *             ix.   Added error handling when tmctl_getIfacePriorityQueueSize returns TMCTL_DEFAULT_QUEUE_SIZE (-1)
 *             x.    Ignoring TMCTL_DEV_XTM in tmctl_getIfacePriorityQueueSize by returning TMCTL_DEFAULT_QUEUE_SIZE (-1)
 *
 * ----------------------------------------------------------------------------
 */
#define TMCTL_VERSION                  "1.7"

#include "bcmtypes.h"
#include "bcmctl_syslogdefs.h"

//#define HAS_NONBRCM_SW  1

/* #define CC_TMCTL_DEBUG 1 */

#if defined(CC_TMCTL_DEBUG)
#define TMCTL_DEBUGCODE(code)          code
#else
#define TMCTL_DEBUGCODE(code)
#endif /* CC_TMCTL_DEBUG */

#define TMCTL_ERRCODE(code)            code

#define tmctl_debug(fmt, arg...) \
{ \
    TMCTL_DEBUGCODE(printf("%s.%u: " fmt "\n", __FUNCTION__, __LINE__, ##arg); ) \
    BCMCTL_SYSLOGCODE(tmctl, LOG_DEBUG, fmt, ##arg); \
}

#define tmctl_error(fmt, arg...) \
{ \
    TMCTL_ERRCODE(printf("ERROR[%s.%u]: " fmt "\n", __FUNCTION__, __LINE__, ##arg); ) \
    BCMCTL_SYSLOGCODE(tmctl, LOG_ERR, fmt, ##arg); \
}

/*
 * Upper layer is unaware of the real number of queues created at RDPA level,
 * priority is pushed up, so sp queues will be at the top. Note this macro is
 * only applicable to some platforms.
 */
#define tmctl_adapt_prio(devType, if_p, prio_p) \
{ \
    tmctl_portTmParms_t tmParms_p; \
    memset(&tmParms_p, 0x0, sizeof(tmctl_portTmParms_t)); \
    tmctl_getPortTmParms(devType, (tmctl_if_t*)if_p, &tmParms_p); \
    switch (tmParms_p.numQueues) \
    { \
        case 32: \
            *prio_p += 24; \
            break; \
        case 16: \
            *prio_p += 16; \
            break; \
        default: \
            break; \
    } \
}

#define TMCTL_DEFAULT_QUEUE_SIZE       -1
#define TMCTL_UNDEFINED_LINK_SPEED     -1

#define TMCTL_PRIO_MIN                 (0)
#define TMCTL_PRIO_MAX                 (7)

/* Port TM scheduling capability */
#define TMCTL_SP_CAPABLE               0x1
#define TMCTL_WRR_CAPABLE              0x2
#define TMCTL_WDRR_CAPABLE             0x4
#define TMCTL_WFQ_CAPABLE              0x8
#define TMCTL_SP_WRR_CAPABLE           0x10
#define TMCTL_SP_WDRR_CAPABLE          0x20
#define TMCTL_1LEVEL_CAPABLE           0x40

/* Port TM initialization config flags
 * bit 0: indicate whether the default queues for the port will be automatically
 *        configured or not.
 * bits 8-11: indicate the scheduler type to be configured for the port.
 * Other bits: reserved.
 */
#define TMCTL_INIT_DEFAULT_QUEUES      0x00000001
#define TMCTL_QIDPRIO_MAP_Q7P7         0x00000002
#define TMCTL_QIDPRIO_MAP_Q0P7         0x00000004
   
#define TMCTL_SET_DUAL_RATE            0x00000008
   
#define TMCTL_SCHED_TYPE_MASK          0x00000F00
   
#define TMCTL_SCHED_TYPE_SP_WRR        0x00000000
#define TMCTL_SCHED_TYPE_SP            0x00000100
#define TMCTL_SCHED_TYPE_WRR           0x00000200
#define TMCTL_SCHED_TYPE_WDRR          0x00000300
#define TMCTL_SCHED_TYPE_WFQ           0x00000400

#define TMCTL_INVALID_KEY              (-1)
#define TMCTL_INVALID_KEY_U64          (~0ULL)


#define SVCQ_ROOT                      "srvcq"
#define EPON_ROOT                      "epon0"
#define GPON_ROOT                      "gpondef"
#define VEIP_IFC_PREFIX                "veip"
#define EPON_IFC_PREFIX                "epon"

typedef enum
{
   TMCTL_SCHED_INVALID = 0,
   TMCTL_SCHED_SP,
   TMCTL_SCHED_WRR,
   TMCTL_SCHED_WDRR,
   TMCTL_SCHED_WFQ

} tmctl_sched_e;

typedef enum
{
   TMCTL_DROP_DT = 0,
   TMCTL_DROP_RED,
   TMCTL_DROP_WRED,
   TMCTL_DROP_CODEL,
   TMCTL_DROP_PI2,
   TMCTL_DROP_L4S_CQ,
   TMCTL_DROP_L4S_LQ,
   TMCTL_DROP_FLOW_CTRL
} tmctl_dropAlg_e;

typedef enum
{
   TMCTL_ERROR   = -1,
   TMCTL_SUCCESS = 0,
   TMCTL_NOT_FOUND,
   TMCTL_UNSUPPORTED,
   TMCTL_RET_MAX
} tmctl_ret_e;

/* ----------------------------------------------------------------------------
 * tmctl device type.
 * Used for selecting the specific member of the union structure tmctl_if_t.
 * ----------------------------------------------------------------------------
 */
typedef enum
{
   TMCTL_DEV_ETH = 0,
   TMCTL_DEV_EPON,
   TMCTL_DEV_GPON,
   TMCTL_DEV_XTM,
   TMCTL_DEV_SVCQ,
   TMCTL_DEV_NONE,
   TMCTL_DEV_WLAN
} tmctl_devType_e;


/* ----------------------------------------------------------------------------
 * Union structure for tmctl interface/port identification
 * Union member can be selected by tmctl device type.
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   const char* ifname;

} tmctl_ethIf_t;

typedef struct
{
   int llid;

} tmctl_eponIf_t;

typedef struct
{
   int tcontid;

} tmctl_gponIf_t;

typedef struct
{
   const char* ifname;

} tmctl_xtmIf_t;

typedef struct
{
   uint32 portMap;

} tmctl_portMap_t;

typedef union
{
   tmctl_ethIf_t  ethIf;
   tmctl_eponIf_t eponIf;
   tmctl_gponIf_t gponIf;
   tmctl_xtmIf_t  xtmIf;
   tmctl_portMap_t portId;

} tmctl_if_t;

/* ----------------------------------------------------------------------------
 * Structure for port tm parameters
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   uint32_t schedCaps;   /* A bitmap indicating the queue scheduler
                          * capability of this port. Each bit denotes
                          * a scheduling capability defined by constants
                          * TMCTL_SP_CAPABLE, TMCTL_WRR_CAPABLE, etc.
                          */
   int      maxQueues;   /* Max number of queues supported by this port */
   int      maxSpQueues; /* Max number of SP queues allowed by the
                          * queue scheduler when co-exist with other
                          * WRR queues on this port.
                          */
   BOOL     portShaper;  /* Boolean to indicate whether port rate
                          * shaping is supported by this port.
                          */
   BOOL     queueShaper; /* Boolean to indicate whether queue rate
                          * shaping is supported by this port.
                          */
   BOOL     dualRate;    /* Boolean to indicate whether dual rate
                          * is supported by this port.
                          */
   uint32_t cfgFlags;    /* Port TM actual initialization config flags.
                          * See bit definitions above. (dynamic variable)
                          */
   int      numQueues;   /* Port TM actual number of queues set at initialization.
                          * supported only for PON. (dynamic variable)
                          */
   BOOL     dpiQueueExt; /* Boolean to indicate whether DPI queues extention for
                          * best effort queue is supported by this port
                          */
} tmctl_portTmParms_t;

/* ----------------------------------------------------------------------------
 * Structure for shaper configuration parameters
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   int shapingRate;           /* Shaping rate in kbps. 0 implies no shaping, -1 implies not supported. */
   int shapingBurstSize;      /* Shaping burst size in bytes. -1 implies not supported. */
   int minRate;               /* Minimum rate in kbps. 0 implies no shaping, -1 implies not supported. */

} tmctl_shaper_t;

/* ----------------------------------------------------------------------------
 * Structure for queue configuration parameters
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   int            qid;        /* Queue ID. [0..maxQueues-1]. */
   int            priority;   /* Queue priority. [0..highestPriority].
                               * Greater value denotes higher priority level.
                               * i.e. 0 is the lowest priority level.
                               */
   int            qsize;      /* Queue size */
   int            weight;     /* Queue weight. Ignored if SP */
   tmctl_sched_e  schedMode;  /* Queue scheduling mode */
   tmctl_shaper_t shaper;     /* Queue Shaper configuration */
   BOOL           bestEffort; /* Queue is best effort */
   int            minBufs;    /* reserved_packet_buffers */
   BOOL           l4s;        /* Queue is l4s */

} tmctl_queueCfg_t;

/* ----------------------------------------------------------------------------
 * Structure for port queue configurations
 * ----------------------------------------------------------------------------
 */
#define MAX_TMCTL_QUEUES_BASELINE   8
#define MAX_TMCTL_QUEUES_EXTENDED   32

typedef struct
{
   int              numQueues;   /* Number of queues configured */
   tmctl_queueCfg_t qcfg[MAX_TMCTL_QUEUES_BASELINE];

} tmctl_portQcfg_t;

/* ----------------------------------------------------------------------------
 * Structure for queue profile configuration
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   int             dropProb; /* 0 to 100 */
   int             minThreshold;
   int             maxThreshold;

} tmctl_queueProfile_t;

/* ----------------------------------------------------------------------------
 * Structure for queue drop algorithm configuration
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   int redMinThreshold;
   int redMaxThreshold;
   int redPercentage;

} tmctl_queueDropAlgExt_t;

typedef struct
{
   tmctl_dropAlg_e dropAlgorithm; /* DT, RED, WRED */
   uint32_t        priorityMask0;
   uint32_t        priorityMask1;
   tmctl_queueDropAlgExt_t dropAlgLo;
   tmctl_queueDropAlgExt_t dropAlgHi;

} tmctl_queueDropAlg_t;


/* ----------------------------------------------------------------------------
 * Structure for queue flow control configuration
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   uint8_t enable;
   int pauseThr;
   int hystThr;
} tmctl_queueFlowCtrl_t;

/* ----------------------------------------------------------------------------
 * Structure for queue statistics
 * ----------------------------------------------------------------------------
 */
typedef struct
{
   uint32_t txPackets;
   uint32_t txBytes;
   uint32_t droppedPackets;
   uint32_t droppedBytes;

} tmctl_queueStats_t;


/* ----------------------------------------------------------------------------
 * Data structure for dscp to pbit feature
 * ----------------------------------------------------------------------------
 */

#define MAX_PBIT_VALUE   7
#define TOTAL_PBIT_NUM   8
#define TOTAL_TC_NUM     8
#define TOTAL_DSCP_NUM   64
#define U32_MAX         ((uint32_t)~0U)

typedef struct
{
    uint32_t dscp[TOTAL_DSCP_NUM];
    tmctl_devType_e devType;
    tmctl_if_t* if_p;
    BOOL remark;
} tmctl_dscpToPbitCfg_t;


typedef struct
{
    uint32_t pbit[TOTAL_PBIT_NUM];
} tmctl_pbitToQCfg_t;


typedef struct
{
    int32_t tc[TOTAL_TC_NUM];
} tmctl_tcToQCfg_t;


typedef enum
{
    TMCTL_QOS_FC = 0,
    TMCTL_QOS_IC,
    TMCTL_QOS_MCAST,
    TMCTL_QOS_MAX
} tmctl_qosType_e;


typedef enum
{
    TMCTL_DIR_DN = 0,
    TMCTL_DIR_UP,
    TMCTL_DIR_MAX
} tmctl_dir_e;

/* ----------------------------------------------------------------------------
 * Structure for policer configuration parameters
 * ----------------------------------------------------------------------------
 */

/* Traffic policer type */
typedef uint64_t tmctl_policerRate_t;

typedef enum {
    TMCTL_POLICER_SINGLE_TOKEN_BUCKET,        /**< Single token bucket */
    TMCTL_POLICER_SINGLE_RATE_TCM,	          /**< Single Rate Three Color Marker (srTCM)(RFC2697) */
    TMCTL_POLICER_TWO_RATE_TCM,               /**< Two Rate Three Color Marker (trTCM)(RFC2698) */
    TMCTL_POLICER_TWO_RATE_WITH_OVERFLOW,     /**< Two Rate with overflowW */
} tmctl_policerType_e;

typedef struct {
    int policerId;                        /**< TMCTL_INVALID_KEY means system will allocate a policerId */
    tmctl_policerType_e type;             /**< Policer type */
    tmctl_policerRate_t cir;              /**< Committed Information Rate (CIR) - bps (100K-1G/10G) */
    uint32_t cbs;                         /**< Committed Burst Size (CBS) - bytes (1K-100M) */
    uint32_t ebs;                         /**< Excess Burstsize (EBS) - bytes, used for srTCM */
    tmctl_policerRate_t pir;              /**< PEAK Information Rate (PIR) - bps (100K-1G/10G) */
    uint32_t pbs;                         /**< PEAK Burst Size (PBS) - bytes (1K-100M) */
    uint8_t dei_mode;                     /**< DEI remark enabled - Used for dual bucket only, affects outer dei only */
    uint8_t color_aware_enabled;          /**< Color aware enabled - Change policer coloring when DEI is set in received packets */ 
    int8_t rl_overhead;                   /**< Policer overhead to be added for every packet */
} tmctl_policer_t;

/* ----------------------------------------------------------------------------
 * Structure for mirror configuration parameters
 * ----------------------------------------------------------------------------
 */
typedef enum {
    TMCTL_MIRROR_RX_GET,
    TMCTL_MIRROR_RX_CLR,
    TMCTL_MIRROR_RX_ADD,
    TMCTL_MIRROR_RX_DEL,
    TMCTL_MIRROR_TX_GET,
    TMCTL_MIRROR_TX_CLR,
    TMCTL_MIRROR_TX_ADD,
    TMCTL_MIRROR_TX_DEL,
} tmctl_mirror_op_e;

typedef struct {
    tmctl_mirror_op_e op;
    union {
        uint64_t srcMap;
        int srcPort;                   /* for addmirror op only */
    };
    int destPort;
    int unit;                          /* for sf2 based */
} tmctl_mirror_t;

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
BOOL    tmctl_loadQsizeConfig     (const char *config_file, const uint16_t profile_idx);

/**
 * @brief This function returns queue size as per particular link_speed & priority_idx
 * @param[i] dev_type      network interface device type
 * @param[i] link_speed    network interface link speed.
 * @param[i] priority_idx  priority queue Id
 * @return int priority_idx queue size or TMCTL_DEF_Q_SIZE_1_MB if failed
 */
int tmctl_getPriorityQueueSize  (const tmctl_devType_e dev_type,
                                 const uint32_t link_speed, 
                                 const uint16_t priority_idx);

/**
 * @brief This function returns queue size for particular network interface
 * @note  The function will check the configuration and modify the queue_size
 *        accordingly only if the queue_size value is set to TMCTL_DEFAULT_QUEUE_SIZE.
 * @param[i] dev_type   network interface type
 * @param[i] if_p       network interface structure
 * @param[i] queue_id   egress queue Id
 * @param[i] queue_size original queue size
 * @return int 
 *         i.   configuration based queue size as per network interface and queue id if original [i] queue_size is TMCTL_DEFAULT_QUEUE_SIZE (-1),
 *         ii.   -1 if network interface capabilities can not be retrieved,
 *         iii. original [i] queue_size otherwise.
 */
int tmctl_getIfacePriorityQueueSize(tmctl_devType_e  dev_type,
                                    const tmctl_if_t *if_p,
                                    const uint16_t   queue_id,
                                    int              queue_size);

/**
 * @brief This function returns network intrface link capability
 * @note TMCTL_SVQC device is not supported here
 * @param dev_type 
 * @param if_p 
 * @return uint32_t speed capability for all interfaces besides TMCTL_DEV_XTM & TMCTL_SVQC, TMCTL_UNDEFINED_LINK_SPEED  for TMCTL_SVQC
 */
uint32_t tmctl_getIfaceLinkSpeed(tmctl_devType_e dev_type, const tmctl_if_t *if_p);

/* ----------------------------------------------------------------------------
 * This function initializes the basic TM settings for a port based on its
 * TM capability.
 *
 * Note that if the port had already been initialized, all its existing
 * configuration will be deleted before re-initialization.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p (IN) Port identifier.
 *    cfgFlags (IN) Port TM initialization flags. See bit definitions above.
 *    numQueues (IN) Number of queues to be set for TM.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_portTmInit(tmctl_devType_e devType,
                             tmctl_if_t*     if_p,
                             uint32_t        cfgFlags,
                             int             numQueues);


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
                               tmctl_if_t*     if_p);


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
                              tmctl_queueCfg_t* qcfg_p);


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
 *                - For 63138, 63148, 63158 or 63178 TMCTL_DEV_ETH device type,
 *                  -- the priority of SP queue must be set to qid.
 *                  -- the priority of WRR/WDRR/WFQ queue must be set to 0.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueCfg(tmctl_devType_e   devType,
                              tmctl_if_t*       if_p,
                              tmctl_queueCfg_t* qcfg_p);


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
                              int             queueId);


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
                                tmctl_shaper_t* shaper_p);


/* ----------------------------------------------------------------------------
 * This function configures the port rx rate for shaping rate, shaping burst
 * size and minimum rate. If port shaping is to be done by the external
 * Switch, the corresponding Switch port rx rate will be configured.
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
                                tmctl_shaper_t* shaper_p);

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
                                tmctl_shaper_t* shaper_p);


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
                                tmctl_shaper_t* shaper_p);

/* ----------------------------------------------------------------------------
 * This function gets the overall shaper configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p     (OUT) the portmap which linked to overall_rl tm
 *    shaper_p (OUT) The shaper parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getOverAllShaper(tmctl_devType_e devType,
                                tmctl_if_t*     if_p,
                                tmctl_shaper_t* shaper_p);


/* ----------------------------------------------------------------------------
 * This function configures the overall shaper for shaping rate, shaping burst
 * size and minimum rate.
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
                                tmctl_shaper_t* shaper_p);


/* ----------------------------------------------------------------------------
 * This function link the port to the overall shaper configuration.
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
                                tmctl_if_t*     if_p);

/* ----------------------------------------------------------------------------
 * This function unlink the port from the overall shaper configuration.
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
                                tmctl_if_t*     if_p);


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
                                  tmctl_queueDropAlg_t* dropAlg_p);


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
                                  tmctl_queueDropAlg_t* dropAlg_p);

#define tmctl_setQueueDropAlgExt tmctl_setQueueDropAlg


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
                                       tmctl_queueDropAlg_t* dropAlg_p);


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
                                       tmctl_queueDropAlg_t* dropAlg_p);


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
                                   tmctl_queueFlowCtrl_t* flowCtrl_p);



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
                                   tmctl_queueFlowCtrl_t* flowCtrl_p);


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
                                tmctl_queueStats_t* stats_p);


/* ----------------------------------------------------------------------------
 * This function gets port TM parameters (capabilities) from the device driver.
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
                                 tmctl_portTmParms_t* tmParms_p);


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
tmctl_ret_e tmctl_getDscpToPbit(tmctl_dscpToPbitCfg_t* cfg_p);


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
tmctl_ret_e tmctl_setDscpToPbit(tmctl_dscpToPbitCfg_t* cfg_p);


tmctl_ret_e tmctl_setTcToQueue(int table_index, tmctl_tcToQCfg_t* cfg_p);


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
                                 tmctl_pbitToQCfg_t* cfg_p);


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
                                 tmctl_pbitToQCfg_t* cfg_p);


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
tmctl_ret_e tmctl_getForceDscpToPbit(tmctl_dir_e dir, BOOL* enable_p);


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
tmctl_ret_e tmctl_setForceDscpToPbit(tmctl_dir_e dir, BOOL* enable_p);


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
                                  BOOL* enable_p);


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
                                  BOOL* enable_p);
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
                                     tmctl_if_t* if_p,
                                     int queueId,
                                     int size);

/* ----------------------------------------------------------------------------
 * This function sets shaper of a queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    shaper    (IN)  Queue Shaper configuration
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_setQueueShaper(tmctl_devType_e devType,
                                     tmctl_if_t* if_p,
                                     int queueId,
                                     tmctl_shaper_t *shaper_p);

/* ----------------------------------------------------------------------------
 * This function create shaper for multi queues.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    shaper  (IN)  Queue Shaper configuration
 *    shaper_obj [OUT] the address for object
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */

tmctl_ret_e tmctl_CreateShaper(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     tmctl_shaper_t     *shaper_p,
                                     uint64_t *shaper_obj);

/* ----------------------------------------------------------------------------
 * This function delete shaper for multi queues.
 *
 * Parameters:
 *    shaper_obj [IN] the address for object
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */

tmctl_ret_e tmctl_DeleteShaper(uint64_t shaper_obj);


/* ----------------------------------------------------------------------------
 * This function set shaper for multi queues for 1 queue.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    queueId (IN) Queue ID.
 *    shaper_obj [IN] the address for object - NULL to delete shaper from queue
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */

tmctl_ret_e tmctl_SetShaperQueue(tmctl_devType_e    devType,
                                     tmctl_if_t*        if_p,
                                     int queueId,
                                     uint64_t shaper_obj);

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
tmctl_ret_e tmctl_createPolicer(tmctl_policer_t *policer_p);

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
tmctl_ret_e tmctl_modifyPolicer(tmctl_policer_t *policer_p);

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
tmctl_ret_e tmctl_deletePolicer(int policerId);

/* ----------------------------------------------------------------------------
 * This function gets the rx/tx mirror configuration.
 *
 * Parameters:
 *    op (IN)  RX/TX parameter.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_getMirror(tmctl_mirror_op_e op);

/* ----------------------------------------------------------------------------
 * This function clears the rx/tx mirror configuration.
 *
 * Parameters:
 *    mirror_p (IN) The mirror parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_clrMirror(tmctl_mirror_op_e op);

/* ----------------------------------------------------------------------------
 * This function adds the rx/tx mirror configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    destIf  (IN) dest port identifier.
 *    mirror_p (IN) The mirror parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_addMirror(tmctl_devType_e        devType,
                                tmctl_if_t*         if_p,
                                tmctl_mirror_op_e   op,
                                tmctl_ethIf_t       destIf,
                                unsigned int        truncate);

/* ----------------------------------------------------------------------------
 * This function dels the rx/tx mirror configuration.
 *
 * Parameters:
 *    devType (IN) tmctl device type.
 *    if_p    (IN) Port identifier.
 *    mirror_p (IN) The mirror parameters.
 *
 * Return:
 *    tmctl_ret_e enum value
 * ----------------------------------------------------------------------------
 */
tmctl_ret_e tmctl_delMirror(tmctl_devType_e        devType,
                                tmctl_mirror_op_e   op,
                                tmctl_if_t*         if_p);

/* ----------------------------------------------------------------------------
 * This function sets port enable/disable state.
 * In GPON OMCI stack should be capable to block entire upstream and downstream
 * traffic at WAN interface. While DS traffic can be blocked at GEM ports level,
 * the upstream traffic can be stopped by disabling egress queues e.g. by
 * disabling the T-CONT(s). So the capability to disable port has been added
 * to TMCtl; meanwhile implemented only for devType == TMCTL_DEV_GPON
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
                                   BOOL            state);

/* ----------------------------------------------------------------------------
 * This function gets port tm  state.
 *
 * Parameters:
 *    devType         (IN) tmctl device type.
 *    if_p            (IN) Port identifier.
 *    state           (OUT) Port has TM configured (TRUE) else  (FALSE).
 *
 * Return:
 *    None
 * ----------------------------------------------------------------------------
 */
void tmctl_getPortTmState(tmctl_devType_e devType,
                              tmctl_if_t*     if_p,
                              BOOL            *state);

#if defined(BCMCTL_SYSLOG_SUPPORTED)
DECLARE_setSyslogLevel(tmctl);
DECLARE_getSyslogLevel(tmctl);
DECALRE_isSyslogLevelEnabled(tmctl);
DECLARE_setSyslogMode(tmctl);
DECLARE_isSyslogEnabled(tmctl);
#endif /* BCMCTL_SYSLOG_SUPPORTED */


#endif /* _TMCTL_API_H_ */
