#ifndef SETTINGS_H
#define SETTINGS_H

#include "ofEvent.h"
#include "ofParameter.h"
#include "ofShader.h"
#include "ofxPanel.h"
#include <functional>
#include <glm/glm.hpp>
#include <vector>

namespace Settings {

template <typename T>
class DerivedParameter {
public:
    DerivedParameter() = default;
    DerivedParameter(const DerivedParameter &) = delete;
    DerivedParameter &operator=(const DerivedParameter &) = delete;

    ofEvent<void> invalidated;

    /**
     * @brief	Binds the derived parameter to one or more OpenFrameworks parameters.
     * @param	fn	    Recompute function.
     * @param	deps	All OF parameters to bind to.
     */
    template <typename F, typename... Deps>
    void bind(F fn, Deps &...deps) {
        compute = [fn = std::move(fn), &deps...]() -> T { return fn(valueOf(deps)...); };
        listeners.unsubscribeAll();
        (listen(deps), ...);
        markDirty();
    }

    const T &get() {
        if (dirty) {
            value = compute();
            dirty = false;
        }
        return value;
    }

    void invalidate() {
        markDirty();
    }

private:
    std::function<T()> compute;
    ofEventListeners listeners;
    T value{};
    bool dirty = true;

    template <typename P>
    static const P &valueOf(ofParameter<P> &p) {
        return p.get();
    }

    template <typename U>
    static const U &valueOf(DerivedParameter<U> &d) {
        return d.get();
    }

    template <typename P>
    void listen(ofParameter<P> &p) {
        listeners.push(p.newListener([this](P &) { markDirty(); }));
    }

    template <typename U>
    void listen(DerivedParameter<U> &d) {
        listeners.push(d.invalidated.newListener([this]() { markDirty(); }));
    }

    void markDirty() {
        dirty = true;
        invalidated.notify(this);
    }
};

class Uniforms {
public:
    template <typename P>
    void add(const std::string &name, ofParameter<P> &p) {
        entries.push_back([name, &p](ofShader &s) { set(s, name, p.get()); });
    }

    template <typename U>
    void add(const std::string &name, DerivedParameter<U> &d) {
        entries.push_back([name, &d](ofShader &s) { set(s, name, d.get()); });
    }

    void apply(ofShader &s) const {
        for (const auto &entry : entries) {
            entry(s);
        }
    }

private:
    static void set(ofShader &s, const std::string &name, float v) {
        s.setUniform1f(name, v);
    }

    static void set(ofShader &s, const std::string &name, glm::vec2 v) {
        s.setUniform2f(name, v);
    }

    static void set(ofShader &s, const std::string &name, glm::vec3 v) {
        s.setUniform3f(name, v);
    }

    static void set(ofShader &s, const std::string &name, glm::vec4 v) {
        s.setUniform4f(name, v);
    }

    static void set(ofShader &s, const std::string &name, int v) {
        s.setUniform1i(name, v);
    }

    std::vector<std::function<void(ofShader &)>> entries;
};

struct Group {
    Group() = default;
    Group(const Group &) = delete;
    Group &operator=(const Group &) = delete;

    ofParameterGroup params;
    Uniforms uniforms;

protected:
    template <typename... P>
    void bindParameters(const std::string &name, P &...ps) {
        params.setName(name);
        (params.add(ps), ...);
    }
};

struct UniversalPhysics : Group {
    ofParameter<float> lightSpeed{"Speed of Light", 1.0f, 0.01f, 10.0f};
    ofParameter<float> gravConstant{"Gravitational Constant", 1.0f, 0.01f, 10.0f};

