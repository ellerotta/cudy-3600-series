/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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

#include <string.h>
#include <ctype.h>
#include <sys/shm.h>

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_obj.h"
#include "sysutil.h"
#include "genutil_rbtree.h"
#include "mdm.h"
#include "mdm_private.h"

/*
 * This file contains helper functions for mdm.c
 * mdm.c is shipped to customers as a binary only object, which means
 * it cannot be recompiled.  But the customer may need to customize
 * our reference software, which may cause some constants to get redefined
 * or some structures to change in size.  If mdm.c uses these constants
 * or structures, than the mdm.o binary object will not work correctly
 * because it was compiled with the old values.  Hence, mdm.c will use
 * the functions in this file to get those changed definitions
 * so that mdm.c will work correctly even though it was not recompiled.
 * The functions in this file may look trivially simple, but they are needed
 * because this file can be recompiled to get the changed definitions, but
 * mdm.c cannot be recompiled.
 */


/*
 * Info shared by other layers in the cms_core library, but not by
 * any other applications.
 */
MdmLibraryContext mdmLibCtx={FALSE, EID_INVALID, 0,
                             CMS_INVALID_PID, NULL,
                             {{0,NULL}, {0,NULL}, {0,NULL}, {0,NULL}},
                             UNINITIALIZED_SHM_ID, NULL,
                             ALLOC_ZEROIZE, FALSE, TRUE};



/* An arry of EIDs' that have full write access and object create privileges */
#define MAX_FULL_WRITE_ACCESS_ENTRIES    50

// This is the actual array used at runtime.  Can be modified at runtime.
CmsEntityId fullWriteAccessEidArray[MAX_FULL_WRITE_ACCESS_ENTRIES];

// This is the static, defined-at-compile-time array.
CmsEntityId fullWriteAccessEidArrayStatic[] = {EID_SMD,
                       EID_SSK, EID_DSL_SSK, EID_DIAG_SSK,
                       EID_OSGID, EID_OPENWRTD, EID_DOCKERMD,
                       EID_UDPECHO, EID_OMCID, EID_OMCIPMD,
                       EID_VOICE, EID_DECT, EID_DECTDBGD, EID_BBCD,
                       EID_EPON_APP, EID_FIREWALLD, EID_MODSWD, EID_OPENPLAT_MD,
                       EID_BMUD, EID_1905, EID_ECMS, EID_WLCTDM, EID_WLSSK,
                       EID_WLDIAG, EID_WLDATAELD, EID_WLNVRAM, EID_WLNVRAM_LIB,
                       EID_MDM_CLI, // TODO: why is MDM_CLI given full write access?
                       EID_INVALID};

void mdm_dumpFullWriteAccessEidArray()
{
   UINT32 i;

   printf("MDM FullWriteAccessEidArray (max entries=%d)\n",
          MAX_FULL_WRITE_ACCESS_ENTRIES);
   for (i=0; i < MAX_FULL_WRITE_ACCESS_ENTRIES; i++)
      printf("[%02d] %d\n", i, fullWriteAccessEidArray[i]);

   return;
}

void mdm_initFullWriteAccessEidArray()
{
   UINT32 i, num_of_ArrayStatic;

   // First init the array with invalid EID, meaning the slot is empty.
   for (i=0; i < MAX_FULL_WRITE_ACCESS_ENTRIES; i++)
      fullWriteAccessEidArray[i] = EID_INVALID;
   
   num_of_ArrayStatic = sizeof(fullWriteAccessEidArrayStatic)/sizeof(CmsEntityId);
   
   // copy the defined-at-compile-time array to the runtime array.
   for (i=0; i < num_of_ArrayStatic; i++)
   {
      fullWriteAccessEidArray[i] = fullWriteAccessEidArrayStatic[i];

      if (fullWriteAccessEidArrayStatic[i] == EID_INVALID)
         return;
   }

   cmsLog_error("MDM FullWriteAccessEidArray is full! (%d)", i);
   mdm_dumpFullWriteAccessEidArray();
   return;
}

UBOOL8 mdm_isFullWriteAccessEid(CmsEntityId eid)
{
   UINT32 i=0;
   CmsEntityId gEid = GENERIC_EID(eid);

   // This array may have "holes", so need to check every entry.
   for (i=0; i < MAX_FULL_WRITE_ACCESS_ENTRIES; i++)
   {
      if (gEid == fullWriteAccessEidArray[i])
      {
         return TRUE;
      }
   }

   return FALSE;
}

CmsRet cmsMdm_addEidToFullWriteAccessArray(CmsEntityId eid)
{
   UINT32 i=0;
   CmsEntityId gEid = GENERIC_EID(eid);
   
   if (mdm_isFullWriteAccessEid(gEid))
      return CMSRET_SUCCESS;
   
   // not already in array, add it.
   for (i=0; i < MAX_FULL_WRITE_ACCESS_ENTRIES; i++)
   {
      if (fullWriteAccessEidArray[i] == EID_INVALID)
      {
         fullWriteAccessEidArray[i] = gEid;
         return CMSRET_SUCCESS;
      }
   }

   cmsLog_error("MDM FullWriteAccessEidArray is full! (%d)", i);
   mdm_dumpFullWriteAccessEidArray();
   return CMSRET_RESOURCE_EXCEEDED;
}

void cmsMdm_deleteEidFromFullWriteAccessArray(CmsEntityId eid)
{
   UINT32 i=0;
   CmsEntityId gEid = GENERIC_EID(eid);

   for (i=0; i < MAX_FULL_WRITE_ACCESS_ENTRIES; i++)
   {
      if (fullWriteAccessEidArray[i] == gEid)
      {
         fullWriteAccessEidArray[i] = EID_INVALID;
         break;
      }
   }
   return;
}


MdmPathDescriptor *mdm_allocatePathDescriptor(UINT32 flags)
{
   return ((MdmPathDescriptor *) cmsMem_alloc(sizeof(MdmPathDescriptor), flags));
}

UINT32 mdm_getMaxOid(void)
{
   return MDM_MAX_OID;
}

UINT32 mdm_getMaxParamNameLength(void)
{
   return MAX_MDM_PARAM_NAME_LENGTH;
}

void mdm_initPathDescriptor(MdmPathDescriptor *pathDesc)
{
   INIT_PATH_DESCRIPTOR(pathDesc);
}

void mdm_initParamName(char *paramName)
{
   INIT_MDM_PARAM_NAME(paramName);
}

UINT16 mdm_getDefaultAccessBitMask(void)
{
   return DEFAULT_ACCESS_BIT_MASK;
}

key_t mdm_getZoneLockSemaphoreKey()
{
   return MDM_ZONE_LOCK_SEMAPHORE_KEY;
}

key_t mdm_getMetaLockSemaphoreKey()
{
   return MDM_META_LOCK_SEMAPHORE_KEY;
}


SINT32 cmsMdm_compareIidStacks(const InstanceIdStack *iidStack1,
                               const InstanceIdStack *iidStack2)
{
   UINT32 depth1, depth2, max;

   if (iidStack1 == NULL || iidStack2 == NULL)
   {
      cmsLog_error("iidStack1(%p) or iidStack2(%p) is NULL", iidStack1, iidStack2);
      return -1;
   }

   depth1 = DEPTH_OF_IIDSTACK(iidStack1);
   depth2 = DEPTH_OF_IIDSTACK(iidStack2);
   max = (depth1 > depth2) ? depth1 : depth2;

   return cmsMdm_compareIidStacksToDepth(iidStack1, iidStack2, max);
}



