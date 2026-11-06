import serial, numpy as np
s = serial.Serial('COM3')
n = np.arange(200)
buf = np.empty(400, np.uint8)
buf[0::2] = (64 + 63*np.cos(2*np.pi*n/200)).astype(np.uint8)
buf[1::2] = (64 + 63*np.sin(2*np.pi*n/200)).astype(np.uint8)
while True: s.write(buf.tobytes())