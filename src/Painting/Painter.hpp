#ifndef GX_Painting_Painter_hpp
#define GX_Painting_Painter_hpp

#include "nanoflann/nanoflann.hpp"
#include "Renderer/Renderer.hpp"
#include "glm/gtc/random.hpp"
#include <iostream>
#include "Core/Unwrapping.hpp"
#include "Core/KGPhysicsManager.h"

namespace GX
{
namespace Painting
{

struct Painter
{
	struct Node
	{
		enum class Type : uint8_t
		{
			Dead,
			Blah,
		};
		glm::vec3 position;
		Type type;
		uint8_t data[3];
		float timeLeft;
	};
	
	/*struct NodePointCloud
	{
		std::vector<Node> nodes;
		
		size_t kdtree_get_point_count(void) const
		{
			return nodes.size();
		}
		
		float kdtree_distance(const float* p, size_t index, size_t size) const
		{
			assert(size == 3);
			return glm::dot(glm::vec3(p[0], p[1], p[2]), nodes[index].position);
		}
		
		float kdtree_get_pt(size_t index, int dimension) const
		{
			return nodes[index].position[dimension];
		}
		
		template<class Box>
		bool kdtree_get_bbox(Box& boundingBox) const {return false;}
	};*/
	
	struct NodePointCloud
	{
		std::vector<Node>* nodes;
		
		size_t kdtree_get_point_count(void) const
		{
			return nodes->size();
		}
		
		float kdtree_distance(const float* p, size_t index, size_t size) const
		{
			assert(size == 3);
			return glm::dot(glm::vec3(p[0], p[1], p[2]), (*nodes)[index].position);
		}
		
		float kdtree_get_pt(size_t index, int dimension) const
		{
			return (*nodes)[index].position[dimension];
		}
		
		template<class Box>
		bool kdtree_get_bbox(Box& boundingBox) const {return false;}
	};
	
	struct PaintedPointCloud
	{
		struct Mark
		{
			glm::vec3 position;
			bool isAlive;
		};
		std::vector<Mark> marks;
		
		size_t kdtree_get_point_count(void) const
		{
			return marks.size();
		}
		
		float kdtree_distance(const float* p, size_t index, size_t size) const
		{
			assert(size == 3);
			return glm::dot(glm::vec3(p[0], p[1], p[2]), marks[index].position);
		}
		
		float kdtree_get_pt(size_t index, int dimension) const
		{
			return marks[index].position[dimension];
		}
		
		template<class Box>
		bool kdtree_get_bbox(Box& boundingBox) const {return false;}
		
		void sweep(void)
		{
			size_t count = 0;
			for (size_t i = 0; i < marks.size(); i++)
			{
				if (not marks[i].isAlive)
					continue;
				marks[count] = marks[i];
				count++;
			}
			marks.resize(count);
		}
	};
	
	struct PaintableMesh
	{
		GX::Unwrapping::Mesh mesh;
		glm::vec3 center, extents;
		glm::quat rotation;
		KGPhysicsManagerBodyID bodyID;
		KGPhysicsManagerColliderID colliderID;
		/*uint16_t
			clusterIndex,
			vertexBufferIndex,
			indexBufferIndex,
			diffuseImageIndex,
			normalImageIndex,
			materialIndex;*/
	};
	
