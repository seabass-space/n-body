#include "trajectories.h"
#include "SDL3/SDL_gpu.h"
#include "constants.h"
#include "simulation.h"
#include "ghost.h"

#include "HandmadeMath.h"

#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTION_LENGTH

SDL_AppResult trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu) {
    trajectories->ghost_pipeline = CreateGPUComputePipeline(gpu, "shaders/ghost_trajectory.comp.spv");
    SDL_GPUComputePipeline *euler = CreateGPUComputePipeline(gpu, "shaders/trajectory/euler.comp.spv");
    SDL_GPUComputePipeline *verlet = CreateGPUComputePipeline(gpu, "shaders/trajectory/verlet.comp.spv");
    SDL_GPUComputePipeline *runge_kutta = CreateGPUComputePipeline(gpu, "shaders/trajectory/runge_kutta.comp.spv");
    if (!trajectories->ghost_pipeline) panic("Failed to create ghost trajectories compute pipeline!");
    if (!euler) panic("Failed to create trajectory euler compute pipeline!");
    if (!verlet) panic("Failed to create trajectory verlet compute pipeline!");
    if (!runge_kutta) panic("Failed to create trajectory runge kutta compute pipeline!");
    memcpy(trajectories->integrators, (SDL_GPUComputePipeline*[3]){ euler, verlet, runge_kutta }, sizeof(trajectories->integrators));

    trajectories->positions = CreateGPUArray(gpu, PREDICTION_SIZE, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    trajectories->velocities = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    trajectories->ghost = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo) {
        .size = sizeof(HMM_Vec2) + PREDICTION_SIZE,
        .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
    });

    if (!trajectories->positions.buffer) panic("Failed to create trajectory positions buffer!");
    if (!trajectories->velocities.buffer) panic("Failed to create trajectory velocities buffer!");
    if (!trajectories->ghost) panic("Failed to create trajectories ghost buffer!");

    trajectories->enabled = true;
    return SDL_APP_CONTINUE;
}

void trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass,
                           const HMM_Vec2 position) {
    HMM_Vec2 trajectory[PREDICTION_LENGTH];
    for (usize i = 0; i < PREDICTION_LENGTH; i++) trajectory[i] = position;

    const AppendGPUArrayBinding bindings[] = {
        { .array = &trajectories->positions, .source = (u8 *) &trajectory, .size = PREDICTION_SIZE },
        { .array = &trajectories->velocities, .source = (u8 *) &trajectory, .size = sizeof(HMM_Vec2) },
    };

    AppendGPUArrays(gpu, copy_pass, bindings, sizeof(bindings) / sizeof(AppendGPUArrayBinding));
}

void trajectories_update(const Trajectories *trajectories, const TrajectoriesUpdateInfo *info) {
    if (!trajectories->enabled) return;
    const struct {
        u32 count;
        f32 gravity;
        f32 softening;
        f32 delta_time;
    } constants = {
        info->sim->body_count,
        info->sim->options.gravity,
        info->sim->options.softening,
        info->delta_time,
    };

    const struct {
        HMM_Vec2 position;
        HMM_Vec2 velocity;
        f32 mass;
        bool enabled;
    } ghost_info = {
        info->ghost->position,
        info->ghost->velocity,
        info->ghost->mass,
        info->ghost->enabled
    };

    SDL_PushGPUComputeUniformData(info->command_buffer, 0, &constants, sizeof(constants));
    SDL_PushGPUComputeUniformData(info->command_buffer, 1, &ghost_info, sizeof(ghost_info));

    SDL_GPUBuffer *buffers[] = {
        trajectories->positions.buffer,
        trajectories->velocities.buffer,
        trajectories->ghost,
        info->sim->positions.buffer,
        info->sim->velocities.buffer,
        info->sim->masses.buffer,
        info->sim->movable.buffer
    };

    SDL_BindGPUComputeStorageBuffers(info->compute_pass, 0, buffers, sizeof(buffers) / sizeof(SDL_GPUBuffer *));

    SDL_GPUComputePipeline *integrator = trajectories->integrators[info->sim->options.integrator];
    for (u32 i = 0; i < PREDICTION_LENGTH; i++) {
        SDL_PushGPUComputeUniformData(info->command_buffer, 2, &i, sizeof(i));
        SDL_BindGPUComputePipeline(info->compute_pass, integrator);
        SDL_DispatchGPUCompute(info->compute_pass, info->sim->body_count, 1, 1);
        SDL_BindGPUComputePipeline(info->compute_pass, trajectories->ghost_pipeline);
        SDL_DispatchGPUCompute(info->compute_pass, 1, 1, 1);
    }
}

void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu) {
    for (u8 i = 0; i < 3; i++) SDL_ReleaseGPUComputePipeline(gpu, trajectories->integrators[i]);
    SDL_ReleaseGPUComputePipeline(gpu, trajectories->ghost_pipeline);
    SDL_ReleaseGPUBuffer(gpu, trajectories->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->ghost);
}

