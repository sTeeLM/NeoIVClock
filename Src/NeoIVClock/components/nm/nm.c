#include "esp_attr.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_system.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "esp_netif_sntp.h"
#include "esp_mac.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "nm.h"
#include "logger.h"
#include "delay.h"
#include "config.h"
#include "task.h"
#include "cext.h"

#include <string.h>

static const char *TAG = "NETWORK";

// 热点配置
#define AP_SSID "NeoIVClock"
#define AP_PASS ""
#define MAX_SCANNED_AP_CNT 20

// 全局句柄管理：用于后续停止服务时精准销毁
static httpd_handle_t nm_httpd_handle = NULL;
static TaskHandle_t nm_dns_task_handle = NULL;
static esp_netif_t *nm_ap_netif;
static int nm_dns_socket;

// ==========================================
// 1. 静态分配 PSRAM 大缓冲区
// ==========================================
// 关键：无 malloc 内存申请，使用静态指定且落入外部 PSRAM (默认初始化为 0)

// 用于缓存后台扫描到的周围 Wi-Fi SSID
EXT_RAM_BSS_ATTR static char nm_scanned_ssids[MAX_SCANNED_AP_CNT][32];
EXT_RAM_BSS_ATTR static int nm_scanned_ap_count = 0;
EXT_RAM_BSS_ATTR wifi_ap_record_t nm_ap_records[MAX_SCANNED_AP_CNT];
EXT_RAM_BSS_ATTR static char nm_dynamic_html_buffer[10240];
EXT_RAM_BSS_ATTR static char nm_dynamic_resp_buffer[10240];
EXT_RAM_BSS_ATTR static uint8_t nm_dns_rx_buffer[1024];
EXT_RAM_BSS_ATTR static char nm_parsed_device_id[NM_DEVICE_ID_MAX+1];
EXT_RAM_BSS_ATTR static char nm_parsed_ssid[NM_SSID_MAX+1];   // Wi-Fi SSID 最大 32 字节 + '\0'
EXT_RAM_BSS_ATTR static char nm_parsed_pass[NM_PASS_MAX+1];   // WPA2 密码最大 64 字节 + '\0'
EXT_RAM_BSS_ATTR static char nm_parsed_ntp[NM_SERVER_URL_MAX+1];  // 域名通常限制在 128 字节内 + '\0'
EXT_RAM_BSS_ATTR static char nm_parsed_report_url[NM_SERVER_URL_MAX+1];
EXT_RAM_BSS_ATTR static char nm_parsed_report_user[NM_USER_MAX+1];
EXT_RAM_BSS_ATTR static char nm_parsed_report_pass[NM_PASS_MAX+1];

static bool nm_is_purposely_disconnected;
static bool nm_is_sntp_started;
static esp_event_handler_instance_t  nm_sta_instance_any_id;
static esp_event_handler_instance_t  nm_sta_instance_got_ip;
static esp_netif_t *nm_sta_netif;
EXT_RAM_BSS_ATTR static char nm_device_id[NM_DEVICE_ID_MAX+1];
EXT_RAM_BSS_ATTR static char nm_wifi_ssid[NM_SSID_MAX+1];
EXT_RAM_BSS_ATTR static char nm_wifi_pass[NM_PASS_MAX+1];
EXT_RAM_BSS_ATTR static char nm_ntp_server[NM_SERVER_URL_MAX+1];
EXT_RAM_BSS_ATTR static char nm_report_url[NM_SERVER_URL_MAX+1];
EXT_RAM_BSS_ATTR static char nm_report_user[NM_USER_MAX+1];
EXT_RAM_BSS_ATTR static char nm_report_pass[NM_PASS_MAX+1];

#define NM_HTTP_CLIENT_TIMEO_MS 5000

static nm_state_t  nm_state;

