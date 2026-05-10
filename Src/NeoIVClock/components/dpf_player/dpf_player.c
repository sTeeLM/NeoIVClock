#include "dpf_player.h"
#include "logger.h"
#include "usart_wrapper.h"
#include "gpio_wrapper.h"
#include "delay.h"
#include <string.h>

#include "driver/gpio.h"
#include "driver/gpio_filter.h"

#define DPF_PLAYER_MAX_DIR_INDEX 99
#define DPF_PLAYER_MIN_DIR_INDEX 1

#define DPF_PLAYER_MAX_FILE_INDEX 255
#define DPF_PLAYER_MIN_FILE_INDEX 1

#define DPF_PLAYER_MAX_HUGE_DIR_INDEX 15
#define DPF_PLAYER_MIN_HUGE_DIR_INDEX 1

#define DPF_PLAYER_MAX_HUGE_FILE_INDEX 3000
#define DPF_PLAYER_MIN_HUGE_FILE_INDEX 1

#define DPF_PLAYER_MAX_TRACK_INDEX 2999
#define DPF_PLAYER_MIN_TRACK_INDEX 0

#define DPF_PLAYER_MAX_VOLUME  30
#define DPF_PLAYER_MIN_VOLUME  0

#define DPF_PLAYER_WAIT_CNT 40

#define DPF_PLAYER_MAX_AMP_GAIN 31
#define DPF_PLAYER_MIN_AMP_GAIN 0

#define DPF_PLAYER_DEV_MASK_U      1
#define DPF_PLAYER_DEV_MASK_TF     2
#define DPF_PLAYER_DEV_MASK_PC     4
#define DPF_PLAYER_DEV_MASK_FLASH  8

static const char * TAG = "DPF_PLAYER";
static usart_wrapper_dev_handle_t usart_dev_handle;
static dpf_player_msg_t dpf_player_msg;
static usart_wrapper_dev_handle_t usart_dev_handle;

dpf_player_done_callback_handler_t dpf_player_done_callback = NULL;

void dpf_player_dump_msg(const dpf_player_msg_t * msg)
{
  NEO_LOGD(TAG, "msg: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    msg->signature,
    msg->version,
    msg->length,
    msg->command,
    msg->feedback,  
    msg->dh,
    msg->dl,
    msg->checksum_hi,
    msg->checksum_lo,
    msg->end);
}

//
// for (int i=1; i<7; i++) {
//		checksum += cmd[i];
// uint16_t leftsum = 0x010000 - checksum
// cmd[7] = (uint8_t)(leftsum >> 8);
// cmd[8] = (uint8_t)(leftsum);
// 7E FF 06 03 00 00 01 FF E6 EF
static uint16_t dpf_player_cal_checksum(dpf_player_msg_t * msg)
{
  uint8_t * cmd = (uint8_t *)msg;
  uint16_t checksum = 0, leftsum;
  
  for (int i=1; i<7; i++) {
      checksum += cmd[i];
  }
  
  leftsum = 0x010000 - checksum;
  
  return leftsum; 
}

static void dpf_player_fill_checksum(dpf_player_msg_t * msg)
{   uint8_t * cmd = (uint8_t *)msg;
   uint16_t checksum = dpf_player_cal_checksum(msg);
   cmd[7] = (uint8_t)(checksum >> 8); 
   cmd[8] = (uint8_t)(checksum);   
}


static bool dpf_player_verify_checksum(dpf_player_msg_t* msg)
{
  uint8_t * cmd = (uint8_t *)msg;
  uint16_t checksum = dpf_player_cal_checksum(msg);
  if((cmd[7] != (uint8_t)(checksum >> 8)) ||
    (cmd[8] != (uint8_t)(checksum))) {
    return false;
  }
  return true;
}

static bool dpf_player_wait_response(dpf_player_msg_t * msg)
{
  uint8_t * p;
  uint8_t buf, index;
  ssize_t err;
  
  index = 0;
  p = (uint8_t *)msg;

  while((err = usart_wrapper_read(&usart_dev_handle, &buf, 1)) == 1) {
    if(buf == 0x7E && index != 0 && (p[0] != 0x7E || p[1] != 0xFF)) {
      index = 0;
    }
    p[index ++] = buf;
    if(index >= sizeof(dpf_player_msg_t)) {
      break;
    }
  }
  
  if(err != 1 || index != sizeof(dpf_player_msg_t))
    return false;
  
  NEO_LOGD(TAG, "dpf_player_wait_response receive new msg");
  dpf_player_dump_msg(msg);
  
  if(!dpf_player_verify_checksum(msg)) {
    NEO_LOGW(TAG, "checksum error");
    return false;
  }
  return true;
}

static bool dpf_player_send_message(dpf_player_msg_t * msg)
{
  NEO_LOGD(TAG, "dpf_player_send_message send new msg");
  dpf_player_fill_checksum(msg);
  dpf_player_dump_msg(msg);
  if(usart_wrapper_write(&usart_dev_handle, (const uint8_t *)msg, sizeof(dpf_player_msg_t)) == sizeof(dpf_player_msg_t))
  {
    if(msg->feedback) {
      memset(msg, 0, sizeof(dpf_player_msg_t));
      if(dpf_player_wait_response(msg)) {
        if(msg->command != DPF_PLAYER_RES_REPLY) {
          NEO_LOGE(TAG, "dpf_player_send_message failed!");
          return false;
        }
      }
    }
  } else {
    return false;
  }
  return true;
}

