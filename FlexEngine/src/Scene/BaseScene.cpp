#include "stdafx.hpp"

#include "Scene/BaseScene.hpp"

IGNORE_WARNINGS_PUSH
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

#include <glm/gtx/norm.hpp> // for distance2
IGNORE_WARNINGS_POP

#include "Audio/AudioManager.hpp"
#include "Callbacks/GameObjectCallbacks.hpp"
#include "Cameras/BaseCamera.hpp"
#include "Cameras/CameraManager.hpp"
#include "FlexEngine.hpp"
#include "Graphics/Renderer.hpp"
#include "Helpers.hpp"
#include "InputManager.hpp"
#include "JSONParser.hpp"
#include "Physics/PhysicsHelpers.hpp"
#include "Physics/PhysicsManager.hpp"
#include "Physics/PhysicsWorld.hpp"
#include "Physics/RigidBody.hpp"
#include "Player.hpp"
#include "PlayerController.hpp"
#include "Profiler.hpp"
#include "Platform/Platform.hpp"
#include "Scene/GameObject.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/MeshComponent.hpp"
#include "Scene/SceneManager.hpp"
#include "Track/BezierCurve.hpp"

namespace flex
{
	std::vector<JSONObject> BaseScene::s_ParsedMaterials;
	std::vector<JSONObject> BaseScene::s_ParsedMeshes;
	std::vector<JSONObject> BaseScene::s_ParsedPrefabs;

	BaseScene::BaseScene(const std::string& fileName) :
		m_FileName(fileName),
		m_TrackManager(this),
		m_CartManager(this)
	{
		// Default day sky
		m_SkyboxData = {};
		m_SkyboxData.top = glm::pow(glm::vec4(0.22f, 0.58f, 0.88f, 0.0f), glm::vec4(2.2f));
		m_SkyboxData.mid = glm::pow(glm::vec4(0.66f, 0.86f, 0.95f, 0.0f), glm::vec4(2.2f));
		m_SkyboxData.btm = glm::pow(glm::vec4(0.75f, 0.91f, 0.99f, 0.0f), glm::vec4(2.2f));
	}

	BaseScene::~BaseScene()
	{
	}