void nm_init(void)
{
  esp_err_t ret = ESP_OK;
  config_val_t val;
  NEO_LOGI(TAG, "nm");

  val.valblob.body = (const uint8_t *)nm_device_id;
  val.valblob.len = sizeof(nm_device_id) - 1;
  config_read("device_id", &val);
  nm_device_id[sizeof(nm_device_id) - 1] = 0;

  val.valblob.body = (const uint8_t *)nm_wifi_ssid;
  val.valblob.len = sizeof(nm_wifi_ssid) - 1;
  config_read("wifi_ssid", &val);
  nm_wifi_ssid[sizeof(nm_wifi_ssid) - 1] = 0;

  val.valblob.body = (const uint8_t *)nm_wifi_pass;
  val.valblob.len = sizeof(nm_wifi_pass) - 1;
  config_read("wifi_pass", &val);  
  nm_wifi_pass[sizeof(nm_wifi_pass) - 1] = 0;

  val.valblob.body = (const uint8_t *)nm_ntp_server;
  val.valblob.len = sizeof(nm_ntp_server) - 1;
  config_read("ntp_server", &val);  
  nm_ntp_server[sizeof(nm_ntp_server) - 1] = 0;  

  val.valblob.body = (const uint8_t *)nm_report_url;
  val.valblob.len = sizeof(nm_report_url) - 1;
  config_read("report_url", &val);  
  nm_report_url[sizeof(nm_report_url) - 1] = 0; 

  val.valblob.body = (const uint8_t *)nm_report_user;
  val.valblob.len = sizeof(nm_report_user) - 1;
  config_read("report_user", &val);  
  nm_report_user[sizeof(nm_report_user) - 1] = 0;  

  val.valblob.body = (const uint8_t *)nm_report_pass;
  val.valblob.len = sizeof(nm_report_pass) - 1;
  config_read("report_pass", &val);  
  nm_report_pass[sizeof(nm_report_pass) - 1] = 0;   

  NEO_LOGD(TAG, "device id %s", nm_device_id);

  if(nm_wifi_ssid[0]) {
    NEO_LOGD(TAG, "wifi ssid %s", nm_wifi_ssid);
  }

  if(nm_wifi_pass[0]) {
    NEO_LOGD(TAG, "wifi pass %s", nm_wifi_pass);
  }  

  if(nm_ntp_server[0]) {
    NEO_LOGD(TAG, "ntp server %s", nm_ntp_server);
  }   

  if(nm_report_url[0]) {
    NEO_LOGD(TAG, "wifi pass %s", nm_report_url);
  }  

  if(nm_report_user[0]) {
    NEO_LOGD(TAG, "ntp server %s", nm_report_user);
  }   

  if(nm_report_pass[0]) {
    NEO_LOGD(TAG, "ntp server %s", nm_report_pass);
  }   

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  nm_state = NM_STATE_DISCONNECTED;
}

// ==========================================
// 2. HTTP 服务器请求处理句柄
// ==========================================

