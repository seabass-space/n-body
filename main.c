#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#include "lib/types.h"
#include "lib/arena.h"
#include "lib/list.h"

#include "gui.h"

#define MAX_LENGTH 2048
#define DEFAULT_WIDTH 1200
#define DEFAULT_HEIGHT 900

typedef struct {
    Vector2 positions[TRAIL_MAX];
    usize count;
    usize oldest;
} Trail;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    f32 mass;
    bool movable;
    Color color;
    Trail trail;
} Planet;

typedef struct {
    Planet *data;
    usize length;
    usize capacity;
} Planets;

typedef struct {
    GUIState gui;
    Camera2D camera;
    i32 target; // index into planets for camera target, -1 if no target

    Planets planets;
    Planets previous;

    Vector2 create_position;
} SimulationState;

void simulation_init(SimulationState *simulation, Arena *arena);
void planet_create(SimulationState *simulation, Arena *arena);
void planet_update(SimulationState *simulation, usize index, f32 dt);
void planet_draw(SimulationState *simulation, usize index);
void new_planet_draw(SimulationState *simulation);
void target_set(SimulationState *simulation);
void camera_update(SimulationState *simulation, f32 dt);
void planet_update_gui(SimulationState *simulation);

i32 main() {
    Arena arena = arena_new(1024 * 1024);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "esby is confused");
    SetTargetFPS(60);

    SimulationState simulation;
    simulation_init(&simulation, &arena);

    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        f32 dt = GetFrameTime();

        if (IsKeyPressed(KEY_R) || simulation.gui.reset) simulation_init(&simulation, &arena);
        if (IsKeyPressed(KEY_SPACE)) simulation.gui.paused = !simulation.gui.paused;
        if (IsKeyPressed(KEY_ESCAPE) && simulation.gui.create) simulation.gui.create = false;
        if (IsKeyPressed(KEY_LEFT_BRACKET)) simulation.target = (simulation.target - 1) % simulation.planets.length;
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) simulation.target = (simulation.target + 1) % simulation.planets.length;
        if (IsKeyPressed(KEY_C)) simulation.gui.create = !simulation.gui.create;
        if (IsKeyPressed(KEY_M)) simulation.gui.movable = !simulation.gui.movable;

        target_set(&simulation);
        camera_update(&simulation, dt);
        planet_update_gui(&simulation);

        planet_create(&simulation, &arena);

        Planet previous_data[simulation.planets.length];
        memcpy(previous_data, simulation.planets.data, sizeof(previous_data));
        simulation.previous = (Planets) { previous_data, simulation.planets.length, simulation.planets.capacity };

        for (usize i = 0; i < simulation.planets.length; i++) {
            if (!simulation.gui.paused) planet_update(&simulation, i, dt);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(simulation.camera);

        for (usize i = 0; i < simulation.planets.length; i++) {
            planet_draw(&simulation, i);
        }

        new_planet_draw(&simulation);

        EndMode2D();
        gui_draw(&simulation.gui);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void simulation_init(SimulationState *simulation, Arena *arena) {
    simulation->gui = gui_init();
    simulation->target = -1;
    simulation->camera = (Camera2D) {
        .zoom = 1.0,
        .offset = (Vector2) { GetScreenWidth() / 2.0, GetScreenHeight() / 2.0 },
    };

    Planets *planets = &simulation->planets;
    planets->length = 0;
    planets->capacity = 0;
}

void planet_create(SimulationState *simulation, Arena *arena) {
    if (!simulation->gui.create) return;

    if (GetMouseWheelMove() != 0) {
        f32 scale = 0.2 * GetMouseWheelMove();
        simulation->gui.mass = expf(logf(simulation->gui.mass) + scale);
    }

    if (CheckCollisionPointRec(GetMousePosition(), simulation->gui.layout[0])) return;
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), simulation->camera);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { simulation->create_position = mouse; }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        Vector2 target_velocity = (simulation->target != -1)
            ? simulation->planets.data[simulation->target].velocity
            : Vector2Zero();
        Vector2 dragged_velocity = Vector2Subtract(simulation->create_position, mouse);

        *list_push(&simulation->planets, arena) = (Planet) {
            .position = simulation->create_position,
            .velocity = Vector2Add(target_velocity, dragged_velocity),
            .mass = simulation->gui.mass,
            .movable = simulation->gui.movable,
            .color = simulation->gui.color,
        };
    }
}

typedef struct {
    Vector2 position;
    Vector2 velocity;
} RK4State;

static inline RK4State rk4_add(RK4State a, RK4State b);
static inline RK4State rk4_scale(RK4State state, f32 scale);
static inline RK4State rk4_delta(RK4State state, const SimulationState *simulation, usize index);

