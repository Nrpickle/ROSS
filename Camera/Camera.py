#!/usr/bin/python
import os
import time
from sys import argv


if len(argv) == 2 and argv[1] == 'H':
	os.system("fswebcam -r 1920x1080 --no-banner /home/pi/Pictures/HighRes/1080p-%s.jpg" %time.strftime("%Y-%m-%dT%H%M%S"))
	
elif len(argv) == 1:
	os.system("fswebcam -r 320x180 /home/pi/Pictures/LowRes/200p-%s.jpg" %time.strftime("%Y-%m-%dT%H%M%S"))

else:
        print "ERROR: Improper arguments."


