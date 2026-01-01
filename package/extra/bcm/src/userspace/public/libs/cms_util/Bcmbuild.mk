LIB := libcms_util.so

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


ARCH=$(PROFILE_ARCH)
LIB_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/public/lib
HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/public/include
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(CURR_DIR) \
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
                         -I$(BCM_FSBUILD_DIR)/bcmdrivers/include

export ARCH CFLAGS LIB_INSTALL_DIR HEADER_INSTALL_DIR ETC_INSTALL_DIR \
       BUILD_DIR KERNEL_DIR BRCM_CHIP \
       BUILD_BRCM_CMS BUILD_DISTRIBUTED_MDM BUILD_NETAPP_RESTRICT


# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(EXT_DEVICEFS_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
FINAL_ETC_INSTALL_DIR := $(EXT_DEVICEFS_DIR)/etc/cms_entity_info.d
export FINAL_ETC_INSTALL_DIR


ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_CMS_UTIL := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_CMS_UTIL := 1
endif
ifneq ($(strip $(BUILD_CMS_UTIL)),)
COND_BUILD_CMS_UTIL := 1
endif

ifeq ($(strip $(COND_BUILD_CMS_UTIL)),1)

conditional_build:
	mkdir -p objs
	$(MAKE) -C objs -f ../Makefile install
	mkdir -p $(FINAL_LIB_INSTALL_DIR)
	cp -upf $(LIB_INSTALL_DIR)/$(LIB) $(FINAL_LIB_INSTALL_DIR)

else

conditional_build:
	@echo "Skipping $(LIB) (not configured)"

endif


clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)
	rm -f $(addprefix $(FINAL_ETC_INSTALL_DIR)/,$(EID_FILES))
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile clean
	rm -rf objs



shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
