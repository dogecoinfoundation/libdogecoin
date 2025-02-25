package=libusb
$(package)_version=1.0.27
$(package)_download_path=https://github.com/libusb/libusb/releases/download/v1.0.27
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=ffaa41d741a8a3bee244ac8e54a72ea05bf2879663c098c82fc5757853441575

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-static --disable-udev
  $(package)_config_opts_mingw32=--enable-threads=windows
endef

define $(package)_config_cmds
  $($(package)_autoconf) CFLAGS="-fPIC"
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
