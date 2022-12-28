#include "AttoLib.h"

#include "AttoAsset.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <al/alc.h>
#include <al/al.h>

#include <iostream>
#include <string>
#include <stdarg.h> 

#include "Pong.h"

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

    static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)     {
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
            return;
        }

        std::cout << "---------------" << std::endl;
        std::cout << "Debug message (" << id << "): " << message << std::endl;

        switch (source)
        {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
        } std::cout << std::endl;

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behavior"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behavior"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
        } std::cout << std::endl;

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
        } std::cout << std::endl;
        std::cout << std::endl;
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

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        //glfwSetCursorPosCallback(window, MousePositionCallBack);
        glfwSetKeyCallback(app.window, KeyCallback);
        //glfwSetMouseButtonCallback(window, MouseButtonCallBack);

        // Initialize GLAD
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

#if ATTO_DEBUG_RENDERING
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

        const byte* glVendor = glGetString(GL_VENDOR);
        const byte* glRenderer = glGetString(GL_RENDERER);

        ATTOINFO("GL Vendor %s", glVendor);
        ATTOINFO("GL Renderer %s", glRenderer);

        app.alDevice = alcOpenDevice(nullptr);
        if (app.alDevice == nullptr) {
            ATTOFATAL("Could not open OpenAL device")
            return false;
        }

        app.alContext = alcCreateContext(app.alDevice, nullptr);
        if (app.alContext == nullptr) {
            alcCloseDevice(app.alDevice);
            ATTOFATAL("Could not create OpenAL context");
            return false;
        }

        if (!alcMakeContextCurrent(app.alContext)) {
            ATTOFATAL("Could not make OpenAL context current");
            return false;
        }

        if (app.useLooseAssets) {
            ATTOTRACE("Using raw assets");
            app.engine = new LooseAssetLoader();
            app.engine->Initialize(&app);
        }
        else {
            ATTOTRACE("Using packed assets");
            Assert(0, "ddd");
        }

        app.gameState = new Pong();
        app.gameState->Initialize(&app);

        return true;
    }

    void Application::DestroyApp(AppState &app) {
        app.engine->Shutdown();
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(app.alContext);
        alcCloseDevice(app.alDevice);
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