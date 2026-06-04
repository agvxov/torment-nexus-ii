#include "pch.hpp"

// ---

using namespace std;

const int W = 1800;
const int H = 900;

/* The nature of the game is such a monolith is in the center
 *  and all stalkers move towards it
 */
Vector2 CENTER = (Vector2){
    (float) W / 4,
    (float) H / 2,
};

unsigned long long TICK = 0;
const int TICKS_PER_SECOND = 60; // we have no lag compensation, but thats fine

typedef struct {
    float x;
    float y;
    float w;
    float h;
    Color color;
    int render_layer;
} hitbox_t;
// ---

#define SANDYBROWN (Color) {188, 143, 143, 255}

// ---

const char * game_messages[] = {
    "In the year of our Lord 1972, they came.\n"
    "6 Visitor ships surrounded the planet.\n"
    ,
    "After 2 days, they left without a word.\n"
    "But they left behind the monoliths.\n"
    ,
    "I am responsible for the Farming\nof the Siberian Zone.\n"
    ,
    "WOOOOOOAAAAAAAAAAAAAAAAAAAAAAHHH\n"
    "BUUUUUUDDDYYYYYYYYYYYYYYYYYYYYYY!"
};

// i still have C++ enums
[[maybe_unused]] constexpr int WELCOME        = 0;
[[maybe_unused]] constexpr int SHIP_LEAVING   = 1;
[[maybe_unused]] constexpr int GAMEPLAY       = 2;
[[maybe_unused]] constexpr int ENDTIMES       = 3;
[[maybe_unused]] constexpr int GAME_STATE_END = 4;
int game_state = WELCOME;

bool is_ufo_gone = false;

bool is_endable = false;

// ---
// price functions
float my_square(float f) {
    return f * f;
}
float my_times_two(float f) {
    return f * 2;
}
float my_times_10(float f) {
    return f * 10;
}
float my_slow_rise(float f) {
    return f * 1.5;
}
float my_retarded_sigmoid(float f) {
    return (1.0 / (1.0 + exp(-((f - 20.0) / 10.0)))) * 300.0;
}
float my_const(float f) {
    return f;
}
// ---

// XXX
unsigned long long blood = 0;
//unsigned long long blood = 1'000;
unsigned long long blood_multiplier = 1;

struct speed_t {
    float speed;
};

long stalker_cap = 10'000;
long n_stalkers = 0;
float stalker_spawns_per_second = 0.0f;
unsigned long long spawn_rate_price[] = {3};

Vector2 selection_point_a = {0,0};
Vector2 selection_point_b = {W/2,H};
Rectangle spawn_rect = (Rectangle) {
    .x      = 0,
    .y      = 0,
    .width  = W/2,
    .height = H,
};

unsigned long long multi_price[] = {10};

int n_bursters = 0;
unsigned long long burster_price[] = {3};

struct erradiation_t {
    bool is_erradiated = false;
};
int n_radareas = 0;
unsigned long long radarea_price[] = {20};

int n_gravtraps = 0;
unsigned long long gravtrap_price[] = {50};


struct librarian_movement_t {
    hitbox_t shadow_hb;
    Vector2 direction = {0,0};
    int sneed = 60;
    entt::entity prey = entt::null;
};
int n_librarians = 0;
unsigned long long librarian_price[] = {500};

// ---

//bool clip_mode = false;
bool clip_mode = false;
Texture2D clip_texture;

// ---

class splatter_effect_t final {
  public:
    static constexpr int half_life = 60;
    unsigned long long birth_tick;

    static constexpr int n_blood_particles = 5;
    class BloodParticle final {
      public:
        const float G = 98.0f / 200;
        Color color;

        Vector2 position;
        Vector2 velocity;

        void init_random_trajectory(void) {
            velocity.y = (rand() % 10) + 1;
            velocity.x = (rand() % (2*2)) - 2;
        }

        void update(void) {
            position.x += velocity.x;
            position.y -= velocity.y;

            velocity.y -= G;
        }

