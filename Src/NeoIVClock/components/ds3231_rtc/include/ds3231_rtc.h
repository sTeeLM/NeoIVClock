#ifndef NEO_IV_CLOCK_DS3231_RTC_H
#define NEO_IV_CLOCK_DS3231_RTC_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _ds3231_rtc_data_type_t {
  DS3231_TYPE_TIME    = 0, // 时间
  DS3231_TYPE_DATE    = 1, // 日期
  DS3231_TYPE_ALARM0  = 2, // 闹钟0
  DS3231_TYPE_ALARM1  = 3, // 闹钟1
  DS3231_TYPE_TEMP    = 4, // 温度
  DS3231_TYPE_CTL     = 5,
} ds3231_rtc_data_type_t;

typedef enum _ds3231_rtc_alarm_mode_t
{
  DS3231_ALARM0_MOD_PER_SEC                 = 0,
  DS3231_ALARM0_MOD_MATCH_SEC               = 1,  
  DS3231_ALARM0_MOD_MATCH_MIN_SEC           = 2, 
  DS3231_ALARM0_MOD_MATCH_HOUR_MIN_SEC      = 3, 
  DS3231_ALARM0_MOD_MATCH_DATE_HOUR_MIN_SEC = 4,   
  DS3231_ALARM0_MOD_MATCH_DAY_HOUR_MIN_SEC  = 5,
  DS3231_ALARM0_MOD_CNT                     = 6, 
  DS3231_ALARM1_MOD_PER_MIN                 = 7,
  DS3231_ALARM1_MOD_MATCH_MIN               = 8,  
  DS3231_ALARM1_MOD_MATCH_HOUR_MIN          = 9, 
  DS3231_ALARM1_MOD_MATCH_DATE_HOUR_MIN     = 10,   
  DS3231_ALARM1_MOD_MATCH_DAY_HOUR_MIN      = 11,     
} ds3231_rtc_alarm_mode_t;

typedef enum _ds3231_rtc_square_rate_t
{
  DS3231_SQUARE_RATE_1HZ    = 0,
  DS3231_SQUARE_RATE_1024HZ = 1,
  DS3231_SQUARE_RATE_4096HZ = 2,
  DS3231_SQUARE_RATE_8192HZ = 3,    
} ds3231_rtc_square_rate_t;

typedef enum _ds3231_rtc_alarm_index_t {
  DS3231_ALARM0 = 0,
  DS3231_ALARM1
} ds3231_rtc_alarm_index_t;

// 初始化
void ds3231_rtc_init(void);
// 获取当前时间
void ds3231_rtc_get_time(uint8_t * hour, uint8_t * min, uint8_t * sec);
// 设置当前时间
void ds3231_rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec);
// 获取当前日期, 注意year为0~99，mon为1~12，date为1~31，day为1~7
bool ds3231_rtc_get_date(uint8_t * year, uint8_t * mon, uint8_t * date, uint8_t * day);
// 设置当前日期, 注意year为0~99，mon为1~12，date为1~31，day为1~7
void ds3231_rtc_set_date(bool centry, uint8_t year, uint8_t mon, uint8_t date, uint8_t day);
// 使能或禁止32KHz输出
void ds3231_rtc_enable_32768HZ(bool enable);


// 需要在调用其他接口之前调用，进行底层设备的数据读取
void ds3231_rtc_read_data(ds3231_rtc_data_type_t type);

// 需要在调用其他接口之后调用，进行底层设备的数据写入
void ds3231_rtc_write_data(ds3231_rtc_data_type_t type);

// date相关接口，调用前需要先调用ds3231_rtc_read_data(DS3231_TYPE_DATE)进行数据读取
// 读取小时，0~23
int8_t ds3231_rtc_time_get_hour(void);
// 设置小时，0~23
void ds3231_rtc_time_set_hour(uint8_t hour);
// 设置12小时制，enable为true时为12小时制，false时为24小时制
void ds3231_rtc_time_set_hour_12(bool enable);
// 获取是否为12小时制，true为12小时制，false为24小时制
bool ds3231_rtc_time_get_hour_12(void);
// 读取分钟，0~59
uint8_t ds3231_rtc_time_get_min(void);
// 设置分钟，0~59
void ds3231_rtc_time_set_min(uint8_t min);
// 读取秒钟，0~59
uint8_t ds3231_rtc_time_get_sec(void);
// 设置秒钟，0~59
void ds3231_rtc_time_set_sec(uint8_t sec);

