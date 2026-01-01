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

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_qos.h"
#include "cms_net.h"
#include "dal_network.h"




CmsRet dalWan_setDefaultGatewayAndDns(char *webVarGwIfcs, char *webVarDnsIfcs, char *webVarDns1, char *webVarDns2)
{
   CmsRet ret;
   
   cmsLog_debug("webVarGwIfcs=%s webVarDnsIfcs=%s webVarDns1=%s webVarDns2=%s",
                webVarGwIfcs, webVarDnsIfcs, webVarDns1, webVarDns2);
   
  
   if ((ret = dalRt_setDefaultGatewayList(webVarGwIfcs)) != CMSRET_SUCCESS)
   {
      cmsLog_error("dalRt_setDefaultGatewayList failed. ret=%d", ret);
   }
   else 
   {
      /* does this func support IPv6?  existing code implies it is IPv4 only. */
      if (cmsUtl_strcmp(webVarDnsIfcs, ""))
      {
         /*
          * If we want to use DnsIfNameList, we must delete any static DNS
          * servers because static DNS servers have higher precedence.
          */
         dalDns_deleteAllStaticIpvxDnsServers(CMS_AF_SELECT_IPV4);
         ret = dalDns_setIpvxDnsIfNameList(CMS_AF_SELECT_IPV4, webVarDnsIfcs);
      }
      else if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, webVarDns1))
      {
         ret = dalDns_setStaticIpvxDnsServers(CMS_AF_SELECT_IPV4, webVarDns1, webVarDns2);
      }
   }

   cmsLog_debug("Exit. ret=%d", ret);
   
   return ret;
   
}


extern UBOOL8 rutWan_IsPPPIpExtension(void);

UBOOL8 dalWan_isPPPIpExtension(void)
{
   return (rutWan_IsPPPIpExtension());   
}

#ifdef DMP_X_BROADCOM_COM_IPV6_1
/* deprecated proprietary Broadcom IPv6 */
void removeIfcFromNetworkCfg6(MdmObjectId oid, InstanceIdStack *iidStackDel)
{
   MdmPathDescriptor pathDesc;
   IPv6LanHostCfgObject   *lanCfg = NULL;
   IPv6L3ForwardingObject *l3fwd  = NULL;
   InstanceIdStack iidStack;
   char *connDeleted;
   UBOOL8 lanCfgChanged = FALSE;
   CmsRet ret;
   
   pathDesc.oid      = oid;
   pathDesc.iidStack = *iidStackDel;
   pathDesc.paramName[0] = '\0';

   if ((ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, &connDeleted)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. ret=%d", ret);
      return;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_getNext(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack, (void **)&lanCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
      cmsMem_free(connDeleted);
      return;
   }
   if (cmsUtl_strcmp(lanCfg->IPv6DNSWANConnection, connDeleted) == 0)
   {
      cmsMem_free(lanCfg->IPv6DNSWANConnection);
      lanCfg->IPv6DNSWANConnection = NULL;
      lanCfgChanged = TRUE;
   }
   if (lanCfgChanged)
   {
      if ((ret = cmsObj_set((void *)lanCfg, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set returns error. ret=%d", ret);
         cmsObj_free((void **)&lanCfg);
         cmsMem_free(connDeleted);
         return;
      }
   }
   cmsObj_free((void **)&lanCfg);

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_I_PV6_L3_FORWARDING, &iidStack, 0, (void **)&l3fwd)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_I_PV6_L3_FORWARDING> returns error. ret=%d", ret);
      cmsMem_free(connDeleted);
      return;
   }
   if (cmsUtl_strcmp(l3fwd->defaultConnectionService, connDeleted) == 0)
   {
      cmsMem_free(l3fwd->defaultConnectionService);
      l3fwd->defaultConnectionService = NULL;

      if ((ret = cmsObj_set((void *)l3fwd, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set returns error. ret=%d", ret);
         cmsObj_free((void **)&l3fwd);
         cmsMem_free(connDeleted);
         return;
      }
   }
   cmsObj_free((void **)&l3fwd);
   cmsMem_free(connDeleted);

}  /* End of removeIfcFromNetworkCfg6() */

#endif   /* DMP_X_BROADCOM_COM_IPV6_1 */

