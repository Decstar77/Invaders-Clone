#pragma once

#include "AttoLib.h"
#include "AttoAsset.h"
#include "AttoLua.h"
#include "AttoRendering.h"

namespace atto
{
#define SPACE_INVADERS 0
#if SPACE_INVADERS
    class Console {
    public:
        static void KeyCallback(i32 key, i32 scancode, i32 action, i32 mods, void *dataPtr);

        LargeString             text = "";
        bool                    isOpen = false;
        f32                     cursorFlashTimer = 0.0f;
    };

    class PlayerShip {
    public:
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               velocity = glm::vec2(0,0);
        glm::vec2               size = glm::vec2(25);
        f32                     rotation = 0.0f;
        f32                     shootCooldown = 0.0f;
    };

    class Meteor {
    public:
        bool                    isBig = false;
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               velocity = glm::vec2(0, 0);
        glm::vec2               size = glm::vec2(25);
        f32                     rotation = 0.0f;
    };

    class Laser {
    public:
        glm::vec2               position = glm::vec2(0, 0);
        glm::vec2               size = glm::vec2(9, 37) * 0.3f;
        f32                     rotation = 0.0f;
        f32                     direction;
    };

    class Star {
    public:
        glm::vec2               position = glm::vec2(0, 0);
        f32                     rotation = 0.0f;
        Texture*                texture = nullptr;
    };

    class Level {
    public:
        void Start();
        void End();
    };

    class CosmicCombat {
        bool                    Initialize();

        LeEngine*            assetLoader = nullptr;
        LooseAssetLoader        looseAssetLoader;
        PackedAssetLoader       packedAssetLoader;

        DebugRenderer            lineRenderer;
        FontRenderer            fontRenderer;
        SpriteRenderer          spriteRenderer;
    };

    class SpaceInvaders {
    public:
        void Initialize(AppState* app);
        void UpdateAndRender(AppState *app);

        void LoadTexture(const char* filename, Texture* texture, bool generateMipMaps, i32 wrapMode);
        void LoadAudioClip(const char* filename, AudioClip* audioClip);
        void LoadFont(const char* fileaname, Font* font, u32 fontSize);

        void StartNewGame();

        void SpawnMeteor(bool isBig);
        void SpawnLaser(const glm::vec2& position, f32 direction);

        void SpawnMeteorWave();

        void DrawSprite(glm::vec2 position, glm::vec2 size, f32 rotation, Texture* texture);
        
        Console                 console;

        LuaScript               firstScript;

        LooseAssetLoader        looseAssetLoader;
        PackedAssetLoader       packedAssetLoader;
        LeEngine*            assetLoader = nullptr;

        DebugRenderer            lineRenderer;
        FontRenderer            fontRenderer;
        SpriteRenderer          spriteRenderer;

        SpriteAsset                  spritePlayer;

        PlayerShip              playerShip;
        FixedList<Meteor, 32>   meteors;
        FixedList<Laser, 512>   lasers;
        FixedList<Star, 7>      stars;

        bool                    useRawAssets;

        f32                     worldWidth;
        f32                     worldHeight;
        f32                     waveTimer;
        u32                     difficulty;
        u32                     score;
        bool                    gameOver;
        f32                     menuSelectionUnderlinePercent;
        i32                     menuSelection;

        Program                 drawShader;
        Program                 fontShader;
        Mesh                    unitQuad;
        Texture                 texturePlayerShip;
        Texture                 textureSmallMeteor;
        Texture                 textureLargeMeteor;
        Texture                 textureGreenLaser;
        Texture                 textureBackground;
        Texture                 textureStar1;
        Texture                 textureStar2;
        Texture                 textureStar3;
        Texture                 texturePowerupBlue;

        AudioClip              audioMainMenuMusic;
        AudioClip              audioLaser1;
        AudioClip              audioExplosion1;
        
        Font                   fontMain;
    };

#endif
}
