#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;
uniform vec3 heightmapColor;

void main()
{
	//FragColor = texture(texture1, TexCoord);
	FragColor = vec4(heightmapColor, 1.0f);
}