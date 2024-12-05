#pragma once

#include "../core/src/contracts/qpi.h"

using namespace QPI;


struct BridgeContract : public ContractBase {

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
        array<uint8, 32> message;
    };

    struct completeOrder_input {
        uint64 orderId;
    };

    struct completeOrder_output {
        uint8 status;
        array<uint8, 32> message;
    };

    struct refundOrder_input {
        uint64 orderId;
    };

    struct refundOrder_output {
        uint8 status;
        array<uint8, 32> message;
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


private:
    // Contract State
    QPI::HashMap<uint64, BridgeOrder, 256> orders; // Storage for orders (fixed size)
    uint64 nextOrderId;                            // Counter for order IDs
    uint64 lockedTokens;                           // Total locked tokens in the contract (balance)
    uint64 transactionFee;                         // Fee for creating an order
    id admin;                                      // Admin address
    QPI::HashMap<id, bit, 16> managers;            // Managers list
    uint64 totalReceivedTokens;                    // Total tokens received

    // Internal methods for admin/manager permissions using qpi CALLS
    PRIVATE_FUNCTION_WITH_LOCALS(isAdmin)
        output = (qpi.invocator() == state.admin);
    _

        PRIVATE_FUNCTION_WITH_LOCALS(isManager)
        output = state.managers.get(qpi.invocator(), bit());
    _

        /*
        // Check if the invocator is the admin
        bool isAdmin() const {
            return qpi.invocator() == state.admin;
        }

        // Check if the invocator is a manager
        bool isManager() const {
            bit isManager = false;
            state.managers.get(qpi.invocator(), isManager);
            return isManager;
        }*/

public:
    // Create a new order and lock tokens
    PUBLIC_FUNCTION_WITH_LOCALS(createOrder)

        // Validate the input 
        if (input.amount == 0) {
            output.status = 1; // Error
            copyMemory(output.message, "Amount must be greater than 0");
            return;
        }

        if (qpi.invocationReward() < state.transactionFee) {
            output.status = 2; // Error
            copyMemory(output.message, "Insufficient transaction fee");
            return;
        }

        // Create the order
        BridgeOrder newOrder;
        newOrder.orderId = state.nextOrderId++;
        newOrder.qubicSender = qpi.invocator();
        newOrder.ethAddress = input.ethAddress;
        newOrder.amount = input.amount;
        newOrder.orderType = 0; // Default order type
        newOrder.status = 0; // Created
        newOrder.fromQubicToEthereum = input.fromQubicToEthereum;

        // Store the order
        state.orders.set(newOrder.orderId, newOrder);

        output.status = 0; // Success
        copyMemory(output.message, "Order created successfully");
    _

    // Retrieve an order
    PUBLIC_FUNCTION_WITH_LOCALS(getOrder)

        BridgeOrder order;
        if (!state.orders.get(input.orderId, order)) {
            output.status = 1; // Error
            copyMemory(output.message, "Order not found");
            return;
        }

        OrderResponse orderResp;
        // Populate OrderResponse with BridgeOrder data
        orderResp.orderId = order.orderId;
        orderResp.originAccount = order.qubicSender;
        orderResp.destinationAccount = order.ethAddress;
        orderResp.amount = order.amount;
        copyMemory(orderResp.memo, "Bridge transfer details"); // Placeholder for metadata
        orderResp.sourceChain = state.sourceChain;

        output.status = 0; // Success
        output.order = orderResp;
        copyMemory(output.message, "Order retrieved successfully");
    _

    // Admin Functions
    PUBLIC_PROCEDURE_WITH_LOCALS(setAdmin)

        if (qpi.invocator() != state.admin) {
            output.status = 1; // Error
            copyMemory(output.message, "Only the current admin can set a new admin");
            return;
        }
        state.admin = input.address;
        output.status = 0; // Success
        copyMemory(output.message, "Admin updated successfully");
    _

   PUBLIC_PROCEDURE_WITH_LOCALS(addManager)

        if (qpi.invocator() != state.admin) {
            output.status = 1; // Error
            copyMemory(output.message, "Only the admin can add managers");
            return;
        }
        state.managers.set(input.address, 1); // Add manager
        output.status = 0; // Success
        copyMemory(output.message, "Manager added successfully");
    _

    PUBLIC_PROCEDURE_WITH_LOCALS(removeManager)

        if (qpi.invocator() != state.admin) {
            output.status = 1; // Error
            copyMemory(output.message, "Only the admin can remove managers");
            return;
        }

        state.managers.removeByKey(input.address); // Remove manager
        output.status = 0; // Success
        copyMemory(output.message, "Manager removed successfully");
    _

    PUBLIC_FUNCTION_WITH_LOCALS(getTotalReceivedTokens)

        output.totalTokens = state.totalReceivedTokens;
        copyMemory(output.message, "Total tokens received by the contract");
    _

    // Complete an order and release tokens
    PUBLIC_PROCEDURE_WITH_LOCALS(completeOrder)

        bit isManager = false;
        CALL(isManager, NoData{}, isManager);

        if (!isManager) {
            output.status = 1; // Error
            copyMemory(output.message, "Only managers can complete orders");
            return;
        }

        // Retrieve the order
        BridgeOrder order;
        if (!state.orders.get(input.orderId, order)) {
            output.status = 1; // Error
            copyMemory(output.message, "Order not found");
            return;
        }

        // Check the status
        if (order.status != 0) { // Ensure it's not already completed or refunded
            output.status = 2; // Error
            copyMemory(output.message, "Order not in a valid state for completion");
            return;
        }

        // Handle order based on transfer direction
        if (order.fromQubicToEthereum) {
            // Ensure sufficient tokens were transferred to the contract
            if (state.totalReceivedTokens < order.amount) {
                output.status = 4; // Error
                copyMemory(output.message, "Insufficient tokens transferred to contract");
                return;
            }

            state.lockedTokens += order.amount;
        }
        else {
            // Ensure sufficient tokens are locked for the order
            if (state.lockedTokens < order.amount) {
                output.status = 5; // Error
                copyMemory(output.message, "Insufficient locked tokens");
                return;
            }

            // Transfer tokens back to the user
            if (qpi.transfer(order.qubicSender, order.amount) < 0) {
                output.status = 6; // Error
                copyMemory(output.message, "Token transfer failed");
                return;
            }

            state.lockedTokens -= order.amount;
        }

        // Mark the order as completed
        order.status = 1; // Completed
        state.orders.set(order.orderId, order);

        output.status = 0; // Success
        copyMemory(output.message, "Order completed successfully");
    _

    // Refund an order and unlock tokens
    PUBLIC_PROCEDURE_WITH_LOCALS(refundOrder)

        bit isManager = false;
        CALL(isManager, NoData{}, isManager);
        if (!isManager) {
            output.status = 1; // Error
            copyMemory(output.message, "Only managers can refund orders");
            return;
        }

        // Retrieve the order
        BridgeOrder order;
        if (!state.orders.get(input.orderId, order)) {
            output.status = 1; // Error
            copyMemory(output.message, "Order not found");
            return;
        }

        // Check the status
        if (order.status != 0) { // Ensure it's not already completed or refunded
            output.status = 2; // Error
            copyMemory(output.message, "Order not in a valid state for refund");
            return;
        }

        // Update the status and refund tokens
        qpi.transfer(order.qubicSender, order.amount);
        state.lockedTokens -= order.amount;
        order.status = 2; // Refunded
        state.orders.set(order.orderId, order);

        output.status = 0; // Success
        copyMemory(output.message, "Order refunded successfully");
    _

    // Transfer tokens to the contract
    PUBLIC_PROCEDURE_WITH_LOCALS(transferToContract)

        if (input.amount == 0) {
            output.status = 1; // Error
            copyMemory(output.message, "Amount must be greater than 0");
            return;
        }

        if (state.userBalances.get(qpi.invocator(), 0) < input.amount) {
            output.status = 3; // Error
            copyMemory(output.message, "Insufficient balance for transfer");
            return;
        }
        else {

            if (qpi.transfer(SELF, input.amount) < 0) {
                output.status = 2; // Error
                copyMemory(output.message, "Transfer failed");
                return;
            }

            // Update the total received tokens
            state.totalReceivedTokens += input.amount;

            output.status = 0; // Success
            copyMemory(output.message, "Tokens transferred successfully");
        }
    _

    // Register Functions and Procedures
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_FUNCTION(createOrder, 1);
        REGISTER_USER_FUNCTION(getOrder, 2);

        REGISTER_USER_PROCEDURE(setAdmin, 3);
        REGISTER_USER_PROCEDURE(addManager, 4);
        REGISTER_USER_PROCEDURE(removeManager, 5);

        REGISTER_USER_PROCEDURE(completeOrder, 6);
        REGISTER_USER_PROCEDURE(refundOrder, 7);
        REGISTER_USER_PROCEDURE(transferToContract, 8);
    _

    // Initialize the contract
    INITIALIZE
        state.nextOrderId = 0;
        state.lockedTokens = 0;
        state.totalReceivedTokens = 0;
        state.transactionFee = 1000;
        // Let's try to set admin as the contract creator, not the contract owner
        state.admin = qpi.invocator(); //If this fails, set a predetermined address
        state.managers.reset(); // Initialize managers list
        state.sourceChain = 0; //Arbitrary numb. No-EVM chain
    _
};
