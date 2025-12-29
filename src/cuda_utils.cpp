// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cuda_utils.h"

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
bool CudaGpuAvailable()
{
    return false;
}

std::vector<std::string> ListCudaDevices(std::string* error_out)
{
    if (error_out) {
        *error_out = "CUDA device listing is not supported on this platform.";
    }
    return {};
}
#else

#include <dlfcn.h>

namespace {
struct CudaSymbols {
    void* handle = nullptr;
    using CUresult = int;
    using CUdevice = int;

    using cuInitFn = CUresult (*)(unsigned int);
    using cuDeviceGetCountFn = CUresult (*)(int*);
    using cuDeviceGetNameFn = CUresult (*)(char*, int, CUdevice);

    cuInitFn cuInit = nullptr;
    cuDeviceGetCountFn cuDeviceGetCount = nullptr;
    cuDeviceGetNameFn cuDeviceGetName = nullptr;
};

CudaSymbols LoadCuda()
{
    CudaSymbols symbols;
    const char* kLibraries[] = {
        "libcuda.so",
        "libcuda.so.1",
        "libcuda.dylib",
    };
    for (const char* lib : kLibraries) {
        symbols.handle = dlopen(lib, RTLD_LAZY);
        if (symbols.handle) {
            break;
        }
    }
    if (!symbols.handle) {
        return symbols;
    }

    symbols.cuInit = reinterpret_cast<CudaSymbols::cuInitFn>(dlsym(symbols.handle, "cuInit"));
    symbols.cuDeviceGetCount = reinterpret_cast<CudaSymbols::cuDeviceGetCountFn>(
        dlsym(symbols.handle, "cuDeviceGetCount"));
    symbols.cuDeviceGetName = reinterpret_cast<CudaSymbols::cuDeviceGetNameFn>(
        dlsym(symbols.handle, "cuDeviceGetName"));

    if (!symbols.cuInit || !symbols.cuDeviceGetCount || !symbols.cuDeviceGetName) {
        dlclose(symbols.handle);
        symbols.handle = nullptr;
    }

    return symbols;
}
} // namespace

bool CudaGpuAvailable()
{
    CudaSymbols symbols = LoadCuda();
    if (!symbols.handle) {
        return false;
    }

    if (symbols.cuInit(0) != 0) {
        dlclose(symbols.handle);
        return false;
    }

    int device_count = 0;
    if (symbols.cuDeviceGetCount(&device_count) != 0 || device_count <= 0) {
        dlclose(symbols.handle);
        return false;
    }

    dlclose(symbols.handle);
    return true;
}

std::vector<std::string> ListCudaDevices(std::string* error_out)
{
    if (error_out) {
        error_out->clear();
    }

    CudaSymbols symbols = LoadCuda();
    if (!symbols.handle) {
        if (error_out) {
            *error_out = "CUDA runtime not found.";
        }
        return {};
    }

    if (symbols.cuInit(0) != 0) {
        if (error_out) {
            *error_out = "Failed to initialize CUDA driver.";
        }
        dlclose(symbols.handle);
        return {};
    }

    int device_count = 0;
    if (symbols.cuDeviceGetCount(&device_count) != 0) {
        if (error_out) {
            *error_out = "Failed to query CUDA devices.";
        }
        dlclose(symbols.handle);
        return {};
    }

    if (device_count <= 0) {
        if (error_out) {
            *error_out = "No CUDA devices found.";
        }
        dlclose(symbols.handle);
        return {};
    }

    std::vector<std::string> devices;
    devices.reserve(static_cast<size_t>(device_count));
    for (int device_index = 0; device_index < device_count; ++device_index) {
        char name[256] = {};
        if (symbols.cuDeviceGetName(name, sizeof(name), device_index) == 0) {
            devices.emplace_back("CUDA Device " + std::to_string(device_index) + ": " + name);
        } else {
            devices.emplace_back("CUDA Device " + std::to_string(device_index) + ": Unknown device");
        }
    }

    dlclose(symbols.handle);
    return devices;
}

#endif
