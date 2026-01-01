/***********************************************************************
 *
 *  Copyright (c) 2017  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2017:DUAL/GPL:standard
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 * 
 ************************************************************************/

/*
 * dhcp_common.c
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */
 

/*
 * the function in this file is used for both DHCPv4 and DHCPv6
 */


#include "cms_util.h"
#include "dhcp_config_key.h"
#include "bcm_boardutils.h"
#include "bcm_boarddriverctl.h"
#include "bcm_retcodes.h"

#define UNUSED_PARA(x) (void)(x)

typedef struct {
    char *duid;
    unsigned char wanType;
}OptionInParms;

#define WAN_PKTCBL_EDVA 0xFF

/* For debug */
static void show_Option(const char *option, int len) 
{
    int i;

    for (i = 0; i < len; i++) 
    {
        printf("%02x ", option[i]);
        if ((i + 1) % 16 == 0) 
        {
            printf("\n");
        }
    }
    printf("\n\n");
}

CmsRet cmsDhcp_mkCfgDir(DhcpVersion dhcpVer, const char *ifName)
{
    char cmd[BRCM_UDHCPC_CONFIG_FILE_NAME_LEN + 16];
    
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/%s", 
        (DHCP_V4 == dhcpVer) ? BRCM_UDHCPC_CONFIG_DIR : BRCM_UDHCP6C_CONFIG_DIR, ifName);

    if(system(cmd) < 0 )
    {
        cmsLog_error("cmd %s failed", cmd);
    }

    return CMSRET_SUCCESS;
}

