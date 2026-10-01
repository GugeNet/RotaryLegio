#define _CRT_SECURE_NO_WARNINGS
#include "../RotaryDSP.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

static void Check(bool ok,const char* message) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
int main(int argc,char** argv) {
    using namespace rotary;
    constexpr float fs=48000;
    RotorState h,d; h.Init(fs,0,1.2f,1.8f); d.Init(fs,1,4,5);
    for(int i=0;i<48000;++i) { h.Process(6.8f); d.Process(5.7f); }
    Check(std::fabs(h.hz-6.8f*(1-std::exp(-1/1.2f)))<.001f,"horn acceleration matches exponential");
    Check(std::fabs(d.hz-5.7f*(1-std::exp(-.25f)))<.001f,"drum acceleration matches exponential");
    const float initial=h.hz;
    for(int i=0;i<48000;++i) h.Process(0);
    Check(std::fabs(h.hz-initial*std::exp(-1/1.8f))<.001f,"brake deceleration matches exponential");
    Check(h.angle>=0 && h.angle<2*pi,"rotor angle remains wrapped");
    FractionalDelay<64> delay; delay.Clear();
    for(int i=0;i<1000;++i) {
        delay.Push(float(i));
        if(i>70) {
            Check(std::fabs(delay.Read(10)-float(i-10))<.001f,"integer delay including buffer wrap");
            Check(std::fabs(delay.Read(10.5f)-(i-10.5f))<.001f,"fractional delay reproduces ramp");
        }
    }
    for(float f : {40.f,200.f,800.f,2000.f,10000.f}) {
        CrossoverLR4 c; c.Init(fs); double input=0,output=0;
        for(int i=0;i<96000;++i) {
            float x=std::sin(2*pi*f*i/fs),lo,hi; c.Process(x,lo,hi);
            if(i>=48000) { input+=x*x; output+=(lo+hi)*(lo+hi); }
        }
        Check(std::fabs(10*std::log10(output/input))<.06,"LR4 recombination magnitude flat");
    }
    RotorBand band; band.Init(fs,true); band.UpdatePaths(1,.8f,1,true);
    float last=band.DelaySamples(0), minimum=10000, maximum=0;
    for(int i=0;i<480000;++i) {
        band.rotor.Process(6.8f);
        if(i%16==0) band.UpdatePaths(1,.8f,1);
        band.Process(.1f);
        const float current=band.DelaySamples(0);
        Check(std::fabs(current-last)<.0402f,"delay slew bounded; positive read speed");
        last=current;
        if(i>400000) { minimum=std::min(minimum,current); maximum=std::max(maximum,current); }
    }
    Check(maximum-minimum>40,"rotor produces geometric Doppler excursion");
    band.UpdatePaths(0,.8f,1,true); last=band.DelaySamples(0);
    for(int i=0;i<48000;++i) {
        band.rotor.Process(6.8f);
        if(i%16==0) band.UpdatePaths(0,.8f,1);
        band.Process(.1f);
        Check(std::fabs(band.DelaySamples(0)-last)<.0001f,"zero depth removes delay modulation");
    }
    RotarySpeaker effect; effect.Init(fs); Parameters p; p.geometry=0;
    double difference=0,energy=0;
    for(int i=0;i<480000;++i) {
        effect.SetParameters(p); const float x=.3f*std::sin(2*pi*1000*i/fs);
        const auto y=effect.Process(x,x);
        if(i>400000) { difference+=std::fabs(y.left-y.right); energy+=y.left*y.left; }
    }
    Check(difference<.001,"zero mic spread collapses output to mono");
    Check(energy>1,"effect produces nonzero audio");
    effect.Init(fs); p.geometry=1; p.mode=Mode::Tremolo;
    float peak=0; double stereoDiff=0;
    unsigned random=1234567;
    for(int i=0;i<1440000;++i) {
        if(i%12000==0) {
            p.depth=float((i/12000)%2); p.geometry=((i/12000)%3)*.5f;
            p.drive=((i/12000)%4)/3.f; p.balance=((i/12000)%5)/4.f;
            p.cabinet=(i/12000)%3; p.speed=((i/12000)%2) ? .5f : 1.5f;
            p.mode=(i/12000)%3==0 ? Mode::Brake : (i/12000)%3==1 ? Mode::Chorale : Mode::Tremolo;
            effect.SetParameters(p);
        }
        random=1664525*random+1013904223;
        float x=float((random>>8)&65535)/32768.f-1;
        const auto y=effect.Process(x,-.4f*x);
        Check(std::isfinite(y.left) && std::isfinite(y.right),"finite output under parameter stress");
        peak=std::max(peak,std::max(std::fabs(y.left),std::fabs(y.right)));
        Check(effect.MaxDelay()>2 && effect.MaxDelay()<2045,"delay stays in allocated storage");
        stereoDiff+=std::fabs(y.left-y.right);
    }
    Check(peak<=1.001f,"bounded output under full-scale stress");
    Check(stereoDiff>100,"nonzero spread creates stereo difference");
    p.bypass=true; effect.SetParameters(p);
    Stereo y{};
    for(int i=0;i<48000;++i) y=effect.Process(.2f,-.3f);
    Check(std::fabs(y.left-.2f)<.00001f && std::fabs(y.right+.3f)<.00001f,"bypass preserves stereo channels");
    std::printf("PASS: inertia, brake, delay interpolation/wrap, crossover, Doppler excursion/slew/depth-zero, mono collapse, stereo spread, 30s stress, bypass\nPeak stress output: %.6f\nDSP object: %zu bytes\n",peak,sizeof(effect));

    if(argc>1) {
        FILE* file=std::fopen(argv[1],"wb"); Check(file!=nullptr,"open offline render");
        effect.Init(fs); p=Parameters{}; p.geometry=.8f; p.depth=1;
        // Synthetic sustained organ-like chord; 4s dry, 8s slow, 12s fast, 10s brake.
        for(int i=0;i<int(34*fs);++i) {
            const float t=i/fs; p.bypass=t<4;
            p.mode=t<12 ? Mode::Chorale : t<24 ? Mode::Tremolo : Mode::Brake;
            effect.SetParameters(p);
            float x=0;
            for(float f : {130.8128f,164.8138f,195.9977f})
                x+=.055f*(std::sin(2*pi*f*t)+.45f*std::sin(4*pi*f*t)+.2f*std::sin(8*pi*f*t));
            const auto sample=effect.Process(x,x);
            const float pair[2]={sample.left,sample.right};
            Check(std::fwrite(pair,sizeof(float),2,file)==2,"write render");
        }
        Check(std::fclose(file)==0,"close render");
    }
}
