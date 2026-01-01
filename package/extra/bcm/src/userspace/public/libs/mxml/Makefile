# Makefile to build libmxml

APP = mxml-3.3.1

check_config: $(APP).tar.gz
	@if [ ! -e $(APP)/Makefile ]; then \
	   tar xf "$(APP).tar.gz"; \
	   cd "$(APP)" && ./configure \
		--host="$(TOOLCHAIN_PREFIX)" \
		--target="$(TOOLCHAIN_PREFIX)" \
		--prefix="$(BCM_FSBUILD_DIR)/public" \
		--enable-shared; \
	 @echo "$(APP) is configured"; \
	 cd ..; \
	fi;

install: check_config
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install

clean:
	-[ ! -e $(APP)/Makefile ] || make -C $(APP) uninstall
	rm -rf $(APP)
