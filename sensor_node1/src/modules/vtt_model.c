/**
 * @file vtt_model.c
 * @brief Implementation of VTT Mathematical Logic
 * * * This file implements the mathematical equations for:
 * 1. Critical Relative Humidity (RH_crit) as a function of Temperature.
 * 2. Mold Growth Rate (dM/dt > 0) based on Temp, RH, and Surface Quality.
 * 3. Mold Decline Rate (dM/dt < 0) based on dry periods.
 */
#include "vtt_model.h"
#include <math.h>
#include <string.h>

// --- Private Constants & Tuning ---

// Maximum Mold Index (Physical limit)
#define MAX_INDEX_CAP       6.0f
#define MIN_INDEX_CAP       0.0f

// Baseline constant for Critical RH at warm temperatures (>20C)
#define RH_CRIT_MIN_WARM    80.0f

// --- Helper Functions (Private) ---

/**
 * @brief Clamps a float value between a minimum and maximum.
 */
static inline float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief Returns the scaling factor (k1) for growth intensity.
 * @param mat Material Type
 * @return float k1 coefficient (0.0 to 1.0)
 */
static float get_material_k1(vtt_material_t mat){
    switch (mat)
    {
    case VTT_MAT_SENSITIVE: return 1.0f;
    case VTT_MAT_MEDIUM_RESISTANT: return 0.3f;
    case VTT_MAT_RESISTANT: return 0.1f;
    
    default: return 1.0f;
    
    }
}

/**
 * @brief Calculates the Critical Humidity (RH_crit) required for mold to start growing.
 * Formula depends on current temperature.
 * * @param ctx Context (stores result in ctx->rh_crit)
 * @param T Temperature in Celsius
 */
static void calculate_rh_critical(vtt_state_t *ctx, float T){
    if (T > 20){
        // For warm temps, the baseline is constant
        ctx->rh_crit = RH_CRIT_MIN_WARM + ctx->rh_mat;
    } else {
        // Polynomial approximation for cooler temps (VTT equation)
        // RH_crit rises as temperature drops (harder for mold to grow in cold)
        float t2 = T * T;
        float t3 = t2 * T;
        float rh_base = (-0.00267f * t3) + (0.160f * t2) - (3.13f * T) + 100.0f;
        ctx->rh_crit = rh_base + ctx->rh_mat;
    }
}

// --- Public API Implementation ---

void vtt_init(vtt_state_t *ctx, vtt_material_t mat){
    memset(ctx, 0, sizeof(vtt_state_t));
    ctx -> material = mat;
    ctx -> mold_index = 0.0f;

    // TODO: Decide SQ, W, rh_mat Values - rh_mat values are dummy, will confirm the correct values afterwards
    // Set material specific coefficients (SQ, W, rh_mat)
    // These tune the sensitivity of the differential equations
    switch (mat)
    {
    case VTT_MAT_SENSITIVE:
        ctx->surface_quality = 0.0f; // Rough (easier for spores)
        ctx->wood_species = 0.0f;    // Pine (nutrient rich)
        ctx->rh_mat = 0.0f;          // No extra resistance
        break;

    case VTT_MAT_MEDIUM_RESISTANT:
        ctx->surface_quality = 1.0f; // Smoother
        ctx->wood_species = 1.0f;
        ctx->rh_mat = 3.0f;          // Needs +3% higher RH to grow
        break;

    case VTT_MAT_RESISTANT:
        ctx-> surface_quality = 1.0f;
        ctx-> wood_species = 1.0f;
        ctx-> rh_mat = 6.0f;
        break;

    default: // Fail-safe defaults (Worst Case)
        ctx-> surface_quality = 0.0f;
        ctx-> wood_species = 0.0f;
        ctx-> rh_mat = 0.0f;
        break;
    }
}

void vtt_update(vtt_state_t *ctx, float temp_c, float rh_percent, float time_step_hours){

    // 1. Sanitize Inputs (Prevent math errors)
    float safe_t = clampf(temp_c, 0.1f, 60.0f); 
    float safe_rh = clampf(rh_percent, 1.0f, 100.0f);

    // 2. Determine if conditions allow growth
    calculate_rh_critical(ctx,safe_t);

    if (safe_rh > ctx->rh_crit){
        // --- GROWTH PHASE (Wet) ---
        ctx -> time_wet_hours += time_step_hours;
        ctx -> time_dry_hours = 0; 
        ctx -> growing_condition = true; //Growing Conditions 

        // Step A: Calculate Maximum Possible Mold Index for this RH
        // (Mold cannot grow infinitely if RH is only slightly above critical)
        float m_max_calc = 6.0f * (safe_rh - ctx->rh_crit) / (100.0f - ctx->rh_crit);
        ctx -> max_possible_index = clampf(m_max_calc, 0.0f, 6.0f);

        // Step B: Calculate Base Growth Speed (Polynomial Regression)

        float exponent = (-0.68f * logf(safe_t)) - (13.9f * logf(safe_rh)) + (0.14f * ctx->wood_species) - (0.33f * ctx->surface_quality) + 66.02f;
        float base_growth_rate = 1.0f / (7.0f * expf(exponent));

        // Step C: Apply Intensity Scaling (k1) and Saturation (k2)
        // Growth slows down as it approaches the max possible index (k2 factor)
        float k1 = get_material_k1(ctx -> material);
        float dist_to_max = ctx->mold_index - ctx->max_possible_index;

        // k2 = max(1 - exp(2.3 * (M - M_max)), 0)
        float k2 = fmaxf(1.0f - expf(2.3f * dist_to_max), 0.0f); 

        // Step D: Integrate (Euler Method)
        float dM = k1 * k2 * base_growth_rate * time_step_hours;
        ctx->mold_index += dM;
        ctx->last_growth_rate = dM / time_step_hours;
    } else {
        // --- DECLINE PHASE (Dry) ---
        ctx -> time_dry_hours += time_step_hours;
        ctx -> time_wet_hours = 0;
        ctx -> growing_condition = false; //Decline Conditions 

        float decline_rate = 0.0f;

        // Step A: Determine Decline Rate based on Dry Duration
        // Short dry spells cause slow decline; long spells kill spores faster.
        // TODO: Decide Decline Rates - Temporary decline rates, these are not final - will have to further research to get the best rates.
        if (ctx->time_dry_hours <= 6.0f){
            decline_rate = -0.032f; //Initial resistance (Latency)
        } else if (ctx->time_dry_hours <= 24.0f) { 
            decline_rate = 0.0f; // Stability period
        } else {
            decline_rate = -0.016f; // Long-term die-off
        }

        // Step B: Integrate
        float dM = decline_rate * time_step_hours;
        ctx -> mold_index += dM;
        ctx -> last_growth_rate = dM / time_step_hours; 
    }
    // 3. Final Clamp (Index cannot be negative or exceed 6.0)
    ctx -> mold_index = clampf(ctx->mold_index, MIN_INDEX_CAP, MAX_INDEX_CAP);
}


vtt_risk_level_t vtt_get_risk_level(vtt_state_t *ctx){
    if (ctx->mold_index < 1.0f) return MOLD_RISK_CLEAN;
    if (ctx->mold_index < 3.0f) return MOLD_RISK_WARNING;
    if (ctx->mold_index < 4.0f) return MOLD_RISK_ALERT;
    return MOLD_RISK_CRITICAL;
}