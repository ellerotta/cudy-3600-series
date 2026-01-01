APP = ncurses-6.4

check_config: $(APP).tar.gz
	@if [ ! -e $(APP)/Makefile ]; then \
	   tar xvf $(APP).tar.gz; \
	   cd $(APP); ./configure \
	    --host=$(TOOLCHAIN_PREFIX) \
	    --prefix= \
	    --exec-prefix= \
	    --with-install-prefix=$(BCM_FSBUILD_DIR)/public \
	    --without-ada \
	    --without-cxx \
	    --without-cxx-binding \
	    --without-manpages \
	    --without-progs \
	    --without-tack \
	    --without-tests \
	    --without-debug \
	    --without-profile \
	    --without-develop \
	    --disable-home-terminfo \
	    --enable-echo \
	    --enable-const \
	    --enable-overwrite \
	    --disable-rpath \
	    --disable-big-core \
	    --with-shared \
	    CFLAGS="-s -Os -Wno-strict-aliasing" \
	    CPPFLAGS="-P"; \
	 @echo "$(APP) is configured"; \
	 cd ..; \
	fi;

install: check_config
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install.libs install.data
	echo "Done building $(APP)"

clean:
	-[ ! -e $(APP)/Makefile ] || make -C $(APP) uninstall
	rm -rf $(APP)
