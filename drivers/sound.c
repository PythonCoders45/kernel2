#include "../include/types.h"

/* ==========================================================================
 * 1. ARCHITECTURE-SPECIFIC LOW-LEVEL I/O ROUTINES
 * ========================================================================== */

#if defined(__i386__) || defined(__x86_64__)
// x86 uses port I/O for the PIT and PC Speaker
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#elif defined(__aarch64__) || defined(__arm__)
// ARM typically uses Memory-Mapped I/O (MMIO) for hardware control
// You would define pointer mappings to your board's timer/PWM registers here
static volatile uint32_t* const ARM_TIMER_BASE = (uint32_t*) 0x3F00B000; // Example BCM2835 base

#elif defined(__riscv)
// RISC-V uses MMIO mapped via platform-level interrupt controllers (PLIC/CLINT)
static volatile uint64_t* const RISCV_MTIME = (uint64_t*) 0x0200bff8;
#endif


/* ==========================================================================
 * 2. UNIFIED ARCHITECTURE-INDEPENDENT PUBLIC API
 * ========================================================================== */

// Play a tone at a given frequency (Hz)
void play_sound(uint32_t frequency) {
    if (frequency == 0) return;

#if defined(__i386__) || defined(__x86_64__)
    // x86 PC Speaker Implementation via PIT Channel 2
    uint32_t div = 1193180 / frequency;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div));
    outb(0x42, (uint8_t)(div >> 8));

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }

#elif defined(__aarch64__) || defined(__arm__)
    // ARM PWM / Audio Interface Placeholder
    // (Actual implementation talks to board PWM controller registers)
    (void)frequency; // Prevent unused parameter warning during early development

#elif defined(__riscv)
    // RISC-V Timer-based sound toggle Placeholder
    (void)frequency;

#else
    #error "Unsupported CPU architecture for sound driver!"
#endif
}

// Stop audio playback
void nosound(void) {
#if defined(__i386__) || defined(__x86_64__)
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);

#elif defined(__aarch64__) || defined(__arm__)
    // Turn off ARM PWM output

#elif defined(__riscv)
    // Turn off RISC-V audio line
#endif
}
