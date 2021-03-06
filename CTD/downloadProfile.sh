#!/bin/bash

# create a file to indicate that downloading is in progress
touch /home/pi/CTD/LogFiles/ctddownloadinprogress


# restart the wireless
echo "Restarting wlan1."
/sbin/ifdown wlan1
sleep 5
/sbin/ifup wlan1 
echo "wlan1 is now back up."

# wait until the connection with the CTD is established
error=0
pingcount=0
while ! ping -c 1 -W 1 1.2.3.4; do
    sleep 1
    let pingcount=$pingcount+1
    if [ $pingcount -ge 20 ]
      then
       error=1
       touch /home/pi/CTD/LogFiles/noPingFound.txt
       break
    fi
done

if [ $error = 0 ]
 then
  echo "Got ping."
  touch /home/pi/CTD/LogFiles/PingFound.txt
  if [ $# = 2 ]
    then
        numChannels=$1
        outpath=$2
        if [[ "$numChannels" -ge 1 && "$numChannels" -le 6 ]]
            then
		        packetSize=$((8+(4*numChannels)))
                touch $outpath/combinedProfile.bin
		        profileSize=$(wc --bytes < $outpath/combinedProfile.bin)
		        profileSize=$(((profileSize/packetSize)*packetSize)) #Ensure no extraneous bits produce garbage values
		        echo $profileSize
		        timeStamp=$(date +%Y%m%d%H%M%S)
		        /home/pi/CTD/Downloader/bin/logger2wifidownloader -offset=$profileSize -length=all > $outpath/placeHolder.bin
		        cat $outpath/placeHolder.bin > $outpath/Profile-$timeStamp.bin
		        cat $outpath/placeHolder.bin >> $outpath/combinedProfile.bin
		        cat $outpath/combinedProfile.bin | /home/pi/CTD/ParseReader/a.out 6 > $outpath/combinedParsedProfiles.txt
		        cat $outpath/Profile-$timeStamp.bin | /home/pi/CTD/ParseReader/a.out 6 > $outpath/parsedProfile-$timeStamp.txt
		        cp $outpath/parsedProfile-$timeStamp.txt $outpath/latestparsedProfile.txt
	    else
          	echo "Valid numbers of channels are 1 - 6."
	    fi
  else
    echo "Provide number of channels and output folder destination."
  fi



else
  echo "Timed out trying to connect to CTD wifi."
fi

rm /home/pi/CTD/LogFiles/ctddownloadinprogress
