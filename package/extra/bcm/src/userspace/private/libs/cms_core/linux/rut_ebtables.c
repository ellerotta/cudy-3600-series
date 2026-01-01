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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_ether.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ebtables.h"

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
#ifndef SUPPORT_NF_TABLES
static void rutEbt_executeMacFilterRule(char *cmd, char *cmd2, const char *policy);
#endif
#endif

#ifdef DMP_BASELINE_1
/*
 * The next few functions look at the TR98 data model, so can only be used
 * in Legacy TR98 and Hybrid TR98+TR181 modes.
 */
UBOOL8 rut_isBridgedWanExisted()
{
   UBOOL8 exist = FALSE;
   CmsRet ret;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *wanIpConn = NULL;

   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanIpConn)) == CMSRET_SUCCESS)
   {
      if (strcmp(wanIpConn->connectionType, MDMVS_IP_BRIDGED) == 0)
      {
         exist = TRUE;
         cmsObj_free((void **) &wanIpConn);
         break;
      }
      cmsObj_free((void **) &wanIpConn);
   }
   
   return exist;
}

UBOOL8 rut_isRoutedWanExisted()
{
   UBOOL8 exist = FALSE;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *wan_ppp_con = NULL;
   _WanIpConnObject *wan_ip_con = NULL;

   while (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack1, OGF_NO_VALUE_UPDATE, (void **) &wan_ip_con) == CMSRET_SUCCESS)
   {
      if ( strcmp(wan_ip_con->connectionType, MDMVS_IP_BRIDGED) != 0 ) 
      {
         cmsObj_free((void **) &wan_ip_con);    
         return TRUE;
      }

      cmsObj_free((void **) &wan_ip_con);        
   }

   if ( cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack2, OGF_NO_VALUE_UPDATE, (void **) &wan_ppp_con) == CMSRET_SUCCESS ) 
   {
      cmsObj_free((void **) &wan_ppp_con);
      return TRUE;
   }

    return exist;
}

#endif  /* DMP_BASELINE_1 */


void rut_accessTimeRestriction(const _AccessTimeRestrictionObject *Obj, const UBOOL8 add)
{
   char eb_options[BUFLEN_1024];
   char cmd[BUFLEN_1024+BUFLEN_64];

   eb_options[0] = cmd[0] = '\0';
   
   cmsLog_debug("setting info: %s/%s/%s/%s/%s with add = %d", Obj->username, Obj->days, Obj->startTime, Obj->endTime, Obj->MACAddress, add);
   sprintf(eb_options, "--timestart %s --timestop %s -s %s --days %s", Obj->startTime, Obj->endTime, Obj->MACAddress, Obj->days);  

   strncat( eb_options, " -j DROP", sizeof( eb_options )-1 );

   /* This needs to be BridgeMode */
   if ( qdmIpIntf_isBridgedWanExistedLocked() )
   {
      sprintf( cmd, "ebtables --concurrent -%c FORWARD %c %s", add?'I':'D', add?'1':' ', eb_options );     
      rut_doSystemAction( "rut_accessTimeRestriction", cmd );
   }
   
   if ( qdmIpIntf_isRoutedWanExistedLocked() )
   {
      sprintf( cmd, "ebtables --concurrent -%c INPUT %c %s", add?'I':'D', add?'1':' ', eb_options );   
      rut_doSystemAction( "rut_accessTimeRestriction", cmd );
   }

}


void rutEbt_avoidDhcpAcceleration(void)
{
   char cmd[BUFLEN_256];

   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -D FORWARD -p ip --ip-protocol 17 --ip-destination-port 68 -j SKIPLOG 2>/dev/null");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -A FORWARD -p ip --ip-protocol 17 --ip-destination-port 68 -j SKIPLOG");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -D FORWARD -p ip --ip-destination 255.255.255.255 -j SKIPLOG 2>/dev/null");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -A FORWARD -p ip --ip-destination 255.255.255.255 -j SKIPLOG");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   return;
}


#ifdef DMP_X_BROADCOM_COM_SECURITY_1
#ifndef SUPPORT_NF_TABLES
void rutEbt_changeMacFilterPolicy(const char *ifName, UBOOL8 isForward)
{
   char cmd[BUFLEN_256];

   /* If the policy is BLOCKED, we need to block all traffic of the interface */
   if (!isForward)
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -D FORWARD -i %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -A FORWARD -i %s -j DROP", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -D FORWARD -o %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -A FORWARD -o %s -j DROP", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
   }
   else
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -D FORWARD -i %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -D FORWARD -o %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
   }
}

