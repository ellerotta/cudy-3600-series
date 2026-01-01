/*
* <:copyright-BRCM:2023:proprietary:standard
* 
*    Copyright (c) 2023 Broadcom 
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

/**
 * @file pon_pwm_sm.h
 * @author Yoel Tulatov (yoel.tulatov@broadcom.com)
 * @brief Inter space common gpon power management definitions
 * @date 2023-05-07
 */

#ifndef _PON_PWM_SM_H_
#define _PON_PWM_SM_H_

typedef enum
{
    PON_PWM_STATE_STANDBY,
    PON_PWM_STATE_ACTIVE_HELD,
    PON_PWM_STATE_ACTIVE_FREE,
    PON_PWM_STATE_AWARE,
    PON_PWM_STATE_LOW_POWER,
    PON_PWM_STATE_NUM
}
PON_PWM_STATE;

typedef enum
{
    PON_PWM_EVENT_NONE,
    PON_PWM_EVENT_FWI,
    PON_PWM_EVENT_SA_OFF,
    PON_PWM_EVENT_SA_ON,
    PON_PWM_EVENT_LWI, 
    PON_PWM_EVENT_PON_ACTIVE,
    PON_PWM_EVENT_LSI,
    PON_PWM_EVENT_LDI,
    PON_PWM_EVENT_LPI,
    PON_PWM_EVENT_TLOWPOWER,
    PON_PWM_EVENT_TAWARE,
    PON_PWM_EVENT_THOLD,
    PON_PWM_EVENT_TTRANS_TTX_INIT,
    PON_PWM_EVENTS_NUM
} 
PON_PWM_EVENTS;

typedef enum
{
    PON_PWM_SR_AWAKE,
    PON_PWM_SR_DOZE,
    PON_PWM_SR_SLEEP,
    PON_PWM_SR_WSLEEP,
}
PON_PWM_SLEEP_REQUEST;

typedef enum
{
    PON_PWM_LPWR_CYCLE,
    PON_PWM_LPWR_AWAKE,
    PON_PWM_LPWR_DEACT
}
PON_PWM_LOW_POWER_EXIT_REASON;


/* Powr management mode */

#define PON_PWM_LOW_POWER_MODE_BIT_DOZE           0x1
#define PON_PWM_LOW_POWER_MODE_BIT_SLEEP          0x2
#define PON_PWM_LOW_POWER_MODE_BIT_WSLEEP         0x4

#define PON_PWM_LOW_POWER_MODE_CAP                (PON_PWM_LOW_POWER_MODE_BIT_WSLEEP)

typedef enum 
{
    PON_PWM_LOCAL_WAKE_INDICATOR  = 0x00,
    PON_PWM_LOCAL_DOSE_INDICATOR  = PON_PWM_LOW_POWER_MODE_BIT_DOZE,
    PON_PWM_LOCAL_SLEEP_INDICATOR = PON_PWM_LOW_POWER_MODE_BIT_SLEEP,
    PON_PWM_LOCAL_LOW_POWER_INDICATOR = PON_PWM_LOW_POWER_MODE_BIT_WSLEEP,
}
PON_PWM_LOCAL_INDICATORS;

#endif /* _PON_PWM_SM_H_ */