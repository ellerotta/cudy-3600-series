/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2020:proprietary:standard

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
#ifdef DMP_X_BROADCOM_COM_PPTPNS_2
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "dal_wan.h"
#include "rut2_pptpns.h"

#define PS_BUF_STR_LEN  512
#define LINE_BUF_STR_LEN  128

CmsRet dalPptpNsCfg(PPTPNS pptpns)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2PppInterfaceObject *pppIntfObj=NULL;
   Dev2PppInterfacePptpNsObject *pptpNsCfg = NULL;
   UBOOL8 found = FALSE; 
   
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&pppIntfObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(pppIntfObj->name, "PptpNs"))
      {
         InstanceIdStack pptpNsIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_PPTP_NS, &iidStack, 
                                             &pptpNsIidStack, (void **) &pptpNsCfg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get pptpNsCfg, error=%d", ret);
            cmsObj_free((void **)&pppIntfObj);
            return ret;
         }
         
         found = TRUE;
         cmsLog_debug("pptpns object found");          
                      
         if (pptpNsCfg)
         {
            if (strcmp(pptpns.enablepptpns, "1") == 0)
            {
               pptpNsCfg->enable = TRUE;
               CMSMEM_REPLACE_STRING(pptpNsCfg->pptpnsName, pptpns.pptpnsname);
               CMSMEM_REPLACE_STRING(pptpNsCfg->pptpnsLocalIp, pptpns.pptpnslocalip);
               CMSMEM_REPLACE_STRING(pptpNsCfg->peerSatrtIp, pptpns.peerstartip);
               CMSMEM_REPLACE_STRING(pptpNsCfg->peerEndIp, pptpns.peerendip);
               CMSMEM_REPLACE_STRING(pptpNsCfg->username, pptpns.pptpnsusername);
               CMSMEM_REPLACE_STRING(pptpNsCfg->password, pptpns.pptpnspassword);
               if (strcmp(pptpns.enablechap, "1") == 0) 
                  pptpNsCfg->enableCHAP = TRUE;
               else 
                  pptpNsCfg->enableCHAP = FALSE;
                  
               if (strcmp(pptpns.enablepap, "1") == 0) 
                  pptpNsCfg->enablePAP = TRUE;
               else 
                  pptpNsCfg->enablePAP = FALSE;   
                  
               if (strcmp(pptpns.enablemschap, "1") == 0) 
                  pptpNsCfg->enableMSCHAP = TRUE;
               else 
                  pptpNsCfg->enableMSCHAP = FALSE;     
                  
               if (strcmp(pptpns.enablemschapv2, "1") == 0) 
                  pptpNsCfg->enableMSCHAPv2 = TRUE;
               else 
                  pptpNsCfg->enableMSCHAPv2 = FALSE;  
                  
               if (strcmp(pptpns.enabledebugpptpns, "1") == 0) 
                  pptpNsCfg->enableDebugPptpns = TRUE;
               else 
                  pptpNsCfg->enableDebugPptpns = FALSE;  
            }
            else
            {
               pptpNsCfg->enable = FALSE;
            }

            ret = cmsObj_set(pptpNsCfg, &iidStack);
            cmsObj_free((void **) &pptpNsCfg);
         }
      }
      else
      {
         cmsObj_free((void **)&pppIntfObj);
      }
   }
   
   if (found)
      cmsObj_free((void **)&pppIntfObj);
   else
   {
      /* Add PPP interface instance*/
      cmsLog_debug("Create new pptpns object");  
       
      if ((ret = cmsObj_addInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to add PPP intf Instance, ret = %d", ret);
         return ret;
      } 
    
      /* Get PPP interface object */
      if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE, &iidStack, 0, (void **) &pppIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get pppIntfObj, ret = %d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
         return ret;
      }
   
      if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_PPTP_NS, &iidStack, 0, (void **) &pptpNsCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get Dev2PppInterfacePptpNsObject, ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE_PPTP_NS, &iidStack);
         cmsObj_free((void **) &pppIntfObj);   
         return ret;
      }

      pppIntfObj->enable = TRUE;

      /* fill in service name */
      CMSMEM_REPLACE_STRING(pppIntfObj->name, "PptpNs");   /* todo. just assign some thing for now */

      /* Set and Activate PPP interface */
      if ((ret = cmsObj_set(pppIntfObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Set pppIntf failed , ret = %d", ret);
      }
      cmsObj_free((void **) &pppIntfObj);

      if (strcmp(pptpns.enablepptpns, "1") == 0)
      {
         pptpNsCfg->enable = TRUE;
         CMSMEM_REPLACE_STRING(pptpNsCfg->pptpnsName, pptpns.pptpnsname);
         CMSMEM_REPLACE_STRING(pptpNsCfg->pptpnsLocalIp, pptpns.pptpnslocalip);
         CMSMEM_REPLACE_STRING(pptpNsCfg->peerSatrtIp, pptpns.peerstartip);
         CMSMEM_REPLACE_STRING(pptpNsCfg->peerEndIp, pptpns.peerendip);
         CMSMEM_REPLACE_STRING(pptpNsCfg->username, pptpns.pptpnsusername);
         CMSMEM_REPLACE_STRING(pptpNsCfg->password, pptpns.pptpnspassword);       
           
         if (strcmp(pptpns.enablechap, "1") == 0) 
            pptpNsCfg->enableCHAP = TRUE;
         else 
            pptpNsCfg->enableCHAP = FALSE;
                  
         if (strcmp(pptpns.enablepap, "1") == 0) 
            pptpNsCfg->enablePAP = TRUE;
         else 
            pptpNsCfg->enablePAP = FALSE;   
                  
         if (strcmp(pptpns.enablemschap, "1") == 0) 
            pptpNsCfg->enableMSCHAP = TRUE;
         else 
            pptpNsCfg->enableMSCHAP = FALSE;     
                  
         if (strcmp(pptpns.enablemschapv2, "1") == 0) 
            pptpNsCfg->enableMSCHAPv2 = TRUE;
         else 
            pptpNsCfg->enableMSCHAPv2 = FALSE;  
                  
         if (strcmp(pptpns.enabledebugpptpns, "1") == 0) 
            pptpNsCfg->enableDebugPptpns = TRUE;
         else 
            pptpNsCfg->enableDebugPptpns = FALSE;  
      }
      else
      {
         pptpNsCfg->enable = FALSE;
      }

      ret = cmsObj_set(pptpNsCfg, &iidStack);
      cmsObj_free((void **) &pptpNsCfg);
   }

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2PppInterfacePptpNsObject, ret = %d", ret);
   }
   
   return ret;
}

