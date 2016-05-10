#include "Renderer.hpp"
#include "Utility.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

using namespace GX;

Renderer::Renderer(void)
{
	cl_int error = CL_SUCCESS;
	error = clGetPlatformIDs(1, & clplatform, NULL);
	CheckCLError(error);
	
#ifdef GLFW_EXPOSE_NATIVE_GLX
	std::cout << "GLFW_EXPOSE_NATIVE_GLX is defined." << std::endl;
	cl_context_properties properties[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(glfwGetCurrentContext()),
		CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
		CL_CONTEXT_PLATFORM, (cl_context_properties) clplatform,
		0
	};
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
	CGLContextObj cglcontext = CGLGetCurrentContext();
	CGLShareGroupObj sharegroup = CGLGetShareGroup(cglcontext);
	cl_context_properties properties[] =
	{
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties) sharegroup,
		0
	};
#else
	cl_context_properties* properties = NULL;
#endif

	//This is the correct way of getting a device
	//size_t numberOfSupportingDevices = 0;
	//error = clGetGLContextInfoKHR(properties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), & cldevice, & numberOfSupportingDevices);
	//assert(numberOfSupportingDevices > 0);

	//This is the way to get a device that actually links because I don't want to do it 100% correctly
	/*error = clGetDeviceIDs(clplatform, CL_DEVICE_TYPE_GPU, 1, & cldevice, NULL);

	CheckCLError(error);
	clcontext = clCreateContext(properties, 1, & cldevice, NULL, NULL, & error);
	CheckCLError(error);
	clqueue = clCreateCommandQueue(clcontext, cldevice, 0, &error);
	CheckCLError(error);*/
	
	//This is the way to get a device that actually links because I don't want to do it 100% correctly
	error = clGetDeviceIDs(clplatform, CL_DEVICE_TYPE_GPU, 1, & clgpudevice, NULL);
	CheckCLError(error);
	error = clGetDeviceIDs(clplatform, CL_DEVICE_TYPE_CPU, 1, & clcpudevice, NULL);
	CheckCLError(error);
	{
		cl_device_id deviceIDs[2] = {clcpudevice, clgpudevice};
		clcontext = clCreateContext(properties, 2, deviceIDs, NULL, NULL, & error);
	}
	CheckCLError(error);
	clgpuqueue = clCreateCommandQueue(clcontext, clgpudevice, 0, &error);
	CheckCLError(error);
	clcpuqueue = clCreateCommandQueue(clcontext, clcpudevice, 0, &error);
	CheckCLError(error);
	
	{
		size_t length = 0;
		clGetPlatformInfo(clplatform, CL_PLATFORM_EXTENSIONS, 0, NULL, & length);
		char* string = new char[length + 1];
		clGetPlatformInfo(clplatform, CL_PLATFORM_EXTENSIONS, length, string, NULL);
		printf("Platform: %s\n", string);
	}
	
	{
		size_t length = 0;
		clGetDeviceInfo(clgpudevice, CL_DEVICE_EXTENSIONS, 0, NULL, & length);
		char* string = new char[length + 1];
		clGetDeviceInfo(clgpudevice, CL_DEVICE_EXTENSIONS, length, string, NULL);
		printf("GPU Device: %s\n", string);
	}
	
	{
		size_t length = 0;
		clGetDeviceInfo(clcpudevice, CL_DEVICE_EXTENSIONS, 0, NULL, & length);
		char* string = new char[length + 1];
		clGetDeviceInfo(clcpudevice, CL_DEVICE_EXTENSIONS, length, string, NULL);
		printf("CPU Device: %s\n", string);
	}
	
	glGenVertexArrays(1, & vertexArrayObject);
	
	/*{
		#include "StaticDefault_vs.h"
		#include "Default_fs.h"
		//staticDefaultProgram =
		programs[std::make_pair(vertexBuffer.type, material.presentTextures)] = CompileProgram(StaticDefault_vs, NULL, Default_fs);
	}
	
	{
		#include "SkinnedDefault_vs.h"
		#include "Default_fs.h"
		skinnedDefaultProgram = CompileProgram(SkinnedDefault_vs, NULL, Default_fs);
	}*/
	
	{
		#include "StaticDefault_vs.h"
		#include "SkinnedDefault_vs.h"
		#include "Default_fs.h"
		#include "DiffuseDefault_fs.h"
		#include "NormalDefault_fs.h"
		#include "DiffuseNormalDefault_fs.h"
		
		std::pair<VertexBuffer::Type, InternalMaterial::PresentTextureFlags> key;
		key = std::make_pair(VertexBuffer::Type::Static, InternalMaterial::PresentTextureFlags::None);
		programs[key] = CompileProgram(StaticDefault_vs, NULL, Default_fs);
		//programs.insert(std::make_pair(key, CompileProgram(StaticDefault_vs, NULL, Default_fs)));
		key = std::make_pair(VertexBuffer::Type::Skinned, InternalMaterial::PresentTextureFlags::None);
		programs[key] = CompileProgram(SkinnedDefault_vs, NULL, Default_fs);
		//programs.insert(std::make_pair(key, CompileProgram(SkinnedDefault_vs, NULL, Default_fs)));
		
		key = std::make_pair(VertexBuffer::Type::Static, InternalMaterial::PresentTextureFlags::Diffuse);
		programs[key] = CompileProgram(StaticDefault_vs, NULL, DiffuseDefault_fs);
		key = std::make_pair(VertexBuffer::Type::Skinned, InternalMaterial::PresentTextureFlags::Diffuse);
		programs[key] = CompileProgram(SkinnedDefault_vs, NULL, DiffuseDefault_fs);
		//programs.insert(std::make_pair(std::make_pair(VertexBuffer::Type::Static, InternalMaterial::PresentTextureFlags::Diffuse), CompileProgram(StaticDefault_vs, NULL, DiffuseDefault_fs)));
		//programs.insert(std::make_pair(std::make_pair(VertexBuffer::Type::Skinned, InternalMaterial::PresentTextureFlags::Diffuse), CompileProgram(SkinnedDefault_vs, NULL, DiffuseDefault_fs)));
		
			
		key = std::make_pair(VertexBuffer::Type::Static, InternalMaterial::PresentTextureFlags::Normal);
		programs[key] = CompileProgram(StaticDefault_vs, NULL, NormalDefault_fs);
		key = std::make_pair(VertexBuffer::Type::Skinned, InternalMaterial::PresentTextureFlags::Normal);
		programs[key] = CompileProgram(SkinnedDefault_vs, NULL, NormalDefault_fs);
		//programs.insert(std::make_pair(std::make_pair(VertexBuffer::Type::Static, InternalMaterial::PresentTextureFlags::Normal), CompileProgram(StaticDefault_vs, NULL, NormalDefault_fs)));
		//programs.insert(std::make_pair(std::make_pair(VertexBuffer::Type::Skinned, InternalMaterial::PresentTextureFlags::Normal), CompileProgram(SkinnedDefault_vs, NULL, NormalDefault_fs)));
		
		key = std::make_pair(VertexBuffer::Type::Static, (InternalMaterial::PresentTextureFlags) (InternalMaterial::PresentTextureFlags::Diffuse | InternalMaterial::PresentTextureFlags::Normal));
		programs[key] = CompileProgram(StaticDefault_vs, NULL, DiffuseNormalDefault_fs);
		key = std::make_pair(VertexBuffer::Type::Skinned, (InternalMaterial::PresentTextureFlags) (InternalMaterial::PresentTextureFlags::Diffuse | InternalMaterial::PresentTextureFlags::Normal));
		programs[key] = CompileProgram(SkinnedDefault_vs, NULL, DiffuseNormalDefault_fs);
		//programs.insert(std::make_pair(std::make_pair(VertexBuffer::Type::Static, InternalMaterial::PresentTextureFlags::Diffuse | InternalMaterial::PresentTextureFlags::Normal), CompileProgram(StaticDefault_vs, NULL, DiffuseNormalDefault_fs)));
		//programs.insert(std::make_pair(std::make_pair(VertexBuffer::Type::Skinned, InternalMaterial::PresentTextureFlags::Diffuse | InternalMaterial::PresentTextureFlags::Normal), CompileProgram(SkinnedDefault_vs, NULL, DiffuseNormalDefault_fs)));
	}
	
	jointBuffers[0] = std::move(JointBuffer());
	
	cubeVertexBufferObject = 0;
	
	{
		//Ripped from http://www.opengl-tutorial.org/beginners-tutorials/tutorial-4-a-colored-cube/
		//Because I'm so lazy I can't even describe a cube.
		static const GLfloat g_vertex_buffer_data[] =
		{
			-1.0f,-1.0f,-1.0f, // triangle 1 : begin
			-1.0f,-1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f, // triangle 1 : end
			1.0f, 1.0f,-1.0f, // triangle 2 : begin
			-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f,-1.0f, // triangle 2 : end
			1.0f,-1.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,
			1.0f, 1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,
			-1.0f,-1.0f,-1.0f,
			-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,
			1.0f,-1.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,
			1.0f,-1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f,-1.0f,-1.0f,
			1.0f, 1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f,-1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f,-1.0f,
			-1.0f, 1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f,-1.0f, 1.0f
		};
		
		glGenBuffers(1, & cubeVertexBufferObject);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBufferObject);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	pointTextureEmitterVertexBufferObject = 0;
	pointTextureEmitterCount = 0;
	
	{
		#include "PointTextureEmitter_vs.h"
		#include "PointTextureEmitter_gs.h"
		#include "PointTextureEmitter_fs.h"
		pointTextureEmitterProgram = CompileProgram(PointTextureEmitter_vs, PointTextureEmitter_gs, PointTextureEmitter_fs);
	}
	
	{
		#include "StaticPainting_vs.h"
		#include "DiffuseNormalPainting_fs.h"
		staticPaintingProgram = CompileProgram(StaticPainting_vs, NULL, DiffuseNormalPainting_fs);
	}
	
	/*glGenFramebuffers(1, & gbuffer.framebufferObject);
	glGenTextures(1, & gbuffer.depthTextureObject);
	glGenTextures(1, & gbuffer.normalTextureObject);
	glGenTextures(1, & gbuffer.colorTextureObject);*/
}

