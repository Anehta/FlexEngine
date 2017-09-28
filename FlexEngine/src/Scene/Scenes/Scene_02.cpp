#include "stdafx.h"

#include "Scene/Scenes/Scene_02.h"

#include <glm/vec3.hpp>

namespace flex
{
	Scene_02::Scene_02(const GameContext& gameContext) :
		BaseScene("Scene_02")
	{
		UNREFERENCED_PARAMETER(gameContext);
	}

	Scene_02::~Scene_02()
	{
	}

	void Scene_02::Initialize(const GameContext& gameContext)
	{
		// Materials

		Renderer::MaterialCreateInfo skyboxMatInfo = {};
		skyboxMatInfo.name = "Skybox";
		skyboxMatInfo.shaderIndex = 4;

		const std::string directory = RESOURCE_LOCATION + "textures/skyboxes/ame_starfield/";
		const std::string fileName = "ame_starfield";
		const std::string extension = ".tga";

		skyboxMatInfo.cubeMapFilePaths = {
			directory + fileName + "_r" + extension,
			directory + fileName + "_l" + extension,
			directory + fileName + "_u" + extension,
			directory + fileName + "_d" + extension,
			directory + fileName + "_b" + extension,
			directory + fileName + "_f" + extension,
		};
		const MaterialID skyboxMatID = gameContext.renderer->InitializeMaterial(gameContext, &skyboxMatInfo);

		const int sphereCountX = 7;
		const int sphereCountY = 7;
		const float sphereSpacing = 2.5f;
		const glm::vec3 offset = glm::vec3(-sphereCountX / 2 * sphereSpacing, -sphereCountY / 2 * sphereSpacing, 0.0f);
		const size_t sphereCount = sphereCountX * sphereCountY;
		m_Spheres.resize(sphereCount);
		for (size_t i = 0; i < sphereCount; ++i)
		{
			int x = i % sphereCountX;
			int y = int(i / sphereCountY);

			const std::string iStr = std::to_string(i);

			Renderer::MaterialCreateInfo pbrMatInfo = {};
			pbrMatInfo.shaderIndex = 3;
			pbrMatInfo.name = "PBR simple " + iStr;
			pbrMatInfo.albedo = glm::vec3(0.1f, 0.1f, 0.5f);
			pbrMatInfo.metallic = float(x) / (sphereCountX - 1);
			pbrMatInfo.roughness = glm::clamp(float(y) / (sphereCountY - 1), 0.05f, 1.0f);
			pbrMatInfo.ao = 1.0f;
			const MaterialID pbrMatID = gameContext.renderer->InitializeMaterial(gameContext, &pbrMatInfo);


			m_Spheres[i] = new MeshPrefab(pbrMatID, "PBR sphere " + iStr);
			m_Spheres[i]->LoadFromFile(gameContext, RESOURCE_LOCATION + "models/sphere.fbx", true, true);
			m_Spheres[i]->GetTransform().position = offset + glm::vec3(x * sphereSpacing, y * sphereSpacing, 0.0f);
			AddChild(m_Spheres[i]);
		}


		m_Skybox = new MeshPrefab(skyboxMatID);
		m_Skybox->LoadPrefabShape(gameContext, MeshPrefab::PrefabShape::SKYBOX);
		AddChild(m_Skybox);

		// Lights
		Renderer::PointLight light1 = {};
		light1.position = glm::vec4(-6.0f, -6.0f, -8.0f, 0.0f);
		light1.color = glm::vec4(300.0f, 300.0f, 300.0f, 0.0f);
		gameContext.renderer->InitializePointLight(light1);

		Renderer::PointLight light2 = {};
		light2.position = glm::vec4(-6.0f, 6.0f, -8.0f, 0.0f);
		light2.color = glm::vec4(300.0f, 300.0f, 300.0f, 0.0f);
		gameContext.renderer->InitializePointLight(light2);

		Renderer::PointLight light3 = {};
		light3.position = glm::vec4(6.0f, 6.0f, -8.0f, 0.0f);
		light3.color = glm::vec4(300.0f, 300.0f, 300.0f, 0.0f);
		gameContext.renderer->InitializePointLight(light3);

		Renderer::PointLight light4 = {};
		light4.position = glm::vec4(6.0f, -6.0f, -8.0f, 0.0f);
		light4.color = glm::vec4(300.0f, 300.0f, 300.0f, 0.0f);
		gameContext.renderer->InitializePointLight(light4);

	}

	void Scene_02::Destroy(const GameContext& gameContext)
	{
		UNREFERENCED_PARAMETER(gameContext);
	}

	void Scene_02::Update(const GameContext& gameContext)
	{
		UNREFERENCED_PARAMETER(gameContext);
	}
} // namespace flex
