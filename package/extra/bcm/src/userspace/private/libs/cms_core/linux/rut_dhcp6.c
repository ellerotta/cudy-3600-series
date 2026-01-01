/***********************************************************************
 *
 *  Copyright (c) 2017  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2017:proprietary:standard

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

/*
 * rut_dhcp6.c
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */

#ifdef SUPPORT_IPV6

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_dhcp6.h"
#include <arpa/inet.h>

CmsRet rutDhcp6_createOption17(PKTCBL_WAN_TYPE wanType, const char *ifName)
{
    uint16_t code = DHCP_V6_VDR_SPECIFIC_INFO;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;

    buffPtr = &optionFrame[VDR_OPTION_CODE_OFFSET];
    *(uint16_t *)buffPtr = htons(code);
    buffPtr += 2;
    totalLen += 2;
    
    /* skip option len first */
    buffPtr += 2;
    totalLen += 2;

    if ((WAN_PKTCBL_EMTA != wanType) && (WAN_EPON_EPTA != wanType))
    {
        cmsLog_error("unknown wanType %d", wanType);
        return CMSRET_INTERNAL_ERROR;
    }

    if (CMSRET_SUCCESS != cmsDhcp_constructOption17Value(wanType, NULL, buffPtr, &dataLen))
    {
        cmsLog_error("fail");
        return CMSRET_INTERNAL_ERROR;
    }


    buffPtr = &optionFrame[VDR_OPTION_V6_LEN_OFFSET]; 
    *(uint16_t *)buffPtr = htons(dataLen);
    
    totalLen += dataLen;
    return cmsDhcp_saveOption(DHCP_V6, ifName, code, optionFrame, totalLen);
}  

#if defined(BRCM_PKTCBL_SUPPORT)
CmsRet rutDhcp6_getAcsUrlFromOption17(const char *ifName, char *acsURL, int inLen)
{
    uint16_t code = DHCP_V6_VDR_SPECIFIC_INFO;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    char subOptionData[CMS_MAX_ACS_URL_LENGTH] = {0};
    int subDataLen = 0;
    uint32_t enterprise_num = 0;
    char urlType;

    if (!ifName || !acsURL)
    {
        cmsLog_error("invalid params");
        return CMSRET_INVALID_ARGUMENTS;
    }

    totalLen = sizeof(optionFrame);
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V6, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }

    buffPtr = &optionFrame[2];
    dataLen = ntohs(*(uint16_t *)buffPtr) - 4; /* ' -4' for removing size of enterprise number */

    /* check enterprise_num */
    buffPtr = &optionFrame[4];
    enterprise_num = ntohl(*(uint32_t *)buffPtr);
    buffPtr += 4;

    /* check enterprise_num */
    if (DHCP_ENTERPRISE_NUMBER_CTL != enterprise_num)
    {        
        cmsLog_error("unknow enterprise number %d", enterprise_num);
        return CMSRET_INVALID_ARGUMENTS;
    }

    subDataLen = sizeof(subOptionData);
    if (CMSRET_SUCCESS != cmsDhcp_getSubOptionData(DHCP_V6, buffPtr, dataLen, 
                    DHCP6_OPTION17_SUBOPTION40, subOptionData, &subDataLen))
    {
        cmsLog_error("get sub-option%d fail", DHCP6_OPTION17_SUBOPTION40);
        return CMSRET_INVALID_ARGUMENTS;
    }

    urlType = subOptionData[0];
    if ((17 == subDataLen) && (1 == urlType)) /* subDataLen == 17 && type == IPv6 address */
    {
        inet_ntop(AF_INET6, (void *)&subOptionData[1], acsURL, inLen);
    }
    else if (0 == urlType) /* type == FQDN */
    {
        int fqdnLen = subDataLen - 1; /* '-1' for removing size of 'type' */
        strncpy(acsURL, &subOptionData[1], 
                (fqdnLen < inLen) ? fqdnLen : inLen);
    }
    else
    {
        cmsLog_error("unknown url type %d", urlType);
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_debug("acsURL=%s", acsURL);
    return CMSRET_SUCCESS;
}

CmsRet rutDhcp6_getNtpserversFromOption56(const char *ifName, char *ntpServerList, int inLen)
{
    uint16_t code = DHCP_V6_VDR_NTP_SERVER;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    char subOptionData[CMS_MAX_ACS_URL_LENGTH] = {0};
    int subDataLen = 0;
    int subCode;
    UBOOL8 found = FALSE;

    if (!ifName || !ntpServerList)
    {
        cmsLog_error("invalid params");
        return CMSRET_INVALID_ARGUMENTS;
    }

    totalLen = sizeof(optionFrame);
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V6, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }

    buffPtr = &optionFrame[2];
    dataLen = ntohs(*(uint16_t *)buffPtr);
    buffPtr += 2;

    for (subCode = 1; subCode <= 3; subCode++)
    {
        subDataLen = sizeof(subOptionData);
        memset(subOptionData, 0, subDataLen);
        if (CMSRET_SUCCESS != cmsDhcp_getSubOptionData(DHCP_V6, buffPtr, dataLen, 
                        subCode, subOptionData, &subDataLen))
        {
            cmsLog_notice("get sub-option%d fail", subCode);
            continue;
        }

        if ((DHCP6_OPTION56_SUBOPTION1 == subCode) || (DHCP6_OPTION56_SUBOPTION2 == subCode))
        {
            if (subDataLen != 16)
            {
                cmsLog_error("subCode=%d: invalid subDataLen=%d", subCode, subDataLen);
                return CMSRET_INVALID_ARGUMENTS;
            }

            inet_ntop(AF_INET6, (void *)&subOptionData[0], ntpServerList, inLen);
            found = TRUE;
            break;
        }
        else if ((DHCP6_OPTION56_SUBOPTION3 == subCode))
        {
            strncpy(ntpServerList, subOptionData, (subDataLen < inLen) ? subDataLen : inLen);
            found = TRUE;
            break;
        }      
    }

    if (!found)
    {
        cmsLog_error("not found");
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_debug("ntpServerList=%s", ntpServerList);
    return CMSRET_SUCCESS;
}

#endif // BRCM_PKTCBL_SUPPORT

#endif // SUPPORT_IPV6
