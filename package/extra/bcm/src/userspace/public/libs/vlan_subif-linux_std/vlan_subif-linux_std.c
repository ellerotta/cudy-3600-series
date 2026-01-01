/*
<:copyright-BRCM:2019:DUAL/GPL:standard

   Copyright (c) 2019 Broadcom
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

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
#include <linux/if_ether.h>
#include <ifaddrs.h>
#include <os_defs.h>
#include <dirent.h>
#include "board.h"
#include "bcm_boardctl.h"
#include "sysutil_net.h"
#include "bcmnet.h"

#define VLANSUBIF_DEBUG

#include <linux/bcm_skb_defines.h>
#include <vlan_subif.h>

#ifdef VLANSUBIF_DEBUG
static vlansubif_logLevel_t vlansubif_logLevel = VLANSUBIF_LOG_LEVEL_DEBUG;
#else
static vlansubif_logLevel_t vlansubif_logLevel = VLANSUBIF_LOG_LEVEL_ERROR;
#endif

#define vlansubif_err_print(fmt, arg...) \
     fprintf(stderr, "%s:%d >> " fmt, __FILE__, __LINE__, ##arg);

/*
 * Local variables
 */

static int is_initialized;
#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
#define DEFAULT_STDOUT_FILE     "/dev/tty"
#endif

#define VLAN_SUBIF_MAX_TAG_NUM             2
#define VLAN_SUBIF_MAX_CMD_LENGTH          200
#define VLAN_SUBIF_LAN_BASE_PRIO           10000
#define VLAN_SUBIF_BASE_SUB_PRIO           30000
#define VLAN_SUBIF_UNTAGGED_MATCHALL_PRIO  60000
#define VLAN_SUBIF_TAGGED_PRIO_BASE        50000
#define VLAN_SUBIF_SUB_PRIO_NUMS           10
#define VLAN_SUBIF_SUB_PRIO_INTERVAL       (VLAN_SUBIF_SUB_PRIO_NUMS*2)
#define NEXT_PRIO(a)                       ((a)+2)

