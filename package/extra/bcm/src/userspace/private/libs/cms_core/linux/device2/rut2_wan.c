/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1



#include "cms_core.h"
#include "cms_util.h"
#include "rut_wan.h"
#include "cms_qdm.h"
#include "rut_ebtables.h"



UBOOL8 rutWan_findFirstRoutedAndConnected_dev2(char *ifName)
{
   return (rutWan_findFirstIpvxRoutedAndConnected_dev2(CMS_AF_SELECT_IPV4, ifName));
}


UBOOL8 rutWan_findFirstIpvxRoutedAndConnected_dev2(UINT32 ipvx, char *ifName)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;


   cmsLog_debug("Enter: ipvx=%d", ipvx);
   ifName[0] = '\0';

   /*
    * Loop through all IP.Interface entries looking for an Upstream,
    * and IPv4ServiceStatus == SERVICEUP or IPv6ServiceStatus == SERVICEUP,
    * depending on ipvx.
    */
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream &&
          !ipIntfObj->X_BROADCOM_COM_BridgeService)
      {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
         if ((ipvx & CMS_AF_SELECT_IPV6) &&
             !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP))
         {
            found = TRUE;
            strcpy(ifName, ipIntfObj->name);
            cmsLog_debug("found IPv6 routed and connected ifName %s", ifName);
         }
#endif

         if (!found &&
             (ipvx & CMS_AF_SELECT_IPV4) &&
             !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP))
         {
            found = TRUE;
            strcpy(ifName, ipIntfObj->name);
            cmsLog_debug("found IPv4 routed and connected ifName %s", ifName);
         }
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   cmsLog_debug("Exit: found=%d ifName=%s", found, ifName);

   return found;
}


UBOOL8 rutWan_findFirstNattedAndConnected_dev2(char *natIntfName, const char *excludeIntfName)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   cmsLog_debug("Entered: excludeIntfName=%s", excludeIntfName);

   natIntfName[0] = '\0';

   /*
    * Loop through all IP.Interface entries looking for an Upstream,
    * and IPv4ServiceStatus == SERVICEUP and
    * depending on ipvx.
    */
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if ((ipIntfObj->X_BROADCOM_COM_Upstream) &&
          (!ipIntfObj->X_BROADCOM_COM_BridgeService) &&
          (!cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP)) &&
          (cmsUtl_strcmp(ipIntfObj->name, excludeIntfName)) &&
          (qdmIpIntf_isNatEnabledOnIntfNameLocked(ipIntfObj->name)))
      {
         found = TRUE;
         strcpy(natIntfName, ipIntfObj->name);
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   cmsLog_debug("Exit: found=%d natIntfName=%s", found, natIntfName);

   return found;
}


CmsRet rutWan_getDhcpDeviceInfo_dev2(char *oui, char *serialNum, char *productClass)
{
   Dev2DeviceInfoObject *deviceInfoObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get device info object!, ret=%d", ret);
   }
   else
   {
      if (oui && deviceInfoObj->manufacturerOUI)
         sprintf(oui, "%s", deviceInfoObj->manufacturerOUI);
      if (serialNum && deviceInfoObj->serialNumber)
         sprintf(serialNum, "%s", deviceInfoObj->serialNumber);
      if (productClass && deviceInfoObj->productClass)
         sprintf(productClass, "%s", deviceInfoObj->productClass);

      cmsObj_free((void**)&deviceInfoObj);
   }

   return ret;
}


CmsRet rutWan_fillInPPPIndexArray_dev2(SINT32 *intfArray)
{
   SINT32 index = 0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2PppInterfaceObject *pppIntf;

   while (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack, OGF_NORMAL_UPDATE, (void **) &pppIntf) == CMSRET_SUCCESS)
   {
      if (pppIntf->name == NULL)
      {
         cmsLog_debug("This is one in creating so ifName is NULL, continue...");
         /* this is one we just created and is NULL, so just skip it */
      }
      else
      {
         index = atoi(&(pppIntf)->name[cmsUtl_strlen(PPP_IFC_STR)]);

         cmsLog_debug("pppIntf->name =%s, index=%d", pppIntf->name , index);

         if (index > IFC_WAN_MAX)
         {
            cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
            cmsObj_free((void **) &pppIntf);
            return CMSRET_INTERNAL_ERROR;
         }
         *(intfArray+index) = 1;            /* mark the interface used */
      }
      cmsObj_free((void **) &pppIntf);
      
   }

   return CMSRET_SUCCESS;
   
}

