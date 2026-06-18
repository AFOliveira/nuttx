/****************************************************************************
 * arch/risc-v/src/erbium/erbium_serial.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/irq.h>
#include <nuttx/serial/serial.h>
#include <nuttx/wdog.h>

#include <arch/irq.h>

#include "chip.h"
#include "hardware/erbium_memorymap.h"
#include "hardware/erbium_sysreg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ERBIUM_UART_CLOCK       400000000ul
#define ERBIUM_UART0_BAUD       115200ul
#define ERBIUM_UART0_RXBUFSIZE  256
#define ERBIUM_UART0_TXBUFSIZE  256
#define ERBIUM_UART_BAUD_DIV    (ERBIUM_UART_CLOCK / (16ul * ERBIUM_UART0_BAUD))
#define ERBIUM_UART_RXPOLL_TICKS MSEC2TICK(10)

#define SHAKTI_UART_BAUD        0x00
#define SHAKTI_UART_TX_REG      0x08
#define SHAKTI_UART_RCV_REG     0x10
#define SHAKTI_UART_STATUS      0x18
#define SHAKTI_UART_IEN         0x30
#define SHAKTI_UART_RX_THRESHOLD 0x40

#define STATUS_TX_EMPTY         (1u << 0)
#define STATUS_TX_FULL          (1u << 1)
#define STATUS_RX_NOT_EMPTY     (1u << 2)

#define IEN_TX_EMPTY            (1u << 0)
#define IEN_RX_NOT_EMPTY        (1u << 2)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct erbium_uart_s
{
  uintptr_t base;
  uint8_t irq;
  uint32_t ien;
  struct wdog_s rxpoll;
  bool rxpoll_enabled;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  erbium_setup(struct uart_dev_s *dev);
static void erbium_shutdown(struct uart_dev_s *dev);
static int  erbium_attach(struct uart_dev_s *dev);
static void erbium_detach(struct uart_dev_s *dev);
#ifdef CONFIG_ERBIUM_PLIC
static int  erbium_interrupt(int irq, void *context, void *arg);
#endif
static int  erbium_ioctl(struct file *filep, int cmd, unsigned long arg);
static int  erbium_receive(struct uart_dev_s *dev, unsigned int *status);
static void erbium_rxint(struct uart_dev_s *dev, bool enable);
static bool erbium_rxavailable(struct uart_dev_s *dev);
static void erbium_send(struct uart_dev_s *dev, int ch);
static void erbium_txint(struct uart_dev_s *dev, bool enable);
static bool erbium_txready(struct uart_dev_s *dev);
static bool erbium_txempty(struct uart_dev_s *dev);
static void erbium_rxpoll(wdparm_t arg);
static void erbium_rxpoll_start(struct uart_dev_s *dev);
static void erbium_rxpoll_stop(struct erbium_uart_s *priv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_uart0rxbuffer[ERBIUM_UART0_RXBUFSIZE];
static char g_uart0txbuffer[ERBIUM_UART0_TXBUFSIZE];

static struct erbium_uart_s g_uart0priv =
{
  .base = ERBIUM_UART0_BASE,
  .irq = ERBIUM_IRQ_UART0,
};

static const struct uart_ops_s g_uart_ops =
{
  .setup        = erbium_setup,
  .shutdown     = erbium_shutdown,
  .attach       = erbium_attach,
  .detach       = erbium_detach,
  .ioctl        = erbium_ioctl,
  .receive      = erbium_receive,
  .rxint        = erbium_rxint,
  .rxavailable  = erbium_rxavailable,
#ifdef CONFIG_SERIAL_IFLOWCONTROL
  .rxflowcontrol = NULL,
#endif
  .send         = erbium_send,
  .txint        = erbium_txint,
  .txready      = erbium_txready,
  .txempty      = erbium_txempty,
};

static uart_dev_t g_uart0port =
{
#ifdef CONFIG_DEV_CONSOLE
  .isconsole = true,
#endif
  .recv =
  {
    .size   = ERBIUM_UART0_RXBUFSIZE,
    .buffer = g_uart0rxbuffer,
  },
  .xmit =
  {
    .size   = ERBIUM_UART0_TXBUFSIZE,
    .buffer = g_uart0txbuffer,
  },
  .ops  = &g_uart_ops,
  .priv = &g_uart0priv,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint32_t erbium_serialin(struct erbium_uart_s *priv, uintptr_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static void erbium_serialout(struct erbium_uart_s *priv, uintptr_t offset,
                             uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

static void erbium_setuartint(struct erbium_uart_s *priv, uint32_t ien)
{
  priv->ien = ien;
  erbium_serialout(priv, SHAKTI_UART_IEN, ien);
}

static void erbium_enable_uart(void)
{
  uint32_t config;
  volatile uint32_t *system_config =
    (volatile uint32_t *)ERBIUM_SYSREG_SYSTEM_CONFIG;

  config = *system_config;
  *system_config = config | ERBIUM_SYSREG_SYSTEM_CONFIG_UART_ENABLE;
}

static void erbium_disableuartint(struct erbium_uart_s *priv, uint32_t *ien)
{
  irqstate_t flags;

  flags = enter_critical_section();

  if (ien != NULL)
    {
      *ien = priv->ien;
    }

  erbium_setuartint(priv, 0);

  leave_critical_section(flags);
}

static void erbium_restoreuartint(struct erbium_uart_s *priv, uint32_t ien)
{
  irqstate_t flags;

  flags = enter_critical_section();
  erbium_setuartint(priv, ien);
  leave_critical_section(flags);
}

static void erbium_rxpoll(wdparm_t arg)
{
  struct uart_dev_s *dev = (struct uart_dev_s *)arg;
  struct erbium_uart_s *priv;

  DEBUGASSERT(dev != NULL && dev->priv != NULL);
  priv = (struct erbium_uart_s *)dev->priv;

  if ((priv->ien & IEN_RX_NOT_EMPTY) != 0)
    {
      uart_recvchars(dev);
    }

  if (priv->rxpoll_enabled)
    {
      wd_start(&priv->rxpoll, ERBIUM_UART_RXPOLL_TICKS,
               erbium_rxpoll, (wdparm_t)dev);
    }
}

static void erbium_rxpoll_start(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  priv->rxpoll_enabled = true;
  wd_start(&priv->rxpoll, ERBIUM_UART_RXPOLL_TICKS,
           erbium_rxpoll, (wdparm_t)dev);
}

static void erbium_rxpoll_stop(struct erbium_uart_s *priv)
{
  priv->rxpoll_enabled = false;
  wd_cancel(&priv->rxpoll);
}

static void erbium_tx_trigger(struct erbium_uart_s *priv)
{
  erbium_serialout(priv, SHAKTI_UART_BAUD, ERBIUM_UART_BAUD_DIV);
}

static void erbium_putc(struct erbium_uart_s *priv, int ch)
{
  while ((erbium_serialin(priv, SHAKTI_UART_STATUS) & STATUS_TX_FULL) != 0)
    {
    }

  erbium_serialout(priv, SHAKTI_UART_TX_REG, (uint32_t)ch & 0xffu);
  erbium_tx_trigger(priv);
}

static int erbium_setup(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  erbium_enable_uart();
  erbium_serialout(priv, SHAKTI_UART_BAUD, ERBIUM_UART_BAUD_DIV);
  erbium_serialout(priv, SHAKTI_UART_RX_THRESHOLD, 0);
  erbium_setuartint(priv, priv->ien);

  return OK;
}

static void erbium_shutdown(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  erbium_rxpoll_stop(priv);
  erbium_disableuartint(priv, NULL);
}

static int erbium_attach(struct uart_dev_s *dev)
{
#ifdef CONFIG_ERBIUM_PLIC
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;
  int ret;

  ret = irq_attach(priv->irq, erbium_interrupt, dev);

  if (ret == OK)
    {
      up_enable_irq(priv->irq);
    }

  return ret;
#else
  (void)dev;
  return OK;
#endif
}

static void erbium_detach(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  erbium_rxpoll_stop(priv);
#ifdef CONFIG_ERBIUM_PLIC
  up_disable_irq(priv->irq);
  irq_detach(priv->irq);
#endif
}

#ifdef CONFIG_ERBIUM_PLIC
static int erbium_interrupt(int irq, void *context, void *arg)
{
  struct uart_dev_s *dev = (struct uart_dev_s *)arg;
  struct erbium_uart_s *priv;
  uint32_t pending;
  int passes;

  DEBUGASSERT(dev != NULL && dev->priv != NULL);
  priv = (struct erbium_uart_s *)dev->priv;

  for (passes = 0; passes < 256; passes++)
    {
      pending = erbium_serialin(priv, SHAKTI_UART_STATUS) & priv->ien;

      if (pending == 0)
        {
          break;
        }

      if ((pending & IEN_RX_NOT_EMPTY) != 0)
        {
          uart_recvchars(dev);
        }

      if ((pending & IEN_TX_EMPTY) != 0)
        {
          uart_xmitchars(dev);
        }
    }

  return OK;
}
#endif

static int erbium_ioctl(struct file *filep, int cmd, unsigned long arg)
{
  return -ENOTTY;
}

static int erbium_receive(struct uart_dev_s *dev, unsigned int *status)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  if (status != NULL)
    {
      *status = 0;
    }

  return (int)(erbium_serialin(priv, SHAKTI_UART_RCV_REG) & 0xffu);
}

static void erbium_rxint(struct uart_dev_s *dev, bool enable)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;
  irqstate_t flags;

  flags = enter_critical_section();

  if (enable)
    {
      erbium_setuartint(priv, priv->ien | IEN_RX_NOT_EMPTY);
    }
  else
    {
      erbium_setuartint(priv, priv->ien & ~IEN_RX_NOT_EMPTY);
    }

  leave_critical_section(flags);

  if (enable)
    {
      erbium_rxpoll_start(dev);
      uart_recvchars(dev);
    }
  else
    {
      erbium_rxpoll_stop(priv);
    }
}

static bool erbium_rxavailable(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  return (erbium_serialin(priv, SHAKTI_UART_STATUS) &
          STATUS_RX_NOT_EMPTY) != 0;
}

static void erbium_send(struct uart_dev_s *dev, int ch)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  erbium_putc(priv, ch);
}

static void erbium_txint(struct uart_dev_s *dev, bool enable)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;
  irqstate_t flags;

  flags = enter_critical_section();

  if (enable)
    {
      erbium_setuartint(priv, priv->ien | IEN_TX_EMPTY);
      uart_xmitchars(dev);
    }
  else
    {
      erbium_setuartint(priv, priv->ien & ~IEN_TX_EMPTY);
    }

  leave_critical_section(flags);
}

static bool erbium_txready(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  return (erbium_serialin(priv, SHAKTI_UART_STATUS) & STATUS_TX_FULL) == 0;
}

static bool erbium_txempty(struct uart_dev_s *dev)
{
  struct erbium_uart_s *priv = (struct erbium_uart_s *)dev->priv;

  return (erbium_serialin(priv, SHAKTI_UART_STATUS) & STATUS_TX_EMPTY) != 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void up_earlyserialinit(void)
{
  erbium_disableuartint(&g_uart0priv, NULL);
  erbium_setup(&g_uart0port);
}

void up_serialinit(void)
{
#ifdef CONFIG_DEV_CONSOLE
  uart_register("/dev/console", &g_uart0port);
#endif

  uart_register("/dev/ttyS0", &g_uart0port);
}

void up_putc(int ch)
{
  uint32_t ien;

  erbium_disableuartint(&g_uart0priv, &ien);

  if (ch == '\n')
    {
      erbium_putc(&g_uart0priv, '\r');
    }

  erbium_putc(&g_uart0priv, ch);
  erbium_restoreuartint(&g_uart0priv, ien);
}
