#include "network_manager.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include "logger.h"
#include "config.h"

#include <string.h>

static const char *TAG = "NETWORK";

// 热点配置
#define AP_SSID "NeoIVClock"
#define AP_PASS ""
#define MAX_SCANNED_AP_CNT 20

// 全局句柄管理：用于后续停止服务时精准销毁
static httpd_handle_t network_manager_httpd_handle = NULL;
static TaskHandle_t network_manager_dns_task_handle = NULL;

// ==========================================
// 1. 静态分配 PSRAM 大缓冲区
// ==========================================
// 关键：无 malloc 内存申请，使用静态指定且落入外部 PSRAM (默认初始化为 0)

// 用于缓存后台扫描到的周围 Wi-Fi SSID
EXT_RAM_BSS_ATTR static char network_manager_scanned_ssids[MAX_SCANNED_AP_CNT]
                                                          [33];
EXT_RAM_BSS_ATTR static int network_manager_scanned_ap_count = 0;
EXT_RAM_BSS_ATTR wifi_ap_record_t
    network_manager_ap_records[MAX_SCANNED_AP_CNT];
EXT_RAM_BSS_ATTR static char network_manager_dynamic_html_buffer[8192];
EXT_RAM_BSS_ATTR static uint8_t network_manager_dns_rx_buffer[1024];

EXT_RAM_BSS_ATTR static char network_manager_wifi_ssid[33];
EXT_RAM_BSS_ATTR static char network_manager_wifi_pass[33];

void network_manager_init(void) {
  esp_err_t ret = ESP_OK;
  config_val_t val;
  NEO_LOGI(TAG, "network_manager_init");

  val.valblob.body = (const uint8_t *)network_manager_wifi_ssid;
  val.valblob.len = sizeof(network_manager_wifi_ssid);
  config_read("wifi_ssid", &val);
  network_manager_wifi_ssid[32] = 0;

  val.valblob.body = (const uint8_t *)network_manager_wifi_pass;
  val.valblob.len = sizeof(network_manager_wifi_pass);
  config_read("wifi_pass", &val);  
  network_manager_wifi_pass[32] = 0;

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
}

// ==========================================
// 2. HTTP 服务器请求处理句柄
// ==========================================

