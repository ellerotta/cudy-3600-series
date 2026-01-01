/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2020:proprietary:standard

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

#include "cms_util.h"
#include "phl_merge.h"
#include "genutil_regex.h"

typedef enum {
    ACTION_KEEP_ONE = 0,
    ACTION_ADD_VALUE = 1,
    ACTION_INSERT = 2
} MERGE_ACTION;

typedef struct {
    char *pattern;
    MERGE_ACTION action;
    RE_NODE *renode;     // compiled version of the pattern
} PhlMergeRenode;

// Put all ACTION_ADD_VALUE and ACTION_KEEP_ONE in pass1 so these rows will
// be dealt with first.
static PhlMergeRenode renodes_pass1[] = {
    {"^Device\\.$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.InterfaceStackNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.[a-zA-Z_]+$", ACTION_KEEP_ONE, NULL},  // this catches all other param names in Device.
    {"^Device\\.Services\\.$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.Services\\.StorageServiceNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.Services\\.VoiceServiceNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.X_BROADCOM_COM_TM\\.", ACTION_KEEP_ONE, NULL},
    {"^Device\\.X_BROADCOM_COM_AppCfg\\.$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.X_BROADCOM_COM_AppCfg\\.BcmMsgdCfg\\.", ACTION_KEEP_ONE, NULL},
    {"^Device\\.IP\\.$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.IP\\.InterfaceNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.IP\\.ActivePortNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.IP\\.[a-zA-Z_]+$", ACTION_KEEP_ONE, NULL},  // see comment in rcl_dev2IpObject()
    {"^Device\\.QoS\\.$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.QoS\\.ClassificationNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.AppNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.FlowNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.PolicerNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.QueueNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.QueueStatsNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.ShaperNumberOfEntries$", ACTION_ADD_VALUE, NULL},
    {"^Device\\.QoS\\.[a-zA-Z_]+\\.$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.QoS\\.[a-zA-Z_]+$", ACTION_KEEP_ONE, NULL},
    {"^Device\\.InterfaceStack\\.$", ACTION_KEEP_ONE, NULL},
};

#define NUM_RENODES_PASS1  (sizeof(renodes_pass1) / sizeof(PhlMergeRenode))

// Put ACTION_INSERT in pass2 because ACTION_INSERT will grab a chunk of
// rows, which may contain ADD_VALUE or KEEP_ONE entries.
static PhlMergeRenode renodes_pass2[] = {
    {"^Device\\.Services\\.[a-zA-Z_]+", ACTION_INSERT, NULL},
    {"^Device\\.IP\\.[a-zA-Z0-9_]+", ACTION_INSERT, NULL},  // keep Device.IP from sysmgmt and Device.IP.Diagnostics from diag comp together
    {"^Device\\.X_BROADCOM_COM_AppCfg\\.[a-zA-Z0-9_]+\\.", ACTION_INSERT, NULL},
    {"^Device\\.QoS\\.[a-zA-Z_]+\\.[0-9]+\\.", ACTION_INSERT, NULL},
    {"^Device\\.InterfaceStack\\.[0-9]+\\.", ACTION_INSERT, NULL},
};

#define NUM_RENODES_PASS2  (sizeof(renodes_pass2) / sizeof(PhlMergeRenode))

static UBOOL8 renodes_initialized = FALSE;

static void init_renodes()
{
    UINT32 i;

    if (renodes_initialized)
    {
        return;
    }
    renodes_initialized = TRUE;

    for (i = 0; i < NUM_RENODES_PASS1; i++)
    {
        renodes_pass1[i].renode = regex_compile(renodes_pass1[i].pattern);
        if (renodes_pass1[i].renode == NULL)
        {
            cmsLog_error("Failed to compile pass1 regex pattern [%d] %s",
                         i, renodes_pass1[i].pattern);
        }
    }

    for (i = 0; i < NUM_RENODES_PASS2; i++)
    {
        renodes_pass2[i].renode = regex_compile(renodes_pass2[i].pattern);
        if (renodes_pass2[i].renode == NULL)
        {
            cmsLog_error("Failed to compile pass2 regex pattern [%d] %s",
                         i, renodes_pass2[i].pattern);
        }
    }

    return;
}

static UBOOL8 mergeKeepOne(ParamIndexEntry *table, SINT32 tableSize,
                           SINT32 index);

static SINT32 mergeInsert(ParamIndexEntry *table, SINT32 tableSize,
                          RE_NODE *re_node, SINT32 index);

static UBOOL8 mergeAddValue(ParamIndexEntry *table, SINT32 tableSize,
                            RE_NODE *re_node, SINT32 index);


