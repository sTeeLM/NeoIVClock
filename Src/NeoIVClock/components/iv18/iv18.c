#include "iv18.h"
#include "logger.h"
#include "logger.h"
#include "delay.h"
#include "gpio_wrapper.h"

#define IV18_DP    0x40
#define IV18_BLINK 0x20000

static uint8_t  iv18_cur_index;
static uint16_t  iv18_cur_loop;
static uint32_t iv18_data[9];


static const char * TAG = "IV18";

/*
Out0  G9
Out1  G1
Out2  G3
Out3  G5
Out4  G8
Out5  G7
Out6  G6
Out7  G4
Out8  G2
Out9  a
Out10 dp
Out11 d
Out12 c
Out13 e
Out14 g
Out15 b
Out16 f

   a
f     b
   g
e     c
   d    dp
*/

/*
   -------------|b|scan code|8 dig code
   000000000000000|000000000|0d000000
   b: blink
*/



static const uint16_t iv18_scan_code[9] = 
{
  0x100, // 100000000 
  0x010, // 000010000 
  0x008, // 000001000
  0x004, // 000000100
  0x020, // 000100000
  0x002, // 000000010
  0x040, // 001000000
  0x001, // 000000001  
  0x080, // 010000000
};

static const uint8_t iv18_dig_special[] =
{
        /* aPdcegbf */
   0x00, //00000000  BLANK
   0x87, //10000111  o  degree assume ascii 1!! 
   0x82, //10000010  progress 0,ab
   0x06, //00000110  progress 1,bg
   0x0c, //00001100  progress 2,ge 
   0x28, //00101000  progress 3,ed
   0x30, //00110000  progress 4,dc
   0x14, //00010100  progress 5,cg
   0x05, //00000101  progress 6,gf
   0x81, //10000001  progress 7,fa
};

static const uint8_t iv18_dig_ascii[] =
{
        /* aPdcegbf */
   0x04, //00000100  -  ascii: 0x2D
   0x00, //00000000  .
   0x00, //00000000  /
   0xBB, //10111011  0    
   0x12, //00010010  1
   0xAE, //10101110  2
   0xB6, //10110110  3
   0x17, //00010111  4
   0xB5, //10110101  5
   0xBD, //10111101  6
   0x93, //10010011  7
   0xBF, //10111111  8
   0xB7, //10110111  9
   0x00, //00000000  :
   0x00, //00000000  ;
   0x28, //00101000  <
   0x24, //00100100  =  
   0x30, //00110000  >
   0x00, //00000000  ?
   0x00, //00000000  @
   
        /* aPdcegbf */   
   0x9F, //10011111  A
   0x3D, //00111101  B
   0xA9, //10101001  C
   0x3E, //00111110  D
   0xAD, //10101101  E
   0x8D, //10001101  F
   0xB9, //10111001  G
   0x1F, //00011111  H
   0x09, //00001001  I
   0x32, //00110010  J 
   0x00, //00000000  K
   0x29, //00101001  L
   0x15, //00010101  M
   0x9B, //10011011  N
   0x3C, //00111100  O
   0x8F, //10001111  P
   0x00, //10010111  Q
   0x0C, //00001100  R
   0xB5, //10110101  S == 5
   0x2D, //00101101  T
   0x3B, //00111011  U
   0x70, //01110000  V
   0x0E, //00001110  W
   0x00, //00000000  X
   0x0F, //00001111  Y
   0x00, //00000000  Z ascii 0x5A
};

static void iv18_show_data(uint32_t data)
{
  uint8_t i;
  for(i = 0 ; i < 17 ; i ++) {   
    gpio_wrapper_set_level(IV18_DIN_GPIO_PIN, (data & 0x1) ? 1 : 0);
    gpio_wrapper_set_level(IV18_CLK_GPIO_PIN, 1);
    gpio_wrapper_set_level(IV18_CLK_GPIO_PIN, 0);
    data >>= 1;
  }
  gpio_wrapper_set_level(IV18_LOAD_GPIO_PIN, 1);
  gpio_wrapper_set_level(IV18_LOAD_GPIO_PIN, 0);
};

