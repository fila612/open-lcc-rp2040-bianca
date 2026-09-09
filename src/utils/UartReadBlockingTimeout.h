//
// Created by Magnus Nordlander on 2022-12-24.
//

#ifndef SMART_LCC_UARTREADBLOCKINGTIMEOUT_H
#define SMART_LCC_UARTREADBLOCKINGTIMEOUT_H

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// [MOD] Was pico/timeout_helper.h's init_single_timeout_until()/check_timeout_fn, whose
// check_timeout_fn signature gained a second (reset) parameter in newer Pico SDK versions and
// broke compilation here. Replaced with a plain time_reached() check against the deadline,
// which is what a "single timeout until" helper amounts to anyway, and matches the pattern
// already used elsewhere in this codebase (e.g. Automations.cpp, SystemController.cpp).
static inline bool uart_read_blocking_timeout(uart_inst_t *uart, uint8_t *dst, size_t len, absolute_time_t timeout_time) {
    for (size_t i = 0; i < len; ++i) {
        while (!uart_is_readable(uart)) {
            if (time_reached(timeout_time)) {
                return false;
            }

            tight_loop_contents();
        }
        *dst++ = uart_get_hw(uart)->dr;
    }

    return true;
}

#endif //SMART_LCC_UARTREADBLOCKINGTIMEOUT_H
