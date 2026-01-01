/*
#
#  Copyright 2011, Broadcom Corporation
#
# <:label-BRCM:2011:proprietary:standard
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
*/
#ifdef SUPPORT_NF_TABLES
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/utsname.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_lan.h"
#include "rut_upnp.h"
#include "rut_network.h"
#include "rut_virtualserver.h"
#include "rut_pmap.h"
#include "rut_wan.h"
#include "rut_ipsec.h"
#include "prctl.h"
#include "genutil_crc.h"

#define MAX_NAME_LEN 32
#define MAX_DEP_NUM  8
#define MAX_DIR_LEN  32

extern UBOOL8 isModuleInserted(char *moduleName);
extern void rutNft_insertModules();

// cmd : ex, "nft -a list chain bridge filter MacFilter"
static int get_nft_rule_handle_by_chain_name(char * cmd, char * chain_name)
{
   FILE * fp;
   int handle = -1;
  
   fp = popen(cmd, "r");
   if (fp)
   {
        char line_buff[BUFLEN_128] = { 0 };
        char *ch_ptr;
        
        while( fgets(line_buff, sizeof(line_buff), fp) != NULL )
        {
            if (strstr(line_buff, chain_name))
            {
               ch_ptr = strstr(line_buff, "#");
               sscanf(ch_ptr, "# handle %d", &handle);
               break;
            }
        }
        pclose(fp);        
    }
    return handle;
}


static void _delete_chain(char *tableName, char *chainName)
{
   char cmd[BUFLEN_256];

   if((tableName != NULL) && (chainName != NULL))
   {
      // flush usrChain
      snprintf(cmd, sizeof(cmd), "nft 'flush chain %s %s' 2>/dev/null", tableName, chainName);
      rut_doSystemAction("_delete_chain", cmd);

      // delete userChain
      snprintf(cmd, sizeof(cmd), "nft 'delete chain %s %s' 2>/dev/null", tableName, chainName);
      rut_doSystemAction("_delete_chain", cmd);         
   }
   return;
}

static void _delete_attached_chain(char * tableName, char * rootChain)
{
   FILE * fp;
   char cmd[BUFLEN_256];

   if((tableName != NULL) && (rootChain != NULL))
   {      
      snprintf(cmd, sizeof(cmd), "nft -a list chain %s %s", tableName, rootChain);

      fp = popen(cmd, "r");
      if (fp)
      {
        char line_buff[BUFLEN_128] = { 0 };
        char leafChain[BUFLEN_64] = {0};
        char *ch_ptr;
        int handle;
        
        while( fgets(line_buff, sizeof(line_buff), fp) != NULL )
        {
            // ex, jump t1 # handle 70
            if ((ch_ptr = strstr(line_buff, "jump")))
            {
               sscanf(ch_ptr, "jump %s # handle %d", leafChain, &handle);

               //  nft delete rule by handle
               snprintf(cmd, sizeof(cmd), "nft 'delete rule %s %s handle %d' 2>/dev/null", tableName, rootChain, handle);
               rut_doSystemAction("_delete_attached_chain", cmd);

               _delete_chain(tableName, leafChain);
            }
        }
        pclose(fp);        
      }
   }
   return;
}

void remove_nft_user_chain(char *baseChain, char *userChain)
{
   char cmd[BUFLEN_256];
   int handle;
   char *ch_ptr;
   
   if((baseChain == NULL) || (userChain == NULL))
   {
         cmsLog_error("remove_nft_user_chain : error baseChain = %s, userChain = %s", baseChain, userChain);
         return;
   }

   snprintf(cmd, sizeof(cmd), "nft -a list chain %s", baseChain);
   if ((handle = get_nft_rule_handle_by_chain_name(cmd, userChain)) >= 0 )
   {
      //  nft delete rule by handle
      snprintf(cmd, sizeof(cmd), "nft 'delete rule %s handle %d' 2>/dev/null", baseChain, handle);
      rut_doSystemAction(userChain, cmd);

      // baseChain is now family + table (Ex, "inet filter Input", now "inet filter")
      ch_ptr = strrchr(baseChain, ' ');
      *ch_ptr = '\0';

      // delete chain attached to userChain
      _delete_attached_chain(baseChain, userChain);

      // delete userChain
      _delete_chain(baseChain, userChain);
   }
   return;
}
#if 0
int isChainExist(char * tableName, char * chainName)
{
   FILE * fp;
   char cmd[BUFLEN_256];

   if((tableName == NULL) || (chainName == NULL))
   {  
      printf("isChainExist : error tableName = %s, chainName = %s\n", tableName, chainName);
      return -1;
   }
   
   snprintf(cmd, sizeof(cmd), "nft list table %s", tableName);
   printf("---isChainExist : cmd = %s, chainName = %s\n", cmd, chainName);
   
   fp = popen(cmd, "r");
   if (fp)
   {
        char line_buff[BUFLEN_128] = { 0 };
        
        while( fgets(line_buff, sizeof(line_buff), fp) != NULL )
        {
            //printf("--- line = %s", line_buff);
            if (strstr(line_buff, chainName))
            {
               return 1;
            }
        }
        pclose(fp);        
    }
    return 0;
}
#endif

