# General Workflow of Custom Mining

- A solution's nonce must satisfy `nonce % NUMBER_OF_COMPUTORS == computorID` to be considered valid for a specific computor.
- Custom mining only occurs during the **idle phase** (solution submission, verification, etc.).
- **Accepted Solutions** = Total Received Solutions - Invalid Solutions ( or **Accepted Solutions** = non-verified Solutions + Valid Solutions).

## During the Idle State (Custom Mining Active)
- The node receives broadcast messages and propagates them across the network.
- It verifies whether each message is a **task from the DISPATCHER** or a **solution from a COMPUTOR**.
- **Tasks** are stored for future querying.
- **Solutions** are also stored, checked for duplication, and can be queried later.
- Ownership of a solution is determined by `nonce % NUMBER_OF_COMPUTORS`.

## At the End of Idle State
- The node aggregates all valid solutions it has seen.
- It packs them following the **vote counter design**, embeds this into a transaction, and broadcasts it.
- **Each computor is limited to 1023 solutions per phase** due to transaction size limits. 
  If this is exceeded, the console logs a message for the DISPATCHER to adjust the difficulty.

## When a Node Receives a Custom Mining Transaction
- It accumulates the received solution counts into its internal counter.

## At the End of the Epoch
- The node calculates revenue scores:
  - Sum all counted solutions.
  - Multiply the sum by the current revenue score.
  - Apply the formula again to determine each computor’s final revenue.

---

# Solution Verification

- A **verifier** requests data from the node using `RequestedCustomMiningData`, authenticated with the **OPERATOR's seed** (`core/src/network_message/custommining.h`).
- First, it requests a **range of tasks** using their `taskIndex`.
- Then, using these indices, it requests **associated solutions**.
- The node responds with all **unverified solutions** related to those tasks (max 1MB) using `RespondCustomMiningData`.
- The verifier validates these solutions and reports results using `RequestedCustomMiningSolutionVerification`.
- The node updates the solutions' **verified** status and marks them **valid** or **invalid**.

**Reference Verifier**: [`oc_verifier`](https://github.com/qubic/outsourced-computing/tree/main/monero-poc/verifier)

---

# Core API Overview

## `processBroadcastMessage`
- If the destination public key is all-zero:
  - **From Dispatcher**:
    - Decode the gamming key to determine task type.
    - If it’s a `CUSTOM_MINING_MESSAGE_TYPE` and idle state is active, store the task.
  - **From Computor**:
    - Decode the gamming key.
    - If it's a custom mining solution:
      - Check for duplicates in cache.
      - If new, store the solution and update stats.

## `processRequestCustomMiningData`
- Verify the OPERATOR signature.
- Determine requested data type: **Tasks (`taskType`)** or **Solutions (`solutionType`)**.

### `taskType`
- Extract task data in the requested `taskIndex` range.
- Respond with packed task data or empty response if none found.

### `solutionType`
- Extract solutions related to the given `taskIndex`.
- Iterate through them:
  - Only send solutions that are **not yet verified** and match the task/nonce.
  - Respond with those solutions or an empty response if none are found.

## `processRequestedCustomMiningSolutionVerificationRequest`
- Accept solutions if **valid** or **not yet verified**.
- Only process if in the custom mining state.
- Mark the solution as verified, with valid/invalid status.
- If the solution exists and is invalid:
  - Decrement the computor’s share count.
  - Set response status to `invalid`.
- Respond with the verification status of the solution.