	/*struct Eraser
	{
		//KGPhysicsManagerBodyID bodyID;
		glm::vec3 target;
		bool hasTarget;
		glm::vec3 position;
		
		void initialize(KGPhysicsManager* physicsManager, KGPhysicsManagerColliderID brushColliderID)
		{
			//bodyID = KGPhysicsManagerCreateBodyWithMassAndCollider(physicsManager, 1, brushColliderID);
			//KGPhysicsManagerSetBodyFriction(physicsManager, bodyID, 0);
			hasTarget = false;
		}
		
		void setPosition(KGPhysicsManager* physicsManager, glm::vec3 to)
		{
			//KGPhysicsManagerSetBodyPosition(physicsManager, bodyID, glm::value_ptr(to));
		}
		
		void setTarget(KGPhysicsManager* physicsManager, glm::vec3 to)
		{
			hasTarget = true;
			target = to;
			std::cout << "Found target " << glm::to_string(to) << std::endl;
		} 
		
		glm::vec3 getPosition(KGPhysicsManager* physicsManager)
		{
			float position[3];
			KGPhysicsManagerGetBodyPosition(physicsManager, bodyID, position);
			return glm::vec3(position[0], position[1], position[2]);
		}
		
		glm::vec3 getLinearVelocity(KGPhysicsManager* physicsManager)
		{
			float linearVelocity[3];
			KGPhysicsManagerGetBodyLinearVelocity(physicsManager, bodyID, linearVelocity);
			return glm::vec3(linearVelocity[0], linearVelocity[1], linearVelocity[2]);
		}
		
		void deinitialize(KGPhysicsManager* physicsManager)
		{
			KGPhysicsManagerDestroyBody(physicsManager, bodyID);
		}
		
		void update(KGPhysicsManager* physicsManager, Renderer* renderer, double seconds)
		{
			if (not hasTarget)
			{
				{
					int w, h;
					glfwGetWindowSize(glfwGetCurrentContext(), & w, & h);
					const size_t testPointCount = 16;
					for (size_t i = 0; i < testPointCount; i++)
					{
						glm::vec2 point = glm::linearRand(glm::vec2(0), glm::vec2(w, h));
						glm::vec3 color = renderer->readColor(point.x, point.y);
						if (color.x < 1 or color.y < 1 or color.z < 1)
						{
							setTarget(physicsManager, renderer->unproject(point.x, point.y));
							break;
						}
					}
				}
			}
			if (not hasTarget)
				return;
			glm::vec3 position = getPosition(physicsManager);
			glm::vec3 difference = target - position;
			glm::vec3 direction = glm::normalize(difference);
			float distance = glm::length(difference);
			if (distance < 1)
			{
				renderer->erase(target, 1);
				hasTarget = false;
			}
			else
			{
				glm::vec3 linearVelocity = getLinearVelocity(physicsManager);
				
				glm::vec3 damping = linearVelocity * 1.0f;
				glm::vec3 spring = direction * 3.0f;
				glm::vec3 force = spring + glm::sphericalRand(1.0f) - damping;
				KGPhysicsManagerApplyBodyCentralForce(physicsManager, bodyID, glm::value_ptr(force));
				position += direction * seconds;
				//KGPhysicsManagerSetBodyPosition(physicsManager, bodyID, glm::value_ptr(position));
			}
			
			renderer->erase(position, 1);
		}
	};*/
	
	struct Eraser
	{
		//KGPhysicsManagerBodyID bodyID;
		glm::vec2 target, position, origin;
		bool hasTarget;
		
		void initialize(KGPhysicsManager* physicsManager, KGPhysicsManagerColliderID brushColliderID)
		{
			int w, h;
			glfwGetWindowSize(glfwGetCurrentContext(), & w, & h);
			hasTarget = false;
			position = origin = glm::linearRand(glm::vec2(0), glm::vec2(w, h));
		}
		
		void setTarget(glm::vec2 to)
		{
			hasTarget = true;
			origin = position;
			target = to;
			//std::cout << "Found target " << glm::to_string(to) << std::endl;
		}
		
		void deinitialize(KGPhysicsManager* physicsManager)
		{
		}
		
		void erase(Painter* painter)
		{
			auto renderer = painter->renderer;
			auto physicsManager = painter->physicsManager;
			glm::vec3 unprojectedPosition = renderer->unproject(position.x, position.y);
			glm::vec3 unprojectedTarget = renderer->unproject(target.x, target.y);
			
			bool shouldErase = false;
			
			for (size_t i = 0; i < 4; i++)
			{
				glm::vec2 point = position + glm::diskRand(64.0f);
				glm::vec3 color = renderer->readColor(point.x, point.y);
				if (color.x < 1 or color.y < 1 or color.z < 1)
				{
					shouldErase = true;
					break;
				}
			}
			
			if (shouldErase)
				renderer->erase(unprojectedPosition, 0.5);
		}
		
