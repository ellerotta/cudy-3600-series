LIB := libsys_directory.so

all install: conditional_build


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common



# BRCM_SUPPORTS_MULTIARCH_BUILD
ARCH                  := $(PROFILE_ARCH)
LOCAL_BUILD_SUBDIR    := objs/$(ARCH)
LIB_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/lib$(BCM_INSTALL_SUFFIX_DIR)
HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/private/include
ALLOWED_INCLUDE_PATHS := -I . \
                         -I $(BCM_FSBUILD_DIR)/private/include \
                         -I $(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/public/include/json-c \
                         -I $(BUILD_DIR)/userspace/public/include  \
                         -I $(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I $(BUILD_DIR)/userspace/private/include  \
                         -I $(BUILD_DIR)/userspace/private/include/$(OALDIR)

# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/public


export ARCH CFLAGS LIB_INSTALL_DIR HEADER_INSTALL_DIR CMS_LIB_PATH


# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)


# sys_directory supports both D-Bus and ubus now
ifneq ($(strip $(BUILD_BDK_SYS_DIRECTORY)),)
COND_BUILD_SYS_DIRECTORY := 1
ifneq ($(strip $(BUILD_BRCM_OPENWRT)),)
CFLAGS += -DSUPPORT_BRCM_OPENWRT
endif
endif

ifeq ($(strip $(COND_BUILD_SYS_DIRECTORY)),1)

conditional_build:
	mkdir -p $(LOCAL_BUILD_SUBDIR)
	$(MAKE) -C $(LOCAL_BUILD_SUBDIR) -f ../../Makefile install
	mkdir -p  $(FINAL_LIB_INSTALL_DIR)
	cp -p $(LIB_INSTALL_DIR)/$(LIB) $(FINAL_LIB_INSTALL_DIR)

else

conditional_build:
	@echo "Skipping $(LIB) (not configured)"

endif


clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)
	-mkdir -p $(LOCAL_BUILD_SUBDIR)
	-$(MAKE) -C $(LOCAL_BUILD_SUBDIR) -f ../../Makefile clean
	rm -rf objs


CONSUMER_RELEASE_BINARYONLY_PREPARE: delete_source

save_binaries:
ifneq ($(wildcard objs/$(ARCH)),)
	-$(MAKE) -C objs/$(ARCH) -f ../../Makefile ARCH=$(ARCH) save_binaries
endif

delete_source: save_binaries
	-$(MAKE) -f Makefile delete_source
	rm -rf objs


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
