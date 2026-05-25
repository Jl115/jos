/* ============================================================================
 * Mock GICv2 registers for host-side testing.
 * Declares the volatile globals that replace the MMIO base-address casts
 * when TEST_BUILD is defined.
 * ============================================================================ */
#include "drivers/gic/registry.h"

volatile gicdT mock_gicd;
volatile giccT mock_gicc;
