# Seamless Epoch Transition

Since **Epoch 102**, Qubic has introduced **Seamless Epoch Transition**. This feature eliminates the need for computor operators to manually stop and update their nodes every Wednesday. Consequently, this reduces network downtime and simplifies operations.

---

## Before vs After

Previously:

![image](https://github.com/qubic/core/assets/39078779/e0f4835d-5bb1-48cc-8da8-9e1eeaca3e82)

Now:

![image](https://github.com/qubic/core/assets/39078779/62c849f9-96c1-45c8-a922-2f4106167579)


---

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

---


## Updating Your Node

Operators are now free to deploy updates at their convenience. However:

- Updates must be deployed **mid-epoch or earlier** to be included in the next epoch.
- Updated nodes should initially run in **AUX mode** and will automatically sync with the rest of the network.
- Once syncing is complete, you can safely switch to **MAIN mode** to participate in consensus.

⚠️ *Until the "hyper-sync" feature is available, updates applied too late in an epoch may not take effect in time.*


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