// date相关接口，调用前需要先调用ds3231_rtc_read_data(DS3231_TYPE_DATE)进行数据读取
// 读取年份，0~99
uint8_t ds3231_rtc_date_get_year(void);
// 设置年份，0~99
void ds3231_rtc_date_set_year(uint8_t year);
// 获取世纪位，true世纪位置位，false世纪位清零
bool ds3231_rtc_date_get_century(void);
// 设置世纪位，true世纪位置位，false世纪位清零
void ds3231_rtc_date_set_century(bool set);
// 读取月份，1~12
uint8_t ds3231_rtc_date_get_month(void);
// 设置月份，1~12
void ds3231_rtc_date_set_month(uint8_t month);
// 读取日期，1~31
uint8_t ds3231_rtc_date_get_date(void);
// 设置日期，1~31
bool ds3231_rtc_date_set_date(uint8_t date);
// 读取星期，1~7
uint8_t ds3231_rtc_date_get_day(void);
//  设置星期，1~7
void ds3231_rtc_date_set_day(uint8_t day);


// alarm相关接口，调用前需要先调用ds3231_rtc_read_data(DS3231_TYPE_ALARM0)
// 或ds3231_rtc_read_data(DS3231_TYPE_ALARM1)进行数据读取
// 读取闹钟小时，0~23
uint8_t ds3231_rtc_alarm_get_hour(ds3231_rtc_alarm_index_t alarm_index);
// 设置闹钟小时，0~23
void ds3231_rtc_alarm_set_hour(ds3231_rtc_alarm_index_t alarm_index, uint8_t hour);
// 设置闹钟12小时制，enable为true时为12小时制，false时为24小时制
bool ds3231_rtc_alarm_get_hour_12(ds3231_rtc_alarm_index_t alarm_index);
// 获取闹钟是否为12小时制，true为12小时制，false为24小时制
void ds3231_rtc_alarm_set_hour_12(ds3231_rtc_alarm_index_t alarm_index, bool enable);
// 读取闹钟分钟，0~59
uint8_t ds3231_rtc_alarm_get_min(ds3231_rtc_alarm_index_t alarm_index);
// 设置闹钟分钟，0~59
void ds3231_rtc_alarm_set_min(ds3231_rtc_alarm_index_t alarm_index, uint8_t min);
// 读取闹钟秒钟，0~59
uint8_t ds3231_rtc_alarm_get_sec(ds3231_rtc_alarm_index_t alarm_index);
// 设置闹钟秒钟，0~59
void ds3231_rtc_alarm_set_sec(ds3231_rtc_alarm_index_t alarm_index, uint8_t sec);
// 读取闹钟星期，1~7
uint8_t ds3231_rtc_alarm_get_day(ds3231_rtc_alarm_index_t alarm_index);
// 设置闹钟星期，1~7
void ds3231_rtc_alarm_set_day(ds3231_rtc_alarm_index_t alarm_index, uint8_t day);
// 读取闹钟日期，1~31
uint8_t ds3231_rtc_alarm_get_date(ds3231_rtc_alarm_index_t alarm_index);
// 设置闹钟日期，1~31
void ds3231_rtc_alarm_set_date(ds3231_rtc_alarm_index_t alarm_index, uint8_t date);
// 获取闹钟模式
ds3231_rtc_alarm_mode_t ds3231_rtc_alarm_get_mode(ds3231_rtc_alarm_index_t alarm_index);
// 设置闹钟模式
void ds3231_rtc_alarm_set_mode(ds3231_rtc_alarm_index_t alarm_index, ds3231_rtc_alarm_mode_t mode);



// ds3231_rtc_read_data(DS3231_TYPE_CTL)之后调用
// 获取闹钟模式字符串
const char * ds3231_rtc_alarm_get_mode_str(ds3231_rtc_alarm_index_t alarm_index);
// 允许alarm中断
void ds3231_rtc_alarm_enable_int(ds3231_rtc_alarm_index_t index, bool enable);
// 测试alarm中断是否允许
bool ds3231_rtc_alarm_test_int(ds3231_rtc_alarm_index_t index);
// 清除alarm中断标志
void ds3231_rtc_alarm_clear_int_flag(ds3231_rtc_alarm_index_t index);
// 测试alarm中断标志
bool ds3231_rtc_alarm_test_int_flag(ds3231_rtc_alarm_index_t index);

// 控制接口
// ds3231_rtc_read_data(DS3231_TYPE_CTL)之后调用
// EOSC:
// Enable Oscillator (EOSC). When set to logic 0, the oscillator is started. 
// When set to logic 1, the oscillator is stopped when the DS3231 switches to VBAT. 
// This bool is clear (logic 0) when power is first applied. 
// When the DS3231 is powered by VCC, the oscillator is always on regardless of the status of the EOSC bool. 
bool ds3231_rtc_test_eosc(void);
void ds3231_rtc_set_eosc(bool val);

