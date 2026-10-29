# Async Score Verification

Mining solutions are verified in the background and settled a fixed number of ticks after the tick that
includes them. Verification no longer runs inside tick processing, so a burst of solutions does not slow
the tick that contains it. This document describes the design, the rules a change must keep, and where
the code is, so that the work can be continued.

That fixed number of ticks is the **settlement window**, written W below
(`SOLUTION_SETTLEMENT_WINDOW_TICKS` in `src/public_settings.h`). Every node of the network uses the same
value; it is a consensus parameter.

## Why

Scoring one solution takes up to about a second, and a tick can carry many solutions. Before async score
verification, every solution of a tick was scored before the tick could be voted, so the slowest
solution held the tick open.

Deferring settlement takes scoring off the tick path: the tick processor only appends records and later
settles results that are already computed, so tick time no longer depends on the slowest solution. The
window does not raise verification throughput; it moves the slow tail out of the tick.

## At a glance

```
tick T                      ticks T+1 .. T+W                   END_TICK of tick T+W
tick processor              solution processors                tick processor

phase 1: record       --->  score in the background      --->  phase 2: settle
(cheap checks, append)      (pure function, any order)         digest fold, tree commit,
                                                               deposit refund, ranking
```

On the last tick of an epoch, phase 2 settles every solution still pending, inside that tick's vote.

### Example

Illustrative numbers: tick T carries 120 solutions, none of them scored in advance, one score takes
1 s, and 12 solution processors score in parallel.

```
Before: scored inside the tick

  tick      T-1    T                                           T+1    T+2
            [--]   [--- 120 scores / 12 processors = 10 s ---] [--]   [--]
                    the vote for T waits for the last score

Now: scored in the background

  tick      T-1    T      T+1    T+2    T+3    T+4    T+5    ...    T+W
            [--]   [--]   [--]   [--]   [--]   [--]   [--]          [--]
  workers          [--- 120 scores on 12 processors, 10 s ---]
                   ^                                                ^
                   T only appends 120 records                       END_TICK of T+W settles them
                                                                    from results already computed
```

Tick T keeps its usual length, and the 10 s of scoring spreads over the following ticks.

The scoring capacity is the same in both: here 12 scores per second. If solutions keep arriving faster
than that, the backlog grows until settlement reaches unscored slots and scores them inline, and those
ticks get longer again. The window absorbs bursts; it does not add capacity.

## Terms

| Term | Meaning |
| --- | --- |
| W | The settlement window, in ticks |
| Kind | A solution transaction type with its own record, score and settle code: ant (bpp9000 colony) and standalone |
| Record | What phase 1 captures from the transaction. Hashed into the window digest and saved. Identical on every node |
| Result | What a worker produces when it scores a record. Node-local and recomputable, never saved |
| Slot | One record plus its result, in the window's arena |
| Settle tick | The tick whose END_TICK settles a solution: T + W, or the epoch's last tick if that comes first |
| Drain | Settling the whole window at once, on the epoch's last tick |

## Lifecycle

Inside one tick:

```
processTick(T), tick processor                            request processors (workers)

BEGIN_TICK     contracts' BEGIN_TICK
transactions   phase 1: record tick T's solutions  --->  window  --->  claim, score, write result
               (append to the window)                                  (any time, oldest first)
anchor         tick T joins the anchor ring (non-empty ticks)
END_TICK       phase 2: settle tick T - W, scoring inline
               whatever no worker finished;
               on the epoch's last tick, drain everything
               contracts' END_TICK
digests        salted spectrum, universe and computer digests
               -> the vote for tick T covers the settlement
```

1. **Phase 1, record (tick processor, tick T).** `processTickTransaction` calls `recordAntSolution` or
   `recordStandaloneSolution`. They run only checks every node computes identically, then
   `SolutionSettlementWindow::append`.
   - Ant: seen filter (`RejectReplay`), the own-computor RECORDED mark, the parent rule
     (`RejectParentNotRegistered`), the anchor (`RejectStale`), then append (`RejectTickOutOfRange` if the
     window refuses).
   - Standalone: a bpp9000 nonce returns at once, with nothing appended or marked (standalone mining is
     disabled for bpp9000); otherwise the duplicate filter, the own-computor RECORDED mark, then append.
   - Nothing is scored, folded or refunded here. The generic transaction code has already debited the
     deposit. A solution rejected at phase 1, or refused by the window, never reaches settlement, so its
     deposit is forfeited.
