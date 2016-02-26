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
speed = int(argv[1]) #Collect runtime parameters
depth = int(argv[2])
upperByte = depth >> 8
lowerByte = depth & 0xFF
duration = int(argv[3])
units = int(argv[4])
checksum = ((((speed ^ upperByte) ^ lowerByte) ^ duration) ^ units) #XOR all variables to create checksum
footer = 255

parameters = [header , speed, upperByte, lowerByte, duration, units, checksum, footer] #Save parameters in array
print parameters

ser.open()
for x in parameters:
	x = chr(x) #cast to char to make single bite
	ser.write(x)
	sleep(0.1)

ser.close()
closeCheck()

	
	
