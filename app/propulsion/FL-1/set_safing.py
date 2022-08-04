#!/usr/bin/python3

import os
import sys

if len(sys.argv) != 2:
    print("usage: ./set_safing.py [high | low]")
    exit(-1)

safing = sys.argv[1]

gsw = os.getenv("GSW_HOME")
if gsw is None:
    print("GSW_HOME not set!")
    quit()

cmd = gsw + "/app/propulsion/ec_cmd/ec_cmd "

def exec_cmd(control, state):
    os.system(cmd + control + " " + state)


if safing == "high":
    exec_cmd("901", "1")
elif safing == "low":
    exec_cmd("901", "0")
else:
    print("invalid command")
