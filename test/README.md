### Testing checklist
Importance Levels:
- **High**: the networks and node can't function properly, need to fix asap or we should not release.

- **Medium**: Funds are cryptographically secured but extra serivces don't work as expected (moneyFlew, state save/load features). These services can be turned off temporarily.

- **Low**: Not really harmful, eg: display error. Can be fixed later


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
- [High] Files that are generated at the end of each epoch must be matched between nodes (universe, spectrum, contractXXXX, `.futureComputor` part of `system`)
- [High] No stuck for more than 10 mins (not misalignment)
- [Medium] Successfully epoch transition. After the transition, fresh node can join with the EFI file with new tick and epoch.
- [Low] All displayed stats are reset after epoch transition


Extra services:
- [High] The tx add on (moneyflew)
- [Medium] The node is able to save ticks data and reload when reseting.
- [Medium] All loggings work as expected. Including: qu transfer, share transfer, contract message, burning loggings


Solution scoring system:
- [High] No misalignment on resourceTestingDigest
- [High] Submit 30 valid solutions from different addresses, node shows +30 solutions, deposited coins are returned
- [High] Submit 30 invalid solutions from different addresses, node doesn't show anything, M burns logs appear
- [High] Submit 4 valid solutions with 2 invalid solutions from the same address, node shows +4 solutions, 4 deposits are returned, 2 get burned
- [High] Mining seed changes between mining&idle phases.
- [Medium] Solution processing time is under 1000ms (only on BM avx512)
- [Medium] Can set mining threshold remotely
- [Medium] Repeatedly submitting 30 valid solutions, expect the "Cache hit number" increases

Smart contract processor:
- [High] No misalignment on computerDigest/spectrumDigest/universeDigest
- [High] Good stack memory usage (can only know by pressing F2 for now)

Request processors and networking:
- [High] All threads are healthy.
- [Medium] Success rate of connection to the node: >90% Good. >70% OK. Acceptable >50%. Potentially bug <50%



### Score test

The Score Test will compare the generated results with the ground truth files located in test/data.

#### Grouth truth files
The ground truth files will be read by the score test, typically consist of two files:
1. **samples_xxx.csv** contains the input data.
    - Each column is hex presentation of mining seeds, public keys and nonces.
    - Each row is a sample
2. **scores_xxx.csv** the score running on the sample file
    - Column presents score setting
    - Row will corresponds to a row in the sample file. For example, the 10th row in scores_xxx.csv is the result of the data in the 10th row of the sample file.

#### Reading grouth truth files

The ground truth files are in CSV format and can be read using various applications such as OpenOffice, MS Excel, and Google Sheets.

For reference on how to read these files, please see **core/test/utils.h**.

#### Generate grouth truth files
Ground truth files can be generated using the tools available in the **core/tools** directory. You can pass the -h argument to get detailed instructions.

For example,

- Generate a sample file *samples_1234.csv* with 32 samples and run the reference score computation, saving the result into *score_1234.csv*
```
score_test_generator.exe -m generator -s samples_1234.csv -n 32 -o score_1234.csv
```

- Use an existing sample file *samples_1234.csv*, run the reference score computation, and save the result into *score_1234.csv*
```
score_test_generator.exe -m generator -s samples_1234.csv -o score_1234.csv
```