CmsRet rutWan_initPPPoE_dev2(const InstanceIdStack *iidStack __attribute__((unused)), void *obj)
{
   SINT32 pppPid=0;
   char cmdLine[BUFLEN_256]={0};
   char serverFlag[BUFLEN_64]={0};
   char staticIPAddrFlag[BUFLEN_32]={0};
   char passwordFlag[BUFLEN_32]={0};
   char idlelimit[BUFLEN_32] = {0};
   CmsRet ret=CMSRET_SUCCESS;
   InstanceIdStack pppoeIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack vlaniidStack = EMPTY_INSTANCE_ID_STACK;   
   _Dev2PppInterfacePpoeObject *pppoeObj = NULL;
   _Dev2VlanTerminationObject *vlanObj = NULL;
   UBOOL8 found = FALSE;
   SINT32 vlanMuxId=-1, vlanMuxPr=-1;
   UINT32 vlanTpid=0;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   _Dev2PppInterfaceObject *newObj = (_Dev2PppInterfaceObject *) obj;
   
   cmsLog_debug("Enter");

   /* From lowerlayer full path, get the lowerlayer ifName (baseL3IfName) */
   if (IS_EMPTY_STRING(newObj->lowerLayers))
   {
      cmsLog_error("PPP interface is not pointing to lowerlayer object!");
      return CMSRET_INTERNAL_ERROR;
   }      

   if (rut_wanGetIntfIndex(newObj->name) > 0)
   {
      cmsLog_debug("ppp interface is already created");
      return ret;
   }


   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(newObj->lowerLayers, 
                                                         baseL3IfName, 
                                                         sizeof(baseL3IfName))) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked_dev2 failed. ret %d", ret);
      return ret;
   }
   
   if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_PPOE, 
                                          iidStack, 
                                          &pppoeIidStack,
                                          (void **) &pppoeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get pppoeObj, ret=%d", ret);
      return ret;
   }


  /* password is an optional parameter */
   if (newObj->password && newObj->password[0] != '\0')
   {
      snprintf(passwordFlag, sizeof(passwordFlag), "-p %s", newObj->password);
   }

   /*
    *  server name (on web display it is service name)
    * The -r argument is passed to pppoe.  So it cannot be an empty string,
    * and it cannot be a string containing any white spaces.  The check below
    * only guards against empty string, but it should also guard against white
    * space.  So far, this has not been a problem.
    */

   if ((pppoeObj->serviceName != NULL) && (strlen(pppoeObj->serviceName ) > 0))
   {
      snprintf(serverFlag, sizeof(serverFlag), "rp_pppoe_service %s", pppoeObj->serviceName);
   }

   /* static IP address */
// for IPV4 only, check ipcp for IPv4 ?   if (newObj->X_BROADCOM_COM_IPv4Enabled && newObj->X_BROADCOM_COM_UseStaticIPAddress)
   if (newObj->X_BROADCOM_COM_UseStaticIPAddress)
   {
      snprintf(staticIPAddrFlag, sizeof(staticIPAddrFlag), "%s:", newObj->X_BROADCOM_COM_LocalIPAddress);
   }
  
   snprintf(cmdLine, sizeof(cmdLine), "nodetach plugin /lib%s/rp-pppoe.so "
            "%s %s %s linkname %s user %s password %s nodeflate",
#if INTPTR_MAX == INT64_MAX   /* 64-bit platform */
            "64",
#else
            "",
#endif        
            baseL3IfName, serverFlag, staticIPAddrFlag,
            newObj->name,
            newObj->username, newObj->password);

