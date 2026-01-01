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
#ifdef DMP_X_BROADCOM_COM_L2TPNS_2
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "dal_wan.h"
#include "rut2_l2tpns.h"

#define PS_BUF_STR_LEN  512
#define LINE_BUF_STR_LEN  128

CmsRet dalL2tpNsCfg(L2TP_LNS lns)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2PppInterfaceObject *pppIntfObj=NULL;
   Dev2PppInterfaceL2tpNsObject *l2tpNsCfg = NULL;
   UBOOL8 found = FALSE; 
   
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&pppIntfObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(pppIntfObj->name, "L2tpNs"))
      {
         InstanceIdStack l2tpNsIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_L2TP_NS, &iidStack, 
                                             &l2tpNsIidStack, (void **) &l2tpNsCfg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get l2tpNsCfg, error=%d", ret);
            cmsObj_free((void **)&pppIntfObj);
            return ret;
         }
         
         found = TRUE;
         cmsLog_debug("l2tpns object found");          
                      
         if (l2tpNsCfg)
         {
            if (strcmp(lns.enablelns, "1") == 0)
            {
               l2tpNsCfg->enable = TRUE;
               CMSMEM_REPLACE_STRING(l2tpNsCfg->lnsName, lns.l2tpnsname);
               CMSMEM_REPLACE_STRING(l2tpNsCfg->lnsLocalIp, lns.l2tpnslocalip);
               CMSMEM_REPLACE_STRING(l2tpNsCfg->peerSatrtIp, lns.peerstartip);
               CMSMEM_REPLACE_STRING(l2tpNsCfg->peerEndIp, lns.peerendip);
               CMSMEM_REPLACE_STRING(l2tpNsCfg->username, lns.l2tpnsusername);
               CMSMEM_REPLACE_STRING(l2tpNsCfg->password, lns.l2tpnspassword);
               if (strcmp(lns.enablechap, "1") == 0) 
                  l2tpNsCfg->enableCHAP = TRUE;
               else 
                  l2tpNsCfg->enableCHAP = FALSE;
                  
               if (strcmp(lns.enablepap, "1") == 0) 
                  l2tpNsCfg->enablePAP = TRUE;
               else 
                  l2tpNsCfg->enablePAP = FALSE;   
                  
               if (strcmp(lns.enablemschap, "1") == 0) 
                  l2tpNsCfg->enableMSCHAP = TRUE;
               else 
                  l2tpNsCfg->enableMSCHAP = FALSE;     
                  
               if (strcmp(lns.enablemschapv2, "1") == 0) 
                  l2tpNsCfg->enableMSCHAPv2 = TRUE;
               else 
                  l2tpNsCfg->enableMSCHAPv2 = FALSE;  
                  
               if (strcmp(lns.enablelenbit, "1") == 0) 
                  l2tpNsCfg->enableLengthBit = TRUE;
               else 
                  l2tpNsCfg->enableLengthBit = FALSE;  
            }
            else
            {
               l2tpNsCfg->enable = FALSE;
            }

            ret = cmsObj_set(l2tpNsCfg, &iidStack);
            cmsObj_free((void **) &l2tpNsCfg);
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
      cmsLog_debug("Create new pptpac object");  
       
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
   
      if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_L2TP_NS, &iidStack, 0, (void **) &l2tpNsCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get Dev2PppInterfaceL2tpNsObject, ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE_L2TP_NS, &iidStack);
         cmsObj_free((void **) &pppIntfObj);   
         return ret;
      }

      pppIntfObj->enable = TRUE;

      /* fill in service name */
      CMSMEM_REPLACE_STRING(pppIntfObj->name, "L2tpNs");   /* todo. just assign some thing for now */

      /* Set and Activate PPP interface */
      if ((ret = cmsObj_set(pppIntfObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Set pppIntf failed , ret = %d", ret);
      }
      cmsObj_free((void **) &pppIntfObj);

      if (strcmp(lns.enablelns, "1") == 0)
      {
         l2tpNsCfg->enable = TRUE;
         CMSMEM_REPLACE_STRING(l2tpNsCfg->lnsName, lns.l2tpnsname);
         CMSMEM_REPLACE_STRING(l2tpNsCfg->lnsLocalIp, lns.l2tpnslocalip);
         CMSMEM_REPLACE_STRING(l2tpNsCfg->peerSatrtIp, lns.peerstartip);
         CMSMEM_REPLACE_STRING(l2tpNsCfg->peerEndIp, lns.peerendip);
         CMSMEM_REPLACE_STRING(l2tpNsCfg->username, lns.l2tpnsusername);
         CMSMEM_REPLACE_STRING(l2tpNsCfg->password, lns.l2tpnspassword);
         if (strcmp(lns.enablechap, "1") == 0) 
            l2tpNsCfg->enableCHAP = TRUE;
         else 
            l2tpNsCfg->enableCHAP = FALSE;
                  
         if (strcmp(lns.enablepap, "1") == 0) 
            l2tpNsCfg->enablePAP = TRUE;
         else 
            l2tpNsCfg->enablePAP = FALSE;   
                  
         if (strcmp(lns.enablemschap, "1") == 0) 
            l2tpNsCfg->enableMSCHAP = TRUE;
         else 
            l2tpNsCfg->enableMSCHAP = FALSE;     
                  
         if (strcmp(lns.enablemschapv2, "1") == 0) 
            l2tpNsCfg->enableMSCHAPv2 = TRUE;
         else 
            l2tpNsCfg->enableMSCHAPv2 = FALSE;  
                  
         if (strcmp(lns.enablelenbit, "1") == 0) 
            l2tpNsCfg->enableLengthBit = TRUE;
         else 
            l2tpNsCfg->enableLengthBit = FALSE;  
      }
      else
      {
         l2tpNsCfg->enable = FALSE;
      }

      ret = cmsObj_set(l2tpNsCfg, &iidStack);
      cmsObj_free((void **) &l2tpNsCfg);
   }

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2PppInterfaceL2tpNsObject, ret = %d", ret);
   }
   
   return ret;
}

