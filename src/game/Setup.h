#ifndef SETUP_H
#define SETUP_H

#include "../engine/GameObject.h"
#include "BlackHoleScene.h"
#include <memory>

class Setup : public GameObject {
    void setup(GameContext &ctx) override {
        ctx.engine.addObject(std::make_shared<BlackHoleScene>());
        kill(ctx);
    }
};

#endif
