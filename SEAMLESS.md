# Seamless Epoch Transition

Since **Epoch 102**, Qubic has introduced **Seamless Epoch Transition**. This feature eliminates the need for computor operators to manually stop and update their nodes every Wednesday. Consequently, this reduces network downtime and simplifies operations.

## Table of Contents

- [Before vs After](#before-vs-after)
- [How Seamless Transition Works (MAIN/AUX Mode & Index Explained)](#how-seamless-transition-works-mainaux-mode--index-explained)
  - [When Does Ticking Start After the Epoch Transition?](#when-does-ticking-start-after-the-epoch-transition)
- [Updating Your Node](#updating-your-node)
  - [1. Sync Before the Epoch Transition (if updating shortly before)](#1-sync-before-the-epoch-transition-if-updating-shortly-before)
  - [2. Set the Correct START_NETWORK_FROM_SCRATCH Flag](#2-set-the-correct-start_network_from_scratch-flag)
- [Understanding START_NETWORK_FROM_SCRATCH](#understanding-start_network_from_scratch)
- [Version Compatibility and Seamless Updating](#version-compatibility-and-seamless-updating)
- [What to Do in Case of Sync Issues](#what-to-do-in-case-of-sync-issues)
- [Remarks for Core-Developers](#remarks-for-core-developers)


## Before vs After

Previously:

![image](https://github.com/qubic/core/assets/39078779/e0f4835d-5bb1-48cc-8da8-9e1eeaca3e82)

Now:

![image](https://github.com/qubic/core/assets/39078779/62c849f9-96c1-45c8-a922-2f4106167579)


## How Seamless Transition Works (MAIN/AUX Mode & Index Explained)

Seamless epoch transition depends on how your node is configured during the switch between epochs:

| Transition Mode | Description |
|-----------------|-------------|
| `MAIN/MAIN`   | The **same node** continues in MAIN mode into the next epoch. This is the most seamless option — **no restart or update needed**. |
| `AUX/MAIN`    | The node was in AUX mode during the previous epoch and becomes MAIN in the new one. This allows for smooth upgrading or switching nodes across epochs. |
| `MAIN/AUX`    | The node downgrades to AUX mode, usually for backup or updating purposes. |
| `AUX/AUX`     | Passive mode used for ticking without taking part in consensus. |

> ⚠️ **Important:** Only **one node per computor ID** should run in `MAIN` mode at any given time.  
> Running multiple MAIN nodes with the same ID will result in **faulty behavior**.

### When Does Ticking Start After the Epoch Transition?

Ticking in the new epoch begins **only after the arbitrator sends out the new computor list**; a step known as **"getting index"**.

Until the index is received, your node will not tick. Once received, the index includes:

- The list of computors for the new epoch
- Their assigned IDs

At this point, a node in `MAIN` mode will start ticking automatically **once the quorum of 451 votes is reached**.

<br>

## Updating Your Node

You can update your node **at any time**: during an epoch, right before or right after an epoch transition. However, to ensure your node continues ticking correctly, there are two important things to consider:

### 1. Sync Before the Epoch Transition (if updating shortly before)

If you're updating close to the epoch switch (Wednesdays at 12:00 UTC), make sure your node is **fully synced before the transition happens**.

- The network only retains a limited number of ticks from the previous epoch: `#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 100`
- If your node is not in sync before the transition, it may not be able to retrieve the final tick from the previous epoch and fail to join the new one.

⚠️ *Until the "hyper-sync" feature is available, updates applied too late in an epoch may not take effect in time.*

### 2. Set the Correct `START_NETWORK_FROM_SCRATCH` Flag


Regardless of **when** you update, you must check how the **last epoch transition** was performed:

- If the last transition was a **seamless epoch transition**, set:
  `#define START_NETWORK_FROM_SCRATCH 0`
  Your node will fetch the necessary initial tick and timestamp from peers to join the current epoch.

- If the last transition was a **restart from scratch**, set:
  `#define START_NETWORK_FROM_SCRATCH 1`
  Your node will start from a predefined tick (`TICK` in `public_settings.h`) and the default timestamp (Wednesday 12:00:00.000 UTC).

> Good to know: You can have multiple seamless transitions in a row, but only if the **previous transition was also seamless** and the **software versions remain compatible**.

<br>

## Understanding `START_NETWORK_FROM_SCRATCH`

The `START_NETWORK_FROM_SCRATCH` flag in `public_settings.h` determines how your node handles the beginning of a new epoch:

### `#define START_NETWORK_FROM_SCRATCH 0` *(default for seamless transition)*

Use this when your node is updating or joining a running network that has transitioned **seamlessly** into the new epoch (i.e. *not* restarted from scratch). The node will try to **fetch the initial tick** (`prevDigests`, `timestamp`) from peers.

This works if:
- You have a valid database from the previous epoch.
- Other nodes have already transitioned and are ticking.

> ⚠️ If your node starts from scratch when the rest of the network used seamless transition, it will **fail to sync** without the correct tick information.

### `#define START_NETWORK_FROM_SCRATCH 1` *(only for full restarts)*

Use this only if:
- You are starting a new network from scratch (e.g. testnet or full restart).
- All 676 computor IDs are restarting and coordinated to use this flag.

Behavior: Sets the very first tick of the epoch to the `#define TICK` value in `public_settings.h` and the very first tick timestamp to `12:00:00.000UTC` of the corresponding Wednesday.

## Version Compatibility and Seamless Updating

Qubic releases follow a versioning scheme: `vX.Y.Z`

- If two versions have the **same `X` and `Y`**, they are **protocol-compatible**:
  - Nodes can participate in seamless transitions together
  - Example: `v1.2.0` → `v1.2.4` is compatible and seamless

- If **`X` or `Y` differ**, the versions are **not compatible**:
  - A **coordinated network restart** is required
  - This restart occurs at **Wednesday 12:00 UTC**
  - All 676 computor IDs must be updated and started with:
    `#define START_NETWORK_FROM_SCRATCH 1`

> 🔁 You can chain multiple seamless transitions in a row, **as long as each previous transition was seamless** and all versions share the same `X.Y`.

## What to Do in Case of Sync Issues
- **F7 function**: In rare cases, a node may fail to transition due to mismatched data or missing consensus votes. Pressing `F7` will **force** the node to switch to the new epoch.
- **Getting the initial tick manually**:  
  You can query any ticking node using the CLI:

  ```bash
  ./qubic-cli -nodeip 1.2.3.4 -getcurrenttick
  ```

## Remarks for Core-Developers
- Since we have "Seamless Epoch Transition", a critical modification to the protocol necessitates the implementation of conditional logic within the codebase, for example:
```
// Currently it's epoch 102, this code is supposed to change the way processing data from epoch 103
if (system.epoch >= 103){
  // do something new
} else {
  // Keep processing like the old way
}
```
- For large changes in protocol, it's still best to restart the whole network with the flag `#define START_NETWORK_FROM_SCRATCH 1`
