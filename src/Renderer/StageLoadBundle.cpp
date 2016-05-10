#include "Stage.hpp"
#include <algorithm>
#include "physfs/physfs.h"
#include <sstream>
#include "lua/lua.hpp"
#include <map>
#include <string>
#include "../Core/G2AssimpScene.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#include <iostream>

struct Builder
{
	std::map<std::string, uint16_t> materialIndices;
	GX::Stage* stage;
	uint16_t bundleIndex;
	
	GX::Stage::LoadedBundle* getBundle(void)
	{
		return & stage->loadedBundles[bundleIndex];
	}
};

static uint16_t LoadImage(GX::Renderer* renderer, const char* path)
{
	int width, height, components;
	unsigned char* pixels = stbi_load(path, & width, & height, & components, 4);
	//assert(components == 3);
	if (not pixels)
	{
		std::cerr << "Could not load " << path << std::endl;
	}
	assert(pixels);
	uint16_t imageIndex = renderer->createImage(GX::Renderer::PixelType::R8G8B8A8, width, height, pixels);
	stbi_image_free(pixels);
	
	return imageIndex;
}

static void LoadSceneOld(Builder* builder, const char* path)
{
	GX::Renderer* renderer = & builder->stage->renderer;

	G2AssimpScene* assimpScene = G2AssimpSceneNewFromFile(path);
	
	struct Mesh
	{
		uint16_t
			vertexBufferIndex,
			indexBufferIndex,
			jointBufferIndex;
	};
	
	std::vector<Mesh> meshes;
	
	for (size_t meshIndex = 0; meshIndex < assimpScene->meshCount; meshIndex++)
	{
		const G2AssimpSceneSkinnedMesh* mesh = assimpScene->meshes + meshIndex;
		
		Mesh outmesh;
		
		size_t vertexCount = mesh->vertexCount;
		auto vertices = new GX::Renderer::StaticVertex[vertexCount];
		for (size_t i = 0; i < vertexCount; i++)
		{
			for (int j = 0; j < 3; j++)
				vertices[i].position[j] = mesh->vertices[i].position[j];
			for (int j = 0; j < 4; j++)
				vertices[i].qtangent[j] = mesh->vertices[i].qtangent[j];
			for (int j = 0; j < 2; j++)
				vertices[i].texcoord[j] = mesh->vertices[i].texcoord[j];
		}
		outmesh.vertexBufferIndex = renderer->createStaticVertexBuffer(vertexCount, vertices);
		delete[] vertices;
	
		size_t indexCount = mesh->indexCount;
		auto* indices = new uint16_t[indexCount];
		for (size_t i = 0; i < indexCount; i++)
		{
			assert(mesh->indices[i] <= UINT16_MAX);
			indices[i] = mesh->indices[i];
		}
		outmesh.indexBufferIndex = renderer->createIndexBuffer(indexCount, indices);
		delete[] indices;

		outmesh.jointBufferIndex = renderer->createJointBuffer(0, NULL);

		builder->getBundle()->vertexBufferIndices.push_back(outmesh.vertexBufferIndex);
		builder->getBundle()->indexBufferIndices.push_back(outmesh.indexBufferIndex);
		builder->getBundle()->jointBufferIndices.push_back(outmesh.jointBufferIndex);

		meshes.push_back(outmesh);
	}
	assert(assimpScene->meshCount == meshes.size());

	//std::map<std::pair<size_t, std::string>, std::vector<GX::Renderer::Instance>> clusterInstances;
	std::map<std::pair<size_t, std::string>, std::vector<GX::Renderer::StaticMeshRendererInstance>> clusterInstances;

	for (size_t entityIndex = 0; entityIndex < assimpScene->entityCount; entityIndex++)
	{
		const G2AssimpSceneEntity* entity = assimpScene->entities + entityIndex;
		if (not entity->hasMesh) continue;
		assert(meshes.size() > entity->mesh);
		
		//GX::Renderer::Instance instance;
		GX::Renderer::StaticMeshRendererInstance instance;
		for (int i = 0 ; i < 3; i++) instance.position[i] = entity->position[i];
		for (int i = 0 ; i < 4; i++) instance.rotation[i] = entity->rotation[i];
		
		std::string materialName(entity->materialName);
		std::pair<size_t, std::string> key(entity->mesh, std::move(materialName));
		clusterInstances[std::move(key)].push_back(instance);
	}
	
	G2AssimpSceneDelete(assimpScene);
	
	for (auto i = clusterInstances.cbegin(); i not_eq clusterInstances.cend(); i++)
	{
		/*GX::Renderer::Cluster cluster;
		
		cluster.vertexBufferIndex = meshes[i->first.first].vertexBufferIndex;
		cluster.indexBufferIndex = meshes[i->first.first].indexBufferIndex;
		cluster.jointBufferIndex = meshes[i->first.first].jointBufferIndex;
		cluster.materialIndex = renderer->createMaterial(0, NULL);
		cluster.instanceBufferIndex = renderer->createInstanceBuffer(i->second.size(), & i->second[0]);*/
		
		std::cerr << "Using material " << i->first.second << std::endl;
		uint16_t materialIndex = builder->materialIndices.at(i->first.second);
		assert (builder->materialIndices.find(i->first.second) not_eq builder->materialIndices.cend());
		
		uint16_t clusterIndex = renderer->createStaticMeshRendererCluster(meshes[i->first.first].vertexBufferIndex, meshes[i->first.first].indexBufferIndex, materialIndex, i->second.size(), & i->second[0]);
		builder->getBundle()->clusterIndices.push_back(clusterIndex);
		//renderer->createCluster(cluster);
		//renderer->createClusterBuffer(1, & cluster);
	}
}

