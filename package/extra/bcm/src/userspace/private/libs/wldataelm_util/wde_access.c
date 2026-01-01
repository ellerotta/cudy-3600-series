/***********************************************************************
 *
 *  Copyright (c) 2023  Broadcom
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

#include "tables.h"

static int wde_get_value_translate(int src_type, WDE_DATA_TYPE dst_type, void *src_val, void **output_val, hint_data *hint)
{
    int ret = 0;

    if (!src_val || !output_val || !hint)
    {
        printf("#### invalid input parameters! ####\n");
        return -1;
    }

    //printf("(%s:%d) src_type=[%s] --> dst_type=[%s] \n", __func__, __LINE__, 
    //         XML_DATA_TYPE_STRING[src_type], WDE_DATA_TYPE_STRING[dst_type]);
    switch (TRANS_TYPE(dst_type, src_type))
    {
        case TRANS_TYPE(WDE_TYPE_STRING, XML_TYPE_STRING):
            {
                if (hint->type == HINT_MAC_STR && strlen((char*)src_val) == 0)
                {
                    // assign default value if it's empty
                    *output_val = (void*)json_object_new_string("00:00:00:00:00:00");
                }
                else
                {
                    *output_val = (void*)json_object_new_string((char*)src_val);
                }
                break;
            }

        case TRANS_TYPE(WDE_TYPE_INTEGER, XML_TYPE_STRING):
            {
                int64_t val_int64;
                char *endptr;

                val_int64 =  strtoll((char*)src_val, &endptr, 10);
                *output_val = (void*)json_object_new_int64(val_int64);
                break;
            }

        case TRANS_TYPE(WDE_TYPE_BOOLEAN, XML_TYPE_STRING):
            {
                if (0 == strcmp((char*)src_val, "TRUE"))
                {
                    *output_val = (void*)json_object_new_boolean(1);
                }
                else
                {
                    *output_val = (void*)json_object_new_boolean(0);
                }
                break;
            }

        case TRANS_TYPE(WDE_TYPE_ARRAY, XML_TYPE_STRING):
            {
                char *saveptr = NULL;
                char *substr = NULL;
                const char *delim = ",";
                json_object *val_array = NULL;
                json_object *val = NULL;

                val_array = json_object_new_array();

                substr = strtok_r((char*)src_val, delim, &saveptr);
                while (substr != NULL) {
                    //printf("substr:[%s]\n", substr);
                    if (hint->type == HINT_STR_ARRAY || hint->type == HINT_MAC_ARRAY)
                    {
                        wde_get_value_translate(src_type, WDE_TYPE_STRING, (void*)substr, (void**)&val, hint);
                    }
                    else if (hint->type == HINT_INT_ARRAY)
                    {
                        wde_get_value_translate(src_type, WDE_TYPE_INTEGER, (void*)substr, (void**)&val, hint);
                    }

                    json_object_array_add(val_array, val);
                    substr = strtok_r(NULL, delim, &saveptr);
                }
                *output_val = (void*)val_array;
            }
            break;

        case TRANS_TYPE(WDE_TYPE_OBJECT, XML_TYPE_STRING):
            break;
        default:
            break;
    }
    return ret;
}

int build_wde_obj_and_value_mapped(char *fullpath, char *src_val, json_object *jobj)
{
    Wde_writeback_entry *mapped_entry = NULL;
    json_object *output_val = NULL;
    int added = 0;

    if (!fullpath || !src_val || !jobj)
    {
        printf("#### invalid input parameters! ####\n");
        return -1;
    }

    mapped_entry = get_mapping_entry(fullpath);
    if (mapped_entry == NULL)
    {
        //printf("#### can't get mapping entry!!! ####\n");
        return -1;
    }

    wde_get_value_translate(XML_TYPE_STRING, mapped_entry->wde_node.data_type, (void*)src_val, (void**)&(output_val), &(mapped_entry->wde_node.hint));
    if (output_val)
    {
        // if the fullpath of a mapped wde node is null, then no need to add json object for it
        if (mapped_entry->wde_node.fullpath)
        {
            json_object_object_add(jobj, mapped_entry->wde_node.fullpath, output_val);
            added = 1;
        }

        // additional process for the mapped wde node represents "a number of" wde nodes(or elements)
        // if the number is 0, we still have to create an empty array for the corresponding node/element.
        if (mapped_entry->wde_node.hint.type == HINT_NUM_OF)
        {
            if (0 == json_object_get_int64(output_val))
            {
                //printf("#### ADD null array for [%s] ####\n", mapped_entry->wde_node.hint.element);
                json_object_object_add(jobj, mapped_entry->wde_node.hint.element, json_object_new_array());
            }
        }

        if (!added)
        {
            json_object_put(output_val);
        }
    }
    return 0;
}

int build_wde_obj_mapped(char *fullpath, json_object **cur_jobj, json_object *parent_jobj)
{
    json_object *jobj = NULL;
    int ret = 0;
    Wde_writeback_entry *mapped_entry = NULL;

    if (!fullpath || !cur_jobj || !parent_jobj)
    {
        printf("#### invalid input parameters! ####\n");
        return -1;
    }

    mapped_entry = get_mapping_entry(fullpath);
    if (mapped_entry == NULL)
    {
        //printf("#### can't get mapping entry!!! ####\n");
        return -1;
    }

    if (mapped_entry->wde_node.data_type == WDE_TYPE_OBJECT) // for single instance object
    {
        jobj = json_object_new_object();
        json_object_object_add(parent_jobj, mapped_entry->wde_node.fullpath, jobj);
        ret = 1;
    }
    else if (mapped_entry->wde_node.data_type == WDE_TYPE_ARRAY) // for multiple instances object
    {
        jobj = json_object_new_array();
        json_object_object_add(parent_jobj, mapped_entry->wde_node.fullpath, jobj);
        ret = 0;
    }
    else
    {
        printf("#### unexpected wde type [%d] ####\n", mapped_entry->wde_node.data_type);
        return -1;
    }
    *cur_jobj = jobj;
    return ret;
}