Renderer::~Renderer(void)
{
	CheckGLErrors();
	for (auto i = programs.begin(); i not_eq programs.end(); i++)
	{
		glDeleteProgram(i->second);
	}
	//glDeleteProgram(staticDefaultProgram);
	//glDeleteProgram(skinnedDefaultProgram);
	glDeleteVertexArrays(1, & vertexArrayObject);
	CheckGLErrors();
	
	cl_int error = CL_SUCCESS;
	//error = clReleaseCommandQueue(clqueue);
	//CheckCLError(error);
	
	error = clReleaseCommandQueue(clgpuqueue);
	CheckCLError(error);
	error = clReleaseCommandQueue(clcpuqueue);
	CheckCLError(error);
	error = clReleaseContext(clcontext);
	CheckCLError(error);
	CheckGLErrors();
}

void Renderer::setMainViewResolution(GLsizei width, GLsizei height)
{
	mainCamera.resolutionWidth = width;
	mainCamera.resolutionHeight = height;
	
	
	/*glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.framebufferObject);
	
	glBindTexture(GL_TEXTURE_2D, gbuffer.depthTextureObject);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gbuffer.depthTextureObject, 0);

	CheckGLErrors();
	
	glBindTexture(GL_TEXTURE_2D, gbuffer.normalTextureObject);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer.normalTextureObject, 0);

	CheckGLErrors();
	
	glBindTexture(GL_TEXTURE_2D, gbuffer.colorTextureObject);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.colorTextureObject, 0);
	
	CheckFramebufferErrors(gbuffer.framebufferObject);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	CheckGLErrors();*/
}

void Renderer::setMainViewPosition(glm::vec3 to)
{
	mainCamera.position = to;
}

void Renderer::setMainViewRotation(glm::quat to)
{
	mainCamera.rotation = to;
}

void Renderer::setMainViewPerspective(GLfloat near, GLfloat far, GLfloat fov)
{
	mainCamera.near = near;
	mainCamera.far = far;
	mainCamera.fov = fov;
}