        void draw(void) {
            if (clip_mode) {
                DrawTexturePro(
                    clip_texture,
                    (Rectangle) {
                        0,
                        0,
                        (float)clip_texture.width,
                        (float)clip_texture.height
                    },
                    (Rectangle) {
                        position.x,
                        position.y,
                        (float)20,
                        (float)20,
                    },
                    {0,0},
                    0.0f,
                    WHITE
                );
            } else {
                DrawRectangle(
                    position.x,
                    position.y,
                    4,
                    4,
                    color
                );
            }
        }
    } blood_particles[n_blood_particles];

    splatter_effect_t(int x, int y, Color c) {
        birth_tick = TICK;
        for (int i = 0; i < n_blood_particles; i++) {
            blood_particles[i].position.x = x;
            blood_particles[i].position.y = y;
            blood_particles[i].color      = c;
            blood_particles[i].init_random_trajectory();
        }
    }

    void update(void) {
        for (int i = 0; i < n_blood_particles; i++) {
            blood_particles[i].update();
        }
    }

    void draw(void) {
        for (int i = 0; i < n_blood_particles; i++) {
            blood_particles[i].draw();
        }
    }

    bool should_be_destroyed(void) {
        return birth_tick + half_life < TICK;
    }
};
// ---

#include "anomali_collision_grid.hpp"

enum {
    LAYER_GROUND,
    LAYER_STALKER,
    LAYER_BUILDING,
    LAYER_AIR,
    LAYER_END,
};

Rectangle hitbox2rect(hitbox_t hb) {
    return (Rectangle) {
        .x      = hb.x,
        .y      = hb.y,
        .width  = hb.w,
        .height = hb.h,
    };
}

static inline int rand_between(int min, int max) { return min + rand() % (max - min + 1); }

static inline
void seek_point(Vector2 c, float & x, float & y, float speed = 0.5) {
    float dx = c.x - x;
    float dy = c.y - y;

    float len = sqrt(dx * dx + dy * dy);
    if (len == 0.0f) { return; }

    x += (dx / len) * speed;
    y += (dy / len) * speed;
}

static inline
void wrap_position(float & x, float & y) {
    if (x < 0) {
        x = W/2 - x;
    } else
    if (x > W/2) {
        x = 0 + (x - W/2);
    } else
    if (y < 0) {
        y = H - y;
    } else
    if (y > H) {
        y = 0 + (y - H);
    }
}

entt::registry entities;

void init_splatter_effect(int x, int y, Color c) {
    const auto e = entities.create();
    entities.emplace<splatter_effect_t>(e, x, y, c);
}

int kill_stalker(entt::entity s) {
    int r;
    const hitbox_t hitbox  = entities.get<const hitbox_t>(s);
    const erradiation_t er = entities.get<const erradiation_t>(s);
    entities.destroy(s);

    r = 1 * blood_multiplier * (er.is_erradiated + 1);
    blood += r;

    init_splatter_effect(
        hitbox.x,
        hitbox.y,
        er.is_erradiated ? GREEN : RED
    );

    return r;
}

typedef struct { ; } stalker_tag;
typedef struct { ; } monolith_tag;
typedef struct { ; } ufo_tag;
typedef struct { ; } burster_tag;
typedef struct { ; } radarea_tag;
typedef struct { ; } gravtrap_tag;
typedef struct { ; } librarian_tag;

static inline
Rectangle to_raylib_rect(hitbox_t hb) {
    return (Rectangle) {
        hb.x,
        hb.y,
        hb.w,
        hb.h
    };
}

static inline
bool does_collide(hitbox_t a, hitbox_t b) {
    return CheckCollisionRecs(
        to_raylib_rect(a),
        to_raylib_rect(b)
    );
}

static inline
void move_stalkers(void) {
    const Vector2 stalker_point = { CENTER.x, CENTER.y - 2 };
    auto stalkers = entities.view<const stalker_tag, hitbox_t, const speed_t>();

    for (auto s : stalkers) {
        auto & hitbox = stalkers.get<hitbox_t>(s);
        auto & sp     = stalkers.get<speed_t>(s);
        seek_point(stalker_point, hitbox.x, hitbox.y, sp.speed);
    }
}

