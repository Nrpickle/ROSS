from sys import argv
import serial
from time import sleep

ser = serial.Serial(
    port='COM5',\
    baudrate=9600,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
    timeout=0) #Open serial port

depth = int(argv[2])

if depth <= 255:
	depthByte1 = 0
	depthByte2 = depth

if depth > 255:
	depthByte2 = 255
	depthByte1 = depth - 255

header = 255
speed = int(argv[1]) #Collect runtime parameters
duration = int(argv[3])
units = int(argv[4])
checksum = ((((speed ^ depthByte1) ^ depthByte2) ^ duration) ^ units)
footer = 255

parameters = [header , speed, depthByte1, depthByte2, duration, units, checksum, footer] #Save parameters in array
print parameters

#parameters = bytearray(parameters) #Convert array to byte array
#print parameters #Print parameters to screen then send via serial

for x in parameters:
	ser.write(x)
	sleep(0.1)
	
sleep(15)
