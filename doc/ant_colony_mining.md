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

With that split in mind: mining is a search for a **solution** that does well on a fixed task, and the
algorithm defines what a solution is and how it scores. Under **bpp9000** a solution is a neural network
(an "ANN"), scored by an **error count** over the task's data windows - range `[0, 8088]`, **lower is
better** (a flawless network makes zero mistakes). The rest of this overview uses bpp9000's terms, but
the tree structure around them is identical for any algorithm.

Under bpp9000 the network's **wiring** (which neuron reads which) is **global** - the same graph for
everyone, from the epoch's task file - and a miner searches over each neuron's **lookup table (LUT)**,
the ternary function it computes.

Standalone mining searches alone: every attempt starts from scratch. Ant-colony mining searches
**together, as a tree**:

- Every **mining identity** (a computor or candidate public key) owns its **own tree** - the colony is
  a per-identity forest. A pool's workers extend the tree of the computor they mine for.
- Every tree starts from the same **virtual root**: one starting solution per epoch, derived from the
  epoch's spectrum digest alone - identical for all identities, identical every time you derive it,
  and never stored or submitted. All identities search from one shared origin; the trees branching
  from it stay per-identity.
- To mine, you pick a **parent** (the root, or any node already in your tree), **inherit** it, vary it
  under your nonce, and score the result.
- If the result **strictly beats the parent** and clears the epoch **threshold**, you **submit** it. On
  acceptance it becomes a new tree node that you - or anyone - can extend further.

So the colony converges toward better solutions: each accepted node beats its parent, and the next
miner starts from there instead of from scratch. The goal of the epoch is the single best solution
found anywhere in the forest.

```
        virtual root (shared per epoch, not stored)
                 |
            +----+----+
            |         |
         node A     node B        each node beats its parent
            |         |           and clears the threshold
         node C     node D
            |
         node E   <-- best in this tree so far
```

Concretely, error gates every attachment: it only falls down a branch (a child must beat its parent),
and a *start* - a depth-1 child of the root - must clear the threshold.

```
    error = error count, lower is better          threshold = 4000

    root  ~4200 raw     the epoch root (same for everyone) sits above 4000; a start must mutate below it
      |
      +-- A  3900   <= threshold                            ACCEPT (depth-1 start)
      |    |
      |    +-- B  3540   < 3900, beats A                    ACCEPT
      |    |    |
      |    |    +-- D  3120   < 3540, beats B               ACCEPT
      |    |    +-- E  3560   not < 3540                    REJECT (must beat parent)
      |    |
      |    +-- C  3700   < 3900, beats A                    ACCEPT
      |
      +-- X  4100   > threshold                             REJECT (over threshold)

    Error only falls as you go deeper. The epoch winner is the single lowest-error node
    found in any identity's forest.
```

At epoch end the node ranks every identity by its **single best** score and **harvests the top 676**
(the number of computors).

**Anti-spam deposit.** Each solution a computor publishes on-chain carries a **refundable
1000000 QU deposit**, funded by the computor - not the worker. It is returned when the solution is
accepted **and** its claimed score matches the node's recompute; otherwise it is kept. So a computor
only publishes solutions it has already validated, and an honest, correct one costs nothing.

---

## Part 2 - Miner / pool integration guide

**In short.** A miner works one identity's tree. It reads the epoch context, takes a **parent** (the
epoch's shared virtual root, or a node already in the tree), picks a canonical **nonce**, inherits the
parent's network, and **mutates and scores** it - reproducing the node's score exactly. If the result
**beats its parent** and **clears the threshold**, it hands the solution to the **computor**, which
re-checks it and **publishes it on-chain**; every node then recomputes the score, folds it into
consensus, attaches the node to the tree, and refunds the deposit. A miner's entire job is an **exact
scorer** - the tree, gates, deposit, and queries are the wrapper around it.

### 2.1 The mining loop

1. **Epoch context** - `REQUEST_ANT_EPOCH_CONTEXT` (public). Read the threshold, freshness window,
   epoch spectrum digest, and child cap for this epoch, and **verify your task file** against the
   returned `topologyHash` / `dataHash` (section 2.7a) before doing any work.
