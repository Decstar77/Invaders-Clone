#pragma once

#include "AttoLib.h"
#include "AttoAsset.h"
#include "AttoLua.h"
#include "AttoRendering.h"

#define PONG 1
#if PONG

namespace atto
{
    enum PongEntityType {
        PONG_ENTITY_TYPE_INVALID = 0,
        PONG_ENTITY_TYPE_PLAYER_PADDEL = 1,
        PONG_ENTITY_TYPE_AI_PADDEL = 2,
        PONG_ENTITY_TYPE_BALL = 3,
    };

    struct PongEntity {
        i32                    type = PONG_ENTITY_TYPE_INVALID;
        glm::vec2              pos = glm::vec2(0, 0);
        glm::vec2              vel = glm::vec2(0, 0);
        glm::vec2              acc = glm::vec2(0, 0);
        f32                    mass = 1.0f;
        SpriteAsset            sprite;
    };

    class Pong : public GameState {
    public:
        bool                            Initialize(AppState* app) override;
        void                            UpdateAndRender(AppState* app) override;
        void                            Shutdown(AppState* app) override;

        bool                            LoadLuaScripts();
        bool                            LoadLevel();
        
        BoxBounds                       GetPlayerPaddleBounds();
        BoxBounds                       GetAIPaddleBounds();

        static int                      Lua_CreateEntity(lua_State* L);
        static int                      Lua_PushQuad(lua_State* L);

        const TextureAsset* test;

        LuaScript                       luaLogic;
        FixedList<PongEntity, 256>      entities;

        AssetRegistry*                  assetRegistry;

        LineRenderer                    lineRenderer;
        FontRenderer                    fontRenderer;
        SpriteRenderer                  spriteRenderer;
    };
}

#endif

