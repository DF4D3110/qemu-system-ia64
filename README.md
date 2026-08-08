# qemu-system-ia64

Experimental QEMU full-system emulation target for IA-64/Itanium guests.

> [!IMPORTANT]
> This codebase is written using LLMs. Do not send pull requests related to this project to the QEMU upstream.

## Contents

- [Quick start](#quick-start)
- [Windows x86_64 build](#windows-x86_64-build)
- [Emulated platform](#emulated-platform)
- [Windows XPDM display driver](#windows-xpdm-display-driver)
- [Machine profiles](#machine-profiles)
- [CPU models](#cpu-models)
- [Build configurations](#build-configurations)
- [Runtime configuration](#runtime-configuration)
- [EFI firmware and shell](#efi-firmware-and-shell)
- [Tests](#tests)
- [Status](#status)
- [Legal disclaimer](#legal-disclaimer)

## Quick start

### Available releases ![Build status](https://github.com/syunnPC/qemu-system-ia64/actions/workflows/build.yml/badge.svg)

The latest binaries (for Windows/Linux x86-64 and firmware) are available on 
[Actions](https://github.com/syunnPC/qemu-system-ia64/actions/workflows/build.yml) and [Releases](https://github.com/syunnPC/qemu-system-ia64/releases).

### Prerequisite

The firmware build requires an IA-64 ELF cross toolchain named `ia64-linux-gnu-*` in `PATH`.

### Build

Configure and build the IA-64 target with GTK:

```sh
./configure --enable-gtk
ninja -C build qemu-system-ia64 roms/ia64-firmware/ia64-firmware.bin
```

### Boot installation media

```sh
./build/qemu-system-ia64 \
  -machine ia64-vpc \
  -bios ./build/roms/ia64-firmware/ia64-firmware.bin \
  -drive file=/path/to/guest-media.iso,media=cdrom,format=raw,readonly=on \
  -display gtk
```

## Windows x86_64 build

The following procedure cross-compiles the IA-64 emulator for 64-bit Windows
on Debian or Ubuntu, including under WSL. Run every command from the root of
this source tree unless the command explicitly changes directory.

> [!WARNING]
> The Windows build has received less testing than the Linux build and may
> contain Windows-specific bugs.

### Install host tools

On Debian or Ubuntu:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
  build-essential \
  bison \
  ca-certificates \
  curl \
  flex \
  g++-mingw-w64-x86-64 \
  gcc-mingw-w64-x86-64 \
  git \
  meson \
  ninja-build \
  pkg-config \
  python3 \
  python3-venv \
  zstd
```

The build below includes the EFI firmware and therefore requires the IA-64
toolchain described in [Prerequisite](#prerequisite).

### Fetch the pinned Windows dependencies

The helper downloads the pinned MSYS2 MinGW64 dependencies, verifies their
SHA-256 checksums, and prints the extracted sysroot path:

```sh
WIN_SYSROOT="$(./scripts/fetch-win64-deps.sh)"
```

### Configure and build

```sh
HOST_PKG_CONFIG="$(command -v pkg-config)"
mkdir -p build-win64

(
  cd build-win64
  PKG_CONFIG="$HOST_PKG_CONFIG" \
  PKG_CONFIG_LIBDIR="$WIN_SYSROOT/mingw64/lib/pkgconfig" \
  PKG_CONFIG_SYSROOT_DIR="$WIN_SYSROOT" \
  ../configure \
    --cross-prefix=x86_64-w64-mingw32- \
    --host-cc=gcc \
    --python=/usr/bin/python3 \
    --target-list=ia64-softmmu \
    --enable-system \
    --enable-tcg \
    --enable-pixman \
    --enable-fdt=internal \
    --enable-sdl \
    --enable-slirp \
    --enable-vnc \
    --disable-docs \
    --disable-werror
)

ninja -C build-win64 -j"$(nproc)" \
  qemu-system-ia64.exe \
  qemu-system-ia64w.exe \
  roms/ia64-firmware/ia64-firmware.bin
```

### Create a relocatable Windows directory

The executables dynamically link to the pinned MinGW libraries. Copy the
runtime DLLs and IA-64 data files beside the executables before moving them to
Windows:

```sh
WIN_SYSROOT="$(./scripts/fetch-win64-deps.sh)"
rm -rf build-win64-dist
WIN_DIST="$PWD/build-win64-dist"
WINPTHREAD_DLL="$(
  x86_64-w64-mingw32-gcc -print-file-name=libwinpthread-1.dll
)"

mkdir -p "$WIN_DIST/share/keymaps" "$WIN_DIST/licenses"

install -m 0755 \
  build-win64/qemu-system-ia64.exe \
  build-win64/qemu-system-ia64w.exe \
  "$WIN_DIST/"

for dll in \
  SDL2.dll \
  libglib-2.0-0.dll \
  libiconv-2.dll \
  libintl-8.dll \
  libpcre2-8-0.dll \
  libpixman-1-0.dll \
  libslirp-0.dll \
  zlib1.dll; do
  install -m 0755 "$WIN_SYSROOT/mingw64/bin/$dll" "$WIN_DIST/$dll"
done
install -m 0755 "$WINPTHREAD_DLL" "$WIN_DIST/libwinpthread-1.dll"

install -m 0644 \
  build-win64/roms/ia64-firmware/ia64-firmware.bin \
  "$WIN_DIST/share/ia64-firmware.bin"
install -m 0644 \
  pc-bios/efi-e1000.rom \
  pc-bios/vgabios-ati.bin \
  pc-bios/vgabios-stdvga.bin \
  "$WIN_DIST/share/"
install -m 0644 pc-bios/keymaps/* "$WIN_DIST/share/keymaps/"

install -m 0644 COPYING COPYING.LIB LICENSE README.md "$WIN_DIST/"
cp -R "$WIN_SYSROOT/mingw64/share/licenses/." "$WIN_DIST/licenses/"

x86_64-w64-mingw32-strip \
  "$WIN_DIST/qemu-system-ia64.exe" \
  "$WIN_DIST/qemu-system-ia64w.exe"
```

### Run on Windows

Copy or extract `build-win64-dist` on Windows. Open Command Prompt in that
directory, then run:

```bat
qemu-system-ia64.exe ^
  -machine ia64-vpc ^
  -bios share\ia64-firmware.bin ^
  -drive file="C:\path\to\guest-media.iso",media=cdrom,format=raw,readonly=on ^
  -display sdl ^
  -nic user,model=e1000
```

Use `qemu-system-ia64w.exe` for the same SDL interface without a separate
console window.

## Emulated platform

The default machine is `ia64-vpc`. It models an Itanium 2 based virtual PC intended for firmware, boot-loader, and operating-system bring-up.

### Defaults

| Component | Default | Available options and notes |
| --- | --- | --- |
| Machine | `ia64-vpc` | Backward-compatible alias of `itanium2-vpc`; `itanium-vpc` is available for first-generation compatibility |
| CPU | `montecito` | `merced` and `madison` are also available |
| vCPUs | 1 | Configurable from 1 to 64; use `-accel tcg,thread=single` for one vCPU and `thread=multi` for 2 to 64 vCPUs |
| RAM | 2 GiB | Override with the standard QEMU `-m` option |
| Firmware | Project-owned IA-64 EFI firmware | Built from source under `roms/ia64-firmware/` |
| Graphics | ATI-compatible PCI graphics | Standard VGA is available with `-vga std` |
| Input | PS/2 | Setting `i8042=off` automatically attaches a USB keyboard and absolute USB tablet |
| Network | e1000, 82540EM-compatible | User-mode and TAP backends are supported; EFI network boot is not currently provided |

### Emulated facilities

- TCG translation for all CPU models, including PAL/SAL helpers, the register stack engine, TLB/VHPT paths, and architectural floating-point state
- EFI boot and runtime services, an interactive pre-boot shell, PE/COFF and EBC image loading, decompression, filesystems, graphics, storage, USB/input, and debug-support protocols
- Local SAPIC, I/O SAPIC, ACPI platform tables, RTC, watchdog, persistent NVRAM, and serial/debug ports
- A PCI root bus with LSI53C895A SCSI boot storage, ICH9 AHCI, e1000 Ethernet, OHCI/UHCI USB, and optional CMD646 IDE/ATAPI

## Windows XPDM display driver

An IA-64 XPDM driver for the emulated ATI VGA device is available for Windows
XP through Windows Server 2008 R2 in the
[qemu-system-ia64-ati-xpdm](https://github.com/syunnPC/qemu-system-ia64-ati-xpdm)
repository.

## Machine profiles

| Profile | Default CPU | Intended use |
| --- | --- | --- |
| `itanium2-vpc` | `montecito` | Standards-oriented profile with early-firmware compatibility workarounds disabled |
| `ia64-vpc` | `montecito` | Backward-compatible alias of `itanium2-vpc` |
| `itanium-vpc` | `merced` | First-generation systems requiring narrowly scoped firmware and SAL compatibility |

Machine and CPU selection are intentionally independent. A `-cpu` override changes processor-generation behavior, but it does not enable or disable the selected machine profile's firmware compatibility flags.

For a first-generation guest:

```sh
./build/qemu-system-ia64 \
  -machine itanium-vpc \
  -bios ./build/roms/ia64-firmware/ia64-firmware.bin \
  -drive file=/path/to/guest-media.iso,media=cdrom,format=raw,readonly=on \
  -display gtk
```

## CPU models

The `ia64-vpc` and `itanium2-vpc` machines use `montecito` by default, while `itanium-vpc` uses `merced`. Select a different model with `-cpu`.

CPU selection changes guest-visible CPUID and PAL information as well as the available instruction set.

### `montecito` — default for Itanium 2 generation machines

- Implements the later 16-byte operations `ld16`, `ld16.acq`, `st16`, `st16.rel`, `cmp8xchg16.acq`, and `cmp8xchg16.rel`.
- Does not provide a hardware IA-32 execution engine. An eligible `br.ia` or `rfi` request to enter IA-32 mode raises a Disabled ISA Transition fault.
- Recognizes `vmsw.0` and `vmsw.1` as virtualization instructions. Because the emulator does not provide an IA-64 virtual-machine environment, they produce the architecturally appropriate Privileged Operation or Virtualization fault instead of executing a mode switch.

### `madison`

- Provides the hardware IA-32 execution environment.
- Does not provide the later 16-byte operations or virtualization instructions; those encodings raise an Illegal Operation fault.

### `merced` - default for the Itanium (1st generation) machine

- Provides its native IA-32 execution environment.
- Does not provide later long-branch, 16-byte atomic, or virtualization facilities.

`-cpu help` lists exactly `merced`, `madison`, `montecito`, `itanium`, and `itanium2`. The `itanium` name aliases `merced`, and `itanium2` aliases `montecito`.

## Build configurations

### Performance-testing build

For guest-performance measurements, use an IA-64-only build without compiler hardening passes that add substantial overhead to TCG helper calls:

```sh
./configure --target-list=ia64-softmmu \
  --enable-lto \
  --disable-qom-cast-debug \
  --disable-stack-protector \
  --extra-cflags='-fno-stack-protector -fzero-call-used-regs=skip -ftrivial-auto-var-init=uninitialized' \
  -Doptimization=2 \
  --enable-gtk
ninja -C build qemu-system-ia64 roms/ia64-firmware/ia64-firmware.bin
```

This configuration is intended for performance testing. Keep a separate debug build for development and correctness testing.

### Profile-guided optimization

Use a dedicated build directory and the same compiler for both stages.

Generate profiles:

```sh
mkdir build-ia64-pgo
cd build-ia64-pgo
../configure --target-list=ia64-softmmu \
  --enable-lto \
  --disable-qom-cast-debug \
  --disable-stack-protector \
  --extra-cflags='-fno-stack-protector -fzero-call-used-regs=skip -ftrivial-auto-var-init=uninitialized' \
  -Doptimization=2 \
  -Db_pgo=generate
ninja qemu-system-ia64 roms/ia64-firmware/ia64-firmware.bin
```

Run representative guest workloads with the instrumented binary. The training set should cover boot, integer, floating-point, SIMD, MMU-intensive, and RSE-intensive code paths.

Consume the generated profiles:

```sh
./pyvenv/bin/meson configure -Db_pgo=use -Dwerror=false
ninja qemu-system-ia64 roms/ia64-firmware/ia64-firmware.bin
```

The use stage can legitimately report missing profiles for auxiliary targets that
were not part of the training workload. Disabling warnings as errors allows those
targets to use their normal optimization while trained QEMU code consumes its
profiles.

Profiles are tied to the exact build tree, compiler, and source revision. Do not copy stale profile data between builds.

### x86-64-v3 comparison build

On hosts that all support the x86-64-v3 ISA level, add `--x86-version=3` to a separate comparison build. This changes the minimum host ISA for QEMU and its C helpers; TCG-generated guest code still selects host vector features at runtime.

## Runtime configuration

### Multiprocessing, MTTCG, memory, and USB input

The following example uses four vCPUs, MTTCG, 8 GiB of RAM, persistent NVRAM, and USB input without the PS/2 controller:

```sh
./build/qemu-system-ia64 \
  -machine ia64-vpc,i8042=off,nvram=/path/to/guest.nvram \
  -bios ./build/roms/ia64-firmware/ia64-firmware.bin \
  -drive file=/path/to/guest-media.iso,media=cdrom,format=raw,readonly=on \
  -accel tcg,thread=multi \
  -smp 4 \
  -m 8G \
  -display gtk
```

For a one-vCPU guest, select `-accel tcg,thread=single`. MTTCG cannot
parallelize guest execution with only one vCPU and adds synchronization
overhead. Use `-accel tcg,thread=multi` with two to four vCPUs so that guest
CPUs can execute in parallel.

When `i8042=off` is used, the machine automatically attaches a USB keyboard and absolute USB tablet; `-usb` is not required.

### Graphics

Omitting `-vga` selects the default ATI-compatible display. This is recommended for graphical guests. Use `-vga std` only when standard VGA compatibility is specifically needed.

The IA-64 VBE bridge takes its preferred resolution from properties of the
VGA device, rather than from machine properties. Specify `-vga ati` together
with the ATI VGA globals. For example, to expose 3840x2160 modes and a matching
DDC EDID with 32 MiB of video memory:

```sh
./build/qemu-system-ia64 \
  -machine ia64-vpc \
  -bios ./build/roms/ia64-firmware/ia64-firmware.bin \
  -vga ati \
  -global ati-vga.xres=3840 \
  -global ati-vga.yres=2160 \
  -global ati-vga.vgamem_mb=32 \
  -display gtk
```

`xres` and `yres` must be specified together. They select the preferred
resolution and, unless `xmax` and `ymax` are also supplied, the largest
resolution advertised through INT 10h VBE. Increase `vgamem_mb` when the
preferred 32-bpp mode does not fit; the IA-64 fixed PCI layout supports up to
64 MiB and 5120x2880x32. The default remains 1280x1024 with 16 MiB.

The host monitor resolution is not detected automatically. Explicit VGA
properties therefore give the same guest-visible modes with GTK, SDL, VNC,
and headless display backends.

### Networking

An e1000, 82540EM-compatible PCI network controller is attached by default.

When QEMU is built with libslirp, connect it to user-mode networking with:

```sh
-nic user,model=e1000
```

For a host TAP interface:

```sh
-nic tap,model=e1000,ifname=tap0,script=no,downscript=no
```

Use `-nic none` to omit the controller. EFI network boot is not currently provided; the controller is available to the guest operating system.

### Serial and debug output

Use `-serial stdio` to view serial output.

The `-debug-port` option publishes the guest debug transport described by the ACPI DBGP table. For example:

```sh
-debug-port tcp::4444,server=on,wait=on,nodelay=on
```

### EFI variable persistence

EFI variables are persistent by default. Machines load and save a 64 KiB file named `nvram` in the directory containing the firmware selected by `-bios`.

Use a separate file for each virtual machine:

```sh
-machine ia64-vpc,nvram=/path/to/guest.nvram
```

For volatile EFI variables:

```sh
-machine ia64-vpc,nvram=none
```

Relative paths are resolved from QEMU's current working directory.

## EFI firmware and shell

At each startup, the firmware waits three seconds for F2, F12, or Delete before continuing normal boot. Any of these keys opens the embedded EFI shell on both the graphical and serial consoles.

The shell can inspect the machine and its filesystems, launch EFI applications, select a boot target, update the boot order, and set the real-time clock. Example commands:

```text
info
map
ls fs0:\EFI\BOOT
run fs0:\EFI\BOOT\TOOL.EFI argument
boot
boot Boot0001
boot fs0:
bootorder Boot0001 Boot0000
bootnext Boot0001
date 2026-07-17
time 12:34:56
exit
```

`boot fsN:` launches `\EFI\BOOT\BOOTIA64.EFI` from that filesystem. `bootnext` is consumed by the next automatic boot attempt.

Boot order, next-boot selection, and clock changes survive a reset when the machine has NVRAM backing. With `nvram=none`, they remain valid only for the current process.

### Boot an installed EFI system

Attach an installed EFI system with an ordinary disk drive:

```sh
-drive file=/path/to/guest-disk.qcow2,format=qcow2
```

The firmware supports persistent EFI boot entries, including short-form hard-drive device paths, and can boot supported loaders from FAT partitions.

### Screenshots

<table align="center">
  <tr>
    <td width="50%">
      <img
        width="100%"
        alt="Microsoft Windows Server 2008 R2"
        src="https://github.com/user-attachments/assets/ff3563ff-7fb2-4245-bc42-6ec86ed51ce6"
      />
    </td>
    <td width="50%">
      <img
        width="100%"
        alt="Microsoft Windows Codename Longhorn build 4051"
        src="https://github.com/user-attachments/assets/bab22228-ff1e-421b-bc79-c314850c2cab"
      />
    </td>
  </tr>
  <tr>
    <td width="50%">
      <img
        width="100%"
        alt="Microsoft Windows Whistler Advanced Server 64-bit Edition"
        src="https://github.com/user-attachments/assets/3c2bf20f-51eb-4ef3-b560-7dc75a01f6ac"
      />
    </td>
    <td width="50%">
      <img
        width="100%"
        alt="Debian 7.11.0 with GUI"
        src="https://github.com/user-attachments/assets/d1e5cdaa-64d6-4f91-9215-277423e268a2"
      />
    </td>
  </tr>
</table>

## Tests

Use the build-local Meson selected by QEMU's configure process. A host `meson` of another version may be unable to read `build/meson-private/build.dat`, and plain `meson test` from the source directory is not valid because the Meson build data lives under `build`.

Run the behavior-oriented IA-64 unit, TCG, machine, and functional tests after building:

```sh
build/pyvenv/bin/meson test -C build --suite ia64 --print-errorlogs
build/pyvenv/bin/meson test -C build --suite qtest-ia64 --print-errorlogs
build/pyvenv/bin/meson test -C build --suite func-ia64 --print-errorlogs
build/pyvenv/bin/meson test -C build --suite func-ia64-thorough --print-errorlogs
```

### Test coverage

- The TCG registry currently contains about 1,200 architectural microprograms divided between core, memory/NaT, floating-point, RSE, MMU, interruption, and PAL groups.
- Machine tests cover platform wiring and display behavior.
- The functional suite builds project-owned EFI applications and boots them from deterministic FAT, GPT, MBR, El Torito, and UDF media.
- The functional suite also exercises the firmware shell through PS/2, USB, and serial input, including direct application execution and NVRAM persistence across restarts.

See [`docs/devel/testing/ia64.rst`](docs/devel/testing/ia64.rst) for focused runs and test-authoring rules.

## Status

The current implementation boots several IA-64 operating-system installers and supports up to four guest processors.

Instruction coverage, privileged behavior, floating-point corner cases, and device compatibility still require validation against the IA-64, EFI, SAL, and ACPI specifications.

## Legal disclaimer

### Third-party materials

This repository does not include third-party operating-system images, disk images, firmware images, machine ROM dumps, proprietary firmware blobs, or operating-system binaries.

Guest operating-system images, firmware, installation media, and other third-party materials must be supplied by users under their own applicable licenses.

### Project independence

This project is an independent experimental QEMU IA-64 system-emulation project. It is not affiliated with, endorsed by, sponsored by, or supported by Intel, HPE, the QEMU Project, or Microsoft Corporation.

### Licensing and trademarks

QEMU is used as the upstream base for this fork. QEMU as a whole is licensed under the GNU General Public License, version 2. See the license files in this repository for details.

Microsoft and Windows are trademarks of the Microsoft group of companies.

The screenshot of Windows Operating System is used with permission from Microsoft.

All other product names, project names, company names, and trademarks are the property of their respective owners.
