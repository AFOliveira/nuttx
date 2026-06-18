AIFoundry Erbium Minion
=======================

This is a NuttX port for the AIFoundry Erbium Minion target.  It is an
RV64IMFC machine-mode target intended for the ET-platform Erbium simulator.
The same low-level port uses the documented Erbium system-register and PLIC
MMIO blocks used by silicon.

Supported features
------------------

  - RV64IMFC core configuration
  - Machine timer
  - UART0 console
  - PLIC-backed external interrupts
  - System soft reset and reset-cause reporting

Build
-----

  $ make distclean
  $ ./tools/configure.sh -l minion:nsh
  $ make -j

The OS test configuration can be built with:

  $ make distclean
  $ ./tools/configure.sh -l minion:ostest
  $ make -j

Run
---

The ET-platform simulator reserves the first 512 bytes of MRAM, so the NuttX
image is linked at 0x40000200 and should be started there:

  $ erbium_emu -elf_load nuttx -reset_pc 0x40000200

Use an ET-platform simulator build with Erbium PLIC PMA support; older
simulator binaries trap on PLIC MMIO accesses at 0xA0000000.  The documented
Erbium PLIC register map exposes external interrupt sources 1 through 6, with
UART0 on source 3.

Known limitations
-----------------

The simulator configuration keeps the idle loop runnable instead of executing
WFI.  The ostest configuration disables the FPU subtest because the simulator
currently traps on single-precision divide instructions.
