gst-launch-1.0 v4l2src device="/dev/video0" ! videoconvert ! clockoverlay ! x264enc tune=zerolatency ! mpegtsmux ! hlssink playlist-root=http://192.168.0.11:8080 location=video-segments/segment_%05d.ts target-duration=5 max-files=5 

