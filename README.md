# android-system-debughelper
lib provides useful apis for debugging android system issue;


#1, how to use libdebughelper.so
a) add following lines in your android.mk
LOCAL_C_INCLUDES += external/libdebughelper/
LOCAL_SHARED_LIBRARIES += libdebughelper
b) call DebugHelper debug api;

#2, dump binder state
a) import "init.debughelper.rc" to init.rc
b) user setprop "sys.debughelper.dump.binder" to "true" to dump binder state

#3, surfaceflinger core dump
a) import "init.debughelper.rc" to init.rc
b) call DebugHelper::enableCoreDump() in surfaceflinger main();
c) setprop "sys.debughelper.stop.sf" to "true" to send SIGESGV to surfaceflinger


#ATTENTATION:
a)Use libcutils may cause crash. (Ex, when use debughelper in libbacktrace, Android will can't boot up.)
