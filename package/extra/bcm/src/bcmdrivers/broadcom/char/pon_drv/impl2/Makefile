#
# Makefile for the BCM Pon Drv module
#

obj-$(CONFIG_BCM_PON_DRV) += bcm_pondrv.o


bcm_pondrv-objs := pon_drv.o
bcm_pondrv-objs += pon_drv_nl.o
bcm_pondrv-objs += serdes_debug.o
bcm_pondrv-objs += pon_drv_serdes_util.o
bcm_pondrv-objs += pon_drv_serdes_fixup.o pon_drv_task.o
EXTRA_CFLAGS += $(INC_RDP_FLAGS) -Werror -Wfatal-errors -I$(src)
EXTRA_CFLAGS += -Wno-error=pointer-to-int-cast -Wno-error=int-to-pointer-cast


-include $(TOPDIR)/Rules.make

INSTALL_DONE = $(src)/.done_install
INSTALL_DST_DIR = $(INSTALL_DIR)/bin/
INSTALL_SRC = $(src)/scripts/serdesctrl

clean-files := $(notdir $(INSTALL_DONE))

$(obj)/bcm_pondrv.o: $(INSTALL_DONE)

$(INSTALL_DONE): $(INSTALL_SRC)
	$(Q)install -m 755 $^ $(INSTALL_DST_DIR)
	$(Q)touch $@

# Makefile trick: breaking the following into two rules allows
# the "%.o : %.c" rule (defined earlier by kbuild), to take precidence
%.o : %.o_tmp
	$(Q)mv $< $@

%.o_tmp : %.$(BCMARCH).o_saved
	@echo "Using $<"
	$(Q)cp $< $@

.PHONY: clean

