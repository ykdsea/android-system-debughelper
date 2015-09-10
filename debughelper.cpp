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
    char *stack_content = (char*)calloc(stack_content_len, 1);
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

/* mali mem info sample
 /sys/kernel/debug/mali/gpu_memory                                          <
  Name (:bytes)              pid         mali_mem    max_mali_mem     external_mem     ump_mem     dma_mem   
==============================================================================================================
  RenderThread               11065       38064128    38064128         0                0           33177600  
  RenderThread               10780       20676608    65626112         0                0           0         
  RenderThread               3598        34230272    51191808         0                0           0         
  RenderThread               3356        2883584     11755520         0                0           2211840   
  surfaceflinger             2743        4440064     6791168          49766400         0           43683840  
Mali mem usage: 100294656
Mali mem limit: 1073741824
*/
#define MALI_MEM_DEBUG_PATH "/sys/kernel/debug/mali/gpu_memory"
#define MALI_MEM_PREFIX "Mali mem usage:"
long long DebugHelper::getMaliMem(){
    const size_t content_len = 4096;
    char * content = (char*)calloc(content_len, 1);
    long long totalsize = 0;

    //dump system heap
    content[0] = 0;
    readFileToStr(MALI_MEM_DEBUG_PATH, content, content_len);

    //get total size
    char* totalStr = strstr(content,MALI_MEM_PREFIX);
    if(totalStr != NULL) {
        sscanf(totalStr+strlen(MALI_MEM_PREFIX), "%lld", &totalsize);
        ALOGV("get mali mem total %lld\n", totalsize);
    }

    free (content);
    content = NULL;
    return totalsize;
}

void DebugHelper::dumpMaliUsage(){
     //dump mali usage
    const size_t content_len = 4096;
    char * content = (char*)calloc(content_len, 1);

    //dump system heap
    content[0] = 0;
    readFileToStr(MALI_MEM_DEBUG_PATH, content, content_len);
    ALOGE("Mali mem usage: ");
    ALOGE("%s",content);

    free(content);
}

/* ion memory sample
          client              pid             size
----------------------------------------------------
  surfaceflinger             2743         43687936
----------------------------------------------------
orphaned allocations (info is from last known client):
----------------------------------------------------
  total orphaned                0
          total          43687936
   deferred free                0
----------------------------------------------------
0 order 8 highmem pages in pool = 0 total
0 order 8 lowmem pages in pool = 0 total
0 order 4 highmem pages in pool = 0 total
0 order 4 lowmem pages in pool = 0 total
0 order 3 highmem pages in pool = 0 total
0 order 3 lowmem pages in pool = 0 total
0 order 2 highmem pages in pool = 0 total
0 order 2 lowmem pages in pool = 0 total
0 order 1 highmem pages in pool = 0 total
286 order 1 lowmem pages in pool = 2342912 total
0 order 0 highmem pages in pool = 0 total
1172 order 0 lowmem pages in pool = 4800512 total
*/

#define ION_SYSTEM_HEAP_DEBUG_PATH "/sys/kernel/debug/ion/heaps/vmalloc_ion"
#define ION_CARVEOUT_HEAP_DEBUG_PATH "/sys/kernel/debug/ion/heaps/carveout_ion"
#define ION_ORPHANED_MEM_PREFIX "total orphaned"
#define ION_MEM_PREFIX "total"

long long DebugHelper::getIonMem(){
    const size_t content_len = 4096;
    char * content = (char*)calloc(content_len, 1);
    long long totalsize = 0;

    //dump system heap
    content[0] = 0;
    readFileToStr(ION_SYSTEM_HEAP_DEBUG_PATH, content, content_len);
    //get total size
    char* tempStr = strstr(content,ION_ORPHANED_MEM_PREFIX);//skip orphaned info
    tempStr += strlen(ION_ORPHANED_MEM_PREFIX);
    char* totalStr = strstr(tempStr,ION_MEM_PREFIX);
    if(totalStr != NULL) {
        sscanf(totalStr+strlen(ION_MEM_PREFIX), "%lld", &totalsize);
        ALOGV("get vmalloc totalsize %lld\n", totalsize);
    }

    free (content);
    content = NULL;
    return totalsize;
}

void DebugHelper::dumpIonUsage() {
    //dump ion usage
    const size_t content_len = 4096;
    char * content = (char*)calloc(content_len, 1);

    //dump system heap
    content[0] = 0;
    readFileToStr(ION_SYSTEM_HEAP_DEBUG_PATH, content, content_len);
    ALOGE("ION System Heap: ");
    ALOGE("%s",content);

    //dump carveout heap
    content[0] = 0;
    readFileToStr(ION_CARVEOUT_HEAP_DEBUG_PATH, content, content_len);
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
                    ALOGV("Watche callback triggered (%d) .", pdata->trigTimes);
                    pdata->cbk(pdata->watchData);
                    SLEEP_AND_WAIT_STATUS_CHANGE(pdata->intervMs/1000, (pdata->intervMs%1000)*1000*1000);
                } else {
                    ALOGV("No watch callbak, looping empty !");
                    SLEEP_AND_WAIT_STATUS_CHANGE(30, 0);
                }
                break;
            case WATCH_INIT:
                ALOGV("Watch init, wait to START");
                SLEEP_AND_WAIT_STATUS_CHANGE(30, 0);
                break;
            case WATCH_STOP:
                ALOGV("Watch stoped, wait to RESTART or EXIT");
                SLEEP_AND_WAIT_STATUS_CHANGE(30, 0);
                break;
            case WATCH_UNINIT:
                ALOGV("Watch Exit watch thread!");
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

int DebugHelper::createWatch(WatchCbk cbk, void*data, int intervalMs) {
    if(mWatchData.watchStatus == WATCH_UNINIT) {
        mWatchData.watchStatus = WATCH_INIT;
        mWatchData.intervMs = intervalMs;
        mWatchData.cbk = cbk;
        mWatchData.watchData = data;
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
void DebugHelper::watchFdUsage(void* data) {
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

