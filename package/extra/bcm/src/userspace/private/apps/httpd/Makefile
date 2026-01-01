#
# In most cases, you only need to modify this first section.
#
EXE = httpd

OBJS = main.o httpd.o basic.o ej.o cgi_cmd.o cgi_dhcpd.o cgi_main.o \
       cgi_ntwk.o cgi_route.o cgi_sec.o cgi_sts.o cgi_upload.o cgi_util.o cgi_wan.o \
       cgi_selt.o \
       cgi2_util.o cgi2_wan.o cgi2_sec.o cgi2_sts.o \
       cgi2_ethintf.o cgi2_bridging.o cgi2_route.o cgi2_usb_hosts.o cgi.o \
       cgi_diag.o cgi2_diag.o cgi2_stsdsl.o cgi2_stsxtm.o cgi_pptpacintf.o

WAN_OBJS = cgi_qos.o cgi2_qos.o \
           cgi_qos_class.o cgi2_qos_class.o \
           cgi_qos_queue.o cgi2_qos_queue.o \
           cgi_qos_policer.o cgi2_qos_policer.o \
           cgi_qos_port_shaping.o cgi2_qos_port_shaping.o \
           cgi_ethintf.o \
           cgi_seclog.o cgi_autodetection.o


all dynamic install: $(EXE) generic_exe_install cpall_html cpall_html_libs

clean: generic_clean cpall_html_libs_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)
	rm -f minimized_webs.tar

.PHONY: all dynamic install


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
include $(BUILD_DIR)/create_web.mk
include $(BUILD_DIR)/make.wlan


CUSTOM_CFLAGS += $(BRCM_APP_CFLAGS)

LIBS = -lcms_dal -lcms_msg $(CMS_COMMON_LIBS) $(CMS_CORE_LIBS)

ifneq ($(strip $(BUILD_CUSTOMER)),)
LIBS += -lcms_dal_customer
include customers/$(BUILD_CUSTOMER_NAME)/make.$(BUILD_CUSTOMER_NAME)
endif

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
ifneq ($(strip $(BUILD_BCM_WLAN_DPDCTL)),)
ifneq ($(strip $(BUILD_PWRCTL)),)
CUSTOM_CFLAGS += -DBUILD_BCM_WLAN_DPDCTL
endif
endif
endif

BASE_HTMLS = backupsettings.html defaultsettings.html dhcpinfo.html diag.html                    \
             diagbr.html diaglan.html dnscfg.html footer.html hlpethconn.html hlpusbconn.html hlpwlconn.html \
             index.html info.html lancfg2.html lockerror.html logconfig.html logintro.html       \
             logo.html main.html menu.html password.html rebootinfo.html resetrouter.html        \
             restoreinfo.html routeadd.html StaticIpAdd.html StaticIpErr.html statsifc.html      \
             statsifcreset.html updatesettings.html upload.html uploadinfo.html diagethoam.html  \
             statsqueue.html lanvlancfg.html

WAN_HTMLS =  algcfg.html diagipow.html diagmer.html diagpppoa.html diagpppoe.html                   \
             enblbridge.html enblservice.html hlpatmetoe.html hlpatmseg.html hlppngdns.html         \
             hlppnggw.html hlppppoasess.html hlppppoeauth.html hlppppoeconn.html hlppppoeip.html    \
             hlptstdns.html ifcdns.html ifcgateway.html ipoacfg.html natcfg2.html   \
             ntwksum2.html pppoe.html pradd.html qoscls.html qospoliceradd.html qosportshaping.html \
             qosqmgmt.html qosqueueadd.html rtdefaultcfgerr.html rtdefaultcfg.html                  \
             scdmz.html scinflt.html scmacflt.html scmacpolicy.html scoutflt.html scprttrg.html     \
             scvrtsrv.html seclogintro.html statswanreset.html wanadderr.html wancfg.html

UNUSED_HTMLS = enblbridge.html enblservice.html statsatmerr.html statsatm.html statsatmreset.html

