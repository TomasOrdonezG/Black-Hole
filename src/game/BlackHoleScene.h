#ifndef BLACK_HOLE_SCENE_H
#define BLACK_HOLE_SCENE_H

#include "../engine/GameObject.h"
#include "../engine/ShaderManager.h"
#include "Settings.h"
#include "common.hpp"
#include "ofAppRunner.h"
#include "ofConstants.h"
#include "ofEvents.h"
#include "ofGLUtils.h"
#include "ofGraphicsConstants.h"
#include "ofMesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

class BlackHoleScene : public GameObject {
public:
    BlackHoleScene() {
        initQuad();
    }

    void setup(GameContext &ctx) override {
        setupNoiseTexture();
        mConfig.setup();
    }

    void draw(GameContext &ctx) override {
        ofShader shader = ShaderManager::getInstance().watchShader("../../src/game/shaders/blackHole");
        shader.begin();
        setUniforms(ctx, shader);
        mQuad.draw();
        shader.end();
        cleanUpDrawBinds();

        std::string fps = "FPS: " + ofToString(ofGetFrameRate(), 1);
        float x = ofGetWidth() - fps.length() * 8 - 10;
        ofDrawBitmapStringHighlight(fps, x, 20);

        mConfig.drawGui();
    }

    void mouseDragged(GameContext &ctx, int x, int y, int button) override {
        if (button == OF_MOUSE_BUTTON_LEFT) {
            float dx = ofGetMouseX() - ofGetPreviousMouseX();
            float dy = ofGetMouseY() - ofGetPreviousMouseY();
            mConfig.camera.incrementAngles(dx, -dy);
        }
    }

    void mouseScrolled(GameContext &ctx, int x, int y, float scrollX, float scrollY) override {
        mConfig.camera.incrementDistance(-scrollY);
    }

private:
    SceneConfiguration mConfig;
    ofMesh mQuad;
    GLuint mNoiseTex = 0;

    void initQuad() {
        mQuad.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        mQuad.addVertex({-1, -1, 0});
        mQuad.addVertex({1, -1, 0});
        mQuad.addVertex({-1, 1, 0});
        mQuad.addVertex({1, 1, 0});
    }

    void setupNoiseTexture(int size = 128, float freq = 4.0f) {
        std::vector<uint8_t> data(size * size * size);

        for (int z = 0; z < size; z++) {
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    glm::vec3 p = glm::vec3(x, y, z) / float(size) * freq;
                    float n = glm::perlin(p, glm::vec3(freq));
                    n = glm::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
                    data[(z * size + y) * size + x] = uint8_t(n * 255.0f + 0.5f);
                }
            }
        }

        glGenTextures(1, &mNoiseTex);
        glBindTexture(GL_TEXTURE_3D, mNoiseTex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, size, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
        glGenerateMipmap(GL_TEXTURE_3D);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    void setUniforms(GameContext &ctx, ofShader &shader) {
        // Game Context
        shader.setUniform2f("u_resolution", glm::vec2(static_cast<float>(ofGetWidth()), static_cast<float>(ofGetHeight())));
        shader.setUniform1f("u_time", ctx.time);

        // Clouds
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, mNoiseTex);
        shader.setUniform1i("u_noise", 0);

        // Scene Settings
        mConfig.applyUniforms(shader);
    }

    void cleanUpDrawBinds() {
        glBindTexture(GL_TEXTURE_3D, 0);
    }
};

#endif
