# network_messages

This directory contains all definitions of the network messages used in the Qubic communication protocol. 
This includes all constants and data types used in the network message definition.

You may use the code of this directory in your software project for communicating with the Qubic Core.
In order to disable dependencies to other code, `#define NETWORK_MESSAGES_WITHOUT_CORE_DEPENDENCIES`
before including any header of this directory or edit the `#if ...` line in `common_def.h`.
