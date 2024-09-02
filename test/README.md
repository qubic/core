### Checkmark for successful testing network
✅ All nodes ticking without being stuck. In rare instances of weak connections, nodes may require manual intervention (F9) to re-issue votes. Forcing an empty tick (F5) is discouraged as it disrupts normal operation.

#### Common reasons for being stuck:
- Digest Mismatches (`spectrum`, `universe`, `ResourceTesting`):
  - Buggy smart contract execution affecting the `spectrum` digest.
  - Incorrect asset transfers impacting the `universe` digest.
  - Miscalculated solutions leading to mismatch in the `ResourceTesting` digest.
- Weak connections: May necessitate manual intervention (F9) to re-establish communication.
- Node hangups: In rare cases, the node may require manual restarting.

✅ No freeze and crash during ticking, `beginEpoch`, `endEpoch`.

✅ New files with correct names for the new epoch are generated after `endEpoch` event.

✅ No thread gets crashed 

#### Seamless epoch transition:

✅ Fresh nodes can sync from scratch with new blockchain files.

✅ Third-party services should be able to get all logs generated during the previous epoch.

✅ Old solutions are reset and stop broadcasting them to new epoch.

✅ The new mining seed is retrieved from the updated `spectrum`.

✅ New computors can participate in voting and "pass" salted digests, even when syncing from scratch. This scenario is tested by:
- After endEpoch event, 226 computors are replaced (451 old computors remain), and they will run on a fresh node.
- Simulating a lack of votes from 51 "old" computors (400 out of 451 participated).
- 226 computors spin up the node. We expect the network ticking despite the absence of these 51 "old" computors.

✅ The network continues functioning smoothly and can do another seamless epoch transition.

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
