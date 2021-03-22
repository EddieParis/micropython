#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/obj.h"
#include "py/objint.h"

#include "freertos/semphr.h"

#include "i2s.h"

#include "driver/i2s.h"
#include "mp3_decoder.h"

static const int i2s_num = 0; // i2s port number

SemaphoreHandle_t readPtrSem;

typedef struct  {
    short* buffer;
    int actual_data_size;
} decoded_buffer_t;

decoded_buffer_t decoded_buffers[2];

QueueHandle_t decodedQueue;
QueueHandle_t freeQueue;

int readPtr = 0;

int ringSize = 0;
uint8_t* ringBuffer = NULL;

STATIC void init_decoded_buffer(decoded_buffer_t *buff);
STATIC void i2s_inject_entrypoint(void* args);
STATIC void mp3_decoder_entrypoint(void* args);

static const i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,
    .sample_rate = 44100,
    .bits_per_sample = 16,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
};

static const i2s_pin_config_t pin_config = {
    .bck_io_num = 4,
    .ws_io_num = 5,
    .data_out_num = 15,
    .data_in_num = I2S_PIN_NO_CHANGE
};

//~ short output[1152*2*2];

STATIC mp_obj_t i2s_init(mp_obj_t pin_in, mp_obj_t buf_in) {
    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);   //install and start i2s driver

    i2s_set_pin(i2s_num, &pin_config);

    readPtrSem = xSemaphoreCreateMutex();

    init_decoded_buffer(&decoded_buffers[0]);
    init_decoded_buffer(&decoded_buffers[1]);

    MP3Decoder_AllocateBuffers();

    decodedQueue = xQueueCreate(2, sizeof(decoded_buffer_t));
    freeQueue = xQueueCreate(2, sizeof(decoded_buffer_t));

    xQueueSend(freeQueue, &decoded_buffers[0], portMAX_DELAY);
    xQueueSend(freeQueue, &decoded_buffers[1], portMAX_DELAY);

    TaskHandle_t xHandle = NULL;

    //~ xTaskCreate( i2s_inject_entrypoint, "I2S_INJECTOR", 1024, (void*)&decoded_buffers, tskIDLE_PRIORITY, &xHandle );
    xTaskCreate( i2s_inject_entrypoint, "I2S_INJECTOR", 1024, NULL, 5, &xHandle );
    configASSERT( xHandle );

    return mp_const_none;
}

STATIC void init_decoded_buffer(decoded_buffer_t* buff) {
    buff->buffer = (short*)malloc(1152*2*2);
    buff->actual_data_size = 1152*2;
}

STATIC void setReadPtr(int value) {
    xSemaphoreTake(readPtrSem, portMAX_DELAY);
    readPtr = value;
    xSemaphoreGive(readPtrSem);
}

STATIC mp_obj_t player_get_read_ptr() {
    xSemaphoreTake(readPtrSem, portMAX_DELAY);
    mp_obj_t ret = mp_obj_new_int(readPtr);
    xSemaphoreGive(readPtrSem);
    return ret;
}

STATIC mp_obj_t mp3_set_buffer(mp_obj_t ringbuff) {
    //~ printf("setting buffer");
    mp_buffer_info_t buf_in_info;

    mp_get_buffer_raise(ringbuff, &buf_in_info, MP_BUFFER_READ);

    //~ printf("Got buffer info");

    ringBuffer = (void*)buf_in_info.buf;
    ringSize = buf_in_info.len;
    readPtr = 0;

    //~ printf("Ring size %d", ringSize);

    TaskHandle_t xHandle = NULL;

    xTaskCreate( mp3_decoder_entrypoint, "MP3_DECODER", 2048, NULL, 5, &xHandle );
    configASSERT( xHandle );

    return mp_const_none;
}

