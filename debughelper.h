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

#define SLEEP_TIME 1*1e4
#define DUMP_TIME 20*60*1e6 //20 minutes

class DebugHelper {
public:
    static DebugHelper* getInstance();

    //core dump
    static void enableCoreDump();
    static void dumpCore();
    static void setSigToDumpCore(int signum);

    //dump kernel stack of current process
    static void dumpKernelStack(pid_t tid);
    static void dumpAllKernelStack();
    static int getTaskComm(pid_t tid, char* tskname, size_t namelen);

    //sysrq dump
    static void dumpKernelBlockStat();
    static void dumpKernelActiveCpuStat();
    static void dumpKernelMemoryStat();

    //misc functions
    void buildTracesFilePath(char* filepath);
    int getTraceCount() {return m_iTraceCount;}

    static bool isSystemServer();

    // dump file descriptor perodically
    static bool dumpFds();

private:
    DebugHelper(){m_iTraceCount = 0;}

    static int writeToFile(const char*path, const char* content, const size_t len);
    static int readFileToStr(const char* filepath, char* result, const size_t result_len);

    static DebugHelper * gInstance;
    int m_iTraceCount;
};

#endif