/*
    pppd command list example:
    pppd debug nodetach plugin /lib/rp-pppoe.so eth4.1 linkname ppp0.1 user abc password abc nodeflate &
*/
   
/*  PPPAuthenticationProtocol 
    0 is AUTO 
    1 is PAP => require-pap 
    2 is CHAP => require-chap
    3 is MSCHAP => require-mschap
*/
   
   switch (cmsUtl_pppAuthToNum(newObj->authenticationProtocol)) {
   case PPP_AUTH_METHOD_PAP:
       strncat(cmdLine, " require-pap refuse-chap refuse-mschap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_CHAP:
       strncat(cmdLine, " require-chap refuse-pap refuse-mschap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_MSCHAP:
       strncat(cmdLine, " require-mschap refuse-pap refuse-chap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_AUTO: 
   default:
       break;
   }

   /* enable ppp debugging if it is selected */
   if (newObj->X_BROADCOM_COM_Enable_Debug)
   {
      strncat(cmdLine, " debug", sizeof(cmdLine)-1);
   }

   /* if on demand is selected add the parameter.  IdleDisconnectTime is in seconds */
   if (newObj->idleDisconnectTime > 0)
   {
      snprintf(idlelimit, sizeof(idlelimit), " demand idle %d", newObj->idleDisconnectTime);
      strncat(cmdLine, idlelimit, sizeof(cmdLine) -1);
   }

   /* IP extension */
   if (pppoeObj->X_BROADCOM_COM_IPExtension)
   {
      strncat(cmdLine, " -x", sizeof(cmdLine)-1);
   }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
   /* IPv6 ??? */
   if (newObj->IPv6CPEnable)
   {
      strncat(cmdLine, " +ipv6", sizeof(cmdLine)-1);
   }

   /* disable IPCP if it's IPv6 only */
   if (!newObj->IPCPEnable)
   {
      strncat(cmdLine, " noip", sizeof(cmdLine)-1);
   }
#endif

   while ((!found) &&
         ((ret = cmsObj_getNextFlags(MDMOID_DEV2_VLAN_TERMINATION, &vlaniidStack, OGF_NO_VALUE_UPDATE, (void **)&vlanObj)) == CMSRET_SUCCESS))
   {
      if (found = (0 == cmsUtl_strcmp(vlanObj->name, baseL3IfName)))
      {
         vlanMuxId = vlanObj->VLANID;
         vlanMuxPr = vlanObj->X_BROADCOM_COM_Vlan8021p;
         vlanTpid = vlanObj->TPID;     
         if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(vlanObj->lowerLayers, l2IfName)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed, error=%d", ret);
         }
      }
      cmsObj_free((void **) &vlanObj);
   }

   ret = rutWan_configPPPoE(cmdLine, l2IfName, baseL3IfName, pppoeObj->X_BROADCOM_COM_AddPppToBridge, vlanMuxId, vlanMuxPr, vlanTpid, &pppPid);
   cmsObj_free((void **) &pppoeObj);
   
   if (ret == CMSRET_SUCCESS)
   {
      newObj->X_BROADCOM_COM_Pid = pppPid;
   }

   cmsLog_debug("ret %d", ret);
   return ret;
}

   
CmsRet rutWan_cleanUpPPPoE_dev2(const InstanceIdStack *iidStack, const void *obj)
{
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret=CMSRET_SUCCESS;
   _Dev2PppInterfaceObject *currObj = (_Dev2PppInterfaceObject *) obj;
   InstanceIdStack pppoeIidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2PppInterfacePpoeObject *pppoeObj = NULL;
   UINT32 specificEid;
   
   /* Need baseL3IfName for PPPoE */
   /* From lowerlayer full path, get the lowerlayer ifName (baseL3IfName) */
   if (IS_EMPTY_STRING(currObj->lowerLayers))
   {
      cmsLog_error("PPP interface is not pointing to lowerlayer object!");
      return CMSRET_INTERNAL_ERROR;
   }      

   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(currObj->lowerLayers, 
                                                         baseL3IfName, 
                                                         sizeof(baseL3IfName))) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked_dev2 failed. ret %d", ret);
      return ret;
   }
   
   if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_PPOE, 
                                          iidStack, 
                                          &pppoeIidStack,
                                          (void **) &pppoeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get pppoeObj, ret=%d", ret);
      return ret;
   }
  
   cmsObj_free((void **) &pppoeObj);

   return ret;
}

