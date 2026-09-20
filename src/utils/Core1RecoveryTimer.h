#ifndef SMART_LCC_CORE1RECOVERYTIMER_H
#define SMART_LCC_CORE1RECOVERYTIMER_H

#include <optional.hpp>
#include <pico/time.h>

// [MOD] Only restart Core 1 while its status queue remains stalled. Recovery
// cancels a pending restart, including the five-second retry after a restart.
class Core1RecoveryTimer {
public:
    bool shouldRestart(bool queueFull, absolute_time_t now) {
        if (!queueFull) {
            deadline.reset();
        } else if (!deadline.has_value()) {
            deadline = delayed_by_ms(now, 2000);
        } else if (absolute_time_diff_us(deadline.value(), now) >= 0) {
            deadline = delayed_by_ms(now, 5000);
            return true;
        }
        return false;
    }

private:
    nonstd::optional<absolute_time_t> deadline{};
};

#endif