#define macNumToStr(_macNum, _macStr) \
  sprintf(_macStr, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x", \
  (UINT8)_macNum[0], (UINT8)_macNum[1], (UINT8)_macNum[2], \
  (UINT8)_macNum[3], (UINT8)_macNum[4], (UINT8)_macNum[5]);

/* QoS tag rules, match skb flow id. */
#define FLOW_ID_2_PRIO_1(a)         ((a)*4)
#define TC_FILTER_EGRESS_QOS_RULE \
  "tc filter add dev %s egress pri %u u32 match mark 0x%x 0x7F800 action vlan modify priority %u id %u continue"


/* flow_id = mark[18:11] */
#define SET_MARK_FLOWID_VALUE(a)               ((a)<<11)

/* Default Egress QoS Mapping Table
 * 0 ~ 7: Normal data, or high priority data, or TOS mapping from ip forward
 * 8 ~ 15: Are used for pbit remark based on QoS rules */
#define DEF_EGRESS_QOS_MAPPING               "8:0 9:1 10:2 11:3 12:4 13:5 14:6 15:7"
#define REV_EGRESS_QOS_MAPPING(x)            "0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u", x,x,x,x,x,x,x,x
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

static int vlanSubif_system(const char *cmd)
{
    int ret;
    VLANSUBIF_LOG_DEBUG(">>> '%s'\n", cmd);
    ret = system(cmd);
    if (ret)
    {
        VLANSUBIF_LOG_ERROR("<<< Command '%s' failed with error %s\n", cmd,
            (ret == -1) ? "-1" : strerror(WEXITSTATUS(ret)));
    }
    else
    {
        VLANSUBIF_LOG_DEBUG("<<< Command '%s' executed successfully\n", cmd);
    }
    return ret;
}


/*
 * Public functions
 */

/*
 * VLAN subif APIs
 */

int vlanSubif_init(void)
{
    if (!is_initialized)
    {
#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
        vlanSubif_logRedirect(VLAN_SUBIF_LOG_REDIRECT_TO_FILE);
#endif
        is_initialized = 1;
    }
    return 0;
}

void vlanSubif_cleanup(void)
{
    is_initialized = 0;
#ifdef VLAN_SUBIF_LOG_REDIRECT_TO_FILE
    vlanSubif_logRedirect(NULL);
#endif
}

static int vlanSubif_qdiscExist(const char *ifName)
{
    char cmdstr[BUFLEN_128] = {0};
    char data[BUFLEN_128] ={0};
    FILE* file = NULL;
    int ret = 0;

    sprintf(cmdstr, "tc qdisc show dev %s clsact", ifName);

    file = popen(cmdstr, "r");
    if (!file)
        return 0;
    
    if (fgets(data, sizeof(data), file) != NULL)
    {
        ret = 1;
    }

    pclose(file);

    return ret;
}

static int vlanSubif_macInvalid(const uint8_t *mac)
{
    return (((mac[0] & 0x01) == 0x01) ||
        (!mac[0] && !mac[1] && !mac[2] && !mac[3] && !mac[4] && !mac[5]));
}

static int vlanSubif_isValidIfName(const char* IfName)
{
   if (IfName == NULL || strlen(IfName) == 0)
   {
      return 0;
   }

   return 1;
}

static int vlanSubif_isSubSubDev(const char *vlanIfaceName, char *baseL3IfaceName)
{
    char  *subStr  = NULL;
    char  *sSubStr = NULL;
    char   *ptr    = NULL;    
    int    ret = 0;

    ptr    = (char *)vlanIfaceName;
    subStr = strstr(vlanIfaceName, ".");
    if (subStr != NULL)
    {
        subStr++;
        sSubStr = strstr(subStr, ".");
        if (sSubStr != NULL)
        {
            if (baseL3IfaceName != NULL)
            {
                strncpy(baseL3IfaceName, vlanIfaceName, (sSubStr-ptr));
            }
            
            ret = 1;
        }
    }
    
    return ret;
}

/*************************************************************************************
 Make the priority of sub-sub interfaces(veip0.1.pt) always be lower than the priority 
 of normal interfaces(veip0.1).
 
 Assume the operated vlanDevName like veip0.1 or veip0.1.pt, lowerDevName is veip0.
*************************************************************************************/
static int vlanSubif_getStaticPriByIfName(const char *lowerDev, const char *vlanDev)
{
    int subIndex  = 0;
    int prio      = 0;
    int num       = 0;
    char *ptr       = NULL;
    char *numStrPtr = NULL;
    char numStr[BUFLEN_8] = {0};
    char lowerDevName[BUFLEN_64] = {0};
    char vlanDevName[BUFLEN_64] = {0};

    strncpy(vlanDevName, vlanDev, sizeof(vlanDevName) - 1);
    strncpy(lowerDevName, lowerDev, sizeof(lowerDevName) - 1);

    /*Get subIndex 1 from vlanIface, eth0.0/eth0.1/veip0.1/veip0.1.pt/...*/
    ptr = strstr(vlanDevName,".");
    if (ptr != NULL)
    {   
        num = 0;
        ptr ++;
        numStrPtr = ptr;
        
        /*get subIndex 1 from 1.pt*/
        while(isdigit(*ptr))
        {
            num ++;
            ptr ++;
        }
        strncpy(numStr, numStrPtr, num);
    }
    
    subIndex = atoi(numStr);

    if (subIndex)
    {        
        if (vlanSubif_isSubSubDev(vlanDevName, NULL))
        {   
            prio = subIndex*VLAN_SUBIF_SUB_PRIO_INTERVAL + VLAN_SUBIF_BASE_SUB_PRIO;
        }
        else
        {
            prio = subIndex*VLAN_SUBIF_SUB_PRIO_INTERVAL;
        }
    }
    /* Currently, LAN always use subIndex 0, like eth0.0/eth1.0/...*/
    else  
    {
        prio = VLAN_SUBIF_LAN_BASE_PRIO;
    }
    
    return prio;
}
/*************************************************************************************
 The matchall classifier simply matches all packets, then the VLAN devices will not
 receive any traffic if there is a matchall rule on the lowerlayer device.
 The solution is to add a classifier which matches VLANs with pass action to forward the
 VLAN traffic before hitting the matchall classifier.
 This function is to manage the priority(pref) for these classifiers.
*************************************************************************************/
static int vlanSubif_getFilterPriForVlanDev(int vlanId)
{
    return vlanId*2 + VLAN_SUBIF_TAGGED_PRIO_BASE;
}

static int vlanSubif_getMacAddrByIfName(const char *ifname, uint8_t *macAddr)
{
   int            ret = 0;
   int            sockfd;
   struct ifreq   ifr;

   if (ifname == NULL || macAddr == NULL)
   {
      return -1;
   }
   
   /* open socket to get INET info */
   if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror("socket returns error.");
      return -1;
   }

   memset(&ifr, 0, sizeof(struct ifreq));
   strcpy(ifr.ifr_name, ifname);

   if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) >= 0)
   {
      memcpy(macAddr, (uint8_t *)&ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN);
   }
   else
   {
      perror("SIOCGIFHWADDR returns error.");
      ret = -1;
   }
   
   close(sockfd);

   return ret;
}

static int vlanSubif_allocMacAddrForInterface(const char *vlanDevName, const char *lowerDevName, uint8_t *macNum, int isRoute)
{
    char    macStr[MAC_STR_LEN + 1] = {0};
    char    cmd[VLAN_SUBIF_MAX_CMD_LENGTH] = {0};
    uint8_t allocMac[MAC_ADDR_LEN] = {0}; 
    int  ret = 0;

    /* 1, alloc MAC for the interface */
    if (isRoute)
    {
        ret = devCtl_getMacAddress(allocMac, if_nametoindex(vlanDevName));
    }
    else
    {
        ret = devCtl_getBaseMacAddress(allocMac);
    }
    
    if (ret || vlanSubif_macInvalid(allocMac))
    {
        return  -1;
    }

    memcpy(macNum, allocMac, MAC_ADDR_LEN);
    
    /*2, set MAC to the interface */
    snprintf(cmd, sizeof(cmd), "ifconfig %s down", vlanDevName);
    vlanSubif_system(cmd);
    
    macNumToStr(allocMac, macStr);
    snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %s", vlanDevName, macStr);
    ret = vlanSubif_system(cmd);

    return ret;
}


static int vlanSubif_releaseMacAddress(uint8_t *macNum)
{
    return devCtl_releaseMacAddress(macNum);
}