	void BaseScene::Initialize()
	{
		if (!m_bInitialized)
		{
			if (m_PhysicsWorld)
			{
				PrintWarn("Scene is being initialized more than once!\n");
			}

			// Physics world
			m_PhysicsWorld = new PhysicsWorld();
			m_PhysicsWorld->Initialize();

			m_PhysicsWorld->GetWorld()->setGravity({ 0.0f, -9.81f, 0.0f });

			m_CartManager.Initialize();

			// Use save file if exists, otherwise use default
			//const std::string savedShortPath = SCENE_SAVED_LOCATION + m_FileName;
			const std::string defaultPath = SCENE_DEFAULT_LOCATION + m_FileName;
			const std::string defaultShortPath = StripLeadingDirectories(defaultPath);
			//const std::string savedPath = RESOURCE_LOCATION + savedShortPath;
			//m_bUsingSaveFile = FileExists(savedPath);

			std::string shortFilePath;
			std::string filePath;
			//if (m_bUsingSaveFile)
			//{
			//	shortFilePath = savedShortPath;
			//	filePath = savedPath;
			//}
			//else
			{
				shortFilePath = defaultShortPath;
				filePath = defaultPath;
			}

			if (FileExists(filePath))
			{
				if (g_bEnableLogging_Loading)
				{
					Print("Loading scene from %s\n", shortFilePath.c_str());
				}

				JSONObject sceneRootObject;
				if (!JSONParser::ParseFromFile(filePath, sceneRootObject))
				{
					PrintError("Failed to parse scene file: %s\n\terror: %s\n", shortFilePath.c_str(), JSONParser::GetErrorString());
					return;
				}

				constexpr bool printSceneContentsToConsole = false;
				if (printSceneContentsToConsole)
				{
					Print("Parsed scene file:\n");
					PrintLong(sceneRootObject.Print(0).c_str());
				}

				if (!sceneRootObject.SetIntChecked("version", m_SceneFileVersion))
				{
					m_SceneFileVersion = 1;
					PrintError("Scene version missing from scene file. Assuming version 1\n");
				}

				sceneRootObject.SetStringChecked("name", m_Name);

				sceneRootObject.SetBoolChecked("spawn player", m_bSpawnPlayer);

				JSONObject skyboxDataObj;
				if (sceneRootObject.SetObjectChecked("skybox data", skyboxDataObj))
				{
					// TODO: Add SetGammaColourChecked
					if (skyboxDataObj.SetVec4Checked("top colour", m_SkyboxData.top))
					{
						m_SkyboxData.top = glm::pow(m_SkyboxData.top, glm::vec4(2.2f));
					}
					if (skyboxDataObj.SetVec4Checked("mid colour", m_SkyboxData.mid))
					{
						m_SkyboxData.mid = glm::pow(m_SkyboxData.mid, glm::vec4(2.2f));
					}
					if (skyboxDataObj.SetVec4Checked("btm colour", m_SkyboxData.btm))
					{
						m_SkyboxData.btm = glm::pow(m_SkyboxData.btm, glm::vec4(2.2f));
					}
				}

				JSONObject cameraObj;
				if (sceneRootObject.SetObjectChecked("camera", cameraObj))
				{
					std::string camType;
					if (cameraObj.SetStringChecked("type", camType))
					{
						if (camType.compare("terminal") == 0)
						{
							g_CameraManager->PushCameraByName(camType, true, false);

						}
						else
						{
							g_CameraManager->SetCameraByName(camType, true);
						}
					}

					BaseCamera* cam = g_CameraManager->CurrentCamera();

					real zNear, zFar, fov, aperture, shutterSpeed, lightSensitivity, exposure, moveSpeed;
					if (cameraObj.SetFloatChecked("near plane", zNear)) cam->zNear = zNear;
					if (cameraObj.SetFloatChecked("far plane", zFar)) cam->zFar = zFar;
					if (cameraObj.SetFloatChecked("fov", fov)) cam->FOV = fov;
					if (cameraObj.SetFloatChecked("aperture", aperture)) cam->aperture = aperture;
					if (cameraObj.SetFloatChecked("shutter speed", shutterSpeed)) cam->shutterSpeed = shutterSpeed;
					if (cameraObj.SetFloatChecked("light sensitivity", lightSensitivity)) cam->lightSensitivity = lightSensitivity;
					if (cameraObj.SetFloatChecked("exposure", exposure)) cam->exposure = exposure;
					if (cameraObj.SetFloatChecked("move speed", moveSpeed)) cam->moveSpeed = moveSpeed;

					if (cameraObj.HasField("aperture"))
					{
						cam->aperture = cameraObj.GetFloat("aperture");
						cam->shutterSpeed = cameraObj.GetFloat("shutter speed");
						cam->lightSensitivity = cameraObj.GetFloat("light sensitivity");
					}

					JSONObject cameraTransform;
					if (cameraObj.SetObjectChecked("transform", cameraTransform))
					{
						glm::vec3 camPos = ParseVec3(cameraTransform.GetString("position"));
						if (IsNanOrInf(camPos))
						{
							PrintError("Camera pos was saved out as nan or inf, resetting to 0\n");
							camPos = VEC3_ZERO;
						}
						cam->position = camPos;

						real camPitch = cameraTransform.GetFloat("pitch");
						if (IsNanOrInf(camPitch))
						{
							PrintError("Camera pitch was saved out as nan or inf, resetting to 0\n");
							camPitch = 0.0f;
						}
						cam->pitch = camPitch;

						real camYaw = cameraTransform.GetFloat("yaw");
						if (IsNanOrInf(camYaw))
						{
							PrintError("Camera yaw was saved out as nan or inf, resetting to 0\n");
							camYaw = 0.0f;
						}
						cam->yaw = camYaw;
					}

					cam->CalculateExposure();
				}

				// TODO: Only initialize materials currently present in this scene
				for (JSONObject& materialObj : s_ParsedMaterials)
				{
					MaterialCreateInfo matCreateInfo = {};
					Material::ParseJSONObject(materialObj, matCreateInfo);

					MaterialID matID = g_Renderer->InitializeMaterial(&matCreateInfo);
					m_LoadedMaterials.push_back(matID);
				}

				// This holds all the objects in the scene which do not have a parent
				const std::vector<JSONObject>& rootObjects = sceneRootObject.GetObjectArray("objects");
				for (const JSONObject& rootObjectJSON : rootObjects)
				{
					GameObject* rootObj = GameObject::CreateObjectFromJSON(rootObjectJSON, this, m_SceneFileVersion);
					rootObj->GetTransform()->UpdateParentTransform();
					AddRootObject(rootObj);
				}

				if (sceneRootObject.HasField("track manager"))
				{
					const JSONObject& trackManagerObj = sceneRootObject.GetObject("track manager");

					m_TrackManager.InitializeFromJSON(trackManagerObj);
				}
			}
			else
			{
				// File doesn't exist, create a new blank one
				Print("Creating new scene at: %s\n", shortFilePath.c_str());

				// Skybox
				{
					//MaterialCreateInfo skyboxMatCreateInfo = {};
					//skyboxMatCreateInfo.name = "Skybox";
					//skyboxMatCreateInfo.shaderName = "skybox";
					//skyboxMatCreateInfo.generateHDRCubemapSampler = true;
					//skyboxMatCreateInfo.enableCubemapSampler = true;
					//skyboxMatCreateInfo.enableCubemapTrilinearFiltering = true;
					//skyboxMatCreateInfo.generatedCubemapSize = glm::vec2(512.0f);
					//skyboxMatCreateInfo.generateIrradianceSampler = true;
					//skyboxMatCreateInfo.generatedIrradianceCubemapSize = glm::vec2(32.0f);
					//skyboxMatCreateInfo.generatePrefilteredMap = true;
					//skyboxMatCreateInfo.generatedPrefilteredCubemapSize = glm::vec2(128.0f);
					//skyboxMatCreateInfo.environmentMapPath = TEXTURE_LOCATION "hdri/Milkyway/Milkyway_Light.hdr";
					//MaterialID skyboxMatID = g_Renderer->InitializeMaterial(&skyboxMatCreateInfo);

					//m_LoadedMaterials.push_back(skyboxMatID);

					MaterialID skyboxMatID = InvalidMaterialID;
					g_Renderer->FindOrCreateMaterialByName("skybox 01", skyboxMatID);
					assert(skyboxMatID != InvalidMaterialID);

					Skybox* skybox = new Skybox("Skybox");
					skybox->ProcedurallyInitialize(skyboxMatID);

					AddRootObject(skybox);
				}

				MaterialID sphereMatID = InvalidMaterialID;
				g_Renderer->FindOrCreateMaterialByName("pbr chrome", sphereMatID);

				assert(sphereMatID != InvalidMaterialID);

				// Reflection probe
				{
					//MaterialCreateInfo reflectionProbeMatCreateInfo = {};
					//reflectionProbeMatCreateInfo.name = "Reflection Probe";
					//reflectionProbeMatCreateInfo.shaderName = "pbr";
					//reflectionProbeMatCreateInfo.constAlbedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
					//reflectionProbeMatCreateInfo.constMetallic = 1.0f;
					//reflectionProbeMatCreateInfo.constRoughness = 0.0f;
					//MaterialID sphereMatID = g_Renderer->InitializeMaterial(&reflectionProbeMatCreateInfo);

					//m_LoadedMaterials.push_back(sphereMatID);

					ReflectionProbe* reflectionProbe = new ReflectionProbe("Reflection Probe 01");

					JSONObject emptyObj = {};
					reflectionProbe->ParseJSON(emptyObj, this, sphereMatID);
					reflectionProbe->GetTransform()->SetLocalPosition(glm::vec3(0.0f, 8.0f, 0.0f));

					AddRootObject(reflectionProbe);
				}

				GameObject* sphere = new GameObject("sphere", GameObjectType::OBJECT);
				Mesh* mesh = sphere->SetMesh(new Mesh(sphere));
				mesh->LoadFromFile(MESH_DIRECTORY "ico-sphere.glb", sphereMatID);
				AddRootObject(sphere);

				// Default directional light
				DirectionalLight* dirLight = new DirectionalLight();
				g_SceneManager->CurrentScene()->AddRootObject(dirLight);
				dirLight->SetRot(glm::quat(glm::vec3(130.0f, -65.0f, 120.0f)));
				dirLight->SetPos(glm::vec3(0.0f, 15.0f, 0.0f));
				dirLight->data.brightness = 5.0f;
				dirLight->Initialize();
				dirLight->PostInitialize();
			}

			if (m_bSpawnPlayer)
			{
				m_Player0 = new Player(0, glm::vec3(0.0f, 2.0f, 0.0f));
				AddRootObject(m_Player0);

				//m_Player1 = new Player(1);
				//AddRootObject(m_Player1);
			}

			for (GameObject* rootObject : m_RootObjects)
			{
				rootObject->Initialize();
			}

			UpdateRootObjectSiblingIndices();

			m_bInitialized = true;

			m_bLoaded = true;
		}

		// All updating to new file version should be complete by this point
		m_SceneFileVersion = LATEST_SCENE_FILE_VERSION;
		m_MaterialsFileVersion = LATEST_MATERIALS_FILE_VERSION;
		m_MeshesFileVersion = LATEST_MESHES_FILE_VERSION;
	}

	void BaseScene::PostInitialize()
	{
		for (GameObject* rootObject : m_RootObjects)
		{
			rootObject->PostInitialize();
		}

		m_PhysicsWorld->GetWorld()->setDebugDrawer(g_Renderer->GetDebugDrawer());
	}

	void BaseScene::Destroy()
	{
		m_bLoaded = false;
		m_bInitialized = false;

		for (GameObject* rootObject : m_RootObjects)
		{
			if (rootObject)
			{
				rootObject->Destroy();
				delete rootObject;
			}
		}
		m_RootObjects.clear();

		m_TrackManager.Destroy();
		m_CartManager.Destroy();

		m_LoadedMaterials.clear();

		g_Renderer->ClearMaterials();
		g_Renderer->SetSkyboxMesh(nullptr);
		g_Renderer->RemoveDirectionalLight();
		g_Renderer->RemoveAllPointLights();

		if (m_PhysicsWorld)
		{
			m_PhysicsWorld->Destroy();
			delete m_PhysicsWorld;
			m_PhysicsWorld = nullptr;
		}

		m_Player0 = nullptr;
		m_Player1 = nullptr;
	}

	void BaseScene::Update()
	{
		PROFILE_BEGIN("Update Scene");

		if (m_PhysicsWorld)
		{
			m_PhysicsWorld->Update(g_DeltaTime);
		}

		m_CartManager.Update();
		if (g_InputManager->GetKeyPressed(KeyCode::KEY_Z))
		{
			AudioManager::PlaySource(FlexEngine::GetAudioSourceID(FlexEngine::SoundEffect::dud_dud_dud_dud));
		}
		if (g_InputManager->GetKeyPressed(KeyCode::KEY_X))
		{
			AudioManager::PauseSource(FlexEngine::GetAudioSourceID(FlexEngine::SoundEffect::dud_dud_dud_dud));
		}

		{
			PROFILE_AUTO("Tick scene objects");
			for (GameObject* rootObject : m_RootObjects)
			{
				if (rootObject)
				{
					rootObject->Update();
				}
			}
		}

		m_TrackManager.DrawDebug();

		PROFILE_END("Update Scene");
	}

	void BaseScene::LateUpdate()
	{
		if (!m_ObjectsToDestroyAtEndOfFrame.empty())
		{
			for (GameObject* gameObject : m_ObjectsToDestroyAtEndOfFrame)
			{
				if (!DestroyGameObject(gameObject, true))
				{
					PrintWarn("Attempted to remove game object from scene which doesn't exist! %s\n", gameObject->GetName().c_str());
				}
			}
			m_ObjectsToDestroyAtEndOfFrame.clear();
			UpdateRootObjectSiblingIndices();
			g_Renderer->RenderObjectStateChanged();
		}

		if (!m_ObjectsToAddAtEndOfFrame.empty())
		{
			for (GameObject* gameObject : m_ObjectsToAddAtEndOfFrame)
			{
#if THOROUGH_CHECKS
				if (std::find(m_RootObjects.begin(), m_RootObjects.end(), gameObject) != m_RootObjects.end())
				{
					PrintWarn("Attempted to add root object multiple times!\n");
					continue;
				}
#endif

				m_RootObjects.push_back(gameObject);
				gameObject->Initialize();
				gameObject->PostInitialize();
			}
			m_ObjectsToAddAtEndOfFrame.clear();
			UpdateRootObjectSiblingIndices();
			g_Renderer->RenderObjectStateChanged();
		}
	}

	void BaseScene::DrawImGuiObjects()
	{
		ImGui::Checkbox("Spawn player", &m_bSpawnPlayer);

		ImGuiExt::ColorEdit3Gamma("Top", &m_SkyboxData.top.x);
		ImGuiExt::ColorEdit3Gamma("Mid", &m_SkyboxData.mid.x);
		ImGuiExt::ColorEdit3Gamma("Bottom", &m_SkyboxData.btm.x);

		DoSceneContextMenu();
	}

