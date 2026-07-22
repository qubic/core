# Outsourced Computations Protocol Specification

This document specifies the on-chain protocol for Outsourced Computations (OC).
Conceptual background, rationale, and usage examples are in `contracts_outsourced_computations.md`.
This document defines wire formats, state transitions, normative requirements, and constants.


## 1. Constants

| Name                   | Value                            | Notes                                                          |
|------------------------|----------------------------------|----------------------------------------------------------------|
| `NUMBER_OF_COMPUTORS`  | 676                              | Defined in `network_messages/common_def.h`.                    |
| `QUORUM`               | 451                              | `NUMBER_OF_COMPUTORS * 2 / 3 + 1`.                             |
| `SIGNATURE_SIZE`       | 64                               | Bytes per SchnorrQ (FourQ) signature.                          |
| `MAX_INPUT_SIZE`       | 1024                             | Maximum bytes for any transaction input payload.               |
| `MAX_OC_REQUEST_SIZE`  | `MAX_INPUT_SIZE - 16`            | Mirrors `MAX_ORACLE_QUERY_SIZE`. Equals 1008.                  |
| `MIN_OC_INVOCATION_FEE`| 10                               | Minimum value `getInvocationFee` may return. Mirrors `MIN_ORACLE_QUERY_FEE`. |
| `OC_INVOCATION_TIMEOUT_DEFAULT_TICKS` | 3                  | Maximum number of ticks an invocation may remain in `PENDING_AUTHORIZATION` before transitioning to `TIMEOUT`. |
| `MAX_OC_INVOCATIONS_PER_EPOCH` | `2^21` (2,097,152)      | Maximum invocations recorded per epoch (section 3.3). Mirrors OM's `MAX_ORACLE_QUERIES`. |
| `MAX_OC_IN_FLIGHT_INVOCATIONS` | 1024                    | Maximum invocations unresolved at any moment (section 3.1.1). Mirrors OM's `MAX_SIMULTANEOUS_ORACLE_QUERIES`. |
| OC authorization tx `inputType` | 13                      | `OcAuthSignatureTransactionPrefix::transactionType()` in `oc_core/oc_transactions.h`. |
| `OC_MACHINE_INVOCATION` | 192                             | `NetworkMessageType` for the Core-to-OC-machine channel (section 8). |


## 2. Identifiers

### 2.1 Invocation ID

An `invocationId` is a 64-bit signed integer assigned by the OC engine when `__qpiInvokeOC` succeeds.
It MUST be derived from the issuing tick and a per-tick sequence counter, following the OM `getNewNonTxQueryId` convention:

```
invocationId = (sint64(system.tick) << 31) | indexInTick
```

where `indexInTick` is a per-tick counter starting from `NUMBER_OF_TRANSACTIONS_PER_TICK` (which guarantees no collision with transaction-derived IDs in the same name space).

Because `system.tick` increases monotonically across epoch boundaries, this scheme produces globally unique, monotonically increasing IDs without per-epoch reset.
This mirrors `OracleEngine::getNewNonTxQueryId` in `oracle_core/oracle_engine.h`.

### 2.2 Parameters digest

The `paramsDigest` is a 32-byte K12 hash over the exact bytes of the pinned `OcRequest`.
The hash input MUST be the raw `OcRequest` struct as laid out in memory, of length equal to the interface's declared `requestSize`.
Implementations MUST zero-initialize the request buffer in the engine's per-invocation storage before copying `OcRequest` bytes into it.
Contract authors MUST zero-initialize their local `OcRequest` value before assigning any field, to ensure deterministic padding bytes across all computors executing the same contract code.

### 2.3 Authorization message

The message a computor signs is the K12 hash of the concatenation:

```
authMessage = K12(
    "QUBIC_OC_AUTH" ||           // 13 bytes, ASCII, no terminator
    uint16_le(epoch) ||          // 2 bytes
    uint16_le(interfaceIndex) || // 2 bytes
    sint64_le(invocationId) ||   // 8 bytes
    paramsDigest                 // 32 bytes
)
```

The domain separator prefix `"QUBIC_OC_AUTH"` ensures signatures over OC authorizations cannot be replayed as signatures over any other Qubic message type.
The `epoch` field binds a signature to its issuing epoch.