void iv18_scan(void)
{
  uint32_t empty;
  if(iv18_data[ iv18_cur_index] & IV18_BLINK) {
    if( iv18_cur_loop % 128 < 64) {
      iv18_load_data( iv18_cur_index);
    } else {
      empty = iv18_data[ iv18_cur_index];
      empty &= 0x1FF40; // keep scan code and dp
      iv18_show_data(empty);
    }
  } else {
    iv18_load_data( iv18_cur_index);
  }
   iv18_cur_index ++;
   iv18_cur_index %= 9;
  if( iv18_cur_index == 8)
     iv18_cur_loop ++;
}

static void iv18_dev_init(void)
{
  uint8_t i;
  
  memset(iv18_data, 0, sizeof(iv18_data));
  
  for(i = 0 ; i < 9 ; i ++)
    iv18_set_dig(i, 0);
  
  iv18_set_dig(2, 'H');
  iv18_load_data(2);
  iv18_set_dig(3, 'E'); 
  iv18_load_data(3);  
  iv18_set_dig(4, 'L');
  iv18_load_data(4);   
  iv18_set_dig(5, 'L'); 
  iv18_load_data(5);   
  iv18_set_dig(6, 'O'); 
  iv18_load_data(6);   
  
  iv18_cur_index = 0;
  iv18_cur_loop = 0;
}


void iv18_init(void)
{

  NEO_LOGI(TAG, "init\n");
  
  gpio_wrapper_set_level(IV18_EN_GPIO_PIN, 0);
  gpio_wrapper_set_level(IV18_CLK_GPIO_PIN, 0);
  gpio_wrapper_set_level(IV18_DIN_GPIO_PIN, 0);
  gpio_wrapper_set_level(IV18_LOAD_GPIO_PIN, 0);
  gpio_wrapper_set_level(IV18_BLANK_GPIO_PIN, 0);

  iv18_enable(true);
  iv18_set_brightness(100);

  iv18_dev_init();
}

void iv18_set_dig(uint8_t index, uint8_t code)
{
  uint32_t mask;
  if(index > 8)
    index = 8;

  mask = iv18_data[index] & (IV18_BLINK | IV18_DP);
  
  if(code >= 0x2D) {
    code = code - 0x2D;
    if(code >= sizeof(iv18_dig_ascii))
      code = sizeof(iv18_dig_ascii) - 1;
    iv18_data[index] = iv18_scan_code[index];
    iv18_data[index] <<= 8;
    iv18_data[index] |= iv18_dig_ascii[code];
    iv18_data[index] |= mask;    
  } else if(code <= IV18_SPECIAL_MAX) {
    if(code >= sizeof(iv18_dig_special))
      code = sizeof(iv18_dig_special) - 1;
    iv18_data[index] = iv18_scan_code[index];
    iv18_data[index] <<= 8;
    iv18_data[index] |= iv18_dig_special[code];
    iv18_data[index] |= mask;     
  } 
}

void iv18_set_dp(uint8_t index)
{
  if(index > 8)
    index = 8;
  
  iv18_data[index] |= IV18_DP;
}

void iv18_clr_dp(uint8_t index)
{
  if(index > 8)
    index = 8;
  
  iv18_data[index] &= ~IV18_DP;  
}

void iv18_set_blink(uint8_t index)
{
  if(index > 8)
    index = 8;
  
  iv18_data[index] |= IV18_BLINK;
}

void iv18_clr_blink(uint8_t index)
{
  if(index > 8)
    index = 8;
  
  iv18_data[index] &= ~IV18_BLINK;
}

void iv18_clr(void)
{
  uint8_t i;
  for(i = 0 ; i < 9 ; i++) {
    iv18_data[i] = iv18_scan_code[i];
    iv18_data[i] <<= 8; 
  }
}

void iv18_clr_data(uint8_t index)
{
  if(index > 8)
    index = 8;
  iv18_data[index] = iv18_scan_code[index];
  iv18_data[index] <<= 8;
}

void iv18_load_data(uint8_t index)
{
  if(index > 8)
    index = 8;
  iv18_show_data(iv18_data[index]);
}

void iv18_enable(bool enable)
{
  gpio_wrapper_set_level(IV18_EN_GPIO_PIN, enable ? 1 : 0);
}

// level: 0~100
void iv18_set_brightness(uint8_t level)
{
  // do nothing, IV18 has no brightness control
  if(level > 100)
    level = 100;

  if(level == 100)
    gpio_wrapper_set_level(IV18_BLANK_GPIO_PIN, 0);
  else if(level == 0)
    gpio_wrapper_set_level(IV18_BLANK_GPIO_PIN, 1);
  else {

  }
}