    UniversalPhysics() {
        bindParameters("Universal Physics", lightSpeed, gravConstant);
        uniforms.add("u_light_speed", lightSpeed);
        uniforms.add("u_grav_const", gravConstant);
    }
};

struct BlackHole : Group {
    ofParameter<glm::vec3> position{"Position", glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
    ofParameter<float> mass{"Mass", 1.0f, 0.01f, 100.0f};

    Settings::DerivedParameter<float> rs;

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

struct AccretionDisc : Group {
    ofParameter<float> thickness{"Thickness", 30.0f, 5.0f, 300.0f};
    ofParameter<float> edgeFalloffThickness{"Edge Falloff Thickness", 7.5f, 0.0f, thickness / 2.0f};
    ofParameter<float> height{"Height", 0.6f, 0.1f, 5.0f};
    ofParameter<float> verticalThresholdFalloffFactor{"Vertical Falloff", 0.15f, 0.01f, 1.0f};
    ofParameter<float> cloudMinLowerThreshold{"Cloud Min Lower Threshold", 0.3f, 0.01f, 1.0f};
    ofParameter<float> cloudMinUpperThreshold{"Cloud Min Upper Threshold", 0.75f, cloudMinLowerThreshold, 1.0f};
    ofParameter<float> rotationSpeed{"Rotation Speed", 10.0f, 0.0f, 100.0f};
    ofParameter<float> rotationPeriod{"Rotation Period", 100.0f, 1.0f, 500.0f};
    ofParameter<float> maxTemperature{"Max Temperature", 3500.0f, 1000.0f, 40000.0f};
    ofParameter<float> extinctionCoeff{"Extinction Coefficient", 0.5f, 0.0f, 1.0f};
    ofParameter<float> cloudSize{"Cloud Size", 0.15f, 0.01f, 1.0f};

    ofParameter<glm::vec3> normal{"Normal", glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-10.0f), glm::vec3(10.0f)};

    DerivedParameter<glm::vec3> unitNormal;

    AccretionDisc() {
        bindParameters("Accretion Disk", thickness, edgeFalloffThickness, height, verticalThresholdFalloffFactor, cloudMinLowerThreshold,
                       cloudMinUpperThreshold, rotationSpeed, rotationPeriod, maxTemperature, extinctionCoeff, cloudSize, normal);
        unitNormal.bind([](glm::vec3 normal) { return glm::normalize(normal); }, normal);

        uniforms.add("u_accretion_disc_thickness", thickness);
        uniforms.add("u_accretion_disc_edge_falloff_thickness", edgeFalloffThickness);
        uniforms.add("u_accretion_disc_height", height);
        uniforms.add("u_accretion_disc_vertical_threshold_falloff_factor", verticalThresholdFalloffFactor);
        uniforms.add("u_accretion_disc_cloud_min_lower_threshold", cloudMinLowerThreshold);
        uniforms.add("u_accretion_disc_cloud_min_upper_threshold", cloudMinUpperThreshold);
        uniforms.add("u_accretion_disc_rotation_speed", rotationSpeed);
        uniforms.add("u_accretion_disc_rotation_period", rotationPeriod);
        uniforms.add("u_accretion_disc_max_temperature", maxTemperature);
        uniforms.add("u_accretion_disc_cloud_size", cloudSize);
        uniforms.add("u_accretion_dist_extinction_coeff", extinctionCoeff);
        uniforms.add("u_accretion_disc_normal", unitNormal);
    }
};

struct RayMarching : Group {
    ofParameter<int> maxSteps{"Max Steps", 5000, 10, 50000};
    ofParameter<float> maxDist{"Max Distance", 200.0f, 10.0f, 1000.0f};
    ofParameter<float> stepSizeFactor{"Step Size Factor", 0.01f, 0.0001f, 0.5f};
    ofParameter<float> minStepSize{"Min Step Size", 0.001f, 0.0001f, 0.1f};
    ofParameter<float> maxStepSize{"Max Step Size", 5.0f, 1.0f, 20.0f};

    RayMarching() {
        bindParameters("Ray Marching", maxSteps, maxDist, stepSizeFactor, minStepSize, maxStepSize);

        uniforms.add("u_ray_marching_max_steps", maxSteps);
        uniforms.add("u_ray_marching_max_dist", maxDist);
        uniforms.add("u_ray_marching_step_size_factor", stepSizeFactor);
        uniforms.add("u_ray_marching_min_step_size", minStepSize);
        uniforms.add("u_ray_marching_max_step_size", maxStepSize);
    }
};

struct Rendering : Group {
    ofParameter<float> exposure{"Exposure", 1.0f, 0.1f, 3.0f};
    ofParameter<float> levelOfDetail{"Level of Detail", 1.0f, 1.0f, 5.0f};

    Rendering() {
        bindParameters("Rendering", exposure, levelOfDetail);

        uniforms.add("u_exposure", exposure);
        uniforms.add("u_level_of_detail", levelOfDetail);
    }
};

struct Camera : Group {
    ofParameter<float> viewportDist{"Viewport Distance", 10.0f, 1.0f, 30.0f};
    ofParameter<float> viewportHeight{"Viewport Height", 2.0f, 0.5f, 3.0f};
    ofParameter<float> panSensitivity{"Pan Sensitivity", 0.005f, 0.001f, 0.01f};
    ofParameter<float> zoomSensitivity{"Zoom Sensitivity", 5.0f, 1.0f, 10.0f};

    ofParameter<glm::vec3> target{"Camera Target", {0.0f, 0.0f, 0.0f}};
    ofParameter<float> distance{"Distance", 300.0f, 0.0f, 1000.0f};
    ofParameter<float> phi{"Phi", 0.0f};
    ofParameter<float> theta{"Theta", PI / 2.0f - 0.025f};

    DerivedParameter<glm::vec3> position;
    DerivedParameter<glm::vec3> forward;
    DerivedParameter<glm::vec3> down;
    DerivedParameter<glm::vec3> right;
    DerivedParameter<glm::vec3> viewportUnitX;
    DerivedParameter<glm::vec3> viewportUnitY;
    DerivedParameter<glm::vec3> viewportTopLeft;

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
    float minTheta = (PI / 2.0f) - 0.95f * (PI / 4.0f);
    float maxTheta = (PI / 2.0f) + 0.95f * (PI / 4.0f);

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

} // namespace Settings

struct SceneConfiguration {
    Settings::UniversalPhysics physics;
    Settings::BlackHole blackHole{physics};
    Settings::AccretionDisc accretionDisc;
    Settings::RayMarching rayMarching;
    Settings::Rendering rendering;
    Settings::Camera camera;

    SceneConfiguration() = default;
    SceneConfiguration(const SceneConfiguration &) = delete;
    SceneConfiguration &operator=(const SceneConfiguration &) = delete;

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
