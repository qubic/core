# qubic - node
Qubic Node Source Code - this repository contains the source code of a full qubic node.

> MAIN (current version running qubic) <br>
> [![EFIBuild](https://github.com/qubic-network/core/actions/workflows/efi-build-develop.yml/badge.svg?branch=main)](https://github.com/qubic-network/core/actions/workflows/efi-build-develop.yml)
> <br>
> DEVELOP (current version we're working on) <br>
> [![EFIBuild](https://github.com/qubic-network/core/actions/workflows/efi-build-develop.yml/badge.svg?branch=develop)](https://github.com/qubic-network/core/actions/workflows/efi-build-develop.yml)

## Prerequisites
To run a qubic node, you need the following parts:
- Bare Metal Server/Computer with at least 8 Cores (high CPU frequency with AVX2 support)
- At least 128GB memory
- 1Gb/s synchronous internet connection
- An USB Stick or SSD/HD attached to the Computer via USB
- An UEFI Bios

> you will need the current `spectrum, universe and contract` files to be able to start qubic. the latest files can be found in our #network discord channel: https://discord.gg/qubic (ask there for the files)

### Prepare your USB Stick/SSD/HD
1. Your Qubic Boot device should be formatted as FAT32 and have the label QUBIC.
```bash
# sample command in linux
mkfs.fat -F 32 -n QUBIC /dev/sda
```
if you have a disk and want to use partitions, this is possible too. use `gdisk`.
```bash
gdisk /dev/sda
# remove all existing partition with d command
# add the qubic partition with n command
# it is recommended to use <1TB of partition size; let be the start sector. end sector can be specified with size. eg: 200G.
# set the type of partition to ef00
```
2. We recommend to have this structure on the disk.
```
/contract.000.XXX
/contract.001.XXX
/contract.002.XXX
/spectrum.XXX
/system
/universe.XXX
/efi/boot
/efi/boot/Bootx64.efi
/efi/boot/startup.nsh
/efi/boot/Qubic.efi
```
- contract.000.XXX => must be the current contract.000 file. XXX must be replaced with current epoch. (e.g `computer.068`)
- contract.001.XXX => must be the current contract.001 file. XXX must be replaced with current epoch. (e.g `computer.068`). Data from Qx.
- contract.002.XXX => must be the current contract.002 file. XXX must be replaced with current epoch. (e.g `computer.068`). Data from Quottery.
- universe.XXX => must be the current universe file. XXX must be replaced with current epoch. (e.g `universe.068`)
- spectrum.XXX => must be the current spectrum file. XXX must be replaced with current epoch. (e.g `spectrum.068`)
- system => to start from scratch, use an empty file. (e.g. `touch system`)
- Bootx64.efi => boot loader
- startup.nsh => UEFI start script
- Qubic.efi => the compiled qubic node code (efi executable)

the content of your `startup.nsh` could look like:
```batch
timezone -s 00:00
ifconfig -s eth0 dhcp
fs0:
cd efi
cd boot
Qubic.efi
```

- `timezone -s 00:00` sets the timezone to utc
- `ifconfig -s eth0 dhcp` tells the efi to get an ip address from dhcp; if you want to set a fixed ip you can use `ifconfig -s eth0 static <IP> <SUBNETMASK> <GATEWAY>`
- `fs0:` changes to drive 0
- `Qubic.efi` starts qubic

> if you have multiple hard drives, the `fs0:` must changed to meet your environment.

> to make it easier, you can copy & paste our prepared initial disk from https://github.com/qubic-li/qubic/blob/main/qubic-initial-disk.zip

> if you have multiple network interfaces you may disconnect these befor starting qubic.

### Prepare your Server
To be able to start Qubic on your server you need.
- UEFI Bios
- Enabled Network Stack in Bios
- Your USB Stick/SSD should be the boot device

## General Process of deploying a node
1. Find knownPublicPeers public peers (e.g. from: https://app.qubic.li/network/live)
2. Set the needed parameters inside qubic.cpp (https://github.com/qubic-li/qubic/blob/main/qubic.cpp)
3. Compile Source to EFI
4. Start EFI Application on your Computer


## How to run a Listening Node
To run a "listen-only" node, just add 3-4 known pulic peers to the code.
```c++
static const unsigned char knownPublicPeers[][4] = {
};
```
Compile with RELEASE.

## How to run a Computor Node
1. Add your Computor Seed(s)
```c++
static unsigned char computorSeeds[][55 + 1] = {
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
};
```
2. Add your Operator Identity
The Operator Identity is used to identify the Operator. The Operator can send Commands to your Node.
```c++
#define OPERATOR "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
```
3. Add 3-4 Known Public Peers (can be obtained from https://app.qubic.li/network/live)
```c++
static const unsigned char knownPublicPeers[][4] = {
  {12,13,14,12}
};
```

## License
The Anti-Military License. See https://github.com/computor-tools/qubic-js

## Installation and Configuration
Please refer to https://docs.qubic.world

## Limited Support
We cannot support you in any case. You are welcome to provide updates, bugfixes or other code changes by pull requests.
