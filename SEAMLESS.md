Since Epoch 102, Qubic has introduced a feature known as "Seamless Epoch Transition". This feature eliminates the need for computer operators to manually halt and update their nodes each Wednesday. Consequently, this reduces network downtime every week.

Previously:

![image](https://github.com/qubic/core/assets/39078779/e0f4835d-5bb1-48cc-8da8-9e1eeaca3e82)

Now:

![image](https://github.com/qubic/core/assets/39078779/62c849f9-96c1-45c8-a922-2f4106167579)


#### How to deal with updates:
Henceforth, for any updates, operators can deploy the code at their discretion. However, to ensure timely network synchronization and implementation by the subsequent epoch, deployment of new updates is restricted to mid-epoch or earlier (until the "hyper-sync" feature is implemented). These deployed computors will synchronize with the network while operating in AUX mode. Once synchronization is complete, operators may then switch the nodes to MAIN mode.

#### Introducing new features and code
- `#define START_NETWORK_FROM_SCRATCH 1` in `public_settings.h`:
  - If this flag is 1, it indicates that the whole network (all 676 IDs) will start from scratch and agree that the very first tick timestamp will be set at (2022-04-13 Wed 12:00:00.000UTC).
  - If this flag is 0, the node will try to fetch the initial tick info (`prevDigests` and `timestamp`) of the new epoch from other nodes, because the timestamp of the first tick of a new epoch (when doing seamless transition) is always different from (2022-04-13 Wed 12:00:00.000UTC), and `prevDigests` in the initial tick is the digests of the last tick of the last epoch (which are hard to obtained by a fresh nodes).
- `F7` function: in a rare case, slow nodes are unable to do the transition because of mismatched data and lack of votes to reach the consensus. In that case, operators can press `F7` to **FORCE** the node to switch to the new epoch and continue ticking.
- Generally, the initial `TICK` value will be provided on the main branch after switching epoch. You may independently query a ticking node using the `qubic-cli` tool. The command for this purpose is `./qubic-cli -nodeip 1.2.3.4 -getcurrenttick`. This command will return both the correct initial tick and epoch values. Then you can update those numbers (tick, epoch) in `public_settings.h`, set the flag `#define START_NETWORK_FROM_SCRATCH 0` and start the node.
#### Guide for developing
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
