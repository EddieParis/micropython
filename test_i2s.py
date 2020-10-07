import i2s
import struct
import urequest

from esp import mp3_alloc_out_buffer
from esp import mp3_get_next_sample_rate
from esp import mp3_find_sync_word
from esp import mp3_get_frame_length
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
mp3_alloc_out_buffer()

remain = 2000-offset

# ~ buff += mp3_stream.read(fsize-(remain % fsize))
buff = buff[0:418]

while True:
    ret = i.send_mp3(buff)

    mp3_err, remain, i2s_err, sampl = ret
    print(ret)

    buff = buff[fsize-remain:] + mp3_stream.read(fsize-remain)
    print(len(buff))
    # ~ print(i.send_mp3(buff))
    # ~ buff = f.read(1008)
    # ~ if len(buff)==0:
        # ~ break


