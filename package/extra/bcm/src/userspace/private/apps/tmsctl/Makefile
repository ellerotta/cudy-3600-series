
# GLOBAL_RELEASE_SCRIPT_NOTOUCH

default: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
export BUILD_DIR

include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_TMS)),)

conditional_build: make_links tmsctl

else

conditional_build:
	@echo "Skipping TMS (TMS not configured)"

endif


# Currently the tms driver part always use impl1.
# Defined as CONFIG_BCM963XX_TMS_IMPL in hostTools/scripts/defconfig-brcm.template.
make_links:
	tmsimpl=impl1; \
	rm -f tmsctl/bsp; ln -sfn $(BRCMDRIVERS_DIR)/broadcom/char/tms/$$tmsimpl/bsp tmsctl/bsp ; \
	rm -f tmsctl/nci; ln -sfn $(BRCMDRIVERS_DIR)/broadcom/char/tms/$$tmsimpl/nci tmsctl/nci

.PHONY: make_links

tmsctl: make_links
	$(MAKE) -C tmsctl

.PHONY: tmsctl

clean:
	$(MAKE) -C tmsctl clean

binaryonly_dist_clean:
	$(MAKE) -C tmsctl $@

.PHONY: binaryonly_dist_clean

shell:
	bash -i

