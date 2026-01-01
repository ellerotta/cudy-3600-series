######################################################################
# + SDK/bcmdrivers/broadcom/net/wl/implX/main/components/router/Makefile
#

# Purpose:
#	export vars needed by wireless ko
#

#export ARCH=$(LINUX_KARCH)
#export CROSS_COMPILE=$(KERNEL_CROSS)


# Purpose:
#	export some -D to CFLAGS, for %.c -> %.o compiling in user program.
#

ifeq ($(PLATFORM),arm-glibc)
export CFLAGS += -fno-strict-aliasing
export CFLAGS += -fcommon
endif

ifneq ($(strip $(BCA_HNDROUTER)),)
export CFLAGS += -DBCA_HNDROUTER $(USR_CFLAGS)
ifneq ($(strip $(BUILD_MCAST_PROXY)),)
CFLAGS += -DMCPD_PROXY
endif
endif

ifeq ($(CONFIG_RTR_OPTIMIZE_SIZE),y)
export CFLAGS += -Os
export OPTCFLAGS = -Os
else
export CFLAGS += -O2
export OPTCFLAGS = -O2
endif

ifeq ($(CONFIG_NVRAM),y)
export CFLAGS += -DBCMNVRAM
endif

ifeq ($(CONFIG_BCMWPA2),y)
export CFLAGS += -DBCMWPA2
endif

ifeq ($(CONFIG_DHDAP),y)
export CONFIG_DHDAP
export CFLAGS += -D__CONFIG_DHDAP__
endif

ifeq ($(CONFIG_RSDB),y)
export CFLAGS += -D__CONFIG_RSDB__
endif

ifeq ($(CONFIG_GMAC3),y)
export CFLAGS += -D__CONFIG_GMAC3__
endif

ifeq ($(CONFIG_STBAP),y)
export CFLAGS += -D__CONFIG_STBAP__
endif

ifeq ($(PLT),arm)
ifeq ($(CONFIG_PORT_BONDING),y)
export CFLAGS += -DPORT_BONDING
endif
endif

ifeq ($(CONFIG_NFC),y)
# WPS_NFC
export CFLAGS += -D__CONFIG_NFC__
endif

ifeq ($(CONFIG_EMF),y)
export CFLAGS += -D__CONFIG_EMF__
export CONFIG_EMF_ENABLED := $(CONFIG_EMF)
endif

ifeq ($(CONFIG_IGMP_PROXY),y)
export CFLAGS += -D__CONFIG_IGMP_PROXY__
endif

ifeq ($(CONFIG_WL_ACI),y)
export CFLAGS += -D__CONFIG_WL_ACI__
endif

ifeq ($(CONFIG_TRAFFIC_MGMT_RSSI_POLICY),y)
export CFLAGS += -DTRAFFIC_MGMT_RSSI_POLICY
endif

ifeq ($(CONFIG_SOUND),y)
export CFLAGS += -D__CONFIG_SOUND__
endif

ifeq ($(CONFIG_VOIP),y)
export CFLAGS += -DBCMVOIP
endif

ifeq ($(CONFIG_WAPI),y)
export CFLAGS += -DBCMWAPI_WAI -DBCMWAPI_WPI
endif

ifeq ($(CONFIG_PHYMON_UTILITY),y)
export CFLAGS += -DPHYMON
endif

ifeq ($(CONFIG_EXTACS),y)
export CFLAGS += -DEXT_ACS
endif

ifeq ($(CONFIG_BCMESCAND),y)
export CFLAGS += -DBCM_ESCAND
endif

ifeq ($(CONFIG_BCMBSD),y)
export CFLAGS += -DBCM_BSD
endif

ifeq ($(CONFIG_BCMSSD),y)
export CFLAGS += -DBCM_SSD
endif

ifeq ($(RDKB),y)
# only used by RDKB or RDKM builds
export CFLAGS += -DBCM_ECBD
export BCM_ECBD := 1
endif

ifeq ($(RDKB_ONE_WIFI),y)
export CFLAGS += -DRDKB_ONE_WIFI
endif

ifeq ($(CONFIG_BCMEVENTD),y)
export CFLAGS += -DBCM_EVENTD
endif

ifeq ($(CONFIG_TOAD),y)
export CFLAGS += -D__CONFIG_TOAD__
endif

ifeq ($(CONFIG_BCM_APPEVENTD),y)
export CFLAGS += -DBCM_APPEVENTD
endif

ifeq ($(CONFIG_BCM_MEVENTD),y)
export CFLAGS += -DBCM_MEVENTD
endif

ifeq ($(CONFIG_MFP),y)
export CFLAGS += -DMFP
endif

