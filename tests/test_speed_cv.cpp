#include "../ExternalControls.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
using namespace rotary;
static void settle(ExternalControls& c,float volts,bool hit=false) {
    for(int i=0;i<1500;++i) c.Process(.25f+volts*.125f,hit);
}
static bool near(float a,float b) { return std::fabs(a-b)<.0001f; }
int main() {
    ExternalControls c; Parameters p;
    c.Init(); c.Apply(p,1,Mode::Chorale);
    CHECK(p.continuousSpeed && near(p.hornTargetHz,.77f));
    for(float trim : {.5f,1.f,1.5f}) {
        for(float volts : {-2.f,0.f,1.f,2.5f,5.f,7.f}) {
            settle(c,volts); c.Apply(p,trim,Mode::Chorale);
            float fraction=Clamp(volts,0,5)/5;
            CHECK(near(p.hornTargetHz,.77f+(6.9f*trim-.77f)*fraction));
            CHECK(near(p.drumTargetHz,.72f+(6.4f*trim-.72f)*fraction));
            settle(c,volts,true); c.Apply(p,trim,Mode::Chorale);
            CHECK(near(p.hornTargetHz,6.9f*trim) && near(p.drumTargetHz,6.4f*trim));
            c.Apply(p,trim,Mode::Brake); CHECK(p.hornTargetHz==0 && p.drumTargetHz==0);
            settle(c,volts,false); c.Apply(p,trim,Mode::Tremolo);
            CHECK(near(p.hornTargetHz,6.9f*trim));
            c.Apply(p,trim,Mode::Chorale);
            CHECK(near(p.hornTargetHz,.77f+(6.9f*trim-.77f)*fraction));
        }
    }
    settle(c,0); c.Process(.25f,true); c.Process(.25f,true); CHECK(!c.GateActive());
    c.Process(.25f,true); CHECK(c.GateActive());
    settle(c,5); c.Init(); settle(c,0); c.Apply(p,1,Mode::Chorale);
    CHECK(near(p.hornTargetHz,.77f) && !c.GateActive());
    for(int i=0;i<1500;++i) c.Process(std::numeric_limits<float>::quiet_NaN(),false);
    c.Apply(p,1,Mode::Chorale); CHECK(near(p.hornTargetHz,.77f));
    RotarySpeaker effect; effect.Init(48000);
    settle(c,5); c.Apply(p,1.5f,Mode::Chorale); effect.SetParameters(p);
    for(int i=0;i<48000*25;++i) effect.Process(0,0);
    CHECK(near(effect.HornHz(),10.35f) && near(effect.DrumHz(),9.6f));
    float before=effect.HornHz();
    settle(c,0); c.Apply(p,1.5f,Mode::Chorale); effect.SetParameters(p);
    CHECK(effect.HornHz()==before);
    for(int i=0;i<48000*25;++i) effect.Process(0,0);
    CHECK(near(effect.HornHz(),.77f) && near(effect.DrumHz(),.72f));
    c.Apply(p,1,Mode::Brake); effect.SetParameters(p);
    for(int i=0;i<48000*30;++i) effect.Process(0,0);
    CHECK(effect.HornHz()==0 && effect.DrumHz()==0);
    float h=effect.HornAngle(), d=effect.DrumAngle();
    for(int i=0;i<48000;++i) effect.Process(0,0);
    CHECK(effect.HornAngle()==h && effect.DrumAngle()==d);
    puts("PASS: no-calibration startup, CV range, fixed Slow, adjustable shared maximum, Hit/release/Fast/Brake, debounce, invalid input, engine inertia and stop");
}