// 动态页面拼接句柄
static esp_err_t nm_index_get_handler(httpd_req_t *req) {
  // 使用预先分配在 PSRAM 中的全局静态大缓冲区，直接规避 malloc
  memset(network_manager_dynamic_html_buffer, 0, sizeof(network_manager_dynamic_html_buffer));

  strcpy(network_manager_dynamic_html_buffer,
         "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
         "<meta name=\"viewport\" content=\"width=device-width, "
         "initial-scale=1.0\">"
         "<title>NEOIVClock 配置中心</title>"
         "<style>body{font-family:sans-serif;margin:20px;text-align:center;} "
         "select, input{width:85%;padding:10px;margin:10px "
         "0;box-sizing:border-box;font-size:16px;}</style>"
         "</head><body>"
         "<h2>ESP32 系统配置</h2>"
         "<form action=\"/save\" method=\"POST\">"
         "选择 Wi-Fi 网络:<br>"
         "<select name=\"ssid\">");

  if (network_manager_scanned_ap_count == 0) {
    strcat(network_manager_dynamic_html_buffer,
           "<option value=\"\">未扫描到网络，请刷新页面</option>");
  } else {
    for (int i = 0; i < network_manager_scanned_ap_count; i++) {
      if (strlen(network_manager_scanned_ssids[i]) == 0)
        continue;
      char option_buf[128];
      snprintf(
          option_buf, sizeof(option_buf), "<option value=\"%s\">%s</option>",
          network_manager_scanned_ssids[i], network_manager_scanned_ssids[i]);
      strcat(network_manager_dynamic_html_buffer, option_buf);
    }
  }

  strcat(network_manager_dynamic_html_buffer,
         "</select><br>"
         "Wi-Fi 密码:<br><input type=\"password\" name=\"pass\" "
         "placeholder=\"请输入密码\"><br>"
         "NTP 时间服务器:<br><input type=\"text\" name=\"ntp\" "
         "value=\"server.madcat.cc\"><br><br>"
         "<input type=\"submit\" value=\"保存配置并重启\" "
         "style=\"background-color:#007BFF;color:white;border:none;cursor:"
         "pointer;font-size:18px;\">"
         "</form></body></html>");

  httpd_resp_set_status(req, "200 OK");
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_send(req, network_manager_dynamic_html_buffer, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

// 接收表单 POST 提交的数据
static esp_err_t nm_httpd_save_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  NEO_LOGI(TAG, "get form request: %s", buf);

  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_send(req, "<h3>配置成功！正在保存并重启...</h3>",
                  HTTPD_RESP_USE_STRLEN);

  vTaskDelay(pdMS_TO_TICKS(2000));
  esp_restart();
  return ESP_OK;
}

// 通配符拦截句柄
static esp_err_t nm_captive_portal_redirect_handler(httpd_req_t *req) {
  NEO_LOGI(TAG, "get probe request: %s", req->uri);
  return nm_index_get_handler(req);
}

// 启动本地网页服务器
static void network_manager_start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 4;
  config.ctrl_port = 32768;
  config.uri_match_fn = httpd_uri_match_wildcard;

  if (httpd_start(&network_manager_httpd_handle, &config) == ESP_OK) {
    httpd_uri_t save_uri = {.uri = "/save",
                            .method = HTTP_POST,
                            .handler = nm_httpd_save_handler,
                            .user_ctx = NULL};
    httpd_register_uri_handler(network_manager_httpd_handle, &save_uri);

    httpd_uri_t index_uri = {.uri = "/index.html",
                             .method = HTTP_GET,
                             .handler = nm_index_get_handler,
                             .user_ctx = NULL};
    httpd_register_uri_handler(network_manager_httpd_handle, &index_uri);

    httpd_uri_t captive_uri = {.uri = "*",
                               .method = HTTP_GET,
                               .handler = nm_captive_portal_redirect_handler,
                               .user_ctx = NULL};
    httpd_register_uri_handler(network_manager_httpd_handle, &captive_uri);
    NEO_LOGI(TAG, "httpd start ok");
  }
}

