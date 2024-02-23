#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
struct Material {
	sampler2D tex_diffuse1;
	sampler2D tex_specular1;
	sampler2D tex_normal1;
};

uniform Material material;

void main()
{
	FragColor = texture(material.tex_diffuse1, TexCoords);
}