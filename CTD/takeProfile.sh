#!/bin/bash
profileSize=$(wc --bytes < /home/pi/CTD/LogFiles/combinedProfile.txt)
binaryProfileSize=$((((profileSize/32)+1)*32))
echo $profileSize
timeStamp=$(date +%Y%m%d%H%M%S)
./logger2wifidownloader -offset=$profileSize -length=all > /home/pi/CTD/LogFiles/placeHolder.txt
cat /home/pi/CTD/LogFiles/placeHolder.txt > /home/pi/CTD/LogFiles/Profile-$timeStamp.txt  
cat /home/pi/CTD/LogFiles/placeHolder.txt >> /home/pi/CTD/LogFiles/combinedProfile.txt
cat /home/pi/CTD/LogFiles/combinedProfile.txt | /home/pi/CTD/ParseReader/a.out 6 > /home/pi/CTD/LogFiles/combinedParsedProfiles.txt
cat /home/pi/CTD/LogFiles/Profile-$timeStamp.txt | /home/pi/CTD/ParseReader/a.out 6 > /home/pi/CTD/LogFiles/parsedProfile-$timeStamp.txt