static void LoadSceneOld2(Builder* builder, const char* path)
{
	GX::Renderer* renderer = & builder->stage->renderer;

	G2AssimpScene* assimpScene = G2AssimpSceneNewFromFile(path);
	
	struct Mesh
	{
		uint16_t
			vertexBufferIndex,
			indexBufferIndex,
			jointBufferIndex,
			materialIndex;
	};
	
	std::vector<uint16_t> textures;
	
	for (size_t i = 0; i < assimpScene->textureCount; i++)
	{
		uint16_t imageIndex = LoadImage(& builder->stage->renderer, assimpScene->textures[i].path);
		builder->getBundle()->imageIndices.push_back(imageIndex);
		textures.push_back(imageIndex);
	}
	
	std::vector<uint16_t> materials;
	
	for (size_t i = 0; i < assimpScene->materialCount; i++)
	{
		auto material = & assimpScene->materials[i];
		//assert (material->flags & (G2AssimpSceneMaterialFlagHasNormal | G2AssimpSceneMaterialFlagHasDiffuse));
		
		std::cout << "material->name = " << material->name << std::endl;
		
		std::vector<GX::Renderer::MaterialNode> nodes;
		if (material->flags & G2AssimpSceneMaterialFlagHasDiffuse)
		{
			size_t base = nodes.size();
			GX::Renderer::MaterialNode node;
			node.type = GX::Renderer::MaterialNodeType::InputTexcoord;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::SampleTexture;
			node.sampleTexture.imageIndex = material->diffuseTextureIndex;
			node.sampleTexture.texcoordNodeIndex = base;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::OutputBaseColor;
			node.outputBaseColor.nodeIndex = base + 1;
			nodes.push_back(node);
		}
		if (material->flags & G2AssimpSceneMaterialFlagHasNormal)
		{
			size_t base = nodes.size();
			GX::Renderer::MaterialNode node;
			node.type = GX::Renderer::MaterialNodeType::InputTexcoord;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::SampleTexture;
			node.sampleTexture.imageIndex = material->normalTextureIndex;
			node.sampleTexture.texcoordNodeIndex = base;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::OutputTangentSpaceNormal;
			node.outputTangentSpaceNormal.nodeIndex = base + 1;
			nodes.push_back(node);
		}
		
		//assert(nodes.size() > 0);
		//if (nodes.size() > 0)
		if ((material->flags & G2AssimpSceneMaterialFlagHasDiffuse) and (material->flags & G2AssimpSceneMaterialFlagHasNormal))
		{
			uint16_t materialIndex = builder->stage->renderer.createMaterial(nodes.size(), & nodes[0]);
			builder->getBundle()->materialIndices.push_back(materialIndex);
		
			materials.push_back(materialIndex);
		}
		else
			materials.push_back(UINT16_MAX);
			//materials.push_back(0);
	}
	
	std::vector<Mesh> meshes;
	
	for (size_t meshIndex = 0; meshIndex < assimpScene->meshCount; meshIndex++)
	{
		const G2AssimpSceneSkinnedMesh* mesh = assimpScene->meshes + meshIndex;
		
		Mesh outmesh;
		
		size_t vertexCount = mesh->vertexCount;
		auto vertices = new GX::Renderer::StaticVertex[vertexCount];
		for (size_t i = 0; i < vertexCount; i++)
		{
			for (int j = 0; j < 3; j++)
				vertices[i].position[j] = mesh->vertices[i].position[j];
			for (int j = 0; j < 4; j++)
				vertices[i].qtangent[j] = mesh->vertices[i].qtangent[j];
			for (int j = 0; j < 2; j++)
				vertices[i].texcoord[j] = mesh->vertices[i].texcoord[j];
		}
		outmesh.vertexBufferIndex = renderer->createStaticVertexBuffer(vertexCount, vertices);
		delete[] vertices;
	
		size_t indexCount = mesh->indexCount;
		auto* indices = new uint16_t[indexCount];
		for (size_t i = 0; i < indexCount; i++)
		{
			assert(mesh->indices[i] <= UINT16_MAX);
			indices[i] = mesh->indices[i];
		}
		outmesh.indexBufferIndex = renderer->createIndexBuffer(indexCount, indices);
		delete[] indices;

		outmesh.jointBufferIndex = renderer->createJointBuffer(0, NULL);

		builder->getBundle()->vertexBufferIndices.push_back(outmesh.vertexBufferIndex);
		builder->getBundle()->indexBufferIndices.push_back(outmesh.indexBufferIndex);
		builder->getBundle()->jointBufferIndices.push_back(outmesh.jointBufferIndex);

		outmesh.materialIndex = materials[mesh->materialIndex];
		
		meshes.push_back(outmesh);
	}
	assert(assimpScene->meshCount == meshes.size());

	//std::map<std::pair<size_t, std::string>, std::vector<GX::Renderer::Instance>> clusterInstances;
	std::map<std::pair<size_t, std::string>, std::vector<GX::Renderer::StaticMeshRendererInstance>> clusterInstances;

	for (size_t entityIndex = 0; entityIndex < assimpScene->entityCount; entityIndex++)
	{
		const G2AssimpSceneEntity* entity = assimpScene->entities + entityIndex;
		if (not entity->hasMesh) continue;
		assert(meshes.size() > entity->mesh);
		
		//GX::Renderer::Instance instance;
		GX::Renderer::StaticMeshRendererInstance instance;
		for (int i = 0 ; i < 3; i++) instance.position[i] = entity->position[i];
		for (int i = 0 ; i < 4; i++) instance.rotation[i] = entity->rotation[i];
		
		std::string materialName(entity->materialName);
		std::pair<size_t, std::string> key(entity->mesh, std::move(materialName));
		clusterInstances[std::move(key)].push_back(instance);
	}
	
	G2AssimpSceneDelete(assimpScene);
	
	for (auto i = clusterInstances.cbegin(); i not_eq clusterInstances.cend(); i++)
	{
		/*GX::Renderer::Cluster cluster;
		
		cluster.vertexBufferIndex = meshes[i->first.first].vertexBufferIndex;
		cluster.indexBufferIndex = meshes[i->first.first].indexBufferIndex;
		cluster.jointBufferIndex = meshes[i->first.first].jointBufferIndex;
		cluster.materialIndex = renderer->createMaterial(0, NULL);
		cluster.instanceBufferIndex = renderer->createInstanceBuffer(i->second.size(), & i->second[0]);*/
		
		std::cerr << "Using material " << i->first.second << std::endl;
		uint16_t materialIndex = meshes[i->first.first].materialIndex;//builder->materialIndices.at(i->first.second);
		//assert (builder->materialIndices.find(i->first.second) not_eq builder->materialIndices.cend());
		if (builder->materialIndices.find(i->first.second) not_eq builder->materialIndices.cend())
			materialIndex = builder->materialIndices[i->first.second];
		
		std::cout << i->first.second << "'s materialIndex is " << materialIndex << std::endl;
		
		assert(materialIndex not_eq UINT16_MAX);
		uint16_t clusterIndex = renderer->createStaticMeshRendererCluster(meshes[i->first.first].vertexBufferIndex, meshes[i->first.first].indexBufferIndex, materialIndex, i->second.size(), & i->second[0]);
		builder->getBundle()->clusterIndices.push_back(clusterIndex);
		//renderer->createCluster(cluster);
		//renderer->createClusterBuffer(1, & cluster);
	}
}

