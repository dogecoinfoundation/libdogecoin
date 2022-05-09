$(package)_version=2.1.12-stable
$(package)_download_path=https://github.com/libevent/libevent/releases/download/release-$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=92e6de1be9ec176428fd2367677e61ceffc2ee1cb119035037a27d346b0403bb
$(package)_patches=0001-fix-windows-getaddrinfo.patch

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress --disable-samples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
  $(package)_cppflags_mingw32=-D_WIN32_WINNT=0x0601
endef

ifeq ($(target_os),windows)
define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/0001-fix-windows-getaddrinfo.patch && \
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub build-aux
endef
else
define $(package)_preprocess_cmds
  ./autogen.sh
endef
endif

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

ifeq ($(target_os),windows)
define $(package)_postprocess_cmds
  rm lib/*.la
endef
else
define $(package)_postprocess_cmds
  $(package)_config_opts_linux=rm lib/*.la && \
  rm include/ev*.h
endef
endif
