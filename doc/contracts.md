# Smart Contract Development

For a general introduction to Smart Contracts in Qubic, we recommend to read [the Qubic Docs](https://docs.qubic.org/learn/smart-contracts).

1.  [Overview](#overview)
2.  [Development](#development)
3.  [Review and tests](#review-and-tests)
4.  [Procedures and Functions](#procedures-and-functions)
5.  [Restrictions of C++ Language Features](#restrictions-of-c-language-features)
6.  [General Change Management](#general-change-management)

## Overview

In Qubic, smart contracts are implemented in a restricted variant of C++ and compiled into the Qubic Core executable.

In order to isolate contracts, access to functions and data of other contracts and Core internals are only possible through a programming interface called QPI.
The QPI is the only external dependency available for developing a contract. That is, using libraries is forbidden.
Further, contracts cannot use C++ features that are known for imposing security risks, such as pointers, low-level arrays (which lack checking of bounds), and preprocessor directives.
A contract also never gets access to uninitialized memory (all memory is initialized with zeros).

Each contract is implemented in one C++ header file in the directory `src/contracts`.

A contract has a state struct, containing all its data as member variables.
The memory available to the contract is allocated statically, but extending the state will be possible between epochs through special `EXPAND` events.

The state struct also includes the procedures and functions of the contract, which have to be defined using special macros such as `PUBLIC_PROCEDURE`, `PRIVATE_FUNCTION`, or `BEGIN_EPOCH`.
Functions cannot modify the state, but they are useful to query information with the network message `RequestContractFunction`.
Procedures can modify the state and are either invoked by special transactions (user procedures) or internal core events (system procedures).

Contract developers should be aware of the following part of the Qubic protocol that is not implemented yet in the core:
Execution of contract procedures will cost fees that will be paid from its contract fee reserve.
This reserve is initially filled with the QUs burned during the IPO of the contract and refilled by additional burning of QUs happening in the contract.
In the long run, each contract needs to burn QUs to stay alive.
If a contract's execution fee reserve runs empty, the contract will not be executed anymore.


## Development

In order to develop a contract, follow these steps:

1. Write a description of your contract, about what it is supposed to do.
   We recommend to discuss it with others, for example the Qubic community on Discord.
2. Think about a name of your contract and check that it is not used yet.
   You will need a long version such as `YourContractName` for naming the file, a short version of at most 7 capital letters such as `YCN` (asset name of the contract shares), and optionally a longer capital-letter name for the state struct and as a prefix for global constants of the contract, such as `YOURCONTRACTNAME`.
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
    - Add the short form contract name as a prefix to all global constants (if any).
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
    - Implement tests covering all your procedures and functions, making sure they work as intended and as written down in your contract description.
6. Make sure that there are no compiler errors, no compiler warnings, all your tests pass without errors.
7. Double check that the implementation of your contract does what it is supposed to do.
8. Create PR to develop branch of official repository, including the description of your contract.


## Review and tests

Each contract must be validated with the following steps:
1. The contract is verified with a special software tool, ensuring that it complies with the formal requirements mentioned above, such as no use of forbidden C++ features.
   (Currently, this tool has not been implemented yet. Thus, this check needs to be done during the review in point 3.)
2. The features of the contract have to be extensively tested with automated tests implemented within the Qubic Core's GoogleTest framework.
3. The contract and testing code must be reviewed by at least one of the Qubic Core devs, ensuring it meets high quality standards.
4. Before integrating the contract in the official Qubic Core release, the features of the contract must be tested in a test network with multiple nodes, showing that the contract works well in practice and that the nodes run stable with the contract.

After going through this validation process, a contract can be integrated in official releases of the Qubic Core code.


## Deployment

Steps for deploying a contract:

1. Finish development, review, and tests as written above.
2. A proposal for including your contract into the Qubic Core needs to be prepared.
   We recommend to add your proposal description to https://github.com/qubic/proposal/tree/main/SmartContracts via a pull request (this directory also contains files from other contracts added before, which can be used as a template).
   The proposal description should include a detailed description of your contract (see point 1 of the Development section) and the final source code of the contract.
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

In Qubic contracts, procedures and functions are defined starting with a special macro and ending with the footer macro `_` (single underscore).

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


### System procedures

System procedures can modify the state and are invoked by the Qubic Core as event callbacks.

They are defined with the following macros:

1. `INITIALIZE`: Called once after successful IPO, immediately before the beginning of the construction epoch in order to initialize the state.
2. `BEGIN_EPOCH`: Called before each epoch, after node startup or seamless epoch transition, before the `BEGIN_TICK` of the epoch's initial tick.
3. `END_EPOCH`: Called after each epoch, after the last `END_TICK` but before (1) contract shares are generated in case of IPO, (2) revenue and revenue donations are distributed, and (3) the epoch counter is incremented and the next epoch's initial tick is set.
4. `BEGIN_TICK`: Called before a tick is processed, that is before the transactions of the tick are executed.
5. `END_TICK`: Called after finishing to execute all transactions of a tick.
6. `PRE_RELEASE_SHARES`: Called before asset management rights transfer by `qpi.releaseShare()`. Not fully implemented yet.
7. `PRE_ACQUIRE_SHARES`: Called before asset management rights transfer by `qpi.acquireShare()`. Not fully implemented yet.
8. `POST_RELEASE_SHARES`: Called after asset management rights transfer by `qpi.releaseShare()`. Not fully implemented yet.
9. `POST_ACQUIRE_SHARES`: Called after asset management rights transfer by `qpi.acquireShare()`. Not fully implemented yet.

System procedures 1 to 5 have no input and output.

The contract state is passed to each of the procedures as a reference named `state`.
And it can be modified (in contrast to contract functions).

For each of the macros above, there is a variant with the postfix `_WITH_LOCALS`.
These can be used, if the procedure needs local variables, because creating local variables / objects on the regular function call stack is forbidden.
With these, you have to define the struct `[NAME]_locals`.
A reference to an instance of `[NAME]_locals` named `locals` is passed to the procedure (initialized with zeros).

The QPI function `qpi.invocator()` returns `NULL_ID` / `id::zero()` for system procedures.

Each of `INITIALIZE`, `BEGIN_EPOCH`, and `BEGIN_TICK` is executed for all contracts in ascending order, that is, it is executed for the contract with contract index 1 before that with index 2, followed by contract 3 etc.

Each of  `END_TICK` and `END_EPOCH` is executed for all contracts in descending order, that is, it is executed for the contract with the highest contract index first; the contract with index 1 is executed last.


## Restrictions of C++ Language Features

It is prohibited to locally instantiating objects or variables on the function call stack.
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

Variadic arguments are prohibited (character combination `...`).

Double underscores `__` must not be used in a contract, because these are reserved for internal functions and compiler macros that are prohibited to be used directly.
For similar reasons, `::`, `QpiContext`, and `const_cast` are prohibited too.

The keywords `typedef` and `union` are prohibited to make the code easier to read and prevent tricking code audits.

Global variables are not permitted.
Global constants must begin with the name of the contract state struct.

There is a limit for recursion and depth of nested contract function / procedure calls (the limit is 10 at the moment).

## General Change Management
Smart Contract code in Qubic is generally immutable. This ensures stable quality and security accross the network.
However there are situation where you want to change your SC.

### Bugfix
A bugfix is possible at any time. It can be applied during the epoch (if no state is changed) or must be coordinated with an epoch update.

### New Features
If you want to add new features, this needs to be approved by the computors again. Please refer to the [Deployment](#deployment) for the needed steps. The IPO is not anymore needed for an update of your SC.
