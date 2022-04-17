# Test Procedure
# FL-1 hot fire

# define some macros
# matches a single word
OX = 100
FUEL = 101
PURGE_OX = 102
PURGE_FUEL = 103
IGNITER = 200
RED = 300
YELLOW = 301
GREEN = 302
BUZZER = 303

ON = 1          # on state
OFF = 0         # off state

# Run through a basic program
# Format is [absolute time in ms (time referenced from countdown clock)] [control] [state]

# beep the buzzer 5 times, last buzz is longer
# starts beeping 5s before turning fuel on
# end of last buzz is when fuel valve opens
-5000 BUZZER ON
-4500 BUZZER OFF
-4000 BUZZER ON
-3500 BUZZER OFF
-3000 BUZZER ON
-2500 BUZZER OFF
-2000 BUZZER ON
-1500 BUZZER OFF
-1000 BUZZER ON
-500  BUZZER OFF
0 BUZZER OFF

0 FUEL ON
0 MSG "fuel on"

500 IGNITER ON
500 OX ON
500 MSG "ox/igniter on"

2000 IGNITER OFF
2000 MSG "igniter off"

5500 FUEL OFF
5500 MSG "fuel off"

6500 OX OFF
6500 MSG "ox off"

8500 PURGE_OX ON
8500 MSG "purge ox on"

10500 PURGE_OX OFF
10500 MSG "purge ox off"

11500 PURGE_FUEL ON
11500 MSG "purge fuel on"

13500 PURGE_FUEL OFF
13500 MSG "purge fuel off"

end
