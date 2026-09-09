/* ==========================================================================
 * OS Storage Subsystem - Direct ATA/IDE Sector Driver
 * Communicates directly with storage controllers via port I/O
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

/* ==========================================================================
 * 1. LOW-LEVEL PORT I/O HELPERS
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

static inline void storage_insw(uint16_t port, void* addr, uint32_t word_count) {
    __asm__ volatile ("rep insw" : "+D"(addr), "+c"(word_count) : "d"(port) : "memory");
}

static inline void storage_outsw(uint16_t port, const void* addr, uint32_t word_count) {
    __asm__ volatile ("rep outsw" : "+S"(addr), "+c"(word_count) : "d"(port) : "memory");
}
#endif

/* ==========================================================================
 * 2. STORAGE CONTROLLER INITIALIZATION
 * ========================================================================== */

void storage_init(void) {
    // Select primary drive and wait for it to be ready
#if defined(__i386__) || defined(__x86_64__)
    storage_outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0); // Select master drive, LBA mode
    
    // Small delay for drive stabilization
    for (volatile int i = 0; i < 10000; i++);
#endif
}

/* ==========================================================================
 * 3. POLLING & STATUS CHECKS
 * ========================================================================== */

static int storage_poll(void) {
#if defined(__i386__) || defined(__x86_64__)
    // Wait for BSY to clear and DRQ to set
    for (int timeout = 0; timeout < 100000; timeout++) {
        uint8_t status = storage_inb(ATA_PRIMARY_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            return 0; // Ready
        }
        if (status & 0x01) { // ERR bit set
            return -1;
        }
    }
#endif
    return -1; // Timeout
}

/* ==========================================================================
 * 4. READ & WRITE SECTOR OPERATIONS (512 Bytes per Sector)
 * ========================================================================== */

// Read a 512-byte sector from storage into a memory buffer using LBA
int storage_read_sector(uint32_t lba, uint8_t* target_buffer) {
#if defined(__i386__) || defined(__x86_64__)
    storage_outb(ATA_PRIMARY_SEC_COUNT, 1);                    // Read 1 sector
    storage_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)(lba));         // LBA bits 0-7
    storage_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));    // LBA bits 8-15
    storage_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));   // LBA bits 16-23
    storage_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); // Master + LBA bits 24-27
    storage_outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);       // Issue read command

    if (storage_poll() != 0) {
        return -1; // Read error or timeout
    }

    // Transfer 256 words (512 bytes) directly from data port into RAM buffer
    storage_insw(ATA_PRIMARY_DATA, target_buffer, 256);
    return 0;
#else
    (void)lba;
    (void)target_buffer;
    return -1;
#endif
}

// Write a 512-byte sector from a memory buffer directly to storage
int storage_write_sector(uint32_t lba, const uint8_t* source_buffer) {
#if defined(__i386__) || defined(__x86_64__)
    storage_outb(ATA_PRIMARY_SEC_COUNT, 1);                    // Write 1 sector
    storage_outb(ATA_PRIMARY_LBA_LOW, (uint8_t)(lba));
    storage_outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    storage_outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    storage_outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    storage_outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);      // Issue write command

    if (storage_poll() != 0) {
        return -1;
    }

    // Transfer 256 words (512 bytes) from RAM buffer into hardware data port
    storage_outsw(ATA_PRIMARY_DATA, source_buffer, 256);

    // Flush drive cache to ensure data is permanently written
    storage_outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
    storage_poll();

    return 0;
#else
    (void)lba;
    (void)source_buffer;
    return -1;
#endif
}
