KERNEL_DIR ?= $(CURDIR)/../kernel/linux
BRCM_BOARD ?= bcm963xx
TARGETS_DIR ?= $(CURDIR)/../targets
SHARED_DIR ?= $(CURDIR)/../shared
BRCMDRIVERS_DIR ?= $(CURDIR)/../bcmdrivers
INC_BRCMDRIVER_PUB_PATH ?= $(BRCMDRIVERS_DIR)/opensource/include
INC_BRCMSHARED_PUB_PATH ?= $(SHARED_DIR)/opensource/include

# host tools should not use cross compiler
unexport AR AS LD CC CPP CXX NM STRIP SSTRIP OBJCOPY OBJDUMP RANLIB

INCLUDEDIR = -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I.

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	else if [ -x /bin/bash ]; then echo /bin/bash; \
	else echo sh; fi ; fi)
TOPDIR	:= $(shell /bin/pwd)
SSHD_DIR = $(BUILD_DIR)/userspace/public/apps/dropbear/sshd
GENKEY_DIR = $(SSHD_DIR)/genkey
HOST_GENKEY = $(TOPDIR)/host_genkey
CC = gcc
STRIP = strip
RSA_KEY = $(TARGETS_DIR)/fs.src/etc/rsa_host_key
###DSS_KEY = $(TARGETS_DIR)/fs.src/etc/dss_host_key
export SSHD_DIR HOST_GENKEY CC STRIP

#WARNINGS= -Wall

#export DEFS		= -DDEBUG
CFLAGS_NOARCH	= $(DEFS) $(WARNINGS) -O2 -DGNU $(INCLUDEDIR)
CFLAGS		= $(CFLAGS_NOARCH)
ifeq ($(strip $(BUILD_SQUASH_HIGH)),y)
	CFLAGS += -DBUILD_SQUASH_HIGH
endif
ifeq ($(strip $(BUILD_SQUASH_NORMAL)),y)
	CFLAGS += -DBUILD_SQUASH_NORMAL
endif
ifeq ($(strip $(BUILD_SQUASH_LOW)),y)
	CFLAGS += -DBUILD_SQUASH_LOW
endif
ifeq ($(strip $(BRCM_IKOS)),y)
	CFLAGS += -DCONFIG_BRCM_IKOS
endif
export CFLAGS

################################################################################
# TOOLS SET PER LAYER
# When introduce new host tools, please add them in proper layer's TOOLS SET,
# so that the dependency is guaranteed.
#
################################################################################
# tools set for bootloader
TOOLS_SET_BOOTLOADER = build_secbtutils perlmods

# tools set for kernelspace
TOOLS_SET_KERNELSPACE =

# tools set for userspace
TOOLS_SET_USERSPACE = build_cms2bbf build_beep_tools scons build_mlibtool build_meson \
                      build_ninja

# tools set for image generation
TOOLS_SET_IMAGE = build_mkfs build_imageutil build_lz4cmp build_lzop build_vfbnize dtc_utils

# tools set for common tools that are used by multiple layers
TOOLS_SET_COMMON = build_cmplzma

################################################################################
# Full TOOLS SET
################################################################################
BUILD_SET = $(TOOLS_SET_BOOTLOADER) $(TOOLS_SET_KERNELSPACE) $(TOOLS_SET_USERSPACE) \
            $(TOOLS_SET_IMAGE) $(TOOLS_SET_COMMON)

################################################################################
# Overall Targets
################################################################################
.PHONY: $(BUILD_SET) nvram_files
all: $(BUILD_SET)

hosttools_bootloader:  $(TOOLS_SET_BOOTLOADER)
hosttools_kernelspace: $(TOOLS_SET_KERNELSPACE)
hosttools_userspace:   $(TOOLS_SET_USERSPACE)
hosttools_image:       $(TOOLS_SET_IMAGE)
hosttools_common:      $(TOOLS_SET_COMMON)

############################################################################
# Individual Targets
############################################################################
build_imageutil: bcmImageBuilder addvtoken BcmFsTag gencrc32 nvram_files bcmComboMaker

bcmComboMaker:
	$(CC) $(CFLAGS) -o bcmComboMaker bcmComboMaker.c

bcmImageBuilder: bcmImageBuilder.c
	$(CC) $(CFLAGS) -o bcmImageBuilder bcmImageBuilder.c
addvtoken: addvtoken.c
	$(CC) $(CFLAGS) -o addvtoken addvtoken.c
