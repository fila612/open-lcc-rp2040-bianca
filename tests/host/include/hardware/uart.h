#pragma once
#include "pico/time.h"
struct uart_inst_t { volatile uint32_t dr = 0; };
inline bool uart_is_readable(uart_inst_t *) { return false; }
inline uart_inst_t *uart_get_hw(uart_inst_t *uart) { return uart; }
inline void uart_write_blocking(uart_inst_t *, const uint8_t *, size_t) {}
