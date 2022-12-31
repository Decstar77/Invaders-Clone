#include "PhysicsSim.h"

namespace atto
{
    bool PhysicsSim::Initialize(AppState* app) {
        engine = app->engine;
        
        body = {};
        body.pos = glm::vec2(0, -engine->mainSurfaceHeight / 2.0f + 100.0f);
        body.vel = glm::vec2(0);
        
        //engine->AudioPlay(AudioAssetId::Create("assets/sounds/music/battle_1"));

        return true;
    }

    void PhysicsSim::Update(AppState* app) {

        const i32 laserCount = lasers.GetNum();
        for (i32 laserIndex = 0; laserIndex < laserCount; laserIndex++) {
            Entity& laser = lasers[laserIndex];
            laser.pos += glm::vec2(0, 1000) * app->deltaTime;
        }

        const i32 entityCount = entities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < entityCount; entityIndex++) {
            Entity& entity = entities[entityIndex];
            switch (entity.type) {
            case ENTITY_TYPE_MINE: {
                entity.frameTime += app->deltaTime * 20.0f;
                entity.frameIndex = (i32)entity.frameTime % 20;

                entity.pos -= glm::vec2(0, 100) * app->deltaTime;
                }
            }
        }

        f32 acc = 55.0f;
        glm::vec2 imp = glm::vec2(0);
        if (app->input->keys[KEY_CODE_W]) {
            //body.imp.y = acc;
        }
        if (app->input->keys[KEY_CODE_S]) {
            //body.imp.y = -acc;
        }
        if (app->input->keys[KEY_CODE_A]) {
            imp.x = -acc;
        }
        if (app->input->keys[KEY_CODE_D]) {
            imp.x = acc;
        }
        
        body.fireRate = 0.2f;
        body.fireTime = glm::max(body.fireTime - app->deltaTime, 0.0f);
        if (app->input->keys[KEY_CODE_SPACE] && body.fireTime <= 0.0f) {
            i32 index = engine->RandomInt(0, LaserSmallSoundCount - 1);
            Assert(index < LaserSmallSoundCount, "index out of range");

            Entity laser = {};
            laser.type = ENTITY_TYPE_BLUE_LASER;
            laser.frameIndex = 0;
            laser.pos = body.pos + glm::vec2(0, 40);

            lasers.Add(laser);

            body.fireTime = body.fireRate;
        }

        difficultyScores += app->deltaTime;

#define IN_RANGE(x, min, max) ((x) >= (min) && (x) <= (max))

        if (IN_RANGE(difficultyScores, 5, 10) && wave1Spawned == false) {
            ATTOTRACE("Spawning wave 1");
            
            //f32 y = engine->Random((f32)engine->mainSurfaceHeight + 250.0f, (f32)engine->mainSurfaceHeight + 1000.0f);
            f32 y = (f32)engine->mainSurfaceHeight + engine->Random() * 250.0f;
            
            for (i32 i = 0; i < 4; i++) {
                f32 x = -engine->mainSurfaceWidth / 2.0f;
                if (i % 2 == 0) {
                    x += 125;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                }
                else {
                    x += 275;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                    x += 300;
                    entities.Add(SpawnMine(x, y));
                }
                y += 350;
            }
            wave1Spawned = true;
        }

        if (IN_RANGE(difficultyScores, 25, 50) && wave2Spawned == false) {
            ATTOTRACE("Spawning wave 2");
            wave2Spawned = true;
        }

        //i32 sCount = 23;
        //static f32 timer = 0.0f;
        //static bool play = false;
        //if (IsKeyJustDown(app->input, KEY_CODE_1)) {
        //    play = true;
        //    engine->AudioPlay(AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_charge"));
        //}

        //if (play) {
        //    timer += app->deltaTime;
        //}
        //
        //static i32 sIndex = 0;
        //static f32 fireRate = 0.0f;
        //static i32 playCount = 1;
        //fireRate -= app->deltaTime;
        //if (timer > 1.8f) {
        //    if (fireRate < 0.0) {
        //        if (sIndex == sCount) {
        //            if (playCount == 0) {
        //                engine->AudioPlay(AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_end"));
        //                timer = 0;
        //                play = false;
        //                playCount = 2;
        //            }
        //            else {
        //                sIndex = 0;
        //            }
        //        }
        //        else {
        //            engine->AudioPlay(LaserSmallSounds[sIndex]);
        //            ATTOINFO("Playing %d", sIndex);
        //            sIndex++;
        //            if (sIndex == sCount) {
        //                playCount--;
        //            }
        //            
        //            fireRate = 0.09f;
        //        }
        //    }
        //}