void rutIpt_deleteNatMasquerade(const char *ifName, const char *localSubnet, const char *localSubnetMask)
{
   char cmd[CMS_IPADDR_LENGTH*2+CMS_IFNAME_LENGTH+BUFLEN_64];

   cmsLog_debug("ifname=%s localSubnet=%s localSubnetMask=%s", ifName, localSubnet, localSubnetMask);

   /* Delete current private subnet rule */
   snprintf(cmd, sizeof(cmd),  "nft delete chain ip nat NatMasq_%s 2>/dev/null", ifName );
   rut_doSystemAction("deleteNatMasq", cmd);

   return;
}

void rutIpt_insertNatMasquerade(const char *ifName, const char *localSubnet, const char *localSubnetMask)
{
   char cmd[CMS_IPADDR_LENGTH*2+CMS_IFNAME_LENGTH+BUFLEN_64];
   int zerobits = 0;
   struct sockaddr_in mask;
   uint32_t netmask;

   cmsLog_debug("ifName=%s localSubnet=%s localSubnetMask=%s", ifName, localSubnet, localSubnetMask);

   // calculate 0 in localSubnetMask
   inet_aton(localSubnetMask, &(mask.sin_addr));
   netmask = htonl(mask.sin_addr.s_addr);
   while ((netmask & 0x1) == 0) {
      netmask = netmask >> 1;
      zerobits++;
   }

   /* enable nat for the local private subnet only. */
   snprintf(cmd, sizeof(cmd),  "nft 'add chain ip nat NatMasq_%s { type nat hook postrouting priority srcnat; policy accept; }'", ifName );
   rut_doSystemAction("insertNatMasq", cmd);
   
   snprintf(cmd, sizeof(cmd),  "nft add rule ip nat NatMasq_%s oifname %s ip saddr %s/%d counter masquerade", ifName, ifName, localSubnet, (32-zerobits));
   rut_doSystemAction("insertNatMasq", cmd);        

   return;
}

