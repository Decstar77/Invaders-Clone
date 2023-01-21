local GLFW_DIR = "vendor/glfw"
local ASSIMP_DIR = "vendor/assimp"
local GLM_DIR = "vendor/glm"
local AUDIO_FILE_DIR = "vendor/audio"
local ENET_DIR = "vendor/enet"
local STB_DIR = "vendor/stb"
local JSON_DIR = "vendor/json"

solution "Atto"
    location("")
    startproject "Atto"
    configurations { "Ship", "Release", "Debug" }
    platforms "x64"
    architecture "x64"

    filter "configurations:Ship"
        defines {
            "NDEBUG"
        }
        optimize "Full"
    filter "configurations:Release"
        defines {
            "NDEBUG"
        }
        optimize "Full"
    filter "configurations:Debug*"
        defines{
            "_DEBUG"
        }
        optimize "Debug"
        symbols "On"


project "Atto"
    location("atto")
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Default"
    flags { "FatalWarnings", "MultiProcessorCompile" }	
    debugdir "bin"
    
    targetdir("bin/%{cfg.architecture}")
    objdir("tmp/%{cfg.architecture}")

    disablewarnings { 
        "4057", -- Slightly different base types. Converting from type with volatile to without.
        "4100", -- Unused formal parameter. I think unusued parameters are good for documentation.
        "4152", -- Conversion from function pointer to void *. Should be ok.
        "4200", -- Zero-sized array. Valid C99.
        "4201", -- Nameless struct/union. Valid C11.
        "4204", -- Non-constant aggregate initializer. Valid C99.
        "4206", -- Translation unit is empty. Might be #ifdefed out.
        "4214", -- Bool bit-fields. Valid C99.
        "4221", -- Pointers to locals in initializers. Valid C99.
        "4702", -- Unreachable code. We sometimes want return after exit() because otherwise we get an error about no return value.
    }

    includedirs
    {
        path.join(GLFW_DIR, "include"),
        path.join(ASSIMP_DIR, "include"),
        path.join(ENET_DIR, "include"),
        JSON_DIR,
        STB_DIR,
        GLM_DIR,
        AUDIO_FILE_DIR,
        "atto/src/"
    }
    
    libdirs
    {
        path.join(ASSIMP_DIR, "lib"),
    }
    
    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.c",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.hpp",
        path.join(STB_DIR, "stb_vorbis/stb_vorbis.c")
    }

    links { "glfw", "enet", }

    filter "system:windows"
        links { "kernel32", "user32", "ws2_32", "xaudio2", "winmm", "d3d11", "dxguid", "Dxgi", "d3dcompiler" }
        
    filter "configurations:Release"
        links { "assimp" }

     filter "configurations:Debug"
        links { "assimp" }
        

project "glfw"
    location(GLFW_DIR)
    kind "StaticLib"
    language "C"
    files
    {
        path.join(GLFW_DIR, "include/GLFW/*.h"),
        path.join(GLFW_DIR, "src/context.c"),
        path.join(GLFW_DIR, "src/egl_context.*"),
        path.join(GLFW_DIR, "src/init.c"),
        path.join(GLFW_DIR, "src/input.c"),
        path.join(GLFW_DIR, "src/internal.h"),
        path.join(GLFW_DIR, "src/monitor.c"),
        path.join(GLFW_DIR, "src/null*.*"),
        path.join(GLFW_DIR, "src/osmesa_context.*"),
        path.join(GLFW_DIR, "src/platform.c"),
        path.join(GLFW_DIR, "src/vulkan.c"),
        path.join(GLFW_DIR, "src/window.c"),
    }
    includedirs { path.join(GLFW_DIR, "include") }
    filter "system:windows"
        defines "_GLFW_WIN32"
        files
        {
            path.join(GLFW_DIR, "src/win32_*.*"),
            path.join(GLFW_DIR, "src/wgl_context.*")
        }

    filter "action:vs*"
        defines "_CRT_SECURE_NO_WARNINGS"

project "enet"
    location(ENET_DIR)
    kind "StaticLib"
    language "C"

    includedirs { 
        path.join(ENET_DIR, "include") 
    }
    
    defines {
        "_WINSOCK_DEPRECATED_NO_WARNINGS"
    }

    files {
        path.join(ENET_DIR, "inlcude/enet/callbacks.h"),
        path.join(ENET_DIR, "inlcude/enet/enet.h"),
        path.join(ENET_DIR, "inlcude/enet/list.h"),
        path.join(ENET_DIR, "inlcude/enet/protocol.h"),
        path.join(ENET_DIR, "inlcude/enet/time.h"),
        path.join(ENET_DIR, "inlcude/enet/types.h"),
        path.join(ENET_DIR, "inlcude/enet/unix.h"),
        path.join(ENET_DIR, "inlcude/enet/win32.h"),
        path.join(ENET_DIR, "inlcude/enet/utility.h"),
        path.join(ENET_DIR, "callbacks.c"),
        path.join(ENET_DIR, "compress.c"),
        path.join(ENET_DIR, "host.c"),
        path.join(ENET_DIR, "list.c"),
        path.join(ENET_DIR, "packet.c"),
        path.join(ENET_DIR, "peer.c"),
        path.join(ENET_DIR, "protocol.c"),
        path.join(ENET_DIR, "unix.c"),
        path.join(ENET_DIR, "win32.c")
    }
    
    filter "configurations:Debug"
        defines({ "DEBUG" })

