#include "Stage.hpp"
#include <algorithm>
#include "physfs/physfs.h"

GX::Stage::Stage(const char* workingDirectory) : workingDirectory(workingDirectory), painter(& renderer)
{
	
}

GX::Stage::~Stage(void)
{
	
}

uint16_t GX::Stage::acquireBundle(uint64_t permanentBundleID)
{
	uint16_t bundleIndex = loadedBundles.size();
	for (uint16_t i = 0; i < loadedBundles.size(); i++)
	{
		if (loadedBundles[i].referenceCount and loadedBundles[i].permanentBundleID == permanentBundleID)
		{
			loadedBundles[i].referenceCount++;
			return i;
		}
		bundleIndex = std::min(i, bundleIndex);
	}
	
	if (loadedBundles.size() == bundleIndex)
		loadedBundles.push_back(LoadedBundle());
	
	loadedBundles[bundleIndex].referenceCount++;
	loadedBundles[bundleIndex].permanentBundleID = permanentBundleID;
	
	//Do actual loading
	loadBundle(bundleIndex);
	
	return bundleIndex;
}

template<typename F, typename A>
static void DestroyOwnedItems(F deleter, A items)
{
	for (size_t i = 0; i < items->size(); i++)
		deleter((*items)[i]);
	items->clear();
}

void GX::Stage::releaseBundle(uint16_t bundleIndex)
{
	LoadedBundle* bundle = & loadedBundles[bundleIndex];
	bundle->referenceCount--;
	if (bundle->referenceCount)
		return;
	
	DestroyOwnedItems([&](uint16_t i){renderer.destroyVertexBuffer(i);}, & bundle->vertexBufferIndices);
	DestroyOwnedItems([&](uint16_t i){renderer.destroyIndexBuffer(i);}, & bundle->indexBufferIndices);
	DestroyOwnedItems([&](uint16_t i){renderer.destroyJointBuffer(i);}, & bundle->jointBufferIndices);
	//DestroyOwnedItems([&](uint16_t i){renderer.destroyInstanceBuffer(i);}, & bundle->instanceBufferIndices);
	DestroyOwnedItems([&](uint16_t i){renderer.destroyCluster(i);}, & bundle->clusterIndices);
	DestroyOwnedItems([&](uint16_t i){renderer.destroyMaterial(i);}, & bundle->materialIndices);
	DestroyOwnedItems([&](uint16_t i){renderer.destroyImage(i);}, & bundle->imageIndices);
	DestroyOwnedItems([&](uint16_t i){releaseBundle(i);}, & bundle->bundleIndices);
}
