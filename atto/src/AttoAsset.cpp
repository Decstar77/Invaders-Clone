#include "SpaceInvaders.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis/stb_vorbis.c"

#include <audio/AudioFile.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>

#include <al/alc.h>
#include <al/al.h>

#include <filesystem>
#include <random>

namespace atto {
    
    bool Bitmap::Write(byte* pixels, u32 width, u32 height, const char* name) {
        const u32 pixelSize = width * height * 4;

        FileHeader fileHeader = {};
        fileHeader.type[0] = 'B';
        fileHeader.type[1] = 'M';
        fileHeader.size = sizeof(FileHeader) + sizeof(InfoHeader) + pixelSize;
        fileHeader.offset = sizeof(FileHeader) + sizeof(InfoHeader);

        InfoHeader infoHeader = {};
        infoHeader.size = sizeof(InfoHeader);
        infoHeader.width = width;
        infoHeader.height = height;
        infoHeader.planes = 1;
        infoHeader.bitCount = 32;
        infoHeader.compression = 0;
        infoHeader.sizeImage = 0;
        infoHeader.xPelsPerMeter = 3200;
        infoHeader.yPelsPerMeter = 3200;
        infoHeader.clrUsed = 0;
        infoHeader.clrImportant = 0;

        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "Bitmap::Write -> Could not open file " << name << std::endl;
            return false;
        }

        file.write((char*)&fileHeader, sizeof(FileHeader));
        file.write((char*)&infoHeader, sizeof(InfoHeader));
        file.write((char*)pixels, pixelSize);
        file.close();