// 动态页面拼接句柄
static esp_err_t nm_index_get_handler(httpd_req_t *req) 
{
  
  size_t nm_pos = 0;

  // 使用预先分配在 PSRAM 中的全局静态大缓冲区，直接规避 malloc
  memset(nm_dynamic_html_buffer, 0, sizeof(nm_dynamic_html_buffer));

  nm_pos += snprintf(nm_dynamic_html_buffer + nm_pos,
                     sizeof(nm_dynamic_html_buffer) - nm_pos,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, "
        "initial-scale=1.0\">"
        "<title>NEOIVClock 配置中心</title>"
        "<style>body{font-family:sans-serif;margin:20px;text-align:center;} "
        "select, input{width:85;padding:10px;margin:10px "
        "0;box-sizing:border-box;font-size:16px;}</style>"
        "</head><body>"
        "<h2>ESP32 系统配置</h2>"
        "<form action=\"/save\" method=\"POST\">"
        "选择 Wi-Fi 网络:<br>"
        "<select name=\"ssid\">");

  if (nm_scanned_ap_count == 0) {
    nm_pos += snprintf(nm_dynamic_html_buffer + nm_pos,
                       sizeof(nm_dynamic_html_buffer) - nm_pos,
        "<option value=\"\">未扫描到网络，请刷新页面</option>");
  } else {
    for (int i = 0; i < nm_scanned_ap_count; i++) {
      if (strlen(nm_scanned_ssids[i]) == 0)
        continue;
      if(nm_pos >= sizeof(nm_dynamic_html_buffer)) {
        break;
      }
      nm_pos += snprintf(nm_dynamic_html_buffer + nm_pos,
                         sizeof(nm_dynamic_html_buffer) - nm_pos,
                         "<option value=\"%s\">%s</option>",
                         nm_scanned_ssids[i], nm_scanned_ssids[i]);
    }
  }

  nm_pos += snprintf(nm_dynamic_html_buffer + nm_pos,
                     sizeof(nm_dynamic_html_buffer) - nm_pos,
        "</select><br>"
        "Wi-Fi 密码:<br><input type=\"password\" name=\"pass\" "
        "placeholder=\"请输入密码\" value=\"%s\"><br>"
        "Device ID:<br><input type=\"text\" name=\"device_id\" "
        "value=\"%s\"><br>"
        "NTP 时间服务器:<br><input type=\"text\" name=\"ntp\" "
        "value=\"%s\"><br>"
        "上报URL:<br><input type=\"text\" name=\"rurl\" "
        "value=\"%s\"><br>"
        "用户名:<br><input type=\"text\" name=\"ruser\" "
        "value=\"%s\"><br>"
        "密码:<br><input type=\"password\" name=\"rpass\" "
        "placeholder=\"请输入密码\" value=\"%s\"><br>"
        "<br><input type=\"submit\" value=\"保存配置\" "
        "style=\"background-color:#007BFF;color:white;border:none;cursor:"
        "pointer;font-size:18px;\">"
        "</form></body></html>",
        nm_wifi_pass, nm_device_id, nm_ntp_server, nm_report_url, nm_report_user, nm_report_pass);

  if (nm_pos >= sizeof(nm_dynamic_html_buffer)) {
    nm_dynamic_html_buffer[sizeof(nm_dynamic_html_buffer) - 1] = '\0';
  }

  httpd_resp_set_status(req, "200 OK");
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_send(req, nm_dynamic_html_buffer, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

static const char nm_response_format_str[] =       
       "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, "
        "initial-scale=1.0\">"
        "<title>NEOIVClock 配置中心</title>"
        "<style>body{font-family:sans-serif;margin:20px;text-align:center;} "
        "select, input{width:85;padding:10px;margin:10px "
        "0;box-sizing:border-box;font-size:16px;}</style>"
        "</head><body>"
        "<h2> %s </h2>"
        "</body></html>";

// 接收表单 POST 提交的数据
static esp_err_t nm_httpd_save_handler(httpd_req_t *req) 
{
  config_val_t val;
  bool has_device_id, has_ssid, has_pass, has_ntp, has_report_url, has_report_user, has_report_pass;

  // 使用预先分配在 PSRAM 中的全局静态大缓冲区，直接规避 malloc
  memset(nm_dynamic_html_buffer, 0, sizeof(nm_dynamic_html_buffer));
  memset(nm_dynamic_resp_buffer, 0, sizeof(nm_dynamic_resp_buffer));

  int ret = httpd_req_recv(req, nm_dynamic_html_buffer, sizeof(nm_dynamic_html_buffer) - 1);
  if (ret <= 0) {
    // [异常: 接收流异常断开或为空]
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive POST data");
    return ESP_FAIL;
  }
  nm_dynamic_html_buffer[ret] = '\0';

  NEO_LOGI(TAG, "get form request: %s", nm_dynamic_html_buffer);

  // 初始化静态结果缓冲区
  memset(nm_parsed_device_id, 0, sizeof(nm_parsed_device_id));
  memset(nm_parsed_ssid, 0, sizeof(nm_parsed_ssid));
  memset(nm_parsed_pass, 0, sizeof(nm_parsed_pass));
  memset(nm_parsed_ntp, 0, sizeof(nm_parsed_ntp));
  memset(nm_parsed_report_url, 0, sizeof(nm_parsed_report_url));
  memset(nm_parsed_report_user, 0, sizeof(nm_parsed_report_user));
  memset(nm_parsed_report_pass, 0, sizeof(nm_parsed_report_pass));

// 执行安全剥离和流式解码
  has_device_id = cext_extract_form_value(nm_dynamic_html_buffer, "device_id", nm_parsed_device_id, sizeof(nm_parsed_device_id));
  has_ssid = cext_extract_form_value(nm_dynamic_html_buffer, "ssid", nm_parsed_ssid, sizeof(nm_parsed_ssid));
  has_pass = cext_extract_form_value(nm_dynamic_html_buffer, "pass", nm_parsed_pass, sizeof(nm_parsed_pass));
  has_ntp  = cext_extract_form_value(nm_dynamic_html_buffer, "ntp", nm_parsed_ntp, sizeof(nm_parsed_ntp));
  has_report_url = cext_extract_form_value(nm_dynamic_html_buffer, "rurl", nm_parsed_report_url, sizeof(nm_parsed_report_url));
  has_report_user = cext_extract_form_value(nm_dynamic_html_buffer, "ruser", nm_parsed_report_user, sizeof(nm_parsed_report_user));
  has_report_pass  = cext_extract_form_value(nm_dynamic_html_buffer, "rpass", nm_parsed_report_pass, sizeof(nm_parsed_report_pass));

  if (!has_device_id || nm_parsed_device_id[0] == 0) {
    // [异常: 关键的 Device ID 为空，可能是伪造或损坏的数据包]
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    snprintf(nm_dynamic_resp_buffer, sizeof(nm_dynamic_resp_buffer), nm_response_format_str, "错误：Device ID 不能为空！请返回重新选择。");
    nm_dynamic_resp_buffer[sizeof(nm_dynamic_resp_buffer) - 1] = 0;
    httpd_resp_send(req, nm_dynamic_resp_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }  

   if (!has_ssid || nm_parsed_ssid[0] == 0) {
    // [异常: 关键的 SSID 为空，可能是伪造或损坏的数据包]
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    snprintf(nm_dynamic_resp_buffer, sizeof(nm_dynamic_resp_buffer), nm_response_format_str, "错误：Wi-Fi 名称不能为空！请返回重新选择。");
    nm_dynamic_resp_buffer[sizeof(nm_dynamic_resp_buffer) - 1] = 0;
    httpd_resp_send(req, nm_dynamic_resp_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }  

  NEO_LOGI(TAG, "parse config -> DEVICE ID: [%s]", nm_parsed_device_id);
  NEO_LOGI(TAG, "parse config -> SSID: [%s], Pass: [%s], NTP: [%s]", nm_parsed_ssid, nm_parsed_pass, nm_parsed_ntp);
  NEO_LOGI(TAG, "parse config -> RURL: [%s], RUser: [%s], RPass: [%s]", nm_parsed_report_url, nm_parsed_report_user, nm_parsed_report_pass);
  

  httpd_resp_set_type(req, "text/html; charset=utf-8");
  snprintf(nm_dynamic_resp_buffer, sizeof(nm_dynamic_resp_buffer), nm_response_format_str, "配置成功！正在保存..");
  nm_dynamic_resp_buffer[sizeof(nm_dynamic_resp_buffer) - 1] = 0;  
  httpd_resp_send(req, nm_dynamic_resp_buffer, HTTPD_RESP_USE_STRLEN);

  delay_ms(2000);

  if(has_device_id && nm_parsed_device_id[0]) {
    strncpy(nm_device_id, nm_parsed_device_id, sizeof(nm_device_id));
    nm_device_id[sizeof(nm_device_id) - 1] = 0;
    val.valblob.body = (const uint8_t *)nm_device_id;
    val.valblob.len  = sizeof(nm_device_id) - 1;
    config_write("device_id", &val);
  }

  if(has_ssid && nm_parsed_ssid[0]) {
    strncpy(nm_wifi_ssid, nm_parsed_ssid, sizeof(nm_wifi_ssid));
    nm_wifi_ssid[sizeof(nm_wifi_ssid) - 1] = 0;
    val.valblob.body = (const uint8_t *)nm_wifi_ssid;
    val.valblob.len  = sizeof(nm_wifi_ssid) - 1;
    config_write("wifi_ssid", &val);
  }
  
  if(has_pass && nm_parsed_pass[0]) {
    strncpy(nm_wifi_pass, nm_parsed_pass, sizeof(nm_wifi_pass));
    nm_wifi_pass[sizeof(nm_wifi_pass) - 1] = 0;
    val.valblob.body = (const uint8_t *)nm_wifi_pass;
    val.valblob.len  = sizeof(nm_wifi_pass) - 1;
    config_write("wifi_pass", &val);
  }

  if(has_ntp && nm_parsed_ntp[0]) {
    strncpy(nm_ntp_server, nm_parsed_ntp, sizeof(nm_ntp_server));
    nm_ntp_server[sizeof(nm_ntp_server) - 1] = 0;    
    val.valblob.body = (const uint8_t *)nm_ntp_server;
    val.valblob.len  = sizeof(nm_ntp_server) - 1;
    config_write("ntp_server", &val);
  }

  if(has_report_url && nm_parsed_report_url[0]) {
    strncpy(nm_report_url, nm_parsed_report_url, sizeof(nm_report_url));
    nm_report_url[sizeof(nm_report_url) - 1] = 0;    
    val.valblob.body = (const uint8_t *)nm_report_url;
    val.valblob.len  = sizeof(nm_report_url) - 1;
    config_write("report_url", &val);
  }

  if(has_report_user && nm_parsed_report_user[0]) {
    strncpy(nm_report_user, nm_parsed_report_user, sizeof(nm_report_user));
    nm_report_user[sizeof(nm_report_user) - 1] = 0;    
    val.valblob.body = (const uint8_t *)nm_report_user;
    val.valblob.len  = sizeof(nm_report_user) - 1;
    config_write("report_user", &val);
  }  

  if(has_report_pass && nm_parsed_report_pass[0]) {
    strncpy(nm_report_pass, nm_parsed_report_pass, sizeof(nm_report_pass));
    nm_report_pass[sizeof(nm_report_pass) - 1] = 0;    
    val.valblob.body = (const uint8_t *)nm_report_pass;
    val.valblob.len  = sizeof(nm_report_pass) - 1;
    config_write("report_pass", &val);
  } 

  // 发消息表示配置结束
  task_set(EV_NM_CONFIG_END);

  return ESP_OK;
}

// 通配符拦截句柄
static esp_err_t nm_captive_portal_redirect_handler(httpd_req_t *req) 
{
  NEO_LOGI(TAG, "get probe request: %s", req->uri);
  return nm_index_get_handler(req);
}

// 启动本地网页服务器
static void nm_start_webserver(void) 
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 4;
  config.ctrl_port = 32768;
  config.uri_match_fn = httpd_uri_match_wildcard;

  if (httpd_start(&nm_httpd_handle, &config) == ESP_OK) {
    httpd_uri_t save_uri = {.uri = "/save",
                            .method = HTTP_POST,
                            .handler = nm_httpd_save_handler,
                            .user_ctx = NULL};
    httpd_register_uri_handler(nm_httpd_handle, &save_uri);

    httpd_uri_t index_uri = {.uri = "/index.html",
                             .method = HTTP_GET,
                             .handler = nm_index_get_handler,
                             .user_ctx = NULL};
    httpd_register_uri_handler(nm_httpd_handle, &index_uri);

    httpd_uri_t captive_uri = {.uri = "*",
                               .method = HTTP_GET,
                               .handler = nm_captive_portal_redirect_handler,
                               .user_ctx = NULL};
    httpd_register_uri_handler(nm_httpd_handle, &captive_uri);
    NEO_LOGI(TAG, "httpd start ok");
  }
}

// ==========================================
// 3. DNS 劫持任务（使用静态 PSRAM 缓冲区接收网络包）
// ==========================================
static void nm_dns_server_task(void *pvParameters) 
{
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  nm_dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (nm_dns_socket < 0) {
    NEO_LOGE(TAG, "can not create DNS socket");
  }

  if(nm_dns_socket >= 0) {
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(nm_dns_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      NEO_LOGE(TAG, "DNS socket bind failed");
      close(nm_dns_socket);
      nm_dns_socket = -1;
    }
  }

  NEO_LOGI(TAG, "dns hijack is running");

  while (1) {
    if(nm_dns_socket >= 0) {
    // 使用 PSRAM 中分配的静态全局缓冲区接收 UDP 网络数据
      int len = recvfrom(nm_dns_socket, nm_dns_rx_buffer, sizeof(nm_dns_rx_buffer) - 32, 0,
                        (struct sockaddr *)&client_addr, &client_addr_len);
      if (len > 12) {
        nm_dns_rx_buffer[2] |= 0x80;
        nm_dns_rx_buffer[3] |= 0x80;
        nm_dns_rx_buffer[7] = 1;

        nm_dns_rx_buffer[len++] = 0xc0;
        nm_dns_rx_buffer[len++] = 0x0c;
        nm_dns_rx_buffer[len++] = 0x00;
        nm_dns_rx_buffer[len++] = 0x01;
        nm_dns_rx_buffer[len++] = 0x00;
        nm_dns_rx_buffer[len++] = 0x01;
        nm_dns_rx_buffer[len++] = 0x00;
        nm_dns_rx_buffer[len++] = 0x00;
        nm_dns_rx_buffer[len++] = 0x00;
        nm_dns_rx_buffer[len++] = 0x3c;
        nm_dns_rx_buffer[len++] = 0x00;
        nm_dns_rx_buffer[len++] = 0x04;
        nm_dns_rx_buffer[len++] = 192;
        nm_dns_rx_buffer[len++] = 168;
        nm_dns_rx_buffer[len++] = 4;
        nm_dns_rx_buffer[len++] = 1;

        sendto(nm_dns_socket, nm_dns_rx_buffer, len, 0, (struct sockaddr *)&client_addr,
              client_addr_len);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}

// ==========================================
// 4. Wi-Fi AP + STA 混合模式初始化与扫描
// ==========================================
void nm_wifi_init_ap_sta(void) 
{
  uint16_t ap_num = MAX_SCANNED_AP_CNT;
  wifi_scan_config_t scan_config = {.show_hidden = false};
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err = ESP_OK;
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

  nm_ap_netif = esp_netif_create_default_wifi_ap();
  nm_sta_netif = esp_netif_create_default_wifi_sta();

  err = esp_wifi_init(&cfg);
  if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
    NEO_LOGE(TAG, "esp_wifi_init failed %d", err);
    return;
  }

  if (strlen(AP_PASS) == 0) {
    wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  NEO_LOGI(TAG, "wifi ap start ok, SSID:%s", AP_SSID);

  NEO_LOGI(TAG, "scan ssid...");

  ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, nm_ap_records));

  nm_scanned_ap_count = ap_num;
  if (nm_scanned_ap_count > MAX_SCANNED_AP_CNT) nm_scanned_ap_count = MAX_SCANNED_AP_CNT;
  for (int i = 0; i < nm_scanned_ap_count; i++) {
    strncpy(nm_scanned_ssids[i],
            (char *)nm_ap_records[i].ssid,
            sizeof(nm_scanned_ssids[i]));
    nm_scanned_ssids[i][sizeof(nm_scanned_ssids[i]) - 1] = '\0';
    NEO_LOGD(TAG, "found: %s", nm_scanned_ssids[i]);
  }
}

// ==========================================
// 5. 释放销毁函数
// ==========================================
void nm_stop_confg_portal(void) 
{
  NEO_LOGI(TAG, "stop confg portal...");

  // 1. 终止 DNS 任务
  if (nm_dns_task_handle != NULL) {
    vTaskDelete(nm_dns_task_handle);
    nm_dns_task_handle = NULL;
    if(nm_dns_socket >= 0) {
      close(nm_dns_socket);
      nm_dns_socket = -1;
    }
    NEO_LOGI(TAG, "dns hijack stopped");
  }

   // 2. 停止 HTTP 网页服务器
  ESP_ERROR_CHECK(httpd_stop(nm_httpd_handle));
  NEO_LOGI(TAG, "httpd stopped");

  // 3. 关闭 Wi-Fi 芯片发射
  ESP_ERROR_CHECK(esp_wifi_stop());
  NEO_LOGI(TAG, "wifi ap stopped");

  ESP_ERROR_CHECK(esp_wifi_deinit());

  if(nm_ap_netif) {
    esp_netif_destroy_default_wifi(nm_ap_netif);
    nm_ap_netif = NULL;
  }
  
  if(nm_sta_netif) {
    esp_netif_destroy_default_wifi(nm_sta_netif);
    nm_sta_netif = NULL;
  } 

  // 4. 释放计数器状态
  nm_scanned_ap_count = 0;

  nm_state = NM_STATE_DISCONNECTED;

  NEO_LOGI(TAG, "all done");
}

void nm_start_confg_portal(void) 
{
  nm_wifi_init_ap_sta();
  nm_start_webserver();
  xTaskCreate(nm_dns_server_task, "NM DNS", 4096, NULL, 5,
              &nm_dns_task_handle);
  nm_state = NM_STATE_CONFIG;
  task_set(EV_NM_CONFIG_BEGIN);
}

//
//-----------------------------------------------------------------------
// 

static void nm_time_sync_notification_cb(struct timeval *tv)
{
  time_t now;
  struct tm timeinfo;
  
  time(&now); // 获取系统硬件 RTC 的时间戳
  localtime_r(&now, &timeinfo); // 转换为年月日时分秒结构体

  // 打印显示（例如当前已被自动校准为时区正确的时间）
  NEO_LOGI(TAG, "local time: %04d-%02d-%02d %02d:%02d:%02d",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  now = tv->tv_sec;
  localtime_r(&now, &timeinfo);
  NEO_LOGI(TAG, "sync time: %04d-%02d-%02d %02d:%02d:%02d.%03ld",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, tv->tv_usec / 1000);
  
  // 发送时钟同步事件 
  task_set(EV_NM_TIME_SYNC);
}

static void nm_sta_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
  esp_netif_dns_info_t dns_info;

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    NEO_LOGI(TAG, "try to connecting wifi");
    esp_wifi_connect();

    nm_state = NM_STATE_CONNECTING;
    
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    NEO_LOGI(TAG, "wifi phy connected, waiting DHCP assign IP...");
    
    nm_state = NM_STATE_CONNECTED;
  
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    // 清除全局网络就绪标志，通知其他任务不要再发网络包了
    
    nm_state = NM_STATE_DISCONNECTED;

    wifi_event_sta_disconnected_t* disaster = (wifi_event_sta_disconnected_t*) event_data;
    NEO_LOGW(TAG, "wifi phy disconnected, error: %d", disaster->reason);

    // 关键防死锁：只有当不是用户主动断开时，才执行无限后台重连机制
    if (!nm_is_purposely_disconnected) {
      NEO_LOGI(TAG, "try to reconnect after 2 sec...");
      delay_ms(2000);
      esp_wifi_connect();
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    NEO_LOGI(TAG, "got assigned IP: " IPSTR, IP2STR(&event->ip_info.ip));
    NEO_LOGI(TAG, "             GW: " IPSTR, IP2STR(&event->ip_info.gw));
    NEO_LOGI(TAG, "           MASK: " IPSTR, IP2STR(&event->ip_info.netmask));

    if(nm_sta_netif) {
      if(esp_netif_get_dns_info(nm_sta_netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
        NEO_LOGI(TAG, "    DSN Main:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
      }
      if(esp_netif_get_dns_info(nm_sta_netif, ESP_NETIF_DNS_BACKUP, &dns_info) == ESP_OK) {
        NEO_LOGI(TAG, "  DSN Backup: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
      }
      if(esp_netif_get_dns_info(nm_sta_netif, ESP_NETIF_DNS_FALLBACK, &dns_info) == ESP_OK) {
        NEO_LOGI(TAG, "DSN Fallback: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
      }            
    }

    nm_state = NM_STATE_ONLINE;

    // 启动sntp服务
    if(!nm_is_sntp_started) {

      // 1. 设置操作模式为 POLL（轮询模式）
      esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

      // 2. 绑定 NTP 服务器，可以绑定多个作为备用
      if(nm_ntp_server[0]) {
        esp_sntp_setservername(0, nm_ntp_server);
        esp_sntp_setservername(1, "pool.ntp.org");
      } else {
        esp_sntp_setservername(0, "pool.ntp.org");
      }

      // 3. 注册对时成功的回调函数（用于监控后台轮询状态）
      esp_sntp_set_time_sync_notification_cb(nm_time_sync_notification_cb);

      // 4. 终极控制：如何修改轮询间隔（Interval）？
      // 默认是 1 小时（3600000 毫秒）。如果你需要高频校准或极度省电：
      // 修改下方的宏定义（注意：乐鑫为了防止压垮公开NTP服务器，限制此值最小不能低于 15 秒）
      //#ifdef CONFIG_LWIP_SNTP_UPDATE_DELAY
      // 如果你在 menuconfig 中开启了自定义，这里就会生效
      //#endif

      // 5. 启动后台轮询任务
      esp_sntp_init();

      nm_is_sntp_started = true;
    }

  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
    NEO_LOGE(TAG, "IP address losted, force reconnect...");
    nm_state = NM_STATE_OFFLINE;
    if(nm_is_sntp_started) {
      esp_sntp_stop(); 
      nm_is_sntp_started = false;
    }    
    esp_wifi_disconnect();
    nm_is_sntp_started = false;
  }
}

void nm_start_sta_daemon(void)
{
  nm_is_purposely_disconnected = false;
  nm_is_sntp_started = false;

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t wifi_config = {0};
  esp_err_t err = ESP_OK;

  NEO_LOGI(TAG, "nm_start_sta_daemon");

  nm_sta_netif = esp_netif_create_default_wifi_sta();

  err = esp_wifi_init(&cfg);
  if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
    NEO_LOGE(TAG, "esp_wifi_init failed %d", err);
    return;
  }

  // 注册监听器，整个软件生命周期内不调用 unregister
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &nm_sta_event_handler, NULL, &nm_sta_instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &nm_sta_event_handler, NULL, &nm_sta_instance_got_ip));

  NEO_LOGI(TAG, "ssid: %s", nm_wifi_ssid);
  NEO_LOGI(TAG, "pass: %s", nm_wifi_pass);;

  strncpy((char *)wifi_config.sta.ssid, nm_wifi_ssid, sizeof(wifi_config.sta.ssid));
  if (nm_wifi_pass[0]) {
    strncpy((char *)wifi_config.sta.password, nm_wifi_pass, sizeof(wifi_config.sta.password));
  }
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void nm_stop_sta_daemon()
{
  NEO_LOGI(TAG, "nm_stop_sta_daemon");

  nm_is_purposely_disconnected = true;

  if(nm_is_sntp_started) {
    esp_sntp_stop();
    nm_is_sntp_started = false;
  }

  ESP_ERROR_CHECK(esp_wifi_disconnect());
  ESP_ERROR_CHECK(esp_wifi_stop());

  if (nm_sta_instance_any_id != NULL) {
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, nm_sta_instance_any_id));
    nm_sta_instance_any_id = NULL;
  }

  if (nm_sta_instance_got_ip != NULL) {
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, nm_sta_instance_got_ip));
    nm_sta_instance_got_ip = NULL;
  }

  ESP_ERROR_CHECK(esp_wifi_deinit());

  if(nm_sta_netif) {
    esp_netif_destroy_default_wifi(nm_sta_netif);
    nm_sta_netif = NULL;
  }

  nm_state = NM_STATE_DISCONNECTED;
}

bool nm_get_info(
  esp_netif_ip_info_t * ip_info, 
  ip_addr_t * dns1, 
  ip_addr_t * dns2,
  ip_addr_t * dns3,  
  uint8_t mac[6])
{
  const ip_addr_t *dns_main = NULL;
  const ip_addr_t *dns_backup = NULL;
  const ip_addr_t *dns_fallback = NULL;  
  if(nm_state == NM_STATE_ONLINE) {
    if(nm_sta_netif) {
      if(esp_netif_get_ip_info(nm_sta_netif, ip_info) == ESP_OK) {
        esp_read_mac(mac, ESP_MAC_WIFI_STA);

        memset(dns1, 0, sizeof(esp_ip4_addr_t));
        memset(dns2, 0, sizeof(esp_ip4_addr_t));   
        memset(dns3, 0, sizeof(esp_ip4_addr_t));     
        dns_main = dns_getserver(0);
        dns_backup = dns_getserver(1);
        dns_fallback = dns_getserver(2);

        if(dns_main) {
          memcpy(dns1, dns_main, sizeof(esp_ip4_addr_t)); 
        }
        if(dns_backup) {
          memcpy(dns2, dns_backup, sizeof(esp_ip4_addr_t)); 
        }
        if(dns_fallback) {
          memcpy(dns3, dns_fallback, sizeof(esp_ip4_addr_t)); 
        }
        return true;
      }
    }
  }
  return false;
}

nm_state_t nm_get_state(void)
{
  return nm_state;
}

const char * nm_get_ssid(void)
{
  return nm_wifi_ssid;
}

const char * nm_get_config_ssid(void)
{
  return AP_SSID;
}

const char * nm_get_device_id(void)
{
  return nm_device_id;
}

bool nm_sent_data(const char * json)
{
  // 2. 配置 HTTP 客户端参数
  esp_http_client_config_t config = {
        .url = nm_report_url,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = true,
        .auth_type = HTTP_AUTH_TYPE_BASIC,  
        .username = nm_report_user,  
        .password = nm_report_pass, 
        .timeout_ms = NM_HTTP_CLIENT_TIMEO_MS,
  };
  esp_err_t err;
  bool ret = false;
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if(client == NULL) {
    NEO_LOGW(TAG, "esp_http_client_init faild");
    goto err;
  }

    // 3. 设置 HTTP 请求头，声明发送的是 JSON 格式
    if((err = esp_http_client_set_header(client, "Content-Type", "application/json")) != ESP_OK) {
      NEO_LOGW(TAG, "esp_http_client_set_header faild %s", esp_err_to_name(err));
      goto err;
    }
    
    // 4. 填入要发送的 JSON 数据和长度
    if((err = esp_http_client_set_post_field(client, json, strlen(json))) != ESP_OK) {
      NEO_LOGW(TAG, "esp_http_client_set_post_field faild %s", esp_err_to_name(err));
      goto err;
    }

    // 5. 执行 HTTP 请求 
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        NEO_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        ret = true;
    } else {
        NEO_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        goto err;
    }

err:
    if(client != NULL)
      esp_http_client_cleanup(client);
    return ret;
}