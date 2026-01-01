# File: bcmdrivers/char/tms/bcm963xx/Makefile
#
# Makefile for the BCM63xx TMS module
#

BCM_TMS_DIR = $(BRCMDRIVERS_DIR)/broadcom/char/tms/$(LN_NAME)

HAVESRC = $(wildcard $(src)/nci/common/*.c)

ifeq ($(strip $(HAVESRC)),)
ifneq ("$(CONFIG_DEBUG_SPINLOCK)","")
$(error "TMS binary is not compatible with CONFIG_DEBUG_SPINLOCK.")
endif
endif

#ToDo: Should co-work with NCOMM to remove -Wno-unused-function and -Wno-switch flags under Linux 5.15.
KRNL_COMMON := -Wno-unused-function -Wno-switch

KRNL_COMMON += $(CFLAGS_KASAN)

KRNL_3264 := -freg-struct-return
ifeq ($(ARCH),arm64)
KRNL_3264 += -D__64BIT__
else
KRNL_3264 += -D__32BIT__
endif

KRNL_CFLAGS := $(KRNL_COMMON) $(KRNL_3264)
BSP_CFLAGS  := -DBCM96X
NCI_CFLAGS  := -DETH_PACKAGE -DNCI_ETH3AH -DNCI_ETH1AG -DNCI_Y1731
NCI_CFLAGS  += -DEXLINUX -DNCIDEBUG -DNCIXLATE -DTMSLINUX_DISABLE_FLUSH

EXTRA_CFLAGS += $(KRNL_CFLAGS) $(BSP_CFLAGS) $(NCI_CFLAGS)
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/bsp/lnx
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/bsp/lnx/bcm96x
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/common
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/eth/3ah
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/eth/1ag
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/eth/drv
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/eth/line
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/eth/y1731
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/port
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/tmp
EXTRA_CFLAGS += -I$(BCM_TMS_DIR)/nci/xlate

obj-$(CONFIG_BCM_TMS) += nciTMSkmod.o

ifeq ($(strip $(HAVESRC)),)

nciTMSkmod-objs += nciTMSkmod_dep.o
obj-$(CONFIG_BCM_TMS) += nciLservices.o
nciLservices-objs := Lservices.o
obj-$(CONFIG_BCM_TMS) += nciexLinuxETH.o
nciexLinuxETH-objs := exLinuxETH.o

else

nciTMSkmod-objs := nci/eth/line/ethPackage.o \
                   nci/eth/line/eth2App.o \
                   nci/eth/line/eth2Drv.o \
                   nci/eth/line/ethOAM.o \
                   nci/eth/line/ethMUX.o \
                   nci/eth/line/ethTStamp.o \
                   nci/eth/1ag/1ag.o \
                   nci/eth/1ag/1agTLV.o \
                   nci/eth/1ag/1agCCM.o \
                   nci/eth/1ag/1agLBM.o \
                   nci/eth/1ag/1agLTM.o \
                   nci/eth/1ag/1agMIPdb.o \
                   nci/eth/y1731/y1731.o \
                   nci/eth/y1731/y1731CCM.o \
                   nci/eth/y1731/y1731AIS.o \
                   nci/eth/y1731/y1731LCK.o \
                   nci/eth/y1731/y1731CSF.o \
                   nci/eth/y1731/y1731MISC.o \
                   nci/eth/y1731/y1731TST.o \
                   nci/eth/y1731/y1731LMM.o \
                   nci/eth/y1731/y1731DMM.o \
                   nci/eth/y1731/y1731EDM.o \
                   nci/eth/y1731/y1731GNM.o \
                   nci/eth/y1731/y1731ALC.o \
                   nci/eth/y1731/y1731SLM.o \
                   nci/eth/y1731/y1731ENG.o \
                   nci/eth/3ah/3ah.o \
                   nci/eth/3ah/3ahDSC.o \
                   nci/eth/3ah/3ahPKT.o \
                   nci/eth/3ah/3ahLBK.o \
                   nci/eth/3ah/3ahTLV.o \
                   nci/eth/3ah/3ahEVT.o \
                   nci/eth/3ah/3ahVAR.o \
                   nci/eth/3ah/3ahMIB.o \
                   nci/port/nciEXLINUX.o \
                   nci/common/nciTimer.o \
                   nci/common/nciPool.o \
                   nci/common/nciVFifo.o \
                   nci/common/nciVer.o \
                   nci/common/nciTINT.o \
                   nci/common/nciCRC.o \
                   nci/common/nciMEM.o \
                   nci/common/nciContext.o \
                   nci/common/nciBrd.o \
                   nci/common/nciDebug.o \
                   nci/xlate/tmsXlateKrnl.o \
                   nci/drv/exlinuxeth/exlinuxeth1AG.o \
                   nci/drv/exlinuxeth/exlinuxeth3AH.o \
                   bsp/lnx/bcm96x/board.o \
                   bsp/lnx/tmsLINUXkmod.o
obj-$(CONFIG_BCM_TMS) += nciLservices.o
nciLservices-objs := exModules/Lservices/Lservices.o
obj-$(CONFIG_BCM_TMS) += nciexLinuxETH.o
nciexLinuxETH-objs := exModules/exLinuxETH/exLinuxETH.o

endif

.DEFAULT:
ifeq ($(strip $(HAVESRC)),)
	$(Q)cp $(obj)/nciTMSkmod.$(BRCM_CHIP).o_saved $(obj)/nciTMSkmod_dep.o
endif

clean:
	$(Q)rm -f `find . -name '*.o'`
	$(Q)rm -f `find . -name '.*.cmd'`
	$(Q)rm -f *.ko *.mod *.mod.c modules.order

