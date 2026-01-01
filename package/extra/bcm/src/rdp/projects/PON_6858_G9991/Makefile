all: firmware drivers

# Make sure that firmware is built before drivers when building ut
# This causes this instance of Make to execute -j1.
.NOTPARALLEL:

CMAKE_FW_BUILD ?= y

include make.proj_flags
include $(TOP_DIR)/make.common
include $(TOP_DIR)/make.dyn_targets

ut: hw_dts_gen sim firmware drivers
	$(MAKE) -C $(PROJ_UT_TARGET) $@ GEN=$(GEN)