gencrc32: gencrc32.c
	$(CC) $(CFLAGS) -o gencrc32 gencrc32.c

perlmods:
	$(MAKE) -C CBC
	$(MAKE) -C CPAN

ifeq ($(strip $(BUILD_ALLJOYN)),y)
scons:
	$(MAKE) -C scons
else
scons:
	@echo "skipping $@ (not configured)"
endif


# Openplat (formerly beep)
ifeq ($(strip $(BUILD_MODSW_EE)),y)
build_beep_tools:
ifneq ($(wildcard beep/Makefile),)
	@echo "building beep tools ..."
	$(MAKE) -j1 -C beep -f Makefile
endif
else
build_beep_tools:
	@echo "skipping $@ (not configured)"
endif


nvram_files:
	$(CC) -U__cplusplus $(CFLAGS) -E createimg_binary_convert.c  |sed -e 's/^#.*//' -e 's/.*__attribute__.*unused.*//' > nvram.h && \
	grep "for_script" nvram.h > nvram_defaults.h

build_mkfs: build_xz-5.0.3 mksquashfs build_mtdutils

dtc_utils: 
	$(MAKE) -C $(DTC_DIR)

ifeq ($(strip $(BRCM_KERNEL_ROOTFS)),squashfs)
build_mkfs: mk_ext4fs
ifneq ($(strip $(BRCM_KERNEL_AUXFS_JFFS2)),)
build_mkfs: mkjffs2
endif
endif

ifeq ($(strip $(BRCM_KERNEL_ROOTFS)),all)
build_mkfs: mkjffs2 build_mtdutils mk_ext4fs
endif

DTC_DIR=$(TOPDIR)/dtc

SQUASHFS_DIR=$(TOPDIR)/squashfs_4.2
XZ_DIR=$(TOPDIR)/xz-5.0.3
SECBT_DIR=$(TOPDIR)/SecureBootUtils
export LzmaAlone=$(TOPDIR)/lzma457/CPP/7zip/Compress/LZMA_Alone

JFFS2_DIR = $(TOPDIR)/jffs2
RELATIVE_MTD_UTILS_DIR = mtd-utils-2.0.2
MTD_UTILS_DIR = $(TOPDIR)/$(RELATIVE_MTD_UTILS_DIR)
VFBNIZE_DIR = $(TOPDIR)/vflash_utils/vfbnize

LZ4_DIR = $(TOPDIR)/lz4-r127

untar_lzma457:
	if [ ! -e lzma457/.untar_complete ]; then \
	(tar xkfj lzma457.tar.bz2 2> /dev/null || true; touch lzma457/.untar_complete); \
	fi

mklzma_cmd: untar_lzma457
	@echo building lzma host tool ...
	$(MAKE) -C $(LzmaAlone) -f makefile.gcc && \
	cp -f $(LzmaAlone)/lzma lzmacmd

build_lzop: local_install/lzop

local_install/lzop:
	mkdir -p local_install
	tar xzf lzop-1.04.tar.gz
	cd lzop-1.04 && ./configure && $(MAKE) && cp src/lzop ../local_install/lzop

config_xz-5.0.3:
	if [ ! -e xz-5.0.3 ]; then \
	echo Untarring xz-5.0.3 source...; \
	(tar xkfj xz-5.0.3.tar.bz2 2> /dev/null || true); \
	fi && \
	if [ ! -e xz-5.0.3/Makefile ]; then \
	(cd xz-5.0.3; ./configure --enable-shared=no --disable-assembler) ;\
	fi

build_xz-5.0.3: config_xz-5.0.3
	echo Building xz-5.0.3 ...
	$(MAKE) -C $(XZ_DIR) -f Makefile

build_secbtutils: perlmods
	@echo Building secure boot utils ...
	$(MAKE) -C $(SECBT_DIR) -f Makefile

mksquashfs: build_xz-5.0.3
	@echo building mksqushfs host tool ...
	$(MAKE) -C $(SQUASHFS_DIR) && \
	mv $(SQUASHFS_DIR)/mksquashfs mksquashfs

mkcramfs:  cramfs/mkcramfs.cpp $(LZMAOBJS)
	@if [ ! -e /usr/include/zlib.h ]; then \
          echo *****You need to install zlib-devel package to build mkcramfs! ; \
          echo *****You may find it in your Linux distribution CDs. ; \
	  exit ;\
        fi
	g++ $(CFLAGS) -I$(KERNEL_DIR)/include -c cramfs/mkcramfs.cpp && \
	g++  -o mkcramfs mkcramfs.o $(LZMAOBJS) -lz -lm

