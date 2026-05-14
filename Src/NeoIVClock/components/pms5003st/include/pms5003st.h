#ifndef NEO_IV_CLOCK_PMS5003ST_H
#define NEO_IV_CLOCK_PMS5003ST_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _pms5003st_cmd_t
{
    PMS5003ST_CMD_READ_DATA     = 0xe2, // 被动读数
    PMS5003ST_CMD_SET_MODE      = 0xe1, // 主动，被动切换
    PMS5003ST_CMD_STANDBY       = 0xe4, // 待机控制
} pms5003st_cmd_t;

typedef struct _pms5003st_cmd_msg_t
{
    uint8_t signature_h; // 0x42
    uint8_t signature_l; // 0x4d
    uint8_t cmd;         // pms5003st_cmd_t
    uint8_t datah;
    uint8_t datal;
    uint8_t checksumh;
    uint8_t checksuml;
} __attribute__((packed)) pms5003st_cmd_msg_t;

typedef struct _pms5003st_res_msg_t
{
    uint8_t signature_h; // 0x42
    uint8_t signature_l; // 0x4d
    uint8_t lenh;        // 帧长度=2x17+2(数据+校验位)
    uint8_t lenl;
    uint8_t pm10h;       // 数据1 表示PM1.0 浓度（CF=1，标准颗粒物）
    uint8_t pm10l;       // 单位μg/m3
    uint8_t pm25h;       // 数据2 表示PM2.5 浓度（CF=1，标准颗粒物）
    uint8_t pm25l;       // 单位μg/m3
    uint8_t pm100h;      // 数据3 表示PM10 浓度（CF=1，标准颗粒物）
    uint8_t pm100l;      // 单位μg/m3
    uint8_t pm10ah;      // 数据4 表示PM1.0 浓度（大气环境下）
    uint8_t pm10al;      // 单位μg/m3
    uint8_t pm25ah;      // 数据5 表示PM2.5 浓度（大气环境下）
    uint8_t pm25al;      // 单位μg/m3
    uint8_t pm100ah;     // 数据6 表示PM10 浓度（大气环境下）
    uint8_t pm100al;     // 单位μg/m3
    uint8_t pm03cnth;    // 数据7 表示0.1 升空气中直径在0.3um 以上颗粒物个数
    uint8_t pm03cntl;  
    uint8_t pm05cnth;    // 数据8 表示0.1 升空气中直径在0.5um 以上颗粒物个数
    uint8_t pm05cntl;  
    uint8_t pm10cnth;    // 数据9 表示0.1 升空气中直径在1.0um 以上颗粒物个数
    uint8_t pm10cntl;  
    uint8_t pm25cnth;    // 数据10 表示0.1 升空气中直径在2.5um 以上颗粒物个数
    uint8_t pm25cntl;  
    uint8_t pm50cnth;    // 数据11 表示0.1 升空气中直径在5.0um 以上颗粒物个数
    uint8_t pm50cntl;  
    uint8_t pm100cnth;   // 数据12 表示0.1 升空气中直径在10um 以上颗粒物个数
    uint8_t pm100cntl;  
    uint8_t formh;       // 数据13 甲醛浓度数值，真实甲醛浓度值=本数值/1000，单位：mg/m³
    uint8_t forml;        
    uint8_t temph;       // 数据14 温度，真实温度值=本数值/10， 单位：℃
    uint8_t templ; 
    uint8_t molh;        // 数据15 湿度，真实湿度值=本数值/10，单位：％
    uint8_t moll; 
    uint8_t resh;        // 数据16 保留
    uint8_t resl;
    uint8_t ver;         // 数据17 版本号, 错误代码
    uint8_t err;         // 
    uint8_t checksumh;   // 校验码=起始符1+起始符2+……..+数据17 低八位
    uint8_t checksuml;
} __attribute__((packed))  pms5003st_res_msg_t ;

typedef struct _pms5003st_data_t
{
    uint16_t  pm_10;
    uint16_t  pm_25;   
    uint16_t  pm_100;        
    uint16_t  pm_10a; 
    uint16_t  pm_25a;  
    uint16_t  pm_100a; 
    uint16_t  pm_03cnt;
    uint16_t  pm_05cnt;
    uint16_t  pm_10cnt;   
    uint16_t  pm_25cnt; 
    uint16_t  pm_50cnt;   
    uint16_t  pm_100cnt;
    float  form;  
    float  temp;       
    float  mol;                                    
} pms5003st_data_t;

void pms5003st_init(void);
void pms5003st_sleep(bool sleep);
bool pms5003st_read_data(pms5003st_data_t * data);
#endif // NEO_IV_CLOCK_PMS5003ST_H