void Renderer::render(void)
{
	CheckGLErrors();
	
	assert(vertexArrayObject > 0);
	glBindVertexArray(vertexArrayObject);
	
	//glClearColor(0.75, 0.75, 0.8, 0);
	glClearColor(1, 1, 1, 0);
	glBlendFunc(GL_ONE, GL_ZERO);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, mainCamera.resolutionWidth, mainCamera.resolutionHeight);
	glColorMask(true, true, true, true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/*glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.framebufferObject);
	glViewport(0, 0, mainCamera.resolutionWidth, mainCamera.resolutionHeight);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_ONE, GL_ZERO);
	//glEnable(GL_STENCIL_TEST);
	//glStencilMask(0xFF);
	//glStencilFunc(GL_ALWAYS, 1, 0xFF);
	//glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glColorMask(true, true, true, true);
	glDepthMask(true);
	{
		GLenum buffers[] =
		{
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT2,
		};
		glDrawBuffers(2, buffers);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);*/
	
	//for (auto clusterBufferIterator = clusterBuffers.cbegin(); clusterBufferIterator not_eq clusterBuffers.cend(); clusterBufferIterator++)
	{
		//assert(clusterBufferIterator->second.clusters.size() > 0);
		//for (auto clusterIterator = clusterBufferIterator->second.clusters.cbegin(); clusterIterator not_eq clusterBufferIterator->second.clusters.cend(); clusterIterator++)
		for (auto clusterIterator = staticMeshRendererClusters.cbegin(); clusterIterator not_eq staticMeshRendererClusters.cend(); clusterIterator++)
		{
			//const Cluster& cluster = * clusterIterator;
			const InternalStaticMeshRendererCluster& cluster = * clusterIterator;
			const InternalMaterial& material = materials[cluster.materialIndex];
			//const InstanceBuffer& instanceBuffer = instanceBuffers[cluster.instanceBufferIndex];
			const IndexBuffer& indexBuffer = indexBuffers[cluster.indexBufferIndex];
			const VertexBuffer& vertexBuffer = vertexBuffers[cluster.vertexBufferIndex];
			//const JointBuffer& jointBuffer = jointBuffers[cluster.jointBufferIndex];
			
			const std::vector<StaticMeshRendererInstance>& instances = cluster.instances;
			
			GLuint program = 0;
			size_t vertexSize = 0;
			
			//Program selection
			switch (vertexBuffer.type)
			{
				case VertexBuffer::Type::Static:
					//program = staticDefaultProgram;
					vertexSize = sizeof(StaticVertex);
					break;
				case VertexBuffer::Type::Skinned:
					//program = skinnedDefaultProgram;
					vertexSize = sizeof(SkinnedVertex);
					break;
				default:
					abort();
					break;
			}
			
			assert (programs.end() not_eq programs.find(std::make_pair(vertexBuffer.type, material.presentTextures)));
			program = programs[std::make_pair(vertexBuffer.type, material.presentTextures)];
			
			glUseProgram(program);
			CheckGLErrors();
			
			glm::mat4 projectionMatrix = glm::perspective(mainCamera.fov, mainCamera.resolutionWidth / (GLfloat) mainCamera.resolutionHeight, mainCamera.near, mainCamera.far);
			glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(mainCamera.rotation)) * glm::translate(glm::mat4(1.0f), -mainCamera.position);
			BindUniform(program, "ProjectionMatrix_client", projectionMatrix);
			BindUniform(program, "ViewMatrix_client", viewMatrix);
			
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.bufferObject);
			CheckGLErrors();
			
			BindVertexAttribute(program, "VertexPosition_client", 3, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, position));
			BindVertexAttribute(program, "VertexTexcoord_client", 2, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, texcoord));
			BindVertexAttribute(program, "VertexQTangent_client", 4, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, qtangent));
			CheckGLErrors();
			
			/*if (vertexBuffer.type == VertexBuffer::Type::Skinned)
			{
				BindVertexAttribute(program, "VertexJointIndices_client", 4, GL_UNSIGNED_BYTE, false, vertexSize, offsetof(SkinnedVertex, jointIndices));
				BindVertexAttribute(program, "VertexJointWeights_client", 4, GL_UNSIGNED_BYTE, true, vertexSize, offsetof(SkinnedVertex, jointWeights));
			}*/
			
			/*{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_BUFFER, jointBuffer.textureObject);
				BindTextureUnitUniform(program, "Joints_client", 0);
			}*/
			
			if (material.presentTextures & InternalMaterial::PresentTextureFlags::Diffuse)
			{
				glActiveTexture(GL_TEXTURE0);
				CheckGLErrors();
				assert(images[material.diffuseTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.diffuseTextureIndex].dense.textureObject);
				assert(glIsTexture(images[material.diffuseTextureIndex].dense.textureObject));
				CheckGLErrors();
				BindTextureUnitUniform(program, "BaseColorMap_client", 0);
				CheckGLErrors();
			}
			
			CheckGLErrors();
			
			if (material.presentTextures & InternalMaterial::PresentTextureFlags::Normal)
			{
				glActiveTexture(GL_TEXTURE1);
				assert(images[material.normalTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.normalTextureIndex].dense.textureObject);
				BindTextureUnitUniform(program, "NormalMap_client", 1);
			}
			
			
			{
				glActiveTexture(GL_TEXTURE2);
				assert(images[material.paintedTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.paintedTextureIndex].dense.textureObject);
				BindTextureUnitUniform(program, "PaintMap_client", 2);
			}
			
			CheckGLErrors();
			
			for (size_t i = 0; i < instances.size(); i++)
			{
				const StaticMeshRendererInstance& instance = instances[i];
				
				{
					glm::mat4 rm = glm::mat4_cast(glm::make_quat(instance.rotation));
					glm::mat4 tm = glm::translate(glm::mat4(1.0f), glm::make_vec3(instance.position));
					BindUniform(program, "ModelMatrix_client", tm * rm);
				}
				
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.bufferObject);
				glDrawElements(GL_TRIANGLES, indexBuffer.indexCount, GL_UNSIGNED_SHORT, NULL);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			
			CheckGLErrors();
			
			{
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			if (material.presentTextures & InternalMaterial::PresentTextureFlags::Normal)
			{
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			if (material.presentTextures & InternalMaterial::PresentTextureFlags::Diffuse)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			
			
			/*{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_BUFFER, 0);
			}*/
			
			/*if (vertexBuffer.type == VertexBuffer::Type::Skinned)
			{
				UnbindVertexAttribute(program, "VertexJointIndices_client");
				UnbindVertexAttribute(program, "VertexJointWeights_client");
			}*/
			
			UnbindVertexAttribute(program, "VertexQTangent_client");
			UnbindVertexAttribute(program, "VertexTexcoord_client");
			UnbindVertexAttribute(program, "VertexPosition_client");
			
			CheckGLErrors();
		}
	}
	
	CheckGLErrors();
	
	/*glClearColor(0, 0, 0, 0);
	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, mainCamera.resolutionWidth, mainCamera.resolutionHeight);
	glColorMask(true, true, true, true);
	
	return;
	
	//glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.framebufferObject);
	glBlitFramebuffer(0, 0, mainCamera.resolutionWidth, mainCamera.resolutionHeight, 0, 0, mainCamera.resolutionWidth, mainCamera.resolutionHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	
	return;
	
	glEnable(GL_FRAMEBUFFER_SRGB);
	{
		GLuint program = pointTextureEmitterProgram;
		glUseProgram(program);
		glm::mat4 projectionMatrix = glm::perspective(mainCamera.fov, mainCamera.resolutionWidth / (GLfloat) mainCamera.resolutionHeight, mainCamera.near, mainCamera.far);
			glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(mainCamera.rotation)) * glm::translate(glm::mat4(1.0f), -mainCamera.position);
		BindUniform(program, "ProjectionMatrix_client", projectionMatrix);
		BindUniform(program, "ViewMatrix_client", viewMatrix);
		BindUniform(program, "InverseProjectionViewMatrix_client", glm::inverse(projectionMatrix * viewMatrix));
		
		
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gbuffer.normalTextureObject);
		BindTextureUnitUniform(program, "Normal_client", 2);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gbuffer.depthTextureObject);
		BindTextureUnitUniform(program, "Depth_client", 4);
		
		glBindBuffer(GL_ARRAY_BUFFER, pointTextureEmitterVertexBufferObject);
		BindVertexAttribute(program, "Position_vertex", 3, GL_FLOAT, false, sizeof(PointTextureEmitter), offsetof(PointTextureEmitter, position));
		BindVertexAttribute(program, "Data_vertex", 4, GL_UNSIGNED_BYTE, false, sizeof(PointTextureEmitter), offsetof(PointTextureEmitter, type));
		
		glDrawArrays(GL_POINTS, 0, pointTextureEmitterCount);
		
		UnbindVertexAttribute(program, "Data_vertex");
		UnbindVertexAttribute(program, "Position_vertex");
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glUseProgram(0);
	}
	glDisable(GL_FRAMEBUFFER_SRGB);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	CheckGLErrors();*/
}