static inline
void move_librarian(entt::entity l) {
    auto & hb = entities.get<hitbox_t>(l);
    auto & m  = entities.get<librarian_movement_t>(l);

    const int avg_direction_change_tick_duration = 90;

    if (m.prey != entt::null) {
        if (not entities.valid(m.prey)) {
            m.prey = entt::null;
            return;
        }

        auto p_hb = entities.get<hitbox_t>(m.prey);
        if (fabs(p_hb.x - hb.x) < 8.0f
        &&  fabs(p_hb.y - hb.y) < 8.0f) {
            auto b = kill_stalker(m.prey);
            blood += b * b * b;

            init_splatter_effect(
                hb.x,
                hb.y,
                YELLOW
            );
            
            m.prey = entt::null;
        } else {
            seek_point({p_hb.x, p_hb.y}, hb.x, hb.y, 5.0f);
        }
        return;
    }

    if (m.sneed == 0) {
        m.sneed = 60;

        hitbox_t new_shadow_hb;
        const int i = 100;
        new_shadow_hb.w = 20;
        new_shadow_hb.h = 20;
        new_shadow_hb.x = hb.x + (rand() % i) - (i / 2);
        new_shadow_hb.y = hb.y + (rand() % i) - (i / 2);
        new_shadow_hb.color = Fade(ORANGE, 0.3f);

        register_modified_anomali_collision(
            l,
            new_shadow_hb,
            m.shadow_hb
        );
        m.shadow_hb = new_shadow_hb;

        return;
    } else {
        --m.sneed;
    }

    if (rand() % avg_direction_change_tick_duration == 0) {
        if (not (m.direction.x == 0 && m.direction.y == 0)
        &&  rand() % 2) {
            m.direction = {0,0};
        } else {
            m.direction.x = ((rand() % 10) * 0.1) - (10 * 0.1 / 2);
            m.direction.y = ((rand() % 10) * 0.1) - (10 * 0.1 / 2);
        }
    }

    hb.x += m.direction.x;
    hb.y += m.direction.y;

    wrap_position(hb.x, hb.y);
}

static inline
void move_librarians(void) {
    auto librarians = entities.view<const librarian_tag>();

    for (auto l : librarians) {
        move_librarian(l);
    }
}

static inline
int spawns_this_tick(float spawn_rate) {
    float ticks_per_spawn = (float)TICKS_PER_SECOND / spawn_rate;

    float total_now  = (float)TICK / ticks_per_spawn;
    float total_prev = (float)(TICK - 1) / ticks_per_spawn;

    int spawn_now = (int)(total_now) - (int)(total_prev);

    return spawn_now;
}

static inline
Vector2 rand_position_in_spawn_rect(void) {
    return (Vector2) {
        .x = (float)spawn_rect.x + rand_between(0, spawn_rect.width),
        .y = (float)spawn_rect.y + rand_between(0, spawn_rect.height),
    };
}

void spawn_burster(void) {
    auto position = rand_position_in_spawn_rect();

    const auto burster = entities.create();
    entities.emplace<burster_tag>(burster);
    auto hb = entities.emplace<hitbox_t>(burster,
        (float)position.x,
        (float)position.y,
        (float)30,
        (float)30,
        GRAY,
        LAYER_GROUND
    );
    register_new_anomali_collision(burster, hb);

    ++n_bursters;
}

void spawn_radarea(void) {
    auto position = rand_position_in_spawn_rect();

    const auto radarea = entities.create();
    entities.emplace<radarea_tag>(radarea);

    auto hb = entities.emplace<hitbox_t>(radarea,
        (float)position.x,
        (float)position.y,
        (float)50,
        (float)50,
        Fade(GREEN, 0.4),
        LAYER_GROUND
    );
    register_new_anomali_collision(radarea, hb);

    ++n_radareas;
};

void spawn_gravtrap(void) {
    auto position = rand_position_in_spawn_rect();

    const auto gravtrap = entities.create();
    entities.emplace<gravtrap_tag>(gravtrap);

    auto hb = entities.emplace<hitbox_t>(gravtrap,
        (float)position.x,
        (float)position.y,
        (float)20,
        (float)20,
        Fade(BLUE, 0.8),
        LAYER_BUILDING
    );
    register_new_anomali_collision(gravtrap, hb);

    ++n_gravtraps;
};

void spawn_librarian(void) {
    auto position = rand_position_in_spawn_rect();

    const auto librarian = entities.create();
    entities.emplace<librarian_tag>(librarian);

    entities.emplace<hitbox_t>(librarian,
        (float)position.x,
        (float)position.y,
        (float)8,
        (float)8,
        ORANGE,
        LAYER_BUILDING
    );

    auto m = entities.emplace<librarian_movement_t>(librarian);
    register_new_anomali_collision(librarian, m.shadow_hb);

    ++n_librarians;
};

