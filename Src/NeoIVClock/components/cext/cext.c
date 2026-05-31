#include "cext.h"
#include <string.h>

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

float cext_linear_interpolate_float(float x1, float y1, float x2, float y2, float x)
{
  double dx, dy, run;
  float y;
// 检查两点是否重合（导致斜率无穷大）
    if (x1 == x2) {
        return 0.0f;
    }

    // 两点式直线方程: y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
    // 使用 double 防止 (x - x1) * (y2 - y1) 乘法溢出
    dx = (double)x - x1;
    dy = (double)y2 - y1;
    run = (double)x2 - x1;

    // 计算结果并强制转换为 int32_t 返回
    y = (float)(y1 + (dx * dy) / run);
    
    return y;
}

void cext_url_decode_inplace(char *str) 
{
  char *p_src = str;
  char *p_dst = str;
  while (*p_src) {
    if (*p_src == '+') {
      *p_dst++ = ' ';
      p_src++;
    } else if (*p_src == '%' && p_src[1] && p_src[2]) {
      char hex[3] = { p_src[1], p_src[2], '\0' };
      *p_dst++ = (char)strtol(hex, NULL, 16);
      p_src += 3;
    } else {
      *p_dst++ = *p_src++;
    }
  }
  *p_dst = '\0';
}

bool cext_extract_form_value(const char *query_str, const char *key, char *dest_buf, size_t dest_max_len) 
{
  if (query_str == NULL || key == NULL || dest_buf == NULL) return false;
  
  // 寻找 "key=" 的起始位置
  char *p_start = strstr(query_str, key);
  if (p_start == NULL) return false;
  
  // 检查前面是否是字符串起点或分隔符 '&'，防止部分包含的干扰（如 pass 和 my_pass）
  if (p_start != query_str && *(p_start - 1) != '&') {
    p_start = strstr(p_start + 1, key);
    if (p_start == NULL) return false;
  }
  
  p_start += strlen(key); // 移动到 '=' 后面
  if (*p_start != '=') return false;
  p_start++; // 越过 '=' 符号
  
  // 寻找当前键值对的终点位置（即下一个 '&' 或字符串末尾 '\0'）
  char *p_end = strchr(p_start, '&');
  size_t val_len = (p_end != NULL) ? (size_t)(p_end - p_start) : strlen(p_start);
  
  // 防止用户输入的字段长度溢出本地定义的静态缓冲区
  if (val_len >= dest_max_len) {
    val_len = dest_max_len - 1;
  }
  
  // 安全拷贝
  memcpy(dest_buf, p_start, val_len);
  dest_buf[val_len] = '\0';
  
  // 进行就地 URL 解码，将 %20 或 + 转换回正常的人类可见字符
  cext_url_decode_inplace(dest_buf);
  return true;
}