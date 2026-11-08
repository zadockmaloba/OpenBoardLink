#pragma once
#include <stdint.h>

#define HZ_TO_US(x) (uint32_t)(1000000/x)

void obl_clk_square(uint32_t half_period_us, uint32_t cycles);
void obl_clk_square_inf(uint32_t half_period_us);
