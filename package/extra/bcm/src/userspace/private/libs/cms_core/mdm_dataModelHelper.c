/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
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

#include "cms.h"
#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"

/*
 * This file contains functions related to multiple data model support.
 */

extern void mdm_getPtrs_oidInfoArray_igd(const MdmOidInfoEntry **begin, const MdmOidInfoEntry **end);
extern void mdm_getPtrs_oidInfoArray_dev2(const MdmOidInfoEntry **begin, const MdmOidInfoEntry **end);

void mdm_getOidInfoPtrs(const MdmOidInfoEntry **begin, const MdmOidInfoEntry **end)
{
#if defined(SUPPORT_DM_LEGACY98)
   return (mdm_getPtrs_oidInfoArray_igd(begin, end));
#elif defined(SUPPORT_DM_HYBRID)
   return (mdm_getPtrs_oidInfoArray_igd(begin, end));
#elif defined(SUPPORT_DM_PURE181)
   return (mdm_getPtrs_oidInfoArray_dev2(begin, end));
#elif defined(SUPPORT_DM_DETECT)
   if (cmsMdm_isDataModelDevice2())
      return (mdm_getPtrs_oidInfoArray_dev2(begin, end));
   else
      return (mdm_getPtrs_oidInfoArray_igd(begin, end));
#endif
}

const MdmOidInfoEntry *cmsMdm_getAllOidInfoEntries(UINT32 *numEntries)
{
   const MdmOidInfoEntry *begin=NULL;
   const MdmOidInfoEntry *end=NULL;
   
   mdm_getOidInfoPtrs(&begin, &end);
   // end points to the beginning of the last entry.  Need to increment to
   // make the pointer arithmetic work.
   end++;
   *numEntries = end - begin;
   return begin;
}

const MdmOidInfoEntry *cmsMdm_getOidInfoEntry(UINT16 oid)
{
   return mdm_getOidInfo(oid);
}

/** Initialize data model selection stuff.
 *
 * @return -1 on error, 1 if we should load Device2 (Pure181) data model,
 *         0 otherwise (meaning IGD data model, which includes legacy98 and Hybrid)
 */
SINT32 mdm_initDataModelSelection()
{
   /*
    * Legacy98, Hybrid, and Pure181 are easy.  No need to create or look at
    * a PSP entry.  The answer is known at compile time.
    */
#if defined(SUPPORT_DM_LEGACY98)
   return 0;
#elif defined(SUPPORT_DM_HYBRID)
   return 0;
#elif defined(SUPPORT_DM_PURE181)
   return 1;
#elif defined(SUPPORT_DM_DETECT)
   SINT32 rv;
   UINT8 dmc[CMS_DATA_MODEL_PSP_VALUE_LEN]={0};

   rv = cmsPsp_get(CMS_DATA_MODEL_PSP_KEY, dmc, sizeof(dmc));
   if (rv != CMS_DATA_MODEL_PSP_VALUE_LEN)
   {
      /* No PSP file, create one with default entry of 1 (meaning Pure181) */
      /* Write PSP entry of 4 bytes.  Only use the first byte right now,
       * the other 3 are reserved */
      dmc[0] = 1;
      cmsLog_debug("create new CMS Data Model PSP key with value of %d", dmc[0]);
      if (cmsPsp_set(CMS_DATA_MODEL_PSP_KEY, dmc, sizeof(dmc)) != CMSRET_SUCCESS)
      {
         cmsLog_error("initial set of CMS Data Model PSP key failed");
         /* not a fatal error, just return 1 so we can keep running */
         return 1;
      }

      return 1;
   }
   else
   {
      cmsLog_debug("found existing CMS Data Model PSP key=%d", dmc[0]);
      if (dmc[0] == 0)
      {
         return 0;
      }
      else if (dmc[0] == 1)
      {
         return 1;
      }
      else
      {
         cmsLog_error("Unexpected value in CMS Data Model PSP key=%d", dmc[0]);
         /* not a fatal error, just return 1 so we can keep running */
         return 1;
      }
   }
#endif
   cmsLog_error("at least one of the SUPPORT_DM_xyz flags should be defined!");
   return -1;
}


