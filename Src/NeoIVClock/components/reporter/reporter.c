#include "reporter.h"
#include "logger.h"
#include "config.h"
#include "nm.h"
#include "cJSON.h"
#include "sensor_data.h"

static const char * TAG = "REPORTER";

/* 传感器数据上报间隔，
  0: 1min, 1: 5min, 2: 10min, 3: 15min, 4: 30min, 5: 45min, 6: 60min 
*/
static reporter_interval_t reporter_interval;
static reporter_pms_policy_t reporter_pms_policy;

void reporter_init(void)
{
  NEO_LOGI(TAG, "init");
  reporter_interval = config_read_int("reporter_interval") % REPORTER_INTERVAL_CNT;
  reporter_pms_policy = config_read_int("reporter_pms_policy") % REPORTER_PMS_POLICY_CNT;
}
static const wchar_t * reporter_interval_names[] = 
{
  L"1分钟",
  L"5分钟",  
  L"10分钟", 
  L"15分钟", 
  L"30分钟", 
  L"45分钟",
  L"60分钟"
};

const wchar_t * reporter_get_interval_str()
{
  return reporter_interval_names[reporter_interval];
}
reporter_interval_t reporter_get_interval(void)
{
  return reporter_interval;
}


reporter_interval_t reporter_next_interval(void)
{
  reporter_interval = (reporter_interval + 1) % REPORTER_INTERVAL_CNT;
  return reporter_interval;
}

static const wchar_t * reporter_pms_policy_names[] = 
{
  L"常开",
  L"30分钟",  
  L"1小时", 
};
const wchar_t * reporter_get_pms_policy_str()
{
  return reporter_pms_policy_names[reporter_pms_policy];
}
reporter_pms_policy_t reporter_get_pms_policy(void)
{
  return reporter_pms_policy;
}

reporter_pms_policy_t reporter_next_pms_policy(void)
{
  reporter_pms_policy = (reporter_pms_policy + 1) % REPORTER_PMS_POLICY_CNT;
  return reporter_pms_policy;
}

void reporter_save_config(void)
{
  config_write_int("reporter_interval", reporter_interval);
  config_write_int("reporter_pms_policy", reporter_pms_policy);  
}

