#pragma once

#include <stdint.h>

/*
 * Single-cell LiPo state-of-charge helpers.
 *
 * MeshCore reports raw battery millivolts and lets the companion app turn that
 * into a percentage. Every consumer that does so (the on-device battery icon in
 * ui-orig, and the companion apps) uses the same linear map:
 *
 *     percent = (mv - 3000) * 100 / (4200 - 3000)
 *
 * That map does not describe a LiPo. A healthy cell spends the bulk of its
 * discharge between 3.95V and 3.70V, which the linear map compresses into
 * 79%..58%; it then falls off a cliff below 3.6V, which the linear map still
 * reports as "half full". The user-visible symptom is a gauge that sits around
 * half for days and then drops to nothing without warning.
 *
 * battery_percent_from_mv() replaces that with a piecewise-linear discharge
 * curve, sampled densely below 3.9V where the real curve is steep.
 *
 * battery_display_mv() is the compatibility shim ("spoofing"): it converts a
 * true reading into the millivolt value that makes an *unmodified* companion
 * app -- one that still applies the linear map -- display the correct
 * percentage. See docs/battery.md for the trade-off this makes.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Voltage the curve calls empty. Keep this equal to AUTO_SHUTDOWN_MILLIVOLTS
   so that "0%" and "the device powers itself down" are the same event. */
#ifndef BATT_CURVE_EMPTY_MV
  #ifdef AUTO_SHUTDOWN_MILLIVOLTS
    #define BATT_CURVE_EMPTY_MV  AUTO_SHUTDOWN_MILLIVOLTS
  #else
    #define BATT_CURVE_EMPTY_MV  3400
  #endif
#endif

/* Per-unit calibration, in millivolts, added to every reading before the curve
   is applied. Positive values compensate for a divider/ADC that reads low. */
#ifndef BATT_CURVE_OFFSET_MV
  #define BATT_CURVE_OFFSET_MV  0
#endif

/* Use the discharge curve for this build's on-device battery indicator
   instead of the stock linear map. Off by default: ui-orig is shared by ten
   board variants, and a change to how they all report charge is not this
   patch's to make. */
#ifndef BATTERY_USE_CURVE
  #define BATTERY_USE_CURVE  0
#endif

/* Report battery_display_mv() instead of the true reading in the companion
   protocol's PACKET_BATTERY frame, so an app that still applies the linear map
   shows the right percentage. Off unless a variant opts in. */
#ifndef BATTERY_PERCENT_SPOOF
  #define BATTERY_PERCENT_SPOOF  0
#endif

/* Also report the spoofed value in the *self* telemetry the connected app
   renders, so the app's telemetry page and its battery gauge agree. Telemetry
   answered to remote nodes over the mesh is never spoofed. */
#ifndef BATTERY_TELEMETRY_SPOOF
  #define BATTERY_TELEMETRY_SPOOF  0
#endif

/* Append the true state of charge and the true millivolts to PACKET_BATTERY,
   past the 11 documented bytes, for apps that know to look. */
#ifndef BATTERY_EXT_FRAME
  #define BATTERY_EXT_FRAME  0
#endif

/* The linear map the unmodified apps use; battery_display_mv() targets it. */
#ifndef BATT_LINEAR_MIN_MV
  #define BATT_LINEAR_MIN_MV  3000
#endif
#ifndef BATT_LINEAR_MAX_MV
  #define BATT_LINEAR_MAX_MV  4200
#endif

/* State of charge, 0..100, for a true (unspoofed) reading in millivolts.
   Returns 0 for mv == 0, which every caller uses to mean "no reading". */
uint8_t battery_percent_from_mv(uint16_t mv);

/* Percentage -> the millivolt value the linear map decodes back to it. */
uint16_t battery_percent_to_linear_mv(uint8_t percent);

/* True millivolts -> millivolts to report to a companion app that still
   applies the linear map. Passes 0 through unchanged. */
uint16_t battery_display_mv(uint16_t true_mv);

/* ---------------------------------------------------------------------------
   Low-battery decision logic.

   Kept here, as a pure function over an explicit state struct, so the part that
   can switch a user's device off is testable on a host instead of only on a
   flat battery. UITask owns the state and the timing; this owns the decision.
   --------------------------------------------------------------------------- */

typedef struct {
  uint8_t low_streak;   /* consecutive readings below the shutdown threshold */
  uint8_t warned;       /* warning already issued for this discharge */
} batt_watch_state_t;

typedef enum {
  BATT_ACTION_NONE = 0,
  BATT_ACTION_WARN,
  BATT_ACTION_SHUTDOWN
} batt_action_t;

/* Feed one reading. Pass warn_mv or stop_mv as 0 to disable that half.
   stop_readings is how many consecutive sub-threshold readings are required
   before BATT_ACTION_SHUTDOWN is returned (0 and 1 both mean "immediately").
   Returns at most one action per call; shutdown takes precedence. */
batt_action_t battery_watch_step(batt_watch_state_t* st,
                                 uint16_t mv,
                                 int external_powered,
                                 uint16_t warn_mv,
                                 uint16_t stop_mv,
                                 uint8_t stop_readings);

#ifdef __cplusplus
}
#endif
