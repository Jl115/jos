/* ============================================================================
 * JOS Host-Side Unit Test Framework
 * Simple, portable test runner that compiles with zig cc on your host machine.
 * ============================================================================ */

#include "test.h"

#define MAX_PHASES 64
#define COLOR_GREEN "\033[32m"
#define COLOR_RED   "\033[31m"
#define COLOR_RESET "\033[0m"

test_ctx_t g_test = {0};
int g_verbose = 0;
int g_total_tests = 0;
int g_failed_tests = 0;
int g_skipped_tests = 0;

static struct {
    const char *name;
    phase_fn_t  fn;
} g_phases[MAX_PHASES];
static int g_phase_count = 0;

void register_phase(const char *name, phase_fn_t fn) {
    if (g_phase_count >= MAX_PHASES) {
        fprintf(stderr, "Too many phases registered !\n");
        exit(1);
    }
    g_phases[g_phase_count].name = name;
    g_phases[g_phase_count].fn   = fn;
    g_phase_count++;
}

int test_main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            g_verbose = 1;
        }
    }

    printf("\n= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
    printf("  JOS Kernel Unit Tests\n");
    printf("  (feat/adding-testing-framework branch)\n");
    printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");

    for (int i = 0; i < g_phase_count; i++) {
        g_test.phase_name = g_phases[i].name;
        printf("\n --- Phase: %s --- \n", g_phases[i].name);
        g_phases[i].fn();
    }

    printf("\n= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
    printf("  Results:%s  %d total,  %d passed,  %d failed,  %d skipped%s\n",
           (g_failed_tests == 0) ? COLOR_GREEN : COLOR_RED,
           g_total_tests, g_total_tests - g_failed_tests, g_failed_tests, g_skipped_tests,
           COLOR_RESET);
    printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n");

    return g_failed_tests > 0 ? 1 : 0;
}

int main(int argc, char **argv) {
    extern void register_all_phases(void);
    register_all_phases();
    return test_main(argc, argv);
}
