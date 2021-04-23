#!/usr/bin/python3
"""
makes a kml document containing a gxtrack that takes input from stdin
kml documents can be read in by google earth
input must be "lat long altitude" as floats separated by spaces

@author: Will Merges
https://github.com/WillMerges/KML-GPS-Logger
"""
import simplekml
import sys

class GPSTrack(object):

    """
    name is name of doc without .kml extension
    """
    def __init__(self, name: str):
        self.name = name
        #self.times = []
        self.coords = None
        #self.size = 0 #number of points collected

        self.kml = simplekml.Kml(name=self.name+".kml", open=1)
        self.track = self.kml.newgxtrack(name = self.name)

        # self.track.stylemap.normalstyle.iconstyle.icon.href = 'resources/track-0.png'
        self.track.stylemap.normalstyle.iconstyle.icon.href = 'http://earth.google.com/images/kml-icons/track-0.png'
        # self.track.stylemap.normalstyle.linestyle.color = 'FF0000'
        self.track.stylemap.normalstyle.linestyle.width = 6
        # self.track.stylemap.highlightstyle.iconstyle.icon.href = 'resources/track-0.png'
        self.track.stylemap.highlightstyle.iconstyle.icon.href = 'http://earth.google.com/images/kml-icons/track-0.png'
        self.track.stylemap.highlightstyle.iconstyle.scale = 1.2
        self.track.stylemap.highlightstyle.linestyle.color = '99ffac59'
        self.track.stylemap.highlightstyle.linestyle.width = 8

    def update_track(self, line):
            coords = line.strip().split(" ")
            if len(coords) != 3:
                return

            #self.size = self.size + 1
            #self.times.append(coords[3])
            self.coords = [(float(coords[0]), float(coords[1]), float(coords[2]))]
            self.write_track()

    def write_track(self):
        #self.track.newwhen(self.times)
        self.track.newgxcoord(self.coords)
        self.kml.save(self.name + ".kml")

if __name__ == "__main__":
    gps = GPSTrack("track")
    for line in sys.stdin:
        gps.update_track(line)
