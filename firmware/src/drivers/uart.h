#pragma once

#include <stdint.h>
#include <stddef.h>

typedef int (*rx_rdy_cb_t)(uint8_t *data, size_t len);
typedef void (*obl_uart_tx_done_cb_t)(const uint8_t *data, size_t len,
                                      int result, void *user_data);

int obl_uart_init();
void obl_uart_set_rx_ready_cb(rx_rdy_cb_t cb);
int obl_uart_tx(const uint8_t *data, size_t len);
int obl_uart_tx_async(const uint8_t *data, size_t len,
                      obl_uart_tx_done_cb_t cb, void *user_data);
