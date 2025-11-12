#pragma once

#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raymath.h"
#include "lib/raygui.h"
#include "lib/gui_style.h"

#include "lib/types.h"

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_WINDOW_CLOSEBUTTON_SIZE 18
#define CLOSE_TITLE_SIZE_DELTA_HALF (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - RAYGUI_WINDOW_CLOSEBUTTON_SIZE) / 2

#define SIZE_DEFAULT 10.0
#define SIZE_MIN 0.0
#define SIZE_MAX 20.0

#define GRAVITY_DEFAULT 10000.0
#define GRAVITY_MIN 5000.0
#define GRAVITY_MAX 20000.0

#define MASS_DEFAULT 100.0
#define MASS_MIN 0.0
#define MASS_MAX 600.0

#define TRAIL_DEFAULT 64
#define TRAIL_MAX 256
#define TRAIL_MIN 0

#define COLOR_DEFAULT WHITE

typedef struct {
    Vector2 anchor;
    Rectangle layout[17];
    bool minimized;
    bool moving;

    bool reset;
    bool paused;

    f32 gravity;
    f32 trail;

    f32 size;
    bool draw_velocity;
    bool draw_forces;
    bool draw_net_force;

    Color color;
    f32 mass;
    bool movable;
    bool create;
    bool previous_create;
} GUIState;

static void update_layout(GUIState *state);

GUIState gui_init() {
    GuiLoadStyleDark();
    GUIState state = {
        .anchor = (Vector2){ 24, 24 },
        .minimized = false,
        .moving = false,

        .paused = false,

        .gravity = GRAVITY_DEFAULT,
        .trail = TRAIL_DEFAULT,

        .size = SIZE_DEFAULT,
        .draw_velocity = false,
        .draw_forces = false,
        .draw_net_force = false,

        .color = COLOR_DEFAULT,
        .mass = MASS_DEFAULT,
        .movable = true,
        .create = false,
        .previous_create = false,
    };

    update_layout(&state);
    return state;
}

void gui_draw(GUIState *state) {
    state->previous_create = state->create;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !state->moving) {
        Rectangle title_collision_rect = { state->anchor.x, state->anchor.y, state->layout[0].width - (RAYGUI_WINDOW_CLOSEBUTTON_SIZE + CLOSE_TITLE_SIZE_DELTA_HALF), RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT };
        if(CheckCollisionPointRec(GetMousePosition(), title_collision_rect)) {
            state->moving = true;
        }
    }

    if (state->moving) {
        Vector2 mouse_delta = GetMouseDelta();
        state->anchor.x += mouse_delta.x;
        state->anchor.y += mouse_delta.y;

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state->moving = false;
            if (state->anchor.x < 0) state->anchor.x = 0;
            else if (state->anchor.x > GetScreenWidth() - state->layout[0].width) state->anchor.x = GetScreenWidth() - state->layout[0].width;
            if (state->anchor.y < 0) state->anchor.y = 0;
            else if (state->anchor.y > GetScreenHeight()) state->anchor.y = GetScreenHeight() - RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
        }

        update_layout(state);
    }

    if (state->minimized) {
        GuiStatusBar((Rectangle){ state->anchor.x, state->anchor.y, state->layout[0].width, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT }, "N-Body Simulation");
        if (GuiButton((Rectangle){ state->anchor.x + state->layout[0].width - RAYGUI_WINDOW_CLOSEBUTTON_SIZE - CLOSE_TITLE_SIZE_DELTA_HALF,
                                   state->anchor.y + CLOSE_TITLE_SIZE_DELTA_HALF,
                                   RAYGUI_WINDOW_CLOSEBUTTON_SIZE,
                                   RAYGUI_WINDOW_CLOSEBUTTON_SIZE },
                                   "#120#")) {
            state->minimized = false;
        }
    } else {
        state->minimized = GuiWindowBox(state->layout[0], "N-Body Simulation");
        GuiGroupBox(state->layout[9], "Controls");
        state->reset = GuiButton(state->layout[10], "Reset"); 
        GuiToggle(state->layout[11], "Pause", &state->paused);

        GuiGroupBox(state->layout[1], "Simulation Parameters");
        GuiSlider(state->layout[3], "Gravity Modifier", NULL, &state->gravity, GRAVITY_MIN, GRAVITY_MAX);

        GuiGroupBox(state->layout[12], "Drawing Controls");
        GuiSliderBar(state->layout[2], "Size Modifier", NULL, &state->size, SIZE_MIN, SIZE_MAX);
        GuiSliderBar(state->layout[16], "Trail Length", NULL, &state->trail, TRAIL_MIN, TRAIL_MAX);
        GuiCheckBox(state->layout[13], "Draw Velocity", &state->draw_velocity);
        GuiCheckBox(state->layout[14], "Draw Net Force", &state->draw_net_force);
        GuiCheckBox(state->layout[15], "Draw Force Components", &state->draw_forces);

        GuiGroupBox(state->layout[4], "New Body");
        GuiColorPicker(state->layout[5], NULL, &state->color);
        GuiSlider(state->layout[6], "Mass", NULL, &state->mass, MASS_MIN, MASS_MAX);
        GuiCheckBox(state->layout[8], "Moveable?", &state->movable);
        GuiToggle(state->layout[7], "Create!", &state->create);
    }
}

static void update_layout(GUIState *state) {
    state->layout[0] = (Rectangle){ state->anchor.x + 0, state->anchor.y + 0, 320, 584 };
    state->layout[1] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 112, 288, 48 };
    state->layout[2] = (Rectangle){ state->anchor.x + 136, state->anchor.y + 192, 152, 16 };
    state->layout[3] = (Rectangle){ state->anchor.x + 136, state->anchor.y + 128, 152, 16 };
    state->layout[4] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 352, 288, 216 };
    state->layout[5] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 368, 232, 112 };
    state->layout[6] = (Rectangle){ state->anchor.x + 80, state->anchor.y + 496, 208, 16 };
    state->layout[7] = (Rectangle){ state->anchor.x + 160, state->anchor.y + 528, 128, 24 };
    state->layout[8] = (Rectangle){ state->anchor.x + 40, state->anchor.y + 528, 24, 24 };
    state->layout[9] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 40, 288, 56 };
    state->layout[10] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 56, 120, 24 };
    state->layout[11] = (Rectangle){ state->anchor.x + 168, state->anchor.y + 56, 120, 24 };
    state->layout[12] = (Rectangle){ state->anchor.x + 16, state->anchor.y + 176, 288, 160 };
    state->layout[13] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 256, 24, 24 };
    state->layout[14] = (Rectangle){ state->anchor.x + 160, state->anchor.y + 256, 24, 24 };
    state->layout[15] = (Rectangle){ state->anchor.x + 32, state->anchor.y + 296, 24, 24 };
    state->layout[16] = (Rectangle){ state->anchor.x + 136, state->anchor.y + 224, 152, 16 };
}
