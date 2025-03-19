# Qubic Core Testing

## Testnet operation

New features and releases of the Qubic Core must be tested in the target environment (EFI bare metal) in a testnet with multiple nodes.
In this task, developer are supported by the test team, which is led by kavatak and can be initially contacted via the [public test-net channel on Discord](https://discord.com/channels/768887649540243497/1182262429174992937).

### Testing checklist
Importance Levels:
- **High**: The network and nodes can't function properly, need to fix asap or we should not release.

- **Medium**: Funds are cryptographically secured but extra services don't work as expected (moneyFlew, state save/load features). These services can be turned off temporarily.

- **Low**: Not really harmful, eg: display error. Can be fixed later.


5 main components inside qubic core:
```
[Consensus algo (tick processors)]
[Solution scoring system]
[Request processors] + [Networking]
[Smart contract processor]
[Extra services]
```

Consensus algo (tick processors):
- [High] No misalignment
- [High] Files that are generated at the end of each epoch must be matched between nodes (universe, spectrum, contractXXXX, `.futureComputor` part of `system`). Use `md5sum` to check if the files match. To compare the list of future computers, use the analyzeSystem tool.
- [High] No stuck for more than 10 mins (not misalignment)
- [Medium] Successful epoch transition. After the transition, fresh node can join with the EFI file with new tick and epoch (ensure `#define START_NETWORK_FROM_SCRATCH 0` is set).
- [Low] All displayed stats are reset after epoch transition.


Extra services:
- [High] The tx add on (moneyflew). Set `#define ADDON_TX_STATUS_REQUEST 1` to enable it and verify transaction processing.
- [Medium] The node is able to save tick data and reload when resetting. To do this, set `#define TICK_STORAGE_AUTOSAVE_MODE 1` during compilation. Wait for the state to be saved, or press F8. Restart the node.
- [Medium] All logging works as expected. Including: qu transfer, share transfer, contract message, burning loggings. Use the [qlogging tool](https://github.com/qubic/qlogging) to check the log.


Solution scoring system:
- [High] No misalignment on resourceTestingDigest
- [High] Submit 30 valid solutions from different addresses, node shows +30 solutions, deposited coins are returned. Press F2 to see X/Y/Z/W solutions (X: Number of recorded solutions, Y: Number of published solutions, Z: Number of obsolete solutions, W: Total solutions received by the node).
- [High] Submit 30 invalid solutions from different addresses, node doesn't show anything, M burn logs appear.
- [High] Submit 4 valid solutions with 2 invalid solutions from the same address, node shows +4 solutions, 4 deposits are returned, 2 get burned.
- [High] Mining seed changes between mining & idle phases. Use [qubic-cli](https://github.com/qubic/qubic-cli) and the -getsysteminfo command to verify that the RandomMiningSeed value is updated properly between the mining and idle phases.
- [High] Submit 451+ valid solutions for custom mining. Check results using F3 on the node.
- [Medium] Repeatedly submitting 30 valid solutions. Press F2, expect the "Score cache" increases.
- [Medium] Solution processing time is under 1000ms (only on BM avx512).
- [Medium] Can set mining threshold remotely via qubic-cli -setsolutionthreshold.

Smart contract processor:
- [High] No misalignment on computerDigest/spectrumDigest/universeDigest
- [High] Good stack memory usage (can only know by pressing F2 for now)

Request processors and networking:
- [High] All threads are healthy.
- [Medium] Success rate of connection to the node: >90% Good. >70% OK. Acceptable >50%. Potentially bug <50%


## Testing with Test Example SCs

To enable testing of components that cannot be covered by unit tests, the Test Example SCs provide interfaces to run tests in a ticking testnet via qubic-cli's testing commands.
At the moment, only the `TESTEXA` SC is used for this purpose.

### Setup
- Enable the testing SCs in the core code by adding `#define INCLUDE_CONTRACT_TEST_EXAMPLES` before including contract_def.h in qubic.cpp.
- In qubic-cli testUtils.cpp file, make sure that `#define TESTEXA_CONTRACT_INDEX <contract_index>` and `#define TESTEXA_ADDRESS "<contract_address>"` still match the index and address of the `TESTEXA` SC (it will change when new SCs are added).
- Run a test network with the adapted core code.

### Tests to perform via qubic-cli
- [High] Run the command `-testqpifunctionsoutput` with a computor seed. The computor seed is required because other entities have a limit of max. 1 pending tx and the command will send 15 txs for future ticks. The command will wait and print whether the txs were included in the respective ticks. Ideally, they should all be included but sometimes there might be missing ones which did not pass the consensus. Afterwards, the command will check if the output of QPI functions saved in the `TESTEXA` state match the TickData and quorum tick votes.
- [High] The command `-testqpifunctionsoutputpast` will perform the same check as the previous test but only for past ticks (no seed required).
A special case is testing the QPI functions output after loading the state from a snapshot. For this, wait until a snapshot is saved, re-start the node from the snapshot, then run the command quickly. Ideally, the ticks tested by the command should include ticks from before and after loading the snapshot.


## Google Tests

For simplified, automated, and isolated testing of components, we use the "test" project.
It is based on the Google Test framework and runs in your OS, facilitating easy debugging within your dev environment.

### Score test

The Score Test will compare the generated results with the ground-truth files located in test/data.

#### Ground-truth files
The ground-truth files will be read by the score test, typically consist of two files:
1. **samples_xxx.csv** contains the input data.
    - Each column is hex presentation of mining seeds, public keys and nonces.
    - Each row is a sample
2. **scores_xxx.csv** the score running on the sample file
    - Column presents score setting
    - Row will corresponds to a row in the sample file. For example, the 10th row in scores_xxx.csv is the result of the data in the 10th row of the sample file.

#### Reading ground-truth files

The ground-truth files are in CSV format and can be read using various applications such as OpenOffice, MS Excel, and Google Sheets.

For reference on how to read these files, please see **core/test/utils.h**.

#### Generate ground-truth files
Ground-truth files can be generated using the tools available in the **core/tools** directory. You can pass the -h argument to get detailed instructions.

For example,

- Generate a sample file *samples_1234.csv* with 32 samples and run the reference score computation, saving the result into *score_1234.csv*
```
score_test_generator.exe -m generator -s samples_1234.csv -n 32 -o score_1234.csv
```

- Use an existing sample file *samples_1234.csv*, run the reference score computation, and save the result into *score_1234.csv*
```
score_test_generator.exe -m generator -s samples_1234.csv -o score_1234.csv
```