static const MdmOidInfoEntry *getOidInfo(MdmObjectId oid,
                                         const MdmOidInfoEntry *begin,
                                         const MdmOidInfoEntry *end)
{
   const MdmOidInfoEntry *middle;

   middle = &(begin[(end-begin)/2]);

#ifdef trace_search
   const MdmOidInfoEntry *realbegin=NULL;
   const MdmOidInfoEntry *realend=NULL;
   mdm_getOidInfoPtrs(&realbegin, &realend);
   cmsLog_error("[%d] %d[%d] %d[%d] %d[%d]", oid,
         (begin-realbegin), begin->oid,
         (middle-realbegin), middle->oid,
         (end-realbegin), end->oid);
#endif

   if (oid == middle->oid)
   {
//      cmsLog_debug("found oid %d at index %d", oid, (middle-realbegin));
      return middle;
   }

   if ((oid < middle->oid) && (begin < middle))
   {
      return getOidInfo(oid, begin, middle-1);
   }
   else if ((oid > middle->oid) && (middle < end))
   {
      return getOidInfo(oid, middle+1, end);
   }

   cmsLog_error("could not find oid=%d (last oid checked=%d)",
                 oid, begin->oid);
   return NULL;
}


const MdmOidInfoEntry *mdm_getOidInfo(MdmObjectId oid)
{
   const MdmOidInfoEntry *begin=NULL;
   const MdmOidInfoEntry *end=NULL;

   mdm_getOidInfoPtrs(&begin, &end);

   return getOidInfo(oid, begin, end);
}


// Note that just like getOidInfo() above, "end" must point to the last
// valid entry in the array.
static OidNodeEntry *getOidNodeEntry(MdmObjectId oid,
                                     OidNodeEntry *begin, OidNodeEntry *end)
{
   OidNodeEntry *middle;

   if ((begin == NULL) || (end == NULL))
   {
      cmsLog_error("NULL begin %p or end %p", begin, end);
      return NULL;
   }
   if (begin > end)
   {
      // this happens if oid is not in array
      cmsLog_debug("begin %p past end %p", begin, end);
      return NULL;
   }

   middle = &(begin[(end-begin)/2]);
// cmsLog_debug("want oid=%d middle=%d (%p) end=%d (%p)",
//               oid, middle->oid, middle, end->, end);
   if (oid == middle->oid)
   {
      return middle;
   }
   else if ((oid < middle->oid) && (begin < middle))
   {
      return (getOidNodeEntry(oid, begin, middle-1));
   }
   else if ((oid > middle->oid) && (middle < end))
   {
      return (getOidNodeEntry(oid, middle+1, end));
   }

   return NULL;
}

MdmObjectNode *mdm_getLocalObjNode(MdmObjectId oid)
{
   OidNodeEntry *oidNode, *end;

   if ((mdmShmCtx->localOidNodeTableBegin == NULL) ||
       (mdmShmCtx->localOidNodeTableEnd == NULL))
   {
      cmsLog_error("localOidNodeTableBegin (%p) or End (%p) is NULL",
                   mdmShmCtx->localOidNodeTableBegin,
                   mdmShmCtx->localOidNodeTableEnd);
      return NULL;
   }

   // After mdm_init, the localObjNodeTable is completely filled, so it
   // should not be necessary to do a backward search for the end ptr, but
   // just in case some code tries to call this function before mdm_init is
   // done, we have to find the correct end pointer.
   end = mdmShmCtx->localOidNodeTableEnd-1;
   while ((end->oid == 0) &&
          (end > mdmShmCtx->localOidNodeTableBegin))
   {
      end--;
   }

   oidNode = getOidNodeEntry(oid, mdmShmCtx->localOidNodeTableBegin, end);
   if (oidNode != NULL)
   {
      return oidNode->objNode;
   }
   return NULL;
}