void increase_multi(void) {
    blood_multiplier *= 2;
}

void increase_spawn_rate(void) {
    stalker_spawns_per_second *= 1.5;
}

typedef void  (*procedure_fn)(void);
typedef float (*math_function_fn)(float);
void null_fn(void) { ; }

void buy(
    long long unsigned * price,
    procedure_fn f,
    math_function_fn h
) {
    assert(price);

    if (blood >= *price) {
        blood -= *price;
        f();
        *price = h(*price);
    }
}

static inline
void spawn_stalker(void) {
    const Color stalker_color = BLACK;
    const auto stalker = entities.create();

    int correction = 10;

    float x, y;
    switch (rand() % 4) {
        case 0: {
            x = rand_between(0, W/2);
            y = 0 + correction;
        } break;
        case 1: {
            x = 0 + correction;
            y = rand_between(0, H);
        } break;
        case 2: {
            x = W/2 - correction;
            y = rand_between(0, H);
        } break;
        case 3: {
            x = rand_between(0, W/2);
            y = H - correction;
        } break;
    }

    entities.emplace<stalker_tag>(stalker);
    entities.emplace<hitbox_t>(stalker,
        (float)x,
        (float)y,
        (float)6,
        (float)6,
        stalker_color,
        LAYER_STALKER
    );
    entities.emplace<erradiation_t>(stalker);
    entities.emplace<speed_t>(stalker, 0.5f);
}

static inline
void spawn_stalkers(void) {
    if (n_stalkers >= stalker_cap) { return; }

    int n = spawns_this_tick(stalker_spawns_per_second);

    if (n == 0) { return; }

    // XXX there is a batch create overload
    for (int i = 0; i < n; i++) {
        spawn_stalker();
    }
}

static inline
entt::entity get_monolith(void) {
    auto view = entities.view<monolith_tag>();
    return view.empty() ? entt::null : *view.begin();
}

static inline
void burster_touch(entt::entity burster, entt::entity stalker) {
    kill_stalker(stalker);
}

static inline
void radarea_touch(entt::entity radarea, entt::entity stalker) {
    auto & er = entities.get<erradiation_t>(stalker);
    if (er.is_erradiated) {
        return;
    } else {
        er.is_erradiated = true;
        auto & hb = entities.get<hitbox_t>(stalker);
        hb.color = GREEN;
    }
}

static inline
void gravtrap_touch(entt::entity radarea, entt::entity stalker) {
    auto & sp = entities.get<speed_t>(stalker);
    sp.speed = 5.0f;
}

static inline
void librarian_touch(entt::entity librarian, entt::entity stalker) {
    auto & m = entities.get<librarian_movement_t>(librarian);

    if (m.prey != entt::null) {
        return;
    }

    auto & sp = entities.get<speed_t>(stalker);
    sp.speed = 0.0f;
    m.prey = stalker;
}

static inline
void anomali_touch(void) {
    n_stalkers = 0;

    auto stalkers = entities.view<const stalker_tag, hitbox_t>();

    for (auto s : stalkers) {
        ++n_stalkers;
        auto & stalker_hitbox = stalkers.get<hitbox_t>(s);

        const auto & as = colliding_anomalies(stalker_hitbox.x, stalker_hitbox.y);

        for (const auto & a : as) {
            hitbox_t anomali_hitbox = entities.get<hitbox_t>(a);

            if (entities.all_of<librarian_tag>(a)) {
                anomali_hitbox = entities.get<const librarian_movement_t>(a).shadow_hb;
            }

            if (CheckCollisionRecs(hitbox2rect(stalker_hitbox), hitbox2rect(anomali_hitbox))) {
                if (entities.all_of<burster_tag>(a)) {
                    burster_touch(a, s);
                } else
                if (entities.all_of<radarea_tag>(a)) {
                    radarea_touch(a, s);
                } else
                if (entities.all_of<gravtrap_tag>(a)) {
                    gravtrap_touch(a, s);
                } else
                if (entities.all_of<librarian_tag>(a)) {
                    librarian_touch(a, s);
                }
                break;
            }
        }
    }
}

