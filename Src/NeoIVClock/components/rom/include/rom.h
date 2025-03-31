#ifndef NEO_IV_CLOCK_ROM_H
#define NEO_IV_CLOCK_ROM_H

#include <stdint.h>
#include <stdbool.h>

void rom_init(void);
bool rom_read(uint32_t addr, uint8_t * pdata, uint32_t size);
bool rom_write(uint32_t addr, uint8_t * pdata, uint32_t size);

#endif // NEO_IV_CLOCK_ROM_H