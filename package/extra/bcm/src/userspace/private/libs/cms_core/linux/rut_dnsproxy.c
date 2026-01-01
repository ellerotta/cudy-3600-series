/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:proprietary:standard
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
 *
 ************************************************************************/

#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_system.h"
#include "mdm.h"
#include "rut_pmap.h"
#include "rut_route.h"
#include "rut_dns.h"
#include "qdm_tr69c.h"
#include "rut_dnsproxy.h"
#include "rut_network.h"

/** Send a reload message to dnsproxy process, which will tell it to reload
 *  all config files.
 *
 * @return CmsRet enum.
 */
#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 
static CmsRet sendReloadMsgToDnsproxy(void);
#endif

UBOOL8 rutDpx_isEnabled(void)
{
    UBOOL8 dnsProxyEnable = FALSE;

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _DnsProxyCfgObject *dproxyCfg=NULL;
   CmsRet ret;


   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &iidStack, 0, (void **) &dproxyCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get DPROXY_CFG, ret=%d", ret);
   }
   else
   {
      dnsProxyEnable = dproxyCfg->enable;
      cmsObj_free((void **) &dproxyCfg);
   }

   cmsLog_debug("dnsproxy enable status = %d", dnsProxyEnable);
#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */

   return dnsProxyEnable;
   
}



CmsRet rutDpx_updateDnsproxy(void)
{
   CmsRet ret = CMSRET_SUCCESS;

   /* Always create /var/dnsinfo.conf, /etc/resolv.conf for the system even
   * dnsproxy is not used 
   */
   rutDns_createDnsInfoConf();
   rutLan_createResolvCfg();

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 

   _DnsProxyCfgObject *dproxyCfg = NULL;   
   InstanceIdStack dnsProxyIidStack = EMPTY_INSTANCE_ID_STACK;


   if ((ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &dnsProxyIidStack, 0, (void **) &dproxyCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get DnsProxyCfgObject, ret=%d", ret);
      return ret;
   }
   
   if(dproxyCfg->enable)
   {
      /* dnsproxy needs this file, so make sure they are up to date */
#ifndef DESKTOP_LINUX
      rutSys_createHostsFile();
#endif

      /*
       * If DNS proxy is enabled, it should be running.  Even if it is not
       * running, sending a message to it will cause smd to launch it.
       */
      ret = sendReloadMsgToDnsproxy();
   }

   cmsObj_free((void **) &dproxyCfg);	

#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */

   return ret;
}

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_DETECT)
static void rutDpx_DnsmasqExceptInterfaces_igd(FILE *fp_dnsmasq)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConnObj = NULL;
   WanPppConnObject *pppConnObj = NULL;
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void **) &ipConnObj)) == CMSRET_SUCCESS)
   {
      fprintf(fp_dnsmasq, "except-interface=%s\n", ipConnObj->X_BROADCOM_COM_IfName);
      cmsObj_free((void **) &ipConnObj);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      fprintf(fp_dnsmasq, "except-interface=%s\n", pppConnObj->X_BROADCOM_COM_IfName);
      cmsObj_free((void **) &pppConnObj);
   }
}
#endif

#if defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)
static void rutDpx_DnsmasqExceptInterfaces_dev2(FILE *fp_dnsmasq)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2IpInterfaceObject *ipIntf=NULL;

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipIntf) == CMSRET_SUCCESS)
   {
      if ((!ipIntf->X_BROADCOM_COM_Upstream) || (ipIntf->X_BROADCOM_COM_BridgeService))
      {
         /* skip non wan ip interfaces */
         cmsObj_free((void **)&ipIntf);  
         continue;
      }

      fprintf(fp_dnsmasq, "except-interface=%s\n", ipIntf->name);
      cmsObj_free((void **)&ipIntf);
   }
}
#endif

