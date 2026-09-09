/* ==========================================================================
 * OS Core Runtime - cbasis.c (Complete Standard Library Replacement)
 * Provides memory management, string parsing, and data conversion for bare metal
 * ========================================================================== */

#include "../include/types.h"

// Define basic size_t if not already defined
#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

/* ==========================================================================
 * 1. MEMORY MANIPULATION UTILITIES
 * ========================================================================== */

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

/* ==========================================================================
 * 2. STRING MANIPULATION UTILITIES
 * ========================================================================== */

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* saved_dest = dest;
    while ((*dest++ = *src++));
    return saved_dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (*s == '\0') return 0;
        s++;
    }
    return (char*)s;
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest + strlen(dest);
    while (*src) {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

/* ==========================================================================
 * 3. TYPE CONVERSIONS & UTILITIES
 * ========================================================================== */

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    while (*str == ' ' || (*str >= '\t' && *str <= '\r')) str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

char* itoa(int value, char* str, int base) {
    char* rc = str;
    char* ptr = str;
    char* low;
    int val = value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    if (val < 0 && base == 10) {
        *ptr++ = '-';
        val = -val;
    }

    rc = ptr;
    do {
        *ptr++ = "0123456789abcdef"[val % base];
        val /= base;
    } while (val);

    *ptr-- = '\0';
    low = rc;
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return str;
}

/* ==========================================================================
 * 4. DYNAMIC MEMORY ALLOCATOR (HEAP MANAGER)
 * ========================================================================== */

#define HEAP_START_ADDRESS 0x100000 // Safe physical RAM location (1MB mark)
#define HEAP_MAX_SIZE      0x400000 // 4MB Heap window

static uintptr_t heap_current = HEAP_START_ADDRESS;
static uintptr_t heap_limit = HEAP_START_ADDRESS + HEAP_MAX_SIZE;

void* malloc(size_t size) {
    if (size == 0) return 0;

    // Align allocation to 4-byte boundaries for hardware efficiency
    size = (size + 3) & ~3;

    if (heap_current + size > heap_limit) {
        return 0; // Out of heap memory!
    }

    void* allocated_ptr = (void*)heap_current;
    heap_current += size;

    return allocated_ptr;
}

void free(void* ptr) {
    // Note: A basic bump allocator advances forward. 
    // Individual block reclamation requires a linked-list or bitmap free manager.
    (void)ptr;
}

void* realloc(void* ptr, size_t size) {
    if (ptr == 0) return malloc(size);
    if (size == 0) {
        free(ptr);
        return 0;
    }
    
    // Fallback naive realloc behavior for freestanding kernel usage
    void* new_ptr = malloc(size);
    if (new_ptr != 0) {
        // Copy existing data over (assuming standard safe chunk sizing)
        memcpy(new_ptr, ptr, size);
    }
    return new_ptr;
}
