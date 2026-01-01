openthread: conditional_build

OPENTHREAD_ZIP       := openthread-1f1a826.zip
OPENTHREAD_PATCH_DIR := ../brcm_patches
APP                  := openthread-1f1a826d1891fc99d14822abc0a1de257e9c6e9b

CURR_DIR  := $(shell pwd)
BUILD_DIR := $(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR := $(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

ifeq ($(strip $(DESKTOP_LINUX)),y)
ifeq "$(BCM_LD_FLAGS)" "-m32"
BCM_HOST := i386-linux
else
BCM_HOST := $(TOOLCHAIN_PREFIX)
endif
else
BCM_HOST := $(TOOLCHAIN_PREFIX)
endif

ifneq ($(strip $(BUILD_OPENTHREAD)),)

BUILD_FLAGS := FULL_LOGS=1                        \
	DUA=1                                     \
	LINK_METRICS_INITIATOR=1                  \
	LINK_METRICS_SUBJECTS=1                   \
	CSL_RECEIVER=1                            \
	HOST=$(TOOLCHAIN_PREFIX)                  \
	ResultPath=$(BCM_FSBUILD_DIR)/public      \
	AbsTopResultDir=$(BCM_FSBUILD_DIR)/public \
	TargetTuple=                              \
	READLINE=

conditional_build: all
else
conditional_build:
	@echo "skipping OpenThread (not configured)"
endif

$(APP)/.untar: $(OPENTHREAD_ZIP)
	unzip -o $(OPENTHREAD_ZIP); cd $(APP) && \
		(for p in $(OPENTHREAD_PATCH_DIR)/*.patch; do \
			patch -p1 -s < "$$p"; \
		done)
	touch $@
	echo "openthread is untarred"

$(APP)/.bootstrap_done: $(APP)/.untar
	cd $(APP) && ./bootstrap
	touch $@

all: $(APP)/.bootstrap_done
	mkdir -p $(INSTALL_DIR)/lib/public/
	$(MAKE) $(strip $(BUILD_FLAGS)) -C $(APP) -f src/posix/Makefile-posix
	$(MAKE) $(strip $(BUILD_FLAGS)) -C $(APP) -f src/posix/Makefile-posix stage
	$(INSTALL) $(BCM_FSBUILD_DIR)/public/bin/ot-cli $(INSTALL_DIR)/bin
	$(INSTALL) $(BCM_FSBUILD_DIR)/public/bin/spi-hdlc-adapter $(INSTALL_DIR)/bin

clean:
	@-[ -d $(APP) ] && $(RM) -r $(APP) ||:

bcm_dorel_distclean: distclean

distclean: clean

shell:
	bash -i
