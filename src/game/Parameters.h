#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "../engine/parameters/DerivedParameter.h"
#include "../engine/parameters/ParameterGroup.h"
#include "ofParameter.h"
#include "ofShader.h"
#include "ofxPanel.h"
#include <glm/glm.hpp>

struct UniversalPhysics : Engine::Parameters::ParameterGroup {
    ofParameter<float> lightSpeed{"Speed of Light", 1.0f, 0.01f, 10.0f};
    ofParameter<float> gravConstant{"Gravitational Constant", 1.0f, 0.01f, 10.0f};

    UniversalPhysics() {
        bindParameters("Universal Physics", lightSpeed, gravConstant);
        uniforms.add("u_light_speed", lightSpeed);
        uniforms.add("u_grav_const", gravConstant);
    }
};

struct BlackHole : Engine::Parameters::ParameterGroup {
    ofParameter<glm::vec3> position{"Position", glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
    ofParameter<float> mass{"Mass", 1.0f, 0.01f, 100.0f};

    Engine::Parameters::DerivedParameter<float> rs;

    explicit BlackHole(UniversalPhysics &physicsSettings) {
        bindParameters("Black Hole", position, mass);
        rs.bind(&computeRs, physicsSettings.gravConstant, physicsSettings.lightSpeed, mass);
        uniforms.add("u_black_hole_pos", position);
        uniforms.add("u_black_hole_mass", mass);
        uniforms.add("u_black_hole_r_s", rs);
    }

private:
    static float computeRs(float gravConstant, float lightSpeed, float bhMass) {
        return 2.0f * gravConstant * bhMass / (lightSpeed * lightSpeed);
    }
};

struct AccretionDisc : Engine::Parameters::ParameterGroup {
    ofParameter<float> thickness{"Thickness", 500.0f, 5.0f, 1000.0f};
    ofParameter<float> edgeFalloffThicknessPercent{"Edge Falloff Thickness Percent", 0.03f, 0.0f, 0.5f};
    ofParameter<float> height{"Height", 0.4f, 0.1f, 5.0f};
    ofParameter<float> verticalThresholdFalloffFactor{"Vertical Falloff", 0.01f, 0.0f, 1.0f};
    ofParameter<float> cloudMinLowerThreshold{"Cloud Min Lower Threshold", 0.3f, 0.01f, 1.0f};
    ofParameter<float> cloudMinUpperThreshold{"Cloud Min Upper Threshold", 0.65f, cloudMinLowerThreshold, 1.0f};
    ofParameter<float> rotationSpeed{"Rotation Speed", 1000.0f, 0.0f, 2000.0f};
    ofParameter<float> rotationPeriod{"Rotation Period", 5000.0f, 1.0f, 10000.0f};
    ofParameter<float> maxTemperature{"Max Temperature", 10000.0f, 1000.0f, 40000.0f};
    ofParameter<float> swirl{"Swirl", 3.0f, 0.0f, 10.0f};
    ofParameter<float> extinctionCoeff{"Extinction Coefficient", 1.0f, 0.0f, 1.0f};
    ofParameter<float> cloudSize{"Cloud Size", 0.15f, 0.01f, 1.0f};

    ofParameter<glm::vec3> normal{"Normal", glm::vec3(0.0f, 1.0f, 0.1f), glm::vec3(-10.0f), glm::vec3(10.0f)};

    Engine::Parameters::DerivedParameter<glm::vec3> unitNormal;

