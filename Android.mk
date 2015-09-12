
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := system/core/include
LOCAL_SRC_FILES := debughelper.cpp
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_REQUIRED_MODULES := binderdump.sh init.debughelper.rc

LOCAL_MODULE := libdebughelper
LOCAL_MULTILIB := both
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := binderdump.sh
LOCAL_SRC_FILES := misc/binderdump.sh
LOCAL_MODULE_CLASS := EXECUTABLES
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := sf.stop.sh
LOCAL_SRC_FILES := misc/sf.stop.sh
LOCAL_MODULE_CLASS := EXECUTABLES
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := init.debughelper.rc
LOCAL_SRC_FILES := misc/init.debughelper.rc
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_RELATIVE_PATH := ../root
include $(BUILD_PREBUILT)



# build test exe
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	testdebug.cpp

LOCAL_SHARED_LIBRARIES := \
	libdebughelper \
	libcutils \
	liblog \
	libbinder \
	libutils \


LOCAL_MODULE:= testdebug

include $(BUILD_EXECUTABLE)
