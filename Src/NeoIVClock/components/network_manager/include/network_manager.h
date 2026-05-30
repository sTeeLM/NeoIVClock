#ifndef NEO_IV_CLOCK_METWORK_MANAGER_H
#define NEO_IV_CLOCK_METWORK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

void network_manager_init(void);
void network_manager_start_confg_portal(void);
void network_manager_stop_confg_portal(void);

bool network_manager_connect_to_ap(const char *target_ssid, const char *target_password);

#endif //NEO_IV_CLOCK_METWORK_MANAGER_H