import os
import time
from sys import argv

HighRes = False
if argv[1] == 'H':
	HighRes = True
	
if HighRes == True:
	os.system("fswebcam -r 1920x1080 --no-banner --save /home/pi/Picures/HighRes/1080p-%s.jpg" %time.strftime("%Y-%m-%dT%H%M%S"))
	
else:
	os.system("fswebcam -r 320x200 --save /home/pi/Picures/LowRes/200p-%s.jpg" %time.strftime("%Y-%m-%dT%H%M%S"))