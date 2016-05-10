#version 330

//layout(location = 0) out vec4 AlbedoTag_fragment;
//layout(location = 1) out vec4 SpecularSmoothness_fragment;
//layout(location = 2) out vec4 Normal_fragment;
//layout(location = 3) out vec4 LightAccumulation_fragment;

//layout(location = 3) 
out vec4 Color_fragment;

uniform mat4 InverseProjectionViewMatrix_client;

uniform sampler2D Depth_client;
uniform sampler2D Normal_client;

in vec3 Position_geometry;
//in ivec4 Data_geometry;
in float Radius_geometry;

//layout(pixel_center_integer) in vec4 gl_FragCoord;

void main(void)
{
	vec2 texcoord = gl_FragCoord.xy / vec2(textureSize(Normal_client, 0));

	vec3 normal = texture(Normal_client, texcoord).xyz * 2 - 1;
	float depth = texture(Depth_client, texcoord).x;
	vec4 worldCoord = 
		InverseProjectionViewMatrix_client * 
		vec4(texcoord * 2 - 1, depth * 2.0 - 1.0, 1.0);
	worldCoord /= worldCoord.w;

	vec3 direction = Position_geometry - worldCoord.xyz;
	float distance = length(direction);
	direction /= distance;

	//if (Radius_geometry < distance)
	//	discard;

	Color_fragment = vec4(1, 0, 0, 0);
}
