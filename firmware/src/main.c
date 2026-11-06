#include "drivers/uart.h"
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "drivers/uart.h"

/* 10000 msec = 10 sec */
#define SLEEP_TIME_MS 1000

#define LED0_NODE DT_ALIAS(status_led)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED_TASK_STACK_SIZE 256
#define LED_TASK_PRIORITY 5

static void led_task_thread_entry(void *arg1, void *arg2, void *arg3);

/* Create the thread at compile time */
K_THREAD_DEFINE(led_task_thread_id, LED_TASK_STACK_SIZE, led_task_thread_entry,
                NULL, NULL, NULL, LED_TASK_PRIORITY, 0, 100);

static void led_task_thread_entry(void *arg1, void *arg2, void *arg3) {
  int ret;
  bool led_state = true;

  if (!gpio_is_ready_dt(&led)) {
    //printk("GPIO device not ready\n");
    k_panic();
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    //printk("GPIO device not accessible\n");
    k_panic();
  }

  while (1) {
    ret = gpio_pin_toggle_dt(&led);
    if (ret < 0) {
      printk("GPIO device not accessible\n");
      k_panic();
    }

    led_state = !led_state;
    // printf("LED state: %s\n", led_state ? "ON" : "OFF");
    k_msleep(SLEEP_TIME_MS);
  }
}

static int uart_rx_callback(uint8_t *data, size_t len) {
  return obl_uart_tx_async(data, len, NULL, NULL);
}

int main(void) {
  obl_uart_set_rx_ready_cb(uart_rx_callback);
  obl_uart_init();

  while (1) {
    k_msleep(SLEEP_TIME_MS);
  }
  return 0;
}
