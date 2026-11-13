# Ant Colony Mining

This document has two parts:

- **Part 1 - Overview**: what ant-colony mining is.
- **Part 2 - Miner / pool integration guide**: the exact on-the-wire contract.

> Config values (threshold, freshness window, child cap) are per-epoch and can change between epochs.
> Always read the live values from the epoch-context query (Part 2, section 2.7a). Numbers quoted in
> this document are current defaults, not constants to hard-code.

> **Reading this doc.** Part 1 explains the split between the ant-colony *structure* and the mining
> *algorithm*. The parts specific to today's algorithm - the score's range and direction, the nonce
> knobs and their ranges, the mutation walk, and the numeric constants - are labelled **bpp9000** below;
> do not treat them as fixed ant-colony rules.

## Summary of changes - live BTC price feed

The task data no longer comes from a task file. It is built from hourly BTC/USDT prices that the nodes
fetch on-chain, and it grows by one hour at a time during the epoch.

**New rules**

| Rule | Result if broken | Section |
|------|------------------|---------|
| A solution is graded on the task data known before its **anchor tick**. Its frame cannot slide past the **shift limit** of that tick. | the shift stops at the limit | 2.3a |
| The parent's shift must be within the shift limit of the child's anchor tick. | no score: `RejectNonCanonicalNonce`, deposit kept | 2.3a |
| A child of a stored parent must beat that parent **rated on the child's task data**, not only the parent's recorded rating. A child of the virtual root is exempt. | no score: `RejectNonCanonicalNonce`, deposit kept | 2.3a |
| One solution per identity per tick: of several in the same tick, only the one at the highest transaction index is processed. | the others: `SkippedSupersededInTick`, deposit returned, may be published again | 2.4 |

**What a miner or pool has to change**

- Drop the task file. Fetch the targets and the shift limit of the anchor tick with
  `REQUEST_BPP9000_TASK_DATA` (80 / 81) and pass them to the scorer (section 2.7d).
- Limit the shift to the shift limit of the anchor tick, and anchor a child on a tick whose shift limit
  is at least its parent's shift.
- After the walk, submit a child of a stored parent only if it beats the parent's rating on the same
  targets.
- Publish at most one solution per identity per tick.
- `dataHash` of the epoch context is now K12 of the epoch's first 8760 targets (section 2.7a).

**Other changes**

- New public messages: `REQUEST_PRICE_SERIES` / `RESPOND_PRICE_SERIES` (78 / 79) and
  `REQUEST_BPP9000_TASK_DATA` / `RESPOND_BPP9000_TASK_DATA` (80 / 81) (sections 2.7d, 2.7e, 2.8).
- Node files: `bpp9000.task` is no longer read. The node needs `btcPriceSeries.{epoch}` or
  `btc_price_bootstrap.bin` to start, and the snapshot set gains `snapshotBtcPriceSeries.{epoch}`
  (Part 3).

---

## Part 1 - Overview

**Ant colony is not a mining algorithm - it is a search *structure*.** It does not decide what a good
solution looks like or how to score one - a **mining algorithm** does. `nonce[0]` selects which
algorithm runs; today the only one is **bpp9000**, which the ant colony runs by default. A future
algorithm could be added the same way, in the same structure. So "ant colony" and "bpp9000" are two
separate things: the structure, and the algorithm currently running in it.

| The ant-colony **structure** provides (any algorithm) | The **algorithm** provides (bpp9000 today) |
|---|---|
| a per-identity tree of solutions, grown from parents | what a solution *is* |
| the accept rules, the deposit, and the ranking | how to derive a child solution from a parent |
| the request / response queries | how to score a solution |

With that split in mind: mining is a search for a **solution** that does well on a task, and the
algorithm defines what a solution is and how it scores. Under **bpp9000** a solution is a neural network
(an "ANN") that runs on its own from a fixed start state, emitting an output sequence graded against the
task's target - an **error count** inside one `WINDOW_WIDTH`-wide frame of that target, range
`[0, WINDOW_WIDTH]`, **lower is better** (a flawless network makes
zero mistakes). The rest of this overview uses bpp9000's terms, but the tree structure around them is
identical for any algorithm.

