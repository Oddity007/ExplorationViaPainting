#version 330

in vec3 Position_client;
in ivec4 Data_client;

out ivec4 Data_vertex;

void main(void)
{
	gl_Position = vec4(Position_client, 1.0);
	Data_vertex = Data_client;
}

