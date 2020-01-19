LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	tt_watcher_domain.c

#LOCAL_SHARED_LIBRARIES := 

LOCAL_MODULE:= tt_watcher

include $(BUILD_EXECUTABLE)