static int vlanSubif_isBaseMacUsed(const char *vlanDev, uint8_t *macNum)
{
    int ret = 0;
    uint8_t bsaeMac[MAC_ADDR_LEN]; 

    devCtl_getBaseMacAddress(bsaeMac);
    
    ret = vlanSubif_getMacAddrByIfName(vlanDev, macNum);
    if (!ret)
    {
        if (memcmp(macNum, bsaeMac, MAC_ADDR_LEN)) //if not base mac, return 0.
        {
            return 0;
        }
    }
    
    return 1;
}

static void vlanSubif_clearTcRulesByIfName(const char *lowerDevName, const char *vlanDevName, int is_routed)
{
    int  prio = 0;
    int  num = 0;
    int  vlanId = INVALID_VLANID;
    int  is_tagged = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH] = {0};

    // get the vlanId used by the device vlanDevName
    vlanId = sysUtl_getVlanId(vlanDevName);

    is_tagged = (vlanId != INVALID_VLANID);
    if (is_tagged)
    {
        prio = vlanSubif_getFilterPriForVlanDev(vlanId); //vlan device rule
    }
    else if (!is_routed) //untagged bridged
    {
        prio = VLAN_SUBIF_UNTAGGED_MATCHALL_PRIO; //untagged base tc filter rule
    }

    // if prio is set
    if (prio)
    {
        snprintf(cmd, sizeof(cmd) - 1, "tc filter del dev %s ingress pri %d 2>/dev/null", lowerDevName, prio);
        vlanSubif_system(cmd);
    }

    if (bcmnet_ioctl_iswandev(lowerDevName) == 1)
    {
        prio = vlanSubif_getStaticPriByIfName(lowerDevName, vlanDevName);

        /*special case. currently, only for ppp-bridge or untagged route */
        if (vlanSubif_isSubSubDev(vlanDevName, NULL) || ((!is_tagged) && is_routed))
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc qdisc del dev %s clsact", vlanDevName);
            vlanSubif_system(cmd);
        }

        for (num = 0; num < VLAN_SUBIF_SUB_PRIO_NUMS; num++)
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc filter del dev %s ingress pri %d 2>/dev/null", lowerDevName, prio);
            vlanSubif_system(cmd);

            prio = NEXT_PRIO(prio);
        }
    }
}

static int vlanSubif_addUntagBaseTcRules(const char *lowerDevName, 
    const char *vlanDevName, int is_routed)
{
    int  ret      = 0;
    int  basePrio = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];

    basePrio = vlanSubif_getStaticPriByIfName(lowerDevName, vlanDevName);
    /* if is_ok_if_exists, TC rule might exist at this point. Ignore "tc filter add" errors */
    if (is_routed)
    {
        snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d "
                                        "flower num_of_vlans 0 " 
                                        "action pass", 
                                        lowerDevName, basePrio);
    }
    else
    {
        snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d matchall "
                                        "action mirred egress redirect dev %s", 
                                        lowerDevName, VLAN_SUBIF_UNTAGGED_MATCHALL_PRIO, vlanDevName);
    }

    vlanSubif_system(cmd);

    return ret;
}

static int vlanSubif_addPppBridgeUnTagTcRules(const char *lowerDevName, 
    const char *vlanDevName, const vlansubif_options_t *options)
{
    int prio         = 0;
    int num          = 0;
    int ret          = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];
    char baseL3IfName[BUFLEN_32] = {0};
    char macStr[MAC_STR_LEN + 1];
    uint8_t macNum[MAC_ADDR_LEN]; 

    if (vlanSubif_isSubSubDev(vlanDevName, baseL3IfName))
    {
        prio = vlanSubif_getStaticPriByIfName(lowerDevName, baseL3IfName);

        /*replace TC rules for veip0.1, filter MAC*/
        ret = vlanSubif_getMacAddrByIfName(baseL3IfName, macNum);
        if (!ret)
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc filter del dev %s ingress pri %d", 
                                           lowerDevName, prio);
            vlanSubif_system(cmd);  
        
            macNumToStr(macNum, macStr);
            snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d "
                                           "flower dst %s num_of_vlans 0 " 
                                           "action pass", 
                                           lowerDevName, prio, macStr);
            vlanSubif_system(cmd);  
        }

        /*add new TC rules for veip0.1.pt, filter protocol*/
        prio = vlanSubif_getStaticPriByIfName(lowerDevName, vlanDevName);

        if (options && options->num_etype_filters)
        {
            for (num = 0; num < options->num_etype_filters; num++)
            {
                snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d protocol 0x%x matchall "
                                                "action mirred egress redirect dev %s",
                                                lowerDevName, prio, options->etype_filter[num], vlanDevName);
                vlanSubif_system(cmd);  
                prio = NEXT_PRIO(prio);
            }
        }  

        /*add default TC rules for veip0.1*/
        prio = NEXT_PRIO(prio);
        snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d "
                                        "flower num_of_vlans 0 " 
                                        "action mirred egress redirect dev %s", 
                                        lowerDevName, prio, baseL3IfName);
        vlanSubif_system(cmd);
    }

    return ret;
}

