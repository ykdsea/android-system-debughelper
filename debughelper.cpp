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

#define LOG_TAG "DebugHelper"

#include "debughelper.h"

DebugHelper* DebugHelper::gInstance = NULL;

static void dumpCoreAction(int, siginfo_t*, void* sigcontext) {
    kill(getpid(),SIGSEGV);
}

DebugHelper::DebugHelper() {
    mTraceCount = 0;
    memset(&mWatchData, 0, sizeof(mWatchData));
    pthread_mutex_init(&(mWatchData.mutex), NULL);
    pthread_cond_init(&(mWatchData.cond), NULL);
}

DebugHelper* DebugHelper::getInstance() {
    if (gInstance == NULL) {
        gInstance = new DebugHelper();
    }
    return gInstance;
}

void DebugHelper::enableCoreDump() {
    //default is handled by debuggerd, reset it to default handler.
    signal(SIGSEGV, SIG_DFL);

    //default core limit is set to 0, set it to infinitiy;
     struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
     if (setrlimit(RLIMIT_CORE, &rl) < 0) {
            ALOGE("enable core dump fail");
    } else {
           ALOGE("enable core dump successfully");
    }
}

void DebugHelper::dumpCore() {
    kill(getpid(),SIGSEGV);
}

void DebugHelper::setSigToDumpCore(int signum) {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = dumpCoreAction;
    act.sa_flags =  SA_RESTART | SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    sigaction(signum, &act, 0);
}

void DebugHelper::dumpKernelStack(pid_t tid) {
    const size_t stack_content_len = 4096;
    char comm_name[64] = {0};
    char stack_file[64]= {0};
    char *stack_content = (char*)malloc(stack_content_len);
    stack_content[0] = 0;
    sprintf(stack_file,"/proc/self/task/%d/stack",tid);

    getTaskComm(tid, comm_name, 64);
    readFileToStr(stack_file, stack_content, stack_content_len);

    ALOGE("Dump kernel stack of %d : %s-------", tid, comm_name);
    ALOGE("%s",stack_content);
    ALOGE(" --------------------------- ");

    free(stack_content);
    stack_content = 0;
}

void DebugHelper::dumpAllKernelStack() {
    //dump all task kernel stacks
    ALOGE("-----dump all theads's kernel stack of pid %d -----\n", getpid() );

    DIR* d = opendir("/proc/self/task");
    if (!d) {
        return;
    }

    dirent* e;
    while ((e = readdir(d)) != NULL) {
        char* end;
        pid_t tid = strtol(e->d_name, &end, 10);
        if (!*end) {
            dumpKernelStack(tid);
        }
    }
    closedir(d);
}

void DebugHelper::dumpIonUsage() {
    //dump ion usage
    const size_t content_len = 4096;
    char * content = (char*)malloc(content_len);

    //dump system heap
    content[0] = 0;
    readFileToStr("/sys/kernel/debug/ion/heaps/vmalloc_ion", content, content_len);
    ALOGE("ION System Heap: ");
    ALOGE("%s",content);

    //dump carveout heap
    content[0] = 0;
    readFileToStr("/sys/kernel/debug/ion/heaps/carveout_ion", content, content_len);
    ALOGE("ION Carveout Heap(reserver): ");
    ALOGE("%s",content);

    free(content);
}

int DebugHelper::getTaskComm(pid_t tid, char* tskname, size_t namelen) {
    char path[128]={0};
    sprintf(path,"/proc/%d/comm", tid);

    if (readFileToStr(path, tskname, namelen) != 0) {
        return -1;
    }
    return 0;
}

void DebugHelper::dumpKernelBlockStat() {
    writeToFile("/proc/sysrq-trigger","w",1);
}

void DebugHelper::dumpKernelActiveCpuStat() {
    writeToFile("/proc/sysrq-trigger","l",1);
}

void DebugHelper::dumpKernelMemoryStat() {
    writeToFile("/proc/sysrq-trigger","m",1);
}

void DebugHelper::buildTracesFilePath(char* filepath) {
    sprintf(filepath, "/data/anr/traces-%d-%d.txt", getpid(), mTraceCount);
    mTraceCount ++;
}

bool DebugHelper::isSystemServer(){
    char taskname[64] = {0};
    getTaskComm(getpid(), taskname, 64);
    return (strncmp(taskname, "system_server", strlen("system_server")) == 0);
}

int DebugHelper::writeToFile(const char*path, const char* content, const size_t len) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        ALOGE("Write %s to file %s failed!!", content, path);
        return -1;
    }

    size_t writesize = write(fd,content,len);
    close(fd);
    if (writesize != len) {
        ALOGE("Write %zu len to %s failed, only %zu written", len, path, writesize);
        return -1;
    }
    return 0;
}

int DebugHelper::readFileToStr(const char* filepath, char* result, const size_t result_len) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    int readsize = read(fd, result, result_len);
    close(fd);
    if (readsize <=0)
        return -1;
    return 0;
}


//*****************Watch function****************
#define SLEEP_AND_WAIT_STATUS_CHANGE(sec, nsec) \
    do {\
        int ret = 0;\
        struct timespec waittime;\
        struct timeval now;\
        gettimeofday(&now, NULL);\
        waittime.tv_sec = now.tv_sec + sec;\
        waittime.tv_nsec = now.tv_usec * 1000 + nsec;\
        do {\
            ret = pthread_cond_timedwait(&(pdata->cond), &(pdata->mutex), &waittime);\
        } while (ret && (ret != ETIMEDOUT));\
    } while(0);

