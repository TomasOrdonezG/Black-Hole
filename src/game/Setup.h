#ifndef SETUP_H
#define SETUP_H

#include "../engine/GameObject.h"
#include "BlackHoleScene.h"
#include <memory>

class Setup : public Engine::GameObject {
    void setup(Engine::GameContext &ctx) override {
        ctx.engine.addObject(std::make_shared<BlackHoleScene>());
        kill(ctx);
    }
};

#endif
