#ifndef Renderer_Renderer_hpp
#define Renderer_Renderer_hpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define GL_GLEXT_PROTOTYPES
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"
#include "CL/cl_gl.h"
#include "CL/cl_ext.h"
#endif

#include <vector>
#include <map>
#include <queue>
#include <memory>
#include <functional>

#include "ViewFrustum.hpp"

#undef None

namespace GX
{

struct Renderer
{
	enum class PixelType
	{
		R8G8B8A8,
	};
	
	enum class ImageType
	{
		None,
		Dense,
		Sparse,
	};
	
	using ImageFillCallback = std::function<void(Renderer* renderer, uint16_t imageIndex, size_t x, size_t y, size_t width, size_t height, void* pixels)>;
	
	struct StaticVertex
	{
		float
			position[3],
			unused,
			qtangent[4];
		float
			texcoord[2];
	};
	
	struct SkinnedVertex : StaticVertex
	{
		uint8_t
			jointIndices[4],
			jointWeights[4];
	};
	
	using Index = uint16_t;
	
	struct Joint
	{
		float
			position[3],
			unused,
			qtangent[4];
	};
	
	struct StaticMeshRendererInstance
	{
		float
			position[3],
			rotation[4];
	};
	
	struct SkinnedMeshRendererInstance
	{
		float
			position[3],
			rotation[4];
	};
	
	enum class MaterialNodeType : uint8_t
	{
		OutputBaseColor,
		OutputSmoothness,
		OutputMetalicity,
		OutputTangentSpaceNormal,
		
		InputTexcoord,
		//InputNormal,
		SampleTexture,
		InputWorldPosition,
		InputLocalPosition,
		
		/*Add,
		Subtract,
		Multiply,
		Divide,
		Maximum,
		Minimum,
		
		ConstantFloat32,
		ConstantFloat32x2,
		ConstantFloat32x3,
		ConstantFloat32x4,*/
		
		PaintedTexture
	};
	
	struct MaterialNode
	{
		MaterialNodeType type;
		
		union
		{
			struct
			{
				uint16_t imageIndex;
			}paintedTexture;
			struct
			{
				uint16_t nodeIndex;
			}outputBaseColor;
			struct
			{
				uint16_t nodeIndex;
			}outputSmoothness;
			struct
			{
				uint16_t nodeIndex;
			}outputMetalicity;
			struct
			{
				uint16_t nodeIndex;
			}outputTangentSpaceNormal;
			/*struct
			{
				
			}inputNormal;*/
			struct
			{
				
			}inputTexcoord;
			struct
			{
				uint16_t imageIndex;
				uint16_t texcoordNodeIndex;
			}sampleTexture;
			struct
			{
				
			}inputWorldPosition;
			struct
			{
				
			}inputLocalPosition;
			/*struct
			{
				uint16_t operands[2];
			}add;
			struct
			{
				uint16_t operands[2];
			}subtract;
			struct
			{
				uint16_t operands[2];
			}multiply;
			struct
			{
				uint16_t operands[2];
			}divide;
			struct
			{
				uint16_t operands[2];
			}maximum;
			struct
			{
				uint16_t operands[2];
			}minimum;
			struct
			{
				float values[1];
			}constantFloat32;
			struct
			{
				float values[2];
			}constantFloat32x2;
			struct
			{
				float values[3];
			}constantFloat32x3;
			struct
			{
				float values[4];
			}constantFloat32x4;*/
		};
	};
	
	struct PointTextureEmitter
	{
		float
			position[3],
			radius;
		uint8_t
			type,
			data[3];
	};

	Renderer(void);
	~Renderer(void);

	void setMainViewResolution(GLsizei width, GLsizei height);
	void setMainViewPosition(glm::vec3 to);
	void setMainViewRotation(glm::quat to);
	void setMainViewPerspective(GLfloat near, GLfloat far, GLfloat fov);

	void render(void);
	void update(double seconds);
	
	uint16_t createStaticVertexBuffer(size_t elementCount, const StaticVertex* elements);
	uint16_t createSkinnedVertexBuffer(size_t elementCount, const SkinnedVertex* elements);
	void destroyVertexBuffer(uint16_t bufferIndex);
	
	uint16_t createIndexBuffer(size_t elementCount, const Index* elements);
	void destroyIndexBuffer(uint16_t bufferIndex);
	
