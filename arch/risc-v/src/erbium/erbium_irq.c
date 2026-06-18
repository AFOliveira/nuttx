/****************************************************************************
 * arch/risc-v/src/erbium/erbium_irq.c
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <debug.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include "chip.h"
#include "riscv_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void up_irqinitialize(void)
{
#ifdef CONFIG_ERBIUM_PLIC
  int id;
#endif

  up_irq_save();

#ifdef CONFIG_ERBIUM_PLIC
  putreg32(0, ERBIUM_PLIC_ENABLE1);
#endif

#if defined(CONFIG_STACK_COLORATION) && CONFIG_ARCH_INTERRUPTSTACK > 15
  size_t intstack_size = (CONFIG_ARCH_INTERRUPTSTACK & ~15);
  riscv_stack_color(g_intstackalloc, intstack_size);
#endif

#ifdef CONFIG_ERBIUM_PLIC
  for (id = 1; id <= ERBIUM_PLIC_NDEV; id++)
    {
      putreg32(1, (uintptr_t)(ERBIUM_PLIC_PRIORITY + 4 * id));
    }

  putreg32(0, ERBIUM_PLIC_THRESHOLD);
#endif

  riscv_exception_attach();

#ifdef CONFIG_SMP
  irq_attach(RISCV_IRQ_MSOFT, riscv_pause_handler, NULL);
  up_enable_irq(RISCV_IRQ_MSOFT);
#endif

#ifndef CONFIG_SUPPRESS_INTERRUPTS
  up_irq_enable();
#endif
}

void up_disable_irq(int irq)
{
#ifdef CONFIG_ERBIUM_PLIC
  int extirq;
#endif

  if (irq == RISCV_IRQ_SOFT)
    {
      CLEAR_CSR(CSR_IE, IE_SIE);
    }
  else if (irq == RISCV_IRQ_TIMER)
    {
      CLEAR_CSR(CSR_IE, IE_TIE);
    }
  else if (irq > RISCV_IRQ_EXT)
    {
#ifdef CONFIG_ERBIUM_PLIC
      extirq = irq - RISCV_IRQ_EXT;

      if (0 <= extirq && extirq <= ERBIUM_PLIC_NDEV)
        {
          modifyreg32(ERBIUM_PLIC_ENABLE1 + (4 * (extirq / 32)),
                      1 << (extirq % 32), 0);
        }
      else
        {
          ASSERT(false);
        }
#endif
    }
}

void up_enable_irq(int irq)
{
#ifdef CONFIG_ERBIUM_PLIC
  int extirq;
#endif

  if (irq == RISCV_IRQ_SOFT)
    {
      SET_CSR(CSR_IE, IE_SIE);
    }
  else if (irq == RISCV_IRQ_TIMER)
    {
      SET_CSR(CSR_IE, IE_TIE);
    }
  else if (irq > RISCV_IRQ_EXT)
    {
#ifdef CONFIG_ERBIUM_PLIC
      extirq = irq - RISCV_IRQ_EXT;

      if (0 <= extirq && extirq <= ERBIUM_PLIC_NDEV)
        {
          modifyreg32(ERBIUM_PLIC_ENABLE1 + (4 * (extirq / 32)),
                      0, 1 << (extirq % 32));
        }
      else
        {
          ASSERT(false);
        }
#endif
    }
}

irqstate_t up_irq_enable(void)
{
  irqstate_t oldstat;

#ifdef CONFIG_ERBIUM_PLIC
  SET_CSR(CSR_IE, IE_EIE);
#endif
  oldstat = READ_AND_SET_CSR(CSR_STATUS, STATUS_IE);

  return oldstat;
}
