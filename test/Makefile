# Host-side tests for the battery curve and the low-battery watchdog.
# No hardware, no toolchain: just `make` (or `make run`).
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror -O2
# Keep the empty-anchor in step with the firmware build's shutdown threshold.
CXXFLAGS += -DAUTO_SHUTDOWN_MILLIVOLTS=3200 -I../overlay/src/helpers

run: tc
	./tc

tc: test_battery_curve.cpp ../overlay/src/helpers/BatteryCurve.cpp ../overlay/src/helpers/BatteryCurve.h
	$(CXX) $(CXXFLAGS) -o $@ test_battery_curve.cpp ../overlay/src/helpers/BatteryCurve.cpp

clean:
	rm -f tc
.PHONY: run clean
