LIB := libmdm_cli.so

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


ARCH                  := $(PROFILE_ARCH)
LIB_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/lib
HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/private/include
ALLOWED_INCLUDE_PATHS := -I . \
                         -I $(HEADER_INSTALL_DIR) \
                         -I $(BCM_FSBUILD_DIR)/public/include \
                         -I $(BCM_FSBUILD_DIR)/private/include \
                         -I $(BUILD_DIR)/userspace/private/include \
                         -I $(BUILD_DIR)/userspace/private/libs/cms_core


ALLOWED_LIB_DIRS := /lib:/private/lib:/public/lib

export ARCH CFLAGS BCM_LD_FLAGS BCM_LIB_PATH
export LIB_INSTALL_DIR HEADER_INSTALL_DIR


# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/lib


ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_MDM_CLI := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_MDM_CLI := 1
endif

ifeq ($(strip $(COND_BUILD_MDM_CLI)),1)

conditional_build:
	mkdir -p objs
	$(MAKE) -C objs -f ../Makefile install
	mkdir -p  $(FINAL_LIB_INSTALL_DIR)
	cp -p $(LIB_INSTALL_DIR)/$(LIB) $(FINAL_LIB_INSTALL_DIR)

else

conditional_build:
	@echo "Skipping $(LIB) (not configured)"

endif


clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile clean
	rm -rf objs


CONSUMER_RELEASE_BINARYONLY_PREPARE: binaryonly_prepare

binaryonly_prepare:
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile binaryonly_prepare
	rm -rf objs


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
