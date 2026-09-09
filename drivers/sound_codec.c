/* ==========================================================================
 * OS Audio Codec Subsystem - Huffman Decoding & MDCT Transform Pipeline
 * Designed for bare-metal decoding of compressed audio streams (MP3/Ogg)
 * ========================================================================== */

#include "../include/types.h"

// Audio Stream State Structure
typedef struct {
    const uint8_t* data_ptr;
    uint32_t byte_offset;
    uint32_t bit_offset;
    uint32_t total_size_bytes;
} BitStream;

// Huffman Node Structure for Variable-Length Code (VLC) Decoding
typedef struct {
    int16_t value;     // Decoded symbol or audio scale factor
    int16_t left;      // Index of left branch (0 if leaf)
    int16_t right;     // Index of right branch (0 if leaf)
} HuffmanNode;

/* ==========================================================================
 * 1. BITSTREAM READER UTILITIES
 * ========================================================================== */

// Initialize bitstream pointer
void bitstream_init(BitStream* stream, const uint8_t* buffer, uint32_t size) {
    stream->data_ptr = buffer;
    stream->byte_offset = 0;
    stream->bit_offset = 0;
    stream->total_size_bytes = size;
}

// Read a specified number of bits (1 to 32) from the raw compressed stream
uint32_t bitstream_read_bits(BitStream* stream, uint8_t num_bits) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < num_bits; i++) {
        if (stream->byte_offset >= stream->total_size_bytes) {
            return 0; // EOF protection
        }
        
        uint8_t current_byte = stream->data_ptr[stream->byte_offset];
        uint8_t bit = (current_byte >> (7 - stream->bit_offset)) & 1;
        
        result = (result << 1) | bit;
        
        stream->bit_offset++;
        if (stream->bit_offset == 8) {
            stream->bit_offset = 0;
            stream->byte_offset++;
        }
    }
    return result;
}

/* ==========================================================================
 * 2. HUFFMAN DECODING ENGINE
 * ========================================================================== */

// Traverse a Huffman tree using bits read from the stream to decode symbols
int16_t huffman_decode_symbol(BitStream* stream, const HuffmanNode* tree, int root_index) {
    int current_idx = root_index;
    
    // Traverse until a leaf node is reached (where left and right are zero or invalid)
    while (tree[current_idx].left != 0 || tree[current_idx].right != 0) {
        uint32_t bit = bitstream_read_bits(stream, 1);
        if (bit == 0) {
            current_idx = tree[current_idx].left;
        } else {
            current_idx = tree[current_idx].right;
        }
    }
    
    return tree[current_idx].value;
}

/* ==========================================================================
 * 3. INTEGER-OPTIMIZED MDCT (MODIFIED DISCRETE COSINE TRANSFORM)
 * ========================================================================== */

// Transforms frequency-domain spectral coefficients back into time-domain PCM samples
// Optimized using fixed-point math to avoid floating-point overhead in kernel space
void audio_compute_mdct(const int32_t* frequency_spectrum, int16_t* time_domain_pcm, uint32_t sample_count) {
    uint32_t N = sample_count;
    uint32_t half_N = N / 2;

    for (uint32_t n = 0; n < N; n++) {
        int32_t accumulated_sum = 0;
        
        for (uint32_t k = 0; k < half_N; k++) {
            // Simplified fixed-point cosine kernel approximation for bare-metal execution
            // Real codecs use precalculated trigonometric lookup tables (twiddle factors)
            int32_t coefficient = frequency_spectrum[k];
            
            // Phase adjustment calculation
            int32_t angle_numerator = (2 * n + 1 + half_N) * (2 * k + 1);
            
            // Scaled integer approximation of cosine wave behavior
            // (In production, replace with a 1024-entry fixed-point cos table)
            int32_t pseudo_cos = 1024 - ((angle_numerator * angle_numerator) % 2048); 
            
            accumulated_sum += (coefficient * pseudo_cos) / 1024;
        }

        // Windowing and scaling output to 16-bit PCM range
        int32_t scaled_sample = accumulated_sum / 65536;
        if (scaled_sample > 32767)  scaled_sample = 32767;
        if (scaled_sample < -32768) scaled_sample = -32768;

        time_domain_pcm[n] = (int16_t)scaled_sample;
    }
}