	uint16_t createJointBuffer(size_t elementCount, const Joint* elements);
	void destroyJointBuffer(uint16_t bufferIndex);
	void setJointBufferData(uint16_t bufferIndex, size_t elementStart, size_t elementCount, const Joint* elements);
	
	uint16_t createStaticMeshRendererCluster(uint16_t vertexBufferIndex, uint16_t indexBufferIndex, uint16_t materialIndex, size_t instanceCount, const StaticMeshRendererInstance* instances);
	void setStaticMeshRendererClusterInstanceData(uint16_t clusterIndex, size_t elementStart, size_t elementCount, const StaticMeshRendererInstance* elements);
	
	uint16_t createSkinnedMeshRendererCluster(uint16_t vertexBufferIndex, uint16_t indexBufferIndex, uint16_t jointBufferIndex, uint16_t materialIndex, size_t instanceCount, const SkinnedMeshRendererInstance* instances);
	void setSkinnedMeshRendererClusterInstanceData(uint16_t clusterIndex, size_t elementStart, size_t elementCount, const SkinnedMeshRendererInstance* elements);
	
	void destroyCluster(uint16_t clusterIndex);
	
	uint16_t createMaterial(size_t nodeCount, const MaterialNode* nodes);
	void destroyMaterial(uint16_t materialIndex);
	
	uint16_t createImage(PixelType pixelType, size_t width, size_t height, const void* data);
	uint16_t createSparseImage(PixelType pixelType, size_t width, size_t height, const ImageFillCallback& fillSparseImageRegion);
	//void fillSparseImageRegion(uint16_t imageIndex, size_t x, size_t y, size_t width, size_t height, const void* data);
	void destroyImage(uint16_t imageIndex);
	
	//uint16_t createShader(size_t nodeCount, const ShaderNode* nodes);
	//void destroyShader(uint16_t shaderIndex);
	
	glm::vec3 unproject(GLsizei x, GLsizei y);
	void setPointTextureEmitters(size_t start, size_t count, const PointTextureEmitter* emitters);
	void reservePointTextureEmitters(size_t count);
	
	struct Brush
	{
		glm::vec3 at;
		float radius;
		glm::vec3 color;
		/*enum class Type : uint8_t
		{
			None,
		};
		Type type;*/
	};
	
	void paint(const Brush& brush);
	void erase(glm::vec3 at, float radius);
	
	glm::vec3 readColor(GLsizei x, GLsizei y);
	
	//void 
	
	/*enum class ShaderNodeType : uint8_t
	{
		OutputBaseColor,
		OutputSmoothness,
		OutputMetalicity,
		OutputNormal,
				
		InputBasis,
		InputTexture,
		InputWorldPosition,
		InputLocalPosition,
		
		Add,
		Subtract,
		Multiply,
		Divide,
		Maximum,
		Minimum,
		
		ConstantFloat32,
		ConstantFloat32x3,
	};
	
	struct ShaderNode
	{
		ShaderNodeType type;
		
		union
		{
			struct
			{
				uint16_t nodeIndex;
			}outputBaseColor;
			struct
			{
				uint16_t nodeIndex;
			}outputSmoothness;
			struct
			{
				uint16_t nodeIndex;
			}outputSmoothness;
			struct
			{
				uint16_t nodeIndex;
			}outputNormal;
			struct
			{
				
			}inputBasis;
			struct
			{
				
			}inputTexture;
			struct
			{
				
			}inputWorldPosition;
			struct
			{
				
			}inputLocalPosition;
			struct
			{
				
			}add;
			struct
			{
				
			}subtract;
			struct
			{
			
			}multiply;
			struct
			{
			
			}divide;
			struct
			{
			
			}maximum;
			struct
			{
			
			}minimum;
			
			struct
			{
			
			}multiply;
		};
	};
	
	uint16_t createShader(size_t nodeCount, const ShaderNode* nodes);
	void destroyShader(uint16_t shaderIndex);*/
	
	/*struct Shader
	{
		struct Statement
		{
			enum class Type : uint8_t
			{
				OutputBaseColor,
				OutputSmoothness,
				OutputMetalicity,
				OutputNormal,
				
				InputBasis,
				InputTexture,
				InputWorldPosition,
				InputLocalPosition,
				
				Add,
				Subtract,
				Multiply,
				Divide,
				Maximum,
				Minimum,
				Clamp,
				Mix,
			};
			
			Type type;
			union
			{
				struct
				{
					
				}outputBaseColor;
				struct
				{
					
				}outputSmoothness;
				struct
				{
					
				}outputSmoothness;
				struct
				{
					
				}outputNormal;
			} data;
		};
	};*/
	
