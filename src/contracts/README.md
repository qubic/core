# Qubic Contract Development Guidelines

This directory contains the smart contract implementations in header files starting with a capital letter. 

Further, it contains `qpi.h`, the Qubic Programming Interface for implementing the smart contracts.
It is available automatically in the smart contract implementation header files.

Other header files starting with a lowercase letter, such as `math_lib.h`, provide examples of useful functions that can be used in contract code.

This document outlines the guidelines for developing secure and efficient Qubic contracts. Adherence to these guidelines is crucial for ensuring the proper functionality and security of your contracts within the Qubic environment.

### Concepts:

The state is the persistent memory of the contract that is kept aligned in all nodes.

A contract can have member functions and procedures.

Functions cannot change the state of the contract. They can be called via a `RequestContractFunction` network message.

Procedures can change the state of the contract. They are invoked by a transaction and run when the tick containing the transaction is processed.

There are some special procedures that are called by the system at the beginning of the tick etc.

A call of a user procedure usually goes along with a transfer of an invocation reward from the invoking user to the contract.

Procedures can call procedures and functions of the same contract and of contracts with lower contract index.

Functions can call functions of the same contract and of contracts with lower contract ID.

Private functions and procedures cannot be called from other contracts.

In order to be available for invocation by transaction and network message, procedures and functions need to be registered in the special member function `REGISTER_USER_FUNCTIONS_AND_PROCEDURES`.



### Syntax and Formatting:
- Qubic contracts generally inherit the syntax and format of C/C++.
- However, due to security reasons, certain things are prohibited:
  - Declaring and accessing arrays using the C/C++ notation (`[` `]`). Utilize pre-defined array structures within qpi.h such as `collection`, `uint32_64`, `uint32_128`, ...
  - Any pointer-related techniques such as casting, accessing, ...
  - Native data types like `int`, `long`, `char`, ... Use their corresponding predefined data types in `qpi.h` (`uint8`, `sint8`, `uint16`, `sint32`, `uint64`, ...)
  - Inclusion of other files via `#include`. All functions must reside within a single file.
  - Math operators `%` and `/`. Use `mod` and `div` from `qpi.h` instead. `+`, `-`, `*`(multiplication), and bit-wise operators are accepted.
  - Local variable declaration, even for for-loop. You need to define all necessary variables in either in the contract state or in a "locals" struct similar to the input and output struct of a function or procedure.
  - The `typedef`, `union` keyword.
  - Floating point data types (half, float, double)
 
Currently, the maximum contract state size is capped at 1 GiB (03/02/2024). This value is subject to change based on hardware upgrades of computors.
