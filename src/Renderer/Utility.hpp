#ifndef Renderer_Utility_hpp
#define Renderer_Utility_hpp

#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#include <OpenGL/OpenGL.h>
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define GL_GLEXT_PROTOTYPES
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>

static const char* CLErrorMessageToString(cl_int error)
{
	switch (error)
	{
		case CL_SUCCESS: return "Success!";
		case CL_DEVICE_NOT_FOUND: return "Device not found.";
		case CL_DEVICE_NOT_AVAILABLE: return "Device not available";
		case CL_COMPILER_NOT_AVAILABLE: return "Compiler not available";
		case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "Memory object allocation failure";
		case CL_OUT_OF_RESOURCES: return "Out of resources";
		case CL_OUT_OF_HOST_MEMORY: return "Out of host memory";
		case CL_PROFILING_INFO_NOT_AVAILABLE: return "Profiling information not available";
		case CL_MEM_COPY_OVERLAP: return "Memory copy overlap";
		case CL_IMAGE_FORMAT_MISMATCH: return "Image format mismatch";
		case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "Image format not supported";
		case CL_BUILD_PROGRAM_FAILURE: return "Program build failure";
		case CL_MAP_FAILURE: return "Map failure";
		case CL_INVALID_VALUE: return "Invalid value";
		case CL_INVALID_DEVICE_TYPE: return "Invalid device type";
		case CL_INVALID_PLATFORM: return "Invalid platform";
		case CL_INVALID_DEVICE: return "Invalid device";
		case CL_INVALID_CONTEXT: return "Invalid context";
		case CL_INVALID_QUEUE_PROPERTIES: return "Invalid queue properties";
		case CL_INVALID_COMMAND_QUEUE: return "Invalid command queue";
		case CL_INVALID_HOST_PTR: return "Invalid host pointer";
		case CL_INVALID_MEM_OBJECT: return "Invalid memory object";
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "Invalid image format descriptor";
		case CL_INVALID_IMAGE_SIZE: return "Invalid image size";
		case CL_INVALID_SAMPLER: return "Invalid sampler";
		case CL_INVALID_BINARY: return "Invalid binary";
		case CL_INVALID_BUILD_OPTIONS: return "Invalid build options";
		case CL_INVALID_PROGRAM: return "Invalid program";
		case CL_INVALID_PROGRAM_EXECUTABLE: return "Invalid program executable";
		case CL_INVALID_KERNEL_NAME: return "Invalid kernel name";
		case CL_INVALID_KERNEL_DEFINITION: return "Invalid kernel definition";
		case CL_INVALID_KERNEL: return "Invalid kernel";
		case CL_INVALID_ARG_INDEX: return "Invalid argument index";
		case CL_INVALID_ARG_VALUE: return "Invalid argument value";
		case CL_INVALID_ARG_SIZE: return "Invalid argument size";
		case CL_INVALID_KERNEL_ARGS: return "Invalid kernel arguments";
		case CL_INVALID_WORK_DIMENSION: return "Invalid work dimension";
		case CL_INVALID_WORK_GROUP_SIZE: return "Invalid work group size";
		case CL_INVALID_WORK_ITEM_SIZE: return "Invalid work item size";
		case CL_INVALID_GLOBAL_OFFSET: return "Invalid global offset";
		case CL_INVALID_EVENT_WAIT_LIST: return "Invalid event wait list";
		case CL_INVALID_EVENT: return "Invalid event";
		case CL_INVALID_OPERATION: return "Invalid operation";
		case CL_INVALID_GL_OBJECT: return "Invalid OpenGL object";
		case CL_INVALID_BUFFER_SIZE: return "Invalid buffer size";
		case CL_INVALID_MIP_LEVEL: return "Invalid mip-map level";
		default: return "Unknown";
	}
}

static void CheckCLError_internal(cl_int error, const char* function, int line)
{
	if (error == CL_SUCCESS)
		return;
	fprintf(stderr, "Line#%i, %s(): %s\n", line, function, CLErrorMessageToString(error));
		abort();
}

