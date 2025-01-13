#pragma once
#include "qpi.h"

using namespace QPI;

struct ETHBRIDGE2 
{
};

struct ETHBRIDGE : public ContractBase {

public:
    // Bridge Order Structure
    struct BridgeOrder {
        uint64 orderId;                      // Unique ID for the order
        id qubicSender;                      // Sender address on Qubic
        id ethAddress;                       // Destination Ethereum address
        uint64 amount;                       // Amount to transfer
        uint8 orderType;                     // Type of order (e.g., mint, transfer)
        uint8 status;                        // Order status (e.g., Created, Pending, Refunded)
        bit fromQubicToEthereum;             // Direction of transfer
    };

    // Input and Output Structs
    struct createOrder_input {
        id ethAddress;
        uint64 amount;
        bit fromQubicToEthereum;
    };

    struct createOrder_output {
        uint8 status;
    };

    struct setAdmin_input {
        id address;
    };

    struct setAdmin_output {
        uint8 status;
    };

    struct addManager_input {
        id address;
    };

    struct addManager_output {
        uint8 status;
    };

    struct removeManager_input {
        id address;
    };

    struct removeManager_output {
        uint8 status;
    };

    struct getTotalReceivedTokens_input {
    };

    struct getTotalReceivedTokens_output {
        uint64 totalTokens;
    };

    struct getTotalLockedTokens_input {
    };

    struct getTotalLockedTokens_output {
        uint64 totalLockedTokens;
    };

    struct completeOrder_input {
        uint64 orderId;
    };

    struct completeOrder_output {
        uint8 status;
    };

    struct refundOrder_input {
        uint64 orderId;
    };

    struct refundOrder_output {
        uint8 status;
    };

    struct transferToContract_input {
        uint64 amount;
    };

    struct transferToContract_output {
        uint8 status;
    };

    // Order Response Structure 
    struct OrderResponse {
        uint64 orderId;                      // Order ID as uint64
        id originAccount;                    // Origin account
        id destinationAccount;               // Destination account
        uint64 amount;                       // Amount as uint64
        array<uint8, 64> memo;               // Notes or metadata
        uint32 sourceChain;                  // Source chain identifier
    };

    struct getOrder_input {
        uint64 orderId;
    };

    struct getOrder_output {
        uint8 status;
        array<uint8, 32> message;
        OrderResponse order;                 // Updated response format
    };

    struct getAdminID_input {
    };

    struct getAdminID_output {
        id adminId;
    };

    struct getInvocatorID_input {
    };

    struct getInvocatorID_output {
        id invocatorId;
    };

    // Logger structures
    struct EthBridgeLogger {
        uint32 _contractIndex;   // Index of the contract
        uint32 _errorCode;       // Error code
        uint64 _orderId;         // Order ID if applicable
        uint64 _amount;          // Amount involved in the operation
        char _terminator;        // Marks the end of the logged data
    };

    struct AddressChangeLogger {
        uint32 _contractIndex;
        uint8 _eventCode;        // Event code 'adminchanged'
        id _newAdminAddress;     // New admin address
        char _terminator;
    };

    struct TokensLogger {
        uint32 _contractIndex;
        uint64 _lockedTokens;   // Balance tokens locked
        uint64 _totalReceivedTokens; //Balance total receivedTokens
        char _terminator;
    };

    // Enum for error codes
    enum EthBridgeError {
        onlyManagersCanCompleteOrders = 1,
        invalidAmount = 2,
        insufficientTransactionFee = 3,
        orderNotFound = 4,
        invalidOrderState = 5,
        insufficientLockedTokens = 6,
        transferFailed = 7
    };


protected:
    // Contract State
    array<BridgeOrder, 256> orders;      // Storage for orders (fixed size)
    uint64 nextOrderId;                            // Counter for order IDs
    uint64 lockedTokens;                           // Total locked tokens in the contract (balance)
    uint32 transactionFee;                         // Fee for creating an order
    id admin;                                      // Admin address
    array<id, 16> managers;            // Managers list
    uint64 totalReceivedTokens;                    // Total tokens received
    uint32 sourceChain;                            // Source chain identifier

    // Internal methods for admin/manager permissions        
    typedef id isAdmin_input;
    typedef bit isAdmin_output;

   PRIVATE_FUNCTION(isAdmin)
       output = (qpi.invocator() == state.admin);
    _

    typedef id isManager_input;
    typedef bit isManager_output;

