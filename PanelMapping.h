#pragma once
#include "RotaryDSP.h"
namespace rotary {
// On this module BOTH toggles report physical down=1, center=0, up=2.
// These observations supersede libDaisy's generic UP/DOWN enum labels.
inline Mode LeftMode(int raw) {
    return raw==2 ? Mode::Tremolo : raw==1 ? Mode::Brake : Mode::Chorale;
}
inline int RightCabinet(int raw) { return raw==2 ? 0 : raw==1 ? 2 : 1; }
}