MdmObjectNode *mdm_getRemoteObjNode(MdmObjectId oid)
{
   OidNodeEntry *oidNode, *end;

   if ((mdmShmCtx->remoteOidNodeTableBegin == NULL) ||
       (mdmShmCtx->remoteOidNodeTableEnd == NULL))
      return NULL;

   // After mdm_init, remoteOidNodeTableEnd will be updated to point to
   // 1 past the last valid entry.  But during mdm_init, some code will
   // try to call this function, so we have to find the correct end ptr.
   end = mdmShmCtx->remoteOidNodeTableEnd-1;
   while ((end->oid == 0) &&
          (end > mdmShmCtx->remoteOidNodeTableBegin))
   {
      end--;
   }

   oidNode = getOidNodeEntry(oid, mdmShmCtx->remoteOidNodeTableBegin, end);
   if (oidNode != NULL)
   {
      return oidNode->objNode;
   }
   return NULL;
}

MdmObjectNode *mdm_getObjectNode(MdmObjectId oid)
{
   return (mdm_getObjectNodeFlags(oid, GET_OBJNODE_LOCAL, NULL));
}

/** Return objectNode for given OID.
 *  This function is called a lot!  Keep it very efficient and don't put any
 *  debug statements here.
 */
MdmObjectNode *mdm_getObjectNodeFlags(MdmObjectId oid, UINT32 getFlags,
                                      UINT32 *actualFlags)
{
   if (getFlags & GET_OBJNODE_LOCAL)
   {
      MdmObjectNode *node = mdm_getLocalObjNode(oid);
      if (node != NULL)
      {
         if (actualFlags != NULL)
            *actualFlags = GET_OBJNODE_LOCAL;
         return node;
      }
   }

   if (getFlags & GET_OBJNODE_REMOTE)
   {
      MdmObjectNode *node = mdm_getRemoteObjNode(oid);
      if (node != NULL)
      {
         if (actualFlags != NULL)
            *actualFlags = GET_OBJNODE_REMOTE;
         return node;
      }
   }

   return NULL;
}


UBOOL8 cmsMdm_isDataModelDevice2(void)
{
   return (mdmShmCtx->isDataModelDevice2 ? TRUE : FALSE);
}

MdmObjectNode *mdm_getRootObjectNode(void)
{
   return (mdmShmCtx->rootObjNode);
}


UBOOL8 cmsMdm_isCmsClassic(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if ((mdmShmCtx->compName[0] == '\0') ||
       (!strcmp(mdmShmCtx->compName, BDK_COMP_CMS_CLASSIC)))
   {
      return TRUE;
   }
   return FALSE;
}

UBOOL8 cmsMdm_isBdkSysmgmt(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if (!strcmp(mdmShmCtx->compName, BDK_COMP_SYSMGMT))
   {
      return TRUE;
   }
   return FALSE;
}

UBOOL8 cmsMdm_isBdkDevinfo(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if (!strcmp(mdmShmCtx->compName, BDK_COMP_DEVINFO))
   {
      return TRUE;
   }
   return FALSE;
}

UBOOL8 cmsMdm_isBdkDiag(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if (!strcmp(mdmShmCtx->compName, BDK_COMP_DIAG))
   {
      return TRUE;
   }
   return FALSE;
}

UBOOL8 cmsMdm_isBdkTr69(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if (!strcmp(mdmShmCtx->compName, BDK_COMP_TR69))
   {
      return TRUE;
   }
   return FALSE;
}

UBOOL8 cmsMdm_isBdkWifi(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if (!strcmp(mdmShmCtx->compName, BDK_COMP_WIFI))
   {
      return TRUE;
   }
   return FALSE;
}

UBOOL8 cmsMdm_isBdkOpenPlat(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   if (!strcmp(mdmShmCtx->compName, BDK_COMP_OPENPLAT))
   {
      return TRUE;
   }
   return FALSE;
}

const char *cmsMdm_getMyCompName(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   return mdmShmCtx->compName;
}

UBOOL8 cmsMdm_isRemoteCapable(void)
{
   if (mdmShmCtx == NULL)
   {
      cmsLog_error("Must be attached to an initialized MDM before calling %s", __FUNCTION__);
      return FALSE;
   }

   return (mdmShmCtx->isRemoteCapable);
}