static void rutDpx_DnsmasqExceptInterfaces(FILE *fp_dnsmasq)
{
#if defined(SUPPORT_DM_LEGACY98)
   rutDpx_DnsmasqExceptInterfaces_igd(fp_dnsmasq);
#elif defined(SUPPORT_DM_HYBRID)
   rutDpx_DnsmasqExceptInterfaces_igd(fp_dnsmasq);
#elif defined(SUPPORT_DM_PURE181)
   rutDpx_DnsmasqExceptInterfaces_dev2(fp_dnsmasq);
#elif defined(SUPPORT_DM_DETECT)
   if (cmsMdm_isDataModelDevice2())
   {
      rutDpx_DnsmasqExceptInterfaces_dev2(fp_dnsmasq);
   }     
   else
   {
      rutDpx_DnsmasqExceptInterfaces_igd(fp_dnsmasq);
   }
#endif
}

/* write config file for Dnsmasq according to /var/dnsinfo.conf */
CmsRet rutDpx_createDnsmasqCfg(void)
{
   FILE *fp_dnsinfo;
   FILE *fp_dnsmasq;
   char wanIfName[CMS_IFNAME_LENGTH] = {0};
   char subnetCidr[INET6_ADDRSTRLEN] = {0};
   char dnsList[5 * INET6_ADDRSTRLEN] = {0};   /* make space for 5 dns entries in case  they have that many */
   char applicationBuf[BUFLEN_64];
   char line[BUFLEN_512]; /* > (CMS_IFNAME_LENGTH + 6 * INET6_ADDRSTRLEN) + 64 */
   int lines = 0;
   char *curPtr;
   char *nullPtr;
   char dnsmasqCfgPath[CMS_MAX_FULLPATH_LENGTH] = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char *saveptr = NULL;
   char *tempDns = NULL;
   char *localDomainName=NULL;

   cmsLog_debug("Enter");

   // first open the dnsmasq config file
   ret = cmsUtl_getRunTimePath(DNSMASQ_CONF, dnsmasqCfgPath, sizeof(dnsmasqCfgPath));
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
   
   cmsLog_debug("opening file %s", dnsmasqCfgPath);

   if ((fp_dnsmasq = fopen(dnsmasqCfgPath, "w")) == NULL)
   {
      cmsLog_error ("Failed to create %s", dnsmasqCfgPath);
      return CMSRET_OPEN_FILE_ERROR;
   }

   //write the common settings
   fprintf(fp_dnsmasq, "# Never forward plain names (without a dot or domain part)\n");
   fprintf(fp_dnsmasq, "domain-needed\n\n");
   fprintf(fp_dnsmasq, "# Never forward addresses in the non-routed address spaces.\n");
   fprintf(fp_dnsmasq, "bogus-priv\n\n");
   fprintf(fp_dnsmasq, "# Don't store in cache the invalid resolutions\n");
   fprintf(fp_dnsmasq, "no-negcache\n\n");
#ifdef SUPPORT_DNSMASQWITHDNSSEC
   fprintf(fp_dnsmasq, "conf-file=/var/trust-anchors.conf\n\n");
   fprintf(fp_dnsmasq, "dnssec\n\n");
   fprintf(fp_dnsmasq, "dnssec-check-unsigned\n\n");
#endif
#if defined(SUPPORT_DNSMASQWITHDOH) || defined(SUPPORT_DNSMASQWITHDOT)
   fprintf(fp_dnsmasq, "# No resolv file to read\n");
   fprintf(fp_dnsmasq, "no-resolv\n\n");
#else
   fprintf(fp_dnsmasq, "# resolv file to specify upstream servers\n");
   fprintf(fp_dnsmasq, "resolv-file=%s\n\n", DNSMASQ_RESOLV_CONF);
#endif
   fprintf(fp_dnsmasq, "# Set the cachesize here.\n");
   fprintf(fp_dnsmasq, "cache-size=200\n\n");
   //fprintf(fp_dnsmasq, "# Always perform DNS queries to all servers.\n");
   //fprintf(fp_dnsmasq, "all-servers\n\n");
   fprintf(fp_dnsmasq, "# forces dnsmasq to try each query with each server strictly\n");
   fprintf(fp_dnsmasq, "# in the order they appear in resolv file\n");
   fprintf(fp_dnsmasq, "strict-order\n\n");
   fprintf(fp_dnsmasq, "no-hosts\n\n");
   fprintf(fp_dnsmasq, "addn-hosts=/var/hosts\n\n");

   //fprintf(fp_dnsmasq, "interface=br0\n\n");   

#ifdef SUPPORT_DNSMASQWITHDNSSEC
   if ((fp_dnsinfo = fopen("/var/trust-anchors.conf", "w")) != NULL)
   {
      fprintf(fp_dnsinfo, "trust-anchor=.,19036,8,2,49AAC11D7B6F6446702E54A1607371607A1A41855200FD2CE1CDDE32F24E8FB5\n");
      fclose(fp_dnsinfo);
   }
#endif

   // then open the /var/dnsinfo.conf file
   if ((fp_dnsinfo = fopen(DNSINFO_CONF, "r")) == NULL)
   {
      cmsLog_notice(" %s does not exist.", DNSINFO_CONF);
      fclose(fp_dnsmasq);
      return CMSRET_OPEN_FILE_ERROR;
   }

   // read file line by line
   while (fgets(line, sizeof(line), fp_dnsinfo))
   {
      lines++;
      curPtr = line;

      /* get rid of '\n' at the end if there is any */
      if (line[strlen(line) -1] == '\n')
      {
         line[strlen(line) - 1] = '\0';
      }

      /* 1) get Wan IfName */
      if ((nullPtr = strchr(curPtr, ';')) != NULL)
      {
         *nullPtr = '\0';
         cmsUtl_strncpy(wanIfName, curPtr, CMS_IFNAME_LENGTH);
         curPtr = nullPtr + 1;
      }

      /* 2) get subnet in cidr format */
      if ((nullPtr = strchr(curPtr, ';')) != NULL)
      {
         *nullPtr = '\0';
         cmsUtl_strncpy(subnetCidr, curPtr, INET6_ADDRSTRLEN);
         curPtr = nullPtr + 1;
      }

      /* 3)  get dns list separated by ',' */
      if ((nullPtr = strchr(curPtr, ';')) != NULL)
      {
         *nullPtr = '\0';
         cmsUtl_strncpy(dnsList, curPtr, 5 * INET6_ADDRSTRLEN);
         curPtr = nullPtr + 1;
      }

      /* 4) get apps list separated by ','
            * more than 1 applications such as "...;tr69c,voipd", etc on the WAN interface.
            */
      cmsUtl_strncpy(applicationBuf, curPtr, sizeof(applicationBuf));


      cmsLog_debug("wanif=[%s] subnetCidr=[%s], dnsList=[%s], processNameList=[%s]", 
                    wanIfName,  subnetCidr, dnsList, applicationBuf);

      /*  process DNS part: the first line of the dnsinfo.conf will be the default system DNS,
            *  and normally we only write the default DNS into dnsmasq resolv config file.
            */
      if (lines == 1)
      {
         FILE *fp_resolv;
         char dnsEntry[INET6_ADDRSTRLEN];
         char existingDnsList[5 * INET6_ADDRSTRLEN] = {0};

         if ((fp_resolv = fopen(DNSMASQ_RESOLV_CONF, "w")) == NULL)
         {
            cmsLog_error ("Failed to create %s", DNSMASQ_RESOLV_CONF);
            fclose(fp_dnsmasq);
            fclose(fp_dnsinfo);
            return CMSRET_OPEN_FILE_ERROR;
         }
#ifdef SUPPORT_DNSMASQWITHDOH
         {
            char cmdline[BUFLEN_512] = {0};
            // Restart DoH proxy.
            snprintf(cmdline, sizeof(cmdline), "killall https_dns_proxy");
            rut_doSystemAction("DoH", cmdline);
#if 0 // Only for CDR testing.
            snprintf(cmdline, sizeof(cmdline), "https_dns_proxy -d -b %s -p 5054 -r \"https://dns1.cdroutertest.com/dns-query\"", dnsList);
            rut_doSystemAction("DoH", cmdline);
            // fill in the https_dns_proxy's setting
            fprintf(fp_dnsmasq, "server=127.0.0.1#5054\n\n");
#else
            snprintf(cmdline, sizeof(cmdline), "https_dns_proxy -4 -d -b %s ", dnsList);
            // snprintf(cmdline, sizeof(cmdline), "https_dns_proxy -4 -d -b %s -r \"https://dns.google/dns-query\"", dnsList);
            rut_doSystemAction("DoH", cmdline);
            // fill in the https_dns_proxy's setting
            fprintf(fp_dnsmasq, "server=127.0.0.1#5053\n\n");
#endif
         }
#endif /* SUPPORT_DNSMASQWITHDOH */
#ifdef SUPPORT_DNSMASQWITHDOT
         {
            // set upstream to localhost with default port 53000 where the stubby serves and listens
            fprintf(fp_dnsmasq, "server=::1#53000\n\n");
            fprintf(fp_dnsmasq, "server=127.0.0.1#53000\n\n");
         }
#endif /* SUPPORT_DNSMASQWITHDOT */
         // fill in the dns server setting
         for (tempDns = strtok_r(dnsList, ",", &saveptr); tempDns != NULL; tempDns = strtok_r(NULL, ",", &saveptr))
         {  
            snprintf(dnsEntry, sizeof(dnsEntry), "nameserver %s\n", tempDns);
            // add check for duplicated servers
            if (!strstr(existingDnsList, dnsEntry))
            {
               fprintf(fp_resolv, "%s\n", dnsEntry);
               strncat(existingDnsList, dnsEntry, sizeof(existingDnsList) - strlen(existingDnsList) - 1);
            }
         }
         fclose(fp_resolv);
      }

      /*  process applications part: */

      // special handling for "tr69c": we add extra dns server settings to force the 
      // dns queries with domain which matched to "host" portion of ACS URL 
      // to be routed to the dns servers obtained from the wan interface where tr69c used
#ifdef SUPPORT_TR69C
      if ((lines != 1) && (cmsUtl_strstr(applicationBuf, "tr69c")))
      {
         {
            char *ptr = NULL;
            char acs_url[BUFLEN_256] = {0};

            // get the first item from the Wan IfName list
            if ((ptr = strchr(wanIfName, ',')) != NULL)
            {
               *ptr = '\0';
            }
            cmsLog_debug("Wan IfName=%s", wanIfName);

            ret = qdmTr69c_getUrlLocked(acs_url);
            if (ret == CMSRET_SUCCESS)
            {
               char *urlAddr = NULL, *urlPath = NULL;
               UrlProto urlProto;
               UINT16 urlPort;

               cmsLog_debug("ACS URL=%s", acs_url);
               ret = cmsUtl_parseUrl(acs_url, &urlProto, &urlAddr, &urlPort, &urlPath);
               if (ret == CMSRET_SUCCESS)
               {
                  cmsLog_debug("urlAddr=%s", urlAddr);
                  // excepts urlAddr is not an IP address
                  if (!cmsUtl_isValidIpv4Address(urlAddr) && !cmsUtl_strstr(urlAddr, ":"))
                  {
                     saveptr = NULL;
                     tempDns = NULL;
                     cmsLog_error("dnsList=%s", dnsList);
                     for (tempDns = strtok_r(dnsList, ",", &saveptr); tempDns != NULL; tempDns = strtok_r(NULL, ",", &saveptr))
                     {  
                        fprintf(fp_dnsmasq, "server=/%s/%s\n\n", urlAddr, tempDns);
                     }
                  }
               }
               cmsMem_free(urlAddr);
               cmsMem_free(urlPath);
            }
               
         }
      }
#endif
   }

   rutDpx_DnsmasqExceptInterfaces(fp_dnsmasq);

   fclose(fp_dnsinfo);

   // write local domain
   rutNtwk_getDomainName(&localDomainName);
   if (localDomainName)
   {
      fprintf(fp_dnsmasq, "local=/%s/\n\n", localDomainName);
      CMSMEM_FREE_BUF_AND_NULL_PTR(localDomainName);
   }

   if (lines == 0)
   {
      //ret = CMSRET_INTERNAL_ERROR;
      cmsLog_notice("%s is empty??", DNSINFO_CONF);
   }
   fclose(fp_dnsmasq);
   cmsLog_debug("End");
   return ret;
}

