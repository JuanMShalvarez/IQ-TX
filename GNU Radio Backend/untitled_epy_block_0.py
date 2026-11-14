import numpy as np
from gnuradio import gr
import serial


class blk(gr.sync_block):
    def __init__(self):
        gr.sync_block.__init__(self, name='BeeSDR',
                               in_sig=[np.complex64], out_sig=None)
        self.port = 'COM3'
        self.ser = None

    def start(self):
        self.ser = serial.Serial(self.port)
        self.ser.reset_output_buffer()
        return True

    def work(self, input_items, output_items):
        x = input_items[0]
        buf = np.empty(2 * len(x), np.uint8)
        buf[0::2] = (64 + 63 * np.clip(x.real, -1, 1)).astype(np.uint8)  # I -> DAC1 OUT1
        buf[1::2] = (64 + 63 * np.clip(x.imag, -1, 1)).astype(np.uint8)  # Q -> DAC1 OUT2
        self.ser.write(buf.tobytes())
        return len(x)

    def stop(self):
        if self.ser:
            self.ser.close()
            self.ser = None
        return True