mkjffs2:
	@echo building mkjffs2 host tool ...
	$(MAKE) -C $(JFFS2_DIR) && \
	mv jffs2/mkfs.jffs2 .

build_cmplzma: cmplzma

cmplzma: mklzma_cmd cmplzma.cpp
	g++ $(CFLAGS) -c cmplzma.cpp && \
	g++  -o cmplzma cmplzma.o -lm

untar_lz4hc:
	if [ ! -e $(LZ4_DIR) ]; then \
	echo Untarring lz4-r127 source...; \
	(tar xzf r127.tar.gz 2> /dev/null || true); \
	fi

build_lz4cmp: untar_lz4hc lz4comp.c

	gcc lz4comp.c $(LZ4_DIR)/lib/lz4hc.c -o lz4cmp.out -I $(LZ4_DIR)/lib  && \
	chmod +x lz4cmp.out

build_mtdutils:
	@if [ ! -e $(MTD_UTILS_DIR) -o mtd-utils-2.0.2+43369d4.tar.bz2 -nt $(MTD_UTILS_DIR)/Makefile ]; then \
	rm -rf $(MTD_UTILS_DIR) &&  \
	rm -rf mtd-utils &&  \
	echo Untarring mtd-utils source... &&  \
	tar xf mtd-utils-2.0.2+43369d4.tar.bz2 && \
	ln -s $(RELATIVE_MTD_UTILS_DIR) mtd-utils &&  \
	(cd $(MTD_UTILS_DIR)  && ./configure --without-xattr --disable-tests)  \
	fi && \
	echo building mtd-utils host tool ... && \
	$(MAKE) -C $(MTD_UTILS_DIR) BUILDDIR=$(MTD_UTILS_DIR) WITHOUT_XATTR=1

export MK_EXT4FS_DIR = $(TOPDIR)/make_ext4fs
export MK_EXT4FS_TAR = make_ext4fs_android_4.3_src.tar.gz

$(MK_EXT4FS_DIR)/core:
	cd $(MK_EXT4FS_DIR); (tar -xzf $(MK_EXT4FS_TAR) 2> /dev/null || true)

mk_ext4fs: $(MK_EXT4FS_DIR)/core
	cd $(MK_EXT4FS_DIR); $(MAKE); $(MAKE) install

build_cms2bbf:
	$(MAKE) -C cms2bbf

build_vfbnize:
	@echo building vfbnize host tool ...
	$(MAKE) -C $(VFBNIZE_DIR) && \
	mv $(VFBNIZE_DIR)/vfbnize .

$(LZMAOBJS): %.o: %.cpp
	g++ -c $(CFLAGS) $^ -o $@



# In theory, we could unconditionally build meson and ninja.
# But meson requires python3 and ninja takes about 15seconds to build, so there
# might be a few build issues to fix.  For now, only build if systemd/glib/lxc is enabled.
ifneq ($(strip $(BUILD_MESON_NINJA)),)

build_meson:
	$(MAKE) -C meson

build_ninja:
	$(MAKE) -C ninja

else

build_meson:

build_ninja:

endif


######################################################################
#
# This section deals with doxygen
#
# Note that if you want to change the settings in the config file,
# (Doxyfile), you should do it in this makefile.
# Any modifications to the config file itself will be
# wiped out next time you make the build_doxygen target.
#
######################################################################

doxygen:
	mkdir -p doxygen
	tar -jxf doxygen.tar.bz2 -C doxygen
	cd doxygen; cmake -G "Unix Makefiles"; $(MAKE);