CmsRet rutDsl_initPPPoA_dev2(const InstanceIdStack * iidStack  __attribute__((unused)), void *obj)
{
   SINT32 vpi=0;
   SINT32 vci=0;
   UBOOL8 isVCMux;
   SINT32 portId=0;   
   char atmEncap[BUFLEN_16]={0};   
   char staticIPAddrFlag[BUFLEN_32];
   char passwordFlag[BUFLEN_32];
   char cmdLine[BUFLEN_256];
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   SINT32 pid=0;
   CmsRet ret=CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;
   _Dev2AtmLinkObject *atmLinkObject = NULL;     // For Encapsulation ,DestinationAddress 
   _Dev2PppInterfaceObject *newObj = (_Dev2PppInterfaceObject *) obj;

   cmsLog_debug("Enter:");

   if((ret = cmsMdm_fullPathToPathDescriptor(newObj->lowerLayers,&pathDesc)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if(pathDesc.oid != MDMOID_DEV2_ATM_LINK) //For PPPoA, lowlayer OID must be MDMOID_DEV2_ATM_LINK
   {
      cmsLog_error("could not get atmLinkObject object!, lowlayer oid:%d", pathDesc.oid);   
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **) &atmLinkObject)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get atmLinkObject object!, ret=%d", ret);
      return ret;
   }

   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(newObj->lowerLayers,
                              l2IfName, sizeof(l2IfName))) != CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &atmLinkObject);
      cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked_dev2 failed. ret %d", ret);
      return ret;
   }

   if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(atmLinkObject->destinationAddress, &vpi, &vci)) != CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &atmLinkObject);
      return ret;
   }

   if (!cmsUtl_strcmp(atmLinkObject->encapsulation, MDMVS_LLC))
   {
      isVCMux = 0;
   }
   else
   {
      isVCMux = 1;
   }

#ifdef DMP_DEVICE2_X_BROADCOM_COM_ATMLINK_1
   portId = atmLinkObject->X_BROADCOM_COM_ATMInterfaceId;
#endif

   cmsObj_free((void **) &atmLinkObject);

   if (!isVCMux)
   {
      /* VCMux is default for pppoa - no flag.  If non default, ie. LLC, add the flag "-l" */
       strncpy(atmEncap, "llc-encaps", sizeof(atmEncap)-1);      
   }

   cmsLog_debug("%d/%d/%d atmEncap=%s", portId, vpi, vci, atmEncap);

    /* original code in BcmPppoe_startPppd */
   passwordFlag[0] = staticIPAddrFlag[0] = '\0';

   /* password is an optional parameter */
   if (newObj->password && newObj->password[0] != '\0')
   {
      snprintf(passwordFlag, sizeof(passwordFlag), "-p %s", newObj->password);
   }

   /* static IP address */
   if (newObj->X_BROADCOM_COM_UseStaticIPAddress)
   {
       snprintf(staticIPAddrFlag, sizeof(staticIPAddrFlag), "%s:", newObj->X_BROADCOM_COM_LocalIPAddress);
   }
   
/*   
     snprintf(cmdLine, sizeof(cmdLine), "%s -a %s.%d.%d.%d -u %s %s %s -f %d %s",
      newObj->name,
      l2IfName, portId, vpi, vci,
      newObj->username, passwordFlag,
      atmEncap, cmsUtl_pppAuthToNum(newObj->authenticationProtocol), staticIPAddrFlag); 
*/
    snprintf(cmdLine, sizeof(cmdLine),
      "nodetach plugin /lib/pppoatm.so %s.%d.%d.%d linkname %s user %s password %s %s %s nodeflate",
      l2IfName, portId, vpi, vci, newObj->name,
      newObj->username, newObj->password,
      atmEncap, staticIPAddrFlag); 

