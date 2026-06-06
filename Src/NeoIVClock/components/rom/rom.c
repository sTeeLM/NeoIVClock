#include "rom.h"
#include "logger.h"
#include "i2c_wrapper.h"
#include "delay.h"

static const char * TAG = "ROM";

// AT24C01
// page Size is 8 bytes
// total size is 128 bytes (1024 bits)
// addr: 1010 A2 A1 A0
//#define ROM_ADDR_PAGE_MASK 0x0
//#define ROM_PAGE_SIZE 8
//#define ROM_TOTAL_SIZE 128
//#define ROM_PAGES (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_8BIT // 8bits

// AT24C02
// page Size is 8 bytes
// total size is 256 bytes (2048 bits)
// addr: 1010 A2 A1 A0
//#define ROM_ADDR_PAGE_MASK 0x0
//#define ROM_PAGE_SIZE 8
//#define ROM_TOTAL_SIZE 256
//#define ROM_PAGES (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_8BIT // 8bits


// AT24C04
// page Size is 16 bytes
// total size is 512 bytes (4096 bits)
// addr: 1010 A2 A1 P0
//#define ROM_ADDR_PAGE_MASK 0x1
//#define ROM_PAGE_SIZE 16
//#define ROM_TOTAL_SIZE 512
//#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_8BIT // 8bits

// AT24C08
// page Size is 16 bytes
// total size is 1024 bytes (8192 bits)
// addr: 1010 A2 P1 P0
//#define ROM_ADDR_PAGE_MASK 0x3
//#define ROM_PAGE_SIZE 16
//#define ROM_TOTAL_SIZE 1024
//#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_8BIT // 8bits

// AT24C16
// page Size is 16 bytes
// total size is 2048 bytes (16384 bits)
// addr: 1010 P2 P1 P0
//#define ROM_ADDR_PAGE_MASK 0x7
//#define ROM_PAGE_SIZE 16
//#define ROM_TOTAL_SIZE 2048
//#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_8BIT // 8bits

// AT24C32
// page Size is 32 bytes
// total size is 4096 bytes (32768 bits)
// addr: 1010 A2 A1 A0
#define ROM_ADDR_PAGE_MASK 0x0
#define ROM_PAGE_SIZE 32
#define ROM_TOTAL_SIZE 4096
#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_16BIT // 16bits

// AT24C64
// page Size is 32 bytes
// total size is 8192 bytes (65536 bits)
// addr: 1010 A2 A1 A0
//#define ROM_ADDR_MASK 0x0
//#define ROM_PAGE_SIZE 32
//#define ROM_TOTAL_SIZE 8192
//#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_16BIT // 16bits

// AT24C128
// page Size is 64 bytes
// total size is 16384 bytes (131072 bits)
// addr: 1010 0 A1 A0
//#define ROM_ADDR_MASK 0x0
//#define ROM_PAGE_SIZE 64
//#define ROM_TOTAL_SIZE 16384
//#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_16BIT // 16bits

// AT24C256
// page Size is 64 bytes
// total size is 32768 bytes (262144 bits)
// addr: 1010 0 A1 A0
//#define ROM_ADDR_MASK 0x0
//#define ROM_PAGE_SIZE 64
//#define ROM_TOTAL_SIZE 32768
//#define ROM_PAGES  (ROM_TOTAL_SIZE / ROM_PAGE_SIZE)
//#define ROM_MEM_ADDRSIZE I2C_ADDR_MODE_16BIT // 16bits


#define ROM_ADDR 0x57
static i2c_wrapper_dev_handle_t rom_i2c_dev_handle[ROM_ADDR_PAGE_MASK + 1];

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
    uint8_t i;
    NEO_LOGI(TAG, "init");
    memset(rom_i2c_dev_handle, 0, sizeof(rom_i2c_dev_handle));
    for(i = 0 ; i <= ROM_ADDR_PAGE_MASK ; i ++) {
        i2c_wrapper_add_dev((ROM_ADDR & ~ROM_ADDR_PAGE_MASK) + i, 400000, &rom_i2c_dev_handle[i]);
    }
    rom_dump();
}
void rom_read(uint32_t addr, uint8_t * pdata, uint32_t size)
{
    uint8_t page_index = 0;

    if(ROM_MEM_ADDRSIZE == I2C_ADDR_MODE_8BIT) {
        page_index = (addr >> 8) & ROM_ADDR_PAGE_MASK;
    } else if(ROM_MEM_ADDRSIZE == I2C_ADDR_MODE_16BIT) {
        page_index = (addr >> 16) & ROM_ADDR_PAGE_MASK;
    }

    if ((addr + size) > ROM_TOTAL_SIZE) {
        NEO_LOGE(TAG, "rom_read out of range: addr = %04x, size = %04x", addr, size);
        return;
    }
    i2c_wrapper_read(&rom_i2c_dev_handle[page_index], addr, ROM_MEM_ADDRSIZE, (uint8_t*)pdata, size);
    
}

void rom_write(uint32_t addr, uint8_t *pdata, uint32_t size) 
{
    // 1. 处理前端未对齐的零碎字节（写到当前页边界）
    uint32_t page_offset = addr % ROM_PAGE_SIZE;
    uint32_t prefix_len, i;

    uint8_t page_index = 0;

    if(ROM_MEM_ADDRSIZE == I2C_ADDR_MODE_8BIT) {
        page_index = (addr >> 8) & ROM_ADDR_PAGE_MASK;
    } else if(ROM_MEM_ADDRSIZE == I2C_ADDR_MODE_16BIT) {
        page_index = (addr >> 16) & ROM_ADDR_PAGE_MASK;
    }    

    if (page_offset != 0) 
    {
        prefix_len = ROM_PAGE_SIZE - page_offset;
        if (size < prefix_len) 
        {
            prefix_len = size;
        }

        for (i = 0; i < prefix_len; i++) 
        {
            i2c_wrapper_write(&rom_i2c_dev_handle[page_index], addr++, ROM_MEM_ADDRSIZE, pdata++, 1);
            delay_ms(10); // 若单字节写需要等待，在此处调用
        }
        size -= prefix_len;
    }

    // 2. 此时地址已对齐页边界，处理中间的整页数据
    while (size >= ROM_PAGE_SIZE) 
    {
        i2c_wrapper_write(&rom_i2c_dev_handle[page_index], addr, ROM_MEM_ADDRSIZE, pdata, ROM_PAGE_SIZE);
        delay_ms(10); // 页写通常需要等待硬件烧录时间（如5ms）

        addr += ROM_PAGE_SIZE;
        pdata += ROM_PAGE_SIZE;
        size -= ROM_PAGE_SIZE;
    }

    // 3. 处理后端剩余的未对齐零碎字节（不足一页）
    while (size > 0) 
    {
        i2c_wrapper_write(&rom_i2c_dev_handle[page_index], addr++, ROM_MEM_ADDRSIZE, pdata++, 1);
        delay_ms(10);
        size--;
    }
}
