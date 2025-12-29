// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_CUDA_UTILS_H
#define RAVEN_CUDA_UTILS_H

#include <string>
#include <vector>

bool CudaGpuAvailable();
std::vector<std::string> ListCudaDevices(std::string* error_out);

#endif // RAVEN_CUDA_UTILS_H
