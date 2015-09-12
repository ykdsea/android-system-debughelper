#include <debughelper.h>


int main(){
    long long maliMem = DebugHelper::getMaliMem();
    long long ionMem = DebugHelper::getIonMem();
    
    ALOGE("!!TOO MUCH MEM USED FOR GRAPHIC: mali(%lld), ion(%lld)", maliMem, ionMem);
    DebugHelper::dumpIonUsage();
    DebugHelper::dumpMaliUsage();    

    return 1;
}