static bool reporter_report_data_internal(const sensor_data_t * data)
{
  char *json_string = NULL;
  cJSON *root = NULL, *pms5003st_data = NULL, 
    *tpm300_data = NULL, *bmp280_data = NULL, *aht20_data = NULL;
  bool ret = false;

  time_t now;
  time(&now);

    // 1. 创建一个 cJSON 根对象指针
  if ((root = cJSON_CreateObject()) == NULL) {
    NEO_LOGE(TAG, "cJSON_CreateObject root failed");
    goto err;
  }

  if((pms5003st_data = cJSON_AddObjectToObject(root, "pms5003st_data")) == NULL) {
    NEO_LOGE(TAG, "cJSON_CreateObject pms5003st_data failed");
    goto err;
  }
  if((tpm300_data = cJSON_AddObjectToObject(root, "tpm300_data")) == NULL) {
    NEO_LOGE(TAG, "cJSON_CreateObject tpm300_data failed");
    goto err;
  }
  if((bmp280_data = cJSON_AddObjectToObject(root, "bmp280_data")) == NULL) {
    NEO_LOGE(TAG, "cJSON_CreateObject bmp280_data failed");
    goto err;
  }  
  if((aht20_data = cJSON_AddObjectToObject(root, "aht20_data")) == NULL) {
    NEO_LOGE(TAG, "cJSON_CreateObject aht20_data failed");
    goto err;
  }    

  // 2. 像对象中添加键值对
  cJSON_AddNumberToObject(root, "time_stamp", now);

  cJSON_AddNumberToObject(pms5003st_data, "pm_10", data->pms5003st_data.pm_10);
  cJSON_AddNumberToObject(pms5003st_data, "pm_25", data->pms5003st_data.pm_25);
  cJSON_AddNumberToObject(pms5003st_data, "pm_100", data->pms5003st_data.pm_100); 
  cJSON_AddNumberToObject(pms5003st_data, "pm_10a", data->pms5003st_data.pm_10a); 
  cJSON_AddNumberToObject(pms5003st_data, "pm_25a", data->pms5003st_data.pm_25a); 
  cJSON_AddNumberToObject(pms5003st_data, "pm_100a", data->pms5003st_data.pm_100a); 
  cJSON_AddNumberToObject(pms5003st_data, "pm_03cnt", data->pms5003st_data.pm_03cnt);
  cJSON_AddNumberToObject(pms5003st_data, "pm_05cnt", data->pms5003st_data.pm_05cnt);
  cJSON_AddNumberToObject(pms5003st_data, "pm_10cnt", data->pms5003st_data.pm_10cnt);
  cJSON_AddNumberToObject(pms5003st_data, "pm_25cnt", data->pms5003st_data.pm_25cnt); 
  cJSON_AddNumberToObject(pms5003st_data, "pm_50cnt", data->pms5003st_data.pm_50cnt); 
  cJSON_AddNumberToObject(pms5003st_data, "pm_100cnt", data->pms5003st_data.pm_100cnt);
  cJSON_AddNumberToObject(pms5003st_data, "form", data->pms5003st_data.form); 
  cJSON_AddNumberToObject(pms5003st_data, "temp", data->pms5003st_data.temp); 
  cJSON_AddNumberToObject(pms5003st_data, "mol", data->pms5003st_data.mol); 

  cJSON_AddNumberToObject(tpm300_data, "tvoc", data->tpm300_tvoc); 

  cJSON_AddNumberToObject(bmp280_data, "temp", data->bmp280_temp); 
  cJSON_AddNumberToObject(bmp280_data, "press", data->bmp280_press);

  cJSON_AddNumberToObject(aht20_data, "temp", data->aht20_temp); 
  cJSON_AddNumberToObject(aht20_data, "mol", data->aht20_mol);  

  // 3. 将 cJSON 对象转换（序列化）为纯文本字符串（不带缩进换行，最省流量）
  if((json_string = cJSON_PrintUnformatted(root)) == NULL) {
    NEO_LOGE(TAG, "cJSON_PrintUnformatted failed");
    goto err;
  }

  NEO_LOGD(TAG, "report data: %s", json_string);

  if(!(ret = nm_sent_data(json_string))) {
    NEO_LOGE(TAG, "nm_sent_data failed");
  }

err:

  if(root) {
    cJSON_Delete(root);
    root = NULL;
  }
  if(json_string) {
    cJSON_free(json_string);
  }
  return ret;
}

// 根据internval，决定是否上报数据
bool reporter_report_data(const sensor_data_t * data, uint8_t min)
{
  bool should_report = false;
  if(nm_get_state() != NM_STATE_ONLINE) {
    NEO_LOGW(TAG, "nm_state %d not online, skip report", nm_get_state());
    return false;
  }

  switch(reporter_interval) {
    case REPORTER_INTERVAL_01MIN:
      should_report = true;
      break;
    case REPORTER_INTERVAL_05MIN:
      should_report = ((min % 5) == 0);
      break;
    case REPORTER_INTERVAL_10MIN:
      should_report = ((min % 10) == 0);
      break;
    case REPORTER_INTERVAL_15MIN:
      should_report = ((min % 15) == 0);
      break;
    case REPORTER_INTERVAL_30MIN:
      should_report = ((min % 30) == 0);
      break;
    case REPORTER_INTERVAL_45MIN:
      should_report = ((min % 45) == 0);
      break;
    case REPORTER_INTERVAL_60MIN:  
      should_report = ((min % 60) == 0);
      break;
    default:;
  }

  if(!should_report) {
    NEO_LOGW(TAG, "interval not match, min = %u vs %u, skip report", min, should_report);
    return false;
  }

  return reporter_report_data_internal(data);
}