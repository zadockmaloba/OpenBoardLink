#pragma once
#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/iterable_sections.h>

enum at_cmd_type { AT_CMD_EXEC, AT_CMD_QUERY, AT_CMD_TEST, AT_CMD_SET };

/* Return 0 -> "OK", negative -> "ERROR". `args` is NULL unless AT_CMD_SET. */
typedef int (*at_handler_t)(enum at_cmd_type type, char *args);

struct at_cmd {
    const char *name;       /* e.g. "+GMR" (without the leading "AT") */
    at_handler_t handler;
    const char *help;
};

#define AT_CMD_REGISTER(_id, _name, _handler, _help)                     \
    STRUCT_SECTION_ITERABLE(at_cmd, _id) = {                             \
        .name = _name, .handler = _handler, .help = _help,               \
    }

int  obl_at_init(void);
int obl_at_rx_feed(uint8_t *data, size_t len);     /* ISR-safe */
void obl_at_printf(const char *fmt, ...);           /* AT thread only */
int  obl_at_split_args(char *args, char *argv[], int max); /* returns argc, <0 on error */