#ifdef SUPPORT_DNSMASQWITHDOT
CmsRet rutDpx_createStubbyDefaultCfg(void)
{
    FILE *fp;

   if ((fp = fopen(STUBBY_CONF, "w")) == NULL)
   {
      cmsLog_error ("Failed to create %s", STUBBY_CONF);
      return CMSRET_OPEN_FILE_ERROR;
   }

   fprintf(fp, "# For stubby this MUST be set to GETDNS_RESOLUTION_STUB\n");
   fprintf(fp, "resolution_type: GETDNS_RESOLUTION_STUB\n\n");
   fprintf(fp, "# Strict mode (see below) should use only GETDNS_TRANSPORT_TLS.\n");
   fprintf(fp, "dns_transport_list:\n");
   fprintf(fp, "  - GETDNS_TRANSPORT_TLS\n\n");
   fprintf(fp, "# For Strict use        GETDNS_AUTHENTICATION_REQUIRED\n");
   fprintf(fp, "tls_authentication: GETDNS_AUTHENTICATION_REQUIRED\n\n");
   fprintf(fp, "# EDNS0 option to pad the size of the DNS query to the given blocksize\n");
   fprintf(fp, "tls_query_padding_blocksize: 128\n\n");
   fprintf(fp, "# EDNS0 option for ECS client privacy as described in Section 7.1.2 of\n");
   fprintf(fp, "# https://tools.ietf.org/html/rfc7871\n");
   fprintf(fp, "edns_client_subnet_private : 1\n\n");
   fprintf(fp, "# Set to 1 to instruct stubby to distribute queries across all available name servers\n");
   fprintf(fp, "round_robin_upstreams: 1\n\n");
   fprintf(fp, "# This keeps idle TLS connections open to avoid the overhead of opening a new\n");
   fprintf(fp, "# connection for every query.\n");
   fprintf(fp, "idle_timeout: 10000\n\n");
   
   fprintf(fp, "# Set the listen addresses for the stubby DAEMON.\n");
   fprintf(fp, "listen_addresses:\n");
   fprintf(fp, "  - 127.0.0.1@53000\n");
   fprintf(fp, "  - 0::1@53000\n\n");
   fprintf(fp, "# Specify the list of upstream recursive name servers to send queries to\n");
   fprintf(fp, "# In Strict mode upstreams need either a tls_auth_name or a tls_pubkey_pinset\n");
   fprintf(fp, "# so the upstream can be authenticated.\n");
   fprintf(fp, "upstream_recursive_servers:\n");
   fprintf(fp, "####### IPv4 addresses ######\n");
   fprintf(fp, "# The Surfnet/Sinodun servers\n");
   fprintf(fp, "  - address_data: 145.100.185.15\n");
   fprintf(fp, "    tls_auth_name: \"dnsovertls.sinodun.com\"\n");
   fprintf(fp, "    tls_pubkey_pinset:\n");
   fprintf(fp, "      - digest: \"sha256\"\n");
   fprintf(fp, "        value: 62lKu9HsDVbyiPenApnc4sfmSYTHOVfFgL3pyB+cBL4=\n");
   fprintf(fp, "# The getdnsapi.net server\n");
   fprintf(fp, "  - address_data: 185.49.141.37\n");
   fprintf(fp, "    tls_auth_name: \"getdnsapi.net\"\n");
   fprintf(fp, "    tls_pubkey_pinset:\n");
   fprintf(fp, "      - digest: \"sha256\"\n");
   fprintf(fp, "        value: foxZRnIh9gZpWnl+zEiKa0EJ2rdCGroMWm02gaxSc9Q=\n");
#if 0 //for CDRouter test
   fprintf(fp, "# The CDRouter DNS Server\n");
   fprintf(fp, "  - address_data: 202.254.101.1\n");
   fprintf(fp, "    tls_auth_name: \"dns1.cdroutertest.com\"\n");
   fprintf(fp, "    tls_pubkey_pinset:\n");
   fprintf(fp, "      - digest: \"sha256\"\n");
   fprintf(fp, "        value: 4Gl6dxXYbzXLULPP3IT8RIZt4UGD449O9w0Whk3F560=\n");
#endif
   fprintf(fp, "####### IPv6 addresses ######\n");
   fprintf(fp, "# The Surfnet/Sinodun servers\n");
   fprintf(fp, "  - address_data: 2001:610:1:40ba:145:100:185:15\n");
   fprintf(fp, "    tls_auth_name: \"dnsovertls.sinodun.com\"\n");
   fprintf(fp, "    tls_pubkey_pinset:\n");
   fprintf(fp, "      - digest: \"sha256\"\n");
   fprintf(fp, "        value: 62lKu9HsDVbyiPenApnc4sfmSYTHOVfFgL3pyB+cBL4=\n");
   fprintf(fp, "# The getdnsapi.net server\n");
   fprintf(fp, "  - address_data: 2a04:b900:0:100::38\n");
   fprintf(fp, "    tls_auth_name: \"getdnsapi.net\"\n");
   fprintf(fp, "    tls_pubkey_pinset:\n");
   fprintf(fp, "      - digest: \"sha256\"\n");
   fprintf(fp, "        value: foxZRnIh9gZpWnl+zEiKa0EJ2rdCGroMWm02gaxSc9Q=\n");
   fprintf(fp, "\n");

   fclose(fp);

   return CMSRET_SUCCESS;
}
#endif /* SUPPORT_DNSMASQWITHDOT */

