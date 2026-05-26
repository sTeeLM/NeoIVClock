#include "timer.h"
#include "logger.h"
#include "config.h"
#include "player.h"

static const char * TAG = "TIMER";

static uint8_t timer_snd_index;

void timer_init(void)
{
  NEO_LOGI(TAG, "init");

  timer_snd_index = config_read_int("timer_snd");
}


uint8_t timer_get_snd(void)
{
  return timer_snd_index;
}
uint8_t timer_inc_snd(void)
{
  uint8_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_EFFETS);
  timer_snd_index = (timer_snd_index + 1) % snd_cnt;
  return timer_snd_index;
}
uint8_t timer_dec_snd(void)
{
  uint8_t snd_cnt = player_get_snd_cnt(PLAYER_SND_DIR_EFFETS);
  timer_snd_index = (timer_snd_index + (snd_cnt - 1)) % snd_cnt;
  return timer_snd_index;
}
void timer_save_config(void)
{
  config_write_int("timer_snd", timer_snd_index);
}