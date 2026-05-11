#include "rom.h"
#include "logger.h"
#include "i2c_wrapper.h"
#include "delay.h"

static const char * TAG = "ROM";

#define ROM_ADDR 0x57
// AT24C32
// page Size is 32 bytes
// total size is 4096 bytes (32768 bits)
#define ROM_PAGE_SIZE 32
#define ROM_TOTAL_SIZE 4096
#define ROM_PAGES  128
#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_16BIT // 16bits


// AT24C02
// page Size is 32 bytes
// total size is 256 bytes (2048 bits)
//#define ROM_PAGE_SIZE 8
//#define ROM_TOTAL_SIZE 256
//#define ROM_PAGES  32
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_8BIT // 8bits

static i2c_wrapper_dev_handle_t rom_i2c_dev_handle;

static void rom_dump(void) 
{
  int32_t i;
  uint8_t buf[ROM_PAGE_SIZE];
  NEO_LOGD(TAG, "dump rom content begin------------------------");
  for(i = 0 ; i < ROM_PAGES ; i ++) {
    rom_read(i * ROM_PAGE_SIZE, buf, ROM_PAGE_SIZE);
    NEO_LOGD_HEX(TAG, buf, ROM_PAGE_SIZE);
  }
  NEO_LOGD(TAG, "dump rom content end------------------------");
}


void rom_init(void)
{
    NEO_LOGI(TAG, "init");
    i2c_wrapper_add_dev(ROM_ADDR, 400000, &rom_i2c_dev_handle);
    rom_dump();
}
void rom_read(uint32_t addr, uint8_t * pdata, uint32_t size)
{
    if ((addr + size) > ROM_TOTAL_SIZE) {
        NEO_LOGE(TAG, "rom_read out of range: addr = %04x, size = %04x", addr, size);
        return;
    }
    i2c_wrapper_read(&rom_i2c_dev_handle, addr, ROM_MEM_ADDRSIZE, (uint8_t*)pdata, size);
    
}


void rom_write(uint32_t addr, uint8_t * pdata, uint32_t size)
{
    uint32_t i, header_size;
  
    if ((addr + size) > ROM_TOTAL_SIZE) {
        NEO_LOGE(TAG, "rom_write out of range: addr = %04x, size = %04x", addr, size);
        return;
    }
  
    // 计算首部大小，使得后续的写操作都对齐到页边界
    header_size = ROM_PAGE_SIZE - (size % ROM_PAGE_SIZE);
    header_size = header_size > size ? size : header_size;
  
    // 写入首部，使得后续的写操作都对齐到页边界
    for( i = 0 ; i < header_size ; i ++) {
        i2c_wrapper_write(&rom_i2c_dev_handle, addr + i, ROM_MEM_ADDRSIZE, pdata +  i, 1);
        delay_ms(10);
    }
  
    size  -= header_size;
    addr  += header_size;
    pdata += header_size;
    
    // 这部分可以页对齐了，直接按页写入
    for( i = 0 ; i < size / ROM_PAGE_SIZE ; i ++) {
        i2c_wrapper_write(&rom_i2c_dev_handle, addr, ROM_MEM_ADDRSIZE, pdata, ROM_PAGE_SIZE);
        size  -= ROM_PAGE_SIZE;
        addr  += ROM_PAGE_SIZE;
        pdata += ROM_PAGE_SIZE;
        delay_ms(10);
    }
  
    // 写入剩余的部分
    for( i = 0 ; i < size % ROM_PAGE_SIZE ; i ++) {
        i2c_wrapper_write(&rom_i2c_dev_handle, addr + i, ROM_MEM_ADDRSIZE, pdata +  i, 1);
        delay_ms(10);
    }

}