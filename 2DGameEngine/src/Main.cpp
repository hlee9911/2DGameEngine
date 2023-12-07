#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <sol/sol.hpp>

#include "./GameEngine/Game.h"

int main(int argc, char* args[])
{   
    Game gameEngine;

    gameEngine.Init();
    gameEngine.Run();
    gameEngine.Destroy();

    return 0;
}
