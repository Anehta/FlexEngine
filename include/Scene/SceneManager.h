﻿#pragma once

#include <vector>

class BaseScene;
struct GameContext;

class SceneManager
{
public:
	SceneManager();
	virtual ~SceneManager();

	void UpdateAndRender(const GameContext& gameContext);

	void AddScene(BaseScene* newScene);
	void RemoveScene(BaseScene* scene, const GameContext& gameContext);

	void SetCurrentScene(int sceneIndex);
	void SetCurrentScene(std::string sceneName);

	BaseScene* CurrentScene() const;

	void DestroyAllScenes(const GameContext& gameContext);

private:
	int m_CurrentSceneIndex;
	std::vector<BaseScene*> m_Scenes;

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

};
