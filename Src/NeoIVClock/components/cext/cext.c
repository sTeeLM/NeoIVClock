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

/*
  IIR:

  value = (value * (filter_coefficient - 1) + new_data) / filter_coefficient

*/
uint16_t cext_iir_uint16(uint16_t oldv, uint16_t newv, uint8_t coe)
{
  int32_t ret = oldv;
  ret += (int32_t)((newv - oldv) / (float)coe);
  return  (uint16_t) ret;
}

float cext_iir_float(float oldv, float newv, uint8_t coe)
{
  float ret = oldv;
  ret += (float)((newv - oldv) / coe);
  return ret;
}

int32_t cext_linear_interpolate(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x)
{
  int64_t dx, dy, run;
  int32_t y;
// 检查两点是否重合（导致斜率无穷大）
    if (x1 == x2) {
        return 0;
    }

    // 两点式直线方程: y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
    // 使用 int64_t 防止 (x - x1) * (y2 - y1) 乘法溢出
    dx = (int64_t)x - x1;
    dy = (int64_t)y2 - y1;
    run = (int64_t)x2 - x1;

    // 计算结果并强制转换为 int32_t 返回
    y = y1 + (dx * dy) / run;
    
    return y;
}