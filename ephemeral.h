#ifndef EPHEMERAL_H
#define EPHEMERAL_H
#include <stdint.h>
#include <stddef.h>

typedef enum { TASK_SPAWNED=0, TASK_RUNNING=1, TASK_COMPLETED=2, TASK_CANCELLED=3, TASK_EXPIRED=4, TASK_FAILED=5 } EphState;
#define EPH_MAX 32

typedef struct {
    uint32_t task_id; char name[32]; EphState state; uint16_t owner_id;
    uint64_t created, deadline, ttl, cpu_budget, cpu_used;
    uint32_t memory_budget, memory_used; uint8_t sandboxed;
    int32_t result_code; uint8_t output[256]; uint8_t output_len;
} EphTask;

typedef struct { EphTask tasks[EPH_MAX]; uint16_t count; uint32_t next_id; } EphManager;

void eph_init(EphManager *m);
int eph_spawn(EphManager *m, const char *name, uint16_t owner, uint64_t ttl, uint64_t cpu_budget, uint32_t mem_budget, uint8_t sandboxed);
EphTask* eph_find(EphManager *m, uint32_t id);
int eph_start(EphManager *m, uint32_t id);
int eph_complete(EphManager *m, uint32_t id, int32_t result, const uint8_t *output, uint8_t out_len);
int eph_cancel(EphManager *m, uint32_t id);
int eph_tick(EphManager *m, uint64_t now);
int eph_report_cpu(EphManager *m, uint32_t id, uint64_t cycles);
int eph_report_mem(EphManager *m, uint32_t id, uint32_t bytes);
int eph_by_owner(EphManager *m, uint16_t owner, EphTask *results, int max);
int eph_by_state(EphManager *m, EphState state, EphTask *results, int max);
#endif
