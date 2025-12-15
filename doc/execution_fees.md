# Contract Execution Fees

## Overview

Every smart contract in Qubic has an **execution fee reserve** that determines whether the contract can execute its procedures. This reserve is stored in Contract 0's state and is initially funded during the contract's IPO (Initial Public Offering). Contracts must maintain a positive execution fee reserve to remain operational. It is important to note that these execution fees are different from the fees a user pays to a contract upon calling a procedure. To avoid confusion we will call the fees a user pays to a contract 'invocation reward' throughout this document.



## Fee Management

Each contract's execution fee reserve is stored in Contract 0's state in an array `contractFeeReserves[MAX_NUMBER_OF_CONTRACTS]`. The current value of the executionFeeReserve can be queried with the function `qpi.queryFeeReserve(contractIndex)` and returns a `sint64`.

When a contract's IPO completes, the execution fee reserve is initialized based on the IPO's final price. If the `finalPrice > 0`, the reserve is set to `finalPrice * NUMBER_OF_COMPUTORS` (676 computors). However, if the IPO fails and `finalPrice = 0`, the contract is marked as failed with `ContractErrorIPOFailed` and the reserve remains 0 and can't be filled anymore. A contract which failed the IPO will remain unusable.

Contracts can refill their execution fee reserves in the following ways:

- **Contract internal burning**: Any contract procedure can burn its own QU using `qpi.burn(amount)` to refill its own reserve, or `qpi.burn(amount, targetContractIndex)` to refill another contract's reserve.
- **External refill via QUtil**: Anyone can refill any contract's reserve by sending QU to the QUtil contract's `BurnQubicForContract` procedure with the target contract index. All sent QU is burned and added to the target contract's reserve.
- **Legacy QUtil burn**: QUtil provides a `BurnQubic` procedure that burns to QUtil's own reserve specifically.

The execution fee system follows a key principle: **"The Contract Initiating Execution Pays"**. When a user initiates a transaction, the user's destination contract must have a positive executionFeeReserve. When a contract initiates an operation (including any callbacks it triggers), that contract must have positive executionFeeReserve.

Currently, execution fees are checked (contracts must have `executionFeeReserve > 0`) but **not yet deducted** based on actual computation. Future implementation will measure execution time and resources per procedure call, deduct proportional fees from the reserve.

## What Operations Require Execution Fees

The execution fee system checks whether a contract has positive `executionFeeReserve` at different entry points. The table below summarizes when fees are checked and who pays:

| Entry Point | Initiator | executionFeeReserve Checked | Code Location |
|------------|-----------|----------------------------|---------------|
| System procedures (`BEGIN_TICK`, `END_TICK`, etc.) | System | ✅ Contract must have positive reserve | qubic.cpp |
| User procedure call | User | ✅ Contract must have positive reserve | qubic.cpp |
| Contract-to-contract procedure | Contract A | ✅ Called contract (B) must have positive reserve, otherwise error is returned to caller | contract_exec.h |
| Contract-to-contract function | Contract A | ✅ Called contract (B) must have positive reserve, otherwise error is returned to caller | contract_exec.h |
| Contract-to-contract callback (`POST_INCOMING_TRANSFER`, etc.) | System | ❌ Not checked (callbacks execute regardless of reserve) | contract_exec.h |
| Epoch transistion system procedures (`BEGIN_EPOCH`, `END_EPOCH`) | System | ❌ Not checked | qubic.cpp |
| Revenue donation (`POST_INCOMING_TRANSFER`) | System | ❌ Not checked | qubic.cpp |
| IPO refund (`POST_INCOMING_TRANSFER`) | System | ❌ Not checked | ipo.h |
| User functions | User | ❌ Never checked (read-only) | N/A |

**Basic system procedures** (`BEGIN_TICK`, `END_TICK`) require the contract to have `executionFeeReserve > 0`. If the reserve is depleted, these procedures are skipped and the contract becomes dormant. These procedures are invoked by the system directly.

**Epoch transistion system procedures**  `BEGIN_EPOCH`, `END_EPOCH` are executed even with a non-positive `executionFeeReserve` to keep contract state in a valid state.