static int vlanSubif_addPppBridgeTagTcRules(const char *lowerDevName, 
    const char *vlanDevName, signed int vlanId)
{
    int  prio = 0;
    int  num  = 0;
    int  ret  = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];
    char baseL3IfName[BUFLEN_32] = {0};
    char macStr[MAC_STR_LEN + 1];
    uint8_t macNum[MAC_ADDR_LEN]; 

    if (vlanSubif_isSubSubDev(vlanDevName, baseL3IfName))
    {
        prio = vlanSubif_getStaticPriByIfName(lowerDevName, baseL3IfName);
        ret  = vlanSubif_getMacAddrByIfName(baseL3IfName, macNum);
        if (!ret)
        {
            /*TC rules for veip0.1, filter MAC*/
            macNumToStr(macNum, macStr);
            snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress "
                                            "pri %d flower dst %s action pass", 
                                            lowerDevName, prio, macStr);
            vlanSubif_system(cmd);
        }

        if(!vlanSubif_qdiscExist(vlanDevName))
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc qdisc add dev %s clsact", vlanDevName);
            vlanSubif_system(cmd);
        }

        prio = vlanSubif_getStaticPriByIfName(lowerDevName, vlanDevName);
        /*TC ruels on vlanDev veip0.1.pt*/
        snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d matchall action vlan pop",
                                       vlanDevName, prio); 
        ret = ret ? ret : vlanSubif_system(cmd);
        
        snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s egress pri %d matchall action vlan push protocol 802.1Q id %d continue",
                                       vlanDevName, prio, vlanId); 
        ret = ret ? ret : vlanSubif_system(cmd);

        /*TC ruels on lowerDev veip0*/
        for (num = 1; num <= VLAN_SUBIF_MAX_TAG_NUM; num++)
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pri %d flower num_of_vlans %d vlan_id %d action vlan pop pipe mirred egress redirect dev %s",
                                           lowerDevName, prio, num, vlanId, vlanDevName); 
            ret = ret ? ret : vlanSubif_system(cmd);

            prio = NEXT_PRIO(prio);
        }
    }
    return ret;
}

static int vlanSubif_setMulticast(const char *vlanIfaceName, int isDisabled)
{
    int  ret = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];
    
    if (isDisabled)
    {
        snprintf(cmd, sizeof(cmd) - 1, "ip link set %s multicast off", vlanIfaceName);
        ret = vlanSubif_system(cmd);
    }
    else
    {
        snprintf(cmd, sizeof(cmd) - 1, "ip link set %s multicast on", vlanIfaceName);   
        ret = vlanSubif_system(cmd);
    }

    return ret;
}

static int vlanSubif_setAllMulticast(const char *vlanIfaceName, int isDisabled)
{
    int  ret = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];
    
    if (isDisabled)
    {
        snprintf(cmd, sizeof(cmd) - 1, "ip link set %s allmulticast off", vlanIfaceName);
        ret = vlanSubif_system(cmd);
    }
    else
    {
        snprintf(cmd, sizeof(cmd) - 1, "ip link set %s allmulticast on", vlanIfaceName);   
        ret = vlanSubif_system(cmd);
    }

    return ret;
}


/** Check if the rule is exists in ebtables 
 *
 * @param *xTabCmd   (IN) Cmd to list all rules 
 * @param *chain     (IN) The name of chain
 * @param *rule      (IN) The rule want to check 
 *
 * @return 0 - not exits; 1 - exists
 * */
int vlanSubif_ebtRuleExists(const char *xTabCmd, const char *chain, const char *rule)
{
    char cmd[BUFLEN_512]={0};
    char line_buff[BUFLEN_256] = {0};
    FILE * fp;
    int isExists = 0, startLine, endLine;

    /* Find the start line of chain */ 
    snprintf(cmd, sizeof(cmd), "%s | awk '/Bridge chain: %s/ {print NR}'", xTabCmd, chain);
    fp = popen(cmd, "r");
    if (!fp)
        goto RuleFindExit;

    if(fgets(line_buff, sizeof(line_buff), fp) == NULL)
        goto  RuleFindExit;

    sscanf(line_buff, "%d", &startLine);
    pclose(fp);

    /* Find the end line of chain */
    memset(line_buff, 0, sizeof(line_buff));
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "%s | awk 'NR>%d {if(index($0,\"Bridge chain:\")>0) print NR} END {print NR}'", 
                        xTabCmd, startLine);
    fp = popen(cmd, "r");
    if (!fp)
        goto RuleFindExit;

    if(fgets(line_buff, sizeof(line_buff), fp) == NULL)
        goto RuleFindExit;

    sscanf(line_buff, "%d", &endLine);
    pclose(fp);
    
    snprintf(cmd, sizeof(cmd), "%s | awk 'NR>=%d&&NR<=%d {if (index($0,\"%s\")>0) print \"Found\";}'", xTabCmd, 
                        startLine, endLine, rule); 
    fp = popen(cmd, "r");
    if (fp)
    {
        while( fgets(line_buff, sizeof(line_buff), fp) != NULL )
        {
            if (strstr(line_buff, "Found"))
            {
                isExists = 1;
                break;               
            }
        }
    }

RuleFindExit:
    if (fp)
        pclose(fp);
        
    return isExists;
}

/* Due to MSCS feature, the skb priority might be set none-zero value for the data coming from WiFi.
 * It will impact the QoS rules. We need to overwrite it, the conditions are that:
 *  Input interface is wlXX; skb flow id value is 0. */