CmsRet dalGetL2tpNsInfo(char *info)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2PppInterfaceObject *pppIntfObj = NULL;
   Dev2PppInterfaceL2tpNsObject *l2tpNsCfg = NULL;
   UBOOL8 found = FALSE;
   
   if(info == NULL)
   {
      return CMSRET_INVALID_PARAM_TYPE;
   }
   else
   {
      strcpy(info, "0|default||||||1|1|1|1|1"); //set default value to info string
   }
   
   /* Look thru  all l interfaces and check for the lowerlayer matches with
   * the layer 2 interface (vlantermation here).  
   */
   
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&pppIntfObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(pppIntfObj->name, "L2tpNs"))
      {
         InstanceIdStack l2tpNsIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_L2TP_NS, &iidStack, 
                                             &l2tpNsIidStack, (void **) &l2tpNsCfg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get l2tpNsCfg, error=%d", ret);
            cmsObj_free((void **)&pppIntfObj);
            return ret;
         }

         found = TRUE;

         if (l2tpNsCfg)
         {
             sprintf(info, "%d|%s|%s|%s|%s|%s|%s|%d|%d|%d|%d|%d", 
             l2tpNsCfg->enable, l2tpNsCfg->lnsName, 
             l2tpNsCfg->lnsLocalIp, l2tpNsCfg->peerSatrtIp, l2tpNsCfg->peerEndIp,
             l2tpNsCfg->username, l2tpNsCfg->password,
             l2tpNsCfg->enableCHAP, l2tpNsCfg->enablePAP, l2tpNsCfg->enableMSCHAP, 
             l2tpNsCfg->enableMSCHAPv2, l2tpNsCfg->enableLengthBit);
             cmsObj_free((void **) &l2tpNsCfg);
         }
      }
      cmsObj_free((void **)&pppIntfObj);
   }
   
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPNS_2 */
