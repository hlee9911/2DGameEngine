#include "Game.h"
#include "../Logger/Logger.h"
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../Systems/Systems.h"
#include "../GameEngine/LevelLoader.h"

unsigned int Game::windowWidth;
unsigned int Game::windowHeight;
int Game::mapWidth;
int Game::mapHeight;

Game::Game() noexcept :
	m_window(nullptr),
	m_renderer(nullptr),
	m_registry(std::make_unique<Registry>()),
	m_assetStore(std::make_unique<AssetStore>()),
	m_eventBus(std::make_unique<EventBus>()),
	isRunning(false),
	isDebugMode(false),
	currentLevel(0)
{
	Logger::Log("Game contructor is called");
}

Game::~Game() noexcept
{
	Logger::Log("Game destructor is called");
}

void Game::Init() noexcept
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		Logger::Error("Error initializing SDL");
		return;
	}

	if (TTF_Init() != 0)
	{
		Logger::Error("Error initializing SDL TTF");
		return;
	}

	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);
	windowWidth = 800/*displayMode.w*/;
	windowHeight = 600/* displayMode.h*/;

	m_window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		windowWidth,
		windowHeight,
		//SDL_WINDOW_FULLSCREEN // special flags
		SDL_WINDOW_SHOWN
	);

	if (!m_window)
	{
		Logger::Error("Error creating SDL window");
		return;
	}

	m_renderer = SDL_CreateRenderer(
		m_window,
		-1, // get the default monitor
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC // special flags
	);

	if (!m_renderer)
	{
		Logger::Error("Error creating SDL renderer");
		return;
	}

	// initialize the imGui context
	ImGui::CreateContext();
	ImGuiSDL::Initialize(m_renderer, windowWidth, windowHeight);

	// initialize the camera view with the entire screen area
	m_camera.x = 0;
	m_camera.y = 0;
	m_camera.w = windowWidth;
	m_camera.h = windowHeight;

	isRunning = true;

	// setting the logical size to be 800x600
	SDL_RenderSetLogicalSize(m_renderer, 800, 600);

	// SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
}

void Game::Run() noexcept
{
	SetUp();
	// game loop
	while (isRunning)
	{
		ProcessInput();
		Update();
		Render();
	}
}

void Game::SetUp() noexcept
{
	// Add the systems to that need to be processed in our game
	m_registry->AddSystem<MovementSystem>();
	m_registry->AddSystem<RenderSystem>();
	m_registry->AddSystem<AnimationSystem>();
	m_registry->AddSystem<CollisionSystem>();
	m_registry->AddSystem<RenderColliderSystem>();
	m_registry->AddSystem<DamageSystem>();
	m_registry->AddSystem<KeyboardControlSystem>();
	m_registry->AddSystem<CameraMovementSystem>();
	m_registry->AddSystem<ProjectileEmitSystem>();
	m_registry->AddSystem<ProjectileLifeCycleSystem>();
	m_registry->AddSystem<RenderTextSystem>();
	m_registry->AddSystem<RenderHealthBarSystem>();
	m_registry->AddSystem<RenderGUISystem>();
	m_registry->AddSystem<ScriptSystem>();

	// create the bindings between C++ and Lua
	// m_registry->GetSystem<ScriptSystem>().CreateLuaBindings(lua);

	// load first level
	LevelLoader loader;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os);
	loader.LoadLevel(lua, m_registry, m_assetStore, m_eventBus, m_renderer, 2);
}

void Game::ProcessInput() noexcept
{
	SDL_Event sdlEvent;
	// SDL_PollEvent returns 1 if there is a pending event or 0 if there are none available
	while (SDL_PollEvent(&sdlEvent))
	{
		// ImGui SDL Input
		ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
		ImGuiIO& io = ImGui::GetIO();

		int mouseX, mouseY;
		const int buttons = SDL_GetMouseState(&mouseX, &mouseY);

		io.MousePos = ImVec2(mouseX, mouseY);
		io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
		io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);

		// handle core SDL events (close window, key pressed, and etc)
		switch (sdlEvent.type)
		{
			case SDL_QUIT: // event that happens when we click the 'X' button
				isRunning = false;
				break;
			case SDL_KEYDOWN:
				if (sdlEvent.key.keysym.sym == SDLK_ESCAPE)
					isRunning = false;
				if (sdlEvent.key.keysym.sym == SDLK_F1)
					isDebugMode = !isDebugMode; // toggle debug mode, shows the colliders boxes
				// Publish events on key pressed
				m_eventBus->PublishEvent<KeyPressedEvent>(sdlEvent.key.keysym.sym);
				// Logger::Log("Key pressed");
				break;
		}
	}
}

