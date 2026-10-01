#include "../RotaryDSP.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
float Gain(float f,int preset) {
    rotary::CabinetVoicing filter; filter.Init(48000);
    double inEnergy=0,outEnergy=0;
    for(int i=0;i<96000;++i) {
        const float x=.1f*std::sin(2*rotary::pi*f*i/48000.f);
        const auto y=filter.Process({x,-x},float(preset));
        CHECK(std::fabs(y.left+y.right)<.000001f);
        if(i>48000) { inEnergy+=x*x; outEnergy+=y.left*y.left; }
    }
    return float(10*std::log10(outEnergy/inEnergy));
}
int main() {
    const float open=Gain(6000,0),classic=Gain(6000,1),dark=Gain(6000,2);
    CHECK(open>3 && dark<-6 && open-dark>9);
    CHECK(std::fabs(classic)<.00001f);
    CHECK(Gain(100,0)<-1 && Gain(100,2)>.5f);
    rotary::CabinetVoicing filter; filter.Init(48000);
    for(int i=0;i<10000;++i) {
        const float x=float(i%100)/100.f;
        const auto y=filter.Process({x,-x},1);
        CHECK(y.left==x && y.right==-x); // center voicing is exactly unchanged
    }
    for(int i=0;i<48000;++i) filter.Process({.1f,.1f},0);
    float previous=filter.Process({.1f,.1f},0).left, maxStep=0;
    for(int i=0;i<48000;++i) {
        const auto y=filter.Process({.1f,.1f},2);
        maxStep=std::max(maxStep,std::fabs(y.left-previous)); previous=y.left;
    }
    CHECK(maxStep<.0001f);
    std::printf("PASS: cabinet response and transition smoothing; 6kHz Open %.2f dB, Classic %.2f dB, Dark %.2f dB\n",open,classic,dark);
}