CmsRet cmsDhcp_readOption(DhcpVersion dhcpVer, const char *ifName, int code, char *option, int *len)
{
    char file[BRCM_UDHCPC_CONFIG_FILE_NAME_LEN] = {0};
    char buffer[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    FILE *in;
    int rlen, cpLen, vlen = 0;
    CmsRet ret = CMSRET_SUCCESS;

    if (NULL == ifName || NULL == option || NULL == len)
    {
        cmsLog_error("param invalid!");
        return CMSRET_INVALID_ARGUMENTS;
    }

    /* Full name example: /var/udhcpc/veip0.1/option122.out */
    snprintf(file, sizeof(file), "%s/%s/%s%d%s", 
                   (DHCP_V4 == dhcpVer) ? BRCM_UDHCPC_CONFIG_DIR : BRCM_UDHCP6C_CONFIG_DIR, ifName, 
                    BRCM_UDHCPC_CONFIG_FILE_BASE_NAME, code,
                    BRCM_UDHCPC_CONFIG_OUT_FILE_SUFFIX); /* output file from dhcp */
    if (!(in = fopen(file, "r")))
    {
        cmsLog_error("unable to open option file: %s", file);
        return CMSRET_OPEN_FILE_ERROR;
    }

    rlen = fread(buffer, 1, VDR_MAX_DHCP_OPTION_LEN, in);

    /* decrypt */
    dhcpEncryptCfgFile(buffer, rlen, BRCM_DHCP_CONFIG_KEY);

    if (DHCP_V4 == dhcpVer)
        vlen = buffer[VDR_OPTION_LEN_OFFSET] + VDR_OPTION_SUBCODE_OFFSET;
    else if (DHCP_V6 == dhcpVer)
    {
        buffPtr = &buffer[VDR_OPTION_V6_LEN_OFFSET];
        vlen = ntohs(*((uint16_t *)buffPtr)) + VDR_OPTION_V6_SUBCODE_OFFSET;
    }

    /* verify */
    if (rlen != vlen)
    {
        cmsLog_error("Invalid option file: %s", file);
        ret = CMSRET_INTERNAL_ERROR;
        goto exit;
    }

    if(cmsLog_getLevel() == LOG_LEVEL_DEBUG) 
    {
        cmsLog_debug("Option%d:\n", code);
        show_Option(buffer, rlen);
    }

    cpLen = rlen < (*len) ? rlen : (*len);
    memcpy(option, buffer, cpLen);
    *len = cpLen;

exit:
    fclose(in);
    return ret;
}

CmsRet cmsDhcp_saveOption(DhcpVersion dhcpVer, const char *ifName, int code, const char *option, int len)
{
    char file[BRCM_UDHCPC_CONFIG_FILE_NAME_LEN] = {0};
    char buffer[VDR_MAX_DHCP_OPTION_LEN] = {0};
    FILE *out;
    CmsRet ret = CMSRET_SUCCESS;
    unsigned int cnt = 1;
    int wLen = len < VDR_MAX_DHCP_OPTION_LEN ? len : VDR_MAX_DHCP_OPTION_LEN;

    if(cmsLog_getLevel() == LOG_LEVEL_DEBUG)
    {
        cmsLog_debug("Option%d:\n", code);
        show_Option(option, len);
    }

    /* encrypt */
    memcpy(buffer, option, wLen);
    dhcpEncryptCfgFile(buffer, wLen, BRCM_DHCP_CONFIG_KEY);

    /* Full name example: /var/udhcpc/veip0.1/option43.in */
    snprintf(file, sizeof(file), "%s/%s/%s%d%s", 
                   (DHCP_V4 == dhcpVer) ? BRCM_UDHCPC_CONFIG_DIR : BRCM_UDHCP6C_CONFIG_DIR, ifName, 
                    BRCM_UDHCPC_CONFIG_FILE_BASE_NAME, code,
                    BRCM_UDHCPC_CONFIG_IN_FILE_SUFFIX); /* input file for dhcp */
    if (!(out = fopen(file, "w"))) 
    {
        cmsLog_error("unable to open option file: %s", file);
        return CMSRET_OPEN_FILE_ERROR;
    }

    if (fwrite(buffer, wLen, cnt, out) != cnt)
    {
        cmsLog_error("write option file: %s fail", file);
        ret = CMSRET_INTERNAL_ERROR;
        goto exit;
    }

exit:
    fclose(out);
    return ret;
}

CmsRet cmsDhcp_encapsulateSubOption(uint16_t code, DhcpSubOptionTable *subOptTable, 
                  int subOptTableLen, const void *generalParm, char* optData, int *dataLen,
                  const DhcpOptionCodeLen codeLen, const DhcpOptionSizeLen sizeLen)
{
    char *dataPtr, *subCodeLenPtr, *valPtr;
    uint16_t sub_code;
    int subLen = 0, totalLen = 0, cnt = 0;
    int loop, value, i;
    int need;
    char type;
    char valNew[VDR_MAX_DHCP_SUB_OPTION_LEN] = {0};
    CmsRet ret = CMSRET_SUCCESS;

    dataPtr = optData;
    for(loop = 0; loop < subOptTableLen; loop++)
    {
        need = 1;
        if (NULL != subOptTable[loop].needFn)
            need = subOptTable[loop].needFn(generalParm);
        if (!need)
            continue;

        sub_code = subOptTable[loop].subCode;
        type = subOptTable[loop].type; 
        if (OPTION_CODE_LEN1  == codeLen)
            *dataPtr++ = sub_code;
        else if (OPTION_CODE_LEN2  == codeLen)
        {
            *((uint16_t *)dataPtr) = htons(sub_code);
            dataPtr += 2;
        }
        else
        {
            cmsLog_error("Unsupported codeLen=%d\n", codeLen);
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }

        if (OPTION_SIZE_LEN1 == sizeLen)
            subCodeLenPtr = dataPtr++;
        else if (OPTION_SIZE_LEN2 == sizeLen)
        {
            subCodeLenPtr = dataPtr;
            dataPtr += 2;
        }
        else
        {
            cmsLog_error("Unsupported sizeLen=%d\n", sizeLen);
            ret = CMSRET_INVALID_ARGUMENTS;
            goto exit;
        } 

        if (NULL == subOptTable[loop].valFn)
            valPtr = subOptTable[loop].valDef;
        else
        {
            uint16_t len = sizeof(valNew);
            memset(valNew, 0, len);
            if (!subOptTable[loop].valFn(generalParm, valNew, &len))
            {
                valPtr = valNew;
            }
            else
            {
                cmsLog_error("option %d, subOption %d: update value fail!", code, sub_code);
                valPtr = subOptTable[loop].valDef;
            }
        }         
        
        if (OPTION_CHAR_STRING == type)
        {
            subLen = strlen(valPtr);
            strncpy(dataPtr, valPtr, subLen);
            dataPtr += subLen;
        }
        else if(OPTION_HEX_STRING == type) 
        {
            cnt = 0;
            while ( sscanf(valPtr, "%2x%n", &value, &i) == 1 )
            {
                *dataPtr++ = value;
                valPtr += i;
                cnt++;
            }
            subLen = cnt;
        }
        else
        {
            cmsLog_error("Unsupported type %d\n", type);
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }

        if (OPTION_SIZE_LEN1 == sizeLen)            
            *subCodeLenPtr = subLen;
        else if (OPTION_SIZE_LEN2 == sizeLen)
            *(uint16_t *)subCodeLenPtr = htons(subLen);        
        
        totalLen += codeLen + sizeLen + subLen;
    }

    *dataLen = totalLen;

exit:
    if (ret)
    {
        cmsLog_error("Encapsulate fail! code = %d\n", code);
    }
    return ret;    
}

 
CmsRet cmsDhcp_getSubOptionData(DhcpVersion dhcpVer, const char *optionData, int dataLen,
                                    int subCode, char *subOptionData, int *subDataLen)
{
    const char *ptr = optionData;
    uint16_t subCodeTmp = 0, subLenTmp = 0;

    if (!optionData || !subOptionData || !subDataLen)
    {
        cmsLog_error("param invalid\n");
        return CMSRET_INVALID_ARGUMENTS;
    }

    while (ptr < (optionData + dataLen))
    {
        if (DHCP_V4 == dhcpVer)
        {
            subCodeTmp = *ptr;
            ptr += 1;
            subLenTmp = *ptr;
            ptr += 1;
        }
        else /* DHCPv6 */
        {
            subCodeTmp = ntohs(*(uint16_t *)ptr);
            ptr += 2;
            subLenTmp = ntohs(*(uint16_t *)ptr);
            ptr += 2;
        }

        if (subCodeTmp == subCode)
        {
            memcpy(subOptionData, ptr, (subLenTmp < (*subDataLen)) ? subLenTmp : (*subDataLen));
            *subDataLen = subLenTmp;
            return CMSRET_SUCCESS;
        }

        ptr += subLenTmp;
    }

   return CMSRET_METHOD_NOT_SUPPORTED; /* not found the given sub-option */
}


static inline int option_getOro(const void* parm, char* string, uint16_t* len)
{
#define EMTA_ORO "002000210022087b0028" /* option 32, 33, 34, 2171, 40 */
#define EPTA_ORO "0028"                 /* option 40 */

    unsigned char wanType;
    char *pValue = NULL;

    wanType = ((OptionInParms *)parm)->wanType;

    if ((WAN_PKTCBL_EMTA == wanType) ||(WAN_PKTCBL_EDVA == wanType))
        pValue = EMTA_ORO;
    else if (WAN_EPON_EPTA == wanType)
        pValue = EPTA_ORO;
    else
        return BCMRET_METHOD_NOT_SUPPORTED;

    if (*len <= strlen(pValue))
        return BCMRET_RESOURCE_EXCEEDED;

    strncpy(string, pValue, *len);

    return BCMRET_SUCCESS;
}

static inline int option_getDevType(const void* parm, char* string, uint16_t* len)
{
#define EMTA_STR "EMTA"
#define EPTA_STR "EPTA"
#define EDVA_STR "EDVA"

    unsigned char wanType;
    char *pValue = NULL;

    wanType = ((OptionInParms *)parm)->wanType;

    if (WAN_PKTCBL_EMTA == wanType)
        pValue = EMTA_STR;
    else if (WAN_EPON_EPTA == wanType)
        pValue = EPTA_STR;
    else if (WAN_PKTCBL_EDVA == wanType)
        pValue = EDVA_STR;
    else
        return BCMRET_METHOD_NOT_SUPPORTED;

    if (*len <= strlen(pValue))
        return BCMRET_RESOURCE_EXCEEDED;

    strncpy(string, pValue, *len);

    return BCMRET_SUCCESS;
}

static inline int option_getSN(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    return bcmUtl_getSerialNumber(string, *len);
}

static inline int option_getHwV(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    return bcmUtl_getHardwareVersion(string, *len);
}

static inline int option_getSwV(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    return bcmUtl_getSoftwareVersion(string, *len);
}

static inline int option_getBtV(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    return bcmUtl_getBootloaderVersion(string, *len);
}

static inline int option_getOUI(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    char macAddrBuf[32]={0};
    int i;
    BcmRet ret = BCMRET_SUCCESS;

    ret = bcmUtl_getBaseMacAddress(macAddrBuf);
    if (ret != BCMRET_SUCCESS)
        return ret;

    if (*len < 7)
        return BCMRET_RESOURCE_EXCEEDED;

    for (i = 0; i < 8; i++)
    {
        if (macAddrBuf[i] == ':')
            continue;

        if ((macAddrBuf[i] >= 'A') && (macAddrBuf[i] <= 'F'))
            *(string++) = macAddrBuf[i] + 32;
        else
            *(string++) = macAddrBuf[i];
    }
    return ret;
}

static inline int option_getModelNum(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    BcmRet ret = BCMRET_SUCCESS;

    ret = bcmUtl_getModelName(string, *len);
    if (ret != BCMRET_SUCCESS)
    {
        cmsLog_error("Could not get BoardId, ret=%d", ret);
    }

    return ret;
}

static inline int option_getVdrName(const void* parm, char* string, uint16_t* len)
{
    UNUSED_PARA(parm);
    return bcmUtl_getManufacturer(string, *len);
}

/* Please refer to DHCPv4: option43_getMacAddr() */
static inline int option17_getMacAddr(const void* parm, char* string, uint16_t* len)
{
    /* actually the SN in DeviceInfo is MacAddr, so get from SN directly */
    return option_getSN(parm, string, len);
}

static inline int option43_getMacAddr(const void* parm, char* string, uint16_t* len)
{
    /* actually the SN in DeviceInfo is MacAddr, so get from SN directly */
    return option_getSN(parm, string, len);
}                 


static DhcpSubOptionTable option43_subOptions[] = 
{
    { 2,   NULL, OPTION_CHAR_STRING, "dev_type",        "EPTA",               option_getDevType },
    { 4,   NULL, OPTION_CHAR_STRING, "sn",              "001018b0ff00",       option_getSN },
    { 5,   NULL, OPTION_CHAR_STRING, "hw_ver",          "V1.0",               option_getHwV },
    { 6,   NULL, OPTION_CHAR_STRING, "sw_ver",          "5.02L.04",           option_getSwV },
    { 7,   NULL, OPTION_CHAR_STRING, "bootroom_ver",    "CFE=1.0",            option_getBtV },
    { 8,   NULL, OPTION_CHAR_STRING, "OUI",             "001018",             option_getOUI },
    { 9,   NULL, OPTION_CHAR_STRING, "model_num",       "968580XREF",         option_getModelNum },
    { 10,  NULL, OPTION_CHAR_STRING, "vendor_name",     "Broadcom",           option_getVdrName },
    { 31,  NULL, OPTION_HEX_STRING,  "mac",             "001018b0ff00",       option43_getMacAddr },
    { 32,  NULL, OPTION_HEX_STRING,  "id_correlation",  "1DA82FF9",           NULL }
};

CmsRet cmsDhcp_constructOption43Value(PKTCBL_WAN_TYPE wanType, const char* duid, char* optionValue, int* optionLen)
{
    int dataLen = 0;
    OptionInParms parm;

    if ((WAN_PKTCBL_EMTA != wanType) && (WAN_EPON_EPTA != wanType))
    {
        cmsLog_error("unknown wanType %d", wanType);
        return CMSRET_INTERNAL_ERROR;
    }

    parm.duid = (char *)duid;
    parm.wanType = wanType;

    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(DHCP_VDR_SPECIFIC_INFO, option43_subOptions, 
        sizeof(option43_subOptions)/sizeof(DhcpSubOptionTable), (void *)&parm, 
        optionValue, &dataLen,
        OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        return CMSRET_INTERNAL_ERROR;
    }

    *optionLen = dataLen;
    return CMSRET_SUCCESS;
}  


static int option60_getEndPointNum(const void* parm, char* string, uint16_t* len)
{
    /* TODO:  hard code tmp */
    int cpyLen = 2;
    UNUSED_PARA(parm);

    memcpy(string, "02", cpyLen+1);
    *len = cpyLen;

    return 0;
}

static int option60_getIfIndex(const void * parm, char* string, uint16_t* len)
{
    /* TODO:  hard code tmp */
    int cpyLen = 2;
    UNUSED_PARA(parm);

    memcpy(string, "09", cpyLen+1);
    *len = cpyLen;

    return 0;
}

/* For SubOption descripton, Pls refer to "PacketCable 1.5 Specification":  "PKT-SP-PROV1.5-I04-090624.doc" */
static DhcpSubOptionTable option60_subOptions[] = {
    { 1,    NULL,   OPTION_HEX_STRING,     "",      "02",             NULL },
    { 2,    NULL,   OPTION_HEX_STRING,     "",      "02",             option60_getEndPointNum },
    { 3,    NULL,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 4,    NULL,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 9,    NULL,   OPTION_HEX_STRING,     "",      "01" ,            NULL },
    { 11,   NULL,   OPTION_HEX_STRING,     "",      "06090f",         NULL },
    { 12,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 13,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 15,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 16,   NULL,   OPTION_HEX_STRING,     "",      "09",             option60_getIfIndex },
    { 18,   NULL,   OPTION_HEX_STRING,     "",      "0007",           NULL },
    { 19,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 20,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 21,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 22,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 23,   NULL,   OPTION_HEX_STRING,     "",      "02003f",         NULL },
    { 24,   NULL,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 25,   NULL,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 26,   NULL,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 38,   NULL,   OPTION_HEX_STRING,     "",      "01",             NULL }
};

/* to share the descripter with DHCPv6 option17->subOption35 */
int get_dhcpV4Option60DesPtr(DhcpSubOptionTable **desPtr, int *TableLen)
{
    *desPtr = option60_subOptions;
    *TableLen = sizeof(option60_subOptions)/sizeof(DhcpSubOptionTable);
    return 0;
}

CmsRet cmsDhcp_constructOption60Value(char* optionValue, int* optionLen)
{
    /*pktc2.0:0541... */
    #define OPTION60_SUB_HEADER "pktc2.0:05"
    #define OPTION60_SUB_LEN_SIZE  2 //sub option size

    char code = DHCP_VDR;
    char dataFrame[VDR_MAX_DHCP_OPTION_LEN] = {0};
    int dataOffset, dataLen = 0;
    int subHeaderLen = strlen(OPTION60_SUB_HEADER);
    int i;

    dataOffset = subHeaderLen + OPTION60_SUB_LEN_SIZE;
    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option60_subOptions, 
        sizeof(option60_subOptions)/sizeof(DhcpSubOptionTable), NULL, 
        dataFrame, &dataLen,
        OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        return CMSRET_INTERNAL_ERROR;
    }

    memcpy(&optionValue[0], OPTION60_SUB_HEADER, subHeaderLen);

    /* Hex to Hex ASCII string */
    sprintf(&optionValue[subHeaderLen], "%02x", (char)dataLen);
    for (i = 0; i < dataLen && i < VDR_MAX_DHCP_OPTION_LEN; i++)
    {
        sprintf(&optionValue[dataOffset], "%02x", dataFrame[i]);
        dataOffset += 2;        
    }

    *optionLen = subHeaderLen + OPTION60_SUB_LEN_SIZE + dataLen*2;

    return CMSRET_SUCCESS;
}