static void LoadScene(Builder* builder, const char* path)
{
	GX::Renderer* renderer = & builder->stage->renderer;
	
	uint16_t fancyMaterialIndex = 0;

	{
		constexpr size_t nodeCount = 6;
		GX::Renderer::MaterialNode nodes[nodeCount];
		nodes[0].type = GX::Renderer::MaterialNodeType::InputTexcoord;
		nodes[1].type = GX::Renderer::MaterialNodeType::SampleTexture;
		nodes[1].sampleTexture.imageIndex = LoadImage(renderer, "data/Images/AOlarge.png");
		//nodes[1].sampleTexture.imageIndex = LoadImage(renderer, "/Users/Oliver/Documents/College/60-412/GX 60-412 Edition/data/Images/AO.png");
		nodes[1].sampleTexture.texcoordNodeIndex = 0;
		nodes[2].type = GX::Renderer::MaterialNodeType::OutputBaseColor;
		nodes[2].outputBaseColor.nodeIndex = 1;
		nodes[3].type = GX::Renderer::MaterialNodeType::SampleTexture;
		//nodes[3].sampleTexture.imageIndex = LoadImage(renderer, "/Users/Oliver/Documents/College/60-412/GX 60-412 Edition/data/Images/NormalMap.png");
		nodes[3].sampleTexture.imageIndex = LoadImage(renderer, "data/Images/NormalMap.png");
		nodes[3].sampleTexture.texcoordNodeIndex = 0;
		nodes[4].type = GX::Renderer::MaterialNodeType::OutputTangentSpaceNormal;
		nodes[4].outputTangentSpaceNormal.nodeIndex = 3;
		
		//constexpr size_t size = 4096 * 2;
		constexpr size_t size = 2048;
		uint8_t* pixels = new uint8_t[size * size * 4];
		for (size_t y = 0; y < size; y++)
		for (size_t x = 0; x < size; x++)
		{
			pixels[(y * size + x) * 4 + 0] = 255;
			pixels[(y * size + x) * 4 + 1] = 255;
			pixels[(y * size + x) * 4 + 2] = 255;
			pixels[(y * size + x) * 4 + 3] = 255;
		}
		nodes[5].paintedTexture.imageIndex = renderer->createImage(GX::Renderer::PixelType::R8G8B8A8, size, size, pixels);
		nodes[5].type = GX::Renderer::MaterialNodeType::PaintedTexture;
		delete[] pixels;
	
		fancyMaterialIndex = builder->stage->renderer.createMaterial(nodeCount, nodes);
	}

	G2AssimpScene* assimpScene = G2AssimpSceneNewFromFile(path);
	
	struct Mesh
	{
		uint16_t
			vertexBufferIndex,
			indexBufferIndex,
			jointBufferIndex,
			materialIndex;
	};
	
	std::vector<uint16_t> textures;
	
	for (size_t i = 0; i < assimpScene->textureCount; i++)
	{
		uint16_t imageIndex = LoadImage(& builder->stage->renderer, assimpScene->textures[i].path);
		builder->getBundle()->imageIndices.push_back(imageIndex);
		textures.push_back(imageIndex);
	}
	
	std::vector<uint16_t> materials;
	
	for (size_t i = 0; i < assimpScene->materialCount; i++)
	{
		auto material = & assimpScene->materials[i];
		//assert (material->flags & (G2AssimpSceneMaterialFlagHasNormal | G2AssimpSceneMaterialFlagHasDiffuse));
		
		std::cout << "material->name = " << material->name << std::endl;
		
		std::vector<GX::Renderer::MaterialNode> nodes;
		if (material->flags & G2AssimpSceneMaterialFlagHasDiffuse)
		{
			size_t base = nodes.size();
			GX::Renderer::MaterialNode node;
			node.type = GX::Renderer::MaterialNodeType::InputTexcoord;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::SampleTexture;
			node.sampleTexture.imageIndex = material->diffuseTextureIndex;
			node.sampleTexture.texcoordNodeIndex = base;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::OutputBaseColor;
			node.outputBaseColor.nodeIndex = base + 1;
			nodes.push_back(node);
		}
		if (material->flags & G2AssimpSceneMaterialFlagHasNormal)
		{
			size_t base = nodes.size();
			GX::Renderer::MaterialNode node;
			node.type = GX::Renderer::MaterialNodeType::InputTexcoord;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::SampleTexture;
			node.sampleTexture.imageIndex = material->normalTextureIndex;
			node.sampleTexture.texcoordNodeIndex = base;
			nodes.push_back(node);
			node.type = GX::Renderer::MaterialNodeType::OutputTangentSpaceNormal;
			node.outputTangentSpaceNormal.nodeIndex = base + 1;
			nodes.push_back(node);
		}
		
		{
			GX::Renderer::MaterialNode node;
			constexpr size_t size = 2 * 2;
			uint8_t pixels[size * size * 4];
			for (size_t y = 0; y < size; y++)
			for (size_t x = 0; x < size; x++)
			{
				pixels[(y * size + x) * 4 + 0] = 255;
				pixels[(y * size + x) * 4 + 1] = 255;
				pixels[(y * size + x) * 4 + 2] = 255;
				pixels[(y * size + x) * 4 + 3] = 255;
			}
			node.paintedTexture.imageIndex = renderer->createImage(GX::Renderer::PixelType::R8G8B8A8, size, size, pixels);
			node.type = GX::Renderer::MaterialNodeType::PaintedTexture;
			nodes.push_back(node);
		}
		
		//assert(nodes.size() > 0);
		//if (nodes.size() > 0)
		if ((material->flags & G2AssimpSceneMaterialFlagHasDiffuse) and (material->flags & G2AssimpSceneMaterialFlagHasNormal))
		{
			uint16_t materialIndex = builder->stage->renderer.createMaterial(nodes.size(), & nodes[0]);
			builder->getBundle()->materialIndices.push_back(materialIndex);
		
			materials.push_back(materialIndex);
		}
		else
			materials.push_back(UINT16_MAX);
			//materials.push_back(0);
	}
	
	std::vector<Mesh> meshes;
	
	glm::vec2 mintexcoord, maxtexcoord;
	bool isFirst = true;
	
	for (size_t meshIndex = 0; meshIndex < assimpScene->meshCount; meshIndex++)
	{
		const G2AssimpSceneSkinnedMesh* mesh = assimpScene->meshes + meshIndex;
		
		Mesh outmesh;
		
		size_t vertexCount = mesh->vertexCount;
		auto vertices = new GX::Renderer::StaticVertex[vertexCount];
		for (size_t i = 0; i < vertexCount; i++)
		{
			for (int j = 0; j < 3; j++)
				vertices[i].position[j] = mesh->vertices[i].position[j];
			for (int j = 0; j < 4; j++)
				vertices[i].qtangent[j] = mesh->vertices[i].qtangent[j];
			for (int j = 0; j < 2; j++)
				vertices[i].texcoord[j] = mesh->vertices[i].texcoord[j];
			vertices[i].texcoord[1] = 1.0f - mesh->vertices[i].texcoord[1];
			{
				glm::vec2 p(vertices[i].texcoord[0], vertices[i].texcoord[1]);
				if (isFirst)
				{
					mintexcoord = maxtexcoord = p;
				}
				else
				{
					mintexcoord = glm::min(p, mintexcoord);
					maxtexcoord = glm::max(p, maxtexcoord);
				}
			}
		}
		outmesh.vertexBufferIndex = renderer->createStaticVertexBuffer(vertexCount, vertices);
		delete[] vertices;
	
		size_t indexCount = mesh->indexCount;
		auto* indices = new uint16_t[indexCount];
		for (size_t i = 0; i < indexCount; i++)
		{
			assert(mesh->indices[i] <= UINT16_MAX);
			indices[i] = mesh->indices[i];
		}
		outmesh.indexBufferIndex = renderer->createIndexBuffer(indexCount, indices);
		delete[] indices;

		outmesh.jointBufferIndex = renderer->createJointBuffer(0, NULL);

		builder->getBundle()->vertexBufferIndices.push_back(outmesh.vertexBufferIndex);
		builder->getBundle()->indexBufferIndices.push_back(outmesh.indexBufferIndex);
		builder->getBundle()->jointBufferIndices.push_back(outmesh.jointBufferIndex);

		outmesh.materialIndex = materials[mesh->materialIndex];
		
		meshes.push_back(outmesh);
	}
	assert(assimpScene->meshCount == meshes.size());
	
	std::cout << "maxtexcoord " << glm::to_string(maxtexcoord) << std::endl;
	std::cout << "mintexcoord " << glm::to_string(mintexcoord) << std::endl;

	//std::map<std::pair<size_t, std::string>, std::vector<GX::Renderer::Instance>> clusterInstances;
	std::map<std::pair<size_t, std::string>, std::vector<GX::Renderer::StaticMeshRendererInstance>> clusterInstances;

	for (size_t entityIndex = 0; entityIndex < assimpScene->entityCount; entityIndex++)
	{
		const G2AssimpSceneEntity* entity = assimpScene->entities + entityIndex;
		if (not entity->hasMesh) continue;
		assert(meshes.size() > entity->mesh);
		
		//GX::Renderer::Instance instance;
		GX::Renderer::StaticMeshRendererInstance instance;
		for (int i = 0 ; i < 3; i++) instance.position[i] = entity->position[i];
		for (int i = 0 ; i < 4; i++) instance.rotation[i] = entity->rotation[i];
		
		std::string materialName(entity->materialName);
		std::pair<size_t, std::string> key(entity->mesh, std::move(materialName));
		clusterInstances[std::move(key)].push_back(instance);
		
		const auto& mesh = assimpScene->meshes[entity->mesh];
		
		auto positionFunction = 
		[&](size_t i)
		{
			auto v = mesh.vertices[i];
			return glm::vec3(v.position[0], v.position[1], v.position[2]);
		};
		
		auto indexFunction = 
		[&](size_t i)
		{
			return mesh.indices[i];
		};
		
		glm::vec3 position(instance.position[0], instance.position[1], instance.position[2]);
		glm::quat rotation(instance.rotation[3], instance.rotation[0], instance.rotation[1], instance.rotation[2]);
		builder->stage->painter.insertMesh(position, rotation, positionFunction, mesh.vertexCount, indexFunction, mesh.indexCount);
	}
	
	G2AssimpSceneDelete(assimpScene);
	
	for (auto i = clusterInstances.cbegin(); i not_eq clusterInstances.cend(); i++)
	{
		/*GX::Renderer::Cluster cluster;
		
		cluster.vertexBufferIndex = meshes[i->first.first].vertexBufferIndex;
		cluster.indexBufferIndex = meshes[i->first.first].indexBufferIndex;
		cluster.jointBufferIndex = meshes[i->first.first].jointBufferIndex;
		cluster.materialIndex = renderer->createMaterial(0, NULL);
		cluster.instanceBufferIndex = renderer->createInstanceBuffer(i->second.size(), & i->second[0]);*/
		
		std::cerr << "Using material " << i->first.second << std::endl;
		uint16_t materialIndex = meshes[i->first.first].materialIndex;//builder->materialIndices.at(i->first.second);
		//assert (builder->materialIndices.find(i->first.second) not_eq builder->materialIndices.cend());
		if (builder->materialIndices.find(i->first.second) not_eq builder->materialIndices.cend())
			materialIndex = builder->materialIndices[i->first.second];
		
		std::cout << i->first.second << "'s materialIndex is " << materialIndex << std::endl;
		
		assert(materialIndex not_eq UINT16_MAX);
		uint16_t clusterIndex = renderer->createStaticMeshRendererCluster(meshes[i->first.first].vertexBufferIndex, meshes[i->first.first].indexBufferIndex, fancyMaterialIndex, i->second.size(), & i->second[0]);
		builder->getBundle()->clusterIndices.push_back(clusterIndex);
		//renderer->createCluster(cluster);
		//renderer->createClusterBuffer(1, & cluster);
	}
}

