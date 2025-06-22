# gpu-pcie-bench

GPU PCIe Bandwidth Benchmark

---

## What kind of a tool is this?

`gpu-pcie-bench` measures the data transfer bandwidth between your CPU host memory and  
GPU device memory over the PCIe bus using OpenCL.  

It benchmarks Host-to-Device and Device-to-Host transfer speeds for configurable buffer  
sizes and iteration counts.

---

## Features

- Measures Host → GPU and GPU → Host bandwidth separately or both  
- Supports configurable buffer sizes (in MB) and iteration counts  
- Output in MB/s or GB/s (default GB/s)  
- Works on Linux and Windows (via MinGW cross-compile)  
- Simple command-line interface with useful options

---

## Requirements

- OpenCL runtime and drivers installed  
- GPU with OpenCL support  
- For cross-compilation: MinGW-w64 toolchain + Windows OpenCL SDK (included in the repo)

---

## Usage

Run

    ./gpu-pcie-bench --help

to see all options.

    ./gpu-pcie-bench

Or run benchmark for 25 iterations, buffer sizes 1MB, 10MB, 100MB:

    ./gpu-pcie-bench --rounds 25 --sizes 1,10,100 --direction both --unit gb

Example Output (Windows):

```shell
CPU: AMD Ryzen 9 9950X3D 16-Core Processor
GPU: gfx1201 (16304 MB)  (This is an AMD RX 9070 XT)

[Buffer size: 512 KB]
  Iteration 100/100
Host to Device:
  Avg: 5.57 GB/s
  Min: 1.60 GB/s
  Max: 6.80 GB/s
Device to Host:
  Avg: 5.66 GB/s
  Min: 1.57 GB/s
  Max: 6.82 GB/s

[Buffer size: 1 MB]
  Iteration 100/100
Host to Device:
  Avg: 9.85 GB/s
  Min: 3.19 GB/s
  Max: 11.48 GB/s
Device to Host:
  Avg: 10.07 GB/s
  Min: 2.91 GB/s
  Max: 11.68 GB/s

[Buffer size: 10 MB]
  Iteration 100/100
Host to Device:
  Avg: 32.54 GB/s
  Min: 11.53 GB/s
  Max: 36.49 GB/s
Device to Host:
  Avg: 35.64 GB/s
  Min: 12.71 GB/s
  Max: 38.89 GB/s

[Buffer size: 100 MB]
  Iteration 100/100
Host to Device:
  Avg: 48.47 GB/s
  Min: 17.56 GB/s
  Max: 50.02 GB/s
Device to Host:
  Avg: 49.15 GB/s
  Min: 17.31 GB/s
  Max: 50.68 GB/s

[Buffer size: 512 MB]
  Iteration 100/100
Host to Device:
  Avg: 50.53 GB/s
  Min: 18.56 GB/s
  Max: 51.72 GB/s
Device to Host:
  Avg: 51.31 GB/s
  Min: 20.11 GB/s
  Max: 52.37 GB/s

[Buffer size: 1 GB]
  Iteration 100/100
Host to Device:
  Avg: 50.79 GB/s
  Min: 18.26 GB/s
  Max: 52.05 GB/s
Device to Host:
  Avg: 51.33 GB/s
  Min: 16.84 GB/s
  Max: 52.49 GB/s

[Buffer size: 2 GB]
  Iteration 100/100
Host to Device:
  Avg: 39.18 GB/s
  Min: 21.89 GB/s
  Max: 40.15 GB/s
Device to Host:
  Avg: 39.52 GB/s
  Min: 38.83 GB/s
  Max: 40.21 GB/s
```

## Compilation

### Linux (Debian/Ubuntu native build)

Make sure you have OpenCL headers and libraries installed:

    sudo apt update  
    sudo apt install build-essential ocl-icd-opencl-dev opencl-headers

Compile with:

    make

The binary will be created in:

    bin/<ARCH>/gpu-pcie-bench

---

### Windows (cross-compile with MinGW)

Currently the Makefile supports only cross-compilation for Windows.

Install MinGW-w64 toolchain on Debian/Ubuntu:

    sudo apt update  
    sudo apt install mingw-w64

Build 64-bit Windows binary:

    make USE_MINGW=1 ARCH=x86_64

Build 32-bit Windows binary:

    make USE_MINGW=1 ARCH=i686

The .exe will be in:

  `bin/x86_64/gpu-pcie-bench.exe`  or `bin/i686/gpu-pcie-bench.exe`.

## License

MIT

This tool was created with the help of ChatGPT. While it may not be perfect, it does its job.
