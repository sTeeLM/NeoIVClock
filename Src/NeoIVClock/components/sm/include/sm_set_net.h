#ifndef NEO_IV_CLOCK_SM_SET_NET_H
#define NEO_IV_CLOCK_SM_SET_NET_H

#include "task.h"
#include "sm.h"

extern const char * sm_states_names_set_net[];
extern const sm_trans_t * sm_trans_set_net[];

enum sm_states_set_net
{
  SM_SET_NET_INIT, 
  SM_SET_NET_SEL, // 显示菜单，配网或者是信息
  SM_SET_NET_INFO,   // 展示信息，ip地址，mac地址，联网时间
  SM_SET_NET_QRCODE,  // 展示QRCode，扫描后打开配置页面
};

void do_set_net_init(uint8_t from_func, uint8_t from_state, uint8_t to_func, uint8_t to_state, task_event_t ev);

#endif // NEO_IV_CLOCK_SM_SET_NET_H