CmsRet cmsPhl_mergeParams(char **pStrLocalList, SINT32 numLocal,
                              char **pStrRemoteList, SINT32 numRemote,
                              ParamIndexEntry **result, SINT32 *numIndex,
                              UBOOL8 addValue)
{
    CmsRet ret = CMSRET_SUCCESS;
    SINT32 i, tableSize, chunkSize;
    UINT32 m;
    ParamIndexEntry *table;
    UBOOL8 processed;

    cmsLog_notice("Entered: numLocal=%d numRemote=%d", numLocal, numRemote);

    init_renodes();

    tableSize = numLocal + numRemote;
    table = cmsMem_alloc(tableSize * sizeof(ParamIndexEntry), ALLOC_ZEROIZE);
    if (table == NULL)
    {
        cmsLog_error("Failed to allocate ParamIndexEntry table (%d)!", tableSize);
        return CMSRET_RESOURCE_EXCEEDED;
    }

    // Fill the final merged table with local entries first.
    for (i = 0; i < numLocal; i++)
    {
        table[i].source = LOCAL;
        table[i].index = i;
        table[i].fullPath = cmsMem_strdup(pStrLocalList[i]);
    }

    // Then fill the table with remote entries.
    for (i = 0; i < numRemote; i++)
    {
        table[numLocal + i].source = REMOTE;
        table[numLocal + i].index = i;
        table[numLocal + i].fullPath = cmsMem_strdup(pStrRemoteList[i]);
    }

    // If remoteList is empty, there is no need to merge (there are only local entries).
    // But even if localList is empty, still need to merge because remoteLlist
    // can contain multiple components, and there could be duplicates in there.
    if (numRemote == 0)
    {
        goto exit;
    }

    //Go through the merge table to determine action of each item in remote list.
    //Pass1: do the keep_one and add_value actions.
    for (i = numLocal; i < tableSize; i++)
    {
        for (m = 0; m < NUM_RENODES_PASS1; m++)
        {
            if (renodes_pass1[m].renode == NULL)
            {
                // there was an error when we tried to compile this regex.
                cmsLog_error("pass1 renode[%d] is NULL!  skip it.", m);
                continue;
            }

            if (1 == regex_match(renodes_pass1[m].renode, table[i].fullPath))
            {
                cmsLog_notice("pass1 [%d]%s matched pattern [%d]%s action=%d",
                              i, table[i].fullPath,
                              m, renodes_pass1[m].pattern,
                              renodes_pass1[m].action);
                switch (renodes_pass1[m].action)
                {
                    case ACTION_KEEP_ONE:
                        if (TRUE == mergeKeepOne(table, tableSize, i))
                        {
                            tableSize--;
                            i--;
                        }
                        break;

                    case ACTION_ADD_VALUE:
                        // getParamValues will addValue, but getParamNames and getParamAttr will not.
                        if (addValue == TRUE)
                        {
                            processed = mergeAddValue(table, tableSize,
                                                      renodes_pass1[m].renode,
                                                      i);
                        }
                        else
                        {
                            processed = mergeKeepOne(table, tableSize, i);
                        }

                        if (TRUE == processed)
                        {
                            tableSize--;
                            i--;
                        }
                        break;

                    default:
                        cmsLog_error("pass1: unknown or invalid action [%d]%d!",
                                     m, renodes_pass1[m].action);
                        break;
                }
                break;
            }
        }
    }

    // Pass2: do the insert (pullup) action
    for (i = numLocal; i < tableSize; i++)
    {
        for (m = 0; m < NUM_RENODES_PASS2; m++)
        {
            if (renodes_pass2[m].renode == NULL)
            {
                // there was an error when we tried to compile this regex.
                cmsLog_error("pass2 renode[%d] is NULL!  skip it.", m);
                continue;
            }

            if (1 == regex_match(renodes_pass2[m].renode, table[i].fullPath))
            {
                cmsLog_notice("pass2 [%d]%s matched pattern [%d]%s action=%d",
                              i, table[i].fullPath,
                              m, renodes_pass2[m].pattern,
                              renodes_pass2[m].action);
                switch (renodes_pass2[m].action)
                {
                    case ACTION_INSERT:
                        chunkSize = mergeInsert(table, tableSize,
                                                renodes_pass2[m].renode, i);
                        if (chunkSize > 0)
                        {
                            i += chunkSize - 1;
                        }
                        break;

                    default:
                        cmsLog_error("pass2: unknown or invalid action [%d]%d!",
                                     m, renodes_pass2[m].action);
                        break;
                }
                break;
            }
        }
    }

exit:
    *result = table;
    *numIndex = tableSize;
    return ret;
}

//Return true if removed one item. Otherwise return FALSE.
static UBOOL8 mergeKeepOne(ParamIndexEntry *table, SINT32 tableSize,
                           SINT32 index)
{
    SINT32 i;

    if (index > tableSize)
    {
        cmsLog_error("index out of bound! index=%u, tableSize=%u", index, tableSize);
        return FALSE;
    }

    // Look at all the entries in the table prior to current index
    for (i = 0; i < index; i++)
    {
        if (0 == strcmp(table[i].fullPath, table[index].fullPath))
        {
            //Remove the table entry at index
            cmsLog_notice("Removing %s from %dth", table[index].fullPath, index);
            CMSMEM_FREE_BUF_AND_NULL_PTR(table[index].fullPath);
            // pull all table entries up by 1 slot
            memmove(table + index, table + index + 1,
                    (tableSize - index - 1) * sizeof(ParamIndexEntry));
            return TRUE;
        }
    }
    return FALSE;
}

