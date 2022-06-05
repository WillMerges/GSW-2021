# Test Procedure
# 3-cat-2 hot fire

# macros
# relays
IGNITER = 100
LINE_CUTTER = 101
ON = 1
OFF = 0

# valves
NITROS_VALVE = 200
CO2_VALVE = 201
OPEN_NITROS = 40
CLOSE_NITOS = 135
OPEN_CO2 = 40
CLOSE_CO2 = 150


# procedure

-5000 MSG "starting hot fire sequence"

0 IGNITER ON
0 MSG "igniter on"

5000 LINE_CUTTER ON
5000 MSG "line cutter on"

10000 IGNITER OFF
10000 MSG "igniter off"

15000 LINE_CUTTER OFF
15000 MSG "line cutter off"

15000 MSG "hot fire sequence complete"
