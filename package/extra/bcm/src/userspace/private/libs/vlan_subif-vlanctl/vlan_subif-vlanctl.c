/*
* <:copyright-BRCM:2011:proprietary:standard
*
*    Copyright (c) 2011 Broadcom
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
:>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include <os_defs.h>
#include <vlanctl_api.h>
#include <vlan_subif.h>

#ifdef VLANSUBIF_DEBUG
static vlansubif_logLevel_t vlansubif_logLevel = VLANSUBIF_LOG_LEVEL_DEBUG;
#else
static vlansubif_logLevel_t vlansubif_logLevel = VLANSUBIF_LOG_LEVEL_ERROR;
#endif

/*
 * Local variables
 */

static int is_initialized;
#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
#define DEFAULT_STDOUT_FILE     "/dev/tty"
#endif

#if defined(DMP_X_BROADCOM_COM_EPONWAN_1)
#define PSP_KEY_VALUE            "/tmp/pspValue"
#define RDPA_ONU_TYPE_PSP_KEY    "OnuType"

static int sfuModeCheck = -1;

static char *getPspValue(char *pspKey)
{
    static char keyValue[BUFLEN_8] = {0};
    char cmdstr[BUFLEN_64] = {0};
    FILE* file = NULL;

    sprintf(cmdstr, "pspctl dump %s ", pspKey);

    file = popen(cmdstr, "r");
    if (file != NULL)
    {
        if (fscanf(file, "%7s", keyValue) != 1)
        {
            printf("fscanf error!!");
        }

        pclose(file);
    }

    return keyValue;
}

static int isSfuMode(void)
{
#if defined(EPON_HGU)
    if (sfuModeCheck < 0)
    {
        sfuModeCheck = 0;
        if ((strcasecmp(getPspValue(RDPA_ONU_TYPE_PSP_KEY), "sfu") == 0))
        {
            sfuModeCheck = 1;
        }
    }

#else
    sfuModeCheck = 1;
#endif

    return (sfuModeCheck == 1)? 1:0;
}
#endif

/*
 * Private functions
 */

