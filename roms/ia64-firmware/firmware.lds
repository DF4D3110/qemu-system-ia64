/* SPDX-License-Identifier: GPL-2.0-or-later */
/* IA-64 EFI firmware linker script. Linked at the machine-model load address. */

ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .text.boot : {
        KEEP(*(.text._start))
        KEEP(*(.text.pal_proc_entry))
        KEEP(*(.text.start_after_pal))
        KEEP(*(.text.start_secondary))
        KEEP(*(.text.fw_pal_halt_light))
    }

    . = ALIGN(0x2000);
    __runtime_code_start = .;

    .text : {
        *(.text*)
        *(.plt*)
    }

    . = ALIGN(0x2000);
    __runtime_data_start = .;

    . = ALIGN(16);

    .rodata : {
        *(.rodata*)
    }

    . = ALIGN(16);

    .opd : {
        *(.opd)
    }

    . = ALIGN(16);

    .IA_64.unwind_info : {
        *(.IA_64.unwind_info*)
    }

    . = ALIGN(16);

    .IA_64.unwind : {
        *(.IA_64.unwind*)
    }

    .data : {
        *(.data*)
    }

    . = ALIGN(16);

    .got : {
        __gp_start = .;
        *(.got*)
    }

    . = ALIGN(16);

    .bss : {
        _bss_start = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    }

    __firmware_payload_end = .;
    ASSERT(__firmware_payload_end < 0x300000,
           "firmware payload must remain below 3 MiB")

    . = 0x300000;
    _end = .;
}
