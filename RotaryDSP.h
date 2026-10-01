#pragma once
#include "RotorTuning.h"
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace rotary {
constexpr float pi = 3.14159265358979323846f;
inline float Clamp(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }
enum class Mode { Brake, Chorale, Tremolo };
struct Parameters {
    Mode mode = Mode::Chorale;
    float depth = .6f, geometry = .5f, speed = 1.f, balance = .5f, drive = .0f;
    int cabinet = 1; // open, classic, dark
    bool bypass = false;
    bool continuousSpeed = false;
    float hornTargetHz=0, drumTargetHz=0;
};
struct Stereo { float left, right; };

class Biquad {
    float b0=0,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
public:
    void Init(float fs, float fc, bool high) {
        const float w=2*pi*fc/fs, c=std::cos(w), s=std::sin(w);
        const float alpha=s/(2*.70710678118f), norm=1/(1+alpha);
        b0=(high ? (1+c) : (1-c))*.5f*norm;
        b1=(high ? -(1+c) : (1-c))*norm; b2=b0;
        a1=-2*c*norm; a2=(1-alpha)*norm; z1=z2=0;
    }
    float Process(float x) {
        const float y=b0*x+z1;
        z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y;
    }
};
class CrossoverLR4 {
    Biquad low[2], high[2];
public:
    void Init(float fs) { for(int i=0;i<2;++i) { low[i].Init(fs,800,false); high[i].Init(fs,800,true); } }
    void Process(float x, float& l, float& h) { l=low[1].Process(low[0].Process(x)); h=high[1].Process(high[0].Process(x)); }
};
class RotorState {
    float fs=48000, accel=0, decel=0, rounding=0;
public:
    float angle=0, hz=0;
    void Init(float rate, float phase, float up, float down) {
        fs=rate; angle=phase; hz=0; rounding=0;
        accel=-std::expm1(-1/(rate*up)); decel=-std::expm1(-1/(rate*down));
    }
    void Process(float target) {
        const float step=(target-hz)*(target>hz ? accel : decel)-rounding;
        const float next=hz+step; rounding=(next-hz)-step; hz=next;
        if(target==0 && hz<.0001f) { hz=0; rounding=0; }
        angle+=2*pi*hz/fs; if(angle>=2*pi) angle-=2*pi;
    }
};
template<size_t N> class FractionalDelay {
    static_assert((N & (N-1))==0, "Power of two required");
    float data[N]{}; size_t write=0;
public:
    void Clear() { for(auto& x:data) x=0; write=0; }
    void Push(float x) { write=(write+1)&(N-1); data[write]=x; }
    float Read(float delay) const {
        delay=Clamp(delay,2.f,float(N-3));
        float p=float(write)-delay; if(p<0) p+=N;
        const int i=int(p); const float t=p-i;
        const float a=data[(i-1)&(N-1)], b=data[i&(N-1)],
                    c=data[(i+1)&(N-1)], d=data[(i+2)&(N-1)];
        return b+.5f*t*(c-a+t*(2*a-5*b+4*c-d+t*(3*(b-c)+d-a)));
    }
};
struct Path { float delay=200, gain=.5f, tone=.3f; };
class RotorBand {
    FractionalDelay<2048> delay;
    Path current[2], delta[2], origin[2];
    unsigned interpolated=0;
    float toneState[2]{}, fs=48000, radius=.18f;
    bool horn=true;
public:
    RotorState rotor;
    void Init(float rate, bool isHorn) {
        fs=rate; horn=isHorn; radius=horn ? .18f : .24f;
        rotor.Init(fs,horn ? 0.f : 1.7f,
                   horn ? tuning::hornAccelerationTau : tuning::drumAccelerationTau,
                   horn ? tuning::hornDecelerationTau : tuning::drumDecelerationTau);
        delay.Clear();
        interpolated=0;
        for(int j=0;j<2;++j) { current[j]=origin[j]=Path{}; delta[j]=Path{0,0,0}; toneState[j]=0; }
    }
    // Geometry is evaluated every 16 samples; audio-rate interpolation below
    // keeps modulation continuous. All paths use this band's one rotor state.
    void UpdatePaths(float depth, float geometry, float cabinet, bool snap=false) {
        interpolated=0;
        const float distance=2.0f-1.25f*geometry;
        const float spread=geometry*.95f;
        for(int j=0;j<2;++j) {
            const float micAngle=j==0 ? -spread : spread;
            const float mx=distance*std::cos(micAngle), my=distance*std::sin(micAngle);
            float theta=rotor.angle;
            float sx=radius*std::cos(theta), sy=radius*std::sin(theta);
            float dx=mx-sx, dy=my-sy, dist=std::sqrt(dx*dx+dy*dy);
            const float initialDelay=distance+depth*(dist-distance);
            // One retarded-time refinement aligns directivity with the delayed
            // excitation rather than the receiver-time source orientation.
            theta-=2*pi*rotor.hz*initialDelay/343.f;
            const float cs=std::cos(theta), sn=std::sin(theta);
            dx=mx-radius*cs; dy=my-radius*sn; dist=std::sqrt(dx*dx+dy*dy);
            const float facing=Clamp((cs*dx+sn*dy)/dist,-1,1);
            const float forward=.5f*(facing+1);
            Path target;
            target.delay=fs*(distance+depth*(dist-distance))/343.f;
            const float directivity=horn ? .28f+.72f*forward*forward : .72f+.28f*forward;
            target.gain=Clamp(distance/dist,.65f,1.5f)*directivity;
            const float openness=1.f-.22f*cabinet;
            const float cutoff=(horn ? 2000+12500*forward : 650+2400*forward)*openness;
            const float w=2*pi*cutoff/fs;
            target.tone=w/(1+w); // stable one-pole coefficient; no exp in audio loop
            if(snap) current[j]=target;
            origin[j]=current[j];
            delta[j].delay=Clamp((target.delay-current[j].delay)/16.f,-.04f,.04f);
            delta[j].gain=(target.gain-current[j].gain)/16.f;
            delta[j].tone=(target.tone-current[j].tone)/16.f;
        }
    }
    Stereo Process(float x) {
        delay.Push(x); float out[2];
        if(interpolated<16) ++interpolated;
        const float fraction=static_cast<float>(interpolated);
        for(int j=0;j<2;++j) {
            current[j].delay=origin[j].delay+fraction*delta[j].delay;
            current[j].gain=origin[j].gain+fraction*delta[j].gain;
            current[j].tone=origin[j].tone+fraction*delta[j].tone;
            const float y=delay.Read(current[j].delay);
            toneState[j]+=current[j].tone*(y-toneState[j]);
            out[j]=toneState[j]*current[j].gain;
        }
        return {out[0],out[1]};
    }
    float DelaySamples(int j) const { return current[j].delay; }
};
// Broad cabinet tilt around 1 kHz. Center is exactly unity; its filter state
// keeps running so switching voicings does not expose an uninitialized filter.
class CabinetVoicing {
    float low[2]{}, pole=0, alpha=0, position=1;
public:
    void Init(float fs) {
        low[0]=low[1]=0; position=1;
        pole=-std::expm1(-2*pi*1000/fs);
        alpha=-std::expm1(-1/(fs*.03f));
    }
    Stereo Process(Stereo input,float requested) {
        position+=alpha*(Clamp(requested,0,2)-position);
        const float open=std::max(0.f,1-position), dark=std::max(0.f,position-1);
        const float bassDelta=-.15f*open+.12f*dark;
        const float trebleDelta=.60f*open-.60f*dark;
        float values[2]={input.left,input.right};
        for(int j=0;j<2;++j) {
            low[j]+=pole*(values[j]-low[j]);
            values[j]+=bassDelta*low[j]+trebleDelta*(values[j]-low[j]);
        }
        return {values[0],values[1]};
    }
};
class RotarySpeaker {
    CrossoverLR4 crossover;
    CabinetVoicing voicing;
    RotorBand horn, drum;
    Parameters target;
    float fs=48000, depth=.6f, geometry=.5f, balance=.5f, drive=0, cabinet=1;
    float paramAlpha=0, bypassAlpha=0, wet=0, dcX=0, dcY=0, dcPole=0;
    unsigned tick=0;
public:
    void Init(float rate) {
        fs=rate; target=Parameters{}; depth=.6f; geometry=.5f; balance=.5f; drive=0; cabinet=1;
        wet=0; dcX=dcY=0; tick=0;
        paramAlpha=1-std::exp(-16/(fs*.15f));
        bypassAlpha=1-std::exp(-1/(fs*.03f)); dcPole=std::exp(-2*pi*15/fs);
        crossover.Init(fs); voicing.Init(fs); horn.Init(fs,true); drum.Init(fs,false);
        horn.UpdatePaths(depth,geometry,cabinet,true); drum.UpdatePaths(depth,geometry,cabinet,true);
    }
    void SetParameters(Parameters p) {
        p.depth=Clamp(p.depth,0,1); p.geometry=Clamp(p.geometry,0,1);
        p.balance=Clamp(p.balance,0,1); p.drive=Clamp(p.drive,0,1);
        p.speed=Clamp(p.speed,.5f,1.5f);
        p.hornTargetHz=Clamp(p.hornTargetHz,0,1.5f*tuning::hornFastHz);
        p.drumTargetHz=Clamp(p.drumTargetHz,0,1.5f*tuning::drumFastHz);
        p.cabinet=std::max(0,std::min(2,p.cabinet)); target=p;
    }
    Stereo Process(float left, float right) {
        const float hTarget=target.mode==Mode::Brake ? 0 : target.continuousSpeed ? target.hornTargetHz
            : target.speed*(target.mode==Mode::Chorale ? tuning::hornSlowHz : tuning::hornFastHz);
        const float dTarget=target.mode==Mode::Brake ? 0 : target.continuousSpeed ? target.drumTargetHz
            : target.speed*(target.mode==Mode::Chorale ? tuning::drumSlowHz : tuning::drumFastHz);
        horn.rotor.Process(hTarget); drum.rotor.Process(dTarget);
        if((tick++ & 15)==0) {
            depth+=paramAlpha*(target.depth-depth); geometry+=paramAlpha*(target.geometry-geometry);
            balance+=paramAlpha*(target.balance-balance); drive+=paramAlpha*(target.drive-drive);
            cabinet+=paramAlpha*(target.cabinet-cabinet);
            horn.UpdatePaths(depth,geometry,cabinet); drum.UpdatePaths(depth,geometry,cabinet);
        }
        float mono=.5f*(left+right);
        const float dc=mono-dcX+dcPole*dcY; dcX=mono; dcY=dc; mono=dc;
        // Mild non-oversampled amplifier character, bypassed at zero drive.
        const float boosted=mono*(1+3*drive);
        mono=(1-drive)*mono+drive*boosted/(1+std::fabs(boosted));
        float lo,hi; crossover.Process(mono,lo,hi);
        const Stereo h=horn.Process(hi), d=drum.Process(lo);
        const float hg=2*balance, dg=2*(1-balance);
        const Stereo colored=voicing.Process({.65f*(hg*h.left+dg*d.left),
                                              .65f*(hg*h.right+dg*d.right)},cabinet);
        const float l=Clamp(colored.left,-.98f,.98f);
        const float r=Clamp(colored.right,-.98f,.98f);
        wet+=bypassAlpha*((target.bypass ? 0.f : 1.f)-wet);
        return {left+wet*(l-left),right+wet*(r-right)};
    }
    float HornHz() const { return horn.rotor.hz; }
    float DrumHz() const { return drum.rotor.hz; }
    float HornAngle() const { return horn.rotor.angle; }
    float DrumAngle() const { return drum.rotor.angle; }
    float MaxDelay() const { return std::max(std::max(horn.DelaySamples(0),horn.DelaySamples(1)),std::max(drum.DelaySamples(0),drum.DelaySamples(1))); }
};
} // namespace rotary