static inline
void monolith_touch(void) {
    auto monolith = get_monolith();
    hitbox_t monolith_hitbox = entities.get<hitbox_t>(monolith);

    auto stalkers = entities.view<const stalker_tag, const hitbox_t>();

    for (auto s : stalkers) {
        hitbox_t s_hitbox = stalkers.get<hitbox_t>(s);
        if (does_collide(monolith_hitbox, s_hitbox)) {
            kill_stalker(s);
        }
    }
}

static inline
void bump_effects(void) {
    auto es = entities.view<splatter_effect_t>();

    for (auto e : es) {
        auto & ef = es.get<splatter_effect_t>(e);
        if (ef.should_be_destroyed()) {
            entities.destroy(e);
            continue;
        }
        ef.update();
    }
}

static inline
void move_ufo(void) {
    auto es = entities.view<ufo_tag>();

    if (es.empty()) {
        return;
    }

    entt::entity ufo = *(es.begin());
    hitbox_t & ufo_hitbox = entities.get<hitbox_t>(ufo);

    if (game_state > WELCOME) {
        ufo_hitbox.x += 0.3f * 2;
        ufo_hitbox.y -= 1 * 2;
    }

    if (ufo_hitbox.x + ufo_hitbox.w < 0
    ||  ufo_hitbox.y + ufo_hitbox.h < 0) {
        entities.destroy(ufo);
        is_ufo_gone = true;
    }
}

static inline
void draw_entities(void) {
    auto es = entities.view<const hitbox_t>();

    for (int i = 0; i < LAYER_END; i++) {
        for (auto e : es) {
            auto & hitbox = es.get<hitbox_t>(e);
    
            if (hitbox.render_layer != i) {
                continue;
            }
            
            if (clip_mode) {
                DrawTexturePro(
                    clip_texture,
                    (Rectangle) {
                        0,
                        0,
                        (float)clip_texture.width,
                        (float)clip_texture.height
                    },
                    (Rectangle) {
                        hitbox.x,
                        hitbox.y,
                        hitbox.w * 3,
                        hitbox.h * 3,
                    },
                    {0,0},
                    0.0f,
                    WHITE
                );
            } else {
                DrawRectangle(
                    (float)hitbox.x,
                    (float)hitbox.y,
                    (float)hitbox.w,
                    (float)hitbox.h,
                    hitbox.color
                );
            }
        }
    }

    do {
        DrawRectangle(
            selection_point_a.x,
            selection_point_a.y,
            3,
            3,
            PINK
        );

        DrawRectangle(
            selection_point_b.x,
            selection_point_b.y,
            3,
            3,
            PURPLE
        );
    } while (0);

    do {
        auto es = entities.view<splatter_effect_t>();
        for (auto e : es) {
            auto ef = es.get<splatter_effect_t>(e);
            ef.draw();
        }
    } while (0);
}

void debug_draw_librarian_shadow_hitboxes(void) {
    auto librarians = entities.view<const librarian_tag>();

    for (auto l : librarians) {
        const auto hb = entities.get<hitbox_t>(l);
        const auto m  = entities.get<librarian_movement_t>(l);
        DrawRectangleRec(
            hitbox2rect(m.shadow_hb),
            m.shadow_hb.color
        );
        DrawLine(
            hb.x,
            hb.y,
            m.shadow_hb.x,
            m.shadow_hb.y,
            m.shadow_hb.color
        );
    }
}

static inline
void smite(void) {
    auto stalkers = entities.view<const stalker_tag>();

    if (stalkers.begin() == stalkers.end()) {
        return;
    }

    auto s = *(stalkers.begin());
    kill_stalker(s);
}

#include "buttons.hpp"
#include "gui.h"

void usage(void) {
    puts("./torment-nexus-ii.out [--help|--skip-intro]");
}

void setup_gameplay(void) {
    is_ufo_gone = true;
    stalker_spawns_per_second = 0.5f;
}