static DhcpSubOptionTable option125_subOptions[] = {
    { 1,    NULL,    OPTION_HEX_STRING,  "oro",             "0206",   NULL }, /* option 2, 6 */
};

CmsRet cmsDhcp_constructOption125Value(char* optionValue, int* optionLen)
{
    char code = DHCP_VDR_VI_VENDOR;
    char *buffPtr = NULL;
    int dataLen = 0;
    uint32_t enterprise_num = DHCP_ENTERPRISE_NUMBER_CTL;

    buffPtr = optionValue;
    
    *(uint32_t *)buffPtr = htonl(enterprise_num);
    buffPtr += 4;

    /* skip data-len first */
    buffPtr += 1;

    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option125_subOptions, 
        sizeof(option125_subOptions)/sizeof(DhcpSubOptionTable), NULL, 
        buffPtr, &dataLen, OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        cmsLog_error("fail");
        return CMSRET_INTERNAL_ERROR;
    }

    optionValue[4] = dataLen;
    *optionLen = 4 + 1 + dataLen;

    return CMSRET_SUCCESS;
}


/* For option17->subOption35: TLV5, CL_OPTION_MODEM_CAPABILITIES. Pls refer to [CL-SP-CANN-DHCP-Reg-I14-170111.pdf] section 5.2.15 
** which has same format as DHCPv4 option60_subOptions.
*/
static DhcpSubOptionTable *option17_35_subOptions = NULL;

