#ifndef NEO_IV_CLOCK_ENS160_H
#define NEO_IV_CLOCK_ENS160_H

#include <stdint.h>
#include <stdbool.h>

void ens160_init(void);

typedef enum _ens160_mode_t
{
  ENS160_MODE_SLEEP = 0,
  ENS160_MODE_IDLE = 1,
  ENS160_MODE_STANDARD = 2,
  ENS160_MODE_RESET = 0xF0
} ens160_mode_t;

typedef struct _ens160_config_t
{
  bool int_polarity_positive; // INTn pin polarity: 0: active low, 1: active high
  bool int_drive_push_pull;   // INTn pin drive: 0: Open drain 1: Push / Pull
  bool int_gpr; // INTn pin asserted when new data is presented in the General Purpose Read Registers
  bool int_data; // INTn pin asserted when new data is presented in the DATA_XXX Registers
  bool int_enable; // 0: disable all interrupt, 1: enable interrupt according to above settings
} __attribute__((packed)) ens160_config_t;

void ens160_set_config(const ens160_config_t * config);

typedef struct _ens160_data_t
{
  float tvoc;
  float eco2;
  uint16_t iaq;
} __attribute__((packed)) ens160_data_t;

bool ens160_read_data(ens160_data_t * data);

void ens160_clr_gpt(void);

#define ENS160_STATUS_NEWGPR_BIT (1 << 0)
#define ENS160_STATUS_NEWDAT_BIT (1 << 1)
#define ENS160_STATUS_STATER_BIT (1 << 6)
#define ENS160_STATUS_STATAS_BIT (1 << 7)
#define ENS160_STATUS_MASK(status) (((status) & 0x0C) >> 2)

/*
  Status
  0: Normal operation
  1: Warm-Up phase
  2: Initial Start-Up phase
  3: Invalid output
*/
typedef enum _ens160_status_t
{
  ENS160_STATUS_NORMAL  = 0,
  ENS160_STATUS_WARMUP  = 1,
  ENS160_STATUS_STARTUP = 2,
  ENS160_STATUS_INVALID = 3
} ens160_status_t;

uint8_t ens160_get_status(void);

void ens160_get_fw_version(uint8_t * major, uint8_t * minor, uint8_t * release);

void ens160_compensate(float temp, float humidity);

#endif // NEO_IV_CLOCK_ENS160_H