static int LuaLoadImage(lua_State* L)
{
	const char* path = lua_tostring(L, 1);
	Builder* builder = (Builder*) lua_touserdata(L, lua_upvalueindex(1));
	uint16_t imageIndex = LoadImage(& builder->stage->renderer, path);
	builder->getBundle()->imageIndices.push_back(imageIndex);
	lua_pushnumber(L, imageIndex);
	return 1;
}

static int LuaLoadSimpleMaterial(lua_State* L)
{
	Builder* builder = (Builder*) lua_touserdata(L, lua_upvalueindex(1));
	/*std::vector<GX::Renderer::MaterialNode> nodes;
	lua_pushnil(L);
	while (lua_next(L, 1))
	{
		auto key = lua_tonumber(L, -2);
		//const char* value = lua_tostring(L, -1);
		GX::Renderer::MaterialNode node;
		
		
		lua_pop(L, 1);
	}*/
	
	lua_getfield(L, -1, "baseColorImageIndex");
	uint16_t baseColorImageIndex = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "normalImageIndex");
	uint16_t normalImageIndex = lua_tonumber(L, -1);
	lua_pop(L, 1);
	
	constexpr size_t nodeCount = 6;
	GX::Renderer::MaterialNode nodes[nodeCount];
	nodes[0].type = GX::Renderer::MaterialNodeType::InputTexcoord;
	nodes[1].type = GX::Renderer::MaterialNodeType::SampleTexture;
	nodes[1].sampleTexture.imageIndex = baseColorImageIndex;
	nodes[1].sampleTexture.texcoordNodeIndex = 0;
	nodes[2].type = GX::Renderer::MaterialNodeType::OutputBaseColor;
	nodes[2].outputBaseColor.nodeIndex = 1;
	nodes[3].type = GX::Renderer::MaterialNodeType::SampleTexture;
	nodes[3].sampleTexture.imageIndex = normalImageIndex;
	nodes[3].sampleTexture.texcoordNodeIndex = 0;
	nodes[4].type = GX::Renderer::MaterialNodeType::OutputTangentSpaceNormal;
	nodes[4].outputTangentSpaceNormal.nodeIndex = 3;
	
	{
		GX::Renderer::MaterialNode node;
		constexpr size_t size = 2 * 2;
		uint8_t pixels[size * size * 4];
		for (size_t y = 0; y < size; y++)
		for (size_t x = 0; x < size; x++)
		{
			pixels[(y * size + x) * 4 + 0] = 255;
			pixels[(y * size + x) * 4 + 1] = 255;
			pixels[(y * size + x) * 4 + 2] = 255;
			pixels[(y * size + x) * 4 + 3] = 255;
		}
		node.paintedTexture.imageIndex = builder->stage->renderer.createImage(GX::Renderer::PixelType::R8G8B8A8, size, size, pixels);
		node.type = GX::Renderer::MaterialNodeType::PaintedTexture;
		nodes[5] = node;
	}
	
	uint16_t materialIndex = builder->stage->renderer.createMaterial(nodeCount, nodes);
	builder->getBundle()->materialIndices.push_back(materialIndex);
	lua_pushnumber(L, materialIndex);
	return 1;
}