static int option17_35getTLV5(const void* parm, char* string, uint16_t* len)
{
    uint16_t SubCode = 35;
    int i, dataOffset;
    char dataFrame[VDR_MAX_DHCP_OPTION_LEN] = {0};
    int TableLen, dataLen = 0;

    UNUSED_PARA(parm);

    get_dhcpV4Option60DesPtr(&option17_35_subOptions, &TableLen);
    if(NULL == option17_35_subOptions)
    {
        cmsLog_error("get_dhcpV4Option60DesPtr failed!");
        return -1;
    }

    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(SubCode, option17_35_subOptions, 
        TableLen, NULL, dataFrame, &dataLen, 
        OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        return CMSRET_INTERNAL_ERROR;
    }

    /* Hex to Hex ASCII string */
    if ((dataLen * 2) >= (*len))
    {
        cmsLog_error("str too long!");
        return -1;
    }
    dataOffset = 0;
    for (i = 0; i < dataLen && i < VDR_MAX_DHCP_OPTION_LEN; i++)
    {
        sprintf(&string[dataOffset], "%02x", dataFrame[i]);
        dataOffset += 2;
    }
    *len = dataLen * 2;

    return 0;
}

static int isTlv5Required(const void *parm)
{
    PKTCBL_WAN_TYPE wanType;

    wanType = ((OptionInParms *)parm)->wanType;
    if (WAN_PKTCBL_EMTA == wanType)
        return 1;
    return 0;
}

