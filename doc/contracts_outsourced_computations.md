# Outsourced Computations from contracts

Outsourced Computations (OC) let Smart Contracts push intent out of Qubic to be executed by an external Processor. Only contracts can invoke OC; users can only trigger it indirectly, by calling a contract that does.
Where an Oracle Machine retrieves information from outside Qubic and makes it available on-chain, an OC carries an authorized request out of Qubic so that an external system can act on it.
The protocol defines how requests are expressed and authorized.
What happens at the external system is the responsibility of the Processor and the interface.

OC invocations are subject to a fee to avoid spam.
The fee is burned.

OC machines (the counterpart to OM nodes) are run by computor operators.
Each operator connects their Core node to one or more OC machines via a private, out-of-band channel, mirroring the OM topology.


## Invocation lifecycle

When a contract initiates an OC invocation, the following happens:

1. The fee is deducted and burned (if an error occurs during the start of the invocation, the fee is refunded),
2. An invocation ID is generated that allows to track the invocation,
3. The invocation parameters are pinned: they are recorded deterministically as a side effect of tick execution, so all computors see identical inputs.

The call returns immediately with the invocation ID; the invoking procedure continues normally, and authorization proceeds asynchronously over the following ticks.

After the invocation has been recorded, each computor independently emits an OC authorization signature transaction signing `(epoch, interfaceIndex, invocationId, paramsDigest)`.
When 451 valid, agreeing signatures have been processed, the invocation becomes AUTHORIZED.
If an invocation does not reach 451 signatures within `OC_INVOCATION_TIMEOUT_DEFAULT_TICKS` (3 ticks), it transitions to TIMEOUT; the fee is not refunded.

Once an invocation is AUTHORIZED, each node with OC machine peers configured assembles an authorization bundle and enqueues it to all of its OC machine peers over the private channel.
The authorization bundle is a single message containing the pinned request parameters and the 451 `(computor index, signature)` pairs.
Everything in it derives from on-chain data (the pinned parameters and the authorization signature transactions), so a recipient can verify the signatures against the epoch's signed computor list without trusting the forwarding node.
The OC machine performs the work against the external system.
Because the bundle is only constructed and forwarded after the engine has confirmed 451 valid signatures, the OC machine does not need to re-verify the quorum.

The protocol does not provide a return path through OC.
If the contract needs to observe whether the work was performed, it does so via a subsequent Oracle query.


## OC interfaces

An OC interface defines the input type used to describe a unit of work: `OcRequest`.
There is no reply type, because OC does not return information through the protocol.
Multiple Processors may implement the same interface; multiple OC interfaces differ in their definition of `OcRequest` because they target different external systems and require different parameters.

The source code of OC interfaces resides in the directory `src/oc_interfaces/` of the Qubic Core repository, with one header file per interface.
A central definition file `src/oc_core/oc_interfaces_def.h` includes all interface headers and assigns each one a unique `OC_INTERFACE_INDEX`, exactly as `src/oracle_core/oracle_interfaces_def.h` does for OM interfaces.

In Qubic contracts, OC interfaces are made available through the namespace `OCI`.
For example, you can access the request struct type of the `Mock` interface via `OCI::Mock::OcRequest`.

Each interface header defines a struct with three mandatory members:

```
struct InterfaceName
{
    /// Interface index, assigned in oc_interfaces_def.h
    static constexpr uint32 ocInterfaceIndex = OC_INTERFACE_INDEX;

    /// Data sent with each invocation; pinned at QPI call time
    struct OcRequest
    {
        /* interface-specific fields */
    };

    /// Per-invocation fee in QU, burned when invokeOC is called
    static sint64 getInvocationFee(const OcRequest& request);
};
```

The size of `OcRequest` is bounded by `MAX_OC_REQUEST_SIZE`.
The `OcRequest` struct must be deterministically serializable: every computor pins and hashes the same bytes when computing `paramsDigest`.

Interfaces may also expose helper functions or constants used by contracts to build a valid `OcRequest`, in the same style as the `Price` OM interface exposes oracle identifier helpers.


## QPI surface

Contracts trigger an OC invocation through a single QPI procedure call, wrapped in a macro for symmetry with `QUERY_ORACLE`:

```
#define INVOKE_OC(OcInterface, request) qpi.__qpiInvokeOC<OcInterface>(request)
```

The internal entry point on the QPI context is:

```
template <typename OcInterface>
inline sint64 QpiContextProcedureCall::__qpiInvokeOC(
    const typename OcInterface::OcRequest& request
) const;
```

Behavior:

1. Validates that the calling contract index matches the QPI context and that the interface index is known.
2. Computes the invocation fee via `OcInterface::getInvocationFee(request)` and attempts to deduct it from the contract's spectrum balance. The fee is burned (logged as a `QuTransfer` to `NULL_ID`). If the balance is insufficient, returns `-1` and the call has no other effect.
3. Allocates a fresh `invocationId` and copies the request bytes into the engine's per-invocation storage. This is the parameter pinning step.
4. Records the invocation in the engine; subsequent tick processing produces the authorization signature transactions as described in the lifecycle section.

If an error occurs during step 3 or 4 (for example, storage exhaustion), the fee deducted in step 2 is refunded to the contract.
This matches the fee reimbursement behavior of `QUERY_ORACLE`.

Return value: the `invocationId` on success (a non-negative `sint64`), or `-1` on any failure.
The call is non-blocking, has no notification callback, and produces no further effect on the contract's state.

