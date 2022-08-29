#!/usr/bin/python3
"""
Simulate telemetry data by using KSP

Usage: ./sim [IP address of kRPC server]

@author: Will Merges
"""
import sys
import os
import time
import krpc
import struct
import socket

VESSEL_DATA_ADDR = ("224.0.0.1", 8000)
FLIGHT_DATA_ADDR = ("224.0.0.1", 8001)

#check args
if len(sys.argv) < 2:
    print("usage: ./sim [IP address of kRPC server]")
    quit()

# connect to kRPC
conn = krpc.connect(address=sys.argv[1])
vessel = conn.space_center.active_vessel
refframe = vessel.orbit.body.reference_frame

# create socket for sending telemetry over
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def poll_vessel_data(vessel, sock):
    # mission time
    met = vessel.met # double

    # create a string of all the current crew members
    crew_list = vessel.crew
    crew_str = ""
    for crew in crew_list:
        crew_str += crew.name + ", "
    
    # pad the string to a bytes object with a max of 25 characters
    l = list(crew_str[:-2])
    
    if len(l) >= 25:
        # truncate the list
        l[-1] = '\0'
    else:
        # pad the list
        while len(l) < 25:
            l.append('\0')

    crew = [ord(c) for c in ''.join(l)]
    
    # mass
    mass = vessel.mass # float

    # thrust
    thrust = vessel.thrust # float

    # pack into a struct in network order
    data = struct.pack("!dff25s", met, mass, thrust, bytes(crew))

    sock.sendto(data, VESSEL_DATA_ADDR)

def poll_flight_data(flight, sock):
    # G force
    gs = flight.g_force # float

    # mean altitude (meters)
    alt = flight.mean_altitude # float

    # elevation (meters)
    elev = flight.elevation # float

    # latitude
    lat = flight.latitude # double

    # longitude
    lon = flight.longitude # double

    # speed (meters / sec)
    speed = flight.speed # double

    # pitch (degrees)
    pitch = flight.pitch # float

    # heading (degrees)
    heading = flight.heading # float

    # roll (degrees)
    roll = flight.roll # float

    # air temperature (Kelvin)
    air_temp = flight.total_air_temperature # float

    # pack into a struct in network order
    data = struct.pack("!fffdddffff", gs, alt, elev, lat, lon, speed, pitch, heading, roll, air_temp)

    sock.sendto(data, FLIGHT_DATA_ADDR)

# poll for game data
while True:
    poll_vessel_data(vessel, sock)
    
    # poll for the newest flight data
    flight = vessel.flight(refframe)
    poll_flight_data(flight, sock)
