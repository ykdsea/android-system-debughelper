# android-system-debughelper
lib provides useful apis for debugging android system issue;


#1, how to use libdebughelper.so
a) add following lines in your android.mk
LOCAL_C_INCLUDES += external/libdebughelper/
LOCAL_SHARED_LIBRARIES += libdebughelper
b) import "init.debughelper.rc" to init.rc
c) call DebugHelper debug api;

#2, dump binder state
setprop "sys.debughelper.dump.binder" to "true" to dump binder state

#3, surfaceflinger core dump
a) call DebugHelper::enableCoreDump() in surfaceflinger main();
b) setprop "sys.debughelper.stop.sf" to "true" to send SIGESGV to surfaceflinger

#4, watch functon
a) call DebugHelper::createWatch(WatchCbk cbk, int intervalMs) to start watch thread;
b) call DebugHelper::startWatch() when you want to watch;
c) call DebugHelper::stopWatch() to pause the watch thread;
d) call DebugHelper::destroyWatch() to destory watch thread;

#4, dump fd usage, for checke memory leak
use watch function to check fd looply. Samples:
DebugHelper::getInstance()->createWatch(DebugHelper::watchFdUsage, 20000); //check fd usage every 20 seconds.



#ATTENTATION:
a)Use libcutils may cause crash. (Ex, when use debughelper in libbacktrace, Android will can't boot up.)
