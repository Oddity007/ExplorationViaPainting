#version 330

//layout(location = 0)
out vec4 Albedo_fragment;
//layout(location = 1) out vec4 SpecularSmoothness_fragment;
//layout(location = 2) out vec4 Normal_fragment;

in mat3 TBNMatrix_vertex;
in vec2 Texcoord_vertex;
in vec3 WorldPosition_vertex;

uniform vec3 BrushCenter_client;
uniform float BrushRadius_client;
uniform vec3 BrushColor_client;

void main(void)
{
	vec3 difference = WorldPosition_vertex - BrushCenter_client;
	float distance = length(difference);
	if (distance > BrushRadius_client)
		discard;

	mat3 tbnm = mat3(normalize(TBNMatrix_vertex[0]), normalize(TBNMatrix_vertex[1]), normalize(TBNMatrix_vertex[2]));
	
	vec3 geonormal = normalize(TBNMatrix_vertex[2]);
	
	vec3 blendingWeights = geonormal * geonormal;
	
	Albedo_fragment.rgb = BrushColor_client;
	Albedo_fragment.a = (1.0 - (distance / BrushRadius_client));
	//Albedo_fragment.a *= Albedo_fragment.a;
	
	//Albedo_fragment.rgb *= max(0.25, dot(normal, normalize(vec3(-1, 1, 0))));
	
	//SpecularSmoothness_fragment = texture(SpecularSmoothnessMap_client, Texcoord_vertex);
	//Normal_fragment.xyz = normal * 0.5 + 0.5;
	//Normal_fragment.w = 1;
}

