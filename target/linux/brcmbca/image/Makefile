#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk


BOOT_BCM_DIR:=$(LINUX_DIR)/../../bcm-bootloader_1.0/

DEVICE_DTS_DIR := $(PLATFORM_SUBDIR)/$(subst DEVICE_,,$(PROFILE))
SUPPORTED_DEVICES := $(subst DEVICE_,,$(PROFILE))
BOARD_NAME:=$(SUPPORTED_DEVICES)

SDK_BCM_BUILD_DIR:=$(CURDIR)/obj/binaries
SDK_BCM_BUILD_UBOOT:=$(CURDIR)/obj/uboot
SDK_BCM_TOOL_DIR:=$(CURDIR)/obj/uboot/tools

define Image/mkfs/squashfs-common
	$(STAGING_DIR_HOST)/bin/mksquashfs4 $(call mkfs_target_dir,$(1)) $@ \
		-noappend -all-root \
		-comp xz $(SQUASHFSOPT)
endef

define Image/Build/squashfs
#	$(call prepare_generic_squashfs,$(KDIR)/root.squashfs)
endef

define CompressLzma
	$(STAGING_DIR_HOST)/bin/lzma e $(1) -lc1 -lp2 -pb2 $(2)
endef

define Image/BuildKernel
	cp $(KDIR)/vmlinux.elf $(KDIR)/$(IMG_PREFIX)-vmlinux.elf
	cp $(KDIR)/vmlinux $(KDIR)/vmlinux.bin
	$(call CompressLzma,$(KDIR)/vmlinux.bin,$(KDIR)/vmlinux.bin.lzma)
	cp $(KDIR)/vmlinux* $(BIN_DIR)/

	$(INSTALL_DIR) $(BIN_DIR)/dtbs
	$(CP) $(DTS_DIR)/*.dtb  $(BIN_DIR)/dtbs/
endef

define Image/BuildKernel/FIT
	mkdir -p $(SDK_BCM_BUILD_DIR)/linux
	mkdir -p $(SDK_BCM_TOOL_DIR)
	cp $(KDIR)/vmlinux.bin.lzma $(SDK_BCM_BUILD_UBOOT)/
	cp $(BIN_DIR)/dtbs/* $(SDK_BCM_BUILD_DIR)/linux/
	cp $(BIN_DIR)/rootfs.squashfs $(SDK_BCM_BUILD_DIR)/rootfs_nonubifs
	cp $(BOOT_BCM_DIR)/bootloaders/obj/uboot/tools/mkenvimage $(SDK_BCM_TOOL_DIR)/
	cp $(BOOT_BCM_DIR)/bootloaders/obj/uboot/tools/mkimage $(SDK_BCM_TOOL_DIR)/
	cp $(DEVICE_DTS_DIR)/*.its $(SDK_BCM_BUILD_DIR)/
	cp $(DEVICE_DTS_DIR)/*.ubinize $(SDK_BCM_BUILD_DIR)/

	$(SDK_BCM_BUILD_UBOOT)/tools/mkimage -f $(SDK_BCM_BUILD_DIR)/brcm_fit_linux.its -E $(SDK_BCM_BUILD_DIR)/brcm_fit_linux.itb
endef


define Image/Build
	$(call Image/Build/$(1))
	dd if=$(KDIR)/root.$(1) of=$(BIN_DIR)/rootfs.$(1) bs=2k conv=sync
endef

$(eval $(call BuildImage))