static void vlanSubif_addDefBrRulesForMscs(void)
{
    char cmd[BUFLEN_256]={0};
    char rule[BUFLEN_64]={0};
    int isAdded = 0;

    snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t filter -L");
    snprintf(rule, sizeof(rule), "-i wl+ --mark 0x0/0x%x -j skbpriority", SKBMARK_FLOW_ID_M);
    isAdded = vlanSubif_ebtRuleExists(cmd, "INPUT", rule);
    if(isAdded)
        return;

    snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t filter -I FORWARD -i wl+ -o ! wl+ --mark 0/0x%x -j skbpriority --set-class 0:0 --skbpriority-target CONTINUE 2>/dev/null",
               SKBMARK_FLOW_ID_M);
    cmd[sizeof(cmd) - 1] = 0;
    (void)vlanSubif_system(cmd); 

    snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t filter -I INPUT -i wl+ --mark 0/0x%x -j skbpriority --set-class 0:0 --skbpriority-target CONTINUE 2>/dev/null",
               SKBMARK_FLOW_ID_M);
    cmd[sizeof(cmd) - 1] = 0;
    (void)vlanSubif_system(cmd); 

    isAdded = 1;
    return;
}

static UBOOL8 vlanSubif_validateIfAttribute(char *cmdstr)
{
    FILE* file = NULL;
    char data[BUFLEN_128] = {0};
    UBOOL8 match = 0;

    file = popen(cmdstr, "r");
    if (!file)
        return 0;
    
    if (fgets(data, sizeof(data), file) != NULL)
    {
        match = 1;
    }

    pclose(file);

    return match;
}

static UBOOL8 vlanSubif_isIfAttributeMatch(const char *vlanIfaceName, signed int vlanId,
    const vlansubif_options_t *options)
{
    UBOOL8 is_tagged = (vlanId >= 0);
    UBOOL8 is_multicast_disabled = (options != NULL && options->is_multicast_disabled);
    UBOOL8 is_routed = (options != NULL && options->is_routed);
    UBOOL8 is_ppp_bridge = (options != NULL && (options->subif_type == VLANSUBIF_TYPE_PPP_BRIDGE));
    int pbit = (options != NULL) ? options->pbit : 0;
    int tpid = (options != NULL && options->tpid) ? options->tpid : ETH_P_8021Q;
    UBOOL8 match = 1;
    char cmdstr[VLAN_SUBIF_MAX_CMD_LENGTH];
    char buf[BUFLEN_64];

    if (!is_tagged)
    {
        if (is_routed)
        {
            snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s type macvlan", vlanIfaceName);
            match = vlanSubif_validateIfAttribute(cmdstr);
        }
        else
        {
            snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s type vfrwd", vlanIfaceName);
            match = vlanSubif_validateIfAttribute(cmdstr);
        }

        if (match)
        {
             snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s| grep MULTICAST", vlanIfaceName);
             if (is_multicast_disabled == vlanSubif_validateIfAttribute(cmdstr))
                 match = 0;
        }
    }
    else
    {
        if (is_ppp_bridge)
        {
            snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s type vfrwd", vlanIfaceName);
            match = vlanSubif_validateIfAttribute(cmdstr);
        }
        else
        {
            snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s type vlan | grep 'protocol %s id %d'", 
                     vlanIfaceName,
                     tpid == ETH_P_8021Q? "802.1Q" : "802.1ad",
                     vlanId);
            match = vlanSubif_validateIfAttribute(cmdstr);
            if (!match)
                goto exit;

            if (pbit > 0 && pbit <= 7)
            {
                 snprintf(buf, sizeof(buf) - 1, REV_EGRESS_QOS_MAPPING(pbit));
                 snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s type vlan | grep '%s'", vlanIfaceName, buf);
                 match = vlanSubif_validateIfAttribute(cmdstr);
                 if (!match)
                     goto exit;
            }
            else if (pbit == 0)
            {
                 snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s type vlan | grep 'egress-qos-map { 0:'", vlanIfaceName);
                 match = !vlanSubif_validateIfAttribute(cmdstr);
                 if (!match)
                     goto exit;
            }
            else
            {
                match = 0;
                goto exit;
            }

            snprintf(cmdstr, sizeof(cmdstr) - 1, "ip -d link show %s| grep MULTICAST", vlanIfaceName);
            if (is_multicast_disabled == vlanSubif_validateIfAttribute(cmdstr))
                 match = 0;
        }
    }
exit:
    return match;
}