void planet_update(SimulationState *simulation, usize index, f32 dt) {
    Planet *self = &simulation->planets.data[index];
    if (!self->movable) return;

    RK4State state = {
        .position = self->position,
        .velocity = self->velocity
    };

    RK4State k_1 = rk4_delta(state, simulation, index);
    RK4State k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, dt/2.0)), simulation, index);
    RK4State k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, dt/2.0)), simulation, index);
    RK4State k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, dt)), simulation, index);

    RK4State k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    RK4State new_state = rk4_add(state, rk4_scale(k_sum, dt/6.0));
    self->position = new_state.position;
    self->velocity = new_state.velocity;

    if (!self->movable) return;
    self->trail.positions[(self->trail.oldest + self->trail.count) % TRAIL_MAX] = new_state.position;
    if (self->trail.count < TRAIL_MAX) {
        self->trail.count++;
    } else {
        self->trail.oldest = (self->trail.oldest + 1) % TRAIL_MAX;
    }
}

static Vector2 compute_acceleration(Vector2 position, const SimulationState *simulation, usize index) {
    Planet *planet = &simulation->previous.data[index];
    Vector2 displacement = Vector2Subtract(planet->position, position);
    Vector2 direction = Vector2Normalize(displacement);
    f32 length_squared = Vector2LengthSqr(displacement);
    if (length_squared < EPSILON) { return Vector2Zero(); }
    return Vector2Scale(direction, (simulation->gui.gravity * planet->mass) / length_squared);
}

static Vector2 compute_field(Vector2 position, const SimulationState *simulation, usize index) {
    Vector2 net_acceleration = Vector2Zero();
    for (usize i = 0; i < simulation->previous.length; i++) {
        if (i == index) { continue; }
        Vector2 acceleration = compute_acceleration(position, simulation, i);
        net_acceleration = Vector2Add(net_acceleration, acceleration);
    }

    return net_acceleration;
}

static inline RK4State rk4_add(RK4State a, RK4State b) {
    return (RK4State) {
        .position = Vector2Add(a.position, b.position),
        .velocity = Vector2Add(a.velocity, b.velocity),
    };
}

static inline RK4State rk4_scale(RK4State state, f32 scale) {
    return (RK4State) {
        .position = Vector2Scale(state.position, scale),
        .velocity = Vector2Scale(state.velocity, scale),
    };
}

static inline RK4State rk4_delta(RK4State state, const SimulationState *simulation, usize index) {
    return (RK4State) {
        .position = state.velocity,
        .velocity = compute_field(state.position, simulation, index),
    };
}

void DrawVector(Vector2 start, Vector2 vector, Color color) {
    #define ARROW_HEAD_LENGTH 10
    #define ARROW_ANGLE 3*PI/4
    
    Vector2 end = Vector2Add(start, vector);
    DrawLineV(start, end, color);
    
    Vector2 head_1 = Vector2Normalize(Vector2Rotate(vector, ARROW_ANGLE));
    Vector2 head_2 = Vector2Normalize(Vector2Rotate(vector, -ARROW_ANGLE));
    DrawLineV(end, Vector2Add(end, Vector2Scale(head_1, ARROW_HEAD_LENGTH)), color);
    DrawLineV(end, Vector2Add(end, Vector2Scale(head_2, ARROW_HEAD_LENGTH)), color);
}

void planet_draw(SimulationState *simulation, usize index) {
    Planet *planet = &simulation->planets.data[index];

    void (*draw)(Vector2, f32, Color) = planet->movable ? DrawCircleLinesV : DrawCircleV;
    draw(
        planet->position,
        simulation->gui.size * pow(planet->mass, 1.0/3.0),
        planet->color
    );

    // slider to control force arrow size?
    if (simulation->gui.draw_forces) {
        for (usize i = 0; i < simulation->planets.length; i++) {
            if (i == index) continue;
            Vector2 accleration = compute_acceleration(planet->position, simulation, i);
            Vector2 force = Vector2Scale(accleration, planet->mass);
            DrawVector(planet->position, force, simulation->planets.data[i].color);
        }

    }

    // way to reduce computation?
    if (simulation->gui.draw_net_force) {
        Vector2 net_acceleration = compute_field(planet->position, simulation, index);
        Vector2 net_force = Vector2Scale(net_acceleration, planet->mass);
        DrawVector(planet->position, net_force, planet->color);
    }

    if (!planet->movable) return;
    Vector2 target_velocity = (simulation->gui.draw_relative && simulation->target != -1)
        ? simulation->planets.data[simulation->target].velocity
        : Vector2Zero();
    if (simulation->gui.draw_velocity) DrawVector(planet->position, Vector2Subtract(planet->velocity, target_velocity), planet->color);

    usize count = planet->trail.count < simulation->gui.trail ? planet->trail.count : simulation->gui.trail;
    for (usize i = 1; i < count; i++) {
        Vector2 a = planet->trail.positions[(planet->trail.oldest + planet->trail.count - i) % TRAIL_MAX];
        Vector2 b = planet->trail.positions[(planet->trail.oldest + planet->trail.count - i - 1) % TRAIL_MAX];

        if (simulation->gui.draw_relative && simulation->target != -1) {
            Planet *target = &simulation->planets.data[simulation->target];
            a = Vector2Add(target->position, Vector2Subtract(a, target->trail.positions[(target->trail.oldest + target->trail.count - i) % TRAIL_MAX]));
            b = Vector2Add(target->position, Vector2Subtract(b, target->trail.positions[(target->trail.oldest + target->trail.count - i - 1) % TRAIL_MAX]));
        }

        Color color = planet->color;
        color.a = 256 * (count - i) / count;
        DrawLineV(a, b, color);
    }
}