		void update(Painter* painter, double seconds)
		{
			auto renderer = painter->renderer;
			auto physicsManager = painter->physicsManager;
			if (not hasTarget)
			{
				if (painter->fleeTimeLeft <= 0)
				{
					int w, h;
					glfwGetWindowSize(glfwGetCurrentContext(), & w, & h);
					const size_t testPointCount = 4;
					for (size_t i = 0; i < testPointCount; i++)
					{
						glm::vec2 point = glm::linearRand(glm::vec2(0), glm::vec2(w, h));
						glm::vec3 color = renderer->readColor(point.x, point.y);
						if (color.x < 1 or color.y < 1 or color.z < 1)
						{
							setTarget(point);
							break;
						}
					}
				}
				else
				{
					int w, h;
					glfwGetWindowSize(glfwGetCurrentContext(), & w, & h);
					position += glm::normalize(position - painter->lastTouchPosition) * 256 * seconds;
					position = glm::clamp(position, glm::vec2(0), glm::vec2(w, h));
					erase(painter);
				}
			}
			if (not hasTarget)
			{
				return;
			}
			glm::vec2 difference = target - position;
			glm::vec2 direction = glm::normalize(difference);
			glm::vec3 unprojectedPosition = renderer->unproject(position.x, position.y);
			glm::vec3 unprojectedTarget = renderer->unproject(target.x, target.y);
			
			erase(painter);
			
			if (glm::length(unprojectedTarget - unprojectedPosition) < 1)
			{
				//renderer->erase(unprojectedTarget, 0.5);
				hasTarget = false;
			}
			else
			{
				glm::vec2 totalDifference = target - origin;
				//float totalDistance = glm::length(totalDifference);
				glm::vec2 totalDirection = glm::normalize(totalDifference);
				float distance = glm::dot(totalDirection, position - origin);
				//float t = distance / totalDistance;
				glm::quat q = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
				glm::vec3 d = q * glm::vec3(totalDirection.x, totalDirection.y, 0);
				glm::vec2 orthogonalDirection(d.x, d.y);
				float newDistance = glm::linearRand(8.0f, 4 * 64.0f) * seconds + distance;
				position = origin + totalDirection * (newDistance) + orthogonalDirection * (glm::sin(newDistance * 0.0125) * glm::linearRand(8.0f, 64.0f));
				
			}
		}
		
		/*void update(KGPhysicsManager* physicsManager, Renderer* renderer, double seconds)
		{
			if (not hasTarget)
			{
				{
					int w, h;
					glfwGetWindowSize(glfwGetCurrentContext(), & w, & h);
					const size_t testPointCount = 4;
					for (size_t i = 0; i < testPointCount; i++)
					{
						glm::vec2 point = glm::linearRand(glm::vec2(0), glm::vec2(w, h));
						glm::vec3 color = renderer->readColor(point.x, point.y);
						if (color.x < 1 or color.y < 1 or color.z < 1)
						{
							setTarget(point);
							break;
						}
					}
				}
			}
			if (not hasTarget)
				return;
			glm::vec2 difference = target - position;
			glm::vec2 direction = glm::normalize(difference);
			glm::vec3 unprojectedPosition = renderer->unproject(position.x, position.y);
			glm::vec3 unprojectedTarget = renderer->unproject(target.x, target.y);
			
			bool shouldErase = false;
			
			for (size_t i = 0; i < 4; i++)
			{
				glm::vec2 point = position + glm::diskRand(64.0f);
				glm::vec3 color = renderer->readColor(point.x, point.y);
				if (color.x < 1 or color.y < 1 or color.z < 1)
				{
					shouldErase = true;
					break;
				}
			}
			
			if (shouldErase)
				renderer->erase(unprojectedPosition, 0.5);
			
			if (glm::length(unprojectedTarget - unprojectedPosition) < 1)
			{
				renderer->erase(unprojectedTarget, 0.5);
				hasTarget = false;
			}
			else
			{
				position += direction * 32 * seconds;
			}
		}*/
	};
	
