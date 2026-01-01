#***********************************************************************
#
#  Copyright (c) 2017  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

all: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
DOCKER_INSTALL_DIR=$(INSTALL_DIR)/opt/docker

ifeq ($(strip $(KERNEL_ARCH)),aarch64)
DOCKER_BINARIES = docker-binaries/aarch64
else
DOCKER_BINARIES = docker-binaries/armhf
endif
START_DOCKER = S80docker
BUILD_MODSW_DOCKEREE_BR_IP_ESCAPED := $(subst /,\/,$(BUILD_MODSW_DOCKEREE_BR_IP))

ifneq ($(strip $(BRCM_CFG_USER_PARTITIONS)),)
BUILD_MODSW_DOCKEREE_ROOT := \/local\/docker_fs\/etc\/docker
BUILD_MODSW_DOCKEREE_DRV := overlay2
else
BUILD_MODSW_DOCKEREE_ROOT := \/var\/lib\/docker
BUILD_MODSW_DOCKEREE_DRV := vfs
endif

.PHONY: conditional_build

ifneq ($(strip $(BUILD_MODSW_DOCKEREE)),)
conditional_build: conf
	cp -rf $(DOCKER_BINARIES)/* $(INSTALL_DIR)/bin/docker/
	install -m 0755 docker.sh $(INSTALL_DIR)/etc/init.d/
	(cd $(INSTALL_DIR)/etc/rc3.d; rm -f $(START_DOCKER); ln -s ../init.d/docker.sh $(START_DOCKER))
	(cd $(INSTALL_DIR)/etc/; rm -rf docker; ln -s $(BUILD_MODSW_DOCKEREE_ROOT) docker)
	(cd $(INSTALL_DIR)/etc/init.d; sed -i -e 's/_set_by_docker_makefile_root_/$(BUILD_MODSW_DOCKEREE_ROOT)/g' docker.sh)
	(cd $(INSTALL_DIR)/etc/ssl; rm -rf certs; ln -s /var/cert certs)
	(cd $(INSTALL_DIR); rm -rf run; ln -s /var/run run)
	cp -rf scripts/* $(DOCKER_INSTALL_DIR)
	chmod 755 $(DOCKER_INSTALL_DIR)/*.sh
	(cd $(DOCKER_INSTALL_DIR); \
		sed -i -e 's/_set_by_docker_makefile_ip_/$(BUILD_MODSW_DOCKEREE_BR_IP_ESCAPED)/g' docker_start.sh; \
		sed -i -e 's/_set_by_docker_makefile_root_/$(BUILD_MODSW_DOCKEREE_ROOT)/g' docker_start.sh; \
		sed -i -e 's/_set_by_docker_makefile_drv_/$(BUILD_MODSW_DOCKEREE_DRV)/g' docker_start.sh)
else
conditional_build:
	@echo "skipping Docker (not configured)"
endif

clean:
	@echo "Cleaning docker ..."
	rm -rf $(INSTALL_DIR)/bin/docker

conf:
	@mkdir -p $(INSTALL_DIR)/etc/rc3.d
	@mkdir -p $(INSTALL_DIR)/etc/init.d
	@mkdir -p $(INSTALL_DIR)/bin/docker
	@mkdir -p $(INSTALL_DIR)/etc/ssl
	@mkdir -p $(DOCKER_INSTALL_DIR)

distclean: clean
