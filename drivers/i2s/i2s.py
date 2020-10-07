#from esp import

from esp import mp3_alloc_out_buffer
from esp import mp3_get_next_sample_rate
from esp import mp3_find_sync_word

from esp import i2s_init
from esp import i2s_send_mp3
from esp import i2s_send
from esp import i2s_term

class I2S:
    def __init__(self):
        i2s_init(1, 2)

    def __del__(self):
        i2s_term()

    def send(self, data):
        i2s_send(data)

    def send_mp3(self, data):
        return i2s_send_mp3(data)
