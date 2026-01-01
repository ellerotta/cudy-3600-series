APP = libsodium-1.0.18

check_config: $(APP).tar.gz
	@if [ ! -e $(APP)/Makefile ]; then \
	   tar xf $(APP).tar.gz; \
	   cd $(APP); ./configure \
	    --host=$(TOOLCHAIN_PREFIX) \
            --target=$(TOOLCHAIN_PREFIX) \
	    --prefix=$(BCM_FSBUILD_DIR)/public \
	    --sbindir=$(BCM_FSBUILD_DIR)/public/bin \
	    --libdir=$(BCM_FSBUILD_DIR)/public/lib \
	    CFLAGS=-s\ -Os\ -Wno-strict-aliasing; \
	 @echo "$(APP) is configured"; \
	 cd ..; \
	fi;

install: check_config
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install

clean:
	-[ ! -e $(APP)/Makefile ] || make -C $(APP) uninstall
	rm -rf $(APP)
