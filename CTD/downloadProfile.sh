#!/bin/bash
while ! ping -c 1 -W 1 1.2.3.4; do #Wait until connection with CTD is established
	sleep 1
done
if [ $# = 1 ]
	then
		numChannels=$1
		if [[ "$numChannels" -ge 1 && "$numChannels" -le 6 ]]
			then
				packetSize=$((8+(4*numChannels)))
				profileSize=$(wc --bytes < /home/pi/CTD/LogFiles/combinedProfile.bin)
				profileSize=$(((profileSize/packetSize)*packetSize)) #Ensure no extraneous bits produce garbage values
				echo $profileSize
				timeStamp=$(date +%Y%m%d%H%M%S)
				/home/pi/CTD/Downloader/bin/logger2wifidownloader -offset=$profileSize -length=all > /home/pi/CTD/LogFiles/placeHolder.bin
				cat /home/pi/CTD/LogFiles/placeHolder.bin > /home/pi/CTD/LogFiles/Profile-$timeStamp.bin
				cat /home/pi/CTD/LogFiles/placeHolder.bin >> /home/pi/CTD/LogFiles/combinedProfile.bin
				cat /home/pi/CTD/LogFiles/combinedProfile.bin | /home/pi/CTD/ParseReader/a.out 6 > /home/pi/CTD/LogFiles/combinedParsedProfiles.txt
				cat /home/pi/CTD/LogFiles/Profile-$timeStamp.bin | /home/pi/CTD/ParseReader/a.out 6 > /home/pi/CTD/LogFiles/parsedProfile-$timeStamp.txt
		else
			echo "Valid numbers of channels are 1 - 6."
		fi
else
	echo "Provide number of channels."
fi