static DhcpSubOptionTable option17_subOptions[] = {
    { 1,      NULL,              OPTION_HEX_STRING,  "oro",             "002000210022087b0028",   option_getOro }, /* option 32, 33, 34, 2171, 40 */
    { 2,      NULL,              OPTION_CHAR_STRING, "dev_type",        "EDVA",               option_getDevType },
    { 4,      NULL,              OPTION_CHAR_STRING, "sn",              "001018b0ff00",       option_getSN },
    { 5,      NULL,              OPTION_CHAR_STRING, "hw_ver",          "V1.0",               option_getHwV },
    { 6,      NULL,              OPTION_CHAR_STRING, "sw_ver",          "5.02L.04",           option_getSwV },
    { 7,      NULL,              OPTION_CHAR_STRING, "bootroom_ver",    "CFE=1.0",            option_getBtV },
    { 8,      NULL,              OPTION_CHAR_STRING, "OUI",             "001018",             option_getOUI },
    { 9,      NULL,              OPTION_CHAR_STRING, "model_num",       "968580XREF",         option_getModelNum },
    { 10,     NULL,              OPTION_CHAR_STRING, "vendor_name",     "Broadcom",           option_getVdrName },
    { 35,     isTlv5Required,    OPTION_HEX_STRING,  "TLV5",            "00",                 option17_35getTLV5 },
    { 36,     NULL,              OPTION_HEX_STRING,  "mac",             "001018b0ff00",       option17_getMacAddr },
    { 2172,   NULL,              OPTION_HEX_STRING,  "id_correlation",  "1DA82FF9",           NULL }
};

