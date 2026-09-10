/* ==========================================================================
 * MMIO (Memory-Mapped I/O) Utilities
 * Allows direct read/write to hardware register addresses.
 * ========================================================================== */

#ifndef MMIO_H
#define MMIO_H

#include "types.h"

// Inline helper to read a 32-bit hardware register
static inline uint32_t mmio_read32(uint32_t address) {
    return *(volatile uint32_t*)address;
}

// Inline helper to write a 32-bit hardware register
static inline void mmio_write32(uint32_t address, uint32_t value) {
    *(volatile uint32_t*)address = value;
}

#endif
