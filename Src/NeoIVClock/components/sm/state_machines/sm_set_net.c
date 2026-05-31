#include "sm_set_net.h"
#include "mini_font.h"
#include "task.h"
#include "sm.h"
#include "logger.h"
#include "nm.h"
#include "clock.h"
#include "iv18.h"
#include "nm.h"
#include "oled.h"
#include "oled_ext.h"
#include "qrcode_wrapper.h"

#include "sm_func_select.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/dns.h" 
#include "esp_netif_ip_addr.h"



static const char * TAG = "SM_SET_NET";

const char * sm_states_names_set_net[] = {
  "SM_SET_NET_INIT",
  "SM_SET_NET_SEL", // 显示菜单，配网或者是信息
  "SM_SET_NET_INFO",   // 展示信息，ip地址，mac地址，联网时间
  "SM_SET_NET_QRCODE",  // 展示QCode，扫描后打开配置页面  
};

static uint8_t sm_set_net_sel_index;
static void sm_set_net_draw_sel(uint8_t index)
{
  wchar_t buf[16] = {};
  // 画滚动菜单
  oled_clear();

  oled_fill_rect(0,  index * 16 , 128, 16, true);

  swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%ls", L"退出");
  buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
  oled_ext_draw_wstring(0, 0, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  
  swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%ls", L"网络信息");
  buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
  oled_ext_draw_wstring(0, 16, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);

  swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%ls", L"配置网络");
  buf[sizeof(buf)/sizeof(wchar_t) - 1] = 0;
  oled_ext_draw_wstring(0, 32, buf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);

  oled_redraw_buffer(); 
}

static void sm_set_net_draw_info(void)
{
  char buf[64] = {};
  wchar_t wbuf[16] = {};
  esp_netif_ip_info_t ip_info;
  ip_addr_t dns_main, dns_backup, dns_fallback;
  uint8_t mac[6];
  bool info_ok = false;


  oled_clear();
  if(nm_get_state() == NM_STATE_ONLINE) {
    if(nm_get_info(&ip_info, &dns_main, &dns_backup, &dns_fallback, mac)) {

      snprintf(buf, sizeof(buf), "ssid: %s" ,nm_get_ssid());
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 0, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);

      snprintf(buf, sizeof(buf), "ip : " IPSTR, IP2STR(&ip_info.ip));
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 8, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);

      snprintf(buf, sizeof(buf), "msk: " IPSTR, IP2STR(&ip_info.netmask));
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 16, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);

      snprintf(buf, sizeof(buf), "gw : " IPSTR, IP2STR(&ip_info.gw));
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 24, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR); 

      snprintf(buf, sizeof(buf), "dns1: %s", ip_addr_isany(&dns_main)? "n/a": ipaddr_ntoa(&dns_main));
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 32, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);  
 
      snprintf(buf, sizeof(buf), "dns2: %s", ip_addr_isany(&dns_backup)? "n/a": ipaddr_ntoa(&dns_backup));
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 40, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);  
      
      snprintf(buf, sizeof(buf), "dns3: %s", ip_addr_isany(&dns_fallback)? "n/a": ipaddr_ntoa(&dns_fallback));
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 48, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);
      
      snprintf(buf, sizeof(buf), "mac:%02X:%02X:%02X:%02X:%02X:%02X", 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      buf[sizeof(buf) - 1] = 0;
      oled_ext_draw_string(0, 56, buf, MINI_FONT_TYPE_ASCII_6X8, OLED_DRAW_XOR);

      info_ok = true;  
    }
  }
  if(!info_ok) {
    swprintf(wbuf, sizeof(wbuf)/sizeof(wchar_t), L"%ls", L"网络未配置/连接");
    buf[sizeof(wbuf)/sizeof(wchar_t) - 1] = 0;
    oled_ext_draw_wstring(0, 32, wbuf, MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_XOR);
  }

  oled_redraw_buffer(); 
}

