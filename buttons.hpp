#pragma once

typedef struct {
    bool is_enabled;
    const char * text;
    const char * (*tooltip)(void);
    void (*on_click)(void);
} button_t;

button_t b_smite = (button_t){
    .is_enabled = true,
    .text       = "Smite",
    .tooltip    = []() -> const char * {
        return "Carry the will of the Visitors.";
    },
    .on_click = [](){
        smite();
    },
};

button_t b_spawn_rate = (button_t){
    .is_enabled = true,
    .text       = "Draw more",
    .tooltip    = []() -> const char * {
        auto r = TextFormat(
            "Increase our appeal.\n"
            "Cost: %d",
            *spawn_rate_price
        );
        return r;
    },
    .on_click = [](){
        buy(
            spawn_rate_price,
            increase_spawn_rate,
            my_times_two
        );
    },
};

button_t b_multi = (button_t){
    .is_enabled = true,
    .text       = "Yield more",
    .tooltip    = []() -> const char * {
        auto r = TextFormat(
            "Multiply their torment.\n"
            "Cost: %d",
            *multi_price
        );
        return r;
    },
    .on_click   = [](){
        buy(
            multi_price,
            increase_multi,
            my_times_10
        );
    },
};

button_t b_burster = (button_t){
    .is_enabled = true,
    .text       = "Buy Burster",
    .tooltip    = []() -> const char * {
        auto r = TextFormat(
            "Deadly anomaly that extracts more blood.\n"
            "I have: %d | Cost: %d",
            n_bursters,
            *burster_price
        );
        return r;
    },
    .on_click = [](){
        buy(
            burster_price,
            spawn_burster,
            my_slow_rise
        );
    },
};

button_t b_radarea = (button_t){
    .is_enabled = true,
    .text       = "Buy Radarea",
    .tooltip    = []() -> const char * {
        auto r = TextFormat(
            "Poisonous ground doubling yield on death.\n"
            "I have: %d | Cost: %d",
            n_radareas,
            *radarea_price
        );
        return r;
    },
    .on_click = [](){
        buy(
            radarea_price,
            spawn_radarea,
            my_slow_rise
        );
    },
};

button_t b_gravtrap = (button_t){
    .is_enabled = true,
    .text       = "Buy Gravtrap",
    .tooltip    = []() -> const char * {
        auto r = TextFormat(
            "Orb launching matter at terminal velocities.\n"
            "I have: %d | Cost: %d",
            n_gravtraps,
            *gravtrap_price
        );
        return r;
    },
    .on_click = [](){
        buy(
            gravtrap_price,
            spawn_gravtrap,
            my_const
        );
    },
};

button_t b_librarian = (button_t){
    .is_enabled = true,
    .text       = "Employ Librarian",
    .tooltip    = []() -> const char * {
        auto r = TextFormat(
            "Servitours of the cause.\n"
            "I have: %d | Cost: %d",
            n_librarians,
            *librarian_price
        );
        return r;
    },
    .on_click = [](){
        buy(
            librarian_price,
            spawn_librarian,
            my_const
        );
    },
};

button_t buttons[] = {
    b_smite,
    b_spawn_rate,
    b_multi,
    b_burster,
    b_radarea,
    b_gravtrap,
    b_librarian,
};
