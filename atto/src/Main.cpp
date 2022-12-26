#include "AttoLib.h"
#include "AttoLua.h"

#include "Pong.h"
#include "SpaceInvaders.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace atto;

int main(const int argc, const char** argv) {

    LuaScript configScript;
    if (!configScript.LoadSafe("config.lua")) {
        Application::DisplayFatalError("Could not find config.lua");
        return 1;
    }
    
    AppState app = {};
    app.windowTitle = "Cosmic Combat";
    configScript.GetGlobalSafe("windowWidth",           app.windowWidth);
    configScript.GetGlobalSafe("windowHeight",          app.windowHeight);
    configScript.GetGlobalSafe("windowFullscreen",      app.windowFullscreen);
    configScript.GetGlobalSafe("windowCreateCentered",  app.windowCreateCentered);
    configScript.GetGlobal("renderingVsync",            app.windowVsync);
    configScript.GetGlobal("assUseLooseAssets",         app.useLooseAssets);

    app.windowAspect = (f32)app.windowWidth / (f32)app.windowHeight;

    Application::CreateApp(app);

    app.gameState = new Pong();

    f64 currentTime = glfwGetTime();
    f64 lastTime = currentTime;
    f32 dt = 0;

    app.gameState->Initialize(&app);

    while (Application::AppIsRunning(app)) {
        Application::UpdateApp(app);
        app.deltaTime = dt;
        app.gameState->UpdateAndRender(&app);
        if (app.shouldClose || IsKeyJustDown(app.input, GLFW_KEY_ESCAPE)) {
            app.shouldClose = true;
        }

        Application::PresentApp(app);

        app.input->lastKeys = app.input->keys;
        app.input->lastMouseButtons = app.input->mouseButtons;

        lastTime = currentTime;
        currentTime = glfwGetTime();
        dt = (f32)(currentTime - lastTime);
        //std::cout << "Dt = " << dt * 1000 << std::endl;
    }
    
    app.gameState->Shutdown(&app);

    Application::DestroyApp(app);

    return 0;
}
