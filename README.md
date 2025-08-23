# Qubic Core Lite

The lite version of Qubic Core that can run directly on the OS without a UEFI environment.

## Build

> Lite Executable <br>
> [![Build](https://github.com/hackerby888/qubic-core-lite/actions/workflows/efi-build-develop.yml/badge.svg?branch=main)](https://github.com/hackerby888/qubic-core-lite/actions/workflows/efi-build-develop.yml)
> <br>
> The executable is located in `QubicLiteBuildOutput.zip/x64/Release/Qubic.exe`

## Prerequisites

To run a qubic node, you need the following spec:

- 16GB RAM.

> No initial files are needed in this version (eg. spectrum, universe, contract,...)

## Node State

- 676 seeds in `broadcastedComputorSeeds` each has 1M Qubic.

## Ticking

Press **F12** to switch to **MAIN** mode to make the network start ticking (processing transactions).

## Single Node

Do nothing — the default settings are already configured for single-node mode.

## Multiple Nodes

In `private_settings.h`, split the 676 seeds in `broadcastedComputorSeeds` into `computorSeeds` across your nodes (e.g., 300 seeds in node 1, the remaining 376 seeds in node 2):

```c++
static unsigned char computorSeeds[][55 + 1] = {
};
```

> **Warning**
> Do not change the `broadcastedComputorSeeds`.

After that add your node's ip addresses to `knownPublicPeers`

```c++
static const unsigned char knownPublicPeers[][4] = {
    {127, 0, 0, 1}, // DONT REMOVE THIS
    // Add more node ips here
};
```

## Tips

- Default `PORT` is **31841**, you can change it in `qubic.cpp`
- An epoch will have `TESTNET_EPOCH_DURATION` (**3000**) ticks by default, you can change it in `public_settings.h`

## Supporting Platform

- [x] Windows
- [ ] Linux (Soon...)

## Donate The Project

QUBIC Wallet: QPROLAPACSPVBDQADRXXKRGZMXADUAEXXJIQNWPGWFUFUAITRXMHVODDLGBK