int main(int argc, const char * argv[]) {
    bool do_skip_intro = false;

    if (argc > 2) {
        usage();
        return 1;
    } else
    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            usage();
            return 0;
        } else
        if (!strcmp(argv[1], "--skip-intro")) {
            do_skip_intro = true;
        }
    }

    srand(0);

    InitWindow(W, H, "Torment Nexus II");
    SetTargetFPS(TICKS_PER_SECOND);

    clip_texture = LoadTexture("DO_NOT_OPEN_THIS_FOLDER/legally_distinct_paperclip_character.png");
    assert(clip_texture.id != 0 && "Texture failed to load");

    const auto monolith = entities.create();
    entities.emplace<monolith_tag>(monolith);
    entities.emplace<hitbox_t>(monolith,
        (float)CENTER.x - (10 / 2),
        (float)CENTER.y - 40,
        (float)10,
        (float)40,
        GOLD,
        LAYER_BUILDING
    );

    const auto ufo = entities.create();
    entities.emplace<ufo_tag>(ufo);
    entities.emplace<hitbox_t>(ufo,
        (float)0 - 50,
        (float)0,
        (float)W,
        (float)H + 200,
        BLACK,
        LAYER_AIR
    );

    if (do_skip_intro) {
        entities.destroy(ufo);
        setup_gameplay();
        game_state = GAMEPLAY;
    }

    while (!WindowShouldClose()) {
        // Input
        Vector2 mouse_position = GetMousePosition();

        if (mouse_position.x < W / 2) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selection_point_a = mouse_position;
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                selection_point_b = mouse_position;
            }

            spawn_rect = (Rectangle) {
                .x = (selection_point_a.x < selection_point_b.x)
                   ? selection_point_a.x
                   : selection_point_b.x,
                .y = (selection_point_a.y < selection_point_b.y)
                   ? selection_point_a.y
                   : selection_point_b.y,
                .width  = fabsf(selection_point_b.x - selection_point_a.x),
                .height = fabsf(selection_point_b.y - selection_point_a.y),
            };
        }

        // Update
        spawn_stalkers();
        move_stalkers();
        move_librarians();
        monolith_touch();
        anomali_touch();
        bump_effects();
        move_ufo();
        if (blood > 6'000'000) {
            is_endable = true;
        }
        ++TICK;

        // Draw
        BeginDrawing();
            ClearBackground(WHITE);

            BeginScissorMode(0, 0, W/2, H);
                draw_entities();
                //debug_draw_librarian_shadow_hitboxes(); // XXX
            EndScissorMode();

            DrawText(
                TextFormat(
                    "FPS: %d | S: %ld",
                    (int)GetFPS(),
                    n_stalkers
                ),
                10,
                10,
                24,
                BLACK
            );

            BeginScissorMode(W/2, 0, W, H);
                ClearBackground(LIGHTGRAY);
            EndScissorMode();

            do {
                bool b;
                switch (game_state) {
                    case WELCOME: {
                        b = draw_message(game_messages[game_state], 1, "Next");
                    } break;
                    case SHIP_LEAVING: {
                        b = draw_message(game_messages[game_state], is_ufo_gone, "Next");
                        if (b) {
                            setup_gameplay();
                        }
                    } break;
                    case GAMEPLAY: {
                        b = draw_message(game_messages[game_state], is_endable, "Ascend");
                    } break;
                    case ENDTIMES: {
                        b = draw_message(game_messages[game_state], 0, NULL);
                        clip_mode = true;
                    } break;
                }
                if (b) {
                    ++game_state;
                }
            } while (0);

            draw_stats();

            auto gui_rect = get_gui_rect();
            const int button_x_start = 40;
            const int button_gap_x = 220;
            const int button_gap_y = 120;
            int button_x = button_x_start;
            int button_y = 200;

            for (int i = 0; i < sizeof(buttons)/sizeof(button_t); i++) {
                draw_button(
                    gui_rect.x + button_x,
                    button_y,
                    &buttons[i]
                );
                if ((i+1) % 4 == 0) {
                    button_y += button_gap_y;
                    button_x  = button_x_start;
                } else {
                    button_x += button_gap_x;
                }
            }

            if (clip_mode) {
                bool b = GuiButton(
                    (Rectangle) {
                        1200,
                        600,
                        300,
                        150,
                    },
                    "I AM ENLIGHTED."
                );
                if (b) {
                    raise(SIGSEGV);
                }
            }


        EndDrawing();
    }

    CloseWindow();
    return 0;
}
