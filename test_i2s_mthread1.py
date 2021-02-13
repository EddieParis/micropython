import i2s
import struct
import urequest
import _thread

from esp import mp3_alloc_out_buffer
from esp import mp3_get_next_sample_rate
from esp import mp3_find_sync_word
from esp import mp3_get_frame_length
from esp import mp3_get_output_samples_count
from esp import mp3_decode_into
from esp import i2s_set_sample_rate


i=i2s.I2S()

mp3_stream = urequest.urlopen('http://radiofg.impek.com/fg')
buff = mp3_stream.read(418)

# ~ f = open("mp3aa", "rb")
# ~ f.read(0x460)
# ~ buff = f.read(1008)

offset = mp3_find_sync_word(buff)
print(offset)
if offset > 0:
    buff=buff[offset:]

sr = mp3_get_next_sample_rate(buff)
print(sr)
i2s_set_sample_rate(sr)
fsize = mp3_get_frame_length()
print(fsize)

outbuff1 = bytearray(mp3_get_output_samples_count()*2)
outbuff2 = bytearray(mp3_get_output_samples_count()*2)

remain = 2000-offset

# ~ buff += mp3_stream.read(fsize-(remain % fsize))
buff = buff[0:418]

ret = mp3_decode_into(buff, outbuff1)
mp3_err, remain, sampl = ret
buff = buff[fsize-remain:] + mp3_stream.read(fsize-remain)

buff_a = outbuff1
buff_b = outbuff2

lock = _thread.allocate_lock()

ret = mp3_decode_into(buff, buff_a)
mp3_err, remain, sampl = ret
print(ret)

buff = buff[fsize-remain:] + mp3_stream.read(fsize-remain)
print(len(buff))

def injector():
    while not lock.locked():
        pass

    while True:
        lock.release()
        i.send(buff_a)

_thread.start_new_thread(injector, ())

while True:
    lock.acquire()
    ret = mp3_decode_into(buff, buff_b)
    mp3_err, remain, sampl = ret
    print(ret)

    temp = buff_b
    buff_b = buff_a
    buff_a = temp

    buff = buff[fsize-remain:] + mp3_stream.read(fsize-remain)
    print(len(buff))

    # ~ print(i.send_mp3(buff))
    # ~ buff = f.read(1008)
    # ~ if len(buff)==0:
        # ~ break


