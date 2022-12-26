
#include "Pong.h"
#include <random>
#include <glad/glad.h>

#if PONG
namespace atto
{
    static f32 RandomFloat(f32 min, f32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min, max);
        return dis(gen);
    }
    
    static int Lua_Test1(lua_State* L) {
        glm::vec2 size;
        glm::vec4 color;
        LuaScript::GetTableValue(L, "w", 1, size.x);
        LuaScript::GetTableValue(L, "h", 1, size.y);
        LuaScript::GetTableValue(L, "r", 1, color.r);
        LuaScript::GetTableValue(L, "g", 1, color.g);
        LuaScript::GetTableValue(L, "b", 1, color.b);
        LuaScript::GetTableValue(L, "a", 1, color.a);

        return 0;
    }

    bool Pong::Initialize(AppState* app) {
        assetRegistry = app->assetRegistry;

        LoadLuaScripts();

        lineRenderer.Initialize();
        fontRenderer.Initialize();
        spriteRenderer.Initialize();

        LoadLevel();
        
        return true;
    }
    
    bool Pong::LoadLuaScripts() {
        luaLogic.Free();

        if (luaLogic.Load("assets/scripts/pong.lua")) {
            luaLogic.SetFunction("atto_push_quad",      &Pong::Lua_PushQuad);
            luaLogic.SetFunction("atto_create_entity",  &Pong::Lua_CreateEntity);
            luaLogic.SetFunction("atto_test",           &Lua_Test1);

            return true;
        }

        return false;
    }

    bool Pong::LoadLevel() {
        entities.Clear();

        if (LoadLuaScripts()) {
            luaLogic.CallVoidFunction("create_pong_level", this);

            test = assetRegistry->LoadTextureAsset(AssetId::Create("assets/sprites/star_tiny"));
            
            return true;
        }

        return false;
    }

    BoxBounds Pong::GetPlayerPaddleBounds() {
        BoxBounds bounds;
        bounds.min = entities[0].pos;
        bounds.max = bounds.min + entities[0].sprite.drawScale;

        return bounds;
    }

    BoxBounds Pong::GetAIPaddleBounds() {
        BoxBounds bounds;
        bounds.min = entities[1].pos;
        bounds.max = bounds.min + entities[1].sprite.drawScale;

        return bounds;
    }

    void Pong::Shutdown(AppState* app) {

    }

    void Pong::UpdateAndRender(AppState* app) {
        if (IsKeyJustDown(app->input, KEY_CODE_R)) {
            ATTOINFO("Reload level");
            LoadLevel();
        }

        luaLogic.SetGlobal("dt",            app->deltaTime);
        luaLogic.SetGlobal("worldWidth",    app->windowWidth);
        luaLogic.SetGlobal("worlHeight",    app->windowHeight);
        luaLogic.SetGlobal("key_s",         app->input->keys[KEY_CODE_S]);
        luaLogic.SetGlobal("key_w",         app->input->keys[KEY_CODE_W]);

        const i32 entityCount = entities.GetCount();
        for (i32 i = 0; i < entityCount; ++i) {
            PongEntity* entity = &entities[i];
            switch (entity->type)
            {
            case PONG_ENTITY_TYPE_PLAYER_PADDEL: {
                if (app->input->keys[KEY_CODE_W]) {
                    entity->pos.y += 200.0f * app->deltaTime;
                }
                
                if (app->input->keys[KEY_CODE_S]) {
                    entity->pos.y -= 200.0f * app->deltaTime;
                }

                entity->pos.y = glm::clamp(entity->pos.y, 0.0f, app->windowHeight - entity->sprite.drawScale.y);

                SpriteDrawEntry spriteDrawEntry = {};
                spriteDrawEntry.sprite = entity->sprite;
                spriteDrawEntry.scale = entity->sprite.drawScale;
                spriteDrawEntry.position = entity->pos;
                spriteDrawEntry.drawKind = SPRITE_DRAW_KIND_QUAD;

                spriteRenderer.Push(spriteDrawEntry);
            } break;

            case PONG_ENTITY_TYPE_AI_PADDEL: {
                entity->pos.y = entities[2].pos.y;

                entity->pos.y = glm::clamp(entity->pos.y, 0.0f, app->windowHeight - entity->sprite.drawScale.y);

                SpriteDrawEntry spriteDrawEntry = {};
                spriteDrawEntry.sprite = entity->sprite;
                spriteDrawEntry.scale = entity->sprite.drawScale;
                spriteDrawEntry.position = entity->pos;
                spriteDrawEntry.drawKind = SPRITE_DRAW_KIND_QUAD;

                spriteRenderer.Push(spriteDrawEntry);
            } break;

            case PONG_ENTITY_TYPE_BALL: {
                static glm::vec2 ballDirection = glm::vec2(RandomFloat(0, 1), RandomFloat(0, 1));
                static glm::vec2 ballVelocity = glm::dot(glm::vec2(-200.0f, -200.0f), glm::normalize(ballDirection)) * glm::normalize(ballDirection);
         
                BoxBounds ballBounds = {};
                ballBounds.min = entity->pos;
                ballBounds.max = entity->pos + entity->sprite.drawScale;

                BoxBounds playerPaddleBounds = GetPlayerPaddleBounds();
                BoxBounds aiPaddleBounds = GetAIPaddleBounds();
                
                lineRenderer.PushBox(ballBounds);
                lineRenderer.PushBox(playerPaddleBounds);
                lineRenderer.PushBox(aiPaddleBounds);
                

                if (ballBounds.min.x < playerPaddleBounds.max.x && ballBounds.max.x > playerPaddleBounds.min.x) {
                    if (ballBounds.min.y < playerPaddleBounds.max.y && ballBounds.max.y > playerPaddleBounds.min.y) {
                        ballVelocity.x = -ballVelocity.x;
                    }
                }

                if (ballBounds.min.x < aiPaddleBounds.max.x && ballBounds.max.x > aiPaddleBounds.min.x) {
                    if (ballBounds.min.y < aiPaddleBounds.max.y && ballBounds.max.y > aiPaddleBounds.min.y) {
                        ballVelocity.x = -ballVelocity.x;
                    }
                }

                entity->pos += ballVelocity * app->deltaTime;

                if (entity->pos.y < 0 || entity->pos.y > app->windowHeight - entity->sprite.drawScale.y) {
                    ballVelocity.y = -ballVelocity.y;
                }

                SpriteDrawEntry spriteDrawEntry = {};
                spriteDrawEntry.sprite = entity->sprite;
                spriteDrawEntry.scale = entity->sprite.drawScale;
                spriteDrawEntry.position = entity->pos;
                spriteDrawEntry.drawKind = SPRITE_DRAW_KIND_QUAD;

                spriteRenderer.Push(spriteDrawEntry);
            } break;

            default:
                break;
            }
        }

        glm::mat4 projection = glm::ortho(0.0f, (f32)app->windowWidth, 0.0f, (f32)app->windowHeight, -1.0f, 1.0f);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        
        fontRenderer.Draw(projection);
        spriteRenderer.Draw(projection);
        lineRenderer.Draw(projection);
    }

