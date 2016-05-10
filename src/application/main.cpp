#include <string>
#include "../Renderer/Renderer.hpp"
#include "../Renderer/Stage.hpp"
#include "../Core/G2AssimpScene.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"
#include "Painting/Painter.hpp"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <iostream>
#include <sstream>

#include <map>
#include <string>
#include "physfs/physfs.h"

static uint16_t LoadImage(GX::Renderer* renderer, const char* path)
{
	int width, height, components;
	unsigned char* pixels = stbi_load(path, & width, & height, & components, 4);
	//assert(components == 3);
	assert(pixels);
	uint16_t imageIndex = renderer->createImage(GX::Renderer::PixelType::R8G8B8A8, width, height, pixels);
	stbi_image_free(pixels);
	
	return imageIndex;
}

static void LoadScene(GX::Renderer* renderer, const char* path, const std::map<std::string, uint16_t>& materialIndices)
{
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
		uint16_t materialIndex = materialIndices.at(i->first.second);
		assert (materialIndices.find(i->first.second) not_eq materialIndices.cend());
		
		renderer->createStaticMeshRendererCluster(meshes[i->first.first].vertexBufferIndex, meshes[i->first.first].indexBufferIndex, materialIndex, i->second.size(), & i->second[0]);
		//renderer->createCluster(cluster);
		//renderer->createClusterBuffer(1, & cluster);
	}
}


static void OnFramebufferResize(GLFWwindow* window, int width, int height)
{
	if (width <= 0 or height <= 0)
		return;
	GX::Renderer* renderer = (GX::Renderer*) glfwGetWindowUserPointer(window);
	renderer->setMainViewResolution(width, height);
}