int vlanSubif_createVlanInterface(const char *lowerDevName, signed int vlanId,
    const vlansubif_options_t *options)
{
    char vlanIface[BUFLEN_32];
    const char *vlanIfaceName;
    int was_initialized = is_initialized;
    UBOOL8 is_tagged = (vlanId >= 0);
    UBOOL8 is_multicast_disabled = (options != NULL && options->is_multicast_disabled);
    UBOOL8 is_ok_if_exists = (options != NULL && options->is_ok_if_exists);
    UBOOL8 is_if_existed = 0;
    UBOOL8 is_routed = (options != NULL && options->is_routed);
    UBOOL8 is_ppp_bridge = (options != NULL && (options->subif_type == VLANSUBIF_TYPE_PPP_BRIDGE));
    int pbit = (options != NULL) ? options->pbit : 0;
    int tpid = (options != NULL && options->tpid) ? options->tpid : ETH_P_8021Q;
    int prio = 0;
    int ret  = 0;
    int len  = 0;
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];
    char cmd_prefix[BUFLEN_64];
    uint8_t macNum[MAC_ADDR_LEN]; 

    vlanSubif_init();
    
    vlanSubif_addDefBrRulesForMscs();
    
    if (options != NULL && options->name != NULL)
    {
        vlanIfaceName = options->name;
    }
    else
    {
        vlanSubif_IntfName(lowerDevName, vlanId, vlanIface, sizeof(vlanIface));
        vlanIfaceName = vlanIface;
    }

    VLANSUBIF_LOG_DEBUG("Create %s over %s. vlan_id=%d is_mcast_disabled=%d "
                        "is_ok_if_exists=%d is_routed=%d pbit=%d tpid=0x%x\n",
                        vlanIfaceName, lowerDevName, vlanId, is_multicast_disabled,
                        is_ok_if_exists, is_routed, pbit, tpid);

    is_if_existed = sysUtl_interfaceExists(vlanIfaceName);
    if (is_ok_if_exists && is_if_existed)
    {
        if (vlanSubif_isIfAttributeMatch(vlanIfaceName, vlanId, options))
        {
            VLANSUBIF_LOG_DEBUG("Interface %s already exists.\n", vlanIfaceName);
            return 0;
        }
        else
        {
            VLANSUBIF_LOG_ERROR("Cannot modify interface %s attibutes!\n", vlanIfaceName);
            return -EINVAL;
        }
    }
    else
    {
        snprintf(cmd_prefix, sizeof(cmd_prefix) - 1, "ip link add link %s name", lowerDevName);
    }

    cmd[sizeof(cmd) - 1] = 0;
    
    /*1, untagged mode*/
    if (!is_tagged)
    {
        /* vlanctl clients create default sub-interface with vid==0 in order to attach physical
           LAN interfaces to the bridge. This interface appears to be just a tunnel to the physical
           interface. It is unclear how to achieve it using the standard linux tools.
        */
        if (is_routed)
        {
            snprintf(cmd, sizeof(cmd) - 1, "%s %s type macvlan mode private",
                cmd_prefix, vlanIfaceName);
            
            ret = vlanSubif_system(cmd);

            /* macvlan dev needs IFF_ALLMULTI flag to receive mcast data.*/
            ret = ret ? ret : vlanSubif_setAllMulticast(vlanIfaceName, is_multicast_disabled);
        }
        else
        {
            snprintf(cmd, sizeof(cmd) - 1, "%s %s type vfrwd",
                cmd_prefix, vlanIfaceName);
            
            ret = vlanSubif_system(cmd);
        }

        if (ret)
        {
            /*return directly*/
            return ret; 
        }

        if(!vlanSubif_qdiscExist(lowerDevName))
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc qdisc add dev %s clsact", lowerDevName);
            vlanSubif_system(cmd);
        }
        
        if(is_routed)
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc qdisc add dev %s clsact", vlanIfaceName);
            vlanSubif_system(cmd);
        }

        ret =  vlanSubif_allocMacAddrForInterface(vlanIfaceName, lowerDevName, macNum, is_routed);
        if (!ret)
        {
            /*1-a, specific untagged interface*/
            if (is_ppp_bridge)
            {   
                vlanSubif_addPppBridgeUnTagTcRules(lowerDevName, vlanIfaceName, options);
            }
            /*1-b, normal interface*/
            else
            {
                vlanSubif_addUntagBaseTcRules(lowerDevName, vlanIfaceName, is_routed);
            }
        }

        ret = ret ? ret : vlanSubif_setMulticast(vlanIfaceName, is_multicast_disabled);
    }
    /*2, tagged mode*/
    else 
    {
        if(!vlanSubif_qdiscExist(lowerDevName))
        {
            snprintf(cmd, sizeof(cmd) - 1, "tc qdisc add dev %s clsact", lowerDevName);
            vlanSubif_system(cmd);
        }

        if (!is_if_existed)
        {
            /* To bypass the untagged matchall tc filter if any */
            prio = vlanSubif_getFilterPriForVlanDev(vlanId);
            snprintf(cmd, sizeof(cmd) - 1, "tc filter add dev %s ingress pref %d "
                "protocol 802.1q flower vlan_id %d action vlan save pipe pass",
                lowerDevName, prio, vlanId);
            ret = ret ? ret : vlanSubif_system(cmd);
        }

        /*2-a, specific tagged interface*/
        if (is_ppp_bridge) /*ppp bridge interface*/
        {   
            snprintf(cmd, sizeof(cmd) - 1, "%s %s type vfrwd",
                                    cmd_prefix, vlanIfaceName);
            ret = vlanSubif_system(cmd);
            if (ret)
            {
                /*return directly*/
                return ret; 
            }

            vlanSubif_addPppBridgeTagTcRules(lowerDevName, vlanIfaceName, vlanId);
        }
        /*2-b, Normal tagged interface*/
        else   
        {
            /* tagged interface. Create a new VLAN sub-interface using "ip link add vlan" */
            len = snprintf(cmd, sizeof(cmd) - 1, "%s %s type vlan id %u",
                                            cmd_prefix, vlanIfaceName, vlanId);

            /* Add 'protocol' option if tpid != 8021Q (0x8100) */
            if (tpid != ETH_P_8021Q)
            {
                if (tpid == ETH_P_8021AD)
                {
                    len += snprintf(&cmd[len], sizeof(cmd) - 1 - len, " protocol 802.1ad");
                }
                else
                {
                    VLANSUBIF_LOG_ERROR("Interface %s: Invalid TPID value 0x%x."
                                        " Only 0x%x and 0x%x values are supported\n",
                                         vlanIfaceName, tpid, ETH_P_8021Q, ETH_P_8021AD);
                    ret = -EINVAL;
                }
            }

            /*
              linux may use skb priority 0~7 to do schedule(refer TC_PRIO_CONTROL) 
              So rut_qos_class.c use skb priority 8~15 to do classification.
            */
            if (pbit != 0)
            {
                /* 
                   Add egress-qos-map option in order to set PBIT!=0.
                   For now map skb priority 0 ~ 7, 
                   or 8~15(qos classification priority) to the pbit.
                */
                if (pbit <= 7)
                {
                    len += snprintf(&cmd[len], sizeof(cmd) - 1 - len, " egress-qos-map ");
                    len += snprintf(&cmd[len], sizeof(cmd) - 1 - len, REV_EGRESS_QOS_MAPPING(pbit));
                    len += snprintf(&cmd[len], sizeof(cmd) - 1 - len, " %s",DEF_EGRESS_QOS_MAPPING);
                }
                else
                {
                    VLANSUBIF_LOG_ERROR("Interface %s: invalid PBIT value %u\n", vlanIfaceName, pbit);
                    ret = -EINVAL;
                }
            }
            else
            {   
                /* 
                  Set default Egress QoS mapping table.
                  For now map just skb priority 8~15(QOS classification priority) to the pbit.
                */
                len += snprintf(&cmd[len], sizeof(cmd) - 1 - len, " egress-qos-map %s", DEF_EGRESS_QOS_MAPPING);
            }

            /* Finally executre the command */
            ret = ret ? ret : vlanSubif_system(cmd);
            if (ret)
            {
                /*return directly*/
                return ret; 
            }

            ret = vlanSubif_allocMacAddrForInterface(vlanIfaceName, lowerDevName, macNum, is_routed);

            ret = ret ? ret : vlanSubif_setMulticast(vlanIfaceName, is_multicast_disabled);
        } 
    } 

    if (ret) 
    {
        VLANSUBIF_LOG_ERROR("Creating interface %s failed with error %d\n", vlanIfaceName, ret);
 
        /*rollback MAC*/
        if (is_routed)
        {
            vlanSubif_releaseMacAddress(macNum);
        }
        
        /*rollback  TC rules*/
        vlanSubif_clearTcRulesByIfName(lowerDevName, vlanIfaceName, is_routed);

        /*delete interface*/
        snprintf(cmd, sizeof(cmd) - 1, "ip link delete %s", vlanIfaceName);
        vlanSubif_system(cmd);
    }
    else
    {
        if (bcmnet_ioctl_iswandev(lowerDevName))
        {
            /*do not reply ARP at WAN lower interface*/
            VLANSUBIF_LOG_DEBUG("Lower interface %s arp_ignore set to 1\n", lowerDevName);
            snprintf(cmd, sizeof(cmd) - 1, "echo 1 > /proc/sys/net/ipv4/conf/%s/arp_ignore", lowerDevName);
            vlanSubif_system(cmd);
        }
        VLANSUBIF_LOG_DEBUG("Interface %s created successfully\n", vlanIfaceName);
    }

    if (!was_initialized)
        vlanSubif_cleanup();

    return ret;
}

