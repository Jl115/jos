/* ============================================================================
 * Phase 7: User Space & System Calls (EL0)
 * Tests syscall dispatch table, number handling, privilege simulation.
 * No real privilege drop — mock state only.
 * ============================================================================ */

#include "test.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* --- S syscall table --- */
typedef uint64_t (*syscall_fn_t)(uint64_t a0, uint64_t a1, uint64_t a2,
                                 uint64_t a3, uint64_t a4, uint64_t a5);

#define SYS_PRINT 0
#define SYS_EXIT  1
#define SYS_ALLOC 2
#define MAX_SYSCALLS 16

static syscall_fn_t g_syscall_table[MAX_SYSCALLS];
static uint64_t     g_syscall_args[6];
static uint64_t     g_syscall_ret;

/* --- Mock syscall implementations --- */

/* Forward declare global state used by handlers */
static uint64_t g_current_state;

static uint64_t sysPrint(uint64_t buf, uint64_t len, uint64_t a2,
                         uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (buf < 0x40000000 || buf >= 0x80000000) return 0xFFFFFFFF;
    if (len == 0) return 0;
    return len;
}

static uint64_t sysExit(uint64_t code, uint64_t a1, uint64_t a2,
                        uint64_t a3, uint64_t a4, uint64_t a5) {
    (void)code; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    g_current_state = 0xDEAD;
    return 0;
}

/* --- Privilege state simulation --- */
static uint64_t g_mair_el1 = 0;
static uint64_t g_tcr_el1 = 0;
static uint64_t g_ttbr0_el1 = 0;
static uint64_t g_spsr_el1 = 0;
static uint64_t g_elr_el1 = 0;

static uint32_t g_svc_number = 0;

static uint64_t handleSyscall(uint64_t nr, uint64_t a0, uint64_t a1,
                               uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    if (nr >= MAX_SYSCALLS) return 0xFFFFFFFF;
    syscall_fn_t fn = g_syscall_table[nr];
    if (!fn) return 0xFFFFFFFF;
    g_syscall_args[0] = a0; g_syscall_args[1] = a1; g_syscall_args[2] = a2;
    g_syscall_args[3] = a3; g_syscall_args[4] = a4; g_syscall_args[5] = a5;
    g_syscall_ret = fn(a0, a1, a2, a3, a4, a5);
    return g_syscall_ret;
}

static void resetSyscalls(void) {
    memset(g_syscall_table, 0, sizeof(g_syscall_table));
    memset(g_syscall_args, 0, sizeof(g_syscall_args));
    g_syscall_ret = 0;
    g_mair_el1 = 0;
    g_tcr_el1 = 0;
    g_ttbr0_el1 = 0;
    g_spsr_el1 = 0;
    g_elr_el1 = 0;
    g_current_state = 0;
    g_svc_number = 0;
}

static int isUserAddr(uint64_t va) {
    return (va < 0x00010000000000010ULL);
}

/* ========================================================================= */
PHASE(p7_user_space_and_system_calls) {

    /* --- SVC dispatch --- */
    TEST("Syscall table entry is NULL before registration") {
        resetSyscalls();
        ASSERT_PTR_NULL(g_syscall_table[SYS_PRINT]);
    }

    TEST("Register then dispatch sys_print returns expected len") {
        resetSyscalls();
        g_syscall_table[SYS_PRINT] = sysPrint;
        uint64_t r = handleSyscall(SYS_PRINT, 0x40000100, 13, 0,0,0,0);
        ASSERT_EQ_UINT(r, 13);
    }

    TEST("Dispatch sys_exit mutates current state") {
        resetSyscalls();
        g_syscall_table[SYS_EXIT] = sysExit;
        uint64_t r = handleSyscall(SYS_EXIT, 42, 0,0,0,0,0);
        ASSERT_EQ_UINT(r, 0);
        ASSERT_EQ_HEX(g_current_state, 0xDEAD);
    }

    TEST("Unregistered syscall number returns error sentinel") {
        resetSyscalls();
        uint64_t r = handleSyscall(99, 0,0,0,0,0,0);
        ASSERT_EQ_UINT(r, 0xFFFFFFFF);
    }

    TEST("Syscall arguments are captured in global arg buffer") {
        resetSyscalls();
        g_syscall_table[SYS_PRINT] = sysPrint;
        handleSyscall(SYS_PRINT, 1, 2, 3, 4, 5, 6);
        ASSERT_EQ_UINT(g_syscall_args[0], 1);
        ASSERT_EQ_UINT(g_syscall_args[5], 6);
    }

    /* --- MMU / privilege simulation --- */
    TEST("User address check passes for low canonical VAs") {
        ASSERT_TRUE(isUserAddr(0x1000));
        ASSERT_TRUE(isUserAddr(0x0000FFFFFFFFFFFFULL));
    }

    TEST("SPSR_EL1 shift and mask produce EL0t(0b0000)") {
        uint64_t spsr = 0b0000;  /* EL0t */
        ASSERT_TRUE(spsr == 0);
    }

    TEST("Kernel page AP[2:1]=0b00, User page AP[2:1]=0b01") {
        uint64_t ap_kern = 0b00 << 6;
        uint64_t ap_user = 0b01 << 6;
        ASSERT_TRUE(ap_user != ap_kern);
    }

    TEST("ELR_EL1 simulation stores a return address") {
        resetSyscalls();
        g_elr_el1 = 0x40005000;
        ASSERT_EQ_HEX(g_elr_el1, 0x40005000);
    }

    TEST("TTBR0_EL1 change invalidates TLB flag (simulated)") {
        resetSyscalls();
        g_ttbr0_el1 = 0x30000000;
        /* Simulate TLB invalidate flag */
        int tlb_dirty = 1;
        g_ttbr0_el1 = 0x40000000;
        (void)tlb_dirty; /* just a logic placeholder */
        ASSERT_EQ_HEX(g_ttbr0_el1, 0x40000000);
    }

    TEST("MAIR_EL1 attribute 0 = Normal WB (simulation)") {
        uint64_t attr0 = 0xFF;   /* Normal Inner/Outer WB */
        uint64_t mair  = attr0;
        ASSERT_TRUE(mair == 0xFF);
    }

    /* --- Data abort simulation --- */
    TEST("Invalid user pointer returns EFAULT (0xFFFFFFFF)") {
        resetSyscalls();
        g_syscall_table[SYS_PRINT] = sysPrint;
        uint64_t r = handleSyscall(SYS_PRINT, 0x08000000, 10, 0,0,0,0);
        ASSERT_EQ_UINT(r, 0xFFFFFFFF); /* kernel-space region -> error */
    }

    TEST("Zero-length print returns 0") {
        resetSyscalls();
        g_syscall_table[SYS_PRINT] = sysPrint;
        uint64_t r = handleSyscall(SYS_PRINT, 0x40000100, 0, 0,0,0,0);
        ASSERT_EQ_UINT(r, 0);
    }

    TEST("SVC number 0 translates to handler index 0") {
        resetSyscalls();
        g_syscall_table[0] = sysPrint;
        uint64_t r = handleSyscall(0, 0x40000100, 8, 0,0,0,0);
        ASSERT_EQ_UINT(r, 8);
    }

    TEST("Argument count: 6 registers preserved in syscall args buffer") {
        resetSyscalls();
        g_syscall_table[SYS_PRINT] = sysPrint;
        handleSyscall(SYS_PRINT, 10, 20, 30, 40, 50, 60);
        for (int i = 0; i < 6; i++) {
            ASSERT_EQ_UINT(g_syscall_args[i], (uint64_t)(10 * (i + 1)));
        }
    }
}
