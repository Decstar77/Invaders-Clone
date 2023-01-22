#include "AttoLib.h"

#include "AttoAsset.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <stdarg.h> 

#include "AttoAsset.h"

namespace atto
{
    static void StringFormatV(char* dest, size_t size, const char* format, va_list va_listp)
    {
        vsnprintf(dest, size, format, va_listp);
    }

    static void StringFormat(char* dest, size_t size, const char* format, ...)
    {
        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(dest, size, format, arg_ptr);
        va_end(arg_ptr);
    }

    Logger::Logger() {
        Assert(instance == nullptr, "Logger already exists!");
        instance = this;
    }

    Logger::~Logger() {

    }

    void Logger::LogOutput(LogLevel level, const char* message, ...) {
        const char* levelStrings[6] = { "[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: " };
        const char* header = levelStrings[(u32)level];

        memset(instance->logBuffer, 0, sizeof(instance->logBuffer));
        memset(instance->outputBuffer, 0, sizeof(instance->outputBuffer));

        va_list arg_ptr;
        va_start(arg_ptr, message);
        StringFormatV(instance->logBuffer, sizeof(instance->logBuffer), message, arg_ptr);
        va_end(arg_ptr);

        StringFormat(instance->outputBuffer, sizeof(outputBuffer), "%s%s\n", header, instance->logBuffer);

        if (strlen(instance->outputBuffer) < LargeString::CAPCITY) {
            instance->logs.Add( LargeString::FromLiteral( instance->outputBuffer) );
        }

        Application::ConsoleWrite(instance->outputBuffer, (u8)level);

        if (level == LogLevel::FATAL) {
            Application::DisplayFatalError(message);
        }
    }

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        AppState* app = (AppState* )glfwGetWindowUserPointer(window);
        FrameInput* input = app->input;
        
        if (action == GLFW_PRESS) {
            input->lastKeys[key] = false;
            input->keys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            input->lastKeys[key] = true;
            input->keys[key] = false;
        }

        if (input->consoleKeyCallback != nullptr) {
            input->consoleKeyCallback(key, scancode, action, mods, input->consoleKeyCallbackDataPointer);
        }
    }

    static bool isFirstMouse = true;
    static void MousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
        AppState* app = (AppState*)glfwGetWindowUserPointer(window);
        FrameInput* input = app->input;
        if (isFirstMouse) {
            input->lastMousePosPixels.x = (f32)xpos;
            input->lastMousePosPixels.y = (f32)ypos;
            isFirstMouse = false;
        }
        
        f32 dx = (f32)xpos - input->lastMousePosPixels.x;
        f32 dy = input->lastMousePosPixels.y - (f32)ypos;
        
        input->mousePosPixels = glm::vec2((f32)xpos, (f32)ypos);

        app->engine->CallbackMousePosition(dx, dy);
        
        input->lastMousePosPixels = input->mousePosPixels;
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        AppState* app = (AppState*)glfwGetWindowUserPointer(window);
        FrameInput* input = app->input;

        if (action == GLFW_PRESS) {
            input->lastMouseButtons[button] = false;
            input->mouseButtons[button] = true;
        }
        else if (action == GLFW_RELEASE) {
            input->lastMouseButtons[button] = true;
            input->mouseButtons[button] = false;
        }
    }

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        AppState* app = (AppState*)glfwGetWindowUserPointer(window);
        app->engine->CallbackMouseWheel((f32)xoffset, (f32)yoffset);
    }

    static void FramebufferCallback(GLFWwindow* window, i32 w, i32 h) {
        AppState* app = (AppState*)glfwGetWindowUserPointer(window);
        app->windowWidth = w;
        app->windowHeight = h;
        app->windowAspect = (f32)w / (f32)h;
        if (app->engine) {
            app->engine->CallbackResize(w, h);
        }
    }

    bool Application::CreateApp(AppState &app) {
        app.logger  = new Logger();
        app.input   = new FrameInput();

        if (!glfwInit()) {
            ATTOFATAL("Could not init GLFW, your windows is f*cked");
            return false;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

#if ATTO_DEBUG_RENDERING
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
        //glfwWindowHint(GLFW_SAMPLES, 4);

        if (app.windowFullscreen) {
            app.monitor = glfwGetPrimaryMonitor();
        }
        
        app.window = glfwCreateWindow(app.windowWidth, app.windowHeight, app.windowTitle.GetCStr(), app.monitor, 0);
        glfwSetWindowUserPointer(app.window, &app);

        glfwGetFramebufferSize(app.window, &app.windowWidth, &app.windowHeight);

        if (app.window == nullptr) {
            ATTOFATAL("Could not create window, your windows is f*cked");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(app.window);
        glfwSwapInterval((i32)app.windowVsync);
        
        // Center the window
        if (app.windowFullscreen == false && app.windowCreateCentered == true) {
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            glfwSetWindowPos(app.window, (mode->width - app.windowWidth) / 2, (mode->height - app.windowHeight) / 2);
        }

        //glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursorPosCallback(app.window, MousePositionCallback);
        glfwSetKeyCallback(app.window, KeyCallback);
        glfwSetMouseButtonCallback(app.window, MouseButtonCallback);
        glfwSetScrollCallback(app.window, ScrollCallback);
        glfwSetFramebufferSizeCallback(app.window, FramebufferCallback);

        app.engine = new LeEngine();
        app.engine->Initialize(&app);

        return true;
    }

    void Application::DestroyApp(AppState &app) {
        app.engine->Shutdown();
        delete app.engine;
        glfwDestroyWindow(app.window);
        glfwTerminate();
    }

    bool Application::AppIsRunning(AppState& app) {
        return !glfwWindowShouldClose(app.window);
    }

    void Application::PresentApp(AppState& app) {
        glfwSwapBuffers(app.window);
    }

    void Application::UpdateApp(AppState& app) {
        if (app.shouldClose) {
            glfwSetWindowShouldClose(app.window, true);
            return;
        }

        app.shouldClose = glfwWindowShouldClose(app.window);
        if (app.shouldClose) {
            return;
        }
        
        glfwPollEvents();
    }

    void Application::DisableMouse(AppState& app) {
        glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Application::EnableMouse(AppState& app) {
        glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


    //class SomeClass {
//public:
//    void Do(int r) {
//        std::cout << "test " << r << std::endl;
//    }

//    static int Lua_Do(lua_State* L) {
//        SomeClass* c = (SomeClass*)lua_touserdata(L, 1);
//        
//        int r = (int)lua_tonumber(L, 2);
//        c->Do(r);
//        return 1;
//    }
//};
//

//void Lua::RunDummyTests()
//{
//    const char* cmd = "_Do(5)";

//    lua_State* L = luaL_newstate();

//    lua_register(L, "_Do", SomeClass::Lua_Do);
//    
//    SomeClass someClass = {};

//    lua_pushlightuserdata(L, &someClass);
//    int r = luaL_dostring(L, cmd);

//    if (r == LUA_OK)
//    {
//        lua_getglobal(L, "a");
//        if (lua_isnumber(L, -1)) {
//            float a = (float)lua_tonumber(L, -1);
//            std::cout << "A = " << a << std::endl;
//        }
//    }
//    else
//    {
//    }

//    lua_close(L);

//}
}