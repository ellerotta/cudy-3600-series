#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=brcmbca
BOARDNAME:=Broadcom BCA
FEATURES:=squashfs nand usb pci pcie gpio
MAINTAINER:=Guang Qin <elvis.qin@broadcom.com>
SUBTARGETS:=bcm963178 bcm963146 bcm963158 bcm947622 bcm94908 bcm94912 bcm96756 bcm96765 bcm96764 bcm96766 bcm96813 bcm96846 bcm96855 bcm96856 bcm96858 bcm96878 bcm968880

KERNEL_PATCHVER:=4.19
KERNEL_TESTING_PATCHVER:=4.19

define Target/Description
	Build images for Broadcom BCA based BCM63xx or BCM47xx or BCM68xx routers with ARM CPU.
endef

include $(INCLUDE_DIR)/target.mk

KERNELNAME:=zImage dtbs Image

export IMAGE_DIR=$(PLATFORM_DIR)/image
export BIN_PROFILE_DIR=$(BIN_DIR)

DEVICE_PROFILE:=$(call qstrip,$(CONFIG_TARGET_PROFILE))
DEVICE_DTS_DIR:=$(PLATFORM_SUBDIR)/$(subst DEVICE_,,$(DEVICE_PROFILE))
OemKernelDTS := $(DEVICE_DTS_DIR)/*.dts
OemKernelDTSI := $(DEVICE_DTS_DIR)/*.dtsi

#copy from include/kernel-build.mk, and redefine Kernel/CompileImage to add copy command
define Kernel/CompileImage
	if [ \! -e $(OemKernelDTS) ]; then \
		echo "ERROR:file $(OemKernelDTS) was NOT found "; \
	else \
        cp -rf $(OemKernelDTS) $(LINUX_DIR)/arch/arm/boot/dts; \
	fi;

	if [ \! -e $(OemKernelDTSI) ]; then \
		echo "ERROR:file $(OemKernelDTSI) was NOT found "; \
	else \
		cp -rf $(OemKernelDTSI) $(LINUX_DIR)/arch/arm/boot/dts; \
	fi;
	$(call Kernel/CompileImage/Default)
	$(call Kernel/CompileImage/Initramfs)
endef

ifneq ($(strip $(CONFIG_EXTERNAL_KERNEL_TREE)),"")
	define Kernel/Prepare
		$(call Kernel/Prepare/Default)
		rsync -az $(CONFIG_EXTERNAL_KERNEL_TREE)/../bcmkernel $(LINUX_DIR)/../
		rsync -az $(CONFIG_EXTERNAL_KERNEL_TREE)/../dts $(LINUX_DIR)/../
		rsync -az $(CONFIG_EXTERNAL_KERNEL_TREE)/../../bcmdrivers $(LINUX_DIR)/../../
		rsync -az $(CONFIG_EXTERNAL_KERNEL_TREE)/../../userspace $(LINUX_DIR)/../../
		rsync -az $(CONFIG_EXTERNAL_KERNEL_TREE)/../../shared $(LINUX_DIR)/../../
		rsync -az $(CONFIG_EXTERNAL_KERNEL_TREE)/../../rdp $(LINUX_DIR)/../../

		# ln -s impl$(CONFIG_BCM_WLIMPL) $(LINUX_DIR)/../../bcmdrivers/broadcom/net/wl/bcm9$(CONFIG_BCM_CHIP_ID)
		# cp -rf $(BCM_TARGET_PATH)/config/$(CONFIG_BCM_CHIP_ID)/.config $(LINUX_DIR)/../../bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLIMPL)/main/components/router

		cp -rf $(CONFIG_EXTERNAL_KERNEL_TREE)/../dts/ip $(LINUX_DIR)/arch/arm/boot/
		cp -rf $(CONFIG_EXTERNAL_KERNEL_TREE)/../dts/*.dtsi $(LINUX_DIR)/arch/arm/boot/
		cp -rf $(CONFIG_EXTERNAL_KERNEL_TREE)/../dts/$(CONFIG_BCM_CHIP_ID)/*.dts $(LINUX_DIR)/arch/arm/boot/dts/
		cp -rf $(CONFIG_EXTERNAL_KERNEL_TREE)/../dts/$(CONFIG_BCM_CHIP_ID)/*.dtsi $(LINUX_DIR)/arch/arm/boot/dts/

		if [ \! -e $(OemKernelDTS) ]; then \
			echo "ERROR:file $(OemKernelDTS) was NOT found "; \
		else \
			cp -rf $(OemKernelDTS) $(LINUX_DIR)/arch/arm/boot/dts/; \
		fi;

		if [ \! -e $(OemKernelDTSI) ]; then \
			echo "ERROR:file $(OemKernelDTSI) was NOT found "; \
		else \
			cp -rf $(OemKernelDTSI) $(LINUX_DIR)/arch/arm/boot/dts/; \
		fi;
    endef



    define Kernel/Configure
		$(MAKE) -f $(BCM_TARGET_PATH)/build/pre_kernelbuild.mk BCM_TARGET_PATH=$(BCM_TARGET_PATH) BCM_PROFILE=$(CONFIG_BCM_PROFILE) LINUX_DIR=$(LINUX_DIR) BUILD_DIR=$(BUILD_DIR)

		$(MAKE) -f $(BCM_TARGET_PATH)/build/Bcmkernel.mk prepare_bcm_driver BCM_TARGET_PATH=$(BCM_TARGET_PATH) BCM_PROFILE=$(CONFIG_BCM_PROFILE) LINUX_DIR=$(LINUX_DIR) BUILD_DIR=$(BUILD_DIR)

		$(call Kernel/Configure/Default)
    endef


#    define Kernel/CompileImage
#    	cd $(CONFIG_EXTERNAL_KERNEL_TREE)/../..; #make TARGET_PROFILE=$(TARGET_PROFILE) kernelbuild
#    endef
endif

DEFAULT_PACKAGES += \
	kmod-leds-gpio kmod-gpio-button-hotplug \
	kmod-ledtrig-default-on kmod-ledtrig-gpio \
	kmod-ledtrig-timer kmod-ledtrig-netdev \
	kmod-ledtrig-oneshot

$(eval $(call BuildTarget))
