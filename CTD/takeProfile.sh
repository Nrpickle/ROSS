#!/bin/bash
profileSize=$(wc --bytes < /home/pi/CTD/LogFiles/combinedProfile.txt)
profileSize=$((profileSize/1))
echo $profileSize
./logger2wifidownloader -offset=$profileSize -length=all | /home/pi/CTD/ParseReader/a.out 6 > /home/pi/CTD/LogFiles/placeHolder.txt
 cat /home/pi/CTD/LogFiles/placeHolder.txt > /home/pi/CTD/LogFiles/Profile-$(date +%Y%m%d%H%M%S).txt 
 cat /home/pi/CTD/LogFiles/placeHolder.txt >> /home/pi/CTD/LogFiles/combinedProfile.txt