	//using PaintedPointIndexTree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<float, PaintedPointCloud>, PaintedPointCloud>;
	//using NodeIndexTree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<float, NodePointCloud>, NodePointCloud>;
	//NodeIndexTree nodeIndexTree;
	//NodePointCloud nodePointCloud;
	//NodePointCloud nodePointCloud;
	//NodeIndexTree* nodePointCloudIndexTree;
	//PaintedPointCloud paintedPointCloud;
	//PaintedPointIndexTree* paintedPointIndexTree;
	GX::Renderer* renderer;
	std::vector<PaintableMesh> paintableMeshes;
	//static constexpr float paintTimeInterval = 0.125 * 0.5;
	//static constexpr float updateTimeInterval = 0.125 * 0.125 * 0.5;
	static constexpr float paintTimeInterval = 0.125;
	static constexpr float updateTimeInterval = 0.25 * 0.25;
	//static constexpr float updateTimeInterval = 0.125;
	static constexpr float nodeTimeInterval = 5.0f;
	static constexpr float fleeTimeInterval = 0.25;
	float fleeTimeLeft = 0;
	float paintTimeLeft = 0;
	float updateTimeLeft = 0;
	
	glm::vec2 lastTouchPosition;
	
	KGPhysicsManager* physicsManager;
	
	std::array<Eraser, 8> erasers;
	
	struct BrushSettings
	{
		float radius = 0;
		size_t sampleCount = 1;
		size_t maximumLocalSampleCount = 1;
	};
	
	
	struct Brush
	{
		BrushSettings settings;
		KGPhysicsManagerBodyID bodyID;
		float timeLeft;
		
		void initialize(KGPhysicsManager* physicsManager, KGPhysicsManagerColliderID brushColliderID, float initialTime) 
		{
			bodyID = KGPhysicsManagerCreateBodyWithMassAndCollider(physicsManager, 1, brushColliderID);
			KGPhysicsManagerSetBodyFriction(physicsManager, bodyID, 0);
			timeLeft = initialTime;
		}
		
		void setPosition(KGPhysicsManager* physicsManager, glm::vec3 to)
		{
			KGPhysicsManagerSetBodyPosition(physicsManager, bodyID, glm::value_ptr(to));
		}
		
		glm::vec3 getPosition(KGPhysicsManager* physicsManager)
		{
			float position[3];
			KGPhysicsManagerGetBodyPosition(physicsManager, bodyID, position);
			return glm::vec3(position[0], position[1], position[2]);
		}
		
		void deinitialize(KGPhysicsManager* physicsManager)
		{
			KGPhysicsManagerDestroyBody(physicsManager, bodyID);
		}
	};
	
	std::vector<Brush> brushes;
	KGPhysicsManagerColliderID brushColliderID;
	
	Painter(GX::Renderer* renderer) :
		//nodeIndexTree(3, nodePointCloud, nanoflann::KDTreeSingleIndexAdaptorParams(10)),
		renderer(renderer)
	{
		physicsManager = KGPhysicsManagerNew();
		//KGPhysicsManagerSetGravity(physicsManager, glm::value_ptr(glm::vec3(0, -0.8, 0)));
		//KGPhysicsManagerSetGravity(physicsManager, glm::value_ptr(glm::vec3(0, -3.8, 0)));
		KGPhysicsManagerSetGravity(physicsManager, glm::value_ptr(glm::vec3(0, -9.8, 0)));
		brushColliderID = KGPhysicsManagerCreateCollider(physicsManager);
		KGPhysicsManagerSetColliderSphereRadius(physicsManager, brushColliderID, 0.2);
		
		for (size_t i = 0; i < erasers.size(); i++)
		{
			erasers[i].initialize(physicsManager, brushColliderID);
		}
	}
	