    AccretionDisc() {
        bindParameters("Accretion Disk", thickness, edgeFalloffThicknessPercent, height, verticalThresholdFalloffFactor, cloudMinLowerThreshold,
                       cloudMinUpperThreshold, rotationSpeed, rotationPeriod, maxTemperature, swirl, extinctionCoeff, cloudSize, normal);
        unitNormal.bind([](glm::vec3 normal) { return glm::normalize(normal); }, normal);

        uniforms.add("u_accretion_disc_thickness", thickness);
        uniforms.add("u_accretion_dist_edge_thickness_falloff_percent", edgeFalloffThicknessPercent);
        uniforms.add("u_accretion_disc_height", height);
        uniforms.add("u_accretion_disc_vertical_threshold_falloff_factor", verticalThresholdFalloffFactor);
        uniforms.add("u_accretion_disc_cloud_min_lower_threshold", cloudMinLowerThreshold);
        uniforms.add("u_accretion_disc_cloud_min_upper_threshold", cloudMinUpperThreshold);
        uniforms.add("u_accretion_disc_rotation_speed", rotationSpeed);
        uniforms.add("u_accretion_disc_rotation_period", rotationPeriod);
        uniforms.add("u_accretion_disc_max_temperature", maxTemperature);
        uniforms.add("u_accretion_disc_cloud_size", cloudSize);
        uniforms.add("u_accretion_dist_extinction_coeff", extinctionCoeff);
        uniforms.add("u_accretion_disc_swirl", swirl);
        uniforms.add("u_accretion_disc_normal", unitNormal);
    }
};

struct RayMarching : Engine::Parameters::ParameterGroup {
    ofParameter<bool> rk4{"RK4", false};
    ofParameter<int> maxSteps{"Max Steps", 5000, 10, 10000};
    ofParameter<float> maxDist{"Max Distance", 200.0f, 10.0f, 1000.0f};
    ofParameter<float> stepSizeFactor{"Step Size Factor", 0.005f, 0.0001f, 0.01f};
    ofParameter<float> minStepSize{"Min Step Size", 0.001f, 0.0001f, 0.1f};
    ofParameter<float> maxStepSize{"Max Step Size", 5.0f, 1.0f, 20.0f};

    RayMarching() {
        bindParameters("Ray Marching", rk4, maxSteps, maxDist, stepSizeFactor, minStepSize, maxStepSize);

        uniforms.add("u_rk4", rk4);
        uniforms.add("u_ray_marching_max_steps", maxSteps);
        uniforms.add("u_ray_marching_max_dist", maxDist);
        uniforms.add("u_ray_marching_step_size_factor", stepSizeFactor);
        uniforms.add("u_ray_marching_min_step_size", minStepSize);
        uniforms.add("u_ray_marching_max_step_size", maxStepSize);
    }
};

struct Rendering : Engine::Parameters::ParameterGroup {
    ofParameter<float> exposure{"Exposure", 1.0f, 0.1f, 3.0f};
    ofParameter<float> levelOfDetail{"Level of Detail", 1.0f, 1.0f, 5.0f};
    ofParameter<int> antiAliasSamples{"Anti Alias Samples", 1, 1, 5};

    Rendering() {
        bindParameters("Rendering", exposure, levelOfDetail, antiAliasSamples);

        uniforms.add("u_exposure", exposure);
        uniforms.add("u_level_of_detail", levelOfDetail);
        uniforms.add("u_anti_alias_samples", antiAliasSamples);
    }
};

struct Camera : Engine::Parameters::ParameterGroup {
    ofParameter<float> viewportDist{"Viewport Distance", 1.0f, 0.1f, 10.0f};
    ofParameter<float> viewportHeight{"Viewport Height", 2.0f, 0.5f, 3.0f};
    ofParameter<float> panSensitivity{"Pan Sensitivity", 0.005f, 0.001f, 0.01f};
    ofParameter<float> zoomSensitivity{"Zoom Sensitivity", 5.0f, 1.0f, 10.0f};

    ofParameter<glm::vec3> target{"Camera Target", {0.0f, 0.0f, 0.0f}};
    ofParameter<float> distance{"Distance", 150.0f, 0.0f, 1000.0f};
    ofParameter<float> phi{"Phi", 0.0f};
    ofParameter<float> theta{"Theta", PI / 2.0f - 0.05f};

    Engine::Parameters::DerivedParameter<glm::vec3> position;
    Engine::Parameters::DerivedParameter<glm::vec3> forward;
    Engine::Parameters::DerivedParameter<glm::vec3> down;
    Engine::Parameters::DerivedParameter<glm::vec3> right;
    Engine::Parameters::DerivedParameter<glm::vec3> viewportUnitX;
    Engine::Parameters::DerivedParameter<glm::vec3> viewportUnitY;
    Engine::Parameters::DerivedParameter<glm::vec3> viewportTopLeft;

