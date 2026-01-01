

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

APP = sysstat-12.7.2

BINARIES := iostat mpstat pidstat
BINARIES := $(addprefix $(INSTALL_DIR)/bin/,$(strip $(BINARIES)))

clean:
	@if [ -e $(APP)/Makefile ]; then \
	cd $(APP); $(MAKE) distclean ; \
	fi
	-rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	rm -rf $(APP)

.PHONY: conditional_build $(APP)

$(APP)/Makefile: $(APP).tar.gz Makefile
	@echo "Untarring original $(APP) source"
	(tar xvf $(APP).tar.gz 2> /dev/null)
	cd $(APP); ./configure --host=$(TOOLCHAIN_PREFIX) \
		--target=$(TOOLCHAIN_PREFIX) \
		--prefix=$(INSTALL_DIR) \
		--sbindir=$(INSTALL_DIR)/bin \
		--disable-nls \
		--disable-documentation ;
	touch $@

$(BINARIES): $(APP)/Makefile | $(APP)
	install -m 755 $(APP)/$(notdir $@) $@

$(APP): $(APP)/Makefile
	$(MAKE) -C $(APP)


ifneq ($(strip $(BUILD_SYSSTAT)),)
conditional_build: $(BINARIES)
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif
