// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "opencl_utils.h"

#include <cstdint>

#ifdef _WIN32
bool OpenCLGpuAvailable()
{
    return false;
}
#else

#include <dlfcn.h>
#include <vector>

namespace {
struct OpenCLSymbols {
    void* handle = nullptr;
    using cl_int = int32_t;
    using cl_uint = uint32_t;
    struct _cl_platform_id;
    struct _cl_device_id;
    using cl_platform_id = _cl_platform_id*;
    using cl_device_id = _cl_device_id*;
    using cl_device_type = uint64_t;

    using clGetPlatformIDsFn = cl_int (*)(cl_uint, cl_platform_id*, cl_uint*);
    using clGetDeviceIDsFn = cl_int (*)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);

    clGetPlatformIDsFn clGetPlatformIDs = nullptr;
    clGetDeviceIDsFn clGetDeviceIDs = nullptr;
};

OpenCLSymbols LoadOpenCL() {
    OpenCLSymbols symbols;
    const char* kLibraries[] = {
        "libOpenCL.so",
        "libOpenCL.so.1",
        "libOpenCL.dylib",
        "/System/Library/Frameworks/OpenCL.framework/OpenCL",
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

    symbols.clGetPlatformIDs = reinterpret_cast<OpenCLSymbols::clGetPlatformIDsFn>(
        dlsym(symbols.handle, "clGetPlatformIDs"));
    symbols.clGetDeviceIDs = reinterpret_cast<OpenCLSymbols::clGetDeviceIDsFn>(
        dlsym(symbols.handle, "clGetDeviceIDs"));

    if (!symbols.clGetPlatformIDs || !symbols.clGetDeviceIDs) {
        dlclose(symbols.handle);
        symbols.handle = nullptr;
    }

    return symbols;
}
} // namespace

bool OpenCLGpuAvailable()
{
    OpenCLSymbols symbols = LoadOpenCL();
    if (!symbols.handle) {
        return false;
    }

    OpenCLSymbols::cl_uint platform_count = 0;
    if (symbols.clGetPlatformIDs(0, nullptr, &platform_count) != 0 || platform_count == 0) {
        dlclose(symbols.handle);
        return false;
    }

    const OpenCLSymbols::cl_device_type kGpuDeviceType = 1ull << 2;
    std::vector<OpenCLSymbols::cl_platform_id> platforms(platform_count, nullptr);
    if (symbols.clGetPlatformIDs(platform_count, platforms.data(), nullptr) != 0) {
        dlclose(symbols.handle);
        return false;
    }
    bool gpu_found = false;
    for (OpenCLSymbols::cl_platform_id platform : platforms) {
        if (!platform) {
            continue;
        }
        OpenCLSymbols::cl_uint device_count = 0;
        if (symbols.clGetDeviceIDs(platform, kGpuDeviceType, 0, nullptr, &device_count) == 0 &&
            device_count > 0) {
            gpu_found = true;
            break;
        }
    }

    dlclose(symbols.handle);
    return gpu_found;
}

#endif