	/*struct VertexBuffer
	{
		GLuint bufferObject;
	};
	
	struct IndexBuffer
	{
		
	};

	struct Material
	{
		
	};
	
	struct Mesh
	{
		enum class Type
		{
			Static,
			Skinned,
		};
		
		Type type;
		uint16_t
			vertexBufferIndex,
			vertexStart,
			vertexCount,
			
			indexBufferIndex,
			indexStart,
			indexCount;
		GLuint
			indexBufferObject,
			vertexBufferObject;
		
		struct StaticVertex
		{
			glm::half
				position[3],
				unused,
				qtangent[4];
			uint16_t
				texcoord[2];
		};
		
		struct SkinnedVertex : StaticVertex
		{
			uint8_t
				jointIndices[4],
				jointWeights[4];
		};
	};
	
	struct Cluster
	{
		enum class Type
		{
			StaticMeshRenderer
		};
		
		struct StaticMeshRenderer
		{
			struct Instance
			{
				float
					position[3],
					unused,
					rotation[4];
			};
		
			uint16_t
				materialIndex,
				meshIndex,
				instanceCount;
			GLuint instanceBufferObject;
		};
		
		struct SkinnedMeshRenderer
		{
			struct Instance
			{
				float
					position[3],
					jointSetIndex,
					rotation[4];
			};
		
			uint16_t
				materialIndex,
				meshIndex,
				instanceCount;
			GLuint instanceBufferObject;
		};
		
		Type type;
	};*/
	
	struct Camera
	{
		glm::vec3 position;
		GLfloat
			near,
			far,
			fov;
		glm::quat rotation;
		GLsizei
			resolutionWidth,
			resolutionHeight;

		enum class Type
		{
			Main,
		};

		Type type;
		
		Camera(void) :
			position(0, 0, 0), 
			rotation(1, 0, 0, 0), 
			near(1), 
			far(10000), 
			fov(45), 
			resolutionWidth(256),
			resolutionHeight(256),
			type(Type::Main)
		{
			
		}

		glm::mat4 calculateViewMatrix(void) const
		{
			return 
				glm::mat4_cast(glm::inverse(rotation)) * 
				glm::translate(glm::mat4(1.0f), -position);
		}

		glm::mat4 calculateProjectionMatrix(void) const
		{
			float aspect = resolutionWidth / (GLfloat) resolutionHeight;
			return glm::perspective(fov, aspect, near, far);
		}

		ViewFrustum calculateViewFrustum(void) const
		{
			return ViewFrustum(position, rotation, fov, resolutionWidth / float(resolutionHeight), near, far);
		}
	};
	
	struct Image
	{
		ImageType type;
		union
		{
			struct
			{
				uint16_t
					width,
					height;
				GLuint textureObject;
				PixelType pixelType : 4;
			}dense;
			struct
			{
				uint32_t
					width,
					height,
					residentPageCountX,
					residentPageCountY,
					pageCountX,
					pageCountY,
					pageSizeX,
					pageSizeY;
				PixelType pixelType : 4;
				ImageFillCallback fillSparseImageRegion;
				GLuint
					pageTableTextureObject,
					residentTextureObject;
			}sparse;
		};
		
		//Image(void) = delete;
		
		Image(void) : Image(ImageType::Dense)
		{
		
		}
		
		Image(ImageType type) : type(type)
		{
			switch(type)
			{
				case ImageType::Dense:
					dense.pixelType = PixelType::R8G8B8A8;
					dense.textureObject = 0;
					dense.width = 0;
					dense.height = 0;
					break;
				case ImageType::Sparse:
					new (& sparse.fillSparseImageRegion) ImageFillCallback();
					sparse.pixelType = PixelType::R8G8B8A8;
					sparse.pageTableTextureObject = 0;
					sparse.residentTextureObject = 0;
					sparse.width = 0;
					sparse.height = 0;
					sparse.residentPageCountX = 0;
					sparse.residentPageCountY = 0;
					sparse.pageCountX = 0;
					sparse.pageCountY = 0;
					sparse.pageSizeX = 0;
					sparse.pageSizeY = 0;
					break;
			}
		}
		
