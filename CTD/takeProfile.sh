#!/bin/bash
size = wc -c /home/pi/CTD/LogFiles/combinedProfile.txt
./logger2wifidownloader -o $size -l all | /home/pi/CTD/ParseReader/a.out 6 > /home/pi/CTD/LogFiles/placeHolder.txt
 cat /home/pi/CTD/LogFiles/placeHolder.txt > /home/pi/CTD/LogFiles/Profile-$(date +%Y%m%d).txt 
 cat /home/pi/CTD/LogFiles/placeHolder.txt >> /home/pi/CTD/LogFiles/combinedProfile.txt
