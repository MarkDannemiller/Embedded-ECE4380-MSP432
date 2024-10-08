#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source;C:/ti/simplelink_msp432e4_sdk_4_20_00_12/kernel/tirtos/packages;C:/Users/priva/ONEDRI~1/Documents/ccs_workspace/tirtos_builds_MSP_EXP432E401Y_release_ccs/.config
override XDCROOT = C:/ti/xdctools_3_61_01_25_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source;C:/ti/simplelink_msp432e4_sdk_4_20_00_12/kernel/tirtos/packages;C:/Users/priva/ONEDRI~1/Documents/ccs_workspace/tirtos_builds_MSP_EXP432E401Y_release_ccs/.config;C:/ti/xdctools_3_61_01_25_core/packages;..
HOSTOS = Windows
endif
