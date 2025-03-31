#include "config.h"
#include "rom.h"
#include "logger.h"
#include "button.h"

#include <string.h>

static const char * TAG = "config";

static const uint8_t alarm_blob0[] = {0,0,0,0};

static const config_slot_t _slot[] = {
  {"time_12", CONFIG_TYPE_UINT8,   {.val8 = 0}},
  {"acc_en",CONFIG_TYPE_UINT8,     {.val8 = 1}},
  {"bp_en",CONFIG_TYPE_UINT8,      {.val8 = 1}},
  {"alm1_en", CONFIG_TYPE_UINT8,   {.val8 = 1}}, 
  {"alm1_begin", CONFIG_TYPE_UINT8, {.val8 = 8}}, 
  {"alm1_end", CONFIG_TYPE_UINT8,   {.val8 = 22}},  
  {"bat_84", CONFIG_TYPE_UINT16,   {.val16 = 3725}},
  {"bat_0", CONFIG_TYPE_UINT16,   {.val16 = 217}},   
  {"lt_0", CONFIG_TYPE_UINT16,     {.val16 = 354}},
  {"lt_100", CONFIG_TYPE_UINT16,   {.val16 = 920}}, 
  {"mon_lt_en", CONFIG_TYPE_UINT8, {.val8 = 1}},
  {"temp_cen", CONFIG_TYPE_UINT8,  {.val8 = 1}}, 
  {"power_timeo", CONFIG_TYPE_UINT8,  {.val8 = 15}},
  {"tmr_snd", CONFIG_TYPE_UINT8, {.val8 = 0}},
  {"ply_vol", CONFIG_TYPE_UINT8, {.val8 = 10}},  
  {"alm00_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}}, 
  {"alm01_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}},
  {"alm02_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}}, 
  {"alm03_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}},
  {"alm04_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}}, 
  {"alm05_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}},
  {"alm06_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}}, 
  {"alm07_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}},
  {"alm08_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}}, 
  {"alm09_cfg", CONFIG_TYPE_BLOB,  {.valblob.len = sizeof(alarm_blob0),
    .valblob.body = alarm_blob0}},  
};



static void config_factory_reset(void)
{
  uint32_t i, offset = 0;
  for(i = 0 ; i < sizeof(_slot) / sizeof(config_slot_t) ; i ++) { 
    switch(_slot[i].type) {
      case CONFIG_TYPE_UINT8:
        rom_write(offset, (uint8_t *)(&_slot[i].default_val.val8), 1);
        offset += 1;
        break;
      case CONFIG_TYPE_UINT16:
        rom_write(offset, (uint8_t *)(&_slot[i].default_val.val16), 2);
        offset += 2;
        break;
      case CONFIG_TYPE_UINT32:
        rom_write(offset, (uint8_t *)(&_slot[i].default_val.val32), 4);
        offset += 4;
        break;
      case CONFIG_TYPE_BLOB:
        rom_write(offset, (uint8_t *)(&_slot[i].default_val.valblob.len),
          sizeof(_slot[i].default_val.valblob.len));
        offset += sizeof(_slot[i].default_val.valblob.len);
        rom_write(offset, (uint8_t *)(_slot[i].default_val.valblob.body), _slot[i].default_val.valblob.len);
        offset += _slot[i].default_val.valblob.len;
        break; 
      default: ;
    }   
  } 
}

static const config_slot_t * find_config(const char * name, uint32_t * offset)
{
  uint32_t i;
  *offset = 0;
  for(i = 0 ; i < sizeof(_slot) / sizeof(config_slot_t) ; i ++) { 
    if(strcmp(name, _slot[i].name) == 0) {
      NEO_LOGD(TAG, "find_config %s offset at %ld\n", name, *offset);
      return &_slot[i];
    }
    switch(_slot[i].type) {
      case CONFIG_TYPE_UINT8:
        *offset += 1;
        break;
      case CONFIG_TYPE_UINT16:
        *offset += 2;
        break;
      case CONFIG_TYPE_UINT32:
        *offset += 4;
        break;
      case CONFIG_TYPE_BLOB:
        *offset += _slot[i].default_val.valblob.len + sizeof(_slot[i].default_val.valblob.len);
        break; 
      default: ;
    }   
  }
  return NULL;
}

static void config_dump(void)
{
  uint32_t i, j, offset = 0;
  config_val_t val;
  uint8_t buffer[8];
  NEO_LOGI(TAG, "dump all config begin ----------------------\n");
  for(i = 0 ; i < sizeof(_slot) / sizeof(config_slot_t) ; i ++) { 
    switch(_slot[i].type) {
      case CONFIG_TYPE_UINT8:
        rom_read(offset, (uint8_t *)(&val.val8), 1);
        offset += 1;
        break;
      case CONFIG_TYPE_UINT16:
        rom_read(offset, (uint8_t *)(&val.val16), 2);
        offset += 2;
        break;
      case CONFIG_TYPE_UINT32:
        rom_read(offset, (uint8_t *)(&val.val32), 4);
        offset += 4;
        break;
      case CONFIG_TYPE_BLOB:
        rom_read(offset, (uint8_t *)(&val.valblob.len), sizeof(val.valblob.len));
        offset += sizeof(val.valblob.len);
        break;      
      default: ;
    }
    if(_slot[i].type != CONFIG_TYPE_BLOB) {
      NEO_LOGI(TAG, "[0x%08lx] %s : %lu\n", offset, _slot[i].name, 
        _slot[i].type == CONFIG_TYPE_UINT8 ? val.val8 : 
        (_slot[i].type == CONFIG_TYPE_UINT16 ? val.val16 : 
        (_slot[i].type == CONFIG_TYPE_UINT32 ? val.val32 : 0))
      );
    } else {
      NEO_LOGI(TAG, "[0x%08lx] %s (blob len = %d):\n", offset, _slot[i].name, val.valblob.len);
      for(j = 0 ; j < val.valblob.len / 8 ; j ++) {
        rom_read(offset, buffer, sizeof(buffer));
        offset += 8;
        NEO_LOGI_HEX(TAG, buffer, sizeof(buffer));
      }
      rom_read(offset, buffer, val.valblob.len % 8);
      offset += val.valblob.len % 8;
      NEO_LOGI_HEX(TAG, buffer, val.valblob.len % 8);
    }
  } 
  NEO_LOGI(TAG, "dump all config end ----------------------\n");
}

void config_init(void)
{
  NEO_LOGI(TAG, "init\n");
  if(button_is_factory_reset()) { //12:10:30 PM
    NEO_LOGI(TAG, "config factory reset\n");
    config_factory_reset();
  }    
  config_dump();
}

bool config_read(const char * name, config_val_t * val)
{
  uint32_t off = 0;
  bool ret = false;
  const config_slot_t * p = find_config(name, &off);
  if(p) {
    ret = true;
    switch (p->type) {
      case CONFIG_TYPE_UINT8:
        rom_read(off, (uint8_t *)(&val->val8), 1);
        break;
      case CONFIG_TYPE_UINT16:
        rom_read(off, (uint8_t *)(&val->val16), 2);
        break;
      case CONFIG_TYPE_UINT32:
        rom_read(off, (uint8_t *)(&val->val32), 4);
        break;
      case CONFIG_TYPE_BLOB:
        rom_read(off, (uint8_t *)(&val->valblob.len), sizeof(val->valblob.len));
        rom_read(off + sizeof(val->valblob.len), (uint8_t *)(val->valblob.body), val->valblob.len);      
      default: ;
    }
  }
  return ret;
}

uint32_t config_read_int(const char * name)
{
  uint32_t off = 0, ret = 0;
  config_val_t val;
  const config_slot_t * p = find_config(name, &off);
  if(p) {
    switch (p->type) {
      case CONFIG_TYPE_UINT8:
        rom_read(off, (uint8_t *)(&val.val8), 1);
        ret = val.val8;
        break;
      case CONFIG_TYPE_UINT16:
        rom_read(off, (uint8_t *)(&val.val16), 2);
        ret = val.val16;
        break;
      case CONFIG_TYPE_UINT32:
        rom_read(off, (uint8_t *)(&val.val32), 4);
        ret = val.val32;
        break;
      default: ;
    }
  } else {
    NEO_LOGE(TAG, "config_read: %s not exist!\n", name);
  }
  return ret;
}

void config_write(const char * name, const config_val_t * val)
{
  uint32_t off = 0;
  const config_slot_t * p = find_config(name, &off);
  if(p) {
    switch (p->type) {
      case CONFIG_TYPE_UINT8:
        rom_write(off, (uint8_t *)(&val->val8), 1);
        break;
      case CONFIG_TYPE_UINT16:
        rom_write(off, (uint8_t *)(&val->val16), 2);
        break;
      case CONFIG_TYPE_UINT32:
        rom_write(off, (uint8_t *)(&val->val32), 4);
        break;
      case CONFIG_TYPE_BLOB:
        rom_write(off, (uint8_t *)(&val->valblob.len), sizeof(val->valblob.len));
        rom_write(off + sizeof(val->valblob.len), (uint8_t *)(val->valblob.body), val->valblob.len);
      default: ;
    }
  } else {
    NEO_LOGE(TAG, "config_write: %s not exist!\n", name);
  }
}