build_doxygen: doxygen
	cd doxygen; \
	cd bin; rm -f Doxyfile; \
	./doxygen -g > /dev/null; \
	sed -i 's,\(PROJECT_NAME[ \t]*=\),\1 CMS,' Doxyfile; \
	sed -i 's,\(OUTPUT_DIRECTORY[ \t]*=\),\1 $(BUILD_DIR)/docs/doxygen,' Doxyfile; \
	sed -i 's,\(JAVADOC_AUTOBRIEF[ \t]*=\)[ \tA-Z]*,\1 YES,' Doxyfile; \
	sed -i 's,\(OPTIMIZE_OUTPUT_FOR_C[ \t]*=\)[ \tA-Z]*,\1 YES,' Doxyfile; \
	sed -i 's,\(WARN_NO_PARAMDOC[ \t]*=\)[ \tA-Z]*,\1 YES,' Doxyfile; \
	sed -i 's,\(ENUM_VALUES_PER_LINE[ \t]*=\)[ \t0-9]*,\1 1,' Doxyfile; \
	sed -i 's,\(INPUT[ \t]*=\),\1 $(BUILD_DIR)/userspace/public/include,' Doxyfile; \
	sed -i 's,\(INPUT[ \t]*=\),\1 $(BUILD_DIR)/userspace/private/include,' Doxyfile; \
	sed -i 's,\(INPUT[ \t]*=\),\1 $(BUILD_DIR)/userspace/private/libs/cms_core,' Doxyfile; \
	sed -i 's,\(RECURSIVE[ \t]*=\)[ \tA-Z]*,\1 YES,' Doxyfile; \
	sed -i 's,\(GENERATE_LATEX[ \t]*=\)[ \tA-Z]*,\1 NO,' Doxyfile; \
	sed -i 's,\(PREDEFINED[ \t]*=\)[ \tA-Z]*,\1 MDM_SHARED_MEM CMS_MEM_DEBUG,' Doxyfile;

# if we want to generate documentation for the unittests
#sed -i 's,\(INPUT[ \t]*=\),\1 $(BUILD_DIR)/unittests,' Doxyfile;

build_mlibtool:
	$(MAKE) -C mlibtool
######################################################################
#
# Cleaning rules
#
######################################################################

clean:
	rm -f bcmImageBuilder addvtoken mksquashfs mkcramfs mkfs.jffs2 lzmacmd \
		cmplzma $(HOST_GENKEY) BcmFsTag gencrc32 nvram.h nvram_defaults.h \
		bcmComboMaker vfbnize
	rm -fr *.o core
	rm -fr $(GENKEY_DIR)/*.o
	$(MAKE) -C $(SQUASHFS_DIR) clean
	$(MAKE) -C $(JFFS2_DIR) clean
	$(MAKE) -C $(DTC_DIR) clean
	$(MAKE) -C $(MK_EXT4FS_DIR) clean
	$(MAKE) -C $(VFBNIZE_DIR) clean
	if [ -e $(XZ_DIR)/Makefile ]; then $(MAKE) -C $(XZ_DIR) distclean; fi
	$(MAKE) -C $(SECBT_DIR) clean
	if [ -e $(LzmaAlone)/makefile.gcc ]; then $(MAKE) -C $(LzmaAlone) -f makefile.gcc clean; fi
	find lzma457 -type f ! -name comp.cc -exec rm '-f' '{}' \;
	$(MAKE) -C libcreduction clean
	rm -rf  $(MTD_UTILS_DIR)
	rm -rf  mtd-utils
	rm -rf  mtd-utils-20090606 # simplify migration from older version
	if [ -e lz4-r127 ];then rm -rf lz4-r127; fi
	if [ -e lz4cmp.out ];  then rm lz4cmp.out; fi
	if [ -e libelf ]; then rm -rf libelf ; fi
	if [ -e prelink ]; then rm -rf prelink; fi
	if [ -e beep ]; then $(MAKE) -C beep clean; fi
	$(MAKE) -C mlibtool clean
	$(MAKE) -C meson clean
	$(MAKE) -C ninja clean
	rm -rf lzop-1.04/

clean_doxygen:
	rm -fr doxygen


$(TOPDIR)/prelink/src/execstack:
	mkdir -p libelf && \
	tar -xzpf libelf-0.8.13.tar.gz -C libelf --strip-components=1  && \
	cd libelf && mkdir -p $(TOPDIR)/libelf/prefix \
	&& ./configure --prefix=$(TOPDIR)/libelf/prefix && \
	$(MAKE) all install;
	tar -xpjf prelink.tar.bz2 && \
	cd prelink &&  \
	LD_LIBRARY_PATH=$(TOPDIR)/libelf/prefix/lib/ LDFLAGS="-L$(TOPDIR)/libelf/prefix/lib/" CPPFLAGS="-I$(TOPDIR)/libelf/prefix/include -I$(TOPDIR)/libelf/prefix/include/libelf -I$(TOPDIR)/libelf/prefix/" CFLAGS="-I$(TOPDIR)/libelf/prefix/include -I$(TOPDIR)/libelf/prefix/include/libelf" ./configure --enable-static  && cd src && $(MAKE) execstack


build_execstack: $(TOPDIR)/prelink/src/execstack

