
EXTRAINCDIRS += $(curdir)/

$(curdir)-$(CONFIG_LIB_CONTIKI) += contiki/
$(curdir)-y += compat.c
$(curdir)-$(CONFIG_LIB_FLASHMGT) += flashmgt.c
$(curdir)-$(CONFIG_LIB_INIT) += init.c
$(curdir)-$(CONFIG_LIB_LZO) += minilzo/minilzo.c
$(curdir)-$(CONFIG_LIB_ONEWIRE) += onewire.c
$(curdir)-$(CONFIG_LIB_OPTIBOOT) += optiboot.c
$(curdir)-$(CONFIG_LIB_PID) += pid.c
$(curdir)-$(CONFIG_LIB_POLYFS) += polyfs.c
$(curdir)-$(CONFIG_LIB_POLYFS_CFS) += polyfs_cfs.c
$(curdir)-$(CONFIG_LIB_POLYFS_DF) += polyfs_df.c
$(curdir)-$(CONFIG_LIB_PREFS) += prefs.c
$(curdir)-$(CONFIG_LIB_RESOLV_HELPER) += resolv_helper.c
$(curdir)-$(CONFIG_LIB_RESOLV_HELPER) += pton.c
$(curdir)-$(CONFIG_LIB_SETTINGS) += settings.c
$(curdir)-$(CONFIG_LIB_SNTP) += sntp.c
$(curdir)-$(CONFIG_LIB_STACK) += stack.c
$(curdir)-$(CONFIG_LIB_STRFTIME) += strftime.c
$(curdir)-$(CONFIG_LIB_STUBBOOT) += stubboot.c
$(curdir)-$(CONFIG_LIB_TFTP) += tftp.c
$(curdir)-$(CONFIG_LIB_TIME) += time.c

$(eval $(call subdir,$(curdir)))

