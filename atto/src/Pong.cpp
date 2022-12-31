
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

    bool Pong::Initialize(AppState* app) {
        engine = app->engine;

        LoadLuaScripts();

        debugRenderer.Initialize();
        fontRenderer.Initialize();
        spriteRenderer.Initialize();

        LoadLevel();
        
        return true;
    }
    
    bool Pong::LoadLuaScripts() {
        luaLogic.Free();

        if (luaLogic.Load("assets/scripts/pong.lua")) {
            luaLogic.RegisterFunction("atto_create_entity",  &Pong::Lua_CreateEntity);

            return true;
        }

        return false;
    }

    bool Pong::LoadLevel() {
        entities.Clear();

        if (LoadLuaScripts()) {
            luaLogic.CallVoidFunction("create_pong_level", this);

            if (!engine->AudioIsSpeakerAlive(speaker)) {
                speaker = engine->AudioPlay(AudioAssetId::Create("assets/music/battle_2"), true, 0.5f);
            }

            return true;
        }

        return false;
    }

    BoxBounds Pong::GetPlayerPaddleBounds() {
        BoxBounds bounds;
        bounds.min = entities[0].pos;
        bounds.max = bounds.min + entities[0].sprite.frameSize;

        return bounds;
    }

    BoxBounds Pong::GetAIPaddleBounds() {
        BoxBounds bounds;
        bounds.min = entities[1].pos;
        bounds.max = bounds.min + entities[1].sprite.frameSize;

        return bounds;
    }

    void Pong::Shutdown(AppState* app) {

    }

    void Pong::Update(AppState* app) {
        if (IsKeyJustDown(app->input, KEY_CODE_R)) {
            ATTOINFO("Reload level");
            LoadLevel();
            return;
        }


        if (!IsKeyJustDown(app->input, KEY_CODE_T) && !app->input->keys[KEY_CODE_G]) {
            //return;
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

                entity->pos.y = glm::clamp(entity->pos.y, 0.0f, app->windowHeight - entity->sprite.frameSize.y);

            } break;

            case PONG_ENTITY_TYPE_AI_PADDEL: {
                entity->pos.y = entities[2].pos.y - entity->sprite.frameSize.y * 0.5f;

                entity->pos.y = glm::clamp(entity->pos.y, 0.0f, app->windowHeight - entity->sprite.frameSize.y);

            } break;

            case PONG_ENTITY_TYPE_BALL: {
                BoxBounds playerPaddleBounds = GetPlayerPaddleBounds();
                BoxBounds aiPaddleBounds = GetAIPaddleBounds();
                
                                
                Ray2D ballRay = {};
                ballRay.origin = entity->pos + entity->sprite.frameSize * 0.5f;
                ballRay.direction = glm::normalize( entity->vel );
                
                f32 t;
                if (RayCast::Box(playerPaddleBounds, ballRay, t)) {
                    if (t > 0 && t < 10) {
                        entity->vel.x *= -1.0f;
                        engine->AudioPlay(AudioAssetId::Create("assets/sounds/laserLarge_000"));
                    }
                }

                if (RayCast::Box(aiPaddleBounds, ballRay, t)) {
                    if (t > 0 && t < 10) {
                        entity->vel.x *= -1.0f;
                        engine->AudioPlay(AudioAssetId::Create("assets/sounds/laserLarge_000"));
                    }
                }
                
                if (entity->pos.y < 0 || entity->pos.y > app->windowHeight - entity->sprite.frameSize.y) {
                    engine->AudioPlay(AudioAssetId::Create("assets/sounds/forceField_003"));
                    entity->vel.y = -entity->vel.y;
                }
                
                if (entity->pos.x < 0 || entity->pos.x > app->windowWidth) {
                    if (entity->pos.x < 0) {
                        player1Score++;
                    }
                    else {
                        player2Score++;
                    }
                    
                    engine->AudioPlay(AudioAssetId::Create("assets/sounds/forceField_000"));
                    LoadLevel();
                }

                entity->pos += entity->vel * app->deltaTime;

                BoxBounds ballBounds = {};
                ballBounds.min = entity->pos;
                ballBounds.max = entity->pos + entity->sprite.frameSize;

                //debugRenderer.PushBox(playerPaddleBounds);
                //debugRenderer.PushBox(aiPaddleBounds);
                //debugRenderer.PushBox(ballBounds);
                //debugRenderer.PushRay(ballRay);

            } break;

            default:
                break;
            }
        }
    }

    void Pong::Render(AppState* app) {

        const i32 entityCount = entities.GetCount();
        for (i32 i = 0; i < entityCount; ++i) {
            PongEntity* entity = &entities[i];
            switch (entity->type)
            {
            case PONG_ENTITY_TYPE_PLAYER_PADDEL: {
                SpriteDrawEntry spriteDrawEntry = {};
                spriteDrawEntry.sprite = entity->sprite;
                spriteDrawEntry.scale = entity->sprite.frameSize;
                spriteDrawEntry.position = entity->pos;
                spriteDrawEntry.drawKind = SPRITE_DRAW_KIND_QUAD;

                spriteRenderer.Push(spriteDrawEntry);
            } break;

            case PONG_ENTITY_TYPE_AI_PADDEL: {
                SpriteDrawEntry spriteDrawEntry = {};
                spriteDrawEntry.sprite = entity->sprite;
                spriteDrawEntry.scale = entity->sprite.frameSize;
                spriteDrawEntry.position = entity->pos;
                spriteDrawEntry.drawKind = SPRITE_DRAW_KIND_QUAD;

                spriteRenderer.Push(spriteDrawEntry);
            } break;

            case PONG_ENTITY_TYPE_BALL: {
                SpriteDrawEntry spriteDrawEntry = {};
                spriteDrawEntry.sprite = entity->sprite;
                spriteDrawEntry.scale = entity->sprite.frameSize;
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

        engine->DrawTextSetFont(FontAssetId::Create("assets/fonts/arial"));
        engine->DrawText(StringFormat::Small("%d", player1Score), glm::vec2(app->windowWidth - 50, app->windowHeight - 50));
        engine->DrawText(StringFormat::Small("%d", player2Score), glm::vec2(20, app->windowHeight - 50));

        spriteRenderer.Draw(projection);
        engine->DEBUGSubmit();
    }

#define CHECK_PARAM_COUNT(x) if (lua_gettop(L) != x) { ATTOERROR("Invalid number of parameters for function %s", __func__); return -1; }
#define GET_GAME_STATE() Pong *pong = (Pong*)LuaScript::GetUserData(L, 1)
    
    int Pong::Lua_CreateEntity(lua_State* L) {
        CHECK_PARAM_COUNT(2);
        GET_GAME_STATE();

        PongEntity entity = {};

        LuaTable def(L, 2);
        def["type"].Get(entity.type);
        def["x"].Get(entity.pos.x);
        def["y"].Get(entity.pos.y);
        
        SmallString spriteAssetName = {};
        entity.sprite = SpriteAsset::CreateDefault();
        def["sprite"]["texture"].Get(spriteAssetName);
        def["sprite"]["w"].Get(entity.sprite.frameSize.x);
        def["sprite"]["h"].Get(entity.sprite.frameSize.y);

        if (spriteAssetName.GetLength()) {
            TextureAssetId id = TextureAssetId::Create(spriteAssetName.GetCStr());
            entity.sprite.texture = pong->engine->LoadTextureAsset(id);
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
}

#endif