void Game::Update() noexcept
{
	// TODO: if we are too fast, waste some time until we reach the target frame time
	int timeToWait = MILLISECOND_PER_FRAME - (SDL_GetTicks() - millisecondPreviousFrame);
	if (timeToWait > 0 && timeToWait <= MILLISECOND_PER_FRAME)
	{
		SDL_Delay(timeToWait); // delay the execution until we reach the target frame time in milliseconds
	}
	
	// the difference in ticks since the last frame, converted to seconds
	double deltaTime = (SDL_GetTicks() - millisecondPreviousFrame) / 1000.0;

	// store the current frame time
	millisecondPreviousFrame = SDL_GetTicks();

	// Reset all event handlers for the current frame
	// m_eventBus->Reset();

	// shouldn't we move this to SetUp()?
	// Perform the subscription of the events that are waiting to be subscribed
	// m_registry->SubscribeToEvents(m_eventBus);

	// Updat the registry to process the entities that are waiting to be created/deleted
	// Invoke all the systems that need to be updated
	m_registry->Update(deltaTime, m_eventBus, m_camera, m_registry, m_assetStore, m_renderer, SDL_GetTicks());
}

void Game::Render() noexcept
{
	SDL_SetRenderDrawColor(m_renderer, 21, 21, 21, 255); // select the color
	SDL_RenderClear(m_renderer); // clear the previous frame
	
	// Invoke all the systems that need to be rendered (using loop)
	m_registry->Render(m_renderer, m_assetStore, m_camera, m_registry, isDebugMode);

	SDL_RenderPresent(m_renderer); // draw the frame buffer
}

void Game::Destroy() noexcept
{
	ImGuiSDL::Deinitialize();
	ImGui::DestroyContext();
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

// fake fullscreen vs real fullscreen
// fake fullscreen: windowed mode with the window border removed
// real fullscreen: the game takes over the entire screen

// double-buffering: drawing to a separate frame buffer
// swap the frame buffer with the one being displayed
// back buffer draw the renderers and front buffer displays the renderers
// then they swap places and the back buffer becomes the front buffer and vice versa

// Data-Oriented Design
// - Data-Oriented Design is a programming paradigm that focuses on the data and how it is organized in memory
// - optimization approach motivated by efficient usage of CPU cache, largely popularized by game developers
// - the claim is that tradition OOP is not cache-friendly and leads to poor data locality
// - the approach is to focus on the data layout, separating and sorting fields according to when they are needed, and primarily thinking about transformations of data
// - focus on performance, data transformation, and computer-friendly code
// - A cpu is a hardware cache used by central processing unit to reduce the average cost to access data from the main memory
// - a cache is a smaller, faster memory, closer to a processor core, which stores copies of the data from frequently used main memory locations
// - most cpu have a hierarchy of three cache levels: L1, L2, and L3

// struct of arrays vs array of structs
// struct of arrays: each component is stored in a separate array
// array of structs: each entity is stored in a separate array

// a retained mode API is declarative
// the applications constructs a scene from primitives such as buttons, inputs, rectangles, lines, and etc
// the graphic library stores a hierarchical model of the entire scene in memory, usually using sor tof oop
// we need to query the object model to fetch or update values from the UI widgets
// interaction is strongly based on Events
// to render a frame, the graphics library transform the scene that is stored in memory into a set of drawing commands

// an immediate mode API is imperative
// each time a new frame is drawn, the application directly issues the drawing commands
// the graphics library does not store a scene model in memory between
// instead of the application, keeps the of the scne and UI data
// no events, if a button is clicked, the render pass will execute the logic that needs to be executed
// less memroy overhead, no event change of the flow, everything is drawn proceesed frame by frame
