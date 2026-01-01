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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"

/*!\file rcl2_pptpns.c
 * \brief This file contains PPTPNS related functions.
 *
 */

CmsRet rcl_dev2PppInterfacePptpNsObject( _Dev2PppInterfacePptpNsObject *newObj,
                const _Dev2PppInterfacePptpNsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS; 
   /* The newObj/currObj path is Device.PPP.Interface.{i}.PPTPNS. , i is instance number*/
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }
   
    /* enable PptpNs connection */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Start PPTP Server");
      if ((ret = rutPptpNS_start_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("PPTP Server start failed, ret=%d", ret);
         return ret;
      }     
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Restart PPTP Server"); 
      cmsLog_debug("delete the old PPTP Server and restart the new one");
      rutPptpNS_stop_dev2((Dev2PppInterfacePptpNsObject *) currObj);
      sleep(1);
      if ((ret = rutPptpNS_start_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("PPTP Server restart failed, ret=%d", ret);
         return ret;
      }
           
   }
   /* delete PptpNs connection */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Delete PPTP Server"); 

      if(!newObj && currObj)
      {
        /* This case was callback function from delete ppp instance,  
         * and the ppp instance is not including the configured pptpns node, 
         * it should be triggered by deleting ppp instance with configured l2tpac node,
         * dalL2tpAc_deleteL2tpAcInterface_dev2 will remove whole ppp instance
         */
         if(currObj->username == NULL  &&  currObj->password == NULL )
         {   
	        cmsLog_debug("I am pptpns, not my job");
	        return ret;
         }
      }
      rutPptpNS_stop_dev2((Dev2PppInterfacePptpNsObject *) currObj);
   }
   
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_PPTPNS_2 */
