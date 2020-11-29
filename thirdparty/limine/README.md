# Limine

### What is Limine?

Limine is an advanced x86/x86_64 BIOS Bootloader which supports *modern* PC features
such as Long Mode, 5-level paging, and SMP (multicore), to name a few.

### Limine's boot menu

![Reference screenshot](/screenshot.png?raw=true "Reference screenshot")

[Photo by Nishant Aneja from Pexels](https://www.pexels.com/photo/close-up-photo-of-waterdrops-on-glass-2527248/)

### Supported boot protocols
* Linux
* stivale and stivale2 (Limine's native boot protocols, see STIVALE{,2}.md for details)
* Chainloading

### Supported filesystems
* ext2/3/4
* echfs
* FAT32

### Supported partitioning schemes
* MBR
* GPT

## Warning about using `unstable`

Please refrain from using the `unstable` branch of this repository directly, unless
you have a *very* good reason to.
The `unstable` branch is unstable, and non-backwards compatible changes are made to it
routinely.

Use instead a [release](https://github.com/limine-bootloader/limine/releases).

One can clone a release directly using
```bash
git clone https://github.com/limine-bootloader/limine.git --branch=v0.5.7
```
(replace `v0.5.7` with the chosen release)

Also note that the documentation contained in `unstable` does not reflect the
documentation for the specific releases, and one should refer to the releases'
documentation instead, contained in their files.

## Building

### Building the bootloader
Building the bootloader is not necessary as a prebuilt copy is shipped in this
repository (`limine.bin`).

Should one want to build the bootloader to make sure the shipped copy is authentic,
to develop, to debug, or any other reason, it is necessary to first build the
set of tools that the bootloader needs in order to be built.

This can be accomplished by running:
```bash
make toolchain
```
*The above step may take a while*

After that is done, the bootloader itself can be built with:
```bash
make
```

A newly generated `limine.bin` image should now be present in the root of the repo.

This newly built image should match 1:1 (aka, same checksum) with the one shipped
with the respective commit.

### Compiling `limine-install`
To build the `limine-install` program, simply run `make limine-install` in the root
of the repo.

## How to use

### MBR
In order to install Limine on a MBR device (which can just be a raw image file),
run the `limine-install` as such:

```bash
limine-install <bootloader image> <path to device/image>
```

Where `<bootloader image>` is the path to a `limine.bin` file.

### GPT
If using a GPT formatted device, it will be necessary to create an extra partition
(of at least 32K in size) to store stage 2 code. Then it will be necessary to tell
`limine-install` where this partition is located by specifying the start sector
number (in decimal).

```bash
fdisk <device>    # Create bootloader partition using your favourite method
limine-install <bootloader image> <path to device/image> <start sector of boot partition> <sector size>
```

The `<sector size>` argument is optional. Use it to specify the sector size in bytes
if it is not Limine's expected default of 512 bytes.

### Configuration
Then make sure the device/image contains at least 1 partition formatted in
a supported filesystem containing a `/limine.cfg` or `/boot/limine.cfg` file
and the kernel/modules one wants to load.

An example `limine.cfg` file can be found in `test/limine.cfg`.

More info on the format of `limine.cfg` can be found in `CONFIG.md`.

### Example
For example, to create an empty image file of 64MiB in size, 1 echfs partition
on the image spanning the whole device, format it, copy the relevant files over,
and install Limine, one can do:

```bash
dd if=/dev/zero bs=1M count=0 seek=64 of=test.img
parted -s test.img mklabel msdos
parted -s test.img mkpart primary 1 100%
parted -s test.img set 1 boot on # Workaround for buggy BIOSes

echfs-utils -m -p0 test.img quick-format 32768
echfs-utils -m -p0 test.img import path/to/limine.cfg limine.cfg
echfs-utils -m -p0 test.img import path/to/kernel.elf kernel.elf
echfs-utils -m -p0 test.img import <path to file> <path in image>
...
limine-install test.img
```

One can get `echfs-utils` by installing https://github.com/qword-os/echfs.

## Acknowledgments
Limine uses a stripped-down version of [tinf](https://github.com/jibsen/tinf).

## Discord server
We have a [Discord server](https://discord.gg/QEeZMz4) if you need support, info, or
you just want to hang out with us.
