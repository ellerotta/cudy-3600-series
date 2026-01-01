/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
#ifdef SUPPORT_MAP

#include "cms_core.h"
#include "rut2_map.h"

CmsRet rutMap_getDomainInfo(const InstanceIdStack *ruleiidStack, char *mechanism, char *brprefix, char *wanintf,
                            UBOOL8 *firewall, UINT32 *psidOffset, UINT32 *psidLen, UINT32 *psid)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack;
   Dev2MapDomainObject *domain = NULL;

   iidStack = *ruleiidStack;
   if (cmsObj_getAncestorFlags(MDMOID_DEV2_MAP_DOMAIN, MDMOID_DEV2_MAP_DOMAIN_RULE,
                               &iidStack, OGF_NO_VALUE_UPDATE,
                               (void **) &domain) != CMSRET_SUCCESS )
   {
      cmsLog_error("Cannot get domain obj");
      return CMSRET_MDM_TREE_ERROR;
   }

   cmsUtl_strncpy(mechanism, domain->transportMode, BUFLEN_24);
   cmsUtl_strncpy(brprefix, domain->BRIPv6Prefix, BUFLEN_32);
   qdmIntf_getIntfnameFromFullPathLocked_dev2(domain->WANInterface, wanintf, CMS_IFNAME_LENGTH);
   *firewall = qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(domain->WANInterface);
   *psidOffset = domain->PSIDOffset;
   *psidLen = domain->PSIDLength;
   *psid = domain->PSID;

   cmsObj_free((void **) &domain);

   return ret;
}