ifeq ($(CONFIG_BCMDRSDBD),y)
export CFLAGS += -DBCM_DRSDBD
endif

ifeq ($(CONFIG_MOCA),y)
export CFLAGS += -DBCM_MOCA
endif

ifeq ($(CONFIG_BCM_CEVENT),y)
export CFLAGS += -D__CONFIG_BCM_CEVENT__ -DBCM_CEVENT
export CONFIG_BCM_CEVENT
endif

ifeq ($(CONFIG_HSPOT),y)
export CFLAGS += -DNAS_GTK_PER_STA -DHSPOT_OSEN
export ICONPATH := /webs/wlrouter/hspot
export CFLAGS += -DICONPATH=\"$(ICONPATH)\"
endif

ifeq ($(CONFIG_WBD),y)
export CONFIG_WBD
export CFLAGS += -DBCM_WBD -DMULTIAP
ifeq ($(CONFIG_QOSMGMT),y)
CONFIG_QOSMGMT_MULTIAP := y
export CFLAGS += -DBCM_QOSMGMT_MULTIAP
export CONFIG_QOSMGMT_MULTIAP
endif
endif

ifeq ($(CONFIG_SIGMA),y)
export CFLAGS += -D__CONFIG_SIGMA__
endif

ifeq ($(CONFIG_MINI_ROUTER), y)
export CFLAGS += -D__CONFIG_ROUTER_MINI__
endif

ifeq ($(CONFIG_WNM), y)
export CFLAGS += -DWLWNM
endif

ifeq ($(CONFIG_BCMASPMD),y)
export CFLAGS += -DBCM_ASPMD
endif # ASPMD

ifeq ($(CONFIG_FBT),y)
export	CFLAGS += -DWLHOSTFBT
endif

ifeq ($(CONFIG_QOSMGMT),y)
export CFLAGS += -DBCM_QOSMGMT
export CONFIG_QOSMGMT
export CFLAGS += -DBCM_QOSMGMT_R1
ifeq ($(BCM_QOSMGMT_REL),2)
export CFLAGS += -DBCM_QOSMGMT_R2
export CFLAGS += -DBCM_QOSMGMT_R3
export BCM_QOSMGMT_R3 := 1
endif
endif # CONFIG_QOSMGMT

ifeq ($(BUILD_HND_EAP),y)
export	CFLAGS += -DBCM_SKB_FREE_OFFLOAD
endif

ifeq ($(CONFIG_AFCD), y)
export CFLAGS += -DBCM_LOCPOLD
endif

# CONFIG_EAP is an Enterprise AP indication exported to
# other non-driver related modules, for example, the
# CLI command parser (wl.exe)
ifeq ($(CONFIG_EAP),y)
export CONFIG_EAP
export CFLAGS += -DWL_EAP_AP
# export CFLAGS += -DWL_PROXDETECT
obj-y += tpdump
else
# BCM949408EAP platform builds (linux 4.1.x) require different check
ifeq ($(BUILD_HND_EAP),y)
export CONFIG_EAP := y
export CFLAGS += -DWL_EAP_AP
# export CFLAGS += -DWL_PROXDETECT
endif
endif

ifeq ($(CONFIG_BCMESCAND),y)
export CFLAGS += -DWL_SCAN_TX
export CFLAGS += -DWL_SCAN_BEACON_DELAY
export CFLAGS += -DWL_SCAN_DATA_SNOOP
endif

ifeq ($(BUILD_BRCM_AIRIQ),y)
export CONFIG_AIRIQ := y
endif
ifneq ($(RDK_BUILD),y)
obj-$(CONFIG_AIRIQ) += airiq
endif

ifeq ($(BUILD_BRCM_HOSTAPD),y)
export CFLAGS += -DCONFIG_HOSTAPD
endif

ifneq ($(wildcard $(IQOS_DIR)),)
export CFLAGS += -D__CONFIG_TREND_IQOS__ -DCONFIG_TREND_IQOS_ENABLED
endif # IQOS_DIR

ifeq ($(WLTEST),1)
export CFLAGS += -DWLTEST
endif

ifeq ($(CONFIG_WLEXE),y)
export CONFIG_WLEXE
export RWL ?= 0
endif

ifeq ($(CONFIG_STBAP),y)
export CFLAGS	+= -DSTBAP
export CFLAGS += -D__CONFIG_STBAP__
export CFLAGS	+= -DSTB
export STB := 1
endif # CONFIG_STBAP

#
# - SDK/bcmdrivers/broadcom/net/wl/implX/main/components/router/Makefile
######################################################################