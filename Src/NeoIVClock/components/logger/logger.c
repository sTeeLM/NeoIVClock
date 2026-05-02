#include <stdio.h>
#include "logger.h"
static const char* TAG = "LOGGER";

void logger_init(void)
{
    NEO_LOGI(TAG, "init");
}