void Renderer::update(double seconds)
{
	
}

/*uint16_t Renderer::createVertexBuffer(size_t vertexAttributeCount, const VertexAttribute* vertexAttributes, size_t elementCount, const void* elements)
{
	assert (vertexBuffers.size() < UINT16_MAX);
	
	uint16_t bufferIndex = vertexBuffers.size();
	while (vertexBuffers.cend() not_eq vertexBuffers.find(bufferIndex))
		bufferIndex++;
	vertexBuffers[bufferIndex] = std::move(VertexBuffer());
	
	return bufferIndex;
}*/

void Renderer::destroyVertexBuffer(uint16_t bufferIndex)
{
	vertexBuffers.erase(bufferIndex);
}

uint16_t Renderer::createStaticVertexBuffer(size_t elementCount, const StaticVertex* elements)
{
	assert (vertexBuffers.size() < UINT16_MAX);
	
	uint16_t bufferIndex = vertexBuffers.size();
	while (vertexBuffers.cend() not_eq vertexBuffers.find(bufferIndex))
		bufferIndex++;
	vertexBuffers[bufferIndex] = std::move(VertexBuffer());
	
	vertexBuffers[bufferIndex].type = VertexBuffer::Type::Static;
	
	glGenBuffers(1, & vertexBuffers[bufferIndex].bufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[bufferIndex].bufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(StaticVertex) * elementCount, elements, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	vertexBuffers[bufferIndex].vertexCount = elementCount;
	CheckGLErrors();
	
	return bufferIndex;
}

/*void Renderer::destroyStaticVertexBuffer(uint16_t bufferIndex)
{
	vertexBuffers.erase(bufferIndex);
}*/
	
uint16_t Renderer::createSkinnedVertexBuffer(size_t elementCount, const SkinnedVertex* elements)
{
	assert (vertexBuffers.size() < UINT16_MAX);
	
	uint16_t bufferIndex = vertexBuffers.size();
	while (vertexBuffers.cend() not_eq vertexBuffers.find(bufferIndex))
		bufferIndex++;
	vertexBuffers[bufferIndex] = std::move(VertexBuffer());
	
	vertexBuffers[bufferIndex].type = VertexBuffer::Type::Skinned;
	
	glGenBuffers(1, & vertexBuffers[bufferIndex].bufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[bufferIndex].bufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SkinnedVertex) * elementCount, elements, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	vertexBuffers[bufferIndex].vertexCount = elementCount;
	CheckGLErrors();
	
	return bufferIndex;
}

/*void Renderer::destroySkinnedVertexBuffer(uint16_t bufferIndex)
{
	vertexBuffers.erase(bufferIndex);
}*/
	
uint16_t Renderer::createIndexBuffer(size_t elementCount, const Index* elements)
{
	assert (indexBuffers.size() < UINT16_MAX);
	
	uint16_t bufferIndex = indexBuffers.size();
	while (indexBuffers.cend() not_eq indexBuffers.find(bufferIndex))
		bufferIndex++;
	indexBuffers[bufferIndex] = std::move(IndexBuffer());
	
	glGenBuffers(1, & indexBuffers[bufferIndex].bufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffers[bufferIndex].bufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * elementCount, elements, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	indexBuffers[bufferIndex].indexCount = elementCount;
	CheckGLErrors();
	
	return bufferIndex;
}

void Renderer::destroyIndexBuffer(uint16_t bufferIndex)
{
	indexBuffers.erase(bufferIndex);
}

uint16_t Renderer::createJointBuffer(size_t elementCount, const Joint* elements)
{
	assert (jointBuffers.size() < UINT16_MAX);
	
	if (elementCount == 0)
		return 0;
	
	uint16_t bufferIndex = jointBuffers.size();
	while (jointBuffers.cend() not_eq jointBuffers.find(bufferIndex))
		bufferIndex++;
	jointBuffers[bufferIndex] = std::move(JointBuffer());
	
	glGenBuffers(1, & jointBuffers[bufferIndex].bufferObject);
	glBindBuffer(GL_TEXTURE_BUFFER, jointBuffers[bufferIndex].bufferObject);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(Joint) * elementCount, elements, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	CheckGLErrors();
	glGenTextures(1, & jointBuffers[bufferIndex].textureObject);
	glBindTexture(GL_TEXTURE_BUFFER, jointBuffers[bufferIndex].textureObject);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, jointBuffers[bufferIndex].bufferObject);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	CheckGLErrors();
	
	return bufferIndex;
}

void Renderer::destroyJointBuffer(uint16_t bufferIndex)
{
	jointBuffers.erase(bufferIndex);
}

void Renderer::setJointBufferData(uint16_t bufferIndex, size_t elementStart, size_t elementCount, const Joint* elements)
{
	glBindBuffer(GL_TEXTURE_BUFFER, jointBuffers[bufferIndex].bufferObject);
	glBufferSubData(GL_TEXTURE_BUFFER, elementStart * sizeof(Joint), sizeof(Joint) * elementCount, elements);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	CheckGLErrors();
}

/*uint16_t Renderer::createInstanceBuffer(size_t elementCount, const Instance* elements)
{
	assert (instanceBuffers.size() < UINT16_MAX);
	
	uint16_t bufferIndex = instanceBuffers.size();
	while (instanceBuffers.cend() not_eq instanceBuffers.find(bufferIndex))
		bufferIndex++;
	instanceBuffers[bufferIndex] = std::move(InstanceBuffer());
	
	instanceBuffers[bufferIndex].instances.resize(elementCount);
	for (size_t i = 0; i < elementCount; i++)
		instanceBuffers[bufferIndex].instances[i] = elements[i];
	
	return bufferIndex;
}

void Renderer::destroyInstanceBuffer(uint16_t bufferIndex)
{
	instanceBuffers.erase(bufferIndex);
}

void Renderer::setInstanceBufferData(uint16_t bufferIndex, size_t elementStart, size_t elementCount, const Instance* elements)
{
	for (size_t i = 0; i < elementCount; i++)
		instanceBuffers[bufferIndex].instances[elementStart + i] = elements[i];
}*/

uint16_t Renderer::createStaticMeshRendererCluster(uint16_t vertexBufferIndex, uint16_t indexBufferIndex, uint16_t materialIndex, size_t instanceCount, const StaticMeshRendererInstance* instances)
{
	assert (clusters.size() < UINT16_MAX);
	
	uint16_t clusterIndex = clusters.size();
	while (clusters.cend() not_eq clusters.find(clusterIndex))
		clusterIndex++;
	
	InternalClusterReference reference;
	reference.type = InternalClusterReference::Type::Static;
	reference.index = staticMeshRendererClusters.size();
	clusters[clusterIndex] = reference;
	
	InternalStaticMeshRendererCluster cluster;
	cluster.vertexBufferIndex = vertexBufferIndex;
	cluster.indexBufferIndex = indexBufferIndex;
	cluster.materialIndex = materialIndex;
	cluster.instances.assign(instances, instances + instanceCount);
	
	staticMeshRendererClusters.push_back(std::move(cluster));
	
	std::cout << "Creating cluster " << clusterIndex << " with materialIndex " << materialIndex << std::endl;
	
	return clusterIndex;
}

void Renderer::setStaticMeshRendererClusterInstanceData(uint16_t clusterIndex, size_t elementStart, size_t elementCount, const StaticMeshRendererInstance* elements)
{
	auto reference = clusters[clusterIndex];
	assert(reference.type == InternalClusterReference::Type::Static);
	auto& cluster = staticMeshRendererClusters[reference.index];
	//std::cout << "elementStart " << elementStart << " elementCount " << elementCount << " cluster.instances.size() " << cluster.instances.size() << std::endl;
	for (size_t i = 0; i < elementCount; i++)
	{
		size_t j = elementStart + i;
		assert(j < cluster.instances.size());
		cluster.instances[j] = elements[i];
	}
}

/*uint16_t Renderer::createCluster(const Cluster& cluster)
{
	assert (clusters.size() < UINT16_MAX);
	
	uint16_t clusterIndex = clusters.size();
	while (clusters.cend() not_eq clusters.find(clusterIndex))
		clusterIndex++;
	
	clusters[clusterIndex] = cluster;
	
	return clusterIndex;
}*/

void Renderer::destroyCluster(uint16_t clusterIndex)
{
	const InternalClusterReference reference = clusters[clusterIndex];
	if (staticMeshRendererClusters.size() == 1)
	{
		clusters.erase(clusterIndex);
		return;
	}
	
	std::swap(staticMeshRendererClusters[reference.index], staticMeshRendererClusters.back());
	staticMeshRendererClusters.pop_back();
	
	for (auto i = clusters.begin(); i not_eq clusters.end(); i++)
	{
		if (i->second.type == InternalClusterReference::Type::Static and i->second.index == staticMeshRendererClusters.size())
		{
			i->second.index = reference.index;
			break;
		}
	}
	clusters.erase(clusterIndex);
}

uint16_t Renderer::createMaterial(size_t nodeCount, const MaterialNode* nodes)
{
	assert (materials.size() < UINT16_MAX);
	
	uint16_t materialIndex = materials.size();
	while (materials.cend() not_eq materials.find(materialIndex))
		materialIndex++;
	materials[materialIndex] = std::move(InternalMaterial());
	
	bool hasBaseColor = false;
	bool hasNormal = false;
	bool hasPaintedTexture = false;
	
	std::cout << "Material" << std::endl;
	std::cout << "#nodes: " << nodeCount << std::endl;
	
	for (size_t i = 0; i < nodeCount; i++)
	{
		std::cout << uint16_t(nodes[i].type) << std::endl;
		switch (nodes[i].type)
		{
			case MaterialNodeType::OutputBaseColor:
			{
				MaterialNode outputNode = nodes[i];
				MaterialNode samplerNode = nodes[outputNode.outputBaseColor.nodeIndex];
				assert (samplerNode.type == MaterialNodeType::SampleTexture);
				MaterialNode texcoordNode = nodes[samplerNode.sampleTexture.texcoordNodeIndex];
				assert (texcoordNode.type == MaterialNodeType::InputTexcoord);
				materials[materialIndex].diffuseTextureIndex = samplerNode.sampleTexture.imageIndex;
				hasBaseColor = true;
				break;
			}
			case MaterialNodeType::OutputTangentSpaceNormal:
			{
				MaterialNode outputNode = nodes[i];
				MaterialNode samplerNode = nodes[outputNode.outputTangentSpaceNormal.nodeIndex];
				assert (samplerNode.type == MaterialNodeType::SampleTexture);
				MaterialNode texcoordNode = nodes[samplerNode.sampleTexture.texcoordNodeIndex];
				assert (texcoordNode.type == MaterialNodeType::InputTexcoord);
				materials[materialIndex].normalTextureIndex = samplerNode.sampleTexture.imageIndex;
				hasNormal = true;
				break;
			}
			case MaterialNodeType::PaintedTexture:
			{
				MaterialNode outputNode = nodes[i];
				materials[materialIndex].paintedTextureIndex = outputNode.paintedTexture.imageIndex;
				hasPaintedTexture = true;
				break;
			}
			default:
				break;
		}
	}
	
	assert(hasBaseColor and hasNormal and hasPaintedTexture);
	
	InternalMaterial::PresentTextureFlags presentTextures = (InternalMaterial::PresentTextureFlags) 0;
	if (hasBaseColor)
		presentTextures = InternalMaterial::PresentTextureFlags(presentTextures | InternalMaterial::PresentTextureFlags::Diffuse);
	if (hasNormal)
		presentTextures = InternalMaterial::PresentTextureFlags(presentTextures | InternalMaterial::PresentTextureFlags::Normal);
	materials[materialIndex].presentTextures = presentTextures;
	
	//assert (hasBaseColor and hasNormal);
	
	std::cout << "diffuseTextureIndex: " << materials[materialIndex].diffuseTextureIndex << std::endl;
	std::cout << "normalTextureIndex: " << materials[materialIndex].normalTextureIndex << std::endl;
	
	std::cout << "materialIndex: " << materialIndex << std::endl;
	return materialIndex;
}

void Renderer::destroyMaterial(uint16_t materialIndex)
{
	materials.erase(materialIndex);
}
	
uint16_t Renderer::createImage(PixelType pixelType, size_t width, size_t height, const void* data)
{
	assert (images.size() < UINT16_MAX);
	
	uint16_t imageIndex = images.size();
	//std::cout << "imageIndex: " << imageIndex << std::endl;
	while (images.cend() not_eq images.find(imageIndex))
		imageIndex++;
	//images[imageIndex] = std::move(Image(false));
	assert(images.find(imageIndex) == images.cend());
	
	Image image(ImageType::Dense);
	image.dense.textureObject = 0;
	image.dense.width = width;
	image.dense.height = height;
	glGenTextures(1, & image.dense.textureObject);
	glBindTexture(GL_TEXTURE_2D, image.dense.textureObject);
	CheckGLErrors();
	image.dense.pixelType = pixelType;
	switch (pixelType)
	{
		case PixelType::R8G8B8A8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			break;
	}
	
	//glGenerateMipmap(GL_TEXTURE_2D);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGLErrors();
	
	auto textureObject = image.dense.textureObject;
	
	images.emplace(imageIndex, std::move(image));
	assert(images.find(imageIndex) not_eq images.cend());
	assert(images[imageIndex].dense.textureObject == textureObject);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureObject);
	CheckGLErrors();
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	
	assert(glIsTexture(textureObject));
	
	std::cout << "Generated texture object" << textureObject << std::endl;
	
	return imageIndex;
}

