#include "ephemeral.h"
#include <string.h>

static int copy_str(char *dst, const char *src, int max) {
    int i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
    return i;
}

void eph_init(EphManager *m) {
    memset(m, 0, sizeof(*m));
    m->next_id = 1;
}

int eph_spawn(EphManager *m, const char *name, uint16_t owner, uint64_t ttl, uint64_t cpu_budget, uint32_t mem_budget, uint8_t sandboxed) {
    if (m->count >= EPH_MAX) return -1;
    for (int i = 0; i < EPH_MAX; i++) {
        if (m->tasks[i].state == 0 && m->tasks[i].task_id == 0) {
            EphTask *t = &m->tasks[i];
            memset(t, 0, sizeof(*t));
            t->task_id = m->next_id++;
            copy_str(t->name, name, 32);
            t->state = TASK_SPAWNED;
            t->owner_id = owner;
            t->ttl = ttl;
            t->cpu_budget = cpu_budget;
            t->memory_budget = mem_budget;
            t->sandboxed = sandboxed;
            m->count++;
            return (int)t->task_id;
        }
    }
    return -1;
}

EphTask* eph_find(EphManager *m, uint32_t id) {
    for (int i = 0; i < EPH_MAX; i++)
        if (m->tasks[i].task_id == id) return &m->tasks[i];
    return NULL;
}

int eph_start(EphManager *m, uint32_t id) {
    EphTask *t = eph_find(m, id);
    if (!t || t->state != TASK_SPAWNED) return -1;
    t->state = TASK_RUNNING;
    return 0;
}

int eph_complete(EphManager *m, uint32_t id, int32_t result, const uint8_t *output, uint8_t out_len) {
    EphTask *t = eph_find(m, id);
    if (!t || t->state != TASK_RUNNING) return -1;
    t->state = TASK_COMPLETED;
    t->result_code = result;
    /* out_len is uint8_t, already clamped to 0-255 */
    t->output_len = out_len;
    if (output && out_len) memcpy(t->output, output, out_len);
    return 0;
}

int eph_cancel(EphManager *m, uint32_t id) {
    EphTask *t = eph_find(m, id);
    if (!t || (t->state != TASK_SPAWNED && t->state != TASK_RUNNING)) return -1;
    t->state = TASK_CANCELLED;
    return 0;
}

int eph_tick(EphManager *m, uint64_t now) {
    int expired = 0;
    for (int i = 0; i < EPH_MAX; i++) {
        EphTask *t = &m->tasks[i];
        if (t->task_id && t->state == TASK_SPAWNED && t->deadline && now > t->deadline) {
            t->state = TASK_EXPIRED;
            expired++;
        }
    }
    return expired;
}

int eph_report_cpu(EphManager *m, uint32_t id, uint64_t cycles) {
    EphTask *t = eph_find(m, id);
    if (!t || t->state != TASK_RUNNING) return -1;
    t->cpu_used += cycles;
    if (t->cpu_budget && t->cpu_used > t->cpu_budget) {
        t->state = TASK_FAILED;
        t->result_code = -1;
    }
    return 0;
}

int eph_report_mem(EphManager *m, uint32_t id, uint32_t bytes) {
    EphTask *t = eph_find(m, id);
    if (!t || t->state != TASK_RUNNING) return -1;
    t->memory_used += bytes;
    return 0;
}

int eph_by_owner(EphManager *m, uint16_t owner, EphTask *results, int max) {
    int n = 0;
    for (int i = 0; i < EPH_MAX && n < max; i++)
        if (m->tasks[i].task_id && m->tasks[i].owner_id == owner)
            results[n++] = m->tasks[i];
    return n;
}

int eph_by_state(EphManager *m, EphState state, EphTask *results, int max) {
    int n = 0;
    for (int i = 0; i < EPH_MAX && n < max; i++)
        if (m->tasks[i].task_id && m->tasks[i].state == state)
            results[n++] = m->tasks[i];
    return n;
}
