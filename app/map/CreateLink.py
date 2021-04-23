#!/usr/bin/python3
"""
Creates a KML file named 'main.kml' containing a network link
usage:
    ./CreateLink.py <kml file to link to>
"""
import sys
import simplekml

# globals
KML_SAVE_NAME = "main.kml"

if len(sys.argv) != 2:
    print("usage: CreateLink.py <kml file to link to>")

kml = simplekml.Kml()
netlink = kml.newnetworklink(name=sys.argv[1])
netlink.link.href = sys.argv[1] 
netlink.link.viewrefreshmode = simplekml.ViewRefreshMode.onrequest
kml.save(KML_SAVE_NAME)
