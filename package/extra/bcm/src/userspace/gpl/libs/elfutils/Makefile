
all dynamic install: conditional_build

ifeq ($(V),1)
  Q =
else
  Q = @
endif

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

LIB := elfutils-0.187
LIBNAME := libelf
STATIC_LIBS := libelf.a
SHARED_LIBS = libelf*.so*

# BRCM_SUPPORTS_MULTIARCH_BUILD
ARCH                  := $(PROFILE_ARCH)
LOCAL_BUILD_SUBDIR    := objs/$(ARCH)
LIBELF_BUILD_PREFIX   := $(BCM_FSBUILD_DIR)/gpl
ALLOWED_INCLUDE_PATHS := -I . \
                         -I $(BCM_FSBUILD_DIR)/include \
                         -I $(BCM_FSBUILD_DIR)/gpl/include \
                         -I $(BCM_FSBUILD_DIR)/public/include

export ARCH CFLAGS 

# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(BCM_FSINSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)


.PHONY: conditional_build

ifneq ($(strip $(BUILD_ELFUTILS)),)
conditional_build: build
else
conditional_build: 
	@echo "skipping $(LIB) (not configured)"
endif

$(LIB):
	$(Q)mkdir $(LIB)

$(LIB)/.buildtarg_untar: $(LIB).tar.bz2 | $(LIB)
	@echo "Untarring original $(LIB) source";
	$(Q)tar xfj $(LIB).tar.bz2 2> /dev/null || true;
	$(Q)touch $@

$(LIB)/.buildtarg_patch: $(LIB)/.buildtarg_untar $(wildcard $(LIB).patch) 
	$(Q)if [ -e $(APP).patch ]; then \
	    echo "Applying patches to $(LIB)"; \
	    patch -p1 -b -s -f -d$(LIB) < $(LIB).patch; \
	    if [ $$? -ne 0 ]; then \
	         exit 1; \
	    fi; \
	else \
	    echo "No patch file for $(LIB), not patching"; \
	fi
	$(Q)touch $@

# Only build the libs, which are dual GPLv2/LGPLv3.
# Do not build or install the utilities, which are GPLv3.
# (also remove src and tests, which contain GPLv3 files to prevent accidental use.)
build: $(LIB)/.buildtarg_patch
	mkdir -p $(LOCAL_BUILD_SUBDIR)
	cd $(LOCAL_BUILD_SUBDIR); ../../$(LIB)/configure --host=$(TOOLCHAIN_PREFIX) \
				--disable-debuginfod --with-zlib \
				--disable-libdebuginfod \
				--prefix=$(LIBELF_BUILD_PREFIX) \
				--libdir=$(LIBELF_BUILD_PREFIX)/lib$(BCM_INSTALL_SUFFIX_DIR) \
				LDFLAGS="-L$(BCM_FSBUILD_DIR)/public/lib$(BCM_INSTALL_SUFFIX_DIR) " \
				CFLAGS="-I$(BCM_FSBUILD_DIR)/public/include";
	mv $(LIB)/src/Makefile.in Makefile.in.save; rm -rf $(LIB)/src; mkdir $(LIB)/src; mv Makefile.in.save $(LIB)/src/Makefile.in;
	mv $(LIB)/tests/Makefile.in Makefile.in.save; rm -rf $(LIB)/tests; mkdir $(LIB)/tests; mv Makefile.in.save $(LIB)/tests/Makefile.in;
	cd $(LOCAL_BUILD_SUBDIR); $(MAKE) -C lib && $(MAKE) -C libelf install;
	mkdir -p $(LIBELF_BUILD_PREFIX)/lib$(BCM_INSTALL_SUFFIX_DIR)/pkgconfig
	cp -d $(LOCAL_BUILD_SUBDIR)/config/$(LIBNAME).pc $(LIBELF_BUILD_PREFIX)/lib$(BCM_INSTALL_SUFFIX_DIR)/pkgconfig
	mkdir -p $(FINAL_LIB_INSTALL_DIR);
	cp -d $(LIBELF_BUILD_PREFIX)/lib$(BCM_INSTALL_SUFFIX_DIR)/$(SHARED_LIBS) $(FINAL_LIB_INSTALL_DIR)
	cp -f $(CURR_DIR)/$(LIB)/version.h $(LIBELF_BUILD_PREFIX)/include/elfutils/

clean:
	rm -rf objs
	rm -f Makefile.in.save
	rm -rf $(LIB)
	rm -f $(LIBELF_BUILD_PREFIX)/lib/pkgconfig/$(LIBNAME).pc
	rm -f $(LIBELF_BUILD_PREFIX)/lib/aarch64/pkgconfig/$(LIBNAME).pc
	rm -f $(LIBELF_BUILD_PREFIX)/lib/libelf*
	rm -f $(LIBELF_BUILD_PREFIX)/lib/aarch64/libelf*
	rm -f $(LIBELF_BUILD_PREFIX)/include/libelf.h
	rm -rf $(LIBELF_BUILD_PREFIX)/include/elfutils
	rm -f $(BCM_FSINSTALL_DIR)/lib/libelf*
	rm -f $(BCM_FSINSTALL_DIR)/lib/aarch64/libelf*
	

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean


check1:
	@echo INSTALL_DIR is $(INSTALL_DIR);

mk_patch:
	@echo building patch for $(APP)
	@echo "  creating backup at $(APP).bk"
	cp -r $(APP) $(APP).bk;
	cd $(APP); make distclean;
	mv $(APP) $(APP).new
	tar xf $(APP).tar.bz2 2> /dev/null || true
	diff -du $(APP) $(APP).new > $(APP).patch || true
	rm -rf $(APP).new
	@echo "...done"


