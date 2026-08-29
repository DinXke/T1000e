#include <cstdio>
#include <cstdlib>
#include "BatteryCurve.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); failures++; } } while (0)

static int linear_percent(int mv) {           // what the stock/app formula does
  int p = (mv - 3000) * 100 / (4200 - 3000);
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  return p;
}

int main() {
  // 1. endpoints
  CHECK(battery_percent_from_mv(0) == 0, "0mV must read as 0 (no reading)");
  CHECK(battery_percent_from_mv(4200) == 100, "4200mV -> 100, got %u", battery_percent_from_mv(4200));
  CHECK(battery_percent_from_mv(4500) == 100, "over-voltage clamps to 100");
  CHECK(battery_percent_from_mv(BATT_CURVE_EMPTY_MV) == 0, "empty anchor -> 0, got %u", battery_percent_from_mv(BATT_CURVE_EMPTY_MV));
  CHECK(battery_percent_from_mv(3000) == 0, "below empty -> 0");

  // 2. monotonic and never out of range across the whole domain
  int prev = -1;
  for (int mv = 2500; mv <= 4400; mv++) {
    int p = battery_percent_from_mv((uint16_t)mv);
    CHECK(p >= 0 && p <= 100, "%dmV out of range: %d", mv, p);
    CHECK(p >= prev, "not monotonic at %dmV: %d after %d", mv, p, prev);
    prev = p;
  }

  // 3. the actual complaint: 3.7V must NOT read as "more than half"
  int p37 = battery_percent_from_mv(3700);
  CHECK(linear_percent(3700) == 58, "sanity: stock map says 58%% at 3.7V, got %d", linear_percent(3700));
  CHECK(p37 < 25, "3.7V should be low, curve says %d%%", p37);

  // 4. spoofing round-trip: an app applying the linear map to battery_display_mv()
  //    must recover the curve percentage exactly, across the whole domain.
  for (int mv = 3000; mv <= 4300; mv++) {
    uint8_t truth = battery_percent_from_mv((uint16_t)mv);
    uint16_t spoof = battery_display_mv((uint16_t)mv);
    int app = linear_percent(spoof);
    CHECK(app == truth, "round-trip at %dmV: curve=%u spoof=%umV app=%d", mv, truth, spoof, app);
  }
  CHECK(battery_display_mv(0) == 0, "0mV must pass through unspoofed");

  // 5. spoofed values stay inside the range apps expect
  for (int mv = 2500; mv <= 4400; mv++) {
    uint16_t s = battery_display_mv((uint16_t)mv);
    CHECK(s >= BATT_LINEAR_MIN_MV && s <= BATT_LINEAR_MAX_MV, "spoof %u out of band at %dmV", s, mv);
  }


  // ---- low-battery watchdog -------------------------------------------------
  {
    const uint16_t WARN = 3400, STOP = 3200;
    const uint8_t  N = 3;
    batt_watch_state_t st = {0, 0};

    // healthy cell: nothing happens, ever
    for (int i = 0; i < 20; i++)
      CHECK(battery_watch_step(&st, 3900, 0, WARN, STOP, N) == BATT_ACTION_NONE, "healthy cell must be quiet");

    // crossing the warning threshold warns exactly once
    CHECK(battery_watch_step(&st, 3390, 0, WARN, STOP, N) == BATT_ACTION_WARN, "must warn on first low reading");
    for (int i = 0; i < 10; i++)
      CHECK(battery_watch_step(&st, 3390, 0, WARN, STOP, N) == BATT_ACTION_NONE, "must not warn repeatedly");

    // a single dip below the stop threshold must NOT shut down (TX sag)
    CHECK(battery_watch_step(&st, 3100, 0, WARN, STOP, N) == BATT_ACTION_NONE, "1 low reading must not shut down");
    CHECK(battery_watch_step(&st, 3390, 0, WARN, STOP, N) == BATT_ACTION_NONE, "recovery resets the streak");
    CHECK(battery_watch_step(&st, 3100, 0, WARN, STOP, N) == BATT_ACTION_NONE, "streak restarts after recovery");
    CHECK(battery_watch_step(&st, 3100, 0, WARN, STOP, N) == BATT_ACTION_NONE, "2 of 3");
    CHECK(battery_watch_step(&st, 3100, 0, WARN, STOP, N) == BATT_ACTION_SHUTDOWN, "3 consecutive lows must shut down");

    // external power suppresses everything and re-arms the warning
    batt_watch_state_t st2 = {0, 0};
    CHECK(battery_watch_step(&st2, 3000, 1, WARN, STOP, N) == BATT_ACTION_NONE, "no action while charging");
    CHECK(battery_watch_step(&st2, 3000, 1, WARN, STOP, N) == BATT_ACTION_NONE, "still no action while charging");
    CHECK(battery_watch_step(&st2, 3000, 1, WARN, STOP, N) == BATT_ACTION_NONE, "charging never accumulates a streak");
    CHECK(st2.low_streak == 0, "streak must stay clear while charging");
    CHECK(battery_watch_step(&st2, 3390, 0, WARN, STOP, N) == BATT_ACTION_WARN, "warning re-arms after a charge");

    // a missing reading is not a flat battery
    batt_watch_state_t st3 = {0, 0};
    for (int i = 0; i < 10; i++)
      CHECK(battery_watch_step(&st3, 0, 0, WARN, STOP, N) == BATT_ACTION_NONE, "0mV must never trigger shutdown");

    // thresholds of 0 disable each half independently
    batt_watch_state_t st4 = {0, 0};
    for (int i = 0; i < 10; i++)
      CHECK(battery_watch_step(&st4, 2000, 0, 0, 0, N) == BATT_ACTION_NONE, "disabled thresholds must do nothing");
    batt_watch_state_t st5 = {0, 0};
    CHECK(battery_watch_step(&st5, 2000, 0, WARN, 0, N) == BATT_ACTION_WARN, "warn-only still warns");
    for (int i = 0; i < 10; i++)
      CHECK(battery_watch_step(&st5, 2000, 0, WARN, 0, N) == BATT_ACTION_NONE, "warn-only never shuts down");

    // shutdown wins over warning when a cell arrives already flat
    batt_watch_state_t st6 = {0, 0};
    CHECK(battery_watch_step(&st6, 3000, 0, WARN, STOP, 1) == BATT_ACTION_SHUTDOWN, "stop_readings=1 acts at once");
    batt_watch_state_t st7 = {0, 0};
    CHECK(battery_watch_step(&st7, 3000, 0, WARN, STOP, 0) == BATT_ACTION_SHUTDOWN, "stop_readings=0 means immediately");

    // the streak counter must not wrap on a long-flat cell
    batt_watch_state_t st8 = {0, 0};
    for (int i = 0; i < 1000; i++) battery_watch_step(&st8, 3000, 0, WARN, STOP, N);
    CHECK(st8.low_streak >= N, "streak must saturate, not wrap to 0");
  }

  printf("\n  mV   curve%%  stock%%  spoofed mV\n");
  for (int mv = 4200; mv >= 3300; mv -= 50) {
    printf("  %4d   %3u     %3d     %4u\n", mv, battery_percent_from_mv(mv), linear_percent(mv), battery_display_mv(mv));
  }

  printf("\n%s\n", failures ? "TESTS FAILED" : "all battery curve tests passed");
  return failures ? 1 : 0;
}