	~Painter(void)
	{
		for (size_t i = 0; i < erasers.size(); i++)
			erasers[i].deinitialize(physicsManager);
		for (size_t i = 0; i < brushes.size(); i++)
			brushes[i].deinitialize(physicsManager);
		//{
		//	KGPhysicsManagerDestroyBody(physicsManager, brushes[i].bodyID);
		//}
		
		for (size_t i = 0; i < paintableMeshes.size(); i++)
		{
			KGPhysicsManagerDestroyBody(physicsManager, paintableMeshes[i].bodyID);
			KGPhysicsManagerDestroyCollider(physicsManager, paintableMeshes[i].colliderID);
		}
		KGPhysicsManagerDestroyCollider(physicsManager, brushColliderID);
		KGPhysicsManagerDelete(physicsManager);
	}
	
	void paint(const glm::vec3& at, const BrushSettings& settings)
	{
		for (size_t i = 0; i < erasers.size(); i++)
		{
			erasers[i].hasTarget = false;
		}
		fleeTimeLeft = fleeTimeInterval;
		bool canSpawn = false;
		if (paintTimeLeft <= 0)
		{
			canSpawn = true;
			paintTimeLeft = paintTimeInterval;
		}
		//std::cout << "Painting" << std::endl;
		/*std::vector<std::pair<size_t, float>> indexDistancePairs;
		nodeIndexTree.radiusSearch(glm::value_ptr(at), settings.radius, indexDistancePairs, nanoflann::SearchParams(32, 0, false));
		for (size_t i = 0; i < indexDistancePairs.size(); i++)
		{
			nodePointCloud.nodes[indexDistancePairs[i].first].type = Node::Type::Dead;
		}*/
		
		/*GX::Renderer::Brush rendererBrush;
		rendererBrush.at = at;
		rendererBrush.radius = 0.9;
		rendererBrush.color = glm::vec3(153, 184, 165);
		renderer->paint(rendererBrush);*/
		
		for (size_t i = 0; i < settings.sampleCount; i++)
		{
			//glm::vec3 p = at + renderer->mainCamera.rotation * glm::vec3(0, 0, glm::gaussRand(0.3f, 0.1f));
			glm::vec3 p = at + renderer->mainCamera.rotation * glm::vec3(0, 0, glm::gaussRand(0.0f, 0.2f));
			GX::Renderer::Brush rendererBrush;
			rendererBrush.at = p;
			rendererBrush.radius = 1;
			float timeLeft = glm::linearRand(0.0f, nodeTimeInterval);
			rendererBrush.color = glm::mix(glm::vec3(153, 184, 165) * (1.0f / 256.0f), glm::vec3(189, 169, 140) * (1.0f / 256.0f), (timeLeft / nodeTimeInterval));
			renderer->paint(rendererBrush);
			
			//if (not canSpawn or glm::linearRand(0.0f, 1.0f) > 0.5f)
			if (not canSpawn)
				continue;
				//or glm::linearRand(0.0f, 1.0f) > 0.9f
		
			Brush brush;
			brush.initialize(physicsManager, brushColliderID, timeLeft);
			//brush.setPosition(physicsManager, at + glm::gaussRand(0.0f, settings.radius));
			//brush.setPosition(physicsManager, at + glm::gaussRand(0.0f, 0.1f));
			brush.setPosition(physicsManager, p);
			brushes.push_back(brush);
		}
	}
	
