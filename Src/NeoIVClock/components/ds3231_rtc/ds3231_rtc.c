#include "ds3231_rtc.h"
#include "logger.h"
#include "i2c_wrapper.h"
#include "cext.h"

#define DS3231_I2C_ADDR 0x68

static const char * TAG = "DS3231_RTC";
static i2c_wrapper_dev_handle_t ds3231_dev_handle;
static uint8_t ds3231_rtc_data[0x13];  // data cache for 0x00~0x12 registers


// 在ds3231_rtc_data中的位移
#define DS3231_TIME_OFFSET   0x00
#define DS3231_DATE_OFFSET   0x03
#define DS3231_ALARM0_OFFSET 0x07
#define DS3231_ALARM1_OFFSET 0x0B
#define DS3231_CTL_OFFSET    0x0E
#define DS3231_TEMP_OFFSET   0x11


static const char * ds3231_alarm_mode_str[] = 
{
  "DS3231_ALARM0_MOD_PER_SEC",
  "DS3231_ALARM0_MOD_MATCH_SEC",  
  "DS3231_ALARM0_MOD_MATCH_MIN_SEC", 
  "DS3231_ALARM0_MOD_MATCH_HOUR_MIN_SEC", 
  "DS3231_ALARM0_MOD_MATCH_DATE_HOUR_MIN_SEC",   
  "DS3231_ALARM0_MOD_MATCH_DAY_HOUR_MIN_SEC",
  "DS3231_ALARM0_MOD_CNT", 
  "DS3231_ALARM1_MOD_PER_MIN",
  "DS3231_ALARM1_MOD_MATCH_MIN",  
  "DS3231_ALARM1_MOD_MATCH_HOUR_MIN", 
  "DS3231_ALARM1_MOD_MATCH_DATE_HOUR_MIN",   
  "DS3231_ALARM1_MOD_MATCH_DAY_HOUR_MIN", 
};

static const char * ds3231_square_rate_str[] = 
{
  "DS3231_SQUARE_RATE_1HZ",
  "DS3231_SQUARE_RATE_1024HZ",
  "DS3231_SQUARE_RATE_4096HZ",
  "DS3231_SQUARE_RATE_8192HZ"
};

static void ds3231_rtc_dump_raw(void)
{
  uint8_t addr;
  uint8_t c;
  NEO_LOGD(TAG, "DS3231 raw content:");
  for(addr = 0; addr < 0x12; addr ++) {
    i2c_wrapper_read(&ds3231_dev_handle, addr, I2C_ADDR_MODE_8BIT,&c, 1);
    NEO_LOGD(TAG, "ds3231 [%02x] = 0x%02x", addr, c);
  }
}

