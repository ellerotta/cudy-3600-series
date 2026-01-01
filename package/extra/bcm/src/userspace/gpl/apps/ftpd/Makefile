

all dynamic install: conditional_build

APP = ftpd
EID_FILE = eid_bcm_bftpd.txt

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



.PHONY: check_untar_patch_configure conditional_build check_versions ftpd


# This ftpd is only used for loading CMS/BDK config files and firmware images, not storage.
# This ftpd uses cms_msg to validate config files, so it requires a CMS/BDK build.
ifneq ($(strip $(BUILD_FTPD)),)

ifeq ($(strip $(BUILD_FTPD_STORAGE)),)

ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)
conditional_build: $(APP)
else
conditional_build: sanity_check
	@echo "skipping $(APP) (this app requires CMS or BDK config)"
endif

else
# userspace/gpl/apps/bftpd is used for storage
conditional_build: sanity_check
	@echo "skipping $(APP) (this app is not used for FTPD_STORAGE)"
endif

else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif


ftpd: generic_eid_file_install
	(tar xkfj $(APP).tar.bz2 2> /dev/null || true)
	$(MAKE) -C $(APP) $(BUILD_FTPD)


clean: generic_eid_file_clean
	-make -C $(APP) clean