// BBSQW:
// Battery-Backed Square-Wave Enable (BBSQW). 
// When set to logic 1 and the DS3231 is being powered by the VBAT pin, 
// this bool enables the square- wave or interrupt output when VCC is absent. 
// When BBSQW is logic 0, the INT/SQW pin goes high imped- ance when VCC falls below the power-fail trip point. 
// This bool is disabled (logic 0) when power is first applied.
bool ds3231_rtc_test_bbsqw(void);
void ds3231_rtc_set_bbsqw(bool val);

// CONV:
// Convert Temperature (CONV). 
// Setting this bool to 1 forces the temperature sensor to convert the temperature 
// into digital code and execute the TCXO algorithm to update the capacitance array to the oscillator. 
// This can only happen when a conversion is not already in progress.
// The user should check the status bool BSY before forcing the controller to start a new TCXO execution.
// A user-initiated temperature conversion does not affect the internal 64-second update cycle.
// A user-initiated temperature conversion does not affect the BSY bool for approximately 2ms. 
// The CONV bool remains at a 1 from the time it is written until the conver- sion is finished, 
// at which time both CONV and BSY go to 0. 
// The CONV bool should be used when monitoring the status of a user-initiated conversion.
void ds3231_rtc_set_conv(bool val);
bool ds3231_rtc_test_conv(void);

// RS:
// Rate Select (RS2 and RS1). These bits control the frequency of the square-wave output when
// the square wave has been enabled. 
// The following table shows the square-wave frequencies that can be select- ed with the RS bits. 
// These bits are both set to logic 1 (8.192kHz) when power is first applied.
void ds3231_rtc_set_square_rate(ds3231_rtc_square_rate_t rt);
ds3231_rtc_square_rate_t ds3231_rtc_get_square_rate(void);
const char * ds3231_rtc_get_square_rate_str(void);

// INTCN:
// Interrupt Control (INTCN). 
// This bool controls the INT/SQW signal. 
// When the INTCN bool is set to logic 0, a square wave is output on the INT/SQW pin. 
// When the INTCN bool is set to logic 1, then a match between the timekeeping registers 
// and either of the alarm registers activates the INT/SQW output (if the alarm is also enabled). 
// The corresponding alarm flag is always set regardless of the state of the INTCN bool. 
// The INTCN bool is set to logic 1 when power is first applied.
void ds3231_rtc_set_intcn(bool val);
bool ds3231_rtc_test_intcn(void);


// OSF:
// Oscillator Stop Flag (OSF). A logic 1 in this bool indicates that the oscillator 
// either is stopped or was stopped for some period and may be used to judge the 
// validity of the timekeeping data. This bool is set to logic 1 any time that the oscillator stops.
// The following are exam- ples of conditions that can cause the OSF bool to be set:
// 1) The first time power is applied.
// 2) The voltages present on both VCC and VBAT are insufficient to support oscillation.
// 3) The EOSC bool is turned off in battery-backed mode.
// 4) External influences on the crystal (i.e., noise, leakage, etc.).
// This bool remains at logic 1 until written to logic 0.
bool ds3231_rtc_test_osf(void);
void ds3231_rtc_set_osf(bool val);

// EN32kHz:
// Enable 32kHz Output (EN32kHz). 
// This bool controls the status of the 32kHz pin. 
// When set to logic 1, the 32kHz pin is enabled and outputs a 32.768kHz squarewave signal. 
// When set to logic 0, the 32kHz pin goes to a high-impedance state. 
// The initial power-up state of this bool is logic 1, and a 32.768kHz square-wave signal 
// appears at the 32kHz pin after a power source is applied to the DS3231 (if the oscillator is running).
void ds3231_rtc_set_en32khz(bool val);
bool ds3231_rtc_test_en32khz(void);

// BSY:
// Busy (BSY). This bool indicates the device is busy executing TCXO functions.
// It goes to logic 1 when the conversion signal to the temperature 
// sensor is asserted and then is cleared when the device is in the 1-minute idle state.
bool ds3231_rtc_test_bsy(void);


// ds3231_rtc_read_data(DS3231_TYPE_TEMP)之后调用
// 获取温度, 返回值为摄氏度，精确到0.25度
float ds3231_rtc_get_temperature(void);


#endif  // NEO_IV_CLOCK_DS3231_RTC_H
