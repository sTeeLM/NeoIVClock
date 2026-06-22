#include "blem.h"
#include "logger.h"
#include "delay.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"


static const char * TAG = "BLEM";

static TaskHandle_t blem_task_handle = NULL;
static bool blem_enabled = false;

static void blem_update_bthome_data(float temp, float hum, float press, uint32_t pm25, uint32_t pm10, float tvoc, float eco2)
{
    uint8_t bthome_adv[31] = {0};
    uint8_t bthome_scan_rsp[31] = {0};
    uint8_t idx_adv = 0;
    uint8_t idx_rsp = 0;
    int rc;
    uint8_t len_idx_adv, len_rsp_idx;
    int16_t t_val;
    uint16_t h_val;
    uint32_t p_val;
    uint32_t tvoc_val;
    uint32_t tvoc_eco2;
    struct ble_gap_adv_params adv_params = {0};


    // 标准 BLE 广播 Flags (3 字节)
    bthome_adv[idx_adv++] = 0x02; 
    bthome_adv[idx_adv++] = 0x01; 
    bthome_adv[idx_adv++] = 0x06; 

    // BTHome 报头占位（先跳过长度字节，后面动态计算）
    len_idx_adv = idx_adv; 
    idx_adv++; // 留给 bthome_adv[len_idx] 填 Service Data 长度
    bthome_adv[idx_adv++] = 0x16; // 类型: Service Data
    bthome_adv[idx_adv++] = 0xD2; // BTHome UUID 低字节 (0xFCD2)
    bthome_adv[idx_adv++] = 0xFC; // BTHome UUID 高字节
    bthome_adv[idx_adv++] = 0x40; // Device Info: V2版本，无加密

    // 填充温度：Object ID = 0x02 (2字节有符号, 放大100倍)
    t_val = (int16_t)(temp * 100);
    bthome_adv[idx_adv++] = 0x02;
    bthome_adv[idx_adv++] = (uint8_t)(t_val & 0xFF);        // 低 8 位
    bthome_adv[idx_adv++] = (uint8_t)((t_val >> 8) & 0xFF); // 高 8 位

    // 填充湿度：Object ID = 0x03 (2字节无符号, 放大100倍)
    h_val = (uint16_t)(hum * 100);
    bthome_adv[idx_adv++] = 0x03;
    bthome_adv[idx_adv++] = (uint8_t)(h_val & 0xFF);
    bthome_adv[idx_adv++] = (uint8_t)((h_val >> 8) & 0xFF);

    // 填充气压：Object ID = 0x04 (3字节无符号, 放大100倍)
    // 1013.25 hPa -> 101325 -> 0x018BCD (低位在前: CD -> 8B -> 01)
    p_val = (uint32_t)(press); // 气压单位通常是 hPa，由于传入的参数是Pa，所以不需要乘以100
    bthome_adv[idx_adv++] = 0x04;
    bthome_adv[idx_adv++] = (uint8_t)(p_val & 0xFF);
    bthome_adv[idx_adv++] = (uint8_t)((p_val >> 8) & 0xFF);
    bthome_adv[idx_adv++] = (uint8_t)((p_val >> 16) & 0xFF); // 确认为 3 字节 uint24

    // 回填精确的数据包长度
    // 长度 = 总体写入字节数 - 核心 Flags占用的3字节 - 长度自身占用的1字节
    bthome_adv[len_idx_adv] = idx_adv - 4; 

    // Complete Local Name
    bthome_adv[idx_adv++] = 0x0B;   // 长度：后面 1 字节类型 + 10 字节文本 = 11 字节
    bthome_adv[idx_adv++] = 0x09;   // 类型标志：0x09 代表 Complete Local Name (完整本地设备名)
    bthome_adv[idx_adv++] = 'N';
    bthome_adv[idx_adv++] = 'e';
    bthome_adv[idx_adv++] = 'o';
    bthome_adv[idx_adv++] = 'I';
    bthome_adv[idx_adv++] = 'V';
    bthome_adv[idx_adv++] = 'C';
    bthome_adv[idx_adv++] = 'l';
    bthome_adv[idx_adv++] = 'o';
    bthome_adv[idx_adv++] = 'c';
    bthome_adv[idx_adv++] = 'k';  

    len_rsp_idx = idx_rsp; 
    idx_rsp++; // 留着回填响应包 BTHome 数据长度
    bthome_scan_rsp[idx_rsp++] = 0x16; 
    bthome_scan_rsp[idx_rsp++] = 0xD2; 
    bthome_scan_rsp[idx_rsp++] = 0xFC; 
    bthome_scan_rsp[idx_rsp++] = 0x40;
    
    // 填充 PM2.5：Object ID = 0x0D (确认为 2 字节无符号 uint16, 1:1不放大)
    bthome_scan_rsp[idx_rsp++] = 0x0D;
    bthome_scan_rsp[idx_rsp++] = (uint8_t)(pm25 & 0xFF);
    bthome_scan_rsp[idx_rsp++] = (uint8_t)((pm25 >> 8) & 0xFF); // 只占 2 字节！

    // 填充 PM10：Object ID = 0x0E (确认为 2 字节无符号 uint16, 1:1不放大)
    bthome_scan_rsp[idx_rsp++] = 0x0E;
    bthome_scan_rsp[idx_rsp++] = (uint8_t)(pm10 & 0xFF);
    bthome_scan_rsp[idx_rsp++] = (uint8_t)((pm10 >> 8) & 0xFF); // 只占 2 字节！

    // 填充 TVOC：Object ID = 0x13 (确认为 2 字节无符号 uint16, 1:1不放大)
    tvoc_val = (uint32_t)(tvoc * 4.09f); // 从ppb换算成ug/m3
    bthome_scan_rsp[idx_rsp++] = 0x13;
    bthome_scan_rsp[idx_rsp++] = (uint8_t)(tvoc_val & 0xFF);
    bthome_scan_rsp[idx_rsp++] = (uint8_t)((tvoc_val >> 8) & 0xFF); // 只占 2 字节！

    // 填充 eCO2：Object ID = 0x12 (确认为 2 字节无符号 uint16, 1:1不放大)
    tvoc_eco2 = (uint32_t)(eco2);
    bthome_scan_rsp[idx_rsp++] = 0x12;
    bthome_scan_rsp[idx_rsp++] = (uint8_t)(tvoc_eco2 & 0xFF);
    bthome_scan_rsp[idx_rsp++] = (uint8_t)((tvoc_eco2 >> 8) & 0xFF); // 只占 2 字节！

    // 回填响应包中 BTHome 数据段的精确字节长度
    bthome_scan_rsp[len_rsp_idx] = idx_rsp - (len_rsp_idx + 1);

    // 停止广播 -> 写入干净的数据 -> 重启
    ble_gap_adv_stop();

    rc = ble_gap_adv_set_data(bthome_adv, idx_adv);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set BLE adv data: %d", rc);
        return;
    } else {
        NEO_LOGI(TAG, "BTHome report adv data: %d bytes", idx_adv);
    }

    rc = ble_gap_adv_rsp_set_data(bthome_scan_rsp, idx_rsp);
    if (rc != 0) {
        NEO_LOGE(TAG, "Failed to set BLE adv rsp data: %d", rc);
        return;
    } else {
        NEO_LOGI(TAG, "BTHome report scan response data: %d bytes", idx_rsp);
    }  

    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(200);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(300);
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc == 0) {
        NEO_LOGI(TAG, "BTHome report data: %d + %d bytes", idx_adv, idx_rsp);
    } else {
        NEO_LOGE(TAG, "Failed to start BLE advertising: %d", rc);
    }
    
}
static void blem_start(void);

