# Contract Execution Fees

## Overview

Every smart contract in Qubic has an **execution fee reserve** that determines whether the contract can execute its procedures. This reserve is stored in Contract 0's state and is initially funded during the contract's IPO (Initial Public Offering). Contracts must maintain a positive execution fee reserve to remain operational.


## Fee Management

Each contract's execution fee reserve is stored in Contract 0's state in an array `contractFeeReserves[MAX_NUMBER_OF_CONTRACTS]`. Access to a contract's reserve is provided via the `contractFeeReserve(contractIndex)` helper function.

When a contract's IPO completes, the execution fee reserve is initialized based on the IPO's final price. If the `finalPrice > 0`, the reserve is set to `finalPrice * NUMBER_OF_COMPUTORS` (676 computors). However, if the IPO fails and `finalPrice = 0`, the contract is marked as failed with `ContractErrorIPOFailed` and the reserve remains 0.

Contracts can refill their execution fee reserves in the following ways:

- **Contract internal burning**: Any contract procedure can burn its own QU using `qpi.burn(amount)` to refill its own reserve, or `qpi.burn(amount, targetContractIndex)` to refill another contract's reserve.
- **External refill via QUtil**: Anyone can refill any contract's reserve by sending QU to the QUtil contract's `BurnQubicForContract` procedure with the target contract index. All sent QU is burned and added to the target contract's reserve.
- **Legacy QUtil burn**: QUtil provides a `BurnQubic` procedure that burns to QUtil's own reserve specifically.

The execution fee system follows a key principle: **"Who Initiates Execution Pays"**. When a user initiates a transaction, the user's destination contract must have a positive executionFeeReserve. When a contract initiates an operation (including any callbacks it triggers), that contract must have positive executionFeeReserve. However, when the system initiates operations (system procedures, revenue donations, IPO refunds), no fees are required.

Currently, execution fees are checked (contracts must have `executionFeeReserve > 0`) but **not yet deducted** based on actual computation. Future implementation will measure execution time and resources per procedure call, deduct proportional fees from the reserve, and implement dynamic fee rates based on network load.

## What Operations Require Fees

The execution fee system checks whether a contract has positive `executionFeeReserve` at different entry points. The table below summarizes when fees are checked and who pays:

| Entry Point | Initiator | executionFeeReserve Checked | Code Location |
|------------|-----------|----------------------------|---------------|
| System procedures (`INITIALIZE`, `BEGIN_EPOCH`, etc.) | System | ✅ Contract must have fees | qubic.cpp |
| User procedure call | User | ✅ Contract must have fees | qubic.cpp |
| Contract-to-contract procedure | Contract A | ✅ Only caller (A) must have fees | contract_exec.h |
| Contract-to-contract transfer (`POST_INCOMING_TRANSFER`) | Contract A | ❌ Only caller (A) checked | qpi_spectrum_impl.h → contract_exec.h |
| Revenue donation (`POST_INCOMING_TRANSFER`) | System | ❌ Not checked | qubic.cpp |
| IPO refund (`POST_INCOMING_TRANSFER`) | System | ❌ Not checked | ipo.h |
| User functions | User | ❌ Never checked (read-only) | N/A |

**System procedures** (`INITIALIZE`, `BEGIN_EPOCH`, `END_EPOCH`, `BEGIN_TICK`, `END_TICK`) require the contract to have `executionFeeReserve > 0`. If the reserve is depleted, these procedures are skipped and the contract becomes dormant.

**User procedure calls** check the contract's execution fee reserve before execution. If `executionFeeReserve <= 0`, the transaction fails and any attached amount is refunded to the user. If the contract has fees, the procedure executes normally and may trigger `POST_INCOMING_TRANSFER` callback first if amount > 0.

**User functions** (read-only queries) are always available regardless of executionFeeReserve. They are defined with `PUBLIC_FUNCTION()` or `PRIVATE_FUNCTION()` macros, provide read-only access to contract state, and cannot modify state or trigger procedures.

**Contract-to-contract callbacks** execute without checking the recipient's fees. When Contract A calls Contract B (via `qpi.transfer()`, `INVOKE_OTHER_CONTRACT_PROCEDURE`, or asset transfers), only Contract A needs positive executionFeeReserve. This applies to all callbacks: `POST_INCOMING_TRANSFER`, `PRE_ACQUIRE_SHARES`, `POST_ACQUIRE_SHARES`, `PRE_RELEASE_SHARES`, and `POST_RELEASE_SHARES`. Contract A initiated the operation, so Contract A pays for all resulting execution including callbacks in Contract B.

Example: Contract A (executionFeeReserve = 1000) transfers 500 QU to Contract B (executionFeeReserve = 0). Contract A's procedure executes, money transfers from A to B, and Contract B's `POST_INCOMING_TRANSFER` executes—all paid for by Contract A's fees.

**System-initiated transfers** (revenue donations and IPO refunds) do not require the recipient contract to have positive executionFeeReserve. The `POST_INCOMING_TRANSFER` callback executes regardless of the destination's reserve status. These are system-initiated transfers that contracts didn't request, so contracts should be able to receive system funds even if dormant.

## Best Practices

### For Contract Developers

1. **Plan for sustainability**: Design fee collection mechanisms (e.g., service fees, transaction fees)
2. **Burn collected fees**: Regularly call `qpi.burn()` to replenish executionFeeReserve
3. **Monitor reserve**: Implement a function to expose current reserve level
4. **Graceful degradation**: Consider what happens when reserve runs low

### For Contract Users

1. **Check contract status**: Before using a contract, verify it has positive executionFeeReserve
2. **Transaction failures**: If your transaction fails due to insufficient fees, the attached amount will be automatically refunded
3. **No funds lost**: The system ensures amounts are refunded if a contract cannot execute