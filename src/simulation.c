#include "simulation.h"
#include "SDL3/SDL_gpu.h"
#include "constants.h"
#include "sdl_utils.h"

#include "stb_ds.h"

SDL_AppResult simulation_init(Simulation *sim, SDL_GPUDevice *gpu) {
    sim->options = (SimulationOptions) {
        .gravity = GRAVITY_DEFAULT,
        .softening = SOFTENING_DEFAULT,
        .density = DENSITY_DEFAULT,
        .integrator = INTEGRATOR_DEFAULT,
        .paused = false
    };

    SDL_GPUComputePipeline *euler = CreateGPUComputePipeline(gpu, "shaders/simulation/euler.comp.spv");
    SDL_GPUComputePipeline *verlet = CreateGPUComputePipeline(gpu, "shaders/simulation/verlet.comp.spv");
    SDL_GPUComputePipeline *runge_kutta = CreateGPUComputePipeline(gpu, "shaders/simulation/runge_kutta.comp.spv");
    if (!euler) panic("Failed to create simulation euler compute pipeline!");
    if (!verlet) panic("Failed to create simulation verlet compute pipeline!");
    if (!runge_kutta) panic("Failed to create simulation runge kutta compute pipeline!");
    memcpy(sim->integrators, (SDL_GPUComputePipeline*[3]) { euler, verlet, runge_kutta }, sizeof(sim->integrators));

    sim->current_buffer = SIM_POSITIONS_A;
    sim->positions_a = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    sim->positions_b = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    sim->velocities = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    sim->masses = CreateGPUArray(gpu, sizeof(f32), SDL_GPU_BUFFERUSAGE_READDRAW);
    sim->movable = CreateGPUArray(gpu, sizeof(f32), SDL_GPU_BUFFERUSAGE_READDRAW);
    if (!sim->positions_a.buffer) panic("Failed to create simulation position a buffer!");
    if (!sim->positions_b.buffer) panic("Failed to create simulation position b buffer!");
    if (!sim->velocities.buffer) panic("Failed to create simulation velocities buffer!");
    if (!sim->masses.buffer) panic("Failed to create simulation masses buffer!");
    if (!sim->movable.buffer) panic("Failed to create simulation movable buffer!");

    return SDL_APP_CONTINUE;
}

u32 simulation_add_body(
    Simulation *sim,
    SDL_GPUDevice *gpu,
    SDL_GPUCopyPass *copy_pass,
    const SimulationAddBodyInfo *body
) {
    AppendGPUArrays(gpu, copy_pass, (AppendGPUArrayBinding[]) {
        { .array = &sim->positions_a, .source = (u8*) &body->position, .size = sizeof(HMM_Vec2) },
        { .array = &sim->positions_b, .source = (u8*) &body->position, .size = sizeof(HMM_Vec2) },
        { .array = &sim->velocities, .source = (u8*) &body->velocity, .size = sizeof(HMM_Vec2) },
        { .array = &sim->masses, .source = (u8*) &body->mass, .size = sizeof(f32) },
        { .array = &sim->movable, .source = (u8*) &(f32) { body->movable }, .size = sizeof(f32) },
    }, 5);
    return sim->body_count++;
}

void simulation_update(
    Simulation *sim,
    SDL_GPUCommandBuffer *command_buffer,
    SDL_GPUComputePass *compute_pass,
    const f32 delta_time
) {
    if (sim->options.paused || !sim->body_count) return;

    const struct {
        u32 body_count;
        f32 gravity;
        f32 softening;
        f32 delta_time;
    } constants = {
        sim->body_count,
        sim->options.gravity,
        sim->options.softening,
        delta_time
    };

    SDL_PushGPUComputeUniformData(command_buffer, 0, &constants, sizeof(constants));

    SDL_GPUComputePipeline *integrator = sim->integrators[sim->options.integrator];
    SDL_BindGPUComputePipeline(compute_pass, integrator);

    if (sim->current_buffer == SIM_POSITIONS_A) {
        SDL_BindGPUComputeStorageBuffers(compute_pass, 0, (SDL_GPUBuffer*[]) {
            sim->positions_a.buffer,
            sim->positions_b.buffer,
        }, 2);
        sim->current_buffer = SIM_POSITIONS_B;
    } else {
        SDL_BindGPUComputeStorageBuffers(compute_pass, 0, (SDL_GPUBuffer*[]) {
            sim->positions_b.buffer,
            sim->positions_a.buffer,
        }, 2);
        sim->current_buffer = SIM_POSITIONS_A;
    }

    SDL_BindGPUComputeStorageBuffers(compute_pass, 2, (SDL_GPUBuffer*[]) {
        sim->velocities.buffer,
        sim->masses.buffer,
        sim->movable.buffer
    }, 3);
    SDL_DispatchGPUCompute(compute_pass, sim->body_count, 1, 1);
}

void simulation_free(const Simulation *sim, SDL_GPUDevice *gpu) {
    for (u8 i = 0; i < 3; i++) SDL_ReleaseGPUComputePipeline(gpu, sim->integrators[i]);
    SDL_ReleaseGPUBuffer(gpu, sim->positions_a.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->positions_b.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->masses.buffer);
    SDL_ReleaseGPUBuffer(gpu, sim->movable.buffer);
}
