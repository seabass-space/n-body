#ifndef N_BODY_FIELD
#define N_BODY_FIELD

#include "sdl_utils.h"

typedef struct Simulation Simulation;

typedef struct Field {
    SDL_GPUComputePipeline *pipeline;
    GPUArray lines;
    GPUArray line_ids;
    u32 line_count;
    bool enabled;
} Field;

SDL_AppResult field_init(Field *field, SDL_GPUDevice *gpu);
typedef struct {
    SDL_GPUDevice *gpu;
    SDL_GPUCopyPass *copy_pass;
    const f32 mass;
    const u32 index;
} FieldAddBodyInfo;
void field_add_body(Field *field, const FieldAddBodyInfo *info);
void field_update(const Field *field, const Simulation *sim, SDL_GPUCommandBuffer *command_buffer, SDL_GPUComputePass *compute_pass);
void field_free(const Field *field, SDL_GPUDevice *gpu);
#endif