void* DebugHelper::watchLoop(void*data) {
    WatchData* pdata = (WatchData*)data;

    while (true) {
        pthread_mutex_lock(&(pdata->mutex));
        switch (pdata->watchStatus) {
            case WATCH_START:
                if(pdata->cbk != NULL) {
                    pdata->trigTimes ++;
                    ALOGE("Watche callback triggered (%d) .", pdata->trigTimes);
                    pdata->cbk();
                    SLEEP_AND_WAIT_STATUS_CHANGE(0, pdata->intervMs*1000*1000);
                } else {
                    ALOGE("No watch callbak, looping empty !");
                    SLEEP_AND_WAIT_STATUS_CHANGE(10, 0);
                }
                break;
            case WATCH_INIT:
                ALOGD("Watch init, wait to START");
                SLEEP_AND_WAIT_STATUS_CHANGE(10, 0);
                break;
            case WATCH_STOP:
                ALOGD("Watch stoped, wait to RESTART or EXIT");
                SLEEP_AND_WAIT_STATUS_CHANGE(10, 0);
                break;
            case WATCH_UNINIT:
                ALOGE("Watch Exit watch thread!");
                pthread_mutex_unlock(&(pdata->mutex));
                pthread_exit(NULL);
                break;
            default:
                ALOGE("error watch status %d", pdata->watchStatus);
                break;
        }
        pthread_mutex_unlock(&(pdata->mutex));
    }

    return NULL;
}

int DebugHelper::createWatch(WatchCbk cbk, int intervalMs) {
    if(mWatchData.watchStatus == WATCH_UNINIT) {
        mWatchData.watchStatus = WATCH_INIT;
        mWatchData.intervMs = intervalMs;
        mWatchData.cbk = cbk;
        mWatchData.trigTimes = 0;
        pthread_mutex_init(&(mWatchData.mutex), NULL);
        pthread_cond_init(&(mWatchData.cond), NULL);

        if (0 != pthread_create(&mWatchThread, NULL, DebugHelper::watchLoop, (void*)(&mWatchData))) {
            memset(&mWatchData,0,sizeof(mWatchData));
            return -1;
        }
        return 0;
    } else {
        ALOGE("Watch already created!");
        return -1;
    }
}

void DebugHelper::startWatch() {
    if (mWatchData.watchStatus > WATCH_UNINIT) {
        pthread_mutex_lock(&(mWatchData.mutex));
        mWatchData.watchStatus = WATCH_START;
        mWatchData.trigTimes = 0;
        pthread_cond_signal(&(mWatchData.cond));
        pthread_mutex_unlock(&(mWatchData.mutex));
    } else {
        ALOGE("Watch havn't init!");
    }
}

void DebugHelper::stopWatch() {
    if (mWatchData.watchStatus == WATCH_START) {
        pthread_mutex_lock(&(mWatchData.mutex));
        mWatchData.watchStatus = WATCH_STOP;
        pthread_cond_signal(&(mWatchData.cond));
        pthread_mutex_unlock(&(mWatchData.mutex));
    } else {
        ALOGE("Watch havn't start!");
    }
}

void DebugHelper::destroyWatch() {
    if (mWatchData.watchStatus > WATCH_UNINIT) {
        pthread_mutex_lock(&(mWatchData.mutex));
        mWatchData.watchStatus = WATCH_UNINIT;
        pthread_cond_signal(&(mWatchData.cond));
        pthread_mutex_unlock(&(mWatchData.mutex));

        if (pthread_join(mWatchThread, NULL) != 0)
            ALOGE("wait watch thread exit fail!\n");
        pthread_mutex_destroy(&(mWatchData.mutex));
        pthread_cond_destroy(&(mWatchData.cond));
    } else {
        ALOGE("Watch havn't init!");
    }
}

//**************Watch functions for dump*******************
void DebugHelper::watchFdUsage() {
    static bool bDumpDetail = true;
    char name[128], link[128], comm[128];
    char *fdlink;
    DIR *dp, *d_fd;
    struct dirent *entry, *dirp;
    int pid, baseofs, readsize, fd_count;

    if ((dp = opendir("/proc")) == NULL) {
        ALOGE("open /proc error\n");
        return ;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;   // ignore dot and dot-dot

        pid = atoi(dirp->d_name);

        if (pid <= 0)
            continue;

        baseofs = sprintf(name, "/proc/%d/fd/", pid);
        DebugHelper::getTaskComm(pid, comm, sizeof(comm));

        for (int i = 0; i < sizeof(comm); i++) {
            if (comm[i] == '\n') {
                comm[i] = '\0';
                break;
            }
        }

        fd_count = 0;

        if ((d_fd = opendir(name)) == NULL) {
            ALOGD("open %s error\n", name);
        } else {
            while ((entry = readdir(d_fd)) != NULL) {
                /* Skip entries '.' and '..' (and any hidden file) */
                if (entry->d_name[0] == '.')
                    continue;

                strncpy(name + baseofs, entry->d_name, 10);
                if ((readsize = readlink(name, link, sizeof(link))) == -1)
                    continue;

                link[readsize] = '\0';
                fd_count++;
                if (bDumpDetail) {
                    ALOGD("\t%d --> %s\t%s -- %s \n", pid, comm, entry->d_name, link);
                }
            }
        }

        ALOGD("%d --> %s\t total fds:%d\n", pid, comm, fd_count);
    }

    bDumpDetail = !bDumpDetail;
}

