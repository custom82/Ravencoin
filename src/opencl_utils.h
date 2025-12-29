// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_OPENCL_UTILS_H
#define RAVEN_OPENCL_UTILS_H

#include <string>
#include <vector>

bool OpenCLGpuAvailable();
std::vector<std::string> ListOpenCLDevices(std::string* error_out);

#endif // RAVEN_OPENCL_UTILS_H
