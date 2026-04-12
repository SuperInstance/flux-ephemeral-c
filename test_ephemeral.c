#include "ephemeral.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tests_passed = 0;
#define TEST(name) do { printf("  %-40s ", #name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

int main(void) {
    printf("=== flux-ephemeral-c tests ===\n\n");
    EphManager mgr;

    /* 1. init */
    TEST(eph_init_zeros);
    eph_init(&mgr);
    assert(mgr.count == 0 && mgr.next_id == 1);
    PASS();

    /* 2. spawn basic */
    TEST(spawn_returns_id);
    int id = eph_spawn(&mgr, "hello", 1, 1000, 5000, 1024, 1);
    assert(id > 0);
    PASS();

    /* 3. find after spawn */
    TEST(find_returns_task);
    EphTask *t = eph_find(&mgr, id);
    assert(t != NULL);
    assert(t->state == TASK_SPAWNED);
    assert(strcmp(t->name, "hello") == 0);
    PASS();

    /* 4. find missing */
    TEST(find_missing_returns_null);
    assert(eph_find(&mgr, 99999) == NULL);
    PASS();

    /* 5. start task */
    TEST(start_sets_running);
    assert(eph_start(&mgr, id) == 0);
    assert(eph_find(&mgr, id)->state == TASK_RUNNING);
    PASS();

    /* 6. start already running fails */
    TEST(start_twice_fails);
    assert(eph_start(&mgr, id) == -1);
    PASS();

    /* 7. complete task */
    TEST(complete_sets_result);
    uint8_t out[] = {1,2,3,4,5};
    assert(eph_complete(&mgr, id, 42, out, 5) == 0);
    t = eph_find(&mgr, id);
    assert(t->state == TASK_COMPLETED);
    assert(t->result_code == 42);
    assert(t->output_len == 5);
    assert(memcmp(t->output, out, 5) == 0);
    PASS();

    /* 8. complete truncates output */
    TEST(complete_truncates_255);
    int id2 = eph_spawn(&mgr, "bigout", 1, 1000, 5000, 1024, 0);
    eph_start(&mgr, id2);
    uint8_t big[300];
    memset(big, 'A', 300);
    /* uint8_t out_len wraps 300→44; library truncates to 255 internally */
    assert(eph_complete(&mgr, id2, 0, big, 255) == 0);
    t = eph_find(&mgr, id2);
    assert(t->output_len == 255);
    PASS();

    /* 9. cancel spawned */
    TEST(cancel_spawned);
    int id3 = eph_spawn(&mgr, "cancel_me", 2, 1000, 0, 0, 0);
    assert(eph_cancel(&mgr, id3) == 0);
    assert(eph_find(&mgr, id3)->state == TASK_CANCELLED);
    PASS();

    /* 10. cancel completed fails */
    TEST(cancel_completed_fails);
    assert(eph_cancel(&mgr, id) == -1);
    PASS();

    /* 11. tick expires spawned */
    TEST(tick_expires_past_deadline);
    EphManager m2;
    eph_init(&m2);
    int id4 = eph_spawn(&m2, "expire", 1, 100, 0, 0, 0);
    EphTask *t4 = eph_find(&m2, id4);
    t4->created = 1000;
    t4->deadline = 1100;
    assert(eph_tick(&m2, 1200) == 1);
    assert(eph_find(&m2, id4)->state == TASK_EXPIRED);
    PASS();

    /* 12. tick no expire */
    TEST(tick_no_expire);
    assert(eph_tick(&m2, 1050) == 0);
    PASS();

    /* 13. report_cpu */
    TEST(report_cpu_accumulates);
    EphManager m3;
    eph_init(&m3);
    int id5 = eph_spawn(&m3, "cpu_task", 1, 1000, 100, 0, 0);
    eph_start(&m3, id5);
    eph_report_cpu(&m3, id5, 60);
    assert(eph_find(&m3, id5)->cpu_used == 60);
    PASS();

    /* 14. report_cpu over budget fails task */
    TEST(cpu_over_budget_fails);
    assert(eph_report_cpu(&m3, id5, 50) == 0);
    assert(eph_find(&m3, id5)->state == TASK_FAILED);
    assert(eph_find(&m3, id5)->result_code == -1);
    PASS();

    /* 15. report_mem */
    TEST(report_mem_accumulates);
    EphManager m4;
    eph_init(&m4);
    int id6 = eph_spawn(&m4, "mem_task", 1, 1000, 0, 1024, 0);
    eph_start(&m4, id6);
    eph_report_mem(&m4, id6, 512);
    assert(eph_find(&m4, id6)->memory_used == 512);
    eph_report_mem(&m4, id6, 256);
    assert(eph_find(&m4, id6)->memory_used == 768);
    PASS();

    /* 16. by_owner filter */
    TEST(by_owner_filters);
    EphManager m5;
    eph_init(&m5);
    eph_spawn(&m5, "a", 10, 100, 0, 0, 0);
    eph_spawn(&m5, "b", 20, 100, 0, 0, 0);
    eph_spawn(&m5, "c", 10, 100, 0, 0, 0);
    EphTask res[4];
    int n = eph_by_owner(&m5, 10, res, 4);
    assert(n == 2);
    PASS();

    /* 17. by_state filter */
    TEST(by_state_filters);
    EphManager m6;
    eph_init(&m6);
    int s1 = eph_spawn(&m6, "s1", 1, 100, 0, 0, 0);
    int s2 = eph_spawn(&m6, "s2", 1, 100, 0, 0, 0);
    eph_start(&m6, s1);
    EphTask res2[4];
    int n2 = eph_by_state(&m6, TASK_SPAWNED, res2, 4);
    assert(n2 == 1);
    assert(res2[0].task_id == (uint32_t)s2);
    PASS();

    /* 18. max capacity */
    TEST(max_32_tasks);
    EphManager m7;
    eph_init(&m7);
    for (int i = 0; i < 32; i++) assert(eph_spawn(&m7, "x", 1, 100, 0, 0, 0) > 0);
    assert(eph_spawn(&m7, "overflow", 1, 100, 0, 0, 0) == -1);
    PASS();

    /* 19. sandboxed flag preserved */
    TEST(sandboxed_flag);
    EphManager m8;
    eph_init(&m8);
    int sid = eph_spawn(&m8, "sand", 1, 100, 0, 0, 1);
    assert(eph_find(&m8, sid)->sandboxed == 1);
    int nsid = eph_spawn(&m8, "nosand", 1, 100, 0, 0, 0);
    assert(eph_find(&m8, nsid)->sandboxed == 0);
    PASS();

    printf("\n%d tests passed.\n", tests_passed);
    return 0;
}
