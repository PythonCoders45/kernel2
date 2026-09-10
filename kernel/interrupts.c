/* ==========================================================================
 * Interrupt Service Routine (ISR) Hub
 * Manages hardware signals, timers, and peripheral interrupts.
 * ========================================================================== */

#include "../include/types.h"

// Define interrupt types/channels
typedef enum {
    IRQ_TIMER = 0,
    IRQ_ACCESORY_PORT,
    IRQ_CAMERA_FRAME,
    IRQ_MAX
} IRQType;

// Function pointer type for interrupt handlers
typedef void (*ISRHandler)(void);

static ISRHandler interrupt_table[IRQ_MAX];

/* ==========================================================================
 * 1. INTERRUPT REGISTRATION & DISPATCH
 * ========================================================================== */

void isr_init(void) {
    for (int i = 0; i < IRQ_MAX; i++) {
        interrupt_table[i] = 0; // Clear all handlers initially
    }
}

// Register a custom function to handle a specific hardware interrupt
void isr_register_handler(IRQType irq, ISRHandler handler) {
    if (irq < IRQ_MAX) {
        interrupt_table[irq] = handler;
    }
}

// The CPU calls this master function when an interrupt hardware signal fires
void kernel_interrupt_dispatcher(IRQType irq) {
    if (irq < IRQ_MAX && interrupt_table[irq] != 0) {
        // Execute the registered driver handler immediately
        interrupt_table[irq]();
    }
}