static void dpf_player_fill_msg(dpf_player_msg_t * msg)
{
  msg->signature = 0x7E;
  msg->version   = 0xFF;   
  msg->length    = 6;
  msg->command   = 0;
  msg->feedback  = 1;
  msg->dh        = 0;
  msg->dl        = 0; 
  msg->checksum_hi = 0;
  msg->checksum_lo = 0;  
  msg->end       = 0xEF;    
}


bool dpf_player_reset(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_RESET;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_standby(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_STANDBY;
  dpf_player_msg.feedback = 0;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_wakeup(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_NORMAL;
  dpf_player_msg.feedback = 0;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_next_track(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_NEXT_TRACK;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_prev_track(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PREV_TRACK;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_play_track(uint16_t track)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PLAY_TRACK;
  if(track > DPF_PLAYER_MAX_TRACK_INDEX) track = DPF_PLAYER_MAX_TRACK_INDEX;
  dpf_player_msg.dh = (track & 0xFF00) >> 8;
  dpf_player_msg.dl = track & 0xFF;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_inc_volume(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_INC_VOL;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_dec_volume(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_DEC_VOL;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_set_volume(uint8_t volume)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_SET_VOL;
  if(volume > DPF_PLAYER_MAX_VOLUME) volume = DPF_PLAYER_MAX_VOLUME;
  dpf_player_msg.dl = volume;
  return dpf_player_send_message(&dpf_player_msg);
}

uint8_t dpf_player_get_max_volume(void)
{
  return DPF_PLAYER_MAX_VOLUME;
}

uint8_t dpf_player_get_min_volume(void)
{
  return DPF_PLAYER_MIN_VOLUME;
}

bool dpf_player_set_eq(dpf_player_eq_type_t eq)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_SET_EQ;
  dpf_player_msg.dl = eq;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_set_dev(dpf_player_dev_type_t dev)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_SET_DEV;
  dpf_player_msg.dl = dev;
  return dpf_player_send_message(&dpf_player_msg);
}


bool dpf_player_play(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PLAY;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_pause(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PAUSE;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_stop(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_STOP;
  return dpf_player_send_message(&dpf_player_msg);
}


bool dpf_player_play_dir_file(uint8_t dir, uint8_t file)
{
  NEO_LOGD(TAG, "dpf_player_play_dir_file dir = %d file = %d", dir, file);
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PLAY_DIR_FILE;
  if(dir > DPF_PLAYER_MAX_DIR_INDEX) dir = DPF_PLAYER_MAX_DIR_INDEX;
  if(dir < DPF_PLAYER_MIN_DIR_INDEX) dir = DPF_PLAYER_MIN_DIR_INDEX;  
  // 如下检查不用进行
  // if(file > DPF_PLAYER_MAX_FILE_INDEX) file = DPF_PLAYER_MAX_FILE_INDEX;
  // if(file < DPF_PLAYER_MIN_FILE_INDEX) file = DPF_PLAYER_MIN_FILE_INDEX;  
  dpf_player_msg.dh = dir;
  dpf_player_msg.dl = file;  
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_play_mp3_dir_track(uint16_t track)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PLAY_DIR_MP3_TRACK; 
  dpf_player_msg.dh = (track & 0xFF) >> 8;
  dpf_player_msg.dl = track & 0xFF;  
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_play_advert_dir_track(uint16_t track)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PLAY_DIR_ADVERT_TRACK; 
  dpf_player_msg.dh = (track & 0xFF) >> 8;
  dpf_player_msg.dl = track & 0xFF;  
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_stop_advert(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_STOP_ADVERT;  
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_play_huge_dir_file(uint8_t dir, uint16_t file)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_PLAY_HUGE_DIR;
  if(dir > DPF_PLAYER_MAX_HUGE_DIR_INDEX) dir = DPF_PLAYER_MAX_HUGE_DIR_INDEX;
  if(dir < DPF_PLAYER_MIN_HUGE_DIR_INDEX) dir = DPF_PLAYER_MIN_HUGE_DIR_INDEX;  
  if(file > DPF_PLAYER_MAX_HUGE_FILE_INDEX) file = DPF_PLAYER_MAX_HUGE_FILE_INDEX;
  if(file < DPF_PLAYER_MIN_HUGE_FILE_INDEX) file = DPF_PLAYER_MIN_HUGE_FILE_INDEX;  
  dpf_player_msg.dh = (dir << 4) | ((file & 0xF00) >> 8);
  dpf_player_msg.dl = file & 0xFF;  
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_set_audio_amp(bool enable, uint8_t gain)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_AUDIO_AMP;
  dpf_player_msg.dh = enable ? 1 : 0;
  if(gain > DPF_PLAYER_MAX_AMP_GAIN) gain = DPF_PLAYER_MAX_AMP_GAIN;
  dpf_player_msg.dl = gain;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_repeat_dev(bool start)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_REPEAT_DEV;
  dpf_player_msg.dl = start ? 1 : 0;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_repeat_dir(uint8_t dir)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_REPEAT_DIR;
  dpf_player_msg.dl = dir;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_repeat_track(uint16_t track)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_REPEAT_TRACK;
  dpf_player_msg.dl = track & 0xFF;
  dpf_player_msg.dh = (track & 0xFF00) >> 8;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_random_dev(void)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_RANDOM_DEV;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_repeat_current_track(bool On)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_REPEAT_CUR_TRACK;
  dpf_player_msg.dl = On ? 0 : 1;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_set_dac(bool On)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_SET_DAC;
  dpf_player_msg.dl = On ? 0 : 1;
  return dpf_player_send_message(&dpf_player_msg);
}

bool dpf_player_query_status(dpf_player_status_type_t * status)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_QUERY_STATUS;
  dpf_player_msg.feedback = 0;
  if(dpf_player_send_message(&dpf_player_msg)) {
    if(dpf_player_wait_response(&dpf_player_msg)) {
      if(dpf_player_msg.command == DPF_PLAYER_CMD_QUERY_STATUS) {
        *status = dpf_player_msg.dl;
        return true;
      }
    }
  }
  return false;
}

bool dpf_player_query_volume(uint8_t * Volume)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_QUERY_VOL;
  dpf_player_msg.feedback = 0;
  if(dpf_player_send_message(&dpf_player_msg)) {
    if(dpf_player_wait_response(&dpf_player_msg)) {
      if(dpf_player_msg.command == DPF_PLAYER_CMD_QUERY_VOL) {
        *Volume = dpf_player_msg.dl;
        return true;
      }
    }
  }
  return false;
}

bool dpf_player_query_eq(dpf_player_eq_type_t * eq)
{
  dpf_player_fill_msg(&dpf_player_msg);
  dpf_player_msg.command   = DPF_PLAYER_CMD_QUERY_EQ;
  dpf_player_msg.feedback = 0;
  if(dpf_player_send_message(&dpf_player_msg)) {
    if(dpf_player_wait_response(&dpf_player_msg)) {
      if(dpf_player_msg.command == DPF_PLAYER_CMD_QUERY_EQ) {
        *eq = dpf_player_msg.dl;
        return true;
      }
    }
  }
  return false;
}

static void IRAM_ATTR dpf_player_isr_handler (void* param)
{
  NEO_EARLY_LOGD(TAG, "dpf_player_isr_handler triggered!");
  if(dpf_player_done_callback) {
    dpf_player_done_callback();
  }
}

static void dpf_player_pin_init(void)
{
  
  uart_config_t uart_config = {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };

  usart_wrapper_dev_add(&usart_dev_handle, UART_NUM_1, DPF_PLAYER_TX_GPIO_PIN, DPF_PLAYER_RX_GPIO_PIN, &uart_config);

  ESP_ERROR_CHECK(gpio_intr_enable(DPF_PLAYER_BSY_GPIO_PIN));
  ESP_ERROR_CHECK(gpio_set_intr_type(DPF_PLAYER_BSY_GPIO_PIN, GPIO_INTR_POSEDGE));
  ESP_ERROR_CHECK(gpio_isr_handler_add(DPF_PLAYER_BSY_GPIO_PIN, dpf_player_isr_handler, NULL));
}

void dpf_player_set_done_callback(dpf_player_done_callback_handler_t callback)
{
  dpf_player_done_callback = callback;
}

bool dpf_player_init(void)
{
  uint32_t wait_cnt = 0;

  NEO_LOGI("DPF_PLAYER", "init");
  
  dpf_player_pin_init();
  
  if(!dpf_player_reset()) {
    NEO_LOGE(TAG, "dpf_player_init Error!");
    return false;
  } else {
    while(!dpf_player_wait_response(&dpf_player_msg)){
      delay_ms(100);
      wait_cnt ++;
      NEO_LOGD(TAG, "wait for dpf_player response... cnt = %d", wait_cnt);
      if(wait_cnt > DPF_PLAYER_WAIT_CNT) {
        NEO_LOGE(TAG, "dpf_player_init Timeout!");
        return false;
      }
    }
    NEO_LOGI(TAG, "DPF Dev Online: U[%s]|TF[%s]|PC[%s]|Flash[%s]",
      dpf_player_msg.dl & DPF_PLAYER_DEV_MASK_U ? "Yes" : "No",
      dpf_player_msg.dl & DPF_PLAYER_DEV_MASK_TF ? "Yes" : "No",
      dpf_player_msg.dl & DPF_PLAYER_DEV_MASK_PC ? "Yes" : "No", 
      dpf_player_msg.dl & DPF_PLAYER_DEV_MASK_FLASH ? "Yes" : "No"      
    );
    
    if(!(dpf_player_msg.dl & DPF_PLAYER_DEV_MASK_TF)) {
      NEO_LOGE(TAG, "dpf_player_init: no TF found!");
      return false;
    }
  }

  delay_ms(100);
  
  return true;
}