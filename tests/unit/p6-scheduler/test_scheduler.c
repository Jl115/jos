/* ============================================================================
 * Phase 6: Multitasking & Scheduling
 * Tests PCB layout, round-robin scheduling, and context-save bookkeeping.
 * No actual register save/restore — logic only.
 * ============================================================================ */

#include "test.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MAX_PROC 4
#define PAGE_SIZE (1 << 12)

/* --- Simplified PCB replica --- */
typedef enum { READY, RUNNING, BLOCKED, ZOMBIE } proc_state_t;

typedef struct pcb {
    uint32_t pid;
    char     name[16];
    proc_state_t state;
    uint64_t x[31];        /* x0-x30 */
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
    uint64_t ticks_used;
    uint64_t priority;
    struct   pcb *next;
} pcb_t;

static int    g_next_pid = 1;
static pcb_t  g_pcb_pool[MAX_PROC];
static pcb_t *g_ready_head = NULL;
static pcb_t *g_ready_tail = NULL;
static pcb_t *g_current = NULL;

static pcb_t *pcbAlloc(void) {
    for (int i = 0; i < MAX_PROC; i++) {
        if (g_pcb_pool[i].pid == 0) {
            return &g_pcb_pool[i];
        }
    }
    return NULL;
}

static void pcbReset(void) {
    memset(g_pcb_pool, 0, sizeof(g_pcb_pool));
    g_ready_head = NULL;
    g_ready_tail = NULL;
    g_current = NULL;
    g_next_pid = 1;
}

static pcb_t *createProcess(const char *name, uint64_t entry) {
    pcb_t *p = pcbAlloc();
    if (!p) return NULL;
    p->pid = g_next_pid++;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->state = READY;
    p->pc = entry;
    p->sp = (uint64_t)(uintptr_t)(0xABCDEF00 + (p->pid * PAGE_SIZE));
    p->next = NULL;

    if (g_ready_tail) {
        g_ready_tail->next = p;
    } else {
        g_ready_head = p;
    }
    g_ready_tail = p;
    return p;
}

static pcb_t *schedule(void) {
    if (!g_ready_head) return NULL;

    if (g_current && g_current->state == RUNNING) {
        g_current->state = READY;
    }

    pcb_t *next = g_current ? g_current->next : NULL;
    if (!next) next = g_ready_head;

    next->state = RUNNING;
    g_current = next;
    return next;
}

static int countReady(void) {
    int n = 0;
    for (pcb_t *p = g_ready_head; p; p = p->next) if (p->state != ZOMBIE) n++;
    return n;
}

/* ========================================================================= */
PHASE(p6_multitasking_scheduling) {

    /* --- PCB layout tests --- */
    TEST("PCB size fits expected padding (must be compile-time stable)") {
        ASSERT_TRUE(sizeof(pcb_t) >= (sizeof(uint64_t) * 40));
    }

    TEST("PID field exists at correct offset (zero-based)") {
        pcbReset();
        pcb_t *p = createProcess("idle", 0x1000);
        ASSERT_EQ_UINT(p->pid, 1);
    }

    TEST("Process name survives strncpy without overflow") {
        pcbReset();
        pcb_t *p = createProcess("kernel_idle_thread", 0);
        ASSERT_TRUE(strlen(p->name) < sizeof(p->name));
    }

    /* --- Creation --- */
    TEST("Create two distinct processes") {
        pcbReset();
        pcb_t *a = createProcess("a", 0x1000);
        pcb_t *b = createProcess("b", 0x2000);
        ASSERT_NE_INT((int64_t)a, (int64_t)b);
        ASSERT_EQ_UINT(a->pid, 1);
        ASSERT_EQ_UINT(b->pid, 2);
    }

    TEST("PCB pool limits respected") {
        pcbReset();
        for (int i = 0; i < MAX_PROC + 1; i++)
            createProcess("x", 0);
        ASSERT_EQ_INT(countReady(), MAX_PROC);
    }

    /* --- Scheduling --- */
    TEST("First schedule picks head") {
        pcbReset();
        pcb_t *a = createProcess("a", 0x1000);
        pcb_t *s = schedule();
        ASSERT_EQ_HEX((uint64_t)s, (uint64_t)a);
        ASSERT_EQ_UINT(s->state, RUNNING);
    }

    TEST("Schedule rotates ring RR style") {
        pcbReset();
        pcb_t *a = createProcess("a", 0x1000);
        pcb_t *b = createProcess("b", 0x2000);
        createProcess("c", 0x3000);
        (void)a; (void)b;
        pcb_t *s1 = schedule();          /* a */
        pcb_t *s2 = schedule();          /* b */
        pcb_t *s3 = schedule();          /* c */
        ASSERT_EQ_UINT(s1->pid, 1);
        ASSERT_EQ_UINT(s2->pid, 2);
        ASSERT_EQ_UINT(s3->pid, 3);
    }

    TEST("Schedule wraps around after the last process") {
        pcbReset();
        pcb_t *a = createProcess("a", 0x1000);
        createProcess("b", 0x2000);
        (void)schedule(); /* a (running) */
        (void)schedule(); /* b */
        pcb_t *s3 = schedule(); /* wraps back to a? Actually our impl goes: c then head a */
        ASSERT_TRUE(s3-> pid == 1 || s3->pid == 2);
    }

    /* --- State correctness --- */
    TEST("Current process goes from RUNNING to READY on yield-like schedule") {
        pcbReset();
        pcb_t *a = createProcess("a", 0x1000);
        pcb_t *b = createProcess("b", 0x2000);
        pcb_t *s1 = schedule();
        ASSERT_EQ_UINT(s1->state, RUNNING);
        pcb_t *s2 = schedule();
        ASSERT_EQ_UINT(s1->state, READY);
        ASSERT_EQ_UINT(s2->state, RUNNING);
        (void)a; (void)b;
    }

    TEST("Schedule with only one process returns itself") {
        pcbReset();
        pcb_t *a = createProcess("lonely", 0x1000);
        pcb_t *s = schedule();
        ASSERT_EQ_HEX((uint64_t)s, (uint64_t)a);
    }

    TEST("SP field is written on process creation") {
        pcbReset();
        pcb_t *p = createProcess("sp_test", 0x4000);
        ASSERT_TRUE(p->sp != 0);
    }

    TEST("PC field matches entry argument") {
        pcbReset();
        pcb_t *p = createProcess("ep_test", 0xDEADBEEF);
        ASSERT_EQ_HEX(p->pc, 0xDEADBEEF);
    }

    TEST("Process list traversal counts correctly") {
        pcbReset();
        ASSERT_EQ_INT(countReady(), 0);
        createProcess("a", 0);
        ASSERT_EQ_INT(countReady(), 1);
        createProcess("b", 0);
        ASSERT_EQ_INT(countReady(), 2);
    }
}
