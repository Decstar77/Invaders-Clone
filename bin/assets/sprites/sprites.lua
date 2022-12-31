
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

    missle = {
        texture = "space_shooter_03/missle",
        frameSize = {29, 75},
        frameCount = 1
    }
}