// Return number of rows pulled up.  Note that if the current chunk of rows
// are already at the right position, the chunksize is still returned so that
// the scanning algorithm in the caller can resume scanning at the bottom of
// the chunk.  Also Note this function does not delete any rows, it only moves
// a block of rows up.
static SINT32 mergeInsert(ParamIndexEntry *table, SINT32 tableSize,
                          RE_NODE *re_node, SINT32 index)
{
    SINT32 i, j;
    SINT32 chunkSize = 0;

    if (index > tableSize)
    {
        cmsLog_error("index out of bound! index=%u, tableSize=%u", index, tableSize);
        return -1;
    }


    // Go backwards from the matched point (index) to see if there are other
    // similar parameters we can pull this chunk of params up to.
    // if i >= 0, that is the place we want to pull this chunk of params to.
    for (i = index - 1; i >= 0; i--)
    {
        if (1 == regex_match(re_node, table[i].fullPath))
        {
            cmsLog_debug("insert after [%d]:%s", i, table[i].fullPath);
            cmsLog_debug("first chunk to be moved [%d]:%s", index, table[index].fullPath);
            break;
        }
    }
    if (i >= 0)
    {
        for (j = index; j < tableSize; j++)
        {
            if (1 != regex_match(re_node, table[j].fullPath))
            {
                break;
            }
        }

        // chunkSize is the number of similar params that we will pull-up.
        // i+1 is destination point
        // index is src point
        // (index-i-1) is the amount of orginal data we have to move down.
        chunkSize = j - index;
        cmsLog_debug("chunkSize=%d i+1=%d index=%d (index-i-1)=%d",
                     chunkSize, i+1, index, index-i-1);
        if (index-i-1 == 0) // could be re-written as (i == index - 1)
        {
           // This chunk is already at the right place, no need to move.
           return chunkSize;
        }
        if (chunkSize > 0)
        {
            // copy the chunk to be moved.
            ParamIndexEntry *dup;
            dup = cmsMem_alloc(chunkSize * sizeof(ParamIndexEntry), ALLOC_ZEROIZE);
            memcpy(dup, table + index, chunkSize * sizeof(ParamIndexEntry));

            // move original table contents at the insert point (table+i+1) down.
            // Note that memmove can handle scenario where dest and src overlap.
            memmove(table+i+1+chunkSize, table+i+1, (index-i-1)*sizeof(ParamIndexEntry));

            // copy the chunk up to the correct destination.
            memcpy(table+i+1, dup, chunkSize * sizeof(ParamIndexEntry));
            cmsMem_free(dup);
            return chunkSize;
        }
    }
    return 0;
}

static UBOOL8 mergeAddValue(ParamIndexEntry *table, SINT32 tableSize,
                            RE_NODE *re_node, SINT32 index)
{
    SINT32 i;

    if (index > tableSize)
    {
        cmsLog_error("index out of bound! index=%u, tableSize=%u", index, tableSize);
        return FALSE;
    }

    // Look at the previous entries for the same param to add to.
    // If found, i will be >= 0.
    for (i = index - 1; i >= 0; i--)
    {
        if (1 == regex_match(re_node, table[i].fullPath))
        {
            cmsLog_notice("Found item to be added: [%d]:%s", i, table[i].fullPath);
            break;
        }
    }
    if (i >= 0)
    {
        // Add the ParamIndexEntry at index to the tail of the ParamIndexEntry
        // at i position.
        ParamIndexEntry *dup, *tail;

        dup = cmsMem_alloc(sizeof(ParamIndexEntry), ALLOC_ZEROIZE);
        if (dup == NULL)
        {
            cmsLog_error("Failed to allocate memory!");
            return FALSE;
        }
        *dup = table[index];  // note this dup entry is now the owner of fullpath

        tail = table + i;
        while (tail->next != NULL)
        {
            tail = tail->next;
        }
        tail->next = dup;

        // pull all table entries up by 1 slot
        memmove(table+index, table+index+1,
                (tableSize-index-1)*sizeof(ParamIndexEntry));
        return TRUE;
    }
    return FALSE;
}

void freeParamIndexTable(ParamIndexEntry *table, SINT32 tableSize)
{
    SINT32 i;
    ParamIndexEntry *p, *q;

    for (i = 0; i < tableSize; i++)
    {
        p = table + i;
        while(p->next != NULL)
        {
            q = p->next;
            p->next = q->next;
            cmsMem_free(q->fullPath);
            cmsMem_free(q);
        }
        cmsMem_free(p->fullPath);
    }
    cmsMem_free(table);
}
