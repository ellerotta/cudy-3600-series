/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "cms_util.h"
#include "cms_phl.h"
#include "cms_obj.h"
#include "os_defs.h"
#include "special.h"
#include "nvc.h"
#include "nvn.h"
#include "conv.h"
#include "chanspec.h"
#include "cms_helper.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"


extern SpecialHandler special_handler_table[];
extern const unsigned int SPECIAL_TABLE_SIZE;
static pcre2_code **re_table = NULL;

UBOOL8 match_name(pcre2_code *re, const char *nvname);

WlmdmRet special_init()
{
    pcre2_code *re;
    PCRE2_SPTR pattern;     /* PCRE2_SPTR is a pointer to unsigned code units of */
    int errornumber;
    PCRE2_SIZE erroroffset;
    int  i;

    if (re_table != NULL)
    {
        return WLMDM_OK;
    }

    re_table = cmsMem_alloc(sizeof(pcre2_code *) * SPECIAL_TABLE_SIZE, ALLOC_ZEROIZE);
    if (re_table == NULL)
    {
        cmsLog_error("Failed to alloc buffer!");
        return WLMDM_GENERIC_ERROR;
    }

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        //Regex compilation for the patterns defined in special_handler_table.
        pattern = (PCRE2_SPTR) special_handler_table[i].pattern;
        re = pcre2_compile (
                            pattern,               /* the pattern */
                            PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
                            0,                     /* default options */
                            &errornumber,          /* for error number */
                            &erroroffset,          /* for error offset */
                            NULL);                 /* use default compile context */
        
        /* Compilation failed: print the error message and exit. */
        
        if (re == NULL)
        {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
            cmsLog_error("PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset, buffer);
        }
        else
        {
            re_table[i] = re;
        }
    }
    return WLMDM_OK;
}

void special_free()
{
    int i;

    if (re_table == NULL)
        return;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        pcre2_code_free(re_table[i]);
    }

    cmsMem_free(re_table);
}

WlmdmRet special_set(const char *nvname, const char *value)
{
    unsigned int i;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        if (match_name(re_table[i], nvname) == TRUE)
        {
            return special_handler_table[i].actions->set(nvname, value);
        }
    }
    return WLMDM_NOT_FOUND;
}

WlmdmRet special_get(const char *nvname, char *value, size_t size)
{
    unsigned int i;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        if (match_name(re_table[i], nvname) == TRUE)
        {
            return special_handler_table[i].actions->get(nvname, value, size);
        }
    }
    return WLMDM_NOT_FOUND;
}

WlmdmRet special_foreach(nvc_for_each_func for_each_func, void *data)
{
    unsigned int i;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        cmsLog_debug("handling %s", special_handler_table[i].pattern);
        special_handler_table[i].actions->foreach(for_each_func, data);
    }
    return WLMDM_OK;
}

UBOOL8 match_name(pcre2_code *re, const char *nvname)
{
    int rc;
    UBOOL8 matched = FALSE;
    pcre2_match_data *match_data;

    
    if (re == NULL)
    {
        cmsLog_error("invalid parameter!");
        return FALSE;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    rc = pcre2_match(
                     re,                   /* the compiled pattern */
                     (PCRE2_SPTR)nvname,   /* the subject string */
                     strlen(nvname),       /* the length of the subject */
                     0,                    /* start at offset 0 in the subject */
                     0,                    /* default options */
                     match_data,           /* block for storing the result */
                     NULL);                /* use default match context */
    
    if (rc < 0)
    {
        matched = FALSE;
        if (rc != PCRE2_ERROR_NOMATCH)
        {
            cmsLog_error("Matching error!");
        }
    }
    else
    {
        matched = TRUE;
    }

    pcre2_match_data_free(match_data);
    return matched;
}
