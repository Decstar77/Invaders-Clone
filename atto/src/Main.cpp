#include "AttoLib.h"
#include "AttoLua.h"

#include "AttoAsset.h"
#include "AttoGrad.h"

#include <iostream>

#include <GLFW/glfw3.h>

/*
* TODO:
* -- FONT: Add support for multiple lines
* -- FONT: Add support for text alignment (INCOMPLETE)
* -- FONT: Add support for text wrapping (MAYBE)
* 
* -- ASSETS: Locked down asset paths
* -- ASSETS: Add threading to asset loading
* -- ASSETS: Pack asset files
* 
*/

/*
* PROBLEM:
* Now that we use a fixed times step we can loose inputs across frame. I'm not sure that this is the biggest issue but 
* still something to be aware of. Not sure what the best solution would be if any. Maybe poll input events only on a game update ?
* App <-- t1 = 1, t2 = 0 (P)
* App <-- t1 = 0, t2 = 1 (R)
* Game <-- Misses the pushed key.
*/

using namespace atto;

int main(const int argc, const char** argv) {

    AppState app = {};
    //configScript.GetGlobalSafe("windowWidth",           app.windowWidth);
    //configScript.GetGlobalSafe("windowHeight",          app.windowHeight);
    //configScript.GetGlobalSafe("windowFullscreen",      app.windowFullscreen);
    //configScript.GetGlobalSafe("windowCreateCentered",  app.windowCreateCentered);
    //configScript.GetGlobal("renderingVsync",            app.windowVsync);
    //configScript.GetGlobal("assUseLooseAssets",         app.useLooseAssets);

    app.windowAspect = (f32)app.windowWidth / (f32)app.windowHeight;

    Application::CreateApp(app);
    
    const f64 targetFrameTimeMS = 16.0;
    f64 lastTimeMS = glfwGetTime() * 1000.0;
    f64 lagMS = 0.0f;

    while (Application::AppIsRunning(app)) {
        Application::UpdateApp(app);
        f64 currentTimeMS = glfwGetTime() * 1000.0;
        f64 elapsedMS = currentTimeMS - lastTimeMS;
        lastTimeMS = currentTimeMS;
        
        lagMS += elapsedMS;

        if (app.shouldClose || IsKeyJustDown(app.input, GLFW_KEY_ESCAPE)) {
            app.shouldClose = true;
        }

        // Need to cache the state here because we are going to update the state in the game loop
        FixedList<b8, KEY_CODE_COUNT> keysBeforeUpdate = app.input->lastKeys;
        FixedList<b8, MOUSE_BUTTON_LAST> mouseBeforeUpdate = app.input->lastMouseButtons;

        while (lagMS >= targetFrameTimeMS) {
            app.deltaTime = (f32)(targetFrameTimeMS / 1000.0);
            app.engine->Update(&app);
            lagMS -= targetFrameTimeMS;

            app.input->lastKeys = app.input->keys;
            app.input->lastMouseButtons = app.input->mouseButtons;
        }

        // Restore the state before the render update
        app.input->lastKeys = keysBeforeUpdate;
        app.input->lastMouseButtons = mouseBeforeUpdate;
        
        app.engine->Render(&app);

        app.input->lastKeys = app.input->keys;
        app.input->lastMouseButtons = app.input->mouseButtons;

        Application::PresentApp(app);

        //ATTOINFO("Delta time = %f", elapsedMS);
    }

    Application::DestroyApp(app);

    return 0;
}