static int LuaLoadScene(lua_State* L)
{
	const char* path = lua_tostring(L, 1);
	Builder* builder = (Builder*) lua_touserdata(L, lua_upvalueindex(1));
	
	builder->materialIndices.clear();
	
	assert(lua_istable(L, 2));
	
	lua_pushnil(L);
	while (lua_next(L, 2))
	{
		auto key = lua_tostring(L, -2);
		uint16_t value  = lua_tonumber(L, -1);
		
		builder->materialIndices[std::string(key)] = value;
		
		lua_pop(L, 1);
	}
	
	LoadScene(builder, path);
	return 0;
}

void GX::Stage::loadBundle(uint16_t bundleIndex)
{
	LoadedBundle* bundle = & loadedBundles[bundleIndex];
	
	std::stringstream bundlePath;
	bundlePath << workingDirectory << "/raw_bundles/" << bundle->permanentBundleID << ".lua";
	/*PHYSFS_File* file = PHYSFS_openRead(bundlePath.str().c_str());
	
	//Load other bundles
	PHYSFS_uint64 bundleIndexCount = 0;
	PHYSFS_readULE64(file, & bundleIndexCount);
	for (PHYSFS_uint64 i = 0; i < bundleIndexCount; i++)
	{
		PHYSFS_uint64 permanentBundleID = 0;
		PHYSFS_readULE64(file, & permanentBundleID);
		loadedBundles[bundleIndex].bundleIndices.push_back(acquireBundle(permanentBundleID));
	}
	
	PHYSFS_close(file);*/
	
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	
	Builder builder;
	builder.bundleIndex = bundleIndex;
	builder.stage = this;
	
	/*{
		//std::string materialName("Material");
		constexpr size_t nodeCount = 5;
		auto baseColorImageIndex = LoadImage(& renderer, "/Users/Oliver/Documents/Dev/Projects/Games/G4_copy/BrickLargeBlocks0026_1_S.jpg");
		auto normalImageIndex = LoadImage(& renderer, "/Users/Oliver/Documents/Dev/Projects/Games/G4_copy/BrickLargeBlocks0026_1_S normalmap.png");
		GX::Renderer::MaterialNode nodes[nodeCount];
		nodes[0].type = GX::Renderer::MaterialNodeType::InputTexcoord;
		nodes[1].type = GX::Renderer::MaterialNodeType::SampleTexture;
		nodes[1].sampleTexture.imageIndex = baseColorImageIndex;
		nodes[1].sampleTexture.texcoordNodeIndex = 0;
		nodes[2].type = GX::Renderer::MaterialNodeType::OutputBaseColor;
		nodes[2].outputBaseColor.nodeIndex = 1;
		nodes[3].type = GX::Renderer::MaterialNodeType::SampleTexture;
		nodes[3].sampleTexture.imageIndex = normalImageIndex;
		nodes[3].sampleTexture.texcoordNodeIndex = 0;
		nodes[4].type = GX::Renderer::MaterialNodeType::OutputTangentSpaceNormal;
		nodes[4].outputTangentSpaceNormal.nodeIndex = 3;
		auto materialIndex = renderer.createMaterial(nodeCount, nodes);
		builder.getBundle()->materialIndices.push_back(materialIndex);
		builder.materialIndices[std::string("DefaultMaterial")] = materialIndex;
		builder.materialIndices[std::string("Material")] = materialIndex;
		builder.materialIndices[std::string("Material.003")] = materialIndex;
		builder.materialIndices[std::string("Material.002")] = materialIndex;
		builder.materialIndices[std::string("Material.001")] = materialIndex;
	}*/
	
	lua_pushlightuserdata(L, & builder);
	lua_pushcclosure(L, LuaLoadImage, 1);
	lua_setglobal(L, "LoadImage");
	
	//lua_pushcfunction(L, LuaCreateMaterial);
	lua_pushlightuserdata(L, & builder);
	lua_pushcclosure(L, LuaLoadSimpleMaterial, 1);
	lua_setglobal(L, "LoadSimpleMaterial");
	
	lua_pushlightuserdata(L, & builder);
	lua_pushcclosure(L, LuaLoadScene, 1);
	lua_setglobal(L, "LoadScene");
	
	if(luaL_dofile(L, bundlePath.str().c_str()))
	{
		std::cerr << lua_tostring(L, -1) << std::endl;
		abort();
	}
	
	lua_close(L);
}
