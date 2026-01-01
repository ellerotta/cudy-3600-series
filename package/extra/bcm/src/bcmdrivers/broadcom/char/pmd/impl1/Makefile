#
# Makefile for dummy PMD driver
#

pmd-objs = dummy.o
obj-$(CONFIG_BCM_PMD) += pmd.o

$(obj)/pmd.o: $(INSTALL_DIR)/etc/pmd_fw_img.bin

clean:
	rm -f core *.o *.a *.s .*.cmd *.ko

.PHONY: clean

$(INSTALL_DIR)/etc/pmd_fw_img.bin:
	$(Q)install $(src)/fw_img/pmd_fw_img.bin $(INSTALL_DIR)/etc