static void ds3231_rtc_dump(void)
{
  NEO_LOGD(TAG, "DS3231 content:");
  
  ds3231_rtc_read_data(DS3231_TYPE_DATE);
  NEO_LOGD(TAG, "date/day: (%02u)%02u-%02u-%02u/%u",
    ds3231_rtc_date_get_century() ? 1 : 0,
    ds3231_rtc_date_get_year(), 
    ds3231_rtc_date_get_month(), 
    ds3231_rtc_date_get_date(),
    ds3231_rtc_date_get_day()
  );
  
  ds3231_rtc_read_data(DS3231_TYPE_TIME);
  NEO_LOGD(TAG, "time: %02u:%02u:%02u, is12: %s",
    ds3231_rtc_time_get_hour(), 
    ds3231_rtc_time_get_min(), 
    ds3231_rtc_time_get_sec(),
    ds3231_rtc_time_get_hour_12() ? "ON" : "OFF"
  );
  
  ds3231_rtc_read_data(DS3231_TYPE_ALARM0);
  NEO_LOGD(TAG, "alarm0 mode: %s", ds3231_rtc_alarm_get_mode_str(DS3231_ALARM0));
  NEO_LOGD(TAG, "  day:%02u", ds3231_rtc_alarm_get_day(DS3231_ALARM0));
  NEO_LOGD(TAG, "  date:%02u", ds3231_rtc_alarm_get_date(DS3231_ALARM0));  
  NEO_LOGD(TAG, "  hour:%02u", ds3231_rtc_alarm_get_hour(DS3231_ALARM0));
  NEO_LOGD(TAG, "  min:%02u", ds3231_rtc_alarm_get_min(DS3231_ALARM0));  
  NEO_LOGD(TAG, "  sec:%02u", ds3231_rtc_alarm_get_sec(DS3231_ALARM0));
  NEO_LOGD(TAG, "  is12:%s", ds3231_rtc_alarm_get_hour_12(DS3231_ALARM0) ? "ON" : "OFF");  

  ds3231_rtc_read_data(DS3231_TYPE_ALARM1);
  NEO_LOGD(TAG, "alarm1 mode: %s", ds3231_rtc_alarm_get_mode_str(DS3231_ALARM1));
  NEO_LOGD(TAG, "  day:%02u", ds3231_rtc_alarm_get_day(DS3231_ALARM1));
  NEO_LOGD(TAG, "  date:%02u", ds3231_rtc_alarm_get_date(DS3231_ALARM1));  
  NEO_LOGD(TAG, "  hour:%02u", ds3231_rtc_alarm_get_hour(DS3231_ALARM1));
  NEO_LOGD(TAG, "  min:%02u", ds3231_rtc_alarm_get_min(DS3231_ALARM1));  
  NEO_LOGD(TAG, "  is12:%s", ds3231_rtc_alarm_get_hour_12(DS3231_ALARM1) ? "ON" : "OFF");
  
  ds3231_rtc_read_data(DS3231_TYPE_CTL);
  NEO_LOGD(TAG, "control:");
  NEO_LOGD(TAG, "  alarm0 int enable:%s", ds3231_rtc_alarm_test_int(DS3231_ALARM0) ? "ON" : "OFF");
  NEO_LOGD(TAG, "  alarm1 int enable:%s", ds3231_rtc_alarm_test_int(DS3231_ALARM1) ? "ON" : "OFF");
  NEO_LOGD(TAG, "  alarm0 int flag:%s", ds3231_rtc_alarm_test_int_flag(DS3231_ALARM0) ? "ON" : "OFF");
  NEO_LOGD(TAG, "  alarm1 int flag:%s", ds3231_rtc_alarm_test_int_flag(DS3231_ALARM1) ? "ON" : "OFF");
  NEO_LOGD(TAG, "  eosc:%c", ds3231_rtc_test_eosc() ? '1' : '0');  
  NEO_LOGD(TAG, "  bbsqw:%c", ds3231_rtc_test_bbsqw() ? '1' : '0');  
  NEO_LOGD(TAG, "  conv:%c", ds3231_rtc_test_conv() ? '1' : '0');  
  NEO_LOGD(TAG, "  square_rate:%s", ds3231_rtc_get_square_rate_str());
  NEO_LOGD(TAG, "  intcn:%c", ds3231_rtc_test_intcn() ? '1' : '0');  
  NEO_LOGD(TAG, "  osf:%c", ds3231_rtc_test_osf() ? '1' : '0');  
  NEO_LOGD(TAG, "  en32khz:%c", ds3231_rtc_test_en32khz() ? '1' : '0');  
  NEO_LOGD(TAG, "  bsy:%c", ds3231_rtc_test_bsy() ? '1' : '0');  
}

void ds3231_rtc_read_data(ds3231_rtc_data_type_t type)
{
  uint8_t offset = 0;
  switch(type) {
    case DS3231_TYPE_TIME:
      offset = DS3231_TIME_OFFSET; break;
    case DS3231_TYPE_DATE:
      offset = DS3231_DATE_OFFSET; break;    
    case DS3231_TYPE_ALARM0:
      offset = DS3231_ALARM0_OFFSET; break;
    case DS3231_TYPE_ALARM1:
      offset = DS3231_ALARM1_OFFSET; break;
    case DS3231_TYPE_TEMP:
      offset = DS3231_TEMP_OFFSET; break; 
    case DS3231_TYPE_CTL:
      offset = DS3231_CTL_OFFSET; break;     
  }

  i2c_wrapper_read(&ds3231_dev_handle, offset, I2C_ADDR_MODE_8BIT, ds3231_rtc_data + offset, 4);
  
}

void ds3231_rtc_write_data(ds3231_rtc_data_type_t type)
{
  uint8_t offset = 0;
  switch(type) {
    case DS3231_TYPE_TIME:
      offset = DS3231_TIME_OFFSET; break;
    case DS3231_TYPE_DATE:
      offset = DS3231_DATE_OFFSET; break; 
    case DS3231_TYPE_ALARM0:
      offset = DS3231_ALARM0_OFFSET; break;
    case DS3231_TYPE_ALARM1:
      offset = DS3231_ALARM1_OFFSET; break;
    case DS3231_TYPE_TEMP:
      offset = DS3231_TEMP_OFFSET; break; 
    case DS3231_TYPE_CTL:
      offset = DS3231_CTL_OFFSET; break;       
  }
  
  i2c_wrapper_write(&ds3231_dev_handle, offset, I2C_ADDR_MODE_8BIT, ds3231_rtc_data + offset, 4);
   
}

////////////////////////////////////////////////////////
// ds3231_rtc_read_data(DS3231_TYPE_TIME)之后调用

static uint8_t ds3231_rtc_get_hour_internal(uint8_t hour)
{
  uint8_t ret;
  if(hour & 0x40) { // 是12小时表示
    ret = (hour & 0x0F) + ((hour & 0x10) >> 4) * 10;
    if( hour & 0x20 ) { // 是PM
      ret += 12;
    }
  } else { // 是24小时表示
      ret =  (hour & 0x0F) + ((hour & 0x30) >> 4) * 10;
  }
  return ret;
}