        body.vel *= 0.9f;
        body.vel += imp * app->deltaTime;
        body.vel.x = glm::clamp(body.vel.x, -7.0f, 7.0f);
        body.pos += body.vel;

        bool moved = false;
        f32 m = 50.0f;
        if (app->input->keys[KEY_CODE_D]) {
            body.frameTime += app->deltaTime * m;
            moved = true;
        }

        if (app->input->keys[KEY_CODE_A]) {
            body.frameTime -= app->deltaTime * m;
            moved = true;
        }

        if (!moved) {
            body.frameTime = Lerp(0.0f, body.frameTime, app->deltaTime * m);
        }

        body.frameTime = glm::clamp(body.frameTime, -5.0f, 5.0f);
        body.frameIndex = (i32)body.frameTime;
    }

    void PhysicsSim::Render(AppState* app) {
        
        //engine->DrawSprite(SpriteInstance, pos, frameIndex, )

        engine->DrawShapeSetColor(glm::vec4(0.1, 0.1, 0.1, 1));
        engine->DrawShapeRectCenterDim(glm::vec2(0, 0), glm::vec2(engine->mainSurfaceWidth, engine->mainSurfaceHeight));


        engine->DrawShapeSetColor(glm::vec4(1));
        //engine->DrawShapeRectCenterDim(glm::vec2(0, 0), glm::vec2(100));
        //engine->DrawShapeRectCenterDim(glm::vec2(580, -300), glm::vec2(100));

        const i32 laserCount = lasers.GetNum();
        for (i32 laserIndex = 0; laserIndex < laserCount; laserIndex++) {
            Entity& laser = lasers[laserIndex];
            engine->DrawSprite(AssetId::Create("blue_laser"), laser.pos, laser.frameIndex);
        }

        engine->DrawTextSetFont(FontAssetId::Create("assets/fonts/arial"));
        engine->DrawTextSetHalign(FONT_HALIGN_CENTER);
        engine->DrawText(StringFormat::Small("diffScore %.2f", difficultyScores), glm::vec2(0, engine->mainSurfaceHeight / 2 - 100));
        engine->DrawText(StringFormat::Small("%.2f %.2f", body.vel.x, body.frameTime), body.pos + glm::vec2(0, 100));

        if (body.frameIndex >= 0) {
            engine->DrawSprite(AssetId::Create("player_one_ship_right"), body.pos, body.frameIndex);
        }
        else {
            engine->DrawSprite(AssetId::Create("player_one_ship_left"), body.pos, glm::abs(body.frameIndex));
        }

        const i32 entityCount = entities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < entityCount; entityIndex++) {
            Entity& entity = entities[entityIndex];
            switch (entity.type) {
                case ENTITY_TYPE_MINE: {
                    //static f32 frameTime = 0;
                    //frameTime += app->deltaTime * 20.0f;
                    //i32 frameIndex = (i32)frameTime % 25;
                    engine->DrawSprite(AssetId::Create("enemy_mine"), entity.pos, entity.frameIndex);
                }
            }
        }

        engine->DrawSprite(AssetId::Create("enemy_2"), glm::vec2(0, 0), 0);

        //engine->DrawSprite(AssetId::Create("missle"), glm::vec2(0, 0), 0);

        //engine->DrawShapeRectCenterDim(glm::vec2(700, 200), glm::vec2(100, 100));
        //engine->DrawShapeRectCenterDim(glm::vec2(900, 200), glm::vec2(50));
        ////engine->DrawShapeRect(glm::vec2(300, 200), glm::vec2(400, 200));
        //

        //engine->DrawShapeCircle(glm::vec2(50), 10);

        engine->EditorToggleConsole();
        //engine->DrawShapeRect(glm::vec2(400, 400), glm::vec2(600, 600));
    }

    void PhysicsSim::Shutdown(AppState* app) {

    }

    Entity PhysicsSim::SpawnMine(f32 x, f32 y) {
        Entity mine = {};
        mine.type = ENTITY_TYPE_MINE;
        mine.frameTime = engine->Random(0, 7);
        mine.pos = glm::vec2(x, y);
        mine.vel = glm::vec2(0, 0);

        ATTOINFO("pos %.2f %.2f", x, y);

        return mine;
    }

}

