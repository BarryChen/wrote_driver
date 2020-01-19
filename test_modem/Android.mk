# Copyright 2005 The Android Open Source Project
#
# Android.mk for adb
#

LOCAL_PATH:= $(call my-dir)

# adb host tool
# =========================================================
include $(CLEAR_VARS)

LOCAL_MODULE := test_modem
LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils  


LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)
