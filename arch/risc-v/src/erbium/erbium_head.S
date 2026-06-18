/****************************************************************************
 * arch/risc-v/src/erbium/erbium_head.S
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

#include <arch/arch.h>
#include <arch/irq.h>

#include "chip.h"
#include "riscv_internal.h"
#include "riscv_macros.S"

/****************************************************************************
 * Public Symbols
 ****************************************************************************/

  .section .text.__start, "ax", @progbits
  .global __start

__start:
  la   t0, __erbium_trap_vec
  csrw mtvec, t0
  j    __erbium_entry

  .section .text.erbium_entry, "ax", @progbits
  .global __erbium_entry

__erbium_entry:
  csrr a0, mhartid

  /* Load the number of CPUs that the kernel supports */

#ifdef CONFIG_SMP
  li   t1, CONFIG_SMP_NCPUS
#else
  li   t1, 1
#endif

  /* If a0 (mhartid) >= t1 (the number of CPUs), stop here */

  blt  a0, t1, 1f
  csrw CSR_MIE, zero
  wfi

1:
  /* Set stack pointer to the idle thread stack */

  riscv_set_inital_sp ERBIUM_IDLESTACK_BASE, SMP_STACK_SIZE, a0
  csrw CSR_MIE, zero

  jal  x1, erbium_start

  .global _init
  .global _fini

_init:
_fini:
  ret

  .section .text.exceptions, "ax", @progbits
  .global __erbium_trap_vec

__erbium_trap_vec:
  j    exception_common
  nop
