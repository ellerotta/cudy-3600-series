/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 Broadcom Ltd.
 */

#include <common.h>
#include <fdtdec.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <spl.h>
#include <nand.h>
#include "spl_ddrinit.h"
#include <asm/arch/misc.h>
#include "boot_blob.h"
#include "boot_flash.h"
#include "tpl_params.h"
#include "spl_env.h"
#include "early_abort.h"
#include "bcm_secure.h"
#include "bca_common.h"
#if defined(CONFIG_BCMBCA_OTP) 
#include "bcm_otp.h"
#endif
#include "pmc_drv.h"
#if defined(CONFIG_SPL_BCMBCA_UBUS4)
#include "bcm_ubus4.h"
#endif
#include "bcm_strap_drv.h"

#include <wdt.h>
#include <uboot_aes.h>
DECLARE_GLOBAL_DATA_PTR;

tpl_params tplparams;

void spl_board_deinit(void);
extern void jump_to_image(uintptr_t entry, void* param, int clean_cache);

static void setup_tpl_parms(tpl_params *parms)
{
	parms->environment = NULL;
	/* tplparams.early_flags = boot_params; */
#if defined(CONFIG_BCMBCA_DDRC)
	parms->ddr_size = get_ddr_size();
#else
	parms->ddr_size = 64*1024*1024;
#endif
	parms->boot_device = (u8)bcm_get_boot_device();
	parms->environment = load_spl_env((void*)TPL_ENV_ADDR);
}
__weak int decrypt_tpl(void* img, u32 len)
{
#if defined(CONFIG_BCMBCA_DECRYPT_TPL)
	if (bcm_sec_state() == SEC_STATE_GEN3_MFG || 
		bcm_sec_state() == SEC_STATE_GEN3_FLD ) {
		u8* key = NULL; 
		u32 num_aes_blocks;
		u8 key_schedule[AES128_EXPAND_KEY_LENGTH];
		bcm_sec_cb_arg_t cb_args[SEC_CTRL_ARG_MAX] = {0};
		cb_args[SEC_CTRL_ARG_KEY].arg[0].ctrl = SEC_CTRL_KEY_GET;
		bcm_sec_do(SEC_SET, cb_args);
		bcm_sec_get_active_aes_key(&key); 
		if (!key) {
			return -1;
		}	
		printf("SPL: Decrypting TPL ...\n");
		aes_expand_key(key, AES128_KEY_LENGTH, key_schedule);
		num_aes_blocks = (len + AES128_KEY_LENGTH - 1) / AES128_KEY_LENGTH;
		aes_cbc_decrypt_blocks(AES128_KEY_LENGTH, key_schedule, 
			key + AES128_KEY_LENGTH, img, img, num_aes_blocks);
	}
#endif
	return 0;
}
/* spl load and start tpl. never return */
__weak void start_tpl(tpl_params *parms)
{
	typedef void __noreturn(*image_entry_t) (void *);
	image_entry_t image_entry =
		(image_entry_t) CONFIG_TPL_TEXT_BASE;
	void *new_params = (void*)TPL_PARAMS_ADDR;
	int size = CONFIG_TPL_MAX_SIZE;

	memcpy(new_params, parms, sizeof(tpl_params));

	if (load_boot_blob(TPL_TABLE_MAGIC, 0x0, (void *)CONFIG_TPL_TEXT_BASE,
		&size) == 0) {
		decrypt_tpl((void *)CONFIG_TPL_TEXT_BASE, size);
		spl_board_deinit();
		image_entry((void *)new_params);
	}

#if !defined(CONFIG_BCMBCA_IKOS) 
	bcm_sec_abort();		
#endif
}

__weak void arch_cpu_deinit(void)
{

}

int board_fit_config_name_match(const char *name)
{
	return 0;
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_NONE;
}

#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
int reserve_mmu(void)
{
#ifdef CONFIG_BCMBCA_PGTBL_IN_MEMC_SRAM
	bcmbca_enable_memc_sram(CONFIG_SYS_PAGETBL_BASE, CONFIG_SYS_PAGETBL_SIZE);
#endif
	gd->arch.tlb_addr = CONFIG_SYS_PAGETBL_BASE;
	gd->arch.tlb_size = CONFIG_SYS_PAGETBL_SIZE;

	return 0;
}
#endif

#ifdef CONFIG_BCMBCA_LDO_TRIM
static void bcmbca_set_ldo_trim(void)
{
	u32 trim = 0;

	bcm_otp_get_ldo_trim(&trim);
	if (trim) {
		printf("Apply trim code 0x%x reg 0x%x from otp to LDO controller...\n", 
			trim, (trim<<LDO_VREG_CTRL_TRIM_SHIFT)&LDO_VREG_CTRL_TRIM_MASK );
		TOPCTRL->LdoCtl &= ~LDO_VREG_CTRL_TRIM_MASK;	
		TOPCTRL->LdoCtl |=
			(trim<<LDO_VREG_CTRL_TRIM_SHIFT) & LDO_VREG_CTRL_TRIM_MASK;
	}
}
#endif

__weak void bcm_setsw(void)
{
}

__weak void reset_plls(void)
{
}

