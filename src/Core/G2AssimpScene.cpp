#include "G2AssimpScene.h"
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <assimp/vector3.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <functional>
#include <iostream>
#include <vector>
#include <map>
#include <cstring>

#define PUBLIC_C_VISIBILITY extern "C" __attribute__ ((visibility ("default")))

static char* CloneString(const char* s)
{
	if (not s) return NULL;
	size_t size = strlen(s) + 1;
	char* r = (char*) memcpy(malloc(size), s, size);
	r[size - 1] = 0;
	return r;
}

PUBLIC_C_VISIBILITY
G2AssimpScene* G2AssimpSceneNewFromFile(const char* filename)
{
	std::string directory(filename);
	{
		size_t length = strlen(filename);
		
		for (size_t i = length; i-- > 0; )
		{
			if (filename[i] == '/')
				break;
			directory.pop_back();
		}
	}

	G2AssimpScene* outputScene = (G2AssimpScene*) calloc(1, sizeof(G2AssimpScene));
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, UINT16_MAX);
	importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 33.0f);
	//importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
	//& (~aiProcess_GenSmoothNormals) | aiProcess_GenNormals
	// & (~aiProcess_GenSmoothNormals) | aiProcess_GenNormals
	//& (~aiProcess_RemoveRedundantMaterials)
	//| aiProcess_RemoveComponent
	const aiScene* scene = importer.ReadFile(std::string(filename), (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_TransformUVCoords | aiProcess_GenSmoothNormals | aiProcess_SplitLargeMeshes));
	
	if (not scene)
	{
		std::cerr << importer.GetErrorString() << "\n";
		free(outputScene);
		return NULL;
	}
	assert(scene);
	
	std::vector<G2AssimpSceneSkinnedMesh> outputMeshes;
	for (size_t i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[i];
		G2AssimpSceneSkinnedMesh outputMesh;
		memset(& outputMesh, 0, sizeof(outputMesh));
		outputMesh.vertices = (G2AssimpSceneSkinnedMeshVertex*) calloc(mesh->mNumVertices, sizeof(G2AssimpSceneSkinnedMeshVertex));
		outputMesh.vertexCount = mesh->mNumVertices;
		for (size_t j = 0; j < mesh->mNumVertices; j++)
		{
			assert (mesh->mVertices);
			{
				outputMesh.vertices[j].position[0] = mesh->mVertices[j].x;
				outputMesh.vertices[j].position[1] = mesh->mVertices[j].y;
				outputMesh.vertices[j].position[2] = mesh->mVertices[j].z;
			}
			
			if(mesh->mNumUVComponents[0] == 2)
			{
				outputMesh.vertices[j].texcoord[0] = mesh->mTextureCoords[0][j].x;
				outputMesh.vertices[j].texcoord[1] = mesh->mTextureCoords[0][j].y;
			}
			
			if(mesh->mNormals and mesh->mTangents and mesh->mBitangents)
			{
				glm::vec3 normal(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
				glm::vec3 tangent(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
				glm::vec3 bitangent(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);

				//Assimp doesn't guarrantee orthogonality

				normal = glm::normalize(normal);
				tangent = glm::normalize(tangent);
				bitangent = glm::normalize(bitangent);
				
				tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);
				bitangent = glm::normalize(bitangent - glm::dot(bitangent, normal) * normal - glm::dot(bitangent, tangent) * tangent);

				//assert(glm::dot(normal, normal) > 0);
				//assert(glm::dot(tangent, tangent) > 0);
				//assert(glm::dot(bitangent, bitangent) > 0);
				//assert(glm::abs(glm::dot(normal, tangent)) < 0.01);
				//assert(glm::abs(glm::dot(bitangent, tangent)) < 0.01);
				//assert(glm::abs(glm::dot(normal, bitangent)) < 0.01);

				glm::mat3 m = glm::mat3(tangent, bitangent, normal);
				//Make special orthogonal (det m = 1)
				if (glm::abs(glm::determinant(m) - 1.0f) > 0.01)
					m = glm::mat3(tangent, -bitangent, normal);
				//assert (glm::abs(glm::determinant(m) - 1.0f) < 0.01);
				glm::quat qtangent = glm::normalize(glm::quat_cast(m));

				if(qtangent.w < 0)
					qtangent = -qtangent;
				const float bias = 0.0001;
				if (qtangent.w < bias)
				{
					/*float factor = sqrt(1 - bias*bias);
					qtangent.x *= factor;
					qtangent.y *= factor;
					qtangent.z *= factor;*/
					qtangent.w = bias;
					qtangent = glm::normalize(qtangent);
				}
				//if(glm::dot(glm::cross(normal, tangent), bitangent) < 0)
				if (glm::dot(qtangent * glm::vec3(0, 1, 0), bitangent) < 0)
					qtangent = -qtangent;
				//qtangent = glm::quat(0, normal.x, normal.y, normal.z);*/
				outputMesh.vertices[j].qtangent[0] = qtangent.x;
				outputMesh.vertices[j].qtangent[1] = qtangent.y;
				outputMesh.vertices[j].qtangent[2] = qtangent.z;
				outputMesh.vertices[j].qtangent[3] = qtangent.w;
			}
		}
		
		outputMesh.indices = (size_t*) calloc(mesh->mNumFaces * 3, sizeof(size_t));
		outputMesh.indexCount = 0;
		//outputMesh.indexCount = mesh->mNumFaces * 3;
		for (size_t j = 0; j < mesh->mNumFaces; j++)
		{
			if (mesh->mFaces[j].mNumIndices not_eq 3)
				continue;
			assert(mesh->mFaces[j].mNumIndices == 3);
			assert(mesh->mFaces[j].mIndices);
			for (size_t k = 0; k < 3; k++)
				outputMesh.indices[outputMesh.indexCount++] = mesh->mFaces[j].mIndices[k];
				//outputMesh.indices[j * 3 + k] = mesh->mFaces[j].mIndices[k];
		}
		
		auto addVertexJoint =
		[&](size_t vertexID, float weight, size_t jointID)
		{
			for (size_t k = 0; k < 4; k++)
			{
				if(outputMesh.vertices[vertexID].jointWeights[k] == 0)
				{
					outputMesh.vertices[vertexID].jointWeights[k] = weight;
					outputMesh.vertices[vertexID].joints[k] = jointID;
					return;
				}
			}
			abort();
		};
		
		//To do: Generate parent hierarchy
		outputMesh.joints = (G2AssimpSceneSkinnedMeshJoint*) calloc(mesh->mNumBones, sizeof(G2AssimpSceneSkinnedMeshJoint));
		outputMesh.jointCount = mesh->mNumBones;
		for (size_t j = 0; j < mesh->mNumBones; j++)
		{
			outputMesh.joints[j].name = CloneString(mesh->mBones[j]->mName.C_Str());
			aiVector3D scale, position;
			aiQuaternion rotation;
			
			mesh->mBones[j]->mOffsetMatrix.Decompose(scale, rotation, position);
			
			outputMesh.joints[j].position[0] = position.x;
			outputMesh.joints[j].position[1] = position.y;
			outputMesh.joints[j].position[2] = position.z;
			outputMesh.joints[j].rotation[0] = rotation.x;
			outputMesh.joints[j].rotation[1] = rotation.y;
			outputMesh.joints[j].rotation[2] = rotation.z;
			outputMesh.joints[j].rotation[3] = rotation.w;
			
			for (size_t k = 0; k < mesh->mBones[j]->mNumWeights; k++)
			{
				addVertexJoint(mesh->mBones[j]->mWeights[k].mVertexId, mesh->mBones[j]->mWeights[k].mWeight, j);
			}
		}
		
		outputMesh.materialIndex = mesh->mMaterialIndex;
		outputMesh.name = CloneString(mesh->mName.C_Str());
		outputMeshes.push_back(outputMesh);
	}
	
	std::map<std::string, size_t> textureMappings;
	
	auto generateTextureIndex =
	[&](const std::string& path)
	{
		if (textureMappings.find(path) not_eq textureMappings.end())
			return textureMappings[path];
		size_t index = textureMappings.size();
		textureMappings[path] = index;
		return index;
	};
	
	std::vector<G2AssimpSceneMaterial> materials;
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		G2AssimpSceneMaterial outmaterial;
		aiMaterial* material = scene->mMaterials[i];
		
		outmaterial.name = NULL;
		
		aiString name;
		if(AI_SUCCESS == material->Get(AI_MATKEY_NAME, name))
			outmaterial.name = CloneString(name.C_Str());
		
		outmaterial.flags = 0;
		
		aiString path;
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			if(AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, & path))
			{
				outmaterial.flags |= G2AssimpSceneMaterialFlagHasDiffuse;
				outmaterial.diffuseTextureIndex = generateTextureIndex(std::string(path.C_Str()));
			}
		}
		
		if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
		{
			if(AI_SUCCESS == material->GetTexture(aiTextureType_NORMALS, 0, & path))
			{
				outmaterial.flags |= G2AssimpSceneMaterialFlagHasNormal;
				outmaterial.normalTextureIndex = generateTextureIndex(std::string(path.C_Str()));;
			}
		}
		
		materials.push_back(outmaterial);
	}
	
	std::vector<G2AssimpSceneEntity> entities;
	std::function<void(const aiNode& node, const aiMatrix4x4& parentTransform, size_t parentID)> processNode;
	
	processNode =
	[&](const aiNode& node, const aiMatrix4x4& parentTransform, size_t parentID)
	{
		G2AssimpSceneEntity self;
		aiMatrix4x4 transform = parentTransform * node.mTransformation;
		aiVector3D scale, position;
		aiQuaternion rotation;
		
		transform.Decompose(scale, rotation, position);
		
		self.position[0] = position.x;
		self.position[1] = position.y;
		self.position[2] = position.z;
		
		self.rotation[0] = rotation.x;
		self.rotation[1] = rotation.y;
		self.rotation[2] = rotation.z;
		self.rotation[3] = rotation.w;
		
		self.name = CloneString(node.mName.C_Str());
		self.parent = parentID;
		
		self.hasMesh = false;
		
		self.materialName = NULL;
		
		size_t selfID = entities.size();
		entities.push_back(self);
		
		if (node.mNumMeshes == 1)
		{
			self.hasMesh = true;
			self.mesh = node.mMeshes[0];
			aiString name;
			if(AI_SUCCESS == scene->mMaterials[scene->mMeshes[node.mMeshes[0]]->mMaterialIndex]->Get(AI_MATKEY_NAME, name))
				self.materialName = CloneString(name.C_Str());
		}
		else
		{
			for (size_t i = 0; i < node.mNumMeshes; i++)
			{
				G2AssimpSceneEntity selfSubmesh;
				
				selfSubmesh.position[0] = position.x;
				selfSubmesh.position[1] = position.y;
				selfSubmesh.position[2] = position.z;
		
				selfSubmesh.rotation[0] = rotation.x;
				selfSubmesh.rotation[1] = rotation.y;
				selfSubmesh.rotation[2] = rotation.z;
				selfSubmesh.rotation[3] = rotation.w;
				
				selfSubmesh.hasMesh = true;
				selfSubmesh.mesh = node.mMeshes[i];
				
				selfSubmesh.name = CloneString(node.mName.C_Str());
				selfSubmesh.parent = parentID;
				
				aiString name;
				if(AI_SUCCESS == scene->mMaterials[scene->mMeshes[node.mMeshes[i]]->mMaterialIndex]->Get(AI_MATKEY_NAME, name))
					selfSubmesh.materialName = CloneString(name.C_Str());
				
				entities.push_back(selfSubmesh);
			}
		}
		
		entities[selfID] = self;
		
		for (size_t i = 0; i < node.mNumChildren; i++)
		{
			processNode(*node.mChildren[i], transform, selfID);
		}
	};
	
	aiMatrix4x4 rootTransform;
	rootTransform.FromEulerAnglesXYZ(0, 0, 0);
	processNode(*scene->mRootNode, rootTransform, 0);
	outputScene->entityCount = entities.size();
	outputScene->entities = (G2AssimpSceneEntity*) memcpy(malloc(sizeof(G2AssimpSceneEntity) * outputScene->entityCount), &entities[0], sizeof(G2AssimpSceneEntity) * outputScene->entityCount);
	
	outputScene->meshCount = outputMeshes.size();
	outputScene->meshes = (G2AssimpSceneSkinnedMesh*) memcpy(malloc(sizeof(G2AssimpSceneSkinnedMesh) * outputScene->meshCount), & outputMeshes[0], sizeof(G2AssimpSceneSkinnedMesh) * outputScene->meshCount);
	
	outputScene->materialCount = materials.size();
	outputScene->materials = (G2AssimpSceneMaterial*) memcpy(malloc(sizeof(G2AssimpSceneMaterial) * outputScene->materialCount), & materials[0], sizeof(G2AssimpSceneMaterial) * outputScene->materialCount);
	
	outputScene->textureCount = textureMappings.size();
	outputScene->textures = (G2AssimpSceneTexture*) malloc(sizeof(G2AssimpSceneTexture) * outputScene->textureCount);
	for (auto i = textureMappings.begin(); i not_eq textureMappings.end(); i++)
	{
		std::string path = std::string(directory) + i->first;
		outputScene->textures[i->second].path = CloneString(path.c_str());
	}
	
	return outputScene;
}

PUBLIC_C_VISIBILITY
void G2AssimpSceneDelete(G2AssimpScene* self)
{
	if (not self) return;
	for (size_t i = 0; i < self->textureCount; i++)
	{
		free(self->textures[i].path);
	}
	free(self->textures);
	for (size_t i = 0; i < self->materialCount; i++)
	{
		free(self->materials[i].name);
	}
	free(self->materials);
	for (size_t i = 0; i < self->entityCount; i++)
	{
		free(self->entities[i].name);
		free(self->entities[i].materialName);
	}
	free(self->entities);
	for (size_t i = 0; i < self->meshCount; i++)
	{
		for (size_t j = 0; j < self->meshes[i].jointCount; j++)
		{
			free(self->meshes[i].joints[j].name);
		}
		free(self->meshes[i].joints);
		free(self->meshes[i].indices);
		free(self->meshes[i].name);
		free(self->meshes[i].vertices);
	}
	free(self->meshes);
	free(self);
}
