#ifndef DEBUG_SETTINGS_H
#define DEBUG_SETTINGS_H

#include "GameContext.h"
#include "ofEvents.h"

namespace Engine {

struct DebugSettings {
    bool paused = false;
    bool stepOnceIfPaused = false;
    int stepsPerFrame = 1;

    bool shouldRunFrame() {
        return !paused || stepOnceIfPaused;
    }

    void update(GameContext &ctx) {
        stepOnceIfPaused = false;

        if (ctx.input.areKeysJustPressed(OF_KEY_CONTROL, ' ')) {
            paused = !paused;
        } else if (ctx.input.areKeysJustPressed(OF_KEY_CONTROL, OF_KEY_RIGHT)) {
            stepOnceIfPaused = true;
        }
    }
};

} // namespace Engine

#endif
