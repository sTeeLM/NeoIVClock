#include "cext.h"

// 24 小时的小时到12小时的小时
bool cext_cal_hour12(uint8_t hour, uint8_t * hour12)
{
  bool ispm;
  if(hour == 0) {
    *hour12 = 12;
    ispm = false;
  } else if(hour >= 1 && hour < 12) {
    *hour12 = hour;
    ispm = false;
  } else if(hour == 12){
    *hour12 = hour;
    ispm = true;
  } else {
    *hour12 = hour - 12;
    ispm = true;
  }
  return ispm;
}