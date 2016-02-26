from sys import argv
import serial
from time import sleep

def closeCheck():
	if ser.isOpen() == False:
		print "Port closed"	
	elif ser.isOpen() == True:
		print "Port is still open"
		ser.close()
		closeCheck()
		
ser = serial.Serial(
    port='COM5',\
    baudrate=9600,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
    timeout= None) #Open serial port
ser.close()



header = 255
speedOut = int(argv[1]) #Collect runtime parameters
speedIn = int(argv[2])
depth = int(argv[3])
upperDepthByte = depth >> 8
lowerDepthByte = depth & 0xFF
stop = int(argv[4])
checksum = (((speedOut ^ speedIn) ^ upperDepthByte) ^ lowerDepthByte) #XOR all variables to create checksum
footer = 255

parameters = [header , speedOut, speedIn, upperDepthByte, lowerDepthByte, checksum, footer] #Save parameters in array

if stop != 0:
	if stop == 1:
		for x in parameters:
			x = 0xAA
	elif stop == 2:
		for x in parameters:
			x = 0xBB
	elif stop == 3:
		for x in parameters:
			x = 0xCC

print parameters

ser.open()
for x in parameters:
	x = chr(x) #cast to char to make single bite
	ser.write(x)
	sleep(0.1)

ser.close()
closeCheck()

	
	
