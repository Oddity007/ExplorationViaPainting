#version 330

uniform mat4 ModelMatrix_client;
uniform mat4 ViewMatrix_client;
uniform mat4 ProjectionMatrix_client;

uniform samplerBuffer Joints_client;

in vec3 VertexPosition_client;
in vec4 VertexQTangent_client;
in vec2 VertexTexcoord_client;
in ivec4 VertexJointIndices_client;
in vec4 VertexJointWeights_client;

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

/*void GetTruePositionAndQTangent(out vec3 position, out vec4 qtangent, out float handedness)
{
	handedness = sign(VertexQTangent_client.w);
	position = VertexPosition_client;
	qtangent = VertexQTangent_client;
}*/

void GetSingleJointTransform(int jointID, out vec3 position, out vec4 rotation)
{
	position = texelFetch(Joints_client, jointID * 2 + 0).xyz;
	rotation = texelFetch(Joints_client, jointID * 2 + 1).xyzw;
}

void GetTruePositionAndQTangent(out vec3 position, out vec4 qtangent, out float handedness)
{
	vec3 jp1, jp2, jp3, jp4;
	vec4 jr1, jr2, jr3, jr4;
	GetSingleJointTransform(VertexJointIndices_client.x, jp1, jr1);
	GetSingleJointTransform(VertexJointIndices_client.y, jp2, jr2);
	GetSingleJointTransform(VertexJointIndices_client.z, jp3, jr3);
	GetSingleJointTransform(VertexJointIndices_client.w, jp4, jr4);
	
	vec3 jp = vec3(0, 0, 0)
		+ VertexJointWeights_client.x * jp1
		+ VertexJointWeights_client.y * jp2
		+ VertexJointWeights_client.z * jp3
		+ VertexJointWeights_client.w * jp4;
	
	vec4 jr = vec4(0, 0, 0, 0)
		+ VertexJointWeights_client.x * jr1
		+ VertexJointWeights_client.y * jr2
		+ VertexJointWeights_client.z * jr3
		+ VertexJointWeights_client.w * jr4;
	
	jr = normalize(jr);

	handedness = sign(VertexQTangent_client.w);
	position = qrot(jr, VertexPosition_client) + jp;
	qtangent = qmul(jr, VertexQTangent_client);
}

void main(void)
{
	vec3 vertexPosition;
	vec4 vertexQTangent;
	float handedness = 1.0;
	GetTruePositionAndQTangent(vertexPosition, vertexQTangent, handedness);

	/*gl_Position =
		ProjectionMatrix_client *
		ViewMatrix_client *
		ModelMatrix_client *
		vec4(vertexPosition, 1.0);*/
	
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
