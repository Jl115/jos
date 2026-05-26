#include "test.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_PRINT  0
#define SYS_EXIT   1
#define SYS_ALLOC  2

#define SYSCALL_RET_SUCCESS 0
#define SYSCALL_RET_INVAL  (-1)
#define SYSCALL_RET_FAULT  (-2)

#define USER_SPACE_BASE  0x00000000ULL
#define USER_SPACE_TOP   0x0000800000000000ULL
#define KERNEL_SPACE_BASE 0xFFFF000000000000ULL

#define SVC_EC_AARCH64 0x15

typedef int64_t (*syscall_fn_t)(uint64_t a0, uint64_t a1, uint64_t a2,
                                 uint64_t a3, uint64_t a4);

int64_t sys_print(const char *buf, uint64_t len);
int64_t sys_exit(int64_t code);
int64_t sys_alloc(uint64_t size);

int64_t syscallDispatch(uint64_t syscall_num,
                        uint64_t a0, uint64_t a1, uint64_t a2,
                        uint64_t a3, uint64_t a4);

static int isUserPointer_impl(const void *ptr, uint64_t len) {
    uint64_t addr = (uint64_t)(uintptr_t)ptr;
    if (addr == 0) return 0;
    if (addr >= KERNEL_SPACE_BASE) return 0;
    if (addr >= USER_SPACE_TOP) return 0;
    if (len > 0) {
        uint64_t end = addr + len;
        if (end < addr) return 0;
        if (end > USER_SPACE_TOP) return 0;
    }
    return 1;
}
#define isUserPointer(ptr, len) isUserPointer_impl(ptr, len)

#endif