2. **Get a starting point** - derive the epoch's shared virtual root (from the spectrum digest), or
   fetch an existing node you want to extend (`REQUEST_ANT_PARENT_ANN`).
3. **Pick a parent** - the root, or any node in your own tree.
4. **Search** - choose a nonce (section 2.2), inherit the parent LUT, run the mutation walk, score
   (section 2.3).
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
| `nonce[1]`  | `L` = LUT entries rewritten per mutation step | `[1, 10]` |
| `nonce[2]`  | `K` = number of **explore** steps | `[0, 100]` |
| `nonce[3..31]` | the walk seed (the actual search space) | any |

**Canonical-nonce rule.** The scorer **refuses** any non-canonical nonce - it returns no score, and the
node rejects the submission with `RejectNonCanonicalNonce`. For an ant solution the rule is:

```
algo == bpp9000  &&  nonce[1] in [1, 10]  &&  nonce[2] in [0, 100]
```

`nonce[0..2]` (the algo / `L` / `K` knobs) are **excluded from the RNG seed** - zeroed before hashing -
so `L` and `K` can be chosen without changing the walk seed. This makes the score-relevant bytes equal
to the identity/dedup bytes: there is no malleability room, and two nonces that differ only in these
knobs are not two solutions.

### 2.3 Scoring - bpp9000 (must be bit-exact)

Throughout, `publicKey` is the **mining identity you are extending** - the computor you mine for, which
becomes the transaction's `sourcePublicKey`. The **mutation seed** derives from **that** key, not your
worker key, or the node's recompute will not match yours. The **root** derives from no key at all -
see below.

**Root.** `deriveRootANN(spectrumDigest, epochPool)`: `K12(spectrumDigest)` - the epoch-start
spectrum digest from the epoch context - seeds a per-neuron LUT from the epoch's random pool (the
pool itself also comes from that digest). No mutation walk. Never stored. **One root per epoch,
identical for every identity**; per-identity variation enters only through the mutation seeds.

**Child.** `computeScoreFromParent(parentLUT, publicKey, nonce, anchorTickDigest)`:

1. Inherit `parentLUT`.
2. `mutationSeed = K12(publicKey || nonce || anchorTickDigest)` with `nonce[0..2]` zeroed in place
   (the full 32-byte nonce is hashed, its first 3 bytes set to 0, not dropped) - still keyed by the
   mining identity, so different identities walk differently from the shared root.
3. Walk `numberOfMutations = 100` steps. Each step rewrites `L` LUT entries. For the first `K` steps
   accept a worse-or-equal result (**explore**); after that accept only better-or-equal (**exploit**);
   one-step rollback on reject. Keep and return the **best** score seen. The best is seeded with the
   inherited network's own score, so a child that fails to improve on its parent is rejected (see
   `RejectLeParent`).

**Anchor digest.** `anchorTickDigest = K12(anchorTick || transactionDigest)`, where `transactionDigest`
is `K12(TickData)` of the anchor tick's `TickData` (`REQUEST_TICK_DATA`). This binds a solution to a tick.

**Empty ticks carry no anchor.** A tick without `TickData` records no anchor digest, and a published
solution whose `anchorTick` is an empty tick is rejected (`RejectStale`) with the **deposit
forfeited**. Anchor only on ticks that have `TickData`, make sure select a non-empty tick as an anchor tick.

Score is an error count in `[0, 8088]`; lower is better.

### 2.4 Accept rules

A submission is accepted (`Valid` or `ValidNotStored`) only if **all** of these hold. On failure the
reject reason names the rule the node found violated (the exact evaluation order is an internal
detail and can change):