CmsRet sendReloadMsgToDnsproxy(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   int pid;
   char cmdline[BUFLEN_256] = {0};

   ret = rutDpx_createDnsmasqCfg();
   if (ret == CMSRET_SUCCESS)
   {
      //snprintf(cmdline, sizeof(cmdline), "-C %s -k -q --log-facility=/var/log/dnsmasq.log", DNSMASQ_CONF);
      snprintf(cmdline, sizeof(cmdline), "-C %s -k -q", DNSMASQ_CONF);
      //cmsLog_error("cmdline=%s", cmdline);
      pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_DNSMASQ, cmdline, strlen(cmdline)+1);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start or restart DNSMASQ");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("restarting DNSMASQ, pid=%d", pid);
      }
#ifdef SUPPORT_DNSMASQWITHDOT
      {
         if (!cmsFil_isFilePresent(STUBBY_CONF))
         {
            // create stubby config file if the runtime config file "STUBBY_CONF" is not found
            rutDpx_createStubbyDefaultCfg();
         }

         // restart stubby daemon
         rut_doSystemAction("DoT", "killall -q stubby");
         rut_doSystemAction("DoT", "rm -f /var/run/stubby.pid");
         snprintf(cmdline, sizeof(cmdline), "stubby -g -C %s", STUBBY_CONF);
         rut_doSystemAction("DoT", cmdline);
      }
#endif /* SUPPORT_DNSMASQWITHDOT */
   }
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */

