/*
  ------------------------------------------------------------------------------
  gpu-pcie-bench: GPU <-> Host Bandwidth Benchmark via OpenCL
  ------------------------------------------------------------------------------

  This tool measures memory transfer bandwidth between host (CPU RAM) and GPU
  using OpenCL buffer transfers over the PCIe bus.

  It supports:
    - Measuring Host to Device (write) and Device to Host (read) separately
    - Configurable buffer sizes and iteration counts
    - Direction control: --direction host2dev | dev2host | both
    - Output unit: --unit mb | gb (default is gb)
    - Real-time progress display
    - Clean summary output: min / avg / max bandwidth
    - CPU and GPU name output (GPU name with memory in MB)
    - GPU memory size aware buffer size filtering for standard sizes
    - Version info via --version

  Requires:
    - OpenCL runtime and ICD loader installed
    - GPU with OpenCL support

  Usage example:
    gpu-pcie-bench --rounds 50 --sizes 512K,1M,10M --direction both --unit gb
*/

#define VERSION "1.1"

#define CL_TARGET_OPENCL_VERSION 120
#define CHECK(status, msg) \
  do { \
    if ((status) != CL_SUCCESS) { \
      std::cerr << (msg) << " (" << (status) << ")\n"; \
      return 1; \
    } \
  } while (0)

#include <CL/cl.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <limits>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <intrin.h> // __cpuid
#else
#include <fstream>
#endif

#ifdef _WIN32
std::string get_cpu_name() {
  int cpuInfo[4] = {0};
  char cpuBrand[0x40] = {0};
  __cpuid(cpuInfo, 0x80000000);
  unsigned int nExIds = cpuInfo[0];
  if (nExIds >= 0x80000004) {
    __cpuid(reinterpret_cast<int*>(cpuInfo), 0x80000002);
    memcpy(cpuBrand, cpuInfo, sizeof(cpuInfo));
    __cpuid(reinterpret_cast<int*>(cpuInfo), 0x80000003);
    memcpy(cpuBrand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(reinterpret_cast<int*>(cpuInfo), 0x80000004);
    memcpy(cpuBrand + 32, cpuInfo, sizeof(cpuInfo));
    return std::string(cpuBrand);
  }
  return "Unknown CPU";
}
#else
std::string get_cpu_name() {
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line;
  while (std::getline(cpuinfo, line)) {
    if (line.find("model name") != std::string::npos) {
      auto pos = line.find(":");
      if (pos != std::string::npos) {
        return line.substr(pos + 2);
      }
    }
  }
  return "Unknown CPU";
}
#endif

enum class Direction {
  HostToDevice,
  DeviceToHost,
  Both
};

enum class Unit {
  MBps,
  GBps
};

void print_help() {
  std::cout << "gpu-pcie-bench version " << VERSION << "\n"
            << "GPU <-> Host Bandwidth Benchmark via OpenCL\n\n"
            << "Measures transfer speeds between your GPU and system memory over PCIe using OpenCL.\n\n"
            << "Usage: gpu-pcie-bench [options]\n"
            << "Options:\n"
            << "  --device N           Target gpu device (default: 0)\n"
            << "  --rounds N           Number of iterations per test (default: 100)\n"
            << "  --sizes SIZES        Comma-separated buffer sizes with optional units (e.g. 1,10K,100M,1G)\n"
            << "  --direction MODE     Transfer direction: host2dev, dev2host, both (default)\n"
            << "  --unit mb|gb         Output unit (default: gb)\n"
            << "  --version            Show version info\n"
            << "  --help               Show this help message\n";
}

std::vector<size_t> parse_sizes(const std::string& str) {
  std::vector<size_t> result;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, ',')) {
    try {
      size_t multiplier = 1;
      std::string numberPart = item;
      if (!item.empty()) {
        char lastChar = item.back();
        if (lastChar == 'K' || lastChar == 'k') {
          multiplier = 1024ULL;
          numberPart = item.substr(0, item.size() - 1);
        } else if (lastChar == 'M' || lastChar == 'm') {
          multiplier = 1024ULL * 1024ULL;
          numberPart = item.substr(0, item.size() - 1);
        } else if (lastChar == 'G' || lastChar == 'g') {
          multiplier = 1024ULL * 1024ULL * 1024ULL;
          numberPart = item.substr(0, item.size() - 1);
        }
      }
      size_t val = static_cast<size_t>(std::stoull(numberPart));
      result.push_back(val * multiplier);
    } catch (...) {
      std::cerr << "Invalid size: " << item << "\n";
    }
  }
  return result;
}

Direction parse_direction(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower == "host2dev") return Direction::HostToDevice;
  if (lower == "dev2host") return Direction::DeviceToHost;
  if (lower == "both")     return Direction::Both;
  std::cerr << "Unknown direction: " << s << "\n";
  exit(1);
}

Unit parse_unit(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower == "mb") return Unit::MBps;
  if (lower == "gb") return Unit::GBps;
  std::cerr << "Unknown unit: " << s << "\n";
  exit(1);
}

void filter_static_sizes_by_gpu_memory(std::vector<size_t>& sizes, size_t gpuMemSize) {
  std::vector<size_t> staticSizes = {
    512 * 1024,
    1 * 1024 * 1024,
    10 * 1024 * 1024,
    100 * 1024 * 1024,
    512 * 1024 * 1024
  };

#if UINTPTR_MAX == 0xffffffffffffffff
  staticSizes.push_back(1024ULL * 1024 * 1024);    // 1 GB
  staticSizes.push_back(2048ULL * 1024 * 1024);    // 2 GB
  staticSizes.push_back(4096ULL * 1024 * 1024);    // 4 GB
#endif

  for (size_t sz : staticSizes) {
    if (sz > gpuMemSize / 4) {
      sizes.erase(std::remove(sizes.begin(), sizes.end(), sz), sizes.end());
    } else {
      if (std::find(sizes.begin(), sizes.end(), sz) == sizes.end()) {
        sizes.push_back(sz);
      }
    }
  }

  std::sort(sizes.begin(), sizes.end());
}

std::string format_size(size_t bytes) {
  if (bytes >= 1024ULL * 1024 * 1024) {
    return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
  } else if (bytes >= 1024 * 1024) {
    return std::to_string(bytes / (1024 * 1024)) + " MB";
  } else if (bytes >= 1024) {
    return std::to_string(bytes / 1024) + " KB";
  } else {
    return std::to_string(bytes) + " B";
  }
}

double measure(cl_command_queue queue, cl_mem deviceBuf, void* hostPtr, size_t size, bool write) {
  auto start = std::chrono::high_resolution_clock::now();
  cl_int status = write ?
    clEnqueueWriteBuffer(queue, deviceBuf, CL_TRUE, 0, size, hostPtr, 0, nullptr, nullptr) :
    clEnqueueReadBuffer(queue, deviceBuf, CL_TRUE, 0, size, hostPtr, 0, nullptr, nullptr);
  clFinish(queue);
  CHECK(status, write ? "Write failed" : "Read failed");
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double>(end - start).count();
}

int main(int argc, char* argv[]) {
  int rounds = 100;
  int targetDevice = 0;
  Direction direction = Direction::Both;
  Unit unit = Unit::GBps;
  bool userSpecifiedSizes = false;

  std::vector<size_t> sizes = {
    512 * 1024,
    1 * 1024 * 1024,
    10 * 1024 * 1024,
    100 * 1024 * 1024,
    512 * 1024 * 1024,
  };

#if UINTPTR_MAX == 0xffffffffffffffff
  sizes.push_back(1024ULL * 1024 * 1024);   // 1 GB
  sizes.push_back(2048ULL * 1024 * 1024);   // 2 GB
  sizes.push_back(4096ULL * 1024 * 1024);   // 4 GB
#endif

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help") {
      print_help();
      return 0;
    } else if (arg == "--version") {
      std::cout << "gpu-pcie-bench version " << VERSION << "\n";
      return 0;
    } else if (arg == "--rounds" && i + 1 < argc) {
      rounds = std::stoi(argv[++i]);
    } else if (arg == "--sizes" && i + 1 < argc) {
      sizes = parse_sizes(argv[++i]);
      userSpecifiedSizes = true;
    } else if (arg == "--direction" && i + 1 < argc) {
      direction = parse_direction(argv[++i]);
    } else if (arg == "--unit" && i + 1 < argc) {
      unit = parse_unit(argv[++i]);
    } else if (arg == "--device" && i + 1 < argc) {
      targetDevice = std::stoi(argv[++i]);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      print_help();
      return 1;
    }
  }

  // CPU Name
  std::cout << "CPU: " << get_cpu_name() << "\n";

  cl_int status;
  cl_uint numPlatforms = 0;
  CHECK(clGetPlatformIDs(0, nullptr, &numPlatforms), "Failed to get platform count");
  if (numPlatforms == 0) {
    std::cerr << "No OpenCL platforms found.\n";
    return 1;
  }
  std::vector<cl_platform_id> platforms(numPlatforms);
  CHECK(clGetPlatformIDs(numPlatforms, platforms.data(), nullptr), "Failed to get platforms");

  cl_platform_id platform = platforms[0];
  cl_uint numDevices = 0;
  CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices), "Failed to get device count");
  if (numDevices == 0) {
    std::cerr << "No GPU devices found on platform.\n";
    return 1;
  }
  std::vector<cl_device_id> devices(numDevices);
  CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices.data(), nullptr), "Failed to get devices");

  if (targetDevice >= numDevices) {
    std::cerr << "Target GPU device " << targetDevice << " is beyond GPU devices found on platform.\n";
    return 1;
  }

  cl_device_id device = devices[targetDevice];

  // GPU Name
  size_t gpuNameSize = 0;
  CHECK(clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &gpuNameSize), "Failed to get GPU name size");
  std::vector<char> gpuName(gpuNameSize);
  CHECK(clGetDeviceInfo(device, CL_DEVICE_NAME, gpuNameSize, gpuName.data(), nullptr), "Failed to get GPU name");

  cl_ulong gpuMemSize = 0;
  CHECK(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(gpuMemSize), &gpuMemSize, nullptr), "Failed to get GPU memory size");

  std::cout << "GPU: " << std::string(gpuName.data()) << " (" << (gpuMemSize / (1024 * 1024)) << " MB)\n";

  if (!userSpecifiedSizes) {
    filter_static_sizes_by_gpu_memory(sizes, static_cast<size_t>(gpuMemSize));
  }

  if (sizes.empty()) {
    std::cerr << "No buffer sizes fit GPU memory constraints. Exiting.\n";
    return 1;
  }

  cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  CHECK(status, "Failed to create context");

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &status);
  CHECK(status, "Failed to create command queue");

  std::cout << std::fixed << std::setprecision(2);

  for (size_t dataSize : sizes) {
    std::cout << "\n[Buffer size: " << format_size(dataSize) << "]\n";

    cl_mem hostBuf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSize, nullptr, &status);
    CHECK(status, "Failed to allocate pinned host buffer");

    cl_mem recvBuf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dataSize, nullptr, &status);
    CHECK(status, "Failed to allocate pinned receive buffer");

    void* hostPtr = clEnqueueMapBuffer(queue, hostBuf, CL_TRUE, CL_MAP_WRITE, 0, dataSize, 0, nullptr, nullptr, &status);
    CHECK(status, "Failed to map host buffer");

    void* recvPtr = clEnqueueMapBuffer(queue, recvBuf, CL_TRUE, CL_MAP_READ, 0, dataSize, 0, nullptr, nullptr, &status);
    CHECK(status, "Failed to map recv buffer");

    memset(hostPtr, 1, dataSize);

    cl_mem deviceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, dataSize, nullptr, &status);
    CHECK(status, "Failed to allocate device buffer");

    double sumH2D = 0, minH2D = 1e10, maxH2D = 0;
    double sumD2H = 0, minD2H = 1e10, maxD2H = 0;

    for (int i = 0; i < rounds; ++i) {
      std::cout << "\r  Iteration " << (i + 1) << "/" << rounds << std::flush;

      if (direction == Direction::HostToDevice || direction == Direction::Both) {
        double t = measure(queue, deviceBuffer, hostPtr, dataSize, true);
        sumH2D += t;
        minH2D = std::min(minH2D, t);
        maxH2D = std::max(maxH2D, t);
      }

      if (direction == Direction::DeviceToHost || direction == Direction::Both) {
        double t = measure(queue, deviceBuffer, recvPtr, dataSize, false);
        sumD2H += t;
        minD2H = std::min(minD2H, t);
        maxD2H = std::max(maxD2H, t);
      }
    }

    std::cout << std::endl;

    auto convert = [=](double sec) {
      if (unit == Unit::GBps) return dataSize / (sec * 1024.0 * 1024.0 * 1024.0);
      else                    return dataSize / (sec * 1024.0 * 1024.0);
    };
    const char* label = (unit == Unit::GBps) ? "GB/s" : "MB/s";

    if (direction == Direction::HostToDevice || direction == Direction::Both) {
      std::cout << "Host to Device:\n";
      std::cout << "  Avg: " << convert(sumH2D / rounds) << " " << label << "\n";
      std::cout << "  Min: " << convert(maxH2D) << " " << label << "\n";
      std::cout << "  Max: " << convert(minH2D) << " " << label << "\n";
    }

    if (direction == Direction::DeviceToHost || direction == Direction::Both) {
      std::cout << "Device to Host:\n";
      std::cout << "  Avg: " << convert(sumD2H / rounds) << " " << label << "\n";
      std::cout << "  Min: " << convert(maxD2H) << " " << label << "\n";
      std::cout << "  Max: " << convert(minD2H) << " " << label << "\n";
    }

    clEnqueueUnmapMemObject(queue, hostBuf, hostPtr, 0, nullptr, nullptr);
    clEnqueueUnmapMemObject(queue, recvBuf, recvPtr, 0, nullptr, nullptr);

    clReleaseMemObject(hostBuf);
    clReleaseMemObject(recvBuf);
    clReleaseMemObject(deviceBuffer);
  }

#ifdef _WIN32
  std::cout << "\nPress Enter to exit...";
  std::cin.get();
#endif

  clReleaseCommandQueue(queue);
  clReleaseContext(context);
  return 0;
}
