
libjsonc: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


ifeq ($(strip $(DESKTOP_LINUX)),y)
CFLAGS=$(BCM_LD_FLAGS) 
CXXFLAGS=$(BCM_LD_FLAGS) 
LDFLAGS=$(BCM_LD_FLAGS)
export CFLAGS CXXFLAGS LDFLAGS
endif

ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS+=$(SSP_TYP_COMPILER_OPTS)
LDFLAGS+=-L$(BCM_FSINSTALL_DIR)/lib $(SSP_LIBS)
endif

export LINUX_VER_STR TOOLCHAIN_PREFIX

# Use exact same tarball that we use in the Openwrt 21.02.1 build
APP=json-c-0.17
REQUIRED_CMAKE_VERSION := 3.5.1

ifneq ($(strip $(BUILD_LIBJSONC)),)
conditional_build: all
else ifneq ($(strip $(BCM_COND_HAVE_LIBJSONC)),)
conditional_build:
	@echo "libjsonc will be provided externally (not built by native BDK build system)"
else
conditional_build:
	@echo "skipping libjsonc (not configured)"
endif


$(APP)/cmake-configure: $(APP)-nodoc.tar.gz
	@echo "Untar $(APP)-nodoc.tar.gz"
	tar -xkzf $(APP)-nodoc.tar.gz
	@echo "Applying jsonc patches..."
	for i in patches/[0-9]*.patch ; do echo "Applying $$i" ; patch -p1 -b -s -d$(APP) < $$i ; done
	touch $@

check_cmake_version: $(APP)/cmake-configure
	@echo "Checking cmake version mini-version $(REQUIRED_CMAKE_VERSION):"
	@$(HOSTTOOLS_DIR)/scripts/checkver.pl -e cmake -r $(REQUIRED_CMAKE_VERSION) || $(COND_FAIL);

do_cmake: check_cmake_version
	mkdir -p obj; cd obj; cmake ../$(APP) -DDISABLE_WERROR=ON -DCMAKE_INSTALL_PREFIX=$(BCM_FSBUILD_DIR)/public/ \
		-DBUILD_STATIC_LIBS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_LIBDIR=$(BCM_FSBUILD_DIR)/public/lib

all: do_cmake
	mkdir -p $(BCM_FSINSTALL_DIR)/lib/
	$(MAKE) -C obj
	$(MAKE) -C obj install
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libjson*.so* $(BCM_FSINSTALL_DIR)/lib/

clean:
	-[ ! -e obj/Makefile ] || ($(MAKE) -C obj clean; $(MAKE)  -C obj uninstall)
	rm -f $(BCM_FSINSTALL_DIR)/lib/libjson*.so*
	rm -rf obj; rm -rf $(APP);

bcm_dorel_distclean: distclean

distclean: 
	rm -rf $(APP)

shell:
	bash -i


