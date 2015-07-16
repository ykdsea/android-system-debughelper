#define LOG_TAG "DebugHelper"

#include "debughelper.h"


static void dumpCoreAction(int, siginfo_t*, void* sigcontext) {
    kill(getpid(),SIGSEGV);
}

void DebugHelper::enableCoreDump() {
    //default is handled by debuggerd, reset it to default handler.
    signal(SIGSEGV, SIG_DFL);

    //default core limit is set to 0, set it to infinitiy;
     struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
     if (setrlimit(RLIMIT_CORE, &rl) < 0) 
            ALOGE("enable core dump fail");
    else
           ALOGE("enable core dump successfully");
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

    if(readFileToStr(path, tskname, namelen) != 0) {
        return -1;
    }
    return 0;
}

void DebugHelper::dumpKernelBlockStat() {
    //writeToFile("/proc/sysrq-trigger","w",1);
    writeToFile("/proc/sysrq-trigger","l",1);
}

void DebugHelper::dumpKernelMemoryStat() {
    writeToFile("/proc/sysrq-trigger","m",1);
}

void DebugHelper::buildTracesFilePath(char* filepath) {
    sprintf(filepath, "/data/anr/traces-%d-%d.txt", getpid(), iTraceCount);
    iTraceCount ++;
}

bool DebugHelper::isSystemServer(){
    char taskname[64] = {0};
    getTaskComm(getpid(), taskname, 64);
    return (strncmp(taskname, "system_server", strlen("system_server")) == 0);
}

int DebugHelper::writeToFile(const char*path, const char* content, const size_t len) {
    int fd = open(path, O_WRONLY);
    if(fd < 0) {
        ALOGE("Write %s to file %s failed!!", content, path);
        return -1;
    }

    size_t writesize = write(fd,content,len);
    close(fd);
    if( writesize != len) {
        ALOGE("Write %zu len to %s failed, only %zu written", len, path, writesize);
        return -1;
    }
    return 0;
}

int DebugHelper::readFileToStr(const char* filepath, char* result, const size_t result_len) {
    char content[256];
    int fd = open(filepath, O_RDONLY);
    if(fd < 0){
        return -1;
    }

    int readsize = read(fd, result, result_len);
    close(fd);
    if (readsize <=0)
        return -1;
    return 0;
}


