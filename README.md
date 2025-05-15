<div align="center">  
  <h1 align="center">  
    <img src="Report and Figures/im0.png" alt="App Icon" width="400">  
    <br>ZNCC Algorithm-Based Stereo Disparity Computation Using OpenCL
  </h1>  

  <h3>OpenCL, OpenMP, C, NVIDIA CUDA Driver, loadpng, Visual Studio Code, MinGW ⚙️</h3>  

  <p align="center">  
    <img src="https://img.shields.io/badge/NVIDIA_CUDA-red?style=flat&logo=nvidia&logoColor=white&labelColor=gray" alt="NVIDIA CUDA" />  
    <img src="https://img.shields.io/badge/OpenCL-orange?style=flat&logo=opencl&logoColor=white&labelColor=gray" alt="OpenCL" />  
    <img src="https://img.shields.io/badge/OpenMP-red?style=flat&logo=openmp&logoColor=white&labelColor=gray" alt="OpenMP" />  
    <img src="https://img.shields.io/badge/C-language-blue?style=flat&logo=c&logoColor=white&labelColor=gray" alt="C Language" />  
    <img src="https://img.shields.io/badge/Visual_Studio_Code-blue?style=flat&logo=visual-studio-code&logoColor=white&labelColor=gray" alt="VS Code" />  
    <img src="https://img.shields.io/badge/MinGW-blue?style=flat&logo=gnu&logoColor=white&labelColor=gray" alt="MinGW" />  
  </p>  
  <p align="center">  
    <img src="https://img.shields.io/github/license/Abu-Taher-web/Multiprocessor-Programming-Course?style=for-the-badge&color=5D6D7E" alt="GitHub license" />  
    <img src="https://img.shields.io/github/last-commit/Abu-Taher-web/Multiprocessor-Programming-Course?style=for-the-badge&color=5D6D7E" alt="GitHub last commit" />  
    <img src="https://img.shields.io/github/commit-activity/m/Abu-Taher-web/Multiprocessor-Programming-Course?style=for-the-badge&color=5D6D7E" alt="GitHub commit activity" />  
    <img src="https://img.shields.io/github/languages/top/Abu-Taher-web/Multiprocessor-Programming-Course?style=for-the-badge&color=5D6D7E" alt="GitHub top language" />  
  </p>  
</div>

## Overview

This project implements a highly optimized **ZNCC (Zero-mean Normalized Cross Correlation) stereo disparity computation algorithm** using OpenCL for GPU acceleration. Stereo disparity maps provide depth information from pairs of stereo images and are crucial for applications like 3D reconstruction, robotics, autonomous driving, and augmented reality.

The core contribution of this work lies in transitioning a computationally expensive local stereo matching algorithm from a single-threaded CPU implementation to a fully parallelized GPU implementation with advanced optimizations. This results in drastic speedups, enabling near real-time performance for disparity map calculation.

Key highlights include:

- Multithreaded CPU implementation using **OpenMP** for parallel processing across CPU cores.  
- GPU implementation using **OpenCL kernels**, optimized to leverage device memory hierarchy (local, private, global memory).  
- Performance tuning via **vectorization**, **memory coalescing**, and **register usage reduction**.  
- Benchmarking across multiple phases showcasing speed improvements from seconds to milliseconds.

<div align="center">  
  <img src="Report and Figures/depth-image.png" alt="App Icon" width="600">    
  <p align="center"> <i> Figure 1: Intermediate images during disparity map calculations.</i></p>   
</div>

---



## Key Highlights

The project demonstrates significant performance improvements through progressive optimization phases:

| Phase                     | Average Execution Time (Left Disparity) | Speedup vs Single-thread CPU |
|---------------------------|-----------------------------------------|-----------------------------|
| Single-threaded CPU       | ~30 seconds                            | Baseline                    |
| Multi-threaded CPU (OpenMP) | ~7.5 seconds                          | ~4× faster                  |
| GPU Unoptimized (OpenCL) | ~120 milliseconds                      | ~250× faster                |
| GPU Optimized (OpenCL)    | ~55 milliseconds                       | ~550× faster                |

**Key Observations:**  
- The transition from CPU to GPU reduced execution time by over two orders of magnitude.  
- Optimizations such as vector data types, local memory usage, and memory coalescing further halved the GPU execution time.  
- The optimized OpenCL kernels achieve efficient utilization of GPU resources, delivering high-throughput stereo disparity computation suitable for real-time applications.

<div align="center">  
  <img src="Report and Figures/disparity_times.png" alt="App Icon" width="500">    
  <p align="center"> <i> Figure 2: Execution time comparison between single and multi-thread implementation</i></p>   
</div>


<div align="center">  
  <img src="Report and Figures/disparity_comparison_marked.png" alt="App Icon" width="800">    
  <p align="center"> <i>Figure 3: Execution time for zncc_left() and zncc_right() for unoptimized, optimized and ultra-optimized cases. </i></p>   
</div>

---

## Tools and Technologies

- **Programming Languages:** C for CPU implementation and OpenCL C for GPU kernels.  
- **Parallel Computing:** OpenMP for CPU multithreading; OpenCL for cross-platform GPU acceleration.  
- **Hardware Platform:** NVIDIA GPU with CUDA driver supporting OpenCL execution.  
- **Image Processing:** PNG image loading and saving via the lightweight [LodePNG](https://lodev.org/lodepng/) library.  
- **Development Environment:** Visual Studio Code on Windows with MinGW GCC compiler for building the project.  
- **Profiling and Benchmarking:** Execution time measured rigorously over multiple runs to ensure accuracy.

---


## How to Use This Repository

1. **Clone the repository:**
```bash
git clone https://github.com/Abu-Taher-web/Multi-Processor-Programming.git
cd Multi-Processor-Programming

```

2. Setup your environment:

- Install MinGW or another GCC compiler for Windows.
- Ensure NVIDIA CUDA drivers are installed for GPU support.
- Confirm that OpenCL drivers for your GPU are correctly installed.

3. Build the project:

- Open the solution or run provided build scripts (adjust as needed for your environment).

- The project is configured for Visual Studio Code with C/C++ extensions and MinGW compiler.

4. Run the disparity computation:

- Use the executable to process stereo image pairs (PNG format).

- Output includes left and right disparity maps saved as images.

5. Experiment with parameters:

- Adjust disparity range (MAX_DISP) and window size for patch matching in source code.

- Explore the benchmarking scripts to measure performance on your hardware.

## Contributions and Further Work
This repository contains well-commented, modular code ideal for extending with:

 - Additional stereo matching algorithms or global optimization methods.

- Integration into real-time vision pipelines or robotics applications.

- Further GPU kernel optimizations and support for other hardware platforms.

### License
- This project is released under the MIT License — see LICENSE for details.

### Contact
For questions, suggestions, or collaborations, please reach out via GitHub or email at abutaher.kuet.ece@gmail.com.