#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
static void vlanSubif_logRedirect(const char *filename)
{
    int fd;
    int flags = O_CREAT | O_APPEND | O_WRONLY | O_NONBLOCK;
    if (filename == NULL)
    {
        filename = DEFAULT_STDOUT_FILE;
        flags = O_WRONLY;
    }
    fd = open(filename, flags);
    if (fd >= 0 && fd != STDOUT_FILENO)
    {
        close(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}
#endif

/*
 * Public functions
 */

/*
 * VLAN subif APIs
 */

int vlanSubif_init(void)
{
    int ret = 0;
    if (!is_initialized)
    {
#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
        vlanSubif_logRedirect(VLAN_SUBIF_LOG_REDIRECT_TO_FILE);
#endif
        if (vlansubif_logLevel == VLANSUBIF_LOG_LEVEL_DEBUG)
            vlanCtl_setLogLevel(VLANCTL_LOG_LEVEL_DEBUG);

        ret = vlanCtl_init();
        if (!ret)
        {
            vlanCtl_setIfSuffix(".");
            is_initialized = 1;
        }
        else
        {
            VLANSUBIF_LOG_ERROR("vlanCtl_init() returned error %d\n", ret);
            vlanSubif_cleanup();
        }
    }
    return ret;
}

void vlanSubif_cleanup(void)
{
    vlanCtl_cleanup();
    is_initialized = 0;
#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
    vlanSubif_logRedirect(NULL);
#endif
}

static int vlanSubif_InterfaceExists(const char *name)
{
    struct ifaddrs *ifaddr, *ifa;
    int ret = 0;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return 0;
    }

    /* Walk through linked list, maintaining head pointer so we
        can free list later */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL ||
            ifa->ifa_addr->sa_family != AF_PACKET)
            continue;
        if (!strcmp(ifa->ifa_name, name))
        {
            ret = 1;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ret;
}

int vlanSubif_createVlanInterface(const char *lowerDevName, signed int vlanId,
    const vlansubif_options_t *options)
{
    char vlanIface[BUFLEN_32];
    char *vlanIfaceName;
    int was_initialized = is_initialized;
    UINT32 tagRuleId = VLANCTL_DONT_CARE;
    unsigned int tpidTable[BCM_VLAN_MAX_TPID_VALUES];
    vlanCtl_ruleInsertPosition_t position;
    int is_tagged = (vlanId >= 0);
    int is_routed = (options != NULL && options->is_routed);
    int is_multicast_disabled = (options != NULL && options->is_multicast_disabled);
    int is_ok_if_exists = (options != NULL && options->is_ok_if_exists);
    int is_phyif_other_owned = (options != NULL && options->is_phyif_other_owned);
    int pbits = 0;
    int ret;

    if (options != NULL)
    {
        pbits = options->pbit;
    }

    ret = vlanSubif_init();
    if (ret)
        return ret;

    if (options != NULL && options->name != NULL)
    {
        vlanIfaceName = (char *)(long)options->name;
    }
    else
    {
        vlanSubif_IntfName(lowerDevName, vlanId, vlanIface, sizeof(vlanIface));
        vlanIfaceName = vlanIface;
    }

    if (is_ok_if_exists && vlanSubif_InterfaceExists(vlanIfaceName))
    {
        vlanCtl_removeAllTagRule(vlanIfaceName);
    }
    else
    {
        ret = vlanCtl_createVlanInterfaceByName((char *)(long)lowerDevName,
            vlanIfaceName, is_routed, !is_multicast_disabled);
    }

    if (!is_phyif_other_owned)
    {
        ret = ret ? ret : vlanCtl_setRealDevMode((char *)(long)lowerDevName, BCM_VLAN_MODE_RG);
    }
    else
    {
        VLANSUBIF_LOG_DEBUG("vlanCtl_setRealDevMode(RG) skipped, lowerDevName=%s, vlanId=%d\n",
          vlanIfaceName, vlanId);
    }

    ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_TX, 0, VLANCTL_ACTION_DROP, NULL);
    ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_TX, 1, VLANCTL_ACTION_DROP, NULL);
    ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_TX, 2, VLANCTL_ACTION_DROP, NULL);

#if defined(DMP_X_BROADCOM_COM_EPONWAN_1)
    if (isSfuMode())
    {
        ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_RX, 0,
            VLANCTL_ACTION_ACCEPT, vlanIfaceName);
        ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_RX, 1,
            VLANCTL_ACTION_ACCEPT, vlanIfaceName);
        ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_RX, 2,
            VLANCTL_ACTION_ACCEPT, vlanIfaceName);
        if ((!is_tagged) && (strcmp(lowerDevName, "epon0")))
        {
            /* Allow local out packet on non-epon0 dev */
            vlanCtl_initTagRule();
            ret = ret ? ret : vlanCtl_filterOnRxRealDevice("lo");
            ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_TX, 0, VLANCTL_POSITION_APPEND,
                                VLANCTL_DONT_CARE, &tagRuleId);
        }
        goto done;
    }
#endif

    if (is_tagged)
    {
        // setting TPID table of veip0 to support TPID as either 0x8100, 0x88a8, or 0x9100
        // when vlan interface such as veip0.1 is created on top of veip0.
        tpidTable[0] = 0x8100;   // Q_TAG_TPID
        tpidTable[1] = 0x8100;   // C_TAG_TPID
        tpidTable[2] = 0x88A8;   // S_TAG_TPID
        tpidTable[3] = 0x9100;   // D_TAG_TPID
        ret = ret ? ret : vlanCtl_setTpidTable((char *)(long)lowerDevName, tpidTable);
    }

    if (!is_phyif_other_owned)
    {
        ret = ret ? ret : vlanCtl_setRealDevMode((char *)(long)lowerDevName, BCM_VLAN_MODE_RG);
    }
    else
    {
        VLANSUBIF_LOG_DEBUG("vlanCtl_setRealDevMode(RG) skipped, lowerDevName=%s, vlanId=%d\n",
          vlanIfaceName, vlanId);
    }

    ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_TX, 0, VLANCTL_ACTION_DROP, NULL);
    ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_TX, 1, VLANCTL_ACTION_DROP, NULL);
    ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_TX, 2, VLANCTL_ACTION_DROP, NULL);