static void sm_set_net_draw_qcode(bool is_begin)
{
  char buf[64];
  oled_clear();
  if(is_begin) {
    oled_ext_draw_wstring(0, 32, L"正在启动配网服务", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
  } else {
    oled_draw_bitmap(0, 0, 33, 33, qrcode_wrapper_draw_txt("http://192.168.4.1"), OLED_DRAW_XOR);
    snprintf(buf, sizeof(buf), "%s", nm_get_config_ssid());
    buf[sizeof(buf) - 1] = 0;
    
    oled_ext_draw_wstring(33, 0, L" 连接SSID后", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
    oled_ext_draw_string(0, 33, buf, MINI_FONT_TYPE_ASCII_8X16, OLED_DRAW_OVERWRITE);
    oled_ext_draw_wstring(33, 16, L" 扫描二维码", MINI_FONT_TYPE_ASCII_8X16, MINI_FONT_TYPE_CHINESE_16X16, OLED_DRAW_OVERWRITE);
  }
  oled_redraw_buffer(); 
}

void do_set_net_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  NEO_LOGD(TAG, "do_set_alarm_init");

  // 将IV18设置为时钟状态
  iv18_reset_ps_timeo();
  clock_set_display_mode(CLOCK_DISPLAY_MODE_TIME);
  sm_set_net_sel_index   = 1;
}

static void do_set_net_sel(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_EC11_UP || ev == EV_V1 || ev == EV_NM_CONFIG_END || ev == EV_EC11_C || ev == EV_EC11_FAST_C || ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
    if(ev == EV_EC11_C || ev == EV_EC11_FAST_C) {
      sm_set_net_sel_index = (sm_set_net_sel_index + 1) % 3;
    } else if(ev == EV_EC11_CC || ev == EV_EC11_FAST_CC) {
      sm_set_net_sel_index = (sm_set_net_sel_index + 2) % 3;
    }
    sm_set_net_draw_sel(sm_set_net_sel_index);
  } else if (ev == EV_EC11_PRESS) {
    switch(sm_set_net_sel_index) {
      case 0: task_set(EV_V1); break; // quit
      case 1: task_set(EV_V2); break; // info
      case 2: task_set(EV_V3); break; // qcode
    }
  }
}

static void do_set_net_info(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V2) {
    sm_set_net_draw_info();
  } else if (ev == EV_EC11_PRESS) {
    task_set(EV_V1);
  }
}

static void do_set_net_qrcode(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev)
{
  if(ev == EV_V3) {
    sm_set_net_draw_qcode(true);
    nm_stop_sta_daemon();
    nm_start_confg_portal();
  } else if (ev == EV_EC11_PRESS ||  ev == EV_NM_CONFIG_END) {
    nm_stop_confg_portal();
    nm_start_sta_daemon();
    task_set(EV_V1);
  } if(ev == EV_NM_CONFIG_BEGIN) {
    sm_set_net_draw_qcode(false);
  }
}

static const sm_trans_t sm_trans_set_net_init[] = {
  {EV_EC11_UP, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_net_sel[] = {
  {EV_EC11_C, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},
  {EV_EC11_FAST_C, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},
  {EV_EC11_CC, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},
  {EV_EC11_FAST_CC, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},
  {EV_EC11_PRESS, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},
  {EV_V1, SM_FUNC_SELECT, SM_FUNC_SELECT_INIT, do_func_select_init},  
  {EV_V2, SM_SET_NET, SM_SET_NET_INFO, do_set_net_info},
  {EV_V3, SM_SET_NET, SM_SET_NET_QRCODE, do_set_net_qrcode},    
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_net_info[] = {
  {EV_EC11_PRESS, SM_SET_NET, SM_SET_NET_INFO, do_set_net_info},
  {EV_V1, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel},  
  {0, 0, 0, NULL}
};

static const sm_trans_t sm_trans_set_net_qrcode[] = {
  {EV_EC11_PRESS, SM_SET_NET, SM_SET_NET_QRCODE, do_set_net_qrcode},
  {EV_NM_CONFIG_BEGIN, SM_SET_NET, SM_SET_NET_QRCODE, do_set_net_qrcode},
  {EV_NM_CONFIG_END, SM_SET_NET, SM_SET_NET_QRCODE, do_set_net_qrcode},
  {EV_V1, SM_SET_NET, SM_SET_NET_SEL, do_set_net_sel}, 
  {0, 0, 0, NULL}
};

const sm_trans_t * sm_trans_set_net[] = {
  sm_trans_set_net_init,
  sm_trans_set_net_sel,
  sm_trans_set_net_info,
  sm_trans_set_net_qrcode,
};
