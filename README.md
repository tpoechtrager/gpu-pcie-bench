# gpu-pcie-bench

GPU PCIe Bandwidth Benchmark

---

## What is this?

`gpu-pcie-bench` measures the data transfer bandwidth between your CPU host memory and GPU device memory over the PCIe bus using OpenCL.  
It benchmarks Host-to-Device and Device-to-Host transfer speeds for configurable buffer sizes and iteration counts.

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
- For cross-compilation: MinGW-w64 toolchain + Windows OpenCL SDK

---

## Usage

Run

    ./gpu-pcie-bench --help

to see all options.

---

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

---

## Example

    ./gpu-pcie-bench

Or run benchmark for 25 iterations, buffer sizes 1MB, 10MB, 100MB:

    ./gpu-pcie-bench --rounds 25 --sizes 1,10,100 --direction both --unit gb

Example Output:

```shell
CPU: AMD Ryzen 9 9950X3D 16-Core Processor
GPU: gfx1201 (This is an AMD RX 9070 XT)

[Buffer size: 1 MB]
Iteration 100/100
Host to Device:
Avg: 7.29 GB/s
Min: 2.44 GB/s
Max: 10.04 GB/s
Device to Host:
Avg: 8.67 GB/s
Min: 2.56 GB/s
Max: 11.50 GB/s

[Buffer size: 10 MB]
Iteration 100/100
Host to Device:
Avg: 31.44 GB/s
Min: 11.54 GB/s
Max: 35.69 GB/s
Device to Host:
Avg: 33.27 GB/s
Min: 10.17 GB/s
Max: 38.49 GB/s

[Buffer size: 100 MB]
Iteration 100/100
Host to Device:
Avg: 47.94 GB/s
Min: 17.08 GB/s
Max: 49.69 GB/s
Device to Host:
Avg: 48.95 GB/s
Min: 17.26 GB/s
Max: 50.67 GB/s

[Buffer size: 512 MB]
Iteration 100/100
Host to Device:
Avg: 50.32 GB/s
Min: 18.60 GB/s
Max: 51.53 GB/s
Device to Host:
Avg: 51.24 GB/s
Min: 19.21 GB/s
Max: 52.26 GB/s

[Buffer size: 1024 MB]
Iteration 100/100
Host to Device:
Avg: 50.59 GB/s
Min: 17.86 GB/s
Max: 51.83 GB/s
Device to Host:
Avg: 51.28 GB/s
Min: 16.56 GB/s
Max: 52.50 GB/s

[Buffer size: 2048 MB]
Iteration 100/100
Host to Device:
Avg: 35.02 GB/s
Min: 21.83 GB/s
Max: 37.99 GB/s
Device to Host:
Avg: 35.37 GB/s
Min: 31.20 GB/s
Max: 38.08 GB/s
```

## License

MIT

This tool was created with the help of ChatGPT. While it may not be perfect, it does its job.