static void blem_app_on_sync(void) 
{
    if(blem_enabled) {
      blem_start();
      // 首次初始化时，先填入一组默认的初始数据
      blem_update_bthome_data(25.0f, 50.0f, 4.20f, 0, 0, 0, 0);       
    }
}

void blem_host_task(void *param) 
{
    nimble_port_run(); // 阻塞运行，维持蓝牙协议栈
}

static void blem_start(void) 
{
  // 配置并【单次启动】底层 GAP 广播
  struct ble_gap_adv_params adv_params;
  int rc = 0;

  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_NON; 
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; 
  adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(200); // 正常工作建议调到 200~300ms 发一次，更省电
  adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(300); 

  rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
  if (rc == 0) {
     NEO_LOGI(TAG, "BLE Broadcast started successfully");
  } else {
    NEO_LOGE(TAG, "BLE Broadcast failed: %d", rc);
  }
}

static void blem_stop(void) 
{
    ble_gap_adv_stop();
}

bool blem_test_enabled(void)
{
  return blem_enabled;
}

bool blem_enable(bool enable)
{
  if(enable && !blem_enabled) {
    // 这里不需要做什么，因为 BLE 广播已经在初始化时启动了，后续通过更新广告数据来上报即可
    NEO_LOGI(TAG, "BLE enabled");
    blem_start();
    blem_enabled = true;
  } else if(!enable && blem_enabled) {
    blem_stop();
    NEO_LOGI(TAG, "BLE disabled");
    blem_enabled = false;
  }
  return blem_enabled;
}

void blem_init(void)
{
  bool enable = false;
  NEO_LOGI(TAG, "init");

  blem_enabled = false;
  
  enable = config_read_int("bthome_en") != 0;
  
  ESP_ERROR_CHECK(nimble_port_init());
  ble_svc_gap_device_name_set("IVClock");
  ble_hs_cfg.sync_cb = blem_app_on_sync;
  // 启动蓝牙宿主线
  xTaskCreate(blem_host_task, "ble_host_task", 4096, NULL, 5, &blem_task_handle);

  blem_enable(enable);
}

void blem_save_config(void)
{
  config_write_int("bthome_en", blem_enabled ? 1 : 0);
}

bool blem_report_data(float temp, float hum, float press, uint32_t pm25, uint32_t pm10, float tvoc, float eco2)
{
  if(blem_enabled) {
    NEO_LOGI(TAG, "report data: temp %.2f, humi %.2f, press %.2f, pm25 %u, tvoc %.2f", temp, hum, press, pm25, tvoc);
    blem_update_bthome_data(temp, hum, press, pm25, pm10, tvoc, eco2);
  } else {
    NEO_LOGW(TAG, "BLE is disabled, cannot report data");
  }
  return true;
}
