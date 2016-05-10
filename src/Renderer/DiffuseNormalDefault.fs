#version 330

//layout(location = 0)
out vec4 Albedo_fragment;
//layout(location = 1) out vec4 SpecularSmoothness_fragment;
//layout(location = 2) out vec4 Normal_fragment;

uniform sampler2D BaseColorMap_client;
//uniform sampler2D SpecularSmoothnessMap_client;
uniform sampler2D NormalMap_client;
uniform sampler2D PaintMap_client;

in mat3 TBNMatrix_vertex;
in vec2 Texcoord_vertex;
in vec3 WorldPosition_vertex;

/*uniform float Time_client;


//The psuedo random number generator from http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random1(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co, vec2(a, b));
	float sn = mod(dt, 3.14);
	return fract(sin(sn) * c);
}

vec3 random3(vec2 co)
{
	float x = random1(co);
	float y = random1(co.yx);
	return vec3(x, y, random1(vec2(x, y)));
}

vec3 randomunit(vec2 co)
{
	return normalize(random3(Time_client * co) * 2 - 1);
}*/

void main(void)
{
	mat3 tbnm = mat3(normalize(TBNMatrix_vertex[0]), normalize(TBNMatrix_vertex[1]), normalize(TBNMatrix_vertex[2]));
	vec4 normalMapSample = vec4(0.5, 0.5, 0, 0);//
	//normalMapSample.xyz = texture(NormalMap_client, Texcoord_vertex).xyz;
	
	vec3 normal = tbnm * (normalMapSample.xyz * 2.0 - 1.0);
	vec3 geonormal = normalize(TBNMatrix_vertex[2]);
	
	//Albedo_fragment = vec4(1, 1, 1, 1);
	//Albedo_fragment = vec4(normal * 0.5 + 0.5, 1);

	//Albedo_fragment = texture(BaseColorMap_client, Texcoord_vertex);
	//Albedo_fragment = vec4(0, 0, 0, 1);
	vec3 blendingWeights = geonormal * geonormal;
	
	normal = (tbnm * (texture(NormalMap_client, Texcoord_vertex).rgb * 2.0 - 1.0));
	
	vec3 base = texture(BaseColorMap_client, Texcoord_vertex).rgb;
	vec3 paint = texture(PaintMap_client, Texcoord_vertex).rgb;
	if (paint.x < 1 || paint.y < 1 || paint.z < 1)
		Albedo_fragment.rgb = base * paint;
	else
		Albedo_fragment.rgb = vec3(1, 1, 1);
		//(mix(base, vec3(1), dot(paint, vec3(0.1, 0.6, 0.3))))
		//Albedo_fragment.rgb = mix(base, vec3(1), 0.95);
	
	//Albedo_fragment.rgb *= max(0.25, dot(normal, normalize(vec3(-1, 1, 0))));
	
	//SpecularSmoothness_fragment = texture(SpecularSmoothnessMap_client, Texcoord_vertex);
	//Normal_fragment.xyz = normal * 0.5 + 0.5;
	//Normal_fragment.w = 1;
}

