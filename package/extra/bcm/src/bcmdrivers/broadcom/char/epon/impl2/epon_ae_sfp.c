/*
*  Copyright 2021, Broadcom Corporation
*
* <:copyright-BRCM:2021:proprietary:standard
* 
*    Copyright (c) 2021 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/
#include "EponTypes.h"
#include "wan_drv.h"
#include "mac_drv.h"
#include "Laser.h"
#include "mac_drv_ae.h"
#include "phy_drv_int_ae.h"
#include "AeDriver.h"
#include "opticaldet.h"
#include "trxbus.h"
#include <pmc_wan.h>

mac_speed_t laser2mac[LaserRate10G+1] = {MAC_SPEED_UNKNOWN, MAC_SPEED_1000, MAC_SPEED_2500, MAC_SPEED_10000};

static SUPPORTED_WAN_TYPES_BITMAP sfp_trx_compliance_to_wan_type_bm(long trx_compliance)
{
    SUPPORTED_WAN_TYPES_BITMAP wan_type_bm = 0;
    
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "trx compliance map 0x%x\n", trx_compliance);
    
    if (trx_compliance & BCM_SFF8472_CC_10GBASE_R)
        wan_type_bm |= SUPPORTED_WAN_TYPES_BIT_AE_10_10;
    
    if (trx_compliance & BCM_SFF8472_CC_1GBASE_X)
        wan_type_bm |= SUPPORTED_WAN_TYPES_BIT_AE_1_1;
    
    return wan_type_bm;
}

static BOOL epon_ae_sfp_is_copper(void)
{
    long connector = 0;

    if (trxbus_mon_read(wantop_bus_get(), bcmsfp_mon_id_connector, 0, &connector))
        return FALSE;

    if (connector == SFF8024_CONNECTOR_COPPER_PIGTAIL 
        || connector == SFF8024_CONNECTOR_RJ45)
        return TRUE;
    else
        return FALSE;
}

static SUPPORTED_WAN_TYPES_BITMAP epon_ae_sfp_get_wan_type_bm(SUPPORTED_WAN_TYPES_BITMAP *wan_type_bm)
{
    long trx_compliance = BCM_TRX_COMPLIANCE_UNKNOWN;
    TRX_TYPE trx_type;
    int rc = 0;
    
    rc = trx_get_type(wantop_bus_get(), &trx_type);
    if (rc == OPTICALDET_NOSFP)
        return -1;
    
    if (trx_type == TRX_TYPE_UNKNOWN)
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_AE, "unknown trx type\n");
        if (epon_ae_sfp_is_copper())
        {
            BCM_LOG_DEBUG(BCM_LOG_ID_AE, "unknown copper sfp skip detect\n");
            *wan_type_bm = 0;
        }
        else
        {
            trxbus_mon_read(wantop_bus_get(), bcmsfp_mon_trx_compliance, 0, &trx_compliance);
            
            BCM_LOG_DEBUG(BCM_LOG_ID_AE, "optical sfp trx compliance 0x%x\n", trx_compliance);
            *wan_type_bm =  sfp_trx_compliance_to_wan_type_bm(trx_compliance);
        }
    }
    else
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_AE, "known trx type\n");
        trx_get_supported_wan_type_bm(wantop_bus_get(), wan_type_bm);
    }
    
    return 0;
}

static LaserRate wan_type_bm_try_laser_rate(SUPPORTED_WAN_TYPES_BITMAP *wan_type_bm)
{
    LaserRate rate = LaserRateOff;
    
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "wan type bm 0x%x\n", *wan_type_bm);
    
    if (*wan_type_bm & SUPPORTED_WAN_TYPES_BIT_AE_10_10)
    {
        rate = LaserRate10G;
        *wan_type_bm = *wan_type_bm & ~SUPPORTED_WAN_TYPES_BIT_AE_10_10;
    }
    else if (*wan_type_bm & SUPPORTED_WAN_TYPES_BIT_AE_2_2)
    {
        rate = LaserRate2G;
        *wan_type_bm = *wan_type_bm & ~SUPPORTED_WAN_TYPES_BIT_AE_2_2;
    }
    else if (*wan_type_bm & SUPPORTED_WAN_TYPES_BIT_AE_1_1)
    {
        rate = LaserRate1G;
        *wan_type_bm = *wan_type_bm & ~SUPPORTED_WAN_TYPES_BIT_AE_1_1;
    }
    
    return rate;
}

static WAN_INTF epon_ae_rate_to_wan_interface(LaserRate rate)
{
    if (rate == LaserRate10G)
        return WAN_INTF_10GEPON;
    
    return WAN_INTF_EPON;
}

#define SERDES_AUTO_DETECT_INT_MS 200
int epon_ae_mac_rate_detect(LaserRate cur_rate)
{
    LaserRate sfp_rate;
    mac_cfg_t mac_cfg;
    SUPPORTED_WAN_TYPES_BITMAP wan_type_detect_bm = 0;

    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "Detecting EPON AE rate\n");
    if (epon_ae_sfp_get_wan_type_bm(&wan_type_detect_bm))
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_AE, "sfp info get fail\n");
        return -1;
    }
    
    sfp_rate = wan_type_bm_try_laser_rate(&wan_type_detect_bm);
    while (sfp_rate != LaserRateOff)
    {
        if (pmc_wan_interface_power_control(epon_ae_rate_to_wan_interface(sfp_rate), 1))
        {
            BCM_LOG_ERROR(BCM_LOG_ID_AE, "wan interface power on fail, sfp_rate=%d\n", sfp_rate);
            return -1;
        }

        mac_cfg.speed = laser2mac[sfp_rate];
        epon_ae_mac_cfg_set(NULL, &mac_cfg);
        msleep(SERDES_AUTO_DETECT_INT_MS);
        if (epon_ae_mac_locked(sfp_rate))
            return 0;
        
        if (pmc_wan_interface_power_control(epon_ae_rate_to_wan_interface(sfp_rate), 0))
        {
            BCM_LOG_ERROR(BCM_LOG_ID_AE, "wan interface power off fail, sfp_rate=%d\n", sfp_rate);
        }
        
        sfp_rate = wan_type_bm_try_laser_rate(&wan_type_detect_bm);
    }
    
    return 0;
}