SINT32 cmsMdm_compareIidStacksToDepth(const InstanceIdStack *iidStack1,
                                      const InstanceIdStack *iidStack2,
                                      UINT32 n)
{
   UINT32 i, depth1, depth2, max;

   if (iidStack1 == NULL || iidStack2 == NULL)
   {
      cmsLog_error("iidStack1(%p) or iidStack2(%p) is NULL", iidStack1, iidStack2);
      return -1;
   }

   depth1 = DEPTH_OF_IIDSTACK(iidStack1);
   depth2 = DEPTH_OF_IIDSTACK(iidStack2);
   max = (depth1 > depth2) ? depth1 : depth2;
   n = (n < max) ? n : max;

   for (i=0; i < n; i++)
   {
      if (i >= depth1 && i < depth2)
      {
         return -1;
      }
      else if (i < depth1 && i >= depth2)
      {
         return 1;
      }

      if (iidStack1->instance[i] < iidStack2->instance[i])
      {
         return -1;
      }
      else if (iidStack1->instance[i] > iidStack2->instance[i])
      {
         return 1;
      }
   }

   return 0;
}




char *cmsMdm_dumpIidStack(const InstanceIdStack *iidStack)
{
   static char buf[IIDSTACK_BUF_LEN] = {0};

   return (cmsMdm_dumpIidStackToBuf(iidStack, buf, IIDSTACK_BUF_LEN));
}



char *cmsMdm_dumpIidStackToBuf(const InstanceIdStack *iidStack, char *buf, UINT32 len)
{
   UINT32 consumed;
   UINT32 minLen=4;  /* need at least enough space for {z}\0 or nul\0 */
   UINT8 index=0;
   char *currBuf = buf;
   UBOOL8 truncated=FALSE;

   if (buf == NULL)
   {
      return NULL;
   }

   memset(buf, 0, len);

   if (len < minLen)
   {
      cmsLog_error("buf too small, got %d min %d", len, minLen);
      return buf;
   }

   if (iidStack == NULL)
   {
      snprintf(buf, minLen, "nul");
      return buf;
   }

   len -= 2; /* reserve space for the ending }\0 */

   sprintf(currBuf, "{");
   currBuf++;
   len--;
   
   while ((len > 0) && (index < iidStack->currentDepth))
   {
      if (index < iidStack->currentDepth - 1)
      {
         consumed = snprintf(currBuf, len, "%u,", iidStack->instance[index]);
      }
      else
      {
         consumed = snprintf(currBuf, len, "%u", iidStack->instance[index]);
      }

      if (consumed >= len)
      {
         truncated = TRUE;
         currBuf += len - 1; /* snprintf has put a null termination for us, back up over it */
         break;
      }
      else
      {
         currBuf += consumed;
         len -= consumed;
         index++;
      }
   }

   if (truncated ||
       ((iidStack->currentDepth != 0) && (index < iidStack->currentDepth)))
   {
      sprintf(currBuf, "z}");
   }
   else
   {
      sprintf(currBuf, "}");
   }

   return buf;
}




UBOOL8 mdm_isContainedInSubTree(const MdmObjectNode *objNode1,
                                const InstanceIdStack *iidStack1,
                                const MdmObjectNode *objNode2,
                                const InstanceIdStack *iidStack2)
{
   MdmObjectNode *tmpObjNode = (MdmObjectNode *) objNode2;


   /* basic check */
   if (DEPTH_OF_IIDSTACK(iidStack1) > DEPTH_OF_IIDSTACK(iidStack2))
   {
      return FALSE;
   }

   /*
    * Check if objNode1 is an ancestor of objNode2.
    */
   while ((tmpObjNode != NULL) && (tmpObjNode != objNode1))
   {
      tmpObjNode = tmpObjNode->parent;
   }

   if (tmpObjNode != objNode1)
   {
      /* objNode1 is not an ancestor of objNode2, no way objNode2 is in sub-tree */
      return FALSE;
   }


   if (objNode1 == objNode2)
   {
      if (DEPTH_OF_IIDSTACK(iidStack1) == DEPTH_OF_IIDSTACK(iidStack2))
      {
         /*
          * Here, objNode1 == objNode2 and they have the same number of instances
          * in their iidStack.  If the iidStacks are not equal, they definately
          * are not in the same sub-tree.  But the more interesting question is:
          * if iidStack1 == iidStack2, is iidStack2 considered part of the same
          * sub-tree?  For what I am using it for, in getNextObjectNode, the
          * answer is YES.
          * So in both cases, we can return false.
          */
         if (cmsMdm_compareIidStacks(iidStack1, iidStack2))
         {
            return FALSE;
         }
      }
      else
      {
         /* iidStack1 is shallower than iidStack2 */
         if (cmsMdm_compareIidStacksToDepth(iidStack1, iidStack2,
                                            DEPTH_OF_IIDSTACK(iidStack1)))
         {
            return FALSE;
         }
      }
   }
   else
   {
      /* objNode1 != objNode2, but we know that objNode1 is an ancestor of objNode2 */

      if (DEPTH_OF_IIDSTACK(iidStack1) == DEPTH_OF_IIDSTACK(iidStack2))
      {
         if (cmsMdm_compareIidStacks(iidStack1, iidStack2))
         {
            return FALSE;
         }
      }
      else
      {
         /* iidStack1 is shallower than iidStack2 */
         if (cmsMdm_compareIidStacksToDepth(iidStack1, iidStack2,
                                            DEPTH_OF_IIDSTACK(iidStack1)))
         {
            return FALSE;
         }
      }
   }

   return TRUE;
}


UBOOL8 mdm_isPathDescContainedInSubTree(const MdmPathDescriptor *pathDesc1,
                                        const MdmPathDescriptor *pathDesc2)
{
   MdmObjectNode *objNode1, *objNode2;

   if ((objNode1 = mdm_getObjectNode(pathDesc1->oid)) == NULL)
   {
      cmsLog_error("invalid pathDesc1 oid (%d)", pathDesc1->oid);
      return FALSE;
   }

   if ((objNode2 = mdm_getObjectNode(pathDesc2->oid)) == NULL)
   {
      cmsLog_error("invalid pathDesc2 oid (%d)", pathDesc2->oid);
      return FALSE;
   }


   return (mdm_isContainedInSubTree(objNode1, &(pathDesc1->iidStack),
                                    objNode2, &(pathDesc2->iidStack)));
}



const char *cmsMdm_paramTypeToString(MdmParamTypes paramType)
{
   const char *str;

   switch(paramType)
   {
   case MPT_STRING:
      str = "string";
      break;

   case MPT_INTEGER:
      str = "int";
      break;

   case MPT_UNSIGNED_INTEGER:
      str = "unsignedInt";
      break;

   case MPT_BOOLEAN:
      str = "boolean";
      break;

   case MPT_DATE_TIME:
      str = "dateTime";
      break;

   case MPT_BASE64:
      str = "base64";
      break;

   case MPT_HEX_BINARY:
      str = "hexBinary";
      break;

   case MPT_UNSIGNED_LONG64:
      str = "unsignedLong";
      break;

   case MPT_LONG64:
      str = "long";
      break;

   case MPT_UUID:
      str = "UUID";
      break;

   case MPT_IP_ADDR:
      str = "IPAddress";
      break;

   case MPT_MAC_ADDR:
      str = "MACAddress";
      break;

   case MPT_STATS_COUNTER32:
      str = "StatsCounter32";
      break;

   case MPT_STATS_COUNTER64:
      str = "StatsCounter64";
      break;

   case MPT_DECIMAL:
      str = "decimal";
      break;

   default:
      cmsLog_error("invalid paramType enum %d", paramType);
      str = "invalid";
      break;
   }

   return str;
}

