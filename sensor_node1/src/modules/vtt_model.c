#include "vtt_model.h"
#include <math.h>
#include <string.h>

// --- Private Constants & Tuning ---

// Maximum Mold Index (Physical limit)
#define MAX_INDEX_CAP       6.0f
#define MIN_INDEX_CAP       0.0f

// Critical Humidity Model Constants (Pine/Sensitive Baseline)
// Using single precision floats (f suffix) for FPU optimization
#define RH_CRIT_MIN_WARM    80.0f

// --- Helper Functions (Private) ---

/**
 * Clamps a value between min and max.
 */
static inline float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static float get_material_k1(vtt_material_t mat){
    switch (mat)
    {
    case VTT_MAT_SENSITIVE: return 1.0f;
    case VTT_MAT_MEDIUM_RESISTANT: return 0.3f;
    case VTT_MAT_RESISTANT: return 0.1f;
    
    default: return 1.0f;
    
    }
}

static void calculate_rh_critical(vtt_state_t *ctx, float T){
    if (T > 20){
        ctx->rh_crit = RH_CRIT_MIN_WARM + ctx->rh_mat;
    } else {
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
    switch (mat)
    {
    case VTT_MAT_SENSITIVE:
        ctx-> surface_quality = 0.0f;
        ctx-> wood_species = 0.0f;
        ctx-> rh_mat = 0.0f;
        break;

    case VTT_MAT_MEDIUM_RESISTANT:
        ctx-> surface_quality = 1.0f;
        ctx-> wood_species = 1.0f;
        ctx-> rh_mat = 3.0f;
        break;

    case VTT_MAT_RESISTANT:
        ctx-> surface_quality = 1.0f;
        ctx-> wood_species = 1.0f;
        ctx-> rh_mat = 6.0f;
        break;

    default: //setting it as the worst case scenerio
        ctx-> surface_quality = 0.0f;
        ctx-> wood_species = 0.0f;
        ctx-> rh_mat = 0.0f;
        break;
    }
}

void vtt_update(vtt_state_t *ctx, float temp_c, float rh_percent, float time_step_hours){

    // I think might be unnecessary but good to have some safety.
    float safe_t = clampf(temp_c, 0.1f, 60.0f); 
    float safe_rh = clampf(rh_percent, 1.0f, 100.0f);

    calculate_rh_critical(ctx,safe_t);


    if (safe_rh > ctx->rh_crit){
        // growth phase, wet conditions
        ctx -> time_wet_hours += time_step_hours;
        ctx -> time_dry_hours = 0; 
        ctx -> condition = 1; //Growing Conditions 

        // 1. Calculate Maximum Possible Mold Index

        float m_max_calc = 6.0f * (safe_rh - ctx->rh_crit) / (100.0f - ctx->rh_crit);
        ctx -> max_possible_index = clampf(m_max_calc, 0.0f, 6.0f);

        // 2. Calculate Base Growth Speed

        float exponent = (-0.68f * logf(safe_t)) - (13.9f * logf(safe_rh)) + (0.14f * ctx->wood_species) - (0.33f * ctx->surface_quality) + 66.02f;
        float base_growth_rate = 1.0f / (7.0f * expf(exponent));

        // 2.1. Get Intensity of the material, impact: Slow or fast growth.
        float k1 = get_material_k1(ctx -> material);
        float dist_to_max = ctx->mold_index - ctx->max_possible_index;

        // 2.2. Calculate Saturation Factor
        float k2 = fmaxf(1.0f - expf(2.3f * dist_to_max), 0.0f); //might need clamping

        // 3. Calculate Mold Index
        float dM = k1 * k2 * base_growth_rate * time_step_hours;
        ctx->mold_index += dM;
        ctx->last_growth_rate = dM / time_step_hours;
    } else {
        // decline phase, dry conditions
        ctx -> time_dry_hours += time_step_hours;
        ctx -> time_wet_hours = 0;
        ctx -> condition = 0; //Decline Conditions 

        float decline_rate = 0.0f;

        // 1. Set Decline Rate.
        // TODO: Decide Decline Rates - Temporary decline rates, these are not final - will have to further research to get the best rates.
        if (ctx->time_dry_hours <= 6.0f){
            decline_rate = -0.032f;
        } else if (ctx->time_dry_hours <= 24.0f) {
            decline_rate = 0;
        } else {
            decline_rate = -0.016f;
        }

        // 2. Calculate Mold Index.
        float dM = decline_rate * time_step_hours;
        ctx -> mold_index += dM;
        ctx -> last_growth_rate = dM / time_step_hours; 
    }

    ctx -> mold_index = clampf(ctx->mold_index, MIN_INDEX_CAP, MAX_INDEX_CAP);
}


vtt_risk_level_t vtt_get_risk_level(vtt_state_t *ctx){
    if (ctx->mold_index < 1.0f) return MOLD_RISK_CLEAN;
    if (ctx->mold_index < 3.0f) return MOLD_RISK_WARNING;
    if (ctx->mold_index < 4.0f) return MOLD_RISK_ALERT;
    return MOLD_RISK_CRITICAL;
}