HTMLS = html/*.js html/*.png html/*.css $(addprefix html/,$(BASE_HTMLS))

ifneq ($(strip $(BUILD_WAN_HTML)),)
OBJS += $(WAN_OBJS)
HTMLS += $(addprefix html/,$(WAN_HTMLS))
CUSTOM_CFLAGS += -DSUPPORT_WAN_HTML
endif


ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
LIBS += -lwlsysutil
ifneq ($(strip $(BUILD_BRCM_CPEROUTER)),)
LIBS += -lwifihttp -lnvram -lbcmcrypto -lshared -L$(INSTALL_DIR)/usr/lib
endif
CUSTOM_CFLAGS += -DWL_IMPL_PLUS
CFLAGS += -I$(HNDAPPS_DIR)/shared  -I$(HNDAPPS_DIR)/../wlioctl/include -I$(HNDAPPS_DIR)/../proto/include

ifneq ($(strip $(BUILD_WLVISUALIZATION)),)
CUSTOM_CFLAGS += -DSUPPORT_WLAN_VISUALIZATION
CUSTOM_CFLAGS += -I$(WIRELESS_DRIVER_PATH)/router/shared
endif

ifneq ($(strip $(BUILD_BSTREAM_IQOS)),)
ifneq ($(wildcard $(COMPON_WIRELESS_DRIVER_PATH)/vendor/trend/iqos),)
CUSTOM_CFLAGS += -DSUPPORT_WLAN_IQOS
endif
LIBS += -ljson-c
endif
endif #BRCM_DRIVER_WIRELESS

ifneq ($(strip $(BRCM_VOICE_SUPPORT)),)
include $(BUILD_DIR)/make.voice
LIBS += -lpthread
ifeq ($(BUILD_TR104_PROFILES),1)
OBJS += cgi_voice.o
endif
ifeq ($(BUILD_TR104_PROFILES),2)
OBJS += cgi2_voice.o
endif
HTMLS += html/voice*.html
endif

ifneq ($(strip $(BUILD_OMCI)),)
LIBS += -lcgi_gpon -lomciutil
OBJS += cgi_omci_misc.o cgi_omci_macro.o
HTMLS += html/omci*.html
CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/private/include
ifneq ($(strip $(BUILD_BCMIPC)),)
CUSTOM_CFLAGS += -DBUILD_BCMIPC
LIBS += -lbcmipc
endif
endif

ifneq ($(strip $(BUILD_GPON)),)
OBJS += cgi_gponintf.o cgi_sts_dev2.o
HTMLS += html/*gpon*.html html/statsopticifc*.html
endif

ifneq ($(strip $(BUILD_QUICKSETUP)),)
OBJS += cgi_quicksetup.o cgi2_quicksetup.o
HTMLS += html/quick*.html html/qsmain.html
endif

ifneq ($(strip $(BUILD_STORAGESERVICE)),)
OBJS += cgi_storageservice.o
HTMLS += html/storage*.html
endif

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
OBJS += cgi_wl.o cgi_sts_wifi.o
HTMLS += html/cfgwlwan.html
ifneq ($(strip $(BUILD_TR98_PROFILES)),)
# TR98 only files go here (currently none)
endif

ifneq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
LIBS += -lstaged -ljson-c -lpthread
 ifneq ($(strip $(BUILD_CUSTOMER)),)
  LIBS += -lwlmdm_customer
 else
  LIBS += -lwlmdm
 endif
CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/private/include
endif # BUILD_BRCM_UNFWLCFG

endif # BRCM_DRIVER_WIRELESS

ifneq ($(strip $(BUILD_LIBMODSW)),)
OBJS += cgi_modsw_ee.o
LIBS += -lmodsw -lbeeputil -lssl -lcrypto -lcurl -ljson-c
ifeq ($(strip $(BUILD_LIBCURL_WITH_HTTP2)),y)
LIBS   += -lnghttp2
endif
endif

ifneq ($(strip $(BUILD_LXC)),)
OBJS += cgi_container.o
HTMLS += html/containertable.html html/containerdetail.html
endif

ifneq ($(strip $(BUILD_WEBUI_LICENSE_UPLOAD)),)
HTMLS += html/uploadlicense.html
endif

ifneq ($(strip $(BUILD_CERT)),)
OBJS += cgi_cert.o
HTMLS += html/cert*.html
endif

ifneq ($(strip $(BUILD_VCONFIG)),)
OBJS += cgi_pmap.o
HTMLS += html/port*.html
endif

ifneq ($(strip $(BUILD_BMU)),)
OBJS += cgi_bmu.o
HTMLS += html/bmu.html
endif

ifneq ($(strip $(BUILD_DDNSD)),)
OBJS += cgi_ddns.o
HTMLS += html/ddns*.html
endif

ifneq ($(strip $(BUILD_DLNA)),)
OBJS += cgi_dlna.o
HTMLS += html/dlna*.html
endif

ifneq ($(strip $(BUILD_DNSMASQ)),)
OBJS += cgi_dnsproxy.o
HTMLS += html/dnsp*.html
endif

ifneq ($(strip $(BUILD_DSL)),)
OBJS += cgi_dslintf.o cgi2_dslintf.o
HTMLS += html/*dsl*.html html/cfgatm.html html/cfgptm.html html/atmdelerr.html html/ber*.html html/statsxtmreset.html html/ptm*.html
endif

ifeq "$(BRCM_DSL_PHY_BAS)" "y"
CUSTOM_CFLAGS += -DSUPPORT_BAS_DSL
OBJS += cgi_diag_xdsl.o
HTMLS += html/diaglpselt.html
endif

ifneq ($(strip $(BUILD_DEBUG_TOOLS)),)
OBJS += cgi_dump.o cgi_eng.o
HTMLS += html/eng*.html
endif

ifneq ($(strip $(BUILD_EPONWAN)),)
OBJS += cgi_eponintf.o
HTMLS += html/*epon*.html
endif

ifneq ($(strip $(BUILD_EPON_SDK)),)
OBJS += cgi_epon.o
HTMLS += html/eponloid.html
endif

ifneq ($(strip $(BUILD_ETHWAN)),)
HTMLS += html/eth*.html html/cfgeth.html
endif

ifneq ($(strip $(BUILD_IPPD)),)
OBJS += cgi_ipp.o
HTMLS += html/ippcfg.html
endif

ifneq ($(strip $(BUILD_IPSEC_TOOLS)),)
OBJS += cgi_ipsec.o cgi2_ipsec.o
HTMLS += html/ips*.html
endif

ifneq ($(strip $(BUILD_VXLAN_TUNNEL)),)
OBJS += cgi2_vxlan.o cgi2_vxlan_sts.o
HTMLS += html/vxlan.html html/vxlanif.html html/statsvxlan.html
endif

ifneq ($(strip $(BUILD_GRE_TUNNEL)),)
ifneq ($(strip $(BUILD_DM_PURE181)),)
OBJS += cgi2_gre.o cgi2_gre_sts.o
HTMLS += html/gre.html html/greif.html html/statsgre.html
endif
endif


ifneq ($join $(strip $(DMP_X_BROADCOM_COM_IGMP_1)), $(strip $(DMP_X_BROADCOM_COM_MLD_1)), $(strip $(DMP_X_BROADCOM_COM_MCAST_1)),)
OBJS += cgi_multicast.o
endif

ifneq ($(strip $(BUILD_L2TPAC)),)
OBJS += cgi_l2tpacintf.o
HTMLS += html/cfgl2tpac.html
endif

ifneq ($(strip $(BUILD_L2TPNS)),)
ifneq ($(strip $(BUILD_DM_PURE181)),)
OBJS += cgi_l2tpnsintf.o
HTMLS += html/l2tpnscfg.html
endif
endif

ifneq ($(strip $(BUILD_ACCEL_PPTPNS)),)
ifneq ($(strip $(BUILD_DM_PURE181)),)
OBJS += cgi_pptpnsintf.o
HTMLS += html/pptpnscfg.html
endif
endif

ifneq ($(strip $(BUILD_IPV6)),)
OBJS += cgi_iptunnel.o cgi_lan6.o
HTMLS += html/ipv6lancfg.html html/tunnel*.html
ifneq ($(strip $(BUILD_MAP)),)
HTMLS += html/map.html
endif
endif

ifneq ($(strip $(BUILD_MCAST_PROXY)),)
HTMLS += html/multicast.html
endif

ifneq ($(strip $(BUILD_PWRCTL)),)
OBJS += cgi_pwrmngt.o
HTMLS += html/pwrmngt.html
endif

ifneq ($(strip $(BUILD_SNMP)),)
HTMLS += html/snmpconfig.html
endif

ifneq ($(strip $(BUILD_DBUSREMOTE_TCP)),)
HTMLS += html/dbusremotecfg.html
endif

ifneq ($(strip $(BUILD_BCMBUSYBOX_NTPD)),)
HTMLS += html/ntpdcfg.html
endif

ifneq ($(strip $(BUILD_TOD)),)
OBJS += cgi_tod.o
HTMLS += html/todadd.html
endif

ifneq ($(strip $(BUILD_TR69C))$(strip $(BUILD_TR69C_SSL)),)
HTMLS += html/tr69*.html
endif

ifneq ($(strip $(BUILD_BAS2))$(strip $(BUILD_OPS_BAS)),)
HTMLS += html/basdcfg.html
endif

ifneq ($(strip $(BUILD_BAS2)),)
OBJS += cgi_basd.o
HTMLS += html/basdclnt*.html
endif

ifneq ($(strip $(BUILD_USP)),)
OBJS += cgi_usp.o cgi_usp_mqtt.o
HTMLS += html/stomp*.html html/agent*.html html/cntrl*.html
HTMLS += html/subscription*.html html/ctrole*.html html/mqtt*.html
HTMLS += html/ctchallenge*.html
endif

ifneq ($(strip $(BUILD_UPNP)),)
HTMLS += html/upnpcfg.html
endif

ifneq ($(strip $(BUILD_URLFILTER)),)
HTMLS += html/url_add.html
OBJS += cgi_urlfilter.o
endif

ifeq ($(strip $(BUILD_DSL_SELT_TEST)),y)
HTMLS += html/seltcfg.html html/seltcfgc.html
endif

ifeq ($(strip $(BUILD_SPDSVC)), y)
HTMLS += html/speedsvc.html
OBJS += cgi_spdsvc.o
endif

ifneq ($(strip $(BUILD_XMPP)),)
HTMLS += html/xmppconn.html
OBJS += cgi_xmpp_conn.o
endif

ifneq ($(strip $(BUILD_JQPLOT)),)
ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
HTMLS += html/cpumemcharts.html
HTMLS += html/flowstats.html
endif
endif

ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
ifneq ($(strip $(BUILD_DPI)),)
HTMLS += html/dpicharts.html html/dpi.css
HTMLS += html/dpi.js
endif
endif

ifneq ($(strip $(BUILD_OPENVSWITCH)),)
ifneq ($(strip $(BUILD_DM_PURE181)),)
OBJS += cgi2_openVS.o
HTMLS += html/openvs_cfg.html html/openvs_addbr.html html/openvs_edit.html
else
OBJS += cgi_openVS.o
HTMLS += html/openvswitchcfg.html
endif
endif

ifeq ($(strip $(BUILD_STUN)),y)
HTMLS += html/stuncfg.html
endif

ifneq ($(strip $(BRCM_KERNEL_BONDING)),)
ifneq ($(strip $(BUILD_DM_PURE181)),)
OBJS += cgi2_ethlag.o
HTMLS += html/cfgethlag.html

# support LAN interface in EthLAG WAN
ifeq ($(strip $(BCM_DSL_RDP)),y)
CFLAGS += -DSUPPORT_LAN_IN_ETHLAG_WAN
endif

endif
endif

ifneq ($(strip $(BRCM_DRIVER_CROSSBOW_FULL_OFFLOAD)),)
CFLAGS += -DSUPPORT_CROSSBOW_FULL_OFFLOAD
endif

ifneq ($(strip $(BCM_PON)),)
OBJS += cgi_wan_select.o
HTMLS += html/wan_select.html
endif

ifeq ($(strip $(BUILD_ACCEL_PPTP)),y)
HTMLS += html/pptpcfg.html
endif

$(eval $(call CREATE_WEB_TRG, cpall_html, html, $(INSTALL_DIR)/webs, staging, minimized_webs.tar, $(HTMLS:html/%=%)))

.PHONY: cpall_html_libs cpall_html_libs_clean
ifeq "$(BRCM_DSL_PHY_BAS)" "y"
cpall_html_libs:
	@echo running target $@
	@make -C html_lib

cpall_html_libs_clean:
	@echo running target $@
	@make -C html_lib clean
endif

#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
                         -I$(KERNEL_LINKS_DIR)

#
# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
# TODO: eventually, we should remove the legacy and incorrect paths of /lib/private and /lib/public
ALLOWED_LIB_DIRS := /lib:/usr/lib:/private/lib:/public/lib:/public/lib/lib:/lib/private:/lib/public


ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
ALLOWED_INCLUDE_PATHS +=   -I$(WIRELESS_DRIVER_PATH)/include \
                           -I$(BCM_FSBUILD_DIR)/private/include \
                           -I$(INC_BCMDRIVER_PATH)/include


endif


# For non basic only httpd configure, openssl is required.
ifeq ($(strip $(BUILD_HTTPD)),)
	LIBS += -lssl -lcrypto -lexpat -lbcm_sslconf
	CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include
	CFLAGS += -DSUPPORT_OPENSSL
endif

ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS += $(SSP_MIN_COMPILER_OPTS)
LIBS += $(SSP_LIBS)
endif

ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
LIBS += -lhttpdshared
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/userspace/public/libs/httpdshared
endif


LIBS += -lc 
LIBS += $(LIBS_CC_RT)


# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors


#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
#
# TODO: eventually, remove CMS_LIB_PATH but for now still needed.  Should use BCM_LIB_PATH only.
$(EXE): $(OBJS)
	@echo building $@
	$(CC) $(BCM_LD_FLAGS) -o $@ $^ $(CMS_RPATH_OPTION) $(BCM_RPATH_LINK_OPTION) $(BCM_LIB_PATH) $(CMS_LIB_PATH) $(LIBS)



# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)

shell:
	bash -i
