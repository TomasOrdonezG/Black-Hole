#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "Canvas.h"
#include "EngineContext.h"
#include "GameContext.h"
#include "GameObject.h"
#include "ofAppRunner.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <memory>
#include <vector>

namespace Engine {

class GameEngine : public EngineContext {
public:
    GameEngine() : mContext(*this, mCanvas) {
    }
    GameEngine(const GameEngine &) = delete;
    GameEngine &operator=(const GameEngine &) = delete;
    GameEngine(GameEngine &&) = delete;
    GameEngine &operator=(GameEngine &&) = delete;
    ~GameEngine() = default;

    void addObject(std::shared_ptr<GameObject> obj) override {
        mObjectBuffer.push_back(obj);
    }

    void setup(int canvasScale = 1) {
        mCanvas.setup(canvasScale);
    }

    void update() {
        mContext.dt = std::fmin(ofGetLastFrameTime(), 1.0f / 30.0f);
        mContext.time = ofGetElapsedTimef();

        std::vector<std::shared_ptr<GameObject>> tmpObjects;
        tmpObjects.reserve(mObjects.size() + mObjectBuffer.size());

        // Clean up dead objects
        for (std::shared_ptr<GameObject> &obj : mObjects) {
            if (obj->isAlive()) {
                tmpObjects.push_back(std::move(obj));
            }
        }

        // Add any new objects added since last frame
        std::vector<std::shared_ptr<GameObject>> pending;
        pending.swap(mObjectBuffer);
        for (std::shared_ptr<GameObject> &obj : pending) {
            obj->setup(mContext);
            if (obj->isAlive()) {
                tmpObjects.push_back(std::move(obj));
            }
        }

        tmpObjects.swap(mObjects);

        // Update all objects
        for (const std::shared_ptr<GameObject> &obj : mObjects) {
            obj->update(mContext);
        }
    }

    void draw() {
        ofBackground(0);
        mCanvas.clear();

        // Draw objects
        for (std::shared_ptr<GameObject> &obj : mObjects) {
            if (obj->isAlive()) {
                obj->draw(mContext);
            }
        }

        ofSetColor(255);
        mCanvas.present();
    }

    void keyPressed(int key) {
        if (key == 's' || key == 'S') {
            std::string filename = "../../screenshots/" + ofGetTimestampString() + ".png";
            ofSaveScreen(filename);
        }
    }

    void keyReleased(int key) {
    }

    void mouseMoved(int x, int y) {
    }

    void mouseDragged(int x, int y, int button) {
        for (std::shared_ptr<GameObject> &obj : mObjects) {
            obj->mouseDragged(mContext, x, y, button);
        }
    }

    void mousePressed(int x, int y, int button) {
        for (std::shared_ptr<GameObject> &obj : mObjects) {
            obj->mousePressed(mContext, x, y, button);
        }
    }

    void mouseReleased(int x, int y, int button) {
    }

    void mouseEntered(int x, int y) {
    }

    void mouseExited(int x, int y) {
    }

    void mouseScrolled(int x, int y, float scrollX, float scrollY) {
        for (std::shared_ptr<GameObject> &obj : mObjects) {
            obj->mouseScrolled(mContext, x, y, scrollX, scrollY);
        }
    }

    void windowResized(int w, int h) {
        mCanvas.resize(w, h);
    }

    void gotMessage(ofMessage msg) {
    }

    void dragEvent(ofDragInfo dragInfo) {
    }

private:
    Canvas mCanvas;
    GameContext mContext;
    std::vector<std::shared_ptr<GameObject>> mObjects;
    std::vector<std::shared_ptr<GameObject>> mObjectBuffer;
};

} // namespace Engine

#endif