CmsRet rutMap_parseDomainRuleInfo(const char *IPv6Prefix, const char *IPv4Prefix, UINT32 eaLen, UINT32 psidLen, UINT32 psid,
                                  char *ipv6AddrBuf, char *subnetCidr4, char *addr, UINT32 *ipv4MaskLen)
{
   CmsRet ret = CMSRET_SUCCESS;
   char addr6[CMS_IPADDR_LENGTH];
   UINT32 i, plen, tmpMask=0;
   struct in6_addr in6Addr;
   struct in_addr in4Addr, tmpAddr4;

   if (cmsUtl_parsePrefixAddress(IPv4Prefix, addr, ipv4MaskLen) != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid ipv4 address=%s", IPv4Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (inet_pton(AF_INET, addr, &in4Addr) <= 0)
   {
      cmsLog_error("Invalid ipv4 address=%s", IPv4Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (psidLen != (eaLen + *ipv4MaskLen - 32))
   {
      cmsLog_error("psidLen != (eaLen + ipv4PrefixLen - 32)");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   for (i = 0; i < *ipv4MaskLen; i++)
   {
      tmpMask |= 1 << i;
   }

   tmpMask = (int)htonl(tmpMask << (32 - *ipv4MaskLen));
   cmsLog_debug("tmpMask= %x", tmpMask);

   /* get rule-ipv4_prefix, ex:10.0.0.0 */

   /* 
    * in4Addr from inet_pton is in network order, so tmpMask needs to be in
    * network order too. so tmpAddr4 will be in network order.
    */
   tmpAddr4.s_addr = ((int)in4Addr.s_addr & tmpMask);
   inet_ntop(AF_INET, &tmpAddr4, subnetCidr4, INET_ADDRSTRLEN);
   cmsLog_debug("tmpAddr4= %s %x\n", subnetCidr4, (int)tmpAddr4.s_addr);

   {
      char tmp[BUFLEN_8];
      snprintf(tmp, sizeof(tmp), "/%d", *ipv4MaskLen); /* Rule prefix */
      cmsUtl_strncat(subnetCidr4, CMS_IPADDR_LENGTH, tmp);
   }

   /* Create prefix information for the MAP address */
   if (cmsUtl_parsePrefixAddress(IPv6Prefix, addr6, &plen) != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid ipv6 address=%s", IPv6Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (inet_pton(AF_INET6, addr6, &in6Addr) <= 0)
   {
      cmsLog_error("Invalid ipv6 address=%s", IPv6Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   /* get map-ipv6_address */

   {
      int pbw0, pbi0, pbi1;
      UINT32 psidMask;

      if (*ipv4MaskLen < 32) {
         pbw0 = plen >> 5;
         pbi0 = plen & 0x1f;
         in6Addr.s6_addr32[pbw0] |= htonl((ntohl(in4Addr.s_addr) << *ipv4MaskLen) >> pbi0);
         pbi1 = pbi0 - *ipv4MaskLen;
         if (pbi1 > 0)
            in6Addr.s6_addr32[pbw0+1] |= htonl(ntohl(in4Addr.s_addr) << (32 - pbi1));
      }

      if (psidLen > 0) {
         psidMask = (1 << psidLen) - 1;
         pbw0 = (plen + 32 - *ipv4MaskLen) >> 5;
         pbi0 = (plen + 32 - *ipv4MaskLen) & 0x1f;
         in6Addr.s6_addr32[pbw0] |= htonl(((psid & psidMask) << (32 - psidLen)) >> pbi0);
         pbi1 = pbi0 - (32 - psidLen);
         if (pbi1 > 0)
            in6Addr.s6_addr32[pbw0+1] |= (psid & psidMask) << (32 - pbi1);
      }

      /* IID translation: RFC 7597 Section 6 Figure 8 */
      in6Addr.s6_addr32[2] |= htonl(ntohl(in4Addr.s_addr) >> 16);
      in6Addr.s6_addr32[3] |= htonl(ntohl(in4Addr.s_addr) << 16);
      in6Addr.s6_addr32[3] |= htonl(psid);
   }

   inet_ntop(AF_INET6, &in6Addr, ipv6AddrBuf, INET6_ADDRSTRLEN);
   cmsLog_debug("tmpAddr6= %s", ipv6AddrBuf);

   return ret;
}

CmsRet rutMap_parseDhcp6cInfo(const char *sitePrefix, const char *IPv6Prefix, char *IPv4Prefix, UINT32 eaLen, UINT32 psidLen)
{
   CmsRet ret = CMSRET_SUCCESS;
   char addr[CMS_IPADDR_LENGTH];
   char addr6[CMS_IPADDR_LENGTH];
   UINT32 plen, ipv4MaskLen;
   struct in6_addr in6Addr;
   struct in_addr in4Addr;

   if (cmsUtl_parsePrefixAddress(IPv4Prefix, addr, &ipv4MaskLen) != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid ipv4 address=%s", IPv4Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (inet_pton(AF_INET, addr, &in4Addr) <= 0)
   {
      cmsLog_error("Invalid ipv4 address=%s", IPv4Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (cmsUtl_parsePrefixAddress(sitePrefix, addr6, &plen) != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid ipv6 address=%s", sitePrefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (inet_pton(AF_INET6, addr6, &in6Addr) <= 0)
   {
      cmsLog_error("Invalid ipv6 address=%s", sitePrefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (cmsUtl_parsePrefixAddress(IPv6Prefix, addr6, &plen) != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid ipv6 address=%s", IPv6Prefix);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   /* get nat-public_address */

   if (psidLen == (eaLen + ipv4MaskLen - 32))
   {
      int pbw0, pbi0, pbi1;
      UINT32 d;

      if (ipv4MaskLen < 32) {
         pbw0 = plen >> 5;
         pbi0 = plen & 0x1f;
         d = (ntohl(in6Addr.s6_addr32[pbw0]) << pbi0) >> ipv4MaskLen;
         pbi1 = pbi0 - ipv4MaskLen;
         if (pbi1 > 0)
            d |= ntohl(in6Addr.s6_addr32[pbw0+1]) >> (32 - pbi1);
         in4Addr.s_addr |= htonl(d);
      }

      inet_ntop(AF_INET, &in4Addr, IPv4Prefix, INET_ADDRSTRLEN);
      cmsLog_debug("v4address= %s %x\n", IPv4Prefix, (int)in4Addr.s_addr);

      {
         char tmp[BUFLEN_8];
         snprintf(tmp, sizeof(tmp), "/%d", ipv4MaskLen); /* Rule prefix */
         cmsUtl_strncat(IPv4Prefix, CMS_IPADDR_LENGTH, tmp);
      }
   }

   return ret;
}

CmsRet rutMap_newPortSetRange(UINT32 psidOffset, UINT32 psidLen, UINT32 psid, PortSet **ps)
{
   CmsRet ret = CMSRET_SUCCESS;

   if (*ps != NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (psidLen > 0)
   {
      UINT16 min, max, p1, p2, p3;
      int i, numRanges;
      NaptBlock *range;
      PortSet *tmpPs;

      tmpPs = cmsMem_alloc(sizeof(*tmpPs), ALLOC_ZEROIZE);
      if (tmpPs == NULL)
      {
         cmsLog_error("Failed to allocate ps");
         return CMSRET_RESOURCE_EXCEEDED;
      }

      numRanges = psidOffset ? (1 << psidOffset) - 1 : 1;

      range = cmsMem_alloc(numRanges * sizeof(*range), ALLOC_ZEROIZE);
      if (range == NULL)
      {
         cmsLog_error("Failed to allocate range");
         cmsMem_free(tmpPs);
         return CMSRET_RESOURCE_EXCEEDED;
      }

      for (i = 0; i < numRanges; i++)
      {
         p1 = (psidOffset ? i + 1 : i) << (16 -	psidOffset);
         p2 = psid << (16 - psidOffset - psidLen);
         p3 = 0xffff >> (psidOffset + psidLen);
         min = p1 | p2;
         max = p1 | p2 | p3;
         range[i].min = min;
         range[i].max = max;

         cmsLog_debug( "range[%4d] = %6d(0x%04x) - %6d(0x%04x)",
                       i,
                       range[i].min, range[i].min,
                       range[i].max, range[i].max);
      }

      tmpPs->range = range;
      tmpPs->numRanges = numRanges;
      *ps = tmpPs;
   }
   else
   {
      cmsLog_debug("Do nothing in 1:1 mapping case");
   }

   return ret;
}

void rutMap_deletePortSetRange(PortSet *ps)
{
   if (ps)
   {
      cmsMem_free(ps->range); /* numRanges >= 1 */
      cmsMem_free(ps);
   }
}

void rutMap_mapControl(const char *ipIntfFullPath)
{
   UBOOL8 found = FALSE;
   Dev2MapDomainObject *domain;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char brprefix[BUFLEN_32] = {0};

   cmsLog_debug("ipIntfPath: %s", ipIntfFullPath);

   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_MAP_DOMAIN, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&domain) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(domain->WANInterface, ipIntfFullPath))
      {
         found = TRUE;
         cmsUtl_strncpy(brprefix, domain->BRIPv6Prefix, sizeof(brprefix));
      }

      cmsObj_free((void **)&domain);
   }

   cmsLog_debug("found<%d> brprefix<%s>", found, brprefix);

   if (found && !IS_EMPTY_STRING(brprefix))
   {
      Dev2MapDomainRuleObject *rule = NULL;
      InstanceIdStack iidStack_rule = EMPTY_INSTANCE_ID_STACK;

      while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_MAP_DOMAIN_RULE,
                              &iidStack, &iidStack_rule,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&rule) == CMSRET_SUCCESS)
      {
         if ( cmsObj_set(rule, &iidStack_rule) != CMSRET_SUCCESS )
         {
            cmsLog_error("Failed to set MDMOID_DEV2_MAP_RULE object");
         }
         cmsObj_free((void **) &rule);
      }
   }

   return;
}


UBOOL8 rutTunnel_isMapDynamic(const char *wanIpIntfPath, char *mechanism)
{
   UBOOL8 ret = FALSE;
   UBOOL8 found = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2MapDomainObject *domain;

   cmsLog_debug("wanIpIntfPath<%s>", wanIpIntfPath);

   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_MAP_DOMAIN, &iidStack,
                 OGF_NO_VALUE_UPDATE, (void **)&domain) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(domain->WANInterface, wanIpIntfPath))
      {
         Dev2MapDomainRuleObject *rule = NULL;
         InstanceIdStack iidStack_rule = EMPTY_INSTANCE_ID_STACK;

         found = TRUE;
         cmsUtl_strncpy(mechanism, domain->transportMode, BUFLEN_24);

         if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_MAP_DOMAIN_RULE, &iidStack, &iidStack_rule, OGF_NO_VALUE_UPDATE, (void **)&rule) == CMSRET_SUCCESS)
         {
            if (!cmsUtl_strcmp(rule->origin, MDMVS_DHCPV6))
            {
               ret = TRUE;
            }

            cmsObj_free((void **)&rule);
         }
         else
         {
            cmsLog_notice("cannot get associated domain rule");
         }
      }

      cmsObj_free((void **)&domain);
   }

   cmsLog_debug("found<%d> ismap<%c> isdynamic<%d>", found, *mechanism, ret);

   return ret;
}

#endif /* SUPPORT_MAP */
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

