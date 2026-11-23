#ifndef PARAMETER_GROUP_H
#define PARAMETER_GROUP_H

#include "UniformBinder.h"
#include "ofParameter.h"

namespace Engine::Parameters {

struct ParameterGroup {
    ParameterGroup() = default;
    ParameterGroup(const ParameterGroup &) = delete;
    ParameterGroup &operator=(const ParameterGroup &) = delete;

    ofParameterGroup params;
    UniformBinder uniforms;

protected:
    template <typename... P>
    void bindParameters(const std::string &name, P &...ps) {
        params.setName(name);
        (params.add(ps), ...);
    }
};

} // namespace Engine::Parameters

#endif