#if defined(DMP_X_BROADCOM_COM_GPONWAN_1)
    if (is_routed)
    {
        ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_RX, 0, VLANCTL_ACTION_DROP, NULL);
        ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_RX, 1, VLANCTL_ACTION_DROP, NULL);
        ret = ret ? ret : vlanCtl_setDefaultAction(lowerDevName, VLANCTL_DIRECTION_RX, 2, VLANCTL_ACTION_DROP, NULL);
    }
#endif

    /* ======== Set rx rules ======== */
    position = is_routed ? VLANCTL_POSITION_BEFORE : VLANCTL_POSITION_APPEND;

#if !defined(DMP_X_BROADCOM_COM_GPONWAN_1)
    if (is_routed && is_tagged)
    {
        /* Filter on the Rx interface and not allow for multicast */
        vlanCtl_initTagRule();
        ret = ret ? ret : vlanCtl_setReceiveVlanDevice(vlanIfaceName);
        vlanCtl_filterOnVlanDeviceMacAddr(0);
        ret = ret ? ret : vlanCtl_cmdDropFrame();
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
            1, position, VLANCTL_DONT_CARE, &tagRuleId);
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
            2, position, VLANCTL_DONT_CARE, &tagRuleId);
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
            0, position, VLANCTL_DONT_CARE, &tagRuleId);
    }
#endif

    vlanCtl_initTagRule();
    ret = ret ? ret : vlanCtl_setReceiveVlanDevice(vlanIfaceName);
    if (is_tagged)
    {
        /* Note: Always set bridge interface rx rules at the bottom of the tables
         * using VLANCTL_POSITION_APPEND. Always set route interface rx rules at
         * the top of the tables using VLANCTL_POSITION_BEFORE.
         */
        ret = ret ? ret : vlanCtl_filterOnTagVid(vlanId, 0);
        if (is_routed && !is_multicast_disabled)
            vlanCtl_filterOnVlanDeviceMacAddr(1);
        ret = ret ? ret : vlanCtl_cmdPopVlanTag();
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
            1, position, VLANCTL_DONT_CARE, &tagRuleId);
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
            2, position, VLANCTL_DONT_CARE, &tagRuleId);
    }
    else
    {
        if (is_routed)
        {
            /* Filter on the Rx interface and not allow for multicast */
            vlanCtl_initTagRule();
            ret = ret ? ret : vlanCtl_setReceiveVlanDevice(vlanIfaceName);
            vlanCtl_filterOnVlanDeviceMacAddr(0);
            ret = ret ? ret : vlanCtl_cmdDropFrame();
            ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
                1, position, VLANCTL_DONT_CARE, &tagRuleId);
            ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
                2, position, VLANCTL_DONT_CARE, &tagRuleId);
            if (!is_multicast_disabled)
            {
                vlanCtl_initTagRule();
                ret = ret ? ret : vlanCtl_setReceiveVlanDevice(vlanIfaceName);
                vlanCtl_filterOnVlanDeviceMacAddr(1);
                ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
                    0, position, VLANCTL_DONT_CARE, &tagRuleId);
            }
        }
        if (!is_routed && (options == NULL || !options->num_etype_filters))
        {
            /* Unconditionally, forward frames to the rx vlan interface */
            ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
                0, VLANCTL_POSITION_LAST, VLANCTL_DONT_CARE, &tagRuleId);
            ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
                1, VLANCTL_POSITION_LAST, VLANCTL_DONT_CARE, &tagRuleId);
            ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX,
                2, VLANCTL_POSITION_LAST, VLANCTL_DONT_CARE, &tagRuleId);
        }
    }
    if (options && options->num_etype_filters)
    {
        int filt;
        for (filt = 0; filt < options->num_etype_filters && !ret; filt++)
        {
            vlanCtl_initTagRule();
            ret = ret ? ret : vlanCtl_setReceiveVlanDevice(vlanIfaceName);
            vlanCtl_filterOnEthertype(options->etype_filter[filt]);
            /* Set rule to the top of rx tag rule table-0. */
            vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX, 0, VLANCTL_POSITION_APPEND,
                VLANCTL_DONT_CARE, &tagRuleId);
            if (is_tagged)
            {
                vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX, 1, VLANCTL_POSITION_APPEND,
                    VLANCTL_DONT_CARE, &tagRuleId);
                vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_RX, 2, VLANCTL_POSITION_APPEND,
                    VLANCTL_DONT_CARE, &tagRuleId);
            }
        }
    }

    /* ======== Set tx rules ======== */
    /* Tag outgoing pkts with vid=vlanId */
    vlanCtl_initTagRule();
    ret = ret ? ret : vlanCtl_filterOnTxVlanDevice(vlanIfaceName);
    if (is_tagged)
    {
        ret = ret ? ret : vlanCtl_cmdPushVlanTag();
        ret = ret ? ret : vlanCtl_cmdSetTagVid(vlanId, 0);
        ret = ret ? ret : vlanCtl_cmdSetTagPbits(pbits, 0);
        if (options && options->tpid)
            ret = ret ? ret : vlanCtl_cmdSetEtherType(options->tpid);
    }
    ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_TX,
        0, VLANCTL_POSITION_BEFORE, VLANCTL_DONT_CARE, &tagRuleId);
    if (!is_routed)
    {
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_TX,
            1, VLANCTL_POSITION_BEFORE, VLANCTL_DONT_CARE, &tagRuleId);
        ret = ret ? ret : vlanCtl_insertTagRule(lowerDevName, VLANCTL_DIRECTION_TX,
            2, VLANCTL_POSITION_BEFORE, VLANCTL_DONT_CARE, &tagRuleId);
    }

