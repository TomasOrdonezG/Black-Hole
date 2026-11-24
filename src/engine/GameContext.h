#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "Canvas.h"
#include "EngineContext.h"
#include "InputManager.h"

namespace Engine {

class EngineContext;
class Canvas;

class GameContext {
public:
    GameContext(EngineContext &e, Canvas &c, InputManager &i) : engine(e), canvas(c), input(i) {
    }

    EngineContext &engine;
    Canvas &canvas;
    InputManager &input;

    float dt = 0.0f;
    float time = 0.0f;
};

} // namespace Engine

#endif
