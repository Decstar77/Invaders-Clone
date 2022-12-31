#include "SpaceInvaders.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <string>
#include <stdarg.h> 

#include <al/alc.h>
#include <al/al.h>

#include <glad/glad.h>

namespace atto {
#if SPACE_INVADERS
    
    template<typename T>
    static T PickRandom(T&& arg) {
        return arg;
    }

    template<typename _type_, typename... Args>
    static auto PickRandom(_type_&& arg, Args&&... args) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof...(args));
        int random_index = dis(gen);
        if (random_index == 0) {
            return arg;
        }
        else {
            return PickRandom(std::forward<Args>(args)...);
        }
    }

    static f32 Lerp(const f32 a, const f32 b, const f32 t) {
        return a + (b - a) * t;
    }

    static void StringFormatV(char* dest, size_t size, const char* format, va_list va_listp) {
        vsnprintf(dest, size, format, va_listp);
    }

    f32 RandomFloat(f32 min, f32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min, max);
        return dis(gen);
    }

    static std::string ReadEntireFile(const char* filePath) {
        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open()) {
            Assert(0, "File is not open");
            return "";
        }

        std::stringstream stream;
        stream << file.rdbuf();
        file.close();
        return stream.str();
    }


    void Console::KeyCallback(i32 key, i32 scancode, i32 action, i32 mods, void* dataPtr) {
        Console* console = (Console*)dataPtr;
        
        // If key is letter
        if (key >= KEY_CODE_A && key <= KEY_CODE_Z) {
            if (action == ACTION_PRESS || action == ACTION_REPEAT) {
                char c = 'a' + (key - KEY_CODE_A);
                if (console->text.GetLength() < console->text.CAPCITY) {
                    console->text.Add(c);
                }
            }
        }

        if (key == KEY_CODE_SPACE){
            if (action == ACTION_PRESS || action == ACTION_REPEAT) {
                if (console->text.GetLength() < console->text.CAPCITY) {
                    console->text.Add(' ');
                }
            }
        }

        if (key == KEY_CODE_BACKSPACE) {
            if (action == ACTION_PRESS || action == ACTION_REPEAT) {
                if (console->text.GetLength() > 0) {
                    console->text.RemoveCharacter(console->text.GetLength() - 1);
                }
            }
        }
    }

    static int Lua_Test(lua_State* L) {
        ATTOINFO("Lua calls me");

        if (lua_isfunction(L, 1)) {
            int a = 2;
            lua_CFunction f = lua_tocfunction(L, 1);
            const char * test = lua_tostring(L, 1);

            lua_pcall(L, 0, 0, 0);
        }



        return 0;
    }

    void SpaceInvaders::Initialize(AppState* window) {
        drawShader.Load();
        unitQuad.CreateUnitQuad();

        window->input->consoleKeyCallbackDataPointer = &console;
        window->input->consoleKeyCallback = Console::KeyCallback;

        firstScript.Load("assets/scripts/first.lua");
        firstScript.RegisterFunction("atto_test", Lua_Test);
        firstScript.SetGlobal("dt", 0.016f);
        firstScript.PrepareFunction("update_main_menu");
        firstScript.CallFunction(0, 0);

        useRawAssets = true;
        if (useRawAssets) {
            assetLoader = &looseAssetLoader;
        }
        else {
            assetLoader = &packedAssetLoader;
        }

        assetLoader->Initialize();

        LoadTexture("assets/sprites/ship_A.png", &texturePlayerShip, true, GL_REPEAT);
        LoadTexture("assets/sprites/meteor_detailedSmall.png", &textureSmallMeteor, true, GL_REPEAT);
        LoadTexture("assets/sprites/meteor_detailedLarge.png", &textureLargeMeteor, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Lasers/laserGreen13.png", &textureGreenLaser, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/Backgrounds/blue.png", &textureBackground, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Effects/star1.png", &textureStar1, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Effects/star2.png", &textureStar2, true, GL_REPEAT);
        LoadTexture("assets/sprites_01/PNG/Effects/star3.png", &textureStar3, true, GL_REPEAT);
        LoadTexture("assets/main/power_up_blue_strip14.png", &texturePowerupBlue, true, GL_REPEAT);
        
        LoadAudioClip("assets/music/peace_1.ogg", &audioMainMenuMusic);
        LoadAudioClip("assets/sounds/laserSmall_000.ogg", &audioLaser1);
        LoadAudioClip("assets/sounds/explosionCrunch_000.ogg", &audioExplosion1);

        LoadFont("assets/fonts/arial.ttf", &fontMain, 28);
        assetLoader->Shutdown();

        spritePlayer.texture = texturePowerupBlue;
        spritePlayer.animated = true;
        spritePlayer.animationDuration = 1.0f;
        spritePlayer.frameSize = glm::vec2(68, 67);
        spritePlayer.frameCount = 14;
        
        worldWidth = (f32)window->windowWidth;
        worldHeight = (f32)window->windowHeight;

        lineRenderer.Initialize(2048);
        fontRenderer.Initialize();
        spriteRenderer.Initialize();

        for (u32 i = 0; i < stars.GetCapcity(); i++) {
            Star star = {};
            star.position = glm::vec2(RandomFloat(0, worldWidth), RandomFloat(0, worldHeight));
            star.rotation = RandomFloat(0, 360);
            star.texture = PickRandom(&textureStar1, &textureStar2, &textureStar3);
            stars.Add(star);
        }

        //audioMainMenuMusic.Play();
        gameOver = true;
    }

    void SpaceInvaders::UpdateAndRender(AppState* app) {
        worldWidth = (f32)app->windowWidth;
        worldHeight = (f32)app->windowHeight;
        f32 dt = app->deltaTime;
        
        glm::vec2 worldDims = glm::vec2(worldWidth, worldHeight);

        glm::mat4 projection = glm::ortho(0.0f, (f32)app->windowWidth, 0.0f, (f32)app->windowHeight, -1.0f, 1.0f);
        drawShader.SetMat4f("projection", projection);

        //glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //BoxCollider box;
        //box.CreateFromCenterSize(glm::vec2(400, 400), glm::vec2(100, 100));
        //lineRenderer.PushLine(glm::vec2(0), glm::vec2(worldWidth, worldHeight), glm::vec4(0, 1, 0, 1));
        //lineRenderer.PushBox(box);
        //lineRenderer.Draw(projection);

        // Draw the background
        for (f32 x = 0; x < 5; x++) {
            for (f32 y = 0; y < 3; y++) {
                glm::vec2 pos = glm::vec2(x * textureBackground.width, y * textureBackground.height);
                //DrawSprite(pos, glm::vec2(textureBackground.width, textureBackground.height), 0.0f, &textureBackground);
            }
        }

        for (u32 starIndex = 0; starIndex < stars.GetCount(); starIndex++) {
            Star& star = stars[starIndex];
            DrawSprite(star.position, glm::vec2(star.texture->width * 0.25f, star.texture->height * 0.25f), star.rotation, star.texture);
        }

        spritePlayer.animationPlayhead += dt * 15.0f;
        if (spritePlayer.animationPlayhead > (f32)spritePlayer.frameCount) {
            spritePlayer.animationPlayhead = 0;
        }
        spritePlayer.frameIndex = (i32)(spritePlayer.animationPlayhead);

        //SpriteDrawEntry drawEntry = {};
        //drawEntry.sprite = spritePlayer;
        //drawEntry.position = glm::vec2(400, 400);
        //spriteRenderer.Push(drawEntry);
        //spriteRenderer.PushQuad(glm::vec2(400, 400), glm::vec2(100, 100), glm::vec4(1, 0, 0, 0.4));
        //spriteRenderer.Draw(projection);
#if 1
        if (gameOver) {
            if (app->input->keys[KEY_CODE_W]  && menuSelection != 0) {
                menuSelection = 0;
                menuSelectionUnderlinePercent = 0.0f;
            }
            if (app->input->keys[KEY_CODE_S] && menuSelection != 1) {
                menuSelection = 1;
                menuSelectionUnderlinePercent = 0.0f;
            }

            if ((IsKeyJustDown(app->input, KEY_CODE_E) || IsKeyJustDown(app->input, KEY_CODE_ENTER))) {
                if (menuSelection == 0) {
                    StartNewGame();
                    return;
                }
                else if (menuSelection == 1) {
                    app->shouldClose = true;
                }
            }

            menuSelectionUnderlinePercent = Lerp(menuSelectionUnderlinePercent, 1.0f, dt * 5.0f);
            menuSelectionUnderlinePercent = glm::clamp(menuSelectionUnderlinePercent, 0.0f, 1.0f);
                
            FontDrawEntry baseEntry = {};
            baseEntry.font = &fontMain;
            baseEntry.pivot = FontPivot::Center;
            baseEntry.text = StringBuilder::FormatLarge("Score %d", score);
            baseEntry.position = glm::vec2(worldWidth / 2.0f, worldHeight * 0.95f);
            
            FontDrawEntry entry = baseEntry;
            fontRenderer.Push(entry);

            entry = baseEntry;
            entry.text = "Play";
            entry.position = glm::vec2(worldWidth / 2.0f, worldHeight / 2.0f + 50);
            if (menuSelection == 0) { 
                entry.underlineThinkness = 2.0f;
                entry.underlinePercent = menuSelectionUnderlinePercent;
            }
            fontRenderer.Push(entry);

            entry = baseEntry;
            entry.text = "Quit";
            entry.position = glm::vec2(worldWidth / 2.0f, worldHeight / 2.0f - 50);
            if (menuSelection == 1) { 
                entry.underlineThinkness = 2.0f;
                entry.underlinePercent = menuSelectionUnderlinePercent;
            }
            fontRenderer.Push(entry);

            fontRenderer.Draw(projection);
            lineRenderer.Draw(projection);
        }
        else {
            if (app->input->keys[KEY_CODE_A]) {
                playerShip.position.x -= 300.0f * dt;
            }

            if (app->input->keys[KEY_CODE_D]) {
                playerShip.position.x += 300.0f * dt;
            }

            playerShip.position = glm::clamp(playerShip.position, glm::vec2(0, 0) + playerShip.size, glm::vec2(worldWidth, worldHeight) - playerShip.size);

            playerShip.shootCooldown -= dt;
            if (app->input->keys[KEY_CODE_SPACE]) {
                if (playerShip.shootCooldown <= 0.0f) {
                    playerShip.shootCooldown = 0.5f;
                    SpawnLaser(playerShip.position - glm::vec2(0, 20), 1.0f);
                }
            }

            BoxBounds playerBox;
            playerBox.CreateFromCenterSize(playerShip.position, playerShip.size);
            //lineRenderer.PushBox(playerBox);

            DrawSprite(playerShip.position, playerShip.size, playerShip.rotation, &texturePlayerShip);

            for (u32 meteorIndex = 0; meteorIndex < meteors.GetCount(); meteorIndex++) {
                Meteor& meteor = meteors[meteorIndex];
                meteor.position.y += 100 * dt;
                meteor.rotation += 25 * dt;

                BoxBounds meteorBox;
                meteorBox.CreateFromCenterSize(meteor.position, meteor.size);
                //lineRenderer.PushBox(meteorBox);

                bool shouldRemove = meteor.position.y > worldHeight + 100;

                if (playerBox.Intersects(meteorBox)) {
                    gameOver = true;
                    audioExplosion1.Play();
                    break;
                }

                for (u32 laserIndex = 0; laserIndex < lasers.GetCount(); laserIndex++) {
                    Laser& laser = lasers[laserIndex];
                    BoxBounds laserBox;
                    laserBox.CreateFromCenterSize(laser.position, laser.size);

                    if (laserBox.Intersects(meteorBox)) {
                        lasers.Remove(laserIndex);
                        laserIndex--;
                        shouldRemove = true;
                        audioExplosion1.Play();
                        score++;
                        break;
                    }
                }

                if (shouldRemove) {
                    meteors.Remove(meteorIndex);
                    meteorIndex--;
                }

                Texture* texture = meteor.isBig ? &textureLargeMeteor : &textureSmallMeteor;
                DrawSprite(meteor.position, meteor.size, meteor.rotation, texture);
            }

            for (u32 laserIndex = 0; laserIndex < lasers.GetCount(); laserIndex++) {
                Laser& laser = lasers[laserIndex];
                laser.position.y -= 300 * dt * laser.direction;
                DrawSprite(laser.position, laser.size, laser.rotation, &textureGreenLaser);

                if (laser.position.y > worldHeight - 100) {
                    lasers.Remove(laserIndex);
                    laserIndex--;
                }
                else if (laser.position.y < -50) {
                    lasers.Remove(laserIndex);
                    laserIndex--;
                }
            }

            waveTimer -= dt;
            if (waveTimer <= 0) {
                SpawnMeteorWave();
            }

            //for (i32 i = 0; i < fontMain.tileSheet.tiles.GetNum(); i++) {
            //    BoxBounds b;
            //    b.min = fontMain.tileSheet.tiles[i].uv0 * worldDims;
            //    b.max = fontMain.tileSheet.tiles[i].uv1 * worldDims;
            //    lineRenderer.PushBox(b);
            //}

            FontDrawEntry baseEntry = {};
            baseEntry.font = &fontMain;
            baseEntry.pivot = FontPivot::BottomLeft;
            baseEntry.text = StringBuilder::FormatLarge("Score %d", score);
            baseEntry.position = glm::vec2(worldWidth * 0.01f, worldHeight * 0.95f);
            fontRenderer.Push(baseEntry);

            fontRenderer.Draw(projection);
            lineRenderer.Draw(projection);
        }
#endif

        if (IsKeyJustDown(app->input, KEY_CODE_TLDA)) {
            console.isOpen = !console.isOpen;
        }

        if (console.isOpen) {
            console.cursorFlashTimer += dt;
            glm::vec2 base = glm::vec2(10, 10);
            if (console.cursorFlashTimer < 0.5f) {
                glm::vec2 pos = base + glm::vec2(fontRenderer.GetTextWidth(&fontMain, console.text.GetCStr()), 0);
                spriteRenderer.PushQuad(pos, glm::vec2(10, 30), glm::vec4(1));
            }

            if (console.cursorFlashTimer >= 1.0f) {
                console.cursorFlashTimer = 0.0f;
            }
            spriteRenderer.PushQuad(glm::vec2(0, 0), glm::vec2(worldWidth, 50), glm::vec4(0.2, 0.2, 0.4, 0.5f));
            spriteRenderer.Draw(projection);

            fontRenderer.Push(&fontMain, base, console.text.GetCStr());
            fontRenderer.Draw(projection);

        }
    }

    void SpaceInvaders::LoadTexture(const char* filename, Texture* texture, bool generateMipMaps, i32 wrapMode) {
        TextureAsset textureAsset = {};
        textureAsset.generateMipMaps = generateMipMaps;
        textureAsset.wrapMode = wrapMode;
        assetLoader->LoadTextureAsset(filename, textureAsset);
        texture->CreateFromAsset(textureAsset);
    }

    void SpaceInvaders::LoadAudioClip(const char* filename, AudioClip* audioClip) {
        AudioAsset audioAsset = {};
        assetLoader->LoadAudioAsset(filename, audioAsset);
        audioClip->CreateFromAsset(audioAsset);
    }

    void SpaceInvaders::LoadFont(const char* fileaname, Font* font, u32 fontSize) {
        FontAsset fontAsset = {};
        fontAsset.fontSize = fontSize;
        assetLoader->LoadFontAsset(fileaname, fontAsset);
        font->CreateFromAsset(fontAsset);
    }

    void SpaceInvaders::StartNewGame() {
        waveTimer = 0;
        difficulty = 10;
        gameOver = false;
        menuSelection = 0;
        score = 0;
        menuSelectionUnderlinePercent = 0.0f;

        // Create the player ship
        playerShip.position = glm::vec2(worldWidth / 2.0f, worldHeight * 0.87f);

        // Clear the meteors and lasers
        meteors.Clear();
        lasers.Clear();

        // Create the meteors
        SpawnMeteorWave();
    }

    void SpaceInvaders::DrawSprite(glm::vec2 position, glm::vec2 size, f32 rotation, Texture* texture) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(size, 1.0f));

        drawShader.SetMat4f("model", model);
        drawShader.SetSampler("ourTexture", 0);
        drawShader.Bind();
        texture->Bind(0);
        unitQuad.BindAndDraw();
    }

    void SpaceInvaders::SpawnMeteor(bool isBig) {
        f32 x = RandomFloat(0, worldWidth);
        f32 y = -RandomFloat(100.0f, worldHeight);

        Meteor meteor;
        meteor.position = glm::vec2(x, y);
        meteor.isBig = isBig;

#if 0
        ATTOINFO("Meteor spawned at " << x << ' ' << y);
#endif

        meteors.AddIfPossible(meteor);
    }

    void SpaceInvaders::SpawnLaser(const glm::vec2& position, f32 direction) {
        Laser laser;
        laser.direction = direction;
        laser.position = position;

        if (lasers.AddIfPossible(laser)) {
            audioLaser1.Play();
        }
    }

    void SpaceInvaders::SpawnMeteorWave() {
        ATTOTRACE("Spawning meteor wave, new difficulty = %d", difficulty + 2);
        for (u32 i = 0; i < difficulty; i++) {
            f32 x = RandomFloat(0.0f, 1.0f);
            SpawnMeteor(x < 0.2f);
        }

        waveTimer = 10.0f;
        difficulty += 2;
    }
    
#endif
}
