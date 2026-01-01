EXE := ndisc6

all install: conditional_build 


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


IS_GCC5_ABOVE := 1
ifeq ($(strip $(DESKTOP_LINUX)),y)
MIN_GCC_VERSION = "8"
GCC_VERSION := $(shell $(CROSS_COMPILE)gcc -dumpversion)
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
IS_GCC5_ABOVE := $(shell expr "$(GCC_VERSION)" ">=" "5")
CFLAGS += $(BCM_LD_FLAGS)
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
CFLAGS += -Wno-format-truncation -Wno-format-overflow
endif
endif

EXE_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/sbin
FINAL_EXE_INSTALL_DIR := $(INSTALL_DIR)/bin

export CFLAGS


ifeq "$(IS_GCC5_ABOVE)" "1"
ifneq ($(and $(strip $(BUILD_ANY_CMS_IPV6)),$(strip $(BUILD_PURE181_PROFILES))),)
conditional_build:
	$(MAKE) -f Makefile install
	mkdir -p $(FINAL_EXE_INSTALL_DIR)
	cp -p $(EXE_INSTALL_DIR)/$(EXE) $(FINAL_EXE_INSTALL_DIR)
else
conditional_build: 
	@echo "skipping $(EXE) (not configured)"
endif
else
conditional_build: 
	@echo "skipping $(EXE) (DESKTOP_LINUX with old gcc)"
endif


clean:
	rm -rf $(FINAL_EXE_INSTALL_DIR)/$(EXE)
	-$(MAKE) -f Makefile clean

shell:
	bash -i
