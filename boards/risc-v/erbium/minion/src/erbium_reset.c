/****************************************************************************
 * boards/risc-v/erbium/minion/src/erbium_reset.c
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

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/boardctl.h>

#include <nuttx/board.h>

#include "hardware/erbium_sysreg.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static inline uint32_t erbium_getreg32(uintptr_t addr)
{
  return *(volatile uint32_t *)addr;
}

static inline void erbium_putreg32(uint32_t value, uintptr_t addr)
{
  *(volatile uint32_t *)addr = value;
}

#ifdef CONFIG_BOARDCTL_RESET
int board_reset(int status)
{
  (void)status;

  erbium_putreg32(ERBIUM_SYSREG_SOFT_RESET_SYSTEM, ERBIUM_SYSREG_SOFT_RESET);

  for (; ; )
    {
      __asm__ volatile("wfi");
    }

  return -EIO;
}
#endif

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
int board_reset_cause(FAR struct boardioc_reset_cause_s *cause)
{
  uint32_t reset_cause;

  if (cause == NULL)
    {
      return -EINVAL;
    }

  reset_cause = erbium_getreg32(ERBIUM_SYSREG_RESET_CAUSE);
  cause->flag = 0;

  if ((reset_cause & ERBIUM_SYSREG_RESET_CAUSE_POR) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_SYS_CHIPPOR;
    }
  else if ((reset_cause & ERBIUM_SYSREG_RESET_CAUSE_WATCHDOG_TIMEOUT) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_SYS_RWDT;
    }
  else if ((reset_cause & ERBIUM_SYSREG_RESET_CAUSE_BROWNOUT) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_SYS_BOR;
    }
  else if ((reset_cause & ERBIUM_SYSREG_RESET_CAUSE_HRESETN) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_CPU_SOFT;
    }
  else if ((reset_cause & (ERBIUM_SYSREG_RESET_CAUSE_SOFTRESET |
                          ERBIUM_SYSREG_RESET_CAUSE_SYSRESET_REQ)) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_CORE_SOFT;
      cause->flag = BOARDIOC_SOFTRESETCAUSE_USER_REBOOT;
    }
  else
    {
      cause->cause = BOARDIOC_RESETCAUSE_NONE;
    }

  return 0;
}
#endif