## 3. OC engine state

The OC engine mirrors `OracleEngine` in `oracle_core/oracle_engine.h`.
It owns per-invocation state, processes authorization signature transactions, and forwards authorized bundles to OC machine peers.

### 3.1 Invocation record

The engine MUST maintain one record per active invocation containing at least:

| Field             | Type                          | Description                                                          |
|-------------------|-------------------------------|----------------------------------------------------------------------|
| `invocationId`    | `sint64`                      | ID assigned at QPI call time (section 2.1).                          |
| `epoch`           | `uint16`                      | Epoch in which the invocation was recorded.                          |
| `contractIndex`   | `uint16`                      | Calling contract index.                                              |
| `interfaceIndex`  | `uint16`                      | OC interface index.                                                  |
| `paramsOffset`    | `uint32`                      | Offset into the engine's request storage buffer.                     |
| `paramsSize`      | `uint16`                      | Equals `OCI::ocInterfaces[interfaceIndex].requestSize`.              |
| `paramsDigest`    | `m256i`                       | K12 over the pinned bytes (section 2.2).                             |
| `status`          | `uint8`                       | One of the consensus status constants in section 5.                  |
| `creationTick`    | `uint32`                      | Tick in which the invocation was recorded.                           |
| `agreeingSigs`    | `uint16`                      | Count of distinct valid signatures observed for `paramsDigest`.      |
| `signedBy`        | bitmap `[NUMBER_OF_COMPUTORS]`| One bit per computor index; set when that computor's signature has been counted. |
| `signatures`      | `uint8[QUORUM][SIGNATURE_SIZE]` | The first `QUORUM` distinct valid signatures, indexed by their order of acceptance. |
| `signerIndices`   | `uint16[QUORUM]`              | Computor index in `broadcastedComputors` for the signature at the same position. |

The `signatures` and `signerIndices` arrays are populated as valid signatures arrive.
Once `agreeingSigs == QUORUM`, both arrays are full and form the authorization bundle delivered to OC machines (section 8).
Signatures arriving after the QUORUM threshold MUST be discarded by the signature processing rules in section 7.1 (the array bounds make appending past `signatures[QUORUM - 1]` an out-of-bounds write).

### 3.1.1 Record size

The fields above sum to approximately 30 KB per record, dominated by the 451-signature bundle (~28.8 KB).
An engine sized for thousands of concurrent invocations per epoch therefore needs tens to hundreds of MB of fixed allocation for the invocation table alone.

The specification does not prescribe a particular layout.
Implementations MAY adopt either or both of the following storage optimizations, as long as the externally-visible behavior of section 7.1 (signature counting and quorum transition) and section 8 (delivery) is preserved:

- **Discard the bundle after a single delivery attempt.** As soon as the bundle has been enqueued once for the configured OC peers, the engine frees `signatures` and `signerIndices` and retains only metadata. The record's `status` remains `AUTHORIZED`. This is fire-once-and-forget: delivery is a best-effort broadcast to whichever OC peers are currently connected, with no per-peer receipt tracking and no re-delivery (see section 8.2). Re-delivery after restart is likewise impossible from engine state alone; see section 12.
- **Split bundle storage from metadata.** Hold the bundle bytes in a separate `bundleStorage` buffer (the same split OM uses between `queryStorage` and `OracleReplyState`), with each invocation record referencing its bundle by offset. A small bundle pool can then serve a large metadata table without paying ~30 KB per slot.

The reference implementation in `oc_core/oc_engine.h` adopts both, following the split OM uses between the 72-byte `OracleQueryMetadata` and the pooled `OracleReplyState`:

- The per-epoch `OcInvocationRecord` is 64 bytes and holds only identity, digest, offsets, and status.
- All per-computor tracking (the accumulating signature bundle, the `signedBy`/`scheduledBy` bitmaps, and `agreeingSigs`) lives in a pooled `OcInFlightAuthState` (~30 KB) of `MAX_OC_IN_FLIGHT_INVOCATIONS = 1024` slots (~30 MB total), referenced from the record only while the invocation is unresolved.
- The slot is allocated when the invocation is recorded and freed on timeout or after the single delivery attempt. Because slot state is derived purely from tick processing, allocation and release are deterministic across all nodes.
- If the pool is exhausted, `startContractInvocation` rejects the invocation (returns `-1`, and the fee is refunded per section 6.3). The pool size therefore acts as deterministic admission control bounding the number of concurrently unresolved invocations; it does not bound the per-epoch total, since slots recycle within `OC_INVOCATION_TIMEOUT_DEFAULT_TICKS + 1` ticks.

