define CONFIG_DEFINE
$(strip $(1))
endef

# BCM_BUILDROOT_CONFIG_DIR = product_config/$(PR_NAME)
# BCMCFG_STR = XX=xx YY=yy ...
BCMCFG_STR=$(patsubst CONFIG_BCM_%, %, $(filter CONFIG_BCM_%, $(shell cat $(BCM_BUILDROOT_CONFIG_DIR)/.config | sed /^\#.*/d)))

$(foreach cfg, $(BCMCFG_STR), $(eval $(call CONFIG_DEFINE, $(cfg))))


