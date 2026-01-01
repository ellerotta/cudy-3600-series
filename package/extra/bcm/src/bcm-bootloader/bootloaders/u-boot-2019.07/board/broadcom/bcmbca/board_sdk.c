/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 Broadcom Ltd.
 */

#include <common.h>
#include <fdt_support.h>
#include <fdtdec.h>


#include <linux/ctype.h>
#include <mtd.h>
#include <stdlib.h>
#include <string.h>
#include <environment.h>
#include <cli.h>
#include "bca_common.h"
#include "bcm_bootstate.h"
#include "httpd/bcmbca_net.h"
#include "bca_sdk.h"
#include "bcm_secure.h"
#include "bcm_thermal.h"
#if defined(CONFIG_BCMBCA_OTP) 
#include "bcm_otp.h"
#endif
#include "bcm_strap_drv.h"
#if defined(CONFIG_SMC_BASED) && !defined(CONFIG_BCMBCA_NO_SMC_BOOT)
#include "ba_svc.h"
#endif
#if defined(CONFIG_WDT)
#include <wdt.h>
#endif

void bcmbca_xrdp_eth_init(void);
int board_init_flash_parts(int erase_img_part);
uint32_t env_boot_magic_search_size(void);

DECLARE_GLOBAL_DATA_PTR;

// Parse loaded FIT image and set values in linux fdt if required 
int bcm_board_boot_fdt_fixup_from_fit(void* fit_img_ptr)
{
	int rc = 0;
	/* Place holder for any future required actions */
	(void)fit_img_ptr;
	return rc;
}

#if defined(CONFIG_BCM_BOOTSTATE)
static void bcmbca_bootstate_reached_uboot(void)
{
/*
 * clearing boot resason in 138/148 also clears the DO_NOT_RESET_ON_WATCHDOG
 * which results reset status to get set to default 0x22ff value
 * which in terurn results wrong image to boot
 * */
#if defined(CONFIG_BCM63148) || defined(CONFIG_BCM63138)
    bcmbca_set_boot_reason(0);
#else
    bcmbca_clear_boot_reason();
#endif
#if defined(CONFIG_SMC_BASED) && !defined(CONFIG_BCMBCA_NO_SMC_BOOT)
    bcm_rpc_ba_report_boot_success(BA_SVC_RESET_BOOT_WDOG | BA_SVC_RESET_BOOT_COUNT);
#endif
    unregister_cli_job_cb(bcmbca_bootstate_reached_uboot);
}
#endif

#if defined(CONFIG_SYS_MTDPARTS_RUNTIME)

#define LINUX_NAND_MTD_ID		"brcmnand.0"
#define LINUX_SPINAND_MTD_ID	"spi1.0"
#define LINUX_SPINOR_MTD_ID		"spi-nor.0"

static char bcmbca_def_mtd_ids[128] = {0};
static char bcmbca_def_mtd_parts[512] = {0};
static char bcmbca_linux_mtd_ids[128] = {0};

void board_mtdparts_default(const char **mtdids, const char **mtdparts)
{
	struct mtd_info *mtd = bcmbca_get_image_mtd_device();
	int bdev;
	
	if (!IS_ERR_OR_NULL(mtd)) {
		bdev = bcm_get_boot_device();
		/* check driver name to see if it is spi nand or parallel nand */
		switch(bdev) {
		case BOOT_DEVICE_NAND:
			sprintf(bcmbca_def_mtd_ids, "%s=%s", mtd->name, LINUX_NAND_MTD_ID);
			sprintf(bcmbca_def_mtd_parts, "%s:%lld(loader)", 
				LINUX_NAND_MTD_ID, (long long)env_boot_magic_search_size());
			sprintf(bcmbca_linux_mtd_ids, "%s", LINUX_NAND_MTD_ID);
			break;
		case BOOT_DEVICE_SPI:
			sprintf(bcmbca_def_mtd_ids, "%s=%s", mtd->name, LINUX_SPINAND_MTD_ID);
			sprintf(bcmbca_def_mtd_parts, "%s:%lld(loader)", 
				LINUX_SPINAND_MTD_ID, (long long)env_boot_magic_search_size());
			sprintf(bcmbca_linux_mtd_ids, "%s", LINUX_SPINAND_MTD_ID);
			break;
		case BOOT_DEVICE_NOR:
			sprintf(bcmbca_def_mtd_ids, "%s=%s", mtd->name, LINUX_SPINOR_MTD_ID);
			sprintf(bcmbca_def_mtd_parts, "%s:%lld(loader)", 
				LINUX_SPINOR_MTD_ID, (long long)env_boot_magic_search_size());
			break;
		default:
			break;
		}

		put_mtd_device(mtd);
	}

	*mtdids = bcmbca_def_mtd_ids;
	*mtdparts = bcmbca_def_mtd_parts;
}
#endif

#ifdef CONFIG_BCMBCA_UPDATE_MCB_IN_ENV
#include "spl_ddrinit.h"

