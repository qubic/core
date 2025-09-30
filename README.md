# Qubic Core Lite

[![Build](https://github.com/hackerby888/qubic-core-lite/actions/workflows/efi-build-develop.yml/badge.svg?branch=main)](https://github.com/hackerby888/qubic-core-lite/actions/workflows/efi-build-develop.yml)

The lite version of Qubic Core that can run directly on the OS without a UEFI environment.

## Supporting Networks

- [x] Mainnet (Beta)
- [x] Local Testnet

## Prerequisites

### Local Testnet

To run a qubic **local testnet** node, you need the following spec:

- **16GB** RAM.

> **No initial files are needed** in this version (eg. spectrum, universe, contract,...)

### Mainnet

To run a qubic **mainnet** node, you need the following spec:

- High frequency CPU with AVX2/AVX512 support (recommend VCPU AMD 7950x @ 8theads)
- 1Gb/s synchronous internet connection
- **32GB** RAM.
- **500GB** fast SSD disk.

> **Initial files are needed** in this version (eg. spectrum, universe, contract,...)

## Build Config

### Local Testnet

Do nothing — the default settings are already configured for local testnet single-node mode.

**Local Testnet Multiple Nodes**

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

### Mainnet

In `qubic.cpp`

**1.** comment out `#define TESTNET`

**2.** uncomment out `#define USE_SWAP`

```cpp
// #define TESTNET // COMMENT this line if you want to compile for mainnet

// this option enables using disk as RAM to reduce hardware requirement for qubic core node
// it is highly recommended to enable this option if you want to run a full mainnet node on SSD
// UNCOMMENT this line to enable it
#define USE_SWAP
```

**3.** add public peers from https://app.qubic.li/network/live to `knownPublicPeers`

## Build

### Windows

- Open .sln file in project root folder in Visual Studio
- Change build config to Release -> Right click at Qubic project -> Build

### Linux

Detailed instruction can be found here: [Linux Build Tutorial](./README_CLANG.md)

## Node State

### Local Testnet

- 676 seeds in `broadcastedComputorSeeds` and `customSeeds` each has 10B Qubic.

### Mainnet

Current mainnet state

## Ticking

Press **F12** to switch to **MAIN** mode to make the network start ticking (processing transactions).

## Tips

- **For Local Testnet:** Default `PORT` is **31841**, you can change it in `qubic.cpp`
- **For Local Testnet:** An epoch will have `TESTNET_EPOCH_DURATION` (**3000**) ticks by default, you can change it in `public_settings.h`
- You can deploy your own RPC server to core lite - [how to](https://qubic-sc-docs.pages.dev/rpc/setup-rpc)
- Change `TICK_STORAGE_AUTOSAVE_MODE` in `private_settings.h` to `1` to enable **Snapshot** mode (your node will start from latest saved snapshot state when crash/restart instead of from scratch)

## FAQs

- **My node stop ticking after restart, why?**
Delete the **system** file at your current working folder, it may make your node start with wrong state.

## Supporting Platform

- [x] Windows
- [x] Linux

## Donate The Project

QUBIC Wallet: QPROLAPACSPVBDQADRXXKRGZMXADUAEXXJIQNWPGWFUFUAITRXMHVODDLGBK
