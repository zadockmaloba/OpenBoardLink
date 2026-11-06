#include <stdio.h>
#include <zephyr/kernel.h>

#include "drivers/uart.h"
#include "lib/at_commands.h"

/* 10000 msec = 10 sec */
#define SLEEP_TIME_MS 1000

int main(void) {
  obl_at_init();

  while (1) {
    k_msleep(SLEEP_TIME_MS);
  }
  return 0;
}
