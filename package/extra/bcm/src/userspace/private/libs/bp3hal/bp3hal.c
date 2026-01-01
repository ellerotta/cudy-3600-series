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

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <linux/limits.h>
#include <errno.h>
#include <bp3_license.h>

#include <bp3user.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int __bp3_drv_fd = 0;

/**
 * BP3_SEC_HAL_Join()
 *
 * Application should call this HAL function before other 
 *	BP3_SEC_HAL_xxx() functions
 * 
 * Return: 0 on success, otherwise errno-base.h
 * 
 */
int BP3_SEC_HAL_Join(void)
{
	int rc = 0;

	if (__bp3_drv_fd)
		return (EBUSY);

        __bp3_drv_fd = open("/dev/bp3", O_RDWR);
        if (__bp3_drv_fd < 0) {
                rc = errno;
        }

	return rc;
}

/**
 * BP3_SEC_HAL_Leave()
 *
 * Application should call this HAL function to finalize the BP3 Driver usage.
 * 
 * Return: 0
 * 
 */
int BP3_SEC_HAL_Leave(void)
{
	if (__bp3_drv_fd) {
		close(__bp3_drv_fd);
		__bp3_drv_fd = 0;
	}

	return 0;
}

/**
 * BP3_SEC_HAL_GetSessionToken()
 * @session: Pointer to Session Token buffer.
 * @session_size: Size of the buffer.
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetSessionToken(__u8 **session, __u32 *session_size)
{
	int rc = 0;
	struct bp3_sec_hal_io_get_session_token_params *param;

	if (!__bp3_drv_fd || !session_size) {
		return (EINVAL);
	}

	*session_size = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_get_session_token_params));
	if (!param) {
		return (ENOMEM);
	}

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_GET_SESSION_TOKEN, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}
	
	*session = (__u8 *)malloc(param->session_size);
	if (*session) {
		memcpy((void *)*session, (void *)param->session, param->session_size);
		*session_size = param->session_size;
	} else {
		rc = (ENOMEM);
	}

failure:

	free(param);

	return rc;
}

static int __BP3_SEC_HAL_GetChipInfo(__u8 *feature_list, 
			    __u32 feature_list_size, 
			    __u32 *prod_id, 
			    __u32 *security_code, 
			    __u32 *bond_option, 
			    _Bool *provisioned,
			    __u8 **chip_sn, 
			    __u32 *chip_sn_size)
{
	int rc = 0;
	struct bp3_sec_hal_io_get_chip_info_params *param;

	if (!__bp3_drv_fd || !feature_list || !prod_id || !security_code || !bond_option || !provisioned) {
		return (EINVAL);
	}

	*prod_id = 0;
	*security_code = 0;
	*bond_option = 0;
	*provisioned = 0;

	if (chip_sn_size) {
		*chip_sn_size = 0;
	}

	param = malloc(sizeof(struct bp3_sec_hal_io_get_chip_info_params));
	if (!param) {
		return (ENOMEM);
	}

	if (feature_list_size > sizeof(param->feature_list)) {
		rc = (EINVAL);
		goto failure;
	}

	param->feature_list_size = feature_list_size;

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_GET_CHIP_INFO, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	memcpy((void *)feature_list, (void *)param->feature_list, feature_list_size);
	*prod_id = param->prod_id;
	*security_code = param->security_code;
	*bond_option = param->bond_option;
	*provisioned = (_Bool)param->provisioned;

	if (chip_sn && chip_sn_size && param->chip_sn_size) {
		*chip_sn = (__u8 *)malloc(param->chip_sn_size);
		if (*chip_sn) {
			memcpy((void *)*chip_sn, (void *)param->chip_sn, param->chip_sn_size);
			*chip_sn_size = param->chip_sn_size;
		} else {
			rc = (ENOMEM);
		}
	}

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_GetChipInfo()
 * @feature_list: Pointer to Feature List buffer.
 * @feature_list_size: Size of the buffer.
 * @prod_id: Product Identification Code
 * @security_code: Security Code
 * @bond_option: Bond Option
 * @provisioned: Is provisioned
 * 
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetChipInfo(__u8 *feature_list, 
			    __u32 feature_list_size, 
			    __u32 *prod_id, 
			    __u32 *security_code, 
			    __u32 *bond_option, 
			    _Bool *provisioned)
{
	return __BP3_SEC_HAL_GetChipInfo(feature_list, feature_list_size, 
		prod_id, security_code, bond_option, provisioned, 0, 0);
}

/**
 * BP3_SEC_HAL_GetChipInfoEx()
 * @feature_list: Pointer to Feature List buffer.
 * @feature_list_size: Size of the buffer.
 * @prod_id: Product Identification Code
 * @security_code: Security Code
 * @bond_option: Bond Option
 * @provisioned: Is provisioned
 * @chip_sn: Pointer to Chip Serial Number buffer
 * @chip_sn_size: Size of the buffer
 * 
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetChipInfoEx(__u8 *feature_list, 
			    __u32 feature_list_size, 
			    __u32 *prod_id, 
			    __u32 *security_code, 
			    __u32 *bond_option, 
			    _Bool *provisioned,
			    __u8 **chip_sn, 
			    __u32 *chip_sn_size)
{
	return __BP3_SEC_HAL_GetChipInfo(feature_list, feature_list_size, 
		prod_id, security_code, bond_option, provisioned, chip_sn, chip_sn_size);
}

/**
 * BP3_SEC_HAL_GetProvisioningBlob()
 * @provisioning_blob: Completion Message buffer.
 * @provisioning_blob_size: Size of the Completion Message buffer.
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetProvisioningBlob(__u8 **provisioning_blob, 
				    __u32 *provisioning_blob_size)
{
	int rc = 0;
	struct bp3_sec_hal_io_get_provisioning_blob_params *param;

	if (!__bp3_drv_fd || !provisioning_blob_size) {
		return (EINVAL);
	}

	*provisioning_blob_size = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_get_provisioning_blob_params));
	if (!param) {
		return (ENOMEM);
	}

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_GET_PROVISIONING_BLOB, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	*provisioning_blob = (__u8 *)malloc(param->provisioning_blob_size);
	if (*provisioning_blob) {
		memcpy((void *)*provisioning_blob, (void *)param->provisioning_blob, param->provisioning_blob_size);
		*provisioning_blob_size = param->provisioning_blob_size;
	} else {
		rc = (ENOMEM);
	}

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_ProvisionBp3()
 * @ccf: Configuration Container File buffer.
 * @ccf_size: Size of the Configuration Container File buffer.
 * @log: Completion Message buffer.
 * @log_size: Size of the Completion Message buffer.
 * @status: Status Array
 * @status_size: Size of the Status Array
 * @bin: License binary blob
 * @bin_size: Size of the License binary blob
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_ProvisionBp3(__u8 *ccf, 
			     __u32 ccf_size, 
			     __u8 **log, 
			     __u32 *log_size, 
			     __u32 **status, 
			     __u32 *status_size,
			     __u8 **bin, 
			     __u32 *bin_size)
{
	int rc = 0;
	struct bp3_sec_hal_io_provision_bp3_params *param;

	if (!__bp3_drv_fd || !ccf || !log_size || !status_size) {
		return (EINVAL);
	}

	*log_size = 0;
	*status_size = 0;
	if (bin_size) {
		*bin_size = 0;
	}

	param = malloc(sizeof(struct bp3_sec_hal_io_provision_bp3_params));
	if (!param) {
		return (ENOMEM);
	}

	if (ccf_size > sizeof(param->ccf)) {
		rc = (EINVAL);
		goto failure;
	}

	memcpy((void *)param->ccf, (void *)ccf, ccf_size);
	param->ccf_size = ccf_size;

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_PROVISION_BP3, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	*status = (__u32 *)malloc(param->status_size * sizeof((*status)[0]));
	if (*status) {
		memcpy((void *)*status, (void *)param->status, param->status_size * sizeof((*status)[0]));
		*status_size = param->status_size;
	} else {
		rc = (ENOMEM);
	}

	*log = (__u8 *)malloc(param->log_size);
	if (*log) {
		memcpy((void *)*log, (void *)param->log, param->log_size);
		*log_size = param->log_size;
	} else {
		rc = (ENOMEM);
	}

	if (bin && bin_size) {
		*bin = (__u8 *)malloc(param->bin_size);
		if (*bin) {
			memcpy((void *)*bin, (void *)param->bin, param->bin_size);
			*bin_size = param->bin_size;
		} else {
			rc = (ENOMEM);
		}
	}

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_ProvisionBP3Pak()
 * @file_name: PAK file name.
 * @provision_status: PAK provisioning status.
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_ProvisionBP3Pak(char *file_name, __u32 *provision_status)
{
	int rc = 0;
	struct bp3_sec_hal_io_provision_bp3_pak_params *param;

	if (!__bp3_drv_fd || !provision_status || !file_name) {
		return (EINVAL);
	}

	if ((NAME_MAX - 1) < strlen(file_name)) {
		return (EINVAL);
	}

	*provision_status = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_provision_bp3_pak_params));
	if (!param) {
		return (ENOMEM);
	}

	strncpy(param->file_name, file_name, NAME_MAX);

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_PROVISION_BP3_PAK, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	*provision_status = param->provision_status;

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_GetOtpId()
 * @otp_id_high: 
 * @otp_id_low: 
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetOtpId(__u32 *otp_id_high, __u32 *otp_id_low)
{
	*otp_id_high = 0;
	*otp_id_low = 0;

	return 0;
}

/**
 * BP3_SEC_HAL_CommitBp3Provision()
 * @ccf: 
 * @ccf_size: 
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_CommitBp3Provision(__u8 *ccf, __u32 ccf_size)
{
	(void)ccf;
	(void)ccf_size;

	return 0;
}

/**
 * BP3_SEC_HAL_IsFeatureEnabled()
 * @feature: 
 * @enabled: 
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_IsFeatureEnabled(__s32 feature, __u32 *enabled)
{
	int rc = 0;
	struct bp3_sec_hal_io_is_feature_enabled_params *param;

	if (!__bp3_drv_fd || !enabled) {
		return (EINVAL);
	}

	*enabled = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_is_feature_enabled_params));
	if (!param) {
		return (ENOMEM);
	}

	param->feature = feature;
	param->status0 = 0;
	param->status1 = 0;

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_IS_FEATURE_ENABLED, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	*enabled = param->status0;

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_GetFeaturesStatus()
 * @bp3_features: 
 * @feature_list_size: 
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetFeaturesStatus(__u8 **feature_list, __u32 *feature_list_size)
{
	int rc = 0;
	struct bp3_sec_hal_io_get_features_status_params *param;

	if (!__bp3_drv_fd || !feature_list_size) {
		return (EINVAL);
	}

	*feature_list_size = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_get_features_status_params));
	if (!param) {
		return (ENOMEM);
	}

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_GET_FEATURES_STATUS, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}


	*feature_list = (__u8 *)malloc(param->feature_list_size);
	if (*feature_list) {
		memcpy((void *)*feature_list, (void *)param->feature_list, param->feature_list_size);
		*feature_list_size = param->feature_list_size;
	} else {
		rc = (ENOMEM);
	}

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_GetProductType()
 * @platform_name: Pointer to Product Name buffer.
 * @platform_name_size: Size of the buffer.
 * @protocol_name: Pointer to Protocol Name buffer.
 * @protocol_name_size: Size of the buffer.
 * 
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_GetProductType(char *platform_name, 
                               __u32 platform_name_size, 
                               char *protocol_name, 
                               __u32 protocol_name_size)
{
	int rc = 0;
	struct bp3_sec_hal_io_get_product_type_params *param;

	if (!__bp3_drv_fd || !platform_name || !protocol_name) {
		return (EINVAL);
	}

	platform_name[0] = 0;
	protocol_name[0] = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_get_product_type_params));
	if (!param) {
		return (ENOMEM);
	}

	if ((platform_name_size < sizeof(param->platform_name)) || 
	    (protocol_name_size < sizeof(param->protocol_name))) {
		rc = (EINVAL);
		goto failure;
	}

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_GET_PRODUCT_TYPE, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	memcpy((void *)platform_name, (void *)param->platform_name, sizeof(param->platform_name));
	memcpy((void *)protocol_name, (void *)param->protocol_name, sizeof(param->protocol_name));

failure:

	free(param);

	return rc;
}

/**
 * BP3_SEC_HAL_RestoreBp3bin()
 * @file_name: BP3.bin file name.
 * @status: BP3.bin restore status.
 *
 * Description
 * 
 * Return: 0 on success, otherwise errno-base.h
 * Negative errno-base.h values on driver's internal errors
 * 
 */
int BP3_SEC_HAL_RestoreBp3bin(char *file_name, __u32 *status)
{
	int rc = 0;
	struct bp3_sec_hal_io_restore_bp3_bin_params *param;

	if (!__bp3_drv_fd || !status || !file_name) {
		return (EINVAL);
	}

	if ((NAME_MAX - 1) < strlen(file_name)) {
		return (EINVAL);
	}

	*status = 0;

	param = malloc(sizeof(struct bp3_sec_hal_io_restore_bp3_bin_params));
	if (!param) {
		return (ENOMEM);
	}

	strncpy(param->file_name, file_name, NAME_MAX);

	rc = ioctl(__bp3_drv_fd, BP3_IOCTL_BP3_SEC_HAL_RESTORE_BP3_BIN, param);
	if (rc) {
		rc = (-errno);
		goto failure;
	}

	*status = param->status;

failure:

	free(param);

	return rc;
}