#define CheckCLError(error) CheckCLError_internal(error, __func__, __LINE__)

static cl_program CompileCLProgram(cl_context context, cl_device_id device, const char* source)
{
	cl_int error = CL_SUCCESS;
	cl_program program = clCreateProgramWithSource(context, 1, & source, NULL, & error);
	CheckCLError(error);
	
	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	size_t logSize = 0;
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, & logSize);
	char* log = new char[logSize + 1];
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
	log[logSize] = 0;
	std::cout << log << std::endl;
	delete[] log;
	CheckCLError(error);
	
	return program;
}

static const char* GLErrorString(GLenum error)
{
	switch (error)
	{
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_NO_ERROR: return "GL_NO_ERROR";
		default: return "Unknown";
	}
	return "Unknown";
}

static void CheckGLErrors_internal(const char* function, int line)
{
	GLenum error;
	bool hadError = false;
	while ((error = glGetError()) not_eq GL_NO_ERROR)
	{
		hadError = true;
		fprintf(stderr, "Line#%i, %s(): %s\n", line, function, GLErrorString(error));
	}
	if (hadError) abort();
}

#define CheckGLErrors() CheckGLErrors_internal(__func__, __LINE__)

static void CheckFramebufferErrors_internal(GLuint framebufferObject, const char* function, int line)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status not_eq GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "Line#%i, %s(): ", line, function);
	switch (status)
	{
		case GL_FRAMEBUFFER_COMPLETE: break;
		case GL_FRAMEBUFFER_UNDEFINED: puts("GL_FRAMEBUFFER_UNDEFINED"); abort();
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: puts("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); abort();
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: puts("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); abort();
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: puts("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); abort();
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: puts("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); abort();
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: puts("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); abort();
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: puts("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); abort();
		default: puts("Unknown Framebuffer Error"); abort();
	}
	
	assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#define CheckFramebufferErrors(fbo) CheckFramebufferErrors_internal(fbo, __func__, __LINE__)

static void AttachShaderToProgram(GLuint program, GLenum type, const GLchar* source)
{
	if (not source) return;
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, & source, NULL);
	glCompileShader(shader);
	
	GLint shaderStatus, estimatedErrorLogLength, errorLogLength;
	glGetShaderiv(shader, GL_COMPILE_STATUS, & shaderStatus);
	
	if (not shaderStatus)
	{
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, & estimatedErrorLogLength);

		char* errorLog = new char[estimatedErrorLogLength];
	
		glGetShaderInfoLog(shader, estimatedErrorLogLength, & errorLogLength, errorLog);
	
		fprintf(stderr, "Shader Error: %s\nShader:\n%s\n", errorLog, source);
	
		delete[] errorLog;
	}
	
	glAttachShader(program, shader);
	glDeleteShader(shader);
}

static GLuint CompileProgram(const GLchar* vertexShaderString, const GLchar* geometryShaderString, const GLchar* fragmentShaderString)
{
	GLuint program = glCreateProgram();
	
	AttachShaderToProgram(program, GL_VERTEX_SHADER, vertexShaderString);
	AttachShaderToProgram(program, GL_GEOMETRY_SHADER, geometryShaderString);
	AttachShaderToProgram(program, GL_FRAGMENT_SHADER, fragmentShaderString);
	
	glLinkProgram(program);
	
	GLint linkStatus, estimatedErrorLogLength, errorLogLength;
	glGetProgramiv(program, GL_LINK_STATUS, & linkStatus);
	
	if (not linkStatus)
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, & estimatedErrorLogLength);

		char* errorLog = new char[estimatedErrorLogLength];
	
		glGetProgramInfoLog(program, estimatedErrorLogLength, & errorLogLength, errorLog);
	
		fprintf(stderr, "Program Error: %s\n", errorLog);
	
		delete[] errorLog;
	}
	
	return program;
}

static void BindUniform(GLuint program, const char* name, glm::mat4 m)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(m));
}

static void BindUniform(GLuint program, const char* name, glm::vec3 v)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniform3fv(location, 1, glm::value_ptr(v));
}

