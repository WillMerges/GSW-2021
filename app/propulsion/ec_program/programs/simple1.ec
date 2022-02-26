# Simple Engine Controller Test Program
# Runs over 22 seconds

# define some macros
# matches a single word
FUEL = 100      # solenoid 1
OX = 101        # solenoid 2
PURGE = 102     # solenoid 3
IGNITER = 200   # igniter 1
ON = 1          # on state
OFF = 0         # off state

# Run through a basic program
# Format is [absolute time in ms (time referenced from countdown clock)] [control] [state]

0 PURGE ON
0 MSG "purge on"

3000 OX ON
3000 MSG "ox on"

3000 PURGE OFF
3000 MSG "purge off"

5000 FUEL ON
5000 MSG "fuel on"

19000 FUEL OFF
19000 MSG "fuel off"

20000 OX OFF
20000 MSG "ox off"

20000 PURGE ON
20000 MSG "purge on"

22000 PURGE OFF
22000 MSG "purge off"

end
