/***********************************************************************
 *
 *  Copyright (c) 2020-2023  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2023:proprietary:standard

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
#include "cms_util.h"
#include "sysutil_net.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_util.h"
#include "rut2_ethernet.h"
#include "rut2_ethcablediag.h"
#include "rut_ethintf.h"

#include "ethctl_api.h"

const MdmObjectId pairOid[4] = {
   MDMOID_DEV2_CABLE_DIAG_BROWN_PAIR,
   MDMOID_DEV2_CABLE_DIAG_BLUE_PAIR, 
   MDMOID_DEV2_CABLE_DIAG_GREEN_PAIR, 
   MDMOID_DEV2_CABLE_DIAG_ORANGE_PAIR
};


static int is_crossbar(char* ifname)
{
   struct ifreq ifr;
   int skfd, sub_port_map;
   int ret = -1;

   strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
   ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';

   if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      fprintf(stderr, "socket open error\n");
      return ret;
   }

   if (ioctl(skfd, SIOCGIFINDEX, &ifr) < 0 )
   {
      fprintf(stderr, "ioctl failed. check if %s exists\n", ifr.ifr_name);
      close(skfd);
      return ret;
   }

   sub_port_map = 0;
   ifr.ifr_data = (char*)&sub_port_map;
   if (ioctl(skfd, SIOCGQUERYNUMPORTS, &ifr) != 0)
      ret = 0;
   else
      ret = 1;
   close(skfd);
   return ret;
}

CmsRet rutEthCableDiag_Run(char* ifname, int manual)
{
   int err, ret;
   ethcd_t ethcd = {0};

   if (IS_EMPTY_STRING(ifname))
      return CMSRET_INVALID_PARAM_VALUE;

   if (is_crossbar(ifname))
   {
      return CMSRET_INTERNAL_ERROR;
   }

   if (manual == 1)
      ethcd.op = ETHCD_RUN;
   else
      ethcd.op = ETHCD_QUERY;

   err = ethernet_cable_diagnostics(ifname, &ethcd);
   if (err)
   {
      cmsLog_error("command return error!, ret=%d", err);
      return CMSRET_INTERNAL_ERROR;
   }

   if (ethcd.return_value == ETHCD_OK)
   {
      int i;
      char timeBuf[BUFLEN_128] = {0};
      char length[BUFLEN_16] = {0};
      Dev2CableDiagPerInterfaceResultObject *perIntfResultObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if ((ret = cmsObj_addInstance(MDMOID_DEV2_CABLE_DIAG_PER_INTERFACE_RESULT, &iidStack)) != CMSRET_SUCCESS)
         return CMSRET_RESOURCE_EXCEEDED;

      if ((ret = cmsObj_get(MDMOID_DEV2_CABLE_DIAG_PER_INTERFACE_RESULT, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&perIntfResultObj)) != CMSRET_SUCCESS)
         return CMSRET_INTERNAL_ERROR;

      // Interface
      qdmIntf_intfnameToFullPathLocked(ifname, 1, &(perIntfResultObj->interface));

      // LengthMeter
      snprintf(length, sizeof(length), "%d.%d", ethcd.pair_len_cm[0]/100, ethcd.pair_len_cm[0]%100);
      CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->lengthMeter, length, mdmLibCtx.allocFlags);

      // Timestamp
      cmsTms_getXSIDateTime(ethcd.time_stamp, timeBuf, 128);
      CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->timestamp, timeBuf, mdmLibCtx.allocFlags);

      // Link Status
      if (ethcd.link)
      {
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->linkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->linkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      // Cable Status
      switch (ethcd.cable_code)
      {
      case ETHCD_STATUS_GOOD_CONNECTED:
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->status, MDMVS_CONNECTED, mdmLibCtx.allocFlags);
         break;

      case ETHCD_STATUS_GOOD_OPEN:
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->status, MDMVS_OPEN, mdmLibCtx.allocFlags);
         break;


      case ETHCD_STATUS_NO_CABLE:
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->status, MDMVS_NOCABLE, mdmLibCtx.allocFlags);
         break;
      
      case ETHCD_STATUS_BAD_OPEN:
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->status, MDMVS_BADCABLEOPEN, mdmLibCtx.allocFlags);
         break;

      case ETHCD_STATUS_MIXED_BAD:
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->status, MDMVS_BADCABLEMIXED, mdmLibCtx.allocFlags);
         break;

      default:
         CMSMEM_REPLACE_STRING_FLAGS(perIntfResultObj->status, MDMVS_BADCABLEMIXED, mdmLibCtx.allocFlags);
         break;
      }
      
      ret = cmsObj_set(perIntfResultObj, &iidStack);
      cmsObj_free((void **)&perIntfResultObj);

      if (ret != CMSRET_SUCCESS)
         return CMSRET_INTERNAL_ERROR;
    
      for (i = 0 ; i < 4 ; i ++) 
      {
         // All twisted pair object should share the same structure.
         Dev2CableDiagBrownPairObject  *pairObj = NULL;
         if ((ret = cmsObj_get(pairOid[i], &iidStack, OGF_NO_VALUE_UPDATE, (void **)&pairObj)) == CMSRET_SUCCESS)
         {
            // pair length
            snprintf(length, sizeof(length), "%d.%d", ethcd.pair_len_cm[i]/100, ethcd.pair_len_cm[i]%100);
            CMSMEM_REPLACE_STRING_FLAGS(pairObj->lengthMeter, length,  mdmLibCtx.allocFlags);

            // pair status
            switch (ethcd.pair_code[i])
            {
            case ETHCD_PAIR_OK:
               CMSMEM_REPLACE_STRING_FLAGS(pairObj->status, MDMVS_GOOD, mdmLibCtx.allocFlags);
               break;
              
            case ETHCD_PAIR_OPEN:
               CMSMEM_REPLACE_STRING_FLAGS(pairObj->status, MDMVS_OPEN, mdmLibCtx.allocFlags);
               break;

            case ETHCD_PAIR_INTRA_SHORT:
               CMSMEM_REPLACE_STRING_FLAGS(pairObj->status, MDMVS_INTRAPAIRSHORT, mdmLibCtx.allocFlags);
               break;

            case ETHCD_PAIR_INTER_SHORT:
               CMSMEM_REPLACE_STRING_FLAGS(pairObj->status, MDMVS_INTERPAIRSHORT, mdmLibCtx.allocFlags);
               break;

            default:
               CMSMEM_REPLACE_STRING_FLAGS(pairObj->status, MDMVS_INVALID, mdmLibCtx.allocFlags);
               break;
            }

            ret = cmsObj_set(pairObj, &iidStack);
            cmsObj_free((void **)&pairObj);

            if (ret != CMSRET_SUCCESS)
               return CMSRET_INTERNAL_ERROR;
         }
         else
            return CMSRET_INTERNAL_ERROR;
      } /* for loop */
   } /* code == ETHCD_OK */
   else if (ethcd.return_value == ETHCD_NOT_SUPPORTED)
      cmsLog_notice("%s: Cable Diagnostics is Not Supported!", ifname);
   else
      cmsLog_error("%s: Cable Diagnostics is failed! ret:%d", ifname, ethcd.return_value);

   return CMSRET_SUCCESS;
}

CmsRet rutEthCableDiag_SetupAutoEnable(char* ifname, UBOOL8 enable)
{
   int err, ret;
   ethcd_t ethcd = {0};

   if (IS_EMPTY_STRING(ifname))
      return CMSRET_INVALID_PARAM_VALUE;

   if (is_crossbar(ifname))
   {
      return CMSRET_INTERNAL_ERROR;
   }

   ethcd.op = ETHCD_SET;
   ethcd.value = enable;
   
   err = ethernet_cable_diagnostics(ifname, &ethcd);
   if (err)
   {
      cmsLog_error("command return error!, ret=%d", err);
      return CMSRET_INTERNAL_ERROR;
   }
   ret = ethcd.return_value;

   if (ret < 0)
   {
      if (ret == ETHCD_NOT_SUPPORTED)
         cmsLog_notice("%s: Cable Diagnostics is Not Supported!", ifname);
      else
         cmsLog_error("Error! Configure %s Cable Diagnostics auto run failed! (ret:%d)", ifname, ret);
   }

   return CMSRET_SUCCESS;
}
