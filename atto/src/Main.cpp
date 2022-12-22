#include "SpaceInvaders.h"

#include <iostream>

#include <al/alc.h>
#include <al/al.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace atto;

const i32 WINDOW_WIDTH = 1280;
const i32 WINDOW_HEIGHT = 720;

void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

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

int main()
{
    if (!glfwInit()) {
        Assert(0, " Could not initialize GLFW");
        return 0;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

#if ATTO_DEBUG_RENDERING
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    GLFWwindow* window = nullptr;
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Invaders clone", 0, 0);
    if (!window) {
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Center the window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - WINDOW_WIDTH) / 2, (mode->height - WINDOW_HEIGHT) / 2);

    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //glfwSetCursorPosCallback(window, MousePositionCallBack);
    //glfwSetKeyCallback(window, KeyCodeCallBack);
    //glfwSetMouseButtonCallback(window, MouseButtonCallBack);

    // Initialize GLAD
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

#if ATTO_DEBUG_RENDERING
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    const byte * glVendor = glGetString(GL_VENDOR);
    const byte * glRenderer = glGetString(GL_RENDERER);
    
    std::cout << "GL Vendor: " << glVendor << std::endl;
    std::cout << "GL Renderer: " << glRenderer << std::endl;

    // Initialize OpenAL
    ALCdevice* alDevice = alcOpenDevice(nullptr);
    if (alDevice == nullptr) {
        std::cout << "Could not open OpenAL device" << std::endl;
        return 0;
    }

    ALCcontext* alContext = alcCreateContext(alDevice, nullptr);
    if (alContext == nullptr) {
        alcCloseDevice(alDevice);
        std::cout << "Could not create OpenAL context" << std::endl;
        return 0;
    }
    
    if (!alcMakeContextCurrent(alContext)) {
        std::cout << "Could not make OpenAL context current" << std::endl;
        return 0;
    }

    alListener3f(AL_POSITION, 0, 0, 0.0f);
    alListener3f(AL_VELOCITY, 0, 0, 0);
    
    WindowData windowData = {};
    windowData.width = WINDOW_WIDTH;
    windowData.height = WINDOW_HEIGHT;
    windowData.aspect = (f32)windowData.width / (f32)windowData.height;
    windowData.title = "Invaders clone";
    windowData.shouldClose = false;

    FrameInput currentInput = {};
    FrameInput lastInput = {};
    currentInput.lastInput = &lastInput;;
    
    f64 currentTime = glfwGetTime();
    f64 lastTime = currentTime;
    f32 dt = 0;

    SpaceInvaders* game = new SpaceInvaders();
    game->Initialize(&windowData);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        currentInput.a = glfwGetKey(window, GLFW_KEY_A);
        currentInput.d = glfwGetKey(window, GLFW_KEY_D);
        currentInput.w = glfwGetKey(window, GLFW_KEY_W);
        currentInput.s = glfwGetKey(window, GLFW_KEY_S);
		currentInput.e = glfwGetKey(window, GLFW_KEY_E);
        currentInput.space = glfwGetKey(window, GLFW_KEY_SPACE);
        currentInput.escape = glfwGetKey(window, GLFW_KEY_ESCAPE);
        currentInput.enter = glfwGetKey(window, GLFW_KEY_ENTER);
        currentInput.tab = glfwGetKey(window, GLFW_KEY_TAB);

        game->UpdateAndRender(&windowData, &currentInput, dt);
        if (windowData.shouldClose || glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        glfwSwapBuffers(window);

        lastInput = currentInput;
        
        lastTime = currentTime;
        currentTime = glfwGetTime();
        dt = (f32)(currentTime - lastTime);
        //std::cout << "Dt = " << dt * 1000 << std::endl;
    }
    
    delete game;

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(alContext);
	alcCloseDevice(alDevice);

    glfwTerminate();

    return 0;
}
