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

#ifndef __TABLES_H__
#define __TABLES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <json-c/json.h>
#include <assert.h>

#define TO_STR(X) #X
#define ENUM_TO_SPECFIC_STR(X, Y) [(X)] = (Y)
#define ENUM_TO_STR(X) ENUM_TO_SPECFIC_STR(X, TO_STR(X))
#define FULLPATH_LEN 1024

typedef enum
{
	INSTANCE_DEVICE,
	INSTANCE_INTERFACE,
	INSTANCE_BSS,
	INSTANCE_STATION,
	INSTANCE_NEIGHBOR,
	INSTANCE_LEGACY_NEIGHBOR,
	INSTANCE_BRIDGE_ENTRY,
	INSTANCE_UNSPECIFIED,
	INSTANCE_TYPE_MAX
} INSTANCE_TYPE;

typedef enum
{
	HINT_NONE,
	HINT_STRING,
	HINT_STR_ARRAY,
	HINT_INT_ARRAY,
	HINT_MAC_ARRAY,
	HINT_MAC_STR,
	HINT_NUM_OF
} HINT_TYPE;

#define WDE_DATA_TYPE_BIT (4)

#define XML_TYPE_STRING 0x0

#define TRANS_TYPE(wde_type, xml_type) (((xml_type)<<WDE_DATA_TYPE_BIT)|(wde_type))

static char* XML_DATA_TYPE_STRING[1] __attribute__ ((unused)) =
{
	ENUM_TO_SPECFIC_STR(XML_TYPE_STRING,	"STRING")
};

typedef enum
{
	WDE_TYPE_STRING,
	WDE_TYPE_INTEGER,
	WDE_TYPE_OBJECT,
	WDE_TYPE_BOOLEAN,
	WDE_TYPE_ARRAY,
	WDE_TYPE_MAX = ((1<<(WDE_DATA_TYPE_BIT))-1)
} WDE_DATA_TYPE;

static char* WDE_DATA_TYPE_STRING[WDE_TYPE_MAX+1] __attribute__ ((unused)) =
{
	ENUM_TO_SPECFIC_STR(WDE_TYPE_STRING,	"STRING"),
	ENUM_TO_SPECFIC_STR(WDE_TYPE_INTEGER,	"INTEGER"),
	ENUM_TO_SPECFIC_STR(WDE_TYPE_OBJECT,	"OBJECT"),
	ENUM_TO_SPECFIC_STR(WDE_TYPE_BOOLEAN,   "BOOLEAN"),
	ENUM_TO_SPECFIC_STR(WDE_TYPE_ARRAY,	    "ARRAY"),
	ENUM_TO_SPECFIC_STR(WDE_TYPE_MAX,	    "MAX")
};

typedef struct
{
	HINT_TYPE type;
	union {
		//For HINT_STRING
		char*        str;
        //For HINT_NUM_OF
		char*        element;
	};
} hint_data, *hint_data_p;

typedef int (*Retrieve_Hook)(INSTANCE_TYPE instance_type, void* instance_ptr, hint_data *hint,
	WDE_DATA_TYPE data_type, void** out, unsigned int *out_flag);

typedef struct wde_writeback_entry Wde_writeback_entry;

typedef struct
{
	char *name;
	json_object *value;
} wde_node_info;

typedef struct
{
	char *fullpath;
	int oid;
	char *name;
} mdm_node_desc;

typedef struct
{
	char *fullpath;
	WDE_DATA_TYPE  data_type;
	Retrieve_Hook  retrieve_func;
	hint_data  hint;
} wde_node_desc;

typedef struct wde_writeback_entry Wde_writeback_entry;
struct wde_writeback_entry
{
	mdm_node_desc mdm_node;
	wde_node_desc wde_node;
};

typedef struct
{
    char last_fullpath[FULLPATH_LEN];
    Wde_writeback_entry *entry;
} wde_wb_entry_cache;

int sort_table(void);
Wde_writeback_entry* get_mapping_entry(char *fullpath);

#endif //__TABLES_H__
