#include "field.h"
#include "constants.h"

#include "HandmadeMath.h"

const u32 MAX_STEPS = 512;

SDL_AppResult field_init(Field *field, SDL_GPUDevice *gpu) {
    field->pipeline = CreateGPUComputePipeline(gpu, "shaders/field.comp.spv");
    if (!field->pipeline) panic("Failed to create field lines compute pipeline!");

    field->lines = CreateGPUArray(gpu, FIELD_LINE_LENGTH * sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    if (!field->lines.buffer) panic("Failed to create field lines storage buffer!");

    field->enabled = true;
    return SDL_APP_CONTINUE;
}

void field_add_body(const Field *field, f32 mass) {

}

void field_update(const Field *field, SDL_GPUComputePass *compute_pass) {
    SDL_BindGPUComputePipeline(compute_pass, field->pipeline);
}

void field_free(const Field *field, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUBuffer(gpu, field->lines.buffer);
    SDL_ReleaseGPUComputePipeline(gpu, field->pipeline);
}
