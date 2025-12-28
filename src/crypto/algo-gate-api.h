#ifndef RAVEN_ALGO_GATE_API_H
#define RAVEN_ALGO_GATE_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct algo_gate_t {
    void* scanhash;
    void* hash;
    void* hash_alt;
} algo_gate_t;

static inline void algo_not_tested(void)
{
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // RAVEN_ALGO_GATE_API_H
