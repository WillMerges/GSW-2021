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

# beep the buzzer 5 times
# starts beeping 4.5s before turning fuel on
# end of last buzz is when fuel valve opens
-4000 BUZZER ON
-2500 BUZZER OFF
-3000 BUZZER ON
-2500 BUZZER OFF
-2000 BUZZER ON
-1500 BUZZER OFF
-1000 BUZZER ON
-500 BUZZER OFF
0 BUZZER ON
500 BUZZER OFF

500 FUEL ON
500 MSG "fuel on"

1000 IGNITER ON
1000 OX ON
1000 MSG "ox/igniter on"

2500 IGNITER OFF
2500 MSG "igniter off"

6000 FUEL OFF
6000 MSG "fuel off"

7000 OX OFF
7000 MSG "ox off"

9000 PURGE_OX ON
9000 MSG "purge ox on"

11000 PURGE_OX OFF
11000 MSG "purge ox off"

12000 PURGE_FUEL ON
12000 MSG "purge fuel on"

12000 PURGE_FUEL OFF
12000 MSG "purge fuel off"

end
