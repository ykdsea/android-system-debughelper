/*
 *  android system debug library;
 *
 *  Copyright (C) Amlogic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SYSTEM_DEBUG_HELPER_H_
#define SYSTEM_DEBUG_HELPER_H_


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#include "log/log.h"
#include "cutils/properties.h"

typedef void (*WatchCbk) (void* data);

class DebugHelper {
public:
    static DebugHelper* getInstance();

    //watch thread
    int createWatch(WatchCbk cbk, void*data, int intervalMs);
    void startWatch();
    void stopWatch();
    void destroyWatch();
    static void* watchLoop(void*data);

    //core dump
    static void enableCoreDump();
    static void dumpCore();
    static void setSigToDumpCore(int signum);

    //dump kernel stack of current process
    static void dumpKernelStack(pid_t tid);
    static void dumpAllKernelStack();
    static void dumpIonUsage();
    static void dumpMaliUsage();
    static int getTaskComm(pid_t tid, char* tskname, size_t namelen);
    static int getIonMem();
    static int getMaliMem();


    //sysrq dump
    static void dumpKernelBlockStat();
    static void dumpKernelActiveCpuStat();
    static void dumpKernelMemoryStat();

    //misc functions
    void buildTracesFilePath(char* filepath);
    int getTraceCount() {return mTraceCount;}

    static bool isSystemServer();

    // function used to watch
    static void watchFdUsage(void* data);

public:
    enum WatchStat {WATCH_UNINIT =0, WATCH_INIT, WATCH_START, WATCH_STOP};
    struct WatchData {
            suseconds_t intervMs;
            int trigTimes;
            WatchStat watchStatus;
            WatchCbk cbk;
            void* watchData;
            pthread_mutex_t mutex;
            pthread_cond_t cond;
    };

private:
    DebugHelper();

    static int writeToFile(const char*path, const char* content, const size_t len);
    static int readFileToStr(const char* filepath, char* result, const size_t result_len);

    static DebugHelper * gInstance;
    int mTraceCount;

    //for watch thread
    pthread_t mWatchThread;
    WatchData mWatchData;
};

#endif
