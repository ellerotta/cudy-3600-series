EXE    := ippd

all install: conditional_build


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

ifneq ($(strip $(BRCM_VOICE_SUPPORT)),)
include $(BUILD_DIR)/make.voice
endif


ARCH                  := $(PROFILE_ARCH)
EXE_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/bin
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BCM_FSBUILD_DIR)/public/lib/glib-2.0/include \
                         -I$(BCM_FSBUILD_DIR)/public/include/glib-2.0 \
                         -I$(BCM_FSBUILD_DIR)/public/include/gio-unix-2.0 \
                         -I$(BCM_FSBUILD_DIR)/public/include/websockets \
                         -I$(BCM_FSBUILD_DIR)/public/include/json-c \
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)

ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public

ifeq ($(BUILD_RDPA),y)
ALLOWED_INCLUDE_PATHS += $(INC_RDP_FLAGS)
endif

export ARCH CFLAGS BCM_LD_FLAGS CMS_LIB_PATH CMS_OPTION_RPATH CMS_LIB_RPATH EXE_INSTALL_DIR BUILD_SYSV_INIT BUILD_SYSTEMD


# Remove all mdm_cbk_* and bcm_*_hal libs from CMS_CORE_LIBS.  The local
# Makefile will specify which mdm_cbk and hal libs it needs.
MDM_CORE_LIBS := $(patsubst -lmdm_cbk_%,,$(CMS_CORE_LIBS))
MDM_CORE_LIBS := $(patsubst -lbcm_%_hal,,$(MDM_CORE_LIBS))

export CMS_COMMON_LIBS MDM_CORE_LIBS


# Final location of EXE for system image.  Only the BRCM build system needs to
# know about this.
FINAL_EXE_INSTALL_DIR := $(INSTALL_DIR)/bin$(BCM_INSTALL_SUFFIX_DIR)


ifneq ($(strip $(BUILD_IPPD)),)

ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)

conditional_build:
	mkdir -p objs
	$(MAKE) -C objs -f ../Makefile install
	mkdir -p $(FINAL_EXE_INSTALL_DIR)
	cp -p $(EXE_INSTALL_DIR)/$(EXE) $(FINAL_EXE_INSTALL_DIR)

else

conditional_build:
	@echo "Skipping $(EXE) ($(EXE) requires CMS or BDK)"

endif

else

conditional_build:
	@echo "Skipping $(EXE) ($(EXE) is not configured)"

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
