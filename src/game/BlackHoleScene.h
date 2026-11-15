#ifndef BLACK_HOLE_SCENE_H
#define BLACK_HOLE_SCENE_H

#include "../engine/GameObject.h"
#include "../engine/ShaderManager.h"
#include "common.hpp"
#include "geometric.hpp"
#include "ofAppRunner.h"
#include "ofConstants.h"
#include "ofEvents.h"
#include "ofGLUtils.h"
#include "ofGraphicsConstants.h"
#include "ofMesh.h"
#include "vector_float3.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

constexpr float C = 1.0f;
constexpr float G = 1.0f;
constexpr float _3_SQRT_3 = 5.19615242271f;
constexpr float CB_d_M = _3_SQRT_3 * G / C;
constexpr int MAX_RAY_IT = 5000;
constexpr float R_MAX = 200.0f;
constexpr float DT_EPS = 0.01f;
constexpr float DT_MIN = 0.001f;
constexpr float DT_MAX = 5.0f;

struct BlackHole {
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    float M = 1.0f;
    float r_s = (2.0f * G * M) / (C * C);
    float cb = CB_d_M * M;
};

struct Camera {
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    float distance = 100.0f;
    float theta = PI / 2.0f;
    float phi = 0.0f;
    float viewportDistance = 10.0f;
    float viewportHeight = 2.0f;
    float panSensitivity = 0.005f;
    float zoomSensitivity = 5.0f;

    glm::vec3 position;
    glm::vec3 viewportUnitX;
    glm::vec3 viewportUnitY;
    glm::vec3 viewportTopLeft;

    float minTheta = (PI / 2.0f) - 0.95f * (PI / 4.0f);
    float maxTheta = (PI / 2.0f) + 0.95f * (PI / 4.0f);

    void recompute() {
        glm::vec3 offset = glm::vec3(glm::sin(theta) * glm::cos(phi), glm::cos(theta), glm::sin(theta) * glm::sin(phi));
        position = target + distance * offset;

        glm::vec3 forward = glm::normalize(target - position);
        glm::vec3 down = glm::normalize(glm::vec3(forward.x * forward.y, -forward.x * forward.x - forward.z * forward.z, forward.y * forward.z));
        glm::vec3 right = glm::cross(down, forward);

        float viewportWidth = viewportHeight * ofGetWidth() / ofGetHeight();
        viewportUnitX = right * viewportWidth / ofGetWidth();
        viewportUnitY = down * viewportHeight / ofGetHeight();

        glm::vec3 viewportCenter = position + viewportDistance * forward;
        viewportTopLeft = viewportCenter - viewportUnitX * (ofGetWidth() / 2.0f) - viewportUnitY * (ofGetHeight() / 2.0f);
        viewportTopLeft += (viewportUnitX / 2.0f) + (viewportUnitY / 2.0f);
    }

    void incrementTheta(float dtheta) { theta = glm::clamp(theta + dtheta * panSensitivity, minTheta, maxTheta); }
    void incrementPhi(float dphi) { phi = glm::mod(phi + dphi * panSensitivity, 2 * PI); }
};

class BlackHoleScene : public GameObject {
public:
    void setup(GameContext &ctx) override {
        mCamera.recompute();
        loadNoiseTexture();

        // Full screen quad
        mQuad.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        mQuad.addVertex({-1, -1, 0});
        mQuad.addVertex({1, -1, 0});
        mQuad.addVertex({-1, 1, 0});
        mQuad.addVertex({1, 1, 0});
    }

    void update(GameContext &ctx) override { mCamera.target = mBlackHole.pos; }

    void draw(GameContext &ctx) override {
        ofShader shader = ShaderManager::getInstance().watchShader("../../src/game/shaders/blackHole");
        shader.begin();
        setUniforms(ctx, shader);
        mQuad.draw();
        shader.end();
        cleanUpDrawBinds();
    }

    void mouseDragged(GameContext &ctx, int x, int y, int button) override {
        if (button == OF_MOUSE_BUTTON_LEFT) {
            float dx = ofGetMouseX() - ofGetPreviousMouseX();
            float dy = ofGetMouseY() - ofGetPreviousMouseY();
            mCamera.incrementTheta(-dy);
            mCamera.incrementPhi(dx);
            mCamera.recompute();
        }
    }

    void mouseScrolled(GameContext &ctx, int x, int y, float scrollX, float scrollY) override {
        mCamera.distance += (scrollX - scrollY) * mCamera.zoomSensitivity;
        mCamera.recompute();
    }

private:
    BlackHole mBlackHole;
    Camera mCamera;
    ofMesh mQuad;
    GLuint mNoiseTex = 0;

    void loadNoiseTexture(int size = 64, float freq = 4.0f) {
        std::vector<float> data(size * size * size);

        for (int z = 0; z < size; z++) {
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    glm::vec3 p = glm::vec3(x, y, z) / float(size) * freq;
                    data[(z * size + y) * size + x] = glm::perlin(p, glm::vec3(freq));
                }
            }
        }

        glGenTextures(1, &mNoiseTex);
        glBindTexture(GL_TEXTURE_3D, mNoiseTex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, size, size, size, 0, GL_RED, GL_FLOAT, data.data());
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    void setUniforms(GameContext &ctx, ofShader &shader) {
        shader.setUniform2f("u_resolution", glm::vec2(static_cast<float>(ofGetWidth()), static_cast<float>(ofGetHeight())));
        shader.setUniform1f("u_time", ctx.time);

        // Camera
        shader.setUniform3f("u_camera_pos", mCamera.position);
        shader.setUniform3f("u_viewport_unit_x", mCamera.viewportUnitX);
        shader.setUniform3f("u_viewport_unit_y", mCamera.viewportUnitY);
        shader.setUniform3f("u_viewport_top_left", mCamera.viewportTopLeft);

        // Constants
        shader.setUniform1f("u_light_speed", C);
        shader.setUniform1f("u_grav_const", G);

        // Settings
        shader.setUniform1i("u_max_ray_iters", MAX_RAY_IT);
        shader.setUniform1f("u_max_ray_rad", R_MAX);
        shader.setUniform1f("u_dt_eps", DT_EPS);
        shader.setUniform1f("u_dt_min", DT_MIN);
        shader.setUniform1f("u_dt_max", DT_MAX);

        // Black hole
        shader.setUniform3f("u_black_hole_pos", mBlackHole.pos);
        shader.setUniform1f("u_black_hole_mass", mBlackHole.M);
        shader.setUniform1f("u_black_hole_r_s", mBlackHole.r_s);
        shader.setUniform1f("u_black_hole_cb", mBlackHole.cb);

        // Clouds
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, mNoiseTex);
        shader.setUniform1i("u_noise", 0);
    }

    void cleanUpDrawBinds() { glBindTexture(GL_TEXTURE_3D, 0); }
};

#endif