    explicit Camera() {
        bindParameters("Camera", viewportDist, viewportHeight, panSensitivity, zoomSensitivity);

        position.bind(&computePosition, target, distance, phi, theta);
        forward.bind(&computeForward, position, target);
        down.bind(&computeDown, forward);
        right.bind(&computeRight, down, forward);
        viewportUnitY.bind(&computeViewportUnitY, down, viewportHeight);
        viewportUnitX.bind(&computeViewportUnitX, right, viewportHeight);
        viewportTopLeft.bind(&computeViewportTopLeft, position, viewportDist, forward, viewportUnitX, viewportUnitY);

        uniforms.add("u_camera_pos", position);
        uniforms.add("u_viewport_unit_x", viewportUnitX);
        uniforms.add("u_viewport_unit_y", viewportUnitY);
        uniforms.add("u_viewport_top_left", viewportTopLeft);
    }

    void incrementDistance(float scroll) {
        distance.set(distance.get() + scroll * zoomSensitivity);
    }

    void incrementAngles(float dphi, float dtheta) {
        theta.set(glm::clamp(theta.get() + dtheta * panSensitivity, minTheta, maxTheta));
        phi.set(glm::mod(phi.get() + dphi * panSensitivity, 2 * PI));
    }

private:
    float minTheta = (PI / 2.0f) - 0.99f * (PI / 2.0f);
    float maxTheta = (PI / 2.0f) + 0.99f * (PI / 2.0f);

    static glm::vec3 computePosition(glm::vec3 target, float distance, float phi, float theta) {
        glm::vec3 offset = glm::vec3(glm::sin(theta) * glm::cos(phi), glm::cos(theta), glm::sin(theta) * glm::sin(phi));
        return target + distance * offset;
    }

    static glm::vec3 computeForward(glm::vec3 position, glm::vec3 target) {
        return glm::normalize(target - position);
    }

    static glm::vec3 computeDown(glm::vec3 forward) {
        return glm::normalize(glm::vec3(forward.x * forward.y, -forward.x * forward.x - forward.z * forward.z, forward.y * forward.z));
    }

    static glm::vec3 computeRight(glm::vec3 down, glm::vec3 forward) {
        return glm::cross(down, forward);
    }

    static glm::vec3 computeViewportUnitY(glm::vec3 down, float viewportHeight) {
        return down * viewportHeight / ofGetHeight();
    }

    static glm::vec3 computeViewportUnitX(glm::vec3 right, float viewportHeight) {
        float viewportWidth = viewportHeight * ofGetWidth() / ofGetHeight();
        return right * viewportWidth / ofGetWidth();
    }

    static glm::vec3 computeViewportTopLeft(glm::vec3 position, float viewportDist, glm::vec3 fwd, glm::vec3 viewportUnitX, glm::vec3 viewportUnitY) {
        glm::vec3 viewportCenter = position + viewportDist * fwd;
        glm::vec3 viewportTopLeft = viewportCenter - viewportUnitX * (ofGetWidth() / 2.0f) - viewportUnitY * (ofGetHeight() / 2.0f);
        viewportTopLeft += (viewportUnitX / 2.0f) + (viewportUnitY / 2.0f);
        return viewportTopLeft;
    }
};

struct SceneParameters {
    UniversalPhysics physics;
    BlackHole blackHole{physics};
    AccretionDisc accretionDisc;
    RayMarching rayMarching;
    Rendering rendering;
    Camera camera;

    SceneParameters() = default;
    SceneParameters(const SceneParameters &) = delete;
    SceneParameters &operator=(const SceneParameters &) = delete;

    void setup() {
        gui.setup("Settings");
        gui.add(physics.params);
        gui.add(blackHole.params);
        gui.add(accretionDisc.params);
        gui.add(rayMarching.params);
        gui.add(rendering.params);
        gui.add(camera.params);
    }

    void drawGui() {
        gui.draw();
    }

    void applyUniforms(ofShader &shader) const {
        physics.uniforms.apply(shader);
        blackHole.uniforms.apply(shader);
        accretionDisc.uniforms.apply(shader);
        rayMarching.uniforms.apply(shader);
        rendering.uniforms.apply(shader);
        camera.uniforms.apply(shader);
    }

private:
    ofxPanel gui;
};

#endif