static int update_memcfg(void)
{

	if (env_get("boardid") == NULL)
		printf("ERROR: boardid is not defined in uboot environment \nPlease use uboot command \'setenv boardid [board name]\' to set boardid\n");
	else
	{
		const uint32_t* memcfg;
		uint32_t fdt_mcb, env_mcb;
		int offset = fdt_path_offset(gd->fdt_blob, "/memory_controller");
		if (offset < 0)
		{
			printf("Can't find /memory_controller node in board Device Tree\n");
			return -1;
		}
		memcfg = fdt_getprop(gd->fdt_blob, offset, "memcfg", NULL);
		if (memcfg == NULL)
		{
			printf("Can't find memcfg parameter in Device Tree\n");
			return -1;
		}
		fdt_mcb = be32_to_cpu(*memcfg);
		env_mcb = env_get_hex("MCB", 0);

		if (!env_mcb || ((fdt_mcb!=env_mcb) && !(env_mcb&BP_DDR_CONFIG_OVERRIDE)))
		{
			printf("Updating MCB environment from 0x%x to 0x%x\n", env_mcb, fdt_mcb);
			env_set_hex("MCB", fdt_mcb);
			env_save();
			printf("Memory Configuration Changed -- REBOOT NEEDED\n");
		}
	}

	return 0;
}
#endif

int board_sdk_late_init_e(void)
{
	bcm_sec_init();
	bcm_sec_cb_arg_t cb_args[SEC_CTRL_ARG_MAX] = {0};
	cb_args[SEC_CTRL_ARG_KEY].arg[0].ctrl = SEC_CTRL_KEY_GET;
	cb_args[SEC_CTRL_ARG_KEY].arg[1].ctrl = SEC_CTRL_KEY_CHAIN_RSA;
	cb_args[SEC_CTRL_ARG_KEY].arg[1].ctrl_arg = (void*)gd->fdt_blob;
	cb_args[SEC_CTRL_ARG_KEY].arg[2].ctrl = SEC_CTRL_KEY_CHAIN_AES;
	cb_args[SEC_CTRL_ARG_KEY].arg[2].ctrl_arg = (void*)gd->fdt_blob;
	bcm_sec_do(SEC_SET, cb_args);
#if 0 
	bcm_sec_key_arg_t* _keys;
	bcm_sec_get_active_aes_key(&_keys);
	if (_keys) {
		int i;
		for (i = 0; i < _keys->len; i++ ) {
			printf("Got key %s %llx %llx %llx %llx \n",_keys->aes[i].id, 
						((u64*)_keys->aes[i].key)[0],
						((u64*)_keys->aes[i].key)[1],
						((u64*)_keys->aes[i].key)[2],
						((u64*)_keys->aes[i].key)[3]);
		}

	}
#endif

	return 0;
}

int board_sdk_init_e(void)
{
#if defined(CONFIG_BCMBCA_OTP) 	
	int rc=bcm_otp_init();

	if (rc) {
		hang();
	}
	return rc;
#else
	return 0;
#endif	
}

int board_sdk_late_init_l(void)
{
	char *cp;
	int node, len;

#ifdef CONFIG_BCMBCA_UPDATE_MCB_IN_ENV
	update_memcfg();
#endif

#ifdef CONFIG_BCMBCA_HTTPD
	if(!httpd_check_net_env())
		register_cli_job_cb(0, http_poll);
#endif

#if defined(CONFIG_BCM_BOOTSTATE)
	register_cli_job_cb(0, bcmbca_bootstate_reached_uboot);
#endif

	node = fdt_path_offset(gd->fdt_blob, "/chosen");
	if (node < 0) {
		printf("Can't find /chosen node in cboot DTB\n");
		return node;
	}
	cp = (char *)fdt_getprop(gd->fdt_blob, node, "boot_device", &len);
	if (cp) { 
		printf("boot_device is %s\n",cp);
	}

	board_init_flash_parts(0);

	return(0);
}