static void OnKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/*if (key == GLFW_KEY_F)
	{
		Renderer* renderer = (Renderer*) glfwGetWindowUserPointer(window);
		renderer->saveScreenshot();
	}*/
}

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		//std::cerr << "Error: Missing argument string (must provide a scene file)." << std::endl;
		return EXIT_FAILURE;
	}

	if(not glfwInit())
	{
		std::cerr << "Failed to initialize glfw" << std::endl;
		return EXIT_FAILURE;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_SRGB_CAPABLE, true);

	//glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 0);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_STENCIL_BITS, 8);
	
	glfwWindowHint(GLFW_SAMPLES, 1);
	glfwWindowHint(GLFW_REFRESH_RATE, 30);

	//int width = 800 * 1.5;
	//int height = 500 * 1.5;
	int width = 1920;
	int height = 1080;
	GLFWwindow* window = glfwCreateWindow(width, height, "GX", glfwGetPrimaryMonitor(), NULL);

	if(not window)
	{
		glfwTerminate();
		std::cerr << "Failed to create window" << std::endl;
		return 1;
	}

	glfwMakeContextCurrent(window);
	
	glfwGetWindowSize(window, & width, & height);

	//glfwSwapInterval(1);
	
	PHYSFS_init(argv[0]);

	//GX::Stage* stage = new GX::Stage("/Users/Oliver/Documents/Dev/Projects/GameEngines/GX/data");
	GX::Stage* stage = new GX::Stage("data");
	stage->acquireBundle(0);
	
	//GX::Painting::Painter painter(& stage->renderer);

	//GX::Renderer* renderer = new GX::Renderer();
	
	/*std::map<std::string, uint16_t> materialIndices;
	{
		//std::string materialName("Material");
		constexpr size_t nodeCount = 5;
		auto baseColorImageIndex = LoadImage(renderer, "/Users/Oliver/Documents/Dev/Projects/Games/G4_copy/BrickLargeBlocks0026_1_S.jpg");
		auto normalImageIndex = LoadImage(renderer, "/Users/Oliver/Documents/Dev/Projects/Games/G4_copy/BrickLargeBlocks0026_1_S normalmap.png");
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
		auto materialIndex = renderer->createMaterial(nodeCount, nodes);
		materialIndices[std::string("DefaultMaterial")] = materialIndex;
		materialIndices[std::string("Material")] = materialIndex;
		materialIndices[std::string("Material.003")] = materialIndex;
		materialIndices[std::string("Material.002")] = materialIndex;
		materialIndices[std::string("Material.001")] = materialIndex;
	}
	LoadScene(renderer, "/Users/Oliver/Documents/Dev/Projects/Games/G4/Test4_2.obj", materialIndices);*/
	
	glfwSetWindowUserPointer(window, & stage->renderer);
	glfwSetKeyCallback(window, OnKeyPress);

	stage->renderer.setMainViewResolution(width, height);
	//stage->renderer.setMainViewPerspective(0.1, 100, glm::radians(50.0f));
	stage->renderer.setMainViewPerspective(0.1, 100, glm::radians(60.0f));
	//stage->renderer.setMainViewPerspective(1, 100, 45);

	glfwSetFramebufferSizeCallback(window, OnFramebufferResize);
	
	//glm::vec3 cameraPosition = glm::vec3(0, 0, 0);
	//glm::quat cameraRotation = glm::quat(1, 0, 0, 0);
	//glm::vec3 cameraPosition = glm::vec3(-1.818933, 2.185727, -2.936637);
	//glm::quat cameraRotation = glm::quat(-0.0486068, -0.00289908, -0.997042, 0.0594671);

	//glm::vec3 cameraPosition = glm::vec3(-2.308295, 2.711969, -1.711241);
	//glm::quat cameraRotation = glm::quat(0.00377781, 2.87863e-06, -0.999993, 0.000761978);
	glm::vec3 cameraPosition = glm::vec3(-2.307039, 2.691374, -0.862112);
	//glm::quat cameraRotation = glm::quat(0.0366599, 0.00364387, -0.994421, 0.0988422);
	glm::quat cameraRotation = glm::quat(0.0334263, 0.00320566, -0.994872, 0.0954104);
	
	constexpr int cameraOrientationCount = 6;
	int cameraOrientationIndex = 0;
	constexpr float switchTimeInterval = 60;
	float switchTimeLeft = switchTimeInterval;
	
	glm::vec3 cameraPositions[cameraOrientationCount] = 
	{
		cameraPosition,
		glm::vec3(-8.025851, 13.865633, 10.595398),
		glm::vec3(-12.645594, 5.720737, 0.857288),
		glm::vec3(1.799470, 13.471046, -0.768554),
		glm::vec3(-12.138775, 8.784929, 39.195065),
		glm::vec3(-11.999206, 2.358982, 29.945190)
	};
	
	glm::quat cameraRotations[cameraOrientationCount] = 
	{
		cameraRotation,
		glm::quat(0.761719, -0.238913, -0.574646, -0.180238),
		glm::quat(0.432295, -0.0421686, -0.896491, -0.0874491),
		glm::quat(-0.251298, 0.0468328, -0.950412, -0.177122),
		glm::quat(0.885282, -0.284508, -0.350231, -0.112556),
		glm::quat(0.754781, 0.0621451, -0.650825, 0.0535859)
	};

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, 0, 0);
	
	double then = glfwGetTime();
	
	glfwSwapInterval(1);
	
	float frameCount = 0;
	float timeCount = 0;

	while(not glfwWindowShouldClose(window))
	{
		double now = glfwGetTime();
		double secondsElapsed = now - then;
		then = now;
		
		if (timeCount > 0.25)
		{
			float spf = timeCount / frameCount;
			float fps = frameCount / timeCount;
			std::stringstream titleStream;
			titleStream << "GX (FPS: " << (1 / secondsElapsed) << ", SPF: " << secondsElapsed << ")";
			glfwSetWindowTitle(window, titleStream.str().c_str());
			
			timeCount = 0;
			frameCount = 0;
		}
		
		frameCount += 1;
		timeCount += secondsElapsed;
		
		switchTimeLeft -= secondsElapsed;
		
		if (switchTimeLeft <= 0)
		{
			switchTimeLeft = switchTimeInterval;
			cameraOrientationIndex++;
			cameraOrientationIndex %= cameraOrientationCount;
			cameraPosition = cameraPositions[cameraOrientationIndex];
			cameraRotation = cameraRotations[cameraOrientationIndex];
		}

		/*{
			double x, y;
			glfwGetCursorPos(window, & x, & y);
			float angleX = -glm::clamp(float(y * 0.1), -90.0f, 90.0f);
			float angleY = -x * 0.1;
			cameraRotation =
				glm::angleAxis(glm::radians(angleY), glm::vec3(0, 1, 0)) *
				glm::angleAxis(glm::radians(angleX), glm::vec3(1, 0, 0)) *
				glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
			cameraRotation = glm::normalize(cameraRotation);
			
			float dz = glfwGetKey(window, GLFW_KEY_S) - glfwGetKey(window, GLFW_KEY_W);
			float dx = glfwGetKey(window, GLFW_KEY_A) - glfwGetKey(window, GLFW_KEY_D);
			glm::vec3 delta = glm::vec3(-dx, 0, dz);
			cameraPosition += cameraRotation * (delta * float(secondsElapsed * 10));
		}*/
		
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			//glfwSetCursorPos(window, 0, 0);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			//glfwSetCursorPos(window, 0, 0);
		}

		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			double x, y;
			glfwGetCursorPos(window, & x, & y);
			float angleX = -glm::clamp(float(y * 0.1), -90.0f, 90.0f);
			float angleY = -x * 0.1;
			cameraRotation =
				glm::angleAxis(glm::radians(angleY), glm::vec3(0, 1, 0)) *
				glm::angleAxis(glm::radians(angleX), glm::vec3(1, 0, 0)) *
				glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 0, 1));
			cameraRotation = glm::normalize(cameraRotation);
		}
		
		{
			float dz = glfwGetKey(window, GLFW_KEY_S) - glfwGetKey(window, GLFW_KEY_W);
			float dx = glfwGetKey(window, GLFW_KEY_A) - glfwGetKey(window, GLFW_KEY_D);
			glm::vec3 delta = glm::vec3(-dx, 0, dz);
			cameraPosition += cameraRotation * (delta * float(secondsElapsed * 10));
		}
		
		if (glfwGetKey(window, GLFW_KEY_E))
		{
			std::cout << "cameraPosition " << glm::to_string(cameraPosition) << std::endl;
			std::cout << "cameraRotation x: " << cameraRotation.x << " y: " << cameraRotation.y << " z: " << cameraRotation.z << " w: " << cameraRotation.w << std::endl;
		}
		
		stage->renderer.setMainViewPosition(cameraPosition);
		stage->renderer.setMainViewRotation(cameraRotation);

		stage->renderer.update(secondsElapsed);
		stage->renderer.render();
		
		glFinish();
		
		stage->painter.update(secondsElapsed);
		
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
		{
			double x, y;
			glfwGetCursorPos(window, & x, & y);
			stage->painter.lastTouchPosition = glm::vec2(x, height - y);
			glm::vec3 point = stage->renderer.unproject(x, height - y);
			GX::Painting::Painter::BrushSettings settings;
			stage->painter.paint(point, settings);
			//painter.paint(point, settings);
			GX::Renderer::Brush brush;
			brush.at = point;
			brush.radius = 1;
			brush.color = glm::vec3(0, 0, 0);
			//brush.type = GX::Renderer::Brush::Type::None;
			//stage->renderer.paint(brush);
		}

		//glFlush();
		//glFinish();

		glfwSwapBuffers(window);
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) break;
	}
	
	//delete renderer;
	delete stage;

	PHYSFS_deinit();

	glfwTerminate();
	return 0;
}

