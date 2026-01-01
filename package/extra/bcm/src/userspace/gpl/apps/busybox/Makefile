
all dynamic install: conditional_build

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common
include $(BUILD_DIR)/make.wlan

export INC_KERNEL_BASE

# these vars are needed by busybox
export BUILD_BCMBUSYBOX_NTPD \
       BUILD_VANILLA_BUSYBOX_OPENSYNC \
       BUILD_VANILLA_BUSYBOX_LITE \
       BUILD_UBOOT\
       BUILD_SGS \
       BUILD_LIBMODSW \
       BUILD_ASAN \
       DISABLE_NOR_RAW_PARTITION \
       ENABLE_APP_COREDUMPS \
       BRCM_NFS_MOUNT_EN \
       BRCM_QEMU \
       BUILD_CUSTOMER \
       BUILD_CUSTOMER_NAME


APP = busybox-1.36.1
BCM_APP=bcm_$(APP)
VANILLA_APP=vanilla_$(APP)


.PHONY: check_untar_patch_configure conditional_build check_versions


target_bb :=
ifneq ($(strip $(BUILD_VANILLA_BUSYBOX)),)
target_bb += $(VANILLA_APP)_install
endif
ifneq ($(strip $(BUILD_BCMBUSYBOX)),)
target_bb += $(BCM_APP)_install
endif


conditional_build: $(target_bb)
ifneq ($(strip $(BCM_COND_HAVE_VANILLA_BUSYBOX)),)
	@echo "Vanilla busybox will be provided externally (not built by BDK build system)"
endif
ifeq ($(strip $(BUILD_BCMBUSYBOX))$(strip $(BUILD_VANILLA_BUSYBOX))$(strip $(BCM_COND_HAVE_VANILLA_BUSYBOX)),)
	@echo "skipping busybox (not configured)"
endif


BUILD_BUSYBOX_STATIC_FLAG=n
ifeq ($(strip $(DESKTOP_LINUX)),)
ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)
BUILD_BUSYBOX_STATIC_FLAG=y
endif
endif


# Depend on tar file
# Use $(BCM_APP)/Makefile to mark tar progress is completed
$(BCM_APP)/Makefile: $(APP).tar.bz2 Makefile
	rm -rf $(BCM_APP)
	mkdir $(BCM_APP)
	cd $(BCM_APP) && tar --strip-components=1 -xkf ../$(APP).tar.bz2
	@echo "$(BCM_APP) is untarred"
	@echo "*********************Applying CMS/BDK customization to $(BCM_APP)"
	patch -p1 -b -s -d$(BCM_APP) < $(BCM_APP).patch
	touch $@

# Depend on $(BCM_APP)/Makefile so that tar progress should be done only once
$(BCM_APP)/.config: $(BCM_APP)/Makefile $(wildcard bcm_config.d/*.conf) bcm_config.d/brcm.config
	cat bcm_config.d/brcm.config > $(BCM_APP)/.config
	for i in bcm_config.d/[0-9]*.conf ; \
		do echo "config script $$i" ; \
		$$i $(BCM_APP)/.config; \
	done
	@echo "$(BCM_APP) is configured"

$(BCM_APP)/Config.h: $(BCM_APP)/.config
	$(MAKE) -C $(BCM_APP) include/autoconf.h
	cd $(BCM_APP) && cp -f include/autoconf.h Config.h

ifneq ($(strip $(BUILD_SPDTEST)),)
SPDTEST_LIBS = bcm_spdt bdmf
endif

# Check if native rpc exists in toolchain
ifneq ("$(wildcard $(TOOLCHAIN_INCLUDE_DIR)/rpc/rpc.h)","")
    BRCM_EXTRA_FLAGS= 
else
    BRCM_EXTRA_FLAGS = CONFIG_EXTRA_CFLAGS="-I$(BCM_FSBUILD_DIR)/public/include/tirpc" CONFIG_EXTRA_LDFLAGS="-L$(BCM_FSBUILD_DIR)/public/lib" CONFIG_EXTRA_LDLIBS="tirpc pthread" 
endif

# Depend on $(BCM_APP)/.config so that make progress can continue where it is stopped before
$(BCM_APP): $(BCM_APP)/.config $(BCM_APP)/Config.h
	@echo "**********Building $(BCM_APP)"
	BRCM_LIBS="cms_msg cms_util gen_util bcm_flashutil bcm_util sys_util dl bcm_boardctl resolv $(SPDTEST_LIBS)" $(MAKE) CC="${CC}" ${BRCM_EXTRA_FLAGS} -C $(BCM_APP) prepare
	@echo "Done building $(BCM_APP)"

$(BCM_APP)_install: $(BCM_APP)
	BRCM_LIBS="cms_msg cms_util gen_util bcm_flashutil bcm_util sys_util dl bcm_boardctl resolv $(SPDTEST_LIBS)" $(MAKE) CC="${CC}" ${BRCM_EXTRA_FLAGS} -C $(BCM_APP) install

# Depend on tar file
# Use $(VANILLA_APP)/Makefile to mark tar progress is completed
$(VANILLA_APP)/Makefile: $(APP).tar.bz2 Makefile
	rm -rf $(VANILLA_APP)
	mkdir $(VANILLA_APP)
	cd $(VANILLA_APP) && tar --strip-components=1 -xkf ../$(APP).tar.bz2
	@echo "$(VANILLA_APP) is untarred"
	@echo "*********************Applying common build patches to $(VANILLA_APP)"
	patch -p1 -b -s -d$(VANILLA_APP) < $(VANILLA_APP).patch
ifneq ($(strip $(BUILD_OPENSYNC)),)
	@echo "*********************Applying OpenSync patches to $(VANILLA_APP)"
	for i in opensync_patches/[0-9]*.patch ; do echo "Applying patch $$i" ; patch -p1 -b -s -d$(VANILLA_APP) < $$i ; done
endif
	touch $@

# Depend on $(VANILLA_APP)/configure so that tar progress should be done only once
$(VANILLA_APP)/.config: $(VANILLA_APP)/Makefile $(wildcard vanilla_config.d/*.conf) vanilla_config.d/brcm.config
	cat vanilla_config.d/brcm.config > $(VANILLA_APP)/.config
	for i in vanilla_config.d/[0-9]*.conf ; \
		do echo "config script $$i" ; \
		BUILD_BUSYBOX_STATIC=$(BUILD_BUSYBOX_STATIC_FLAG) perl $$i $(VANILLA_APP)/.config; \
	done
	@echo "$(VANILLA_APP) is configured"

$(VANILLA_APP)/Config.h: $(VANILLA_APP)/.config
	$(MAKE) -C $(VANILLA_APP) include/autoconf.h
	cd $(VANILLA_APP) && cp -f include/autoconf.h Config.h

# Depend on check_config so that make progress can continue where it is stopped before
$(VANILLA_APP): $(VANILLA_APP)/.config $(VANILLA_APP)/Config.h
	@echo "**********Building $(VANILLA_APP)"
	$(MAKE) CC="$(CC)" ${BRCM_EXTRA_FLAGS} -C $(VANILLA_APP) prepare


$(VANILLA_APP)_install: $(VANILLA_APP)
	$(MAKE) CC="$(CC)" ${BRCM_EXTRA_FLAGS} -C $(VANILLA_APP) install

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(BCM_APP) $(VANILLA_APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