static void ds3231_rtc_set_hour_internal(uint8_t hour, uint8_t * dat)
{
  if(*dat & 0x40) { // 是12小时表示
    if(hour > 12) {
      *dat |= 0x20; // 设置PM标志
      hour -= 12;
    } else {
      *dat &= ~0x20; // 清除PM标志
    }
    *dat &= 0xF0; // clear lsb
    *dat |= hour % 10;
    *dat &= 0xEF; // clear msb
    *dat |= (hour / 10) << 4;    
  } else {
    // 24小时表示
    *dat &= 0xF0; // clear lsb
    *dat |= hour % 10;
    *dat &= 0xCF; // clear msb
    *dat |= (hour / 10) << 4;
  }  
}

static void ds3231_rtc_set_hour_12_internal(bool enable, uint8_t * dat)
{
  uint8_t hour;
  hour = ds3231_rtc_get_hour_internal(*dat);
  if(enable) {
    *dat |= 0x40;
  } else {
    *dat &= ~0x40;
  }
  ds3231_rtc_set_hour_internal(hour, dat);  
}

static bool ds3231_rtc_get_hour_12_internal(uint8_t hour)
{
  return ((hour & 0x40) != 0);
}

static uint8_t ds3231_rtc_get_min_sec_internal(uint8_t min)
{
  return (min & 0x0F) + ((min & 0x70) >> 4) * 10;
}

static void ds3231_rtc_set_min_sec_internal(uint8_t min, uint8_t * dat)
{
  *dat &= 0xF0;
  *dat |= min % 10;
  *dat &= 0x8F;
  *dat |= (min / 10) << 4;
}

int8_t ds3231_rtc_time_get_hour(void)
{
  return ds3231_rtc_get_hour_internal(ds3231_rtc_data[2]);
}


void ds3231_rtc_time_set_hour(uint8_t hour)
{
  ds3231_rtc_set_hour_internal(hour, &ds3231_rtc_data[2]);
}

void ds3231_rtc_time_set_hour_12(bool enable)
{
  ds3231_rtc_set_hour_12_internal(enable, &ds3231_rtc_data[2]);
}

bool ds3231_rtc_time_get_hour_12()
{
  return ds3231_rtc_get_hour_12_internal(ds3231_rtc_data[2]);
}

uint8_t ds3231_rtc_time_get_min(void)
{
  return ds3231_rtc_get_min_sec_internal(ds3231_rtc_data[1]);
}

void ds3231_rtc_time_set_min(uint8_t min)
{
  ds3231_rtc_set_min_sec_internal(min, &ds3231_rtc_data[1]);
}

uint8_t ds3231_rtc_time_get_sec(void)
{
  return ds3231_rtc_get_min_sec_internal(ds3231_rtc_data[0]);
}

void ds3231_rtc_time_set_sec(uint8_t sec)
{
  ds3231_rtc_set_min_sec_internal(sec, &ds3231_rtc_data[0]);
}

/////////////////////////////////////////////////////////
// ds3231_rtc_read_data(BSP_DS3231_TYPE_DATE)之后调用
uint8_t ds3231_rtc_date_get_year(void)
{
  return (ds3231_rtc_data[6] & 0x0F) + ((ds3231_rtc_data[6] & 0xF0) >> 4) * 10;
}

bool ds3231_rtc_date_get_century(void )
{
  return ((ds3231_rtc_data[5] & 0x80) != 0);
}

void ds3231_rtc_date_set_century(bool set)
{
  ds3231_rtc_data[5] &= ~0x80;
  if(set)
    ds3231_rtc_data[5] |= 0x80;
}

void ds3231_rtc_date_set_year(uint8_t year)
{
  ds3231_rtc_data[6] &= 0xF0;
  ds3231_rtc_data[6] |= year % 10;
  ds3231_rtc_data[6] &= 0x0F;
  ds3231_rtc_data[6] |= (year / 10) << 4;  
}

uint8_t ds3231_rtc_date_get_month(void)
{
  return (ds3231_rtc_data[5] & 0x0F) + ((ds3231_rtc_data[5] & 0x10) >> 4) * 10;
}

void ds3231_rtc_date_set_month(uint8_t month)
{
  ds3231_rtc_data[5] &= 0xF0;
  ds3231_rtc_data[5] |= month % 10;
  ds3231_rtc_data[5] &= 0x8F;
  ds3231_rtc_data[5] |= (month / 10) << 4;  
}

static uint8_t ds3231_rtc_get_date_internal(uint8_t date)
{
  return (date & 0x0F) + ((date & 0x30) >> 4) * 10;
}

static void ds3231_rtc_set_date_internal(uint8_t date, uint8_t * dat)
{
  *dat &= 0xF0;
  *dat |= date % 10;
  *dat &= 0xCF;
  *dat |= (date / 10) << 4; 
}

