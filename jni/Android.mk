# Generated by VisualGDB

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := FancyUSBService
#VisualGDBAndroid: AutoUpdateSourcesInNextLine
LOCAL_SRC_FILES := client.cpp FancyUSBService.cpp MyLog.cpp PlatfromAndroid.cpp Thread.cpp
LOCAL_C_INCLUDES := .
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES :=
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2 -lz -lOpenSLES
LOCAL_CFLAGS :=
LOCAL_CPPFLAGS := -std=c++11
LOCAL_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)
