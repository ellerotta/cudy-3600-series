LIB := libbcm_generic_dsl_hal.so

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
                         -I $(BUILD_DIR)/userspace/public/include  \
                         -I $(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I $(BUILD_DIR)/userspace/private/include  \
                         -I $(BUILD_DIR)/userspace/private/include/$(OALDIR)

ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public

export ARCH CFLAGS LIB_INSTALL_DIR HEADER_INSTALL_DIR BCM_LD_FLAGS CMS_LIB_PATH CMS_OPTION_RPATH CMS_LIB_RPATH 

# Remove all mdm_cbk_* and bcm_*_hal libs from CMS_CORE_LIBS.  The local
# Makefile will specify which mdm_cbk and hal libs it needs.
MDM_CORE_LIBS := $(patsubst -lmdm_cbk_%,,$(CMS_CORE_LIBS))
MDM_CORE_LIBS := $(patsubst -lbcm_%_hal,,$(MDM_CORE_LIBS))

export CMS_COMMON_LIBS MDM_CORE_LIBS

# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)

ifneq ($(strip $(BUILD_DSL)),)
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
ifneq ($(strip $(BUILD_HAL_LIBS)),)
COND_BUILD_GENERIC_DSL_HAL := 1
endif
endif
endif

ifeq ($(strip $(COND_BUILD_GENERIC_DSL_HAL)),1)

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
