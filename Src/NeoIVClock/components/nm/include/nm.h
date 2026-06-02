#ifndef NEO_IV_CLOCK_NM_H
#define NEO_IV_CLOCK_NM_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/dns.h"

#define NM_DEVICE_ID_MAX 16
#define NM_SERVER_URL_MAX  128
#define NM_SSID_MAX  32
#define NM_USER_MAX  64
#define NM_PASS_MAX  64

typedef enum _nm_state_t
{
  NM_STATE_NONE = 0,
  NM_STATE_CONNECTING,    // 连接AP中
  NM_STATE_CONNECTED,     // 连接成功
  NM_STATE_DISCONNECTED,  // 断开
  NM_STATE_ONLINE,        // 获得IP
  NM_STATE_OFFLINE,       // IP丢失
  NM_STATE_CONFIG         // 处于配网状态
} nm_state_t;

void nm_init(void);
nm_state_t nm_get_state(void);

// 连接AP，并且启动监测连接状态线程
void nm_start_sta_daemon();
void nm_stop_sta_daemon();

// 启动soft-ap，创建配置服务器
void nm_start_confg_portal(void); 
void nm_stop_confg_portal(void);

bool nm_sent_data(const char * json);

// 获取信息, 仅在NM_STATE_ONLINE时有效
bool nm_get_info(
  esp_netif_ip_info_t * ip_info, 
  ip_addr_t * dns1, 
  ip_addr_t * dns2,
  ip_addr_t * dns3,  
  uint8_t mac[6]);

const char * nm_get_ssid(void);
const char * nm_get_config_ssid(void);
const char * nm_get_device_id(void);

#endif //NEO_IV_CLOCK_NM_H