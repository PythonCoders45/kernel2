/* ==========================================================================
 * OS Security & Stability Subsystem - CPU Safeguards
 * Prevents memory corruption, buffer overflows, and invalid CPU operations
 * ========================================================================== */

#include "../include/types.h"

// Define valid system memory boundaries (e.g., 0MB to 16MB)
#define SYSTEM_MEM_MIN  0x00000000
#define SYSTEM_MEM_MAX  0x01000000 

/* ==========================================================================
 * 1. MEMORY BOUNDS & INTEGRITY CHECKING
 * ========================================================================== */

// Validates that a target pointer falls within safe kernel RAM limits
int security_validate_pointer(const void* ptr, size_t size) {
    uintptr_t addr = (uintptr_t)ptr;
    
    // Check for null pointers
    if (addr == 0) return 0;
    
    // Check if the memory block exceeds allowed upper bounds or wraps around
    if (addr < SYSTEM_MEM_MIN || (addr + size) > SYSTEM_MEM_MAX || (addr + size) < addr) {
        return -1; // Out-of-bounds violation detected!
    }
    
    return 0; // Pointer is safe
}

// Safe memory copy wrapper with automatic bounds checking
int security_safe_memcpy(void* dest, const void* src, size_t size) {
    if (security_validate_pointer(dest, size) != 0 || 
        security_validate_pointer(src, size) != 0) {
        return -1; // Blocked potential buffer overflow or illegal read/write
    }

    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return 0;
}

/* ==========================================================================
 * 2. CPU EXCEPTION & FAULT HOOKS
 * ========================================================================== */

// Catch unexpected CPU exceptions (e.g., Division by Zero, Invalid Opcode)
void security_handle_cpu_exception(uint32_t exception_vector, uint32_t error_code) {
    // In a full OS, this halts execution safely and logs the error 
    // rather than letting the CPU loop into undefined states.
    
    (void)exception_vector;
    (void)error_code;

    // Trigger emergency system lockout / safe halt
    __asm__ volatile ("cli; hlt"); 
}

/* ==========================================================================
 * 3. STREAM & PACKET FILTERING (Software Firewall)
 * ========================================================================== */

// Filters incoming data packets from storage or network to block malicious payloads
int security_filter_input_stream(const uint8_t* stream, size_t length) {
    if (length == 0 || stream == 0) return -1;

    // Scan for dangerous header anomalies or unauthorized command flags
    for (size_t i = 0; i < length - 3; i++) {
        // Example check: Block forbidden byte patterns or malformed execution headers
        if (stream[i] == 0xDE && stream[i+1] == 0xAD && stream[i+2] == 0xBE && stream[i+3] == 0xEF) {
            return -1; // Flagged malicious pattern
        }
    }

    return 0; // Stream passed safety checks
}
