#include "pms5003st.h"
#include "logger.h"
#include "usart_wrapper.h"
#include "gpio_wrapper.h"
#include "delay.h"

#include <string.h>

#define PMS_5003_ST_SIG1 0x42
#define PMS_5003_ST_SIG2 0x4d

#define PMS_5003_ST_COMBINE(hi, lo) ((uint16_t)hi << 8 | lo)
#define PMS_5003_ST_COMBINE_SIGNED(hi, lo) ((int16_t)((uint16_t)(hi) << 8 | (lo)))

#define PMS5003_MAX_WAIT_CNT 10

static const char * TAG = "PMS5003ST";

static usart_wrapper_dev_handle_t usart_dev_handle;

static void pms5003st_dump_cmd(const pms5003st_cmd_msg_t * cmd)
{
    NEO_LOGD(TAG, "pms5003st_cmd:");
    NEO_LOGD(TAG, "  signature_h = %02x", cmd->signature_h);
    NEO_LOGD(TAG, "  signature_l = %02x", cmd->signature_l); 
    NEO_LOGD(TAG, "  cmd = %02x", cmd->cmd);
    NEO_LOGD(TAG, "  datah = %02x", cmd->datah); 
    NEO_LOGD(TAG, "  datal = %02x", cmd->datal);    
    NEO_LOGD(TAG, "  checksumh = %02x", cmd->checksumh); 
    NEO_LOGD(TAG, "  checksuml = %02x", cmd->checksuml);      
} 

static void pms5003st_dump_res(const pms5003st_res_msg_t *res)
{
    NEO_LOGD(TAG, "pms5003st_res:");
    NEO_LOGD(TAG, "  signature_h = %02x", res->signature_h);
    NEO_LOGD(TAG, "  signature_l = %02x", res->signature_l);
    NEO_LOGD(TAG, "  lenh = %02x", res->lenh);
    NEO_LOGD(TAG, "  lenl = %02x", res->lenl);
    NEO_LOGD(TAG, "  pm10h = %02x", res->pm10h);
    NEO_LOGD(TAG, "  pm10l = %02x", res->pm10l);
    NEO_LOGD(TAG, "  pm25h = %02x", res->pm25h);
    NEO_LOGD(TAG, "  pm25l = %02x", res->pm25l);
    NEO_LOGD(TAG, "  pm100h = %02x", res->pm100h);
    NEO_LOGD(TAG, "  pm100l = %02x", res->pm100l);
    NEO_LOGD(TAG, "  pm10ah = %02x", res->pm10ah);
    NEO_LOGD(TAG, "  pm10al = %02x", res->pm10al);
    NEO_LOGD(TAG, "  pm25ah = %02x", res->pm25ah);
    NEO_LOGD(TAG, "  pm25al = %02x", res->pm25al);
    NEO_LOGD(TAG, "  pm100ah = %02x", res->pm100ah);
    NEO_LOGD(TAG, "  pm100al = %02x", res->pm100al);
    NEO_LOGD(TAG, "  pm03cnth = %02x", res->pm03cnth);
    NEO_LOGD(TAG, "  pm03cntl = %02x", res->pm03cntl);
    NEO_LOGD(TAG, "  pm05cnth = %02x", res->pm05cnth);
    NEO_LOGD(TAG, "  pm05cntl = %02x", res->pm05cntl);
    NEO_LOGD(TAG, "  pm10cnth = %02x", res->pm10cnth);
    NEO_LOGD(TAG, "  pm10cntl = %02x", res->pm10cntl);
    NEO_LOGD(TAG, "  pm25cnth = %02x", res->pm25cnth);
    NEO_LOGD(TAG, "  pm25cntl = %02x", res->pm25cntl);
    NEO_LOGD(TAG, "  pm50cnth = %02x", res->pm50cnth);
    NEO_LOGD(TAG, "  pm50cntl = %02x", res->pm50cntl);
    NEO_LOGD(TAG, "  pm100cnth = %02x", res->pm100cnth);
    NEO_LOGD(TAG, "  pm100cntl = %02x", res->pm100cntl);
    NEO_LOGD(TAG, "  formh = %02x", res->formh);
    NEO_LOGD(TAG, "  forml = %02x", res->forml);
    NEO_LOGD(TAG, "  temph = %02x", res->temph);
    NEO_LOGD(TAG, "  templ = %02x", res->templ);
    NEO_LOGD(TAG, "  molh = %02x", res->molh);
    NEO_LOGD(TAG, "  moll = %02x", res->moll);
    NEO_LOGD(TAG, "  resh = %02x", res->resh);
    NEO_LOGD(TAG, "  resl = %02x", res->resl);
    NEO_LOGD(TAG, "  ver = %02x", res->ver);
    NEO_LOGD(TAG, "  err = %02x", res->err);
    NEO_LOGD(TAG, "  checksumh = %02x", res->checksumh);
    NEO_LOGD(TAG, "  checksuml = %02x", res->checksuml);
}