uint8_t ds3231_rtc_date_get_date(void)
{
  return ds3231_rtc_get_date_internal(ds3231_rtc_data[4]);
}

bool ds3231_rtc_date_set_date(uint8_t date)
{
  uint8_t mon = ds3231_rtc_date_get_month();
  bool century = ds3231_rtc_date_get_century();

  NEO_LOGD(TAG, "ds3231_rtc_date_set_date, valid check...");
  if(mon == 1 || mon == 3 || mon == 5 || mon == 7 
    || mon == 8 || mon == 10 || mon == 12) {
    if(date > 32) return false;
  } else {
    if(date > 31) return false;
    if(mon == 2 && cext_is_leap_year(ds3231_rtc_date_get_year() + century ? 2000 : 1900)) {
      if(date > 30) return false;
    } else if(mon == 2 && !cext_is_leap_year(ds3231_rtc_date_get_year() + century ? 2000 : 1900)) {
      if(date > 29) return false;
    }
  }
  
  NEO_LOGD(TAG, "ds3231_rtc_date_set_date, valid check...OK");
  ds3231_rtc_set_date_internal(date, &ds3231_rtc_data[4]);

  return true;  
}

uint8_t ds3231_rtc_date_get_day(void)
{
  return ds3231_rtc_data[3];
}

void ds3231_rtc_date_set_day(uint8_t day)
{
  if (day >= 1 && day <= 7) {
    ds3231_rtc_data[3] = day;
  }
}

// ds3231_rtc_read_data(BSP_DS3231_TYPE_ALARM0)ds3231_rtc_read_data(BSP_DS3231_TYPE_ALARM1)之后调用
uint8_t ds3231_rtc_alarm_get_hour(ds3231_rtc_alarm_index_t alarm_index)
{
  return ds3231_rtc_get_hour_internal(alarm_index == DS3231_ALARM0 ? ds3231_rtc_data[9] : ds3231_rtc_data[0xc]);
}

bool ds3231_rtc_alarm_get_hour_12(ds3231_rtc_alarm_index_t alarm_index)
{
  return ds3231_rtc_get_hour_12_internal(alarm_index == DS3231_ALARM0 ? ds3231_rtc_data[9] : ds3231_rtc_data[0xc]);
}

void ds3231_rtc_alarm_set_hour_12(ds3231_rtc_alarm_index_t alarm_index, bool enable)
{
  ds3231_rtc_set_hour_12_internal(enable, alarm_index == DS3231_ALARM0 ? &ds3231_rtc_data[9] : &ds3231_rtc_data[0xc]);
}

void ds3231_rtc_alarm_set_hour(ds3231_rtc_alarm_index_t alarm_index, uint8_t hour)
{
  ds3231_rtc_set_hour_12_internal(hour, alarm_index == DS3231_ALARM0 ? &ds3231_rtc_data[9] : &ds3231_rtc_data[0xc]);
}

uint8_t ds3231_rtc_alarm_get_min(ds3231_rtc_alarm_index_t alarm_index)
{
  return ds3231_rtc_get_min_sec_internal(alarm_index == DS3231_ALARM0 ? ds3231_rtc_data[8] : ds3231_rtc_data[0xb]);
}

void ds3231_rtc_alarm_set_min(ds3231_rtc_alarm_index_t alarm_index, uint8_t min)
{
  ds3231_rtc_set_min_sec_internal(min, alarm_index == DS3231_ALARM0 ? &ds3231_rtc_data[8] : &ds3231_rtc_data[0xb]); 
}

uint8_t ds3231_rtc_alarm_get_day(ds3231_rtc_alarm_index_t alarm_index)
{
  return alarm_index == DS3231_ALARM0 ? (ds3231_rtc_data[0xa] & 0x0F) : (ds3231_rtc_data[0xd] & 0x0F);
}

void ds3231_rtc_alarm_set_day(ds3231_rtc_alarm_index_t alarm_index, uint8_t day)
{
  if(alarm_index == DS3231_ALARM0) {
    ds3231_rtc_data[0xa] &= 0xF0;
    ds3231_rtc_data[0xa] |= day;
  } else {
    ds3231_rtc_data[0xd] &= 0xF0;
    ds3231_rtc_data[0xd] |= day;
  }
}

uint8_t ds3231_rtc_alarm_get_date(ds3231_rtc_alarm_index_t alarm_index)
{
  return alarm_index == DS3231_ALARM0 ?
    ds3231_rtc_get_date_internal(ds3231_rtc_data[0xa]) : ds3231_rtc_get_date_internal(ds3231_rtc_data[0xd]);
}

void ds3231_rtc_alarm_set_date(ds3231_rtc_alarm_index_t alarm_index, uint8_t date)
{
  alarm_index == DS3231_ALARM0 ? ds3231_rtc_set_date_internal(date, &ds3231_rtc_data[0xa]) : 
    ds3231_rtc_set_date_internal(date, &ds3231_rtc_data[0xd]);
}

