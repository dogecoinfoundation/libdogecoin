packages:=libevent
native_packages := native_ccache

wallet_packages=

upnp_packages=

darwin_native_packages = 

ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_libtapi

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
darwin_native_packages+= native_clang
endif

endif
