import i2s
import struct
import urequest
import _thread
import time

from esp import mp3_alloc_out_buffer
from esp import mp3_get_next_sample_rate
from esp import mp3_find_sync_word
from esp import mp3_get_frame_length
from esp import mp3_get_output_samples_count
from esp import mp3_decode_into
from esp import i2s_set_sample_rate


class RingBuff:
    def __init__(self, initial_buff):
        self.lock = _thread.allocate_lock()
        self.buff = bytearray(initial_buff)
        self.read_idx=0
        self.write_idx=0

    def get_curr(self):
        self.lock.acquire()
        if self.read_idx < len(self.buff)-418:
            ret = self.buff[self.read_idx:self.read_idx+418]
            self.read_idx += 418
        else:
            ret = self.buff[self.read_idx:]
            missing = 418-(len(self.buff)-self.read_idx)
            ret += self.buff[:missing]
            self.read_idx = missing
        self.lock.release()
        return ret

    def got_extra(self, bytes_count):
        self.lock.acquire()
        if self.read_idx != 0:
            self.read_idx -= 1
        else:
             self.read_idx=len(self.buff)-1
        self.lock.release()

    def hungry(self):
        self.lock.acquire()
        ret = len(self.buff)-self.read_idx + self.write_idx > len(self.buff)/2
        self.lock.release()
        return ret

    def set_data(self, data):
        if self.write_idx+len(data) < len(self.buff):
            self.buff[self.write_idx:self.write_idx+len(data)] = data
            self.lock.acquire()
            self.write_idx += len(data)
            self.lock.release()
        else:
            remain = self.write_idx+len(data) % len(self.buff)
            self.buff[self.write_idx:self.write_idx+remain] = data
            self.buff[:remain] = data[
            self.lock.acquire()
            self.write_idx = remain
            self.lock.release()

i=i2s.I2S()

mp3_stream = urequest.urlopen('http://radiofg.impek.com/fg')
ring_buffer = RingBuff(mp3_stream.read(100*418))

# ~ f = open("mp3aa", "rb")
# ~ f.read(0x460)
# ~ buff = f.read(1008)

# ~ offset = mp3_find_sync_word(ring_buffer.get_curr())
# ~ print(offset)
# ~ if offset > 0:
    # ~ buff=buff[offset:]

buff = ring_buffer.get_curr()
sr = mp3_get_next_sample_rate(buff)
print(sr)
i2s_set_sample_rate(sr)
fsize = mp3_get_frame_length()
print(fsize)

outbuff1 = bytearray(mp3_get_output_samples_count()*2)

# ~ remain = 2000-offset

# ~ buff += mp3_stream.read(fsize-(remain % fsize))
# ~ buff = buff[0:418]

# ~ ret = mp3_mp3_decode_into(buff, outbuff1)
# ~ mp3_err, remain, sampl = ret
# ~ buff = buff[fsize-remain:] + mp3_stream.read(fsize-remain)

# ~ ret = mp3_mp3_decode_into(buff, buff_a)
# ~ mp3_err, remain, sampl = ret
# ~ print(ret)

# ~ buff = buff[fsize-remain:] + mp3_stream.read(fsize-remain)
# ~ print(len(buff))

def injector():
    while True:
        if not ring_buffer.hungry():
            print ("injecting")
            ring_buffer.set_data(mp3_stream.read(25*418))
        time.sleep(.1)
_thread.start_new_thread(injector, ())

while True:
    ret = mp3_decode_into(buff, outbuff1)
    mp3_err, remain, sampl = ret
    if remain:
        ring_buffer.got_extra(remain)
    # ~ print(ret)

    i.send(outbuff1)

    buff = ring_buffer.get_curr()

    # ~ print(i.send_mp3(buff))
    # ~ buff = f.read(1008)
    # ~ if len(buff)==0:
        # ~ break