#define LUA_CHECK_PARAM_COUNT(x) if (lua_gettop(L) != x) { ATTOERROR("Invalid number of parameters for function %s", __func__); return -1; }
#define GET_GAME_STATE() Pong *pong = (Pong*)LuaScript::GetUserData(L, 1)
    
    int Pong::Lua_CreateEntity(lua_State* L) {
        LUA_CHECK_PARAM_COUNT(2);
        GET_GAME_STATE();

        PongEntity entity = {};

        LuaTable def(L, 2);
        def["type"].Get(entity.type);
        def["x"].Get(entity.pos.x);
        def["y"].Get(entity.pos.y);
        
        SmallString spriteAssetName = "";
        entity.sprite = SpriteAsset::Create();
        def["sprite"]["texture"].Get(spriteAssetName);
        def["sprite"]["w"].Get(entity.sprite.drawScale.x);
        def["sprite"]["h"].Get(entity.sprite.drawScale.y);

        if (spriteAssetName.GetLength()) {
            AssetId id = AssetId::Create(spriteAssetName.GetCStr());
            entity.sprite.texture = pong->assetRegistry->LoadTextureAsset(id);
        }

        glm::vec2 dir = glm::vec2(0, 0);
        if (def["dir"].Get(dir)) {
            f32 speed = 200.0f;
            def["speed"].Get(speed);

            entity.vel = glm::normalize(dir) * speed;
        }

        pong->entities.Add(entity);

        return 0;
    }

    int Pong::Lua_PushQuad(lua_State* L) {
        if (lua_gettop(L) != 9) {
            return -1;
        }

        Pong* pong = (Pong*)lua_touserdata(L, 1);
        f32 x = (f32)lua_tonumber(L, 2);
        f32 y = (f32)lua_tonumber(L, 3);
        f32 w = (f32)lua_tonumber(L, 4);
        f32 h = (f32)lua_tonumber(L, 5);
        f32 r = (f32)lua_tonumber(L, 6);
        f32 g = (f32)lua_tonumber(L, 7);
        f32 b = (f32)lua_tonumber(L, 8);
        f32 a = (f32)lua_tonumber(L, 9);

        pong->spriteRenderer.PushQuad(glm::vec2(x, y), glm::vec2(w, h), glm::vec4(r, g, b, a));

        return 0;
    }
}

#endif