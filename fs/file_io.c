/* ==========================================================================
 * OS File I/O Subsystem - Read & Write Engine
 * Connects high-level file reading/writing directly to storage.c sectors
 * ========================================================================== */

#include "../include/types.h"

// External declarations from storage.c
extern int storage_read_sector(uint32_t lba, uint8_t* target_buffer);
extern int storage_write_sector(uint32_t lba, const uint8_t* source_buffer);
extern int storage_find_file(const char* filename, void* out_entry);

#define SECTOR_SIZE 512

/* ==========================================================================
 * 1. FILE READ OPERATION
 * ========================================================================== */

// Read data from a named file into a RAM buffer
int file_read(const char* filename, uint8_t* buffer, uint32_t bytes_to_read) {
    // Structure matching our FFS FileEntry in storage.c
    typedef struct {
        char     filename[32];
        uint32_t starting_lba;
        uint32_t file_size;
        uint8_t  is_valid;
    } FileEntry;

    FileEntry file;
    if (storage_find_file(filename, &file) != 0) {
        return -1; // File not found on disk
    }

    // Ensure we don't read past the actual file size
    if (bytes_to_read > file.file_size) {
        bytes_to_read = file.file_size;
    }

    uint32_t sectors_to_read = (bytes_to_read + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint8_t temp_sector[SECTOR_SIZE];

    for (uint32_t i = 0; i < sectors_to_read; i++) {
        if (storage_read_sector(file.starting_lba + i, temp_sector) != 0) {
            return -1; // Sector read failure
        }

        // Copy sector contents into the target RAM buffer
        uint32_t bytes_to_copy = SECTOR_SIZE;
        if (i == sectors_to_read - 1) {
            bytes_to_copy = bytes_to_read - (i * SECTOR_SIZE);
            if (bytes_to_copy == 0) bytes_to_copy = SECTOR_SIZE;
        }

        for (uint32_t b = 0; b < bytes_to_copy && (i * SECTOR_SIZE + b) < bytes_to_read; b++) {
            buffer[(i * SECTOR_SIZE) + b] = temp_sector[b];
        }
    }

    return (int)bytes_to_read;
}

/* ==========================================================================
 * 2. FILE WRITE OPERATION
 * ========================================================================== */

// Write data from a RAM buffer directly to a named file's sectors on disk
int file_write(const char* filename, const uint8_t* buffer, uint32_t bytes_to_write) {
    typedef struct {
        char     filename[32];
        uint32_t starting_lba;
        uint32_t file_size;
        uint8_t  is_valid;
    } FileEntry;

    FileEntry file;
    if (storage_find_file(filename, &file) != 0) {
        return -1; // File must exist on disk before writing
    }

    // Check if the write fits within the allocated file size boundaries
    if (bytes_to_write > file.file_size) {
        bytes_to_write = file.file_size;
    }

    uint32_t sectors_to_write = (bytes_to_write + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint8_t temp_sector[SECTOR_SIZE];

    for (uint32_t i = 0; i < sectors_to_write; i++) {
        // Read sector first to preserve any trailing padding if partial write
        storage_read_sector(file.starting_lba + i, temp_sector);

        uint32_t bytes_to_copy = SECTOR_SIZE;
        if (i == sectors_to_write - 1) {
            bytes_to_copy = bytes_to_write - (i * SECTOR_SIZE);
            if (bytes_to_copy == 0) bytes_to_copy = SECTOR_SIZE;
        }

        // Copy new data into sector buffer
        for (uint32_t b = 0; b < bytes_to_copy; b++) {
            temp_sector[b] = buffer[(i * SECTOR_SIZE) + b];
        }

        // Flush modified sector back to hardware storage
        if (storage_write_sector(file.starting_lba + i, temp_sector) != 0) {
            return -1; // Sector write failure
        }
    }

    return (int)bytes_to_write;
}
