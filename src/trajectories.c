#include "trajectories.h"
#include "SDL3/SDL_gpu.h"
#include "constants.h"
#include "simulation.h"
#include "ghost.h"

#include "HandmadeMath.h"

#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTION_LENGTH

SDL_AppResult trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu) {
    trajectories->pipeline = CreateGPUComputePipeline(gpu, "shaders/trajectory.comp.spv");
    trajectories->ghost_pipeline = CreateGPUComputePipeline(gpu, "shaders/ghost_trajectory.comp.spv");
    if (!trajectories->pipeline) panic("Failed to create trajectories compute pipeline!");
    if (!trajectories->ghost_pipeline) panic("Failed to create ghost trajectory compute pipeline!");

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

void trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass) {
    ExpandGPUArray(&trajectories->positions, gpu, copy_pass, PREDICTION_SIZE);
    ExpandGPUArray(&trajectories->velocities, gpu, copy_pass, sizeof(HMM_Vec2));
    trajectories->positions.used += PREDICTION_SIZE;
    trajectories->velocities.used += sizeof(HMM_Vec2);
}

void trajectories_update(const Trajectories *trajectories, const TrajectoriesUpdateInfo *info) {
    if (!trajectories->enabled || !(info->sim->body_count || info->ghost->enabled)) return;
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

    for (u32 i = 0; i < PREDICTION_LENGTH; i++) {
        SDL_PushGPUComputeUniformData(info->command_buffer, 2, &i, sizeof(i));
        SDL_BindGPUComputePipeline(info->compute_pass, trajectories->pipeline);
        SDL_DispatchGPUCompute(info->compute_pass, info->sim->body_count, 1, 1);
    }
}

void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, trajectories->pipeline);
    SDL_ReleaseGPUComputePipeline(gpu, trajectories->ghost_pipeline);
    SDL_ReleaseGPUBuffer(gpu, trajectories->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->velocities.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->ghost);
}

