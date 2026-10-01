#include "../EncoderButton.h"
#include <cassert>
#include <cstdio>
int main() {
    rotary::EncoderButton b; unsigned t=0; int holds=0,clicks=0; bool bypass=false;
    auto run=[&](bool down,unsigned count) { for(unsigned i=0;i<count;++i) {
        auto e=b.Process(down,t++); holds+=e.hold; clicks+=e.click;
        if(e.hold) bypass=!bypass;
    }};
    run(false,1000); assert(!bypass && holds==0 && clicks==0);
    run(true,100); run(false,100); assert(clicks==1 && !bypass);
    run(true,1000); assert(bypass && holds==1);
    for(int i=0;i<20;++i) { run(false,5); run(true,100); }
    assert(bypass && holds==1); // contact glitches during hold cannot re-arm
    run(false,1000); assert(bypass && holds==1 && clicks==1);
    run(true,2000); run(false,1000); assert(!bypass && holds==2 && clicks==1);
    rotary::EncoderButton bootHeld;
    for(unsigned i=0;i<2000;++i) assert(!bootHeld.Process(true,i).hold);
    for(unsigned i=2000;i<2100;++i) { auto e=bootHeld.Process(false,i); assert(!e.hold && !e.click); }
    rotary::EncoderButton wrap;
    unsigned now=0xffffff00u;
    for(unsigned i=0;i<100;++i) wrap.Process(false,now++);
    int transitions=0;
    for(unsigned i=0;i<2000;++i) transitions+=wrap.Process(true,now++).hold;
    assert(transitions==1);
    puts("PASS: boot enabled, short click, one-shot hold, release latch, contact glitches, second toggle, boot held, clock wrap");
}
