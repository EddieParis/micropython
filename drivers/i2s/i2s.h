#ifndef MICROPY_INCLUDED_DRIVERS_I2S_I2S_H
#define MICROPY_INCLUDED_DRIVERS_I2S_I2S_H

#include "py/obj.h"

MP_DECLARE_CONST_FUN_OBJ_1(mp3_find_sync_word_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp3_get_next_sample_rate_obj);
//~ MP_DECLARE_CONST_FUN_OBJ_0(mp3_alloc_out_buffer_obj);
MP_DECLARE_CONST_FUN_OBJ_0(mp3_get_frame_length_obj);
MP_DECLARE_CONST_FUN_OBJ_0(mp3_get_output_samples_count_obj);
//~ MP_DECLARE_CONST_FUN_OBJ_2(mp3_decode_into_obj);
MP_DECLARE_CONST_FUN_OBJ_2(i2s_init_obj);
MP_DECLARE_CONST_FUN_OBJ_1(i2s_send_mp3_obj);
MP_DECLARE_CONST_FUN_OBJ_1(i2s_set_sample_rate_obj);
MP_DECLARE_CONST_FUN_OBJ_1(i2s_send_obj);
MP_DECLARE_CONST_FUN_OBJ_0(player_get_read_ptr_obj);
MP_DECLARE_CONST_FUN_OBJ_0(i2s_term_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp3_set_buffer_obj);

#endif // MICROPY_INCLUDED_DRIVERS_I2S_I2S_H
