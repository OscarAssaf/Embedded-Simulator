#include "hal_uart.h"

#define UART0_BASE       (0x4000C000UL)
#define UART_DR          (*(volatile uint32_t *)(UART0_BASE + 0x000))
#define UART_FR          (*(volatile uint32_t *)(UART0_BASE + 0x018))
#define UART_IBRD        (*(volatile uint32_t *)(UART0_BASE + 0x024))
#define UART_FBRD        (*(volatile uint32_t *)(UART0_BASE + 0x028))
#define UART_LCRH        (*(volatile uint32_t *)(UART0_BASE + 0x02C))
#define UART_CTL         (*(volatile uint32_t *)(UART0_BASE + 0x030))

#define UART_FR_TXFF     (1UL << 5)
#define UART_FR_RXFE     (1UL << 4)

void hal_uart_init(void) {
    UART_CTL = 0;
    UART_IBRD = 1;
    UART_FBRD = 0;
    UART_LCRH = (0x3UL << 5); /* 8-bit word length, FIFO enabled */
    UART_CTL = (1UL << 0) | (1UL << 8) | (1UL << 9); /* UARTEN, TXE, RXE */
}