void board_init_f(ulong dummy)
{
#if defined(CONFIG_ARCH_CPU_INIT)
	arch_cpu_init();
#endif

#if defined(CONFIG_SYS_ARCH_TIMER)
	timer_init();
#endif
	if (spl_early_init())
#if !defined(CONFIG_BCMBCA_IKOS) 
		bcm_sec_abort();
#endif

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();
#if defined(PERF)
	printf("RevID: %X\n",PERF->RevID);
#endif

#if defined(CONFIG_BCMBCA_IKOS_SPL_JUMP_TO_UBOOT)
	jump_to_image((uintptr_t)CONFIG_SYS_TEXT_BASE, NULL, 0);
#endif

	bcm_strap_drv_reg();

#if  defined(CONFIG_BCMBCA_OTP)
	if (bcm_otp_init()) {
#if !defined(CONFIG_BCMBCA_IKOS) 
		bcm_sec_abort();
#endif
	}
#endif
	bcm_sec_init();

#if defined(BUILD_TAG)
	printf("$SPL: "BUILD_TAG" $\n");
#endif
	early_abort();

#ifdef CONFIG_BCMBCA_DDRC_WBF_EARLY_INIT
	bcm_ddrc_mc2_wbf_buffers_init();
#endif
#if defined(CONFIG_SPL_BCMBCA_UBUS4) &&\
	(defined(CONFIG_BCM63178) || defined(CONFIG_BCM47622))
	bcm_ubus_drv_init();
	/* force axi write reply to workaround wifi memory write ordering issue */
	ubus_master_cpu_enable_axi_write_cache(0);	
#endif		

#if defined(CONFIG_BCMBCA_PMC)
 	bcm_setsw();
#if defined(EMMC_RESET_PLL) || defined(CPU_RESET_PLL) 
	bcm_pmc_drv_reg();
 	reset_plls();
#endif
#endif

#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
	reserve_mmu();
	enable_caches();
#endif
}

void spl_board_deinit(void)
{
#if defined(CONFIG_BCMBCA_DISABLE_SECURE_VERIFY)
	/* Lock SOTP, wipe keys in SRAM and locally in .data */
	bcm_sec_deinit();		
#else
	/* Just wipe keys stored locally in .data */
	bcm_sec_clean_keys(bcm_sec());
#endif

#if defined(CONFIG_BCMBCA_OTP)
	/* Wipe temp SOTP items */
	bcm_otp_deinit();
#endif
	/* 
	 * even thought the name says linux but it does everything needed for
	 * boot to the next image: flush and disable cache, disable mmu
	 */
	cleanup_before_linux();

	arch_cpu_deinit();

#ifdef CONFIG_BCMBCA_PGTBL_IN_MEMC_SRAM
	bcmbca_disable_memc_sram();
#endif
}

void spl_board_ddrinit(early_abort_t* ea_info)
{
#if defined(CONFIG_BCMBCA_DDRC)  
	uint32_t mcb_sel = 0,mcb_mode = 0;

	if ((ea_info->status&SPL_EA_DDR_MCB_SEL)) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_MCB_SEL);
		mcb_sel = ea_info->data;
	}
	else if ((ea_info->status&(SPL_EA_DDR3_SAFE_MODE))) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_DDR3_SAFE_MODE);
	}
#if defined(CONFIG_BCMBCA_DDR4)
	else if ((ea_info->status&SPL_EA_DDR4_SAFE_MODE)) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_DDR4_SAFE_MODE);
	}
#endif
#if defined(CONFIG_BCMBCA_LPDDR4)
	else if ((ea_info->status&SPL_EA_LPDDR4_SAFE_MODE)) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_LPDDR4_SAFE_MODE);
	}
	else if ((ea_info->status&SPL_EA_LPDDR4X_SAFE_MODE)) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_LPDDR4X_SAFE_MODE);
	}
#endif
#if defined(CONFIG_BCMBCA_LPDDR5)
	else if ((ea_info->status&SPL_EA_LPDDR5_SAFE_MODE)) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_LPDDR5_SAFE_MODE);
	}
	else if ((ea_info->status&SPL_EA_LPDDR5X_SAFE_MODE)) {
		mcb_mode = (SPL_DDR_INIT_MCB_OVRD|SPL_DDR_INIT_LPDDR5X_SAFE_MODE);
	}
#endif

	/*printf("\nGot mcb_mode 0x%x\n",mcb_mode);*/
	spl_ddrinit(mcb_mode, mcb_sel);
#endif
	return;
}

void spl_board_init(void)
{
	early_abort_t* ea_info;

#ifdef CONFIG_BCMBCA_LDO_TRIM
	bcmbca_set_ldo_trim();
#endif
	boot_flash_init();

	ea_info = early_abort_info();

#ifdef CONFIG_BCMBCA_EARLY_ABORT_JTAG_UNLOCK
	printf("WARNING -- JTAG UNLOCK IS ENABLED\n");
#endif
	spl_board_ddrinit(ea_info);

	if ((ea_info->status&(SPL_EA_IMAGE_FB))) {
		tplparams.early_flags = SPL_EA_IMAGE_FB;
	}else if ((ea_info->status&SPL_EA_IMAGE_RECOV)) {
		tplparams.early_flags = SPL_EA_IMAGE_RECOV;
	}
	if ((ea_info->status&SPL_EA_IGNORE_BOARDID)) {
		tplparams.early_flags |= SPL_EA_IGNORE_BOARDID;
	}
	if ((ea_info->status&SPL_EA_DDR_SAFE_MODE_MASK)) {
		tplparams.early_flags |= (ea_info->status&SPL_EA_DDR_SAFE_MODE_MASK);
	}	
	/*printf("\nGot TPL flags 0x%x\n",tplparams.early_flags);*/
	setup_tpl_parms(&tplparams);

#if defined(CONFIG_SPL_WATCHDOG_SUPPORT) && defined(CONFIG_WDT)
	if (!(ea_info->status&(SPL_EA_WDT_DISABLE))) 
		initr_watchdog();
#endif
	start_tpl(&tplparams);
}
