#!/usr/bin/python3

import os
import sys

if len(sys.argv) != 2:
    print("usage: ./change_mode.py [disabled | test | cold | hot]")
    exit(-1)

mode = sys.argv[1]

cmd = "../ec_cmd/ec_cmd "
def exec_cmd(control, state):
    os.system(cmd + control + " " + state)

if mode == "disabled":
    exec_cmd("300", "0")
    exec_cmd("301", "0")
    exec_cmd("302", "1")
    exec_cmd("900", "0")
elif mode == "test":
    exec_cmd("300", "1")
    exec_cmd("301", "1")
    exec_cmd("302", "1")
    exec_cmd("900", "69")
elif mode == "cold":
    exec_cmd("300", "0")
    exec_cmd("301", "1")
    exec_cmd("302", "0")
    exec_cmd("900", "1")
elif mode == "hot":
    exec_cmd("300", "1")
    exec_cmd("301", "0")
    exec_cmd("302", "0")
    exec_cmd("900", "99")
else:
    print("invalid command")
