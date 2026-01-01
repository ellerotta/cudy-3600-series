LIB :=  libbcm_sslconf.so
OBJS := bcm_sslconf.o


default: install

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common


HEADER_INSTALL_DIR = $(BCM_FSBUILD_DIR)/public/include
HEADER_FILE = bcm_sslconf.h
CONFIG_DIR = $(BCM_FSINSTALL_DIR)/etc/openssl
CONFIG_FILE = sslconf.xml
LIB_INSTALL_DIR := $(BCM_FSBUILD_DIR)/public/lib
# normally, the copy from LIB_INSTALL_DIR to FINAL_LIB_INSTALL_DIR is done
# by Bcmbuild.mk, but we don't have a Bcmbuild.mk in this directory yet.
FINAL_LIB_INSTALL_DIR := $(BCM_FSINSTALL_DIR)/lib


ifeq ("$(strip $(DO_BUILD_OPENSSL))$(strip $(DO_BUILD_EXPAT))", "yy")
COND_BUILD_BCM_SSLCONF := 1
endif

ifeq ("$(strip $(BUILD_BRCM_BDK))$(strip $(BUILD_BRCM_OPENWRT))", "yy")
COND_BUILD_BCM_SSLCONF := 1
endif

#
# Public libs are only allowed to include header files from the
# public directory.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BCM_FSBUILD_DIR)/public/include

ALLOWED_LIB_DIRS := /lib:/public/lib


LIBS := -lexpat

ifeq ($(strip $(BRCM_USER_SSP)),y)
LIBS   += $(SSP_LIBS)
CFLAGS += $(SSP_TYP_COMPILER_OPTS)
endif

# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors


ifeq ($(strip $(COND_BUILD_BCM_SSLCONF)),)
all install:
	@echo "Skipping $(LIB) (not configured)"
else

$(HEADER_INSTALL_DIR)/$(HEADER_FILE): $(LIB)
	mkdir -p $(HEADER_INSTALL_DIR)
	$(INSTALL) -p -m 444 $(HEADER_FILE) $(HEADER_INSTALL_DIR)

$(CONFIG_DIR)/$(CONFIG_FILE): $(LIB)
	mkdir -p $(CONFIG_DIR)
	$(INSTALL) -p -m 444 $(CONFIG_FILE) $(CONFIG_DIR)

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$@ -o $@ $(OBJS) -Wl,--no-whole-archive $(BCM_LIB_PATH) $(LIBS)

all: $(LIB)

install: all $(HEADER_INSTALL_DIR)/$(HEADER_FILE) $(CONFIG_DIR)/$(CONFIG_FILE) sanity_check
	mkdir -p $(LIB_INSTALL_DIR)
	$(INSTALL) -p -t $(LIB_INSTALL_DIR) $(LIB)
	mkdir -p  $(FINAL_LIB_INSTALL_DIR)
	cp -upf $(LIB_INSTALL_DIR)/$(LIB) $(FINAL_LIB_INSTALL_DIR)

endif


clean:
	rm -f *.o *.d $(LIB)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)
	rm -f $(HEADER_INSTALL_DIR)/$(HEADER_FILE)
	rm -rf $(CONFIG_DIR)
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)

shell:
	bash -i