void rutEbt_addMacFilter_raw(char* protocol,char* direction ,char* sourceMAC
                   ,char* destinationMAC,const char *ifName, const char *policy, UBOOL8 add) 
{
   char action = 'A', rulenum = ' ';
   char cmd[BUFLEN_128], cmd2[BUFLEN_128], cmdStr[BUFLEN_128];
   char directionStr[BUFLEN_128];

   if ( add ) 
   {
      action = 'I';
      rulenum = '1';
   }
   else
      action = 'D';

   cmd[0] = cmd2[0] = directionStr[0] = cmdStr[0] = '\0';   
      
   if (!cmsUtl_strcmp(protocol, MDMVS_NONE))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_PPPOE))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p PPP_DISC ", action, rulenum);
      snprintf(cmd2, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p PPP_SES ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPV4))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p ARP ", action, rulenum);
      snprintf(cmd2, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p IPv4 ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPV6))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p IPv6 ", action, rulenum);
      /* ICMPv6 neighbour-solicitation message used */
      if (!cmsUtl_strcmp(policy, MDMVS_BLOCKED))
          snprintf(cmd2, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p IPv6 --ip6-protocol 58 --ip6-icmp-type 135 ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_APPLETALK))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p ATALK ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPX))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p IPX ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_NETBEUI))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p NetBEUI ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IGMP))
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -%c FORWARD %c -p IPv4 --ip-proto 2 ", action, rulenum);
   }

   if (cmsUtl_strcmp(destinationMAC, "\0")) 
   {
      strcat(cmd, "-d ");
      strncat(cmd, destinationMAC, sizeof(cmd)-1);
      cmd[sizeof(cmd)-1] = '\0';        
      strcat(cmd, " ");
      if (cmd2[0] != '\0' && cmsUtl_strcmp(protocol, MDMVS_IPV6))
      {
         strcat(cmd2, "-d ");
         strncat(cmd2, destinationMAC, sizeof(cmd2)-1);
         cmd2[sizeof(cmd2)-1] = '\0';         
         strcat(cmd2, " ");
      }
   }
   
   if (cmsUtl_strcmp(sourceMAC, "\0")) 
   {
      strcat(cmd, "-s ");
      strncat(cmd, sourceMAC, sizeof(cmd)-1);
      cmd[sizeof(cmd)-1] = '\0';        
      strcat(cmd, " ");
      if (cmd2[0] != '\0') 
      {
         strcat(cmd2, "-s ");
         strncat(cmd2, sourceMAC, sizeof(cmd2)-1);
         cmd2[sizeof(cmd2)-1] = '\0';           
         strcat(cmd2, " ");
      }
   }

   if (!cmsUtl_strcmp(direction, MDMVS_LAN_TO_WAN))
   {
      strcpy(directionStr, "-o ");
   }
   else if (!cmsUtl_strcmp(direction, MDMVS_WAN_TO_LAN))
   {
      strcpy(directionStr, "-i ");
   }
   else if (!cmsUtl_strcmp(direction, MDMVS_BOTH))
   {
      char cmdSave[BUFLEN_128], cmd2Save[BUFLEN_128];
      strcpy(cmdSave, cmd);
      strcpy(cmd2Save, cmd2);
      strcat(cmd, "-o ");
      strncat(cmd, ifName, sizeof(cmd)-1);
      cmd[sizeof(cmd)-1] = '\0';      
      if (cmd2[0] != '\0') 
      {
         strcat(cmd2, "-o ");
         strncat(cmd2, ifName, sizeof(cmd2)-1);
         cmd2[sizeof(cmd2)-1] = '\0';           
      }
      rutEbt_executeMacFilterRule(cmd, cmd2, policy);
      strcat(cmdSave, "-i ");
      strncat(cmdSave, ifName, sizeof(cmdSave)-1);
      cmdSave[sizeof(cmdSave)-1] = '\0';        
      if (cmd2Save[0] != '\0') 
      {
         strcat(cmd2Save, "-i ");
         strncat(cmd2Save, ifName, sizeof(cmd2Save)-1);
         cmd2Save[sizeof(cmd2Save)-1] = '\0';             
      }
      rutEbt_executeMacFilterRule(cmdSave, cmd2Save, policy);
      return;
   }

   strcat(cmd, directionStr);
   if (cmd2[0] != '\0')
      strcat(cmd2, directionStr);

   strncat(cmd, ifName, sizeof(cmd)-1);
   cmd[sizeof(cmd)-1] = '\0';     
   
   if (cmd2[0] != '\0')
   {
      strncat(cmd2, ifName, sizeof(cmd2)-1);
      cmd2[sizeof(cmd2)-1] = '\0';   
   }
   rutEbt_executeMacFilterRule(cmd, cmd2, policy);
}
#endif

void rutEbt_addMacFilter(const _MacFilterCfgObject *InObj, const char *ifName, const char *policy, UBOOL8 add) 
{
   rutEbt_addMacFilter_raw(
       InObj->protocol
      ,InObj->direction
      ,InObj->sourceMAC
      ,InObj->destinationMAC
      ,ifName, policy, add);
}