| Check | Reject reason if it fails |
|-------|---------------------------|
| Parent record exists | `RejectParentNotRegistered` |
| Parent is in the **same** identity's tree (the tx `sourcePublicKey`) | `RejectWrongTree` |
| Nonce is canonical | `RejectNonCanonicalNonce` |
| Anchor not in the future, published within `freshnessWindow` ticks of it | `RejectStale` / `RejectTickOutOfRange` |
| Score `<=` epoch threshold | `RejectBelowThreshold` |
| Score **strictly** below the parent's score | `RejectLeParent` |
| Parent holds fewer than `maxChildrenPerParent` children (`0` = unbounded) | `RejectMaxChildrenPerParent` |
| `(publicKey, parentRef, nonce)` not already committed this epoch | `RejectReplay` |
| Store and miner index not full | `RejectDedupFull` / `RejectMinerIndexFull` |

Separately, the **claimed score must equal the node's recompute** - if not, the solution may still be
recorded but the **deposit is kept** and the miner is **not ranked**.

**Starting a tree.** The root's record score is the worst possible value, so a first (depth-1) child
passes the "beats parent" check trivially - the **threshold is the only score gate** for starting a
tree. The shared epoch root scores far above the threshold, so a start still requires real mutation -
and every identity starts from the same score, so ranking differences reflect search effort only.

**`ValidNotStored`.** Accepted, refunded, and ranked exactly like `Valid`, but the per-epoch store was
full so the node was not persisted for others to extend. Ranking and refund are unaffected.

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

A root solution starts a tree: its parent is the epoch's shared virtual root, which is derived rather
than stored, so you never fetch it. This is the first solution every identity submits, and it differs
from extending an existing node only in how the parent is named and scored.

1. **Parent is the root.** Set `parentRef = (parentTick = 0, parentSolutionIndexInTick = 0xFFFFFFFF)`.
   Both fields are load-bearing: `(0, 0xFFFFFFFF)` is the only value the node reads as root; a `0`
   tick with any other index is treated as a normal parent, found nowhere, and rejected with
   `RejectParentNotRegistered`.
2. **Derive the parent LUT yourself.** `deriveRootANN(spectrumDigest, epochPool)` from the epoch
   context (section 2.3) - do **not** call `REQUEST_ANT_PARENT_ANN` for the root; it answers
   `status = IS_ROOT` with no ANN payload precisely so you derive it locally. The root is identical
   for every identity.
3. **Search.** Pick a canonical nonce (section 2.2), inherit the derived root LUT, run the walk, and
   take the best score - exactly as for any parent.
4. **The only score gate is the threshold.** The root's record score is the worst possible value, so
   the "strictly beats the parent" rule passes trivially; a root child is accepted on score iff its
   score is `<=` the epoch threshold. The shared root scores far above the threshold, so a valid
   start still requires real mutation - and since every identity starts from the same root score,
   ranking reflects search effort alone.
5. **Anchor and submit.** Choose a non-empty anchor tick within `freshnessWindow` of the publish tick
   (section 2.3), fill the payload with the root `parentRef` above, and hand it to your computor
   (section 2.6, stage 1). The computor publishes it as the usual `AntColonyMiningSolutionTransaction`.

On the node, a root submission is recognized by `parentRef.isRoot()`: the parent lookup returns a
null record (root is not a stored solution), the node derives the shared root itself to recompute
your `claimedScore`, and root children are de-duplicated per miner because the root is shared by all
identities. Once accepted, the node becomes a normal parent - extend it by copying its `selfTick` /
`selfSolutionIndexInTick` from the identity-tree query (section 2.7b) into a child's `parentRef`.

### 2.7 Read queries

