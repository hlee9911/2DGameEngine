#include "LevelLoader.h"

LevelLoader::LevelLoader() noexcept
{

}

LevelLoader::~LevelLoader() noexcept
{

}

void LevelLoader::TileMapSetUp(sol::state& lua, const std::unique_ptr<Registry>& m_registry) noexcept
{

}

void LevelLoader::LoadLevel(sol::state& lua,
							const std::unique_ptr<Registry>& m_registry,
							const std::unique_ptr<AssetStore>& m_assetStore,
							std::unique_ptr<EventBus>& m_eventBus,
							SDL_Renderer* m_renderer,
							unsigned int level) noexcept
{
	currentLevel = level;
	
	sol::load_result script = lua.load_file("./assets/scripts/Level" + std::to_string(level) + ".lua");

	// checks the syntax of the lua script but does not execute it
	if (!script.valid())
	{
		sol::error err = script;
		Logger::Error("Error loading the lua script: " + std::string(err.what()));
		return;
	}

	lua.script_file("./assets/scripts/Level" + std::to_string(level) + ".lua");

	sol::table levelmap = lua["Level"];
	sol::table assets = levelmap["assets"];

	int i = 0;
	while (true)
	{
		sol::optional<sol::table> existsAssetIndexNode = assets[i];
		if (existsAssetIndexNode == sol::nullopt)
		{
			break;
		}
		sol::table asset = assets[i];
		std::string assetType = assets[i]["type"];
		std::string assetId = assets[i]["id"];
		if (assetType == "texture")
		{
			m_assetStore->AddTexture(m_renderer, assetId, asset["file"]);
			Logger::Log("Added texture: " + assetId);
		}
		else if (assetType == "font")
		{
			m_assetStore->AddFont(assetId, asset["file"], asset["font_size"]);
			Logger::Log("Added font: " + assetId);
		} 
		i++;
	}

	// load the entities and components from the lua file and execute it
	//  lua.script_file("./assets/scripts/Level" + std::to_string(level) + ".lua");

	// Load the tilemap
	sol::table map = levelmap["tilemap"];
	std::string mapFilePath = map["map_file"];
	std::string mapTextureAssestId = map["texture_asset_id"];
	int mapNumRows = map["num_rows"];
	int mapNumCols = map["num_cols"];
	int tileSize = map["tile_size"];
	double tileScale = map["scale"];
	std::fstream mapFile;
	mapFile.open(mapFilePath);

	if (!mapFile.is_open())
	{
		Logger::Error("Error loading the tilemap file");
		return;
	}

	////////////////////////////////////////////////////////////////////////////
	// Read the level tilemap information
	////////////////////////////////////////////////////////////////////////////
	// reading the tilemap file
	for (int row = 0; row < mapNumRows; ++row)
	{
		for (int col = 0; col < mapNumCols; ++col)
		{
			// will give correct tile from the tilemap
			char ch[2] = { 0, 0 };
			mapFile.get(ch[0]);
			int srcRectY = std::atoi(&ch[0]) * tileSize;
			mapFile.get(ch[1]);
			int srcRectX = std::atoi(&ch[1]) * tileSize;
			mapFile.ignore(); // ignore the comma

			// Create an entity for each tile
			Entity tile = m_registry->CreateEntity();
			tile.Group(tileGroup);
			tile.AddComponent<TransformComponent>(glm::vec2(col * (tileScale * tileSize), row * (tileScale * tileSize)), glm::vec2(tileScale, tileScale), 0.0);
			tile.AddComponent<SpriteComponent>(mapTextureAssestId, tileSize, tileSize, 0, false, srcRectX, srcRectY);
		}
	}
	mapFile.close();

	// calculate the map width and height
	Game::mapWidth = mapNumCols * tileSize * tileScale;
	Game::mapHeight = mapNumRows * tileSize * tileScale;

	////////////////////////////////////////////////////////////////////////////
	// Read the level entities and their components
	////////////////////////////////////////////////////////////////////////////
	sol::table entities = levelmap["entities"];
	i = 0;
	while (true) {
		sol::optional<sol::table> hasEntity = entities[i];
		if (hasEntity == sol::nullopt) {
			break;
		}

		sol::table entity = entities[i];

		Entity newEntity = m_registry->CreateEntity();

		// Tag
		sol::optional<std::string> tag = entity["tag"];
		if (tag != sol::nullopt) {
			newEntity.Tag(entity["tag"]);
		}

		// Group
		sol::optional<std::string> group = entity["group"];
		if (group != sol::nullopt) {
			newEntity.Group(entity["group"]);
		}

		// Components
		sol::optional<sol::table> hasComponents = entity["components"];
		if (hasComponents != sol::nullopt) {
			// Transform
			sol::optional<sol::table> transform = entity["components"]["transform"];
			if (transform != sol::nullopt) {
				newEntity.AddComponent<TransformComponent>(
					glm::vec2(
						entity["components"]["transform"]["position"]["x"],
						entity["components"]["transform"]["position"]["y"]
					),
					glm::vec2(
						entity["components"]["transform"]["scale"]["x"].get_or(1.0),
						entity["components"]["transform"]["scale"]["y"].get_or(1.0)
					),
					entity["components"]["transform"]["rotation"].get_or(0.0)
				);
			}

			// RigidBody
			sol::optional<sol::table> rigidbody = entity["components"]["rigidbody"];
			if (rigidbody != sol::nullopt) {
				newEntity.AddComponent<RigidbodyComponent>(
					glm::vec2(
						entity["components"]["rigidbody"]["velocity"]["x"].get_or(0.0),
						entity["components"]["rigidbody"]["velocity"]["y"].get_or(0.0)
					)
				);
			}

			// Sprite
			sol::optional<sol::table> sprite = entity["components"]["sprite"];
			if (sprite != sol::nullopt) {
				newEntity.AddComponent<SpriteComponent>(
					entity["components"]["sprite"]["texture_asset_id"],
					entity["components"]["sprite"]["width"],
					entity["components"]["sprite"]["height"],
					entity["components"]["sprite"]["z_index"].get_or(1),
					entity["components"]["sprite"]["fixed"].get_or(false),
					entity["components"]["sprite"]["src_rect_x"].get_or(0),
					entity["components"]["sprite"]["src_rect_y"].get_or(0)
				);
			}

			// Animation
			sol::optional<sol::table> animation = entity["components"]["animation"];
			if (animation != sol::nullopt) {
				newEntity.AddComponent<AnimationComponent>(
					entity["components"]["animation"]["num_frames"].get_or(1),
					entity["components"]["animation"]["speed_rate"].get_or(1)
				);
			}

			// BoxCollider
			sol::optional<sol::table> collider = entity["components"]["boxcollider"];
			if (collider != sol::nullopt) {
				newEntity.AddComponent<BoxColliderComponent>(
					entity["components"]["boxcollider"]["width"],
					entity["components"]["boxcollider"]["height"],
					glm::vec2(
						entity["components"]["boxcollider"]["offset"]["x"].get_or(0),
						entity["components"]["boxcollider"]["offset"]["y"].get_or(0)
					)
				);
			}


			// ProjectileEmitter
			sol::optional<sol::table> projectileEmitter = entity["components"]["projectile_emitter"];
			if (projectileEmitter != sol::nullopt) {
				newEntity.AddComponent<ProjectileEmitterComponent>(
					glm::vec2(
						entity["components"]["projectile_emitter"]["projectile_velocity"]["x"],
						entity["components"]["projectile_emitter"]["projectile_velocity"]["y"]
					),
					static_cast<int>(entity["components"]["projectile_emitter"]["repeat_frequency"].get_or(1)) * 1000,
					static_cast<int>(entity["components"]["projectile_emitter"]["projectile_duration"].get_or(10)) * 1000,
					static_cast<int>(entity["components"]["projectile_emitter"]["hit_percentage_damage"].get_or(10)),
					entity["components"]["projectile_emitter"]["friendly"].get_or(false),
					entity["components"]["projectile_emitter"]["manual"].get_or(false)
				);
			}

			// CameraFollow
			sol::optional<sol::table> cameraFollow = entity["components"]["camera_follow"];
			if (cameraFollow != sol::nullopt) {
				newEntity.AddComponent<CameraFollowComponent>();
			}

			// KeyboardControlled
			sol::optional<sol::table> keyboardControlled = entity["components"]["keyboard_controller"];
			if (keyboardControlled != sol::nullopt) {
				newEntity.AddComponent<KeyboardControlledComponent>(
					glm::vec2(
						entity["components"]["keyboard_controller"]["up_velocity"]["x"],
						entity["components"]["keyboard_controller"]["up_velocity"]["y"]
					),
					glm::vec2(
						entity["components"]["keyboard_controller"]["right_velocity"]["x"],
						entity["components"]["keyboard_controller"]["right_velocity"]["y"]
					),
					glm::vec2(
						entity["components"]["keyboard_controller"]["down_velocity"]["x"],
						entity["components"]["keyboard_controller"]["down_velocity"]["y"]
					),
					glm::vec2(
						entity["components"]["keyboard_controller"]["left_velocity"]["x"],
						entity["components"]["keyboard_controller"]["left_velocity"]["y"]
					)
				);
			}

			// Health
			sol::optional<sol::table> health = entity["components"]["health"];
			if (health != sol::nullopt) {
				newEntity.AddComponent<HealthComponent>(
					static_cast<int>(entity["components"]["health"]["health_percentage"].get_or(100)),
					static_cast<int>(entity["components"]["health"]["health_percentage"].get_or(100))
				);
			}
		}
		i++;
	}

	Entity label = m_registry->CreateEntity();
	SDL_Color green = { 0, 255, 0 };
	label.AddComponent<TextLabelComponent>(glm::vec2(Game::windowWidth / 2 - 40, 10), "CHOPPER 1.0", "charriot-font", green, true);

	m_registry->SubscribeToEvents(m_eventBus);

	//// Adding assets to the asset store
	//m_assetStore->AddTexture(m_renderer, tankImage, "./assets/images/tank-panther-right.png");
	//m_assetStore->AddTexture(m_renderer, truckImage, "./assets/images/truck-ford-right.png");
	//m_assetStore->AddTexture(m_renderer, chopperImage, "./assets/images/chopper-spritesheet.png");
	//m_assetStore->AddTexture(m_renderer, radarImage, "./assets/images/radar.png");
	//m_assetStore->AddTexture(m_renderer, jungleTileTexture, "./assets/tilemaps/jungle.png");
	//m_assetStore->AddTexture(m_renderer, bulletImage, "./assets/images/bullet.png");
	//m_assetStore->AddTexture(m_renderer, treeImage, "./assets/images/tree.png");

	//m_assetStore->AddFont("charriot-font", "./assets/fonts/charriot.ttf", 20);
	//m_assetStore->AddFont("pico8-font-8", "./assets/fonts/charriot.ttf", 8);
	//m_assetStore->AddFont("pico8-font-10", "./assets/fonts/charriot.ttf", 10);

	//// Load the tilemap
	//TileMapSetUp(lua, m_registry);

	//// Create the entities
	//Entity chopper = m_registry->CreateEntity();
	//chopper.Tag(playerTag);
	//chopper.AddComponent<TransformComponent>(glm::vec2(10.0, 10.0), glm::vec2(1.0, 1.0), 0.0);
	//chopper.AddComponent<RigidbodyComponent>(glm::vec2(0.0, 0.0));
	//chopper.AddComponent<SpriteComponent>(chopperImage, IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT, 2);
	//chopper.AddComponent<BoxColliderComponent>(IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT);
	//chopper.AddComponent<AnimationComponent>(2, 16, true);
	//chopper.AddComponent<ProjectileEmitterComponent>(glm::vec2(250.0, 250.0), 400, 3000, 10, true, true);
	//chopper.AddComponent<KeyboardControlledComponent>(glm::vec2(0, -100), glm::vec2(100, 0), glm::vec2(0, 100), glm::vec2(-100, 0));
	//chopper.AddComponent<CameraFollowComponent>();
	//chopper.AddComponent<HealthComponent>(100, 100);

	//Entity radar = m_registry->CreateEntity();
	//radar.AddComponent<TransformComponent>(glm::vec2(Game::windowWidth - 74, 10.0), glm::vec2(1.0, 1.0), 0.0);
	//radar.AddComponent<RigidbodyComponent>(glm::vec2(0.0, 0.0));
	//radar.AddComponent<SpriteComponent>(radarImage, IMAGE_SIZE_WIDTH * 2, IMAGE_SIZE_HEIGHT * 2, 5, true);
	//radar.AddComponent<AnimationComponent>(8, 8, true);

	//Entity tank = m_registry->CreateEntity();
	//tank.Group(enemyGroup);
	//tank.AddComponent<TransformComponent>(glm::vec2(500.0, 495.0), glm::vec2(1.0, 1.0), 0.0);
	//tank.AddComponent<RigidbodyComponent>(glm::vec2(20.0, 0.0));
	//tank.AddComponent<SpriteComponent>(tankImage, IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT, 1);
	//tank.AddComponent<BoxColliderComponent>(IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT);
	//// tank.AddComponent<ProjectileEmitterComponent>(glm::vec2(150.0, 0.0), 2000, 3000, 10, false, false);
	//tank.AddComponent<HealthComponent>(50, 50);

	//Entity treeA = m_registry->CreateEntity();
	//treeA.Group(obstaclesGroup);
	//treeA.AddComponent<TransformComponent>(glm::vec2(600.0, 495), glm::vec2(1.0, 1.0), 0.0);
	//treeA.AddComponent<SpriteComponent>(treeImage, 16, IMAGE_SIZE_HEIGHT, 2);
	//treeA.AddComponent<BoxColliderComponent>(16, 32);

	//Entity treeB = m_registry->CreateEntity();
	//treeB.Group(obstaclesGroup);
	//treeB.AddComponent<TransformComponent>(glm::vec2(400.0, 495.0), glm::vec2(1.0, 1.0), 0.0);
	//treeB.AddComponent<SpriteComponent>(treeImage, 16, IMAGE_SIZE_HEIGHT, 2);
	//treeB.AddComponent<BoxColliderComponent>(16, 32);

	//Entity truck = m_registry->CreateEntity();
	//truck.Group(enemyGroup);
	//truck.AddComponent<TransformComponent>(glm::vec2(10.0, 10.0), glm::vec2(1.0, 1.0), 90.0);
	//truck.AddComponent<RigidbodyComponent>(glm::vec2(0.0, 0.0));
	//truck.AddComponent<SpriteComponent>(truckImage, IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT, 1);
	//truck.AddComponent<BoxColliderComponent>(IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT);
	//truck.AddComponent<ProjectileEmitterComponent>(glm::vec2(0.0, 150.0), 2000, 5000, 10, false, false);
	//truck.AddComponent<HealthComponent>(50, 50);

	//Entity label = m_registry->CreateEntity();
	//SDL_Color green = { 0, 255, 0 };
	//label.AddComponent<TextLabelComponent>(glm::vec2(Game::windowWidth / 2 - 40, 10), "CHOPPER 1.0", "charriot-font", green, true);

	//// Perform the subscription of the events that are waiting to be subscribed
	// m_registry->SubscribeToEvents(m_eventBus);
}
