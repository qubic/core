# Qubic Contract Development Guidelines

This directory contains the smart contract implementations in header files starting with a capital letter. 

Further, it contains `qpi.h`, the Qubic Programming Interface for implementing the smart contracts.
It is available automatically in the smart contract implementation header files.

Other header files starting with a lowercase letter, such as `math_lib.h`, provide examples of useful functions that can be used in contract code.

This document outlines the guidelines for developing secure and efficient Qubic contracts. Adherence to these guidelines is crucial for ensuring the proper functionality and security of your contracts within the Qubic environment.

### Syntax and Formatting:
- Qubic contracts generally inherit the syntax and format of C/C++.
- However, due to security reasons, certain things are prohibited:
  - Declaring and accessing arrays using the C/C++ notation (`[` `]`). Utilize pre-defined array structures within qpi.h such as `collection`, `uint32_64`, `uint32_128`,...
  - Any pointer-related techniques such as casting, accessing,...
  - Native data types like `int`, `long`, `char`,... Use their corresponding predefined datatypes in `qpi.h` (`int8`, `int16`, `int32`, `int64`,...)
  - Inclusion of other files via `#include`. All functions must reside within a single file.
  - Math operators `%` and `/`. Use `mod` and `div` from `qpi.h` instead. `+`, `-`, `*`(multiplication), and bit-wise operators are accepted.
  - Local variable declaration, even for for-loop. You need to define all necessary variables in the contract state and utilize them.
  - The `typedef`, `union` keyword.
  - Floating points datatypes (half, float, double)
 
Currently, the maximum contract state size is capped at 1 GiB (03/02/2024). This value is subject to change based on hardware upgrades of computors.
