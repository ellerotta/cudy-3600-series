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
 * rut_dhcp.c
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */



#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_dhcp.h"
#include <arpa/inet.h>


CmsRet rutDhcp_createOption43(PKTCBL_WAN_TYPE wanType, const char *ifName, const char *duid)
{
    char code = DHCP_VDR_SPECIFIC_INFO;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;

    if ((WAN_PKTCBL_EMTA != wanType) && (WAN_EPON_EPTA != wanType))
    {
        cmsLog_error("unknown wanType %d", wanType);
        return CMSRET_INTERNAL_ERROR;
    }

    buffPtr = &optionFrame[VDR_OPTION_CODE_OFFSET];
    *buffPtr = code;
    buffPtr += 1;
    totalLen += 1;

    /* skip option len first */
    buffPtr += 1;
    totalLen += 1;

    if (CMSRET_SUCCESS != cmsDhcp_constructOption43Value(wanType, duid, buffPtr, &dataLen))
    {
        cmsLog_error("fail");
        return CMSRET_INTERNAL_ERROR;
    }

    optionFrame[VDR_OPTION_LEN_OFFSET] = dataLen;  
    totalLen += dataLen;

    return cmsDhcp_saveOption(DHCP_V4, ifName, code, optionFrame, totalLen);
}  

CmsRet rutDhcp_createOption125(const char *ifName)
{
    char code = DHCP_VDR_VI_VENDOR;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;

    buffPtr = &optionFrame[VDR_OPTION_CODE_OFFSET];
    *buffPtr = code;
    buffPtr += 1;
    totalLen += 1;
    
    /* skip option len first */
    buffPtr += 1;
    totalLen += 1;

    if (CMSRET_SUCCESS != cmsDhcp_constructOption125Value(buffPtr, &dataLen))
    {
        cmsLog_error("fail");
        return CMSRET_INTERNAL_ERROR;
    }
    optionFrame[VDR_OPTION_LEN_OFFSET] = dataLen;  

    totalLen += dataLen;
    return cmsDhcp_saveOption(DHCP_V4, ifName, code, optionFrame, totalLen);
}

#if defined(BRCM_PKTCBL_SUPPORT)
CmsRet rutDhcp_createOption60(const char *ifName)
{
    char code = DHCP_VDR;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int dataLen = 0;
    int totalLen = 0;

    buffPtr = &optionFrame[VDR_OPTION_CODE_OFFSET];
    *buffPtr = code;
    buffPtr += 1;
    totalLen += 1;
    
    /* skip option len first */
    buffPtr += 1;
    totalLen += 1;

    if (CMSRET_SUCCESS != cmsDhcp_constructOption60Value(buffPtr, &dataLen))
    {
        cmsLog_error("cmsDhcp_constructOption60Value failed!");
        return CMSRET_INTERNAL_ERROR;
    }

    optionFrame[VDR_OPTION_LEN_OFFSET] = dataLen;  
    totalLen += dataLen;

    return cmsDhcp_saveOption(DHCP_V4, ifName, code, optionFrame, totalLen);
}

CmsRet rutDhcp_getAcsUrlFromOption125(const char *ifName, char *acsURL, int inLen)
{
    char code = DHCP_VDR_VI_VENDOR;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLenN = 0;
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
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V4, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }
    
    buffPtr = &optionFrame[2];
    while (buffPtr < (optionFrame + totalLen))
    {
        enterprise_num = ntohl(*(uint32_t *)buffPtr);
        buffPtr += 4;
        dataLenN = *buffPtr;
        buffPtr += 1;

        /* check enterprise_num */
        if (DHCP_ENTERPRISE_NUMBER_CTL != enterprise_num)
        {
            cmsLog_notice("unknow enterprise number %d", enterprise_num);
            buffPtr += dataLenN;
            continue;
        }

        subDataLen = sizeof(subOptionData);
        if (CMSRET_SUCCESS != cmsDhcp_getSubOptionData(DHCP_V4, buffPtr, dataLenN, 
                            DHCP4_OPTION125_SUBOPTION6, subOptionData, &subDataLen))
        {
            cmsLog_error("get sub-option%d fail", DHCP4_OPTION125_SUBOPTION6);
            return CMSRET_INVALID_ARGUMENTS;
        }

        urlType = subOptionData[0];
        if ((5 == subDataLen) && (1 == urlType)) /* subDataLen == 5 && type == IPv4 address */
        {
            inet_ntop(AF_INET, (void *)&subOptionData[1], acsURL, inLen);
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

        break;
    }

    cmsLog_debug("acsURL=%s", acsURL);
    return CMSRET_SUCCESS;
}

CmsRet rutDhcp_getNtpserversFromOption42(const char *ifName, char *ntpServerList, int inLen)
{
    char code = DHCP_VDR_NTP_SERVERS;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    char *dstPtr = ntpServerList;
    int serverNum = 0;

    if (!ifName || !ntpServerList)
    {
        cmsLog_error("invalid params");
        return CMSRET_INVALID_ARGUMENTS;
    }

    totalLen = sizeof(optionFrame);
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V4, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }

    /* validate dataLen */
    dataLen = optionFrame[1];
    if ((dataLen < 4) || (dataLen%4 != 0))
    {        
        cmsLog_error("invalid dataLen=%d", dataLen);
        return CMSRET_INVALID_ARGUMENTS;
    }

    buffPtr = &optionFrame[2];
    ntpServerList[0] = '\0';
    while (buffPtr < (optionFrame + totalLen))
    {
        inet_ntop(AF_INET, (void *)buffPtr, dstPtr, inLen - strlen(ntpServerList));
        buffPtr += 4;
        dstPtr  += strlen(ntpServerList);

        serverNum++;
        if (serverNum > 5)
        {        
            cmsLog_notice("allows up to 5 NTP servers, ignore excess ones");
            break;
        }

        dstPtr += sprintf(dstPtr, "%s", ","); /* start next one */
    }

    cmsLog_debug("ntpServerList=%s", ntpServerList);
    return CMSRET_SUCCESS;
}


#endif // BRCM_PKTCBL_SUPPORT

