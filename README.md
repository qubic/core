# Qubic Core Lite

[![Build](https://github.com/hackerby888/qubic-core-lite/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hackerby888/qubic-core-lite/actions/workflows/ci.yml)

The lite version of Qubic Core that can run directly on the OS without a UEFI environment.

## Menu

- [Qubic Core Lite](#qubic-core-lite)
  - [Supporting Networks](#supporting-networks)
  - [Parameters](#parameters)
  - [Prerequisites](#prerequisites)
    - [Local Testnet](#local-testnet)
    - [Mainnet](#mainnet)
  - [Build Config](#build-config)
    - [Local Testnet](#local-testnet-1)
    - [Mainnet](#mainnet-1)
  - [Build](#build)
    - [Windows](#windows)
    - [Linux](#linux)
  - [Node State](#node-state)
  - [Ticking](#ticking)
  - [RPC](#rpc)
  - [Tips](#tips)
  - [FAQs](#faqs)
  - [Supporting Platform](#supporting-platform)
  - [Donate The Project](#donate-the-project)

## Supporting Networks

- [x] Mainnet (Beta)
- [x] Local Testnet

## Parameters

- **Security tick** : `./Qubic --security-tick 32`
> The security tick temporarily skips verifying your **node’s contract state (computer digest)** against the quorum. Verification is performed only every `--security-tick` interval.

- **Ticking delay (local testnet)**: `./Qubic --ticking-delay 1000`
> If your local testnet ticking too fast, you can slow it down by `--ticking-delay` ms.

- **Peers**: `./Qubic --peers 1.2.3.4,8.8.8.8`
> You can add more peers using command line

## Prerequisites

### Local Testnet

To run a qubic **local testnet** node, you need the following spec:

- **16GB** RAM.

> **No initial files are needed** in this version (eg. spectrum, universe, contract,...)

### Mainnet

To run a qubic **mainnet** node, you need the following spec:

- High frequency CPU with AVX2/AVX512 support (recommend VCPU AMD 7950x @ 8theads)
- 1Gb/s synchronous internet connection
- **64GB** RAM.
- **500GB** fast SSD disk.

> **Initial files are needed** in this version (eg. spectrum, universe, contract,...)

## Build Config

### Local Testnet

**Local Testnet Single Node**

In `qubic.cpp`

**1.** Uncomment `// #define TESTNET`

```cpp
// #define TESTNET // COMMENT this line if you want to compile for mainnet

// this option enables using disk as RAM to reduce hardware requirement for qubic core node
// it is highly recommended to enable this option if you want to run a full mainnet node on SSD
// UNCOMMENT this line to enable it
#define USE_SWAP
```

**2.** Build

**Local Testnet Multiple Nodes**

Afer single node steps please do:

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

Make sure you have commented `#define TESTNET`

**1.** Add public peers from https://app.qubic.li/network/live to `knownPublicPeers` in `private_settings.h` or add via command line `--peers`

**2.** Prepare the epoch files (blockchain state).

They should be named and structured as follows:

```
./contract0000.XXX
./contract0001.XXX
./contract0002.XXX
./contract0003.XXX
./contract0004.XXX
./contract0005.XXX
./contract0006.XXX
./contract0007.XXX
./contract0008.XXX
./contract0009.XXX
./contract0010.XXX
./contract0011.XXX
./contract00xx.XXX
./spectrum.XXX
./universe.XXX
```

Place all of these files in the same directory where you plan to launch the `Qubic` binary.

**3.** Build

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

## RPC

Qubic Core Lite provides a built-in RPC API that enables developers to interact directly with a Lite node with official RPC style, removing the need for an original complex RPC layer.

### Status

- **RPC Live (OK):** `http://localhost:41841/live/v1`
- **RPC Stats (OK):** `http://localhost:41841/`
- **RPC Query V2 (OK):** `http://localhost:41841/query/v1`
- **RPC Archiver V2:** *Deprecated (not implemented)*

### Documentation

https://qubic.github.io/integration/Partners/swagger/qubic-rpc-doc.html?urls.primaryName=Qubic%20RPC%20Live%20Tree  
> Remember to select the appropriate API definition for each endpoint.

## Tips

- **For Local Testnet:** Default `PORT` is **31841**, you can change it in `qubic.cpp`
- **For Local Testnet:** If you want to fund your custom wallet (seed), you can add these into `customSeeds` in `private_settings.h`
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
