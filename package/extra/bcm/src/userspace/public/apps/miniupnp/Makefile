#APP = miniupnpd-2.3.3

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

IPTABLES_PATH = $(BUILD_DIR)/userspace/gpl/apps/iptables/$(shell find ../../../gpl/apps/iptables/ -type d -name iptables-* | rev | cut -d/ -f1 | rev)
CFLAGS := $(CFLAGS) -I$(BCM_FSBUILD_DIR)/public/include -I$(BCM_FSBUILD_DIR)/gpl/include 
LDFLAGS := $(LDFLAGS) $(BCM_LD_FLAGS) -L$(BCM_FSBUILD_DIR)/public/lib
export CFLAGS LDFLAGS

check_config: $(APP).tar.gz
	if [ ! -e $(APP)/Makefile.linux ]; then \
		echo "Untarring miniupnpd source ..." ; \
		(tar xf $(APP).tar.gz 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		patch -p1 -b -s -d$(APP) < $(APP).patch ; \
		echo "Running configure on $(APP)" ; \
		cd $(APP); ./configure --iptablespath=$(IPTABLES_PATH) --firewall=iptables ; \
		[ -e $(IPTABLES_PATH)/libiptc/.libs/libip4tc.o ] || ln -s $(IPTABLES_PATH)/libiptc/libip4tc.o  $(IPTABLES_PATH)/libiptc/.libs/libip4tc.o; \
	fi

install: check_config
	$(MAKE) -C $(APP) -f Makefile.linux -j1

clean:
	@if [ -e $(APP)/Makefile ]; then \
		cd $(APP); $(MAKE) clean ; \
	fi
	rm -rf $(APP)
