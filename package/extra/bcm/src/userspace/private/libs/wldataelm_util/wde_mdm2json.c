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

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <json-c/json.h>
#include "wde_mdm2json.h"
#include "tables.h"
#include "wde_access.h"

#ifdef LIBXML_TREE_ENABLED

static int is_multiple_instance(xmlNode *node)
{
    xmlChar *instance = NULL;

    instance = xmlGetProp(node, (xmlChar*)"instance");
    if (instance)
    {
        //printf("instance=%s\n", instance);
        xmlFree(instance);
        return 1;
    }
    else
        return 0;
}

static int is_instance_end(xmlNode *node)
{
    xmlChar *instance = NULL;

    instance = xmlGetProp(node, (xmlChar*)"nextInstance");
    if (instance)
    {
        //printf("nextInstance=%s\n", instance);
        xmlFree(instance);
        return 1;
    }
    else
        return 0;
}

static void traverse_elements_and_build_json(xmlNode *anode, json_object *jobj, char *fullpath)
{
    int ret = -1;
    xmlNode *cur_node = NULL;
    json_object *cur_jobj = NULL;
    xmlChar *cur_node_val = NULL;
    json_object *cur_mapped_jarray = NULL;
    static int sorted = 0;

    if (sorted == 0)
    {
        sort_table();
        sorted = 1;
    }

    for (cur_node = anode; cur_node; cur_node = cur_node->next)
    {
        char cur_fullpath[FULLPATH_LEN] = {0};

        if (cur_node->type == XML_ELEMENT_NODE)
        {
            /* processing a leaf node */
            if (xmlChildElementCount(cur_node) == 0)
            {
                if (1 == is_instance_end(cur_node))
                {
                    cur_mapped_jarray = NULL;
                }
                else
                {
                    snprintf(cur_fullpath, FULLPATH_LEN, "%s%s", fullpath, cur_node->name);
                    //printf("(%s:%d) cur_fullpath=[%s]\n", __func__, __LINE__, cur_fullpath);

                    /* create a json object that mapped to WDE according to TR-181 object fullpath,
                     * and fill in the value (might be translated).
                     */
                    cur_node_val = xmlNodeGetContent(cur_node);
                    build_wde_obj_and_value_mapped(cur_fullpath, (char*)cur_node_val, jobj);
                    if (cur_node_val)
                    {
                        xmlFree(cur_node_val);
                    }
                }
            }
            /* processing an internal node with at least one child */
            else
            {
                //printf("cur_node->name=[%s]\n", cur_node->name);

                /*  if the node is an instance of a multiple instances object */
                if (1 == is_multiple_instance(cur_node)) 
                {
                    /* when visiting the first instance of this multiple instances object,
                     * if it can be mapped, normally we will create a json array to contain all the instances of the object
                     */
                    snprintf(cur_fullpath, FULLPATH_LEN, "%s%s.{i}.", fullpath, cur_node->name);
                    //printf("(%s:%d) cur_fullpath=[%s]\n", __func__, __LINE__, cur_fullpath);
                    if (!cur_mapped_jarray)
                    {
                        /* create a json object that mapped to WDE according to TR-181 object fullpath */
                        ret = build_wde_obj_mapped(cur_fullpath, &cur_jobj, jobj);

                        if (ret == 0) // Normal case: mapped and a json ARRAY is created
                        {
                            cur_mapped_jarray = cur_jobj;
                        }
                        else if (ret == 1) // Special case: mapped and a json OBJECT is created
                        {
                            /* This is a special case when a TR-181 multiple instances object was mapped to a single object of WDE
                             * For example:
                             * "Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}." <-> "Network.DeviceList.CACStatus"
                             */
                            goto go_through_child;
                        }
                        else // unmapped and stop going through its child
                        {
                            //printf("(%s:%d) IGNORE fullpath=[%s]\n", __func__, __LINE__, cur_fullpath);
                            continue;
                        }
                    }

                    /* we are visiting the instance that are mapped */

                    // create a new json object for this object instance
                    cur_jobj = json_object_new_object();
                    
                    // add cur_jobj to the mapped json array
                    json_object_array_add(cur_mapped_jarray, cur_jobj);
                }
                /*  if the node is a single instance object */
                else
                {
                    snprintf(cur_fullpath, FULLPATH_LEN, "%s%s.", fullpath, cur_node->name);
                    //printf("(%s:%d) cur_fullpath=[%s]\n", __func__, __LINE__, cur_fullpath);

                    /* create a json object that mapped to WDE according to TR-181 object fullpath */
                    ret = build_wde_obj_mapped(cur_fullpath, &cur_jobj, jobj);

                    if (ret == -1)
                    {
                        /* unmapped, but keep going through its child for a case when
                         * "DataElements.Network.Device.{i}.Radio.{i}.CACCapability." is unmapped and
                         * "DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}." is mapped.
                         */
                        //printf("(%s:%d) IGNORE fullpath=[%s]\n", __func__, __LINE__, cur_fullpath);
                        cur_jobj = jobj;
                    }
                }
            }
go_through_child:
            traverse_elements_and_build_json(cur_node->children, cur_jobj, cur_fullpath);
        }
    }

    return;
}

int wde_mdmToJson(char *xml_buf, int xml_buf_size, json_object *jobj)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    if (!xml_buf)
    {
        fprintf(stderr, "%s: input buffer is NULL\n", __func__);
        return 1;
    }
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /* parse an XML in-memory document and build a tree. */
    doc = xmlReadMemory(xml_buf, xml_buf_size, NULL, NULL, 0);

    if (doc == NULL) {
        fprintf(stderr, "%s: could not parse the input buffer\n", __func__);
        return 1;
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    traverse_elements_and_build_json(root_element, jobj, "Device.WiFi.DataElements.");

    /*free the document */
    xmlFreeDoc(doc);

    /*
     * Free the global variables that may
     * have been allocated by the parser.
     */
    xmlCleanupParser();

    return 0;
}

#else

int wde_mdmToJson(char *xml_buf, int xml_buf_size, json_object *jobj)
{
    fprintf(stderr, "%s: Tree support not compiled in\n", __func__);
    return 1;
}

#endif /* LIBXML_TREE_ENABLED */