void rutIpt_initIpSecPolicy(const char *ifName)
{
   char cmd[BUFLEN_256];

   // add user chain
   snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter IpsecPolicyInput_%s' 2>/dev/null", ifName);
   rut_doSystemAction("ipsecPolicy", cmd); 

   snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter IpsecPolicyFwd_%s' 2>/dev/null", ifName);
   rut_doSystemAction("ipsecPolicy", cmd); 

   snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter IpsecPolicyMangle_%s' 2>/dev/null", ifName);
   rut_doSystemAction("ipsecPolicy", cmd); 

   // add rule - jump to user chain
   snprintf(cmd, sizeof(cmd), "nft insert rule inet filter Input jump IpsecPolicyInput_%s ", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   snprintf(cmd, sizeof(cmd), "nft insert rule inet filter Forward jump IpsecPolicyFwd_%s ", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   snprintf(cmd, sizeof(cmd), "nft insert rule inet filter Mangle jump IpsecPolicyMangle_%s ", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // allow isakmp
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter IpsecPolicyInput_%s iifname %s ip protocol 17 udp dport 500 counter accept", ifName, ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // allow ah, esp
   // add rule ip filter INPUT iifname "eth4.1" ip protocol 50 counter accept
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter IpsecPolicyInput_%s iifname %s ip protocol {ah, esp} counter accept", ifName, ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // mark all esp/ah packets
   //add rule ip mangle PREROUTING iifname "eth4.1" ip protocol 50 counter meta mark set 0x10000000
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter IpsecPolicyMangle_%s iifname %s ip protocol {ah, esp} counter meta mark set 0x10000000", ifName, ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // marked non-esp packets are the output from 2.6 kernel ipsec code, accept
   // (ipsec output re-enters ip chains, mark is preserved when encapsulated ip packet in esp is extracted)

   // add rule ip filter INPUT iifname "eth4.1" ip protocol != 50 mark and 0x10000000 == 0x10000000 counter accept   
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter IpsecPolicyInput_%s iifname %s ip protocol != {ah, esp} mark and 0x10000000 == 0x10000000 counter accept", ifName, ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   snprintf(cmd, sizeof(cmd), "nft add rule inet filter IpsecPolicyFwd_%s iifname %s ip protocol != {ah, esp} mark and 0x10000000 == 0x10000000 counter accept", ifName, ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   return;
}

void rutIpt_removeIpSecPolicy(const char *ifName)
{
   char baseChainName[BUFLEN_64], usrChainName[BUFLEN_64];

   snprintf(usrChainName, sizeof(usrChainName), "IpsecPolicyInput_%s", ifName);
   snprintf(baseChainName, sizeof(baseChainName), "inet filter Input");
   remove_nft_user_chain(baseChainName, usrChainName);

   snprintf(usrChainName, sizeof(usrChainName), "IpsecPolicyFwd_%s", ifName);
   snprintf(baseChainName, sizeof(baseChainName), "inet filter Forward");
   remove_nft_user_chain(baseChainName, usrChainName);

   snprintf(usrChainName, sizeof(usrChainName), "IpsecPolicyMangle_%s", ifName);
   snprintf(baseChainName, sizeof(baseChainName), "inet filter Mangle");
   remove_nft_user_chain(baseChainName, usrChainName);

   return;
}

void rutIpt_initFw(const char *ifName, UBOOL8 isWan)
{
   char cmd[BUFLEN_256];

#ifdef SUPPORT_NF_NAT
   // add user chain 
   snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter FwInput_%s' 2>/dev/null", ifName);
   rut_doSystemAction("Fw", cmd); 
   
   // add rule to user chain
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwInput_%s iifname %s ct state related,established counter accept", ifName, ifName);
   rut_doSystemAction("Fw", cmd);

   // add rule - jump to user chain
   snprintf(cmd, sizeof(cmd), "nft insert rule inet filter Input jump FwInput_%s ", ifName);
   rut_doSystemAction("Fw", cmd);

   if (isWan)
   {
      // add user chain 
      snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter FwFwd_%s' 2>/dev/null", ifName);
      rut_doSystemAction("Fw", cmd); 

      // add rule to user chain
      snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwFwd_%s iifname %s ct state related,established counter accept", ifName, ifName);
      rut_doSystemAction("Fw", cmd);

      // add rule - jump to user chain
      snprintf(cmd, sizeof(cmd), "nft insert rule inet filter Forward jump FwFwd_%s ", ifName);
      rut_doSystemAction("Fw", cmd);
   }
#endif

#ifdef SUPPORT_TR69C
   /*
    * This rule should be added to the FirewallExceptions object under 
    * a specific WANIPConnection or WANPPPConnection.  But for now, just add it
    * to all WAN interfaces.
    */
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwInput_%s iifname %s tcp dport %d counter accept", ifName, ifName, TR69C_CONN_REQ_PORT);
   rut_doSystemAction("Fw", cmd);

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   /* for tr69c_2 */
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwInput_%s iifname %s tcp dport %d counter accept", ifName, ifName, TR69C_2_CONN_REQ_PORT);
   rut_doSystemAction("Fw", cmd);
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
#endif // SUPPORT_TR69C


#if defined(DMP_UDPECHO_1) || defined(DMP_DEVICE2_UDPECHO_1)
   {
      UBOOL8 enable = FALSE;
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      UINT32 port=0;
      char intfName[CMS_IFNAME_LENGTH]={0};
      UINT32 l;
      
      qdmDiag_getUdpEchoCfg(&enable,fullPathBuf,&port);

      
      if (enable)
      {
         if (fullPathBuf[0])
         {
            /* ensure fullpath ends with . before conversion (not necessary?) */
            l = strlen(fullPathBuf);
            if ((fullPathBuf[l-1]) != '.' && l < sizeof(fullPathBuf) -1)
            {
               fullPathBuf[l] = '.';
               fullPathBuf[l + 1] = '\0';
            }
            
            if (qdmIntf_fullPathToIntfnameLocked(fullPathBuf, intfName) == CMSRET_SUCCESS)
            {
               snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwInput_%s iifname %s udp dport %d counter accept", ifName, intfName, port);
            }
            else
            {
               cmsLog_error("Could not convert %s to intfName", fullPathBuf);
            }
         }
         else
         {
            snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwInput_%s udp dport %d counter accept", ifName, port);

         }

         rut_doSystemAction("initFirewall", cmd);   
      }
   }
#endif /* (DMP_UDPECHO_1) || define(DMP_DEVICE2_UDPECHO_1) */


   /* Log malicious packets right before dropping them
    * setup intrusion detection, logging 6 packets per hour to avoid sync log flooding
    */
 
   snprintf(cmd, sizeof(cmd), "nft 'add rule inet filter FwInput_%s iifname %s tcp flags & (fin|syn|rst|ack) == syn limit rate 6/hour burst 5 packets counter log prefix \"Intrusion -> \" level alert'", ifName, ifName);

   rut_doSystemAction("Fw", cmd);
   
   /* diable everything else */
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwInput_%s iifname %s counter drop", ifName, ifName);
   rut_doSystemAction("Fw", cmd);

   if (isWan)
   {
      snprintf(cmd, sizeof(cmd), "nft 'add rule inet filter FwFwd_%s iifname %s tcp flags & (fin|syn|rst|ack) == syn limit rate 6/hour burst 5 packets counter log prefix \"Intrusion -> \" level alert'", ifName, ifName);
      rut_doSystemAction("Fw", cmd);
   
      /* diable everything else */
      snprintf(cmd, sizeof(cmd), "nft add rule inet filter FwFwd_%s iifname %s counter drop", ifName, ifName);
      rut_doSystemAction("Fw", cmd);
   }

   return;
}

void rutIpt_removeFw(const char *ifName)
{
   char baseChainName[BUFLEN_64], usrChainName[BUFLEN_64];

   snprintf(usrChainName, sizeof(usrChainName), "FwInput_%s", ifName);
   snprintf(baseChainName, sizeof(baseChainName), "inet filter Input");
   remove_nft_user_chain(baseChainName, usrChainName);

   snprintf(usrChainName, sizeof(usrChainName), "FwFwd_%s", ifName);
   snprintf(baseChainName, sizeof(baseChainName), "inet filter Forward");
   remove_nft_user_chain(baseChainName, usrChainName);

   return;
}

#ifdef SUPPORT_UPNP
void rutIpt_upnpConfigStopMulticast(char *wanIfcName, UBOOL8 addRules)
{
   char cmd[BUFLEN_256];
   char baseChainName[BUFLEN_64], usrChainName[BUFLEN_64];
  
   if (!addRules)
   {
      snprintf(usrChainName, sizeof(usrChainName), "UpnpStopMc_%s", wanIfcName);
      snprintf(baseChainName, sizeof(baseChainName), "inet filter Output");
      remove_nft_user_chain(baseChainName, usrChainName);
   }
   else
   {
      snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter UpnpStopMc_%s' 2>/dev/null", wanIfcName);
      rut_doSystemAction("UpnpStopMc", cmd); 

      snprintf(cmd, sizeof(cmd), "nft 'add rule inet filter UpnpStopMc_%s oifname %s ip daddr %s counter drop' 2>/dev/null", wanIfcName, wanIfcName, UPNP_IP_ADDRESS);
      rut_doSystemAction("UpnpStopMc", cmd);

      // add rule - jump to user chain
      snprintf(cmd, sizeof(cmd), "nft insert rule inet filter Output jump UpnpStopMc_%s ", wanIfcName);
      rut_doSystemAction("Fw", cmd);      
   }   
}
#endif

void rutIpt_deleteTCPMSSRules(SINT32 domain, const char *ifName)
{
   char cmd[BUFLEN_128];

   /* remove TCP MSS option manipulation */
   snprintf(cmd, sizeof(cmd), "nft delete chain inet filter TcpMss_%s 2>/dev/null", ifName);
   rut_doSystemAction("TCPMSSRules", cmd);
}

void rutIpt_insertTCPMSSRules(SINT32 domain, const char *ifName)
{
   char cmd[BUFLEN_256];

   /* setup TCP MSS option manipulation */

   rutIpt_deleteTCPMSSRules(domain,ifName);

   snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter TcpMss_%s { type filter hook forward priority filter; policy accept; }' 2>/dev/null", ifName);
   rut_doSystemAction("TCPMSSRules", cmd); 

   snprintf(cmd, sizeof(cmd), "nft 'add rule inet filter TcpMss_%s iifname %s tcp flags & (syn|rst) == syn counter tcp option maxseg size set rt mtu' 2>/dev/null", ifName, ifName);
   rut_doSystemAction("TCPMSSRules", cmd);   
   snprintf(cmd, sizeof(cmd), "nft 'add rule inet filter TcpMss_%s oifname %s tcp flags & (syn|rst) == syn counter tcp option maxseg size set rt mtu' 2>/dev/null", ifName, ifName);
   rut_doSystemAction("TCPMSSRules", cmd);
}

// GUI - Security - IpFilter
void rutIpt_doFirewallExceptionRule_dev2(const _Dev2FirewallExceptionRuleObject *InObj, const char *ifName, UBOOL8 add)
{
   char src[BUFLEN_64], dst[BUFLEN_64];
   char sport[BUFLEN_40], dport[BUFLEN_40], protocol[BUFLEN_32],target[BUFLEN_32];
   char sport2[BUFLEN_40], dport2[BUFLEN_40];
   char cmd[BUFLEN_512];
   char chainName[BUFLEN_64];
   char tcpOrUdp = 0; // 0: none, 1: tcp, 2:udp, 3:tcp or udp

   src[0] = dst[0] = sport[0] = dport[0] = protocol[0] = target[0] = '\0';

   // delete existing ipFilter rule
   if (cmsUtl_strstr(ifName, "br") != NULL)
   {
      snprintf(chainName, sizeof(chainName), "inet filter Forward");
   } 
   else
   {
      snprintf(chainName, sizeof(chainName), "inet filter FwInput_%s", ifName);
      remove_nft_user_chain(chainName, InObj->filterName);
      snprintf(chainName, sizeof(chainName), "inet filter FwFwd_%s", ifName);
   }   

   remove_nft_user_chain(chainName, InObj->filterName);
   
   if (!add)
      return; 

   // if (atoi((InObj)->IPVersion) == 6) ??

   /* protocol */
   if ( cmsUtl_strcmp(InObj->protocol, MDMVS_TCP) == 0 ) 
   {
      snprintf(protocol, sizeof(protocol), "ip protocol tcp");
      tcpOrUdp = 1;
   }
   else if ( cmsUtl_strcmp(InObj->protocol, MDMVS_UDP) == 0 ) 
   {
      tcpOrUdp = 2;
      snprintf(protocol, sizeof(protocol), "ip protocol udp");
   }
   else if ( cmsUtl_strcmp(InObj->protocol, MDMVS_TCP_OR_UDP) == 0 ) 
   {
      tcpOrUdp = 3;
   }
   else if ( cmsUtl_strcmp(InObj->protocol, MDMVS_ICMP) == 0 ) 
   {
      snprintf(protocol, sizeof(protocol), "ip protocol icmp");
   }
   else if ( cmsUtl_strcmp(InObj->protocol, MDMVS_ICMPV6) == 0 ) 
   {
      snprintf(protocol, sizeof(protocol), "ip portocol icmpv6");
   }
   else
   {
      // None
      protocol[0] = '\0';
   }

   /* source address/mask */
   if ( cmsUtl_strcmp(InObj->sourceIPAddress, "\0") != 0 ) 
   {
      if (strchr(InObj->sourceIPAddress, ':') == NULL)
      {
         /* IPv4 address */
         if ( cmsUtl_strcmp(InObj->sourceNetMask, "\0") != 0 )
            snprintf(src, sizeof(src), "ip saddr %s/%d", InObj->sourceIPAddress, cmsNet_getLeftMostOneBitsInMask(InObj->sourceNetMask));
         else
            snprintf(src, sizeof(src), "ip saddr %s", InObj->sourceIPAddress);
      }
      else
      {
         /* IPv6 address */
         snprintf(src, sizeof(src), "ip6 saddr %s", InObj->sourceIPAddress);
      }
   }

   /* destination address/mask */
   if ( cmsUtl_strcmp(InObj->destinationIPAddress, "\0") != 0 ) 
   {
      if (strchr(InObj->destinationIPAddress, ':') == NULL)
      {
         /* IPv4 address */
      if ( cmsUtl_strcmp(InObj->destinationNetMask, "\0") != 0 )
         snprintf(dst, sizeof(dst), "ip daddr %s/%d", InObj->destinationIPAddress, cmsNet_getLeftMostOneBitsInMask(InObj->destinationNetMask));
      else
         snprintf(dst, sizeof(dst), "ip daddr %s", InObj->destinationIPAddress);
   }
      else
      {
         /* IPv6 address */
         snprintf(dst, sizeof(dst), "ip6 daddr %s", InObj->destinationIPAddress);
      }
   }

   /* source port */
   if ( InObj->sourcePortStart != 0)
   {
      if (tcpOrUdp == 1) {// tcp
         if ( InObj->sourcePortEnd != 0) {
            snprintf(sport, sizeof(sport), "tcp sport {%d-%d}", InObj->sourcePortStart, InObj->sourcePortEnd);
         }
         else {
            snprintf(sport, sizeof(sport), "tcp sport %d", InObj->sourcePortStart);
         }
      }
      else if(tcpOrUdp == 2) {// udp
         if ( InObj->sourcePortEnd != 0) {
            snprintf(sport, sizeof(sport), "udp sport {%d-%d}", InObj->sourcePortStart, InObj->sourcePortEnd);
         }
         else {
            snprintf(sport, sizeof(sport), "udp sport %d", InObj->sourcePortStart);
         }
      }
      else if(tcpOrUdp == 3) {// tcp or udp
         if ( InObj->sourcePortEnd != 0) {
            snprintf(sport, sizeof(sport), "tcp sport {%d-%d}", InObj->sourcePortStart, InObj->sourcePortEnd);
            snprintf(sport2, sizeof(sport2), "udp sport {%d-%d}", InObj->sourcePortStart, InObj->sourcePortEnd);
         }
         else {
            snprintf(sport, sizeof(sport), "tcp sport %d", InObj->sourcePortStart);
            snprintf(sport2, sizeof(sport2), "udp sport %d", InObj->sourcePortStart);
         }
      }
   }

   /* destination port */
   if ( InObj->destinationPortStart != 0)
   {
      if (tcpOrUdp == 1) {// tcp
         if ( InObj->destinationPortEnd != 0) {
            snprintf(dport, sizeof(dport), "tcp dport {%d-%d}", InObj->destinationPortStart, InObj->destinationPortEnd);
         }
         else
         {
            snprintf(dport, sizeof(dport), "tcp dport %d", InObj->destinationPortStart);
         }
      }
      else if (tcpOrUdp == 2) {// udp
         if ( InObj->destinationPortEnd != 0) {
            snprintf(dport, sizeof(dport), "udp dport {%d-%d}", InObj->destinationPortStart, InObj->destinationPortEnd);
         }
         else
         {
            snprintf(dport, sizeof(dport), "udp dport %d", InObj->destinationPortStart);
         }
      }
      else if (tcpOrUdp == 3) {// tcp or udp
         if ( InObj->destinationPortEnd != 0) {
            snprintf(dport, sizeof(dport), "tcp dport {%d-%d}", InObj->destinationPortStart, InObj->destinationPortEnd);
            snprintf(dport2, sizeof(dport2), "udp dport {%d-%d}", InObj->destinationPortStart, InObj->destinationPortEnd);
         }
         else
         {
            snprintf(dport, sizeof(dport), "tcp dport %d", InObj->destinationPortStart);
            snprintf(dport2, sizeof(dport2), "udp dport %d", InObj->destinationPortStart);
         }
      }
   }

   /* Target */
   if ( cmsUtl_strcmp(InObj->target, MDMVS_ACCEPT) == 0 ) 
   {
      snprintf(target, sizeof(target), "counter accept");
   }
   else
   {
      snprintf(target, sizeof(target), "counter drop");
   }

   // add ipFilter chain
   snprintf(cmd, sizeof(cmd), "nft 'add chain inet filter %s' 2>/dev/null", InObj->filterName);
   rut_doSystemAction("rutIpt_doFirewallExceptionRule_dev2", cmd);

   // add rule in Fwd - jump to user chain
   snprintf(cmd, sizeof(cmd), "nft insert rule %s jump %s ", chainName, InObj->filterName);
   rut_doSystemAction("rutIpt_doFirewallExceptionRule_dev2", cmd);

   // add rule to user chain
   snprintf(cmd, sizeof(cmd), "nft add rule inet filter %s iifname %s %s %s %s %s %s %s 2>/dev/null", InObj->filterName, ifName, protocol, src, dst, sport, dport, target);
   rut_doSystemAction("rutIpt_doFirewallExceptionRule_dev2", cmd);

   if (tcpOrUdp == 3) // tcp or udp
   {
      snprintf(cmd, sizeof(cmd), "nft add rule inet filter %s iifname %s %s %s %s %s %s %s 2>/dev/null", InObj->filterName, ifName, protocol, src, dst, sport2, dport2, target);
      rut_doSystemAction("rutIpt_doFirewallExceptionRule_dev2", cmd);
   }

   /* 
         br incoming drop rule mean it's a "rule for block traffic outgoing to wan". 
         So "outgoing rule" (br incoming drop rule) just add on forward chain only
         Because the traffic destination may be localhost and shouldn't be block via input rule.
         For ex: block web access to wan , but still need access on CPE web
   */   

   if (
        (cmsUtl_strcmp(target, "counter accept") == 0)
        || ((cmsUtl_strcmp(target, "counter accept") != 0) && (cmsUtl_strstr(ifName, "br") == NULL))  //non- br incoming drop rule
   )
   {
      // add rule in Input - jump to user chain
      snprintf(cmd, sizeof(cmd), "nft insert rule inet filter FwInput_%s jump %s ", ifName, InObj->filterName);
      rut_doSystemAction("rutIpt_doFirewallExceptionRule_dev2", cmd);      
   }   
   return;
}

// GUI - Security - MacFilter rule
void rutEbt_changeMacFilterPolicy(const char *ifName, UBOOL8 isForward)
{
   char cmd[BUFLEN_256];
   char baseChainName[BUFLEN_64], usrChainName[BUFLEN_64];

   if (!isModuleInserted("nf_tables")) {
      rutNft_insertModules();
   }

   // remove DefaultMacFilter
   snprintf(usrChainName, sizeof(usrChainName), "DefaultMacFilter_%s", ifName);
   snprintf(baseChainName, sizeof(baseChainName), "bridge filter Forward");
   remove_nft_user_chain(baseChainName, usrChainName);


   /* If the policy is BLOCKED, we need to block all traffic of the interface */
   if (!isForward)
   {
      // add user chain
      snprintf(cmd, sizeof(cmd), "nft 'add chain bridge filter %s' 2>/dev/null", usrChainName);
      rut_doSystemAction("MacFilter", cmd);

      // attach user chain to base chain
      snprintf(cmd, sizeof(cmd), "nft add rule %s jump %s ", baseChainName, usrChainName);
      rut_doSystemAction("MacFilterRaw", cmd);

      // add rule
      snprintf(cmd, sizeof(cmd), "nft 'add rule bridge filter %s iifname %s counter drop' 2>/dev/null", usrChainName, ifName);
      rut_doSystemAction("MacFilter", cmd);

      snprintf(cmd, sizeof(cmd), "nft 'add rule bridge filter %s oifname %s counter drop' 2>/dev/null", usrChainName, ifName);
      rut_doSystemAction("MacFilter", cmd);
   }
}

void rutEbt_executeMacFilterRule(char *cmd, char *cmd2, const char *policy)
{
   char policyStr[BUFLEN_264];

   if ( cmsUtl_strcmp(policy, MDMVS_FORWARD) == 0 )
      strcpy(policyStr, " counter drop");
   else
      strcpy(policyStr, " counter accept");

   strcat(cmd, policyStr);
   rut_doSystemAction("rutEbt_executeMacFilterRule", cmd);

   if (cmd2[0] != '\0') 
   {
      strcat(cmd2, policyStr);  
      rut_doSystemAction("rutEbt_executeMacFilterRule", cmd2);
   }
}

void rutEbt_addMacFilter_raw(char* protocol,char* direction ,char* sourceMAC
                   ,char* destinationMAC,const char *ifName, const char *policy, UBOOL8 add) 
{
   char cmd[BUFLEN_512], cmd2[BUFLEN_512], cmdStr[BUFLEN_512];
   char directionStr[BUFLEN_128];
   char chainName[BUFLEN_128], srcStr[BUFLEN_128], dstStr[BUFLEN_128];
   char buf[BUFLEN_1024];
   UINT32 crc;
   char baseChainName[BUFLEN_64], usrChainName[BUFLEN_64];

   cmd[0] = cmd2[0] = directionStr[0] = cmdStr[0] = chainName[0] = srcStr[0] = dstStr[0] = buf[0] = '\0';   

   snprintf(chainName, sizeof(chainName), "%s_%s_%s_%s", protocol, direction, sourceMAC, destinationMAC); 
   crc = genUtl_getCrc32((UINT8 *) chainName, strlen(chainName), CRC_INITIAL_VALUE);

   // remove user chain
   snprintf(usrChainName, sizeof(usrChainName), "MacFilter_%u", crc);
   snprintf(baseChainName, sizeof(baseChainName), "bridge filter Forward");
   remove_nft_user_chain(baseChainName, usrChainName);

   if ( add ) 
   {
      // add user chain
      snprintf(cmd, sizeof(cmd), "nft add chain bridge filter %s", usrChainName);
      rut_doSystemAction("MacFilterRaw", cmd);

      // attach user chain to base chain
      snprintf(cmd, sizeof(cmd), "nft insert rule %s jump %s ", baseChainName, usrChainName);
      rut_doSystemAction("MacFilterRaw", cmd);
   }
   else
   {
      return;
   }

   // src / dst MAC
   if (cmsUtl_strcmp(sourceMAC, "\0")) 
   {
      snprintf(srcStr, sizeof(srcStr), "ether saddr %s", sourceMAC);
   }

   if (cmsUtl_strcmp(destinationMAC, "\0")) 
   {
      snprintf(dstStr, sizeof(dstStr), "ether daddr %s", destinationMAC);
   }

   // protocol
   if (!cmsUtl_strcmp(protocol, MDMVS_NONE))
   {
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_PPPOE))
   {
      // PPP_DISC        8863                    # PPPoE discovery messages
      // PPP_SES         8864                    # PPPoE session messages
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s meta protocol 0x8863 %s %s", usrChainName, srcStr, dstStr);
      snprintf(cmd2, sizeof(cmd2), "nft add rule bridge filter %s meta protocol 0x8864 %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPV4))
   {
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s meta protocol arp %s %s", usrChainName, srcStr, dstStr);
      snprintf(cmd2, sizeof(cmd2), "nft add rule bridge filter %s meta protocol ip %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPV6))
   {
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s meta protocol ip6 %s %s", usrChainName, srcStr, dstStr);

      /* ICMPv6 neighbour-solicitation message used */
      if (!cmsUtl_strcmp(policy, MDMVS_BLOCKED))
         snprintf(cmd2, sizeof(cmd2), "nft add rule bridge filter %s icmpv6 type nd-neighbor-solicit %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_APPLETALK))
   {
      // ATALK           809B                    # Appletalk
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s meta protocol 0x809b %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPX))
   {
      // IPX             8137                    # Novell IPX
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s meta protocol 0x8137 %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_NETBEUI))
   {
      // NetBEUI         8191                    # NetBEUI
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s meta protocol 0x8191 %s %s", usrChainName, srcStr, dstStr);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IGMP))
   {
      snprintf(cmd, sizeof(cmd), "nft add rule bridge filter %s ip protocol 2 %s %s", usrChainName, srcStr, dstStr);
   }

   if (!cmsUtl_strcmp(direction, MDMVS_LAN_TO_WAN))
   {
      strcpy(directionStr, " oifname ");
   }
   else if (!cmsUtl_strcmp(direction, MDMVS_WAN_TO_LAN))
   {
      strcpy(directionStr, " iifname ");
   }
   else if (!cmsUtl_strcmp(direction, MDMVS_BOTH))
   {
      char cmdSave[BUFLEN_512], cmd2Save[BUFLEN_512];
      strcpy(cmdSave, cmd);
      strcpy(cmd2Save, cmd2);
      strcat(cmd, "oifname ");
      strcat(cmd, ifName);
      if (cmd2[0] != '\0') 
      {
         strcat(cmd2, " oifname ");
         strcat(cmd2, ifName);
      }
      rutEbt_executeMacFilterRule(cmd, cmd2, policy);
      strcat(cmdSave, " iifname ");
      strcat(cmdSave, ifName);
      if (cmd2Save[0] != '\0') 
      {
         strcat(cmd2Save, " iifname ");
         strcat(cmd2Save, ifName);
      }
      rutEbt_executeMacFilterRule(cmdSave, cmd2Save, policy);
      return;
   }

   strcat(cmd, directionStr);
   if (cmd2[0] != '\0')
      strcat(cmd2, directionStr);

   strcat(cmd, ifName);
   if (cmd2[0] != '\0')
      strcat(cmd2, ifName);
   rutEbt_executeMacFilterRule(cmd, cmd2, policy);
}

#endif

