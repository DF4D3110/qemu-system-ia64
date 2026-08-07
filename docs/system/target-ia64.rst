.. _system-target-ia64:

IA-64 System emulator
=====================

QEMU's experimental IA-64 system target provides virtual PC machines for Itanium guest bring-up.
It uses TCG and a project-owned EFI firmware; KVM and IA-64 user-mode emulation are not provided.

Machine profile
---------------

``itanium2-vpc`` is the standards-oriented profile and defaults to a Montecito-class CPU.
The existing ``ia64-vpc`` name is an alias for this profile.
``itanium-vpc`` defaults to a Merced-class CPU and enables the isolated firmware behavior needed by early loaders, including their memory layout and SAL conventions.
The selected machine controls the firmware ABI only: either profile accepts any supported CPU model through ``-cpu``.

Both profiles default to 2 GiB of RAM and otherwise provide the same virtual hardware: an ATI-compatible PCI display, an e1000 network adapter, LSI53C895A SCSI storage, ICH9 AHCI, OHCI/UHCI USB, and PS/2 input.
One to four CPUs are supported.
Use ``-accel tcg,thread=single`` for one CPU and select MTTCG with ``-accel tcg,thread=multi`` for two to four CPUs.
The machines also provide local SAPIC and I/O SAPIC interrupt controllers, ACPI tables, RTC, watchdog, NVRAM, serial I/O, and the firmware debug port.

The primary CPU generation names are ``merced``, ``madison``, and ``montecito``.
``itanium`` and ``itanium2`` are CPU model aliases.

Building and running
--------------------

The firmware requires an ``ia64-linux-gnu-*`` ELF cross toolchain in ``PATH``::

  ./configure --target-list=ia64-softmmu
  ninja -C build qemu-system-ia64 \
      roms/ia64-firmware/ia64-firmware.bin

For a serial-only emulator without the built-in display, network, input, storage, or USB device groups, configure with ``--with-devices-ia64=minimal``.
The default build includes all of those groups.

A typical optical-media boot is::

  build/qemu-system-ia64 \
      -machine ia64-vpc,nvram=/path/to/guest.nvram \
      -bios build/roms/ia64-firmware/ia64-firmware.bin \
      -drive file=/path/to/media.iso,media=cdrom,format=raw,readonly=on \
      -display gtk

Machine properties
------------------

``i8042=on|off``
  Enable or disable the PS/2 controller.
  The default is ``on``.
  With ``i8042=off``, the machine supplies a USB keyboard and absolute tablet when default devices are enabled.

``firmware-ide-dma=on|off``
  Select firmware CMD646 bus-master DMA.  The default is ``on``.

``firmware-console=serial|vga``
  Select the primary console advertised by HCDP.  The default is ``vga``.

``nvram=auto|none|PATH``
  Select the 64 KiB EFI variable store.
  ``auto`` places ``nvram`` beside the selected firmware, ``none`` keeps variables process-local, and a path names an explicit backing file.
  Use a separate file for each VM.

``alat=zero|full``
  Select the ALAT model.
  ``zero`` is the default.
  The full model is not currently SMP-safe and is rejected with more than one vCPU.

VBE display modes
-----------------

The IA-64 INT 10h implementation builds its VBE mode list and DDC EDID from
the selected VGA device's existing ``xres``, ``yres``, ``xmax``, ``ymax``,
and ``vgamem_mb`` properties.  ``xres`` and ``yres`` select the preferred
resolution and are also the VBE mode-list limit when ``xmax`` and ``ymax``
are not specified.  An exact 16-, 24-, and 32-bpp preferred mode is added
when the resolution is not in the built-in mode table.  The default remains
1280x1024 with 16 MiB of video memory.

Use ``-vga`` together with ``-global`` because a VGA global property by
itself suppresses creation of the default display device.  For example, an
ATI-compatible 3840x2160 display needs 32 MiB of video memory::

  -vga ati \
  -global ati-vga.xres=3840 \
  -global ati-vga.yres=2160 \
  -global ati-vga.vgamem_mb=32

The equivalent standard VGA configuration is::

  -vga std \
  -global VGA.xres=3840 \
  -global VGA.yres=2160 \
  -global VGA.vgamem_mb=32

``xmax`` and ``ymax`` may be used to make the enumerated limit larger than
the preferred resolution.  They must be specified together, and the
preferred resolution must not exceed them.  Modes which do not fit in the
configured video memory are omitted, while failure to fit the preferred
32-bpp mode is a startup error.

The IA-64 fixed PCI layout places the framebuffer at 0xc4000000 and VGA MMIO
at 0xc8000000, limiting the framebuffer aperture and ``vgamem_mb`` to 64 MiB.
This accommodates 5120x2880x32, while 7680x4320x32 requires a different PCI
layout and is not supported.  The common Bochs VBE implementation also
requires the horizontal preferred resolution to be a multiple of eight;
for example, use 1360x768 instead of 1366x768.

The host monitor size is not detected automatically.  Explicit VGA
properties give consistent behavior with GTK, VNC, SPICE, and headless
display backends.

Firmware and boot media
-----------------------

The firmware implements EFI boot and runtime services, PAL/SAL entry points, PE/COFF and EBC loading, FAT and UDF filesystems, El Torito boot, storage and input protocols, graphics output, and a pre-boot shell.
Press F2, F12, or Delete during the startup window to enter the shell.
Installed systems can be attached with an ordinary ``-drive`` option.

``-debug-port`` connects the transport described by the ACPI DBGP table to a QEMU character backend.
For example::

  -debug-port tcp::4444,server=on,wait=on,nodelay=on

EFI network boot is not implemented; the default e1000 device is available to the operating system after boot.

Validation
----------

The project-owned tests cover the CPU architecture, machine wiring, firmware services, storage layouts, input, and SMP behavior.
See :doc:`../devel/testing/ia64` for the exact commands and test-authoring rules.