const char *mdm_getParamType(const MdmPathDescriptor *pathDesc)
{
   MdmParamNode *paramNode;
   MdmParamTypes paramType;

   if ((pathDesc == NULL) ||
       ((paramNode = mdm_getParamNode(pathDesc->oid, pathDesc->paramName)) == NULL))
   {
      return "invalid";
   }

   paramType = PARAM_NODE_TYPE(paramNode);
   return (cmsMdm_paramTypeToString(paramType));
}

const char *mdm_getParamBaseType(const MdmPathDescriptor *pathDesc)
{

   MdmParamNode *paramNode;
   MdmParamTypes paramType;

   if ((pathDesc == NULL) ||
       ((paramNode = mdm_getParamNode(pathDesc->oid, pathDesc->paramName)) == NULL))
   {
      return "invalid";
   }

   paramType = PARAM_NODE_TYPE(paramNode) & MPT_BASE_TYPE_FILTER;
   return (cmsMdm_paramTypeToString(paramType));
}

CmsRet mdm_validateParamNodeString(const MdmParamNode *paramNode,
                                   const char *strValue)
{
   CmsRet ret=CMSRET_SUCCESS;

   if (paramNode == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   switch (paramNode->type)
   {
   case MPT_STRING:
   {
      const char **validValues;
      UINT32 maxLen;

      if ((validValues = (const char **)paramNode->vData.min) != NULL)
      {
         /* there is an array of valid values to check against */
         UBOOL8 matched=FALSE;

         while ((!matched) && (*validValues != NULL) && (strValue != NULL))
         {
            if (!strcmp(*validValues, strValue))
            {
               matched = TRUE;
            }
            else
            {
               validValues++;
            }
         }

         ret = matched ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      }
      else if ((maxLen = (uintptr_t)paramNode->vData.max) != 0)
      {
         /* only a length limit was given */
         UINT32 actualLen;
         actualLen = (strValue == NULL) ? 0 : strlen(strValue);

         ret = (actualLen <= maxLen) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      }
      break;
   }
   case MPT_INTEGER:
   {
      SINT32 val;

      if (strValue == NULL)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      ret = cmsUtl_strtol(strValue, NULL, 0, &val);
      
      if ((ret == CMSRET_SUCCESS) &&
          ((paramNode->vData.min != 0) || (paramNode->vData.max != 0)))
      {
         /* we have range information */
         if ((val < (intptr_t) paramNode->vData.min) ||
             (val > (intptr_t) paramNode->vData.max))
         {
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }
      else if (ret != CMSRET_SUCCESS)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
      
      break;
   }
   case MPT_UNSIGNED_INTEGER:
   case MPT_STATS_COUNTER32:
   {
      UINT32 val;
      if (strValue == NULL)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      ret = cmsUtl_strtoul(strValue, NULL, 0, &val);
      if ((ret == CMSRET_SUCCESS) &&
          ((paramNode->vData.min != 0) || (paramNode->vData.max != 0)))
      {
         /* we have range information */
         if ((val < (uintptr_t) paramNode->vData.min) ||
             (val > (uintptr_t) paramNode->vData.max))
         {
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }
      else if (ret != CMSRET_SUCCESS)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
      }

      break;
   }

   case MPT_LONG64:
   {
      SINT64 val;

      if (strValue == NULL)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      ret = cmsUtl_strtol64(strValue, NULL, 0, &val);
      
      /*
       * Can't check min/max values for 64 bit types because
       * the min/max fields are only 32 bits wide.  If I made
       * the min/max fields 64 bits wide, I would waste 8 bytes
       * per parameter.  Since 64 bit types are mostly used for
       * statistics, it does not seem worth the memory.
       * Maybe one day we can go back and add support for
       * min/max checking on 64 bit types.
       */

#ifdef __LP64__
      if ((ret == CMSRET_SUCCESS) &&
          ((paramNode->vData.min != 0) || (paramNode->vData.max != 0)))
      {
         /* we have range information */
         if ((val < (intptr_t) paramNode->vData.min) ||
             (val > (intptr_t) paramNode->vData.max))
         {
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }
      else if (ret != CMSRET_SUCCESS)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
#else
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("64 bit conversion failed, str=%s", strValue);
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
#endif
      
      break;
   }

   case MPT_UNSIGNED_LONG64:
   case MPT_STATS_COUNTER64:
   {
      UINT64 val;
      if (strValue == NULL)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      ret = cmsUtl_strtoul64(strValue, NULL, 0, &val);

      /*
       * Can't check min/max values for 64 bit types because
       * the min/max fields are only 32 bits wide.  If I made
       * the min/max fields 64 bits wide, I would waste 8 bytes
       * per parameter.  Since 64 bit types are mostly used for
       * statistics, it does not seem worth the memory.
       * Maybe one day we can go back and add support for
       * min/max checking on 64 bit types.
       */

#ifdef __LP64__
      if ((ret == CMSRET_SUCCESS) &&
          ((paramNode->vData.min != 0) || (paramNode->vData.max != 0)))
      {
         /* we have range information */
         if ((val < (uintptr_t) paramNode->vData.min) ||
             (val > (uintptr_t) paramNode->vData.max))
         {
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }
      else if (ret != CMSRET_SUCCESS)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
#else
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("64 bit conversion failed, str=%s", strValue);
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
#endif

      break;
   }

   case MPT_BOOLEAN:
   {
      if (strValue == NULL)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      /* just check for valid ways to express a boolean true/false */
      if ((strcmp(strValue, "1")) &&
          (strcmp(strValue, "0")) &&
          (strcasecmp(strValue, "true")) &&
          (strcasecmp(strValue, "false")))
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
      break;
   }
   case MPT_DATE_TIME:
   {
      /* mwang_todo: check date time format later */
      break;
   }
   case MPT_BASE64:
   {
      /*
       * Currently, we only check length.  Note the max length check is done
       * against the length after base64 encoding.  This was the way it was
       * defined in TR-098.  In TR-106, Issue 1, Admendment 2, the max length
       * is specified to be the max length of the data _before_ encoding.
       * The CMS implementation will stay with the TR-098 definition.
       */
      UINT32 maxLen;

      if ((maxLen = (uintptr_t)paramNode->vData.max) != 0)
      {
         /* only a length limit was given */
         UINT32 actualLen;
         actualLen = (strValue == NULL) ? 0 : strlen(strValue);

         ret = (actualLen <= maxLen) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      }

      /* convert base64 to binary to verify the string is legal base64 */
      if ((ret == CMSRET_SUCCESS) && (strValue != NULL))
      {
         UINT8 *binaryBuf=NULL;
         UINT32 binaryBufLen=0;
      
         ret = cmsB64_decode(strValue, &binaryBuf, &binaryBufLen);
         cmsMem_free(binaryBuf);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("invalid base64 string %s (ret=%d)", strValue, ret);
         }
      }

      break;
   }

   case MPT_HEX_BINARY:
   {
      /*
       * The hexBinary data type was first introduced in  TR-106, Issue 1,
       * Admendment 2, which will be approved around Sep 2008.  In that spec,
       * the max length is specified to be the max length of the data _before_
       * encoding. Note that is different from how the base64 max length check
       * is defined.
       */
      UINT32 maxLen;

      if ((maxLen = (uintptr_t)paramNode->vData.max) != 0)
      {
         /* only a length limit was given */
         UINT32 actualLen;
         actualLen = (strValue == NULL) ? 0 : strlen(strValue)/2;

         ret = (actualLen <= maxLen) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      }

      /* convert to binary to make sure the string is actually hex */
      if ((ret == CMSRET_SUCCESS) && (strValue != NULL))
      {
         UINT8 *binaryBuf=NULL;
         UINT32 binaryBufLen=0;
      
         ret = cmsUtl_hexStringToBinaryBuf(strValue, &binaryBuf, &binaryBufLen);
         cmsMem_free(binaryBuf);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("invalid hex string %s (ret=%d)", strValue, ret);
         }
      }

      break;
   }

   case MPT_UUID:
   {
      /*
       * The TR-106 added UUID arround Nov. 2013. In that spec,
       * the max length is fixed to 36.
       */
      ret = cmsUtl_isValidUuid(strValue) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      break;
   }

   case MPT_IP_ADDR:
   {
      /*
       * The TR-106 added IPPrefix and IPAddress arround May 2010. In that spec,
       * the max length is specified to be the max length of IPv6 addrress which
       * is 45.
       */
      UINT32 maxLen;

      if ((maxLen = (uintptr_t)paramNode->vData.max) != 0)
      {
         /* only a length limit was given */
         UINT32 actualLen;
         actualLen = (strValue == NULL) ? 0 : strlen(strValue);

         ret = (actualLen <= maxLen) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      }

      /* IPAddress data type can be empty string */
      if (ret == CMSRET_SUCCESS && !IS_EMPTY_STRING(strValue))
      {
         SINT32 af = AF_INET;
         // IPv6 address must include ':' charactor, and IPv4 must not include it.
         if (strchr(strValue, ':') != NULL)
            af = AF_INET6;

         ret = cmsUtl_isValidIpAddress(af, strValue) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      }
      break;
   }

   case MPT_MAC_ADDR:
   {
      ret = cmsUtl_isValidMacAddress(strValue) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      break;
   }

   case MPT_DECIMAL:
   {
      double min = 0, val = 0, max = 0;
      char *vmin = (char *)paramNode->vData.min, *vmax = (char *)paramNode->vData.max;

      if (strValue == NULL)
      {
         ret = CMSRET_INVALID_PARAM_VALUE;
         break;
      }

      ret = cmsUtl_isValidDecimal(strValue) ? CMSRET_SUCCESS : CMSRET_INVALID_PARAM_VALUE;
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("invalid value %s of paramNode %s", strValue, paramNode->name);
      }
      else if (!IS_EMPTY_STRING(vmin) || !IS_EMPTY_STRING(vmax))
      {
         sscanf(strValue, "%lf", &val);
         if (!IS_EMPTY_STRING(vmin))
         {
            sscanf(vmin, "%lf", &min);
            if (val < min)
            {
               cmsLog_error("invalid decimal %f(%s), smaller than min %f(%s)", val, strValue, min, vmin);
               ret = CMSRET_INVALID_PARAM_VALUE;
            }
         }
         if (ret == CMSRET_SUCCESS && !IS_EMPTY_STRING(vmax))
         {
            sscanf(vmax, "%lf", &max);
            if (val > max)
            {
               cmsLog_error("invalid decimal %f(%s), bigger than max %f(%s)", val, strValue, max, vmax);
               ret = CMSRET_INVALID_PARAM_VALUE;
            }
         }
      }
      break;
   }


   default:
   {
      cmsLog_error("invalid type of paramNode, %s %d",
                   paramNode->name, paramNode->type);
      cmsAst_assert(0);
      ret = CMSRET_INVALID_PARAM_TYPE;
   }

   } /* end of switch */

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("param name=%s, error=%d", paramNode->name, ret);
   }

   return ret;
}


