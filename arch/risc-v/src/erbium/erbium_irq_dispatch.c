/****************************************************************************
 * arch/risc-v/src/erbium/erbium_irq_dispatch.c
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
#include <sys/types.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include "hardware/erbium_plic.h"
#include "riscv_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARCH_RV32
#  define RV_IRQ_MASK 27
#else
#  define RV_IRQ_MASK 59
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void *riscv_dispatch_irq(uintptr_t vector, uintptr_t *regs)
{
  int irq = (vector >> RV_IRQ_MASK) | (vector & 0xf);

  if (RISCV_IRQ_EXT == irq)
    {
#ifdef CONFIG_ERBIUM_PLIC
      uintptr_t val = getreg32(ERBIUM_PLIC_CLAIM);
      irq += val;
#else
      return regs;
#endif
    }

  if (RISCV_IRQ_EXT != irq)
    {
      regs = riscv_doirq(irq, regs);
    }

  if (RISCV_IRQ_EXT <= irq)
    {
#ifdef CONFIG_ERBIUM_PLIC
      putreg32(irq - RISCV_IRQ_EXT, ERBIUM_PLIC_CLAIM);
#endif
    }

  return regs;
}
