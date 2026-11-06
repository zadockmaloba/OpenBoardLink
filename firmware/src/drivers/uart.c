#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/version.h>

#include "uart.h"

static const struct device *const uart_dev =
    DEVICE_DT_GET(DT_NODELABEL(usart1));

struct tx_req {
    void *fifo_reserved;            /* required by k_fifo */
    const uint8_t *data;
    size_t len;
    obl_uart_tx_done_cb_t cb;
    void *user_data;
};

K_MEM_SLAB_DEFINE(tx_req_slab, sizeof(struct tx_req), CONFIG_OBL_UART_TX_REQ_COUNT, 4);
K_FIFO_DEFINE(tx_req_queue);
static K_MUTEX_DEFINE(tx_mutex);

/* Buffer pool for our TX payloads */
NET_BUF_POOL_DEFINE(tx_pool, CONFIG_OBL_UART_BUFFER_COUNT,
                    CONFIG_OBL_UART_MAX_TX_LEN, 0, NULL);

static K_FIFO_DEFINE(tx_queue);
static struct k_spinlock tx_lock;
static struct net_buf *tx_pending_buffer = NULL;
static bool tx_busy;

static uint8_t async_rx_buffer[2][CONFIG_OBL_UART_RX_CHUNK_LEN];
static volatile uint8_t async_rx_buffer_idx;

static rx_rdy_cb_t rx_rdy_callback = NULL;

static void tx_kick(void) {
  k_spinlock_key_t key = k_spin_lock(&tx_lock);

  while (!tx_busy) {
    struct net_buf *buf = k_fifo_get(&tx_queue, K_NO_WAIT);
    if (buf == NULL) {
      break;
    }

    tx_pending_buffer = buf;
    tx_busy = true;

    int rc = uart_tx(uart_dev, buf->data, buf->len, SYS_FOREVER_US);
    if (rc != 0) {
      printk("uart_tx failed (%d)\n", rc);
      tx_pending_buffer = NULL;
      tx_busy = false;
      net_buf_unref(buf);
    }
  }

  k_spin_unlock(&tx_lock, key);
}

static void uart_callback(const struct device *dev, struct uart_event *evt,
                          void *user_data) {
  int rc;

  switch (evt->type) {
  case UART_TX_DONE:
  case UART_TX_ABORTED: {
    k_spinlock_key_t key = k_spin_lock(&tx_lock);
    struct net_buf *done = tx_pending_buffer;

    tx_pending_buffer = NULL;
    tx_busy = false;
    k_spin_unlock(&tx_lock, key);

    if (done != NULL) {
      net_buf_unref(done);
    }
    tx_kick(); /* send next queued chunk */
    break;
  }
  case UART_RX_BUF_REQUEST:
    rc = uart_rx_buf_rsp(dev, async_rx_buffer[async_rx_buffer_idx],
                         sizeof(async_rx_buffer[0]));
    __ASSERT_NO_MSG(rc == 0);
    async_rx_buffer_idx ^= 1;
    break;
  case UART_RX_RDY:
    if (rx_rdy_callback != NULL) {
      rx_rdy_callback(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
    }
    break;
  case UART_RX_BUF_RELEASED:
  case UART_RX_DISABLED:
  default:
    break;
  }
}

int obl_uart_init(void) {
  if (!device_is_ready(uart_dev)) {
    return -ENODEV;
  }

  int rc = uart_callback_set(uart_dev, uart_callback, (void *)uart_dev);
  if (rc != 0) {
    return rc;
  }

  return uart_rx_enable(uart_dev, async_rx_buffer[0],
                        CONFIG_OBL_UART_RX_CHUNK_LEN, 100);
}

/* Thread context only: net_buf_alloc(K_FOREVER) can block */
int obl_uart_tx(const uint8_t *data, size_t len) {
  while (len > 0) {
    struct net_buf *buf = net_buf_alloc(&tx_pool, K_FOREVER);
    size_t chunk = MIN(len, net_buf_tailroom(buf));

    net_buf_add_mem(buf, data, chunk);
    k_fifo_put(&tx_queue, buf);
    tx_kick();

    data += chunk;
    len -= chunk;
  }
  return 0;
}

/* ISR-safe: never blocks. Data must stay valid until cb is called. */
int obl_uart_tx_async(const uint8_t *data, size_t len,
                      obl_uart_tx_done_cb_t cb, void *user_data)
{
    struct tx_req *req;

    if (data == NULL || len == 0) {
        return -EINVAL;
    }
    if (k_mem_slab_alloc(&tx_req_slab, (void **)&req, K_NO_WAIT) != 0) {
        return -ENOMEM;             /* too many requests pending */
    }

    req->data = data;
    req->len = len;
    req->cb = cb;
    req->user_data = user_data;
    k_fifo_put(&tx_req_queue, req);
    return 0;
}

void obl_uart_set_rx_ready_cb(rx_rdy_cb_t cb) {
  __ASSERT(cb != NULL, "callback can't be NULL");
  rx_rdy_callback = cb;
}

static void tx_thread_fn(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

    for (;;) {
        struct tx_req *req = k_fifo_get(&tx_req_queue, K_FOREVER);
        int rc = obl_uart_tx(req->data, req->len);   /* may block, fine here */

        if (req->cb != NULL) {
            req->cb(req->data, req->len, rc, req->user_data);
        }
        k_mem_slab_free(&tx_req_slab, req);
    }
}

K_THREAD_DEFINE(obl_uart_tx_thread, CONFIG_OBL_UART_TX_STACK_SIZE, tx_thread_fn,
                NULL, NULL, NULL, CONFIG_OBL_UART_TX_PRIO, 0, 0);
