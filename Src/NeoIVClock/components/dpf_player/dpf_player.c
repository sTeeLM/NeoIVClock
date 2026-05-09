#include "dpf_player.h"
#include "logger.h"
#include "usart_wrapper.h"
#include "delay.h"
#include <string.h>



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

#define DPF_PLAYER_MAX_RESET_WAIT_MS 4000

#define DPF_PLAYER_MAX_AMP_GAIN 31
#define DPF_PLAYER_MIN_AMP_GAIN 0

#define DPF_PLAYER_DEV_MASK_U      1
#define DPF_PLAYER_DEV_MASK_TF     2
#define DPF_PLAYER_DEV_MASK_PC     4
#define DPF_PLAYER_DEV_MASK_FLASH  8

static const char * TAG = "DPF_PLAYER";

static dpf_player_msg_t dpf_player_msg;

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
    msg->checksum_low,
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
  uint8_t i;
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
  while( (err = BSP_USART2_Receive(&buf, 1) ) == BSP_ERROR_NONE) {
    if(buf == 0x7E && index != 0 && (p[0] != 0x7E || p[1] != 0xFF)) {
      index = 0;
    }
    p[index ++] = buf;
    if(index >= sizeof(dpf_player_msg_t)) {
      break;
  }
  
  if(err != BSP_ERROR_NONE || index != sizeof(dpf_player_msg_t))
    return false;
  
  NEO_LOGD(TAG, "dpf_player_wait_response receive new msg");
  dpf_player_dump_msg(msg);
  
  if(!dpf_player_verify_checksum(msg)) {
    NEO_LOGE(TAG, "checksum error");
    return false;
  }
  return true;
}

static bool dpf_player_send_message(dpf_player_msg_t * msg)
{
  NEO_LOGD(TAG, "dpf_player_send_message send new msg");
  dpf_player_fill_checksum(msg);
  dpf_player_dump_msg(msg);
  if(BSP_USART2_Transmit((uint8_t *)msg, sizeof(dpf_player_msg_t)) == BSP_ERROR_NONE)
  {
    if(msg->Feedback) {
      memset(msg, 0, sizeof(dpf_player_msg_t));
      if(dpf_player_wait_response(msg)) {
        if(msg->command != DPF_PLAYER_RES_REPLY) {
          NEO_LOGE(TAG, "BSP_MP3_Send_Message failed!");
          return false;
        }
      }
    }
  } else {
    return false;
  }
  return true;
}

static void BSP_MP3_Fill_Msg(BSP_MP3_Msg_Type * Msg)
{
  Msg->Signature = 0x7E;
  Msg->Version   = 0xFF;   
  Msg->Length    = 6;
  Msg->Command   = 0;
  Msg->Feedback  = 1;
  Msg->DH  = 0;
  Msg->DL  = 0; 
  Msg->ChecksumHi = 0;
  Msg->ChecksumLow = 0;  
  Msg->End        = 0xEF;    
}

void dpf_player_init(void)
{
    NEO_LOGI("DPF_PLAYER", "init");
}


