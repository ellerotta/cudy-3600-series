#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

# Build in kernel build dir
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bcm-bootloader
PKG_VERSION:=1.0
PKG_RELEASE:=1.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)_$(PKG_VERSION)
BCM_TARGET_PATH:=$(TOPDIR)/package/extra/bcm/src

include $(INCLUDE_DIR)/package.mk

DEVICE_PROFILE:=$(call qstrip,$(CONFIG_TARGET_PROFILE))
DEVICE_DIR:=$(subst DEVICE_,,$(DEVICE_PROFILE))

define Package/bcm-bootloader
	$(call Package/bcm-bootloader/Default)
	CATEGORY:=Boot Loaders
	TITLE:=bootloader for BCM routers
	DEPENDS:=+libopenssl
endef

define Package/$(PKG_NAME)/description
	bootloader for BCM routers
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(TOPDIR)/package/extra/bcm/src/$(PKG_NAME)/* $(PKG_BUILD_DIR)/

	if [ \! -e $(PKG_BUILD_DIR)/bootloaders/u-boot-2019.07/configs/$(DEVICE_DIR) ]; then \
		echo "ERROR:file $(PKG_BUILD_DIR)/bootloaders/u-boot-2019.07/configs/$(DEVICE_DIR) was NOT found "; \
	else \
		$(CP) $(PKG_BUILD_DIR)/bootloaders/u-boot-2019.07/configs/$(DEVICE_DIR)/* $(PKG_BUILD_DIR)/bootloaders/u-boot-2019.07/configs/ ; \
	fi;
endef

define Build/Compile
	make -C $(PKG_BUILD_DIR)/hostTools hosttools_common
	make -C $(PKG_BUILD_DIR)/hostTools hosttools_bootloader
	make -f $(BCM_TARGET_PATH)/build/make.uboot all INCLUDE_OPTEE= INCLUDE_ATF=y IMAGE_GOAL=

	make -f $(BCM_TARGET_PATH)/build/make.uboot atf-build
	echo "Compressing ARM Trusted Firmware using lzma"
	$(PKG_BUILD_DIR)/hostTools/cmplzma -k -2 -lzma $(PKG_BUILD_DIR)/bootloaders/armtf/armtf.elf $(PKG_BUILD_DIR)/bootloaders/armtf/armtf.bin $(PKG_BUILD_DIR)/bootloaders/armtf/armtf.lz

	make -f $(BCM_TARGET_PATH)/build/make.uboot loader-build
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bootloaders/config/dumpimage $(1)/usr/bin
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/hostTools/cmplzma $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/hostTools/lzmacmd $(1)/usr/bin/

	$(CP) $(PKG_BUILD_DIR)/bootloaders/obj/binaries/loader_test_nand_$(CONFIG_BCM_CHIP_ID).bin $(KERNEL_BUILD_DIR)/loader_test_nand_$(CONFIG_BCM_CHIP_ID).bin
	$(CP) $(PKG_BUILD_DIR)/bootloaders/obj/binaries/loader_test_nand_$(CONFIG_BCM_CHIP_ID).bin $(BIN_DIR)/loader_test_nand_$(CONFIG_BCM_CHIP_ID).bin
	$(CP) $(PKG_BUILD_DIR)/bootloaders/obj/uboot/u-boot-nodtb.bin $(KERNEL_BUILD_DIR)/u-boot-nodtb.bin
	$(CP) $(PKG_BUILD_DIR)/bootloaders/obj/uboot/u-boot-nodtb.bin $(BIN_DIR)/u-boot-nodtb.bin
endef

$(eval $(call BuildPackage,bcm-bootloader))
