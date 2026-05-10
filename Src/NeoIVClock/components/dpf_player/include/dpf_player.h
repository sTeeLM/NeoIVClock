#ifndef NEO_IV_CLOCK_DPF_PLAYER_H
#define NEO_IV_CLOCK_DPF_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct _dpf_player_msg_t {
  uint8_t signature;  // 7E
  uint8_t version;  // FF
  uint8_t length;  // lenth of cmd - sig - ver
  uint8_t command;
  uint8_t feedback; 
  uint8_t dh;
  uint8_t dl; 
  uint8_t checksum_hi;
  uint8_t checksum_lo;  
  uint8_t end;  // EF 
} dpf_player_msg_t;

typedef enum _dpf_player_eq_type_t
{
  DPF_PLAYER_EQ_NORMAL = 0,
  DPF_PLAYER_EQ_POP,
  DPF_PLAYER_EQ_ROCK,
  DPF_PLAYER_EQ_JAZZ,
  DPF_PLAYER_EQ_CLASSICAL,
  DPF_PLAYER_EQ_BASE
} dpf_player_eq_type_t;

typedef enum _dpf_player_dev_type_t
{
  DPF_PLAYER_DEV_U = 0,
  DPF_PLAYER_DEV_TF,
  DPF_PLAYER_DEV_AUX,
  DPF_PLAYER_DEV_SLEEP,
  DPF_PLAYER_DEV_FLASH
} dpf_player_dev_type_t;

typedef enum _dpf_player_status_type_t
{
  DPF_PLAYER_STATUS_STOPPED = 0,
  DPF_PLAYER_STATUS_PLAYING = 1,
  DPF_PLAYER_STATUS_PAUSED  = 2,
  DPF_PLAYER_STATUS_SLEEP   = 8
} dpf_player_status_type_t;

typedef enum _dpf_player_cmd_t {
  DPF_PLAYER_CMD_NONE          = 0,
  DPF_PLAYER_CMD_NEXT_TRACK    = 1,
  DPF_PLAYER_CMD_PREV_TRACK    = 2,  
  DPF_PLAYER_CMD_PLAY_TRACK    = 3, //0 - 2999
  DPF_PLAYER_CMD_INC_VOL       = 4, 
  DPF_PLAYER_CMD_DEC_VOL       = 5,
  DPF_PLAYER_CMD_SET_VOL       = 6,    // 0 - 30
  DPF_PLAYER_CMD_SET_EQ        = 7,    // 0 - 5 (Normal,Pop,Rock,Jazz,Class,Base)
  DPF_PLAYER_CMD_REPEAT_TRACK  = 8,    // 0 - 3 (Repeat,Folder Repeat,Single Repeat,Random)
  DPF_PLAYER_CMD_SET_DEV       = 9,    // 0 - 4 (U,TF,Aux,Sleep,Flash)
  DPF_PLAYER_CMD_STANDBY       = 0x0A,
  DPF_PLAYER_CMD_NORMAL        = 0x0B,
  DPF_PLAYER_CMD_RESET         = 0x0C,
  DPF_PLAYER_CMD_PLAY          = 0x0D,
  DPF_PLAYER_CMD_PAUSE         = 0x0E,
  DPF_PLAYER_CMD_PLAY_DIR_FILE = 0x0F, // 1-10
  DPF_PLAYER_CMD_AUDIO_AMP     = 0x10, // {DH=1: Open volume adjust} {DL: set volume gain 0-31}
  DPF_PLAYER_CMD_REPEAT_DEV    = 0x11, // 1: start repeat, 0: stop play
  DPF_PLAYER_CMD_PLAY_DIR_MP3_TRACK = 0x12, // Specify playback of folder named "MP3"
  DPF_PLAYER_CMD_PLAY_DIR_ADVERT_TRACK = 0x13,   // Insert track in the folder "ADVERT"
  DPF_PLAYER_CMD_PLAY_HUGE_DIR  = 0x14,
  DPF_PLAYER_CMD_STOP_ADVERT = 0x15,
  DPF_PLAYER_CMD_STOP          = 0x16,
  DPF_PLAYER_CMD_REPEAT_DIR    = 0x17,
  DPF_PLAYER_CMD_RANDOM_DEV    = 0x18,
  DPF_PLAYER_CMD_REPEAT_CUR_TRACK  = 0x19,  
  DPF_PLAYER_CMD_SET_DAC       = 0x1A,
  DPF_PLAYER_CMD_QUERY_STATUS  = 0x42,
  DPF_PLAYER_CMD_QUERY_VOL     = 0x43,
  DPF_PLAYER_CMD_QUERY_EQ      = 0x44,
  DPF_PLAYER_CMD_QUERY_TRACKS_U = 0x47,
  DPF_PLAYER_CMD_QUERY_TRACKS_TF = 0x48,
  DPF_PLAYER_CMD_QUERY_TRACKS_FLASH = 0x49,  
  DPF_PLAYER_CMD_KEEP_ON         = 0x4A,
  DPF_PLAYER_CMD_QUERY_CUR_TRACK_U = 0x4B,
  DPF_PLAYER_CMD_QUERY_CUR_TRACK_TF = 0x4C,
  DPF_PLAYER_CMD_QUERY_CUR_TRACK_FLASH = 0x4D,  
  DPF_PLAYER_CMD_QUERY_FILE_IN_DIR = 0x4E,
  DPF_PLAYER_CMD_QUERY_DIR_IN_DEV  = 0x4F,
  
} dpf_player_cmd_t;

typedef enum dpf_player_res_t {
  DPF_PLAYER_RES_INIT    = 0x3F,
  DPF_PLAYER_RES_ERROR   = 0x40,
  DPF_PLAYER_RES_REPLY   = 0x41,
} dpf_player_res_t;

bool dpf_player_init(void);
bool dpf_player_reset(void);
bool dpf_player_standby(void);
bool dpf_player_wakeup(void);
bool dpf_player_next_track(void);
bool dpf_player_prev_track(void);
bool dpf_player_play_track(uint16_t track);
bool dpf_player_inc_volume(void);
bool dpf_player_dec_volume(void);
uint8_t dpf_player_get_max_volume(void);
uint8_t dpf_player_get_min_volume(void);
bool dpf_player_set_volume(uint8_t volume);
bool dpf_player_set_eq(dpf_player_eq_type_t eq);
bool dpf_player_set_dev(dpf_player_dev_type_t dev);
bool dpf_player_set_audio_amp(bool enable, uint8_t gain);
bool dpf_player_set_dac(bool on);

bool dpf_player_play(void);
bool dpf_player_pause(void);
bool dpf_player_stop(void);

bool dpf_player_play_dir_file(uint8_t dir, uint8_t file);
bool dpf_player_play_mp3_dir_track(uint16_t track);
bool dpf_player_play_advert_dir_track(uint16_t track);
bool dpf_player_stop_advert(void);
bool dpf_player_play_huge_dir_file(uint8_t dir, uint16_t file);
bool dpf_player_repeat_dev(bool start);
bool dpf_player_repeat_dir(uint8_t dir);
bool dpf_player_repeat_track(uint16_t track);
bool dpf_player_random_dev(void);
bool dpf_player_repeat_current_track(bool on);

bool dpf_player_query_status(dpf_player_status_type_t * status);
bool dpf_player_query_volume(uint8_t * volume);
bool dpf_player_query_eq(dpf_player_eq_type_t * eq);


typedef void (*dpf_player_done_callback_handler_t)(void);
void dpf_player_set_done_callback(dpf_player_done_callback_handler_t callback);

#endif // NEO_IV_CLOCK_DPF_PLAYER_H 