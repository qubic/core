## Compliance
- Result: Contract compliance check PASSED
- Tool: Qubic Contract Verification Tool (`qubic-contract-verify`)

## Test Execution
- Command: `test.exe --gtest_filter=VottunBridge*`
- Result: 14 tests passed (0 failed)
- Tests (scope/expected behavior):
  - `CreateOrder_RequiresFee`: rejects creation if the paid fee is below the required amount.
  - `TransferToContract_RejectsMissingReward`: with pre-funded contract balance, a call without attached QUs fails and does not change balances or `lockedTokens`.
  - `TransferToContract_AcceptsExactReward`: accepts when `invocationReward` matches `amount` and locks tokens.
  - `TransferToContract_OrderNotFound`: rejects when `orderId` does not exist.
  - `TransferToContract_InvalidAmountMismatch`: rejects when `input.amount` does not match the order amount.
  - `TransferToContract_InvalidOrderState`: rejects when the order is not in the created state.
  - `CreateOrder_CleansCompletedAndRefundedSlots`: cleans completed/refunded slots to allow a new order.
  - `CreateProposal_CleansExecutedProposalsWhenFull`: cleans executed proposals to free slots and create a new one.
  - `CreateProposal_InvalidTypeRejected`: rejects out-of-range proposal types.
  - `ApproveProposal_NotOwnerRejected`: blocks approval when invocator is not a multisig admin.
  - `ApproveProposal_DoubleApprovalRejected`: prevents double approval by the same admin.
  - `ApproveProposal_ExecutesChangeThreshold`: executes the proposal once the threshold is reached.
  - `ApproveProposal_ProposalNotFound`: rejects approval for a non-existent `proposalId`.
  - `ApproveProposal_AlreadyExecuted`: rejects approval for a proposal already executed.