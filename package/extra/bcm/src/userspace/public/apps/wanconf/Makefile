EXE := wanconf

OBJS := wanconf.o wan_type_sensing.o wanconf_rdpa.o wanconf_psp.o wanconf_mgmt.o libwanconf.o
CFLAGS += -Werror -Wfatal-errors
CFLAGS += -DKERNELVER=\"${LINUX_VER_STR}\"
ifeq ($(BRCM_PON_WAN_TYPE_AUTO_DETECT),y)
CFLAGS += -DPON_WAN_TYPE_AUTO_DETECT
endif


# Try to order the libs from highest level libs to lowest level libs.
LIBS = -lrdpactl -lbdmf

ifneq ($(strip $(BUILD_PMD)),)
CFLAGS += -DPMD_JSON_LIB
LIBS += -L$(BCM_FSBUILD_DIR)/private/lib -lpmd -L$(BCM_FSBUILD_DIR)/public/lib -ljson-c
PMD_LIB_RPATH = $(CMS_OPTION_RPATH)
endif

ifneq ($(strip $(BUILD_BDK_SYSTEM_MANAGEMENT)),)
CFLAGS +=-DSUPPORT_BDK_SYSTEM_MANAGEMENT
endif

# If we are building a BDK image with either DBus or UBus, build ZBus, which
# is a wrapper to DBus and UBus.
ifneq ($(strip $(BUILD_BRCM_BDK)),)
ifneq ($(strip $(BUILD_DBUS))$(strip $(BUILD_UBUS))$(strip $(BCM_COND_HAVE_UBUS)),)
CFLAGS += -DSUPPORT_ZBUS
OBJS += wanconf_zbus.o
LIBS += -ljson-c

ifneq ($(strip $(BUILD_DBUS)),)
CFLAGS += -DMESSAGE_BUS_DBUS
OBJS += wanconf_dbus.o
LIBS += -lbcm_zbus_intf -lbcm_dbus_intf
LIBS += -ldbus-1 -lgio-2.0 -lglib-2.0 -lgmodule-2.0 -lgobject-2.0 -lgthread-2.0 -lpcre2-8
LIBS += -lz -lexpat -lffi -lpthread -lrt
else
CFLAGS += -DMESSAGE_BUS_UBUS
OBJS += wanconf_ubus.o
LIBS += -lubus -lubox
endif

endif  # (BUILD_DBUS || BUILD_UBUS || BCM_COND_HAVE_UBUS)
endif  # BUILD_BRCM_BDK

ifneq ($(strip $(BUILD_PMD)),)
CFLAGS += -DSUPPORT_PMD
endif	

ifneq ($(filter-out 6888 6813 68880 6837,${BRCM_CHIP}),)
CFLAGS += -DSUPPORT_EPON_AE
endif

LIBS += -lcms_util
ifneq ($(or $(strip $(BUILD_BRCM_BDK)),$(strip $(BUILD_BRCM_CMS))),)
LIBS += -lcms_msg
endif
LIBS += -lbcm_flashutil -lbcm_boardctl -lbcm_util -lsys_util -lgen_util -lrt

all: $(EXE)

install: all
	mkdir -p $(EXE_INSTALL_DIR)
	install -p -m 755 -t $(EXE_INSTALL_DIR) $(EXE)
	mkdir -p $(INSTALL_DIR)/etc/init.d
	mkdir -p $(INSTALL_DIR)/etc/rc3.d
	install $(mdir)/scripts/wanconf.sh $(INSTALL_DIR)/etc/init.d
	(cd $(INSTALL_DIR)/etc/rc3.d; rm -f S85wanconf; ln -s ../init.d/wanconf.sh S85wanconf)
ifeq ($(strip $(RELEASE_BUILD)),)
	install $(mdir)/scripts/xrnd_init.sh $(INSTALL_DIR)/etc/init.d
	(cd $(INSTALL_DIR)/etc/rc3.d; rm -f S95xrndinit; ln -s ../init.d/xrnd_init.sh S95xrndinit)
endif	


$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) $(CMS_RPATH_OPTION) $(CMS_LIB_PATH) $(PMD_LIB_RPATH) $(LIBS)



clean:
	rm -f *.o *.d $(EXE)
	rm -f $(EXE_INSTALL_DIR)/$(EXE)
	rm -f $(INSTALL_DIR)/etc/init.d/wanconf.sh
	rm -f $(INSTALL_DIR)/etc/rc3.d/S85wanconf
	rm -f $(INSTALL_DIR)/etc/init.d/xrnd_init.sh
	rm -f $(INSTALL_DIR)/etc/rc3.d/S95xrndinit


.PHONY: clean


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