void new_planet_draw(SimulationState *simulation) {
    if (!simulation->gui.create) return;
    if (CheckCollisionPointRec(GetMousePosition(), simulation->gui.layout[0])) return;

    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), simulation->camera);
    Vector2 position = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? simulation->create_position : mouse;

    Color color = simulation->gui.color;
    color.a = 128;
    void (*draw)(Vector2, f32, Color) = simulation->gui.movable ? DrawCircleLinesV : DrawCircleV;
    draw(position, simulation->gui.size * pow(simulation->gui.mass, 1.0/3.0), color);

    if (!simulation->gui.movable) return;
    Vector2 target_velocity = (simulation->target != -1 && !simulation->gui.draw_relative)
        ? simulation->planets.data[simulation->target].velocity
        : Vector2Zero();
    Vector2 dragged_velocity = IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? Vector2Subtract(simulation->create_position, mouse) : Vector2Zero();
    DrawVector(
        position,
        Vector2Add(target_velocity, dragged_velocity),
        simulation->gui.color
    );
};

// https://www.youtube.com/watch?v=LSNQuFEDOyQ
Vector2 Vector2ExpDecay(Vector2 a, Vector2 b, f32 decay, f32 dt) {
    return Vector2Add(b, Vector2Scale(
        Vector2Subtract(a, b),
        exp(-decay * dt)
    ));
}

void target_set(SimulationState *simulation) {
    if (simulation->gui.create) return;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    for (usize i = 0; i < simulation->planets.length; i++) {
        Planet planet = simulation->planets.data[i];
        f32 size = simulation->gui.size * pow(planet.mass, 1.0/3.0);
        if (CheckCollisionPointCircle(GetScreenToWorld2D(GetMousePosition(), simulation->camera), planet.position, size)) {
            simulation->target = i;
            simulation->gui.mass = planet.mass;
            simulation->gui.movable = planet.movable;
            simulation->gui.color = planet.color;
        }
    }
}

void camera_update(SimulationState *simulation, f32 dt) {
    #define CAMERA_SMOOTHING 12.0

    Camera2D *camera = &simulation->camera;
    if (simulation->target != -1) {
        camera->target = Vector2ExpDecay(
            camera->target,
            simulation->planets.data[simulation->target].position,
            CAMERA_SMOOTHING,
            dt
        );

        camera->offset = Vector2ExpDecay(
            camera->offset,
            (Vector2) { GetScreenWidth() / 2.0, GetScreenHeight() / 2.0 },
            CAMERA_SMOOTHING,
            dt
        );
    }

    if (CheckCollisionPointRec(GetMousePosition(), simulation->gui.layout[0])) return;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0 / camera->zoom);
        camera->target = Vector2Add(camera->target, delta);
        simulation->target = -1;
    }

    if (GetMouseWheelMove() != 0.0) {
        if (simulation->gui.create) return;
        f32 scale = 0.2 * GetMouseWheelMove();
        camera->zoom = Clamp(expf(logf(camera->zoom) + scale), 0.125, 64.0);

        if (simulation->target == -1) {
            Vector2 mouse_world_position = GetScreenToWorld2D(GetMousePosition(), *camera);
            camera->offset = GetMousePosition();
            camera->target = mouse_world_position;
        }
    }
}

void planet_update_gui(SimulationState *simulation) {
    if (simulation->target == -1) return;
    if (simulation->gui.create) return;

    Planet *planet = &simulation->planets.data[simulation->target];
    if (simulation->gui.previous_create) {
        simulation->gui.mass = planet->mass;
        simulation->gui.movable = planet->movable;
        simulation->gui.color = planet->color;
        return;
    }

    planet->mass = simulation->gui.mass;
    planet->movable = simulation->gui.movable;
    planet->color = simulation->gui.color;
}

