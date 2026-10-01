#include "../PanelMapping.h"
#include "../ExternalControls.h"
#include <cstdio>
#include <cstdlib>
int main() {
    using namespace rotary;
    if(RightCabinet(2)!=0 || RightCabinet(0)!=1 || RightCabinet(1)!=2) return EXIT_FAILURE;
    if(LeftMode(2)!=Mode::Tremolo || LeftMode(0)!=Mode::Chorale || LeftMode(1)!=Mode::Brake) return EXIT_FAILURE;
    ExternalControls cv; cv.Init();
    for(int i=0;i<10;++i) cv.Process(.5f,true);
    if(cv.RequestedMode(LeftMode(1))!=Mode::Brake) return EXIT_FAILURE;
    if(cv.RequestedMode(LeftMode(0))!=Mode::Tremolo) return EXIT_FAILURE;
    puts("PASS: physical toggle up/center/down mappings, physical-down Brake wins over Hit");
}
