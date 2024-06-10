# qubic - node
Qubic Node Source Code - this repository contains the source code of a full qubic node.

> MAIN (current version running qubic) <br>
> [![EFIBuild](https://github.com/qubic/core/actions/workflows/efi-build-develop.yml/badge.svg?branch=main)](https://github.com/qubic/core/actions/workflows/efi-build-develop.yml)
> <br>
> DEVELOP (current version we're working on) <br>
> [![EFIBuild](https://github.com/qubic/core/actions/workflows/efi-build-develop.yml/badge.svg?branch=develop)](https://github.com/qubic/core/actions/workflows/efi-build-develop.yml)

## Prerequisites
To run a qubic node, you need the following spec:
- Bare Metal Server/Computer with at least 8 Cores (high CPU frequency with AVX2 support). AVX-512 support is recommended; check supported CPUs [here](https://www.epey.co.uk/cpu/e/YTozOntpOjUwOTc7YToxOntpOjA7czo2OiI0Mjg1NzUiO31pOjUwOTk7YToyOntpOjA7czoxOiI4IjtpOjE7czoyOiIzMiI7fWk6NTA4ODthOjY6e2k6MDtzOjY6IjQ1NjE1MCI7aToxO3M6NzoiMjM4Nzg2MSI7aToyO3M6NzoiMTkzOTE5OSI7aTozO3M6NzoiMTUwMjg4MyI7aTo0O3M6NzoiMjA2Nzk5MyI7aTo1O3M6NzoiMjE5OTc1OSI7fX1fYjowOw==/)
- At least 500GB of RAM
- 1Gb/s synchronous internet connection
- A USB Stick or SSD/HD attached to the Computer (via NVMe M.2 or USB)
- UEFI Bios 

> You will need the current `spectrum, universe, and contract` files to be able to start Qubic. The latest files can be found in our #computor-operator channel on the Qubic Discord server: https://discord.gg/qubic (inquire there for the files).

### Prepare your USB Stick/SSD/HD
1. Your Qubic Boot device should be formatted as FAT32 with the label QUBIC.
```bash
# sample command in linux
mkfs.fat -F 32 -n QUBIC /dev/sda
```
If you have a disk and want to use partitions, this is possible too. use `gdisk`.
```bash
gdisk /dev/sda

# remove all existing partition with d command
# add the qubic partition with n command
# it is recommended to use <1TB of partition size; the start sector and the end sector can be specified with size. eg: 200G.
# set the type of partition to ef00

echo -e "o\nY\nd\nn\n\n\n+200G\n\nt\n\nef00\nw\nY" | gdisk /dev/sda

```
2. We recommend to have the following structure on the disk.
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
- contract000.XXX => must be the current contract #0 file. XXX must be replaced with the current epoch. (e.g. `contract000.068`)
- contract001.XXX => must be the current contract #1 file. XXX must be replaced with the current epoch. (e.g. `contract001.068`). Data from Qx.
- contract002.XXX => must be the current contract #2 file. XXX must be replaced with the current epoch. (e.g. `contract002.068`). Data from Quottery.
- Other contract files with the same format as above. For now, we have 4 contracts.
- universe.XXX => must be the current universe file. XXX must be replaced with the current epoch. (e.g `universe.068`)
- spectrum.XXX => must be the current spectrum file. XXX must be replaced with the current epoch. (e.g `spectrum.068`)
- system => to start from scratch, use an empty file. (e.g. `touch system`)
- Bootx64.efi => boot loader
- startup.nsh => UEFI start script
- Qubic.efi => the compiled qubic node code (efi executable)

The content of your `startup.nsh` could look like this:
```batch
timezone -s 00:00
ifconfig -s eth0 dhcp
fs0:
cd efi
cd boot
Qubic.efi
```

- `timezone -s 00:00` sets the timezone to UTC
- `ifconfig -s eth0 dhcp` tells the efi to get an ip address from DHCP; if you want to set a fixed ip you can use `ifconfig -s eth0 static <IP> <SUBNETMASK> <GATEWAY>`
- `fs0:` changes to drive 0
- `Qubic.efi` starts qubic

> If you have multiple hard drives, the `fs0:` must changed to meet your environment.

> To make it easier, you can copy & paste our prepared initial disk from https://github.com/qubic/core/blob/main/doc/qubic-initial-disk.zip

> If you have multiple network interfaces, you may disconnect these before starting qubic.

### Prepare your Server
To run Qubic on your server you need the following:
- UEFI Bios
- Enabled Network Stack in Bios
- Your USB Stick/SSD should be the boot device

## General Process of deploying a node
1. Find knownPublicPeers public peers (e.g. from: https://app.qubic.li/network/live)
2. Set the needed parameters inside src/private_settings.h (https://github.com/qubic/core/blob/main/src/private_settings.h)
3. Compile Source to EFI
4. Start EFI Application on your Computer


## How to run a Listening Node
To run a "listen-only" node, just add IP addresses of 3-4 known public peers to the code (including your own IP).
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
2. Add your Operator Identity.
The Operator Identity is used to identify the Operator. The Operator can send Commands to your Node.
```c++
#define OPERATOR "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
```
3. Add static IPs of known public peers (can be obtained from https://app.qubic.li/network/live).
Ideally, add at least 4 including your own IP.
```c++
static const unsigned char knownPublicPeers[][4] = {
  {12,13,14,12}
};
```

## License
The Anti-Military License. See [LICENSE.md](LICENSE.md).

## Installation and Configuration
Please refer to https://docs.qubic.world

## Limited Support
We cannot support you in any case. You are welcome to provide updates, bug fixes, or other code changes by pull requests, see [here](doc/contributing.md).

## More Documentation
- [How to contribute](doc/contributing.md)
- [Qubic protocol](doc/protocol.md)
