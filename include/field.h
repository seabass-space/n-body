#ifndef N_BODY_FIELD
#define N_BODY_FIELD

#include "sdl_utils.h"

typedef struct {
    SDL_GPUComputePipeline *pipeline;
    GPUArray lines;
    u32 lines_count;
    bool enabled;
} Field;

SDL_AppResult field_init(Field *field, SDL_GPUDevice *gpu);
void field_free(const Field *field, SDL_GPUDevice *gpu);

#endif
