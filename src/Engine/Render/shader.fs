#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
struct Material {
	sampler2D tex_diffuse;
	sampler2D tex_specular;
	sampler2D tex_normal;
};

uniform Material material;

void main()
{
	FragColor = texture(material.tex_diffuse, TexCoord);
}