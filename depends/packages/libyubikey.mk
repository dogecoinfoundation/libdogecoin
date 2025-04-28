package=libyubikey
$(package)_version=1.13
$(package)_download_path=https://developers.yubico.com/yubico-c/Releases
$(package)_file_name=libyubikey-$($(package)_version).tar.gz
$(package)_sha256_hash=04edd0eb09cb665a05d808c58e1985f25bb7c5254d2849f36a0658ffc51c3401

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-static
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
