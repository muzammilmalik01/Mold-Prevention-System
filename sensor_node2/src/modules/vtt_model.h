/**
 * @file vtt_model.h
 * @brief VTT Mold Prediction Model Interface
 * * * Implements the mathematical model developed by VTT (Technical Research Centre of Finland)
 * to predict mold growth on building materials.
 * * The model calculates a "Mold Index" (0 to 6) based on fluctuating 
 * Temperature and Relative Humidity conditions over time.
 * * * Key Features:
 * - Dynamic Critical RH calculation
 * - Growth/Decline phase detection
 * - Material sensitivity classes
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */

#ifndef vtt_model
#define vtt_model
#include <stdbool.h>

// --- Configuration Types ---

/**
 * @brief Material Sensitivity Classes (k1 factor).
 * Determines how quickly mold grows on a specific surface.
 */

typedef enum {
    /** * @brief Very Sensitive (k1=0.578)
     * Examples: Pine sapwood, untreated wood, paper, drywall. 
     */
    VTT_MAT_SENSITIVE = 0, 

    /** * @brief Medium Resistant (k1=0.072)
     * Examples: Spruce sapwood,Concrete, cement, aerated concrete, glued wood.
     */
    VTT_MAT_MEDIUM_RESISTANT, 

    /** * @brief Resistant (k1=0.033)
     * Examples: Glass, metal, tiles, high-quality plastics.
     */
    VTT_MAT_RESISTANT 

} vtt_material_t;

/**
 * @brief Simplified Risk Levels for User Alerts.
 * Maps the floating-point Mold Index (M) to actionable status codes.
 */
typedef enum {

    MOLD_RISK_CLEAN = 0,    /**< M < 1.0: No growth. Safe. */
    MOLD_RISK_WARNING,      /**< 1.0 <= M < 3.0: Microscopic growth. Inspect area. */
    MOLD_RISK_ALERT,        /**< 3.0 <= M < 4.0: Visual growth imminent. Action required. */
    MOLD_RISK_CRITICAL      /**< M >= 4.0: Heavy visual growth. Health hazard. */

} vtt_risk_level_t;


// --- State Object ---

/**
 * @brief VTT Model Context
 * Holds the persistent state and history required to solve the differential equations.
 * One instance of this struct is needed per sensor/room.
 */
typedef struct {
    // --- Static Configuration (Set at Init) ---   
    vtt_material_t material;    /**< Material class being monitored */
    float surface_quality;      /**< SQ Factor: 0 (rough) to 1 (smooth) */
    float wood_species;         /**< W Factor: 0 (pine) to 1 (spruce) */
    float rh_mat;               /**< Material specific offset for RH_crit */

    // --- Dynamic State (Updated every Step) ---
    bool growing_condition;     /**< True if currently in Growth Phase, False if Decline */
    float rh_crit;              /**< Calculated Critical Humidity Threshold (%) */
    float mold_index;           /**< Current Mold Index (0.0 to 6.0) */
    
    float time_wet_hours;       /**< Duration spent above RH_crit */
    float time_dry_hours;       /**< Duration spent below RH_crit */

    float last_growth_rate;     /**< Rate of change (dM/dt) from last step (debug/telemetry) */
    float max_possible_index;   /**< Theoretical max index for current RH conditions */


} vtt_state_t;

// --- Public API ---

/**
 * @brief Initialize the VTT context.
 * Resets the Mold Index to 0 and configures material parameters.
 * * @param ctx Pointer to the state object to initialize.
 * @param mat The type of material to simulate (e.g., VTT_MAT_SENSITIVE).
 */
void vtt_init (vtt_state_t *ctx, vtt_material_t mat);

/**
 * @brief Update the model with new sensor data.
 * Solves the differential equation for one time step.
 * * @param ctx Pointer to the state object.
 * @param temp_c Current Temperature (Celsius).
 * @param rh_percent Current Relative Humidity (%).
 * @param time_step_hours Time elapsed since last call (e.g., 0.25 for 15 mins).
 */
void vtt_update(vtt_state_t *ctx, float temp_c, float rh_percent, float time_step_hours);

/**
 * @brief Get the simplified user-facing risk level.
 * * @param ctx Pointer to the state object.
 * @return vtt_risk_level_t Enumerated risk status.
 */
vtt_risk_level_t vtt_get_risk_level(vtt_state_t *ctx);

#endif