#ifndef G2AssimpScene_h
#define G2AssimpScene_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

typedef struct G2AssimpSceneSkinnedMeshVertex G2AssimpSceneSkinnedMeshVertex;
struct G2AssimpSceneSkinnedMeshVertex
{
	float position[3], qtangent[4], jointWeights[4], texcoord[2];
	size_t joints[4];
};

typedef struct G2AssimpSceneSkinnedMeshJoint G2AssimpSceneSkinnedMeshJoint;
struct G2AssimpSceneSkinnedMeshJoint
{
	size_t parent;
	char* name;
	float position[3], rotation[4];
};

typedef struct G2AssimpSceneSkinnedMesh G2AssimpSceneSkinnedMesh;
struct G2AssimpSceneSkinnedMesh
{
	G2AssimpSceneSkinnedMeshVertex* vertices;
	G2AssimpSceneSkinnedMeshJoint* joints;
	size_t* indices;
	size_t vertexCount, jointCount, indexCount;
	char* name;
	size_t materialIndex;
};

typedef struct G2AssimpSceneEntity G2AssimpSceneEntity;
struct G2AssimpSceneEntity
{
	float position[3], rotation[4];
	bool hasMesh;
	size_t parent;
	size_t mesh;
	char* name;
	char* materialName;
};

typedef struct G2AssimpSceneTexture G2AssimpSceneTexture;
struct G2AssimpSceneTexture
{
	char* path;
};

enum
{
	G2AssimpSceneMaterialFlagHasDiffuse = 1 << 0,
	G2AssimpSceneMaterialFlagHasNormal = 1 << 1,
};

typedef uint32_t G2AssimpSceneMaterialFlags;

typedef struct G2AssimpSceneMaterial G2AssimpSceneMaterial;
struct G2AssimpSceneMaterial
{
	char* name;
	size_t
		diffuseTextureIndex,
		normalTextureIndex;
	G2AssimpSceneMaterialFlags flags;
};

typedef struct G2AssimpScene G2AssimpScene;
struct G2AssimpScene
{
	G2AssimpSceneEntity* entities;
	size_t entityCount;
	G2AssimpSceneSkinnedMesh* meshes;
	size_t meshCount;
	G2AssimpSceneTexture* textures;
	size_t textureCount;
	G2AssimpSceneMaterial* materials;
	size_t materialCount;
	//SparseImage* images;
	//size_t imageCount;
};

G2AssimpScene* G2AssimpSceneNewFromFile(const char* filename);
void G2AssimpSceneDelete(G2AssimpScene* self);

#ifdef __cplusplus
}//extern "C"
#endif

#endif