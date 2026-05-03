#include "rom.h"
#include "logger.h"

static const char * TAG = "ROM";

void rom_init(void)
{
    NEO_LOGI(TAG, "init");
}
bool rom_read(uint32_t addr, uint8_t * pdata, uint32_t size)
{
    // Read data from ROM at the specified address
    // This is a placeholder implementation
    // Replace with actual ROM read logic
    return true; // Return true if read was successful
}

bool rom_write(uint32_t addr, uint8_t * pdata, uint32_t size)
{
    // Write data to ROM at the specified address
    // This is a placeholder implementation
    // Replace with actual ROM write logic
    return true; // Return true if write was successful

}