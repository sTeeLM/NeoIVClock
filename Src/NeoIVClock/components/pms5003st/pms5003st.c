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

static bool pms5003st_enabled;

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
    uint16_t len = res->lenh * 256 + res->lenl;
    if(len == 0x24) {
        NEO_LOGD(TAG, "pms5003st_res: (DATA)");
    } else if(len == 0x4) {
        NEO_LOGD(TAG, "pms5003st_res: (CMD RES)");
    } else {
        NEO_LOGW(TAG, "pms5003st_res: (unknown frame %d bytes)", len);
        NEO_LOGW_HEX(TAG, res, len + 4);
        return; 
    }

    NEO_LOGD(TAG, "  signature_h = %02x", res->signature_h);
    NEO_LOGD(TAG, "  signature_l = %02x", res->signature_l);
    NEO_LOGD(TAG, "  lenh = %02x", res->lenh);
    NEO_LOGD(TAG, "  lenl = %02x", res->lenl);
    if(len == 0x24) {
        NEO_LOGD(TAG, "  pm10h = %02x", res->body_data.pm10h);
        NEO_LOGD(TAG, "  pm10l = %02x", res->body_data.pm10l);
        NEO_LOGD(TAG, "  pm25h = %02x", res->body_data.pm25h);
        NEO_LOGD(TAG, "  pm25l = %02x", res->body_data.pm25l);
        NEO_LOGD(TAG, "  pm100h = %02x", res->body_data.pm100h);
        NEO_LOGD(TAG, "  pm100l = %02x", res->body_data.pm100l);
        NEO_LOGD(TAG, "  pm10ah = %02x", res->body_data.pm10ah);
        NEO_LOGD(TAG, "  pm10al = %02x", res->body_data.pm10al);
        NEO_LOGD(TAG, "  pm25ah = %02x", res->body_data.pm25ah);
        NEO_LOGD(TAG, "  pm25al = %02x", res->body_data.pm25al);
        NEO_LOGD(TAG, "  pm100ah = %02x", res->body_data.pm100ah);
        NEO_LOGD(TAG, "  pm100al = %02x", res->body_data.pm100al);
        NEO_LOGD(TAG, "  pm03cnth = %02x", res->body_data.pm03cnth);
        NEO_LOGD(TAG, "  pm03cntl = %02x", res->body_data.pm03cntl);
        NEO_LOGD(TAG, "  pm05cnth = %02x", res->body_data.pm05cnth);
        NEO_LOGD(TAG, "  pm05cntl = %02x", res->body_data.pm05cntl);
        NEO_LOGD(TAG, "  pm10cnth = %02x", res->body_data.pm10cnth);
        NEO_LOGD(TAG, "  pm10cntl = %02x", res->body_data.pm10cntl);
        NEO_LOGD(TAG, "  pm25cnth = %02x", res->body_data.pm25cnth);
        NEO_LOGD(TAG, "  pm25cntl = %02x", res->body_data.pm25cntl);
        NEO_LOGD(TAG, "  pm50cnth = %02x", res->body_data.pm50cnth);
        NEO_LOGD(TAG, "  pm50cntl = %02x", res->body_data.pm50cntl);
        NEO_LOGD(TAG, "  pm100cnth = %02x", res->body_data.pm100cnth);
        NEO_LOGD(TAG, "  pm100cntl = %02x", res->body_data.pm100cntl);
        NEO_LOGD(TAG, "  formh = %02x", res->body_data.formh);
        NEO_LOGD(TAG, "  forml = %02x", res->body_data.forml);
        NEO_LOGD(TAG, "  temph = %02x", res->body_data.temph);
        NEO_LOGD(TAG, "  templ = %02x", res->body_data.templ);
        NEO_LOGD(TAG, "  molh = %02x", res->body_data.molh);
        NEO_LOGD(TAG, "  moll = %02x", res->body_data.moll);
        NEO_LOGD(TAG, "  resh = %02x", res->body_data.resh);
        NEO_LOGD(TAG, "  resl = %02x", res->body_data.resl);
        NEO_LOGD(TAG, "  ver = %02x", res->body_data.ver);
        NEO_LOGD(TAG, "  err = %02x", res->body_data.err);
        NEO_LOGD(TAG, "  checksumh = %02x", res->body_data.checksumh);
        NEO_LOGD(TAG, "  checksuml = %02x", res->body_data.checksuml);    
    } else if(len == 4) {
        NEO_LOGD(TAG, "  cmd = %02x", res->body_cmd.cmd);
        NEO_LOGD(TAG, "  err = %02x", res->body_cmd.err);
        NEO_LOGD(TAG, "  checksumh = %02x", res->body_cmd.checksumh);
        NEO_LOGD(TAG, "  checksuml = %02x", res->body_cmd.checksuml);          
    }

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

static void pms5003st_fill_cmd(pms5003st_cmd_msg_t * cmd)
{
    uint8_t *p = (uint8_t * )cmd, i;
    uint16_t sum = 0;
    cmd->signature_h = PMS_5003_ST_SIG1;
    cmd->signature_l = PMS_5003_ST_SIG2;
    cmd->checksumh = 0;
    cmd->checksuml = 0;
    for(i = 0 ; i < 5 ; i++) {
        sum += p[i];
    }
    cmd->checksuml = sum & 0xff;
    cmd->checksumh = (sum >> 8) & 0xff;
}

static void pms5003st_send_cmd(pms5003st_cmd_msg_t * cmd)
{
    NEO_LOGD(TAG, "pms5003st_send_cmd");
    pms5003st_fill_cmd(cmd);
    pms5003st_dump_cmd((const pms5003st_cmd_msg_t *)cmd);
    usart_wrapper_write(&usart_dev_handle, (const uint8_t *)cmd, sizeof(pms5003st_cmd_msg_t));
}

static int32_t pms5003st_parse_length(const uint8_t * length_buffer, uint8_t length)
{
    return length_buffer[0] * 256 + length_buffer[1];
}

static bool pms5003st_verify_chksum(const void * buffer, uint32_t buffer_length)
{
    pms5003st_res_msg_t * msg = (pms5003st_res_msg_t *)buffer;
    uint16_t chksum = 0;
    uint16_t length = msg->lenh * 256 + msg->lenl, i;
    const uint8_t *p = buffer;
    if(length + 4 > buffer_length)
        return false;

    for(i = 0 ; i < length + 2; i ++) {
        chksum += *p++;
    }
    if(length == 36) {
        if((chksum & 0xff) != msg->body_data.checksuml || ((chksum >>8) & 0xff) != msg->body_data.checksumh) {
            NEO_LOGW(TAG, "chkcum error [%02x%02x] %04x", msg->body_data.checksumh, msg->body_data.checksuml, chksum);
            return false;
        }
    } else if(length == 4) {
        if((chksum & 0xff) != msg->body_cmd.checksuml || ((chksum >>8) & 0xff) != msg->body_cmd.checksumh) {
            NEO_LOGW(TAG, "chkcum error [%02x%02x] %04x", msg->body_cmd.checksumh, msg->body_cmd.checksuml, chksum);
            return false;
        }
    } else {
        NEO_LOGW(TAG, "pms5003spms5003st_verify_chksumt_res: (unknown frame %d bytes)", buffer_length);
        NEO_LOGW_HEX(TAG, buffer, buffer_length);
        return false;
    }

    return true;
}
static ssize_t pms5003st_read_res(pms5003st_res_msg_t *res) 
{
    ssize_t ret;
    const uint8_t sig[2] = {0x42, 0x4D}; 
    NEO_LOGD(TAG, "pms5003st_read_res");
    ret = usart_wrapper_read_frame(&usart_dev_handle, 
        res, 
        sizeof(pms5003st_res_msg_t), 
        sig, 
        2,   // sig length
        2,   // length offset 
        2,   // length size
        pms5003st_parse_length,
        pms5003st_verify_chksum
    );
    if(ret > 0)
        pms5003st_dump_res(res);
    return ret;
}

static void pms5003st_covert_data(const pms5003st_res_msg_t * res, pms5003st_data_t * data)
{
    data->pm_10     = PMS_5003_ST_COMBINE(res->body_data.pm10h, res->body_data.pm10l);
    data->pm_25     = PMS_5003_ST_COMBINE(res->body_data.pm25h, res->body_data.pm25l);
    data->pm_100    = PMS_5003_ST_COMBINE(res->body_data.pm100h, res->body_data.pm100l);
    
    data->pm_10a    = PMS_5003_ST_COMBINE(res->body_data.pm10ah, res->body_data.pm10al);
    data->pm_25a    = PMS_5003_ST_COMBINE(res->body_data.pm25ah, res->body_data.pm25al);
    data->pm_100a   = PMS_5003_ST_COMBINE(res->body_data.pm100ah, res->body_data.pm100al);

    data->pm_03cnt  = PMS_5003_ST_COMBINE(res->body_data.pm03cnth, res->body_data.pm03cntl);
    data->pm_05cnt  = PMS_5003_ST_COMBINE(res->body_data.pm05cnth, res->body_data.pm05cntl);
    data->pm_10cnt  = PMS_5003_ST_COMBINE(res->body_data.pm10cnth, res->body_data.pm10cntl);
    data->pm_25cnt  = PMS_5003_ST_COMBINE(res->body_data.pm25cnth, res->body_data.pm25cntl);
    data->pm_50cnt  = PMS_5003_ST_COMBINE(res->body_data.pm50cnth, res->body_data.pm50cntl);
    data->pm_100cnt = PMS_5003_ST_COMBINE(res->body_data.pm100cnth, res->body_data.pm100cntl);

    // 2. 转换浮点数值（甲醛缩小 1000 倍，温湿度缩小 10 倍）
    data->form      = (float)PMS_5003_ST_COMBINE(res->body_data.formh, res->body_data.forml) / 1000.0f;
    data->temp      = (float)PMS_5003_ST_COMBINE_SIGNED(res->body_data.temph, res->body_data.templ) / 10.0f;
    data->mol       = (float)PMS_5003_ST_COMBINE(res->body_data.molh, res->body_data.moll) / 10.0f;
}

void pms5003st_collect_garbage_data(void)
{
    uint8_t buffer[16] = {};
    ssize_t size;
    while((size = usart_wrapper_read(&usart_dev_handle, buffer, sizeof(buffer))) != 0) {
        NEO_LOGD(TAG, "pms_collect_garbage_data %d bytes", size);
        NEO_LOGD_HEX(TAG, buffer, size);
    }
}

void pms5003st_init(void)
{
    ssize_t size;
    uint8_t try_cnt = 0;

    NEO_LOGI(TAG, "init");

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
    gpio_wrapper_set_level(PMS5003ST_SET_GPIO_PIN,   1);

    // 复位
    gpio_wrapper_set_level(PMS5003ST_RESET_GPIO_PIN, 1);  
    delay_ms(100);
    gpio_wrapper_set_level(PMS5003ST_RESET_GPIO_PIN, 0); 
    delay_ms(100);
    gpio_wrapper_set_level(PMS5003ST_RESET_GPIO_PIN, 1); 

    delay_ms(3000);

    
    // 设置为被动模式
    NEO_LOGD(TAG, "pms5003st_init: try into passive mode");
    cmd.cmd = PMS5003ST_CMD_SET_MODE;
    cmd.datah = 0;
    cmd.datal = 0;    
    pms5003st_send_cmd(&cmd);
    while((size = pms5003st_read_res(&res)) > 0 &&
     !((res.lenh * 256 + res.lenl) == 4 && res.body_cmd.cmd == PMS5003ST_CMD_SET_MODE)) {
        if(size > 0) {
            NEO_LOGW(TAG, "pms5003st_init: skip res msg %d bytes", size);
        } else {
            delay_ms(100);
            try_cnt ++;
            NEO_LOGW(TAG, "wait cnt %d", try_cnt);
            if(try_cnt > PMS5003_MAX_WAIT_CNT)
                break;
        }
    }

    if(res.body_cmd.cmd != PMS5003ST_CMD_SET_MODE) {
        NEO_LOGW(TAG, "pms5003st_init: pms5003t NOT response");
    } else if(res.body_cmd.err != 0) {
            NEO_LOGD(TAG, "pms5003st_init: err = %u", res.body_cmd.err );
    } else {
        NEO_LOGD(TAG, "pms5003st_init: passive mode ok");
    }

    pms5003st_enabled = true;

    pms5003st_collect_garbage_data();
}

