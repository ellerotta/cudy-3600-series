#
# Makefile for eapfwd module
#

eapfwd-objs   := bcm_eapfwd.o

ifeq ($(strip $(BUILD_HND_NIC)),)
EXTRA_CFLAGS += -DBCM_EAPFWD_DHD
endif # BUILD_HND_NIC

ifneq ($(strip $(CONFIG_BCM_PKTFWD)),)
EXTRA_CFLAGS += -DBCM_PKTFWD

ifneq ($(strip $(CONFIG_BCM_EAPFWD)),)
EXTRA_CFLAGS += -DBCM_EAPFWD -DBCM_NBUFF_PKT

ifneq ($(strip $(BUILD_HND_NIC)),)
# Enable credit based Host Flow Control for NIC mode
ifneq ($(strip $(CONFIG_BCM_PKTFWD_FLCTL)),)
EXTRA_CFLAGS += -DBCM_PKTFWD_FLCTL
endif
endif # BUILD_HND_NIC
endif # CONFIG_BCM_EAPFWD
endif # CONFIG_BCM_PKTFWD

obj-$(CONFIG_BCM_EAPFWD) += eapfwd.o

EXTRA_CFLAGS += -I. -I$(src)/../include/
EXTRA_CFLAGS += -I$(src)/../../wl/bcm9$(BRCM_CHIP)/main/src/include
EXTRA_CFLAGS += -I$(src)/../../wl/shared/impl1/
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)

EXTRA_CFLAGS += -Werror -Wall -Wfatal-errors

clean:
	rm -f core *.ko *.o *.a *.s