    PRIVATE_FUNCTION(isManager)
        for (uint64 i = 0; i < state.managers.capacity(); ++i) {
            if (state.managers.get(i) == input) {
                output = true;
                return;
            }
        }
        output = false;
    _

public:
    // Create a new order and lock tokens
    struct createOrder_locals {
        BridgeOrder newOrder;
        EthBridgeLogger log;
        id invocatorAddress;
        bit isManagerOperating;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(createOrder) //transaction fee
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);
        //Check if the order is handled by a manager
        if (!locals.isManagerOperating) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log);
            output.status = 1; // Error
            return;
        }

        // Validate the input 
        if (input.amount == 0) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,               
                EthBridgeError::invalidAmount, 
                0,                            
                input.amount,                 
                0                          
            };
            LOG_INFO(locals.log);
            output.status = 1; // Error
            return;
        }

        if (qpi.invocationReward() < state.transactionFee) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::insufficientTransactionFee,
                0,
                input.amount,
                0
                };
            LOG_INFO(locals.log);
            output.status = 2; // Error
            return;
        }

        // Create the order
        locals.newOrder.orderId = state.nextOrderId++;
        locals.newOrder.qubicSender = qpi.invocator();
        locals.newOrder.ethAddress = input.ethAddress;
        locals.newOrder.amount = input.amount;
        locals.newOrder.orderType = 0; // Default order type
        locals.newOrder.status = 0; // Created
        locals.newOrder.fromQubicToEthereum = input.fromQubicToEthereum;

        // Store the order
        for (uint64 i = 0; i < state.orders.capacity(); ++i) {
            if (state.orders.get(i).status == 255) { // Empty slot
                state.orders.set(i, locals.newOrder);

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0,
                    locals.newOrder.orderId,
                    input.amount,
                    0
                };
                LOG_INFO(locals.log);
                output.status = 0; // Success
                return;
            }
        }
    _

    // Retrieve an order
    struct getOrder_locals {
        EthBridgeLogger log;
        BridgeOrder order;
        OrderResponse orderResp;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getOrder)
        for (uint64 i = 0; i < state.orders.capacity(); ++i) {
            locals.order = state.orders.get(i);
            if (locals.order.orderId == input.orderId && locals.order.status != 255) {
                // Populate OrderResponse with BridgeOrder data
                locals.orderResp.orderId = locals.order.orderId;
                locals.orderResp.originAccount = locals.order.qubicSender;
                locals.orderResp.destinationAccount = locals.order.ethAddress;
                locals.orderResp.amount = locals.order.amount;
                constexpr char placeholderMemo[64] = "Bridge transfer details"; // Placeholder for metadata
                locals.orderResp.sourceChain = state.sourceChain;

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0, // No error
                    locals.order.orderId,
                    locals.order.amount,
                    0
                };
                LOG_INFO(locals.log);

                output.status = 0; // Success
                output.order = locals.orderResp;
                return;
            }
        }

        // If order not found
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            EthBridgeError::orderNotFound,
            input.orderId,
            0, // No amount involved
            0
        };
        LOG_INFO(locals.log);
        output.status = 1; // Error
    _

    // Admin Functions
    struct setAdmin_locals {
        EthBridgeLogger log;
        AddressChangeLogger adminLog;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(setAdmin)

        if (qpi.invocator() != state.admin) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::onlyManagersCanCompleteOrders,
                0, // No order ID involved
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log);
            output.status = 1; // Error
            return;
        }

        state.admin = input.address;
        //Logging the admin address has changed
        locals.adminLog = AddressChangeLogger{
            CONTRACT_INDEX,
            1, // Event code "Admin Changed"
            input.address,
            0
        };

        LOG_INFO(locals.adminLog);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            0, // No order ID involved
            0, // No amount involved
            0
        };
        LOG_INFO(locals.log);
        output.status = 0; // Success
    _
    
    struct addManager_locals {
        EthBridgeLogger log;
        AddressChangeLogger managerLog;
    };

   PUBLIC_PROCEDURE_WITH_LOCALS(addManager)

        if (qpi.invocator() != state.admin) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::onlyManagersCanCompleteOrders,
                0, // No order ID involved
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log)
            output.status = 1; // Error
            return;
        }

       for (uint64 i = 0; i < state.managers.capacity(); ++i) {
           if (state.managers.get(i) == NULL_ID) { // Slot vac�o
               state.managers.set(i, input.address);

               locals.managerLog = AddressChangeLogger{
                   CONTRACT_INDEX,
                   2, // Manager added
                   input.address,
                   0
               };
               LOG_INFO(locals.managerLog);
               output.status = 0; // Success
               return;
           }
       }

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            0, // No order ID involved
            0, // No amount involved
            0
        };
        LOG_INFO(locals.log);
        output.status = 0; // Success
    _

    struct removeManager_locals {
        EthBridgeLogger log;
        AddressChangeLogger managerLog;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(removeManager)

        if (qpi.invocator() != state.admin) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::onlyManagersCanCompleteOrders, 
                0, // No order ID involved
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log);

            output.status = 1; // Error
            return;
        }

        for (uint64 i = 0; i < state.managers.capacity(); ++i) {
            if (state.managers.get(i) == input.address) {
                state.managers.set(i, NULL_ID);

                locals.managerLog = AddressChangeLogger{
                    CONTRACT_INDEX,
                    3, // Manager removed
                    input.address,
                    0
                };
                LOG_INFO(locals.managerLog);
                output.status = 0; // Success
                return;
            }
        }

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            0, // No order ID involved
            0, // No amount involved
            0
        };
        LOG_INFO(locals.log);
        output.status = 0; // Success
    _

    struct getTotalReceivedTokens_locals {
        EthBridgeLogger log;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getTotalReceivedTokens)

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            0, // No order ID involved
            state.totalReceivedTokens, // Amount of total tokens
            0
        };
        LOG_INFO(locals.log);
        output.totalTokens = state.totalReceivedTokens;
    _

    PUBLIC_FUNCTION(getTotalLockedTokens)
        output.totalLockedTokens = state.lockedTokens;
    _

    struct completeOrder_locals {
        EthBridgeLogger log;
        id invocatorAddress;
        bit isManagerOperating;
        bit orderFound;
        BridgeOrder order;
        TokensLogger logTokens;
    };

    // Complete an order and release tokens
    PUBLIC_PROCEDURE_WITH_LOCALS(completeOrder)
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);

        //Check if the order is handled by a manager
        locals.orderFound = false;
        for (uint64 i = 0; i < state.orders.capacity(); ++i) {
            if (state.orders.get(i).orderId == input.orderId) {
                locals.order = state.orders.get(i);
                locals.orderFound = true;
                break;
            }
        }

        // Order nor found
        if (!locals.orderFound) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0,
                0
            };
            LOG_INFO(locals.log);
            output.status = 2; // Error
            return;
        }

        // Check order status
        // All orders status are 0 when created
        if (locals.order.status != 0) { //Check it is not completed or refunded already
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidOrderState,
                input.orderId,
                0,
                0
            };
            LOG_INFO(locals.log);
            output.status = 3; // Error
            return;
        }

        // Handle order based on transfer direction
        if (locals.order.fromQubicToEthereum) {
            // Ensure sufficient tokens were transferred to the contract
            if (state.totalReceivedTokens - state.lockedTokens < locals.order.amount) {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    input.orderId,
                    locals.order.amount,
                    0
                };
                LOG_INFO(locals.log);
                output.status = 4; // Error
                return;
            }

            state.lockedTokens += locals.order.amount; //increase the amount of locked tokens
            state.totalReceivedTokens -= locals.order.amount; //decrease the amount of no-locked (received) tokens
            locals.logTokens = TokensLogger{
                CONTRACT_INDEX,
                state.lockedTokens,
                state.totalReceivedTokens,
                0
            };

            LOG_INFO(locals.logTokens);
        }
        else {
            // Ensure sufficient tokens are locked for the order
            if (state.lockedTokens < locals.order.amount) {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    input.orderId,
                    locals.order.amount,
                    0
                };
                LOG_INFO(locals.log);
                output.status = 5; // Error
                return;
            }

            // Transfer tokens back to the user
            if (qpi.transfer(locals.order.qubicSender, locals.order.amount) < 0) {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::transferFailed,
                    input.orderId,
                    locals.order.amount,
                    0
                };
                LOG_INFO(locals.log);
                output.status = 6; // Error
                return;
            }

            state.lockedTokens -= locals.order.amount;
            locals.logTokens = TokensLogger{
                CONTRACT_INDEX,
                state.lockedTokens,
                state.totalReceivedTokens,
                0
            };

            LOG_INFO(locals.logTokens);
        }

        // Mark the order as completed
        locals.order.status = 1; // Completed
        state.orders.set(locals.order.orderId, locals.order);

        output.status = 0; // Success
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.orderId,
            locals.order.amount,
            0
        };
        LOG_INFO(locals.log);
    _

    // Refund an order and unlock tokens
    struct refundOrder_locals {
        EthBridgeLogger log;
        id invocatorAddress;
        bit isManagerOperating;
        bit orderFound;
        BridgeOrder order;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(refundOrder)
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);
        //Check if the order is handled by a manager
        if (!locals.isManagerOperating) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log);
            output.status = 1; // Error
            return;
        }

        // Retrieve the order
        //Check if the order is handled by a manager
        locals.orderFound = false;
        for (uint64 i = 0; i < state.orders.capacity(); ++i) {
            if (state.orders.get(i).orderId == input.orderId) {
                locals.order = state.orders.get(i);
                locals.orderFound = true;
                break;
            }
        }

        // Order nor found
        if (!locals.orderFound) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0,
                0
            };
            LOG_INFO(locals.log);
            output.status = 2; // Error
            return;
        }

        // Check order status
        // All orders status are 0 when created
        if (locals.order.status != 0) { //Check it is not completed or refunded already
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidOrderState,
                input.orderId,
                0,
                0
            };
            LOG_INFO(locals.log);
            output.status = 3; // Error
            return;
        }

        // Check Qubic to Ethereum direction, update status and refund tokens
        if (locals.order.fromQubicToEthereum) {
            // Ensure sufficient locked tokens for refund
            if (state.lockedTokens < locals.order.amount) {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    input.orderId,
                    locals.order.amount,
                    0
                };
                LOG_INFO(locals.log);
                output.status = 4; // Error
                return;
            }

            // Unlock tokens by decreasing lockedTokens
            state.lockedTokens -= locals.order.amount;
        }
        else {
            // Ethereum to Qubic direction
            // Ensure sufficient received tokens for refund
            if (state.totalReceivedTokens < locals.order.amount) {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    input.orderId,
                    locals.order.amount,
                    0
                };
                LOG_INFO(locals.log);
                output.status = 5; // Error
                return;
            }

            // Deduct from total received tokens (if refund is successful)
            state.totalReceivedTokens -= locals.order.amount;
        }

        // Transfer tokens back to sender
        qpi.transfer(locals.order.qubicSender, locals.order.amount);
        locals.order.status = 2; // Refunded
        state.orders.set(locals.order.orderId, locals.order);

        // Log success
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.orderId,
            locals.order.amount,
            0
        };
        LOG_INFO(locals.log);
        output.status = 0; // Success
    _

    // Transfer tokens to the contract
    struct transferToContract_locals {
        EthBridgeLogger log;
        TokensLogger logTokens;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(transferToContract)

        if (input.amount == 0) {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                0, // No order ID
                input.amount,
                0
            };
            LOG_INFO(locals.log);
            output.status = 1; // Error
            return;
        }


        if (qpi.transfer(SELF, input.amount) < 0) {
            output.status = 2; // Error
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::transferFailed,
                0, // No order ID
                input.amount,
                0
            };
            LOG_INFO(locals.log);
            return;
        }

        // Update the total received tokens
        state.totalReceivedTokens += input.amount;
        locals.logTokens = TokensLogger{
            CONTRACT_INDEX,
            state.lockedTokens,
            state.totalReceivedTokens,
            0
        };

        LOG_INFO(locals.logTokens);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            0, // No order ID
            input.amount,
            0
        };
        LOG_INFO(locals.log);
        output.status = 0; // Success
    _

    PUBLIC_FUNCTION(getAdminID)
        output.adminId = state.admin;
    _

    PUBLIC_FUNCTION(getInvocatorID)
        output.invocatorId = qpi.invocator();
    _
    // Register Functions and Procedures
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(createOrder, 1);
        REGISTER_USER_FUNCTION(getOrder, 2);

        REGISTER_USER_PROCEDURE(setAdmin, 3);
        REGISTER_USER_PROCEDURE(addManager, 4);
        REGISTER_USER_PROCEDURE(removeManager, 5);

        REGISTER_USER_PROCEDURE(completeOrder, 6);
        REGISTER_USER_PROCEDURE(refundOrder, 7);
        REGISTER_USER_PROCEDURE(transferToContract, 8);

        REGISTER_USER_FUNCTION(isAdmin, 9);
        REGISTER_USER_FUNCTION(isManager, 10);
        REGISTER_USER_FUNCTION(getTotalReceivedTokens, 11);
        REGISTER_USER_FUNCTION(getAdminID, 12);
        REGISTER_USER_FUNCTION(getInvocatorID, 13)
        REGISTER_USER_FUNCTION(getTotalLockedTokens, 14);
        _

    // Initialize the contract
    INITIALIZE
        state.nextOrderId = 0;
        state.lockedTokens = 0;
        state.totalReceivedTokens = 0;
        state.transactionFee = 1000;
        state.admin = ID(_P, _X, _A, _B, _Y, _V, _D, _P, _J, _R, _R, _D, _K, _E, _L, _E, _Y, _S, _H, _Z, _W, _J, _C, _B, _E, _F, _J, _C, _N, _E, _R, _N, _K, _K, _U, _W, _X, _H, _A, _N, _C, _D, _P, _Q, _E, _F, _G, _D, _I, _U, _G, _U, _G, _A, _U, _B, _B, _C, _Y, _K);
        state.sourceChain = 0; // No-EVM chain
    _
};
