# Makefile to build obudpst
CONFIG_COMPLETE_FILE = bcm_config_complete

check_config: $(APP).tar.gz
	if [ ! -e $(APP)/Makefile ]; then \
		echo "Untarring obudpst source ..." ; \
		(tar xzf $(APP).tar.gz 2> /dev/null || true) ; \
	fi

$(APP)/Makefile: check_config
	if [ ! -e $(APP)/$(CONFIG_COMPLETE_FILE) ]; then \
		cp -rf $(APP)-patch/* $(APP)/; \
		cp -f ./brcm_udpst_offload.c $(APP)/; \
		cp -f ./brcm_udpst_offload.h $(APP)/; \
		export PKG_CONFIG_LIBDIR=$(BCM_FSBUILD_DIR)/public/lib;  \
		export PKG_CONFIG_PATH=$(BCM_FSBUILD_DIR)/public/lib/pkgconfig; \
		cd $(APP); \
		touch $(CONFIG_COMPLETE_FILE); \
		cmake \
		-DCMAKE_RANLIB=$(RANLIB) \
		-DCMAKE_NM:FILEPATH=$(NM) \
		-DCMAKE_OBJCOPY:FILEPATH=$(OBJCOPY) \
		-DCMAKE_LINKER:FILEPATH="$(LD)" \
		-DCMAKE_AR:FILEPATH=$(AR) \
		-DCMAKE_PREFIX_PATH:PATH="$(BCM_FSBUILD_DIR);$(BCM_FSBUILD_DIR)/public" \
		-DCMAKE_C_FLAGS:STRING="$(BCM_CFLAGS_STRING)" \
		-DCMAKE_EXE_LINKER_FLAGS:STRING="$(BCM_EXE_LINKER_FLAGS_STRING)" \
		-DCMAKE_SKIP_RPATH:BOOL=YES \
		-DDISABLE_INT_TIMER=ON \
		-DHAVE_GSO=OFF || exit $?; \
	fi

install: $(APP)/Makefile
	cd $(APP); \
	$(MAKE) VERBOSE=1 || exit $?;

clean:
	@if [ -e $(APP)/Makefile ]; then \
		rm -rf $(APP); \
	fi
