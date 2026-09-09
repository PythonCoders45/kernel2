/* ==========================================================================
 * OS Extra Utilities & Helper Library (kernel/extra.c)
 * Provides pre-built health metrics, math shortcuts, and system helpers.
 * ========================================================================== */

#include "../include/types.h"
#include "../libs/mathc.h"

// Global User/Player Profile Data managed by the OS
typedef struct {
    float weight_kg;
    float height_m;
    float bmi;
    uint32_t lifetime_activity_points;
} OSUserMetrics;

static OSUserMetrics current_user = { 70.0f, 1.75f, 22.86f, 0 };

/* ==========================================================================
 * 1. BUILT-IN HEALTH & FITNESS MATH
 * ========================================================================== */

// Calculates and caches the user's Body Mass Index automatically
float os_extra_calculate_bmi(float weight_kg, float height_m) {
    if (height_m <= 0.0f) return 0.0f;
    current_user.weight_kg = weight_kg;
    current_user.height_m = height_m;
    current_user.bmi = weight_kg / (height_m * height_m);
    return current_user.bmi;
}

// Allows any fitness game to fetch the current user's BMI without calculation code
float os_extra_get_user_bmi(void) {
    return current_user.bmi;
}

// Adds motion points from workout routines to the user's global profile score
void os_extra_add_activity_score(uint32_t points) {
    current_user.lifetime_activity_points += points;
}

uint32_t os_extra_get_activity_score(void) {
    return current_user.lifetime_activity_points;
}

/* ==========================================================================
 * 2. GENERAL MATH & VECTOR SHORTCUTS (Using mathc)
 * ========================================================================== */

// Clamps a floating-point value between a minimum and maximum limit
float os_extra_clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Quickly computes distance between two 2D coordinate vectors for collision/UI
float os_extra_vector_distance(vec2 a, vec2 b) {
    vec2 diff = { b.x - a.x, b.y - a.y };
    return vec2_length(diff);
}e
