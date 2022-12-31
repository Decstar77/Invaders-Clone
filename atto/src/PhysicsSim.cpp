#include "PhysicsSim.h"

namespace atto
{
    bool PhysicsSim::Initialize(AppState* app) {
        engine = app->engine;
        
        body = {};
        body.pos = glm::vec2(0, -200);
        body.vel = glm::vec2(0);
        
        //engine->AudioPlay(AudioAssetId::Create("assets/sounds/music/battle_1"));

        return true;
    }

    void PhysicsSim::Update(AppState* app) {
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

        //static f32 fireRate = 0.0f;
      
        i32 sCount = 23;
        //fireRate = glm::clamp(fireRate - app->deltaTime, 0.0f, 1000.0f);
        //
        //if (app->input->keys[KEY_CODE_SPACE] && fireRate <= 0.0f) {
        //    i32 index = engine->RandomInt(0, sCount - 1);
        //    Assert(index < sCount, "index out of range");
        //    
        // 

        //   
        //    Entity bullet = {};
        //    bullet.type = ENTITY_TYPE_BUTTET;
        //    bullet.pos = body.pos + glm::vec2(0, 40);

        //    bullets.Add(bullet);

        //    fireRate = 0.09f;
        //}


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
        body.pos += body.vel ;
    }

    void PhysicsSim::Render(AppState* app) {
        engine->DrawClearSurface();
        engine->DrawEnableAlphaBlending();

        //engine->DrawSprite(SpriteInstance, pos, frameIndex, )

        engine->DrawShapeSetColor(glm::vec4(0.1, 0.1, 0.1, 1));
        engine->DrawShapeRectCenterDim(glm::vec2(0, 0), glm::vec2(1280, 720));


        engine->DrawShapeSetColor(glm::vec4(1));
        engine->DrawShapeRectCenterDim(glm::vec2(0, 0), glm::vec2(100));
        engine->DrawShapeRectCenterDim(glm::vec2(580, -300), glm::vec2(100));


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

        engine->DrawTextSetFont(FontAssetId::Create("assets/fonts/arial"));
        engine->DrawTextSetHalign(FONT_HALIGN_CENTER);
        engine->DrawText(StringFormat::Small("%.2f %.2f", body.vel.x, body.frameTime), body.pos + glm::vec2(0, 100));

        if (body.frameIndex >= 0) {
            engine->DrawSprite(AssetId::Create("player_one_ship_right"), body.pos, body.frameIndex);
        }
        else {
            engine->DrawSprite(AssetId::Create("player_one_ship_left"), body.pos, glm::abs(body.frameIndex));
        }

        const i32 bulletCount = bullets.GetNum();
        for (i32 i = 0; i < bulletCount; i++) {
            Entity& bullet = bullets[i];
            engine->DrawSprite(AssetId::Create("missle"), bullet.pos, 0);
        }
        

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
}

