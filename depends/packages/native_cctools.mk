package=native_cctools
$(package)_version=3764b223c011574971ee3ae09ce968ba5dc2f00f
$(package)_download_path=https://github.com/tpoechtrager/cctools-port/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=3e35907bf376269a844df08e03cbb43e345c88125374f2228e03724b5f9a2a04
$(package)_build_subdir=cctools
$(package)_patches=ld64_disable_threading.patch
$(package)_dependencies=native_libtapi

define $(package)_set_vars
  $(package)_config_opts=--target=$(host)
  $(package)_ldflags+=-Wl,-rpath=\\$$$$$$$$\$$$$$$$$ORIGIN/../lib
  ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
  $(package)_config_opts+=--enable-lto-support --with-llvm-config=$(build_prefix)/bin/llvm-config
  endif
  $(package)_cc=$(clang_prog)
  $(package)_cxx=$(clangxx_prog)
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/ld64_disable_threading.patch
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
  rm -rf share
endef