STATIC void mp3_decoder_entrypoint(void* args) {
    printf("mp3decoder spawned");
    decoded_buffer_t decode_buff;
    for (;;) {
        xQueueReceive(freeQueue, &decode_buff, portMAX_DELAY);
        printf("Got free buffer");
        if ((ringSize - readPtr) >= 6 ) {
            MP3GetNextFrameInfo(ringBuffer+readPtr);
        } else {
            uint8_t temp[6];
            uint32_t left = ringSize - readPtr;
            memcpy(&temp[0], ringBuffer+readPtr, left);
            memcpy(&temp[left], ringBuffer, 6-left);
            MP3GetNextFrameInfo(temp);
            assert(0);
        }
        int frameLength = MP3FrameLength();
        //~ printf("Frame length %d", frameLength);
        int bytesLeft = frameLength;
        if ((ringSize - readPtr) >= frameLength ) {
            MP3Decode(ringBuffer+readPtr, &bytesLeft, decode_buff.buffer, 0);
            //~ MP3Decode(ringBuffer+readPtr, &bytesLeft, output, 0);
            decode_buff.actual_data_size = MP3GetOutputSamps()*MP3GetChannels();
            setReadPtr(readPtr+frameLength-bytesLeft);
            //assert(bytesLeft==0);
        } else {
            uint8_t temp[frameLength];
            uint32_t left = ringSize - readPtr;
            memcpy(&temp[0], ringBuffer+readPtr, left);
            memcpy(&temp[left], ringBuffer, frameLength-left);
            MP3Decode(temp, &bytesLeft, decode_buff.buffer, 0);
            decode_buff.actual_data_size = MP3GetOutputSamps()*MP3GetChannels();
            setReadPtr(frameLength-left-bytesLeft);
        }
        //~ printf("Decoded out buff size %d samples", MP3GetOutputSamps()*MP3GetChannels());
        xQueueSend(decodedQueue, &decode_buff, portMAX_DELAY);
    }
}

STATIC void i2s_inject_entrypoint(void* args) {
    size_t written;

    decoded_buffer_t buffer;

    //~ printf("I2S injector spawn");

    for(;;) {
        xQueueReceive(decodedQueue, &buffer, portMAX_DELAY);
        //~ printf("Got new buffer");
        esp_err_t err = i2s_write(i2s_num, buffer.buffer, buffer.actual_data_size, &written, portMAX_DELAY);
        xQueueSend(freeQueue, &buffer, portMAX_DELAY);
    }
}



STATIC mp_obj_t i2s_send_mp3(mp_obj_t buf_in) {

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    void *buf = (void*)bufinfo.buf;
    int bytesLeft=bufinfo.len;

    //~ int mp3_err = MP3Decode(buf, &bytesLeft, output, 0);
    //~ int mp3_err = MP3GetNextFrameInfo(buf);

    size_t written;

    //~ esp_err_t err = i2s_write(i2s_num, output, MP3GetOutputSamps()*2, &written, portMAX_DELAY);

    mp_obj_t obj_array[4];
    //~ obj_array[0]=mp_obj_new_int(mp3_err);
    obj_array[1]=mp_obj_new_int(bytesLeft);
    //~ obj_array[2]=mp_obj_new_int(err);
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

MP_DEFINE_CONST_FUN_OBJ_1(mp3_find_sync_word_obj, mp3_find_sync_word);
MP_DEFINE_CONST_FUN_OBJ_1(mp3_get_next_sample_rate_obj, mp3_get_next_sample_rate);
//~ MP_DEFINE_CONST_FUN_OBJ_0(mp3_alloc_out_buffer_obj, mp3_alloc_out_buffer);
MP_DEFINE_CONST_FUN_OBJ_0(mp3_get_frame_length_obj, mp3_get_frame_length);
MP_DEFINE_CONST_FUN_OBJ_0(mp3_get_output_samples_count_obj, mp3_get_output_samples_count);
//~ MP_DEFINE_CONST_FUN_OBJ_2(mp3_decode_into_obj, mp3_decode_into);

MP_DEFINE_CONST_FUN_OBJ_2(i2s_init_obj, i2s_init);
MP_DEFINE_CONST_FUN_OBJ_1(i2s_send_mp3_obj, i2s_send_mp3);
MP_DEFINE_CONST_FUN_OBJ_1(i2s_send_obj, i2s_send);
MP_DEFINE_CONST_FUN_OBJ_1(i2s_set_sample_rate_obj, i2s_set_sample_rate);
MP_DEFINE_CONST_FUN_OBJ_0(player_get_read_ptr_obj, player_get_read_ptr);
MP_DEFINE_CONST_FUN_OBJ_0(i2s_term_obj, i2s_term);

MP_DEFINE_CONST_FUN_OBJ_1(mp3_set_buffer_obj, mp3_set_buffer);
