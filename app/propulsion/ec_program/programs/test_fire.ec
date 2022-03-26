# Test Procedure

# define some macros
# matches a single word
OX = 100 
FUEL = 101
PURGE_OX = 102     # solenoid 3
PURGE_FUEL = 303     # solenoid 3
IGNITER = 200   # igniter 1
RED = 300
YELLOW = 301
GREEN = 302

ON = 1          # on state
OFF = 0         # off state

# Run through a basic program
# Format is [absolute time in ms (time referenced from countdown clock)] [control] [state]

# TODO make this the buzzer
0 YELLOW ON
500 YELLOW OFF
500 RED ON

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
