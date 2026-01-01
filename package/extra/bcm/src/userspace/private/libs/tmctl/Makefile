#
# In most cases, you only need to modify this first section.
#
LIB = libtmctl.so

OBJS = tmctl_api.o

OBJS += tmctl_api_trace.o


all install: conditional_build

clean: generic_clean
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)


ifeq ($(BCM_MODULAR_BUILD),)
# Old way: infer location of make.common based on pwd.
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BCM_TARGET_PATH)/make.common
INSTALL_DIR := $(CURR_DIR)/install_dir
else
# New Modular Build way: EXT_BUILD_DIR must be set.
# Also point BUILD_DIR to EXT_BUILD_DIR
BUILD_DIR := $(EXT_BUILD_DIR)
include $(EXT_BUILD_DIR)/make.common
endif


ifneq ($(strip $(BUILD_RDPACTL)),)
OBJS += tmctl_ethsw.o
OBJS += tmctl_api_runner.o
OBJS += tmctl_bdmf_rdpa.o
ifneq ($(strip $(BRCM_PLATFORM_RDP_PRV)),)
OBJS += tmctl_pkt_based.o
endif
ifneq ($(strip $(BRCM_RUNNER_QOS_MAPPER)),)
OBJS += tmctl_pkt_based.o
endif
endif

ifneq ($(strip $(BUILD_ARCHERCTL)),)
OBJS += tmctl_archer.o tmctl_ethsw.o tmctl_sysporttm.o
OBJS += tmctl_api_archer.o
endif

ifneq ($(strip $(BUILD_DSL)),)
OBJS += tmctl_xtm.o 
endif

ifneq ($(strip $(BUILD_TMCTL)),)
conditional_build: generic_private_lib_install
else
conditional_build:
	@echo "skipping $(LIB) (not configured)"
endif

#
# Private apps and libs are allowed to include header files from the
# private and public directories
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BCM_FSBUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
                         -I$(BCM_FSBUILD_DIR)/bcmdrivers/include \
                         -I$(BUILD_DIR)/userspace/public/libs/jsonc/obj \
                         -I$(BUILD_DIR)/userspace/public/libs/jsonc/json-c-0.16 \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/public/include \
                         -I$(SHARED_DIR)/opensource/include/bcm963xx


ifneq ($(strip $(BUILD_RDPACTL)),)
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/rdp/drivers/rdpa_gpl/include/ \
                         -I$(BUILD_DIR)/rdp/drivers/bdmf/framework/ \
                         -I$(BUILD_DIR)/rdp/firmware/common/ \
                         -I$(PROJECT_DIR)/target/bdmf/system \
                         -I$(PROJECT_DIR)/target/bdmf/system/sim \
                         -I$(PROJECT_DIR)/target/rdpa_gpl \
                         -I$(PROJECT_DIR)/target/rdpa_user \
                         -I$(PROJECT_DIR)/target/rdpa_user/include \
                         -I$(PROJECT_DIR)/target/rdpa_user/include/autogen/autogen
endif


# For customers who manually dlopen libtmctl.so, we need to explicitly
# list the library dependencies so they are listed as DT_NEEDED in this lib
ifneq ($(strip $(BUILD_RDPACTL)),)
LOCAL_LDFLAGS :=  -L$(INSTALL_DIR)/lib -lrdpactl -lbdmf
endif

ifneq ($(strip $(BUILD_EPONCTL)),)
CFLAGS += -DSUPPORT_FLUSH
LOCAL_LDFLAGS += -L$(INSTALL_DIR)/lib -leponctl
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/userspace/private/libs/eponctl
endif

ifneq ($(strip $(BUILD_ARCHERCTL)),)
LOCAL_LDFLAGS := -L$(INSTALL_DIR)/lib -larcher
CFLAGS += -DBUILD_ARCHERCTL
endif

ifneq ($(strip $(BUILD_DSL)),)
LOCAL_LDFLAGS += -latmctl
CFLAGS += -DBUILD_DSL
ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63158_63146_),_$(strip $(BRCM_CHIP))_)
CFLAGS += -DXTM_PORT_SHAPING
endif
ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63146_),_$(strip $(BRCM_CHIP))_)
CFLAGS += -DXTM_HW_ACCEL_MIRRORING
endif
endif

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_4912_63146_6813_),_$(strip $(BRCM_CHIP))_)
CFLAGS += -DRDPA_XRDP_US_DS_AGNOSTIC -DXRDP
endif

ifneq ($(strip $(BUILD_ETHSWCTL)),)
LOCAL_LDFLAGS += -lethswctl
CFLAGS += -DBUILD_ETHSWCTL
endif

ifneq ($(strip $(BCM_RDPA_TM_C)),)
CFLAGS += -DTM_C_CODE
endif

LOCAL_LDFLAGS += $(TARGET_LDFLAGS)

#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
#
$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive $(LOCAL_LDFLAGS)


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
