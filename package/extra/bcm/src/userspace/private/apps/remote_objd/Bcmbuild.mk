EXE    := remote_objd

all install: conditional_build


ifeq ($(BCM_MODULAR_BUILD),)
# Old way: infer location of make.common based on pwd.
  CURR_DIR := $(shell pwd)
  BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
  BUILD_DIR:=$(word 1, $(BUILD_DIR))
  include $(BUILD_DIR)/make.common
else
  # New Modular Build way: EXT_BUILD_DIR must be set.
  # Also point BUILD_DIR to EXT_BUILD_DIR
  BUILD_DIR := $(EXT_BUILD_DIR)
  include $(EXT_BUILD_DIR)/make.common
endif


ARCH                  := $(PROFILE_ARCH)
EXE_INSTALL_DIR       := $(BCM_FSBUILD_DIR)/private/bin
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BUILD_DIR)/userspace/private/include

ifneq ($(strip $(BUILD_DBUS)),)
ALLOWED_INCLUDE_PATHS += -I$(BCM_FSBUILD_DIR)/public/lib/glib-2.0/include \
                         -I$(BCM_FSBUILD_DIR)/public/include/glib-2.0 \
                         -I$(BCM_FSBUILD_DIR)/public/include/gio-unix-2.0
endif

ALLOWED_LIB_DIRS := /lib:/usr/lib:/private/lib:/public/lib:/public/lib64:/public/lib32

export ARCH CFLAGS BCM_LD_FLAGS BCM_LIB_PATH EXE_INSTALL_DIR
export BCM_RPATH_LINK_OPTION CMS_COMMON_LIBS
export BUILD_DBUS BUILD_UBUS BCM_COND_HAVE_UBUS

# Final location of EXE for system image.  Only the BRCM build system needs to
# know about this.
FINAL_EXE_INSTALL_DIR := $(BCM_FSINSTALL_DIR)/bin

# remote_objd is used by BDK sysmgmt, BDK tr69 component, and BDK usp component
ifneq ($(strip $(BUILD_BDK_SYSTEM_MANAGEMENT)),)
COND_BUILD_REMOTE_OBJD := 1
endif

ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
ifneq ($(strip $(BUILD_TR69C))$(strip $(BUILD_TR69C_SSL)),)
COND_BUILD_REMOTE_OBJD := 1
endif
ifneq ($(strip $(BUILD_USP)),)
COND_BUILD_REMOTE_OBJD := 1
endif
endif

ifneq ($(strip $(COND_BUILD_REMOTE_OBJD)),)
conditional_build:
	mkdir -p objs
	$(MAKE) -C objs -f ../Makefile install
	mkdir -p $(FINAL_EXE_INSTALL_DIR)
	cp -p $(EXE_INSTALL_DIR)/$(EXE) $(FINAL_EXE_INSTALL_DIR)
else
conditional_build:
	@echo "Skipping $(EXE) (not configured)"
endif


clean:
	rm -f $(FINAL_EXE_INSTALL_DIR)/$(EXE)
	-mkdir -p objs
	-$(MAKE) -C objs -f ../Makefile clean
	rm -rf objs


CONSUMER_RELEASE_BINARYONLY_PREPARE: delete_source

# apps do not need to support multi-arch
save_binaries:
ifneq ($(wildcard objs),)
	-$(MAKE) -C objs -f ../Makefile save_binaries
endif

delete_source: save_binaries
	-$(MAKE) -f Makefile delete_source
	rm -rf objs


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."