2. **Score (request processors with `solutionProcessorFlags`).** Each loop iteration of `requestProcessor`
   calls `solutionWorkerStep`, which takes the next unscored slot through one shared cursor and calls the
   kind's score hook: `scoreAntSolution` or `scoreStandaloneSolution`. A score depends only on the slot's
   record, the settled tree, the anchor ring and the epoch's task and seed (rule 3). An unscorable
   solution gets an invalid rating. The ant result carries the rating, the network's hash and the packed
   network; the standalone result carries only the score.
3. **Phase 2, settle (tick processor, END_TICK of the settle tick).** `processTick` calls
   `settlePendingSolutions` before the contracts' END_TICK and before the salted digests are taken, so the
   settle tick's vote covers it. `SolutionSettlementWindow::settleTick` walks each tick's slots in txIndex
   order and calls the kind's settle hook: `settleAntSolution` or `settleStandaloneSolution`.
   - Ant: resolve the parent again (`RejectParentNotRegistered`), check the anchor again (`RejectStale`),
     reject an invalid rating (`RejectNonCanonicalNonce`), fold `rating.digestValue()` and `annHash` into
     `resourceTestingDigest`, `commit()` into the tree, and on Valid or ValidNotStored with a matching
     claimed score, refund the deposit and update the miner ranking.
   - Standalone: fold a valid score; with a matching claim and a good score, refund and rank.
   - The refund is logged under the settle tick, as a transfer from the zero address; the debit was
     logged at the transaction's own tick. `AntSolutionLogMessage`, emitted for every verdict with the
     source and the nonce, lets a log reader join the two.
   - Settlement also runs on empty ticks, so an empty tick can change `resourceTestingDigest` and the
     spectrum.

## Rules a change must keep

Rules 1 to 5 keep every node in agreement: a change that breaks one splits the network. Rules 6 to 8 keep
the results correct and comparable: breaking one gives wrong or misleading results on every node alike.

