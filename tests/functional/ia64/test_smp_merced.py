#!/usr/bin/env python3
"""Minimal Merced AP boot rendezvous without translation-cache pressure."""

# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path

from qemu_test import QemuSystemTest

from ia64.console import Ia64FirmwareTest
from ia64.efi_build import app_path
from ia64.media import make_fat_disk


SMP_MERCED_CASES = {
    "sal-ap-wake", "merced-rendezvous", "merced-rendezvous-return",
}


class Ia64SmpMerced(Ia64FirmwareTest):
    def test_merced_ap_rendezvous(self):
        disk = Path(self.scratch_file("smp-merced.img"))
        nvram = self.make_nvram("smp-merced.nvram")
        make_fat_disk(disk, app_path("smp-merced"))
        vm = self.launch_ia64(
            media=disk, smp=4, memory="512M",
            machine_options=f"firmware-console=serial,nvram={nvram}",
            extra_args=("-cpu", "merced",
                        "-accel", "tcg,thread=multi"))
        result = self.wait_ia64_suite(
            vm, "smp-merced", SMP_MERCED_CASES, timeout=60.0)
        self.assertSetEqual(set(result.cases), SMP_MERCED_CASES)


if __name__ == "__main__":
    QemuSystemTest.main()
