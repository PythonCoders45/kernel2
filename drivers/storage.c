/* ==========================================================================
 * OS Storage & File Subsystem - Advanced High-Performance ATA Driver
 * Features: 32-bit Fast-Block Transfers, Directory Caching, and FFS Management
 * ========================================================================== */

#include "../include/types.h"

// ATA Controller I/O Ports (Primary Bus)
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_FEATURE      0x1F1
#define ATA_PRIMARY_SEC_COUNT    0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMMAND      0x1F7
#define ATA_PRIMARY_STATUS       0x1F7

// ATA Commands
#define ATA_CMD_READ_PIO         0x20
#define ATA_CMD_WRITE_PIO        0x30
#define ATA_CMD_CACHE_FLUSH      0xE7

// Status Register Bits
#define ATA_SR_BSY               0x80    // Busy
#define ATA_SR_DRDY              0x40    // Drive ready
#define ATA_SR_DRQ               0x08    // Data request ready

// File System & Caching Constants
#define MAX_FILES_IN_DIR         64
#define SECTOR_SIZE              512

// File Directory Entry Structure (Stored in Sector 1)
typedef struct {
    char     filename[32];       // Null-terminated file name
    uint32_t starting_lba;   // Starting disk sector
    uint32_t file_size;      // Size in bytes
    uint8_t  is_valid;       // 1 if file exists, 0 if empty slot
} __attribute__((packed)) FileEntry;

typedef struct {
    uint32_t total_files;
    FileEntry entries[MAX_FILES_IN_DIR];
} __attribute__((packed)) DirectoryTable;

// Internal cache state to avoid redundant disk reads for directory lookups
static DirectoryTable cached_directory;
static uint8_t directory_cache_loaded = 0;

/* ==========================================================================
 * 1. HIGH-SPEED ARCHITECTURE-SPECIFIC I/O HELPERS
 * ========================================================================== */

#if defined(__i386__) || defined(__x86_64__)
static inline void storage_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t storage_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Blazing fast 32-bit double-word block transfer (moves 4 bytes per iteration)
static inline void storage_insd(uint16_t port, void* addr, uint32_t dword_count) {
    __asm__ volatile ("rep insd" : "+D"(addr), "+c"(dword_count) : "d"(port) : "memory");
}

static inline void storage_outsd(uint16_t port, const void* addr, uint32_t dword_count) {
    __asm__ volatile ("rep outsd" : "+S"(addr), "+c"(dword_count) : "d"(port) : "memory");
}
#endif

/* ==========================================================================
 * 2. STORAGE CONTROLLER INITIALIZATION & POLLING
 * ========================================================================== */

void storage_init(void) {
#if defined(__i386__) || defined(__x86_64__)
    storage_outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0); // Select master drive, LBA mode
    for (volatile int i = 0; i < 10000; i++);
    directory_cache_loaded = 0;
#endif
}

static int storage_poll(void) {
#if defined(__i386__) || defined(__x86_64__)
    for (int timeout = 0; timeout < 100000; timeout++) {
        uint8_t status = storage_inb(ATA_PRIMARY_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            return 0; // Ready
        }
        if (status & 0x01) {
            return -1; // Hardware error bit set
        }
    }
#endif
    return -1; // Timeout
}

/* ==========================================================================
 * 3. OPTIMIZED SECTOR READ/WRITE ROUTINES
 * ========================================================================== */

int storage_read_sector(uint32_t lba, uint8_t* target_buffer) {
#if defined(__i386__) || defined(__x86_64__)
    storage_outb(ATA_PRIMARY_SEC_COUNT, 1);
    storage_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)(lba));
    storage_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    storage_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    storage_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    storage_outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);

    if (storage_poll() != 0) return -1;

    // Read 512 bytes using 128 double-words (32-bit blocks) for maximum throughput
    storage_insd(ATA_PRIMARY_DATA, target_buffer, 128);
    return 0;
#else
    (void)lba; (void)target_buffer;
    return -1;
#endif
}

int storage_write_sector(uint32_t lba, const uint8_t* source_buffer) {
#if defined(__i386__) || defined(__x86_64__)
    storage_outb(ATA_PRIMARY_SEC_COUNT, 1);
    storage_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)(lba));
    storage_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    storage_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    storage_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    storage_outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);

    if (storage_poll() != 0) return -1;

    // Write 512 bytes using 32-bit double-word blocks
    storage_outsd(ATA_PRIMARY_DATA, source_buffer, 128);

    storage_outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
    storage_poll();
    return 0;
#else
    (void)lba; (void)source_buffer;
    return -1;
#endif
}

/* ==========================================================================
 * 4. ADVANCED FILE SYSTEM & DIRECTORY CACHING LAYER
 * ========================================================================== */

static int storage_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Refresh or load the directory table into high-speed RAM cache
int storage_refresh_directory_cache(void) {
    if (storage_read_sector(1, (uint8_t*)&cached_directory) != 0) {
        return -1;
    }
    directory_cache_loaded = 1;
    return 0;
}

// Find a file instantly using the in-RAM directory cache
int storage_find_file(const char* filename, FileEntry* out_entry) {
    if (!directory_cache_loaded) {
        if (storage_refresh_directory_cache() != 0) {
            return -1;
        }
    }

    for (uint32_t i = 0; i < MAX_FILES_IN_DIR; i++) {
        if (cached_directory.entries[i].is_valid) {
            if (storage_strcmp(cached_directory.entries[i].filename, filename) == 0) {
                *out_entry = cached_directory.entries[i];
                return 0;
            }
        }
    }

    return -1; // File not found
}

// High-speed multi-sector file loader into system RAM
int storage_load_file_to_ram(const char* filename, uint8_t* ram_destination) {
    FileEntry file;
    if (storage_find_file(filename, &file) != 0) {
        return -1; 
    }

    uint32_t sectors_to_read = (file.file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    for (uint32_t i = 0; i < sectors_to_read; i++) {
        uint32_t target_lba = file.starting_lba + i;
        uint8_t* dest_ptr = ram_destination + (i * SECTOR_SIZE);
        
        if (storage_read_sector(target_lba, dest_ptr) != 0) {
            return -1; 
        }
    }

    return 0; 
}