uint8_t ds3231_rtc_alarm_get_sec(ds3231_rtc_alarm_index_t alarm_index)
{
  if(alarm_index == DS3231_ALARM0) {
    return (ds3231_rtc_data[7] & 0x0F) + ((ds3231_rtc_data[7] & 0x70) >> 4) * 10;
  }
  return 0;
}


void ds3231_rtc_alarm_set_sec(ds3231_rtc_alarm_index_t alarm_index, uint8_t sec)
{
  if(alarm_index == DS3231_ALARM0) {
    ds3231_rtc_data[7] &= 0xF0;
    ds3231_rtc_data[7] |= sec % 10;
    ds3231_rtc_data[7] &= 0x8F;
    ds3231_rtc_data[7] |= ((sec / 10) << 4);    
  }
}

// DY A1M4 A1M3 A1M2 A1M1
// X  1    1    1    1    ALARM0_MOD_PER_SEC                 Alarm once per second
// X  1    1    1    0    ALARM0_MOD_MATCH_SEC               Alarm when seconds match
// X  1    1    0    0    ALARM0_MOD_MATCH_MIN_SEC           Alarm when minutes and seconds match
// X  1    0    0    0    ALARM0_MOD_MATCH_HOUR_MIN_SEC      Alarm when hours, minutes, and seconds match
// 0  0    0    0    0    ALARM0_MOD_MATCH_DATE_HOUR_MIN_SEC Alarm when date, hours, minutes, and seconds match
// 1  0    0    0    0    ALARM0_MOD_MATCH_DAY_HOUR_MIN_SEC  Alarm when day, hours, minutes, and seconds match

// DY A2M4 A2M3 A1M2
// X  1    1    1    ALARM1_MOD_PER_MIN                 Alarm once per minute (00 seconds of every minute)
// X  1    1    0    ALARM1_MOD_MATCH_MIN               Alarm when minutes match
// X  1    0    0    ALARM1_MOD_MATCH_HOUR_MIN          Alarm when hours and minutes match
// 0  0    0    0    ALARM1_MOD_MATCH_DATE_HOUR_MIN     Alarm when date, hours, and minutes match
// 1  0    0    0    ALARM1_MOD_MATCH_DAY_HOUR_MIN      Alarm when day, hours, and minutes match

ds3231_rtc_alarm_mode_t ds3231_rtc_alarm_get_mode(ds3231_rtc_alarm_index_t alarm_index)
{
  uint8_t dy, a1m1, a1m2, a1m3, a1m4;
  if(alarm_index == DS3231_ALARM0) {
    dy = ds3231_rtc_data[0xa] & 0x40;
    a1m1 = ds3231_rtc_data[7] & 0x80;
    a1m2 = ds3231_rtc_data[8] & 0x80;
    a1m3 = ds3231_rtc_data[9] & 0x80;
    a1m4 = ds3231_rtc_data[0xa] & 0x80;
    if(a1m1 && a1m2 && a1m3 && a1m4) {
      return DS3231_ALARM0_MOD_PER_SEC;
    } else if(!a1m1 && a1m2 && a1m3 && a1m4) {
      return DS3231_ALARM0_MOD_MATCH_SEC;
    } else if(!a1m1 && !a1m2 && a1m3  && a1m4) {
      return DS3231_ALARM0_MOD_MATCH_MIN_SEC;
    } else if(!a1m1 && !a1m2 && !a1m3  && a1m4) {
      return DS3231_ALARM0_MOD_MATCH_HOUR_MIN_SEC;
    } else if(!a1m1 && !a1m2 && !a1m3  && !a1m4 && !dy) {
      return DS3231_ALARM0_MOD_MATCH_DATE_HOUR_MIN_SEC;
    } else {
      return DS3231_ALARM0_MOD_MATCH_DAY_HOUR_MIN_SEC;
    }
  } else if(alarm_index == DS3231_ALARM1){
    dy = ds3231_rtc_data[0xd] & 0x40;
    a1m2 = ds3231_rtc_data[0xb] & 0x80;
    a1m3 = ds3231_rtc_data[0xc] & 0x80;
    a1m4 = ds3231_rtc_data[0xd] & 0x80;
    if(a1m2 && a1m3 && a1m4) {
      return DS3231_ALARM1_MOD_PER_MIN;
    } else if(!a1m2 && a1m3 && a1m4) {
      return DS3231_ALARM1_MOD_MATCH_MIN;
    } else if(!a1m2 && !a1m3  && a1m4) {
      return DS3231_ALARM1_MOD_MATCH_HOUR_MIN;
    } else if(!a1m2 && !a1m3  && !a1m4) {
      return DS3231_ALARM1_MOD_MATCH_DATE_HOUR_MIN;
    } else if(!a1m2 && !a1m3  && !a1m4 && !dy) {
      return DS3231_ALARM1_MOD_MATCH_DATE_HOUR_MIN;
    } else {
      return DS3231_ALARM1_MOD_MATCH_DAY_HOUR_MIN;
    }
  }
  return DS3231_ALARM0_MOD_CNT;
}

