#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <stdlib.h>

#include "at_commands.h"
#include "drivers/uart.h"
#include "drivers/swd.h"

#define AT_LINE_MAX CONFIG_OBL_AT_MAX_LINE
#define AT_RESP_MAX CONFIG_OBL_AT_MAX_RESP
#define AT_QUEUE_LEN 4

struct at_line {
  uint16_t len;
  bool overflow;
  char buf[AT_LINE_MAX];
};

K_MSGQ_DEFINE(at_msgq, sizeof(struct at_line), AT_QUEUE_LEN, 4);
static bool at_echo = true;

/* ---------- output (AT thread context) ---------- */

static void at_write(const char *s) {
  obl_uart_tx_async((const uint8_t *)s, strlen(s), NULL, NULL);
}

void at_printf(const char *fmt, ...) {
  static char out[AT_RESP_MAX];
  va_list ap;

  va_start(ap, fmt);
  int n = vsnprintf(out, sizeof(out), fmt, ap);
  va_end(ap);

  if (n > 0) {
    obl_uart_tx_async((const uint8_t *)out, MIN((size_t)n, sizeof(out) - 1), NULL, NULL);
  }
}

/* ---------- RX line assembly (ISR context) ---------- */

int obl_at_rx_feed(uint8_t *data, size_t len) {
  static struct at_line cur;

  for (size_t i = 0; i < len; i++) {
    char c = (char)data[i];

    if (c == '\r' || c == '\n') {
      if (cur.len == 0 && !cur.overflow) {
        continue; /* swallow the \n of \r\n */
      }
      cur.buf[cur.len] = '\0';
      k_msgq_put(&at_msgq, &cur, K_NO_WAIT); /* drops if full */
      cur.len = 0;
      cur.overflow = false;
    } else if (cur.len < AT_LINE_MAX - 1) {
      cur.buf[cur.len++] = c;
    } else {
      cur.overflow = true; /* line too long, reject at EOL */
    }
  }

  return 0;
}

/* ---------- parsing helpers ---------- */

static bool name_eq(const char *a, const char *b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) {
      return false;
    }
  }
  return true;
}

/* Splits "1,\"hello, world\",3" in place. Quotes are stripped. */
int obl_at_split_args(char *args, char *argv[], int max) {
  int argc = 0;
  char *p = args;

  if (args == NULL) {
    return 0;
  }
  while (1) {
    if (argc >= max) {
      return -E2BIG;
    }
    if (*p == '"') {
      argv[argc++] = ++p;
      while (*p && *p != '"') {
        p++;
      }
      if (*p != '"') {
        return -EINVAL; /* unterminated quote */
      }
      *p++ = '\0';
      if (*p == '\0')
        break;
      if (*p != ',')
        return -EINVAL;
      p++;
    } else {
      argv[argc++] = p;
      while (*p && *p != ',') {
        p++;
      }
      if (*p == '\0')
        break;
      *p++ = '\0';
    }
  }
  return argc;
}

/* ---------- dispatch ---------- */

static int dispatch(char *p) {
  /* p points just after "AT" */
  if (*p == '\0') {
    return 0; /* bare "AT" */
  }

  /* Basic command: ATE0 / ATE1 */
  if ((p[0] == 'E' || p[0] == 'e') && (p[1] == '0' || p[1] == '1') &&
      p[2] == '\0') {
    at_echo = (p[1] == '1');
    return 0;
  }

  char *name = p;
  while (*p && *p != '?' && *p != '=') {
    p++;
  }
  size_t name_len = p - name;
  enum at_cmd_type type = AT_CMD_EXEC;
  char *args = NULL;

  if (*p == '?') {
    type = AT_CMD_QUERY;
    if (p[1] != '\0')
      return -EINVAL;
  } else if (*p == '=') {
    if (p[1] == '?' && p[2] == '\0') {
      type = AT_CMD_TEST;
    } else {
      type = AT_CMD_SET;
      args = p + 1;
    }
  }

  STRUCT_SECTION_FOREACH(at_cmd, cmd) {
    if (strlen(cmd->name) == name_len && name_eq(cmd->name, name, name_len)) {
      return cmd->handler(type, args);
    }
  }
  return -ENOTSUP;
}

static void process_line(struct at_line *l) {
  if (at_echo) {
    at_write(l->buf);
    at_write("\r\n");
  }
  if (l->overflow) {
    at_write("\r\nERROR\r\n");
    return;
  }

  char *p = l->buf;
  while (isspace((unsigned char)*p))
    p++;

  size_t n = strlen(p);
  while (n && isspace((unsigned char)p[n - 1]))
    p[--n] = '\0';

  if (n < 2 || !name_eq(p, "AT", 2)) {
    at_write("\r\nERROR\r\n");
    return;
  }

  int rc = dispatch(p + 2);
  at_write(rc == 0 ? "\r\nOK\r\n" : "\r\nERROR\r\n");
}

static void at_thread_fn(void *a, void *b, void *c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);
  static struct at_line line;

  for (;;) {
    k_msgq_get(&at_msgq, &line, K_FOREVER);
    process_line(&line);
  }
}

K_THREAD_DEFINE(at_thread, CONFIG_OBL_AT_STACK_SIZE, at_thread_fn, NULL, NULL,
                NULL, 6, 0, 0);

int obl_at_init(void) {
  obl_uart_set_rx_ready_cb(obl_at_rx_feed);
  obl_uart_init();
  return 0;
}

/* ---------- built-in commands ---------- */

static int cmd_gmr(enum at_cmd_type type, char *args) {
  if (type == AT_CMD_EXEC || type == AT_CMD_QUERY) {
    at_printf("\r\n+GMR: zephyr-%s\r\n", KERNEL_VERSION_STRING);
    return 0;
  }
  return -ENOTSUP;
}
AT_CMD_REGISTER(at_gmr, "+GMR", cmd_gmr, "Firmware version");

static int cmd_cmds(enum at_cmd_type type, char *args) {
  if (type != AT_CMD_EXEC)
    return -ENOTSUP;
  STRUCT_SECTION_FOREACH(at_cmd, cmd) {
    at_printf("\r\nAT%s - %s", cmd->name, cmd->help);
  }
  at_write("\r\n");
  return 0;
}
AT_CMD_REGISTER(at_cmds, "+CMDS", cmd_cmds, "List commands");

static int cmd_freq(enum at_cmd_type type, char *args) {
  if (type == AT_CMD_SET ) {
    int num = atoi(args);
    printk("Converted int %d", num);
    obl_clk_square(HZ_TO_US( num ), 100);
    at_printf("\r\n+Freq: %s\r\n", args);
    return 0;
  }
  return -ENOTSUP;
}
AT_CMD_REGISTER(at_freq, "+FREQ", cmd_freq, "Set SWDCLK (PB 13) Freq");
