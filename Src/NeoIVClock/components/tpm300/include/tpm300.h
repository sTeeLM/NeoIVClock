#ifndef NEO_IV_CLOCK_TPM300_H
#define NEO_IV_CLOCK_TPM300_H

#include <stdint.h>
#include <stdbool.h>

void tpm300_init(void);
float tpm300_read_data(void);

#endif // NEO_IV_CLOCK_TPM300_H
