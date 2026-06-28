#include "trajectories.h"
#include "SDL3/SDL_gpu.h"
#include "constants.h"
#include "simulation.h"
#include "ghost.h"

#include "HandmadeMath.h"

#define PREDICTION_SIZE sizeof(HMM_Vec2) * PREDICTION_LENGTH

SDL_AppResult trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu) {
    trajectories->pipeline = CreateGPUComputePipeline(gpu, "shaders/trajectory.comp.spv");
    if (!trajectories->pipeline) panic("Failed to create trajectories compute pipeline!");

    trajectories->positions = CreateGPUArray(gpu, PREDICTION_SIZE, SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    trajectories->velocities = CreateGPUArray(gpu, sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    if (!trajectories->positions.buffer) panic("Failed to create trajectory positions buffer!");
    if (!trajectories->velocities.buffer) panic("Failed to create trajectory velocities buffer!");

    trajectories->options = (TrajectoryOptions) {
        .delta_time_multiplier = TRAJECTORY_DELTA_TIME_MULTIPLIER_DEFAULT,
        .enabled = true
    };

    return SDL_APP_CONTINUE;
}

void trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass) {
    ExpandGPUArray(&trajectories->positions, gpu, copy_pass, PREDICTION_SIZE);
    ExpandGPUArray(&trajectories->velocities, gpu, copy_pass, sizeof(HMM_Vec2));
    trajectories->positions.used += PREDICTION_SIZE;
    trajectories->velocities.used += sizeof(HMM_Vec2);
}

void trajectories_ghost_update(const Trajectories *trajectories, const TrajectoriesGhostUpdateInfo *info) {
    if (!info->ghost->enabled) return;
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(info->command_buffer);
    WriteToGPUBuffers(info->gpu, copy_pass, (WriteGPUBufferBinding[]) {
        {
            .buffer = trajectories->positions.buffer,
            .buffer_offset = info->sim->body_count * PREDICTION_SIZE,
            .source = (u8*) &info->ghost->position,
            .size = sizeof(HMM_Vec2)
        },
        {
            .buffer = trajectories->velocities.buffer,
            .buffer_offset = info->sim->body_count * sizeof(HMM_Vec2),
            .source = (u8*) &info->ghost->velocity,
            .size = sizeof(HMM_Vec2)
        }
    }, 2);
    SDL_EndGPUCopyPass(copy_pass);
}

void trajectories_update(const Trajectories *trajectories, TrajectoriesUpdateInfo *info) {
    if (!trajectories->options.enabled) return;
    u32 trajectory_count = info->sim->body_count;
    if (info->ghost->enabled) trajectory_count += 1;
    if (!trajectory_count) return;

    SDL_BindGPUComputePipeline(info->compute_pass, trajectories->pipeline);

    const struct {
        u32 count;
        f32 gravity;
        f32 softening;
        f32 delta_time;
        f32 ghost_mass;
        bool ghost;
    } constants = {
        info->sim->body_count,
        info->sim->options.gravity,
        info->sim->options.softening,
        info->delta_time * trajectories->options.delta_time_multiplier,
        info->ghost->mass,
        info->ghost->enabled
    };
    SDL_PushGPUComputeUniformData(info->command_buffer, 0, &constants, sizeof(constants));

    SDL_BindGPUComputeStorageBuffers(info->compute_pass, 0, (SDL_GPUBuffer*[]) {
        trajectories->positions.buffer,
        trajectories->velocities.buffer,
        info->sim->current_buffer == SIM_POSITIONS_A
            ? info->sim->positions_b.buffer
            : info->sim->positions_a.buffer,
        info->sim->velocities.buffer,
        info->sim->masses.buffer,
        info->sim->movable.buffer
    }, 6);

    for (u32 i = 0; i < PREDICTION_LENGTH; i++) {
        SDL_PushGPUComputeUniformData(info->command_buffer, 1, &i, sizeof(i));
        SDL_DispatchGPUCompute(info->compute_pass, trajectory_count, 1, 1);
    }
}

void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUComputePipeline(gpu, trajectories->pipeline);
    SDL_ReleaseGPUBuffer(gpu, trajectories->positions.buffer);
    SDL_ReleaseGPUBuffer(gpu, trajectories->velocities.buffer);
}