	void update(double seconds)
	{	
		paintTimeLeft -= seconds;
		if (paintTimeLeft < 0)
			paintTimeLeft = 0;
		fleeTimeLeft -= seconds;
		if (fleeTimeLeft < 0)
			fleeTimeLeft = 0;
		updateTimeLeft -= seconds;
		if (paintTimeLeft > 0)
			return;
		if (updateTimeLeft > 0)
			return;
		double timeDelta = updateTimeInterval - updateTimeLeft;
		updateTimeLeft = updateTimeInterval;
		KGPhysicsManagerUpdate(physicsManager, updateTimeLeft);
		
		std::vector<Brush> newBrushes;
		
		for (size_t i = 0; i < brushes.size(); i++)
		{
			Brush* brush = & brushes[i];
			
			brush->timeLeft -= updateTimeLeft;
			if (brush->timeLeft < 0)
				brush->timeLeft = 0;
			
			GX::Renderer::Brush rendererBrush;
			rendererBrush.at = brush->getPosition(physicsManager);
			float timeLeft = brush->timeLeft;
			rendererBrush.radius = glm::mix(0.5, 0.9, timeLeft / nodeTimeInterval);
			rendererBrush.color = glm::mix(glm::vec3(153, 184, 165) * (1.0f / 256.0f), glm::vec3(189, 169, 140) * (1.0f / 256.0f), (timeLeft / nodeTimeInterval));
			renderer->paint(rendererBrush);
			
			if (brush->timeLeft > 0)
				newBrushes.push_back(* brush);
			else
				brush->deinitialize(physicsManager);
		}
		
		std::swap(newBrushes, brushes);
		
		for (size_t i = 0; i < erasers.size(); i++)
		{
			//erasers[i].update(physicsManager, renderer, updateTimeLeft);
			erasers[i].update(this, updateTimeLeft);
		}
	}
	
	/*void insertMesh(const GX::Unwrapping::Mesh::Vertex* vertices, size_t vertexCount, const size_t* indices, size_t indexCount)
	{
		glm::vec3 min, max;
		for (size_t i = 0; i < vertexCount; i++)
		{
			glm::vec3 p = glm::make_vec3(vertices[i].position);
			if (i == 0)
				min = max = p;
			min = glm::min(p, min);
			max = glm::max(p, max);
		}
		
		glm::vec3 extents = (max - min) * 0.5f;
		glm::vec3 center = extents + min;
		
		GX::Unwrapping::Mesh mesh;
		mesh.vertices.reserve(vertexCount);
		mesh.indices.reserve(indexCount);
		for (size_t i = 0; i < vertexCount; i++)
		{
			mesh.vertices.push_back(vertices[i]);
		}
		for (size_t i = 0; i < indexCount; i++)
			mesh.indices.push_back(indices[i]);
		GX::Unwrapping::Mesh unwrappedMesh = GX::Unwrapping::Unwrap(mesh);
		PaintableMesh paintableMesh;
		paintableMesh.mesh = std::move(unwrappedMesh);
		paintableMesh.center = center;
		paintableMesh.extents = extents;
		GX::Renderer::StaticMeshRendererInstance meshRendererInstance;
		for (int i = 0; i < 3; i++) instance.position[i] = center[i];
		meshRendererInstance.rotation[0] = 0;
		meshRendererInstance.rotation[1] = 0;
		meshRendererInstance.rotation[2] = 0;
		meshRendererInstance.rotation[3] = 1;
		
		{
			auto staticVertexCount = paintableMesh.mesh.vertices.size();
			auto staticVertices = new GX::Renderer::StaticVertex[staticVertexCount];
			for (size_t i = 0; i < staticVertexCount; i++)
			{
				for (int j = 0; j < 3; j++)
					staticVertices[i].position[j] = paintableMesh.mesh.vertices[i].position[j];
				for (int j = 0; j < 4; j++)
					staticVertices[i].qtangent[j] = paintableMesh.mesh.vertices[i].qtangent[j];
				for (int j = 0; j < 2; j++)
				staticVertices[i].texcoord[j] = paintableMesh.mesh.vertices[i].texcoord[j];
			}
			paintableMesh.vertexBufferIndex = renderer->createStaticVertexBuffer(staticVertexCount, staticVertices);
			delete[] staticVertices;
		}
		
		{
			size_t indexCount = paintableMesh.mesh.indices.size();
			auto* indices = new uint16_t[indexCount];
			for (size_t i = 0; i < indexCount; i++)
			{
				assert(paintableMesh.mesh.indices[i] <= UINT16_MAX);
				indices[i] = paintableMesh.mesh.indices[i];
			}
			paintableMesh.indexBufferIndex = renderer->createIndexBuffer(indexCount, indices);
			delete[] indices;
		}
		
		{
			constexpr size_t width = 512;
			constexpr size_t height = 512;
			uint8_t pixels[width * height * 4];
			for (size_t y = 0; y < height; y++)
			for (size_t x = 0; x < width; x++)
			{
				size_t i = y * width + x;
				pixels[i * 4 + 0] = 255;
				pixels[i * 4 + 1] = 255;
				pixels[i * 4 + 2] = 255;
				pixels[i * 4 + 3] = 255;
			}
			paintableMesh.diffuseTextureIndex = renderer->createImage(GX::Renderer::PixelType::R8G8B8A8, width, height, pixels);
		}
		
		{
			constexpr size_t width = 2;
			constexpr size_t height = 2;
			uint8_t pixels[width * height * 4];
			for (size_t y = 0; y < height; y++)
			for (size_t x = 0; x < width; x++)
			{
				size_t i = y * width + x;
				pixels[i * 4 + 0] = 128;
				pixels[i * 4 + 1] = 128;
				pixels[i * 4 + 2] = 255;
				pixels[i * 4 + 3] = 0;
			}
			paintableMesh.normalTextureIndex = renderer->createImage(GX::Renderer::PixelType::R8G8B8A8, width, height, pixels);
		}
		
		std::vector<GX::Renderer::MaterialNode> nodes;
		
		{
			size_t base = nodes.size();
			GX::Renderer::MaterialNode node;
			node.type = GX::Renderer::MaterialNodeType::InputTexcoord;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::SampleTexture;
			node.sampleTexture.imageIndex = paintableMesh.diffuseTextureIndex;
			node.sampleTexture.texcoordNodeIndex = base;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::OutputBaseColor;
			node.outputBaseColor.nodeIndex = base + 1;
			nodes.push_back(node);
		}
		
		{
			size_t base = nodes.size();
			GX::Renderer::MaterialNode node;
			node.type = GX::Renderer::MaterialNodeType::InputTexcoord;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::SampleTexture;
			node.sampleTexture.imageIndex = paintableMesh.normalTextureIndex;
			node.sampleTexture.texcoordNodeIndex = base;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::OutputTangentSpaceNormal;
			node.outputTangentSpaceNormal.nodeIndex = base + 1;
			nodes.push_back(node);
		}
		
		paintableMesh.materialIndex = renderer->createMaterial(nodes.size(), & nodes[0]);
		
		paintableMesh.clusterIndex = renderer->createStaticMeshRendererCluster(paintableMesh.vertexBufferIndex, paintableMesh.indexBufferIndex, materialIndex, 1, & meshRendererInstance);
	}*/
	
