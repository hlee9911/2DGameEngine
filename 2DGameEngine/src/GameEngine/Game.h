#pragma once
#ifndef GAME_H
#define GAME_H

#include <SDL.h>
#include <SDL_image.h>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_sdl.h>
#include <imgui/imgui_impl_sdl.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>

#include "../ECS/ECS.h"
#include "../AssetStore/AssetStore.h"
#include "../EventBus/EventBus.h"
#include "../Events/Events.h"

namespace
{
	constexpr unsigned int FPS = 165;
	constexpr unsigned int MILLISECOND_PER_FRAME = 1000 / FPS;
	constexpr unsigned int IMAGE_SIZE_WIDTH = 32;
	constexpr unsigned int IMAGE_SIZE_HEIGHT = 32;

	// TileMapData
	constexpr int tileSize = 32;
	constexpr double tileScale = 2.0;
	constexpr int mapNumCols = 25;
	constexpr int mapNumRows = 20;
}

class Game
{
public:
	Game() noexcept;
	~Game() noexcept;

	void Init() noexcept;
	void Run() noexcept;
	void TileMapSetUp() noexcept;
	void LoadLevel(unsigned int level) noexcept;
	void SetUp() noexcept;

	// game loop functions
	void ProcessInput() noexcept;
	void Update() noexcept;
	void Render() noexcept;

	void Destroy() noexcept;

	static unsigned int windowWidth;
	static unsigned int windowHeight;
	static int mapWidth;
	static int mapHeight;

private:

	SDL_Window* m_window;
	SDL_Renderer* m_renderer;
	SDL_Rect m_camera;
	bool isRunning; // flag to check if the game is running
	bool isDebugMode; // flag to check if the game is in debug mode
	int millisecondPreviousFrame = 0;

	std::unique_ptr<Registry> m_registry;
	std::unique_ptr<AssetStore> m_assetStore;
	std::unique_ptr<EventBus> m_eventBus;

	unsigned int currentLevel;
};

#endif // GAME_H
