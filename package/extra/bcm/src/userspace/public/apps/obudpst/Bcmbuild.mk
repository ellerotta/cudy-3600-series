# Makefile to build udpst

EID_FILE = eid_obudpst_diag.txt

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

#remove all warnings in CFLAGS
BCM_CFLAGS_STRING += $(patsubst -W%,,$(CFLAGS)) -DBRCM_PATCH


ifneq ($(strip $(BUILD_TR471)),)
ifneq ($(strip $(BUILD_SPDSVC)),)
BCM_CFLAGS_STRING += -DBRCM_UDPST_OFFLOAD
#BCM_CFLAGS_STRING += -DBRCM_POLLING
BCM_CFLAGS_STRING += -DBRCM_UDPST_OFFLOAD_PROFILING
ifneq ($(strip $(BUILD_TR471_MFLOW)),)
# Below compilation flag controls the real mflow support by offload
BCM_CFLAGS_STRING += -DBRCM_UDPST_OFFLOAD_MFLOW
endif
BCM_CFLAGS_STRING += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
UDPST_BRCM_LINKER_FLAGS=-lspdsvc
endif
endif

ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)
BCM_EXE_LINKER_FLAGS_STRING=-lcms_msg $(CMS_COMMON_LIBS) -L$(BCM_FSBUILD_DIR)/public/lib $(UDPST_BRCM_LINKER_FLAGS)
BCM_CFLAGS_STRING += -I$(BUILD_DIR)/userspace/public/include -I$(BUILD_DIR)/userspace/public/include/linux \
		  -I$(BCM_FSBUILD_DIR)/public/include
endif

BCM_CFLAGS_STRING += $(BCM_LD_FLAGS)
BCM_EXE_LINKER_FLAGS_STRING += $(BCM_LD_FLAGS)

export BCM_CFLAGS_STRING CMS_COMMON_LIBS BUILD_DIR BCM_EXE_LINKER_FLAGS_STRING

#BUDPST_VER1 is default version at runtime
OBUDPST_VER1 := 8.1.0
#OBUDPST_VER2 set to compile another version
OBUDPST_VER2 := 8.0.0
APP_BIN = udpst
APP_PREV_BIN = udpst80

ifneq ($(strip $(BUILD_TR471)),)
conditional_build: generic_eid_file_install
	@echo "Making obudpst-$(OBUDPST_VER1)"
	$(MAKE) APP=obudpst-$(OBUDPST_VER1) -f Makefile install
	mkdir -p $(INSTALL_DIR)/usr/bin && \
	$(INSTALL) -m 755 obudpst-$(OBUDPST_VER1)/$(APP_BIN) $(INSTALL_DIR)/usr/bin/$(APP_BIN).$(OBUDPST_VER1) && \
	$(STRIP) $(INSTALL_DIR)/usr/bin/$(APP_BIN).$(OBUDPST_VER1)
	ln -sf $(APP_BIN).$(OBUDPST_VER1) $(INSTALL_DIR)/usr/bin/$(APP_BIN)
ifneq ($(strip $(OBUDPST_VER2)),)
	$(MAKE) APP=obudpst-$(OBUDPST_VER2) -f Makefile install
	$(INSTALL) -m 755 obudpst-$(OBUDPST_VER2)/$(APP_BIN) $(INSTALL_DIR)/usr/bin/$(APP_BIN).$(OBUDPST_VER2) && \
	$(STRIP) $(INSTALL_DIR)/usr/bin/$(APP_BIN).$(OBUDPST_VER2)
	ln -sf $(APP_BIN).$(OBUDPST_VER2) $(INSTALL_DIR)/usr/bin/$(APP_PREV_BIN)
endif	
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean: generic_eid_file_clean
	$(MAKE) APP=obudpst-$(OBUDPST_VER1) -f Makefile clean
ifneq ($(strip $(OBUDPST_VER2)),)
	$(MAKE) APP=obudpst-$(OBUDPST_VER2) -f Makefile clean
endif	
	rm -f $(INSTALL_DIR)/usr/bin/$(APP_BIN)*

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean
