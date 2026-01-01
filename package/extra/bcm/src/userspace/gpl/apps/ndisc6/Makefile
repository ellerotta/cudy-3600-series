VER := 1.0.7
PKG := ndisc6-$(VER)
EXE := ndisc6

check_untar_configure:
	if [ ! -e $(EXE)/$@ ]; then \
		rm -rf $(EXE) ; \
		echo "Untarring source and overrides..." ; \
		mkdir -p $(EXE) ; \
		(tar --strip-components=1 -xjf $(PKG).tar.bz2 -C $(EXE) 2> /dev/null || true) ; \
		if [ "x$(BUILD_BRCM_CMS)" != "x" ] || [ "x$(BUILD_BRCM_BDK)" != "x" ]; then \
			echo "Applying patches to $(PKG)" ; \
			patch -p1 -b -N -s -d$(EXE) < patch.$(EXE)_lowercase ; \
		fi ; \
		cd $(EXE); \
		./configure --host=$(TOOLCHAIN_PREFIX) --prefix=$(BCM_FSBUILD_DIR) \
		--disable-suid-install \
		CFLAGS="$(CFLAGS)" && \
		patch -p1 -b -N -s < ../patch.$(EXE)_nochown && \
		touch $@ ; \
	fi

install: check_untar_configure
	$(MAKE) -C $(EXE)
	$(MAKE) -C $(EXE) install


clean:
	@if [ -e $(EXE)/Makefile ]; then \
		cd $(EXE); $(MAKE) uninstall ; \
	fi
	rm -rf $(EXE)

# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
