#ifndef N_BODY_TRAJECTORY
#define N_BODY_TRAJECTORY

#include "sdl_utils.h"

typedef struct Simulation Simulation;
typedef struct Ghost Ghost;

typedef struct Trajectories {
    SDL_GPUComputePipeline *pipeline;
    SDL_GPUComputePipeline *ghost_pipeline;
    GPUArray positions;
    GPUArray velocities;
    SDL_GPUBuffer *ghost;
    bool enabled;
} Trajectories;

SDL_AppResult trajectories_init(Trajectories *trajectories, SDL_GPUDevice *gpu);

void trajectories_add_body(Trajectories *trajectories, SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass);
typedef struct {
    SDL_GPUCommandBuffer *command_buffer;
    SDL_GPUComputePass *compute_pass;
    const Simulation *sim;
    const Ghost *ghost;
    f32 delta_time;
} TrajectoriesUpdateInfo;
void trajectories_update(const Trajectories *trajectories, const TrajectoriesUpdateInfo *info);
void trajectories_free(const Trajectories *trajectories, SDL_GPUDevice *gpu);

#endif

