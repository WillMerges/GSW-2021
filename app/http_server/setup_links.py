#!/usr/bin/python3

# Create symlinks for all applications that should be executable by the HTTP server
# Places links in the 'links' directory
# Reads entries from the 'apps' file
import os

home = os.getenv("GSW_HOME")
if home is None:
    print("GSW_HOME not set, run '. setenv' first")
    exit(-1)


f = open("apps", "r")
lines = f.readlines()

for line in lines:
    # blank line
    if line == "\n":
        continue

    tokens = line.split(" ")

    # comment
    if tokens[0] == "#":
        continue

    if len(tokens) != 2:
        print("Invalid line: " + line);

    cmd = "ln -s " + home + "/"
    cmd = cmd + tokens[1][0:-1] # ignore the newline
    cmd = cmd + " links/" + tokens[0]

    os.system(cmd)

f.close()
