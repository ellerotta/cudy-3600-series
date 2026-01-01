all dynamic install: conditional_build

VERSION := 1.4.1
APP := protobuf-c-$(VERSION)
LIB = libprotobuf-c
PATCH := protobuf-c-$(VERSION).patch

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

PREFIX := $(BCM_FSBUILD_DIR)/public
PREFIX_HOST := $(BCM_FSBUILD_DIR)/host
CONFIG_COMPLETE_FILE = config_complete

ifneq ($(strip $(BUILD_PROTOBUF_C)),)
conditional_build: $(INSTALL_DIR)/$(LIB)

# Depend on source tar file
# Use $(APP)/Makefile to mark extraction is completed
$(APP)/Makefile: $(APP).tar.gz
	@if [ ! -e $(APP)/Makefile ]; then \
	    echo "Extract original $(APP) source"; \
		(tar xf $(APP).tar.gz 2> /dev/null); \
	    patch -p0 -b -s -d$(APP) < $(PATCH) ; \
	fi

$(APP)/$(CONFIG_COMPLETE_FILE): $(APP)/Makefile
	(cd $(APP); \
	./configure \
		--prefix=$(PREFIX) \
		--host=$(TOOLCHAIN_PREFIX) \
		--includedir=$(PREFIX)/include \
		--disable-protoc \
		--disable-static \
		$(BCM_BLT32_FLAGS) || exit 1);
	touch $@

$(INSTALL_DIR)/$(LIB): $(APP)/$(CONFIG_COMPLETE_FILE) sanity_check
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install
	mkdir -p $(INSTALL_DIR)/lib/public/
	cp -d $(PREFIX)/lib/$(LIB).so* $(INSTALL_DIR)/lib/public/

host/$(APP)/configure: $(APP).tar.gz
	@echo "Extract original $(APP) source"
	mkdir -p host
	cd host/ && \
		(tar xf ../$(APP).tar.gz 2> /dev/null)
	@patch -p0 -b -s -dhost/$(APP) < $(PATCH)
	@touch $@

host/$(APP)/Makefile: host/$(APP)/configure
	export PKG_CONFIG_PATH=$(BCM_FSBUILD_DIR)/public/lib/pkgconfig; \
	cd host/$(APP); \
	./configure \
		--prefix=$(PREFIX_HOST) \
		--includedir=$(PREFIX_HOST)/include \
		--disable-static \
		CC=gcc CXX=g++ AR=ar LD=ld \
		CPPFLAGS=-I$(PREFIX_HOST)/include \
		LDFLAGS=-L$(PREFIX_HOST)/lib \
		PROTOC=$(PREFIX_HOST)/bin/protoc

ifneq ($(strip $(BUILD_GRPC)),)
$(APP)/$(CONFIG_COMPLETE_FILE): $(PREFIX_HOST)/bin/protoc-gen-c

$(PREFIX_HOST)/bin/protoc-gen-c: host/$(APP)/Makefile
	$(MAKE) -C host/$(APP) protobuf-c/protobuf-c.pb.cc
	$(MAKE) -C host/$(APP) protoc-c/protoc-gen-c
	mkdir -p $(PREFIX_HOST)/bin
	@cp -f host/$(APP)/protoc-c/protoc-gen-c $@
endif
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(PREFIX)/lib/$(LIB).*
	rm -f $(INSTALL_DIR)/lib/public/$(LIB).*;
	rm -rf $(PREFIX)/include/protobuf-c
	rm -rf $(PREFIX)/include/google/protobuf-c
	rm -rf $(APP);
	rm -f $(PREFIX_HOST)/bin/protoc-gen-c
	rm -rf host

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
