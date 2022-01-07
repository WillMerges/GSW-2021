#!/usr/bin/python3

import ctypes
import os

try:
    gsw_home = os.environ['GSW_HOME']
except:
    print("Must set GSW environment first!")
    exit()

lib = ctypes.cdll.LoadLibrary(gsw_home+"/lib/bin/libpython.so")

class TelemetryViewer(object):
    def __init__(self):
        lib.TelemetryViewer_new.restype = ctypes.c_void_p

        lib.TelemetryViewer_init0.argtypes = [ctypes.c_void_p]
        lib.TelemetryViewer_init0.restype = ctypes.c_int

        lib.TelemetryViewer_init1.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p]
        lib.TelemetryViewer_init1.restype = ctypes.c_int

        lib.TelemetryViewer_remove_all.argtypes = [ctypes.c_void_p]

        lib.TelemetryViewer_add_all.argtypes = [ctypes.c_void_p]
        lib.TelemetryViewer_add_all.restype = ctypes.c_int

        lib.TelemetryViewer_update.argtypes = [ctypes.c_void_p]
        lib.TelemetryViewer_update.restype = ctypes.c_int

        lib.TelemetryViewer_get.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p]
        lib.TelemetryViewer_get.restype = ctypes.c_char_p

        self.obj = lib.TelemetryViewer_new()

        # set the update mode to be blocking
        lib.TelemetryViewer_set_update_blocking(self.obj)

    def init(self):
        return lib.TelemetryViewer_init0(self.obj)

    def remove_all(self):
        return lib.TelemetryViewer_remove_all(self.obj)

    def add_all(self):
        return lib.TelemetryViewer_add_all(self.obj)

    def update(self):
        return lib.TelemetryViewer_update(self.obj)

    def get(self, measurement):
        return lib.TelemetryViewer_get(self.obj, measurement)


if __name__ == "__main__":
    tm = TelemetryViewer()
    if tm.init() != 0:
        print("TelemetryViewer failed to init")
        quit()

    if tm.add_all() != 0:
        print("Failed to add measurements")
        quit()

    while 1:
        tm.update()
        print(tm.get("UPTIME"))
