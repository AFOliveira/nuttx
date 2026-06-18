/****************************************************************************
 * arch/risc-v/src/erbium/hardware/erbium_sysreg.h
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

#ifndef __ARCH_RISCV_SRC_ERBIUM_HARDWARE_ERBIUM_SYSREG_H
#define __ARCH_RISCV_SRC_ERBIUM_HARDWARE_ERBIUM_SYSREG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/erbium_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ERBIUM_SYSREG_VERSION          (ERBIUM_SYSREG_BASE + 0x00)
#define ERBIUM_SYSREG_SYSTEM_CONFIG    (ERBIUM_SYSREG_BASE + 0x08)
#define ERBIUM_SYSREG_WATCHDOG_COUNT   (ERBIUM_SYSREG_BASE + 0x10)
#define ERBIUM_SYSREG_WATCHDOG         (ERBIUM_SYSREG_BASE + 0x18)
#define ERBIUM_SYSREG_SYS_INTERRUPT    (ERBIUM_SYSREG_BASE + 0x20)
#define ERBIUM_SYSREG_SOFT_RESET       (ERBIUM_SYSREG_BASE + 0x28)
#define ERBIUM_SYSREG_RESET_CAUSE      (ERBIUM_SYSREG_BASE + 0x30)

#define ERBIUM_SYSREG_VERSION_RESPIN_MASK      0x000000fful
#define ERBIUM_SYSREG_VERSION_VARIATION_MASK   0x0000ff00ul
#define ERBIUM_SYSREG_VERSION_VARIATION_SHIFT  8
#define ERBIUM_SYSREG_VERSION_CHIPID_MASK      0xffff0000ul
#define ERBIUM_SYSREG_VERSION_CHIPID_SHIFT     16
#define ERBIUM_SYSREG_VERSION_CHIPID_ERBIUM    0xeb68ul

#define ERBIUM_SYSREG_SYSTEM_CONFIG_SYS_INTERRUPT_ENABLE  (1u << 0)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_MRAM_STARTUP_BYPASS   (1u << 1)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_WDOG_DISABLE          (1u << 2)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_I2C_ENABLE            (1u << 3)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_SPI_ENABLE            (1u << 4)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_QSPI_ENABLE           (1u << 5)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_UART_ENABLE           (1u << 6)
#define ERBIUM_SYSREG_SYSTEM_CONFIG_OSC_OUT_ENABLE        (1u << 7)

#define ERBIUM_SYSREG_WATCHDOG_KICK       (1u << 7)

#define ERBIUM_SYSREG_SOFT_RESET_SYSTEM   (1u << 0)
#define ERBIUM_SYSREG_SOFT_RESET_CPU_WARM (1u << 1)
#define ERBIUM_SYSREG_SOFT_RESET_MRAM_N   (1u << 2)
#define ERBIUM_SYSREG_SOFT_RESET_CPU      (1u << 3)

#define ERBIUM_SYSREG_RESET_CAUSE_POR              (1u << 0)
#define ERBIUM_SYSREG_RESET_CAUSE_WATCHDOG_TIMEOUT (1u << 1)
#define ERBIUM_SYSREG_RESET_CAUSE_SYSRESET_REQ     (1u << 2)
#define ERBIUM_SYSREG_RESET_CAUSE_BROWNOUT         (1u << 3)
#define ERBIUM_SYSREG_RESET_CAUSE_SOFTRESET        (1u << 4)
#define ERBIUM_SYSREG_RESET_CAUSE_HRESETN          (1u << 5)

#endif /* __ARCH_RISCV_SRC_ERBIUM_HARDWARE_ERBIUM_SYSREG_H */