static void pms5003st_dump_data(const pms5003st_data_t *data)
{
    NEO_LOGD(TAG, "pms5003st_data:");
    NEO_LOGD(TAG, "  pm_10 = %u", data->pm_10);
    NEO_LOGD(TAG, "  pm_25 = %u", data->pm_25);
    NEO_LOGD(TAG, "  pm_100 = %u", data->pm_100);
    NEO_LOGD(TAG, "  pm_10a = %u", data->pm_10a);
    NEO_LOGD(TAG, "  pm_25a = %u", data->pm_25a);
    NEO_LOGD(TAG, "  pm_100a = %u", data->pm_100a);
    NEO_LOGD(TAG, "  pm_03cnt = %u", data->pm_03cnt);
    NEO_LOGD(TAG, "  pm_05cnt = %u", data->pm_05cnt);
    NEO_LOGD(TAG, "  pm_10cnt = %u", data->pm_10cnt);
    NEO_LOGD(TAG, "  pm_25cnt = %u", data->pm_25cnt);
    NEO_LOGD(TAG, "  pm_50cnt = %u", data->pm_50cnt);
    NEO_LOGD(TAG, "  pm_100cnt = %u", data->pm_100cnt);
    NEO_LOGD(TAG, "  form = %.4f", data->form);
    NEO_LOGD(TAG, "  temp = %.4f", data->temp);
    NEO_LOGD(TAG, "  mol = %.4f", data->mol);
}

static bool pms5003st_verify_res(pms5003st_res_msg_t * res)
{
    uint8_t chkcum = 0, *p = (uint8_t * )res, i;
    if(res->signature_h != PMS_5003_ST_SIG1 || res->signature_l != PMS_5003_ST_SIG2) {
        NEO_LOGW(TAG, "signalture error %02x %02x", res->signature_h, res->signature_l);
        return false;
    }

    for(i = 0; i < 38; i ++) {
        chkcum += p[i];
    }

    if(chkcum != res->checksuml) {
        NEO_LOGW(TAG, "chkcum error %02x %02x", res->checksuml, chkcum);
        return false;
    }

    if( PMS_5003_ST_COMBINE(res->lenh, res->lenl) != 2*17+2) {
        NEO_LOGW(TAG, "legnth error %d %d", PMS_5003_ST_COMBINE(res->lenh, res->lenl), 2*17+2);
        return false;
    }

    return true;
}

static void pms5003st_fill_cmd(pms5003st_cmd_msg_t * cmd)
{
    uint8_t *p = (uint8_t * )cmd, i;
    cmd->signature_h = PMS_5003_ST_SIG1;
    cmd->signature_l = PMS_5003_ST_SIG2;
    cmd->checksumh = 0;
    cmd->checksuml = 0;
    for(i = 0 ; i < 5 ; i++) {
        cmd->checksuml += p[i];
    }
}

static void pms5003st_send_cmd(pms5003st_cmd_msg_t * cmd)
{
    NEO_LOGD(TAG, "pms5003st_send_cmd");
    pms5003st_fill_cmd(cmd);
    // pms5003st_dump_cmd((const pms5003st_cmd_msg_t *)cmd);
    usart_wrapper_write(&usart_dev_handle, (const uint8_t *)cmd, sizeof(pms5003st_cmd_msg_t));
}

static bool pms5003st_read_res(pms5003st_res_msg_t * res)
{
    ssize_t size;

    NEO_LOGD(TAG, "pms5003st_read_res");

    memset(res, 0, sizeof(pms5003st_res_msg_t));

    if((size = usart_wrapper_read(&usart_dev_handle,  (uint8_t *)res, sizeof(pms5003st_res_msg_t))) 
        == sizeof(pms5003st_res_msg_t)) {
        if(!pms5003st_verify_res(res)) {
            pms5003st_dump_res(res);
            return false;
        }
    } else {
        NEO_LOGW(TAG, "pms5003st_read_res failed, size = %d", size);
        return false;
    }

    // pms5003st_dump_res(res);
    return true;

}

