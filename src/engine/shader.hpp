#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <fstream>
#include <string>
#include <filesystem>

class LogicalDevice;

class Shader {
public:
    // The shader needs to be precompiled and the `filename` is the SPIR-V file
    void init(LogicalDevice device, VkShaderStageFlagBits stage, std::string filepath);
    void destroy(LogicalDevice device);
    VkShaderModule get() { return shaderModule; };
    VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(std::string entryPoint = "main");

    // If `alwaysCompile` is set to `false` (`true` value by default), the compilation will not be done if the SPIR-V file already exists
    // Returns the name of the compiled file
    static std::string compile(std::string filename, VkShaderStageFlagBits stage, bool alwaysCompile=true);

    
private:
    VkShaderModule shaderModule;
    VkShaderStageFlagBits stage;

    // Read a SPIR-V file into a vector representation for easy access during ShaderModule creation
    static std::vector<char> read(std::string filename);
};