The consensus status of an invocation (pending authorization, authorized, or timed out) can be polled with `qpi.getOcInvocationStatus(invocationId)`, which returns one of the `OC_INVOCATION_STATUS_*` constants.
To observe whether the external work happened, the contract issues an Oracle query against an interface that reports the relevant external state.


## Mock OC interface

The implementation ships an OC interface named `Mock`, analogous to the OM `Mock` interface. Authorized `Mock` invocations are forwarded to a mock interface service that verifies the computor signatures and publishes the received invocations.

```
// src/oc_interfaces/Mock.h
using namespace QPI;

/**
* OC interface "Mock".
*
* The mock interface accepts an arbitrary 64-bit value as request payload. OC machines
* forward the authorization bundle to a mock interface service that verifies the computor
* signatures and publishes the received invocations.
*/
struct Mock
{
    /// OC interface index
    static constexpr uint32 ocInterfaceIndex = OC_INTERFACE_INDEX;

    /// OC request data / input to the OC machine
    struct OcRequest
    {
        /// Arbitrary value forwarded to the mock interface service
        uint64 value;
    };

    /// Return invocation fee; constant for the mock
    static sint64 getInvocationFee(const OcRequest& request)
    {
        return 10;
    }
};
```

Registration in `src/oc_core/oc_interfaces_def.h`:

```
#define OC_INTERFACE_INDEX 0
#include "oc_interfaces/Mock.h"
#undef OC_INTERFACE_INDEX

constexpr struct {
    unsigned long long requestSize;
} ocInterfaces[] = {
    DEFINE_OC_INTERFACE(Mock),
};
```

A contract using the mock interface:

```
struct MockTriggerContract : public ContractBase
{
protected:
    sint64 _lastInvocationId;

public:
    struct TriggerOC_input
    {
        uint64 value;
    };

    struct TriggerOC_output
    {
        sint64 invocationId;
    };

    struct TriggerOC_locals
    {
        OCI::Mock::OcRequest request;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TriggerOC)
    {
        // Zero-initialize the request so padding bytes hashed into paramsDigest
        // are deterministic across all computors. Mirrors the OM reply convention
        // (see core_om_network_messages.h: "replies must be set all-0 before
        // setting member data, making sure that alignment/padding bytes are
        // initialized with 0"). setMemory is the contract-safe zero-init helper
        // (contracts cannot call the core setMem directly).
        setMemory(locals.request, 0);
        locals.request.value = input.value;
        output.invocationId = INVOKE_OC(OCI::Mock, locals.request);
        state._lastInvocationId = output.invocationId;
    }
};
```

Test plan for the mock:

1. A test SC procedure calls `INVOKE_OC(OCI::Mock, request)` with a known value. The test verifies that the contract's QU balance decreased by the invocation fee and the return value is a valid `invocationId`.
2. Tick processing produces 451 `OcAuthSignatureTransaction` entries; the engine status of the invocation transitions to `OC_INVOCATION_STATUS_AUTHORIZED`.
3. An OC machine peer receives the authorization bundle on the private channel and forwards the raw bundle bytes to the mock interface service.
4. The mock interface service verifies the computor signatures, deduplicates by `invocationId`, and publishes the invocation with `request.value`, confirming end-to-end flow.

The mock interface is the baseline test used to exercise new OC interfaces and should be wired into the test target alongside the OM Mock tests.


## Transactions

OC introduces one new transaction type:

`OcAuthSignatureTransaction` is emitted by a computor to sign an authorization for one or more invocations.
The transaction prefix is followed by one or more items, each carrying `(invocationId, interfaceIndex, epoch, paramsDigest, signature)`.
The engine deduplicates by signer pubkey and counts agreeing signatures per invocation.
On the 451st signature for a given invocation, the invocation transitions to AUTHORIZED.


## Network channel

Forwarding authorized invocations from a Core node to its OC machine peer uses the same private out-of-band connection pattern as OM.
The connection is outgoing from the Core node, permanently kept open, and the OC machine implements IP whitelisting to accept only its configured Core nodes.

The message sent on this channel carries the authorization bundle: the pinned parameters and the 451 computor signatures with their signer indices.
The bundle is constructed by the Core node only after the engine has confirmed 451 valid signatures, so the OC machine can act on it directly without re-checking the quorum.
OC machines retrieve the signed computor list for the bundle's issuing epoch via the standard peer protocol when they need it for external verification (for example, when forwarding signatures to a verifier contract on another chain).


## Cross-epoch maintenance

OC authorizations are bound to the computor set that signed them.
For interfaces whose external state depends on a fixed signer set (for example, a Bitcoin multisig wallet whose script lists the computors' public keys), continuity across epochs requires explicit operational housekeeping.
Before the active computor set rotates, an OC invocation is used to move the external state to a configuration controlled by the new set.
For Bitcoin this means moving funds from the current multisig address to a new one with the incoming signers.
Without this step, the external state is permanently inaccessible from the new epoch.

For external systems that can validate the Qubic computor list for the bundle's issuing epoch dynamically at the time the action is performed, no per-epoch migration is required.

OC interfaces should document which model they require and, where migration is needed, expose an interface-specific invocation for performing it.


## Constraints on interface design

OC does not deduplicate at the protocol level.
Because every computor that has configured an OC machine peer for an interface independently forwards each authorized invocation, an authorized invocation may be acted upon by every running OC machine for that interface.
The external system must tolerate this.

External systems with natural deduplication satisfy the constraint: Bitcoin (UTXO consumption), Ethereum and other EVM chains (account nonces), HTTP APIs with idempotency keys.
External systems without natural deduplication (general-purpose HTTP POST endpoints, real-world fulfillment that does not check for duplicate orders) do not satisfy the constraint and should not be exposed through OC.