#ifndef SUPPORT_NF_TABLES
void rutEbt_executeMacFilterRule(char *cmd, char *cmd2, const char *policy)
{
   char policyStr[BUFLEN_264];

   if ( cmsUtl_strcmp(policy, MDMVS_FORWARD) == 0 )
      strcpy(policyStr, " -j DROP");
   else
      strcpy(policyStr, " -j ACCEPT");

   strcat(cmd, policyStr);
   rut_doSystemAction("rutEbt_executeMacFilterRule", cmd);
   if (cmd2[0] != '\0') 
   {
      strcat(cmd2, policyStr);  
      rut_doSystemAction("rutEbt_executeMacFilterRule", cmd2);
   }
}
#endif // SUPPORT_NF_TABLES

#endif  /* DMP_X_BROADCOM_COM_SECURITY_1 */


#ifndef SUPPORT_NF_TABLES
void rutEbt_defaultLANSetup6(void)
{
    char line[BUFLEN_512];
    char tblfile[BUFLEN_16];
    FILE* fs = NULL;
    UBOOL8 found = FALSE;
    
    sprintf(tblfile, "/var/tmp_file");
    snprintf(line, sizeof(line), "ebtables -t nat -L > %s", tblfile);
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);

    if ((fs = fopen(tblfile, "r")) != NULL) 
    {
      while (fgets(line, sizeof(line), fs)) 
      {
         if (cmsUtl_strstr(line, "brchain") != NULL)
         {
            found = TRUE;
            break;
         }
      }
    }
    
    if(fs != NULL)
    {
        fclose(fs);
        unlink(tblfile);
    }
    
    if (found)
        return;
    
    snprintf(line, sizeof(line), "ebtables --concurrent -t nat -N brchain 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);
    
    snprintf(line, sizeof(line), "ebtables --concurrent -t nat -I PREROUTING -j brchain 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);
    
    
    snprintf(line, sizeof(line), "ebtables --concurrent -t nat -I brchain -p ipv6 --pkttype-type host --ip6-protocol 58 --ip6-icmp-type 128 -j REJECT --reject-with 0 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);

    snprintf(line, sizeof(line), "ebtables --concurrent -t nat -I brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 --ip6-src fe80::/16 -j RETURN 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);
}
#else
void rutEbt_defaultLANSetup6(void)
{
    char line[BUFLEN_512];
    char tblfile[BUFLEN_16];
    FILE* fs = NULL;
    UBOOL8 found = FALSE;
    
    sprintf(tblfile, "/var/tmp_file");
    snprintf(line, sizeof(line), "nft list ruleset bridge > %s", tblfile);
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);

    if ((fs = fopen(tblfile, "r")) != NULL) 
    {
      while (fgets(line, sizeof(line), fs)) 
      {
         if (cmsUtl_strstr(line, "brchain") != NULL)
         {
            found = TRUE;
            break;
         }
      }
    }
    
    if(fs != NULL)
    {
        fclose(fs);
        unlink(tblfile);
    }
    
    if (found)
        return;
        
   rut_doSystemAction("rut", "nft 'add chain bridge br_filter brchain { type filter hook prerouting priority filter; policy accept; }'");
   rut_doSystemAction("rut", "nft 'add rule bridge br_filter brchain ip6 saddr fe80::/16 icmpv6 type {echo-request} return'");
   rut_doSystemAction("rut", "nft 'add rule bridge br_filter brchain pkttype host icmpv6 type {echo-request} reject with icmpv6 type 0'");
}
#endif /* SUPPORT_NF_TABLES */
void rutEbt_configIPv6Prefix(const char *prefix, UBOOL8 add)
{
    char line[BUFLEN_512];
        
    cmsLog_debug("prefix=%s, add<%d>", prefix, add);
    if (prefix == NULL)
        return;
    
    if (strlen(prefix) == 0)
        return;

    /* WAN may be activated multiple times (like RA+PD mode), 
       to avoid adding the same rule multiple times, do the delete action unconditionally before adding.
    */
    snprintf(line, sizeof(line), "ebtables --concurrent -t nat -D brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 --ip6-src %s -j RETURN 2>/dev/null", prefix);
    rut_doSystemAction("rutEbt_configIPv6Prefix", line);  

    if (add)
    {
        snprintf(line, sizeof(line), "ebtables --concurrent -t nat -I brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 --ip6-src %s -j RETURN 2>/dev/null", prefix);
        rut_doSystemAction("rutEbt_configIPv6Prefix", line);
    }
}

void rutEbt_IPv6PrefixExceptionIf(const char *ifName, UBOOL8 add)
{
    char line[BUFLEN_512];
    char action = 'I';
    
    if (ifName == NULL)
        return;

    if (!add)
        action = 'D';
    
    snprintf(line, sizeof(line), "ebtables --concurrent -t nat -%c brchain -i %s -p ipv6 -j RETURN 2>/dev/null", action, ifName);
    
    rut_doSystemAction("rutEbt_IPv6PrefixExceptionIf", line);
}

