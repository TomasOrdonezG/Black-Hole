#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "ofFileUtils.h"
#include "ofShader.h"
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Engine {

class ShaderManager {
public:
    static ShaderManager &getInstance() {
        static ShaderManager instance;
        return instance;
    }

    ShaderManager(const ShaderManager &) = delete;
    ShaderManager &operator=(const ShaderManager &) = delete;
    ShaderManager(ShaderManager &&) = delete;
    ShaderManager &operator=(ShaderManager &&) = delete;

    /**
     * @brief	Registers and loads a shader or returns an already registered one
     * @param	name	    Shader name
     * @return	ofShader&	Reference to the shader object
     */
    ofShader &getShader(const std::string &name) {
        auto it = mShaders.find(name);
        if (it == mShaders.end()) {
            it = mShaders.emplace(name, ofShader()).first;
            it->second.load(name);

            auto lastVertTime = std::filesystem::last_write_time(ofToDataPath(name + ".vert"));
            auto lastFragTime = std::filesystem::last_write_time(ofToDataPath(name + ".frag"));
            mLastShaderTime.emplace(name, std::max(lastVertTime, lastFragTime));
        }
        return it->second;
    }

    /**
     * @brief	Same as getShade except reloads the shader if it has been updated
     * @param	name        Shader name
     * @return	ofShader&   Reference to the shader object
     */
    ofShader &watchShader(const std::string &name) {
        ofShader &shader = getShader(name);

        auto currVertTime = std::filesystem::last_write_time(ofToDataPath(name + ".vert"));
        auto currFragTime = std::filesystem::last_write_time(ofToDataPath(name + ".frag"));
        auto currShaderTime = std::max(currVertTime, currFragTime);
        if (mLastShaderTime.find(name)->second < currShaderTime) {
            shader.load(name);
            mLastShaderTime.at(name) = currShaderTime;
        }

        return shader;
    }

private:
    ShaderManager() = default;
    ~ShaderManager() = default;

    std::unordered_map<std::string, ofShader> mShaders;
    std::unordered_map<std::string, std::filesystem::file_time_type> mLastShaderTime;
};

} // namespace Engine

#endif