void ds3231_rtc_alarm_set_mode(ds3231_rtc_alarm_index_t alarm_index, ds3231_rtc_alarm_mode_t mode)
{
  if(mode < DS3231_ALARM0_MOD_CNT && alarm_index == DS3231_ALARM0) {
    ds3231_rtc_data[7] &= ~0x80;
    ds3231_rtc_data[8] &= ~0x80;
    ds3231_rtc_data[9] &= ~0x80;
    ds3231_rtc_data[0xa] &= ~0x80; 
    ds3231_rtc_data[0xa] &= ~0x40;    
    switch(mode) {
      case DS3231_ALARM0_MOD_PER_SEC:
        ds3231_rtc_data[7] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM0_MOD_MATCH_SEC:
        ds3231_rtc_data[8] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM0_MOD_MATCH_MIN_SEC:
        ds3231_rtc_data[9] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM0_MOD_MATCH_HOUR_MIN_SEC:
        ds3231_rtc_data[0xa] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM0_MOD_MATCH_DATE_HOUR_MIN_SEC:
        break;
      case DS3231_ALARM0_MOD_MATCH_DAY_HOUR_MIN_SEC:
        ds3231_rtc_data[0xa] |= 0x40;
      default:
        ;
    }
  } else if(mode > DS3231_ALARM0_MOD_CNT && alarm_index == DS3231_ALARM1) {
    ds3231_rtc_data[0xb] &= ~0x80;
    ds3231_rtc_data[0xc] &= ~0x80;
    ds3231_rtc_data[0xd] &= ~0x80;
    ds3231_rtc_data[0xd] &= ~0x40;
    switch (mode) {
      case DS3231_ALARM1_MOD_PER_MIN:
        ds3231_rtc_data[0xb] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM1_MOD_MATCH_MIN:
        ds3231_rtc_data[0xc] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM1_MOD_MATCH_HOUR_MIN:
        ds3231_rtc_data[0xd] |=  0x80;
        __attribute__((fallthrough));
      case DS3231_ALARM1_MOD_MATCH_DATE_HOUR_MIN:
        break;
      case DS3231_ALARM1_MOD_MATCH_DAY_HOUR_MIN:
        ds3231_rtc_data[0xd] |= 0x40;
      default:
        ;
    }
  }
}

const char * ds3231_rtc_alarm_get_mode_str(ds3231_rtc_alarm_index_t alarm_index)
{
  return ds3231_alarm_mode_str[ds3231_rtc_alarm_get_mode(alarm_index)];
}


// ds3231_rtc_read_data(DS3231_TYPE_CTL)之后调用
void ds3231_rtc_alarm_enable_int(ds3231_rtc_alarm_index_t index, bool enable)
{
  if(index == DS3231_ALARM0) {
    if(!enable)
      ds3231_rtc_data[0xe] &= ~1;
    else
      ds3231_rtc_data[0xe] |= 1;
  } else if(index == DS3231_ALARM1) {
    if(!enable)
      ds3231_rtc_data[0xe] &= ~2;
    else
      ds3231_rtc_data[0xe] |= 2;
  }
}

bool ds3231_rtc_alarm_test_int(ds3231_rtc_alarm_index_t index)
{
  if(index == DS3231_ALARM0) {
    return (ds3231_rtc_data[0xe] & 1) == 1;
  } else if(index == DS3231_ALARM1) {
    return (ds3231_rtc_data[0xe] & 2) == 2;
  }
  return 0;
}

void ds3231_rtc_alarm_clear_int_flag(ds3231_rtc_alarm_index_t index)
{
  if(index == DS3231_ALARM0) {
    ds3231_rtc_data[0xf] &= ~1;
  } else if(index == DS3231_ALARM1) {
    ds3231_rtc_data[0xf] &= ~2;    
  } 
}

bool ds3231_rtc_alarm_test_int_flag(ds3231_rtc_alarm_index_t index)
{
  if(index == DS3231_ALARM0) {
    return (ds3231_rtc_data[0xf] & 1) == 1;
  } else if(index == DS3231_ALARM1) {
    return (ds3231_rtc_data[0xf] & 2) == 2;
  }
  return 0;
}

// ds3231_rtc_read_data(DS3231_TYPE_CTL)之后调用

bool ds3231_rtc_test_eosc(void)
{
  return (ds3231_rtc_data[0xe] & 0x80) != 0;
}

void ds3231_rtc_set_eosc(bool val)
{
  ds3231_rtc_data[0xe] &= ~0x80;
  if(val)
    ds3231_rtc_data[0xe] |= 0x80;
}

