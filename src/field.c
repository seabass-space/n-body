#include "field.h"
#include "simulation.h"
#include "constants.h"

#include "HandmadeMath.h"

SDL_AppResult field_init(Field *field, SDL_GPUDevice *gpu) {
    field->pipeline = CreateGPUComputePipeline(gpu, "shaders/field.comp.spv");
    if (!field->pipeline) panic("Failed to create field lines compute pipeline!");

    field->lines = CreateGPUArray(gpu, FIELD_LINE_LENGTH * sizeof(HMM_Vec2), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    field->line_ids = CreateGPUArray(gpu, sizeof(u32), SDL_GPU_BUFFERUSAGE_READWRITEDRAW);
    if (!field->lines.buffer) panic("Failed to create field lines storage buffer!");
    if (!field->line_ids.buffer) panic("Failed to create field line IDs storage buffer!");

    field->enabled = false;
    field->line_count = 0;
    return SDL_APP_CONTINUE;
}

void field_add_body(Field *field, const FieldAddBodyInfo *info) {
    const u32 line_count = (u32) (info->mass / FIELD_LINE_VOLUME);
    const usize lines_size = line_count * FIELD_LINE_LENGTH * sizeof(HMM_Vec2);
    ExpandGPUArray(&field->lines, info->gpu, info->copy_pass, lines_size);
    field->lines.used += lines_size;

    u32 *line_ids = SDL_malloc(line_count * sizeof(u32));
    for (u32 i = 0; i < line_count; i++) line_ids[i] = info->index;
    AppendGPUArrays(info->gpu, info->copy_pass, &(AppendGPUArrayBinding) {
        .array = &field->line_ids,
        .source = (u8*) line_ids,
        .size = line_count * sizeof(u32)
    }, 1);
    SDL_free(line_ids);

    field->line_count += line_count;
}

void field_update(
    const Field *field,
    const Simulation *sim,
    SDL_GPUCommandBuffer *command_buffer,
    SDL_GPUComputePass *compute_pass
) {
    if (!field->line_count || !field->enabled) return;
    const struct {
        u32 body_count;
        f32 G;
        f32 ee;
    } constants = {
        sim->body_count,
        sim->options.gravity,
        sim->options.softening,
    };

    SDL_PushGPUComputeUniformData(command_buffer, 0, &constants, sizeof(constants));
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, (SDL_GPUBuffer*[]) {
        field->lines.buffer,
        field->line_ids.buffer,
        sim->positions.buffer,
        sim->masses.buffer
    }, 4);

    SDL_BindGPUComputePipeline(compute_pass, field->pipeline);
    for (u32 i = 0; i < FIELD_LINE_LENGTH; i++) {
        SDL_PushGPUComputeUniformData(command_buffer, 1, &i, sizeof(i));
        SDL_DispatchGPUCompute(compute_pass, field->line_count, 1, 1);
    }
}

void field_free(const Field *field, SDL_GPUDevice *gpu) {
    SDL_ReleaseGPUBuffer(gpu, field->lines.buffer);
    SDL_ReleaseGPUBuffer(gpu, field->line_ids.buffer);
    SDL_ReleaseGPUComputePipeline(gpu, field->pipeline);
}
