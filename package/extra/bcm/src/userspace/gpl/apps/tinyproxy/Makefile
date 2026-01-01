APP = tinyproxy-1.10.0

APPCFLAGS += -I$(BCM_FSBUILD_DIR)/gpl/include -I$(BCM_FSBUILD_DIR)/public/include

check_config: $(APP).tar.gz
	if [ ! -e $(APP)/Makefile ]; then \
		echo "Untarring tinyproxy source ..." ; \
		(tar xzf $(APP).tar.gz 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		(for PATCH in patches/* ; \
		do \
			patch -p1 -b -N -s -d $(APP) < $$PATCH ; \
		done); \
        cd $(APP); ./autogen.sh; \
        echo "Checking configure on $(APP)/configure"; \
	        if [ -e configure ]; then \
	            echo "Running configure on $(APP)"; \
	            ./configure --host=$(TOOLCHAIN_PREFIX) \
	            --target=$(TOOLCHAIN_PREFIX) CFLAGS="$(APPCFLAGS)" LIBS="-lssl -lcrypto" LDFLAGS="-L$(BCM_FSBUILD_DIR)/gpl/lib -L$(BCM_FSBUILD_DIR)/public/lib"; \
	        fi; \
	fi

install: check_config
	$(MAKE) -C $(APP)

clean:
	@if [ -e $(APP)/Makefile ]; then \
		cd $(APP); $(MAKE) clean ; \
	fi
	rm -rf $(APP)