bool ds3231_rtc_test_bbsqw(void)
{
  return (ds3231_rtc_data[0xe] & 0x40) != 0;
}

void ds3231_rtc_set_bbsqw(bool val)
{
  ds3231_rtc_data[0xe] &= ~0x40;
  if(val)
    ds3231_rtc_data[0xe] |= 0x40;
}

bool ds3231_rtc_test_conv (void)
{
  return (ds3231_rtc_data[0xe] & 0x20) != 0;
}

void ds3231_rtc_set_conv(bool val)
{
  ds3231_rtc_data[0xe] &= ~0x20;
  if(val)
    ds3231_rtc_data[0xe] |= 0x20;
}

ds3231_rtc_square_rate_t ds3231_rtc_get_square_rate(void)
{
  return (((ds3231_rtc_data[0xe] & 0x18) >> 3) & 0x3);
}

void ds3231_rtc_set_square_rate(ds3231_rtc_square_rate_t rt)
{
  uint8_t val = rt;
  ds3231_rtc_data[0xe] &= ~0x18;
  ds3231_rtc_data[0xe] |= val << 3;
}

const char * ds3231_rtc_get_square_rate_str(void)
{
  return ds3231_square_rate_str[ds3231_rtc_get_square_rate()];
}

bool ds3231_rtc_test_intcn(void)
{
  return (ds3231_rtc_data[0xe] & 0x4) != 0;
}

void ds3231_rtc_set_intcn(bool val)
{
  ds3231_rtc_data[0xe] &= ~0x4;
  if(val)
    ds3231_rtc_data[0xe] |= 0x4;
}

bool ds3231_rtc_test_osf(void)
{
  return (ds3231_rtc_data[0xf] & 0x80) != 0;
}

void ds3231_rtc_set_osf(bool val)
{
  ds3231_rtc_data[0xf] &= ~0x80;
  if(val)
    ds3231_rtc_data[0xf] |= 0x80;
}

bool ds3231_rtc_test_en32khz(void)
{
  return (ds3231_rtc_data[0xf] & 0x8) != 0;
}

void ds3231_rtc_set_en32khz(bool val)
{
  ds3231_rtc_data[0xf] &= ~0x8;
  if(val)
    ds3231_rtc_data[0xf] |= 0x8;
}

bool ds3231_rtc_test_bsy(void)
{
  return (ds3231_rtc_data[0xf] & 0x4) != 0;
}

void ds3231_rtc_enter_powersave(void)
{ 
  // 停止32KHZ输出
  ds3231_rtc_read_data(DS3231_TYPE_CTL);
  ds3231_rtc_set_en32khz(0);
  ds3231_rtc_write_data(DS3231_TYPE_CTL);
  
}

void ds3231_rtc_leave_powersave(void)
{
  // 启动32KHZ输出
  ds3231_rtc_read_data(DS3231_TYPE_CTL);
  ds3231_rtc_set_en32khz(1);
  ds3231_rtc_write_data(DS3231_TYPE_CTL);
  
}


// ds3231_rtc_read_data(DS3231_TYPE_TEMP)之后调用
bool ds3231_rtc_get_temperature(uint8_t * integer, uint8_t * flt)
{
  bool sign = ((ds3231_rtc_data[0x11] &  0x80) != 0);
  uint16_t data;
  
  NEO_LOGD(TAG, "ds3231_rtc_get_temperature: 0x%02x 0x%02x", ds3231_rtc_data[0x11], ds3231_rtc_data[0x12]);
  
  data = ds3231_rtc_data[0x11] & ~0x80;
  data <<= 2;
  data |= ((ds3231_rtc_data[0x12] >>= 6) & 0x03);

  if(sign) {
    data --;
    data = ~data;
  }
  
  data &= 0x1FF;
  
  *integer = (data & 0x1FC) >> 2;
  *flt     = (data & 0x3) * 25;
  
  if(*integer > 99) *integer = 99;
  
  NEO_LOGD(TAG, "ds3231_rtc_get_temperature: %c%03d.%02d", sign ? '-':'+', *integer, *flt);
  
  return sign;
}

double ds3231_rtc_get_temperature_double(void)
{
  uint8_t integer, flt;
  bool sign = ds3231_rtc_get_temperature(&integer, &flt);
  double temp = integer + (double)flt / 100.0;
  return sign ? -temp : temp;
}