bool BSP_MP3_Reset(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_RESET;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Standby(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_STANDBY;
  BSP_MP3_Cmd.Feedback = 0;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Wakeup(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_NORMAL;
  BSP_MP3_Cmd.Feedback = 0;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Next_Track(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_NEXT_TRACK;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Prev_Track(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PREV_TRACK;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Play_Track(uint16_t Track)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PLAY_TRACK;
  if(Track > BSP_MP3_MAX_TRACK_INDEX) Track = BSP_MP3_MAX_TRACK_INDEX;
  BSP_MP3_Cmd.DH = (Track & 0xFF00) >> 8;
  BSP_MP3_Cmd.DL = Track & 0xFF;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Inc_Volume(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_INC_VOL;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Dec_Volume(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_DEC_VOL;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Set_Volume(uint8_t Volume)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_SET_VOL;
  if(Volume > BSP_MP3_MAX_VOLUME) Volume = BSP_MP3_MAX_VOLUME;
  BSP_MP3_Cmd.DL = Volume;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

uint8_t BSP_MP3_Get_Max_Volume(void)
{
  return BSP_MP3_MAX_VOLUME;
}

uint8_t BSP_MP3_Get_Min_Volume(void)
{
  return BSP_MP3_MIN_VOLUME;
}

bool BSP_MP3_Set_Eq(BSP_MP3_Eq_Type Eq)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_SET_EQ;
  BSP_MP3_Cmd.DL = Eq;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Set_Dev(BSP_MP3_Dev_Type Dev)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_SET_DEV;
  BSP_MP3_Cmd.DL = Dev;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}


bool BSP_MP3_Play(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PLAY;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Pause(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PAUSE;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Stop(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_STOP;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}


bool BSP_MP3_Play_Dir_File(uint8_t Dir, uint8_t File)
{
  IVDBG("BSP_MP3_Play_Dir_File Dir = %d File = %d", Dir, File);
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PlAY_DIR_FILE;
  if(Dir > BSP_MP3_MAX_DIR_INDEX) Dir = BSP_MP3_MAX_DIR_INDEX;
  if(Dir < BSP_MP3_MIN_DIR_INDEX) Dir = BSP_MP3_MIN_DIR_INDEX;  
  if(File > BSP_MP3_MAX_FILE_INDEX) File = BSP_MP3_MAX_FILE_INDEX;
  if(File < BSP_MP3_MIN_FILE_INDEX) File = BSP_MP3_MIN_FILE_INDEX;  
  BSP_MP3_Cmd.DH = Dir;
  BSP_MP3_Cmd.DL = File;  
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Play_MP3_Dir_File(uint16_t File)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PlAY_DIR_FILE; 
  BSP_MP3_Cmd.DH = (File & 0xFF) >> 8;
  BSP_MP3_Cmd.DL = File & 0xFF;  
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Play_ADVERT_Dir_File(uint16_t File)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PLAY_DIR_ADVERT; 
  BSP_MP3_Cmd.DH = (File & 0xFF) >> 8;
  BSP_MP3_Cmd.DL = File & 0xFF;  
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Stop_Advert(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_STOP_ADVERT;  
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Play_Huge_Dir_File(uint8_t Dir, uint16_t File)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_PLAY_HUGE_DIR;
  if(Dir > BSP_MP3_MAX_HUGE_DIR_INDEX) Dir = BSP_MP3_MAX_HUGE_DIR_INDEX;
  if(Dir < BSP_MP3_MIN_HUGE_DIR_INDEX) Dir = BSP_MP3_MIN_HUGE_DIR_INDEX;  
  if(File > BSP_MP3_MAX_HUGE_FILE_INDEX) File = BSP_MP3_MAX_HUGE_FILE_INDEX;
  if(File < BSP_MP3_MIN_HUGE_FILE_INDEX) File = BSP_MP3_MIN_HUGE_FILE_INDEX;  
  BSP_MP3_Cmd.DH = (Dir << 4) | ((File & 0xF00) >> 8);
  BSP_MP3_Cmd.DL = File & 0xFF;  
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Set_Audio_AMP(bool Enable, uint8_t Gain)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_AUDIO_AMP;
  BSP_MP3_Cmd.DH = Enable ? 1 : 0;
  if(Gain > BSP_MP3_MAX_AMP_GAIN) Gain = BSP_MP3_MAX_AMP_GAIN;
  BSP_MP3_Cmd.DL = Gain;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Repeat_Dev(bool Start)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_REPEAT_DEV;
  BSP_MP3_Cmd.DL = Start ? 1 : 0;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Repeat_Dir(uint8_t Dir)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_REPEAT_DIR;
  BSP_MP3_Cmd.DL = Dir;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Repeat_File(uint16_t File)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_REPEAT_TRACK;
  BSP_MP3_Cmd.DL = File & 0xFF;
  BSP_MP3_Cmd.DH = (File & 0xFF00) >> 8;  
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Random_Dev(void)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_RANDOM_DEV;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Repeat_Current_Track(bool On)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_REPEAT_CUR_TRACK;
  BSP_MP3_Cmd.DL = On ? 0 : 1;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Set_DAC(bool On)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_SET_DAC;
  BSP_MP3_Cmd.DL = On ? 0 : 1;
  return BSP_MP3_Send_Message(&BSP_MP3_Cmd);
}

bool BSP_MP3_Query_Status(BSP_MP3_Status_Type * Status)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_QUERY_STATUS;
  BSP_MP3_Cmd.Feedback = 0;
  if(BSP_MP3_Send_Message(&BSP_MP3_Cmd)) {
    if(BSP_MP3_Wait_Response(&BSP_MP3_Cmd)) {
      if(BSP_MP3_Cmd.Command == BSP_MP3_CMD_QUERY_STATUS) {
        *Status = BSP_MP3_Cmd.DL;
        return TRUE;
      }
    }
  }
  return FALSE;
}

bool BSP_MP3_Query_Volume(uint8_t * Volume)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_QUERY_VOL;
  BSP_MP3_Cmd.Feedback = 0;
  if(BSP_MP3_Send_Message(&BSP_MP3_Cmd)) {
    if(BSP_MP3_Wait_Response(&BSP_MP3_Cmd)) {
      if(BSP_MP3_Cmd.Command == BSP_MP3_CMD_QUERY_VOL) {
        *Volume = BSP_MP3_Cmd.DL;
        return TRUE;
      }
    }
  }
  return FALSE;
}

bool BSP_MP3_Query_Eq(BSP_MP3_Eq_Type * Eq)
{
  BSP_MP3_Fill_Msg(&BSP_MP3_Cmd);
  BSP_MP3_Cmd.Command   = BSP_MP3_CMD_QUERY_EQ;
  BSP_MP3_Cmd.Feedback = 0;
  if(BSP_MP3_Send_Message(&BSP_MP3_Cmd)) {
    if(BSP_MP3_Wait_Response(&BSP_MP3_Cmd)) {
      if(BSP_MP3_Cmd.Command == BSP_MP3_CMD_QUERY_EQ) {
        *Eq = BSP_MP3_Cmd.DL;
        return TRUE;
      }
    }
  }
  return FALSE;
}

static void BSP_MP3_Pin_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : INT_KEY_MOD_Pin */
  GPIO_InitStruct.Pin = INT_MP3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(INT_MP3_GPIO_Port, &GPIO_InitStruct);
  
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(INT_MP3_EXTI_IRQn, BSP_MP3_IRQ_PRIORITY, BSP_MP3_IRQ_SUB_PRIORITY);
  HAL_NVIC_EnableIRQ(INT_MP3_EXTI_IRQn);  
}

BSP_Error_Type BSP_MP3_Init(void)
{
  uint32_t ResetSaveMS;
  uint8_t temp;
  
  BSP_MP3_Pin_Init();
  
  if(!BSP_MP3_Reset()) {
    IVERR("BSP_MP3_Reset Error!");
    return BSP_ERROR_INTERNAL;
  } else {
    ResetSaveMS = HAL_GetTick();
    while(!BSP_MP3_Wait_Response(&BSP_MP3_Cmd)){
      if(((uint32_t)(HAL_GetTick() - ResetSaveMS)) > BSP_MP3_MAX_RESET_WAIT_MS) {
        IVERR("BSP_MP3_Init: wait online msg time out!");
        return BSP_ERROR_INTERNAL; 
      }
    }
    IVINFO("MP3 Dev Online: U[%d]|TF[%d]|PC[%d]|Flash[%d]",
      BSP_MP3_Cmd.DL & BSP_MP3_DEV_MASK_U ? 1 : 0,
      BSP_MP3_Cmd.DL & BSP_MP3_DEV_MASK_TF ? 1 : 0,
      BSP_MP3_Cmd.DL & BSP_MP3_DEV_MASK_PC ? 1 : 0, 
      BSP_MP3_Cmd.DL & BSP_MP3_DEV_MASK_FLASH ? 1 : 0      
    );
    if(!(BSP_MP3_Cmd.DL & BSP_MP3_DEV_MASK_TF)) {
      IVERR("BSP_MP3_Init: no TF found!");
      return BSP_ERROR_INTERNAL;
    }
  }

//  if(!BSP_MP3_Standby()) {
//    IVERR("BSP_MP3_Init: can not into standby");
//    return BSP_ERROR_INTERNAL;
//  } 
//  
  delay_ms(100);
  
  return BSP_ERROR_NONE;
}