#include <stdio.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/obj.h"
#include "py/objint.h"

#include "i2s.h"

#include "driver/i2s.h"
#include "mp3_decoder.h"

static const int i2s_num = 0; // i2s port number

static const i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,
    .sample_rate = 44100,
    .bits_per_sample = 16,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    .use_apll = false
};

static const i2s_pin_config_t pin_config = {
    .bck_io_num = 4,
    .ws_io_num = 5,
    .data_out_num = 15,
    .data_in_num = I2S_PIN_NO_CHANGE
};

short * output = NULL;

STATIC mp_obj_t mp3_find_sync_word(mp_obj_t buf_in) {

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    int offset = MP3FindSyncWord(bufinfo.buf, bufinfo.len);

    return mp_obj_new_int(offset);
}

STATIC mp_obj_t mp3_get_next_sample_rate(mp_obj_t buf_in) {

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    int err = MP3GetNextFrameInfo(bufinfo.buf);

    if (err == ERR_MP3_NONE)
    {
        return mp_obj_new_int(MP3GetSampRate());
    }
    else
    {
        return mp_const_none;
    }
}


STATIC mp_obj_t mp3_get_frame_length() {
    return mp_obj_new_int(MP3FrameLength());
}

STATIC mp_obj_t mp3_get_output_samples_count() {
    return mp_obj_new_int(MP3GetOutputSamps());
}

STATIC mp_obj_t i2s_set_sample_rate(mp_obj_t mp_obj_sr) {

    esp_err_t err = i2s_set_sample_rates(i2s_num, (uint32_t)mp_obj_int_sign(mp_obj_sr));

    if ( err == ESP_ERR_NO_MEM ) {
        mp_raise_OSError(MP_ENOMEM);
    } else if ( err == ESP_OK ) {
        mp_raise_OSError(MP_EINVAL);
    } else {
        return mp_const_none;
    }
}

static mp_obj_t mp3_alloc_out_buffer() {

    if (output != NULL) {
        free(output);
    }

    output = (short *)malloc(MP3GetOutputSamps()*2);
    if (output==NULL)
    {
        mp_raise_OSError(MP_ENOMEM);
    }

    return mp_const_none;
}

STATIC mp_obj_t mp3_decode_into(mp_obj_t buf_in_obj, mp_obj_t buf_out_obj) {
    mp_buffer_info_t buf_in_info;
    mp_buffer_info_t buf_out_info;

    mp_get_buffer_raise(buf_in_obj, &buf_in_info, MP_BUFFER_READ);
    mp_get_buffer_raise(buf_out_obj, &buf_out_info, MP_BUFFER_READ);

    void *buf_in = (void*)buf_in_info.buf;
    int bytesLeft=buf_in_info.len;

    void *buf_out = (void*)buf_out_info.buf;
    int mp3_err = MP3Decode(buf_in, &bytesLeft, buf_out, 0);

    mp_obj_t obj_array[3];
    obj_array[0]=mp_obj_new_int(mp3_err);
    obj_array[1]=mp_obj_new_int(bytesLeft);
    obj_array[2]=mp_obj_new_int(MP3GetOutputSamps());

    return mp_obj_new_list(3, obj_array);
}

STATIC mp_obj_t i2s_init(mp_obj_t pin_in, mp_obj_t buf_in) {

    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);   //install and start i2s driver

    i2s_set_pin(i2s_num, &pin_config);

    MP3Decoder_AllocateBuffers();

    return mp_const_none;
}

STATIC mp_obj_t i2s_send_mp3(mp_obj_t buf_in) {

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    void *buf = (void*)bufinfo.buf;
    int bytesLeft=bufinfo.len;

    int mp3_err = MP3Decode(buf, &bytesLeft, output, 0);
    //~ int mp3_err = MP3GetNextFrameInfo(buf);

    size_t written;

    esp_err_t err = i2s_write(i2s_num, output, MP3GetOutputSamps()*2, &written, portMAX_DELAY);

    mp_obj_t obj_array[4];
    obj_array[0]=mp_obj_new_int(mp3_err);
    obj_array[1]=mp_obj_new_int(bytesLeft);
    obj_array[2]=mp_obj_new_int(err);
    obj_array[3]=mp_obj_new_int(MP3GetOutputSamps());

    return mp_obj_new_list(4, obj_array);
}

STATIC mp_obj_t i2s_send(mp_obj_t buf_in) {

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    void *buf = (void*)bufinfo.buf;

    size_t written;

    esp_err_t err = i2s_write(i2s_num, buf, bufinfo.len, &written, portMAX_DELAY);
    return mp_const_none;
}

STATIC mp_obj_t i2s_term() {
    MP3Decoder_FreeBuffers();
    i2s_driver_uninstall(i2s_num);
    return mp_const_none;
}


MP_DEFINE_CONST_FUN_OBJ_1(mp3_find_sync_word_obj, mp3_find_sync_word);
MP_DEFINE_CONST_FUN_OBJ_1(mp3_get_next_sample_rate_obj, mp3_get_next_sample_rate);
MP_DEFINE_CONST_FUN_OBJ_0(mp3_alloc_out_buffer_obj, mp3_alloc_out_buffer);
MP_DEFINE_CONST_FUN_OBJ_0(mp3_get_frame_length_obj, mp3_get_frame_length);
MP_DEFINE_CONST_FUN_OBJ_0(mp3_get_output_samples_count_obj, mp3_get_output_samples_count);
MP_DEFINE_CONST_FUN_OBJ_2(mp3_decode_into_obj, mp3_decode_into);

MP_DEFINE_CONST_FUN_OBJ_2(i2s_init_obj, i2s_init);
MP_DEFINE_CONST_FUN_OBJ_1(i2s_send_mp3_obj, i2s_send_mp3);
MP_DEFINE_CONST_FUN_OBJ_1(i2s_send_obj, i2s_send);
MP_DEFINE_CONST_FUN_OBJ_1(i2s_set_sample_rate_obj, i2s_set_sample_rate);
MP_DEFINE_CONST_FUN_OBJ_0(i2s_term_obj, i2s_term);
