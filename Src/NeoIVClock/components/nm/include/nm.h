#ifndef NEO_IV_CLOCK_NM_H
#define NEO_IV_CLOCK_NM_H

#include <stdint.h>
#include <stdbool.h>

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


#endif //NEO_IV_CLOCK_NM_H