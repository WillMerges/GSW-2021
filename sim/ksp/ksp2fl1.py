#!/usr/bin/python3
"""
Control FL-1 system based on telemetry received
from Kerbal Space Program

Uses the kRPC mod / Python API

Usage: ./ksp2fl1 [IP address of kRPC server]

@author: Will Merges
"""
import sys
import os
import time
from threading import Lock
import krpc

if len(sys.argv) < 2:
    print("usage: ./ksp2fl1 [IP address of kRPC server]")
    quit()


"""
gsw_dir = os.getenv("GSW_HOME")
if gsw_dir is None:
    print("GSW_HOME variable not set!")
    quit()

# directory with linked commands/scripts
cmd_dir = gsw_dir + "/app/http_server/links"
"""

# connect to kRPC
conn = krpc.connect(address=sys.argv[1])
vessel = conn.space_center.active_vessel

# state variables
launched = False

# when the thrust changes
def burn():
    print("hot")
    os.system("../propulsion/FL-1/change_mode.py hot")
    os.system("../propulsion/ec_cmd/ec_cmd 100 1") # ox on
    os.system("../propulsion/ec_cmd/ec_cmd 101 1") # fuel on

    launched = True

# no thrust, but throttled up
def cold():
    print("cold")
    os.system("../propulsion/FL-1/change_mode.py cold")
    os.system("../propulsion/ec_cmd/ec_cmd 100 0") # ox off
    os.system("../propulsion/ec_cmd/ec_cmd 101 0") # fuel off
    
# no thrust, no throttle
def disable():
    print("disable")
    os.system("../propulsion/FL-1/change_mode.py disabled")
    os.system("../propulsion/ec_cmd/ec_cmd 100 0") # ox off
    os.system("../propulsion/ec_cmd/ec_cmd 101 0") # fuel off
    
    if not launched:
        return

    # run purge
    os.system("../propulsion/ec_cmd/ec_cmd 102 1") # ox purge on
    os.system("../propulsion/ec_cmd/ec_cmd 103 1") # fuel purge on
    time.sleep(2)
    os.system("../propulsion/ec_cmd/ec_cmd 102 0") # ox purge off
    os.system("../propulsion/ec_cmd/ec_cmd 103 0") # fuel purge off

global burning
burning = False
thrust_mutex = Lock()
def thrust_callback(t):
    thrust_mutex.acquire()

    global burning
    print("thrust: " + str(t))
    if t >= 1 and not burning:
        # just started burning
        burn()
        burning = True
    elif t < 1 and burning:
        # just stopped burning
        cold()
        burning = False

    thrust_mutex.release()

global throttle_up
throttle_up = True
throttle_mutex = Lock()
def throttle_callback(t):
    throttle_mutex.acquire()

    global throttle_up
    print("throttle: " + str(t))
    if t <= 0.05 and throttle_up:
        disable()
        throttle_up = False
    elif t > 0.05 and not throttle_up:
        cold()
        throttle_up = True

    throttle_mutex.release()


thrust = conn.add_stream(getattr, vessel, "thrust")
throttle = conn.add_stream(getattr, vessel.control, "throttle")

thrust.add_callback(thrust_callback)
throttle.add_callback(throttle_callback)

thrust.start()
throttle.start()


# loop forever
while True:
    pass
