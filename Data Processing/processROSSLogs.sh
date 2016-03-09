#!/bin/bash
#Remotely Operated Surface Sampler Log Parser
#This script parses out the GPS and IMU data from a Pixhawk binary log file.
#You have to have "mavlogdump.py" in the directory ~/scripts/
#Written by Nick McComb
#Version 1.3.1 [Jan 2016]

#TODO: Add _actual_ time estimation (171 took ~80 minutes (4856 seconds) for first processing)
#TODO: Check if raw output file already exists
#TODO: Add command line args (eg, only parse GPS) | Do we actually want this?

if [ -z "$1" ]; then #If the user didn't enter a file name
  #Then we need to find the latest .tlog or .BIN file in the directory
  filename=$(ls -t | grep '\.BIN\|.tlog' | head -1)
  response='z' #Dummy value
  until [[ "$response" == "y" || "$response" == "n" ]] 
  do  
    echo -e "[The latest valid file in this directory is: \e[34m${filename}\e[39m]"
    echo -en \# 'Is our target? (y/n): '
    read -n 1 response
    echo -e "\n" #This actually makes two newlines
  done
  if [ "$response" == "y" ]; then #User responded yes
    targetFilename=$filename
  else #User responsded no
    echo "Please invoke the program using $0 [log file name]"
    exit
  fi
else #Then the user did enter a file name via command line
  targetFilename=$1
fi

echo -e "[Processing \e[34m${targetFilename}\e[39m]\n"

#targetFilename="exampleLog.tlog"
rawOutputFilename="rawOutput.txt"
GPSOutputFilename="GPSLog.txt"
IMUOutputFilename="IMULog.txt"

TIMEFORMAT='[Processing main file took %R seconds]'
time {
  inputFilesize=$(wc -c < ${targetFilename})
  inputFilesizeMB=$(bc <<< "scale=2; $inputFilesize / 1024 / 1024")
  echo "[Filesize is ${inputFilesizeMB}MB]"
  ##!! MAC AIR SPECIFIC !!## << This section of code is slightly specific to the VM on the Mac Air
  #The Mac Air calculates at about .000005 seconds/byte
  estTime=$(bc <<< "scale=2; $inputFilesize * .000005")
  estTimeMin=$(bc <<< "scale=2; $estTime / 60")
  echo "[Estimated time on Mac Air is $estTime seconds]"
  echo "[Check back in $estTimeMin minutes]"
  ##!! END MAC AIR !!##
  (python ~/scripts/mavlogdump.py ${targetFilename}) > ${rawOutputFilename}
  echo "[Processed main file]"
}
echo -en '\n'
TIMEFORMAT='[Processing GPS Info took %R seconds]'
time {
  echo "[Beginning to extract GPS Info]"
  (cat ${rawOutputFilename} | grep POS) > ${GPSOutputFilename}
  (cat ${rawOutputFilename} | grep GPS) >> ${GPSOutputFilename}
  (awk 'NR%10==0{print $0}' ${GPSOutputFilename}  > "tenth_${GPSOutputFilename}")
  echo "[GPS Info Extracted]"
}
TIMEFORMAT='[Processing IMU Info took %R seconds]'
echo -en '\n'
time {
  (cat ${rawOutputFilename} | grep -e ATT -e IMU -e Mag) > ${IMUOutputFilename} 
  (awk 'NR%10==0{print $0}' ${IMUOutputFilename}  > "tenth_${IMUOutputFilename}")
}
echo -en '\n'
echo "[Finished script]"
