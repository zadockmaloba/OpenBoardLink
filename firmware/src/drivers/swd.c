#include "swd.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/swdp.h>
#include <zephyr/drivers/gpio.h>

static const struct device *const swd_dev =
    DEVICE_DT_GET(DT_NODELABEL(swdp0));

static const struct gpio_dt_spec clk_pin =
    GPIO_DT_SPEC_GET(DT_NODELABEL(swdp0), clk_gpios);

/* Toggle SWCLK as fast as the GPIO API allows, then print the measured rate. */
int obl_clk_test(uint32_t toggles)
{
    if (!gpio_is_ready_dt(&clk_pin)) {
        return -ENODEV;
    }
    int rc = gpio_pin_configure_dt(&clk_pin, GPIO_OUTPUT_LOW);
    if (rc) {
        return rc;
    }

    unsigned int key = irq_lock();          /* no jitter from interrupts */
    uint32_t start = k_cycle_get_32();

    for (uint32_t i = 0; i < toggles; i++) {
        gpio_pin_toggle_dt(&clk_pin);
    }

    uint32_t cycles = k_cycle_get_32() - start;
    irq_unlock(key);

    gpio_pin_set_dt(&clk_pin, 0);

    uint64_t hz = sys_clock_hw_cycles_per_sec();
    uint64_t ns_per_toggle = ((uint64_t)cycles * 1000000000ULL) / (hz * toggles);

    printk("%u toggles: %u cycles, %llu ns/toggle, ~%llu kHz square wave\n",
           toggles, cycles, ns_per_toggle,
           1000000ULL / (2 * ns_per_toggle));
    return 0;
}

/* Square wave at a known frequency for sanity-checking the scope/analyzer. */
void obl_clk_square(uint32_t half_period_us, uint32_t cycles)
{
    int rc = gpio_pin_configure_dt(&clk_pin, GPIO_OUTPUT_LOW);
    if (rc) {
      printk("Error testing PB 13\n");
      return;
    }
    printk("Clock period: %d us", half_period_us);
    for (uint32_t i = 0; i < cycles; i++) {
        gpio_pin_set_dt(&clk_pin, 1);
        k_sleep(K_USEC( half_period_us ));
        gpio_pin_set_dt(&clk_pin, 0);
        k_sleep(K_USEC( half_period_us ));
    }
}

void obl_clk_square_inf(uint32_t half_period_us)
{
    int rc = gpio_pin_configure_dt(&clk_pin, GPIO_OUTPUT_LOW);
    if (rc) {
      printk("Error testing PB 13\n");
      return;
    }
    printk("Clock period: %d us", half_period_us);
    while (1) {
        gpio_pin_set_dt(&clk_pin, 1);
        k_sleep(K_USEC( half_period_us ));
        gpio_pin_set_dt(&clk_pin, 0);
        k_sleep(K_USEC( half_period_us ));
    }
}

int obl_swdp_init() {
  if (!device_is_ready(swd_dev)) {
    return -ENODEV;
  }
}

int obl_swdp_uninit() {
}
