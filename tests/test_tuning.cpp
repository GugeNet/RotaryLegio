#include "../RotaryDSP.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
int main() {
    using namespace rotary;
    RotarySpeaker effect; effect.Init(48000); Parameters p;
    for(int i=0;i<48000*20;++i) effect.Process(0,0);
    CHECK(std::fabs(effect.HornHz()-.77f)<.001f);
    CHECK(std::fabs(effect.DrumHz()-.72f)<.001f);
    const float initialHorn=effect.HornHz(),initialDrum=effect.DrumHz();
    p.mode=Mode::Tremolo; effect.SetParameters(p);
    CHECK(effect.HornHz()==initialHorn && effect.DrumHz()==initialDrum);
    for(int i=0;i<48000*6;++i) {
        effect.Process(0,0);
        if(i==86400-1) CHECK((effect.HornHz()-initialHorn)/(6.9f-initialHorn)>.949f);
    }
    CHECK(std::fabs((effect.DrumHz()-initialDrum)/(6.4f-initialDrum)-.950213f)<.001f);
    for(int i=0;i<48000*20;++i) effect.Process(0,0);
    CHECK(std::fabs(effect.HornHz()-6.9f)<.001f && std::fabs(effect.DrumHz()-6.4f)<.001f);
    p.speed=1.5f; effect.SetParameters(p);
    for(int i=0;i<48000*20;++i) effect.Process(0,0);
    CHECK(std::fabs(effect.HornHz()-10.35f)<.001f && std::fabs(effect.DrumHz()-9.6f)<.001f);
    const float fastHorn=effect.HornHz(),fastDrum=effect.DrumHz();
    p.mode=Mode::Brake; effect.SetParameters(p);
    for(int i=0;i<259200;++i) {
        effect.Process(0,0);
        if(i==115200-1) CHECK(std::fabs(effect.HornHz()/fastHorn-.049787f)<.001f);
    }
    CHECK(std::fabs(effect.DrumHz()/fastDrum-.049787f)<.001f);
    for(int i=0;i<48000*20;++i) effect.Process(0,0);
    CHECK(effect.HornHz()==0 && effect.DrumHz()==0);
    const float stoppedHorn=effect.HornAngle(),stoppedDrum=effect.DrumAngle();
    for(int i=0;i<48000;++i) effect.Process(0,0);
    CHECK(effect.HornAngle()==stoppedHorn && effect.DrumAngle()==stoppedDrum);
    puts("PASS: full-engine nominal/max rotor speeds, 95% ramp targets, no speed jump, brake decay and phase hold");
}
