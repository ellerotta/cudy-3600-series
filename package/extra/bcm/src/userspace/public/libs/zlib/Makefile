all dynamic install: conditional_build
 
LIB = zlib-1.3

# BRCM_SUPPORTS_MULTIARCH_BUILD
# Note this lib supports multi-arch build.  When we are compiling a 64 bit
# kernel, this Makefile will be called twice, once for a 64bit build and once
# for a 32bit build.  See autodetect and userspace/makefile.modsw.autogen.
# So to build this lib, you should not run make from this directory, instead
# do this from the top:
# make -C userspace public/libs/zlib

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
 
ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS := CFLAGS="$(BCM_LD_FLAGS)"
endif
 
unexport CC GCC AR RANLIB NM
 
ifneq ($(strip $(BUILD_ZLIB)),)
conditional_build: build
else
conditional_build: 
	@echo "skipping $(LIB) (not configured)"
endif
 
ARCH                 := $(PROFILE_ARCH)
LOCAL_BUILD_SUBDIR   := objs/$(ARCH)

# This is the "base" install path for the zlib makefiles.  It is used for
# libs, header files, and pkgconfig files.  Note that we do not specify
# multi-arch suffix here.  Set libdir directly below.
ZLIB_BUILD_PREFIX    := $(BCM_FSBUILD_DIR)/public
 
# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(BCM_FSINSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)


clean:
	rm -rf objs
	rm -rf $(LIB)
	rm -f $(ZLIB_BUILD_PREFIX)/include/zconf.h
	rm -f $(ZLIB_BUILD_PREFIX)/include/zlib.h
	rm -f $(ZLIB_BUILD_PREFIX)/lib/pkgconfig/zlib.pc
	rm -f $(ZLIB_BUILD_PREFIX)/lib/arm/pkgconfig/zlib.pc
	rm -f $(ZLIB_BUILD_PREFIX)/lib/aarch64/pkgconfig/zlib.pc
	rm -f $(ZLIB_BUILD_PREFIX)/lib/libz.a
	rm -f $(ZLIB_BUILD_PREFIX)/lib/libz.so*
	rm -f $(ZLIB_BUILD_PREFIX)/lib/arm/libz.a
	rm -f $(ZLIB_BUILD_PREFIX)/lib/arm/libz.so*
	rm -f $(ZLIB_BUILD_PREFIX)/lib/aarch64/libz.a
	rm -f $(ZLIB_BUILD_PREFIX)/lib/aarch64/libz.so*
	rm -f $(BCM_FSINSTALL_DIR)/lib/libz.so*
	rm -f $(BCM_FSINSTALL_DIR)/lib/arm/libz.so*
	rm -f $(BCM_FSINSTALL_DIR)/lib/aarch64/libz.so*

# The next line is a hint to to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
 
$(LIB)/configure: $(LIB).tar.gz
	tar zxf $(LIB).tar.gz; patch -p1 -b -s -d$(LIB) < $(LIB).patch
	@echo "$(LIB) is untarred"
 
build: $(LIB)/configure
	mkdir -p $(LOCAL_BUILD_SUBDIR) 
	cd $(LOCAL_BUILD_SUBDIR); CROSS_PREFIX=$(CROSS_COMPILE) $(BCM_BLT32_FLAGS) ../../$(LIB)/configure --prefix=$(ZLIB_BUILD_PREFIX) --libdir=$(ZLIB_BUILD_PREFIX)/lib$(BCM_INSTALL_SUFFIX_DIR)
	mkdir -p $(FINAL_LIB_INSTALL_DIR)
	cd $(LOCAL_BUILD_SUBDIR); $(MAKE) install
	cp -d $(ZLIB_BUILD_PREFIX)/lib$(BCM_INSTALL_SUFFIX_DIR)/libz.so* $(FINAL_LIB_INSTALL_DIR)
 
 
.PHONY: build check_untar_configure conditional_build

shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
