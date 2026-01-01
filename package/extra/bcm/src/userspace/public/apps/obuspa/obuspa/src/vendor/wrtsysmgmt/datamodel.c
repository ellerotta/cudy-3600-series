/***********************************************************************
 *
 *
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom
   All Rights Reserved

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

#ifdef CONFIG_BRCM_OPENWRT

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "vendor_defs.h"
#include "usp_err_codes.h"
#include "usp_api.h"
#include "bdk_usp.h"
#include "bcm_ulog.h"


#define WRTSYSMGMT_DM_DIR "/var/run/obuspa"
#define WRTSYSMGMT_DM_FILE WRTSYSMGMT_DM_DIR"/wrt_dm"
#define WRTSYSMGMT_DM_FILE_PNUM 5

static int isTopLevelInstance(char *path)
{
   char *p1, *p2;
   
   p1 = strstr(path, "{i}");
   
   if (p1 == NULL)
   {
      return 0;
   }

   p2 = strstr(p1+3, "{i}");

   return ((p2 == NULL));
}


int is_wrtsysmgmt()
{
   return (access(WRTSYSMGMT_DM_FILE, R_OK) == 0);
}


int wrtsysmgmt_registerdm()
{
   char buf[256], *param[WRTSYSMGMT_DM_FILE_PNUM], *p;
   char *unique_keys[MAX_COMPOUND_KEY_PARAMS];
   FILE *fp;
   int i, status, err = USP_ERR_OK;
   unsigned type_flags = 0;
    
   bcmuLog_debug("enter");
    
   if ((fp = fopen(WRTSYSMGMT_DM_FILE, "r")) == NULL)
   {
      bcmuLog_error("fopen %s failed", WRTSYSMGMT_DM_FILE);
      return USP_ERR_INTERNAL_ERROR;
   }

   if (fgets(buf, sizeof(buf), fp) != NULL)
   {
      status = atoi(buf);
       
      bcmuLog_debug("status:%d", status);
       
      if (status)
      {
         bcmuLog_error("%s is error status %d, return", WRTSYSMGMT_DM_FILE, status);
         fclose(fp);
         return USP_ERR_INTERNAL_ERROR;
      }
   }
    
   /* 0:parameter|1:writable|2:type|3:cmd_type|4:unique_keys */
   while (fgets(buf, sizeof(buf), fp) != NULL)
   {
      memset(param, 0, sizeof(param));
      param[0] = buf;
      for (i = 1, p = strchr(buf, '|'); i < WRTSYSMGMT_DM_FILE_PNUM && p; i++, p = strchr(p+1, '|'))
      {
         *p = '\0';
         param[i] = p+1;
      }

      if (strstr(param[2], "xsd:object"))
      {
         err |= USP_REGISTER_GroupedObject(GROUP_ID_USP, param[0], atoi(param[1]));
         if (isTopLevelInstance(param[0]) == 1)
         {
            err |= USP_REGISTER_Object_RefreshInstances(param[0],vendorRefreshInstancesCb);
         }
      }
      else
      {
         type_flags = 0;
         
         if (strstr(param[2], "xsd:string"))
         {
            type_flags = DM_STRING;
         }
         else if (strstr(param[2], "xsd:dateTime"))
         {
            type_flags = DM_DATETIME;
         }
         else if (strstr(param[2], "xsd:boolean"))
         {
            type_flags = DM_BOOL;
         }
         else if (strstr(param[2], "xsd:int"))
         {
            type_flags = DM_INT;
         }
         else if (strstr(param[2], "xsd:unsignedInt"))
         {
            type_flags = DM_UINT;
         }
         else if (strstr(param[2], "xsd:unsignedLong"))
         {
            type_flags = DM_ULONG;
         }
         else if (strstr(param[2], "xsd:base64"))
         {
            type_flags = DM_BASE64;
         }
         else if (strstr(param[2], "xsd:hexBinary"))
         {
            type_flags = DM_HEXBIN;
         }
         else if (strstr(param[2], "xsd:long"))
         {
            type_flags = DM_LONG;
         }
         /* DM_DECIMAL is not support in libbfdm */
         
         if (type_flags)
         {
            if (atoi(param[1]))
            {
               err |= USP_REGISTER_GroupedVendorParam_ReadWrite(GROUP_ID_USP, param[0], type_flags);
            }
            else
            {
               err |= USP_REGISTER_GroupedVendorParam_ReadOnly(GROUP_ID_USP, param[0], type_flags);
            }
         }
         else if (!(strstr(param[2], "xsd:command")))
         {
            bcmuLog_error("skip unrecog parameter %s", buf);
         }
      }
   }

   /*second round parse to register unique key*/
   rewind(fp);
   while (fgets(buf, sizeof(buf), fp) != NULL)
   {
      memset(param, 0, sizeof(param));
      param[0] = buf;
      for (i = 1, p = strchr(buf, '|'); i < WRTSYSMGMT_DM_FILE_PNUM && p; i++, p = strchr(p+1, '|'))
      {
         *p = '\0';
         param[i] = p+1;
      }

      if (param[4] && (i = strlen(param[4])) > 1) 
      {
         param[4][i - 1] = '\0';
         memset(unique_keys, 0, sizeof(unique_keys));
         for (i = 0, p = strtok(param[4], ","); i < MAX_COMPOUND_KEY_PARAMS && p; i++, p = strtok(NULL, ","))
         {
            unique_keys[i] = p;
         }
         err |= USP_REGISTER_Object_UniqueKey(param[0], unique_keys, i);
       }
   }
   fclose(fp);

   /*delete wrt_dm file*/
   if (!err)
   {
      unlink(WRTSYSMGMT_DM_FILE);
   }
   return err;
}

#endif