1. **Only the tick processor changes consensus state.** Phase 1 (the seen flags, the window's records)
   and phase 2 (digest, tree, balances, ranking) both run on it, in transaction order; phase 2 goes tick
   by tick in ascending order, slots in ascending txIndex. Workers write only their own slot's result.
2. **When settlement happens depends only on the tick number and the voted date.** `settlePendingSolutions`
   runs every tick, outside the empty-tick gate, never depending on tick content.
3. **A score never depends on worker timing.** Apart from its own slot's record, a worker reads only the
   settled tree, the anchor ring and the replay cache, which holds results of the same deterministic
   score. It never reads another slot of the window, so a parent still in the window resolves Absent.
4. **A parent must be more than W ticks older than its child** (`isAntParentSettledBefore`). While tick T
   is processed, the tree already holds every committed solution of tick T - W - 1 and earlier, and none
   of a later tick, so every worker resolves such a parent the same way. Phase 1 enforces the rule and
   the scorer restates it.
5. **The anchor is checked at phase 1 and again at settlement.** An anchor only ever leaves the ring, so
   one present at settlement was present whenever a worker read it, and one gone is rejected on every
   node.
6. **Settlement uses the solution's own tick as "now"** (`slot.tick`), never `system.tick`. `system.tick`
   is up to W ticks ahead by then, so every solution would look that much older: freshness, the publish
   tick and the ranking tick would all be off.
7. **Records are zeroed before they are filled.** A record is saved and hashed with its padding, so
   the saved window holds no stack bytes and is the same on every node.
8. **One predicate decides the epoch end.** `isLastTickInEpoch()` (the voted `etalonTick` date) is used
   both for the drain and for the epoch switch, so every solution of an epoch settles under a vote of that
   epoch. A network with another epoch rule changes this function, never one call site alone.
   `endEpoch()` asserts the window is empty.

## Parent resolution

The parent rule (rule 4) over time, for a parent recorded at tick P:

```
tick      P            P + 1 .. P + W                      P + W + 1 ..
parent    recorded     pending in the window;              in the tree
                       settled at END_TICK of P + W
child     -            naming the parent is refused        may name the parent
                       at phase 1
```

`resolveAntParent()` is the only reader of a parent reference. It answers `AntParentReady` (ROOT, or a
solution in the settled tree) or `AntParentAbsent`. Tree entries are never removed within an epoch, so a
parent that is Ready stays Ready. What each caller does with Absent:

| Caller | On Absent |
| --- | --- |
| `scoreAntSolution` | Writes an invalid rating; settlement rejects it the same way on every node |
| `settleAntSolution` | `RejectParentNotRegistered` |
| `queueAntSolution` (message path) | Drops it as `parentUnknown`; the miner sends it again once the parent has settled |
| `publishAntSolutionFor` (pending queue) | Retires the entry as `parentGone` |
| `processRequestAntParentAnn` | Answers `ANT_PARENT_ANN_STATUS_NOT_FOUND` |

`processRequestAntIdentityTree` lists settled solutions only, so every node answers it the same way for a
given identity, apart from how far each has settled.

## The settlement window

`SolutionSettlementWindow` in `src/mining/solution_settlement_window.h`.

Which ticks are pending, at END_TICK of tick T:

```
            settled, in the tree                 |       pending, in the window
   ...   T-W-2     T-W-1     [T-W]               |   T-W+1   ...   T-1     T
                               ^
                               settled now; settledThroughTick becomes T - W
```

During tick T's transactions, tick T - W is still pending as well, so up to W + 1 ticks are in the
window at once. That is why the arena has room for W + 1 ticks.

The arena:

```
             _head                      _cursor                      _tail
               v                           v                            v
    released   | claimed: scored, or being | unclaimed: no scorer yet   | free
               | scored by one worker      |                            |
               |<---------- unsettled slots, in (tick, txIndex) order ->|
               | run of tick a | run of tick b |    ...    | run of tick c |

    Logical index i is stored in _slots[i % capacity].
    _tickIndex[tick & mask] holds { tick, startIdx, count } for each unsettled tick's run.
```

- **Arena.** `(W + 1) * NUMBER_OF_TRANSACTIONS_PER_TICK` slots, allocated once at `init()`. Indices:
  `_head` is the oldest unsettled slot, `_cursor` the next one a worker takes, `_tail` the next free one.
  Appends and settlement both run in (tick, txIndex) order, so each tick's slots form one contiguous run.
- **Tick index.** `_tickIndex`, addressed `tick & (size - 1)`, with size the smallest power of two of at
  least W + 1. It maps a tick to its run. `append()` refuses a tick that would alias a still-unsettled one.
- **Writers.**
  - Tick processor: `append()` writes the record, then the tick index count last; `settleTick()` moves
    `_head`, clears the index count and advances `_settledThroughTick`. When it scores a slot itself
    (inline or in the drain), it writes the result exactly as a worker does.
  - Workers, for the one slot each has claimed:
    1. `claimNextUnscored()` advances `_cursor` and counts the claim, under `_lock`.
    2. The kind's score hook copies the record out, scores it and writes the result into the slot.
    3. `scored` is set with an atomic store after the result, so whoever sees it set reads a complete
       result. Then `_unscored` is decremented atomically.

    After that the worker never touches the slot again, and the slot is reused only once settlement has
    released it. A worker never writes a record field: the record is consensus data, hashed into the
    window digest and saved.
  - Scoring also writes node-local state outside the window, none of which reaches consensus: the replay
    cache (under its own lock), the per-processor scratch networks, and the score engine's buffers
    (under their locks).
- **Claiming.** `claimNextUnscored()` advances `_cursor` under `_lock`, so each slot goes to exactly one
  scorer.
- **Settlement and drain.** How a slot is settled depends on whether it is scored yet; see the next
  section.
- **Release.** A settled slot is released, not scrubbed: a pointer taken earlier reads the same bytes
  until an append reuses the slot. That is what lets `lookup()` run without a lock.
- **Sizes.** A record's kind data is at most `MAX_SOLUTION_RECORD_BYTES` (128), a result at most
  `MAX_SOLUTION_RESULT_BYTES` (576). A slot is 720 bytes: the 140-byte record, the 576-byte result, and
  `scored` with its padding. Static asserts pin each kind's layout.

## What settlement does with each solution

When `settleOneTick()` reaches a tick, it first scores that tick's unclaimed slots, taking them from the
shared cursor alongside the workers until the cursor is past the tick. It then walks the slots in
txIndex order and settles each one. A slot is in one of three states when settlement reaches the tick:

| State when settlement starts | How it is known | What happens | F3 counter |
| --- | --- | --- | --- |
| Scored | `scored` is set | Settled from the stored result | `scored by worker`, or `by drain` on the epoch's last tick |
| Being scored | A worker has claimed it, and `scored` is not set yet | The tick processor waits for that one score, then settles it | `scored by worker`, and `stalled` if the wait happened |
| Not claimed | The cursor has not reached it | Taken from the cursor by the tick processor (`inline`) or by a worker (`scored by worker`), scored with the kind's score hook, then settled | `inline` or `scored by worker` |

- The outcome never depends on which processor scored a slot. Every path calls the kind's one score hook,
  and the settle hook sees the same result.
- A worker writes the result first and `scored` last, so settlement reads a complete result once it sees
  `scored` set.
- An unscorable solution still counts as scored. Its result is an invalid rating, and settlement rejects
  it the same way on every node.
- Settlement only waits on its own tick's slots: it scores the unclaimed ones alongside the workers, so
  a backlog is scored by every solution processor plus the tick processor, then it waits for at most one
  score per slot a worker still holds.

**On the epoch's last tick**, `drainAll()` takes slots from the same cursor as the workers and scores them
on the tick processor (`by drain`), while the workers keep taking and finishing slots too. Once the cursor
is past every slot, it waits until no slot is unscored, then settles every outstanding tick, so by then
every slot is in the "Scored" state.

**After a restart from a snapshot**, every loaded slot is "Not scored" and unclaimed, with the cursor at
the oldest one. Workers score them again from the start of the window, and the replay cache turns most of
these scores into lookups. A slot still unscored when its settle tick comes is scored inline, as above.

## Kinds

`registerKind(inputType, scoreFn, settleFn)` binds a transaction type to its two hooks; the window itself
knows nothing about solution types. `solutionSettlementInit()` registers both kinds, out of at most
`MAX_SOLUTION_KINDS` (4): standalone is input type 2 (`MiningSolutionTransaction`), ant is input type 12
(`AntColonyMiningSolutionTransaction`). The layouts and the parent resolution are in
`src/mining/solution_settlement_kinds.h`.

To add a kind:
1. Define its record and result structs, under the two size limits, with static asserts on their sizes.
2. Write its record function (phase 1 checks, zero the record, append), score hook and settle hook.
3. Register it in `solutionSettlementInit()` and call its record function from `processTickTransaction`.
4. Keep the rules above, in particular rules 1, 3 and 6.

## Two ways a solution arrives

```
 message path (node-local)                            transaction path (consensus)

 miner --message--> processBroadcastMessage
                    queueAntSolution: check, score,
                      warm the replay cache
                    gAntPendingSolutions
                    publishAntSolutionFor: check
                      again, sign with the
                      computor key  --------- tx --->  tick data of tick T
                                                              ^
 miner ----------------- solution transaction ----------------+
                                                              |
                                                              v
                                                       phase 1 at tick T -> window
                                                       settled at END_TICK of T + W
```

- **Transaction.** A solution transaction in tick data goes through phase 1, the window and phase 2, as
  described above.
- **Message.** A miner sends a solution to a computor by message. `processBroadcastMessage` hands it to
  `queueAntSolution` (or, for standalone, to `system.solutions`). It is checked and scored right away, on
  that request processor, because the computor pays the deposit when it publishes: a solution that would
  fail is dropped before it costs anything. A solution that passes goes into `gAntPendingSolutions`.
  `publishAntSolutionFor` later checks it again for the tick it will execute in, signs it with the
  computor's key and broadcasts it as a transaction. From there it follows the transaction path.
- Nothing on the message path is consensus state. For an ant solution, the receipt score lands in the
  replay cache under the same key the published transaction uses, so this node's worker gets a cache hit
  later; other nodes score it in full.

## Save and load

- **When.** The window is saved with the node state: `saveAllNodeStates` calls `saveSolutionSettlement`
  right after it writes the node state, inside the same `invalidateNodeStates` bracket, so a save cut
  short leaves a snapshot the node refuses. `loadAllNodeStates` calls `loadSolutionSettlement`
  after reading the node state and before copying it into the live globals.
- **Records only.** Results are not saved. After a load every slot is unscored and is scored again; the
  replay cache makes that cheap.
- **Pairing.** The header carries `resourceTestingDigestAtSave`. The load compares it with the digest in
  the node state saved beside it, so a window can never load next to a node state from another moment.
- **Checks.** The header carries the epoch and layout guards (`recordBytes`, `windowTicks`,
  `chunkRecords`). Records go back in through `append()`, so a bad file is refused by the live rules, and
  the chained K12 of the records must match the header. Any failure refuses the snapshot, and the node
  falls back to starting from the epoch files.
- **No snapshot.** When no snapshot is loaded, `solutionSettlementBeginEpoch()` starts the window empty at
  `initialTick - 1`, the same state every node has at an epoch start.

### Restarts

- **A restart from files with a new initial tick** starts every node with an empty window. That is
  consistent across nodes, but the solutions pending at the stop never settle and their deposits are not
  refunded. Stopping where the window is empty, as at an epoch start, loses nothing.
- **A restart from a snapshot** reloads the pending solutions unscored. Within the last W ticks of an
  epoch, that leaves more for the last tick's drain to score.

  ```
  save point     top of the tick loop, system.tick = S: processTick(S - 1) has finished,
                 and its END_TICK settled tick S - 1 - W
  window saved   records of ticks S - W .. S - 1, settledThroughTick = S - 1 - W
                 (early in an epoch, settledThroughTick stays initialTick - 1)
  after load     the same records, all unscored, cursor at the oldest
                 processTick(S) runs next; workers score again, mostly replay-cache hits;
                 END_TICK of S settles tick S - W
  ```
- **After any restart**, compare `settled through` and `pending` on F3 with another node before
  this one votes. A window that differs looks healthy until its solutions settle, W ticks later, and then
  splits.

### Files

Async score verification adds two kinds of file to every snapshot, in the snapshot directory `ep<epoch>/`.
`<epoch>` in a file name is the epoch in three digits.

| File | Written | Read | Contents |
| --- | --- | --- | --- |
| `snapshotAsyncScoreHeader.<epoch>` | Every snapshot | Snapshot load | The 32-byte header below |
| `snapshotAsyncScoreRecords.<epoch>.<chunk>` | Every snapshot | Snapshot load | The unsettled records, oldest first, up to 65536 per chunk file (`<chunk>` is 000, 001, ...). Chunk 000 is always written; an empty window writes one zeroed record |

A snapshot therefore has one header file and at least one record file. The window holds at most
(W + 1) * 4096 records, so there are at most (W + 1) / 16 record files, rounded up.

The window header (`SolutionSettlementWindowMeta`, 8 fields of 4 bytes):

| Field | Meaning | Checked at load |
| --- | --- | --- |
| `epoch` | The epoch the window belongs to | Must equal the node's epoch |
| `settledThroughTick` | The last tick already settled | Becomes the window's settled-through tick |
| `recordCount` | Records in the chunk files | At most the arena capacity |
| `windowDigest` | Chained K12 of the records, in order | Must equal the digest of the records loaded |
| `resourceTestingDigestAtSave` | `resourceTestingDigest` when the window was saved | Must equal the one in the saved node state |
| `recordBytes` | Size of one saved record (140) | Must equal this build's |
| `windowTicks` | W | Must equal this build's |
| `chunkRecords` | Records per chunk file (65536) | Must equal this build's |

A saved record (`SettlementRecord`, 140 bytes) is `tick` (4), `txIndex` (4), `inputType` (2), `recordSize` (2)
and 128 bytes of kind data, zero-padded:

| Kind | Kind data | Bytes used |
| --- | --- | --- |
| Ant (`AntSolutionRecordCapture`) | `sourcePublicKey` (32), `nonce` (32), `parentRef` (8), `anchorTick` (4), `claimedScore` (4), `amount` (8) | 88 |
| Standalone (`StandaloneSolutionRecord`) | `sourcePublicKey` (32), `miningSeed` (32), `nonce` (32), `amount` (8), `claimedScore` (4), padding (4) | 112 |

When copying a snapshot to another machine, copy these files with the rest of `ep<epoch>/`.

## Epoch boundary

L is the epoch's last tick: the tick during which `isLastTickInEpoch()` returns true.

```
tick L - 1     END_TICK settles tick L - 1 - W                     vote for L - 1
               etalonTick's date reaches the epoch end
tick L         isLastTickInEpoch() is true
               END_TICK drains ticks L - W .. L (workers help)     vote for L covers the drain
epoch switch   the same predicate starts the switch
               window empty?   yes: always, by construction
                               no:  drain now, after the vote; the main loop logs it
               endEpoch()      asserts the window is empty
               beginEpoch()    tree, anchor ring, seed and window reset
tick L + 1     first tick of epoch E + 1, window empty
```

1. On the epoch's last tick, `settlePendingSolutions` sees `isLastTickInEpoch()` and calls
   `drainPendingSolutions`, which drains the whole window inside that tick's vote.
2. The epoch switch reads the same predicate, so it fires right after that tick and finds the window
   empty. If the two ever disagree, the switch drains the leftover itself, after the last vote, and the
   main loop reports it (see Monitoring). Request processors then park.
3. `endEpoch()` asserts the window is empty. After `beginEpoch()`, `checkAndSwitchMiningPhase(..., true)`
   calls `solutionSettlementBeginEpoch`, which resets the window. Nothing carries over between epochs.

- **Same answer on every node:** `isLastTickInEpoch()` reads the date in `etalonTick`, which is fixed
  during a tick and carried by every vote.
- **Workers help with the drain:** it runs at END_TICK, before the epoch switch parks them.
- **The last tick takes longer:** in exchange, a divergence stops the network at that tick, inside the
  epoch.
- **Not refunded at the boundary:** phase 1 rejects, and solutions anchored in epoch E that land in
  epoch E + 1 (`RejectStale`).

## Operational effects

### For computor operators

| Topic | What changes | What to do |
| --- | --- | --- |
| Deposits | When a solution is accepted, its 1000000 QU deposit comes back at its settle tick, W ticks after the transaction executes (or on the epoch's last tick), not in the same tick. | Keep enough balance for the deposits in flight: the solutions you published during the last W ticks, times 1000000 QU. A transaction the balance cannot cover is not executed, and its solution is not recorded. |
| Solution processors | Verification runs continuously on up to 12 request processors (`NUMBER_OF_SOLUTION_PROCESSORS`). | With fewer cores, verification can fall behind during bursts. The node then scores the rest during settlement: that tick takes longer, and consensus is not affected. Watch `inline` and `stalled` on F3. |
| Snapshots | Each snapshot has the window files (see "Files" under Save and load). | Copy them with the rest of the snapshot. Missing or mismatched window files make the node refuse the snapshot. |
| Epoch end | The last tick of the epoch settles every pending solution. | That tick can take longer than usual. No action needed. |

### For miners and pools

- **Building on a parent.** A solution can name a parent only after the parent has settled, more than W
  ticks after the parent's tick. Children of the root are not affected.
- **Solutions sent to a computor.** One whose parent has not settled yet is dropped on arrival at no cost;
  send it again once the parent has settled.
- **Refunds and ranking.** They appear W ticks after the solution's tick, or on the last tick of the epoch
  if that comes first.
- **Direct solution transactions.** A transaction whose parent is W ticks or less older than its own tick
  is refused when it executes, and its deposit is not refunded. The solution is marked as seen before the
  parent is checked, so the same nonce for that parent is refused as a replay from then on.

When things happen, for a solution transaction executed at tick T:

| Event | Before | Now |
| --- | --- | --- |
| The transaction executes and the solution gets its reference | T | T |
| Refund, ranking and tree membership | T | T + W (or the epoch's last tick) |
| Verdict on a recorded solution (refund or forfeit) | T | T + W (or the epoch's last tick); a phase 1 reject is still final at T |
| Served by `RequestAntParentAnn` and listed in the identity tree | T | T + W |
| Can be named as a parent | T + 1 | T + W + 1 |
| Log: deposit, miner to zero address | (T, its txIndex) | (T, its txIndex) |
| Log: refund, zero address to miner | (T, its txIndex) | (settle tick, `SC_END_TICK_TX`) |

The balance ends up the same; only the moment it becomes final moves. A miner's loop changes in its
cadence: a child waits for its parent to settle, and the identity tree shows a solution only W ticks
after it executed.

## Monitoring

F3 prints:

```
SolutionSettlement: settled through <tick> | pending <n> (unscored <n>) | appended <n>, refused <n> | scored by worker <n>, inline <n>, by drain <n>, stalled <n> | settled <n> | drains <n> | last drain <ms> ms
```

The counters restart from 0 when an epoch begins and when a snapshot is loaded.

| Field | Meaning | Healthy value |
| --- | --- | --- |
| settled through | The last tick whose solutions are settled | About the current tick minus W |
| pending | Solutions recorded but not settled yet | About the number of solutions in the last W ticks |
| unscored | Pending solutions not scored yet | Near 0 |
| appended, refused | Solutions recorded, and refused at record time (a refused solution loses its deposit) | `refused` stays 0 |
| scored by worker | Solutions scored in the background | Nearly all of them |
| inline | Solutions scored during settlement because no worker reached them in time | Rarely grows |
| by drain | Solutions scored by the epoch-end drain | 0, like `drains` |
| stalled | Times settlement waited for a worker to finish a solution | Rarely grows |
| settled | Solutions settled since the counters restarted | Grows with mining activity |
| drains | Drains since the counters restarted | 0: the drain runs on the epoch's last tick, and the counters restart when the next epoch begins |
| last drain | How long the most recent drain that settled something took | Kept across epochs |

One more line appears only on a fault, printed by the main loop every second until the node restarts:

```
SOLUTIONS SETTLED LATE AT EPOCH END: <n> pending, epoch <e>, last tick <t>, settled through <s>
```

It means END_TICK of the last tick `<t>` did not drain the window, so the epoch switch settled `<n>`
solutions after the last vote. Their refunds, tree entries and rankings are correct, but a scoring
divergence among them would show as a split at the next epoch's first tick rather than a stall at `<t>`.
`settled through` shows how far END_TICK got. Every node runs the same code, so expect the same line on
every node.

## Debugging a divergence

- A solution scored differently on some nodes shows up at its settle tick, as a `resourceTestingDigest`
  mismatch (and a spectrum mismatch if a refund differs). If the network stops at tick X, the solutions
  involved are the solution transactions of tick X - W; if X is the epoch's last tick, they can be in any
  of the last W ticks.
- Everything needed to re-score them can be had without the stuck node's state:
  - the transactions, from tick data;
  - the anchor digest (`computeAntAnchorDigest`: K12 of the anchor tick and of the K12 of that tick's
    `TickData`; only non-empty ticks are anchors);
  - the parent's network: a node serves it to its operator through `RequestAntParentAnn` (the request
    must be signed with the operator key), or it can be rebuilt from the root by re-scoring the parent's
    chain from its transactions;
  - the epoch task and the epoch seed.
- A vote's salted RTD is K12 of the computor's public key and the 32-bit digest, so the correct digest can
  be checked against every seat's vote.
- Keep a stuck node's replay cache as evidence before restarting it: it may hold the wrong score.

## Code map

| File | Contents |
| --- | --- |
| `src/mining/solution_settlement_window.h` | The window: arena, tick index, claiming, settle and drain loops, save and load, stats |
| `src/mining/solution_settlement_kinds.h` | Record and result layouts of both kinds, `isAntParentSettledBefore`, `resolveAntParent` |
| `src/qubic.cpp` (settlement glue) | Record, score and settle hooks of both kinds, `scoreAntChildCached`, `rejectAntSolution`, `isLastTickInEpoch`, `settlePendingSolutions`, `drainPendingSolutions`, lifecycle and save/load wrappers |
| `src/qubic.cpp` (callers) | `processTickTransaction` (phase 1), `processTick` (phase 2 at END_TICK), `requestProcessor` (workers), the epoch switch in `tickProcessor`, `saveAllNodeStates` and `loadAllNodeStates`, F3 in `processKeyPresses` |
| `src/qubic.cpp` (message path) | `processBroadcastMessage`, `queueAntSolution`, `publishAntSolutionFor` |
| `src/public_settings.h` | `SOLUTION_SETTLEMENT_WINDOW_TICKS`, `MAX_SOLUTION_RECORD_BYTES`, `MAX_SOLUTION_RESULT_BYTES`, the window file names |
