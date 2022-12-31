json = require("assets.scripts.json")

dt = 0
worldWidth = 1280
worlHeight = 720
key_s = 0
key_w = 0

paddle = {
    w = 10,
    h = 75,
    r = 1,
    g = 1,
    b = 1,
    a = 1
}

spr_ball = {
    w = 15,
    h = 15,
    r = 1,
    g = 1,
    b = 1,
    a = 1
}

player1 = {
    x = 100,
    y = 325
}

player2 = {
    x = 1180,
    y = 325
}

ball = {
    x = 1280 / 2,
    y = 720 / 2
}

math.randomseed(os.time())

PONG_ENTITY_TYPE_PLAYER_PADDEL = 1
PONG_ENTITY_TYPE_AI_PADDEL = 2
PONG_ENTITY_TYPE_BALL = 3

function create_pong_level(gs)

    atto_create_entity(gs, {
        type = PONG_ENTITY_TYPE_PLAYER_PADDEL,
        x = 100,
        y = 325,
        sprite = {
            w = 10,
            h = 75
        },
    })

    atto_create_entity(gs, {
        type = PONG_ENTITY_TYPE_AI_PADDEL,
        x = 1180,
        y = 325,
        sprite = {
            w = 10,
            h = 75
        }
    })

    atto_create_entity(gs, {
        type = PONG_ENTITY_TYPE_BALL,
        x = worldWidth / 2 - 10 / 2,
        y = worlHeight / 2 - 10 / 2,
        sprite = {
            w = 10,
            h = 10
        },
        speed = 250,
        dir = { math.random() , math.random() }
        -- dir = {-1, 1}
    })

    -- j = json.encode(e)
    -- print(j)

    -- e1 = json.decode(j)
    -- print(e1.sprite.w)
end

function draw_paddle(gs, x, y)
    atto_push_quad(gs, x, y, paddle.w, paddle.h, paddle.r, paddle.g, paddle.b, paddle.a);
end

function draw_pong(gs)

    atto_test(paddle)

    if (player1.y ~= player2.y) then
        player2.y = player1.y
    end

    player1.y = player1.y + 550 * dt * (key_w - key_s)

    draw_paddle(gs, player1.x, player1.y)
    draw_paddle(gs, player2.x, player2.y)
    atto_push_quad(gs, ball.x, ball.y, spr_ball.w, spr_ball.h, spr_ball.r, spr_ball.b, spr_ball.g, spr_ball.a)
end