### 3.2 Per-epoch lifetime

Invocation records and their pinned request payloads MUST be retained across all ticks of the epoch in which they were created.
At `beginEpoch`, the engine MUST discard all records from the previous epoch, mirroring the behavior of `OracleEngine::beginEpoch`.

Invocations created near the end of an epoch may not reach quorum before the boundary and are dropped without delivery; the OM engine code acknowledges the same limitation.
A seamless transition mechanism that carries `PENDING_AUTHORIZATION` records into the next epoch with renewed timeout is not part of this specification.

### 3.3 Per-invocation request storage

The engine MUST allocate a contiguous buffer for pinned `OcRequest` payloads, provisioned as a total byte budget rather than `MAX_OC_REQUEST_SIZE` per record (the same approach as OM's `ORACLE_QUERY_STORAGE_SIZE`).
The reference implementation budgets an average of 256 bytes per invocation: `256 * MAX_OC_INVOCATIONS_PER_EPOCH = 536,870,912` bytes (512 MiB).
Since request sizes are fixed per interface, the effective invocation capacity depends on the interface mix; `startContractInvocation` rejects (with fee refund) when either the record-count cap or the byte budget is exhausted, whichever is hit first.

Pinned bytes MUST NOT be mutated after the invocation has been recorded.


## 4. Transactions

### 4.1 `OcAuthSignatureTransaction`

A single `OcAuthSignatureTransaction` carries one or more authorization signatures from the issuing computor.
Each item in the transaction signs exactly one invocation, but a single transaction MAY carry items for multiple distinct invocations.

Wire layout, following the `Transaction` header in `network_messages/transactions.h`:

```
+-------------------------------+
| Transaction header (80 bytes) |   sourcePublicKey, destinationPublicKey,
|                               |   amount, tick, inputType, inputSize
+-------------------------------+
| inputSize bytes payload:      |
|   uint16 itemCount            |
|   uint16 _padding (= 0)       |
|   N x OcAuthSignatureItem     |
+-------------------------------+
| SIGNATURE_SIZE bytes outer    |
| transaction signature         |
+-------------------------------+
```

`OcAuthSignatureItem` layout (112 bytes):

```
struct OcAuthSignatureItem {
    sint64 invocationId;        // 8 bytes
    uint16 interfaceIndex;      // 2 bytes
    uint16 epoch;               // 2 bytes
    uint8  _padding[4];         // 4 bytes, MUST be zero
    uint8  paramsDigest[32];    // 32 bytes
    uint8  signature[64];       // 64 bytes, full SchnorrQ signature
};
static_assert(sizeof(OcAuthSignatureItem) == 112);
```

Constraints:

- `inputType` MUST equal 13 (`OcAuthSignatureTransactionPrefix::transactionType()`, see section 1).
- `inputSize` MUST equal `4 + 112 * itemCount`.
- `itemCount` MUST be in `[1, (MAX_INPUT_SIZE - 4) / 112] = [1, 9]`.
- `sourcePublicKey` MUST be a public key listed in the current epoch's `broadcastedComputors`.
- `destinationPublicKey` MUST be `NULL_ID` (zero).
- `amount` MUST be 0.
- The `signature` field of each item MUST be the SchnorrQ signature over `authMessage` (section 2.3) computed using the issuing computor's private key.
- The transaction's outer signature MUST be valid over the standard transaction digest.


## 5. Status machine

The engine maintains two distinct status concepts:

- **Consensus status** is part of the invocation record and consistent across all nodes processing the same tick stream. It is exposed through public query APIs.
- **Delivery status** is node-local bookkeeping for tracking which OC peers have received the bundle from this node. It is NOT consensus state and MUST NOT be exposed through any public query API. It MUST NOT be included in snapshots.

### 5.1 Consensus status

```
                         +-------------------------+
   __qpiInvokeOC()  ---> | PENDING_AUTHORIZATION   |
                         +-------------------------+
                              |              |
              QUORUM sigs     |              |   timeout
              counted         v              v
                         +-----------+   +---------+
                         | AUTHORIZED|   | TIMEOUT |
                         +-----------+   +---------+
```

Status constants (in the namespace and pattern of `ORACLE_QUERY_STATUS_*`):

| Constant                              | Value | Description                                                                 |
|---------------------------------------|-------|-----------------------------------------------------------------------------|
| `OC_INVOCATION_STATUS_UNKNOWN`        | 0     | Invocation ID not found.                                                    |
| `OC_INVOCATION_STATUS_PENDING_AUTH`   | 1     | Recorded; waiting for `QUORUM` authorization signatures.                    |
| `OC_INVOCATION_STATUS_AUTHORIZED`     | 2     | `QUORUM` signatures counted; bundle eligible for delivery to OC peers.      |
| `OC_INVOCATION_STATUS_TIMEOUT`        | 3     | Authorization did not reach quorum before `OC_INVOCATION_TIMEOUT_DEFAULT_TICKS`. |

Transition rules:

- `UNKNOWN -> PENDING_AUTH` on successful `__qpiInvokeOC`.
- `PENDING_AUTH -> AUTHORIZED` when `agreeingSigs >= QUORUM`. The transition MUST be atomic with respect to additional signature processing.
- `PENDING_AUTH -> TIMEOUT` when `currentTick - creationTick >= OC_INVOCATION_TIMEOUT_DEFAULT_TICKS`.
- No transitions out of `AUTHORIZED` or `TIMEOUT`.

### 5.2 Delivery status (node-local)

The engine MAY track, for each `(invocationId, ocPeerId)` pair, whether the bundle has been successfully enqueued on that peer's outgoing connection.
This state is private to each Core node, MUST NOT enter snapshots, and MUST NOT be exposed via public RPC.
See section 8 for delivery rules.


## 6. QPI surface

### 6.1 Public macro

```
#define INVOKE_OC(OcInterface, request) qpi.__qpiInvokeOC<OcInterface>(request)
```

### 6.2 Internal entry point

```
template <typename OcInterface>
inline sint64 QpiContextProcedureCall::__qpiInvokeOC(
    const typename OcInterface::OcRequest& request
) const;
```

### 6.3 Behavior

The implementation MUST perform the following steps in order:

1. Compile-time checks via `static_assert`:
   - `sizeof(OcInterface::OcRequest) <= MAX_OC_REQUEST_SIZE`.
   - `OcInterface::ocInterfaceIndex < OCI::ocInterfacesCount`.
   - `OCI::ocInterfaces[OcInterface::ocInterfaceIndex].requestSize == sizeof(OcInterface::OcRequest)`.
2. Validate that `_currentContractIndex < MAX_NUMBER_OF_CONTRACTS`. On failure: return `-1`.
3. Compute `fee = OcInterface::getInvocationFee(request)`. If `fee < MIN_OC_INVOCATION_FEE`: return `-1`.
4. Attempt `decreaseEnergy(contractSpectrumIdx, fee)`. On failure: return `-1`. On success: log a `QuTransfer` from the contract ID to `NULL_ID` of `fee` QU.
5. Call the engine's `startContractInvocation(contractIndex, interfaceIndex, &request, sizeof(request))`. On failure: refund `fee` to the contract via `engine.refundFees(_currentContractId, fee)` and return `-1`.
6. Return the `invocationId` assigned by the engine (non-negative).

The call MUST be non-blocking.
The call MUST NOT register a notification callback.
The call MUST NOT mutate the calling contract's state.

### 6.4 Engine entry point

```
sint64 OcEngine::startContractInvocation(
    uint16_t contractIndex,
    uint16_t interfaceIndex,
    const void* requestData,
    uint16_t requestSize);
```

The implementation MUST:

1. Acquire the engine lock.
2. Validate `contractIndex < MAX_NUMBER_OF_CONTRACTS`, `interfaceIndex < OCI::ocInterfacesCount`, `requestSize == OCI::ocInterfaces[interfaceIndex].requestSize`. On failure: release lock and return `-1`.
3. Allocate a new invocation record. On failure (record-count cap or request byte budget exhausted): release lock and return `-1`.
4. Allocate an in-flight auth-state slot for the invocation (section 3.1.1). On failure (pool exhausted): release lock and return `-1`.
5. Zero-initialize the per-invocation storage region at `paramsOffset`.
6. Copy `requestData[0 .. requestSize)` into the per-invocation storage at `paramsOffset`.
7. Compute `paramsDigest = K12(pinnedBytes[0 .. requestSize))`.
8. Initialize the record with `status = OC_INVOCATION_STATUS_PENDING_AUTH`, `creationTick = system.tick`, `agreeingSigs = 0`, `signedBy = 0`, `signatures = 0`, `signerIndices = 0`.
9. Release the lock.
10. Return `invocationId`.

### 6.5 Status query

Contracts poll the consensus status of an invocation through the QPI function-call context:

```
inline uint8 QpiContextFunctionCall::getOcInvocationStatus(sint64 invocationId) const;
```

It returns one of the `OC_INVOCATION_STATUS_*` constants of section 5.1, or `OC_INVOCATION_STATUS_UNKNOWN` if the `invocationId` is not found (including invocations from previous epochs, whose records were dropped at `beginEpoch`).
Only the consensus status is exposed; node-local delivery status (section 5.2) is not reachable from this or any other public API.


## 7. Signature processing

### 7.1 On transaction execution

When a tick contains an `OcAuthSignatureTransaction`, for each contained `OcAuthSignatureItem` the engine MUST:

1. Locate the invocation record with the matching `invocationId`. If none exists, or its `epoch` does not match the item's `epoch`: discard the item.
2. If `record.status != OC_INVOCATION_STATUS_PENDING_AUTH` or `record.agreeingSigs >= QUORUM`: discard the item. This guards against appending signatures after the bundle is complete or the invocation has timed out.
3. Verify that `item.interfaceIndex == record.interfaceIndex` and `item.paramsDigest == record.paramsDigest`. On mismatch: discard the item.
4. Determine the computor index `c` for `sourcePublicKey` in the issuing epoch's `broadcastedComputors` (the epoch in `record.epoch`, which equals `item.epoch` after step 1). If `sourcePublicKey` is not a computor in that epoch: discard the item.
5. If `record.signedBy[c] == 1`: discard the item (duplicate).
6. Recompute `authMessage` per section 2.3 using the record's authoritative `epoch`, `interfaceIndex`, `invocationId`, `paramsDigest`.
7. Verify the SchnorrQ signature `item.signature` against `authMessage` under `sourcePublicKey`. On failure: discard the item.
8. Set `record.signedBy[c] = 1`. Increment `record.agreeingSigs`. Append `item.signature` to `record.signatures[agreeingSigs - 1]` and `c` to `record.signerIndices[agreeingSigs - 1]`.
9. If `record.agreeingSigs == QUORUM`:
   - Set `record.status = OC_INVOCATION_STATUS_AUTHORIZED`.
   - The bundle is now complete and eligible for delivery (section 8).

The engine MUST process items in order within the transaction.

### 7.2 On own QPI invocation

After `startContractInvocation` returns successfully, each computor MUST schedule an `OcAuthSignatureTransaction` for the invocation in a subsequent tick (mirroring `getReplyCommitTransaction` for OM).
The scheduling SHOULD batch multiple invocations into a single transaction up to the `itemCount` limit to reduce tick weight.

A computor MUST NOT emit a signature for any invocation it has not itself executed in tick processing.
A computor MUST NOT emit more than one signature per `(invocationId, ownPublicKey)` pair.


## 8. Delivery to OC machines

When an invocation transitions to `AUTHORIZED`, each Core node that has at least one OC machine peer configured (`ocMachineIPs` in `private_settings.h`) MUST construct an `OcMachineInvocation` message containing the full authorization bundle and enqueue it on every connected OC machine peer.
Delivery is not filtered by `interfaceIndex` on the Core side: the OC machine dispatches by `interfaceIndex` and ignores invocations for interfaces it does not serve.

`OcMachineInvocation` wire format:

```
struct OcMachineInvocation {
    static constexpr uint8 type();   // NetworkMessageType::OC_MACHINE_INVOCATION = 192

    sint64 invocationId;             // 8 bytes
    uint16 epoch;                    // 2 bytes
    uint16 interfaceIndex;           // 2 bytes
    uint16 requestSize;              // 2 bytes
    uint16 signatureCount;           // 2 bytes; MUST equal QUORUM
    // followed by: requestSize bytes of pinned OcRequest payload
    // followed by: signatureCount x SignerEntry
};

struct SignerEntry {
    uint16 computorIndex;            // 2 bytes; index into the epoch's computor list
    uint8  signature[64];            // 64 bytes, SchnorrQ signature over authMessage
};
static_assert(sizeof(SignerEntry) == 66);
```

The OC machine receives the request bytes followed by the `signatureCount` signer entries.
The `(computorIndex, signature)` pairs collectively form the authorization bundle that an external verifier or relayer can validate against the Qubic computor list for `OcMachineInvocation.epoch`, when an interface requires such external validation.

The signed computor list itself is not transmitted in `OcMachineInvocation`.
OC machines retrieve the `broadcastedComputors` for `OcMachineInvocation.epoch` via the standard peer protocol (the same mechanism any Qubic-aware client uses).
This keeps the per-invocation message size bounded and avoids duplicating the computor-list signature data on every invocation.

The OC machine MAY rely on the bundle being correct without re-verifying signatures itself, because the Core node only constructs and sends the bundle after the engine has confirmed `QUORUM` valid signatures.
For external verifiers (such as smart contracts on another chain), re-verification is expected and the included signatures and computor indices are sufficient.

### 8.1 Transport size

`OcMachineInvocation` is transmitted only over the private out-of-band channel from a Core node to its configured OC machine peers.
It is NOT broadcast through the public peer network and is NOT subject to the standard transaction or `BroadcastMessage` payload size limits (`MAX_INPUT_SIZE = 1024`).

The maximum message size, in bytes, is:

```
16 + MAX_OC_REQUEST_SIZE + QUORUM * sizeof(SignerEntry)
= 16 + 1008 + 451 * 66
= 30,790 bytes
```

The 16 bytes are the fixed header fields (`invocationId`, `epoch`, `interfaceIndex`, `requestSize`, `signatureCount`, including alignment).
Implementations of the OC machine channel MUST accept messages up to at least this size.

### 8.2 Delivery rules

Delivery is fire-once-and-forget. When an invocation transitions to `AUTHORIZED`, the node enqueues the bundle exactly once, and it is pushed to every OC machine peer that is connected at that moment. There is no per-peer receipt tracking and no re-delivery:

- The bundle is enqueued once per invocation and then the auth state (the only copy of the signatures) is freed.
- A peer that is disconnected at that moment does not receive the invocation and will not receive it later, even if it reconnects. If no OC peer is connected at all, the enqueue is retried a bounded number of times (waiting for any peer to become ready) and then dropped.
- The reference implementation therefore delivers to the subset of configured OC peers that happen to be connected when the invocation is authorized; it does not guarantee delivery to every configured peer.

Because a contract obligates every OC machine for the interface to act, and external effects are required to be idempotent (see `contracts_outsourced_computations.md`), a single connected peer acting on the bundle is sufficient for the external effect. Interfaces that need stronger delivery guarantees must build them at the interface layer (for example, by having the contract re-issue the invocation).

There is no response message defined for `OcMachineInvocation`.
The protocol provides no return channel from the OC machine through this transport.


## 9. Interface declaration

Each OC interface MUST be declared as a `struct` in a header file in `src/oc_interfaces/` with the following mandatory members:

```
struct InterfaceName
{
    static constexpr uint32 ocInterfaceIndex = OC_INTERFACE_INDEX;

    struct OcRequest {
        /* fields */
    };

    static sint64 getInvocationFee(const OcRequest& request);
};
```

Constraints:

- `sizeof(OcRequest)` MUST be `<= MAX_OC_REQUEST_SIZE`.
- `OcRequest` MUST be a trivially-copyable, standard-layout struct.
- `getInvocationFee` MUST return a value `>= MIN_OC_INVOCATION_FEE`.
- `getInvocationFee` MUST be deterministic: equal inputs MUST produce equal outputs, and the function MUST NOT read any global state.

Each interface MUST be registered in `src/oc_core/oc_interfaces_def.h` with a unique sequential `OC_INTERFACE_INDEX` starting at 0.


## 10. Fee handling

The OC invocation fee is burned.
The Core node MUST log the burn as a `QuTransfer` from the calling contract's ID to `NULL_ID`.
The fee MUST NOT be added to the contract's execution reserve.

If `__qpiInvokeOC` fails after the fee has been deducted, the fee MUST be refunded to the contract via `engine.refundFees(_currentContractId, fee)`.
If `__qpiInvokeOC` fails before the fee deduction, no refund is required.


## 11. Cross-epoch behavior

The engine retains invocation records throughout the epoch in which they were created and discards them at `beginEpoch` of the next epoch.
This matches OM's behavior in `OracleEngine::beginEpoch`.

Within an epoch:

- Signatures with an `epoch` field not matching the current epoch MUST be discarded during signature processing.
- An invocation that fails to reach quorum before the configured timeout transitions to `TIMEOUT` and the fee is not refunded (the fee was burned at QPI call time and represents the cost of the attempted authorization).

Across an epoch boundary:

- Invocations that reached `AUTHORIZED` before the boundary remain valid externally: the bundle's signatures verify against the corresponding epoch's computor list, which remains queryable.
- Invocations in `PENDING_AUTHORIZATION` at the boundary are dropped. Contracts that need delivery guarantees near epoch boundaries SHOULD re-issue the invocation in the new epoch.

OC interfaces whose external state binds to the active computor set (for example, a Bitcoin multisig wallet) MUST document their cross-epoch migration procedure.
The protocol provides no automatic migration mechanism but does not prevent interfaces from defining one.


## 12. Snapshot and restore

The OC engine MUST include the following in node snapshots:

- All invocation records, including pinned request payloads, consensus status, and the `signedBy` bitmap.
- For each record whose bundle is still retained at snapshot time, the accumulated signatures and signer indices. Implementations that adopt the discard-after-delivery optimization of section 3.1.1 MAY omit the bundle for records whose bundle has been freed; the metadata MUST still be included.
- The per-tick sequence counter state used by invocation ID derivation (section 2.1), so that subsequent ID assignments after restart match a node that has not restarted.

The OC engine MUST NOT include node-local delivery state (the per-peer delivery bookkeeping of section 5.2) in snapshots.

On snapshot load:

- The engine reconstructs the invocation table such that subsequent signature processing produces results identical to a node that has not restarted.
- Delivery state is empty after load.
- For `AUTHORIZED` invocations whose bundle was retained in the snapshot (i.e. delivery had not yet been attempted before the snapshot), the node SHOULD treat them as candidates for delivery to its configured OC peers.
- For `AUTHORIZED` invocations whose bundle was already discarded before the snapshot, the node cannot construct an `OcMachineInvocation` and MUST skip re-delivery. The bundle is freed after the single fire-once delivery attempt, so a restart cannot re-attempt a delivery that was already attempted; whether that attempt reached every configured peer is not tracked, consistent with the best-effort semantics of section 8.2.
- A node MAY skip re-delivery of any `AUTHORIZED` invocation, relying on either other Core nodes to deliver or the receiving OC machine to deduplicate by `invocationId`. External deduplication (required by the constraints in `contracts_outsourced_computations.md`) ensures at-most-once external effect across all redundant deliveries.


## 13. Open items

- Whether signature aggregation (BLS or similar) is worth pursuing to reduce per-invocation tx weight.
- Scaling beyond ~2M invocations per epoch requires amortizing the per-invocation quorum cost, e.g. per-tick batch authorization: computors sign one Merkle root over all invocations created in a tick, and the bundle becomes 451 signatures per tick plus a Merkle path per invocation. This changes the authorization transaction format and the OC machine protocol; not needed at the current cap.
- Whether a seamless cross-epoch transition mechanism for `PENDING_AUTHORIZATION` records is desired.
