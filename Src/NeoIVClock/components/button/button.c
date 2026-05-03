#include "button.h"
#include "logger.h"

static const char * TAG = "BUTTON";

void button_init(void)
{
    NEO_LOGI(TAG, "init");
}

bool button_is_factory_reset(void) 
{
    // Check if the button is pressed for factory reset
    // This is a placeholder implementation
    // Replace with actual button press logic
    return false; // Return true if factory reset is needed
}