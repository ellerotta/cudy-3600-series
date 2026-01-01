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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"

/*!\file rcl2_l2tpns.c
 * \brief This file contains l2TPNS related functions.
 *
 */

CmsRet rcl_dev2PppInterfaceL2tpNsObject( _Dev2PppInterfaceL2tpNsObject *newObj,
                const _Dev2PppInterfaceL2tpNsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS; 
   /* The newObj/currObj path is Device.PPP.Interface.{i}.L2TPNS. , i is instance number*/
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }
   
    /* enable L2tpNs connection */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Start L2TP Server");
      if ((ret = rutL2tpNS_start_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("L2TP Server start failed, ret=%d", ret);
         return ret;
      }     
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Restart L2TP Server"); 
      cmsLog_debug("delete the old L2TP Server and restart the new one");
      rutL2tpNS_stop_dev2((Dev2PppInterfaceL2tpNsObject *) currObj);
      sleep(1);
      if ((ret = rutL2tpNS_start_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("L2TP Server restart failed, ret=%d", ret);
         return ret;
      }
           
   }
   /* delete L2tpNs connection */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Delete L2TP Server"); 

      if(!newObj && currObj)
      {
        /* This case was callback function from delete ppp instance,  
         * and the ppp instance is not including the configured l2tpns node, 
         * it should be triggered by deleting ppp instance with configured l2tpac node,
         * dalL2tpAc_deleteL2tpAcInterface_dev2 will remove whole ppp instance
         */
         if(currObj->username == NULL  &&  currObj->password == NULL )
         {   
	        cmsLog_debug("I am l2tpns, not my job");
	        return ret;
         }
      }
      rutL2tpNS_stop_dev2((Dev2PppInterfacePptpObject *) currObj);
   }
   
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPNS_2 */
