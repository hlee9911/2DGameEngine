#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <sol/sol.hpp>

#include "./GameEngine/Game.h"

//int nativeCPPFunction(int a, int b)
//{
//	return a + b;
//}
//
//void TestLua()
//{
//    sol::state lua;
//
//    // include all the base libraries
//    lua.open_libraries(sol::lib::base);
//
//    lua["add"] = nativeCPPFunction;
//
//    lua.script_file("./assets/scripts/myscript.lua");
//
//    // this is how you call a lua function
//
//    int some_variable_in_CPP = lua["some_variable"];
//
//    std::cout << "Value in CPP: " << some_variable_in_CPP << std::endl;
//
//    sol::table config = lua["config"];
//    int width = config["resolution"]["width"];
//    int height = config["resolution"]["height"];
//
//
//    // thisis how youget the value from lua table
//    bool isFullScreen = lua["config"]["fullscreen"];
//    std::cout << "fullscreen val " << isFullScreen << width << height << std::endl;
//    
//    sol::function func = lua["factorial"];
//    int result = func(6);
//    std::cout << "factorial of 6 is " << result << std::endl;
//
//}

int main(int argc, char* args[])
{   
    Game gameEngine;

    gameEngine.Init();
    gameEngine.Run();
    gameEngine.Destroy();

    return 0;
}