uint16_t Renderer::createSparseImage(PixelType pixelType, size_t width, size_t height, const ImageFillCallback& fillSparseImageRegion)
{
	abort();
	assert (images.size() < UINT16_MAX);
	
	uint16_t imageIndex = images.size();
	while (images.cend() not_eq images.find(imageIndex))
		imageIndex++;
	//images[imageIndex] = std::move(Image(false));
	
	Image image(ImageType::Sparse);
	image.sparse.width = width;
	image.sparse.height = height;
	
	image.sparse.pageSizeX = 128;
	image.sparse.pageSizeY = 128;
	image.sparse.residentPageCountX = 4096 / image.sparse.pageSizeX;
	image.sparse.residentPageCountY = 4096 / image.sparse.pageSizeY;
	image.sparse.pageCountX = width / image.sparse.pageSizeX;
	image.sparse.pageCountY = height / image.sparse.pageSizeY;
	
	assert(image.sparse.pageSizeX * image.sparse.pageCountX == width);
	assert(image.sparse.pageSizeY * image.sparse.pageCountY == height);
	
	image.sparse.pixelType = pixelType;
	
	CheckGLErrors();
	glGenTextures(1, & image.sparse.residentTextureObject);
	glBindTexture(GL_TEXTURE_2D, image.sparse.residentTextureObject);
	
	switch (pixelType)
	{
		case PixelType::R8G8B8A8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.sparse.pageSizeX * image.sparse.residentPageCountX, image.sparse.pageSizeY * image.sparse.residentPageCountY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			break;
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGLErrors();
	
	glGenTextures(1, & image.sparse.pageTableTextureObject);
	glBindTexture(GL_TEXTURE_2D, image.sparse.pageTableTextureObject);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, image.sparse.pageCountX, image.sparse.pageCountY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGLErrors();
	
	images.emplace(imageIndex, std::move(image));
	
	return imageIndex;
}

void Renderer::destroyImage(uint16_t imageIndex)
{
	images.erase(imageIndex);
}

glm::vec3 Renderer::unproject(GLsizei x, GLsizei y)
{
	x = glm::clamp(x, GLsizei(0), mainCamera.resolutionWidth);
	y = glm::clamp(y, GLsizei(0), mainCamera.resolutionHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	float distance = 0;
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, & distance);
	glm::vec3 windowCoord(x, y, distance);
	//printf("raw: %f, %f, %f\n", windowCoord.x, windowCoord.y, windowCoord.z);
	return glm::unProject(windowCoord, mainCamera.calculateViewMatrix(), mainCamera.calculateProjectionMatrix(), glm::vec4(0, 0, mainCamera.resolutionWidth, mainCamera.resolutionHeight));
}

void Renderer::setPointTextureEmitters(size_t start, size_t count, const Renderer::PointTextureEmitter* emitters)
{
	assert(pointTextureEmitterCount >= count);
	assert(pointTextureEmitterCount - start >= count);
	glBindBuffer(GL_ARRAY_BUFFER, pointTextureEmitterVertexBufferObject);
	glBufferSubData(GL_ARRAY_BUFFER, (GLintptr) (start * sizeof(Renderer::PointTextureEmitter)), count * sizeof(Renderer::PointTextureEmitter), emitters);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::reservePointTextureEmitters(size_t count)
{
	pointTextureEmitterCount = count;
	glBindBuffer(GL_ARRAY_BUFFER, pointTextureEmitterVertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, pointTextureEmitterCount * sizeof(Renderer::PointTextureEmitter), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glDeleteBuffers(1, & pointTextureEmitterUniformBufferObject);
	//glGenBuffers()
	/*if (pointTextureEmitterUniformBufferObject == 0)
	{
		glGenBuffers(1, & pointTextureEmitterUniformBufferObject);
	}*/
	//pointTextureEmitterCount = count;
}

void Renderer::paint(const Renderer::Brush& brush)
{
	//return;
	GLuint framebufferObject = 0;
	glGenFramebuffers(1, & framebufferObject);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	glDisable(GL_DEPTH_TEST);
	
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);
	
	//for (auto clusterBufferIterator = clusterBuffers.cbegin(); clusterBufferIterator not_eq clusterBuffers.cend(); clusterBufferIterator++)
	{
		//assert(clusterBufferIterator->second.clusters.size() > 0);
		//for (auto clusterIterator = clusterBufferIterator->second.clusters.cbegin(); clusterIterator not_eq clusterBufferIterator->second.clusters.cend(); clusterIterator++)
		for (auto clusterIterator = staticMeshRendererClusters.cbegin(); clusterIterator not_eq staticMeshRendererClusters.cend(); clusterIterator++)
		{
			//const Cluster& cluster = * clusterIterator;
			const InternalStaticMeshRendererCluster& cluster = * clusterIterator;
			const InternalMaterial& material = materials[cluster.materialIndex];
			//const InstanceBuffer& instanceBuffer = instanceBuffers[cluster.instanceBufferIndex];
			const IndexBuffer& indexBuffer = indexBuffers[cluster.indexBufferIndex];
			const VertexBuffer& vertexBuffer = vertexBuffers[cluster.vertexBufferIndex];
			//const JointBuffer& jointBuffer = jointBuffers[cluster.jointBufferIndex];
			//std::cout << "cluster.materialIndex = " << cluster.materialIndex << std::endl;
			
			const std::vector<StaticMeshRendererInstance>& instances = cluster.instances;
			
			GLuint program = staticPaintingProgram;
			size_t vertexSize = sizeof(StaticVertex);
			
			//Program selection
			if (vertexBuffer.type not_eq VertexBuffer::Type::Static)
				continue;
			
			glUseProgram(program);
			CheckGLErrors();
			
			BindUniform(program, "ViewportSize_client", glm::vec2(images[material.diffuseTextureIndex].dense.width, images[material.diffuseTextureIndex].dense.height));
			BindUniform(program, "BrushCenter_client", brush.at);
			BindUniform(program, "BrushRadius_client", brush.radius);
			BindUniform(program, "BrushColor_client", brush.color);
			
			glm::mat4 projectionMatrix = glm::perspective(mainCamera.fov, mainCamera.resolutionWidth / (GLfloat) mainCamera.resolutionHeight, mainCamera.near, mainCamera.far);
			glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(mainCamera.rotation)) * glm::translate(glm::mat4(1.0f), -mainCamera.position);
			BindUniform(program, "ProjectionMatrix_client", projectionMatrix);
			BindUniform(program, "ViewMatrix_client", viewMatrix);
			
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.bufferObject);
			CheckGLErrors();
			
			BindVertexAttribute(program, "VertexPosition_client", 3, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, position));
			BindVertexAttribute(program, "VertexTexcoord_client", 2, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, texcoord));
			BindVertexAttribute(program, "VertexQTangent_client", 4, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, qtangent));
			CheckGLErrors();
			
			/*if (vertexBuffer.type == VertexBuffer::Type::Skinned)
			{
				BindVertexAttribute(program, "VertexJointIndices_client", 4, GL_UNSIGNED_BYTE, false, vertexSize, offsetof(SkinnedVertex, jointIndices));
				BindVertexAttribute(program, "VertexJointWeights_client", 4, GL_UNSIGNED_BYTE, true, vertexSize, offsetof(SkinnedVertex, jointWeights));
			}*/
			
			/*{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_BUFFER, jointBuffer.textureObject);
				BindTextureUnitUniform(program, "Joints_client", 0);
			}*/
			
			/*if (material.presentTextures & InternalMaterial::PresentTextureFlags::Diffuse)
			{
				glActiveTexture(GL_TEXTURE0);
				CheckGLErrors();
				assert(images[material.diffuseTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.diffuseTextureIndex].dense.textureObject);
				assert(glIsTexture(images[material.diffuseTextureIndex].dense.textureObject));
				CheckGLErrors();
				BindTextureUnitUniform(program, "BaseColorMap_client", 0);
				CheckGLErrors();
			}
			
			CheckGLErrors();
			
			if (material.presentTextures & InternalMaterial::PresentTextureFlags::Normal)
			{
				glActiveTexture(GL_TEXTURE1);
				assert(images[material.normalTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.normalTextureIndex].dense.textureObject);
				BindTextureUnitUniform(program, "NormalMap_client", 1);
			}*/
			
			assert(material.presentTextures & InternalMaterial::PresentTextureFlags::Diffuse);
			assert(material.presentTextures & InternalMaterial::PresentTextureFlags::Normal);
			
			//std::cout << "images[material.normalTextureIndex].dense.textureObject = " << images[material.normalTextureIndex].dense.textureObject << std::endl;
			//std::cout << "images[material.diffuseTextureIndex].dense.textureObject = " << images[material.diffuseTextureIndex].dense.textureObject << std::endl;
			
			CheckGLErrors();
			//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, images[material.normalTextureIndex].dense.textureObject, 0);
			//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, images[material.diffuseTextureIndex].dense.textureObject, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, images[material.paintedTextureIndex].dense.textureObject, 0);
			
			CheckFramebufferErrors(framebufferObject);
			CheckGLErrors();
			
			glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);
			{
				GLenum buffers[] =
				{
					GL_COLOR_ATTACHMENT0,
				//	GL_COLOR_ATTACHMENT2,
				};
				//glDrawBuffers(2, buffers);
				glDrawBuffers(1, buffers);
			}
			
			glViewport(0, 0, images[material.paintedTextureIndex].dense.width, images[material.paintedTextureIndex].dense.height);
			CheckGLErrors();
			
			for (size_t i = 0; i < instances.size(); i++)
			{
				const StaticMeshRendererInstance& instance = instances[i];
				
				{
					glm::mat4 rm = glm::mat4_cast(glm::make_quat(instance.rotation));
					glm::mat4 tm = glm::translate(glm::mat4(1.0f), glm::make_vec3(instance.position));
					BindUniform(program, "ModelMatrix_client", tm * rm);
				}
				
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.bufferObject);
				glDrawElements(GL_TRIANGLES, indexBuffer.indexCount, GL_UNSIGNED_SHORT, NULL);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			
			CheckGLErrors();
			
			UnbindVertexAttribute(program, "VertexQTangent_client");
			UnbindVertexAttribute(program, "VertexTexcoord_client");
			UnbindVertexAttribute(program, "VertexPosition_client");
			
			CheckGLErrors();
			
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glBindTexture(GL_TEXTURE_2D, images[material.diffuseTextureIndex].dense.textureObject);
			//glGenerateMipmap(GL_TEXTURE_2D);
			//glBindTexture(GL_TEXTURE_2D, 0);
			
		}
	}
	
	CheckGLErrors();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, & framebufferObject);
}

