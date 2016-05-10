#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 Data_vertex[];

out vec3 Position_geometry;
//out ivec4 Data_geometry;
out float Radius_geometry;

uniform mat4 ProjectionMatrix_client;
uniform mat4 ViewMatrix_client;

void main(void)
{
	Position_geometry = gl_in[0].gl_Position.xyz;
	//Data_geometry = Data_vertex[0];
	
	float radius = 1;

	Radius_geometry = radius;

	vec3 center = (ViewMatrix_client * vec4(Position_geometry, 1)).xyz;
	vec2 mn = (ProjectionMatrix_client * vec4(center.xy - radius, center.z, 1)).xy;
	vec2 mx = (ProjectionMatrix_client * vec4(center.xy + radius, center.z, 1)).xy;

	gl_Position = vec4(mn.x, mn.y, 0, 1);
	EmitVertex();
	gl_Position = vec4(mx.x, mn.y, 0, 1);
	EmitVertex();
	gl_Position = vec4(mn.x, mx.y, 0, 1);
	EmitVertex();
	gl_Position = vec4(mx.x, mx.y, 0, 1);
	EmitVertex();
	EndPrimitive();

	/*gl_Position = vec4(-1, -1, 0, 1);
	EmitVertex();
	gl_Position = vec4(1, -1, 0, 1);
	EmitVertex();
	gl_Position = vec4(-1, 1, 0, 1);
	EmitVertex();
	gl_Position = vec4(1, 1, 0, 1);
	EmitVertex();
	EndPrimitive();*/
}
