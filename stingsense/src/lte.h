#ifndef _LTE_H_
#define _LTE_H_

#include <zephyr/kernel.h>
#include <stdbool.h>

bool init_lte(void);
bool lte_send_data(const char *data, size_t len);
bool lte_is_connected(void);
void lte_get_modem_info(void);
bool lte_reconnect(void);
void lte_shutdown(void);

#endif // _LTE_H_