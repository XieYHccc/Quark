#version 450

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outUVW;

layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 viewproj;
} sceneData;

void main() 
{
	outUVW = inPos;

	// Convert cubemap coordinates into Vulkan coordinate space
	// outUVW.xy *= -1.0;
	
	// Remove translation from view matrix
	mat4 viewMat = mat4(mat3(sceneData.view));
	gl_Position = sceneData.proj * viewMat * vec4(inPos.xyz, 1.0);
}
