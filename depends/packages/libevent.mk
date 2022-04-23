package=libevent
$(package)_version=2.0.22
$(package)_download_path=https://github.com/libevent/libevent/releases/download/release-2.0.22-stable
$(package)_file_name=$(package)-$($(package)_version)-stable.tar.gz
$(package)_sha256_hash=71c2c49f0adadacfdbe6332a372c38cf9c8b7895bb73dabeaa53cdcc1d4e1fa3

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress --disable-samples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
  $(package)_cppflags_mingw32=-D_WIN32_WINNT=0x0601
endef

define $(package)_preprocess_cmds
  ./autogen.sh
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la && \
  rm include/ev*.h
endef