Under bpp9000 a solution has three parts: the **start state** (each neuron's initial trit), the network's
**wiring** (which neuron reads which), and each neuron's **lookup table (LUT)**, the ternary function it
computes. All three start from the identity's own root, drawn in one go from the identity's public key
(the epoch's spectrum digest seeds the random pool the draw reads). The target output sequence the
network is graded against is built from hourly BTC/USDT prices (section 2.3a). Mining mutates
only the **wiring** (mode 2, declared in the nonce); the start state and the LUTs stay as the identity's
root drew them. A solution inherits all three parts from the parent it extends.

Standalone mining searches alone: every attempt starts from scratch. Ant-colony mining searches
**together, as a tree**:

- Every **mining identity** (a computor or candidate public key) owns its **own tree** - the colony is
  a per-identity forest. A pool's workers extend the tree of the computor they mine for.
- Every tree starts from its identity's own **virtual root**: one starting solution per identity per
  epoch, derived from that identity's **public key** (the epoch's spectrum digest supplies the random
  bytes), identical every time you derive it, and never stored or submitted. Each identity searches
  from its own origin.
- To mine, you pick a **parent** (the root, or any node already in your tree), **inherit** it, vary it
  under your nonce, and score the result.
- If the result **strictly beats the parent** and clears the epoch **threshold**, you **submit** it. On
  acceptance it becomes a new tree node that you - or anyone - can extend further.

So the colony converges toward better solutions: each accepted node beats its parent, and the next
miner starts from there instead of from scratch. The goal of the epoch is the single best solution
found anywhere in the forest.

```
        virtual root (per identity per epoch, not stored)
                 |
            +----+----+
            |         |
         node A     node B        each node beats its parent on (shift, error)
            |         |           and clears the threshold
         node C     node D
            |
         node E   <-- best in this tree so far
```

Concretely, the **rating** `(shift, error)` gates every attachment: it only improves down a branch (a
child must beat its parent), and every node still at shift 0 must clear the threshold. In practice that
only bites on a *start* - a depth-1 child of the virtual root - because any deeper child already beats a
parent that cleared it.

```
    every number below is illustrative, not a measured value - only the rules are real

    rating = (shift, error)   higher shift wins; at equal shift, lower error wins
    threshold = 4500          gates shift-0 nodes only; shift > 0 clears it outright

    virtual root        never submitted and never stored; for the beats-parent test it counts
                        as the WORST possible rating, so any child beats it. Its own network
                        scores ~5200 here - above the threshold - so a start has to mutate
                        below 4500 to be accepted at all.
      |
      +-- A  (0, 4400)  error <= threshold                        ACCEPT (depth-1 start)
      |    |
      |    +-- B  (0, 4250)  same shift, beats A on error         ACCEPT
      |    |    |
      |    |    +-- D  (1, 4600)  advanced the frame: beats B on  ACCEPT
      |    |    |                 shift alone, worse error and
      |    |    |                 no threshold check
      |    |    +-- E  (0, 4260)  same shift, not below 4250      REJECT (must beat parent)
      |    |
      |    +-- C  (0, 4300)  same shift, beats A on error         ACCEPT
      |
      +-- X  (0, 4700)  error above threshold at shift 0          REJECT (over threshold)

    Rating only improves as you go deeper. The epoch winner is the best (shift, error) in
    any identity's forest: highest shift first, lowest error as the tie-break.
```

At epoch end the node ranks every identity by its **single best** `(shift, error)` and **harvests the top 676**
(the number of computors).

**Anti-spam deposit.** Each solution a computor publishes on-chain carries a **refundable
1000000 QU deposit**, funded by the computor - not the worker. It is returned when the solution is
accepted **and** its claimed score matches the node's recompute; otherwise it is kept. So a computor
only publishes solutions it has already validated, and an honest, correct one costs nothing.

---

## Part 2 - Miner / pool integration guide

**In short.** A miner works one identity's tree. It reads the epoch context, takes a **parent** (the
identity's own virtual root, or a node already in the tree), picks a canonical **nonce**, inherits the
parent's network, and **mutates and scores** it - reproducing the node's score exactly. If the result
**beats its parent** and **clears the threshold**, it hands the solution to the **computor**, which
re-checks it and **publishes it on-chain**; every node then recomputes the score, folds it into
consensus, attaches the node to the tree, and refunds the deposit. A miner's entire job is an **exact
scorer** - the tree, gates, deposit, and queries are the wrapper around it.

### 2.1 The mining loop

1. **Epoch context** - `REQUEST_ANT_EPOCH_CONTEXT` (public). Read the threshold, freshness window,
   epoch spectrum digest, and child cap for this epoch (section 2.7a).
2. **Get a starting point** - derive the identity's virtual root (from its public key), or
   fetch an existing node you want to extend (`REQUEST_ANT_PARENT_ANN`).
3. **Pick a parent** - the root, or any node in your own tree.
4. **Search** - choose an anchor tick and fetch its task data (`REQUEST_BPP9000_TASK_DATA`, sections
   2.3a and 2.7d), choose a nonce (section 2.2), inherit the parent's start state, wiring, and LUTs,
   run the mutation walk, score (section 2.3).
5. **Submit** - if it passes the local rules (section 2.4), send `AntSolutionBroadcastPayload` to the
   computor you mine for (section 2.6, stage 1).
6. **Publish + confirm** - the computor validates and scores it, then publishes it on-chain as an
   `AntColonyMiningSolutionTransaction` (section 2.6, stage 2). Every node processes that transaction -
   recompute, fold the digest, commit to the computor's tree, refund the deposit. Query the tree
   (section 2.7b) to see accepted nodes and extend them.

### 2.2 The nonce (32 bytes)

| Byte(s)     | Meaning | Valid range |
|-------------|---------|-------------|
| `nonce[0]`  | algorithm selector (must select bpp9000) | - |
| `nonce[1]` bits 0-3 | `L` = changes per mutation step | `[1, MAX_CHANGES_PER_STEP]` |
| `nonce[1]` bits 4-5 | mutation mode: 2 = wiring (bits 6-7 = 0); 1 = start state and 3 = LUTs are not canonical | `2` |
| `nonce[2]`  | `K` = number of **explore** steps | `<= NUMBER_OF_MUTATIONS` (a byte, so any value) |
| `nonce[3..31]` | the walk seed (the actual search space) | any |

**Canonical-nonce rule.** The scorer **refuses** any non-canonical nonce - it returns no score, and the
node rejects the submission with `RejectNonCanonicalNonce`. For an ant solution the rule is:

```
algo == bpp9000  &&  L in [1, MAX_CHANGES_PER_STEP]  &&  mode == 2
                 &&  nonce[1] bits 6-7 == 0  &&  K <= NUMBER_OF_MUTATIONS
```

`nonce[0..2]` (the algo / `L` / mode / `K` knobs) are **excluded from the RNG seed** - zeroed before
hashing - so `L`, the mode, and `K` can be chosen without changing the walk seed. This makes the
score-relevant bytes equal to the identity/dedup bytes: there is no malleability room, and two nonces
that differ only in these knobs are not two solutions.

### 2.3 Scoring - bpp9000 (must be bit-exact)

Throughout, `publicKey` is the **mining identity you are extending** - the computor you mine for, which
becomes the transaction's `sourcePublicKey`. The **mutation seed** derives from **that** key, not your
worker key, or the node's recompute will not match yours. The **root** derives from that same key -
see below.

**Root.** `deriveRootANN(publicKey, epochPool)`: the root **wiring**, **start state** and **LUTs** all
come from `K12(publicKey)`, drawn from the epoch's random pool. The pool itself is seeded from the
epoch-start spectrum digest in the epoch context, so every identity draws from the same pool but at its
own position. No mutation walk. Never stored.
**One root per identity per epoch**, so two identities start from different networks.

**Child.** `computeScoreFromParent(parentANN, publicKey, nonce, anchorTickDigest)`:

1. Inherit the parent's start state, wiring, and LUTs.
2. `mutationSeed = K12(publicKey || nonce || anchorTickDigest)` with `nonce[0..2]` zeroed in place
   (the full 32-byte nonce is hashed, its first 3 bytes set to 0, not dropped) - still keyed by the
   mining identity, so different identities walk differently and from different roots.
3. Walk `numberOfMutations` steps. Each step applies `L` mutations of the mode declared in the
   nonce - `L` start-state trits, `L` links, or `L` LUT entries. For the first `K` steps accept a
   worse-or-equal result (**explore**); after that accept only better-or-equal (**exploit**); one-step
   rollback on reject. Keep and return the **best** score seen. The best is seeded with the inherited
   network's own score, so a child of a stored parent that fails to improve on it gets no score
   (section 2.3a, parent rule).

**Anchor digest.** `anchorTickDigest = K12(anchorTick || transactionDigest)`, where `transactionDigest`
is `K12(TickData)` of the anchor tick's `TickData` (`REQUEST_TICK_DATA`). This binds a solution to a tick.

**Empty ticks carry no anchor.** A tick without `TickData` records no anchor digest, and a published
solution whose `anchorTick` is an empty tick is rejected (`RejectStale`) with the **deposit
forfeited**. Anchor only on ticks that have `TickData`, make sure select a non-empty tick as an anchor tick.

**Shift.** The frame graded is not fixed: `shift` is where it starts in the task data, and a score is the
error count over `[shift, shift + WINDOW_WIDTH)`. When a frame is scored at or below
`ADVANCE_THRESHOLD` (3942), the network has mastered it: `shift` advances by one and the frame is
re-scored at the new position, repeating until it fails or reaches the **shift limit** of the
solution's anchor tick (section 2.3a; never above `SHIFT_CAP` = 168). A solution therefore reports a
**pair** - the shift it reached and the error in that frame.

`shift` is **inherited**: a child's walk starts at its parent's shift, not at 0, so a lineage
accumulates shift and depth buys frame position as well as a lower error. Only a child of the virtual
root starts at 0, once per epoch. It never moves backwards, and an inherited shift above the shift
limit of the child's anchor tick gets no score. A node with `shift > 0` also **bypasses the epoch
threshold** entirely - the threshold gates frame-0 solutions only.

Score is an error count in `[0, WINDOW_WIDTH]` for the frame at that shift; lower is better. Between
two solutions the **higher shift always wins** and error only breaks a tie at equal shift - a better
error can never make up for a lower shift.

### 2.3a Task data - live BTC/USDT prices (bpp9000)

The targets a network is graded against do not come from a file. Every node keeps an **hourly BTC/USDT
price series**, fetched on-chain through the Price oracle (Binance), and builds the targets from it.
The series grows by one hour at a time during the epoch, so the task data of a solution depends on its
**anchor tick**.

**Hours and prices.** Hour `H` is the time `H` hours after 2022-04-13 12:00:00 UTC. The price of hour
`H` is the oracle's reply for that timestamp, stored as an integer in units of 0.00000001 USDT
(`priceE8`).

**Targets.** One target per hour, one byte, `0` or `1`:

```
usdt(H)   = (priceE8(H) + 50000000) / 100000000        // whole USDT, rounded half up
target[t] = usdt(h) >= usdt(h - 1) ? 1 : 0             // h = baseHour - WINDOW_WIDTH + 1 + t
```

`baseHour` is the last hour the series held when the epoch began; it is fixed for the epoch. Targets
`0 .. WINDOW_WIDTH - 1` (8760 targets) are the first frame and end at `baseHour`. Every hour stored
during the epoch adds one target, up to `SHIFT_CAP` (168) more.

**Shift limit.** For an anchor tick `A`, the shift limit is the number of hours after `baseHour` that
were stored in a tick **before** `A`, at most `SHIFT_CAP`. A solution anchored at `A` is graded on
`WINDOW_WIDTH + shiftLimit` targets, and its frame cannot slide past `shiftLimit`:

- the shift stops advancing at `shiftLimit`, even when the frame there scores at or below
  `ADVANCE_THRESHOLD`;
- a child whose parent's shift is above the `shiftLimit` of the child's anchor tick gets no score.
  Anchor a child on a tick whose shift limit is at least its parent's shift.

**Parent rule.** The walk first rates the unmutated parent on the **child's** targets, starting at the
parent's recorded shift. With hours the parent did not have, this rating can be better than the one in
the parent's record. A child of a stored parent gets a score only if its rating strictly beats that
rating. A child of the virtual root is exempt. A miner has both ratings after its own walk: submit only
a child that passes.

**No score.** Both cases end like a non-canonical nonce, as `RejectNonCanonicalNonce`: the solution is
not stored, not ranked, and the deposit is kept.

**When a new hour appears.** The leader of every 5th tick (`BTC_FEED_SLOT_PERIOD`) may send one free
oracle query for the next missing hour, not earlier than 300 seconds after that hour
(`BTC_FEED_QUERY_DELAY_SEC`). The hour is stored in the tick in which the query finishes with a valid
price, and it counts for anchor ticks after that tick. A failed query is repeated after 300 seconds
(`BTC_FEED_RETRY_BACKOFF_SEC`). After 24 hours of failed attempts for one hour
(`BTC_FEED_GIVE_UP_SEC`) the hour is stored with the previous hour's price and flag bit 0 (filled);
its target is `1`.

**Consensus.** Every stored hour is folded into `resourceTestingDigest`, and the state of the series is
folded at the epoch switch and at a start from files. A node that holds a different series differs
from the network in its first vote.

**Reading it.** `REQUEST_BPP9000_TASK_DATA` returns the shift limit and the targets of an anchor tick,
ready for the scorer (section 2.7d). `REQUEST_PRICE_SERIES` returns the prices (section 2.7e).

### 2.4 Accept rules

A submission is accepted (`Valid` or `ValidNotStored`) only if **all** of these hold. On failure the
reject reason names the rule the node found violated (the exact evaluation order is an internal
detail and can change):

| Check | Reject reason if it fails |
|-------|---------------------------|
| Parent record exists | `RejectParentNotRegistered` |
| Parent is in the **same** identity's tree (the tx `sourcePublicKey`) | `RejectWrongTree` |
| The scorer returns a score: the nonce is canonical, the parent's shift is within the shift limit of the anchor tick, and a child of a stored parent beats that parent rated on the child's task data (section 2.3a) | `RejectNonCanonicalNonce` |
| Anchor not in the future, published within `freshnessWindow` ticks of it | `RejectStale` / `RejectTickOutOfRange` |
| Clears the floor: `error <= threshold`, **or** `shift > 0` (an advanced frame bypasses it) | `RejectBelowThreshold` |
| **Beats the parent's recorded rating** on `(shift, error)`: higher shift, or equal shift and strictly lower error | `RejectLeParent` |
| Parent holds fewer than `maxChildrenPerParent` children (`0` = unbounded) | `RejectMaxChildrenPerParent` |
| `(publicKey, parentRef, nonce)` not already committed this epoch | `RejectReplay` |
| Store and miner index not full | `RejectDedupFull` / `RejectMinerIndexFull` |

Separately, the **claimed score must equal the node's recompute** - if not, the solution may still be
recorded but the **deposit is kept** and the miner is **not ranked**.

**Starting a tree.** The root's record score is the worst possible value, so a first (depth-1) child
passes the "beats parent" check trivially - the **threshold is the only score gate** for starting a
tree. A root scores far above the threshold, so a start still requires real mutation.

**`ValidNotStored`.** Accepted, refunded, and ranked exactly like `Valid`, but the per-epoch store was
full so the node was not persisted for others to extend. Ranking and refund are unaffected.

**One solution per identity per tick.** When one identity (`sourcePublicKey`) has several solution
transactions in the same tick, only the one at the highest transaction index is processed. The others
end as `SkippedSupersededInTick`: they are not scored, the deposit is returned, and the same solution
may be published again in a later tick.

### 2.5 The deposit

Every on-chain `AntColonyMiningSolutionTransaction` carries a **1000000 QU** deposit
(`SOLUTION_SECURITY_DEPOSIT`), funded by the **computor** that publishes it - not the miner (see 2.6).
It is refunded **iff** the solution is accepted (`Valid` / `ValidNotStored`) **and** the claimed score
equals the node's recompute; otherwise it is kept. So a computor risks its own deposit and therefore
pre-validates each solution before publishing; a worker posts nothing on-chain. Keep the computor
identity funded above the deposit.

### 2.6 Submission: broadcast (off-chain), then transaction (on-chain)

A solution travels in two stages - an off-chain hand-off to a computor, then the on-chain transaction
that is the actual consensus record.

**Stage 1 - P2P broadcast (`AntSolutionBroadcastPayload`, 48 bytes).** The miner hands its solution to
the computor it mines for, inside the standard `BroadcastMessage` envelope (network type
`BROADCAST_MESSAGE`), message type `MESSAGE_TYPE_ANT_SOLUTION` (`3`):

```
BroadcastMessage {              // 96-byte envelope
    m256i sourcePublicKey;      // the worker key - pool off-chain accounting only, NOT the tree
    m256i destinationPublicKey; // the COMPUTOR you mine for - THIS identity owns the tree and funds the deposit
    m256i gammingNonce;         // first gamming byte selects MESSAGE_TYPE_ANT_SOLUTION (3)
}
// then the payload:
AntSolutionBroadcastPayload {   // 48 bytes
    unsigned int parentTick;      // ABSOLUTE tick of the parent node; the virtual root is (0, 0xFFFFFFFF)
    unsigned int parentSolutionIndexInTick;  // parent's index within its tick; 0xFFFFFFFF with parentTick 0 = root
    unsigned int anchorTick;      // ABSOLUTE tick number
    unsigned int claimedScore;
    m256i        nonce;           // the 32-byte nonce from 2.2
}
```

The computor scores and validates on receipt (a non-canonical or already-seen solution is dropped for
free). Nothing is on-chain yet.

**Stage 2 - on-chain transaction (`AntColonyMiningSolutionTransaction`, `inputType` 12).** When the
computor publishes, it emits a standard transaction into tick data under **its own** key and funds the
deposit from **its own** balance - the computor pays, not the miner. This transaction is the consensus
record: every node processes it in `processTick`, recomputes the score, folds it into
`resourceTestingDigest`, commits the node to the computor's tree, and refunds or keeps the deposit.

```
AntColonyMiningSolutionTransaction : Transaction {  // 80-byte header + 48-byte payload + 64-byte signature
    // --- Transaction header ---
    m256i          sourcePublicKey;      // the COMPUTOR (tree owner); signs the tx and funds the deposit
    m256i          destinationPublicKey; // zero (NULL_ID)
    long long      amount;               // SOLUTION_SECURITY_DEPOSIT = 1000000 QU
    unsigned int   tick;                 // publish tick
    unsigned short inputType;            // ANT_COLONY_MINING_SOLUTION_INPUT_TYPE = 12
    unsigned short inputSize;            // 48
    // --- payload (48 bytes) ---
    unsigned int   parentTick;           // ABSOLUTE tick of the parent node
    unsigned int   parentSolutionIndexInTick;
    unsigned int   anchorTick;           // ABSOLUTE
    unsigned int   claimedScore;
    m256i          nonce;
    // --- 64-byte signature over header + payload ---
}
```

`parentRef = (parentTick, parentSolutionIndexInTick) = (0, 0xFFFFFFFF)` means the **virtual root**
(a depth-1 child).

**Every tick in the protocol is absolute.** `parentTick`, `selfTick` and `anchorTick` are all real
system tick numbers - the same values `getCurrentTick` or a tick-data query returns. A parent is named
by the absolute tick it was committed in (plus its index within that tick), so the ref you copy from
the identity tree is used verbatim - there is no epoch-relative offset to convert.

**For a pool.** The tree belongs to the **computor** - `destinationPublicKey` of the broadcast, which
becomes `sourcePublicKey` of the transaction. Workers hold no tree and post no on-chain deposit; they
hand solutions to the pool's computor, which pre-validates them and risks its own deposit only on
solutions it expects to be accepted and refunded. Fund the computor identity, not the workers.

### 2.6a Submitting a root (depth-1) solution

A root solution starts a tree: its parent is this identity's virtual root, which is derived rather
than stored, so you never fetch it. This is the first solution every identity submits, and it differs
from extending an existing node only in how the parent is named and scored.

1. **Parent is the root.** Set `parentRef = (parentTick = 0, parentSolutionIndexInTick = 0xFFFFFFFF)`.
   Both fields are load-bearing: `(0, 0xFFFFFFFF)` is the only value the node reads as root; a `0`
   tick with any other index is treated as a normal parent, found nowhere, and rejected with
   `RejectParentNotRegistered`.
2. **Derive the parent yourself.** `deriveRootANN(publicKey, epochPool)` - the seed is the mining
   identity's **public key**, and the pool comes from the epoch's spectrum digest (section 2.3). Do
   **not** call `REQUEST_ANT_PARENT_ANN` for the root; it answers `status = IS_ROOT` with no ANN
   payload precisely so you derive it locally. Each identity has its own root, so deriving it from
   the spectrum digest instead of the public key yields a parent the node will not agree with.
3. **Search.** Pick a canonical nonce (section 2.2), inherit the derived root start state, wiring, and
   LUTs, run the walk, and take the best score - exactly as for any parent.
4. **The only score gate is the threshold.** The root's record score is the worst possible value, so
   the "strictly beats the parent" rule passes trivially; a root child is accepted on score iff its
   score is `<=` the epoch threshold. A root scores far above the threshold, so a valid start still
   requires real mutation.
5. **Anchor and submit.** Choose a non-empty anchor tick within `freshnessWindow` of the publish tick
   (section 2.3), fill the payload with the root `parentRef` above, and hand it to your computor
   (section 2.6, stage 1). The computor publishes it as the usual `AntColonyMiningSolutionTransaction`.

On the node, a root submission is recognized by `parentRef.isRoot()`: the parent lookup returns a
null record (root is not a stored solution), the node derives that identity's root itself to recompute
your `claimedScore`, and root children are de-duplicated on `(pubkey, nonce, parentRef)` like any other
solution. Once accepted, the node becomes a normal parent - extend it by copying its `selfTick` /
`selfSolutionIndexInTick` from the identity-tree query (section 2.7b) into a child's `parentRef`.

### 2.7 Read queries

Five request/response pairs. **Identity tree** and **parent ANN** are **operator-signed**; **epoch
context**, **task data** and **price series** are **public**.

Operator signing: the request payload is followed by a **64-byte signature** over `K12(payload)`,
verified against the node's configured **operator public key**. This lets a pool route and filter its
own miners' reads while keeping untrusted parties from spamming the node. There is no monotonic nonce -
replaying a read only costs a duplicate answer.

```
digest    = K12(requestPayload)
signature = sign(operatorSubseed, operatorPublicKey, digest)   // 64 bytes, appended to the payload
```

**(a) Epoch context** - `REQUEST_ANT_EPOCH_CONTEXT` (76) / `RESPOND_ANT_EPOCH_CONTEXT` (77). **Public.**

Request: empty. Response `RespondAntEpochContext` (120 bytes, packed):

```
m256i          spectrumDigest;       // epoch-start spectrum digest; IS the root seed (and seeds the pool)
m256i          topologyHash;         // constant BPP9000_TOPOLOGY_HASH
m256i          dataHash;             // K12 of the epoch's first WINDOW_WIDTH targets (section 2.3a)
unsigned int   threshold;            // per-epoch accept bound
unsigned int   freshnessWindow;      // publish within this many ticks of the anchor
unsigned int   solutionCount;        // accepted solutions so far this epoch
unsigned int   freeAnnSlotsCount;    // free slots in the live ANN pool
unsigned int   maxChildrenPerParent; // per-parent child cap; 0 = unbounded
unsigned short epoch;
unsigned short padding;
```

`dataHash` identifies the task data of the epoch: K12 over its first `WINDOW_WIDTH` (8760) targets, one
byte each. The first 8760 target bytes of every valid `RESPOND_BPP9000_TASK_DATA` of the epoch
(section 2.7d) hash to this value. `topologyHash` is a build constant; scoring reads no topology
block (section 2.3).

**(b) Identity tree** - `REQUEST_ANT_IDENTITY_TREE` (72) / `RESPOND_ANT_IDENTITY_TREE` (73).
**Operator-signed.** Paginated.

Request `RequestAntIdentityTree` (40 bytes) + 64-byte signature:

```
m256i        pubkey;    // whose tree to report (usually your own)
unsigned int fromIndex; // resume cursor, 0 on the first call
unsigned int padding;
```

Response: `RespondAntIdentityTreeHeader` (12 bytes: `count`, `itemSize`, `nextIndex`) followed by
`count` x `AntIdentityTreeNode`. Up to 64 nodes per response; page with `nextIndex` until it is `0`.

```
AntIdentityTreeNode {                     // 36 bytes
    unsigned int selfTick;                // ABSOLUTE; set these two as your parentRef to extend THIS node
    unsigned int selfSolutionIndexInTick;
    unsigned int parentTick;              // this node's own parent (ABSOLUTE); (0, 0xFFFFFFFF) = root
    unsigned int parentSolutionIndexInTick;
    unsigned int score;                   // error count inside this node's frame
    unsigned int shift;                   // this node's frame position; your walk starts here
    unsigned int childCount;              // children already attached (compare vs maxChildrenPerParent)
    unsigned int anchorTick;
    unsigned int depth;
}
```

Paging every node of a pubkey reconstructs the whole tree, edges included, with no further fetches.

**(c) Parent ANN** - `REQUEST_ANT_PARENT_ANN` (74) / `RESPOND_ANT_PARENT_ANN` (75). **Operator-signed.**

Request `RequestAntParentAnn` (8 bytes) + 64-byte signature:

```
unsigned int parentRefTick;
unsigned int parentRefSolutionIndexInTick;
```

Response `RespondAntParentAnnHeader` (16 bytes), then `annSizeBytes` of the **ANN** (the wiring, start
state, and LUTs, the exact form the scorer consumes - no unpacking needed):

```
unsigned int  parentRefTick;
unsigned int  parentRefSolutionIndexInTick;
unsigned int  annSizeBytes;   // sizeof(ANN) when status is OK, else 0
unsigned char status;         // 0 = OK, 1 = NOT_FOUND, 2 = IS_ROOT (derive your own root instead)
unsigned char padding[3];
```

**ANN layout.** The same byte form is used everywhere ANN bytes leave the node: this response, the snapshot pool, and the epoch export. Its size follows the parameters: `P*K*2` wiring + `P` start state + `P*lutSize` LUT, contiguous in that
order, where `P` = POPULATION_THRESHOLD, `K` = NUMBER_OF_NEIGHBORS (3) and `lutSize` = 27.

```
wiring: neighbor[P*K]  P*K uint16 link targets = P neurons x K neighbours; neighbor[n*K + k]
                       is neuron n's k-th neighbour (any neuron index in [0, P))

start state: P bytes   initialNeuronValues[n] is neuron n's initial trit {0,1,2}; every neuron
                       starts the rollout from this value

lut: P*lutSize bytes = P rows of lutSize:
row n, n = 0..P-1      neuron n's LUT, at its absolute neuron index (every neuron computes;
                       bpp9000 has no input neurons)

byte[line] of a row, line = t0 + 3*t1 + 9*t2
                       the neuron's next trit for that neighbor state; t0, t1, t2 are the
                       current trits {0,1,2} of its three wired neighbors (2 = UNKNOWN is
                       an ordinary value)
```

Each neuron's LUT sits at its absolute index, so a miner or tool that keeps the ANN by absolute neuron number uses these bytes directly.

**(d) Task data** - `REQUEST_BPP9000_TASK_DATA` (80) / `RESPOND_BPP9000_TASK_DATA` (81). **Public.**

Request `RequestBpp9000TaskData` (4 bytes):

```
unsigned int anchorTick;
```

Response `RespondBpp9000TaskData`: a 20-byte header (packed), then the targets:

```
unsigned int   tick;        // the node's current tick
unsigned int   anchorTick;  // as requested
unsigned short epoch;
unsigned char  valid;       // 1 = the fields below are set and the targets follow; 0 = nothing follows
unsigned char  reserved;
unsigned int   baseHour;    // target t belongs to hour baseHour - WINDOW_WIDTH + 1 + t
unsigned int   shiftLimit;  // last shift a solution with this anchor tick may reach
// then WINDOW_WIDTH + shiftLimit bytes: the targets, 0 or 1 each
```

`valid` is 0 when `anchorTick` is before the first tick of the epoch or after the node's current tick,
or when the node holds no price series. The number of targets is the payload size minus 20. Pass the
targets to the scorer unchanged and limit the shift to `shiftLimit`. The task data of an anchor tick
does not change once the node has reached that tick.

**(e) Price series** - `REQUEST_PRICE_SERIES` (78) / `RESPOND_PRICE_SERIES` (79). **Public.**

Request `RequestPriceSeries` (8 bytes):

```
unsigned int fromHour;  // first hour wanted
unsigned int count;     // number of hours wanted; at most 256 are returned
```

Response `RespondPriceSeries`: a 124-byte header (packed), then `count` points:

```
unsigned int       tick;             // the node's current tick
unsigned short     epoch;
unsigned char      synced;           // 1 = the node holds a usable series
unsigned char      bound;            // 1 = a query for nextHour is running
unsigned int       source;           // kind of file the series was loaded from:
                                     // 1 bootstrap, 2 epoch switch, 3 F6, 4 snapshot
unsigned int       nextHour;         // first hour not stored yet
unsigned int       failedAttempts;   // failed queries for nextHour
long long          firstFailMs;      // first and last failed attempt for nextHour,
long long          lastFailMs;       // in milliseconds since 2022-04-13 12:00:00 UTC
unsigned char      seriesDigest[32]; // digest of the series, for comparing nodes
unsigned long long numSuccess;       // node-local counters of feed queries
unsigned long long numTimeout;
unsigned long long numUnresolvable;
unsigned long long numInvalidReply;
unsigned long long numStartFailed;
unsigned long long numFilled;        // hours stored with the previous hour's price
unsigned int       baseHour;         // last hour known when the epoch began
unsigned int       count;            // points that follow

RespondPriceSeriesPoint {            // 20 bytes
    unsigned int hour;
    unsigned int appendTick;         // tick the hour was stored in (hours after baseHour)
    unsigned int flags;              // bit 0 = filled with the previous hour's price
    long long    priceE8;            // price in 0.00000001 USDT
}
```

Points are returned from `fromHour` up to `nextHour - 1`; an hour the node does not hold is left out.

### 2.8 Network message + transaction types

| Type | Value | Signed | Direction |
|------|-------|--------|-----------|
| `BROADCAST_MESSAGE` + `MESSAGE_TYPE_ANT_SOLUTION` | `3` | envelope-signed | miner -> computor (submit) |
| `AntColonyMiningSolutionTransaction` (`inputType`) | `12` | computor-signed | computor -> tick data (consensus) |
| `REQUEST_ANT_IDENTITY_TREE` / `RESPOND_ANT_IDENTITY_TREE` | `72` / `73` | operator | read tree |
| `REQUEST_ANT_PARENT_ANN` / `RESPOND_ANT_PARENT_ANN` | `74` / `75` | operator | read one node's ANN |
| `REQUEST_ANT_EPOCH_CONTEXT` / `RESPOND_ANT_EPOCH_CONTEXT` | `76` / `77` | public | read epoch params |
| `REQUEST_PRICE_SERIES` / `RESPOND_PRICE_SERIES` | `78` / `79` | public | read the BTC price series |
| `REQUEST_BPP9000_TASK_DATA` / `RESPOND_BPP9000_TASK_DATA` | `80` / `81` | public | read the task data of an anchor tick |

## Part 3 - Node files

The ant colony adds the following files on the node's disk. File names are defined in
`public_settings.h`; `{epoch}` is the epoch number.

| File | Content | Handling |
|------|---------|----------|
| `snapshotAntColonyHeader.{epoch}` | colony meta + anchor ring + export set | snapshot set |
| `snapshotAntColonyRecords.{epoch}` | accepted solution records | snapshot set |
| `snapshotAntColonyPool.{epoch}` | packed ANN pool | snapshot set |
| `snapshotAntSolutionFlag` | ant solution flags bitmap | snapshot set |
| `antColonyReplayCache.{epoch}` | score cache | optional, recommended |
| `antColonySolutions.eoe` | end-of-epoch export: best 676 networks (pubkey, score, depth, ANN) | output only |
| `btcPriceSeries.{epoch}` | BTC price series the epoch starts from | required at boot, unless the bootstrap file is used |
| `btc_price_bootstrap.bin` | BTC price series shipped with a release | read only when no `btcPriceSeries.{epoch}` is usable |
| `snapshotBtcPriceSeries.{epoch}` | BTC price series at the snapshot tick | snapshot set |

**Snapshot set**: the five snapshot files are part of the node snapshot. When you back up, copy, or restore node state, take them TOGETHER with the other snapshot files (spectrum, universe, contracts, system) from the same point. On load the node cross-checks the colony snapshot's rootSeed and initial tick against the restored node state, plus the record and ANN sizes the build lays out; a colony snapshot from a different moment or a different build is refused and the node will not start from it. The error threshold is logged when it differs but not enforced, so a snapshot still loads after a mid-epoch threshold change.

**Price series**: each file is 393304 bytes. The node writes `btcPriceSeries.{epoch}` for the new epoch at every epoch switch, and `btcPriceSeries.000` on F6. A start at the first tick of an epoch takes the epoch-switch file; if there is none, it takes `btc_price_bootstrap.bin`, which must match `BTC_PRICE_BOOTSTRAP_HASH` and be built for that epoch. A mid-epoch restart with a new initial tick takes the F6 file, renamed to the epoch like the other F6 files. Without a usable series the node does not start. A snapshot whose `snapshotBtcPriceSeries.{epoch}` is missing or unusable is not used.

**Replay cache**: a score memo, not consensus state, same behavior with standalone score. Without it a restart recomputes every stored solution, so keep it for fast restarts; losing it only costs time.

**Export**: written at the epoch transition for offline analysis. It is not read back by the node and is not needed for a restart.
