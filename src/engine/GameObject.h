#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "GameContext.h"

class GameObject {
public:
    GameObject() = default;
    GameObject(const GameObject &) = delete;
    GameObject &operator=(const GameObject &) = delete;
    GameObject(GameObject &&) = delete;
    GameObject &operator=(GameObject &&) = delete;

    virtual ~GameObject() = default;

    virtual void setup(GameContext &ctx) {}
    virtual void update(GameContext &ctx) {}
    virtual void draw(GameContext &ctx) {}

    virtual void kill(GameContext &ctx) { mIsAlive = false; }
    virtual bool isAlive() { return mIsAlive; }

    // virtual void gotMessage(GameContext &ctx, ofMessage msg);

    // virtual void keyPressed(GameContext &ctx, int key);
    // virtual void keyReleased(GameContext &ctx, int key);
    // virtual void mouseMoved(GameContext &ctx, int x, int y);
    virtual void mouseDragged(GameContext &ctx, int x, int y, int button) {}
    virtual void mousePressed(GameContext &ctx, int x, int y, int button) {}
    // virtual void mouseReleased(GameContext &ctx, int x, int y, int button);
    // virtual void mouseEntered(GameContext &ctx, int x, int y);
    // virtual void mouseExited(GameContext &ctx, int x, int y);
    virtual void mouseScrolled(GameContext &ctx, int x, int y, float scrollX, float scrollY) {}

private:
    bool mIsAlive = true;
};

#endif