CmsRet cmsDhcp_constructOption17Value(PKTCBL_WAN_TYPE wanType, char* duid, char* optionValue, int* optionLen)
{
    int dataLen = 0;
    OptionInParms parm;
    uint32_t enterprise_num = DHCP_ENTERPRISE_NUMBER_CTL;
    char *buffPtr = NULL;

    if ((WAN_PKTCBL_EMTA != wanType) && (WAN_EPON_EPTA != wanType))
    {
        cmsLog_error("unknown wanType %d", wanType);
        return CMSRET_INTERNAL_ERROR;
    }

    parm.duid = (char *)duid;
    parm.wanType = (wanType == WAN_PKTCBL_EMTA)?WAN_PKTCBL_EDVA:WAN_EPON_EPTA;

    buffPtr = optionValue;
    *(uint32_t *)buffPtr = htonl(enterprise_num);
    buffPtr += 4;

    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(DHCP_V6_VDR_SPECIFIC_INFO, option17_subOptions, 
        sizeof(option17_subOptions)/sizeof(DhcpSubOptionTable), (void *)&parm, 
        buffPtr, &dataLen,
        OPTION_CODE_LEN2, OPTION_SIZE_LEN2))
    {
        return CMSRET_INTERNAL_ERROR;
    }

    *optionLen = dataLen + 4; /* 4 for enterprise_num */
    return CMSRET_SUCCESS;
}  
  