int board_init_flash_parts(int erase_img_part)
{
	int ret = -1;
#if defined(CONFIG_CMD_MTDPARTS) || defined(CONFIG_CMD_GPT)
	char *cp;
	char *media = NULL;
	int n = 0;
	cp = env_get("IMAGE");
	char *bdptr = env_get("BDINFO");
	if (NULL != cp)
	{
		unsigned long iargs[4];
		char units[4];
		unsigned long bdargs[4];
		char bdunit[4];
		int bdnum = 0;
		n =  parse_env_string_plus_nums(cp, &media, 4, iargs, units);
		bdnum =  parse_env_string_plus_nums(bdptr, &media, 4, bdargs, bdunit);
#ifdef CONFIG_CMD_MTDPARTS
		if( strcasecmp(media, FLASH_DEV_STR_NAND) == 0 ) {
			char *mparts;
			int ncommas = 0;
			mparts = env_get("mtdparts");
			while (mparts && (*mparts != '\0')) {
				/* count the commas in mtdparts */
				if (*mparts == ',') {
					ncommas++;
				}
				mparts++;
			}
			/* only update mtdparts from IMAGE if IMAGE is specified and mtdparts has 0 or 1  
			 * commas -- either 1 or 2 devices defined */
			if ((ncommas < 2 ) && ((n == 3) || (n == 2))) {
				char cmd[100];
				struct mtd_info *mtd;
				long long image_start;
				long long image_max;
				long long image_end = 0;

				long long bd_start;
				long long bd_end = 0;

				bd_start = ((long long)bdargs[0]) << suffix2shift(bdunit[0])  ;
				if ( bdnum == 3) {
					bd_end = ((long long)bdargs[1]) << suffix2shift(bdunit[1])  ;
				}

				image_start = ((long long)iargs[0]) << suffix2shift(units[0])  ;
				if ( n == 3) {
					image_end = ((long long)iargs[1]) << suffix2shift(units[1])  ;
				}
				mtd_probe_devices();
				mtd = bcmbca_get_image_mtd_device();
				if (IS_ERR_OR_NULL(mtd))
				{
					printf("cant get mtd nand device\n");
				}
				image_max = mtd->size - 8 * mtd->erasesize;
				put_mtd_device(mtd);
				if ((n == 2)  || (image_end < 1) || (image_end > image_max)) {
					image_end = image_max;
					printf("adjusted to skip last 8 blocks\n");
				}

				/* Initialize mtd parts */
				printf("image in %s from %lld to %lld\n",media,image_start,image_end);
				run_command("mtdparts delall",0);

				/* Set key mtd env variables */
				env_set("mtdids", bcmbca_def_mtd_ids);
				sprintf(cmd, "%s:%lld(loader),%lld@%lld(bdinfo),%lld@%lld(image)",
					bcmbca_linux_mtd_ids,
					bd_start,
					bd_end-bd_start,
					bd_start,
					image_end-image_start,
					image_start);
				env_set("mtdparts", cmd);
				run_command("mtdparts",0);

				/* erase image partition if required */
				if (erase_img_part) {
					printf("WARNING: Erasing image partition!\n");
					sprintf(cmd,"mtd erase image");
					run_command(cmd,0);
				}
				ret = 0;
			}
		}
#endif /* CONFIG_CMD_MTDPARTS */		

#ifdef CONFIG_CMD_GPT
		if( strcasecmp(media, FLASH_DEV_STR_EMMC) == 0 ) {
			/* Setup default partitions */
			char * partitions = NULL;
			if( run_command("env exists default_partitions",0) == 0 ) {
				run_command("part list mmc 0 curr_parts",0);
				partitions=env_get("curr_parts");
				if(partitions != NULL) {
					if( (strlen(partitions) <= 1) || erase_img_part) {
						if( erase_img_part )
							printf("WARNING: Reformatting eMMC image partitions!\n");
						else						
							printf("Bootstrap Image Detected!, Setting default eMMC partitions!\n");

						run_command("gpt write mmc 0 $default_partitions", 0);
						run_command("gpt verify mmc 0 $default_partitions", 0);
					}
					ret = 0;
				}
			}
		}
#endif /* CONFIG_CMD_GPT */		
		free(media);
	}
#endif
	return ret;
}

/* All operations which must be done RIGHT BEFORE Linux 
 * is launched need to be put into this function. 
 */
void board_preboot_os( void )
{
#if defined(CONFIG_BCM_THERMAL)
	if (!bcm_thermal_is_temperature_safe())
	{
		bcmbca_set_boot_reason(BCM_BOOT_REASON_ACTIVATE);
		run_command("reset", 0);
	}
#endif

#ifdef CONFIG_BCMBCA_XRDP_ETH
	bcmbca_xrdp_eth_uninit();
#endif

#if defined(CONFIG_BCM_BOOTSTATE)
	if(!((bcmbca_get_boot_reason() >> BCM_RESET_REASON_BITS) & BCM_BOOT_REASON_ACTIVATE))
	{
		if( ((bcmbca_get_boot_reason() & BCM_BOOT_PHASE_MASK)) == BCM_BOOT_PHASE_FB_UBOOT )
		{
			printf("reset reason is set to BCM_BOOT_PHASE_FB_UBOOT by tpl, setting now to BCM_BOOT_PHASE_FB_LINUX_START\n");
			bcmbca_set_boot_reason(BCM_BOOT_REASON_WATCHDOG | BCM_BOOT_PHASE_FB_LINUX_START | (bcmbca_get_boot_reason() & 0xffffff00));
		}
		else
		{
			printf("reset reason is set to BCM_BOOT_PHASE_UBOOT by tpl, setting now to BCM_BOOT_PHASE_LINUX_START\n");
			bcmbca_set_boot_reason(BCM_BOOT_REASON_WATCHDOG | BCM_BOOT_PHASE_LINUX_START | (bcmbca_get_boot_reason() & 0xffffff00));
		}
	}
#endif

#if defined(CONFIG_WDT)
	/* Force reset the watchdog */
	if (gd->watchdog_dev)
		wdt_reset(gd->watchdog_dev);
#endif


}