Three request/response pairs. **Identity tree** and **parent ANN** are **operator-signed**; **epoch
context** is **public**.

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
m256i          topologyHash;         // canonical task topology-block hash (BPP9000_TOPOLOGY_HASH)
m256i          dataHash;             // canonical task data-block hash (BPP9000_DATA_HASH)
unsigned int   threshold;            // per-epoch accept bound
unsigned int   freshnessWindow;      // publish within this many ticks of the anchor
unsigned int   solutionCount;        // accepted solutions so far this epoch
unsigned int   freeAnnSlotsCount;    // free slots in the live ANN pool
unsigned int   maxChildrenPerParent; // per-parent child cap; 0 = unbounded
unsigned short epoch;
unsigned short padding;
```

`topologyHash` / `dataHash` identify the exact task the node scores against. After loading your task
file, recompute K12 over your own topology and data blocks and compare against these. If either
differs you are holding the wrong task: **stop**. Mining a stale task wastes work, and the mismatched
score forfeits the computor's deposit on every submission (the node scores with *its* task, so your
claimed score never matches).

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
AntIdentityTreeNode {                     // 32 bytes
    unsigned int selfTick;                // ABSOLUTE; set these two as your parentRef to extend THIS node
    unsigned int selfSolutionIndexInTick;
    unsigned int parentTick;              // this node's own parent (ABSOLUTE); (0, 0xFFFFFFFF) = root
    unsigned int parentSolutionIndexInTick;
    unsigned int score;                   // error count; a child must score strictly below this
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

Response `RespondAntParentAnnHeader` (16 bytes), then `annSizeBytes` of **canonical ANN** (one trit per
byte, the exact form the scorer consumes - no unpacking needed):

```
unsigned int  parentRefTick;
unsigned int  parentRefSolutionIndexInTick;
unsigned int  annSizeBytes;   // ANN LUT size when status is OK, else 0
unsigned char status;         // 0 = OK, 1 = NOT_FOUND, 2 = IS_ROOT (derive the epoch root instead)
unsigned char padding[3];
```

**Canonical ANN layout.** The same byte form is used everywhere ANN bytes leave the node: this response, the snapshot pool, and the epoch export. Under the current bpp9000 parameters it is 1728 bytes = 64 rows of 27:

```
row k, k = 0..45     LUT of neuron updatedNeuronIndices[k]: the k-th NON-INPUT neuron in
                     ascending absolute index (input neurons have no LUT; which indices are
                     inputs comes from the task topology)
row k, k = 46..63    zero (the row count is fixed at the population size, the live count is
                     population minus inputs and so task-dependent)

byte[line] of a row, line = t0 + 3*t1 + 9*t2
                     the neuron's next trit for that neighbor state; t0, t1, t2 are the
                     current trits {0,1,2} of its three wired neighbors in task wiring
                     order (2 = UNKNOWN is an ordinary value)
```

Rows are ordered **by updated-neuron position, not by absolute neuron index**. A miner or tool that keeps LUTs indexed by absolute neuron number must convert through the task's `updatedNeuronIndices` mapping before comparing or reusing these bytes.

### 2.8 Network message + transaction types

| Type | Value | Signed | Direction |
|------|-------|--------|-----------|
| `BROADCAST_MESSAGE` + `MESSAGE_TYPE_ANT_SOLUTION` | `3` | envelope-signed | miner -> computor (submit) |
| `AntColonyMiningSolutionTransaction` (`inputType`) | `12` | computor-signed | computor -> tick data (consensus) |
| `REQUEST_ANT_IDENTITY_TREE` / `RESPOND_ANT_IDENTITY_TREE` | `72` / `73` | operator | read tree |
| `REQUEST_ANT_PARENT_ANN` / `RESPOND_ANT_PARENT_ANN` | `74` / `75` | operator | read one node's ANN |
| `REQUEST_ANT_EPOCH_CONTEXT` / `RESPOND_ANT_EPOCH_CONTEXT` | `76` / `77` | public | read epoch params |

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
| `antColonySolutions.eoe` | end-of-epoch export: best 676 networks (pubkey, score, depth, LUT) | output only |
| `bpp9000.task` | the epoch's pinned task | required at boot |

**Snapshot set**: the four snapshot files are part of the node snapshot. When you back up, copy, or restore node state, take them TOGETHER with the other snapshot files (spectrum, universe, contracts, system) from the same point. On load the node cross-checks the colony snapshot's rootSeed, error threshold, and initial tick against the restored node state, a colony snapshot from a different moment is refused and the node will not start from it.

**Replay cache**: a score memo, not consensus state, same behavior with standalone score. Without it a restart recomputes every stored solution, so keep it for fast restarts; losing it only costs time.

**Export**: written at the epoch transition for offline analysis. It is not read back by the node and is not needed for a restart.