		Image(const Image&) = delete;
		//Image(Image&&) = default;
		Image(Image&& other) : type(other.type)
		{
			switch(type)
			{
				case ImageType::Dense:
					dense = std::move(other.dense);
					break;
				case ImageType::Sparse:
					sparse = std::move(other.sparse);
					break;
			}
			other.type = ImageType::None;
		}
		
		~Image(void)
		{
			switch(type)
			{
				case ImageType::None:
					break;
				case ImageType::Dense:
					glDeleteTextures(1, & dense.textureObject);
					break;
				case ImageType::Sparse:
					sparse.fillSparseImageRegion.~ImageFillCallback();
					glDeleteTextures(1, & sparse.pageTableTextureObject);
					glDeleteTextures(1, & sparse.residentTextureObject);
					break;
			}
		}
	};
	
	struct VertexBuffer
	{
		enum class Type
		{
			None,
			Static,
			Skinned,
		};
		
		GLuint
			bufferObject,
			vertexCount;
		Type type;
		
		VertexBuffer(void)
		{
			type = Type::None;
			bufferObject = 0;
			vertexCount = 0;
		}
		
		VertexBuffer(const VertexBuffer&) = delete;
		//VertexBuffer(VertexBuffer&&) = default;
		
		~VertexBuffer(void)
		{
			glDeleteBuffers(1, & bufferObject);
		}
	};
	
	struct IndexBuffer
	{
		GLuint
			bufferObject,
			indexCount;
			
		IndexBuffer(void)
		{
			bufferObject = 0;
			indexCount = 0;
		}
		
		IndexBuffer(const IndexBuffer&) = delete;
		//IndexBuffer(IndexBuffer&&) = default;
		
		~IndexBuffer(void)
		{
			glDeleteBuffers(1, & bufferObject);
		}
	};
	
	struct JointBuffer
	{
		GLuint
			bufferObject,
			textureObject;
		
		JointBuffer(void)
		{
			bufferObject = 0;
			textureObject = 0;
		}
		
		JointBuffer(const JointBuffer&) = delete;
		//JointBuffer(JointBuffer&&) = default;
		
		~JointBuffer(void)
		{
			glDeleteTextures(1, & textureObject);
			glDeleteBuffers(1, & bufferObject);
		}
	};
	
	struct InternalClusterReference
	{
		enum class Type
		{
			Static,
			Skinned
		};
		
		Type type;
		uint16_t index;
	};
	
	struct InternalStaticMeshRendererCluster
	{
		uint16_t
			vertexBufferIndex,
			indexBufferIndex,
			materialIndex;
		std::vector<StaticMeshRendererInstance> instances;
	};
	
	struct InternalMaterial
	{
		enum PresentTextureFlags : uint32_t
		{
			None = 0,
			Diffuse = 1 << 0,
			Normal = 1 << 1,
		};
		
		uint16_t
			diffuseTextureIndex,
			normalTextureIndex,
			paintedTextureIndex;
		
		PresentTextureFlags presentTextures;
		
		InternalMaterial(void)
		{
			presentTextures = PresentTextureFlags::None;
		}
	};
	
	std::map<uint16_t, VertexBuffer> vertexBuffers;
	std::map<uint16_t, IndexBuffer> indexBuffers;
	std::map<uint16_t, JointBuffer> jointBuffers;
	//std::map<uint16_t, ClusterBuffer> clusterBuffers;
	std::map<uint16_t, InternalClusterReference> clusters;
	std::map<uint16_t, Image> images;
	std::map<uint16_t, InternalMaterial> materials;
	
	std::map<std::pair<VertexBuffer::Type, InternalMaterial::PresentTextureFlags>, GLuint> programs;
	
	std::vector<InternalStaticMeshRendererCluster> staticMeshRendererClusters;
	
	GLuint
		vertexArrayObject,
		cubeVertexBufferObject,
		pointTextureEmitterVertexBufferObject,
		pointTextureEmitterProgram,
		staticPaintingProgram;
		//staticDefaultProgram,
		//skinnedDefaultProgram;
	uint16_t pointTextureEmitterCount;
	
	/*struct
	{
		GLuint
			framebufferObject,
			depthTextureObject,
			normalTextureObject,
			colorTextureObject;
	}gbuffer;*/
	
	cl_platform_id clplatform;
	cl_context clcontext;
	//cl_command_queue clqueue;
	//cl_device_id cldevice;
	cl_command_queue clgpuqueue, clcpuqueue;
	cl_device_id clgpudevice, clcpudevice;
	
	Camera mainCamera;
};

}//namespace GX

#endif
