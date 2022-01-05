#include "SceneManager"
#include "ScreenBuffer"
#include "Scene"
#include <ctime>
#include <random>
SceneManager SceneManager::sceneManager;

SceneManager::SceneManager() : currentScene(new SceneTitle()), nextScene(nullptr)
{
	srand(time(NULL));
}

void SceneManager::Run()
{
	if (nextScene != nullptr)
	{
		SceneInterface* prevScene = currentScene;
		currentScene = nextScene;
		nextScene = nullptr;
		delete prevScene;
	}
	currentScene->Update();
	ScreenBuffer::GetInstance()->Flip();
}

void SceneManager::LoadScene(SceneInterface* newScene)
{
	nextScene = newScene;
}

SceneManager* SceneManager::GetInstance()
{
	return &sceneManager;
}
