EXE    := cmscli

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
EXE_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/bin
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BUILD_DIR)/userspace/private/include

ALLOWED_LIB_DIRS := /lib:/private/lib:/public/lib


export ARCH CFLAGS BCM_LD_FLAGS BCM_RPATH_LINK_OPTION BCM_LIB_PATH
export INSTALL_DIR EXE_INSTALL_DIR
export CMS_COMMON_LIBS CMS_CORE_LIBS
export BUILD_GPON BUILD_EPON_SDK


# Final location of EXE for system image.  Only the BRCM build system needs to
# know about this.
FINAL_EXE_INSTALL_DIR := $(INSTALL_DIR)/bin



ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_CMSCLI := 1
endif
ifneq ($(strip $(BUILD_BRCM_BDK)),)
COND_BUILD_CMSCLI := 1
endif


ifeq ($(strip $(COND_BUILD_CMSCLI)),1)

conditional_build:
	mkdir -p objs
	$(MAKE) -C objs -f ../Makefile install
	mkdir -p $(FINAL_EXE_INSTALL_DIR)
	cp -p $(EXE_INSTALL_DIR)/$(EXE) $(FINAL_EXE_INSTALL_DIR)

else

conditional_build:
	@echo "Skipping $(EXE) (not configured)"

endif


clean:
	rm -f $(FINAL_EXE_INSTALL_DIR)/$(EXE)
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile clean
	rm -rf objs


CONSUMER_RELEASE_BINARYONLY_PREPARE: delete_source

# apps do not need to support multi-arch
save_binaries:
ifneq ($(wildcard objs),)
	-$(MAKE) -C objs -f ../Makefile save_binaries
endif

delete_source: save_binaries
	-$(MAKE) -f Makefile delete_source
	rm -rf objs


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
