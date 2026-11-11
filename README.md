<p align="center">
  <img width="1968" height="1080" alt="SPHlashTitle" src="https://github.com/user-attachments/assets/54aa9760-2e69-4434-b1e0-7f4bbbb2ccc6" />
</p>

**SPHlash** is a real-time, CUDA-based **Smoothed Particle Hydrodynamics (SPH)** fluid simulator with an interactive editor.

TODO PAPER

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.24%2B-orange.svg)

## Third-Party Libraries

---

TODO

## Build

---

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

---

### Controls

TODO

### Editor Panels

- **Control Panel** — TODO
- **Scene Panel** — TODO
- **Properties Panel** — TODO
- **Simulation Settings** — TODO
- **Rendering Settings** — TODO
- **Particles Panel** — TODO
- **Query Data Panel** — TODO
- **Recording Panel** — Start/stop video recording (requires FFmpeg)

## License

---

SPHlash is released under the **MIT License**. See the [LICENSE](LICENSE) file for details.

Some included assets and third-party libraries have their own licenses. See `assets/textures/License.txt` for texture licensing information.
