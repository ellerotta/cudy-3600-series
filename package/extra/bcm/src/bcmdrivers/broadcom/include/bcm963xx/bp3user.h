/***********************************************************************
 *
 *
<:copyright-BRCM:2022:proprietary:standard

   Copyright (c) 2022 Broadcom 
   All Rights Reserved

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

#ifndef __BP3_USER_H
#define __BP3_USER_H

#include <linux/types.h>
#include <linux/limits.h>

#define PROVISIONING_SESSION_TOKEN_BYTES 16
#define FEATURE_LIST_BITS 256
#define CHIP_SERIAL_NUMBER_BYTES 16

struct bp3_sec_hal_io_get_session_token_params {
	__u8 session[PROVISIONING_SESSION_TOKEN_BYTES];
	__u32 session_size;
} __attribute__((__packed__));

struct bp3_sec_hal_io_get_chip_info_params {
	__u8 feature_list[FEATURE_LIST_BITS / 8];
	__u32 feature_list_size;
	__u32 prod_id;
	__u32 security_code;
	__u32 bond_option;
	__u8 provisioned;
	__u8 chip_sn[CHIP_SERIAL_NUMBER_BYTES];
	__u32 chip_sn_size;
} __attribute__((__packed__));

#define PROVISIONING_BLOB_SIZE 512 

struct bp3_sec_hal_io_get_provisioning_blob_params {
	__u8 provisioning_blob[PROVISIONING_BLOB_SIZE];
	__u32 provisioning_blob_size;
} __attribute__((__packed__));

#define CCF_SIZE 2048
#define LOG_SIZE 512
#define STATUS_SIZE 32
#define BIN_SIZE 4096

struct bp3_sec_hal_io_provision_bp3_params {
	__u8 ccf[CCF_SIZE];
	__u32 ccf_size;
	__u8 log[LOG_SIZE];
	__u32 log_size;
	__u32 status[STATUS_SIZE];
	__u32 status_size;
	__u8 bin[BIN_SIZE];
	__u32 bin_size;
} __attribute__((__packed__));

struct bp3_sec_hal_io_provision_bp3_pak_params {
        __u8 file_name[NAME_MAX];
	__u32 provision_status;
} __attribute__((__packed__));

struct bp3_sec_hal_io_is_feature_enabled_params {
        __s32 feature;
        __u32 status0;
        __u32 status1;
} __attribute__((__packed__));

struct bp3_sec_hal_io_get_features_status_params {
        __u8 feature_list[FEATURE_LIST_BITS / 8];
        __u32 feature_list_size;
} __attribute__((__packed__));

struct bp3_sec_hal_io_get_product_type_params {
        __u8 platform_name[NAME_MAX];
        __u8 protocol_name[NAME_MAX];
} __attribute__((__packed__));

struct bp3_sec_hal_io_restore_bp3_bin_params {
        __u8 file_name[NAME_MAX];
	__u32 status;
} __attribute__((__packed__));

#define BP3_IOCTL_BASE 0xACDC0000

#define BP3_IOCTL_BP3_SEC_HAL_GET_SESSION_TOKEN      (BP3_IOCTL_BASE | 'a')
#define BP3_IOCTL_BP3_SEC_HAL_GET_CHIP_INFO          (BP3_IOCTL_BASE | 'b')
#define BP3_IOCTL_BP3_SEC_HAL_GET_PROVISIONING_BLOB  (BP3_IOCTL_BASE | 'c')
#define BP3_IOCTL_BP3_SEC_HAL_PROVISION_BP3          (BP3_IOCTL_BASE | 'd')
#define BP3_IOCTL_BP3_SEC_HAL_PROVISION_BP3_PAK      (BP3_IOCTL_BASE | 'e')
#define BP3_IOCTL_BP3_SEC_HAL_IS_FEATURE_ENABLED     (BP3_IOCTL_BASE | 'f')
#define BP3_IOCTL_BP3_SEC_HAL_GET_FEATURES_STATUS    (BP3_IOCTL_BASE | 'g')
#define BP3_IOCTL_BP3_SEC_HAL_GET_PRODUCT_TYPE       (BP3_IOCTL_BASE | 'h')
#define BP3_IOCTL_BP3_SEC_HAL_RESTORE_BP3_BIN        (BP3_IOCTL_BASE | 'i')

#endif /* __BP3_USER_H */
