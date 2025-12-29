// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "opencl_utils.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
bool OpenCLGpuAvailable()
{
    return false;
}

std::vector<std::string> ListOpenCLDevices(std::string* error_out)
{
    if (error_out) {
        *error_out = "OpenCL device listing is not supported on this platform.";
    }
    return {};
}
#else

#include <dlfcn.h>

namespace {
struct OpenCLSymbols {
    void* handle = nullptr;
    using cl_int = int32_t;
    using cl_uint = uint32_t;
    using cl_size = size_t;
    struct _cl_platform_id;
    struct _cl_device_id;
    using cl_platform_id = _cl_platform_id*;
    using cl_device_id = _cl_device_id*;
    using cl_device_type = uint64_t;

    using clGetPlatformIDsFn = cl_int (*)(cl_uint, cl_platform_id*, cl_uint*);
    using clGetDeviceIDsFn = cl_int (*)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);
    using clGetPlatformInfoFn = cl_int (*)(cl_platform_id, cl_uint, cl_size, void*, cl_size*);
    using clGetDeviceInfoFn = cl_int (*)(cl_device_id, cl_uint, cl_size, void*, cl_size*);

    clGetPlatformIDsFn clGetPlatformIDs = nullptr;
    clGetDeviceIDsFn clGetDeviceIDs = nullptr;
    clGetPlatformInfoFn clGetPlatformInfo = nullptr;
    clGetDeviceInfoFn clGetDeviceInfo = nullptr;
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
    symbols.clGetPlatformInfo = reinterpret_cast<OpenCLSymbols::clGetPlatformInfoFn>(
        dlsym(symbols.handle, "clGetPlatformInfo"));
    symbols.clGetDeviceInfo = reinterpret_cast<OpenCLSymbols::clGetDeviceInfoFn>(
        dlsym(symbols.handle, "clGetDeviceInfo"));

    if (!symbols.clGetPlatformIDs || !symbols.clGetDeviceIDs ||
        !symbols.clGetPlatformInfo || !symbols.clGetDeviceInfo) {
        dlclose(symbols.handle);
        symbols.handle = nullptr;
    }

    return symbols;
}

std::string GetOpenCLInfoString(OpenCLSymbols::clGetPlatformInfoFn info_fn,
                                OpenCLSymbols::cl_platform_id platform,
                                OpenCLSymbols::cl_uint param)
{
    OpenCLSymbols::cl_size size = 0;
    if (info_fn(platform, param, 0, nullptr, &size) != 0 || size == 0) {
        return {};
    }
    std::string value(size, '\0');
    if (info_fn(platform, param, size, &value[0], nullptr) != 0) {
        return {};
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

std::string GetOpenCLInfoString(OpenCLSymbols::clGetDeviceInfoFn info_fn,
                                OpenCLSymbols::cl_device_id device,
                                OpenCLSymbols::cl_uint param)
{
    OpenCLSymbols::cl_size size = 0;
    if (info_fn(device, param, 0, nullptr, &size) != 0 || size == 0) {
        return {};
    }
    std::string value(size, '\0');
    if (info_fn(device, param, size, &value[0], nullptr) != 0) {
        return {};
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
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

std::vector<std::string> ListOpenCLDevices(std::string* error_out)
{
    if (error_out) {
        error_out->clear();
    }

    OpenCLSymbols symbols = LoadOpenCL();
    if (!symbols.handle) {
        if (error_out) {
            *error_out = "OpenCL runtime not found.";
        }
        return {};
    }

    OpenCLSymbols::cl_uint platform_count = 0;
    if (symbols.clGetPlatformIDs(0, nullptr, &platform_count) != 0 || platform_count == 0) {
        if (error_out) {
            *error_out = "No OpenCL platforms found.";
        }
        dlclose(symbols.handle);
        return {};
    }

    std::vector<OpenCLSymbols::cl_platform_id> platforms(platform_count, nullptr);
    if (symbols.clGetPlatformIDs(platform_count, platforms.data(), nullptr) != 0) {
        if (error_out) {
            *error_out = "Failed to query OpenCL platforms.";
        }
        dlclose(symbols.handle);
        return {};
    }

    const OpenCLSymbols::cl_uint kPlatformName = 0x0902;
    const OpenCLSymbols::cl_uint kDeviceName = 0x102B;
    const OpenCLSymbols::cl_device_type kAllDevices = 0xFFFFFFFFull;

    std::vector<std::string> devices;
    for (OpenCLSymbols::cl_uint platform_index = 0; platform_index < platform_count; ++platform_index) {
        OpenCLSymbols::cl_platform_id platform = platforms[platform_index];
        if (!platform) {
            continue;
        }
        std::string platform_name = GetOpenCLInfoString(symbols.clGetPlatformInfo, platform, kPlatformName);
        if (platform_name.empty()) {
            platform_name = "Unknown platform";
        }

        OpenCLSymbols::cl_uint device_count = 0;
        if (symbols.clGetDeviceIDs(platform, kAllDevices, 0, nullptr, &device_count) != 0 ||
            device_count == 0) {
            continue;
        }

        std::vector<OpenCLSymbols::cl_device_id> platform_devices(device_count, nullptr);
        if (symbols.clGetDeviceIDs(platform, kAllDevices, device_count, platform_devices.data(), nullptr) != 0) {
            continue;
        }

        for (OpenCLSymbols::cl_uint device_index = 0; device_index < device_count; ++device_index) {
            OpenCLSymbols::cl_device_id device = platform_devices[device_index];
            if (!device) {
                continue;
            }
            std::string device_name = GetOpenCLInfoString(symbols.clGetDeviceInfo, device, kDeviceName);
            if (device_name.empty()) {
                device_name = "Unknown device";
            }
            devices.push_back(
                "Platform " + std::to_string(platform_index) + ": " + platform_name +
                " - Device " + std::to_string(device_index) + ": " + device_name);
        }
    }

    if (devices.empty() && error_out) {
        *error_out = "No OpenCL devices found.";
    }

    dlclose(symbols.handle);
    return devices;
}

#endif
