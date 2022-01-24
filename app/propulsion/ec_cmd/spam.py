#!/usr/bin/python3

import os
from time import sleep

while 1:
    os.system("./ec_cmd 100 1")
    os.system("./ec_cmd 101 1")
    os.system("./ec_cmd 102 1")
    os.system("./ec_cmd 200 1")
    os.system("./ec_cmd 300 1")
    os.system("./ec_cmd 301 1")
    os.system("./ec_cmd 302 1")
    os.system("./ec_cmd 303 1")

    sleep(0.005)


    os.system("./ec_cmd 100 0")
    os.system("./ec_cmd 101 0")
    os.system("./ec_cmd 102 0")
    os.system("./ec_cmd 200 0")
    os.system("./ec_cmd 300 0")
    os.system("./ec_cmd 301 0")
    os.system("./ec_cmd 302 0")
    os.system("./ec_cmd 303 0")
