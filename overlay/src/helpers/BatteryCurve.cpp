#include "BatteryCurve.h"

/*
 * Resting-voltage discharge curve for a single LiPo cell, from full to the
 * shutdown threshold. Points are dense between 3.85V and 3.69V because that
 * band is where a LiPo actually spends most of its life -- it is the region the
 * old linear map flattened into "about half".
 *
 * The last entry's voltage is BATT_CURVE_EMPTY_MV rather than a fixed constant,
 * so raising or lowering the auto-shutdown threshold moves the 0% anchor with
 * it and the gauge still reaches 0 exactly when the device powers down.
 */
typedef struct {
  uint16_t mv;
  uint8_t  pct;
} batt_point_t;

static const batt_point_t CURVE[] = {
  { 4200, 100 },
  { 4150,  95 },
  { 4110,  90 },
  { 4080,  85 },
  { 4020,  80 },
  { 3980,  75 },
  { 3950,  70 },
  { 3910,  65 },
  { 3870,  60 },
  { 3850,  55 },
  { 3840,  50 },
  { 3820,  45 },
  { 3800,  40 },
  { 3790,  35 },
  { 3770,  30 },
  { 3750,  25 },
  { 3730,  20 },
  { 3710,  15 },
  { 3690,  10 },
  { 3610,   5 },
  { BATT_CURVE_EMPTY_MV, 0 },
};

#define CURVE_POINTS  (sizeof(CURVE) / sizeof(CURVE[0]))

uint8_t battery_percent_from_mv(uint16_t mv) {
  if (mv == 0) return 0;   // no reading available

  int32_t v = (int32_t)mv + (BATT_CURVE_OFFSET_MV);
  if (v <= 0) return 0;

  if (v >= (int32_t)CURVE[0].mv) return 100;

  for (unsigned i = 1; i < CURVE_POINTS; i++) {
    if (v >= (int32_t)CURVE[i].mv) {
      // v sits between CURVE[i] (lower) and CURVE[i-1] (higher)
      uint16_t span_mv  = CURVE[i - 1].mv  - CURVE[i].mv;
      uint8_t  span_pct = CURVE[i - 1].pct - CURVE[i].pct;
      if (span_mv == 0) return CURVE[i].pct;
      return (uint8_t)(CURVE[i].pct +
             ((uint32_t)(v - CURVE[i].mv) * span_pct) / span_mv);
    }
  }
  return 0;   // at or below empty
}

uint16_t battery_percent_to_linear_mv(uint8_t percent) {
  if (percent > 100) percent = 100;
  return (uint16_t)(BATT_LINEAR_MIN_MV +
         ((uint32_t)percent * (BATT_LINEAR_MAX_MV - BATT_LINEAR_MIN_MV)) / 100);
}

uint16_t battery_display_mv(uint16_t true_mv) {
  if (true_mv == 0) return 0;   // keep "no reading" distinguishable
  return battery_percent_to_linear_mv(battery_percent_from_mv(true_mv));
}

batt_action_t battery_watch_step(batt_watch_state_t* st,
                                 uint16_t mv,
                                 int external_powered,
                                 uint16_t warn_mv,
                                 uint16_t stop_mv,
                                 uint8_t stop_readings) {
  if (st == 0) return BATT_ACTION_NONE;

  /* No reading at all, or running on external power: nothing to protect
     against. Coming off the charger re-arms the warning, so the next
     discharge gets its own warning instead of one per lifetime. */
  if (mv == 0 || external_powered) {
    st->low_streak = 0;
    st->warned = 0;
    return BATT_ACTION_NONE;
  }

  if (stop_mv != 0 && mv < stop_mv) {
    if (st->low_streak < 255) st->low_streak++;
    if (st->low_streak >= (stop_readings ? stop_readings : 1)) {
      return BATT_ACTION_SHUTDOWN;
    }
  } else {
    st->low_streak = 0;
  }

  if (warn_mv != 0 && mv < warn_mv && !st->warned) {
    st->warned = 1;
    return BATT_ACTION_WARN;
  }

  return BATT_ACTION_NONE;
}
