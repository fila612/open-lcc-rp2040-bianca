#pragma once
#include <cstddef>
#include <cstdint>
using uint = unsigned int;
using absolute_time_t = uint64_t;
inline absolute_time_t test_now = 1000000;
inline constexpr absolute_time_t nil_time = 0;
inline absolute_time_t get_absolute_time() { return test_now; }
inline int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to) {
    return static_cast<int64_t>(to) - static_cast<int64_t>(from);
}
inline absolute_time_t delayed_by_ms(absolute_time_t time, uint32_t ms) { return time + uint64_t(ms) * 1000; }
inline absolute_time_t make_timeout_time_ms(uint32_t ms) { return delayed_by_ms(test_now, ms); }
inline bool time_reached(absolute_time_t time) { return test_now >= time; }
inline uint32_t to_ms_since_boot(absolute_time_t time) { return time / 1000; }
inline void sleep_until(absolute_time_t time) { if (test_now < time) test_now = time; }
inline void busy_wait_ms(uint32_t ms) { test_now += uint64_t(ms) * 1000; }
inline void tight_loop_contents() { ++test_now; }
