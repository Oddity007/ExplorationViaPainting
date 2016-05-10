#version 330

//layout(location = 0)
out vec4 Albedo_fragment;
//layout(location = 1) out vec4 SpecularSmoothness_fragment;
//layout(location = 2) out vec4 Normal_fragment;

in mat3 TBNMatrix_vertex;
in vec2 Texcoord_vertex;
in vec3 WorldPosition_vertex;

void main(void)
{
	mat3 tbnm = mat3(normalize(TBNMatrix_vertex[0]), normalize(TBNMatrix_vertex[1]), normalize(TBNMatrix_vertex[2]));
	vec4 normalMapSample = vec4(0.5, 0.5, 0, 0);//
	//normalMapSample.xyz = texture(NormalMap_client, Texcoord_vertex).xyz;
	
	vec3 normal = tbnm * (normalMapSample.xyz * 2.0 - 1.0);
	vec3 geonormal = normalize(TBNMatrix_vertex[2]);
	
	Albedo_fragment = vec4(1, 1, 1, 1);
}

