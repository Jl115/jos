/* ============================================================================
 * JOS Host-Side Unit Test Framework
 * Minimal, standalone test runner that runs natively on the development machine.
 *
 * Usage in test files:
 *   #include "test.h"
 *
 *   PHASE("Phase name") {
 *       TEST("test description") {
 *           ASSERT_EQ_INT(2 + 2, 4);
 *       }
 *   }
 *
 * Main runner calls register_phase() then test_main().
 * ============================================================================ */
#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * KEEP these globals in test.c / main.c —  define in exactly ONE translation
 * unit  so the linker can resolve them.
 * DO NOT put definitions in this header (ODR violation).
 * ============================================================================ */

typedef struct {
    const char *phase_name;
    const char *test_name;
    int         line;
    const char *file;
    int         passed;
    int         skipped;
    int         is_running;
} test_ctx_t;

extern test_ctx_t g_test;
extern int g_verbose;
extern int g_total_tests;
extern int g_failed_tests;
extern int g_skipped_tests;

/* ============================================================================
 * Phase registration
 * ============================================================================ */
typedef void (*phase_fn_t)(void);

void register_phase(const char *name, phase_fn_t fn);

/* --- Phase macro — no constructor so it never segfaults on ELF init --- */
#define PHASE(name)                                               \
    void phase_fn_##name(void);                                   \
    void phase_fn_##name(void)

#define RUN_PHASE(name) register_phase(#name, phase_fn_##name)

/* ============================================================================
 * TEST assertion holder macro.
 * Runs body_in_test once, cleaning state each time.
 * ============================================================================ */
#define TEST(desc)                                                \
    for (int __test_once = (                                      \
        g_test.test_name = desc,                                  \
        g_test.file      = __FILE__,                              \
        g_test.line      = __LINE__,                              \
        g_test.passed    = 1,                                     \
        g_test.skipped   = 0,                                     \
        g_test.is_running= 1,                                   \
        g_total_tests++,                                          \
        (g_verbose ? (void)printf("  running: %s\n", desc)       \
                   : (void)0),                                    \
        1);                                                       \
         __test_once;                                             \
         __test_once = 0)                                       \
        if (g_test.passed && !g_test.skipped)

#define SKIP(msg) do {                                            \
    g_test.skipped = 1;                                           \
    g_skipped_tests++;                                            \
    if (g_verbose)                                                \
        printf("    [SKIPPED: %s]\n", msg);                       \
} while (0)

/* ============================================================================
 * Assertion macros
 * ============================================================================ */
#define ASSERT_TRUE(cond) do {                                    \
    if (!(cond)) {                                                \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  assertion: %s\n",             \
               __FILE__, __LINE__, #cond);                        \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ_INT(expected, actual) do {                    \
    int64_t _exp = (expected);                                    \
    int64_t _act = (actual);                                      \
    if (_exp != _act) {                                           \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  expected %ld, got %ld\n",      \
               __FILE__, __LINE__, (int64_t)_exp, (int64_t)_act); \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_EQ_UINT(expected, actual) do {                     \
    uint64_t _exp = (expected);                                   \
    uint64_t _act = (actual);                                     \
    if (_exp != _act) {                                           \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  expected 0x%016lx, got 0x%016lx\n", \
               __FILE__, __LINE__, (uint64_t)_exp, (uint64_t)_act); \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_EQ_HEX(expected, actual) ASSERT_EQ_UINT(expected, actual)

#define ASSERT_NE_INT(expected, actual) do {                      \
    int64_t _exp = (expected);                                    \
    int64_t _act = (actual);                                      \
    if (_exp == _act) {                                           \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  values equal (%ld)\n",       \
               __FILE__, __LINE__, (int64_t)_act);                \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_PTR_NULL(ptr) do {                                 \
    if ((ptr) != NULL) {                                          \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  expected NULL, got %p\n",    \
               __FILE__, __LINE__, (void*)(ptr));                 \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_PTR_NOT_NULL(ptr) do {                             \
    if ((ptr) == NULL) {                                          \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  unexpected NULL\n",          \
               __FILE__, __LINE__);                               \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_STRING_EQ(expected, actual) do {                   \
    const char *_exp = (expected);                                \
    const char *_act = (actual);                                  \
    if (strcmp(_exp, _act) != 0) {                               \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  strings differ:\n"           \
               "      expected: \"%s\"\n"                            \
               "      actual:   \"%s\"\n",                          \
               __FILE__, __LINE__, _exp, _act);                   \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#define ASSERT_MEM_EQ(ptr1, ptr2, len) do {                       \
    if (memcmp((ptr1), (ptr2), (len)) != 0) {                    \
        g_test.passed = 0;                                        \
        printf("    FAILED at %s:%d  memory blocks differ\n",      \
               __FILE__, __LINE__);                               \
        g_failed_tests++;                                         \
    }                                                             \
} while (0)

#endif /* TEST_H */
