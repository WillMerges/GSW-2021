# Simple Engine Controller Test Program
# Runs over 10 seconds

# define some macros
# matches a single word
FUEL = 100      # solenoid 1
OX = 101        # solenoid 2
PURGE = 102     # solenoid 3
IGNITER = 200   # igniter 1
ON = 1          # on state
OFF = 0         # off state

# Run through a basic program
# Format is [absolute time in ms (from start of program)] [control] [state]
0 PURGE ON
3000 OX ON
3500 PURGE OFF
5000 FUEL ON
6000 FUEL OFF
7000 OX OFF
7000 PURGE ON
10000 PURGE OFF

end
