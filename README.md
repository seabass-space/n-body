# N-Body Simulation

A basic n-body simulation explorer to help learn C, Raylib, and the RK4 iterative method, and to play around with the weirdness of Newtonian gravity!

## Controls

| Control | Action |
| ------- | ------ |
| `space` | Toggle Pause |
| Right-Mouse Drag | Pan Camera |
| Mouse Scroll | Zoom Camera |
| Left-Click Planet | Select Target |
| `c` | Toggle Planet Creation |
| `m` | Toggle if Planet is Movable |
| Left-Mouse Drag | Set New Planet Velocity |

## Wants
(in somewhat increasing order of i-dont-know-what-im-doing)
- Fixed timestep for stability
- Classical gravitational field viewer
- Arena + dynamic arrays
- Trajectory viewer?
- Web build
- RK4 vs Verlet? Barnes-Hut?
- Scene saver / loader
    - Three body problem
    - Solar system
    - Lagrange point demonstration
- Parallelism with multi-threading or compute shader??
- Interactive character like KSP?!!

## Running
To run (on macOS anyway):

```bash
brew install raylib pkg-config
git clone https://github.com/seabass-space/n-body
cd n-body
make run
```

For other platforms, the only external dependency is [Raylib](https://www.raylib.com/), so follow build instructions on the [wiki](https://github.com/raysan5/raylib/wiki).