void pms5003st_enable(bool enable)
{
    ssize_t size;
    uint8_t try_cnt = 0;

    pms5003st_cmd_msg_t cmd = {};
    pms5003st_res_msg_t res = {};

    NEO_LOGW(TAG, "pms5003st_enable %s", enable ? "ON" : "OFF");

    if((pms5003st_enabled && enable) || (!pms5003st_enabled && !enable))
        return;

    if(enable) {
        gpio_wrapper_set_level(PMS5003ST_SET_GPIO_PIN, 1);

        // reset
        gpio_wrapper_set_level(PMS5003ST_RESET_GPIO_PIN, 1);  
        delay_ms(100);
        gpio_wrapper_set_level(PMS5003ST_RESET_GPIO_PIN, 0); 
        delay_ms(100);
        gpio_wrapper_set_level(PMS5003ST_RESET_GPIO_PIN, 1); 

        delay_ms(3000);

        // 设置为被动模式
        NEO_LOGD(TAG, "pms5003st_enable: try into passive mode");
        cmd.cmd = PMS5003ST_CMD_SET_MODE;
        cmd.datah = 0;
        cmd.datal = 0;    
        pms5003st_send_cmd(&cmd);
        while((size = pms5003st_read_res(&res)) > 0 &&
        !((res.lenh * 256 + res.lenl) == 4 && res.body_cmd.cmd == PMS5003ST_CMD_SET_MODE)) {
            if(size > 0) {
                NEO_LOGW(TAG, "pms5003st_enable: skip res msg %d bytes", size);
            } else {
                delay_ms(100);
                try_cnt ++;
                NEO_LOGW(TAG, "wait cnt %d", try_cnt);
                if(try_cnt > PMS5003_MAX_WAIT_CNT)
                    break;
            }
        }

        if(res.body_cmd.cmd != PMS5003ST_CMD_SET_MODE) {
            NEO_LOGW(TAG, "pms5003st_enable: pms5003t NOT response");
        }  else if(res.body_cmd.err != 0) {
            NEO_LOGD(TAG, "pms5003st_enable: err = %u", res.body_cmd.err );
        } else {
            NEO_LOGD(TAG, "pms5003st_enable: passive mode ok");
        }

        pms5003st_collect_garbage_data();

    } else {
        // 设置为standby模式
        NEO_LOGD(TAG, "pms5003st_enable: try into standby mode");
        cmd.cmd = PMS5003ST_CMD_STANDBY;
        cmd.datah = 0;
        cmd.datal = 0;    
        pms5003st_send_cmd(&cmd);
        while((size = pms5003st_read_res(&res)) > 0 &&
        !((res.lenh * 256 + res.lenl) == 4 && res.body_cmd.cmd == PMS5003ST_CMD_STANDBY)) {
            if(size > 0) {
                NEO_LOGW(TAG, "pms5003st_enable: skip res msg %d bytes", size);
            } else {
                delay_ms(100);
                try_cnt ++;
                NEO_LOGW(TAG, "wait cnt %d", try_cnt);
                if(try_cnt > PMS5003_MAX_WAIT_CNT)
                    break;
            }
        }

        if(res.body_cmd.cmd != PMS5003ST_CMD_STANDBY) {
            NEO_LOGW(TAG, "pms5003st_enable: pms5003t NOT response");
        } else if(res.body_cmd.err != 0) {
            NEO_LOGW(TAG, "pms5003st_enable: err = %u", res.body_cmd.err );
        } else {
            NEO_LOGD(TAG, "pms5003st_enable: standby mode ok");
        }

        pms5003st_collect_garbage_data();

        gpio_wrapper_set_level(PMS5003ST_SET_GPIO_PIN, 0);
    }

    pms5003st_enabled = enable;
}

bool pms5003st_read_data(pms5003st_data_t * data)
{
    ssize_t size;
    uint8_t try_cnt = 0;    
    pms5003st_cmd_msg_t cmd = {};
    pms5003st_res_msg_t res = {};
    cmd.cmd = PMS5003ST_CMD_READ_DATA;
    cmd.datah = 0;
    cmd.datal = 0;    
    pms5003st_send_cmd(&cmd);
    while((size = pms5003st_read_res(&res)) > 0 &&
     !((res.lenh * 256 + res.lenl) == 36)) {
        if(size > 0) {
            NEO_LOGW(TAG, "pms5003st_read_data: skip res %d bytes", size);
            pms5003st_dump_res(&res);
        } else {
            delay_ms(100);
            try_cnt ++;
            NEO_LOGW(TAG, "wait cnt %d", try_cnt);
            if(try_cnt > PMS5003_MAX_WAIT_CNT)
                break;
        }
    }

    if((res.lenh * 256 + res.lenl) == 36 && res.body_data.err == 0) {
        pms5003st_covert_data(&res, data);
        pms5003st_dump_data(data);
        return true;
    } else if(res.lenh * 256 + res.lenl != 36) {
        NEO_LOGW(TAG, "pms5003st_read_data: no data response");
    } else {
        NEO_LOGW(TAG, "pms5003st_read_data: err = %u", res.body_cmd.err );
    }
    return false;
}
