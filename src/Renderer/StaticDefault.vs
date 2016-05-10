#version 330

uniform mat4 ModelMatrix_client;
uniform mat4 ViewMatrix_client;
uniform mat4 ProjectionMatrix_client;

in vec3 VertexPosition_client;
in vec4 VertexQTangent_client;
in vec2 VertexTexcoord_client;

out mat3 TBNMatrix_vertex;
out vec2 Texcoord_vertex;
out vec3 WorldPosition_vertex;

#define M_PI 3.1415926535897932384626433832795

vec3 qrot(vec4 q, vec3 v)
{
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

vec4 qmul(vec4 a, vec4 b)
{
	return vec4(cross(a.xyz, b.xyz) + a.xyz * b.w + b.xyz * a.w, a.w * b.w - dot(a.xyz, b.xyz));
}

vec4 qconj(vec4 q)
{
	return vec4(-q.xyz, q.w);
}

vec4 qmake(float angle, vec3 axis)
{
	angle = angle * M_PI / 180;
	float halfAngleSine = sin(angle / 2);
	float halfAngleCosine = cos(angle / 2);
	return vec4(axis * halfAngleSine, halfAngleCosine);
}

void main(void)
{
	vec3 vertexPosition = VertexPosition_client;
	vec4 vertexQTangent = VertexQTangent_client;
	float handedness = sign(VertexQTangent_client.w);

	vec4 worldPosition =
		ModelMatrix_client *
		vec4(vertexPosition, 1.0);

	WorldPosition_vertex = worldPosition.xyz;

	gl_Position =
		ProjectionMatrix_client *
		ViewMatrix_client *
		worldPosition;

	TBNMatrix_vertex =
		mat3(ModelMatrix_client) * 
		mat3
		(
			qrot(vertexQTangent, vec3(1, 0, 0)),
			qrot(vertexQTangent, vec3(0, 1, 0)) * handedness,
			qrot(vertexQTangent, vec3(0, 0, 1))
		);
	
	Texcoord_vertex = VertexTexcoord_client;
}
