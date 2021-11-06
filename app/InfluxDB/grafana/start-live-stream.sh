sudo gst-launch-1.0 v4l2src device="/dev/video0" ! videoconvert ! clockoverlay ! x264enc tune=zerolatency ! mpegtsmux ! hlssink playlist-root=http://0.0.0.0:8080 location=segment_%05d.ts target-duration=5  

