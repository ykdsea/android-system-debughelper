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
    sprintf(filepath, "/data/anr/traces-%d-%d.txt", getpid(), m_iTraceCount);
    m_iTraceCount ++;
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
    char content[256];
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

static long elapseTime(struct timeval *t_now, struct timeval *t_prev)
{
    if (t_now == NULL || t_prev == NULL)
        return 0;

    return ((t_now->tv_sec * 1e6 + t_now->tv_usec)-(t_prev->tv_sec * 1e6 + t_prev->tv_usec));
}

static bool doDumpFds(int mCount) {
    char name[128], link[128], comm[128];
    char *fdlink;
    DIR *dp, *d_fd;
    struct dirent *entry, *dirp;
    int pid, baseofs, readsize, fd_count;

    if ((dp = opendir("/proc")) == NULL) {
        ALOGE("open /proc error\n");
        return false;
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
                if (mCount % 2 == 0) {
                    ALOGD("\t%d --> %s\t%s -- %s \n", pid, comm, entry->d_name, link);
                }
            }
        }

        ALOGD("%d --> %s\t total fds:%d\n", pid, comm, fd_count);
    }

    return true;
}


static void *runner(void *) {
    int mCount = 0;
    struct timeval mPrev;
    struct timeval mNow;

    gettimeofday(&mPrev, NULL);

    while (1) {
        gettimeofday(&mNow, NULL);
        if (elapseTime(&mNow, &mPrev) > DUMP_TIME) {
            mPrev = mNow;
            mCount++;
            if (!doDumpFds(mCount)) {
                ALOGD("exit fd dump thread\n");
                break;
            }
        }
        usleep(SLEEP_TIME);
    }

    pthread_exit(NULL);
}

bool DebugHelper::dumpFds() {
    pthread_t pth;
    int ret;

    ret = pthread_create(&pth, NULL, runner, NULL);
    if (ret != 0) {
        ALOGE(" create pthread error\n");
        return false;
    }

    ret = pthread_join(pth, NULL);
    if (ret != 0) {
        ALOGE(" Count not join pthread\n");
        return false;
    }

    return true;
}