        return true;
    }

    static void FindAllFiles(const char* path, const char* extension, List<LargeString> &files) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.path().extension() == extension) {
                files.Add( LargeString::FromLiteral( entry.path().string().c_str() ));
            }
        }
    }

    bool LeEngine::Initialize(AppState* appState) {
        app = appState;

        basePathSprites = LargeString::FromLiteral("assets/sprites/");
        basePathSounds = LargeString::FromLiteral("assets/sounds/");
        
        DrawSurfaceResized(app->windowWidth, app->windowHeight);

        RegisterAssets();

        InitializeLuaBindings();
        
        InitializeShapeRendering();
        InitializeSpriteRendering();
        InitializeTextRendering();
        InitializeDebugRendering();

        return true;
    }

    void LeEngine::Update(AppState* app) {
        //luaEngine.SetGlobal("dt", app->deltaTime);
        //luaEngine.SetGlobal("key_a", app->input->keys[KEY_CODE_A]);
        //luaEngine.SetGlobal("key_d", app->input->keys[KEY_CODE_D]);
        //luaEngine.CallVoidFunction("update");
        app->gameState->Update(app);
    }

    void LeEngine::Render(AppState* app) {
        DrawClearSurface();
        DrawEnableAlphaBlending();
        //luaEngine.CallVoidFunction("render");
        app->gameState->Render(app);
    }

    void LeEngine::Shutdown() {

    }

    void LeEngine::MouseWheelCallback(f32 x, f32 y) {
        //static f32 zoom = 0;
        //zoom -= (i32)y * 150;
        //
        //f32 aspect = 1.777777776f;
        //
        //
        //i32 h = 256 + zoom;
        //i32 w = (i32)((f32)h * aspect);
        //
        //f32 right = (f32)w / 2.0f;
        //f32 left = -right;
        //
        //f32 top = (f32)h / 2.0f;
        //f32 bottom = -top;
        //
        ////glm::mat4 view = glm::inverse( glm::translate(glm::mat4(1), glm::vec3(100, 0, 200 + zoom)) );
        ////projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10000.0f) * view;
        //glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1), glm::vec3(100, 0, 0)));
        //projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f) * view;
        //mainSurfaceWidth = w;
        //mainSurfaceHeight = h;
    }

    f32 LeEngine::Random() {
        return Random(0.0f, 1.0f);
    }

    f32 LeEngine::Random(f32 min, f32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min, max);
        return dis(gen);
    }

    i32 LeEngine::RandomInt(i32 min, i32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dis(min, max);
        return dis(gen);
    }

    void LeEngine::InitializeShapeRendering() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;

            uniform mat4 p;

            void main() {
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            uniform int  mode;
            uniform vec4 color;
            uniform vec4 shapePosAndSize;
            uniform vec4 shapeRadius;

            // from http://www.iquilezles.org/www/articles/distfunctions/distfunctions

            float CircleSDF(vec2 r, vec2 p, float rad) {
                return 1 - max(length(p - r) - rad, 0);
            }

            float BoxSDF(vec2 r, vec2 p, vec2 s) {
                return 1 - length(max(abs(p - r) - s, 0));
            }
            
            float RoundedBoxSDF(vec2 r, vec2 p, vec2 s, float rad) {
                return 1 - (length(max(abs(p - r) - s + rad, 0)) - rad);
            }

            void main() {
                vec2 s = shapePosAndSize.zw;
                vec2 r = shapePosAndSize.xy;
                vec2 p = gl_FragCoord.xy;

                if (mode == 0) {
                    FragColor = color;
                } else if (mode == 1) {
                    float d = CircleSDF(r, p, shapeRadius.x);
                    d = clamp(d, 0.0, 1.0);
                    FragColor = vec4(color.xyz, color.w * d);
                } else if (mode == 2) {
                    float d = RoundedBoxSDF(r, p, s / 2, shapeRadius.x);
                    d = clamp(d, 0.0, 1.0);
                    FragColor = vec4(color.xyz, color.w * d);
                } else {
                    FragColor = vec4(1, 0, 1, 1);
                }
            }
        )";

        shapeRenderingState.color = glm::vec4(1, 1, 1, 1);
        shapeRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        shapeRenderingState.vertexBuffer = SubmitVertexBuffer(sizeof(ShapeVertex) * 6, nullptr, VERTEX_LAYOUT_TYPE_SHAPE, true);

        ATTOTRACE("Completed shape rendering initialization");
    }
    
    void LeEngine::InitializeSpriteRendering() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec2 texCoord;
            layout (location = 2) in vec4 color;

            out vec2 vertexTexCoord;
            out vec4 vertexColor;

            uniform mat4 p;

            void main() {
                vertexTexCoord = texCoord;
                vertexColor = color;
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 vertexTexCoord;
            in vec4 vertexColor;

            uniform sampler2D texture0;

            void main() {
                vec4 sampled = texture(texture0, vertexTexCoord);
                FragColor = sampled * vertexColor;
            }
        )";

        spriteRenderingState.color = glm::vec4(1, 1, 1, 1);
        spriteRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        spriteRenderingState.vertexBuffer = SubmitVertexBuffer(sizeof(SpriteVertex) * 6, nullptr, VERTEX_LAYOUT_TYPE_SPRITE, true);

        ATTOTRACE("Completed sprite rendering initialization");
    }

    void LeEngine::InitializeTextRendering() {
        textRenderingState.color = glm::vec4(1, 1, 1, 1);
        textRenderingState.underlinePercent = 1.0f;

        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec2 texCoord;

            out vec2 vertexTexCoord;

            uniform mat4 p;

            void main() {
                vertexTexCoord = texCoord;
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 vertexTexCoord;

            uniform int mode;
            uniform vec4 color;
            uniform sampler2D texture0;

            void main() {
                if (mode == 0) { // Text rendering
                    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture0, vertexTexCoord).r);
                    FragColor = vec4(1.0) * sampled;
                } else if (mode == 1 ) {
                    FragColor = color;
                }
            }
        )";

        textRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        textRenderingState.vertexBuffer = SubmitVertexBuffer(sizeof(FontVertex) * 6, nullptr, VERTEX_LAYOUT_TYPE_FONT, true);

        ATTOTRACE("Completed text rendering initialization");
    }

    void LeEngine::InitializeDebugRendering() {
        debugRenderingState.color = glm::vec4(0, 1, 0, 1);

        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec4 color;

            out vec4 vertexColor;

            uniform mat4 p;

            void main() {
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
                vertexColor = color;
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec4 vertexColor;

            void main() {
                FragColor = vertexColor;
            }
        )";

        debugRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        
        debugRenderingState.vertexBufer = SubmitVertexBuffer(
            debugRenderingState.lines.GetCapcity() * sizeof(DebugLineVertex), nullptr, 
            VERTEX_LAYOUT_TYPE_DEBUG_LINE, true
        );

        ATTOTRACE("Completed debug rendering initialization");
    }

    void LeEngine::InitializeLuaBindings() {
        luaEngine.CreateNew();

        luaEngine.SetGlobal("engine", this);
        
        luaEngine.RegisterFunction("atto_id_from_string",               Lua_IdFromString);
        luaEngine.RegisterFunction("atto_audio_play",                   Lua_AudioPlay);
        luaEngine.RegisterFunction("atto_draw_shape_set_color",         Lua_DrawShapeSetColor);
        luaEngine.RegisterFunction("atto_draw_shape_rect",              Lua_DrawShapeRect);
        luaEngine.RegisterFunction("atto_draw_shape_rect_cd",           Lua_DrawShapeRectCenterDim);
        luaEngine.RegisterFunction("atto_draw_shape_circle",            Lua_DrawShapeCircle);
        luaEngine.RegisterFunction("atto_draw_shape_round_rect",        Lua_DrawShapeRoundRect);
        luaEngine.RegisterFunction("atto_draw_sprite",                  Lua_DrawSprite);
    
        // TODO: Load assets via asset functions and remove hard coded string!!
        // TODO: Asset system for lua
        luaEngine.LoadFile("assets/scripts/engine.lua");
        
        LuaTable spritesTable = {};
        if (luaEngine.GetGlobal("sprites", spritesTable)) {
            while (spritesTable.Loop()) {
                LuaTable st = {};
                SmallString name = {};
                if (spritesTable.LoopGet(st, name)) {
                    SpriteAsset sprite = SpriteAsset::CreateDefault();
                    sprite.id = AssetId::Create(name.GetCStr());

                    LargeString texture = {};
                    if (!st["texture"].Get(texture)) {
                        ATTOERROR("Sprite %s, does not have a texture", name.GetCStr());
                        continue;
                    }
                    sprite.textureId = TextureAssetId::Create(texture.GetCStr());

                    if (!st["frameSize"].Get(sprite.frameSize)) {
                        ATTOERROR("Sprite %s, does not have a frameSize", name.GetCStr());
                        continue;
                    }

                    if (!st["frameCount"].Get(sprite.frameCount)) {
                        ATTOERROR("Sprite %s, does not have a frameCount", name.GetCStr());
                        continue;
                    }

                    st.Put("id", sprite.id.id);

                    st["animated"].Get(sprite.animated);
                    st["origin"].Get((i32&)sprite.origin);

                    ATTOTRACE("Sprite registered: %s , %ul", name.GetCStr(), sprite.id.id);
                    registeredSprites.Add(sprite);
                }
                else {
                    ATTOERROR("Could not find table in lua sprites table");
                }
            }
            spritesTable.EndLoop();
        }

        luaEngine.CallVoidFunction("start");
    }

    const void* LeEngine::LoadEngineAsset(AssetId id, AssetType type) {
        const i32 count = engineAssets.GetCount();
        for (i32 assetIndex = 0; assetIndex < count; ++assetIndex) {
            EngineAsset& asset = engineAssets[assetIndex];

            if (!asset.id.IsValid()) {
                break;
            }

            if (asset.id == id && asset.type == type) {
                switch (asset.type) {
                    case ASSET_TYPE_TEXTURE: {
                        if (asset.texture.textureHandle != 0) {
                            return &asset.texture;
                        }
                        
                        if (LoadTextureAsset(asset.path.GetCStr(), asset.texture)) {
                            ATTOTRACE("Loaded texture asset %s", asset.path.GetCStr());
                            return &asset.texture;
                        }
                    } break;
                    
                    case ASSET_TYPE_AUDIO: {
                        if (asset.audio.bufferHandle != 0) {
                            return &asset.audio;
                        }

                        if (LoadAudioAsset(asset.path.GetCStr(), asset.audio)) {
                            ATTOTRACE("Loaded audio sasset %s", asset.path.GetCStr());
                            return &asset.audio;
                        }
                    } break;

                    case ASSET_TYPE_FONT: {
                        if (asset.font.textureHandle != 0) {
                            return &asset.font;
                        }

                        if (LoadFontAsset(asset.path.GetCStr(), asset.font)) {
                            ATTOTRACE("Loaded font asset %s", asset.path.GetCStr());
                            return &asset.font;
                        }
                    } break;

                    default: {
                        Assert(0, "");
                    }
                }

                
            }
        }

        return nullptr;
    }

    TextureAsset* LeEngine::LoadTextureAsset(TextureAssetId id) {
        return (TextureAsset*)LoadEngineAsset(id.ToRawId(), ASSET_TYPE_TEXTURE);
    }
    
    FontAsset* LeEngine::LoadFontAsset(FontAssetId id) {
        return (FontAsset*)LoadEngineAsset(id.ToRawId(), ASSET_TYPE_FONT);
    }

    AudioAsset* LeEngine::LoadAudioAsset(AudioAssetId id) {
        return (AudioAsset*)LoadEngineAsset(id.ToRawId(), ASSET_TYPE_AUDIO);
    }

    Speaker LeEngine::AudioPlay(AudioAssetId audioAssetId, bool looping, f32 volume /*= 1.0f*/) {
        const AudioAsset* audioAsset = LoadAudioAsset(audioAssetId);
        if (!audioAsset) {
            ATTOERROR("Could not load audio asset");
            return {};
        }

        const i32 speakerCount = speakers.GetCount();
        for (i32 speakerIndex = 0; speakerIndex < speakerCount; ++speakerIndex) {
            Speaker& speaker = speakers[speakerIndex];
            if (speaker.sourceHandle == 0) {
                continue;
            }

            ALint state = {};
            alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED) {
                alSourcei(speaker.sourceHandle, AL_BUFFER, audioAsset->bufferHandle);
                alSourcei(speaker.sourceHandle, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
                alSourcef(speaker.sourceHandle, AL_GAIN, volume);
                alSourcePlay(speaker.sourceHandle);
                ALCheckErrors();

                return speaker;
            }
        }

        Speaker speaker = {};
        speaker.index = speakers.GetCount();
        alGenSources(1, &speaker.sourceHandle);
        alSourcei(speaker.sourceHandle, AL_BUFFER, audioAsset->bufferHandle);
        alSourcei(speaker.sourceHandle, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
        alSourcef(speaker.sourceHandle, AL_GAIN, volume);
        alSourcePlay(speaker.sourceHandle);
        ALCheckErrors();
        speakers.Add(speaker);

        return speaker;
    }

    void LeEngine::AudioPause(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            alSourcePause(speaker.sourceHandle);
        }
    }

    void LeEngine::AudioStop(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            alSourceStop(speaker.sourceHandle);
        }
    }

    bool LeEngine::AudioIsSpeakerPlaying(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            ALint state = {};
            alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
            return  state == AL_PLAYING;
        }

        return false;
    }

    bool LeEngine::AudioIsSpeakerAlive(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            ALint state = {};
            alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
            return state != AL_STOPPED;
        }

        return false;
    }

    void LeEngine::ShaderProgramBind(ShaderProgram* program) {
        Assert(program->programHandle != 0, "Shader program not created");
        globalRenderingState.program = program;
        glUseProgram(program->programHandle);
    }

    i32 LeEngine::ShaderProgramGetUniformLocation(const char* name) {
        ShaderProgram* program = globalRenderingState.program;
        if (program == nullptr) {
            ATTOERROR("Shader progam in not bound");
            return -1;
        }

        if (program->programHandle == 0) {
            ATTOERROR("Shader progam in not valid");
            return -1;
        }

        const u32 uniformCount = program->uniforms.GetCount();
        for (u32 uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
            ShaderUniform& uniform = program->uniforms[uniformIndex];
            if (uniform.name == name) {
                return uniform.location;
            }
        }

        i32 location = glGetUniformLocation(program->programHandle, name);
        if (location >= 0) {
            ShaderUniform newUniform = {};
            newUniform.location = location;
            newUniform.name = name;

            program->uniforms.Add(newUniform);
        }
        else {
            ATTOERROR("Could not find uniform value %s", name);
        }

        return location;
    }

    void LeEngine::ShaderProgramSetInt(const char* name, i32 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniform1i(location, value);
        }
    }

    void LeEngine::ShaderProgramSetSampler(const char* name, i32 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniform1i(location, value);
        }
    }

    void LeEngine::ShaderProgramSetTexture(i32 location, u32 textureHandle) {
        glBindTextureUnit(0, textureHandle);
    }

    void LeEngine::ShaderProgramSetVec4(const char* name, glm::vec4 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniform4fv(location, 1, glm::value_ptr(value));
        }
    }

    void LeEngine::ShaderProgramSetMat4(const char* name, glm::mat4 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void LeEngine::VertexBufferUpdate(VertexBuffer vertexBuffer, i32 offset, i32 size, const void* data) {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void LeEngine::DrawSurfaceResized(i32 w, i32 h) {
        glViewport(0, 0, w, h);

        // TODO: I'm not sure on how to handle dynamic resolutions...
#if 0
        // IDK
        f32 aspect = (f32)w / (f32)h;
        mainSurfaceHeight = (glm::abs(720 - h) < glm::abs(1080 - h)) ? 720 : 1080;
        mainSurfaceWidth = (i32)((f32)h * aspect);
        glViewport(0, 0, mainSurfaceWidth, mainSurfaceHeight);
#elif 0
        // Widen the camera ???
        mainSurfaceWidth = w;
        mainSurfaceHeight = h;
        glViewport(0, 0, mainSurfaceWidth, mainSurfaceHeight);
#elif 0
        // Stretch
        mainSurfaceWidth = 1280;
        mainSurfaceHeight = 720;
        glViewport(0, 0, w, h);
#else
        // Maintain aspect ratio with black bars
        mainSurfaceWidth = (i32)(1280.0 * 1.6f);
        mainSurfaceHeight = (i32)(720.0 * 1.6f);
        //mainSurfaceWidth = 480;
        //mainSurfaceHeight = 360;

        f32 ratioX = (f32)w / (f32)mainSurfaceWidth;
        f32 ratioY = (f32)h / (f32)mainSurfaceHeight;
        f32 ratio = ratioX < ratioY ? ratioX : ratioY;
        
        i32 viewWidth = (i32)(mainSurfaceWidth * ratio);
        i32 viewHeight = (i32)(mainSurfaceHeight * ratio);
        
        i32 viewX = (i32)((w - mainSurfaceWidth * ratio) / 2);
        i32 viewY = (i32)((h - mainSurfaceHeight * ratio) / 2);

        glViewport(viewX, viewY, viewWidth, viewHeight);
#endif
        
        f32 right = (f32)mainSurfaceWidth / 2.0f;
        f32 left = -right;
        
        f32 top = (f32)mainSurfaceHeight / 2.0f;
        f32 bottom = -top;
        
        cameraProjection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
        ATTOTRACE("Main surface resized to %d x %d", mainSurfaceWidth, mainSurfaceHeight);
    }

    void LeEngine::DrawClearSurface(const glm::vec4& color) {
        //glClearColor(color.r, color.g, color.b, color.a);
        glClearColor(0.43f, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void LeEngine::DrawEnableAlphaBlending() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void LeEngine::DrawShapeSetColor(glm::vec4 color) {
        shapeRenderingState.color = color;
    }

    void LeEngine::DrawShapeRect(glm::vec2 bl, glm::vec2 tr) {
        f32 xpos = bl.x;
        f32 ypos = bl.y;
        f32 h = tr.y - bl.y;
        f32 w = tr.x - bl.x;

        f32 vertices[6][2] = {
            { xpos,     ypos + h, },
            { xpos,     ypos,     },
            { xpos + w, ypos,     },
            
            { xpos,     ypos + h, },
            { xpos + w, ypos,     },
            { xpos + w, ypos + h, }
        };

        f32 centerX = (bl.x + tr.x) / 2.0f;
        f32 centerY = (bl.y + tr.y) / 2.0f;

        ShaderProgramBind(&shapeRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection);
        ShaderProgramSetInt("mode", 0);
        ShaderProgramSetVec4("color", shapeRenderingState.color);

        glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
        VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void LeEngine::DrawShapeRectCenterDim(glm::vec2 center, glm::vec2 dim) {
        glm::vec2 bottomLeft = center - dim / 2.0f;
        glm::vec2 topRight = center + dim / 2.0f;
        DrawShapeRect(bottomLeft, topRight);
    }

    void LeEngine::DrawShapeCircle(glm::vec2 center, f32 radius) {
        f32 x1 = center.x - radius;
        f32 y1 = center.y - radius;
        f32 x2 = center.x + radius;
        f32 y2 = center.y + radius;

        f32 vertices[6][2] = {
            { x1, y2 },
            { x1, y1 },
            { x2, y1 },
            { x1, y2 },
            { x2, y1 },
            { x2, y2 }
        };

        ShaderProgramBind(&shapeRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection);
        ShaderProgramSetInt("mode", 1);
        ShaderProgramSetVec4("color", shapeRenderingState.color);
        ShaderProgramSetVec4("shapePosAndSize", glm::vec4(center.x, center.y, radius, radius));
        ShaderProgramSetVec4("shapeRadius", glm::vec4(radius - 4, 0, 0, 0)); // The 4 here is to stop the circle from being cut of from the edges

        glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
        VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void LeEngine::DrawShapeRoundRect(glm::vec2 bl, glm::vec2 tr, f32 radius) {
        f32 xpos = bl.x;
        f32 ypos = bl.y;
        f32 h = tr.y - bl.y;
        f32 w = tr.x - bl.x;

        f32 vertices[6][2] = {
            { xpos,     ypos + h, },
            { xpos,     ypos,     },
            { xpos + w, ypos,     },

            { xpos,     ypos + h, },
            { xpos + w, ypos,     },
            { xpos + w, ypos + h, }
        };

        f32 centerX = (bl.x + tr.x) / 2.0f;
        f32 centerY = (bl.y + tr.y) / 2.0f;

        ShaderProgramBind(&shapeRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection);
        ShaderProgramSetInt("mode", 2);
        ShaderProgramSetVec4("color", shapeRenderingState.color);
        ShaderProgramSetVec4("shapePosAndSize", glm::vec4(centerX, centerY, w, h));
        ShaderProgramSetVec4("shapeRadius", glm::vec4(radius, 0, 0, 0));

        glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
        VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void LeEngine::DrawSprite(const AssetId& id, glm::vec2 pos, i32 frameIndex) {
        SpriteAsset *sprite = nullptr;

        const i32 spriteCount = registeredSprites.GetCount();
        for (i32 i = 0; i < spriteCount; i++) {
            if (registeredSprites[i].id == id) {
                sprite = &registeredSprites[i];
                break;
            }
        }

        if (sprite == nullptr) {
            ATTOERROR("SPRITE: Could not find sprite asset");
            return;
        }

        if (sprite->texture == nullptr) {
            sprite->texture = LoadTextureAsset(sprite->textureId);
            if (sprite->texture == nullptr) {
                ATTOERROR("SPRITE: Could not load texture asset");
                return;
            }
        }

        ShaderProgramBind(&spriteRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection);
        ShaderProgramSetSampler("texture0", 0);
        ShaderProgramSetTexture(0, sprite->texture->textureHandle);

        f32 xpos = pos.x;
        f32 ypos = pos.y;

        f32 scaleX = 1;
        f32 scaleY = 1;

        f32 w = sprite->frameSize.x * scaleX;
        f32 h = sprite->frameSize.y * scaleY;

        glm::vec2 uv0 = sprite->uv0;
        glm::vec2 uv1 = sprite->uv1;

        if (sprite->origin == SPRITE_ORIGIN_CENTER) {
            xpos -= w / 2.0f;
            ypos -= h / 2.0f;
        }

        uv0.x = (frameIndex * sprite->frameSize.x) / sprite->texture->width;
        uv1.x = (frameIndex * sprite->frameSize.x + sprite->frameSize.x) / sprite->texture->width;
        
        glm::vec4 color = spriteRenderingState.color;

        f32 vertices[6][2 + 2 + 4] = {
            { xpos,     ypos + h,    uv0.x, uv0.y,    color.r, color.g, color.b, color.a },
            { xpos,     ypos,        uv0.x, uv1.y,    color.r, color.g, color.b, color.a },
            { xpos + w, ypos,        uv1.x, uv1.y,    color.r, color.g, color.b, color.a },

            { xpos,     ypos + h,    uv0.x, uv0.y,    color.r, color.g, color.b, color.a },
            { xpos + w, ypos,        uv1.x, uv1.y,    color.r, color.g, color.b, color.a },
            { xpos + w, ypos + h,    uv1.x, uv0.y,    color.r, color.g, color.b, color.a }
        };

        static_assert(sizeof(vertices) == sizeof(SpriteVertex) * 6, "Sprite vertex size mismatch");

        glBindVertexArray(spriteRenderingState.vertexBuffer.vao);
        VertexBufferUpdate(spriteRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void LeEngine::DrawTextSetFont(FontAssetId id) {
        FontAsset* fontAsset = LoadFontAsset(id);
        if (!fontAsset) {
            ATTOERROR("Could not load font asset");
            return;
        }

        textRenderingState.font = fontAsset;
    }

    void LeEngine::DrawTextSetHalign(FontHAlignment hAlignment) {
        textRenderingState.hAlignment = hAlignment;
    }

    void LeEngine::DrawTextSetValign(FontVAlignment vAlignment) {
        textRenderingState.vAlignment = vAlignment;
    }

    f32 LeEngine::DrawTextWidth(FontAsset* currentFont, const char* text) {
        Assert(currentFont, "No font set");

        f32 width = 0;
        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = currentFont->glyphs[index];
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            // bit shift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            width += (ch.advance >> 6);
        }

        return width;
    }

    BoxBounds LeEngine::DrawTextBounds(FontAsset* currentFont, const char* text) {
        Assert(currentFont, "No font set");

        BoxBounds bounds = {};

        f32 x = 0.0f;
        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = currentFont->glyphs[index];

            f32 xpos = (f32)ch.bearing.x;
            f32 ypos = (f32)(-(ch.size.y - ch.bearing.y));
            f32 w = (f32)ch.size.x;
            f32 h = (f32)ch.size.y;

            bounds.min.y = glm::min(bounds.min.y, ypos);
            bounds.max.y = glm::max(bounds.max.y, ypos + h);

            x += (ch.advance >> 6);
        }

        bounds.max.x = x;

        return bounds;
    }

    DrawEntryFont LeEngine::DrawTextCreate(const char* text, glm::vec2 pos) {
        DrawEntryFont entry = {};
        entry.text = text;
        entry.pos = pos;
        entry.font = textRenderingState.font;
        entry.hAlignment = textRenderingState.hAlignment;
        entry.vAlignment = textRenderingState.vAlignment;
        entry.underlineThinkness = textRenderingState.underlineThinkness;
        entry.underlinePercent = textRenderingState.underlinePercent;
        entry.color = textRenderingState.color;

        BoxBounds bounds = DrawTextBounds(textRenderingState.font, text);
        if (entry.hAlignment == FONT_HALIGN_CENTER) {
            f32 width = bounds.max.x - bounds.min.x;
            bounds.min.x -= width / 2.0f;
            bounds.max.x -= width / 2.0f;
        }

        entry.textWidth = DrawTextWidth(textRenderingState.font, text);

        bounds.Translate(entry.pos);
        entry.bounds = bounds;

        return entry;
    }

    void LeEngine::DrawText(const char* inText, glm::vec2 pos) {
        DrawEntryFont entry = DrawTextCreate(inText, pos);

        ShaderProgramBind(&textRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection);
        ShaderProgramSetSampler("texture0", 0);

        glBindVertexArray(textRenderingState.vertexBuffer.vao);

        f32 x = entry.pos.x;
        f32 y = entry.pos.y;
        const char* text = entry.text.GetCStr();
        const f32 textWidth = entry.textWidth;
        
        if (entry.hAlignment == FONT_HALIGN_CENTER) {
            x -= textWidth / 2.0f;
        }

        if (entry.underlineThinkness > 0.0f && entry.underlinePercent > 0.0f) {
            ShaderProgramSetInt("mode", 1);
            ShaderProgramSetVec4("color", glm::vec4(1, 1, 1, 1));

            f32 xpos = x;
            f32 ypos = y - entry.underlineThinkness - 1.0f;
            f32 w = textWidth * entry.underlinePercent;
            f32 h = entry.underlineThinkness;

            if (entry.hAlignment == FONT_HALIGN_CENTER) {
                xpos = x + textWidth / 2.0f - w / 2.0f;
            }

            f32 vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };

            glBindBuffer(GL_ARRAY_BUFFER, textRenderingState.vertexBuffer.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        ShaderProgramSetInt("mode", 0);
        ShaderProgramSetTexture(0, entry.font->textureHandle);

        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = entry.font->glyphs[index];

            f32 xpos = x + ch.bearing.x;
            f32 ypos = y - (ch.size.y - ch.bearing.y);
            f32 w = (f32)ch.size.x;
            f32 h = (f32)ch.size.y;

            glm::vec2 uv0 = ch.uv0;
            glm::vec2 uv1 = ch.uv1;

            f32 vertices[6][4] = {
                { xpos,     ypos + h,   uv0.x, uv0.y },
                { xpos,     ypos,       uv0.x, uv1.y },
                { xpos + w, ypos,       uv1.x, uv1.y },

                { xpos,     ypos + h,   uv0.x, uv0.y },
                { xpos + w, ypos,       uv1.x, uv1.y },
                { xpos + w, ypos + h,   uv1.x, uv0.y }
            };

            glBindBuffer(GL_ARRAY_BUFFER, textRenderingState.vertexBuffer.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            // bit shift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            x += (ch.advance >> 6);
        }

        glBindVertexArray(0);
    }

    void LeEngine::DrawText(SmallString text, glm::vec2 pos) {
        DrawText(text.GetCStr(), pos);
    }

    void LeEngine::DEBUGPushLine(glm::vec2 a, glm::vec2 b) {
        const i32 newCount = debugRenderingState.lines.GetCount() + 2;
        if (newCount * sizeof(DebugLineVertex) > debugRenderingState.vertexBufer.size) {
            ATTOINFO("To many debug lines!!");
            return;
        }

        DebugLineVertex v1;
        v1.position = a;
        v1.color = debugRenderingState.color;

        DebugLineVertex v2;
        v2.position = b;
        v2.color = debugRenderingState.color;

        debugRenderingState.lines.Add(v1);
        debugRenderingState.lines.Add(v2);
    }

    void LeEngine::DEBUGPushRay(Ray2D ray) {
        DEBUGPushLine(ray.origin, ray.origin + ray.direction);
    }

    void LeEngine::DEBUGPushCircle(glm::vec2 pos, f32 radius) {
        const i32 newCount = debugRenderingState.lines.GetCount() + 2 * 32;
        if (newCount * sizeof(DebugLineVertex) > debugRenderingState.vertexBufer.size) {
            ATTOINFO("To many debug lines!!");
            return;
        }

        f32 angle = 0.0f;
        constexpr f32 angleStep = 2.0f * glm::pi<f32>() / 32.0f;
        for (i32 i = 0; i < 32; i++) {
            glm::vec2 a = pos + glm::vec2(cos(angle), sin(angle)) * radius;
            glm::vec2 b = pos + glm::vec2(cos(angle + angleStep), sin(angle + angleStep)) * radius;

            DEBUGPushLine(a, b);

            angle += angleStep;
        }
    }

    void LeEngine::DEBUGPushBox(BoxBounds box) {
        glm::vec2 v1 = box.min;
        glm::vec2 v2 = box.max;
        glm::vec2 v3 = glm::vec2(box.min.x, box.max.y);
        glm::vec2 v4 = glm::vec2(box.max.x, box.min.y);
        DEBUGPushLine(v1, v3);
        DEBUGPushLine(v1, v4);
        DEBUGPushLine(v2, v3);
        DEBUGPushLine(v2, v4);
    }

    void LeEngine::DEBUGSubmit() {
        const u32 vertexCount = debugRenderingState.lines.GetCount();
        const u32 vertexSize = vertexCount * sizeof(DebugLineVertex);

        if (vertexCount == 0) {
            return;
        }

        glNamedBufferSubData(debugRenderingState.vertexBufer.vbo, 0, vertexSize, debugRenderingState.lines.GetData());

        ShaderProgramBind(&debugRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection);
        glBindVertexArray(debugRenderingState.vertexBufer.vao);
        glDrawArrays(GL_LINES, 0, vertexCount);

        debugRenderingState.lines.Clear();
    }

    void LeEngine::EditorToggleConsole() {
        if (app->input->keys[KEY_CODE_F1]) {
            editorState.consoleOpen = true;
        }
        
        if (editorState.consoleOpen) {
            editorState.consoleScroll += app->deltaTime;
            if (editorState.consoleScroll > 1) {
                editorState.consoleScroll = 1;
            }
            
            f32 target = 0.9f;
            f32 amount = Lerp(0.0f, target, editorState.consoleScroll);

            DrawShapeSetColor(glm::vec4(1, 1, 1, 0.1f));
            DrawShapeRect(
                glm::vec2(0.0f, (1 - amount) * mainSurfaceHeight),
                glm::vec2(mainSurfaceWidth, mainSurfaceHeight)
            );
        }
        else {
            editorState.consoleScroll = 0.0f;
        }
    }

    VertexBuffer LeEngine::SubmitVertexBuffer(i32 sizeBytes, const void* data, VertexLayoutType layoutType, bool dyanmic) {
        VertexBuffer buffer = {};
        buffer.size = sizeBytes;

        glGenVertexArrays(1, &buffer.vao);
        glGenBuffers(1, &buffer.vbo);

        glBindVertexArray(buffer.vao);

        glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
        glBufferData(GL_ARRAY_BUFFER, buffer.size, data, dyanmic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

        switch (layoutType) {
            case VERTEX_LAYOUT_TYPE_SHAPE: {
                buffer.stride = sizeof(ShapeVertex);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
            } break;

            case VERTEX_LAYOUT_TYPE_SPRITE: {
                buffer.stride = sizeof(SpriteVertex);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, false, buffer.stride, (void*)(2 * sizeof(f32)));
                glVertexAttribPointer(2, 4, GL_FLOAT, false, buffer.stride, (void*)((2 + 2) * sizeof(f32)));
            } break;

            case VERTEX_LAYOUT_TYPE_FONT: {
                buffer.stride = sizeof(FontVertex);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, false, buffer.stride, (void*)(2 * sizeof(f32)));
            } break;

            case VERTEX_LAYOUT_TYPE_DEBUG_LINE: {
                buffer.stride = sizeof(DebugLineVertex);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
                glVertexAttribPointer(1, 4, GL_FLOAT, false, buffer.stride, (void*)(2 * sizeof(f32)));
            } break;

            default: {
                Assert(0, "");
            }
        }

        glBindVertexArray(0);

        return buffer;
    }

    ShaderProgram LeEngine::SubmitShaderProgram(const char* vertexSource, const char* fragmentSource) {
        ShaderProgram program = {};

        u32 vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        if (!GLCheckShaderCompilationErrors(vertexShader)) {
            return {};
        }

        u32 fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        if (!GLCheckShaderCompilationErrors(fragmentShader)) {
            return {};
        }

        program.programHandle = glCreateProgram();
        glAttachShader(program.programHandle, vertexShader);
        glAttachShader(program.programHandle, fragmentShader);
        glLinkProgram(program.programHandle);
        if (!GLCheckShaderLinkErrors(program.programHandle)) {
            return {};
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    u32 LeEngine::SubmitTextureR8B8G8A8(i32 width, i32 height, byte* data, i32 wrapMode, bool generateMipMaps) {
        u32 textureHandle = 0;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        if (generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        return textureHandle;
    }

    u32 LeEngine::SubmitAudioClip(i32 sizeBytes, byte* data, i32 channels, i32 bitDepth, i32 sampleRate) {
        u32 alFormat = ALGetFormat(channels, bitDepth);

        u32 buffer = 0;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, alFormat, data, (ALsizei)sizeBytes, (ALsizei)sampleRate);

        ALCheckErrors();

        return buffer;
    }

    void LeEngine::ALCheckErrors() {
        ALCenum error = alGetError();
        if (error != AL_NO_ERROR) {
            switch (error)
            {
            case AL_INVALID_NAME:
                ATTOERROR("AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL ");
                break;
            case AL_INVALID_ENUM:
                ATTOERROR("AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL ");
                break;
            case AL_INVALID_VALUE:
                ATTOERROR("AL_INVALID_VALUE: an invalid value was passed to an OpenAL ");
                break;
            case AL_INVALID_OPERATION:
                ATTOERROR("AL_INVALID_OPERATION: the requested operation is n");
                break;
            case AL_OUT_OF_MEMORY:
                ATTOERROR("AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out ");
                break;
            default:
                ATTOERROR("UNKNOWN AL ERROR: ");
            }
            ATTOERROR("");
        }
    }

    u32 LeEngine::ALGetFormat(u32 numChannels, u32 bitDepth) {
        b8 sterio = numChannels > 1;
        if (sterio) {
            if (bitDepth == 8) {
                return AL_FORMAT_STEREO8;
            }
            else if (bitDepth == 16) {
                return AL_FORMAT_STEREO16;
            }
        }
        else {
            if (bitDepth == 8) {
                return AL_FORMAT_MONO8;
            }
            else if (bitDepth == 16) {
                return AL_FORMAT_MONO16;
            }
        }

        ATTOERROR("Unsupported audio format");

        return 0;
    }

    bool LeEngine::GLCheckShaderCompilationErrors(u32 shader) {
        i32 success;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            ATTOERROR("ERROR::SHADER_COMPILATION_ERROR of type: ");
            ATTOERROR(infoLog);
            ATTOERROR("-- --------------------------------------------------- -- ");
        }

        return success;
    }

    bool LeEngine::GLCheckShaderLinkErrors(u32 program) {
        i32 success;
        char infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            ATTOERROR("ERROR::SHADER_LINKER_ERROR of type: ");
            ATTOERROR(infoLog);
            ATTOERROR("-- --------------------------------------------------- -- ");
        }

        return success;
    }

    void LooseAssetLoader::RegisterAssets() {
        List<LargeString> texturePaths;
        FindAllFiles(basePathSprites.GetCStr(), ".png", texturePaths);

        List<LargeString> audioPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ogg", audioPaths);
        FindAllFiles(app->looseAssetPath.GetCStr(), ".wav", audioPaths);

        List<LargeString> fontPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ttf", fontPaths);

        const i32 textureCount = texturePaths.GetNum();
        for (i32 texturePathIndex = 0; texturePathIndex < textureCount; ++texturePathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_TEXTURE;

            LargeString path = texturePaths[texturePathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            path.RemovePathPrefix(basePathSprites.GetCStr());
            asset.id = AssetId::Create(path.GetCStr());

            asset.texture = TextureAsset::CreateDefault();

            engineAssets.Add(asset);

            ATTOTRACE("Found texture asset: %s", path.GetCStr());
        }

        const i32 audioCount = audioPaths.GetNum();
        for (i32 audioPathIndex = 0; audioPathIndex < audioCount; ++audioPathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_AUDIO;

            LargeString path = audioPaths[audioPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            asset.audio = AudioAsset::CreateDefault();

            engineAssets.Add(asset);

            ATTOTRACE("Found audio asset: %s", path.GetCStr());
        }

        const i32 fontCount = fontPaths.GetNum();
        for (i32 fontPathIndex = 0; fontPathIndex < fontCount; ++fontPathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_FONT;

            LargeString path = fontPaths[fontPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            asset.font = FontAsset::CreateDefault();

            engineAssets.Add(asset);

            ATTOTRACE("Found font asset: %s", path.GetCStr());
        }


        
        
    }

    bool LooseAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {

        textureAsset.generateMipMaps = false;

        void* pixelData = stbi_load(name, &textureAsset.width, &textureAsset.height, &textureAsset.channels, 4);

        if (!pixelData) {
            ATTOTRACE("Failed to load texture asset %s", name);
            return false;
        }

        glGenTextures(1, &textureAsset.textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureAsset.textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureAsset.width, textureAsset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        if (textureAsset.generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureAsset.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureAsset.wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureAsset.generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(pixelData);

        return true;
    }

    bool LooseAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
        LargeString filename = LargeString::FromLiteral(name);
        if (filename.Contains(".ogg")) {
            if (LoadOGG(filename.GetCStr(), audioAsset)) {
                return true;
            }
        }
        else if (filename.Contains(".wav")) {
            if (LoadWAV(filename.GetCStr(), audioAsset)) {
                return true;
            }
        }
        else {
            ATTOTRACE("Unsupported audio file type: %s", filename.GetCStr());
        }

        return false;
    }

    bool LooseAssetLoader::LoadFontAsset(const char* filename, FontAsset& fontAsset) {
        FT_Library ft = {};
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return false;
        }

        FT_Face face;
        if (FT_New_Face(ft, filename, 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            return false;
        }

        FT_Set_Pixel_Sizes(face, 0, fontAsset.fontSize);

        TileSheetGenerator tileSheet = {};

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }

            Glyph character = {};
            character.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            character.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            character.advance = face->glyph->advance.x;

            tileSheet.AddTile(character.size.x, character.size.y, face->glyph->bitmap.buffer);

            fontAsset.glyphs.Add(character);
        }

        List<byte> pixels;
        tileSheet.GenerateTiles(pixels, fontAsset.width, fontAsset.height);
        for (byte c = 0; c < 128; c++) {
            tileSheet.GetTileUV(c, fontAsset.glyphs[c].uv0, fontAsset.glyphs[c].uv1);
        }

        fontAsset.textureHandle = SubmitTextureR8B8G8A8(fontAsset.width, fontAsset.height, pixels.GetData(), GL_REPEAT, true);

        //Bitmap::Write(fontAsset.textureAsset.data.GetData(), fontAsset.textureAsset.width, fontAsset.textureAsset.height, "fontyboi.bmp");

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        return true;
    }

    bool LooseAssetLoader::LoadWAV(const char* filename, AudioAsset& audioAsset) {
        AudioFile<f32> audioFile;
        bool loaded = audioFile.load(filename);
        if (!loaded) {
            std::cout << "Failed to load audio file " << filename << std::endl;
            return false;
        }

        audioAsset.bitDepth = audioFile.getBitDepth();
        audioAsset.channels = audioFile.getNumChannels();
        audioAsset.sampleRate = audioFile.getSampleRate();

        std::vector<u8> loadedData;
        audioFile.savePCMToBuffer(loadedData);

        audioAsset.sizeBytes = (i32)loadedData.size();

        audioAsset.bufferHandle = SubmitAudioClip(audioAsset.sizeBytes, loadedData.data(), audioAsset.channels, audioAsset.bitDepth, audioAsset.sampleRate);
        
        return true;
    }

    bool LooseAssetLoader::LoadOGG(const char* filename, AudioAsset& audioAsset) {
        audioAsset.channels = 0;
        audioAsset.sampleRate = 0;
        audioAsset.bitDepth = 16;
        i16* loadedData = nullptr;
        i32 decoded = stb_vorbis_decode_filename(filename, &audioAsset.channels, &audioAsset.sampleRate, &loadedData);
        if (loadedData == nullptr) {
            std::cout << "Failed to load audio file " << filename << std::endl;
            return false;
        }

        audioAsset.sizeBytes = decoded * audioAsset.channels * sizeof(i16);

        audioAsset.bufferHandle = SubmitAudioClip(audioAsset.sizeBytes, (byte*)loadedData, audioAsset.channels, audioAsset.bitDepth, audioAsset.sampleRate);
        
        return true;
    }

    void PackedAssetFile::PutData(byte* data, i32 size) {
        const i32 s = storedData.GetNum();
        storedData.SetNum(s + size, true);
        std::memcpy(storedData.GetData() + s, data, size);
    }

    void PackedAssetFile::GetData(byte* data, i32 size) {
        Assert(currentOffset + size <= storedData.GetNum(), "PackedAssetFile::Get: currentOffset + size > storedData.GetNum()");
        byte* p = storedData.GetData() + currentOffset;
        std::memcpy(data, p, size);
        currentOffset += size;
    }

    void PackedAssetFile::SerializeAsset(TextureAsset& textureAsset) {
        Serialize(textureAsset.width);
        Serialize(textureAsset.height);
        Serialize(textureAsset.channels);
        Serialize(textureAsset.wrapMode);
        Serialize(textureAsset.generateMipMaps);
        //Serialize(textureAsset.data);
    }

    void PackedAssetFile::SerializeAsset(AudioAsset& audioAsset) {
        Serialize(audioAsset.channels);
        Serialize(audioAsset.sampleRate);
        Serialize(audioAsset.sizeBytes);
        Serialize(audioAsset.bitDepth);
    }

    void PackedAssetFile::SerializeAsset(FontAsset& fontAsset) {
        Serialize(fontAsset.fontSize);
        //SerializeAsset(fontAsset.textureAsset);
        Serialize(fontAsset.glyphs);
    }

    void PackedAssetFile::Reset() {
        currentOffset = 0;
    }

    bool PackedAssetFile::Save(const char* name) {
        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("PackedAssetFile::Save -> Could not open file %s", name)
                return false;
        }

        file.write((char*)storedData.GetData(), storedData.GetNum());
        file.close();

        return true;
    }

    bool PackedAssetFile::Load(const char* name) {
        std::ifstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("PackedAssetFile::Load -> Could not open file %s", name);
            return false;
        }

        file.seekg(0, std::ios::end);
        const i32 size = (i32)file.tellg();
        file.seekg(0, std::ios::beg);

        storedData.SetNum(size, true);
        file.read((char*)storedData.GetData(), size);
        file.close();

        isLoading = true;

        return true;
    }

    void PackedAssetFile::Finished() {
        storedData.Clear();
    }

    //bool PackedAssetLoader::Begin() {
    //    texturePakFile.Load("assets/pixel_bois.pak");
    //    audioPakFile.Load("assets/noisy_bois.pak");
    //    fontPakFile.Load("assets/scribly_bois.pak");
    //    return true;
    //}
    //
    //bool PackedAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {
    //    texturePakFile.SerializeAsset(textureAsset);
    //    return true;
    //}
    //bool PackedAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
    //    audioPakFile.SerializeAsset(audioAsset);
    //    return true;
    //}
    //bool PackedAssetLoader::LoadFontAsset(const char* name, FontAsset& fontAsset) {
    //    fontPakFile.SerializeAsset(fontAsset);
    //    return true;
    //}
    //void PackedAssetLoader::End() {
    //    texturePakFile.Finished();
    //    audioPakFile.Finished();
    //    fontPakFile.Finished();
    //}
    
}

