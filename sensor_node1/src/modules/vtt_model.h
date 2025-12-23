#ifndef vtt_model
#define vtt_model
#include <stdbool.h>

// --- Configuration Types ---

typedef enum {

    VTT_MAT_SENSITIVE = 0, // Class A: Pine sapwood, Drywall, Wallpaper (k1=0.578)
    VTT_MAT_MEDIUM_RESISTANT, // Class B: Concrete, Cement, Aerated concrete (k1=0.072)
    VTT_MAT_RESISTANT // Class C: Glass, Metal, Tiles (k1=0.033)

} vtt_material_t;

typedef enum {

    MOLD_RISK_CLEAN = 0, // All good, no mold growth
    MOLD_RISK_WARNING, // Microscopic growth
    MOLD_RISK_ALERT, // Visual growth imminent (Index > 3)
    MOLD_RISK_CRITICAL // Heavy Growth

} vtt_risk_level_t;


// --- State Object ---
// "Context" - Holds the memory for one specific room/sensor
typedef struct {
    //Settings per room, based on the room
    vtt_material_t material;
    float surface_quality;
    float wood_species;
    bool growing_condition;
    float rh_mat;

    //Dynamic States, change after every step.
    float rh_crit;
    float mold_index;
    float time_wet_hours;
    float time_dry_hours;

    float last_growth_rate;
    float max_possible_index;


} vtt_state_t;

// --- Public API ---


/**
 * Initialize the VTT context. Must be called once before use.
 */
void vtt_init (vtt_state_t *ctx, vtt_material_t mat);

/**
 * Update the model. Call this periodically (e.g. every 15 mins).
 * time_step_hours: 0.25 for 15 mins.
 */
void vtt_update(vtt_state_t *ctx, float temp_c, float rh_percent, float time_step_hours);

/**
 * Get the simplified user-facing risk level.
 */
vtt_risk_level_t vtt_get_risk_level(vtt_state_t *ctx);

#endif