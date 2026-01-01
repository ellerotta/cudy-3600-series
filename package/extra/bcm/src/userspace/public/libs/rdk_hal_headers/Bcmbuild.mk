# This dir only installs header files.  No library.
LIB :=

HAL_BASEDIR := halinterface
HAL_TARBALL := halinterface-rdkb-2023q2-dunfell.tar.gz

all install: conditional_build


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/public/include

export HEADER_INSTALL_DIR


ifneq ($(strip $(BUILD_RDK_HAL_HEADERS)),)

PATCH_FILES := patches/dpoe_hal.patch \
               patches/ccsp_hal_ethsw.patch

$(HAL_BASEDIR)/Makefile: $(PATCH_FILES)
	rm -rf $(HAL_BASEDIR)
	mkdir $(HAL_BASEDIR)
	(cd $(HAL_BASEDIR); tar -zxvf ../$(HAL_TARBALL))
	patch $(HAL_BASEDIR)/dpoe_hal.h patches/dpoe_hal.patch
	patch $(HAL_BASEDIR)/ccsp_hal_ethsw.h patches/ccsp_hal_ethsw.patch
	cp Makefile $(HAL_BASEDIR)

conditional_build: $(HAL_BASEDIR)/Makefile
	$(MAKE) -C $(HAL_BASEDIR) -f Makefile install

else

conditional_build:
	@echo "Did not install RDK HAL headers (not configured)"

endif

clean:
	-mkdir -p $(HAL_BASEDIR)
	cp -f Makefile $(HAL_BASEDIR)
	-$(MAKE) -C $(HAL_BASEDIR) -f Makefile clean
	rm -rf $(HAL_BASEDIR)


shell:
	bash -i
