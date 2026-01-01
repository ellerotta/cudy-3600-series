/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom Ltd
 *  All Rights Reserved
 *
<:label-BRCM:2020:DUAL/GPL:standard

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
 *
 ************************************************************************/


/*!\file bcm_generic_hal_utils.c
 * \brief This file implements some common utility functions associated with
 *        the BCM Generic HAL.  These are so useful that some code which do
 *        not use the BCM Generic HAL will still want to call these functions.
 *
 */

#include <stdlib.h>         /* for NULL */

#include "bcm_generic_hal_utils.h"
#include "cms_mem.h"


void cmsUtl_copyParamInfoArray(BcmGenericParamInfo *dst,
                               BcmGenericParamInfo *src,
                               UINT32 numParamInfos,
                               UBOOL8 deepCopy)
{
   UINT32 i;

   for (i=0; i < numParamInfos; i++)
   {
      // First do the initial copy
      *dst = *src;
      // memcpy((void *)dst, (void *)src, sizeof(BcmGenericParamInfo));

      if (deepCopy)
      {
         // dst gets an independent copy of the string ptr.
         dst->fullpath = cmsMem_strdup(src->fullpath);
         dst->type = cmsMem_strdup(src->type);
         dst->value = cmsMem_strdup(src->value);
         dst->profile = cmsMem_strdup(src->profile);
      }
      else
      {
         // transfer ownership of pointers to dst.  src ptrs become NULL.
         src->fullpath = NULL;
         src->type = NULL;
         src->value = NULL;
         src->profile = NULL;
      }

      src++;
      dst++;
   }

   return;
}

void cmsUtl_freeParamInfoArray(BcmGenericParamInfo  **paramInfoArray,
                               UINT32                 numParamInfos)
{
   BcmGenericParamInfo *a;
   UINT32 i;

   if (paramInfoArray == NULL)
      return;

   a = *paramInfoArray;
   if (a == NULL)
      return;

   // Due to the way cmsMem_alloc works, we might have a weird scenario where
   // the numParamInfos is 0, but "a" points to a valid piece of memory that
   // still needs to be freed.  The algo below still works even if num == 0.
   for (i=0; i < numParamInfos; i++)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(a[i].fullpath);
      CMSMEM_FREE_BUF_AND_NULL_PTR(a[i].type);
      CMSMEM_FREE_BUF_AND_NULL_PTR(a[i].value);
      CMSMEM_FREE_BUF_AND_NULL_PTR(a[i].profile);
   }

   // Free the array buffer and set to NULL.
   CMSMEM_FREE_BUF_AND_NULL_PTR(*paramInfoArray);
   return;
}

void cmsUtl_copyParamAttrArray(BcmGenericParamAttr *dst,
                               BcmGenericParamAttr *src,
                               UINT32 numParamAttrs,
                               UBOOL8 deepCopy)
{
   UINT32 i;

   for (i=0; i < numParamAttrs; i++)
   {
      // First do the initial copy
      // memcpy((void *)dst, (void *)src, sizeof(BcmGenericParamAttr));
      *dst = *src;

      if (deepCopy)
      {
         // dst gets an independent copy of the string ptr.
         dst->fullpath = cmsMem_strdup(src->fullpath);
      }
      else
      {
         // transfer ownership of pointers to dst.  src ptrs become NULL.
         src->fullpath = NULL;
      }

      src++;
      dst++;
   }

   return;
}

void cmsUtl_freeParamAttrArray(BcmGenericParamAttr  **paramAttrArray,
                               UINT32                 numParamAttrs)
{
   BcmGenericParamAttr *a;
   UINT32 i;

   if (paramAttrArray == NULL)
      return;

   a = *paramAttrArray;
   if (a == NULL)
      return;

   for (i=0; i < numParamAttrs; i++)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(a[i].fullpath);
   }

   // Free the array buffer and set to NULL.
   CMSMEM_FREE_BUF_AND_NULL_PTR(*paramAttrArray);
   return;
}


/** Free all strings in the array, and then free the array itself.
 *  Set the pointer to NULL (that is why ***).
 */
void cmsUtl_freeArrayOfStrings(char ***array, UINT32 len)
{
   char **arrayOfStrings;
   UINT32 i;

   if (array == NULL)
      return;

   arrayOfStrings = *array;
   if (arrayOfStrings == NULL)
      return;

   // See comments in  bcm_generic_freeParamInfos about len == 0.
   for (i=0; i < len; i++)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(arrayOfStrings[i]);
   }

   // Null out the pointer from the caller so it is not accidentally used.
   CMSMEM_FREE_BUF_AND_NULL_PTR(*array);
   return;
}