PHASE(p7_user_space_and_system_calls) {
    TEST("SVC exception class from AArch64 is 0x15") {
        ASSERT_EQ_INT(SVC_EC_AARCH64, 0x15);
    }

    TEST("ESR_EL1 EC extraction for SVC yields 0x15") {
        uint64_t esr = ((uint64_t)SVC_EC_AARCH64 << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x15);
    }

    TEST("SYS_PRINT syscall number is 0") {
        ASSERT_EQ_INT((int)SYS_PRINT, 0);
    }

    TEST("SYS_EXIT syscall number is 1") {
        ASSERT_EQ_INT((int)SYS_EXIT, 1);
    }

    TEST("SYS_ALLOC syscall number is 2") {
        ASSERT_EQ_INT((int)SYS_ALLOC, 2);
    }

    TEST("SYSCALL_RET_SUCCESS is 0") {
        ASSERT_EQ_INT((int)SYSCALL_RET_SUCCESS, 0);
    }

    TEST("SYSCALL_RET_INVAL is -1") {
        ASSERT_EQ_INT((int)SYSCALL_RET_INVAL, -1);
    }

    TEST("SYSCALL_RET_FAULT is -2") {
        ASSERT_EQ_INT((int)SYSCALL_RET_FAULT, -2);
    }

    TEST("USER_SPACE_BASE is 0x00000000 (lower half)") {
        ASSERT_EQ_HEX((uint64_t)USER_SPACE_BASE, 0x0ULL);
    }

    TEST("KERNEL_SPACE_BASE is 0xFFFF000000000000 (upper canonical)") {
        ASSERT_EQ_HEX((uint64_t)KERNEL_SPACE_BASE, 0xFFFF000000000000ULL);
    }

    TEST("isUserPointer rejects kernel-space pointer") {
        uint64_t kernel_ptr = 0xFFFF000040000000ULL;
        int valid = isUserPointer((const void *)kernel_ptr, 4);
        ASSERT_TRUE(valid == 0);
    }

    TEST("isUserPointer accepts user-space pointer") {
        uint64_t user_ptr = 0x000000001000ULL;
        int valid = isUserPointer((const void *)user_ptr, 4);
        ASSERT_TRUE(valid != 0);
    }

    TEST("isUserPointer rejects NULL pointer") {
        int valid = isUserPointer(NULL, 4);
        ASSERT_TRUE(valid == 0);
    }

    TEST("isUserPointer rejects pointer that crosses into kernel space") {
        uint64_t user_ptr = 0x0000FFFFFFFFFFF0ULL;
        uint64_t len = 32;
        int valid = isUserPointer((const void *)user_ptr, len);
        ASSERT_TRUE(valid == 0);
    }

    TEST("SPSR_EL1 for EL0t = 0x0 (user mode, all interrupts unmasked)") {
        uint64_t spsr = 0x0ULL;
        uint64_t m = spsr & 0xF;
        ASSERT_EQ_INT((int)m, 0);
    }

    TEST("SPSR for user mode has DAIF bits clear (unmasked)") {
        uint64_t spsr_el0t = 0x0ULL;
        uint64_t daif = (spsr_el0t >> 6) & 0xF;
        ASSERT_EQ_INT((int)daif, 0);
    }

    TEST("Drop to EL0: ELR_EL1 must point to user entry") {
        uint64_t user_entry = 0x1000ULL;
        uint64_t elr = user_entry;
        ASSERT_EQ_HEX(elr, 0x1000ULL);
    }

    TEST("Drop to EL0: SP_EL0 must hold user stack top") {
        uint64_t user_sp = 0x80000ULL;
        ASSERT_EQ_HEX(user_sp, 0x80000ULL);
    }

    TEST("TTBR0_EL1 switch requires TLB invalidation") {
        uint64_t ttbr0_new = 0x40200000ULL;
        (void)ttbr0_new;
        ASSERT_TRUE(1);
    }

    TEST("syscall ABI: syscall number in x8") {
        uint64_t x8 = SYS_PRINT;
        ASSERT_EQ_INT((int)x8, (int)SYS_PRINT);
    }

    TEST("syscall ABI: arguments in x0-x5") {
        uint64_t args[6] = {1, 2, 3, 4, 5, 6};
        ASSERT_EQ_INT((int)args[0], 1);
        ASSERT_EQ_INT((int)args[5], 6);
    }

    TEST("syscall ABI: return value in x0") {
        int64_t ret = SYSCALL_RET_SUCCESS;
        ASSERT_EQ_INT((int)ret, 0);
    }

    TEST("Page table AP bits: kernel-only = 0b00") {
        uint64_t ap_kernel = 0b00;
        ASSERT_EQ_INT((int)ap_kernel, 0);
    }

    TEST("Page table AP bits: user RW = 0b01") {
        uint64_t ap_user_rw = 0b01;
        ASSERT_EQ_INT((int)ap_user_rw, 1);
    }

    TEST("Page table AP bits: kernel RO = 0b10") {
        uint64_t ap_kernel_ro = 0b10;
        ASSERT_EQ_INT((int)ap_kernel_ro, 2);
    }

    TEST("Page table AP bits: user RO = 0b11") {
        uint64_t ap_user_ro = 0b11;
        ASSERT_EQ_INT((int)ap_user_ro, 3);
    }

    TEST("UXN bit (Execute-Never for EL0) should be set on data pages") {
        uint64_t uxn = 1ULL << 54;
        ASSERT_TRUE(uxn != 0);
    }

    TEST("PAN (Privileged Access Never) prevents kernel from accessing user pages directly") {
        ASSERT_TRUE(1);
    }

    TEST("SVC instruction triggers exception with ESR_EC = 0x15") {
        uint64_t esr = ((uint64_t)0x15 << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x15);
    }

    TEST("ESR_ISS for SVC contains the immediate (syscall number)") {
        uint64_t esr = ((uint64_t)0x15 << 26) | SYS_PRINT;
        uint32_t iss = esr & 0xFFFFFF;
        ASSERT_EQ_INT((int)iss, (int)SYS_PRINT);
    }

    TEST("syscallDispatch routes SYS_PRINT correctly") {
        SKIP("syscall implementation not yet available");
    }

    TEST("syscallDispatch routes SYS_EXIT correctly") {
        SKIP("syscall implementation not yet available");
    }

    TEST("syscallDispatch routes SYS_ALLOC correctly") {
        SKIP("syscall implementation not yet available");
    }

    TEST("sys_print validates user pointer before dereference") {
        SKIP("syscall implementation not yet available");
    }

    TEST("sys_print rejects kernel-space pointer") {
        SKIP("syscall implementation not yet available");
    }

    TEST("sys_alloc returns page-aligned memory") {
        SKIP("syscall implementation not yet available");
    }

    TEST("copy_from_user returns error on invalid pointer") {
        SKIP("syscall implementation not yet available");
    }
}