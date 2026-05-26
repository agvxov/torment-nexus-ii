#pragma once

#include "dyrect.h"

Rectangle get_gui_rect() {
    auto r = get_screen_rect();
    r.width = r.width / 2;
    r.x = r.width;
    return r;
}

bool draw_message(const char * s, bool is_interactive, const char * interactive_message) {
    int margin = 20;

    auto gui_rect = get_gui_rect();
    auto r = gui_rect;
    r.height = 90;
    r.width -= margin * 2;
    r = balance(gui_rect, r);
    r.y += margin;

    DrawRectangleRec(
        r,
        RED
    );

    int text_margin = 8;
    DrawText(
        s,
        r.x + text_margin,
        r.y + text_margin,
        36,
        BLACK
    );

    if (is_interactive) {
        Rectangle button_rect = {0};
        button_rect.width  = 60;
        button_rect.height = 30;
        button_rect = paper(r, ride(after(r, 1), button_rect));
        button_rect.x -= 3;
        button_rect.y -= 3;

        return GuiButton(button_rect, interactive_message);
    }

    return false;
}

void draw_stats(void) {
    auto gui_rect = get_gui_rect();
    auto r = gui_rect;
    r.x += 10;
    r.y += 130;

    DrawText(
        TextFormat(
            "[Blood - %lld] [Draw - %.2f]",
            blood,
            stalker_spawns_per_second
        ),
        r.x,
        r.y,
        32,
        WHITE
    );
}

void draw_button(
    float x,
    float y,
    button_t * b
) {
    if (clip_mode) {
        DrawTexture(
            clip_texture,
            x,
            y,
            WHITE
        );
        return;
    }

    Rectangle button_rect = (Rectangle) {
        .x = x,
        .y = y,
        .width  = 140,
        .height = 80,
    };

    auto rc = GuiButton(
        button_rect,
        b->text
    );

    if (CheckCollisionPointRec(GetMousePosition(), button_rect)) {
        draw_message(b->tooltip(), false, NULL);
    }

    if (rc) {
        b->on_click();
    }
}
