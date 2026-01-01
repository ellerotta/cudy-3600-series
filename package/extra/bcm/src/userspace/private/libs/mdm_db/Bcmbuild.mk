LIB := libmdm_db.so

all install: conditional_build


ifeq ($(BCM_MODULAR_BUILD),)
# Old way: infer location of make.common based on pwd.
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
else
# New Modular Build way: EXT_BUILD_DIR must be set.
# Also point BUILD_DIR to EXT_BUILD_DIR
BUILD_DIR := $(EXT_BUILD_DIR)
include $(EXT_BUILD_DIR)/make.common
endif

ifneq ($(strip $(BRCM_VOICE_SUPPORT)),)
include $(BUILD_DIR)/make.voice
endif


# BRCM_SUPPORTS_MULTIARCH_BUILD
ARCH                  := $(PROFILE_ARCH)
LOCAL_BUILD_SUBDIR    := objs/$(ARCH)
LIB_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/lib
HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/private/include
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core/linux


export ARCH CFLAGS LIB_INSTALL_DIR HEADER_INSTALL_DIR


# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)


ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_MDM_DB := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_MDM_DB := 1
endif

ifeq ($(strip $(COND_BUILD_MDM_DB)),1)

conditional_build:
	mkdir -p $(LOCAL_BUILD_SUBDIR)
	$(MAKE) -C $(LOCAL_BUILD_SUBDIR) -f ../../Makefile install
	mkdir -p  $(FINAL_LIB_INSTALL_DIR)
	cp -upf $(LIB_INSTALL_DIR)/$(LIB) $(FINAL_LIB_INSTALL_DIR)

else

conditional_build:
	@echo "skipping $(LIB) (not configured)"

endif


clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)
	-mkdir -p $(LOCAL_BUILD_SUBDIR)
	-$(MAKE) -C $(LOCAL_BUILD_SUBDIR) -f ../../Makefile clean
	rm -rf objs


CONSUMER_RELEASE_BINARYONLY_PREPARE: binaryonly_prepare

binaryonly_prepare:
ifneq ($(wildcard objs/$(ARCH)),)
	-mkdir -p objs/$(ARCH)
	-$(MAKE) -C objs/$(ARCH) -f ../../Makefile ARCH=$(ARCH) binaryonly_prepare
endif
	-$(MAKE) -f Makefile binaryonly_final
	rm -rf objs


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
