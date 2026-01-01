/***********************************************************************
 *
 * <:copyright-BRCM:2007:proprietary:standard
 * 
 *    Copyright (c) 2007 Broadcom 
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
 * :>
 *
 ************************************************************************/

#ifdef SUPPORT_CLI_CMD
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_cli.h"
#include "cli.h"
#include "cms_qdm.h"

/***************************************************************************
// Function Name: ValidateMacAddress
// Description  : validate format of the MAC address.
// Parameters   : addr - MAC address.
// Returns      : FALSE - invalid format.
//                TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isMacAddress(char *addr)
{
   UBOOL8 ret = FALSE;
   int i = 0;
   char *pToken = NULL, *pLast = NULL, *pEnd = NULL;
   char buf[18];
   long num = 0;

   if ( addr == NULL || (strlen(addr) > 18) )
      return ret;

      // need to copy since strtok_r updates string
   strcpy(buf, addr);

   // IP address has the following format
   //   xxx.xxx.xxx.xxx where x is decimal number
   pToken = strtok_r(buf, ":", &pLast);
   if ( pToken == NULL )
      return ret;
   num = strtol(pToken, &pEnd, 16);

   if ( *pEnd == '\0' && num <= 255 )
   {
      for ( i = 0; i < 5; i++ )
      {
         pToken = strtok_r(NULL, ":", &pLast);
         if ( pToken == NULL )
            break;
         num = strtol(pToken, &pEnd, 16);
         if ( *pEnd != '\0' || num > 255 )
            break;
      }
      if ( i == 5 )
         ret = TRUE;
   }
   return ret;
}

/***************************************************************************
// Function Name: isIpAddress
// Description  : validate format of the IP address.
// Parameters   : buf - IP address.
// Returns      : CLI_FALSE - invalid format.
//                CLI_TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isIpAddress(const char *addr)
{
   if (addr != NULL)
   {
      struct in_addr inp;

      if (inet_aton(addr, &inp))
      {
         return TRUE;
      }
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isNumber
// Description  : validate decimal number from string.
// Parameters   : buf - decimal number.
// Returns      : CLI_FALSE - invalid decimal number.
//                CLI_TRUE - valid decimal number.
****************************************************************************/
UBOOL8 cli_isNumber(const char *buf)
{
   if ( buf != NULL )
   {
      int size = strlen(buf);
      int i;

      for ( i = 0; i < size; i++ )
      {
         if ( isdigit(buf[i]) == 0 )
         {
            break;
         }
      }
      if ( size > 0 && i == size )
      {
         return TRUE;
      }
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidVpi
// Description  : validate VPI range.
// Parameters   : vpi - VPI.
// Returns      : CLI_FALSE - invalid format.
//                CLI_TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidVpi(const char *vpi)
{
   if ( cli_isNumber(vpi) )
   {
      int num = atoi(vpi);
      if ( num <= 255 && num >= 0 )
         return TRUE;
      else
         printf("\nvpi is out of range [0-255]\n");
   }
   else
   {
      printf("\nInvalid vpi %s\n", vpi? vpi : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidVci
// Description  : validate VCI range.
// Parameters   : vci - VCI.
// Returns      : CLI_FALSE - invalid format.
//                CLI_TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidVci(const char *vci)
{
   if ( cli_isNumber(vci) )
   {
      int num = atoi(vci);
      if ( num <= 65535 && num >= 32 )
         return TRUE;
      else
         printf("\nvci is out of range [32-65535]\n");
   }
   else
   {
      printf("\nInvalid vci %s\n", vci? vci : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidWanId
// Description  : validate WAN connection ID range.
// Parameters   : id - WAN ID.
// Returns      : CLI_FALSE - invalid format.
//                CLI_TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidWanId(const char *id)
{
   if ( cli_isNumber(id) )
   {
      int num = atoi(id);
      if ( num <= IFC_WAN_MAX && num >= 0 )
         return TRUE;
      else
         printf("\nwan id is out of range [0-%d]\n", IFC_WAN_MAX);
   }
   else
   {
      printf("\nInvalid wan id %s\n", id? id : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidVlanId
// Description  : validate VLAN ID range.
// Parameters   : id - WAN ID.
// Returns      : CLI_FALSE - invalid format.
//                CLI_TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidVlanId(const char *id)
{
   if ( cli_isNumber(id) )
   {
      int num = atoi(id);
      if ( num <= IFC_VLAN_MAX && num >= 0 )
         return TRUE;
      else
         printf("\nvlan id is out of range [0-%d]\n", IFC_VLAN_MAX);
   }
   else
   {
      printf("\nInvalid vlan id %s\n", id? id : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidIdleTimeout
// Description  : validate PPP idle timeout.
// Parameters   : timeout - PPP idle timeout.
// Returns      : FALSE - invalid format.
//                TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidIdleTimeout(const char *timeout)
{
   if ( cli_isNumber(timeout) )
   {
      int num = atoi(timeout);
      if ( num <= 1090 && num >= 0 )
         return TRUE;
      else
         printf("\nidle timeout is out of range [0-1090]\n");
   }
   else
   {
      printf("\nInvalid idle timeout %s\n", timeout? timeout : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidWanServiceName
// Description  : validate WAN service name.
// Parameters   : username - PPP user name.
// Returns      : FALSE - invalid format.
//                TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidWanServiceName(const char *service)
{
   if ( service != NULL )
   {
      int len = strlen(service);

      if ( len <= 32 && len >= 0 )
         return TRUE;
      else
         printf("\nlength of service name is out of range [0-32]\n");
   }
   else
   {
      printf("\nInvalid wan service name %s\n", service? service : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidPppUserName
// Description  : validate PPP user name.
// Parameters   : username - PPP user name.
// Returns      : FALSE - invalid format.
//                TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidPppUserName(const char *username)
{
   if ( username != NULL )
   {
      int len = strlen(username);

      if ( len <= 256 && len >= 0 )
         return TRUE;
      else
         printf("\nlength of username is out of range [0-256]\n");
   }
   else
   {
      printf("\nInvalid username %s\n", username? username : "");
   }
   return FALSE;
}

/***************************************************************************
// Function Name: isValidPppPassword
// Description  : validate PPP password.
// Parameters   : password - PPP password.
// Returns      : FALSE - invalid format.
//                TRUE - valid format.
****************************************************************************/
UBOOL8 cli_isValidPppPassword(const char *password)
{
   if ( password != NULL )
   {
      int len = strlen(password);

      if ( len <= 32 && len >= 0 )
         return TRUE;
      else
         printf("\nlength of password is out of range [0-32]\n");
   }
   else
   {
      printf("\nInvalid password %s\n", password? password : "");
   }
   return FALSE;
}


UBOOL8 cli_isValidL2IfName_igd(const char *l2IfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char wanL2IfName[CMS_IFNAME_LENGTH];
   char *p;
   UBOOL8 foundLinkCfg=FALSE;
   UBOOL8 isValid=TRUE;

   strncpy(wanL2IfName, l2IfName, CMS_IFNAME_LENGTH-1);
   if ((p = strchr(wanL2IfName, '/')))
   {
      /* now wanL2IfName is "ptm0", "atm0" etc. */
      *p = '\0';
   }
#ifdef DMP_ETHERNETWAN_1
   else if (!(cmsUtl_strstr(wanL2IfName, ETH_IFC_STR)))
#else
   else
#endif
   {
      cmsLog_error("wanL2IfName %s - wrong format", wanL2IfName);
      return FALSE;
   }

#ifdef DMP_BASELINE_1
#ifdef DMP_ETHERNETWAN_1  /* aka SUPPORT_ETHWAN */
   if (cmsUtl_strstr(wanL2IfName, ETH_IFC_STR) != NULL)
   {
      foundLinkCfg = dalEth_getEthIntfByIfName(wanL2IfName, &iidStack, NULL);
   }
#endif
#endif  /* DMP_BASELINE_1 */

#ifdef DMP_ADSLWAN_1
#ifdef DMP_PTMWAN_1
   if (cmsUtl_strstr(wanL2IfName, PTM_IFC_STR) != NULL)
   {
      foundLinkCfg = dalDsl_getPtmLinkByIfName(wanL2IfName, &iidStack, NULL);
   }

#endif

   if (!foundLinkCfg &&
       (cmsUtl_strstr(wanL2IfName, ATM_IFC_STR) != NULL ||
        cmsUtl_strstr(wanL2IfName, IPOA_IFC_STR) != NULL))
   {
      foundLinkCfg = dalDsl_getDslLinkByIfName(wanL2IfName, &iidStack, NULL);
   }
#endif /* DMP_ADSLWAN_1 */

   /* todo: Currently, connMode and vlanMux is not supported in cli.  Need to modified this if these 
   * are supported later on.
   */
   if (foundLinkCfg)
   {
      InstanceIdStack wanConnIidStack=EMPTY_INSTANCE_ID_STACK;
      InstanceIdStack savedLinkIidStack=iidStack;
      WanIpConnObject *ipConn=NULL;
      WanPppConnObject *pppConn=NULL;

      while (isValid &&
         cmsObj_getNextInSubTreeFlags(MDMOID_WAN_IP_CONN, &iidStack, &wanConnIidStack, OGF_NO_VALUE_UPDATE, (void **)&ipConn) == CMSRET_SUCCESS)
      {
         cmsObj_free((void **) &ipConn);
         isValid = FALSE;
      }

      savedLinkIidStack=iidStack;
      INIT_INSTANCE_ID_STACK(&wanConnIidStack);
      while (isValid &&
         cmsObj_getNextInSubTreeFlags(MDMOID_WAN_PPP_CONN, &savedLinkIidStack, &wanConnIidStack, OGF_NO_VALUE_UPDATE, (void **)&pppConn) == CMSRET_SUCCESS)
      {
         cmsObj_free((void **) &pppConn);
         isValid = FALSE;
      }
   }
   else
   {
      isValid = FALSE;
   }

   return isValid;

}


CmsRet cli_checkQosQueueResources(const PWEB_NTWK_VAR pInfo)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 unUsedQueues;

   if ( cmsUtl_strcmp(pInfo->atmServiceCategory, MDMVS_CBR)    == 0 ||
        cmsUtl_strcmp(pInfo->atmServiceCategory, MDMVS_VBR_RT) == 0 )
   {
      printf("\nQoS can only be enabled with PVC that has service category as UBR, UBRWPCR, or VBR-nrt.\n");
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsDal_getNumUnusedQueues(ATM, &unUsedQueues);

   if ( pInfo->serviceId == 0 )
   {
      if (unUsedQueues < 3)
      {  /* assume a new QoS pvc requires 3 queues */
         printf("\nCannot enable QoS since system is run out of queues for PVC.\n");
         ret = CMSRET_RESOURCE_EXCEEDED;
      }
   }
   else
   {
      if (unUsedQueues < 2)
      {  /* assume an Edit of existing pvc (add QoS) requires addtional 2 queues */
         printf("\nCannot enable QoS since system is run out of queues for PVC.\n");
         ret = CMSRET_RESOURCE_EXCEEDED;
      }
   }
   return ret;

}


#ifdef DMP_ADSLWAN_1

UBOOL8 cli_isVccAddrExist_igd(const WEB_NTWK_VAR *webVar)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   WanDslLinkCfgObject  *dslLinkCfg=NULL;
   UBOOL8 found=FALSE;

   cmsLog_debug("looking for port %d vpi %d vci %d",
                webVar->portId, webVar->atmVpi, webVar->atmVci);

   if (dalWan_getDslLinkCfg(webVar, &iidStack, &dslLinkCfg) == TRUE)
   {
      cmsLog_debug("found dslLinkCfg");

      cmsObj_free((void **) &dslLinkCfg);
      found = TRUE;
   }

   return found;
}

#endif


void cli_wanShowInterfaces_igd(void)
{
#ifdef DMP_ADSLWAN_1

   WanDslIntfCfgObject *dslIntf=NULL;
   WanConnDeviceObject *connDevice=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret, ret2, ret3;
   UBOOL8 vlanMuxEnab = FALSE;

   while ((ret = cmsObj_getNext(MDMOID_WAN_DSL_INTF_CFG, &iidStack, (void **) &dslIntf)) == CMSRET_SUCCESS)
   {
      if (dslIntf->enable)
      {
         if (!cmsUtl_strcmp(dslIntf->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
         {
            /* this is the ATM WANDevice */
            UINT32 atmCount = 0;
            SINT32 vpi=0, vci=0;
            WanDslLinkCfgObject *dslLinkCfg=NULL;

            while ((ret2 = cmsObj_getNextInSubTree(MDMOID_WAN_CONN_DEVICE, &iidStack, &iidStack2, (void **) &connDevice)) == CMSRET_SUCCESS)
            {
               if ((ret3 = cmsObj_get(MDMOID_WAN_DSL_LINK_CFG, &iidStack2, 0, (void **) &dslLinkCfg)) == CMSRET_SUCCESS)
               {

                  if (atmCount == 0)
                  {
                     printf("Type\tPortId\tVPI/VCI\tLink Type\tCateg.\tencap\tEnable QoS\tEnable VLAN Mux\n");
                  }

                  printf("ATM\t%d\t", dslLinkCfg->X_BROADCOM_COM_ATMInterfaceId);

                  cmsUtl_atmVpiVciStrToNum(dslLinkCfg->destinationAddress, &vpi, &vci);
                  printf("%d/%d\t", vpi, vci);

                  printf("%s\t\t", dslLinkCfg->linkType);

                  printf("%s\t", dslLinkCfg->ATMQoS);

                  printf("%s\t", dslLinkCfg->ATMEncapsulation);

                  printf("%s\t\t", dslLinkCfg->X_BROADCOM_COM_ATMEnbQos ? "Yes" : "No");
                  vlanMuxEnab =  !cmsUtl_strcmp(dslLinkCfg->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE);
                  printf("%s\t\t", vlanMuxEnab ? "Yes" : "No");
                  printf("\n");

                  cmsObj_free((void **) &dslLinkCfg);
                  atmCount++;
               }
               else
               {
                  cmsLog_error("could not get ATM_LINK_CFG object, ret=%d", ret3);
               }

               cmsObj_free((void **) &connDevice);
            }
         }
#ifdef DMP_PTMWAN_1
         else if ((!cmsUtl_strcmp(dslIntf->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_PTM) || /* vdsl+ptm (unlikely)*/
                  !cmsUtl_strcmp(dslIntf->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM)))    /* vdsl+ptm */
         {
            /* this is the PTM WANDevice */
            UINT32 ptmCount=0;
            WanPtmLinkCfgObject *ptmCfg=NULL;


            while ((ret2 = cmsObj_getNextInSubTree(MDMOID_WAN_CONN_DEVICE, &iidStack, &iidStack2, (void **) &connDevice)) == CMSRET_SUCCESS)
            {
               if ((ret3 = cmsObj_get(MDMOID_WAN_PTM_LINK_CFG, &iidStack2, 0, (void **) &ptmCfg)) == CMSRET_SUCCESS)
               {
                  if (ptmCount == 0)
                  {
                     printf("Type\tPortId\tPriority\tEnable QoS\tEnable VLAN Mux\n");
                  }

                  printf("PTM\t%d\t", ptmCfg->X_BROADCOM_COM_PTMPortId);
                  if (ptmCfg->X_BROADCOM_COM_PTMPriorityLow && ptmCfg->X_BROADCOM_COM_PTMPriorityHigh)
                  {
                     printf("both\t\t");
                  }
                  else if (ptmCfg->X_BROADCOM_COM_PTMPriorityLow)
                  {
                     printf("normal\t\t");
                  }
                  else if (ptmCfg->X_BROADCOM_COM_PTMPriorityHigh)
                  {
                     printf("high\t\t");
                  }
                  else
                  {
                     printf("none?!\t\t");
                     cmsLog_error("no PTM priority is defined!");
                  }

                  printf("%s\t\t", ptmCfg->X_BROADCOM_COM_PTMEnbQos ? "Yes" : "No");
                  vlanMuxEnab =  !cmsUtl_strcmp(ptmCfg->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE);
                  printf("%s\t\t", vlanMuxEnab? "Yes" : "No");
                  printf("\n");

                  cmsObj_free((void **) &ptmCfg);
                  ptmCount++;
               }
               else
               {
                  cmsLog_error("could not get PTM_LINK_CFG object, ret=%d", ret3);
               }

               cmsObj_free((void **) &connDevice);
            }

         }
#endif
         else
         {
            cmsLog_error("unsupported linkEncapsulation type %s", dslIntf->linkEncapsulationUsed);
         }
      }


      cmsObj_free((void **) &dslIntf);
   }
#endif /* DMP_ADSLWAN_1 */

}


void cli_wanShowDslServices(InstanceIdStack *parentIidStack, const char *vccString, const char *linkType)
{
   InstanceIdStack iidStack;
   WanIpConnObject  *ipConn = NULL;
   WanPppConnObject *pppConn = NULL;
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   UBOOL8 found;
   InstanceIdStack ipIntfIid;
   Dev2IpInterfaceObject *ipIntf = NULL;
   char ipv6AddrStrBuf[CMS_IPADDR_LENGTH] = {0};
#endif

   cmsLog_debug("parentIidStack=%s", cmsMdm_dumpIidStack(parentIidStack));

   INIT_INSTANCE_ID_STACK(&iidStack);
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   INIT_INSTANCE_ID_STACK(&ipIntfIid);
#endif

   while (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN, parentIidStack, &iidStack,
                                  (void **)&ipConn) == CMSRET_SUCCESS)
   {
      UBOOL8  bridgeConn = FALSE;

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      found = FALSE;
      INIT_INSTANCE_ID_STACK(&ipIntfIid);
#endif

      if (vccString)
      {
         printf("%s\t", vccString);
      }
      else
      {
         printf("N/A\t");
      }

      // service name
      if ( strlen(ipConn->name) < 8 )
         printf("%s\t\t", ipConn->name);
      else
         printf("%s\t", ipConn->name);

      // interface name
      if ( strlen(ipConn->X_BROADCOM_COM_IfName) < 8 )
         printf("%s\t\t", ipConn->X_BROADCOM_COM_IfName);
      else
         printf("%s\t", ipConn->X_BROADCOM_COM_IfName);

      // protocol
      if (cmsUtl_strcmp(linkType, MDMVS_EOA) == 0)
      {
         if (strcmp(ipConn->connectionType, MDMVS_IP_ROUTED) == 0)
         {
            printf("IPoE\t");
         }
         else
         {
            printf("Bridged\t");
            bridgeConn = TRUE;
         }
      }
      else
      {
         printf("IPoA\t");
      }

#ifdef DMP_X_BROADCOM_COM_IGMP_1
      printf("%s\t", ipConn->X_BROADCOM_COM_IGMPEnabled? "Enable" : "Disable");
      printf("%s\t", ipConn->X_BROADCOM_COM_IGMP_SOURCEEnabled? "Enable" : "Disable");
#else
      printf("Disable\t");
      printf("Disable\t");
#endif /* DMP_X_BROADCOM_COM_IGMP_1 */
#ifdef DMP_X_BROADCOM_COM_MLD_1
      printf("%s\t", ipConn->X_BROADCOM_COM_MLDEnabled? "Enable" : "Disable");
      printf("%s\t", ipConn->X_BROADCOM_COM_MLD_SOURCEEnabled? "Enable" : "Disable");
#endif
      printf("%s\t", ipConn->connectionStatus? ipConn->connectionStatus : "\t");

      // wan IP address
      if (!bridgeConn)
      {
         printf("%s\t", ipConn->externalIPAddress? ipConn->externalIPAddress : "\t");

         // x.x.x.x
         if (cmsUtl_strlen(ipConn->externalIPAddress) == 7)
         {
            printf("\t");
         }
      }
      else
      {
         printf("\t\t");
      }

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
      printf("%s\t", ipConn->X_BROADCOM_COM_IPv6ConnStatus? ipConn->X_BROADCOM_COM_IPv6ConnStatus : "\t");

      // wan IP address
      if (!bridgeConn)
      {
         printf("%s\t", ipConn->X_BROADCOM_COM_ExternalIPv6Address? ipConn->X_BROADCOM_COM_ExternalIPv6Address : "\t");
      }
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
      while (!found &&
              cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &ipIntfIid, (void **)&ipIntf) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ipIntf->name, ipConn->X_BROADCOM_COM_IfName))
         {
            found = TRUE;
            printf("%s\t", ipIntf->X_BROADCOM_COM_IPv6ServiceStatus? ipIntf->X_BROADCOM_COM_IPv6ServiceStatus : "\t");
         }

         cmsObj_free((void **) &ipIntf);
      }

      if (!found)
      {
         /* In bridge mode, WAN connection is created with IPv6 disabled, there
          * is no IP.Interface object for the IPv6 part, so we will not find
          * an object.  Just print ServiceDown.
          */
         printf("%s\t", MDMVS_SERVICEDOWN);
      }

      // wan IP address
      if (!bridgeConn)
      {
         qdmIpIntf_getIpv6AddressByNameLocked(ipConn->X_BROADCOM_COM_IfName, ipv6AddrStrBuf);
         printf("%s\t", ipv6AddrStrBuf[0]? ipv6AddrStrBuf : "\t");
      }
#endif

      printf("\n");

      cmsObj_free((void **)&ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);

   while (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, parentIidStack, &iidStack,
                                  (void **)&pppConn) == CMSRET_SUCCESS)
   {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      found = FALSE;
      INIT_INSTANCE_ID_STACK(&ipIntfIid);
#endif

      if (vccString)
      {
         printf("%s\t", vccString);
      }
      else
      {
         printf("N/A\t");
      }

      // service name
      if ( strlen(pppConn->name) < 8 )
         printf("%s\t\t", pppConn->name);
      else
         printf("%s\t", pppConn->name);
      // interface name
      if ( strlen(pppConn->X_BROADCOM_COM_IfName) < 8 )
         printf("%s\t\t", pppConn->X_BROADCOM_COM_IfName);
      else
         printf("%s\t", pppConn->X_BROADCOM_COM_IfName);

      if (cmsUtl_strcmp(linkType, MDMVS_EOA) == 0)
      {
         printf("PPPoE\t");
      }
      else
      {
         printf("PPPoA\t");
      }

#ifdef DMP_X_BROADCOM_COM_IGMP_1
      printf("%s\t", pppConn->X_BROADCOM_COM_IGMPEnabled? "Enable" : "Disable");
      printf("%s\t", pppConn->X_BROADCOM_COM_IGMP_SOURCEEnabled? "Enable" : "Disable");
#else
      printf("Disable\t");
      printf("Disable\t");
#endif /* DMP_X_BROADCOM_COM_IGMP_1 */
#ifdef DMP_X_BROADCOM_COM_MLD_1
      printf("%s\t", pppConn->X_BROADCOM_COM_MLDEnabled? "Enable" : "Disable");
      printf("%s\t", pppConn->X_BROADCOM_COM_MLD_SOURCEEnabled? "Enable" : "Disable");
#endif
      printf("%s\t", pppConn->connectionStatus? pppConn->connectionStatus : "\t");

      // wan IP address
      printf("%s\t", pppConn->externalIPAddress? pppConn->externalIPAddress : "\t");

      // x.x.x.x
      if (cmsUtl_strlen(pppConn->externalIPAddress) == 7)
      {
         printf("\t");
      }

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
      printf("%s\t", pppConn->X_BROADCOM_COM_IPv6ConnStatus? pppConn->X_BROADCOM_COM_IPv6ConnStatus : "\t");
      // wan IP address
      printf("%s\t", pppConn->X_BROADCOM_COM_ExternalIPv6Address? pppConn->X_BROADCOM_COM_ExternalIPv6Address : "\t");
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
      while (!found &&
              cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &ipIntfIid, (void **)&ipIntf) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ipIntf->name, pppConn->X_BROADCOM_COM_IfName))
         {
            found = TRUE;
            printf("%s\t", ipIntf->X_BROADCOM_COM_IPv6ServiceStatus? ipIntf->X_BROADCOM_COM_IPv6ServiceStatus : "\t");
         }

         cmsObj_free((void **) &ipIntf);
      }
      // wan IP address
      qdmIpIntf_getIpv6AddressByNameLocked(pppConn->X_BROADCOM_COM_IfName, ipv6AddrStrBuf);
      printf("%s\t", ipv6AddrStrBuf[0]? ipv6AddrStrBuf : "\t");
#endif

      printf("\n");

      cmsObj_free((void **)&pppConn);
   }  /* while */

}


/***************************************************************************
// Function Name: cli_wanShow
// Description  : show specific or all wan connections.
// Returns      : status.
****************************************************************************/
CmsRet cli_wanShowServices(const char *specificVcc)
{
#define CLI_WAN_SHOW_HEADER_1_1 "VCC\tService\t\tInterface\tProto.\tIGMP\tSrc?\t"
#define CLI_WAN_SHOW_HEADER_1_2 "MLD\tSrc?\t"
#define CLI_WAN_SHOW_HEADER_1_3 "IPv4Status\tIPv4\t\t"
#define CLI_WAN_SHOW_HEADER_1_4 "IPv6Status\tIPv6\t\t"

#define CLI_WAN_SHOW_HEADER_2_1 "\tName\t\tName\t\t\t\t\t\t\t\t"
#define CLI_WAN_SHOW_HEADER_2_2 "\t"
#define CLI_WAN_SHOW_HEADER_2_3 "address\t"
#define CLI_WAN_SHOW_HEADER_2_4 "\t\t\taddress\t"


   // print header
   printf("%s", CLI_WAN_SHOW_HEADER_1_1);
#ifdef DMP_X_BROADCOM_COM_MLD_1
   printf("%s", CLI_WAN_SHOW_HEADER_1_2);
#endif
   printf("%s", CLI_WAN_SHOW_HEADER_1_3);
#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   printf("%s", CLI_WAN_SHOW_HEADER_1_4);
#endif
   printf("\n");

   printf("%s", CLI_WAN_SHOW_HEADER_2_1);
#ifdef DMP_X_BROADCOM_COM_MLD_1
   printf("%s", CLI_WAN_SHOW_HEADER_2_2);
#endif
   printf("%s", CLI_WAN_SHOW_HEADER_2_3);
#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   printf("%s", CLI_WAN_SHOW_HEADER_2_4);
#endif
   printf("\n");

   return (cli_wanShowServicesSub(specificVcc));

}



/***************************************************************************
// Function Name: cli_wanShowServicesSub_igd
// Description  : show specific or all wan connections.
// Returns      : status.
****************************************************************************/
CmsRet cli_wanShowServicesSub_igd(const char *specificVcc)
{
   InstanceIdStack commonIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanCommonIntfCfgObject *commonIntfCfg = NULL;
   char vcc[BUFLEN_32];
   SINT32 vpi, vci;

   while (cmsObj_getNext(MDMOID_WAN_COMMON_INTF_CFG, &commonIidStack, (void **)&commonIntfCfg) == CMSRET_SUCCESS)
   {
      /*
       * Outer-most loop, looks at WANCommonInterfaceConfig to
       * determine if this is a DSL (ATM/PTM) WAN connection, or ethernet connection.
       */

      if (!cmsUtl_strcmp(commonIntfCfg->WANAccessType, MDMVS_DSL))
      {
         /* DSL, either ATM or PTM */
         WanDslIntfCfgObject *dslIntfCfg = NULL;

         if ( cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, &commonIidStack, 0, (void **) &dslIntfCfg) == CMSRET_SUCCESS )
         {
            cmsLog_debug("link enacap used=%s modulation type=%s",
                          dslIntfCfg->linkEncapsulationUsed,
                          dslIntfCfg->modulationType);

            if (!cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM) || /* adsl+atm */
                !cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_ATM))    /* vdsl+atm (unlikely) */
            {
               WanDslLinkCfgObject *dslLinkCfg = NULL;

               INIT_INSTANCE_ID_STACK(&iidStack);

               while (cmsObj_getNextInSubTree(MDMOID_WAN_DSL_LINK_CFG, &commonIidStack, &iidStack, (void **)&dslLinkCfg) == CMSRET_SUCCESS)
               {
                  // format VCC
                  if (cmsUtl_atmVpiVciStrToNum(dslLinkCfg->destinationAddress, &vpi, &vci) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("could not convert destinationAddress %s", dslLinkCfg->destinationAddress);
                     strcpy(vcc, "\t");
                  }
                  else
                  {
                     sprintf(vcc, "0.%d.%d", vpi, vci);
                  }

                  if (specificVcc != NULL)
                  {
                     /* user requested a specific VCC */
                     if (strcmp(vcc, specificVcc) == 0)
                     {
                        cli_wanShowDslServices(&iidStack, vcc, dslLinkCfg->linkType);
                     }
                  }
                  else
                  {
                     /* user has not requested specific VCC, show all */
                     cli_wanShowDslServices(&iidStack, vcc, dslLinkCfg->linkType);
                  }

                  cmsObj_free((void **)&dslLinkCfg);
               }
            }
   #ifdef DMP_PTMWAN_1
            else if (!cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_PTM) || /* vdsl+ptm (unlikely)*/
                     !cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))    /* vdsl+ptm */
            {
               WanPtmLinkCfgObject *ptmLinkCfg = NULL;

               INIT_INSTANCE_ID_STACK(&iidStack);

               while (cmsObj_getNextInSubTree(MDMOID_WAN_PTM_LINK_CFG, &commonIidStack, &iidStack, (void **)&ptmLinkCfg) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("show all PTM services under %s", cmsMdm_dumpIidStack(&iidStack));
                  cli_wanShowDslServices(&iidStack, NULL, MDMVS_EOA);
                  cmsObj_free((void **)&ptmLinkCfg);
               }
            }
   #endif
            else
            {
               cmsLog_error("unrecognized or unsupported linkEncapsulation %s", dslIntfCfg->linkEncapsulationUsed);
            }

            cmsObj_free((void **) &dslIntfCfg);
         }
      }
#ifdef DMP_ETHERNETWAN_1  /* aka SUPPORT_ETHWAN */
      else if (!cmsUtl_strcmp(commonIntfCfg->WANAccessType, MDMVS_ETHERNET))
      {
         WanEthLinkCfgObject *ethLinkCfg = NULL;

         INIT_INSTANCE_ID_STACK(&iidStack);

         while (cmsObj_getNextInSubTree(MDMOID_WAN_ETH_LINK_CFG, &commonIidStack, &iidStack, (void **)&ethLinkCfg) == CMSRET_SUCCESS)
         {
            cmsLog_debug("show all ETHWAN services under %s", cmsMdm_dumpIidStack(&iidStack));
            cli_wanShowDslServices(&iidStack, NULL, MDMVS_EOA);
            cmsObj_free((void **)&ethLinkCfg);
         }
      }
#endif
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
      else if (!cmsUtl_strcmp(commonIntfCfg->WANAccessType, MDMVS_X_BROADCOM_COM_L2TPAC))
      {
         /* cwu_todo */
      }
#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */

#ifdef DMP_X_BROADCOM_COM_PONWAN_1

      else if (!cmsUtl_strcmp(commonIntfCfg->WANAccessType, MDMVS_X_BROADCOM_COM_PON))
      {
         /* PON interfaces, (gpon, etc) */
         WanPonIntfObject *ponIntfCfg = NULL;

         if (cmsObj_get(MDMOID_WAN_PON_INTF, &commonIidStack, 0, (void **) &ponIntfCfg) == CMSRET_SUCCESS)
         {


#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
            if (!cmsUtl_strcmp(ponIntfCfg->ponType, MDMVS_GPON))
            {
               WanGponLinkCfgObject *gponLinkCfg = NULL;

               INIT_INSTANCE_ID_STACK(&iidStack);

               while (cmsObj_getNextInSubTree(MDMOID_WAN_GPON_LINK_CFG, &commonIidStack, &iidStack, (void **)&gponLinkCfg) == CMSRET_SUCCESS)
               {
                  cli_wanShowDslServices(&iidStack, NULL, MDMVS_EOA);
                  cmsObj_free((void **)&gponLinkCfg);
               }
            }
#endif

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
            if (!cmsUtl_strcmp(ponIntfCfg->ponType, MDMVS_EPON))
            {
               WanEponLinkCfgObject *eponLinkCfg = NULL;

               INIT_INSTANCE_ID_STACK(&iidStack);

               while (cmsObj_getNextInSubTree(MDMOID_WAN_EPON_LINK_CFG, &commonIidStack, &iidStack, (void **)&eponLinkCfg) == CMSRET_SUCCESS)
               {
                  cli_wanShowDslServices(&iidStack, NULL, MDMVS_EOA);
                  cmsObj_free((void **)&eponLinkCfg);
               }
            }
#endif
            cmsObj_free((void **)&ponIntfCfg);
         }
      }
#endif /* DMP_X_BROADCOM_COM_PONWAN_1 */

      else
      {
         cmsLog_error("unknown or unsupported WAN access type %s", commonIntfCfg->WANAccessType);
      }

      cmsObj_free((void **)&commonIntfCfg);
   }

   return CMSRET_SUCCESS;

}  /* End of cli_wanShow() */


#endif   /* SUPPORT_CLI_CMD */
