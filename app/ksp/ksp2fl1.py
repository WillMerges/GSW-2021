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
import krpc

if len(sys.argv) < 2:
    print("usage: ./ksp2fl1 [IP address of kRPC server]")
    quit()


gsw_dir = os.getenv("GSW_HOME")
if gsw_dir is None:
    print("GSW_HOME variable not set!")
    quit()

# directory with linked commands/scripts
cmd_dir = gsw_dir + "/app/http_server/links"

# connect to kRPC
conn = krpc.connect(address=sys.argv[1])
vessel = conn.space_center.active_vessel

# state variables
launched = False

# when the thrust changes
def burn():
    os.system(cmd_dir + "/change_mode hot")
    os.system(cmd_dir + "/ec_cmd 100 1") # ox on
    os.system(cmd_dir + "/ec_cmd 101 1") # fuel on

    launched = True

# no thrust, but throttled up
def cold():
    os.system(cmd_dir + "/change_mode cold")
    os.system(cmd_dir + "/ec_cmd 100 1") # ox on
    os.system(cmd_dir + "/ec_cmd 101 1") # fuel on
    
# no thrust, no throttle
def disable():
    os.system(cmd_dir + "/change_mode disabled")
    os.system(cmd_dir + "/ec_cmd 100 0") # fuel on
    os.system(cmd_dir + "/ec_cmd 101 0") # ox on
    
    if not launched:
        return

    # run purge
    os.system(cmd_dir + "/ec_cmd 102 1") # ox purge on
    os.system(cmd_dir + "/ec_cmd 103 1") # fuel purge on
    sleep(2)
    os.system(cmd_dir + "/ec_cmd 102 0") # ox purge on
    os.system(cmd_dir + "/ec_cmd 103 0") # fuel purge on


burning = False
def thrust_callback(t):
    if t > 1 and not burning:
        # just started burning
        burn()
        burning = True
    elif burning:
        # just stopped burning
        cold()
        burning = False

throttle_up = False
def throttle_callback(t):
    if t < 0.05 and throttle_up:
        disable()
        throttle_up = False
    elif not throttle_up:
        cold()
        throttle_up = True


thrust = conn.add_stream(getattr, vessel, "thrust")
throttle = conn.add_stream(getattr, vessel.control, "throttle")

thrust.add_callback(thrust_callback)
throttle.add_callback(throttle_callback)

thrust.start()
throttle.start()


# loop forever
while True:
    pass
