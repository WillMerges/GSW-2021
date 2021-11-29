#sudo gst-launch-1.0 v4l2src device="/dev/video0" ! videoconvert ! clockoverlay ! x264enc tune=zerolatency ! mpegtsmux ! hlssink playlist-root=http://0.0.0.0:8080 location=segment_%05d.ts target-duration=1
sudo gst-launch-1.0v4lsrc device="/dev/video0" ! videoconvert ! videoscale ! video/x-raw ! clockoverlay ! theoraenc ! oggmux ! tcpserversink host=0.0.0.0 port=8080
