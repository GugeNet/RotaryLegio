#pragma once
#include "RotaryDSP.h"
#include <cmath>
namespace rotary {
// Provisional board conversion, not measured calibration values.
// libDaisy's FM example uses eight octaves per normalized ADC unit.
// Offset assumes -2 V at normalized zero; hardware verification pending.
constexpr float pitchAdcZero=.25f;
constexpr float pitchAdcPerVolt=.125f;
class ExternalControls {
    float voltage=0, alpha=.019801327f;
    bool gate=false, candidate=false;
    unsigned stable=0;
public:
    void Init(float updateRate=1000) {
        voltage=0; gate=candidate=false; stable=0;
        alpha=-std::expm1(-1/(updateRate*.05f));
    }
    void Process(float normalizedPitch,bool rawGate) {
        float volts=(normalizedPitch-pitchAdcZero)/pitchAdcPerVolt;
        if(!std::isfinite(volts)) volts=0;
        volts=Clamp(volts,0,5);
        if(volts<.08f) volts=0;
        voltage+=alpha*(volts-voltage);
        if(std::fabs(voltage-volts)<.0001f) voltage=volts;
        if(rawGate!=candidate) { candidate=rawGate; stable=1; }
        else if(stable<3) ++stable;
        if(stable>=3) gate=candidate;
    }
    float Speed(float encoderSpeed) const { return Clamp(encoderSpeed,.5f,1.5f); }
    Mode RequestedMode(Mode panel) const {
        return gate && panel==Mode::Chorale ? Mode::Tremolo : panel;
    }
    bool GateActive() const { return gate; }
    float TargetHz(Mode panel,float encoderSpeed,float slowHz,float fastHz) const {
        if(panel==Mode::Brake) return 0;
        const float maximum=fastHz*Speed(encoderSpeed);
        if(RequestedMode(panel)==Mode::Tremolo) return maximum;
        return slowHz+(maximum-slowHz)*voltage*.2f;
    }
    void Apply(Parameters& p,float encoderSpeed,Mode panel) const {
        p.speed=Speed(encoderSpeed);
        p.mode=RequestedMode(panel);
        p.continuousSpeed=true;
        p.hornTargetHz=TargetHz(panel,encoderSpeed,tuning::hornSlowHz,tuning::hornFastHz);
        p.drumTargetHz=TargetHz(panel,encoderSpeed,tuning::drumSlowHz,tuning::drumFastHz);
    }
};
}
