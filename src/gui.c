#include "gui.h"
#include "simulation.h"
#include "camera.h"
#include "ghost.h"
#include "trajectories.h"
#include "field.h"
#include "graphics.h"

#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

void gui_init(Gui *gui, SDL_Window *window, SDL_GPUDevice *gpu) {
    CIMGUI_CHECKVERSION();
    ImGui_CreateContext(NULL);
    gui->io = ImGui_GetIO();
    gui->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    gui->io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    gui->io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    gui->io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_StyleColorsDark(NULL);
    const f32 main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    ImGuiStyle *style = ImGui_GetStyle();
    ImGuiStyle_ScaleAllSizes(style, main_scale);
    style->FontScaleDpi = main_scale;

    cImGui_ImplSDL3_InitForSDLGPU(window);
    cImGui_ImplSDLGPU3_Init(&(ImGui_ImplSDLGPU3_InitInfo) {
        .Device = gpu,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu, window),
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
        .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        .PresentMode = SDL_GPU_PRESENTMODE_VSYNC
    });
}

static void HelpMarker(const char *desc);
static void gui_controls(SimulationOptions *sim, Ghost *ghost);
static void gui_visualizations(GraphicsOptions *graphics, TrajectoryOptions *trajectories, FieldOptions *field);
static void gui_options(ApplicationOptions *app, SimulationOptions *sim, GraphicsOptions *gfx);
void gui_update(const GuiUpdateInfo *info) {
    cImGui_ImplSDLGPU3_NewFrame();
    cImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();

    static bool open = true;
    if (open) {
        ImGui_Begin("HYENA: N-Body Simulator", &open, ImGuiWindowFlags_AlwaysAutoResize);
        gui_controls(&info->sim->options, info->ghost);
        gui_visualizations(&info->gfx->options, &info->trajectories->options, &info->field->options);
        gui_options(info->app, &info->sim->options, &info->gfx->options);
        ImGui_End();
    }

    ImGui_Render();
}

static void gui_controls(SimulationOptions *sim, Ghost *ghost) {
    if (ImGui_CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui_Checkbox("Pause", &sim->paused);
        ImGui_Checkbox("Create bodies!", &ghost->enabled);
        HelpMarker("To create a new body: activate body creation mode, hold right click where you want to create the new body, drag out its velocity, and release!");
        if (ghost->enabled) {
            ImGui_DragFloat("Mass", &ghost->mass);
            HelpMarker("The mass of the new body.");
            ImGui_ColorEdit3("Color", (f32*) &ghost->color, 0);
            HelpMarker("The color of the new body.");
            ImGui_Checkbox("Movable", &ghost->movable);
            HelpMarker("Whether the body should be simulated or remain in place.");
        }
    }
}

static void gui_visualizations(GraphicsOptions *graphics, TrajectoryOptions *trajectories, FieldOptions *field) {
    if (ImGui_CollapsingHeader("Visualizations", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui_Checkbox("Show body trails", &graphics->trails);
        HelpMarker("Show where the bodies have been.");

        ImGui_Checkbox("Show body trajectories", &trajectories->enabled);
        HelpMarker("Simulate bodies into the future and draw their trajectories (expensive compute for lots of bodies!)");
        if (trajectories->enabled) ImGui_DragFloat("Trajectory Time Step Multiplier", &trajectories->delta_time_multiplier);

        ImGui_Checkbox("Show gravitational field", &field->enabled);
        HelpMarker("The gravitational field represented with streamlines (expensive compute for lots of bodies!)");
        if (field->enabled) {
            ImGui_DragFloat("Field Line Volume", &field->line_volume);
            ImGui_DragFloat("Field Line Step", &field->line_step);
        }

        ImGui_Checkbox("Show gravitational potential", &graphics->potential);
        HelpMarker("The gravitational potential represented with color intensity.");
    }
}

static void gui_options(ApplicationOptions *app, SimulationOptions *sim, GraphicsOptions *gfx) {
    if (ImGui_CollapsingHeader("Options", 0)) {
        ImGui_SeparatorText("Simulation Options");
        ImGui_DragFloat("Time Step", &app->fixed_delta_time);
        ImGui_DragFloat("Gravity Coefficient", &sim->gravity);
        HelpMarker("Strength of the gravitational force between two bodies.");
        ImGui_DragFloat("Softening Coefficient", &sim->softening);
        HelpMarker("How much to reduce the gravitational force between two bodies on close encounter for numerical stability.");
        ImGui_DragFloat("Density Coefficient", &sim->density);
        HelpMarker("How dense each body is.");
        const char *integrators[] = { "Semi-Implicit Euler", "Velocity Verlet", "Runge-Kutta 4" };
        ImGui_ComboChar("Integrator", (i32*) &sim->integrator, integrators, IM_COUNTOF(integrators));
        HelpMarker("The algorithm used to calculate the new velocity and position of each body given the acceleration. Euler is the most performant, Verlet is more accurate while still conserving energy, and RK4 is the most accurate across short time spans but does not conserve energy.");

        ImGui_SeparatorText("Drawing Options");
        ImGui_ColorEdit3("Space Color", (f32*) &gfx->clear_color, 0);
        ImGui_SliderFloat("Movable Body Outline", &gfx->movable_outline, 0.0f, 1.0f);
        HelpMarker("The thickness of the outline around movable bodies.");
        ImGui_SliderFloat("Static Body Outline", &gfx->static_outline, 0.0f, 1.0f);
        HelpMarker("The thickness of the outline around non-movable bodies.");
        ImGui_SliderFloat("Trail brightness", &gfx->trail_brightness, 0.0f, 1.0f);
        HelpMarker("The brightness of the trail that each body leaves behind as it moves.");
    }
}

static void HelpMarker(const char *desc) {
    ImGui_SameLine();
    ImGui_TextDisabled("(?)");
    if (ImGui_BeginItemTooltip()) {
        ImGui_PushTextWrapPos(ImGui_GetFontSize() * 35.0f);
        ImGui_TextUnformatted(desc);
        ImGui_PopTextWrapPos();
        ImGui_EndTooltip();
    }
}

void gui_event(const SDL_Event *event) {
    cImGui_ImplSDL3_ProcessEvent(event);
}

void gui_free(void) {
    cImGui_ImplSDL3_Shutdown();
    cImGui_ImplSDLGPU3_Shutdown();
    ImGui_DestroyContext(NULL);
}
