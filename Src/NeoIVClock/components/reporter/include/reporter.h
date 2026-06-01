#ifndef NEO_IV_CLOCK_REPORTER_H
#define NEO_IV_CLOCK_REPORTER_H

#include <stdint.h>
#include <wchar.h>

#include "sensor_data.h"

// 数据上报的协议封装

void reporter_init(void);

typedef enum _reporter_pms_policy_t
{
  REPORTER_PMS_POLICY_ALWAYS_ON = 0,
  REPORTER_PMS_POLICY_30MIN,
  REPORTER_PMS_POLICY_60MIN,
  REPORTER_PMS_POLICY_CNT
} reporter_pms_policy_t;

const wchar_t * reporter_get_pms_policy_str(void);
reporter_pms_policy_t reporter_get_pms_policy(void);
reporter_pms_policy_t reporter_next_pms_policy(void);


typedef enum _reporter_interval_t
{
  REPORTER_INTERVAL_01MIN = 0,
  REPORTER_INTERVAL_05MIN,
  REPORTER_INTERVAL_10MIN,
  REPORTER_INTERVAL_15MIN,
  REPORTER_INTERVAL_30MIN,
  REPORTER_INTERVAL_45MIN,
  REPORTER_INTERVAL_60MIN,
  REPORTER_INTERVAL_CNT
} reporter_interval_t;

const wchar_t * reporter_get_interval_str(void);
reporter_interval_t reporter_get_interval(void);
reporter_interval_t reporter_next_interval(void);

bool reporter_report_data(const sensor_data_t * data, uint8_t min);

void reporter_save_config(void);

#endif // NEO_IV_CLOCK_REPORTER_H
