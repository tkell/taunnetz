import serial
ser = serial.Serial('/dev/ttyACM0', 9600)
from OSC import OSCClient, OSCMessage

client = OSCClient()
client.connect( ("localhost", 9999) )


number_touches = 6
while True:
    touch_string = ser.readline()
    touch_string = touch_string.split(",")[0:-1]

    if len(touch_string) != 6:
        continue
    try:    
        touch_list = [int(t) for t in touch_string]
    except ValueError:
        continue

    print touch_list
    if len(touch_list) > 0:
        msg = OSCMessage("/")
        msg.append(touch_list)
        client.send(msg)
    
