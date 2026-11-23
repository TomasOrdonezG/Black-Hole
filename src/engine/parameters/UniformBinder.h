#ifndef UNIFORM_BINDER_H
#define UNIFORM_BINDER_H

#include "DerivedParameter.h"
#include "ofParameter.h"
#include "ofShader.h"

namespace Engine::Parameters {

class UniformBinder {
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

    static void set(ofShader &s, const std::string &name, bool v) {
        s.setUniform1i(name, v);
    }

    std::vector<std::function<void(ofShader &)>> entries;
};

} // namespace Engine::Parameters

#endif
