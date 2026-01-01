
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

APP = trace-cmd-v2.9.5

clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	rm -rf $(APP)

#.PHONY: check_untar_patch_configure conditional_build


check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP) ]; then \
    	echo "Untarring original $(APP) source"; \
	    (tar xkf $(APP).tar.gz 2> /dev/null); \
        fi; 

ifneq ($(strip $(BUILD_TRACE_CMD)),)

conditional_build: check_untar_patch_configure
	cd $(APP); $(MAKE) trace-cmd 
	$(INSTALL) -m 755 $(APP)/tracecmd/trace-cmd  $(INSTALL_DIR)/bin/trace-cmd
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif
