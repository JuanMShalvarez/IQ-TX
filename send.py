import time, wave
import numpy as np
import serial

FILE    = r"Snap_Out_of_It.wav"
PORT    = "COM3"      # board's COM port (Device Manager)
FS      = 16000       # must match TIM6: 16 MHz / 1000
PRE     = 2048        # bytes prefilled before pacing starts
CH      = 64          # bytes per USB write
START_S = 0           # start offset in file, seconds
MAX_S   = 10          # play length; keep <= ~10 s without feedback

# --- read WAV (header sizes may be 0xFFFFFFFF, so don't trust nframes) ---
w = wave.open(FILE)
n, sr, sw = w.getnchannels(), w.getframerate(), w.getsampwidth()
w.close()
assert sw == 2, "need 16-bit PCM"

raw = open(FILE, "rb").read()
off = raw.find(b"data") + 8
frame = 2 * n
x = np.frombuffer(raw[off:off + (len(raw) - off) // frame * frame], "<i2")
x = x.reshape(-1, n).astype(float).mean(axis=1)           # mono mixdown
x = x[int(START_S * sr):int((START_S + MAX_S) * sr)]

# --- resample to FS ---
if sr % FS == 0:                                           # integer ratio: block average
    r = sr // FS
    x = x[:len(x) // r * r].reshape(-1, r).mean(axis=1)
else:                                                      # fallback: linear interp
    t = np.arange(0, len(x) / sr, 1 / FS)
    x = np.interp(t, np.arange(len(x)) / sr, x)

# --- normalize, convert to unsigned 8-bit ---
x /= max(np.abs(x).max(), 1.0)
u8 = ((0.95 * x + 1) * 127.5).clip(0, 255).astype(np.uint8)
print(f"{len(u8)} samples, {len(u8) / FS:.1f} s")

# --- stream ---
ser = serial.Serial(PORT)
ser.write(u8[:PRE].tobytes())
t0 = time.perf_counter()
for i in range(PRE, len(u8), CH):
    while i > PRE + (time.perf_counter() - t0) * FS:
        time.sleep(0.001)
    ser.write(u8[i:i + CH].tobytes())
ser.close()
