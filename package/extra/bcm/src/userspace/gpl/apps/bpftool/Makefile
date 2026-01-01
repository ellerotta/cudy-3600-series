all dynamic install: conditional_build

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

.PHONY: conditional_build

ifneq ($(strip $(BRCM_BUILD_BPFTOOL)),)
conditional_build: 
	@$(MAKE) -C$(KERNEL_DIR)/tools/bpf/bpftool install \
		prefix=$(BCM_FSBUILD_DIR) bash_compdir=$(BCM_FSBUILD_DIR)/shared \
		CROSS_COMPILE=$(CROSS_COMPILE) EXTRA_CFLAGS="-I$(BCM_FSBUILD_DIR)/public/include -I$(BCM_FSBUILD_DIR)/gpl/lib$(BCM_INSTALL_SUFFIX_DIR)/include " \
		EXTRA_LDFLAGS="-L$(BCM_FSBUILD_DIR)/public/lib$(BCM_INSTALL_SUFFIX_DIR) -L$(BCM_FSBUILD_DIR)/gpl/lib$(BCM_INSTALL_SUFFIX_DIR)/lib"
	@install -m 755 $(BCM_FSBUILD_DIR)/sbin/bpftool $(INSTALL_DIR)/sbin

else
conditional_build:
	@echo "skipping bpftool (not configured)"

endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	@$(MAKE) -C $(KERNEL_DIR)/tools/bpf/bpftool clean
	@rm -f $(BCM_FSBUILD_DIR)/sbin/bpftool $(INSTALL_DIR)/sbin/bpftool

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

