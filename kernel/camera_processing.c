/* ==========================================================================
 * Camera Processing & Snapshot Engine (Using mathc & storage)
 * ========================================================================== */

#include "../libs/mathc.h"
#include "../include/types.h"

#define CAM_WIDTH  320
#define CAM_HEIGHT 240
#define FRAME_SIZE (CAM_WIDTH * CAM_HEIGHT)

// External storage and camera functions
extern int storage_write_file(const char* filename, const uint8_t* data, size_t size);
extern int camera_capture_frame(uint8_t* target_buffer);

// Buffer slots for frame differencing
static uint8_t prev_frame[FRAME_SIZE];
static uint8_t curr_frame[FRAME_SIZE];

/* ==========================================================================
 * 1. MOTION VECTOR PROCESSING
 * ========================================================================== */

// Uses mathc vector logic to measure movement intensity between frames
float process_camera_motion_vectors(void) {
    // Capture current frame from sensor
    camera_capture_frame(curr_frame);

    float total_magnitude = 0.0f;
    int sample_count = 0;

    // Sample blocks across the frame using vector coordinates
    for (int y = 0; y < CAM_HEIGHT; y += 16) {
        for (int x = 0; x < CAM_WIDTH; x += 16) {
            int idx = y * CAM_WIDTH + x;

            // Treat old and current pixel patches as 2D vectors (using vec2)
            vec2 v_old = { (float)prev_frame[idx], (float)prev_frame[idx + 1] };
            vec2 v_curr = { (float)curr_frame[idx], (float)curr_frame[idx + 1] };

            // Calculate distance vector using mathc
            vec2 v_diff;
            v_diff.x = v_curr.x - v_old.x;
            v_diff.y = v_curr.y - v_old.y;

            // Calculate vector length (magnitude of motion)
            float mag = vec2_length(v_diff);
            total_magnitude += mag;
            sample_count++;
        }
    }

    // Copy current frame to previous for the next cycle
    for (size_t i = 0; i < FRAME_SIZE; i++) {
        prev_frame[i] = curr_frame[i];
    }

    return (sample_count > 0) ? (total_magnitude / (float)sample_count) : 0.0f;
}

/* ==========================================================================
 * 2. SNAPSHOT & SAVE FUNCTION
 * ========================================================================== */

// Snaps a picture from the camera and writes it to storage
int camera_snap_and_save(const char* filename) {
    uint8_t snapshot_buffer[FRAME_SIZE];

    // 1. Capture raw frame from camera driver
    if (camera_capture_frame(snapshot_buffer) != 0) {
        return -1; // Capture failed
    }

    // 2. Save raw frame data using your custom storage subsystem
    int result = storage_write_file(filename, snapshot_buffer, FRAME_SIZE);
    
    return result; // Returns 0 on success, negative on error
}
