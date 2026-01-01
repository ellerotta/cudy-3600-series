#
# Makefile for the Epon Stack driver.
#
-include $(src)/../make.epon

obj-$(CONFIG_BCM_EPON_STACK) += bcmepon.o

ifneq (,$(findstring 6858B,$(BRCM_FULL_CHIP_NAME)))
CHIP_NAME:=BCM6858B0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6846")
CHIP_NAME:=BCM6846A0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6856A0")
CHIP_NAME:=BCM6856A0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6878")
CHIP_NAME:=BCM6878A0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"63158B0")
CHIP_NAME:=BCM63158
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6855")
CHIP_NAME:=BCM6855A0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6888A0")
CHIP_NAME:=BCM6888A0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6813A0")
CHIP_NAME:=BCM6813A0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"68880B0")
CHIP_NAME:=BCM68880B0
else ifeq ("$(BRCM_FULL_CHIP_NAME)" ,"6837A0") 
# 6837A0 has same driver files to 68880B0
CHIP_NAME:=BCM68880B0
endif

DRIVER_FILES:=$(shell cd $(src) && find driver/$(CHIP_NAME) -type f -name '*.c')

bcmepon-objs += $(patsubst %.c, %.o, $(DRIVER_FILES))

bcmepon-objs += AeDriver.o
bcmepon-objs += mac_drv_ae.o
bcmepon-objs += phy_drv_int_ae.o
bcmepon-objs += epon_ae_sfp.o
bcmepon-objs += Lif.o
bcmepon-objs += EponMac.o
ifeq ("$(CONFIG_BCM96858)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM96856)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM963158)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM96855)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM96888)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM96813)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM968880)" ,"y")
bcmepon-objs += Xif.o
endif
ifeq ("$(CONFIG_BCM96837)" ,"y")
bcmepon-objs += Xif.o
endif
bcmepon-objs += ../shared/EponTypes.o
bcmepon-objs += ../shared/EponDriver.o

ifeq ("$(EPON_NORMAL_MODE)" ,"y")
    bcmepon-objs += PonManager.o
    bcmepon-objs += EponCmds.o
    bcmepon-objs += Nco.o
    # Files shared with other EPON platform
    bcmepon-objs += ../shared/OntDirector.o
    bcmepon-objs += ../shared/EponUser.o
    bcmepon-objs += ../shared/EponTimer.o
    bcmepon-objs += ../shared/PonConfigDb.o
    bcmepon-objs += ../shared/PonMgrFec.o
    bcmepon-objs += ../shared/rdpa_epon.o
    bcmepon-objs += ../shared/Stream.o
    bcmepon-objs += ../shared/Mpcp.o
    bcmepon-objs += ../shared/OntmMpcp.o
    bcmepon-objs += ../shared/CtcStats.o
    bcmepon-objs += ../shared/CtcAlarms.o
    bcmepon-objs += ../shared/PonMgrEncrypt.o
    bcmepon-objs += ../shared/EponCtrl.o
    bcmepon-objs += ../shared/EponFrame.o
    bcmepon-objs += ../shared/Holdover.o
    bcmepon-objs += ../shared/EponStats.o
    bcmepon-objs += ../shared/OptCtrl.o
    bcmepon-objs += ../shared/OpticCmds.o
    bcmepon-objs += ../shared/OsAstMsgQ.o
    bcmepon-objs += ../shared/ProtSwitch.o
    bcmepon-objs += ../shared/EponRogueDrv.o
    bcmepon-objs += ../shared/PowerSaving.o
    bcmepon-objs += ../shared/mac_drv_epon.o
    ifeq ("$(CONFIG_EPON_CLOCK_TRANSPORT)" ,"y")
    bcmepon-objs += ../shared/ClockTransport.o
    endif
    ifneq ($(strip $(CONFIG_COMPAT)),)
    bcmepon-objs += ../shared/EponCtrlCompat.o
    endif
endif

EXTRA_CFLAGS += $(EXTRA_CFLAGS_BDMF)
EXTRA_CFLAGS += -DUSE_BDMF_SHELL
EXTRA_CFLAGS += -DPHYS_ADDR_64BIT

ifneq ($(strip $(CONFIG_BCM_PMD)),)
    EXTRA_CFLAGS += -DCONFIG_BCM_PMD
endif

EXTRA_CFLAGS += -I$(src)/../shared/include
EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(src)/include/platform
EXTRA_CFLAGS += -I$(src)/driver/$(CHIP_NAME)/hal
EXTRA_CFLAGS += -I$(src)/driver/$(CHIP_NAME)/ru_gen
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/phy
EXTRA_CFLAGS += -I$(INC_RDPA_MW_PATH) 
EXTRA_CFLAGS += $(INC_RDP_FLAGS)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl_ext/impl1/include/
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl/impl1/include/
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/synce_holdover/impl1

ifeq ("$(CONFIG_BCM_PON)" ,"y")
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/egphy
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv
endif


EXTRA_CFLAGS += -DBDMF_DRIVER_GPL_LAYER

EXTRA_CFLAGS += -Werror -Wall -Wno-array-bounds -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
EXTRA_CFLAGS += -DLINUX_KERNEL -DINCLUDE_LOGS -DDBA_DEBUG_STATISTICS
EXTRA_CFLAGS += -DPLL_PPM_ADJ

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_6858_6846_6878_),)
EXTRA_CFLAGS += -DIDLE_PACKET_TX_SUPPORT
endif

#support 10G.
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_6858_6856_63158_6855_6888_6813_68880_6837_),)
EXTRA_CFLAGS += -DEPON_10G_COMMON
endif

#support 10G. 32 llids for Bidirectional(0-31)
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_6858_6856_63158_),)
EXTRA_CFLAGS += -DEPON_10G_BIDIRECT
endif

#support 10G. 16 llids for Bidirectional (0-15), 16 llids for Downstream only(16-31)
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_6855_6888_6813_68880_6837_),)
EXTRA_CFLAGS += -DEPON_10G_DSONLY
endif

#only support 1G.
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_6846_6878_),)
EXTRA_CFLAGS += -DEPON_1G_ONLY
endif

#support AE autoneg ability.
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_6855_6856_6878_),)
EXTRA_CFLAGS += -DAE_AN_ABILITY_SUPPORT
endif

#support allow_overlp_b2b setting
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_68880_6837_),)
EXTRA_CFLAGS += -DOVERLP_B2B_SUPPORT
endif

-include $(TOPDIR)/Rules.make

clean:
	rm -rf `find . -name '*.[o,d]'`
	rm -rf `find ../shared/ -name '*.[o,d]'`
	rm -rf `find . -name '*~'`
	rm -rf `find ../shared/ -name '*~'`
	rm -rf `find . -name '*.ko'`
	rm -rf `find ../shared/ -name '*.ko'`
	rm -rf `find . -name '.*.cmd'`
	rm -rf `find ../shared/ -name '.*.cmd'`
	rm -rf $(MODULE_NAME).mod.c modules.order Module.symvers .tmp_versions