int vlanSubif_deleteVlanInterface(const char *vlanDevName)
{
    int  was_initialized = is_initialized;
    int is_routed = 0;
    int isBaseMacUsed = 0;
    uint8_t macNum[MAC_ADDR_LEN]; 
    char cmd[VLAN_SUBIF_MAX_CMD_LENGTH];
    int lowerDevIfindex = 0;
    char localLowerDevName[BUFLEN_32] = {0};
    int  ret;

    vlanSubif_init();

    VLANSUBIF_LOG_DEBUG(">>>>>> vlanDevName %s\n", vlanDevName?vlanDevName:"NULL");

    if (!vlanSubif_isValidIfName(vlanDevName))
    {
        return -1;
    }

    isBaseMacUsed = vlanSubif_isBaseMacUsed(vlanDevName, macNum);
    is_routed = !isBaseMacUsed;

    lowerDevIfindex = sysUtl_getLowerDeviceIfindex(vlanDevName);
    if (if_indextoname(lowerDevIfindex, localLowerDevName))
    {
        VLANSUBIF_LOG_DEBUG("Interface vlanDevName %s's parent is %s\n", vlanDevName, localLowerDevName);
        vlanSubif_clearTcRulesByIfName(localLowerDevName, vlanDevName, is_routed);
    }
    else
    {
        VLANSUBIF_LOG_ERROR("Interface vlanDevName %s has no parent\n", vlanDevName);
        return -1;
    }

    if (!isBaseMacUsed)
    {
        vlanSubif_releaseMacAddress(macNum);
    }

    cmd[sizeof(cmd) - 1] = 0;
    snprintf(cmd, sizeof(cmd) - 1, "ip link delete %s", vlanDevName);
    ret = vlanSubif_system(cmd);

    if (ret)
    {
        VLANSUBIF_LOG_ERROR("Deleting interface %s failed with error %d\n", vlanDevName, ret);
    }
    else
    {
            
        if (bcmnet_ioctl_iswandev(localLowerDevName) && (sysUtl_upperDeviceExists(localLowerDevName) == 0))
        {
            VLANSUBIF_LOG_DEBUG("Lower interface %s arp_ignore set to 0\n", localLowerDevName);
            snprintf(cmd, sizeof(cmd) - 1, "echo 0 > /proc/sys/net/ipv4/conf/%s/arp_ignore", localLowerDevName);
            vlanSubif_system(cmd);
        }
        VLANSUBIF_LOG_DEBUG("Interface %s deleted successfully\n", vlanDevName);
    }
    if (!was_initialized)
        vlanSubif_cleanup();

    return ret;
}

