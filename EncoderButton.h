#pragma once
#include <cstdint>
namespace rotary {
// Raw active-low button input -> stable gestures. A hold is consumed once,
// and cannot re-arm until a stable release. Startup requires a release first.
class EncoderButton {
    bool candidate=false, stable=false, armed=false, consumed=false;
    uint32_t changed=0, pressedAt=0;
public:
    struct Event { bool click=false, hold=false; };
    Event Process(bool down, uint32_t now) {
        Event event;
        if(down!=candidate) { candidate=down; changed=now; }
        if(now-changed>=30) {
            if(stable!=candidate) {
                stable=candidate;
                if(stable) { pressedAt=now; consumed=false; }
                else {
                    event.click=armed && !consumed;
                    armed=true;
                }
            } else if(!stable) armed=true;
        }
        if(armed && stable && !consumed && now-pressedAt>=800) {
            consumed=true; event.hold=true;
        }
        return event;
    }
};
}