glm::vec3 Renderer::readColor(GLsizei x, GLsizei y)
{
	x = glm::clamp(x, GLsizei(0), mainCamera.resolutionWidth);
	y = glm::clamp(y, GLsizei(0), mainCamera.resolutionHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	float color[3];
	glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, & color);
}

void Renderer::erase(glm::vec3 at, float radius)
{
	//return;
	GLuint framebufferObject = 0;
	glGenFramebuffers(1, & framebufferObject);
	
	glBlendFunc(GL_ONE, GL_ZERO);
	glEnable(GL_BLEND);
	
	glDisable(GL_DEPTH_TEST);
	
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);
	
	//for (auto clusterBufferIterator = clusterBuffers.cbegin(); clusterBufferIterator not_eq clusterBuffers.cend(); clusterBufferIterator++)
	{
		//assert(clusterBufferIterator->second.clusters.size() > 0);
		//for (auto clusterIterator = clusterBufferIterator->second.clusters.cbegin(); clusterIterator not_eq clusterBufferIterator->second.clusters.cend(); clusterIterator++)
		for (auto clusterIterator = staticMeshRendererClusters.cbegin(); clusterIterator not_eq staticMeshRendererClusters.cend(); clusterIterator++)
		{
			//const Cluster& cluster = * clusterIterator;
			const InternalStaticMeshRendererCluster& cluster = * clusterIterator;
			const InternalMaterial& material = materials[cluster.materialIndex];
			//const InstanceBuffer& instanceBuffer = instanceBuffers[cluster.instanceBufferIndex];
			const IndexBuffer& indexBuffer = indexBuffers[cluster.indexBufferIndex];
			const VertexBuffer& vertexBuffer = vertexBuffers[cluster.vertexBufferIndex];
			//const JointBuffer& jointBuffer = jointBuffers[cluster.jointBufferIndex];
			//std::cout << "cluster.materialIndex = " << cluster.materialIndex << std::endl;
			
			const std::vector<StaticMeshRendererInstance>& instances = cluster.instances;
			
			GLuint program = staticPaintingProgram;
			size_t vertexSize = sizeof(StaticVertex);
			
			//Program selection
			if (vertexBuffer.type not_eq VertexBuffer::Type::Static)
				continue;
			
			glUseProgram(program);
			CheckGLErrors();
			
			BindUniform(program, "ViewportSize_client", glm::vec2(images[material.diffuseTextureIndex].dense.width, images[material.diffuseTextureIndex].dense.height));
			BindUniform(program, "BrushCenter_client", at);
			BindUniform(program, "BrushRadius_client", radius);
			BindUniform(program, "BrushColor_client", glm::vec3(1, 1, 1));
			
			glm::mat4 projectionMatrix = glm::perspective(mainCamera.fov, mainCamera.resolutionWidth / (GLfloat) mainCamera.resolutionHeight, mainCamera.near, mainCamera.far);
			glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(mainCamera.rotation)) * glm::translate(glm::mat4(1.0f), -mainCamera.position);
			BindUniform(program, "ProjectionMatrix_client", projectionMatrix);
			BindUniform(program, "ViewMatrix_client", viewMatrix);
			
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.bufferObject);
			CheckGLErrors();
			
			BindVertexAttribute(program, "VertexPosition_client", 3, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, position));
			BindVertexAttribute(program, "VertexTexcoord_client", 2, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, texcoord));
			BindVertexAttribute(program, "VertexQTangent_client", 4, GL_FLOAT, false, vertexSize, offsetof(StaticVertex, qtangent));
			CheckGLErrors();
			
			/*if (vertexBuffer.type == VertexBuffer::Type::Skinned)
			{
				BindVertexAttribute(program, "VertexJointIndices_client", 4, GL_UNSIGNED_BYTE, false, vertexSize, offsetof(SkinnedVertex, jointIndices));
				BindVertexAttribute(program, "VertexJointWeights_client", 4, GL_UNSIGNED_BYTE, true, vertexSize, offsetof(SkinnedVertex, jointWeights));
			}*/
			
			/*{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_BUFFER, jointBuffer.textureObject);
				BindTextureUnitUniform(program, "Joints_client", 0);
			}*/
			
			/*if (material.presentTextures & InternalMaterial::PresentTextureFlags::Diffuse)
			{
				glActiveTexture(GL_TEXTURE0);
				CheckGLErrors();
				assert(images[material.diffuseTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.diffuseTextureIndex].dense.textureObject);
				assert(glIsTexture(images[material.diffuseTextureIndex].dense.textureObject));
				CheckGLErrors();
				BindTextureUnitUniform(program, "BaseColorMap_client", 0);
				CheckGLErrors();
			}
			
			CheckGLErrors();
			
			if (material.presentTextures & InternalMaterial::PresentTextureFlags::Normal)
			{
				glActiveTexture(GL_TEXTURE1);
				assert(images[material.normalTextureIndex].type == ImageType::Dense);
				glBindTexture(GL_TEXTURE_2D, images[material.normalTextureIndex].dense.textureObject);
				BindTextureUnitUniform(program, "NormalMap_client", 1);
			}*/
			
			assert(material.presentTextures & InternalMaterial::PresentTextureFlags::Diffuse);
			assert(material.presentTextures & InternalMaterial::PresentTextureFlags::Normal);
			
			//std::cout << "images[material.normalTextureIndex].dense.textureObject = " << images[material.normalTextureIndex].dense.textureObject << std::endl;
			//std::cout << "images[material.diffuseTextureIndex].dense.textureObject = " << images[material.diffuseTextureIndex].dense.textureObject << std::endl;
			
			CheckGLErrors();
			//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, images[material.normalTextureIndex].dense.textureObject, 0);
			//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, images[material.diffuseTextureIndex].dense.textureObject, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, images[material.paintedTextureIndex].dense.textureObject, 0);
			
			CheckFramebufferErrors(framebufferObject);
			CheckGLErrors();
			
			glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);
			{
				GLenum buffers[] =
				{
					GL_COLOR_ATTACHMENT0,
				//	GL_COLOR_ATTACHMENT2,
				};
				//glDrawBuffers(2, buffers);
				glDrawBuffers(1, buffers);
			}
			
			glViewport(0, 0, images[material.paintedTextureIndex].dense.width, images[material.paintedTextureIndex].dense.height);
			CheckGLErrors();
			
			for (size_t i = 0; i < instances.size(); i++)
			{
				const StaticMeshRendererInstance& instance = instances[i];
				
				{
					glm::mat4 rm = glm::mat4_cast(glm::make_quat(instance.rotation));
					glm::mat4 tm = glm::translate(glm::mat4(1.0f), glm::make_vec3(instance.position));
					BindUniform(program, "ModelMatrix_client", tm * rm);
				}
				
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.bufferObject);
				glDrawElements(GL_TRIANGLES, indexBuffer.indexCount, GL_UNSIGNED_SHORT, NULL);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			
			CheckGLErrors();
			
			UnbindVertexAttribute(program, "VertexQTangent_client");
			UnbindVertexAttribute(program, "VertexTexcoord_client");
			UnbindVertexAttribute(program, "VertexPosition_client");
			
			CheckGLErrors();
			
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glBindTexture(GL_TEXTURE_2D, images[material.diffuseTextureIndex].dense.textureObject);
			//glGenerateMipmap(GL_TEXTURE_2D);
			//glBindTexture(GL_TEXTURE_2D, 0);
			
		}
	}
	
	CheckGLErrors();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, & framebufferObject);
}