	void BaseScene::DoSceneContextMenu()
	{
		bool bClicked = ImGui::IsMouseReleased(1) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

		std::string contextMenuID = "scene context menu " + m_FileName;
		if (ImGui::BeginPopupContextItem(contextMenuID.c_str()))
		{
			{
				const i32 sceneNameMaxCharCount = 256;

				// We don't know the names of scene's that haven't been loaded
				if (m_bLoaded)
				{
					static char newSceneName[sceneNameMaxCharCount];
					if (bClicked)
					{
						strcpy(newSceneName, m_Name.c_str());
					}

					bool bRenameScene = ImGui::InputText("##rename-scene",
						newSceneName,
						sceneNameMaxCharCount,
						ImGuiInputTextFlags_EnterReturnsTrue);

					ImGui::SameLine();

					bRenameScene |= ImGui::Button("Rename scene");

					if (bRenameScene)
					{
						SetName(newSceneName);
						// Don't close popup here since we will likely want to save that change
					}
				}

				static char newSceneFileName[sceneNameMaxCharCount];
				if (bClicked)
				{
					strcpy(newSceneFileName, m_FileName.c_str());
				}

				bool bRenameSceneFileName = ImGui::InputText("##rename-scene-file-name",
					newSceneFileName,
					sceneNameMaxCharCount,
					ImGuiInputTextFlags_EnterReturnsTrue);

				ImGui::SameLine();

				bRenameSceneFileName |= ImGui::Button("Rename file");

				if (bRenameSceneFileName)
				{
					std::string newSceneFileNameStr(newSceneFileName);
					std::string fileDir = ExtractDirectoryString(RelativePathToAbsolute(GetDefaultRelativeFilePath()));
					std::string newSceneFilePath = fileDir + newSceneFileNameStr;
					bool bNameEmpty = newSceneFileNameStr.empty();
					bool bCorrectFileType = EndsWith(newSceneFileNameStr, ".json");
					bool bFileExists = FileExists(newSceneFilePath);
					bool bSceneNameValid = (!bNameEmpty && bCorrectFileType && !bFileExists);

					if (bSceneNameValid)
					{
						if (SetFileName(newSceneFileNameStr, true))
						{
							ImGui::CloseCurrentPopup();
						}
					}
					else
					{
						PrintError("Attempted name scene with invalid name: %s\n", newSceneFileNameStr.c_str());
						if (bNameEmpty)
						{
							PrintError("(file name is empty!)\n");
						}
						else if (!bCorrectFileType)
						{
							PrintError("(must end with \".json\"!)\n");
						}
						else if (bFileExists)
						{
							PrintError("(file already exists!)\n");
						}
					}
				}
			}

			// Only allow current scene to be saved
			if (g_SceneManager->CurrentScene() == this)
			{
				if (ImGui::Button("Save"))
				{
					SerializeToFile(false);

					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				ImGui::PushStyleColor(ImGuiCol_Button, g_WarningButtonColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_WarningButtonHoveredColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, g_WarningButtonActiveColor);

				if (ImGui::Button("Save over default"))
				{
					SerializeToFile(true);

					ImGui::CloseCurrentPopup();
				}

				if (IsUsingSaveFile())
				{
					ImGui::SameLine();

					if (ImGui::Button("Hard reload (deletes save file!)"))
					{
						Platform::DeleteFile(GetRelativeFilePath());
						g_SceneManager->ReloadCurrentScene();

						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
			}

			static const char* duplicateScenePopupLabel = "Duplicate scene";
			const i32 sceneNameMaxCharCount = 256;
			static char newSceneName[sceneNameMaxCharCount];
			static char newSceneFileName[sceneNameMaxCharCount];
			if (ImGui::Button("Duplicate..."))
			{
				ImGui::OpenPopup(duplicateScenePopupLabel);

				std::string newSceneNameStr = m_Name;
				newSceneNameStr += " Copy";
				strcpy(newSceneName, newSceneNameStr.c_str());

				std::string newSceneFileNameStr = StripFileType(m_FileName);

				bool bValidName = false;
				do
				{
					i16 numNumericalChars = 0;
					i32 numEndingWith = GetNumberEndingWith(newSceneFileNameStr, numNumericalChars);
					if (numNumericalChars > 0)
					{
						u32 charsBeforeNum = (u32)(newSceneFileNameStr.length() - numNumericalChars);
						newSceneFileNameStr = newSceneFileNameStr.substr(0, charsBeforeNum) +
							IntToString(numEndingWith + 1, numNumericalChars);
					}
					else
					{
						newSceneFileNameStr += "_01";
					}

					std::string filePathFrom = RelativePathToAbsolute(GetDefaultRelativeFilePath());
					std::string fullNewFilePath = ExtractDirectoryString(filePathFrom);
					fullNewFilePath += newSceneFileNameStr + ".json";
					bValidName = !FileExists(fullNewFilePath);
				} while (!bValidName);

				newSceneFileNameStr += ".json";

				strcpy(newSceneFileName, newSceneFileNameStr.c_str());
			}

			bool bCloseContextMenu = false;
			if (ImGui::BeginPopupModal(duplicateScenePopupLabel,
				NULL,
				ImGuiWindowFlags_AlwaysAutoResize))
			{

				bool bDuplicateScene = ImGui::InputText("Name##duplicate-scene-name",
					newSceneName,
					sceneNameMaxCharCount,
					ImGuiInputTextFlags_EnterReturnsTrue);

				bDuplicateScene |= ImGui::InputText("File name##duplicate-scene-file-path",
					newSceneFileName,
					sceneNameMaxCharCount,
					ImGuiInputTextFlags_EnterReturnsTrue);

				bDuplicateScene |= ImGui::Button("Duplicate");

				bool bValidInput = true;

				if (strlen(newSceneName) == 0 ||
					strlen(newSceneFileName) == 0 ||
					!EndsWith(newSceneFileName, ".json"))
				{
					bValidInput = false;
				}

				if (bDuplicateScene && bValidInput)
				{
					if (g_SceneManager->DuplicateScene(this, newSceneFileName, newSceneName))
					{
						bCloseContextMenu = true;
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				if (g_InputManager->GetKeyPressed(KeyCode::KEY_ESCAPE, true))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Open in explorer"))
			{
				const std::string directory = RelativePathToAbsolute(ExtractDirectoryString(GetRelativeFilePath()));
				Platform::OpenExplorer(directory);
			}

			ImGui::SameLine();

			const char* deleteScenePopupID = "Delete scene";

			ImGui::PushStyleColor(ImGuiCol_Button, g_WarningButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_WarningButtonHoveredColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, g_WarningButtonActiveColor);

			if (ImGui::Button("Delete scene..."))
			{
				ImGui::OpenPopup(deleteScenePopupID);
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			if (ImGui::BeginPopupModal(deleteScenePopupID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, g_WarningTextColor);
				std::string textStr = "Are you sure you want to permanently delete " + m_Name + "? (both the default & saved files)";
				ImGui::Text("%s", textStr.c_str());
				ImGui::PopStyleColor();

				ImGui::PushStyleColor(ImGuiCol_Button, g_WarningButtonColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_WarningButtonHoveredColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, g_WarningButtonActiveColor);
				if (ImGui::Button("Delete"))
				{
					g_SceneManager->DeleteScene(this);

					ImGui::CloseCurrentPopup();

					bCloseContextMenu = true;
				}
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::PopStyleColor();

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				if (g_InputManager->GetKeyPressed(KeyCode::KEY_ESCAPE, true))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if (bCloseContextMenu)
			{
				ImGui::CloseCurrentPopup();
			}

			if (g_InputManager->GetKeyPressed(KeyCode::KEY_ESCAPE, true))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	bool BaseScene::DestroyGameObject(GameObject* targetObject, bool bDestroyChildren)
	{
		for (GameObject* gameObject : m_RootObjects)
		{
			if (DestroyGameObjectRecursive(gameObject, targetObject, bDestroyChildren))
			{
				UpdateRootObjectSiblingIndices();
				g_Renderer->RenderObjectStateChanged();
				return true;
			}
		}
		return false;
	}

	bool BaseScene::IsLoaded() const
	{
		return m_bLoaded;
	}

	bool BaseScene::DestroyGameObjectRecursive(GameObject* currentObject,
		GameObject* targetObject,
		bool bDestroyChildren)
	{
		if (currentObject == targetObject)
		{
			for (auto callback : m_OnGameObjectDestroyedCallbacks)
			{
				callback->Execute(targetObject);
			}

			// Target's parent pointer will be cleared upon removing from parent, cache it before that happens
			GameObject* targetParent = targetObject->m_Parent;
			if (targetObject->m_Parent)
			{
				targetParent->RemoveChild(targetObject);
			}
			else
			{
				auto iter = Find(m_RootObjects, targetObject);
				if (iter != m_RootObjects.end())
				{
					m_RootObjects.erase(iter);
				}
			}

			// Set children's parents
			if (!bDestroyChildren)
			{
				for (GameObject* childObject : targetObject->m_Children)
				{
					if (targetParent)
					{
						targetParent->AddChild(childObject);
					}
					else
					{
						AddRootObject(childObject);
					}
				}
			}

			// If children are still in m_Children array when
			// targetObject is destroyed they will also be destroyed
			if (!bDestroyChildren)
			{
				targetObject->m_Children.clear();
			}

			targetObject->Destroy();

			delete targetObject;

			return true;
		}

		for (GameObject* childObject : currentObject->m_Children)
		{
			if (DestroyGameObjectRecursive(childObject, targetObject, bDestroyChildren))
			{
				return true;
			}
		}

		return false;
	}

	i32 BaseScene::GetMaterialArrayIndex(const Material& material)
	{
		i32 materialArrayIndex = -1;
		for (u32 j = 0; j < m_LoadedMaterials.size(); ++j)
		{
			Material& loadedMat = g_Renderer->GetMaterial(m_LoadedMaterials[j]);
			if (loadedMat.Equals(material))
			{
				materialArrayIndex = (i32)j;
				break;
			}
		}
		return materialArrayIndex;
	}

	void BaseScene::ParseFoundMeshFiles()
	{
		s_ParsedMeshes.clear();

		if (FileExists(MESHES_FILE_LOCATION))
		{
			if (g_bEnableLogging_Loading)
			{
				const std::string cleanedFilePath = StripLeadingDirectories(MESHES_FILE_LOCATION);
				Print("Parsing meshes file at %s\n", cleanedFilePath.c_str());
			}

			JSONObject obj;
			if (JSONParser::ParseFromFile(MESHES_FILE_LOCATION, obj))
			{
				auto meshObjects = obj.GetObjectArray("meshes");
				for (auto meshObject : meshObjects)
				{
					s_ParsedMeshes.push_back(meshObject);
				}
			}
			else
			{
				PrintError("Failed to parse mesh file: %s\n\terror: %s\n", MESHES_FILE_LOCATION, JSONParser::GetErrorString());
				return;
			}
		}
		else
		{
			PrintError("Failed to parse meshes file at %s\n", MESHES_FILE_LOCATION);
			return;
		}

		if (g_bEnableLogging_Loading)
		{
			Print("Parsed %u meshes\n", (u32)s_ParsedMeshes.size());
		}
	}

	void BaseScene::ParseFoundMaterialFiles()
	{
		s_ParsedMaterials.clear();

		if (FileExists(MATERIALS_FILE_LOCATION))
		{
			if (g_bEnableLogging_Loading)
			{
				const std::string cleanedFilePath = StripLeadingDirectories(MATERIALS_FILE_LOCATION);
				Print("Parsing materials file at %s\n", cleanedFilePath.c_str());
			}

			JSONObject obj;
			if (JSONParser::ParseFromFile(MATERIALS_FILE_LOCATION, obj))
			{
				auto materialObjects = obj.GetObjectArray("materials");
				for (auto materialObject : materialObjects)
				{
					s_ParsedMaterials.push_back(materialObject);
				}
			}
			else
			{
				PrintError("Failed to parse materials file: %s\n\terror: %s\n", MATERIALS_FILE_LOCATION, JSONParser::GetErrorString());
				return;
			}
		}
		else
		{
			PrintError("Failed to parse materials file at %s\n", MATERIALS_FILE_LOCATION);
			return;
		}

		if (g_bEnableLogging_Loading)
		{
			Print("Parsed %u materials\n", (u32)s_ParsedMaterials.size());
		}
	}

	void BaseScene::ParseFoundPrefabFiles()
	{
		s_ParsedPrefabs.clear();

		std::vector<std::string> foundFiles;
		if (Platform::FindFilesInDirectory(PREFAB_LOCATION, foundFiles, ".json"))
		{
			for (const std::string& foundFilePath : foundFiles)
			{
				if (g_bEnableLogging_Loading)
				{
					const std::string fileName = StripLeadingDirectories(foundFilePath);
					Print("Parsing prefab: %s\n", fileName.c_str());
				}

				JSONObject obj;
				if (JSONParser::ParseFromFile(foundFilePath, obj))
				{
					s_ParsedPrefabs.push_back(obj);
				}
				else
				{
					PrintError("Failed to parse prefab file: %s, error: %s\n", foundFilePath.c_str(), JSONParser::GetErrorString());
					return;
				}
			}
		}
		else
		{
			PrintError("Failed to find files in \"" PREFAB_LOCATION "\"!\n");
			return;
		}

		if (g_bEnableLogging_Loading)
		{
			Print("Parsed %u prefabs\n", (u32)s_ParsedPrefabs.size());
		}
	}

	bool BaseScene::SerializeMeshFile() const
	{
		JSONObject meshesObj = {};

		meshesObj.fields.emplace_back("version", JSONValue(m_MeshesFileVersion));

		// Overwrite all meshes in current scene in case any values were tweaked
		std::vector<GameObject*> allObjects = g_SceneManager->CurrentScene()->GetAllObjects();
		for (GameObject* obj : allObjects)
		{
			Mesh* mesh = obj->GetMesh();
			if (mesh)
			{
				std::string meshFileName = mesh->GetRelativeFilePath();
				if (!meshFileName.empty())
				{
					meshFileName = meshFileName.substr(strlen(MESH_DIRECTORY));
					bool bFound = false;
					for (JSONObject& parsedMeshObj : s_ParsedMeshes)
					{
						if (parsedMeshObj.GetString("file").compare(meshFileName) == 0)
						{
							parsedMeshObj = mesh->Serialize();
							bFound = true;
							break;
						}
					}

					if (!bFound)
					{
						JSONObject serializedMeshObj = mesh->Serialize();
						s_ParsedMeshes.push_back(serializedMeshObj);
					}

				}
			}
		}

		meshesObj.fields.emplace_back("meshes", JSONValue(s_ParsedMeshes));

		std::string fileContents = meshesObj.Print(0);

		const std::string fileName = StripLeadingDirectories(MESHES_FILE_LOCATION);
		if (WriteFile(MESHES_FILE_LOCATION, fileContents, false))
		{
			Print("Serialized mesh file to: %s\n", fileName.c_str());
		}
		else
		{
			PrintWarn("Failed to serialize mesh file to: %s\n", fileName.c_str());
			return false;
		}

		return true;
	}

	bool BaseScene::SerializeMaterialFile() const
	{
		JSONObject materialsObj = {};

		materialsObj.fields.emplace_back("version", JSONValue(m_MaterialsFileVersion));

		// Overwrite all materials in current scene in case any values were tweaked
		std::vector<MaterialID> currentSceneMatIDs = g_SceneManager->CurrentScene()->GetMaterialIDs();
		for (MaterialID matID : currentSceneMatIDs)
		{
			bool bExistsInFile = false;
			Material& mat = g_Renderer->GetMaterial(matID);
			for (JSONObject& parsedMatObj : s_ParsedMaterials)
			{
				if (parsedMatObj.GetString("name").compare(mat.name) == 0)
				{
					parsedMatObj = mat.Serialize();
					bExistsInFile = true;
					break;
				}
			}

			if (!bExistsInFile)
			{
				s_ParsedMaterials.push_back(mat.Serialize());
			}
		}

		materialsObj.fields.emplace_back("materials", JSONValue(s_ParsedMaterials));

		std::string fileContents = materialsObj.Print(0);

		const std::string fileName = StripLeadingDirectories(MATERIALS_FILE_LOCATION);
		if (WriteFile(MATERIALS_FILE_LOCATION, fileContents, false))
		{
			Print("Serialized materials file to: %s\n", fileName.c_str());
		}
		else
		{
			PrintWarn("Failed to serialize materials file to: %s\n", fileName.c_str());
			return false;
		}

		return true;
	}

	bool BaseScene::SerializePrefabFile() const
	{
		// TODO: Implement
		ENSURE_NO_ENTRY();
		return false;
	}

	std::vector<GameObject*> BaseScene::GetAllObjects()
	{
		std::vector<GameObject*> result;

		for (GameObject* obj : m_RootObjects)
		{
			obj->AddSelfAndChildrenToVec(result);
		}

		return result;
	}

	TrackManager* BaseScene::GetTrackManager()
	{
		return &m_TrackManager;
	}

	CartManager* BaseScene::GetCartManager()
	{
		return &m_CartManager;
	}

	std::string BaseScene::GetUniqueObjectName(const std::string& prefix, i16 digits)
	{
		std::string result = prefix;

		i32 existingCount = 0;
		std::vector<GameObject*> allObjects = GetAllObjects();
		for (GameObject* gameObject : allObjects)
		{
			std::string s = gameObject->GetName();
			if (StartsWith(s, prefix))
			{
				existingCount++;
			}
		}

		result += IntToString(existingCount, digits);

		return result;
	}

	void BaseScene::DestroyObjectAtEndOfFrame(GameObject* obj)
	{
#if DEBUG
		auto iter = std::find(m_ObjectsToDestroyAtEndOfFrame.begin(), m_ObjectsToDestroyAtEndOfFrame.end(), obj);
		if (iter != m_ObjectsToDestroyAtEndOfFrame.end())
		{
			PrintWarn("Attempted to flag object to be removed at end of frame more than once! %s\n", obj->GetName().c_str());
		}
#endif
		m_ObjectsToDestroyAtEndOfFrame.push_back(obj);
	}

	void BaseScene::DestroyObjectsAtEndOfFrame(const std::vector<GameObject*>& objs)
	{
#if DEBUG
		for (auto o : objs)
		{
			auto iter = std::find(m_ObjectsToDestroyAtEndOfFrame.begin(), m_ObjectsToDestroyAtEndOfFrame.end(), o);
			if (iter != m_ObjectsToDestroyAtEndOfFrame.end())
			{
				PrintWarn("Attempted to flag object to be removed at end of frame more than once! %s\n", o->GetName().c_str());
			}
		}
#endif
		m_ObjectsToDestroyAtEndOfFrame.insert(m_ObjectsToDestroyAtEndOfFrame.end(), objs.begin(), objs.end());
	}

	void BaseScene::AddObjectAtEndOFFrame(GameObject* obj)
	{
#if DEBUG
		auto iter = std::find(m_ObjectsToAddAtEndOfFrame.begin(), m_ObjectsToAddAtEndOfFrame.end(), obj);
		if (iter != m_ObjectsToAddAtEndOfFrame.end())
		{
			PrintWarn("Attempted to flag object to be added at end of frame more than once! %s\n", obj->GetName().c_str());
		}
#endif
		m_ObjectsToAddAtEndOfFrame.push_back(obj);
	}

	void BaseScene::AddObjectsAtEndOFFrame(const std::vector<GameObject*>& objs)
	{
#if DEBUG
		for (auto o : objs)
		{
			auto iter = std::find(m_ObjectsToAddAtEndOfFrame.begin(), m_ObjectsToAddAtEndOfFrame.end(), o);
			if (iter != m_ObjectsToAddAtEndOfFrame.end())
			{
				PrintWarn("Attempted to flag object to be added at end of frame more than once! %s\n", o->GetName().c_str());
			}
		}
#endif
		m_ObjectsToAddAtEndOfFrame.insert(m_ObjectsToAddAtEndOfFrame.end(), objs.begin(), objs.end());
	}

	i32 BaseScene::GetSceneFileVersion() const
	{
		return m_SceneFileVersion;
	}

	bool BaseScene::HasPlayers() const
	{
		return m_bSpawnPlayer;
	}

	const SkyboxData& BaseScene::GetSkyboxData() const
	{
		return m_SkyboxData;
	}

	std::vector<MaterialID> BaseScene::RetrieveMaterialIDsFromJSON(const JSONObject& object, i32 fileVersion)
	{
		std::vector<MaterialID> matIDs;
		if (fileVersion >= 3)
		{
			std::vector<JSONField> materialNames;
			if (object.SetFieldArrayChecked("materials", materialNames))
			{
				for (const JSONField& materialNameField : materialNames)
				{
					std::string materialName = materialNameField.label;
					if (!materialName.empty())
					{
						MaterialID materialID = InvalidMaterialID;
						for (MaterialID loadedMatID : m_LoadedMaterials)
						{
							Material& material = g_Renderer->GetMaterial(loadedMatID);
							if (material.name.compare(materialName) == 0)
							{
								materialID = loadedMatID;
								break;
							}
						}
						if (materialID == InvalidMaterialID)
						{
							if (materialName.compare("placeholder") == 0)
							{
								materialID = g_Renderer->GetPlaceholderMaterialID();
							}
						}
						if (materialID != InvalidMaterialID)
						{
							matIDs.push_back(materialID);
						}
					}
					else
					{
						PrintError("Invalid material name for object %s: %s\n", object.GetString("name").c_str(), materialName.c_str());
					}
				}
			}
		}
		else // fileVersion < 3
		{
			MaterialID matID = InvalidMaterialID;
			std::string materialName;
			if (object.SetStringChecked("material", materialName))
			{
				if (!materialName.empty())
				{
					for (MaterialID loadedMatID : m_LoadedMaterials)
					{
						Material& mat = g_Renderer->GetMaterial(loadedMatID);
						if (mat.name.compare(materialName) == 0)
						{
							matID = loadedMatID;
							break;
						}
					}
					if (matID == InvalidMaterialID)
					{
						if (materialName.compare("placeholder") == 0)
						{
							matID = g_Renderer->GetPlaceholderMaterialID();
						}
					}
				}
				else
				{
					matID = InvalidMaterialID;
					PrintError("Invalid material name for object %s: %s\n",
						object.GetString("name").c_str(), materialName.c_str());
				}
			}
			matIDs.push_back(matID);
		}

		return matIDs;
	}

	void BaseScene::UpdateRootObjectSiblingIndices()
	{
		for (i32 i = 0; i < (i32)m_RootObjects.size(); ++i)
		{
			m_RootObjects[i]->UpdateSiblingIndices(i);
		}
	}

	void BaseScene::SerializeToFile(bool bSaveOverDefault /* = false */) const
	{
		bool success = true;

		success &= SerializeMeshFile();
		success &= SerializeMaterialFile();
		//success &= BaseScene::SerializePrefabFile();

		PROFILE_AUTO("Serialize scene");

		JSONObject rootSceneObject = {};

		rootSceneObject.fields.emplace_back("version", JSONValue(m_SceneFileVersion));
		rootSceneObject.fields.emplace_back("name", JSONValue(m_Name));
		rootSceneObject.fields.emplace_back("spawn player", JSONValue(m_bSpawnPlayer));

		JSONObject skyboxDataObj = {};
		skyboxDataObj.fields.emplace_back("top colour", JSONValue(VecToString(glm::pow(m_SkyboxData.top, glm::vec4(1.0f / 2.2f)))));
		skyboxDataObj.fields.emplace_back("mid colour", JSONValue(VecToString(glm::pow(m_SkyboxData.mid, glm::vec4(1.0f / 2.2f)))));
		skyboxDataObj.fields.emplace_back("btm colour", JSONValue(VecToString(glm::pow(m_SkyboxData.btm, glm::vec4(1.0f / 2.2f)))));
		rootSceneObject.fields.emplace_back("skybox data", JSONValue(skyboxDataObj));

		{
			JSONObject cameraObj = {};

			BaseCamera* cam = g_CameraManager->CurrentCamera();

			// TODO: Serialize all non-procedural cameras & last used

			cameraObj.fields.emplace_back("type", JSONValue(cam->GetName().c_str()));

			cameraObj.fields.emplace_back("near plane", JSONValue(cam->zNear));
			cameraObj.fields.emplace_back("far plane", JSONValue(cam->zFar));
			cameraObj.fields.emplace_back("fov", JSONValue(cam->FOV));
			cameraObj.fields.emplace_back("aperture", JSONValue(cam->aperture));
			cameraObj.fields.emplace_back("shutter speed", JSONValue(cam->shutterSpeed));
			cameraObj.fields.emplace_back("light sensitivity", JSONValue(cam->lightSensitivity));
			cameraObj.fields.emplace_back("exposure", JSONValue(cam->exposure));
			cameraObj.fields.emplace_back("move speed", JSONValue(cam->moveSpeed));

			std::string posStr = VecToString(cam->position, 3);
			real pitch = cam->pitch;
			real yaw = cam->yaw;
			JSONObject cameraTransform = {};
			cameraTransform.fields.emplace_back("position", JSONValue(posStr));
			cameraTransform.fields.emplace_back("pitch", JSONValue(pitch));
			cameraTransform.fields.emplace_back("yaw", JSONValue(yaw));
			cameraObj.fields.emplace_back("transform", JSONValue(cameraTransform));

			rootSceneObject.fields.emplace_back("camera", JSONValue(cameraObj));
		}

		std::vector<JSONObject> objectsArray;
		for (GameObject* rootObject : m_RootObjects)
		{
			if (rootObject->IsSerializable())
			{
				objectsArray.push_back(rootObject->Serialize(this));
			}
		}
		rootSceneObject.fields.emplace_back("objects", JSONValue(objectsArray));

		if (!m_TrackManager.m_Tracks.empty())
		{
			rootSceneObject.fields.emplace_back("track manager", JSONValue(m_TrackManager.Serialize()));
		}

		Print("Serializing scene to %s\n", m_FileName.c_str());

		std::string fileContents = rootSceneObject.Print(0);

		const std::string defaultSaveFilePath = SCENE_DEFAULT_LOCATION + m_FileName;
		const std::string savedSaveFilePath = SCENE_SAVED_LOCATION + m_FileName;
		std::string saveFilePath;
		if (bSaveOverDefault)
		{
			saveFilePath = defaultSaveFilePath;
		}
		else
		{
			saveFilePath = savedSaveFilePath;
		}

		saveFilePath = RelativePathToAbsolute(saveFilePath);

		if (bSaveOverDefault)
		{
			//m_bUsingSaveFile = false;

			if (FileExists(savedSaveFilePath))
			{
				Platform::DeleteFile(savedSaveFilePath);
			}
		}


		success &= WriteFile(saveFilePath, fileContents, false);

		if (success)
		{
			AudioManager::PlaySource(FlexEngine::GetAudioSourceID(FlexEngine::SoundEffect::blip));
			g_Renderer->AddEditorString("Saved " + m_Name);

			if (!bSaveOverDefault)
			{
				//m_bUsingSaveFile = true;
			}
		}
		else
		{
			PrintError("Failed to open file for writing at \"%s\", Can't serialize scene\n", m_FileName.c_str());
			AudioManager::PlaySource(FlexEngine::GetAudioSourceID(FlexEngine::SoundEffect::dud_dud_dud_dud));
		}
	}

	void BaseScene::DeleteSaveFiles()
	{
		const std::string defaultSaveFilePath = SCENE_DEFAULT_LOCATION + m_FileName;
		const std::string savedSaveFilePath = SCENE_SAVED_LOCATION + m_FileName;

		bool bDefaultFileExists = FileExists(defaultSaveFilePath);
		bool bSavedFileExists = FileExists(savedSaveFilePath);

		if (bSavedFileExists ||
			bDefaultFileExists)
		{
			Print("Deleting scene's save files from %s\n", m_FileName.c_str());

			if (bDefaultFileExists)
			{
				Platform::DeleteFile(defaultSaveFilePath);
			}

			if (bSavedFileExists)
			{
				Platform::DeleteFile(savedSaveFilePath);
			}
		}

		//m_bUsingSaveFile = false;
	}

	GameObject* BaseScene::AddRootObject(GameObject* gameObject)
	{
		if (!gameObject)
		{
			return nullptr;
		}

		for (GameObject* rootObject : m_RootObjects)
		{
			if (rootObject == gameObject)
			{
				PrintWarn("Attempted to add same root object (%s) to scene more than once\n", rootObject->m_Name.c_str());
				return nullptr;
			}
		}

		gameObject->SetParent(nullptr);
		m_RootObjects.push_back(gameObject);
		UpdateRootObjectSiblingIndices();
		g_Renderer->RenderObjectStateChanged();

		return gameObject;
	}

	void BaseScene::RemoveRootObject(GameObject* gameObject, bool bDestroy)
	{
		auto iter = m_RootObjects.begin();
		while (iter != m_RootObjects.end())
		{
			if (*iter == gameObject)
			{
				if (bDestroy)
				{
					(*iter)->Destroy();
					delete* iter;
				}

				iter = m_RootObjects.erase(iter);
				UpdateRootObjectSiblingIndices();
				g_Renderer->RenderObjectStateChanged();
				return;
			}
			else
			{
				++iter;
			}
		}

		PrintWarn("Attempting to remove non-existent child from scene %s\n", m_Name.c_str());
	}

	void BaseScene::RemoveAllRootObjects(bool bDestroy)
	{
		auto iter = m_RootObjects.begin();
		while (iter != m_RootObjects.end())
		{
			if (bDestroy)
			{
				delete* iter;
			}

			iter = m_RootObjects.erase(iter);
		}
		m_RootObjects.clear();
		g_Renderer->RenderObjectStateChanged();
	}

	bool BaseScene::RemoveObject(GameObject* gameObject, bool bDestroy)
	{
		for (auto iter = m_RootObjects.begin(); iter != m_RootObjects.end(); ++iter)
		{
			if (*iter == gameObject)
			{
				if (bDestroy)
				{
					(*iter)->Destroy();
					delete* iter;
				}

				m_RootObjects.erase(iter);
				UpdateRootObjectSiblingIndices();
				return true;
			}

			for (auto childIter = (*iter)->m_Children.begin(); childIter != (*iter)->m_Children.end(); ++childIter)
			{
				if (RemoveObject(*childIter, bDestroy))
				{
					return true;
				}
			}
		}

		return false;
	}

	std::vector<MaterialID> BaseScene::GetMaterialIDs()
	{
		return m_LoadedMaterials;
	}

	void BaseScene::AddMaterialID(MaterialID newMaterialID)
	{
		m_LoadedMaterials.push_back(newMaterialID);
	}

	void BaseScene::RemoveMaterialID(MaterialID materialID)
	{
		auto iter = m_LoadedMaterials.begin();
		while (iter != m_LoadedMaterials.end())
		{
			Material& material = g_Renderer->GetMaterial(*iter);
			MaterialID matID;
			if (g_Renderer->FindOrCreateMaterialByName(material.name, matID))
			{
				if (matID == materialID)
				{
					// TODO: Find all objects which use this material and give them new mats
					iter = m_LoadedMaterials.erase(iter);
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}

	GameObject* BaseScene::FirstObjectWithTag(const std::string& tag)
	{
		for (GameObject* gameObject : m_RootObjects)
		{
			GameObject* result = FindObjectWithTag(tag, gameObject);
			if (result)
			{
				return result;
			}
		}

		return nullptr;
	}

	Player* BaseScene::GetPlayer(i32 index)
	{
		if (index == 0)
		{
			return m_Player0;
		}
		else if (index == 1)
		{
			return m_Player1;
		}
		else
		{
			PrintError("Requested invalid player from BaseScene with index: %i\n", index);
			return nullptr;
		}
	}

	GameObject* BaseScene::FindObjectWithTag(const std::string& tag, GameObject* gameObject)
	{
		if (gameObject->HasTag(tag))
		{
			return gameObject;
		}

		for (GameObject* child : gameObject->m_Children)
		{
			GameObject* result = FindObjectWithTag(tag, child);
			if (result)
			{
				return result;
			}
		}

		return nullptr;
	}

	std::vector<GameObject*>& BaseScene::GetRootObjects()
	{
		return m_RootObjects;
	}

	void BaseScene::GetInteractableObjects(std::vector<GameObject*>& interactableObjects)
	{
		std::vector<GameObject*> allObjects = GetAllObjects();
		for (GameObject* object : allObjects)
		{
			if (object->m_bInteractable)
			{
				interactableObjects.push_back(object);
			}
		}
	}

	void BaseScene::BindOnGameObjectDestroyedCallback(ICallbackGameObject* callback)
	{
		if (std::find(m_OnGameObjectDestroyedCallbacks.begin(), m_OnGameObjectDestroyedCallbacks.end(), callback) != m_OnGameObjectDestroyedCallbacks.end())
		{
			PrintWarn("Attempted to bind on game object destroyed callback multiple times!\n");
			return;
		}

		m_OnGameObjectDestroyedCallbacks.push_back(callback);
	}

	void BaseScene::UnbindOnGameObjectDestroyedCallback(ICallbackGameObject* callback)
	{
		auto iter = std::find(m_OnGameObjectDestroyedCallbacks.begin(), m_OnGameObjectDestroyedCallbacks.end(), callback);
		if (iter == m_OnGameObjectDestroyedCallbacks.end())
		{
			PrintWarn("Attempted to unbind game object destroyed callback that isn't present in callback list!\n");
			return;
		}

		m_OnGameObjectDestroyedCallbacks.erase(iter);
	}

	void BaseScene::SetName(const std::string& name)
	{
		assert(!name.empty());
		m_Name = name;
	}

	std::string BaseScene::GetName() const
	{
		return m_Name;
	}

	std::string BaseScene::GetDefaultRelativeFilePath() const
	{
		return SCENE_DEFAULT_LOCATION + m_FileName;
	}

	std::string BaseScene::GetRelativeFilePath() const
	{
		//if (m_bUsingSaveFile)
		//{
		//	return SCENE_SAVED_LOCATION + m_FileName;
		//}
		//else
		//{
		return SCENE_DEFAULT_LOCATION + m_FileName;
		//}
	}

	std::string BaseScene::GetShortRelativeFilePath() const
	{
		//if (m_bUsingSaveFile)
		//{
		//	return SCENE_SAVED_LOCATION + m_FileName;
		//}
		//else
		//{
		return SCENE_DEFAULT_LOCATION + m_FileName;
		//}
	}

	bool BaseScene::SetFileName(const std::string& fileName, bool bDeletePreviousFiles)
	{
		bool success = false;

		if (fileName == m_FileName)
		{
			return true;
		}

		std::string absDefaultFilePathFrom = RelativePathToAbsolute(GetDefaultRelativeFilePath());
		std::string absDefaultFilePathTo = ExtractDirectoryString(absDefaultFilePathFrom) + fileName;
		if (Platform::CopyFile(absDefaultFilePathFrom, absDefaultFilePathTo))
		{
			//if (m_bUsingSaveFile)
			{
				std::string absSavedFilePathFrom = RelativePathToAbsolute(GetRelativeFilePath());
				std::string absSavedFilePathTo = ExtractDirectoryString(absSavedFilePathFrom) + fileName;
				if (Platform::CopyFile(absSavedFilePathFrom, absSavedFilePathTo))
				{
					success = true;
				}
			}
			//else
			//{
			//	success = true;
			//}

			// Don't delete files unless copies worked
			if (success)
			{
				if (bDeletePreviousFiles)
				{
					std::string pAbsDefaultFilePath = RelativePathToAbsolute(GetDefaultRelativeFilePath());
					Platform::DeleteFile(pAbsDefaultFilePath, false);

					//if (m_bUsingSaveFile)
					//{
					//	std::string pAbsSavedFilePath = RelativePathToAbsolute(GetRelativeFilePath());
					//	Platform::DeleteFile(pAbsSavedFilePath, false);
					//}
				}

				m_FileName = fileName;
			}
		}

		return success;
	}

	std::string BaseScene::GetFileName() const
	{
		return m_FileName;
	}

	bool BaseScene::IsUsingSaveFile() const
	{
		//return m_bUsingSaveFile;
		return false;
	}

	PhysicsWorld* BaseScene::GetPhysicsWorld()
	{
		return m_PhysicsWorld;
	}

} // namespace flex
