package=ykpers
$(package)_version=1.20.0
$(package)_download_path=https://developers.yubico.com/yubikey-personalization/Releases
$(package)_file_name=ykpers-$($(package)_version).tar.gz
$(package)_sha256_hash=0ec84d0ea862f45a7d85a1a3afe5e60b8da42df211bb7d27a50f486e31a79b93
$(package)_dependencies=libyubikey libusb
$(package)_patches=ykpers-args.patch

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-static --with-backend=libusb-1.0
  $(package)_config_opts_mingw32=--enable-threads=windows
  $(package)_ldflags_darwin=-framework CoreFoundation -framework IOKit -framework Security
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/ykpers-args.patch
endef

define $(package)_config_cmds
  $($(package)_autoconf) CFLAGS="-fPIC" LIBUSB_CFLAGS="-I$(host_prefix)/include/libusb-1.0" LIBUSB_LIBS="-L$(host_prefix)/lib -lusb-1.0"
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
