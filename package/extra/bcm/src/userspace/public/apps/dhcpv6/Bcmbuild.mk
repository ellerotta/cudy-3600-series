LIBS=-L$(INSTALL_DIR)/lib -lcms_util -lcms_msg -lbcm_flashutil -lbcm_boardctl -lbcm_util -lsys_util -lgen_util -lrt

APP := dhcpv6

all install: conditional_build


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS += $(SSP_TYP_COMPILER_OPTS)
LDFLAGS := -L$(INSTALL_DIR)/lib $(SSP_LIBS)
endif

CFLAGS += -D_GNU_SOURCE -DSUPPORT_IPV6
LDFLAGS += $(BCM_LD_FLAGS)

# this app is very very old and uses an internal glibc header
# (sys/cdefs.h) which defines the __P macro for portable
# function prototypes for pre-C90 code. When we're compiling
# with MUSL, just define __P(args) as 'args' on the command
# line.
ifeq ($(BRCM_GLIBC),)
	CFLAGS += -D__P\(args\)=args
endif

ARCH=$(PROFILE_ARCH)
APP_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/sbin
FINAL_APP_INSTALL_DIR := $(INSTALL_DIR)/bin
HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/public/include
ALLOWED_INCLUDE_PATHS := -I . \
                         -I $(BUILD_DIR)/userspace/public/include \
                         -I $(HEADER_INSTALL_DIR)

export ARCH CFLAGS LDFLAGS LIBS


ifneq ($(strip $(BUILD_UDHCP)),)
ifneq ($(strip $(BUILD_IPV6)),)
ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)
conditional_build:
	$(MAKE) -f Makefile install
	mkdir -p $(FINAL_APP_INSTALL_DIR)
	cp -p $(APP_INSTALL_DIR)/dhcp6c $(FINAL_APP_INSTALL_DIR)
	cp -p $(APP_INSTALL_DIR)/dhcp6s $(FINAL_APP_INSTALL_DIR)
else
conditional_build: 
	@echo "skipping $(APP) (CMS/BDK not configured)"
endif
else
conditional_build: 
	@echo "skipping $(APP) (ipv6 not configured)"
endif
else
conditional_build:
	@echo "skipping $(APP) (dhcp not configured)"
endif


clean:
	rm -rf $(FINAL_APP_INSTALL_DIR)/dhcp6s
	rm -rf $(FINAL_APP_INSTALL_DIR)/dhcp6c
	-$(MAKE) -f Makefile clean

shell:
	bash -i
