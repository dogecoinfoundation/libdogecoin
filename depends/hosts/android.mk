ANDROID_API_LEVEL=21
ANDROID_NDK_VERSION=android-ndk-r25c

ANDROID_NDK=$(SDK_PATH)/$(ANDROID_NDK_VERSION)
ANDROID_TOOLCHAIN_BIN=$(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin

ifeq ($(HOST),armv7a-linux-android)
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang
else
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang
endif

android_AR=$(ANDROID_TOOLCHAIN_BIN)/llvm-ar
android_RANLIB=$(ANDROID_TOOLCHAIN_BIN)/llvm-ranlib

android_cmake_system=Android
