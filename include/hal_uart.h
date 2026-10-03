#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>
#include <stdbool.h>

void hal_uart_init(void);
void hal_uart_putc(uint8_t c);
void hal_uart_write(const uint8_t *data, uint16_t length);
bool hal_uart_has_data(void);
bool hal_uart_getc(uint8_t *c);

#endif
