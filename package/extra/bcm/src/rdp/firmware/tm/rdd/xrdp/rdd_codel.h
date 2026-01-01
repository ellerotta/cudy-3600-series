/*
* <:copyright-BRCM:2014:proprietary:standard
*
*    Copyright (c) 2014 Broadcom
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


#ifndef _RDD_CODEL_H
#define _RDD_CODEL_H

#include "rdd.h"

/* interval [1] */
#define  CODEL_BIAS_0   0xc800     /* 51200 */
#define  CODEL_SLOPE_0  0x0000     /* 0     */
/* interval [2-3] */
#define  CODEL_BIAS_1   0xc151     /* 49489 */
#define  CODEL_SLOPE_1  0x19f3     /* 6643  */
/* interval [4-7] */
#define  CODEL_BIAS_2   0x833c     /* 33596 */
#define  CODEL_SLOPE_2  0x081a     /* 2074  */
/* interval [8-15] */
#define  CODEL_BIAS_3   0x5ac4     /* 23236 */
#define  CODEL_SLOPE_3  0x02af     /* 687   */
/* interval [16-31] */
#define  CODEL_BIAS_4   0x3f76     /* 16246 */
#define  CODEL_SLOPE_4  0x00eb     /* 235   */
/* interval [32-63] */
#define  CODEL_BIAS_5   0x2c9e     /* 11422 */
#define  CODEL_SLOPE_5  0x0052     /* 82    */
/* interval [64-127] */
#define  CODEL_BIAS_6   0x1f76     /* 8054  */
#define  CODEL_SLOPE_6  0x001d     /* 29    */
/* interval [128-255] */
#define  CODEL_BIAS_7   0x1636     /* 5686  */
#define  CODEL_SLOPE_7  0x000a     /* 10    */
/* interval [256-511] */
#define  CODEL_BIAS_8   0x0fb2     /* 4018  */
#define  CODEL_SLOPE_8  0x0004     /* 4     */
/* interval [512-1023] */
#define  CODEL_BIAS_9   0x0b18     /* 2840  */
#define  CODEL_SLOPE_9  0x0001     /* 1     */
/* interval > 1024 */
#define  CODEL_BIAS_10  0x07D8     /* 2008  */
#define  CODEL_SLOPE_10 0x0001     /* 0.4   */

#if defined(XRDP_CODEL)

bdmf_error_t rdd_codel_enable(uint16_t queue_index);
bdmf_error_t rdd_codel_disable(uint16_t queue_index);
bdmf_error_t rdd_codel_queue_qd_get(uint16_t queue_index, uint16_t *p_window_ts, uint16_t *p_drop_interval);

#endif

#endif /* _RDD_CODEL_H */
