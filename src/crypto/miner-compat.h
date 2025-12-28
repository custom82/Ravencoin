#ifndef RAVEN_CRYPTO_MINER_COMPAT_H
#define RAVEN_CRYPTO_MINER_COMPAT_H

#include "compat/byteswap.h"
#include "compat/endian.h"

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ALIGN
#if defined(_MSC_VER)
#define _ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
#define _ALIGN(x) __attribute__((aligned(x)))
#else
#define _ALIGN(x)
#endif
#endif

#ifndef likely
#if defined(__GNUC__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
#endif

static inline uint32_t swab32(uint32_t value)
{
    return bswap_32(value);
}

static inline uint32_t be32dec(const void *pp)
{
    uint32_t value;
    memcpy(&value, pp, sizeof(value));
    return be32toh(value);
}

static inline void be32enc(void *pp, uint32_t value)
{
    uint32_t be_value = htobe32(value);
    memcpy(pp, &be_value, sizeof(be_value));
}

static inline int fulltest(const uint32_t *hash, const uint32_t *target)
{
    for (int i = 7; i >= 0; --i) {
        if (hash[i] < target[i]) {
            return 1;
        }
        if (hash[i] > target[i]) {
            return 0;
        }
    }
    return 1;
}

struct work {
    uint32_t data[32];
    uint32_t target[8];
};

struct work_restart {
    volatile int restart;
};

#ifndef RAVEN_WORK_RESTART_SLOTS
#define RAVEN_WORK_RESTART_SLOTS 64
#endif

static struct work_restart work_restart[RAVEN_WORK_RESTART_SLOTS];

#ifdef __cplusplus
} // extern "C"
#endif

#endif // RAVEN_CRYPTO_MINER_COMPAT_H