/* PPPAuthenticationProtocol 
   0 is AUTO 
   1 is PAP => require-pap 
   2 is CHAP => require-chap
   3 is MSCHAP => require-mschap
*/

   switch (cmsUtl_pppAuthToNum(newObj->authenticationProtocol)) {
   case PPP_AUTH_METHOD_PAP:
       strncat(cmdLine, " require-pap refuse-chap refuse-mschap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_CHAP:
       strncat(cmdLine, " require-chap refuse-pap refuse-mschap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_MSCHAP:
       strncat(cmdLine, " require-mschap refuse-pap refuse-chap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_AUTO: 
   default:
       break;
   }

   /* enable ppp debugging if it is selected */
   if (newObj->X_BROADCOM_COM_Enable_Debug)
   {
       strncat(cmdLine, " debug", sizeof(cmdLine)-1);
   }

#ifdef LATE_TODO
   /* TODO: IP extension */
   if (newObj->X_BROADCOM_COM_IPExtension)
   {
      strncat(cmdLine, " -x", sizeof(cmdLine)-1);
   }
#endif

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
   /* IPv6 ??? */
   if (newObj->IPv6CPEnable)
   {
      strncat(cmdLine, " +ipv6", sizeof(cmdLine)-1);
   }

   /* disable IPCP if it's IPv6 only */
   if (!newObj->IPCPEnable)
   {
      strncat(cmdLine, " noip", sizeof(cmdLine)-1);
   }
#endif

   ret = rutDsl_configPPPoA(cmdLine,newObj->name,&(pid));
   if (ret == CMSRET_SUCCESS)
   {
      newObj->X_BROADCOM_COM_Pid = pid;
   }

   return ret;
}


// Even though the following 2 functions are for PON devices only,
// make them available on all builds to avoid messy ifdefs.  These functions
// are small anyways.
UBOOL8 rutOptical_getIntfByIfName(const char *ifName __attribute__((unused)),
                           InstanceIdStack *iidStack __attribute__((unused)),
                 OpticalInterfaceObject **optIntfObj __attribute__((unused)))
{
#ifdef DMP_DEVICE2_OPTICAL_1
    OpticalInterfaceObject *mdmObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;
    int found = FALSE;
    if (ifName == NULL)
    {
        cmsLog_error("ifName is NULL!");
        return FALSE;
    }
    if (IS_EMPTY_STRING(ifName))
    {
        cmsLog_error("ifName is empty string, probably an error");
        return FALSE;
    }
    while (cmsObj_getNextFlags(MDMOID_OPTICAL_INTERFACE, &localIidStack, OGF_NO_VALUE_UPDATE, (void **) &mdmObj) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(mdmObj->name, ifName) == 0)
        {
 
            found = TRUE;
            if (optIntfObj != NULL)
            {
                // pass mdmObj back to caller, who is responsible for free
                *optIntfObj = mdmObj;
            }
            else
            {
                // caller did not want the mdmObj, so just free it here.
                cmsObj_free((void **) &mdmObj);
            }
            if (iidStack != NULL)
            {
                *iidStack = localIidStack;
            }
            break;
        }
        cmsObj_free((void **) &mdmObj);
    }
    return found;
#else
    // We will get unknown OID error if we try to access the Optical.Interface 
    // obj on a non-optical enabled system, so just return FALSE.
    return FALSE;
#endif
} 


UBOOL8 rutOptical_getIntfByIfNameEnabled(const char *ifName, InstanceIdStack *iidStack, OpticalInterfaceObject **optIntfObj, UBOOL8 enabled)
{
    OpticalInterfaceObject *mdmObj = NULL;

    if (rutOptical_getIntfByIfName(ifName, iidStack, &mdmObj) == FALSE)
        return FALSE;

    if (mdmObj->enable == enabled)
        *optIntfObj = mdmObj;
    else
        cmsObj_free((void **) &mdmObj);

    return *optIntfObj != NULL;
}

#endif /* DMP_DEVICE2_BASELINE_1 */

