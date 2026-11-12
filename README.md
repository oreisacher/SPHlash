<h1 align="center">
  SPHlash - SPH fluid simulation
</h1>
<p align="center">
  <img width="1968" height="1080" alt="SPHlashTitle" src="https://github.com/user-attachments/assets/54aa9760-2e69-4434-b1e0-7f4bbbb2ccc6" />
</p>

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.24%2B-orange.svg)

SPHlash is a CUDA-based Smoothed Particle Hydrodynamics (SPH) fluid simulator with an interactive editor.

The implementation draws inspiration from the following publications:
- Monaghan, Joe J. "Smoothed particle hydrodynamics." Reports on progress in physics 68.8 (2005): 1703-1759. https://doi.org/10.1088/0034-4885/68/8/R01
- Müller, Matthias, David Charypar, and Markus Gross. "Particle-based fluid simulation for interactive applications." Proceedings of the 2003 ACM SIGGRAPH/Eurographics symposium on Computer animation. 2003.
- Becker, Markus, and Matthias Teschner. "Weakly compressible SPH for free surface flows." Proceedings of the 2007 ACM SIGGRAPH/Eurographics symposium on Computer animation. 2007.
- Bender, Jan, et al. "Volume maps: An implicit boundary representation for SPH." Proceedings of the 12th ACM SIGGRAPH Conference on Motion, Interaction and Games. 2019. https://doi.org/10.1145/3359566.3360077

## Features
- **GPU-Accelerated Simulation:** Real-time fluid simulation powered by CUDA
- **Boundary Geometry:** Import 3D models to define solid boundaries
- **Scene Management:** Create, edit, import and export simulation scenes
- **Render Modes:** Visualize fluids using particle rendering, marching cubes (smooth and non-smooth), or screen-space rendering
- **Simulation Analysis:** Inspect and export simulation metrics
- **Video Recording:** Record and export simulation sequences 

## Build

### Requirements

- **GPU:** NVIDIA GPU with CUDA compute capability 6.0+
- **CMake:** 3.24+
- **CUDA Toolkit:** 12.9+
- **Compiler:** GCC 10+ or Clang 12+ (C++20 support required)
- **FFmpeg:** Optional — required for video recording (`libavformat`, `libavcodec`, `libavutil`, `libswscale`, `libswresample`)

### Clone the repository

```bash
git clone --recursive git@github.com:oreisacher/SPHlash.git
cd SPHlash
```

The `--recursive` flag is important to initialize the Git submodules.

### Configure and build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCUDAToolkit_ROOT=/path/to/CUDAToolkit -DCMAKE_CUDA_COMPILER=/path/to/nvcc
cmake --build . --parallel
```

Assets are automatically copied to the build directory.

### Running

```bash
./SPHlash
```

## Usage

### Controls

#### Simulation

| Key | Action |
|-----|--------|
| `Space` | Play / Pause simulation |
| `R` | Init (when uninitialized) / Reset simulation |
| `P` | Step one simulation frame (when paused) |

#### Camera Navigation

| Key / Mouse | Action |
|-------------|--------|
| `W` `A` `S` `D` | Move forward, left, backward, right |
| `E` / `Q` | Move up / down |
| `Left Shift` | Hold to increase movement speed |
| Arrow keys | Rotate camera (pitch / yaw) |
| Right mouse button | Rotate camera (look) |
| `Ctrl` + Left mouse button | Pan camera |
| Mouse scroll | Zoom (perspective: move forward/backward; orthographic: scale) |

#### Camera Presets

| Key | Action |
|-----|--------|
| `F1` | Front view (along Z-axis) |
| `F2` | Side view (along X-axis) |
| `F3` | Top view (along Y-axis) |
| `F5` | Switch to perspective projection |
| `F6` | Switch to orthographic projection |

#### Transform Gizmo

| Key | Action |
|-----|--------|
| `F` | Translate mode |
| `G` | Rotate mode |
| `H` | Scale mode |
| `J` | World space |
| `K` | Local space |

#### Misc

| Key | Action |
|-----|--------|
| `Escape` | Deselect current scene node |

## License

SPHlash is released under the **MIT License**. See the [LICENSE](LICENSE) file for details.
