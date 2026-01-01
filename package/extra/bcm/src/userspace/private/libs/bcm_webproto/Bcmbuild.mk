LIB := libbcm_webproto.so

all install: conditional_build


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


ARCH                  := $(PROFILE_ARCH)
LIB_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/lib
HEADER_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/private/include
ALLOWED_INCLUDE_PATHS := -I . \
                         -I $(BCM_FSBUILD_DIR)/public/include \
                         -I $(BCM_FSBUILD_DIR)/private/include \
                         -I $(BUILD_DIR)/userspace/public/include  \
                         -I $(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I $(BUILD_DIR)/userspace/private/include  \
                         -I $(BUILD_DIR)/userspace/private/include/$(OALDIR)


ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS += $(SSP_TYP_COMPILER_OPTS)
endif

# Set flags if user wants to link with OpenSSL
ifneq ($(strip $(BUILD_TR69C_SSL)),)
CFLAGS += -DUSE_SSL -DORIGINAL_OPENSSL -I$(BCM_FSBUILD_DIR)/public/include
endif

# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors


export ARCH CFLAGS LIB_INSTALL_DIR HEADER_INSTALL_DIR


# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)


# could be more specific about when to build this lib (needed by all flavors of
# tr69c and obuspa), but for now, just build it in CMS or BDK mode (but not in
# BASE_SHELL).  The lib is not very big anyways.
ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_BCM_WEBPROTO := 1
endif
ifneq ($(strip $(BUILD_BRCM_BDK)),)
COND_BUILD_BCM_WEBPROTO := 1
endif

ifeq ($(strip $(COND_BUILD_BCM_WEBPROTO)),1)

conditional_build:
	mkdir -p objs
	$(MAKE) -C objs -f ../Makefile install
	mkdir -p  $(FINAL_LIB_INSTALL_DIR)
	cp -p $(LIB_INSTALL_DIR)/$(LIB) $(FINAL_LIB_INSTALL_DIR)

else

conditional_build:
	@echo "Skipping $(LIB) (not configured)"

endif


clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile clean
	rm -rf objs


CONSUMER_RELEASE_BINARYONLY_PREPARE: binaryonly_prepare

binaryonly_prepare:
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile binaryonly_prepare
	rm -rf objs


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
