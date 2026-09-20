#pragma once
#include "pico/time.h"
struct spi_inst_t {};
int spi_write_read_blocking(spi_inst_t *, const uint8_t *, uint8_t *, size_t);
int spi_write_blocking(spi_inst_t *, const uint8_t *, size_t);
int spi_read_blocking(spi_inst_t *, uint8_t, uint8_t *, size_t);