static void BindUniform(GLuint program, const char* name, glm::vec2 v)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniform2fv(location, 1, glm::value_ptr(v));
}

static void BindUniform(GLuint program, const char* name, float v)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniform1f(location, v);
}

static void BindUniform(GLuint program, const char* name, GLuint v)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniform1ui(location, v);
}

static void BindUniform(GLuint program, const char* name, size_t count, const glm::mat4* ms)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniformMatrix4fv(location, count, false, glm::value_ptr(ms[0]));
}

static void BindTextureUnitUniform(GLuint program, const char* name, GLint unit)
{
	GLint location = glGetUniformLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glUniform1i(location, unit);
}

static void BindVertexAttribute(GLuint program, const char* name, GLint size, GLenum type, GLboolean normalized, size_t stride, size_t offset)
{
	GLint location = glGetAttribLocation(program, name);
	CheckGLErrors();
	if (location == -1) return;
	assert(location >= 0);
	glEnableVertexAttribArray(location);
	CheckGLErrors();
	glVertexAttribPointer(location, size, type, normalized, stride, (const void*) offset);
	CheckGLErrors();
}

static void UnbindVertexAttribute(GLuint program, const char* name)
{
	GLint location = glGetAttribLocation(program, name);
	if (location == -1) return;
	assert(location >= 0);
	glDisableVertexAttribArray(location);
}

static bool AABBContains(glm::vec3 center, glm::vec3 extents, glm::vec3 point)
{
	glm::vec3 difference = glm::abs(point - center);
	return glm::all(glm::lessThanEqual(difference, extents));
}

static bool AABBIntersects(glm::vec3 center, glm::vec3 extents, glm::vec3 center2, glm::vec3 extents2)
{
	glm::vec3 difference = glm::abs(center - center2);
	return glm::all(glm::lessThanEqual(difference, extents + extents2));
}

static void GetAABBPoints(glm::vec3 center, glm::vec3 extents, glm::vec3* points)
{
	int i = 0;
	points[i++] = center + extents * glm::vec3(-1, -1, -1);
	points[i++] = center + extents * glm::vec3(-1, -1, +1);
	points[i++] = center + extents * glm::vec3(-1, +1, -1);
	points[i++] = center + extents * glm::vec3(-1, +1, +1);
	points[i++] = center + extents * glm::vec3(+1, -1, -1);
	points[i++] = center + extents * glm::vec3(+1, -1, +1);
	points[i++] = center + extents * glm::vec3(+1, +1, -1);
	points[i++] = center + extents * glm::vec3(+1, +1, +1);
}

static void ProjectAABB(glm::mat4 pvm, glm::vec3 center, glm::vec3 extents, glm::vec3* min, glm::vec3* max)
{
	glm::vec3 points[8];
	GetAABBPoints(center, extents, points);
	glm::vec3 v;
	for (int i = 0; i < 8; i++)
	{
		glm::vec4 p4 = pvm * glm::vec4(points[i], 1);
		glm::vec3 p3 = glm::vec3(p4) / p4.w;
		if (i == 0)
		{
			*min = p3;
			*max = p3;
			v = p3;
		}
		else
		{
			*min = glm::min(*min, p3);
			*max = glm::max(*max, p3);
		}
	}
}

static void TransformAABB(glm::mat4 transform, glm::vec3* center, glm::vec3* extents)
{
	glm::vec3
		min,
		max;
	
	for (int z = 0; z < 2; z++)
	for (int y = 0; y < 2; y++)
	for (int x = 0; x < 2; x++)
	{
		glm::vec3 p = *center + (*extents) * glm::vec3(x * 2 - 1, y * 2 - 1, z * 2 - 1);
		if (z == 0 and y == 0 and x == 0)
		{
			min = max = glm::vec3(transform * glm::vec4(p, 1));
		}
		min = glm::min(min, p);
		max = glm::min(max, p);
	}
	*center = 0.5 * (max - min) + min;
	*extents = 0.5 * (max - min);
}

#endif
