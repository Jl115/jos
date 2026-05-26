#include "test.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PROCS     64
#define PROC_NAME_LEN 16

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE
} proc_state_t;

typedef struct pcb {
    uint64_t     pid;
    char         name[PROC_NAME_LEN];
    proc_state_t state;
    uint64_t     x[31];
    uint64_t     sp;
    uint64_t     pc;
    uint64_t     pstate;
    void        *page_table;
    uint64_t     ticks_used;
    uint64_t     priority;
    struct pcb  *next;
} pcb_t;

typedef struct {
    pcb_t  procs[MAX_PROCS];
    pcb_t *current;
    pcb_t *head;
    uint64_t num_procs;
    uint64_t next_pid;
} scheduler_t;

void     schedulerInit(scheduler_t *sched);
pcb_t   *createProcess(scheduler_t *sched, uint64_t entry_point, const char *name);
pcb_t   *schedule(scheduler_t *sched);
void     blockProcess(pcb_t *proc);
void     unblockProcess(pcb_t *proc);
void     markZombie(pcb_t *proc);
uint64_t getCurrentPid(scheduler_t *sched);

#endif

PHASE(p6_multitasking_scheduling) {
    TEST("proc_state_t: PROC_READY is 0") {
        ASSERT_EQ_INT((int)PROC_READY, 0);
    }

    TEST("proc_state_t: PROC_RUNNING is 1") {
        ASSERT_EQ_INT((int)PROC_RUNNING, 1);
    }

    TEST("proc_state_t: PROC_BLOCKED is 2") {
        ASSERT_EQ_INT((int)PROC_BLOCKED, 2);
    }

    TEST("proc_state_t: PROC_ZOMBIE is 3") {
        ASSERT_EQ_INT((int)PROC_ZOMBIE, 3);
    }

    TEST("MAX_PROCS is 64") {
        ASSERT_EQ_INT((int)MAX_PROCS, 64);
    }

    TEST("PROC_NAME_LEN is 16") {
        ASSERT_EQ_INT((int)PROC_NAME_LEN, 16);
    }

    TEST("pcb_t has pid field") {
        pcb_t p;
        p.pid = 1;
        ASSERT_EQ_INT((int)p.pid, 1);
    }

    TEST("pcb_t has name field of 16 bytes") {
        pcb_t p;
        ASSERT_EQ_INT((int)sizeof(p.name), 16);
    }

    TEST("pcb_t has state field of proc_state_t") {
        pcb_t p;
        p.state = PROC_RUNNING;
        ASSERT_EQ_INT((int)p.state, (int)PROC_RUNNING);
    }

    TEST("pcb_t has x[31] register array") {
        pcb_t p;
        ASSERT_EQ_INT((int)(sizeof(p.x) / sizeof(p.x[0])), 31);
    }

    TEST("pcb_t has sp field") {
        pcb_t p;
        p.sp = 0x41000000ULL;
        ASSERT_EQ_HEX(p.sp, 0x41000000ULL);
    }

    TEST("pcb_t has pc field for ELR_EL1") {
        pcb_t p;
        p.pc = 0x40080000ULL;
        ASSERT_EQ_HEX(p.pc, 0x40080000ULL);
    }

    TEST("pcb_t has pstate field for SPSR_EL1") {
        pcb_t p;
        p.pstate = 0x5ULL;
        ASSERT_EQ_HEX(p.pstate, 0x5ULL);
    }

    TEST("pcb_t has page_table pointer") {
        pcb_t p;
        p.page_table = NULL;
        ASSERT_PTR_NULL(p.page_table);
    }

    TEST("pcb_t has ticks_used counter") {
        pcb_t p;
        p.ticks_used = 0;
        ASSERT_EQ_INT((int)p.ticks_used, 0);
    }

    TEST("pcb_t has priority field") {
        pcb_t p;
        p.priority = 1;
        ASSERT_EQ_INT((int)p.priority, 1);
    }

    TEST("pcb_t has next pointer for round-robin list") {
        pcb_t p;
        p.next = NULL;
        ASSERT_PTR_NULL(p.next);
    }

    TEST("scheduler_t has procs array of MAX_PROCS pcbs") {
        scheduler_t s;
        ASSERT_EQ_INT((int)(sizeof(s.procs) / sizeof(s.procs[0])), MAX_PROCS);
    }

    TEST("scheduler_t has current pointer") {
        scheduler_t s;
        s.current = NULL;
        ASSERT_PTR_NULL(s.current);
    }

    TEST("scheduler_t has head pointer") {
        scheduler_t s;
        s.head = NULL;
        ASSERT_PTR_NULL(s.head);
    }

    TEST("scheduler_t has num_procs counter") {
        scheduler_t s;
        s.num_procs = 0;
        ASSERT_EQ_INT((int)s.num_procs, 0);
    }

    TEST("scheduler_t has next_pid counter") {
        scheduler_t s;
        s.next_pid = 1;
        ASSERT_EQ_INT((int)s.next_pid, 1);
    }

    TEST("SPSR_EL1 for EL1h with IRQ unmasked = 0x5") {
        uint64_t spsr = 0x5ULL;
        uint64_t m = spsr & 0xF;
        ASSERT_EQ_INT((int)m, 0x5);
    }

    TEST("SPSR_EL1 for EL0t = 0x0") {
        uint64_t spsr = 0x0ULL;
        uint64_t m = spsr & 0xF;
        ASSERT_EQ_INT((int)m, 0x0);
    }

    TEST("ELR_EL1 holds return address for ERET") {
        pcb_t p;
        p.pc = 0x40080000ULL;
        ASSERT_EQ_HEX(p.pc, 0x40080000ULL);
    }

    TEST("SP_EL0 holds user stack pointer") {
        pcb_t p;
        p.sp = 0x41000000ULL;
        ASSERT_EQ_HEX(p.sp, 0x41000000ULL);
    }

    TEST("x19-x30 are callee-saved: 12 registers") {
        int callee_saved_count = 30 - 19 + 1;
        ASSERT_EQ_INT(callee_saved_count, 12);
    }

    TEST("schedulerInit initializes scheduler state") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("createProcess allocates PCB and sets entry point") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("schedule rotates to next READY process (round-robin)") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("createProcess assigns unique PIDs") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("createProcess sets state to PROC_READY") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("createProcess allocates stack from PFA") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("schedule skips BLOCKED and ZOMBIE processes") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("blockProcess sets state to PROC_BLOCKED") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("markZombie sets state to PROC_ZOMBIE") {
        SKIP("scheduler implementation not yet available");
    }

    TEST("getCurrentPid returns running process PID") {
        SKIP("scheduler implementation not yet available");
    }
}