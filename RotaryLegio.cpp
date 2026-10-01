#include "daisy_legio.h"
#include "util/CpuLoadMeter.h"
#include "RotaryDSP.h"
#include "EncoderButton.h"
#include "ExternalControls.h"
#include "PanelMapping.h"
#include <atomic>

using namespace daisy;
static DaisyLegio hw;
static rotary::RotarySpeaker effect;
static rotary::Parameters parameters;
static CpuLoadMeter meter;
static rotary::ExternalControls external;




static float baseSpeed=1.f;
// Single atomic packed LED frame avoids concurrent access to LED objects.
static std::atomic<uint32_t> ledFrame{0};
static_assert(ATOMIC_INT_LOCK_FREE == 2, "ISR LED mailbox must be lock-free");
static unsigned page=0;
static uint32_t pageUntil=0;
static rotary::EncoderButton button;
static GPIO buttonPin;
struct StableSwitch {
    int current=0, candidate=0; unsigned count=0;
    void Init(int raw) { current=candidate=raw; count=20; }
    int Process(int raw) {
        if(raw!=candidate) { candidate=raw; count=0; }
        else if(count<20 && ++count==20) current=candidate;
        return current;
    }
};
static StableSwitch modeSwitch, cabinetSwitch;
static uint32_t Pack(float r,float g,float b) {
    return static_cast<uint32_t>(rotary::Clamp(r,0,1)*31)
        | (static_cast<uint32_t>(rotary::Clamp(g,0,1)*31)<<5)
        | (static_cast<uint32_t>(rotary::Clamp(b,0,1)*31)<<10);
}
static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    meter.OnBlockStart();
    hw.ProcessAllControls();
    const uint32_t now=System::GetNow();
    const int mode=modeSwitch.Process(hw.sw[DaisyLegio::SW_LEFT].Read());
    parameters.mode=rotary::LeftMode(mode);
    const int cab=cabinetSwitch.Process(hw.sw[DaisyLegio::SW_RIGHT].Read());
    parameters.cabinet=rotary::RightCabinet(cab);
    parameters.depth=hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_TOP);
    parameters.geometry=hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_BOTTOM);
    const auto gesture=button.Process(!buttonPin.Read(),now);
    if(gesture.hold) parameters.bypass=!parameters.bypass;
    if(gesture.click) { page=(page+1)%3; pageUntil=now+1800; }
    const int inc=hw.encoder.Increment();
    if(inc) {
        if(page==0) baseSpeed=rotary::Clamp(baseSpeed+.02f*inc,.5f,1.5f);
        if(page==1) parameters.balance=rotary::Clamp(parameters.balance+.025f*inc,0,1);
        if(page==2) parameters.drive=rotary::Clamp(parameters.drive+.025f*inc,0,1);
        pageUntil=now+1800;
    }
    external.Process(hw.controls[DaisyLegio::CONTROL_PITCH].Value(), hw.gate.State());
    external.Apply(parameters,baseSpeed,rotary::LeftMode(mode));
    effect.SetParameters(parameters);
    for(size_t i=0;i<size;++i) {
        const auto y=effect.Process(in[0][i],in[1][i]);
        out[0][i]=y.left; out[1][i]=y.right;
    }
    uint32_t l,r;
    if(parameters.bypass) { l=r=Pack(.4f,.4f,.4f); }
    else if(static_cast<int32_t>(pageUntil-now)>0) {
        const float v=page==0 ? baseSpeed- .5f : page==1 ? parameters.balance : parameters.drive;
        const uint32_t elapsed=1800-(pageUntil-now);
        l=elapsed/300<page+1 && elapsed%300<140 ? Pack(1,1,1) : 0;
        r=Pack(.1f+.9f*v,.1f+.9f*v,.1f+.9f*v);
    } else {
        const float h=.2f+.8f*(.5f+.5f*std::cos(effect.HornAngle()));
        const float d=.2f+.8f*(.5f+.5f*std::cos(effect.DrumAngle()));
        l=Pack(h,.35f*h,0); r=Pack(0,.35f*d,d);
    }
    // End meter after DSP/control/UI calculations; atomic publication is tiny.
    meter.OnBlockEnd();
    if(meter.GetMaxCpuLoad()>.8f) { l=r=now%200<100 ? Pack(1,1,1) : 0; }
    ledFrame.store(l|(r<<15),std::memory_order_relaxed);
}
int main(void)
{
    hw.Init(true); // 480 MHz for the first full geometry model.
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);
    buttonPin.Init(seed::D1, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    // Fixed voltage mapping: Pitch and Hit are active on every boot.
    external.Init();
    parameters.bypass=false; // Effect enabled on every boot.
    effect.Init(hw.AudioSampleRate());
    modeSwitch.Init(hw.sw[DaisyLegio::SW_LEFT].Read());
    cabinetSwitch.Init(hw.sw[DaisyLegio::SW_RIGHT].Read());
    meter.Init(hw.AudioSampleRate(),48);
    hw.StartAdc(); hw.StartAudio(AudioCallback);
    while(true) {
        const uint32_t frame=ledFrame.load(std::memory_order_relaxed);
        for(int j=0;j<2;++j) {
            const uint32_t c=frame>>(j*15);
            hw.SetLed(j,(c&31)/31.f,((c>>5)&31)/31.f,((c>>10)&31)/31.f);
        }
        hw.UpdateLeds();
    }
}