#if defined(DMP_X_BROADCOM_COM_EPONWAN_1)
done:
#endif
    if (!was_initialized)
        vlanSubif_cleanup();

    if (ret)
    {
        VLANSUBIF_LOG_ERROR("Creating interface %s failed with error %d\n", vlanIfaceName, ret);
    }
    else
    {
        VLANSUBIF_LOG_DEBUG("Interface %s created: vid=%d pbits=%u routed=%d mc=%d tpid=0x%x et=%d:0x%x 0x%x\n",
            vlanIfaceName, vlanId, pbits, is_routed, !is_multicast_disabled,
            options ? options->tpid : 0,
            options ? options->num_etype_filters : 0,
            options ? options->etype_filter[0] : 0,
            options ? options->etype_filter[1] : 0);
    }

    return ret;
}

int vlanSubif_deleteVlanInterface(const char *vlanDevName)
{
    int was_initialized = is_initialized;

    int ret = vlanSubif_init();
    if (ret)
        return ret;

    ret = vlanCtl_deleteVlanInterface((char *)(long)vlanDevName);
    if (!was_initialized)
        vlanSubif_cleanup();

    if (ret)
    {
        VLANSUBIF_LOG_ERROR("Deleting interface %s failed with error %d\n", vlanDevName, ret);
    }
    else
    {
        VLANSUBIF_LOG_DEBUG("Interface %s deleted successfully\n", vlanDevName);
    }
    return ret;
}

