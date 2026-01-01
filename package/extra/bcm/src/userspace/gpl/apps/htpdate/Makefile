
check_config: $(APP).tar.gz
	if [ ! -e $(APP)/Makefile ]; then \
		echo "Untarring htpdate source ..." ; \
		(tar zxvf $(APP).tar.gz 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		(for PATCH in patches/* ; \
		do \
			patch -p1 -b -N -s -d $(APP) < $$PATCH ; \
		done) \
	fi

install: check_config
	$(MAKE) -C $(APP) CC=$(CC)

clean:
	@if [ -e $(APP)/Makefile ]; then \
		cd $(APP); $(MAKE) clean ; \
	fi
	rm -rf $(APP)