////////////////////////////////////////////////////////
void ds3231_rtc_init(void)
{
  uint8_t count;
  NEO_LOGI(TAG, "init");
  
  // add r2c device, work at 100KHz
  i2c_wrapper_add_dev(DS3231_I2C_ADDR, 100000, &ds3231_dev_handle);

  // dump raw content
  ds3231_rtc_dump_raw();  
  ds3231_rtc_dump(); 


  // RTC内部使用24小时制
  ds3231_rtc_read_data(DS3231_TYPE_TIME); 
  if(ds3231_rtc_time_get_hour_12()) {
    NEO_LOGI(TAG, "DS3231 set time format to 24");
    count = ds3231_rtc_time_get_hour();
    ds3231_rtc_time_set_hour_12(false);
    ds3231_rtc_time_set_hour(count);
    ds3231_rtc_write_data(DS3231_TYPE_TIME); 
  }    
  
  // 闹钟设置为24小时格式
  ds3231_rtc_read_data(DS3231_TYPE_ALARM0);
  if(ds3231_rtc_alarm_get_hour_12( DS3231_ALARM0)) {
    NEO_LOGI(TAG, "DS3231 set alarm0 format to 24");
    ds3231_rtc_alarm_set_hour_12(DS3231_ALARM0, false);
    ds3231_rtc_write_data(DS3231_TYPE_ALARM0);  
  }
  
  ds3231_rtc_read_data(DS3231_TYPE_ALARM1);
  if(ds3231_rtc_alarm_get_hour_12(DS3231_ALARM1)) {
    NEO_LOGI(TAG, "DS3231 set alarm1 format to 24");
    ds3231_rtc_alarm_set_hour_12(DS3231_ALARM1, false);
    ds3231_rtc_write_data(DS3231_TYPE_ALARM1);
  }

  // 清除century标志
  ds3231_rtc_read_data(DS3231_TYPE_DATE);
  ds3231_rtc_date_set_century(0);
  ds3231_rtc_write_data(DS3231_TYPE_DATE);

  // 清除所有闹钟：闹钟配置由alarm自行从rom中读取，写入rtc
  ds3231_rtc_read_data(DS3231_TYPE_CTL);
  ds3231_rtc_alarm_enable_int(DS3231_ALARM0, 0);
  ds3231_rtc_alarm_clear_int_flag(DS3231_ALARM0);
  ds3231_rtc_alarm_enable_int(DS3231_ALARM1, 0);
  ds3231_rtc_alarm_clear_int_flag(DS3231_ALARM1);
  // 允许RTC发中断
  ds3231_rtc_set_intcn(1);
  // 启动32KHZ输出  
  ds3231_rtc_set_en32khz(1);
  // 清除eosc
  ds3231_rtc_set_eosc(0);
  // BBSQW = 0，电池供电时候没有方波或者中断
  ds3231_rtc_set_bbsqw(0);
  ds3231_rtc_write_data(DS3231_TYPE_CTL); 

  // dump raw content
  ds3231_rtc_dump_raw();  
  ds3231_rtc_dump(); 
}

void ds3231_rtc_get_time(uint8_t * hour, uint8_t * min, uint8_t * sec)
{
  ds3231_rtc_read_data(DS3231_TYPE_TIME);
  *hour = ds3231_rtc_time_get_hour();
  *min = ds3231_rtc_time_get_min();
  *sec = ds3231_rtc_time_get_sec();
}

bool ds3231_rtc_get_date(uint8_t * year, uint8_t * mon, uint8_t * date, uint8_t * day)
{  
  ds3231_rtc_read_data(DS3231_TYPE_DATE);
  *year = ds3231_rtc_date_get_year();
  *mon = ds3231_rtc_date_get_month();
  *date = ds3231_rtc_date_get_date();
  *day = ds3231_rtc_date_get_day();
  
  return ds3231_rtc_date_get_century(); 
}
void ds3231_rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec)
{
  NEO_LOGI(TAG, "ds3231_rtc_set_time hour=%d, min=%d, sec=%d", hour, min, sec);
  ds3231_rtc_read_data(DS3231_TYPE_TIME);
  ds3231_rtc_time_set_hour(hour);
  ds3231_rtc_time_set_min(min);
  ds3231_rtc_time_set_sec(sec);
  ds3231_rtc_write_data(DS3231_TYPE_TIME);
}
void ds3231_rtc_set_date(bool century, uint8_t year, uint8_t mon, uint8_t date, uint8_t day)
{
  NEO_LOGI(TAG, "ds3231_rtc_set_date century=%d, year=%d, mon=%d, date=%d, day=%d", century, year, mon, date, day);
  ds3231_rtc_read_data(DS3231_TYPE_DATE);
  ds3231_rtc_date_set_century(century);
  ds3231_rtc_date_set_year(year);
  ds3231_rtc_date_set_month(mon);
  ds3231_rtc_date_set_date(date);
  ds3231_rtc_date_set_day(day);
  ds3231_rtc_write_data(DS3231_TYPE_DATE);
}
void ds3231_rtc_enable_32768HZ(bool enable)
{
  ds3231_rtc_read_data(DS3231_TYPE_CTL);
  ds3231_rtc_set_en32khz(enable);
  ds3231_rtc_write_data(DS3231_TYPE_CTL);
}