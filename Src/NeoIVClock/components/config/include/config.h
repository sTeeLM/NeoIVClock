#ifndef NEO_IV_CLOCK_CONFIG_H
#define NEO_IV_CLOCK_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _config_type_t {
  CONFIG_TYPE_UINT8,
  CONFIG_TYPE_UINT16, 
  CONFIG_TYPE_UINT32,
  CONFIG_TYPE_BLOB,
  CONFIG_TYPE_CNT
} config_type_t;

typedef struct _config_blob_t
{
  uint16_t len;
  const uint8_t* body;
} config_blob_t;

typedef union _config_val_t
{
    uint8_t  val8;
    uint16_t val16;
    uint32_t val32;
    config_blob_t valblob;
} config_val_t;

typedef struct _config_slot_t
{
  const char * name;
  config_type_t type;
  config_val_t  default_val;
} config_slot_t;



void config_init(void);
uint32_t config_read_int(const char * name);
bool config_read(const char * name, config_val_t * val);
void config_write(const char * name, const config_val_t * val);

#endif // NEO_IV_CLOCK_CONFIG_H