#
# In most cases, you only need to modify this first section.
#

all: build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

APP = lmbench-3.0-a9

CROSS := $(CROSS_COMPILE)
export CROSS

CFLAGS += -Wno-error=unused-parameter -Wno-error -Wno-maybe-uninitialized -Wno-unused-variable -DHAVE_socklen_t -I$(BCM_FSBUILD_DIR)/public/include/tirpc -ltirpc -L$(BCM_FSBUILD_DIR)/public/lib
export CFLAGS

ifneq ($(strip $(BUILD_LMBENCH)),)
build: 
	if [ -d $(APP) ]; then rm -rf $(APP); fi
	-tar xkzf $(APP).tgz 2>/dev/null
	cd $(APP)/src ; patch -p1 -b -s < ../../lmbench.patch; make CC=$(CC) OS=$(TOOLCHAIN_PREFIX)
	mkdir -p $(INSTALL_DIR)/opt/lmbench/bin
	cp -f $(APP)/bin/$(TOOLCHAIN_PREFIX)/* $(INSTALL_DIR)/opt/lmbench/bin

else
build: 
	echo "skipping lmbench (not configured)"

endif

clean: 
	rm -rf $(APP)

# Shell target permits creating an interacive shell for debugging
shell:
	bash -i

distclean: clean

