
KERNEL_DIR=$(KERNEL_BUILD_DIR)

nothing: default

include $(BCM_TARGET_PATH)/make.common
include $(BCM_TARGET_PATH)/make.wlan



unexport \
BRCMAPPS                   \
BRCM_KERNEL_AUXFS_JFFS2    \
BRCM_PSI_VERSION           \
BRCM_PTHREADS              \
BRCM_RAMDISK_BOOT_EN       \
BRCM_RAMDISK_SIZE          \
BRCM_NFS_MOUNT_EN          \
BRCM_SNMP                  \
BUILD_VLANCTL              \
BUILD_DDNSD                \
BUILD_EBTABLES             \
BUILD_EPITTCP              \
BUILD_FTPD                 \
BUILD_FTPD_STORAGE         \
BUILD_IPPD                 \
BUILD_IPSEC_TOOLS          \
BUILD_MKSQUASHFS           \
BUILD_PPPD                 \
BUILD_SNMP                 \
BUILD_SSHD                 \
BUILD_SSHD_MIPS_GENKEY     \
BUILD_BRCM_CMS             \
BUILD_TR69C                \
BUILD_TR69C_SSL            \
BUILD_OMCI                 \
BUILD_UDHCP                \
BUILD_UDHCP_RELAY          \
BUILD_ZEBRA                \
BUILD_LIBUSB               \
WEB_POPUP                  \
BUILD_BOARD_LOG_SECTION    \
BRCM_LOG_SECTION_SIZE      \
BRCM_FLASHBLK_SIZE         \
BRCM_AUXFS_PERCENT         \
BRCM_BACKUP_PSI            \
BUILD_GPONCTL              \
BUILD_SPUCTL               \
FLASH_NAND_BLOCK_16KB      \
FLASH_NAND_BLOCK_128KB     \
FLASH_NAND_BLOCK_256KB     \
FLASH_NAND_BLOCK_512KB     \
FLASH_NAND_BLOCK_1024KB     \
BUILD_IQCTL                 \
BUILD_EPONCTL               \
BRCM_PARTITION_CFG_FILE     

export BRCM_KERNEL_DEBUG           \

export INC_BCMDRIVER_PATH 

export BRCM_RDP_PARAM1_SIZE BRCM_RDP_PARAM2_SIZE BRCM_DHD_PARAM1_SIZE BRCM_DHD_PARAM2_SIZE BRCM_DHD_PARAM3_SIZE OOPSLOG_PARTITION_NAME
export BRCM_DRIVER_PKTFLOW_DEBUG BRCM_DRIVER_PKTFLOW_IPV6 BRCM_DRIVER_PKTFLOW_MCAST
export INC_RDPA_MW_PATH INC_SPI_PATH INC_ENET_DMA_FLAGS PROFILE_KERNEL_VER INC_ADSLDRV_PATH 
export INC_BCMLIBS_PATH INC_UTILS_PATH RDPSDK_DIR BCMDRIVERS_DIR
export KERNEL_DEBUG
export BUILD_PHY_ADSL   ### FIXME -- These should be using Kconfig
export ORIG_PROFILE_ARCH
export BUILD_BCM_WLAN_NO_MFGBIN
export BUILD_BCM_WLAN_DGASP
export RDP_PROJECT2
export BRCM_MAINTAINENCETAG

#The kernel build will use the value of 'CROSS_COMPILE'. Make.common will calculate the kernel toolchain and set it in KCROSS_COMPILE
	CROSS_COMPILE:=$(KCROSS_COMPILE)

BCMD_AG_MAKEFILE:=Makefile.autogen
BCMD_AG_KCONFIG:=Kconfig.autogen
BCMD_AG_MAKEFILE_TMP:=$(BCMD_AG_MAKEFILE).tmp
BCMD_AG_KCONFIG_TMP:=$(BCMD_AG_KCONFIG).tmp

$(BCMDRIVERS_AUTOGEN): $(BCM_TARGET_PATH)/kernel/bcmkernel/Kconfig.bcm
	@cd $(BRCMDRIVERS_DIR); echo -e "\n# Automatically generated file -- do not modify manually\n\n" > $(BCMD_AG_KCONFIG_TMP)
	@cd $(BRCMDRIVERS_DIR); echo -e "\n# Automatically generated file -- do not modify manually\n\n" > $(BCMD_AG_MAKEFILE_TMP)
	@cd $(BRCMDRIVERS_DIR); echo -e "\n\$$(info READING AG MAKEFILE)\n\n" >> $(BCMD_AG_MAKEFILE_TMP)
	@alldrivers=""; \
	 cd $(BRCMDRIVERS_DIR); \
	  for autodetect in $$(find * -type f -name autodetect); do \
		dir=$${autodetect%/*}; \
		driver=$$(grep -i "^DRIVER\|FEATURE:" $$autodetect | awk -F ': *' '{ print $$2 }'); \
		[ $$driver ] || driver=$${dir##*/}; \
		[ $$(echo $$driver | wc -w) -ne 1 ] && echo "Error parsing $$autodetect" >2 && exit 1; \
		echo "Processing $$driver ($$dir)"; \
		DRIVER=$$(echo "$${driver}" | tr '[:lower:]' '[:upper:]'); \
		echo "\$$(eval \$$(call LN_RULE_AG, CONFIG_BCM_$${DRIVER}, $$dir, \$$(LN_NAME)))" >> $(BCMD_AG_MAKEFILE_TMP); \
		if [ -e $$dir/Kconfig.autodetect ]; then \
			echo "menu \"$${DRIVER}\"" >> $(BCMD_AG_KCONFIG_TMP);\
			echo "source \"../../bcmdrivers/$$dir/Kconfig.autodetect\"" >> $(BCMD_AG_KCONFIG_TMP); \
			echo "endmenu " >> $(BCMD_AG_KCONFIG_TMP); \
			echo "" >> $(BCMD_AG_KCONFIG_TMP);\
		fi; \
		true; \
	 done; \
	 duplicates=$$(echo $$alldrivers | tr " " "\n" | sort | uniq -d | tr "\n" " "); echo $$duplicates; \
	 [ $V ] && echo "alldrivers: $$alldrivers" && echo "duplicates: $$duplicates" || true; \
	 if [ $$duplicates ]; then \
		echo "ERROR: duplicate drivers found in autodetect -- $$duplicates" >&2; \
		exit 1; \
	 fi
	@# only update the $(BCMD_AG_KCONFIG) and makefile.autogen files if they haven't changed (to prevent rebuilding):
	@cd $(BRCMDRIVERS_DIR); [ -e $(BCMD_AG_MAKEFILE) ] && cmp -s $(BCMD_AG_MAKEFILE) $(BCMD_AG_MAKEFILE_TMP) || mv $(BCMD_AG_MAKEFILE_TMP) $(BCMD_AG_MAKEFILE)
	@cd $(BRCMDRIVERS_DIR);[ -e $(BCMD_AG_KCONFIG) ] && cmp -s $(BCMD_AG_KCONFIG) $(BCMD_AG_KCONFIG_TMP) || mv $(BCMD_AG_KCONFIG_TMP) $(BCMD_AG_KCONFIG)
	@cd $(BRCMDRIVERS_DIR); rm -f $(BCMD_AG_MAKEFILE_TMP) $(BCMD_AG_KCONFIG_TMP)
	@touch $@
ifeq ($(strip $(DESKTOP_LINUX)),)
default version_info headers_install bcmkernel_headers_install olddefconfig modules modules_install clean mrproper tools/perf tools/perf_clean dtbs prepare_bcm_driver:

	$(MAKE) ARCH=$(KARCH) -C $(LINUX_DIR) $(MAKEOVERRIDES) $(MAKECMDGOALS) $(KERN_TARGET) EXTRAVERSION= LOCALVERSION= LINUX_VER_STR=$(LINUX_VER_STR)

	$(BCMDRIVERS_AUTOGEN)
else
default version_info headers_install bcmkernel_headers_install olddefconfig modules modules_install tools/perf tools/perf_clean dtbs prepare_bcm_driver:
	@echo "******************** SKIP kernel build for DESKTOP_LINUX ********************";
	touch $(KERNEL_DIR)/vmlinux
mrproper clean:
	rm -f $(KERNEL_DIR)/vmlinux $(KERNEL_DIR)/.config
endif


bcm_headers_install:
	$(Q)echo "======================================================"
	$(Q)$(MAKE) -C $(BRCMDRIVERS_DIR) bcm_headers_install EXTRAVERSION= LOCALVERSION=

.PHONY: nothing bcm_headers_install version_info headers_install default olddefconfig modules modules_install clean mrproper tools/perf tools/perf_clean dtbs
