#pragma once

#include "AttoLib.h"
#include "AttoAsset.h"
#include "AttoLua.h"
#include "AttoRendering.h"

namespace atto 
{
    //inline static AudioAssetId LaserSmallSounds[]{
    //    AudioAssetId::Create("assets/sounds/sfx/laserSmall_000"),
    //    AudioAssetId::Create("assets/sounds/sfx/laserSmall_001"),
    //    AudioAssetId::Create("assets/sounds/sfx/laserSmall_002"),
    //    AudioAssetId::Create("assets/sounds/sfx/laserSmall_003"),
    //    AudioAssetId::Create("assets/sounds/sfx/laserSmall_004"),
    //};

    //inline static AudioAssetId LaserSmallSounds[] = {
    //    AudioAssetId::Create("assets/sounds/sci-fi_weapon_plasma_pistol_01"),
    //    AudioAssetId::Create("assets/sounds/sci-fi_weapon_plasma_pistol_02"),
    //    AudioAssetId::Create("assets/sounds/sci-fi_weapon_plasma_pistol_03"),
    //    AudioAssetId::Create("assets/sounds/sci-fi_weapon_plasma_pistol_04"),
    //    AudioAssetId::Create("assets/sounds/sci-fi_weapon_plasma_pistol_05"),
    //    AudioAssetId::Create("assets/sounds/sci-fi_weapon_plasma_pistol_06"),
    //};
    static constexpr i32 LaserSmallSoundCount = 23;
    inline static AudioAssetId LaserSmallSounds[] = {
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (1)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (2)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (3)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (4)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (5)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (6)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (7)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (8)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (9)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (10)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (11)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (12)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (13)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (14)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (15)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (16)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (17)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (18)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (19)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (20)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (21)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (22)"),
        AudioAssetId::Create("assets/sounds/blue_laser/blue_laser_ (23)")
    };


    enum EntityType {
        ENTITY_TYPE_INVALID =0,
        ENTITY_TYPE_PLAYER = 1,
        ENTITY_TYPE_BLUE_LASER = 2,
        ENTITY_TYPE_RED_LASER = 3,
        ENTITY_TYPE_MINE = 4,
    };

    struct Entity {
        EntityType type;
        glm::vec2 pos;
        glm::vec2 vel;
        i32 frameIndex;
        f32 frameTime;
        f32 fireRate;
        f32 fireTime;
        f32 laserSpeed;
    };

    class PhysicsSim : public GameState {
    public:
        bool                            Initialize(AppState* app) override;
        void                            Update(AppState* app) override;
        void                            Render(AppState* app) override;
        void                            Shutdown(AppState* app) override;

        f32         difficultyScores = 0.0f;
        bool        wave1Spawned = false;
        bool        wave2Spawned = false;

        

        Entity body;
        
        List<Entity> lasers;
        FixedList<Entity, 1000> entities;

        Entity SpawnMine(f32 x, f32 y);

        LeEngine* engine;
    };
}