	template<typename F1, typename F2>
	void insertMesh(glm::vec3 position, glm::quat rotation, F1 positionFunction, size_t vertexCount, F2 indexFunction, size_t indexCount)
	{
		/*PaintableMesh paintableMesh;
		paintableMesh.colliderID = KGPhysicsManagerCreateCollider(physicsManager);
		std::vector<float> positions;
		for (size_t i = 0; i < indexCount; i++)
		{
			float position[3];
			auto index = indexFunction(i);
			auto p = positionFunction(index);
			for (int j = 0; j < 3; j++) position[j] = p[j];
			positions.push_back(position[0]);
			positions.push_back(position[1]);
			positions.push_back(position[2]);
		}
		
		KGPhysicsManagerSetColliderMeshVertexCount(physicsManager, paintableMesh.colliderID, positions.size());
		KGPhysicsManagerSetColliderMeshVertexPositions(physicsManager, paintableMesh.colliderID, & positions[0]);
		
		paintableMesh.bodyID = KGPhysicsManagerCreateBodyWithMassAndCollider(physicsManager, 0, paintableMesh.colliderID);
		KGPhysicsManagerSetBodyPosition(physicsManager, paintableMesh.bodyID, glm::value_ptr(position));
		//KGPhysicsManagerSetBodyRotation(physicsManager, paintableMesh.bodyID, glm::value_ptr(rotation));*/
	}
};

};//namespace Painting
};//namespace GX

#endif
