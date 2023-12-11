#pragma once
#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <string>
#include <fstream>
#include <memory>
#include <SDL.h>
#include <SDL_image.h>
#include <sol/sol.hpp>

#include "../Logger/Logger.h"
#include "../ECS/ECS.h"
#include "../AssetStore/AssetStore.h"
#include "../GameEngine/Game.h"
#include "../Components/Components.h"
#include "../Systems/Systems.h"
#include "../EventBus/EventBus.h"
#include "../EventBus/Event.h"
#include "../Events/Events.h"

class LevelLoader
{
public:

	LevelLoader() noexcept;
	~LevelLoader() noexcept;

	void TileMapSetUp(sol::state& lua, const std::unique_ptr<Registry>& m_registry) noexcept;
	void LoadLevel(sol::state& lua,
				   const std::unique_ptr<Registry>& m_registry,
				   const std::unique_ptr<AssetStore>& m_assetStore,
				   std::unique_ptr<EventBus>& m_eventBus,
				   SDL_Renderer* m_renderer,
				   unsigned int level) noexcept;

private:

	unsigned int currentLevel;

};

#endif // LEVELLOADER_H