int vlanSubif_addQosVlanTagRule(const char *vlanDevName, const vlansubif_qos_options_t *options)
{
    unsigned int ruleId = VLANCTL_DONT_CARE;

    if(!vlanDevName || vlanDevName[0] == 0)
        return -1;

    if(!options || !options->tx_if_name|| options->tx_if_name[0] == 0)
        return -1;

    /* setup vlanctl rules */
    vlanCtl_init();

    /* Non-vlan packets egress to an untagged vlanmux interface shall be tagged with VID 0 and the class rule p-bits;
    	Non-vlan packets egress to a tagged vlanmux interface shall be tagged with the interface VID and the class rule p-bits; */
    vlanCtl_initTagRule();
    vlanCtl_filterOnTxVlanDevice(options->tx_if_name);
    vlanCtl_filterOnSkbMarkFlowId(options->flow_id);
    vlanCtl_cmdPushVlanTag();
    vlanCtl_cmdSetTagVid(options->vid == -1 ? 0 : options->vid, 0);
    vlanCtl_cmdSetTagPbits(options->pbit, 0);
    vlanCtl_insertTagRule(vlanDevName, VLANCTL_DIRECTION_TX, 0, VLANCTL_POSITION_BEFORE,
    				   VLANCTL_DONT_CARE, &ruleId);

    /* Vlan packets egress to an untagged vlanmux interface shall have the packet p-bits re-marked by the class rule p-bits. No additional vlan tag is added;
    	Vlan packets egress to a tagged vlanmux interface shall be additionally tagged with the packet VID, and the class rule p-bits. */
    vlanCtl_initTagRule();
    vlanCtl_filterOnTxVlanDevice(options->tx_if_name);
    vlanCtl_filterOnSkbMarkFlowId(options->flow_id);
    if (options->vid!= -1)
    {
       vlanCtl_cmdPushVlanTag();
       vlanCtl_cmdCopyTagVid(1, 0);
    }
    vlanCtl_cmdSetTagPbits(options->pbit, 0);
    vlanCtl_insertTagRule(vlanDevName, VLANCTL_DIRECTION_TX, 1, VLANCTL_POSITION_BEFORE,
                            VLANCTL_DONT_CARE, &ruleId);

    vlanCtl_cleanup();

    return 0;
}

int vlanSubif_delQosVlanTagRule(const char *vlanDevName, const vlansubif_qos_options_t *options)
{
    if(!vlanDevName || vlanDevName[0] == 0)
        return -1;

    if(!options || !options->tx_if_name|| options->tx_if_name[0] == 0)
        return -1;

    /* cleanup vlanctl rules */ 		
    vlanCtl_init();
    vlanCtl_initTagRule();
    vlanCtl_filterOnTxVlanDevice(options->tx_if_name);
    vlanCtl_filterOnSkbMarkFlowId(options->flow_id);
    vlanCtl_removeTagRuleByFilter((char *) vlanDevName, VLANCTL_DIRECTION_TX, 0);
    vlanCtl_removeTagRuleByFilter((char *) vlanDevName, VLANCTL_DIRECTION_TX, 1);
    vlanCtl_cleanup();

    return 0;
}

const char *vlanSubif_IntfName(const char *lowerDevName, signed int vlanId, char *intf_name,
    size_t max_intf_name_size)
{
    if (vlanId < 0)
        vlanId = 0;
    intf_name[max_intf_name_size - 1] = 0;
    snprintf(intf_name, max_intf_name_size - 1, "%s.%u", lowerDevName, vlanId);
    return intf_name;
}

 /*
  * Log control
  */

void vlanSubif_log(vlansubif_logLevel_t level, const char *fmt, ...)
{
    va_list args;
    if (level > vlansubif_logLevel)
        return;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

int vlansubif_setLogLevel(vlansubif_logLevel_t logLevel)
{
    if(logLevel > VLANSUBIF_LOG_LEVEL_MAX)
    {
        VLANSUBIF_LOG_ERROR("Invalid Log Level: %d (allowed values are 0 to %d)", logLevel, VLANSUBIF_LOG_LEVEL_MAX-1);
        return -EINVAL;
    }
    vlansubif_logLevel = logLevel;
    return 0;
}

vlansubif_logLevel_t vlansubif_getLogLevel(void)
{
    return vlansubif_logLevel;
}

int vlansubif_logLevelIsEnabled(vlansubif_logLevel_t logLevel)
{
    return (vlansubif_logLevel >= logLevel);
}