CmsRet dalGetPptpNsInfo(char *info)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2PppInterfaceObject *pppIntfObj = NULL;
   Dev2PppInterfacePptpNsObject *pptpNsCfg = NULL;
   UBOOL8 found = FALSE;
   
   if(info == NULL)
   {
      return CMSRET_INVALID_PARAM_TYPE;
   }
   else
   {
      strcpy(info, "0|||||||1|1|1|1|1"); //set default value to info string
   }
   
   /* Look thru  all l interfaces and check for the lowerlayer matches with
   * the layer 2 interface (vlantermation here).  
   */
   
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&pppIntfObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(pppIntfObj->name, "PptpNs"))
      {
         InstanceIdStack pptpNsIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_PPTP_NS, &iidStack, 
                                             &pptpNsIidStack, (void **) &pptpNsCfg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get pptpNsCfg, error=%d", ret);
            cmsObj_free((void **)&pppIntfObj);
            return ret;
         }

         found = TRUE;

         if (pptpNsCfg)
         {
             sprintf(info, "%d|%s|%s|%s|%s|%s|%s|%d|%d|%d|%d|%d", 
             pptpNsCfg->enable, pptpNsCfg->pptpnsName, 
             pptpNsCfg->pptpnsLocalIp, pptpNsCfg->peerSatrtIp, pptpNsCfg->peerEndIp,
             pptpNsCfg->username, pptpNsCfg->password,
             pptpNsCfg->enableCHAP, pptpNsCfg->enablePAP, pptpNsCfg->enableMSCHAP, 
             pptpNsCfg->enableMSCHAPv2, pptpNsCfg->enableDebugPptpns);
             cmsObj_free((void **) &pptpNsCfg);
         }
      }
      cmsObj_free((void **)&pppIntfObj);
   }
   
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_PPTPNS_2 */
