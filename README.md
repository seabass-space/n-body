# Hyena: An N-Body Gravity Explorer

A-N-other Body Simulator? An exploration of Newtonian gravity, weirdness of non-inertial frames, instability in long-run predictions, and interactive field visualizations. Visualize Lagrange points, 3-body instability, epicycles/retrograde motion, and maybe more?

(Named for the organized chaos of gravitational systems)

https://github.com/user-attachments/assets/76de4eaa-7438-4ae6-95b4-dbc0b031be2d

## Features

* Real-time, interactive gravity simulation

   https://github.com/user-attachments/assets/ae43d7fb-a828-4cd9-a0aa-a5963fe4501a
  
   (Jupiter's checking in on those pesky terrestial planets)

* GPU-accelerated integration and trajectory prediction

   https://github.com/user-attachments/assets/714ef65e-b027-4944-b8f0-44fb457661b6

* Non-inertial reference frame visualizations

* Gravitational potential visualization

* Implementation of 3 different integrators and deep simulation control

## Motivation

A lot of great n-body simulations go until they can achieve a realistic looking solar system, but I wanted to be able to see the complex nature of the problem by predicting the motion of the bodies far into the future and [feeling the chaos](https://en.wikipedia.org/wiki/Butterfly_effect). I also took the opportunity to explore the weirdness that arises from a non-inertial vantage point, and how that leads to the observed motion of the planets in the night sky.

I'd love for this to eventually become a pedagogical tool, accessible to teachers, students, and the curious, to explore intricacies of this chaos! For instance, gravity tends to be students' first introduction to force and potential fields, something that becomes central to physics in E&M, GR, QFT, and beyond. I'd like to bring in some field visualization tools (field lines, equipotentials, gravity wells) to introduce field concepts in a familiar setting. And it's a pipe dream of mine to sprinkle orbital dynamic explorations into the simulation one day.

A secondary goal of the project is to explore central concepts in numerical analysis. I've done simple implementations of the [Semi-Implicit Euler](src/shaders/simulation/euler.comp.glsl), [Velocity Verlet](src/shaders/simulation/verlet.comp.glsl), and [Runge-Kutta 4](src/shaders/simulation/runge_kutta.comp.glsl) integrators and would like to implement some tools for visualizing their efficacy.

### Wishlist / Todo

1. ~~Barnes Hut optimization~~ too complicated with compute shaders (for my brain anyway)
2. Normalize constants, fix GUI
3. Gravitational field visualizer
   - field lines, equipotential lines, test mass motion
4. Satellite exploration
   - find a way of visualizing Hohmann Transfers (Interplanetary Transport Networks and manifolds?)
   - Lagrange point visualizations
5. Save/load system
6. Kinetic, potential, total energy charts to evaluating simulation stability
7. Web build - dependent on [SDL_gpu support](https://github.com/libsdl-org/SDL/issues/10768)

## Technical Information

The simulation is written in C and GLSL, running as much as I could on the GPU while trying to keep it cross-platform. It works well on macOS and Linux, though I haven't been able to test it on Windows. This is my first exploration into GPU programming - that's been it's own fun big headache...

The heart of the simulation can be found in the [shaders/simulation](src/shaders/simulation) folder, where the three integrators are implemented.

### Libraries

1. [SDL3](https://libsdl.org/) & [SDL_gpu](https://wiki.libsdl.org/SDL3/CategoryGPU) - Cross platform input, windowing, and rendering (supports Metal, DX12, and Vulkan)
2. [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross) - Cross platform shaders
3. [Dear ImGui](https://github.com/ocornut/imgui) & [dear_bindings](https://github.com/dearimgui/dear_bindings) - Immediate mode user interface (with C bindings)
4. [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h) - Generic dynamic arrays
5. [HandmadeMath.h](https://github.com/HandmadeMath/HandmadeMath) - Simple graphic-focused math library

### Required build dependencies
1. [Git](https://git-scm.com/) - Version control
2. [CMake](https://cmake.org/) - Cross platform build system and dependency control
3. [Python 3](https://www.python.org/) - For generating ImGui bindings and compiling shaders
4. [glslang](https://github.com/KhronosGroup/glslang/releases) - Compiling GLSL shaders to SPIR-V
5. A C compiler, like [Clang](https://clang.llvm.org/), [GCC](https://gcc.gnu.org/), or [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/) - For, well, compiling C code.

### Build steps

1. Clone repository

    ```bash
    git clone https://github.com/cbass-space/n-body.git
    cd n-body
    ```

2. Setup up Python build environment

    ```bash
    python3 -m venv .venv      # optional
    source .venv/bin/activate  # -> macos/linux
    .venv\Scripts\Activate.psl # -> windows powershell
    pip install ply==3.11
    ```

3. Generate CMake profile, build and run

    ```bash
   # clang|gcc
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    cd build && cmake --build .
    ./n-body
   
   # msvc
   cmake -B build
   cmake --build build --config Debug
   cd build/Debug
   \n-body.exe
   ```
   
   I reccomend taking a look at [CLion](https://www.jetbrains.com/clion/) or [Visual Studio](https://visualstudio.microsoft.com/downloads/) if you are new to programming C, it should perform the 3rd step automatically.

Here's the only emoji to show this wasn't A.I. generated 𓃥
