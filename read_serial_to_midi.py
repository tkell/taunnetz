import serial
import sys
import time


ser = serial.Serial('/dev/ttyACM0', 9600)
from OSC import OSCClient, OSCMessage

client = OSCClient()
client.connect( ("localhost", 9999) )

if len(sys.argv) > 1 and sys.argv[1] == "--debug":
    DEBUG = True;
else:
    DEBUG = False

number_touches = 10
print "Ready..."
start_time = time.time()
while True:
    touch_string = ser.readline()
    touch_string = touch_string.split(",")[0:-1]

    if len(touch_string) != number_touches:
        continue
    try:    
        touch_list = [int(t) for t in touch_string]
    except ValueError:
        continue

    if DEBUG:
        print time.time() - start_time
        print touch_list

    
