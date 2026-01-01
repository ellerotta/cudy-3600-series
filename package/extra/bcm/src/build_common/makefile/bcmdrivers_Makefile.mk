######################################################################
# + SDK/bcmdrivers/Makefile +

# use wlan shared code
export WLAN_SHARED_IMPL=1
export BCM_NBUFF_COMMON=1
export WLAN_SHARED_DIR=$(BCM_TARGET_PATH)/bcmdrivers/broadcom/net/wl/shared/impl$(WLAN_SHARED_IMPL)

ifneq ($(strip $(BCA_HNDROUTER)),)
export USE_WLAN_SHARED = 1
endif

# - SDK/bcmdrivers/Makefile -
######################################################################