int vlanSubif_addQosVlanTagRule(const char *vlanDevName, const vlansubif_qos_options_t *options)
{
    /* The pbit remark is done by set skb priority in iptables and ebtables, 
     * then do the mapping at vlan device egress qos mapping.
     * */
    return 0;
}


int vlanSubif_delQosVlanTagRule(const char *vlanDevName, const vlansubif_qos_options_t *options)
{
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

#ifdef CMS_MEM_LEAK_TRACING
 /*
  * Log control
  */
#define HAVE_BACKTRACE_SYMBOLS 1
#define HAVE_EXECINFO_H 1
#define BACKTRACE_STACK_SIZE 64

#ifdef HAVE_LIBUNWIND_H
#include <libunwind.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#ifdef HAVE_LIBEXC_H
#include <libexc.h>
#endif

static void log_stack_trace(void)
{
#ifdef HAVE_LIBUNWIND
    /* Try to use libunwind before any other technique since on ia64
     * libunwind correctly walks the stack in more circumstances than
     * backtrace.
     */
    unw_cursor_t cursor;
    unw_context_t uc;
    unsigned i = 0;

    char procname[256];
    unw_word_t ip, sp, off;

    procname[sizeof(procname) - 1] = '\0';

    if (unw_getcontext(&uc) != 0) {
        goto libunwind_failed;
    }

    if (unw_init_local(&cursor, &uc) != 0) {
        goto libunwind_failed;
    }

    printf("BACKTRACE:\n");

    do {
        ip = sp = 0;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        switch (unw_get_proc_name(&cursor,
            procname, sizeof(procname) - 1, &off) ) {
        case 0:
            /* Name found. */
        case -UNW_ENOMEM:
            /* Name truncated. */
            printf(" #%u %s + %#llx [ip=%#llx] [sp=%#llx]\n",
                i, procname, (long long)off,
                (long long)ip, (long long) sp);
            break;
         default:
         /* case -UNW_ENOINFO: */
         /* case -UNW_EUNSPEC: */
            /* No symbol name found. */
            printf(" #%u %s [ip=%#llx] [sp=%#llx]\n",
                i, "<unknown symbol>",
                (long long)ip, (long long) sp);
        }
        ++i;
    } while (unw_step(&cursor) > 0);

    return;

libunwind_failed:
    printf("unable to produce a stack trace with libunwind\n");

#elif HAVE_BACKTRACE_SYMBOLS
    void *backtrace_stack[BACKTRACE_STACK_SIZE];
    size_t backtrace_size;
    char **backtrace_strings;

    /* get the backtrace (stack frames) */
    backtrace_size = backtrace(backtrace_stack,BACKTRACE_STACK_SIZE);
    backtrace_strings = backtrace_symbols(backtrace_stack, backtrace_size);

    printf("BACKTRACE: %lu stack frames:\n",
        (unsigned long)backtrace_size);

    if (backtrace_strings) {
        int i;

        for (i = 0; i < backtrace_size; i++)
            printf(" #%u %s\n", i, backtrace_strings[i]);

        /* Leak the backtrace_strings, rather than risk what free() might do */
    }

#elif HAVE_LIBEXC

     /* The IRIX libexc library provides an API for unwinding the stack. See
     * libexc(3) for details. Apparantly trace_back_stack leaks memory, but
     * since we are about to abort anyway, it hardly matters.
     */

#define NAMESIZE 32 /* Arbitrary */

    __uint64_t  addrs[BACKTRACE_STACK_SIZE];
    char *names[BACKTRACE_STACK_SIZE];
    char  namebuf[BACKTRACE_STACK_SIZE * NAMESIZE];

    int i;
    int levels;

    ZERO_ARRAY(addrs);
    ZERO_ARRAY(names);
    ZERO_ARRAY(namebuf);

    /* We need to be root so we can open our /proc entry to walk
    * our stack. It also helps when we want to dump core.
    */
    become_root();

    for (i = 0; i < BACKTRACE_STACK_SIZE; i++) {
        names[i] = namebuf + (i * NAMESIZE);
    }

    levels = trace_back_stack(0, addrs, names,
            BACKTRACE_STACK_SIZE, NAMESIZE - 1);

    printf("BACKTRACE: %d stack frames:\n", levels);
    for (i = 0; i < levels; i++) {
        printf(" #%d 0x%llx %s\n", i, addrs[i], names[i]);
    }
#undef NAMESIZE

#else
    printf("unable to produce a stack trace on this platform\n");
#endif
}

#endif /* CMS_MEM_LEAK_TRACING */

void vlanSubif_log(vlansubif_logLevel_t level, const char *fmt, ...)
{
    va_list args;
    if (level > vlansubif_logLevel)
        return;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#ifdef CMS_MEM_LEAK_TRACING
    if (vlansubif_logLevel = VLANSUBIF_LOG_LEVEL_DEBUG)
        log_stack_trace();
#endif
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