// ==========================================
// 3. DNS 劫持任务（使用静态 PSRAM 缓冲区接收网络包）
// ==========================================
static void network_manager_dns_server_task(void *pvParameters) {
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    NEO_LOGE(TAG, "can not create DNS socket");
    vTaskDelete(NULL);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(53);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    NEO_LOGE(TAG, "DNS socket bind failed");
    close(sock);
    vTaskDelete(NULL);
  }

  NEO_LOGI(TAG, "dns hijack is running");

  while (1) {
    // 使用 PSRAM 中分配的静态全局缓冲区接收 UDP 网络数据
    int len = recvfrom(sock, network_manager_dns_rx_buffer, sizeof(network_manager_dns_rx_buffer) - 32, 0,
                       (struct sockaddr *)&client_addr, &client_addr_len);
    if (len > 12) {
      network_manager_dns_rx_buffer[2] |= 0x80;
      network_manager_dns_rx_buffer[3] |= 0x80;
      network_manager_dns_rx_buffer[7] = 1;

      network_manager_dns_rx_buffer[len++] = 0xc0;
      network_manager_dns_rx_buffer[len++] = 0x0c;
      network_manager_dns_rx_buffer[len++] = 0x00;
      network_manager_dns_rx_buffer[len++] = 0x01;
      network_manager_dns_rx_buffer[len++] = 0x00;
      network_manager_dns_rx_buffer[len++] = 0x01;
      network_manager_dns_rx_buffer[len++] = 0x00;
      network_manager_dns_rx_buffer[len++] = 0x00;
      network_manager_dns_rx_buffer[len++] = 0x00;
      network_manager_dns_rx_buffer[len++] = 0x3c;
      network_manager_dns_rx_buffer[len++] = 0x00;
      network_manager_dns_rx_buffer[len++] = 0x04;
      network_manager_dns_rx_buffer[len++] = 192;
      network_manager_dns_rx_buffer[len++] = 168;
      network_manager_dns_rx_buffer[len++] = 4;
      network_manager_dns_rx_buffer[len++] = 1;

      sendto(sock, network_manager_dns_rx_buffer, len, 0, (struct sockaddr *)&client_addr,
             client_addr_len);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ==========================================
// 4. Wi-Fi AP + STA 混合模式初始化与扫描
// ==========================================
void network_manager_wifi_init_ap_sta(void) {
  uint16_t ap_num = MAX_SCANNED_AP_CNT;
  wifi_scan_config_t scan_config = {.show_hidden = false};

  esp_netif_create_default_wifi_ap();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_ap_config = {
      .ap =
          {
              .ssid = AP_SSID,
              .ssid_len = strlen(AP_SSID),
              .password = "",
              .max_connection = 4,
              .authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  if (strlen(AP_PASS) == 0) {
    wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  NEO_LOGI(TAG, "wifi ap start ok, SSID:%s", AP_SSID);

  NEO_LOGI(TAG, "scan ssid...");

  esp_wifi_scan_start(&scan_config, true);

  esp_wifi_scan_get_ap_num(&ap_num);
  esp_wifi_scan_get_ap_records(&ap_num, network_manager_ap_records);

  network_manager_scanned_ap_count = ap_num;
  for (int i = 0; i < network_manager_scanned_ap_count; i++) {
    strncpy(network_manager_scanned_ssids[i],
            (char *)network_manager_ap_records[i].ssid,
            sizeof(network_manager_scanned_ssids[i]));
    NEO_LOGD(TAG, "found: %s", network_manager_scanned_ssids);
  }
}

// ==========================================
// 5. 释放销毁函数
// ==========================================
void network_manager_stop_confg_portal(void) {
  NEO_LOGI(TAG, "stop confg portal...");

  // 1. 终止 DNS 任务
  if (network_manager_dns_task_handle != NULL) {
    vTaskDelete(network_manager_dns_task_handle);
    network_manager_dns_task_handle = NULL;
    NEO_LOGI(TAG, "dns hijack stopped");
  }

  // 2. 停止 HTTP 网页服务器
  if (network_manager_httpd_handle != NULL) {
    if (httpd_stop(network_manager_httpd_handle) == ESP_OK) {
      network_manager_httpd_handle = NULL;
      NEO_LOGI(TAG, "httpd stopped");
    }
  }

  // 3. 关闭 Wi-Fi 芯片发射
  esp_err_t err = esp_wifi_stop();
  if (err == ESP_OK) {
    NEO_LOGI(TAG, "wifi ap stopped");
  }

  // 4. 释放计数器状态
  network_manager_scanned_ap_count = 0;
  NEO_LOGI(TAG, "all done");
}

void network_manager_start_confg_portal(void) {
  network_manager_wifi_init_ap_sta();
  network_manager_start_webserver();
  xTaskCreate(network_manager_dns_server_task, "NM DNS", 4096, NULL, 5,
              &network_manager_dns_task_handle);
}

//-----------------------------------------------------------------------

/* 用于同步连接结果的事件组 */
static EventGroupHandle_t network_manager_sta_wifi_event_group = NULL;
#define STA_WIFI_CONNECTED_BIT BIT0
#define STA_WIFI_FAIL_BIT BIT1

static int network_manager_sta_retry_num = 0;
#define NETWORK_MANAGER_STA_MAXIMUM_RETRY 5 // 最大重试断线重连次数

/* STA 模式专用的事件监听回调 */
static void nm_sta_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (network_manager_sta_retry_num < NETWORK_MANAGER_STA_MAXIMUM_RETRY) {
      esp_wifi_connect();
      network_manager_sta_retry_num++;
      NEO_LOGW(TAG, "connect failed, retry %d ...",
               network_manager_sta_retry_num);
    } else {
      // 超过最大重试次数，认为本次连接彻底失败
      if (network_manager_sta_wifi_event_group) {
        xEventGroupSetBits(network_manager_sta_wifi_event_group,
                           STA_WIFI_FAIL_BIT);
      }
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    NEO_LOGW(TAG, "get ip address: " IPSTR, IP2STR(&event->ip_info.ip));
    network_manager_sta_retry_num = 0;
    // 获取到 IP，说明网络彻底打通
    if (network_manager_sta_wifi_event_group) {
      xEventGroupSetBits(network_manager_sta_wifi_event_group,
                         STA_WIFI_CONNECTED_BIT);
    }
  }
}

/**
 * @brief 连接到指定的特定 Wi-Fi 网络（STA模式）
 *
 * @param[in] target_ssid      目标 Wi-Fi 的名称字符串
 * @param[in] target_password  目标 Wi-Fi 的密码字符串
 * @return bool                true 表示连接成功并成功获取到 IP；false
 * 表示连接失败
 */
bool network_manager_connect_to_ap(const char *target_ssid,
                                   const char *target_password) {
  esp_netif_t *sta_netif;
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err = esp_wifi_init(&cfg);
  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  wifi_config_t wifi_config = {0};
  EventBits_t bits;
  bool connect_success = false;

  if (target_ssid == NULL || strlen(target_ssid) == 0) {
    NEO_LOGE(TAG, "ssid is null or empty");
    return false;
  }

  network_manager_sta_retry_num = 0;
  network_manager_sta_wifi_event_group = xEventGroupCreate();
  if (network_manager_sta_wifi_event_group == NULL) {
    return false;
  }

  // 1.
  // 初始化底层的网络接口（如果您的底层已经有别处初始化过了，这里可以根据需要跳过）
  // esp_netif_init();
  // esp_event_loop_create_default();

  // 创建 STA 默认的网络接口实例（确保系统里没有重复创建）
  sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (sta_netif == NULL) {
    sta_netif = esp_netif_create_default_wifi_sta();
  }

  // 2. 初始化 Wi-Fi 驱动配置
  if (err != ESP_OK &&
      err != ESP_ERR_WIFI_INIT_STATE) { // 过滤掉已经初始化过的报错
    vEventGroupDelete(network_manager_sta_wifi_event_group);
    return false;
  }

  // 3. 注册事件监听器
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &nm_sta_event_handler, NULL,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &nm_sta_event_handler, NULL,
      &instance_got_ip));

  // 4. 配置目标无线网的基本参数
  strncpy((char *)wifi_config.sta.ssid, target_ssid,
          sizeof(wifi_config.sta.ssid));
  if (target_password != NULL) {
    strncpy((char *)wifi_config.sta.password, target_password,
            sizeof(wifi_config.sta.password));
  }
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // 最低安全保障阈值

  // 5. 将工作模式强转为 STA，应用配置并立刻启动
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  NEO_LOGI(TAG, "start conn Wi-Fi: %s ...", target_ssid);

  // 6. 阻塞等待信号同步（成功拿到 IP 或者断开彻底失败）
  bits = xEventGroupWaitBits(network_manager_sta_wifi_event_group,
                             STA_WIFI_CONNECTED_BIT | STA_WIFI_FAIL_BIT,
                             pdFALSE, pdFALSE, portMAX_DELAY);

  // 7. 处理连接状态结果
  if (bits & STA_WIFI_CONNECTED_BIT) {
    NEO_LOGI(TAG, "connect to AP %s success", target_ssid);
    connect_success = true;
  } else if (bits & STA_WIFI_FAIL_BIT) {
    NEO_LOGI(TAG, "connect to AP %s failed", target_ssid);
    connect_success = false;
  }

  // 清理本次连接临时申请的事件及组资源（防止内存泄漏）
  esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        instance_got_ip);
  esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        instance_any_id);
  vEventGroupDelete(network_manager_sta_wifi_event_group);
  network_manager_sta_wifi_event_group = NULL;

  return connect_success;
}

void network_manager_disconnect_to_ap(void) {
  esp_err_t err = ESP_OK;
  NEO_LOGI(TAG, "close connection of Wi-Fi STA...");
  // 1. 主动向路由器发起断开请求
  err = esp_wifi_disconnect();
  if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_CONNECT) {
    NEO_LOGE(TAG, "disconnect failed: %s", esp_err_to_name(err));
    return;
  }

  // 2. 彻底停止 Wi-Fi 驱动的 STA 射频工作，释放硬件射频资源
  // 注意：如果是通过 esp_wifi_stop() 停止，芯片会关闭整个 STA 射频
  err = esp_wifi_stop();
  if (err == ESP_OK) {
    NEO_LOGI(TAG, "Wi-Fi STA close successed!");
  } else {
    NEO_LOGE(TAG, "Wi-Fi STA close failed: %s", esp_err_to_name(err));
  }
}