/****************************************************************************
 * arch/risc-v/src/erbium/erbium_start.c
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

#include <nuttx/arch.h>
#include <nuttx/init.h>

#include <arch/csr.h>

#include "chip.h"
#include "riscv_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_FEATURES
#define showprogress(c) up_putc(c)
#else
#define showprogress(c)
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void erbium_fpuconfig(void)
{
#ifdef CONFIG_ARCH_FPU
  __asm__ volatile
    (
      "csrs mstatus, %0\n"
      "fsflags zero\n"
      "fsrm zero\n"
      :
      : "r"(MSTATUS_FS_INIT)
      : "memory"
    );
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void erbium_start(int mhartid)
{
  uint32_t *dest;

  erbium_fpuconfig();

  if (mhartid > 0)
    {
      goto cpux;
    }

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  showprogress('A');

#ifdef USE_EARLYSERIALINIT
  up_earlyserialinit();
#endif

  showprogress('B');

  nx_start();

cpux:

#ifdef CONFIG_SMP
  riscv_cpu_boot(mhartid);
#endif

  while (true)
    {
      asm("WFI");
    }
}

void riscv_serialinit(void)
{
  up_serialinit();
}
