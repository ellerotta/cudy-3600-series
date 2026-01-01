/***********************************************************************
 *
<:copyright-BRCM:2020:DUAL/GPL:standard

   Copyright (c) 2020 Broadcom
   All Rights Reserved

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

#include <string.h>
#include "cms_retcodes.h"
#include "cms_util.h"
#include "cms_vbuf.h"

CmsVbuf* cmsVbuf_new()
{
    CmsVbuf *s;
    s = (CmsVbuf *) cmsMem_alloc(sizeof(CmsVbuf), ALLOC_ZEROIZE);
    if (s != NULL) {
        s->data = cmsMem_alloc(CMS_VBUF_INITIAL_SIZE, ALLOC_ZEROIZE);
        if (s->data == NULL)
        {
            cmsMem_free(s);
            return NULL;
        }
        s->maxSize = CMS_VBUF_INITIAL_SIZE;
        s->size = 0;
        s->index = 0;
    }
    return s;
}

void cmsVbuf_destroy(CmsVbuf *s)
{
    if (s == NULL)
    {
        return;
    }
    if (s->data != NULL)
    {
        cmsMem_free(s->data);
    }
    cmsMem_free(s);
}

size_t cmsVbuf_getSize(CmsVbuf *s)
{
    if (s == NULL)
    {
        return 0;
    }
    return s->size;
}

static CmsRet cmsVbuf_reserveSpace(CmsVbuf *vbuf, UINT32 size)
{
    if (vbuf == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (vbuf->size + size > vbuf->maxSize)
    {
        UINT32 newSize;
        void *new;

        newSize = vbuf->maxSize + ((size > CMS_VBUF_GROW_SIZE) ?
                                    size : CMS_VBUF_GROW_SIZE);
        new = cmsMem_realloc(vbuf->data, newSize);
        if (new == NULL)
        {
            return CMSRET_RESOURCE_EXCEEDED;
        }
        vbuf->data = new;
        vbuf->maxSize = newSize;
    }
    return CMSRET_SUCCESS;
}

CmsRet cmsVbuf_put(CmsVbuf *vbuf, const void *data, size_t size)
{
    CmsRet ret;
    ret = cmsVbuf_reserveSpace(vbuf, size);
    if (ret == CMSRET_SUCCESS)
    {
        memcpy((char *)(vbuf->data) + vbuf->size, data, size);
        vbuf->size += size;
    }
    return ret;
}

CmsRet cmsVbuf_putUINT16(CmsVbuf *vbuf, UINT16 data)
{
    CmsRet ret;
    ret = cmsVbuf_put(vbuf, &data, sizeof(UINT16));
    return ret;
}

CmsRet cmsVbuf_putUINT32(CmsVbuf *vbuf, UINT32 data)
{
    CmsRet ret;
    ret = cmsVbuf_put(vbuf, &data, sizeof(UINT32));
    return ret;
}

CmsRet cmsVbuf_putUBOOL8(CmsVbuf *vbuf, UBOOL8 data)
{
    CmsRet ret;
    ret = cmsVbuf_put(vbuf, &data, sizeof(UBOOL8));
    return ret;
}

CmsRet cmsVbuf_putString(CmsVbuf *vbuf, const char* data)
{
    CmsRet ret;
    if (data == NULL)
    {
        // Silently replace a NULL input string pointer with empty string.
        ret = cmsVbuf_put(vbuf, "", 1);
    }
    else
    {
        ret = cmsVbuf_put(vbuf, data, strlen(data) + 1);
    }
    return ret;
}


CmsRet cmsVbuf_getAtOffset(CmsVbuf *vbuf, size_t offset, void *pData, size_t size)
{
    if (vbuf == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (offset + size > vbuf->size)
    {
        cmsLog_error("past end of data (offset %zu + size %zu > size %d)",
                     offset, size, vbuf->size);
        return CMSRET_NO_MORE_INSTANCES;
    }

    memcpy(pData, (char *)(vbuf->data) + offset, size);
    return CMSRET_SUCCESS;
}

void cmsVbuf_resetIndex(CmsVbuf *vbuf)
{
    if (vbuf == NULL)
    {
        return;
    }

    vbuf->index = 0;
}

CmsRet cmsVbuf_get(CmsVbuf *vbuf, void *pData, size_t size)
{
    if (vbuf == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (vbuf->index == vbuf->size)
    {
        // This is a normal situation when the caller is reading/iterating
        // through the entire vbuf but we have read to the end of vbuf data.
        cmsLog_debug("no more data (index %d + size %zu > size %d)",
                     vbuf->index, size, vbuf->size);
        return CMSRET_NO_MORE_INSTANCES;
    }

    if (vbuf->index + size > vbuf->size)
    {
        cmsLog_error("past end of data (index %d + size %zu > size %d)",
                     vbuf->index, size, vbuf->size);
        return CMSRET_NO_MORE_INSTANCES;
    }

    memcpy(pData, (char *)(vbuf->data) + vbuf->index, size);
    vbuf->index += size;
    return CMSRET_SUCCESS;
}

CmsRet cmsVbuf_getUINT16(CmsVbuf *vbuf, UINT16 *pData)
{
    CmsRet ret;
    ret = cmsVbuf_get(vbuf, pData, sizeof(UINT16));
    return ret;
}

CmsRet cmsVbuf_getUINT32(CmsVbuf *vbuf, UINT32 *pData)
{
    CmsRet ret;
    ret = cmsVbuf_get(vbuf, pData, sizeof(UINT32));
    return ret;
}

CmsRet cmsVbuf_getUBOOL8(CmsVbuf *vbuf, UBOOL8 *pData)
{
    CmsRet ret;
    ret = cmsVbuf_get(vbuf, pData, sizeof(UBOOL8));
    return ret;
}

CmsRet cmsVbuf_getString(CmsVbuf *vbuf, char **data)
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 i;
    char *cdata, *s;

    if (vbuf == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (vbuf->index >= vbuf->size)
    {
        cmsLog_error("no more data (index %d >= size %d)", vbuf->index, vbuf->size);
        return CMSRET_NO_MORE_INSTANCES;
    }

    // verify there is a null term char before end of data.  Sometimes, the
    // developer might pass in the wrong vbuf.
    cdata = (char *) (vbuf->data);
    i = vbuf->index;
    while (i < vbuf->size)
    {
        if (cdata[i] == '\0')
            break;
        i++;
    }
    if (i >= vbuf->size)
    {
        cmsLog_error("vbuf does not contain string, index=%d size=%d",
                     vbuf->index, vbuf->size);
        return CMSRET_INVALID_ARGUMENTS;
    }

    s = cmsMem_strdup((char *)(vbuf->data) + vbuf->index);
    if (s == NULL)
    {
        cmsLog_error("Failed to duplicate string from index %d!", vbuf->index);
        return CMSRET_RESOURCE_EXCEEDED;
    }
    cmsLog_debug("returning string from index %d:%s", vbuf->index, s);
    vbuf->index += (strlen(s) + 1);
    *data = s;
    return ret;
}
