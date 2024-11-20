LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT := $(LOCAL_PATH)/../../../..

LOCAL_MODULE := main

LOCAL_SRC_FILES := $(wildcard $(PROJECT_ROOT)/src/*.c)
LOCAL_SRC_FILES += $(addprefix $(PROJECT_ROOT)/raylib/,raudio.c rcore.c utils.c)

LOCAL_C_INCLUDES := $(PROJECT_ROOT)/src $(PROJECT_ROOT)/raylib

# Disable miniaudio's AAudio backend because we suspect it causes random crashes
# when the game is closed
LOCAL_CFLAGS := -O2 -DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_ANDROID -D__ANDROID__ \
	-DMA_NO_AAUDIO -DMA_NO_RUNTIME_LINKING

ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=vfpv3-d16
else ifeq ($(TARGET_ARCH),arm64)
	LOCAL_CFLAGS += -mfix-cortex-a53-835769
endif

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -lOpenSLES -dl -lm -lc

# Ensure Android builds are reproducible by disabling NDK build-id
LOCAL_LDFLAGS := -Wl,--build-id=none

LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)

