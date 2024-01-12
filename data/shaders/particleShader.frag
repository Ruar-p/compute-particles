#version 430
out vec4 fragColor;

in vec2 texCoords;
in vec3 color;

uniform sampler2D texture1;

void main()
{
	vec4 texColor = texture(texture1, texCoords);

	if(texColor.a < 0.10f)	// Don't render the transparent parts of the texture
		discard;

	//fragColor = texColor;
	fragColor = vec4((color.xxx)+vec3(0.1f, 0.1f, 0.1f), 1.0f);
}