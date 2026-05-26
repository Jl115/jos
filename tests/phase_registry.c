/* ============================================================================
 * Phase Registry —  declares & registers every test phase.
 * This file knows all phase names so main.c stays generic.
 * ============================================================================ */

#include "./include/test.h"

extern void phase_fn_p1_exception_vector_table(void);
extern void phase_fn_p1_trap_dispatch(void);
extern void phase_fn_p3_fdt_and_pfa(void);
extern void phase_fn_p3_pfa_integration(void);
extern void phase_fn_p2_gic_and_timers(void);
extern void phase_fn_p4_mmu_and_paging(void);
extern void phase_fn_p5_kernel_heap(void);
extern void phase_fn_p6_multitasking_scheduling(void);
extern void phase_fn_p7_user_space_and_system_calls(void);

void register_all_phases(void) {
    RUN_PHASE(p1_exception_vector_table);
    RUN_PHASE(p1_trap_dispatch);
    RUN_PHASE(p3_fdt_and_pfa);
    RUN_PHASE(p3_pfa_integration);
    RUN_PHASE(p2_gic_and_timers);
    RUN_PHASE(p4_mmu_and_paging);
    RUN_PHASE(p5_kernel_heap);
    RUN_PHASE(p6_multitasking_scheduling);
    RUN_PHASE(p7_user_space_and_system_calls);
}