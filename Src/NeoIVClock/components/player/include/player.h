#ifndef NEO_IV_CLOCK_PLAYER_H
#define NEO_IV_CLOCK_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct _player_seq_node_t
{
  uint8_t dir;
  uint8_t file;
} player_seq_node_t;

typedef enum _player_snd_dir_t
{
  PLAYER_SND_DIR_ALARM = 4,
  PLAYER_SND_DIR_EFFETS
} player_snd_dir_t;


void player_init(void);
void player_proc(void);
void player_on(void);
void player_off(void);
bool player_is_on(void);
void player_report_clk_and_temp(void);
void player_play_snd(player_snd_dir_t dir, uint8_t index);
uint8_t player_get_snd_cnt(player_snd_dir_t dir);
void player_stop_play(void);
bool player_is_playing(void);
void player_show(void);
uint8_t player_get_vol(void);
uint8_t player_get_min_vol(void);
uint8_t player_get_max_vol(void);
uint8_t player_inc_vol(void);
uint8_t player_set_vol(uint8_t vol);
void player_save_config(void);
void player_enter_powersave(void);
void player_leave_powersave(void);
#endif  // NEO_IV_CLOCK_PLAYER_H