CmsRet mdm_validateString(const MdmPathDescriptor *pathDesc,
                          const char *strValue)
{
   const MdmParamNode *paramNode;

   if ((pathDesc == NULL) || (strValue == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (((paramNode = mdm_getParamNode(pathDesc->oid, pathDesc->paramName)) == NULL))
   {
      return CMSRET_INVALID_PARAM_NAME;
   }

   return (mdm_validateParamNodeString(paramNode, strValue));
}

CmsRet mdm_getParamNodeString(const MdmParamNode *paramNode,
                              const void *mdmObj,
                              UINT32 flags,
                              char **strValue)
{
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 allocFlags=(flags & ~OGF_OMIT_NULL_VALUES);
   UINT32 omitNullValues=(flags & OGF_OMIT_NULL_VALUES);

   switch(paramNode->type)
   {
   case MPT_STRING:
   case MPT_DATE_TIME:
   case MPT_BASE64:
   case MPT_HEX_BINARY:
   case MPT_UUID:
   case MPT_IP_ADDR:
   case MPT_MAC_ADDR:
   case MPT_DECIMAL:
      {
         char *buf;
         char **typedMdmObj = (char **) mdmObj;

         /* get the address of the string buffer */
         buf = typedMdmObj[paramNode->offsetInObject/sizeof(char *)];

         if (buf != NULL)
         {
            /* make a copy of string value*/
            (*strValue) = cmsMem_strdupFlags(buf, allocFlags);
            if ((*strValue) == NULL)
            {
               ret = CMSRET_RESOURCE_EXCEEDED;
            }
         }
         else
         {
            /* current value is NULL */
            if (omitNullValues)
            {
                /* report value as NULL, which allows PHL to omit */
                (*strValue) = NULL;
            }
            else
            {
               /*
                * In most cases, we report NULL as an empty string so that
                * string based protocols such as TR69 work correctly.
                */
               (*strValue) = cmsMem_alloc(1, allocFlags|ALLOC_ZEROIZE);
               if ((*strValue) == NULL)
               {
                  ret = CMSRET_RESOURCE_EXCEEDED;
               }
            }
         }
         break;
      }

   case MPT_INTEGER:
      {
         SINT32 ival;
         SINT32 *typedMdmObj = (SINT32 *) mdmObj;

         /* get the integer value */
         ival = typedMdmObj[paramNode->offsetInObject/sizeof(SINT32)];

         /* convert integer to string */
         (*strValue) = cmsMem_alloc(BUFLEN_16, allocFlags);
         if ((*strValue) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
         else
         {
            snprintf((*strValue), BUFLEN_16, "%d", ival);
         }
         break;
      }

   case MPT_UNSIGNED_INTEGER:
      {
         UINT32 uval;
         UINT32 *typedMdmObj = (UINT32 *) mdmObj;

         /* get the unsigned integer value */
         uval = typedMdmObj[paramNode->offsetInObject/sizeof(UINT32)];

         /* convert integer to string */
         (*strValue) = cmsMem_alloc(BUFLEN_16, allocFlags);
         if ((*strValue) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
         else
         {
            snprintf((*strValue), BUFLEN_16, "%u", uval);
         }
         break;
      }

   case MPT_LONG64:
      {
         /*
          * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
          * so treat it somewhat like a 32 bit type.
          */
         SINT32 *typedMdmObj = (SINT32 *) mdmObj;
         SINT64 *p64;
         SINT64 ival;

         /*
          * p64 points to the location where the 64 bit value starts,
          * which may be on a 4 byte boundary (not 8 byte boundary).
          */
         p64 = (SINT64 *) &(typedMdmObj[paramNode->offsetInObject/sizeof(SINT32)]);

         /* get the integer value */
         ival = *p64;

         /* convert integer to string */
         (*strValue) = cmsMem_alloc(BUFLEN_32, allocFlags);
         if ((*strValue) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
         else
         {
            snprintf((*strValue), BUFLEN_32, "%" PRId64, ival);
         }
         break;
      }

   case MPT_UNSIGNED_LONG64:
      {
         /*
          * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
          * so treat it somewhat like a 32 bit type.
          */
         UINT32 *typedMdmObj = (UINT32 *) mdmObj;
         UINT64 *p64;
         UINT64 uval;

         /*
          * p64 points to the location where the 64 bit value starts,
          * which may be on a 4 byte boundary (not 8 byte boundary).
          */
         p64 = (UINT64 *) &(typedMdmObj[paramNode->offsetInObject/sizeof(UINT32)]);

         /* get the unsigned integer value */
         uval = *p64;

         /* convert integer to string */
         (*strValue) = cmsMem_alloc(BUFLEN_32, allocFlags);
         if ((*strValue) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
         else
         {
            snprintf((*strValue), BUFLEN_32, "%" PRIu64, uval);
         }
         break;
      }

   case MPT_BOOLEAN:
      {
         UBOOL8 bval;
         UBOOL8 *typedMdmObj = (UBOOL8 *) mdmObj;

         (*strValue) = cmsMem_alloc(BUFLEN_4, ALLOC_ZEROIZE);
         if ((*strValue) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
         else
         {
            bval = typedMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)];
            if (bval == TRUE)
            {
               snprintf((*strValue), BUFLEN_4, "1");
            }
            else if (bval == FALSE)
            {
               snprintf((*strValue), BUFLEN_4, "0");
            }
            else
            {
               /* Since we allow any value to get into the MDM on the
                * cmsObj_set path, we need to support it here.  Report
                * values of 2-255 as 1.
                */
               cmsLog_debug("reporting %d in bool param %s as 1",
                            bval, paramNode->name);
               snprintf((*strValue), BUFLEN_4, "1");
            }
         }
         break;
      }

   default:
      {
         cmsLog_error("invalid type of paramNode,%s %d",
                      paramNode->name, paramNode->type);
         cmsAst_assert(0);
         break;
      }
   } /* end of switch */

   return ret;
}


CmsRet mdm_setParamNodeString(const MdmParamNode *paramNode,
                              const char *strValue,
                              UINT32 allocFlags,
                              void *mdmObj)
{
   CmsRet ret=CMSRET_SUCCESS;
   MdmObjectId oid;

   /* make sure the user passed us the right object */
   oid = GET_MDM_OBJECT_ID(mdmObj);
   if (oid != paramNode->parent->oid)
   {
      cmsLog_error("paramNode belongs to oid %d, mdmObj oid %d", paramNode->parent->oid, oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   switch(paramNode->type)
   {
   case MPT_STRING:
   case MPT_DATE_TIME:
   case MPT_BASE64:
   case MPT_HEX_BINARY:
   case MPT_UUID:
   case MPT_IP_ADDR:
   case MPT_MAC_ADDR:
   case MPT_DECIMAL:
      {
         char **typedMdmObj = (char **) mdmObj;
         char *buf=NULL;

         /* make a copy of the string */
         if ((strValue != NULL) &&
             (buf = cmsMem_strdupFlags(strValue, allocFlags)) == NULL)
         {
            cmsLog_error("malloc failed, param=%s value=%s",
                         paramNode->name, strValue);
            ret = CMSRET_RESOURCE_EXCEEDED;
         }
         else
         {
            /* store the address of the string pointer */
            cmsMem_free(typedMdmObj[paramNode->offsetInObject/sizeof(char *)]);
            typedMdmObj[paramNode->offsetInObject/sizeof(char *)] = buf;
         }

         break;
      }

   case MPT_INTEGER:
      {
         SINT32 *typedMdmObj = (SINT32 *) mdmObj;
         SINT32 ival=0;

         /* convert string to integer */
         ret = cmsUtl_strtol(strValue, NULL, 0, &ival);
         if (ret == CMSRET_SUCCESS)
         {
            /* now store the value inside the mdmObject */
            typedMdmObj[paramNode->offsetInObject/sizeof(SINT32)] = ival;
         }

         break;
      }

   case MPT_UNSIGNED_INTEGER:
      {
         UINT32 *typedMdmObj = (UINT32 *) mdmObj;
         UINT32 uval=0;

         /* convert string to integer */
         ret = cmsUtl_strtoul(strValue, NULL, 0, &uval);
         if (ret == CMSRET_SUCCESS)
         {
            /* now store the value inside the mdmObject */
            typedMdmObj[paramNode->offsetInObject/sizeof(UINT32)] = uval;
         }

         break;
      }

   case MPT_LONG64:
      {
         /*
          * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
          * so treat it somewhat like a 32 bit type.
          */
         SINT32 *typedMdmObj = (SINT32 *) mdmObj;  
         SINT64 *p64;
         SINT64 ival=0;

         /* convert string to integer */
         ret = cmsUtl_strtol64(strValue, NULL, 0, &ival);
         if (ret == CMSRET_SUCCESS)
         {
            /*
             * p64 points to the location where the 64 bit value starts,
             * which may be on a 4 byte boundary (not 8 byte boundary).
             */
            p64 = (SINT64 *) &(typedMdmObj[paramNode->offsetInObject/sizeof(SINT32)]);

            /* now store the value inside the mdmObject */
            *p64 = ival;
         }

         break;
      }

   case MPT_UNSIGNED_LONG64:
      {
         /*
          * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
          * so treat it somewhat like a 32 bit type.
          */
         UINT32 *typedMdmObj = (UINT32 *) mdmObj;
         UINT64 *p64;
         UINT64 uval=0;

         /* convert string to integer */
         ret = cmsUtl_strtoul64(strValue, NULL, 0, &uval);
         if (ret == CMSRET_SUCCESS)
         {
            /*
             * p64 points to the location where the 64 bit value starts,
             * which may be on a 4 byte boundary (not 8 byte boundary).
             */
            p64 = (UINT64 *) &(typedMdmObj[paramNode->offsetInObject/sizeof(UINT32)]);

            /* now store the value inside the mdmObject */
            *p64 = uval;
         }

         break;
      }

   case MPT_BOOLEAN:
      {
         UBOOL8 *typedMdmObj = (UBOOL8 *) mdmObj;

         /*
          * Excel may convert our boolean string "TRUE" to 1.
          */
         if (!strncasecmp(strValue, "TRUE", 4) ||
             !strncmp(strValue, "1", 1))
         {
            typedMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)] = TRUE;
         }
         else if (!strncasecmp(strValue, "FALSE", 5) ||
                  !strncmp(strValue, "0", 1))
         {
            typedMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)] = FALSE;
         }
         else
         {
            cmsLog_error("invalid value in bool paramNode %s ->%s<-",
                         paramNode->name, strValue);
            ret = CMSRET_INVALID_ARGUMENTS;
         }

         break;
      }

   default:
      {
         cmsLog_error("invalid type of paramNode, %s %d",
                      paramNode->name, paramNode->type);
         cmsAst_assert(0);
         ret = CMSRET_INTERNAL_ERROR;
         break;
      }

   } /* end of switch on param->type */

   return ret;
}



static const char *cmsMdm_getConstString(rbtree_t *rbt, const char *input)
{
   rbnode_t *node = NULL;

   if (input == NULL)
   {
      cmsLog_error("NULL input!");
      return NULL;
   }

   oal_lock_meta_rbt();

   node = rbtree_search(rbt, input);
   if (node == NULL)
   {
      // allocate a node with enough storage for input string (key) and
      // return pointer to key.  Nice side effect: if caller tries to free the
      // key, the free will probably crash the app because the key is not the
      // beginning of the actual buffer.
      UINT32 len = sizeof(rbnode_t) + strlen(input) + 1;
      node = cmsMem_alloc(len, mdmLibCtx.allocFlags);
      if (node == NULL)
      {
         cmsLog_error("Could not allocate %d bytes", len);
         return NULL;
      }
      node->key = node + 1;
      strcpy((char *)node->key, input);
      rbtree_insert(rbt, node);
   }

   oal_unlock_meta_rbt();
   return ((const char *) node->key);
}

const char *cmsMdm_getConstProfileString(const char *profile)
{
    return cmsMdm_getConstString(&mdmShmCtx->profilesRBT, profile);
}

const char *cmsMdm_getConstTypeString(const char *type)
{
    return cmsMdm_getConstString(&mdmShmCtx->typesRBT, type);
}

CmsRet mdm_sendLowerLayersChangedMsg(MdmObjectId oid __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     const char *newLowerLayersStr __attribute__((unused)),
                     const char *currLowerLayersStr __attribute__((unused)))
{
#ifdef DMP_DEVICE2_BASELINE_1
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(IntfStackLowerLayersChangedMsgBody)] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   IntfStackLowerLayersChangedMsgBody *llChangedMsg = (IntfStackLowerLayersChangedMsgBody *) (msgHdr + 1);
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsRet ret;

   if (mdmShmCtx->intfStackEid == EID_INVALID)
   {
      cmsLog_error("Cannot send out msg, intfStackEid not set");
      return CMSRET_INTERNAL_ERROR;
   }

   /* fill in header */
   msgHdr->src = cmsMsg_getHandleEid(msgHandle);
   msgHdr->dst = mdmShmCtx->intfStackEid;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_INTFSTACK_LOWERLAYERS_CHANGED;
   msgHdr->dataLength = sizeof(IntfStackLowerLayersChangedMsgBody);

   /* fill in msg body */
   llChangedMsg->oid = oid;
   llChangedMsg->iidStack = *iidStack;

   ret = cmsUtl_diffFullPathCSLs(newLowerLayersStr, currLowerLayersStr,
                                 llChangedMsg->deltaLowerLayers,
                                 sizeof(llChangedMsg->deltaLowerLayers));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsUtl_diffFullPathCSLs failed, newStr=%s currStr=%s ret=%d",
                   newLowerLayersStr, currLowerLayersStr, ret);
      return ret;
   }
   else if (llChangedMsg->deltaLowerLayers[0]=='\0')
   {
      cmsLog_debug("empty deltaLowerLayer!!");
      return ret;
   }

   cmsLog_debug("sending INTFSTACK_LOWERLAYERS_CHANGED");
   if ((ret = cmsMsg_send(msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send INTFSTACK_LOWERLAYERS_CHANGED msg, ret=%d",
                   ret);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet mdm_sendObjectDeletedMsg(MdmObjectId oid __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_DEVICE2_BASELINE_1
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(IntfStackObjectDeletedMsgBody)] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   IntfStackObjectDeletedMsgBody *objDeletedMsg = (IntfStackObjectDeletedMsgBody *) (msgHdr + 1);
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsRet ret;

   if (mdmShmCtx->intfStackEid == EID_INVALID)
   {
      cmsLog_error("Cannot send out msg, intfStackEid not set");
      return CMSRET_INTERNAL_ERROR;
   }

   /* fill in header */
   msgHdr->src = cmsMsg_getHandleEid(msgHandle);
   msgHdr->dst = mdmShmCtx->intfStackEid;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_INTFSTACK_OBJECT_DELETED;
   msgHdr->dataLength = sizeof(IntfStackObjectDeletedMsgBody);

   /* fill in msg body */
   objDeletedMsg->oid = oid;
   objDeletedMsg->iidStack = *iidStack;

   cmsLog_debug("sending INTFSTACK_OBJECT_DELETED");
   if ((ret = cmsMsg_send(msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send INTFSTACK_OBJ_DELETED msg, ret=%d",
                   ret);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet mdm_sendAliasChangedMsg(MdmObjectId oid __attribute__((unused)),
              const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_DEVICE2_BASELINE_1
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(IntfStackAliasChangedMsgBody)] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   IntfStackAliasChangedMsgBody *aliasChangedMsg = (IntfStackAliasChangedMsgBody *) (msgHdr + 1);
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsRet ret;

   if (mdmShmCtx->intfStackEid == EID_INVALID)
   {
      cmsLog_error("Cannot send out msg, intfStackEid not set");
      return CMSRET_INTERNAL_ERROR;
   }

   /* fill in header */
   msgHdr->src = cmsMsg_getHandleEid(msgHandle);
   msgHdr->dst = mdmShmCtx->intfStackEid;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_INTFSTACK_ALIAS_CHANGED;
   msgHdr->dataLength = sizeof(IntfStackAliasChangedMsgBody);

   /* fill in msg body */
   aliasChangedMsg->oid = oid;
   aliasChangedMsg->iidStack = *iidStack;

   cmsLog_debug("sending INTFSTACK_ALIAS_CHANGED");
   if ((ret = cmsMsg_send(msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send INTFSTACK_ALIAS_CHANGED msg, ret=%d",
                   ret);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet cmsMdm_getInfo(MdmInfo *mInfo)
{
   if (mInfo == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   mInfo->shmBegin = mdmShmCtx;
   mInfo->mallocBegin = mdmShmCtx->mallocStart;
   mInfo->shmEnd = mdmShmCtx->shmEnd;
   mInfo->shmId = mdmLibCtx.shmId;

   {
      struct shmid_ds shmbuf;
      memset(&shmbuf, 0, sizeof(shmbuf));
      if (shmctl(mdmLibCtx.shmId, IPC_STAT, &shmbuf) < 0)
      {
         cmsLog_error("shmctl IPC_STAT failed, shmId=%d", mdmLibCtx.shmId);
         return CMSRET_INTERNAL_ERROR;
      }
      mInfo->numAttached = shmbuf.shm_nattch;
   }

   return CMSRET_SUCCESS;
}


// The next set of functions help the MDM (obj, phl, odl) determine whether
// an operation (read, write, create, or delete) is local, remote, or both.
// If an operation is determined to be remote (or possibly remote), it is
// sent to remote_objd, which will use the namespace database to determine
// which component(s) will get the operation.

// This is for detecting when a cmsObj_getNext could traverse over
// multiple components.  Note this is NOT the same as the OBN_MULTI_COMP_OBJ
// flag (multiCompObj=true in data model).
// Currently, this can only happen on Device.QoS.Queue.{i}. QueueStats.{i}.
// and Shaper.{i}.
// If an app calls cmsObj_getNext on MDMOID_DEV2_INTERFACESTACK, it will only
// get the LOCAL intfStack entries (will not get remote entries).
UBOOL8 mdm_isMultiCompGetNextOid(MdmObjectId oid)
{
   if ((oid == MDMOID_DEV2_QOS_QUEUE) ||
       (oid == MDMOID_DEV2_QOS_QUEUE_STATS) ||
       (oid == MDMOID_DEV2_QOS_SHAPER))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

// Given an alias, return the name of the component that the alias points to,
// or NULL if no component name was found in the alias.
const char *mdm_aliasToComponentName(const char *alias)
{
   char aliasBuf[MDM_ALIAS_BUFLEN]={0};
   const char *comp = NULL;
   UINT32 i=0;

   if (IS_EMPTY_STRING(alias))
   {
      return NULL;
   }

   // convert user provided alias to lowercase since our BDK comp names are
   // all lowercase.  We don't seem to have strcasestr in our system.
   while (i < sizeof(aliasBuf) && alias[i] != '\0')
   {
      aliasBuf[i] = (char) tolower(alias[i]);
      i++;
   }

   // If we detect the component name in the alias, return that comp name.
   // Currently, only QoS in Sysmgmt, DSL and Wifi can have this situation.
   if (strstr(aliasBuf, BDK_COMP_SYSMGMT))
   {
      comp = BDK_COMP_SYSMGMT;
   }
   else if (strstr(aliasBuf, BDK_COMP_DSL))
   {
      comp = BDK_COMP_DSL;
   }
   else if (strstr(aliasBuf, BDK_COMP_WIFI))
   {
      comp = BDK_COMP_WIFI;
   }

   if (comp != NULL)
   {
      cmsLog_debug("aliasBuf=%s is in %s", aliasBuf, comp);
   }
   return comp;
}

UBOOL8 mdm_isAliasHintPresent(const FullpathAliasStack *aliasStack)
{
   UINT8 i;
   const char *compName;

   if (aliasStack == NULL)
      return FALSE;

   for (i=0; i < aliasStack->currentDepth; i++)
   {
      // the alias stack contains an alias that has a component name, so
      // yes, alias hint is present.
      compName = mdm_aliasToComponentName(aliasStack->aliasArray[i]);
      if (compName != NULL)
      {
         return TRUE;
      }
   }

   // No hint detected.
   return FALSE;
}

// The next 2 functions should only be called if mdm_isAliasHintPresent() is TRUE.
// So if an alias hint is present, then these function can conclusively say
// whether the alias hint is for the local or remote component.
UBOOL8 mdm_isLocalByAliasStack(const FullpathAliasStack *aliasStack)
{
   UINT8 i;
   const char *compName;

   if (aliasStack == NULL)
      return FALSE;

   if (aliasStack->currentDepth == 0)
   {
      cmsLog_error("aliasStack is empty, should not call this function!");
      return TRUE;  // still return TRUE to keep this local.
   }

   // search for aliases from the end to beginning.  In practice, there is
   // only 1 alias at the end.
   i = aliasStack->currentDepth-1;
   while (TRUE)
   {
      compName = mdm_aliasToComponentName(aliasStack->aliasArray[i]);
      if (compName != NULL)
      {
         // alias hint (component name) detected.
         if (!cmsUtl_strcmp(compName, mdmShmCtx->compName))
         {
            // it is our compName, so must be LOCAL.
            cmsLog_debug("aliasHint detected at %d: %s (LOCAL)", i, compName);
            return TRUE;
         }
         else
         {
            cmsLog_debug("aliasHint detected at %d: %s (REMOTE)", i, compName);
            return FALSE;
         }
      }

      if (i > 0)
         i--;
      else
         break;
   }

   cmsLog_error("No alias hint found, should not call this function!");
   return TRUE;  // still return TRUE to keep this local.
}

UBOOL8 mdm_isRemoteByAliasStack(const FullpathAliasStack *aliasStack)
{
   // See comments above isLocalByAliasStack.
   return (mdm_isLocalByAliasStack(aliasStack) ? FALSE : TRUE);
}



// XXX The instanceIds here are basically hardcoded.  If a component registers another
// set of instance id's at runtime (we don't support this, but could happen in
// the future), this function will not know.
// We only know for sure what the BDK assigned instance id range is.  For all others,
// we should just send to remote_objd, which has the dynamic info from
// sys_directory, to figure it out.
// Given an instance id, return the name of the component that the instance id
// belongs to (see COMP_xxx_FIRST_INSTANCE_ID in cms_mdm.h)
// Return NULL if no component name can be detected from the instance id.
// It only makes sense to call this function on multi-component objects.  Caller
// is responsible for checking that.  (All non-multi-component objects can use
// the entire instance id range.)
const char *mdm_instanceIdToComponentName(UINT32 instanceId)
{
   const char *comp = NULL;

   if (instanceId == 0)
   {
      cmsLog_error("invalid instanceId 0");
      return NULL;
   }

   if (instanceId >= COMP_SYSMGMT_FIRST_INSTANCE_ID && instanceId <= COMP_SYSMGMT_LAST_INSTANCE_ID)
   {
      comp = BDK_COMP_SYSMGMT;
   }
   else if (instanceId >= COMP_DSL_FIRST_INSTANCE_ID && instanceId <= COMP_DSL_LAST_INSTANCE_ID)
   {
      comp = BDK_COMP_DSL;
   }
   else if (instanceId >= COMP_WIFI_FIRST_INSTANCE_ID && instanceId <= COMP_WIFI_LAST_INSTANCE_ID)
   {
      comp = BDK_COMP_WIFI;
   }

   if (comp != NULL)
   {
      cmsLog_debug("instanceId=%u is in %s", instanceId, comp);
   }
   else
   {
      cmsLog_notice("unrecognized instanceId=%u, return NULL", instanceId);
   }
   return comp;
}

// (For now,) the instanceId for determining local or remote is always at depth of 0, see comments below.
UINT32 mdm_instanceIdForLocalOrRemote(const InstanceIdStack *iidStack)
{
   if (iidStack->currentDepth == 0)
   {
      cmsLog_error("empty iidStack! return default instanceId of 1");
      return 1;
   }
   return (iidStack->instance[0]);
}

// XXX checking for instanceIds is only valid on multi-comp and
// type 2 objects (instanceDepth > 0).  In practice, we only use the instanceId
// to determine remote or local for the QoS.Queue, Qos.QueueStats. QoS.Shaper.
// and InterfaceStack objects.
UBOOL8 mdm_isLocalByInstanceId(const InstanceIdStack *iidStack)
{
   UINT32 instanceId = mdm_instanceIdForLocalOrRemote(iidStack);
   const char *compName = mdm_instanceIdToComponentName(instanceId);
   if ((compName != NULL) && (0 == cmsUtl_strcmp(compName, mdmShmCtx->compName)))
   {
      // Prefer to check for LOCAL.  We know for sure what is LOCAL.
      // We got a compName from the instanceId, and it is our compName,
      // so definitely LOCAL (not remote)
      return TRUE;
   }
   else
   {
      // Could not get a compName from instanceId, or compName is not us,
      // so probably remote.
      return FALSE;
   }
}


// See processIsLocalFullpathCmd() in mdm_cmddebug for some interesting
// fullpaths to test against this function.
UBOOL8 mdm_isLocalFullpath(const char *fullpath)
{
   MdmPathDescriptorEx *pathDescEx = NULL;
   UBOOL8 isLocal = FALSE;
   CmsRet ret;

   if (fullpath == NULL)
   {
      cmsLog_error("NULL input fullpath");
      return FALSE;
   }
   else if (fullpath[0] == '\0')
   {
      cmsLog_error("fullpath is empty string!");
      return FALSE;
   }

   if (!IS_REMOTE_OBJ_CAPABLE)
      return TRUE;

   ret = cmsMdm_fullPathToPathDescriptorEx(fullpath, &pathDescEx);
   if (ret != CMSRET_SUCCESS)
   {
      // very rare, internal memory alloc failure.  Not clear what to return.
      // I guess keep it local to avoid sending garbage to remote_objd.
      // Note that a SUCCESS return does not mean the fullpath was
      // successfully parsed/resolved.
      return TRUE;
   }

   if (pathDescEx->objInLocalMdm == 0)
   {
      // Not in local MDM, definitely NOT local.
      cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
      return FALSE;
   }

   /*
    * Everything below this point is for an object found in the local MDM.
    */
   if (pathDescEx->multiCompObj == 0)
   {
       // Found in local MDM and not a multi-comp obj, definitely LOCAL.
       cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
       return TRUE;
   }

   /*
    * Everything below here is an object found in local MDM that has the
    * the OBN_MULTI_COMP_OBJ flag.  It could still be LOCAL only,
    * REMOTE only, or both (a walk).  There is only a small list of objects
    * that can get here (same as in sysdir_ns_isMultiComponentPath):
    * "Device."           <<< this object is at instance depth of 0
      "Device.Services."  <<< this object is at instance depth of 0,
      "Device.X_BROADCOM_COM_AppCfg. <<< this object is at instance depth of 0
      "Device.IP."        <<< this object is at instance depth of 0
      "Device.InterfaceStack.{i}.
      "Device.QoS."       <<< this object is at instance depth of 0
      "Device.QoS.Queue.{i}.
      "Device.QoS.QueueStats.{i}.
      "Device.QoS.Shaper.{i}.
    * Use alias hints or instance numbers to decide.
    */

   if (mdm_isAliasHintPresent(&(pathDescEx->aliasStack)))
   {
      isLocal = mdm_isLocalByAliasStack(&(pathDescEx->aliasStack));
      cmsLog_debug("%s has alias hint, isLocal=%d", fullpath, isLocal);
      cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
      return isLocal;
   }

   if (pathDescEx->instanceDepth == 0)
   {
      // case 1: object is not a table, so this is a cmsObj_get on the object
      // itself, or a walk.
      isLocal = TRUE;
   }
   else
   {
      // object is a table
      if (((DEPTH_OF_IIDSTACK(&(pathDescEx->iidStack))) == 0) ||
          (PEEK_INSTANCE_ID(&(pathDescEx->iidStack)) == 0))
      {
         // case 2: caller did not provide an instance id, so this is a walk,
         // cmsObj_get on the object itself, or new table row creation.
         isLocal = TRUE;
      }
      else
      {
         // case 3: an instanceId was provided, use range to determine.
         isLocal = mdm_isLocalByInstanceId(&(pathDescEx->iidStack));
      }
   }

   cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
   return isLocal;
}

// Cannot just call mdm_isLocalFullpath and return the opposite because
// an object/fullpath may be both local and remote, so just because something
// is local, that does not mean it is not also remote.
UBOOL8 mdm_isPossibleRemoteFullpath(const char *fullpath)
{
   MdmPathDescriptorEx *pathDescEx = NULL;
   UBOOL8 isRemote = FALSE;
   CmsRet ret;

   if (fullpath == NULL)
   {
      cmsLog_error("NULL input fullpath");
      return FALSE;
   }
   else if (fullpath[0] == '\0')
   {
      cmsLog_error("fullpath is empty string!");
      return FALSE;
   }

   if (!IS_REMOTE_OBJ_CAPABLE)
      return FALSE;


   ret = cmsMdm_fullPathToPathDescriptorEx(fullpath, &pathDescEx);
   if (ret != CMSRET_SUCCESS)
   {
      // very rare, internal memory alloc failure.  Not clear what to return.
      // I guess keep it local to avoid sending garbage to remote_objd.
      // Note that a SUCCESS return does not mean the fullpath was
      // successfully parsed/resolved.
      return FALSE;
   }

   // If the obj is not in the local MDM, it must be remote.
   if (pathDescEx->objInLocalMdm == 0)
   {
      cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
      return TRUE;
   }

   /*
    * Everything below this point is for an object found in the local MDM.
    */
   if (pathDescEx->multiCompObj == 0)
   {
      // We found this objNode in the local MDM, and it is not MULTI_COMP, so
      // not remote.
      cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
      return FALSE;
   }

   if (pathDescEx->objInRemoteMdm == 0)
   {
      // OID not found in remote MDM (but was found in local MDM and MULTI_COMP),
      // so probably a third party object in a remote component.
      cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
      return TRUE;
   }

   /*
    * Everything below here is an object found in both local and remote MDM's
    * that has the OBN_MULTI_COMP_OBJ flag.  It could still be LOCAL only,
    * REMOTE only, or both (a walk).  There is only a small list of objects
    * that can get here (same as in sysdir_ns_isMultiComponentPath):
    * "Device."           <<< this object is at instance depth of 0
      "Device.Services."  <<< this object is at instance depth of 0,
      "Device.X_BROADCOM_COM_AppCfg. <<< this object is at instance depth of 0
      "Device.IP."        <<< this object is at instance depth of 0
      "Device.InterfaceStack.{i}.
      "Device.QoS."       <<< this object is at instance depth of 0
      "Device.QoS.Queue.{i}.
      "Device.QoS.QueueStats.{i}.
      "Device.QoS.Shaper.{i}.
    * Use alias hints or instance numbers to decide.
    */

   if (mdm_isAliasHintPresent(&(pathDescEx->aliasStack)))
   {
      UBOOL8 isRemote = mdm_isRemoteByAliasStack(&(pathDescEx->aliasStack));
      cmsLog_notice("%s has alias hint, isRemote=%d", fullpath, isRemote);
      cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
      return isRemote;
   }

   // check instance numbers
   if (pathDescEx->instanceDepth == 0)
   {
      // case 1: object is not a table, so this is a cmsObj_get on the object
      // itself, or a walk.
      isRemote = TRUE;
   }
   else
   {
      // object is a table
      if (((DEPTH_OF_IIDSTACK(&(pathDescEx->iidStack))) == 0) ||
          (PEEK_INSTANCE_ID(&(pathDescEx->iidStack)) == 0))
      {
         // case 2: caller did not provide an instance id, so it is a walk,
         // cmsObj_get on the object itself, or new table row creation.
         // (But if this is a new table row creation, how do we know which
         // component to send it to?  Fortunately, both cmsObj_addInstance
         // and cmsPhl_addObjectInstance will check for isLocalFullpath first.
         // isLocalFullpath will return TRUE in this scenario, so the new
         // row will be created locally.)
         isRemote = TRUE;
      }
      else
      {
         // case 3: an instanceId was provided, use range to determine.
         isRemote = mdm_isLocalByInstanceId(&(pathDescEx->iidStack)) ? FALSE : TRUE;
      }
   }

   cmsMdm_freeMdmPathDescriptorEx(&pathDescEx);
   return isRemote;
}


UBOOL8 mdm_allowSequenceNumError(void)
{
    return FALSE;
}

CmsRet mdm_validateSequenceNum(MdmObjectId oid, const InstanceIdStack *iidStack,
                            UINT16 currSeqNum, UINT16 newSeqNum,
                            SINT32 lastWritePid, const CmsTimestamp *lastWriteTs)
{
    CmsTimestamp nowTs;
    UINT32 deltaMs;

    if (newSeqNum == currSeqNum)
    {
        return CMSRET_SUCCESS;
    }
    /*
     * Allow the same thread to write any sequence number.
     * Needed by OMCI (and maybe other code) which has complicated read/write
     * sequences.  See Jira 33919.
     */
    if (lastWritePid == sysUtl_gettid())
    {
        return CMSRET_SUCCESS;
    }
    /* MDM startup, allow first write to go through */
    if (lastWritePid == CMS_INVALID_PID)
    {
        return CMSRET_SUCCESS;
    }

    cmsTms_get(&nowTs);
    deltaMs = cmsTms_deltaInMilliSeconds(&nowTs, lastWriteTs);
    cmsLog_error("MDM object sequence num error by tid %d on %s (oid %d) "
                 "iidStack %s seq %d->%d (writer tid=%d %dms ago at %d.%03d)",
                 sysUtl_gettid(),
                 mdm_oidToGenericPath(oid), oid, cmsMdm_dumpIidStack(iidStack),
                 newSeqNum, currSeqNum,
                 lastWritePid, deltaMs,
                 lastWriteTs->sec % 1000, lastWriteTs->nsec / NSECS_IN_MSEC);

    /* if we allow sequenceNumError, then we return TRUE here even though
     * the sequence number check failed. */
    if (mdm_allowSequenceNumError())
    {
        return CMSRET_SUCCESS;
    }

    return CMSRET_OBJECT_SEQUENCE_ERROR;
}
