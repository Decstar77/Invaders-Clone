
local SPRITE_ORIGIN_BOTTOM_LEFT = 0
local SPRITE_ORIGIN_CENTER = 1

sprites = {
    player_one_ship_left = {
        texture = "space_shooter_03/player_blue_li_strip6",
        origin = SPRITE_ORIGIN_CENTER,
        frameSize = {98, 89},
        frameCount = 6
    },
    
    player_one_ship_right = {
        texture = "space_shooter_03/player_blue_re_strip6",
        origin = SPRITE_ORIGIN_CENTER,
        frameSize = {98, 89},
        frameCount = 6
    },

    blue_laser = {
        texture = "space_shooter_03/shot_blue_strip3",
        origin = SPRITE_ORIGIN_CENTER,
        frameSize = {92, 108},
        frameCount = 3
    },

    enemy_mine = {
        texture = "space_shooter_03/enemy_ball_ani_strip20",
        origin = SPRITE_ORIGIN_CENTER,
        frameSize = {198, 191},
        frameCount = 20,
        -- frameRate = 1,
        -- animated = true,
    },

    enemy_2 = {
        texture = "space_shooter_03/enemy_fighter_2_ani_strip22",
        origin = SPRITE_ORIGIN_CENTER,
        frameSize = {177, 168},
        frameCount = 22
    },

    enemy_3 = {
        texture = "space_shooter_03/enemy_fighter_3_ani_strip22",
        origin = SPRITE_ORIGIN_CENTER,
        frameSize = {101, 126},
        frameCount = 22
    },

    missle = {
        texture = "space_shooter_03/missle",
        frameSize = {29, 75},
        frameCount = 1
    }
}

json = require ('assets.scripts.json')

str = json.encode(sprites)

filewrite = io.open("scripts.json", "w")
filewrite:write(str)
filewrite:close()



