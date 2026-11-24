#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <unordered_set>
namespace Engine {

class InputManager {
    friend class GameEngine;

public:
    bool isKeyDown(int key) const {
        return currentKeys.count(key) > 0;
    }

    bool isKeyJustPressed(int key) const {
        return isKeyDown(key) && (previousKeys.count(key) == 0);
    }

    bool areKeysJustPressed(int key1, int key2) const {
        bool bothDownNow = isKeyDown(key1) && isKeyDown(key2);
        bool bothDownBefore = (previousKeys.count(key1) > 0) && (previousKeys.count(key2) > 0);
        return bothDownNow && !bothDownBefore;
    }

private:
    std::unordered_set<int> currentKeys;
    std::unordered_set<int> previousKeys;

    void endFrame() {
        previousKeys = currentKeys;
    }

    void onKeyPressed(int key) {
        currentKeys.insert(key);
    }

    void onKeyReleased(int key) {
        currentKeys.erase(key);
    }
};

} // namespace Engine

#endif