**User procedure calls** check the contract's execution fee reserve before execution. If `executionFeeReserve <= 0`, the transaction fails and any attached amount is refunded to the user. If the contract has fees, the procedure executes normally and may trigger `POST_INCOMING_TRANSFER` callback first if amount > 0.

**User functions** (read-only queries) are always available regardless of executionFeeReserve. They are defined with `PUBLIC_FUNCTION()` or `PRIVATE_FUNCTION()` macros, provide read-only access to contract state, and cannot modify state or trigger procedures.

**Contract-to-contract procedure calls** via `INVOKE_OTHER_CONTRACT_PROCEDURE` check that the **called contract (B) has positive executionFeeReserve**. If Contract B has insufficient fees (`executionFeeReserve <= 0`), the call fails and returns `CallErrorInsufficientFees` to Contract A. The procedure is not executed, and Contract A can check the error via the `interContractCallError` variable (or a custom error variable when using `INVOKE_OTHER_CONTRACT_PROCEDURE_E`). Contract developers should check the error after invoking procedures or proactively verify the called contract's fee reserve with `qpi.queryFeeReserve(contractIndex)` before invoking it.

**Contract-to-contract function calls** via `CALL_OTHER_CONTRACT_FUNCTION` also check that the **called contract (B) has positive executionFeeReserve**. If Contract B has insufficient fees or is in an error state, the call fails and returns `CallErrorInsufficientFees` or `CallErrorContractInErrorState` to Contract A. The function is not executed, and Contract A can check the error via the `interContractCallError` variable (or a custom error variable when using `CALL_OTHER_CONTRACT_FUNCTION_E`). This graceful error handling allows Contract A to continue execution and handle failures appropriately.

**Contract-to-contract callbacks** (`POST_INCOMING_TRANSFER`, `PRE_ACQUIRE_SHARES`, `POST_ACQUIRE_SHARES`, etc.) are system-initiated and **do not check executionFeeReserve**. These callbacks execute regardless of the called contract's fee reserve status, allowing contracts to receive system-initiated transfers and notifications even when dormant. This design ensures that contracts can receive revenue donations, IPO refunds, and other system transfers without requiring positive fee reserves.

Example: Contract A (executionFeeReserve = 1000) transfers 500 QU to Contract B (executionFeeReserve = 0) using `qpi.transfer()`. The transfer succeeds and Contract B's `POST_INCOMING_TRANSFER` callback executes regardless of Contract B having no fees, because the callback is system-initiated. However, if Contract A tries to invoke a procedure of Contract B using `INVOKE_OTHER_CONTRACT_PROCEDURE`, the call will fail and return `CallErrorInsufficientFees`. Contract A should check `interContractCallError` after the call and handle the error gracefully (e.g., skip the operation or use fallback logic).

**System-initiated transfers** (revenue donations and IPO refunds) do not require the recipient contract to have positive executionFeeReserve. The `POST_INCOMING_TRANSFER` callback executes regardless of the destination's reserve status. These are system-initiated transfers that contracts didn't request, so contracts should be able to receive system funds even if dormant.

## Best Practices

### For Contract Developers

1. **Plan for sustainability**: Charge invocation rewards for running user procedures
2. **Burn collected invocation rewards**: Regularly call `qpi.burn()` to replenish executionFeeReserve
3. **Monitor reserve**: Implement a function to expose current reserve level
4. **Graceful degradation**: Consider what happens when reserve runs low
5. **Handle inter-contract call errors**: After using `INVOKE_OTHER_CONTRACT_PROCEDURE`, check the `interContractCallError` variable to verify the call succeeded. Handle errors gracefully (e.g., skip operations, use fallback logic). You can also proactively verify the called contract has positive `executionFeeReserve` using `qpi.queryFeeReserve(contractIndex) > 0` before calling.

### For Contract Users

1. **Check contract status**: Before using a contract, verify it has positive executionFeeReserve
2. **Transaction failures**: If your transaction fails due to insufficient execution fees reserve, the attached amount will be automatically refunded
3. **No funds lost**: The system ensures amounts are refunded if a contract cannot execute
