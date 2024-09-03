#pragma once
#include <string>
#include "Quark/Graphic/Device.h"

namespace quark {

struct ShaderTemplateVariant 
{

};

class ShaderTemplate 
{

};

struct ShaderProgramVariant 
{

};

class ShaderProgram 
{

};

class ShaderManager
{
public:
	ShaderManager(graphic::Device* device);

	ShaderProgram* RegisterGraphics(const std::string& vert_path, const std::string& frag_path);
	ShaderProgram* RegisterCompute(const std::string& comp_path);



private:
	graphic::Device* m_Device;

	std::unordered_map<uint64_t, Scope<ShaderTemplate>> m_ShaderTemplates;
	std::unordered_map<uint64_t, Scope<ShaderProgram>> m_ShaderPrograms;
};

}