static void pms5003st_covert_data(const pms5003st_res_msg_t * res, pms5003st_data_t * data)
{
    data->pm_10     = PMS_5003_ST_COMBINE(res->pm10h, res->pm10l);
    data->pm_25     = PMS_5003_ST_COMBINE(res->pm25h, res->pm25l);
    data->pm_100    = PMS_5003_ST_COMBINE(res->pm100h, res->pm100l);
    
    data->pm_10a    = PMS_5003_ST_COMBINE(res->pm10ah, res->pm10al);
    data->pm_25a    = PMS_5003_ST_COMBINE(res->pm25ah, res->pm25al);
    data->pm_100a   = PMS_5003_ST_COMBINE(res->pm100ah, res->pm100al);

    data->pm_03cnt  = PMS_5003_ST_COMBINE(res->pm03cnth, res->pm03cntl);
    data->pm_05cnt  = PMS_5003_ST_COMBINE(res->pm05cnth, res->pm05cntl);
    data->pm_10cnt  = PMS_5003_ST_COMBINE(res->pm10cnth, res->pm10cntl);
    data->pm_25cnt  = PMS_5003_ST_COMBINE(res->pm25cnth, res->pm25cntl);
    data->pm_50cnt  = PMS_5003_ST_COMBINE(res->pm50cnth, res->pm50cntl);
    data->pm_100cnt = PMS_5003_ST_COMBINE(res->pm100cnth, res->pm100cntl);

    // 2. 转换浮点数值（甲醛缩小 1000 倍，温湿度缩小 10 倍）
    data->form      = (float)PMS_5003_ST_COMBINE(res->formh, res->forml) / 1000.0f;
    data->temp      = (float)PMS_5003_ST_COMBINE_SIGNED(res->temph, res->templ) / 10.0f;
    data->mol       = (float)PMS_5003_ST_COMBINE(res->molh, res->moll) / 10.0f;
}

void pms5003st_init(void)
{
    uint8_t wait_cnt = 0;
    NEO_LOGI(TAG, "pms5003st_init");

    pms5003st_cmd_msg_t cmd = {};
    pms5003st_res_msg_t res = {};

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    usart_wrapper_dev_add(&usart_dev_handle, UART_NUM_2, 
        PMS5003ST_TX_GPIO_PIN, PMS5003ST_RX_GPIO_PIN, &uart_config);

    // 唤醒
    gpio_wrapper_set_level(PM5003ST_SET_GPIO_PIN,   1);

    // 复位
    gpio_wrapper_set_level(PM5003ST_RESET_GPIO_PIN, 1);  
    delay_ms(100);
    gpio_wrapper_set_level(PM5003ST_RESET_GPIO_PIN, 0); 
    delay_ms(100);
    gpio_wrapper_set_level(PM5003ST_RESET_GPIO_PIN, 1); 

    // 设置为被动模式
    cmd.cmd = PMS5003ST_CMD_SET_MODE;
    cmd.datah = 0;
    cmd.datal = 0;    
    pms5003st_send_cmd(&cmd);
    // 清空接收队列
    while(pms5003st_read_res(&res)) {
        NEO_LOGW(TAG, "clear res msg in %d", wait_cnt);
        delay_ms(100);
        wait_cnt ++;
        if(wait_cnt >= PMS5003_MAX_WAIT_CNT) {
            NEO_LOGW(TAG, "clear res msg reach max %d", PMS5003_MAX_WAIT_CNT);
            break;
        }
            
    }
}

void pms5003st_sleep(bool sleep)
{
    pms5003st_cmd_msg_t cmd = {};
    pms5003st_res_msg_t res = {};
    NEO_LOGW(TAG, "pms5003st_sleep %s", sleep ? "ON" : "OFF");

    if(!sleep)
        gpio_wrapper_set_level(PM5003ST_SET_GPIO_PIN, 1);

    cmd.cmd = PMS5003ST_CMD_STANDBY;
    cmd.datah = 0;
    cmd.datal = sleep ? 0 : 1;    
    pms5003st_send_cmd(&cmd);
    pms5003st_read_res(&res);

    if(sleep)
        gpio_wrapper_set_level(PM5003ST_SET_GPIO_PIN, 0);
}

bool pms5003st_read_data(pms5003st_data_t * data)
{
    pms5003st_cmd_msg_t cmd = {};
    pms5003st_res_msg_t res = {};
    cmd.cmd = PMS5003ST_CMD_READ_DATA;
    cmd.datah = 0;
    cmd.datal = 0;    
    pms5003st_send_cmd(&cmd);
    if(pms5003st_read_res(&res)) {
        pms5003st_covert_data(&res, data);
        pms5003st_dump_data(data);
        return true;
    }
    return false;
}
