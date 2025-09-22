# Smart Contract Development

For a general introduction to Smart Contracts in Qubic, we recommend to read [the Qubic Docs](https://docs.qubic.org/learn/smart-contracts).

This developer documentation is structured as follows:

1.  [Overview](#overview)
1.  [Development](#development)
1.  [Review and tests](#review-and-tests)
1.  [Deployment](#deployment)
1.  [Procedures and Functions](#procedures-and-functions)
1.  [Assets and shares](#assets-and-shares)
1.  [Other QPI features](#other-qpi-features)
1.  [Restrictions of C++ Language Features](#restrictions-of-c-language-features)
1.  [General Change Management](#general-change-management)
1.  [Interface in qubic-cli](#interface-in-qubic-cli)

## Overview

In Qubic, smart contracts are implemented in a restricted variant of C++ and compiled into the Qubic Core executable.

In order to isolate contracts, access to functions and data of other contracts and Core internals are only possible through a programming interface called QPI.
The QPI is the only external dependency available for developing a contract. That is, using libraries is forbidden.
Further, contracts cannot use C++ features that are known for imposing security risks, such as pointers, low-level arrays (which lack checking of bounds), and preprocessor directives.
A contract also never gets access to uninitialized memory (all memory is initialized with zeros).

Each contract is implemented in one C++ header file in the directory `src/contracts`.

A contract has a state struct, containing all its data as member variables.
The memory available to the contract is allocated statically, but extending the state will be possible between epochs through special `EXPAND` events.

The state struct also includes the procedures and functions of the contract, which have to be defined using special macros such as `PUBLIC_PROCEDURE()`, `PRIVATE_FUNCTION()`, or `BEGIN_EPOCH()`.
Functions cannot modify the state, but they are useful to query information with the network message `RequestContractFunction`.
Procedures can modify the state and are either invoked by special transactions (user procedures) or internal core events (system procedures).

Contract developers should be aware of the following parts of the Qubic protocol that are not implemented yet in the core:
- Execution of contract procedures will cost fees that will be paid from its contract fee reserve.
  This reserve is initially filled with the QUs burned during the IPO of the contract and refilled by additional burning of QUs happening in the contract.
  In the long run, each contract needs to burn QUs to stay alive.
  If a contract's execution fee reserve runs empty, the contract will not be executed anymore.
- In the future, the issuer of an asset, more specifically the managing contract, will have to pay for entries in the ledger.
  This is why it is a good idea to collect fees when issuing an asset.


## Development

In order to develop a contract, follow these steps:

1. Write a description of your contract, about what it is supposed to do.
   We recommend to discuss it with others, for example the Qubic community on Discord.
2. Think about a name of your contract and check that it is not used yet.
   You will need a long version such as `YourContractName` for naming the file,
   a short version of at most 7 capital letters (or digits after the first letter) such as `YCN` ([asset name](#assets-and-shares) of the contract shares),
   and optionally a longer capital-letter name for the state struct and as a prefix for global constants of the contract, such as `YOURCONTRACTNAME`.
   An example: The file is called `Quottery.h`, the asset name of the contract shares is `QTRY`, and the name of the state struct is `QUOTTERY`.
   Another example: Filename `Qx.h`, asset name `QX`, state struct name `QX`.
3. Fork the main repository and create your own branch based on the current `develop` branch.
   Use the name of the contract to name your git branch, ideally name it: `feature/YYYY-MM-DD-YourContractName` (replace YYYY-MM-DD with the current date, that is year-month-day, and append your contract name).
4. Implement the contract:
    - Add a new header file named `YourContractName.h` in the directory `src/contracts`.
      You may copy `EmptyTemplate.h` or another contract as the basis to start with.
    - Add your new file to the MS Visual C++ project "Qubic" in the filter `contracts`.
    - Rename the state struct to match the capital-letter versions of your contract name.
      Also rename the secondary state struct (with postfix `2`), which will be used in future versions for `EXPAND` events.
    - Add the contract in `src/contract_core/contract_def.h` with the next unused contract index.
      See for example [how the QVAULT was added](https://github.com/qubic/core/commit/85019c6e41bf05f14460582af5ada782398badde#diff-003724794b2450f069f8246422df93ed2b1a89f2c5c01289834b7a5e31430de7).
    - Do NOT change any other file in the `src` folder without explicit permission of the core team.
    - Design and implement the interfaces of your contract (the user procedures and user functions along with its inputs and outputs).
      The QPI available for implementing the contract is defined in `src/contracts/qpi.h`.
    - Implement the system procedures needed and remove all the system procedures that are not needed by your contract.
    - Follow the general [qubic style guidelines](https://github.com/qubic/core/blob/main/doc/contributing.md#style-guidelines) when writing your code.
    - Add the short form contract name as a prefix to all global constants, structs and classes (if any).
    - Make sure your code is efficient. Execution time will cost fees in the future.
      Think about the data structures you use, for example if you can use a hash map instead of an array with linear search.
      Check if you can optimize code in loops and especially in nested loops.
    - Also be efficient with the state memory you reserve.
      Larger state sizes will also lead to more execution fees, because the hashing of the state memory will be included in the execution time.
      For example, your contract does not need to be prepared for 1 Million users right from the beginning.
      The size of data structures can be increased later with `EXPAND` events.
      Currently, the contract state size is limited to 1 GB.
    - See more details about how to develop the contract [below](#procedures-and-functions).
5. Implement tests in GoogleTest framework:
    - Create a file for your tests in the directory `test`.
      To get started, you may copy an existing contract test file, such as `test/contract_qearn.cpp`.
      The name of your new file should follow the pattern `contract_[YourContractName].cpp`.
    - Add the file to the MS Visual C++ project "test".
    - Implement tests covering all your procedures and functions, making sure they work as intended and as written down in your contract description.
6. Make sure that there are no compiler errors, no compiler warnings, all your tests pass without errors.
7. Double check that the implementation of your contract does what it is supposed to do.
8. Create PR to develop branch of official repository, including the description of your contract.


## Review and tests

Each contract must be validated with the following steps:
1. The contract is verified with the [Qubic Contract Verification Tool](https://github.com/Franziska-Mueller/qubic-contract-verify), ensuring that it complies with the formal requirements mentioned above, such as no use of [forbidden C++ features](#restrictions-of-c-language-features).
   In the `qubic/core` repository, the tool is run automatically as GitHub workflow for PRs to the `develop` and `main` branches (as well as for commits in these branches).
   However, since workflow runs on PRs require maintainer approval, we highly recommend to either build the tool from source or use the GitHub action provided in the tool's repository to analyze your contract header file before opening your PR.   
2. The features of the contract have to be extensively tested with automated tests implemented within the Qubic Core's GoogleTest framework.
3. The contract and testing code must be reviewed by at least one of the Qubic Core devs, ensuring it meets high quality standards.
4. Before integrating the contract in the official Qubic Core release, the features of the contract must be tested in a test network with multiple nodes, showing that the contract works well in practice and that the nodes run stable with the contract.

After going through this validation process, a contract can be integrated in official releases of the Qubic Core code.


## Deployment

Steps for deploying a contract:

1. Finish development, review, and tests as written above. This includes waiting for approval of your PR by the core dev team. If you need to make any significant changes to the code after the computors accepted your proposal, you will need to make a second proposal. 
2. A proposal for including your contract into the Qubic Core needs to be prepared.
   We recommend to add your proposal description to https://github.com/qubic/proposal/tree/main/SmartContracts via a pull request (this directory also contains files from other contracts added before, which can be used as a template).
   The proposal description should include a detailed description of your contract (see point 1 of the [Development section](#development)) and the final source code of the contract.
3. A computor operator needs to propose to include your contract into the Qubic Core, see https://github.com/qubic/proposal/blob/main/general-computor-proposal-how-to.md.
   The proposal needs to be made with the GQMPROP smart contract ("GeneralQuorumProposal") and include a permanent link to your proposal description.
   Your proposal will be open for voting by computors until the end of the running epoch (the end is next Wednesday 12:00 UTC).
   We recommend to post your proposal before the mid of the epoch (between Wednesday 12:00 UTC and Sunday), because Computors will need some time to read your proposal and decide.
   The epoch of the proposal is called epoch *N* in the following.
4. If least 451 votes have been casted and there are more votes for "yes, include" than for "no, don't include" after epoch *N*, the contract code will be included in epoch *N+1*.
   Further, the IPO of the contract shares is held in this epoch *N+1*.
   The QUs collected in IPO will be burned, filling the initial contract execution fee reserve of the contract.
5. If there were enough bids to sell all shares during the IPO, the contract is "constructed" in the following epoch *N+2*.
   This means that its state is initialized and system procedures are run starting this epoch.
   Further, user functions and procedures can be called starting with this epoch.
   According to the Qubic protocol (not implemented yet): If no shares are bought, the contract will not be executed and removed from the Core code again.

The epoch number specified for the contract in `src/contract_core/contract_def.h` is the construction epoch *N+2*.
Thus, if the proposal (point 3) is done in epoch 150 and accepted by the quorum, the IPO will be held in epoch 151.
If the IPO is successful, epoch 152 will be the construction epoch, the first epoch in which your contract code is run.


## Procedures and Functions

In Qubic contracts, procedures and functions are defined with special macros, such as `PUBLIC_PROCEDURE(ProcedureName)`.
The macro is followed by a block framed with `{` and `}` that contains the implementation.
Here is an example:

```
PUBLIC_PROCEDURE(ProcedureName)
{
    // code
}
```

### User functions

User functions cannot modify the contract's state, but they are useful to query information from the state, either with the network message `RequestContractFunction`, or by a function or procedure of the same or another contract.
Functions can be called by procedures, but procedures cannot be called by functions.

A user function has to be defined with one of the following the QPI macros, passing the name of the function: `PUBLIC_FUNCTION(NAME)`, `PUBLIC_FUNCTION_WITH_LOCALS(NAME)`, `PRIVATE_FUNCTION(NAME)`, or `PRIVATE_FUNCTION_WITH_LOCALS(NAME)`.

Each of the macros requires you to define the data types `[NAME]_input` and `[NAME]_output`.
Both typically are structs, but may be empty if no input or output is needed.
A reference to an instance of `[NAME]_input` named `input` is passed to the function, containing the payload data sent with `RequestContractFunction`.
Further, a reference to an instance of `[NAME]_output` named `output` is passed to the function (initialized with zeros), which should be modified in the function.
It is sent as the response message, answering `RequestContractFunction`.

A `PUBLIC` function can be called by other contracts with larger contract index, a `PRIVATE` function cannot be called by other contracts.

In order to make the function available for external requests through the `RequestContractFunction` network message, you need to call `REGISTER_USER_FUNCTION([NAME], [INPUT_TYPE]);` in the contract initialization procedure `REGISTER_USER_FUNCTIONS_AND_PROCEDURES`.
`[INPUT_TYPE]` is an integer greater or equal to 1 and less or equal to 65535, which identifies the function to call in the `RequestContractFunction` message (`inputType` member).
If the `inputSize` member in `RequestContractFunction` does not match `sizeof([NAME]_input)`, the input data is either cut off or padded with zeros.

The contract state is passed to the function as a const reference named `state`.

Use the macro with the postfix `_WITH_LOCALS` if the function needs local variables, because (1) the contract state cannot be modified within contract functions and (2) creating local variables / objects on the regular function call stack is forbidden.
With these macros, you have to define the struct `[NAME]_locals`.
A reference to an instance of `[NAME]_locals` named `locals` is passed to the function (initialized with zeros).

An instance of the class `QpiContextFunctionCall` named `qpi` is passed to the function, enabling QPI function calls for obtaining various information, for example:

- `qpi.epoch()` returns the current epoch,
- `qpi.tick()` returns the current tick,
- `qpi.getEntity()` allows to get information about an entity (user or contract) given by its `id` (32 byte public key).
  The information includes the `incomingAmount` and `outgoingAmount`, whose difference is the balance of QUs that this entity possesses at the moment.
  The `id` of your contract is available through the macro `SELF`.
- `numberOfShares()` returns the number of shares that exist of a specific asset, optionally further restricting the search with a filtering criterion, such as the owner or the managing contract.

Search for `struct QpiContextFunctionCall` in `qpi.h` to see all QPI functions available for implementing contract user functions.


### User procedures

User procedures can modify the state.
They are invoked either by transactions with the ID (public key) of the contract being the destination address, or from another procedure of the same contract or a different contract.

A user procedure can be defined with one of the following QPI macros: `PUBLIC_PROCEDURE(NAME)`, `PUBLIC_PROCEDURE_WITH_LOCALS(NAME)`, `PRIVATE_PROCEDURE(NAME)`, or `PRIVATE_PROCEDURE_WITH_LOCALS(NAME)`.

Each of the macros requires you to define the data type `[NAME]_input` and `[NAME]_output`.
Both typically are structs, but may be empty if no input or output is needed.
A reference to an instance of `[NAME]_input` named `input` is passed to the procedure, containing the input data either from the transaction or passed from the invoking other procedure.
Further, a reference to an instance of `[NAME]_output` named `output` is passed to the procedure (initialized with zeros), which should be modified.
It is returned if the procedure is invoked from another procedure, but unused if the procedure has been invoked directly through a transaction.

A `PUBLIC` procedure can be called by other contracts with larger contract index, a `PRIVATE` procedure cannot be called by other contracts.

In order to make the function available for invocation by transactions, you need to call `REGISTER_USER_PROCEDURE([NAME], [INPUT_TYPE]);` in the contract initialization procedure `REGISTER_USER_FUNCTIONS_AND_PROCEDURES`.
`[INPUT_TYPE]` is an integer greater or equal to 1 and less or equal to 65535, which identifies the procedure to call in the `Transaction` (`inputType` member).
`REGISTER_USER_PROCEDURE` has its own set of input types, so the same input type number may be used for both `REGISTER_USER_PROCEDURE` and `REGISTER_USER_FUNCTION` (for example there may be one function with input type 1 and one procedure with input type 1).
If the `inputSize` member in `Transaction` does not match `sizeof([NAME]_input)`, the input data is either cut off or padded with zeros.

The contract state is passed to the procedure as a reference named `state`.
And it can be modified (in contrast to contract functions).

Use the macro with the postfix `_WITH_LOCALS` if the procedure needs local variables, because creating local variables / objects on the regular function call stack is forbidden.
With these macros, you have to define the struct `[NAME]_locals`.
A reference to an instance of `[NAME]_locals` named `locals` is passed to the procedure (initialized with zeros).

An instance of the class `QpiContextProcedureCall` named `qpi` is passed to the procedure.
It inherits all QPI functions of `QpiContextFunctionCall`.
Additionally, QPI procedures are provided, such as:

- `qpi.transfer()` transfers QUs from this contract to another entity (user or contract),
- `qpi.burn()` burns QUs owned by this contract, filling the contract execution fee reserve,
- `qpi.issueAsset()` issues a new asset with a fixed number of shares,
- `qpi.transferShareOwnershipAndPossession()` transfers ownership and possession of a certain number of asset shares to another entity.

The following QPI functions are highly relevant for user procedures:

- `invocator()` returns the `id` of the entity that has invoked this user procedure.
  If it has been invoked through a transaction, this returns the public key of the source user, who has created and signed the transaction.
  If it has been invoked from another contract procedure, this returns the public key of the caller contract.
- `invocationReward()` returns the invocation reward, which is the amount of QU transferred to invoked contract by the invoking entity (immediately before invoking the procedure).
- `originator()` returns the `id` of the user, who has created and signed the transaction that led to executing this procedure or function.

All three, `invocator()`, `invocationReward()`, and `originator()` are also available in contract user functions, but they only return non-zero if the user function has been called by a procedure.

Search for `struct QpiContextProcedureCall` and `struct QpiContextFunctionCall` in `qpi.h` to see all QPI functions available for implementing contract procedures.

### System procedures

System procedures can modify the state and are invoked by the Qubic Core (the system) as event callbacks.

They are defined with the following macros:

1. `INITIALIZE()`: Called once after successful IPO, immediately before the beginning of the construction epoch in order to initialize the state.
2. `BEGIN_EPOCH()`: Called before each epoch, after node startup or seamless epoch transition, before the `BEGIN_TICK()` of the epoch's initial tick.
3. `END_EPOCH()`: Called after each epoch, after the last `END_TICK()` but before (1) contract shares are generated in case of IPO, (2) revenue and revenue donations are distributed, and (3) the epoch counter is incremented and the next epoch's initial tick is set.
4. `BEGIN_TICK()`: Called before a tick is processed, that is before the transactions of the tick are executed.
5. `END_TICK()`: Called after finishing to execute all transactions of a tick.
6. `PRE_RELEASE_SHARES()`: Called before asset management rights are transferred from this contract to another contract that called `qpi.acquireShare()`.
7. `PRE_ACQUIRE_SHARES()`: Called before asset management rights are transferred to this contract from another contract that called `qpi.releaseShare()`.
8. `POST_RELEASE_SHARES()`: Called after asset management rights were transferred from this contract to another contract that called `qpi.acquireShare()`
9. `POST_ACQUIRE_SHARES()`: Called after asset management rights were transferred to this contract from another contract that called `qpi.releaseShare()`.
10. `POST_INCOMING_TRANSFER()`: Called after QUs have been transferred to this contract. [More details...](#callback-post_incoming_transfer)

System procedures 1 to 5 have no input and output.
The input and output of system procedures 6 to 9 are discussed in the section about [management rights transfer](#management-rights-transfer).

The contract state is passed to each of the procedures as a reference named `state`.
And it can be modified (in contrast to contract functions).

For each of the macros above, there is a variant with the postfix `_WITH_LOCALS`.
These can be used, if the procedure needs local variables, because creating local variables / objects on the regular function call stack is forbidden.
With these, you have to define the struct `[NAME]_locals`.
A reference to an instance of `[NAME]_locals` named `locals` is passed to the procedure (initialized with zeros).

An instance of the class `QpiContextProcedureCall` named `qpi` is passed to the system procedure, providing the same QPI features as for user procedures.

The QPI functions `qpi.invocator()` and `originator()` return `NULL_ID` / `id::zero()` for system procedures 1 to 5.

For system procedures 6 to 9, `qpi.invocator()` returns the contract ID of the contract calling `qpi.releaseShares()` or `qpi.acquireShares()`.

Each of `INITIALIZE()`, `BEGIN_EPOCH()`, and `BEGIN_TICK()` is executed for all contracts in ascending order, that is, it is executed for the contract with contract index 1 before that with index 2, followed by contract 3 etc.

Each of  `END_TICK()` and `END_EPOCH()` is executed for all contracts in descending order, that is, it is executed for the contract with the highest contract index first; the contract with index 1 is executed last.


## Assets and shares

Next to the 676 contract shares that are created after the IPO of a new contract, a contract can also create a new asset with a fixed number of shares with `qpi.issueAsset()`.

An asset is identified by the pair of issuer and asset name.
Thus, an asset name may not be unique and always needs to be given together with the issuer to uniquely identify an asset.
The issuer is the public key / ID (of user or contract) passed to `qpi.issueAsset()`.
The asset name consists of up to 7 characters.
The first must be an upper-case letter in range A to Z.
The following may be either upper-case letters (A to Z) or digits (0-9).

The fixed number of asset shares created by `qpi.issueAsset()` first are owned and possessed by the issuer entity.
The contract that issued an asset via `qpi.issueAsset()` gets the asset management rights, that is,
it can transfer ownership and possession of shares to other entities through `qpi.transferShareOwnershipAndPossession()`.

A common way of creating assets is by invoking the QX user procedure `IssueAsset`.
The managing contract of all asset shares issued this way is QX.
Thus, only QX can successfully transfer ownership and possession of these shares to other entities, either with the user procedure `TransferShareOwnershipAndPossession` or via selling / buying with ask / bid orders in the QX exchange.

Asset management rights can be transferred to other contracts through `qpi.releaseShares()` or `qpi.acquireShares()`,
for example, if an asset has been issued using QX but an owner of some if its shares wants to trade them using a different exchange (not QX), the management rights of these shares need to be transferred to the other contract first.
Shares that are managed by QX can be released to another contract (transferring rights to manage ownership and possession) by invoking the QX user procedure `TransferShareManagementRights` (with owner/possessor as invocator).
QX rejects all attempts (`qpi.acquireShares()`) of other contracts to acquire rights from QX.


### Referencing of asset shares

TODO

### Management rights transfer

There are two ways of transferring asset management rights:

1. The contract already having management rights releases them to another contract by calling `qpi.releaseShares()`.
2. The contract needing management rights acquires them from the contract having the rights by calling `qpi.acquireShares()`.

#### Transferring rights with `qpi.releaseShares()`

Let's assume contract A has management rights of shares and wants to transfer them to contract B.
Contract A can try to do this by calling `qpi.releaseShares()`.
In this call, the following happens:

![qpi.releaseShares()](releaseShares.png "qpi.releaseShares()")

After checking the inputs passed to `qpi.releaseShares()` by contract A, the system calls `PRE_ACQUIRE_SHARES()` of contract B to (1) query if it is willing to acquire the management rights and (2) query the fee that A needs to pay to B for the rights transfer.

An instance of the following struct is passed to the system procedure `PRE_ACQUIRE_SHARES()` of contract B as `input`:

```
struct PreManagementRightsTransfer_input
{
    Asset asset;
    id owner;
    id possessor;
    sint64 numberOfShares;
    sint64 offeredFee;
    uint16 otherContractIndex;
};
```

An instance of the following struct is passed to the system procedure `PRE_ACQUIRE_SHARES()` of contract B as `output`:

```
struct PreManagementRightsTransfer_output
{
    bool allowTransfer;
    sint64 requestedFee;
};
```

By default, all of `output` is set to zero, that is, `allowTransfer = false`.
Thus, if `PRE_ACQUIRE_SHARES()` is not defined or empty, all transfers are rejected.
Set `output.allowTransfer = true` in order to accept the rights transfer.

If `allowTransfer` is `false` or `requestedFee > offeredFee`, the transfer is canceled.
Otherwise, the `requestedFee` is transferred from contract A to B, followed by the transfer of the management rights from contract A to B.

Finally, the system procedure `POST_ACQUIRE_SHARES()` is called in contract B, passing an instance of the following struct as `input`:

```
struct PostManagementRightsTransfer_input
{
    Asset asset;
    id owner;
    id possessor;
    sint64 numberOfShares;
    sint64 receivedFee;
    uint16 otherContractIndex;
};
```

The output of `POST_ACQUIRE_SHARES()` is empty (`NoData`).

Calling `qpi.releaseShares()` and `qpi.acquireShares()` is not permitted in the system procedures `PRE_ACQUIRE_SHARES()` and `POST_ACQUIRE_SHARES()`, that is, they will return with an error in such a context.

The function `qpi.releaseShares()` has the following parameters and return value:

```
sint64 releaseShares(
    const Asset& asset,
    const id& owner,
    const id& possessor,
    sint64 numberOfShares,
    uint16 destinationOwnershipManagingContractIndex,
    uint16 destinationPossessionManagingContractIndex,
    sint64 offeredTransferFee
);
```

On success, it returns the payed fee, which is >= 0.
If `offeredTransferFee` or the contract balance is not sufficient, it returns `-requestedFee`.
In case of another error, it returns `INVALID_AMOUNT` (which is a negative number of large amount).

For more details, refer to the code of `qpi.releaseShares()` in `src/contract_core/qpi_asset_impl.h`.


#### Transferring rights with `qpi.acquireShares()`

Let's assume contract A has management rights of shares and contract B wants to get them.
Contract B can try to do this by calling `qpi.acquireShares()`.
In this call, the following happens:

![qpi.acquireShares()](acquireShares.png "qpi.acquireShares()")

After checking the inputs passed to `qpi.acquireShares()` by contract B, the system calls `PRE_RELEASE_SHARES()` of contract A to (1) query if it is willing to release the management rights and (2) query the fee that B needs to pay to A for the rights transfer.

An instance of the following struct is passed to the system procedure `PRE_RELEASE_SHARES()` of contract A as `input`:

```
struct PreManagementRightsTransfer_input
{
    Asset asset;
    id owner;
    id possessor;
    sint64 numberOfShares;
    sint64 offeredFee;
    uint16 otherContractIndex;
};
```

An instance of the following struct is passed to the system procedure `PRE_RELEASE_SHARES()` of contract A as `output`:

```
struct PreManagementRightsTransfer_output
{
    bool allowTransfer;
    sint64 requestedFee;
};
```

By default, all of `output` is set to zero, that is, `allowTransfer = false`.
Thus, if `PRE_RELEASE_SHARES()` is not defined or empty, all transfers are rejected.
Set `output.allowTransfer = true` in order to accept the rights transfer.

If `allowTransfer` is `false` or `requestedFee > offeredFee`, the transfer is canceled.
Otherwise, the `requestedFee` is transferred from contract B to A, followed by the transfer of the management rights from contract A to B.

Finally, the system procedure `POST_RELEASE_SHARES()` is called in contract A, passing an instance of the following struct as `input`:

```
struct PostManagementRightsTransfer_input
{
    Asset asset;
    id owner;
    id possessor;
    sint64 numberOfShares;
    sint64 receivedFee;
    uint16 otherContractIndex;
};
```

The output of `POST_RELEASE_SHARES()` is empty (`NoData`).

Calling `qpi.releaseShares()` and `qpi.acquireShares()` is not permitted in the system procedures `PRE_RELEASE_SHARES()` and `POST_RELEASE_SHARES()`, that is, they will return with an error in such a context.

The function `qpi.acquireShares()` has the following parameters and return value:

```
sint64 acquireShares(
    const Asset& asset,
    const id& owner,
    const id& possessor,
    sint64 numberOfShares,
    uint16 sourceOwnershipManagingContractIndex,
    uint16 sourcePossessionManagingContractIndex,
    sint64 offeredTransferFee
);
```

On success, it returns the payed fee, which is >= 0.
If `offeredTransferFee` or the contract balance is not sufficient, it returns `-requestedFee`.
In case of another error, it returns `INVALID_AMOUNT` (which is a negative number of large amount).

For more details, refer to the code of `qpi.acquireShares()` in `src/contract_core/qpi_asset_impl.h`.

#### Notes and recommendations

By default, management rights of shares can be transferred without the agreement of the owner/possessor, given that both contracts agree on the transfer and the requested transfer fee is paid.
However, this feature is to be used with caution, because there is a risk of hijacking management rights, requesting a high fee for getting (back) management rights of shares.
This is why the recommended way (that is implemented in QX) is that the owner/possessor needs to invoke a user procedure that actively releases the management rights by calling `qpi.releaseShares()`.
QX never releases shares passively (following call of `qpi.acquireShares()` by another contract).
The callbacks `PRE_RELEASE_SHARES()` and `PRE_ACQUIRE_SHARES()` may also check that the `qpi.originator()` initiating the transfer is the owner/possessor.


## Other QPI features

### Container types

The following container types are available in the QPI:

- `Array<T, L>`: Array of `L` elements of type `T` (`L` must be 2^N)
- `BitArray<L>`: Array of `L` bits encoded in array of `uint64` (`L` must be 2^N, overall size is at least 8 bytes)
- `Collection<T, L>`: Collection of priority queues of elements with type `T` and total element capacity `L`.
  Each ID pov (point of view) has an own queue.
- `HashMap<KeyT, ValueT, L>`: Hash map of up to `L` pairs of key and value (types `KeyT` and `ValueT`).
  Lookup by key, insert, and remove run in approximately constant time if population is less than 80% of `L`.
- `HashSet<KeyT, L>`: Hash set of keys of type `KeyT` and total capacity `L`.
  Lookup by key, insert, and remove run in approximately constant time if population is less than 80% of `L`.

Please note that removing items from `Collection`, `HashMap`, and `HashSet` does not immediately free the hash map slots used for the removed items.
This may negatively impact the lookup speed, which depends on the maximum population seen since the last cleanup.
If the container isn't emptied by calling the method `reset()` regularly (such as at the end of each epoch), it is recommended to call `cleanup()` or `cleanupIfNeeded()` at the end of the epoch.
Alternatively, if you expect a lot of removes during an epoch, you may call `cleanupIfNeeded()` at the end of a user procedure that removes items.
Cleanup isn't done automatically when removing elements, because, first, it is very expensive compared to the lookup, the add, and remove operations and second, because it invalidates the indices of items, which may be used by the calling function.

For `HashMap` and `HashSet`, the hash function can be individually defined one of the following alternatives:
1. You may define a specialization of the template class `HashFunction` (see `qpi.h`) for your `KeyT`.
   The implementation of the 32-byte `id` uses the first 8 bytes as the hash.
   The default implementation used for other types computes a K12 hash of the key.
2. Alternatively, you may define an own hash function class for your key type and
   pass it as the last template parameter of `HashMap` or `HashSet` (following the capacity `L`),


### Calling other user functions and  procedures

TODO: invoking procedures, calling functions, including those of other contracts

### Callback `POST_INCOMING_TRANSFER()`

If defined in a contract, the contract system procedure `POST_INCOMING_TRANSFER()` (or `POST_INCOMING_TRANSFER_WITH_LOCALS()`) is run to notify the contract about an incoming transfer of QU to the contract entity (increasing the balance of the contract).

Outgoing transfers (decreasing the balance) are always initiated by the contract itself.
So the contract has all the information needed to keep track of outgoing transfers without requiring a dedicated callback procedure.

An instance `input` of the following struct is passed to `POST_INCOMING_TRANSFER()` / `POST_INCOMING_TRANSFER_WITH_LOCALS()`:

```
struct PostIncomingTransfer_input
{
    id sourceId;    // id of entity that the QUs came from
    sint64 amount;  // number of QUs transferred to SELF
    uint8 type;     // type of transfer, see below
};
```

The type of transfer has one of the following values:

- `TransferType::procedureTransaction`: The callback is run with this transfer type while processing a transaction that invokes a procedure of this contract.
  Specifically, it is run after transferring the transaction amount (= invocation reward) but before invoking the procedure.

- `TransferType::standardTransaction`: This transfer type means processing a transaction that does not invoke a procedure, usually with `inputType == 0`.

- `TransferType::qpiTransfer`: This transfer type is triggered by a call to `qpi.transfer()`.
  Please note that transferring to `SELF` triggers this, although it does NOT increase the balance overall, because the balance is decreased by the same amount before.
  This special case can be detected by checking `input.sourceId == SELF`.

- `TransferType::qpiDistributeDividends`: This transfer type is triggered by a call to `qpi.distributeDividends()`.
  Please note the special case when the contract distributing dividends holds shares of itself, similar to the special case of `TransferType::qpiTransfer` mentioned above (next to the incoming transfer of `input.amount` the outgoing transfer needs to be considered to completely track the balance of the contract).

- `TransferType::revenueDonation`: This transfer type occurs when a contract receives a revenue donation.
  It only happens at the end of an epoch, when the computor revenue is generated, after `END_EPOCH()` and before `BEGIN_EPOCH()`.

- `TransferType::ipoBidRefund`: This transfer type is triggered if the contract has placed a bid in a contract IPO with `qpi.bidInIPO()` and QUs are refunded.
  This can happen in while executing `qpi.bidInIPO()`, when an IPO bid transaction is processed, and when the IPO is finished at the end of the epoch (after `END_EPOCH()` and before `BEGIN_EPOCH()`).

In the implementation of the callback procedure, you cannot run `qpi.transfer()`, `qpi.distributeDividends()`, and `qpi.bidInIPO()`.
That is, calls to these QPI procedures will fail to prevent nested callbacks.
If you invoke a user procedure from the callback, the fee / invocation reward cannot be transferred.
In consequence, the procedure is executed but with `qpi.invocationReward() == 0`.


## Restrictions of C++ Language Features

It is prohibited to locally instantiate objects or variables on the function call stack. This includes loop index variables `for (int i = 0; ...)`.
Instead, use the function and procedure definition macros with the postfix `_WITH_LOCALS` (see above).  
In procedures you alternatively may store temporary variables permanently as members of the state.

Defining, casting, and dereferencing pointers is forbidden.
The character `*` is only allowed in the context of multiplication.

The characters `[` and `]` are forbidden in order to prevent creating low-level arrays and accessing buffers without checking bounds.

All kind of preprocessor directives are prohibited (character `#`), such as includes, pragmas, conditional compilation etc.

Hint: You can `#include "qpi.h"` for development, because it makes IntelliSense work better. But when development is finished, all `#include` need to be removed.

Floating point data types (`float` and `double`) cannot be used, because their arithmetics is not well-defined and may lead to inconsistent states.

The division operator `/` and the modulo operator `%` are prohibited to prevent potentially inconsistent behavior in case of division by zero.
Use `div()` and `mod()` instead, which return zero in case of division by zero.

Strings `"` and chars `'` are forbidden, because they can be used to jump to random memory fragments.
If you want to use `static_assert` you can do so via the `STATIC_ASSERT` macro defined in `qpi.h` which does not require a string literal.

Variadic arguments, template parameter packs, and function parameter packs are prohibited (character combination `...`).

Double underscores `__` must not be used in a contract, because these are reserved for internal functions and compiler macros that are prohibited to be used directly.
For similar reasons, `QpiContext` and `const_cast` are prohibited too.

The scope resolution operator `::` is also prohibited, except for structs, enums, and namespaces defined in contracts and `qpi.h`.

The keyword `union` is prohibited to make the code easier to read and prevent tricking code audits.
Similarly, the keywords `typedef` and `using` are only allowed in local scope, e.g. inside structs or functions.
The only exception is `using namespace QPI` which can be used at global scope.

Global variables are not permitted.
Global constants, structs and classes must begin with the name of the contract state struct.

There is a limit for recursion and depth of nested contract function / procedure calls (the limit is 10 at the moment).

The input and output structs of contract user procedures and functions may only use integer and boolean types (such as `uint64`, `sint8`, `bit`) as well as `id`, `Array`, and `BitArray`, and struct types containing only allowed types.
Complex types that may have an inconsistent internal state, such as `Collection`, are forbidden in the public interface of a contract.


## General Change Management
Smart Contract code in Qubic is generally immutable. This ensures stable quality and security across the network.
However there are situations where you want to change your SC.

### Bugfix
A bugfix is possible at any time. It can be applied during the epoch (if no state is changed) or must be coordinated with an epoch update.

### New Features
If you want to add new features, this needs to be approved by the computors again. Please refer to the [Deployment](#deployment) for the needed steps. The IPO is not anymore needed for an update of your SC.


## Interface in qubic-cli

One way of interacting with the contract in the Qubic Core is via the command line tool [qubic-cli](https://github.com/qubic/qubic-cli).
This tool is used a lot by the Core developers and the Core test team.

You may add an your contract interface to qubic-cli by forking the repo, creating your feature branch in your repo, and implementing it in this branch.
If you would like to have it included in the official qubic-cli, open a pull request after testing it successfully.

For implementing your contract in qubic-cli, add a new file named like your contract.

Contract procedures are invoked by creating transactions, which can be easily done with the function `makeContractTransaction()`.
Contract functions can be run by calling the function `runContractFunction()`.
Both are declared in the header file `walletUtils.h`:

```
void makeContractTransaction(const char* nodeIp, int nodePort,
                             const char* seed,
                             uint64_t contractIndex, uint16_t txType, uint64_t amount,
                             int extraDataSize, const void* extraData,
                             uint32_t scheduledTickOffset);
bool runContractFunction(const char* nodeIp, int nodePort,
                         unsigned int contractIndex, unsigned short funcNumber,
                         void* inputPtr, size_t inputSize,
                         void* outputPtr, size_t outputSize,
                         QCPtr* qcPtr = nullptr);`
```

The file `proposal.cpp` has a lot of examples showing how to use both functions.
For example, `getProposalIndices()` shows how to call a contract function requiring input and providing output with `runContractFunction()`.
An example use case of `makeContractTransaction()` can be found in `gqmpropSetProposal()`.
The function `castVote()` is a more complex example combining both, calling a contract function and invoking a contract procedure.




