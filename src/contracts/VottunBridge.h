using namespace QPI;

struct VOTTUNBRIDGE2
{
};

struct VOTTUNBRIDGE : public ContractBase
{
public:
    // Bridge Order Structure
    struct BridgeOrder
    {
        id qubicSender;              // Sender address on Qubic
        id qubicDestination;         // Destination address on Qubic
        Array<uint8, 64> ethAddress; // Destination Ethereum address
        uint64 orderId;              // Unique ID for the order
        uint64 amount;               // Amount to transfer
        uint8 orderType;             // Type of order (e.g., mint, transfer)
        uint8 status;                // Order status (e.g., Created, Pending, Refunded)
        bit fromQubicToEthereum;     // Direction of transfer
        bit tokensReceived;          // Flag to indicate if tokens have been received
        bit tokensLocked;            // Flag to indicate if tokens are in locked state
    };

    // Input and Output Structs
    struct createOrder_input
    {
        id qubicDestination; // Destination address on Qubic (for EVM to Qubic orders)
        uint64 amount;
        Array<uint8, 64> ethAddress;
        bit fromQubicToEthereum;
    };

    struct createOrder_output
    {
        uint8 status;
        uint64 orderId;
    };

    struct setAdmin_input
    {
        id address;
    };

    struct setAdmin_output
    {
        uint8 status;
    };

    struct addManager_input
    {
        id address;
    };

    struct addManager_output
    {
        uint8 status;
    };

    struct removeManager_input
    {
        id address;
    };

    struct removeManager_output
    {
        uint8 status;
    };

    struct getTotalReceivedTokens_input
    {
        uint64 amount;
    };

    struct getTotalReceivedTokens_output
    {
        uint64 totalTokens;
    };

    struct completeOrder_input
    {
        uint64 orderId;
    };

    struct completeOrder_output
    {
        uint8 status;
    };

    struct refundOrder_input
    {
        uint64 orderId;
    };

    struct refundOrder_output
    {
        uint8 status;
    };

    struct transferToContract_input
    {
        uint64 amount;
        uint64 orderId;
    };

    struct transferToContract_output
    {
        uint8 status;
    };

    // Withdraw Fees structures
    struct withdrawFees_input
    {
        uint64 amount;
    };

    struct withdrawFees_output
    {
        uint8 status;
    };

    // Get Available Fees structures
    struct getAvailableFees_input
    {
        // No parameters
    };

    struct getAvailableFees_output
    {
        uint64 availableFees;
        uint64 totalEarnedFees;
        uint64 totalDistributedFees;
    };

    // Order Response Structure
    struct OrderResponse
    {
        id originAccount;                    // Origin account
        Array<uint8, 64> destinationAccount; // Destination account
        uint64 orderId;                      // Order ID as uint64
        uint64 amount;                       // Amount as uint64
        Array<uint8, 64> memo;               // Notes or metadata
        uint32 sourceChain;                  // Source chain identifier
        id qubicDestination;
    };

    struct getOrder_input
    {
        uint64 orderId;
    };

    struct getOrder_output
    {
        uint8 status;
        OrderResponse order; // Updated response format
        Array<uint8, 32> message;
    };

    struct getAdminID_input
    {
        uint8 idInput;
    };

    struct getAdminID_output
    {
        id adminId;
    };

    struct getContractInfo_input
    {
        // No parameters
    };

    struct getContractInfo_output
    {
        id admin;
        Array<id, 16> managers;
        uint64 nextOrderId;
        uint64 lockedTokens;
        uint64 totalReceivedTokens;
        uint64 earnedFees;
        uint32 tradeFeeBillionths;
        uint32 sourceChain;
        // Debug info
        Array<BridgeOrder, 16> firstOrders; // First 16 orders
        uint64 totalOrdersFound;            // How many non-empty orders exist
        uint64 emptySlots;
    };

    // Logger structures
    struct EthBridgeLogger
    {
        uint32 _contractIndex; // Index of the contract
        uint32 _errorCode;     // Error code
        uint64 _orderId;       // Order ID if applicable
        uint64 _amount;        // Amount involved in the operation
        sint8 _terminator;      // Marks the end of the logged data
    };

    struct AddressChangeLogger
    {
        id _newAdminAddress; 
        uint32 _contractIndex;
        uint8 _eventCode; // Event code 'adminchanged'
        sint8 _terminator;
    };

    struct TokensLogger
    {
        uint32 _contractIndex;
        uint64 _lockedTokens;        // Balance tokens locked
        uint64 _totalReceivedTokens; // Balance total receivedTokens
        sint8 _terminator;
    };

    struct getTotalLockedTokens_locals
    {
        EthBridgeLogger log;
    };

    struct getTotalLockedTokens_input
    {
        // No input parameters
    };

    struct getTotalLockedTokens_output
    {
        uint64 totalLockedTokens;
    };

    // Enum for error codes
    enum EthBridgeError
    {
        onlyManagersCanCompleteOrders = 1,
        invalidAmount = 2,
        insufficientTransactionFee = 3,
        orderNotFound = 4,
        invalidOrderState = 5,
        insufficientLockedTokens = 6,
        transferFailed = 7,
        maxManagersReached = 8,
        notAuthorized = 9,
        onlyManagersCanRefundOrders = 10,
        proposalNotFound = 11,
        proposalAlreadyExecuted = 12,
        proposalAlreadyApproved = 13,
        notOwner = 14,
        maxProposalsReached = 15
    };

    // Enum for proposal types
    enum ProposalType
    {
        PROPOSAL_SET_ADMIN = 1,
        PROPOSAL_ADD_MANAGER = 2,
        PROPOSAL_REMOVE_MANAGER = 3,
        PROPOSAL_WITHDRAW_FEES = 4,
        PROPOSAL_CHANGE_THRESHOLD = 5
    };

    // Admin proposal structure for multisig
    struct AdminProposal
    {
        uint64 proposalId;
        uint8 proposalType;           // Type from ProposalType enum
        id targetAddress;             // For setAdmin/addManager/removeManager
        uint64 amount;                // For withdrawFees
        Array<id, 16> approvals;      // Array of owner IDs who approved
        uint8 approvalsCount;         // Count of approvals
        bit executed;                 // Whether proposal was executed
        bit active;                   // Whether proposal is active (not cancelled)
    };

public:
    // Contract State
    Array<BridgeOrder, 1024> orders;
    id admin;                        // Primary admin address (deprecated, kept for compatibility)
    id feeRecipient;                 // Specific wallet to receive fees
    Array<id, 16> managers;          // Managers list
    uint64 nextOrderId;              // Counter for order IDs
    uint64 lockedTokens;             // Total locked tokens in the contract (balance)
    uint64 totalReceivedTokens;      // Total tokens received
    uint32 sourceChain;              // Source chain identifier (e.g., Ethereum=1, Qubic=0)
    uint32 _tradeFeeBillionths;      // Trade fee in billionths (e.g., 0.5% = 5,000,000)
    uint64 _earnedFees;              // Accumulated fees from trades
    uint64 _distributedFees;         // Fees already distributed to shareholders
    uint64 _earnedFeesQubic;         // Accumulated fees from Qubic trades
    uint64 _distributedFeesQubic;    // Fees already distributed to Qubic shareholders

    // Multisig state
    Array<id, 16> admins;            // List of multisig admins
    uint8 numberOfAdmins;            // Number of active admins
    uint8 requiredApprovals;         // Threshold: number of approvals needed (2 of 3)
    Array<AdminProposal, 32> proposals; // Pending admin proposals
    uint64 nextProposalId;           // Counter for proposal IDs

    // Internal methods for admin/manager permissions
    typedef id isAdmin_input;
    typedef bit isAdmin_output;

    PRIVATE_FUNCTION(isAdmin)
    {
        output = (qpi.invocator() == state.admin);
    }

    typedef id isManager_input;
    typedef bit isManager_output;

    struct isManager_locals
    {
        uint64 i;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(isManager)
    {
        for (locals.i = 0; locals.i < state.managers.capacity(); ++locals.i)
        {
            if (state.managers.get(locals.i) == input)
            {
                output = true;
                return;
            }
        }
        output = false;
    }

    typedef id isMultisigAdmin_input;
    typedef bit isMultisigAdmin_output;

    struct isMultisigAdmin_locals
    {
        uint64 i;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(isMultisigAdmin)
    {
        for (locals.i = 0; locals.i < (uint64)state.numberOfAdmins; ++locals.i)
        {
            if (state.admins.get(locals.i) == input)
            {
                output = true;
                return;
            }
        }
        output = false;
    }

public:
    // Create a new order and lock tokens
    struct createOrder_locals
    {
        BridgeOrder newOrder;
        EthBridgeLogger log;
        uint64 i;
        uint64 j;
        bit slotFound;
        uint64 cleanedSlots;  // Counter for cleaned slots
        BridgeOrder emptyOrder; // Empty order to clean slots
        uint64 requiredFeeEth;
        uint64 requiredFeeQubic;
        uint64 totalRequiredFee;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(createOrder)
    {
        // Validate the input
        if (input.amount == 0)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                0,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = 1; // Error
            return;
        }

        // Calculate fees as percentage of amount (0.5% each, 1% total)
        locals.requiredFeeEth = div(input.amount * state._tradeFeeBillionths, 1000000000ULL);
        locals.requiredFeeQubic = div(input.amount * state._tradeFeeBillionths, 1000000000ULL);
        locals.totalRequiredFee = locals.requiredFeeEth + locals.requiredFeeQubic;

        // Verify that the fee paid is sufficient for both fees
        if (qpi.invocationReward() < static_cast<sint64>(locals.totalRequiredFee))
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::insufficientTransactionFee,
                0,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = 2; // Error
            return;
        }

        // Accumulate fees in their respective variables
        state._earnedFees += locals.requiredFeeEth;
        state._earnedFeesQubic += locals.requiredFeeQubic;

        // Create the order
        locals.newOrder.orderId = state.nextOrderId++;
        locals.newOrder.qubicSender = qpi.invocator();

        // Set qubicDestination according to the direction
        if (!input.fromQubicToEthereum)
        {
            // EVM TO QUBIC
            locals.newOrder.qubicDestination = input.qubicDestination;
            
            // Verify that there are enough locked tokens for EVM to Qubic orders
            if (state.lockedTokens < input.amount)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    0,
                    input.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::insufficientLockedTokens; // Error
                return;
            }
        }
        else
        {
            // QUBIC TO EVM
            locals.newOrder.qubicDestination = qpi.invocator();
        }

        for (locals.i = 0; locals.i < 42; ++locals.i)
        {
            locals.newOrder.ethAddress.set(locals.i, input.ethAddress.get(locals.i));
        }
        locals.newOrder.amount = input.amount;
        locals.newOrder.orderType = 0; // Default order type
        locals.newOrder.status = 0;    // Created
        locals.newOrder.fromQubicToEthereum = input.fromQubicToEthereum;
        locals.newOrder.tokensReceived = false;
        locals.newOrder.tokensLocked = false;

        // Store the order
        locals.slotFound = false;
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            if (state.orders.get(locals.i).status == 255)
            { // Empty slot
                state.orders.set(locals.i, locals.newOrder);
                locals.slotFound = true;

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0, // No error
                    locals.newOrder.orderId,
                    input.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = 0; // Success
                output.orderId = locals.newOrder.orderId;
                return;
            }
        }

        // No available slots - attempt cleanup of completed orders
        if (!locals.slotFound)
        {
            // Clean up completed and refunded orders to free slots
            locals.cleanedSlots = 0;
            for (locals.j = 0; locals.j < state.orders.capacity(); ++locals.j)
            {
                if (state.orders.get(locals.j).status == 2) // Completed or Refunded
                {
                    // Create empty order to overwrite
                    locals.emptyOrder.status = 255; // Mark as empty
                    locals.emptyOrder.orderId = 0;
                    locals.emptyOrder.amount = 0;
                    // Clear other fields as needed
                    state.orders.set(locals.j, locals.emptyOrder);
                    locals.cleanedSlots++;
                }
            }
            
            // If we cleaned some slots, try to find a slot again
            if (locals.cleanedSlots > 0)
            {
                for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
                {
                    if (state.orders.get(locals.i).status == 255)
                    { // Empty slot
                        state.orders.set(locals.i, locals.newOrder);
                        locals.slotFound = true;

                        locals.log = EthBridgeLogger{
                            CONTRACT_INDEX,
                            0, // No error
                            locals.newOrder.orderId,
                            input.amount,
                            0 };
                        LOG_INFO(locals.log);
                        output.status = 0; // Success
                        output.orderId = locals.newOrder.orderId;
                        return;
                    }
                }
            }
            
            // If still no slots available after cleanup
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                99, // Custom error code for "no available slots"
                0,  // No orderId
                locals.cleanedSlots,  // Number of slots cleaned
                0 }; // Terminator
            LOG_INFO(locals.log);
            output.status = 3; // Error: no available slots
            return;
        }
    }

    // Retrieve an order
    struct getOrder_locals
    {
        EthBridgeLogger log;
        BridgeOrder order;
        OrderResponse orderResp;
        uint64 i;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getOrder)
    {
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            locals.order = state.orders.get(locals.i);
            if (locals.order.orderId == input.orderId && locals.order.status != 255)
            {
                // Populate OrderResponse with BridgeOrder data
                locals.orderResp.orderId = locals.order.orderId;
                locals.orderResp.originAccount = locals.order.qubicSender;
                locals.orderResp.destinationAccount = locals.order.ethAddress;
                locals.orderResp.amount = locals.order.amount;
                locals.orderResp.sourceChain = state.sourceChain;
                locals.orderResp.qubicDestination = locals.order.qubicDestination;

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0, // No error
                    locals.order.orderId,
                    locals.order.amount,
                    0 };
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
            0 };
        LOG_INFO(locals.log);
        output.status = 1; // Error
    }

    // Multisig Proposal Functions

    // Create proposal structures
    struct createProposal_input
    {
        uint8 proposalType;      // Type of proposal
        id targetAddress;        // Target address (for setAdmin/addManager/removeManager)
        uint64 amount;           // Amount (for withdrawFees)
    };

    struct createProposal_output
    {
        uint8 status;
        uint64 proposalId;
    };

    struct createProposal_locals
    {
        EthBridgeLogger log;
        uint64 i;
        bit slotFound;
        AdminProposal newProposal;
        bit isMultisigAdminResult;
    };

    // Approve proposal structures
    struct approveProposal_input
    {
        uint64 proposalId;
    };

    struct approveProposal_output
    {
        uint8 status;
        bit executed;
    };

    struct approveProposal_locals
    {
        EthBridgeLogger log;
        AddressChangeLogger adminLog;
        AdminProposal proposal;
        uint64 i;
        bit found;
        bit alreadyApproved;
        bit isMultisigAdminResult;
        uint64 proposalIndex;
        uint64 availableFees;
    };

    // Get proposal structures
    struct getProposal_input
    {
        uint64 proposalId;
    };

    struct getProposal_output
    {
        uint8 status;
        AdminProposal proposal;
    };

    struct getProposal_locals
    {
        uint64 i;
    };

    // Create a new proposal (only multisig admins can create)
    PUBLIC_PROCEDURE_WITH_LOCALS(createProposal)
    {
        // Verify that the invocator is a multisig admin
        id invocator = qpi.invocator();
        CALL(isMultisigAdmin, invocator, locals.isMultisigAdminResult);
        if (!locals.isMultisigAdminResult)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::notOwner,
                0,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::notOwner;
            return;
        }

        // Validate proposal type
        if (input.proposalType < PROPOSAL_SET_ADMIN || input.proposalType > PROPOSAL_CHANGE_THRESHOLD)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount, // Reusing error code
                0,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // Find an empty slot for the proposal
        locals.slotFound = false;
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            if (!state.proposals.get(locals.i).active && state.proposals.get(locals.i).proposalId == 0)
            {
                locals.slotFound = true;
                break;
            }
        }

        if (!locals.slotFound)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::maxProposalsReached,
                0,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::maxProposalsReached;
            return;
        }

        // Create the new proposal
        locals.newProposal.proposalId = state.nextProposalId++;
        locals.newProposal.proposalType = input.proposalType;
        locals.newProposal.targetAddress = input.targetAddress;
        locals.newProposal.amount = input.amount;
        locals.newProposal.approvalsCount = 1; // Creator automatically approves
        locals.newProposal.executed = false;
        locals.newProposal.active = true;

        // Set creator as first approver
        locals.newProposal.approvals.set(0, qpi.invocator());

        // Store the proposal
        state.proposals.set(locals.i, locals.newProposal);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            locals.newProposal.proposalId,
            input.amount,
            0 };
        LOG_INFO(locals.log);

        output.status = 0; // Success
        output.proposalId = locals.newProposal.proposalId;
    }

    // Approve a proposal (only multisig admins can approve)
    PUBLIC_PROCEDURE_WITH_LOCALS(approveProposal)
    {
        // Verify that the invocator is a multisig admin
        id invocator = qpi.invocator();
        CALL(isMultisigAdmin, invocator, locals.isMultisigAdminResult);
        if (!locals.isMultisigAdminResult)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::notOwner,
                0,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::notOwner;
            output.executed = false;
            return;
        }

        // Find the proposal
        locals.found = false;
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            locals.proposal = state.proposals.get(locals.i);
            if (locals.proposal.proposalId == input.proposalId && locals.proposal.active)
            {
                locals.found = true;
                locals.proposalIndex = locals.i;
                break;
            }
        }

        if (!locals.found)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::proposalNotFound,
                input.proposalId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::proposalNotFound;
            output.executed = false;
            return;
        }

        // Check if already executed
        if (locals.proposal.executed)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::proposalAlreadyExecuted,
                input.proposalId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::proposalAlreadyExecuted;
            output.executed = false;
            return;
        }

        // Check if this owner has already approved
        locals.alreadyApproved = false;
        for (locals.i = 0; locals.i < (uint64)locals.proposal.approvalsCount; ++locals.i)
        {
            if (locals.proposal.approvals.get(locals.i) == qpi.invocator())
            {
                locals.alreadyApproved = true;
                break;
            }
        }

        if (locals.alreadyApproved)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::proposalAlreadyApproved,
                input.proposalId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::proposalAlreadyApproved;
            output.executed = false;
            return;
        }

        // Add approval
        locals.proposal.approvals.set((uint64)locals.proposal.approvalsCount, qpi.invocator());
        locals.proposal.approvalsCount++;

        // Check if threshold reached and execute
        if (locals.proposal.approvalsCount >= state.requiredApprovals)
        {
            // Execute the proposal based on type
            if (locals.proposal.proposalType == PROPOSAL_SET_ADMIN)
            {
                state.admin = locals.proposal.targetAddress;
                locals.adminLog = AddressChangeLogger{
                    locals.proposal.targetAddress,
                    CONTRACT_INDEX,
                    1, // Admin changed
                    0 };
                LOG_INFO(locals.adminLog);
            }
            else if (locals.proposal.proposalType == PROPOSAL_ADD_MANAGER)
            {
                // Find empty slot in managers
                for (locals.i = 0; locals.i < state.managers.capacity(); ++locals.i)
                {
                    if (state.managers.get(locals.i) == NULL_ID)
                    {
                        state.managers.set(locals.i, locals.proposal.targetAddress);
                        locals.adminLog = AddressChangeLogger{
                            locals.proposal.targetAddress,
                            CONTRACT_INDEX,
                            2, // Manager added
                            0 };
                        LOG_INFO(locals.adminLog);
                        break;
                    }
                }
            }
            else if (locals.proposal.proposalType == PROPOSAL_REMOVE_MANAGER)
            {
                // Find and remove manager
                for (locals.i = 0; locals.i < state.managers.capacity(); ++locals.i)
                {
                    if (state.managers.get(locals.i) == locals.proposal.targetAddress)
                    {
                        state.managers.set(locals.i, NULL_ID);
                        locals.adminLog = AddressChangeLogger{
                            locals.proposal.targetAddress,
                            CONTRACT_INDEX,
                            3, // Manager removed
                            0 };
                        LOG_INFO(locals.adminLog);
                        break;
                    }
                }
            }
            else if (locals.proposal.proposalType == PROPOSAL_WITHDRAW_FEES)
            {
                locals.availableFees = state._earnedFees - state._distributedFees;
                if (locals.proposal.amount <= locals.availableFees && locals.proposal.amount > 0)
                {
                    if (qpi.transfer(state.feeRecipient, locals.proposal.amount) >= 0)
                    {
                        state._distributedFees += locals.proposal.amount;
                    }
                }
            }
            else if (locals.proposal.proposalType == PROPOSAL_CHANGE_THRESHOLD)
            {
                // Amount field is used to store new threshold
                if (locals.proposal.amount > 0 && locals.proposal.amount <= (uint64)state.numberOfAdmins)
                {
                    state.requiredApprovals = (uint8)locals.proposal.amount;
                }
            }

            locals.proposal.executed = true;
            output.executed = true;
        }
        else
        {
            output.executed = false;
        }

        // Update the proposal
        state.proposals.set(locals.proposalIndex, locals.proposal);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.proposalId,
            locals.proposal.approvalsCount,
            0 };
        LOG_INFO(locals.log);

        output.status = 0; // Success
    }

    // Get proposal details
    PUBLIC_FUNCTION_WITH_LOCALS(getProposal)
    {
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            if (state.proposals.get(locals.i).proposalId == input.proposalId)
            {
                output.proposal = state.proposals.get(locals.i);
                output.status = 0; // Success
                return;
            }
        }

        output.status = EthBridgeError::proposalNotFound;
    }

    // Admin Functions
    struct setAdmin_locals
    {
        EthBridgeLogger log;
        AddressChangeLogger adminLog;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(setAdmin)
    {
        // DEPRECATED: Use createProposal/approveProposal with PROPOSAL_SET_ADMIN instead
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            EthBridgeError::notAuthorized,
            0,
            0,
            0 };
        LOG_INFO(locals.log);
        output.status = EthBridgeError::notAuthorized;
        return;
    }

    struct addManager_locals
    {
        EthBridgeLogger log;
        AddressChangeLogger managerLog;
        uint64 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(addManager)
    {
        // DEPRECATED: Use createProposal/approveProposal with PROPOSAL_ADD_MANAGER instead
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            EthBridgeError::notAuthorized,
            0,
            0,
            0 };
        LOG_INFO(locals.log);
        output.status = EthBridgeError::notAuthorized;
        return;
    }

    struct removeManager_locals
    {
        EthBridgeLogger log;
        AddressChangeLogger managerLog;
        uint64 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(removeManager)
    {
        // DEPRECATED: Use createProposal/approveProposal with PROPOSAL_REMOVE_MANAGER instead
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            EthBridgeError::notAuthorized,
            0,
            0,
            0 };
        LOG_INFO(locals.log);
        output.status = EthBridgeError::notAuthorized;
        return;
    }

    struct getTotalReceivedTokens_locals
    {
        EthBridgeLogger log;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getTotalReceivedTokens)
    {
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0,                         // No error
            0,                         // No order ID involved
            state.totalReceivedTokens, // Amount of total tokens
            0 };
        LOG_INFO(locals.log);
        output.totalTokens = state.totalReceivedTokens;
    }

    struct completeOrder_locals
    {
        EthBridgeLogger log;
        id invocatorAddress;
        bit isManagerOperating;
        bit orderFound;
        BridgeOrder order;
        TokensLogger logTokens;
        uint64 i;
        uint64 netAmount;
    };

    // Complete an order and release tokens
    PUBLIC_PROCEDURE_WITH_LOCALS(completeOrder)
    {
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);

        // Verify that the invocator is a manager
        if (!locals.isManagerOperating)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::onlyManagersCanCompleteOrders,
                input.orderId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::onlyManagersCanCompleteOrders; // Error: not a manager
            return;
        }

        // Check if the order exists
        locals.orderFound = false;
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            if (state.orders.get(locals.i).orderId == input.orderId)
            {
                locals.order = state.orders.get(locals.i);
                locals.orderFound = true;
                break;
            }
        }

        // Order not found
        if (!locals.orderFound)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::orderNotFound; // Error
            return;
        }

        // Check order status
        if (locals.order.status != 0)
        { // Check it is not completed or refunded already
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidOrderState,
                input.orderId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidOrderState; // Error
            return;
        }

        // Use full amount without deducting commission (commission was already charged in createOrder)
        locals.netAmount = locals.order.amount;

        // Handle order based on transfer direction
        if (locals.order.fromQubicToEthereum)
        {
            // Verify that tokens were received
            if (!locals.order.tokensReceived || !locals.order.tokensLocked)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::invalidOrderState,
                    input.orderId,
                    locals.order.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::invalidOrderState;
                return;
            }

            // Tokens are already in lockedTokens from transferToContract
            // No need to modify lockedTokens here
        }
        else
        {
            // Ensure sufficient tokens are locked for the order
            if (state.lockedTokens < locals.order.amount)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    input.orderId,
                    locals.order.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::insufficientLockedTokens; // Error
                return;
            }

            // Transfer tokens back to the user
            if (qpi.transfer(locals.order.qubicDestination, locals.netAmount) < 0)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::transferFailed,
                    input.orderId,
                    locals.order.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::transferFailed; // Error
                return;
            }

            state.lockedTokens -= locals.order.amount;
            locals.logTokens = TokensLogger{
                CONTRACT_INDEX,
                state.lockedTokens,
                state.totalReceivedTokens,
                0 };
            LOG_INFO(locals.logTokens);
        }

        // Mark the order as completed
        locals.order.status = 1;                  // Completed
        state.orders.set(locals.i, locals.order); // Use the loop index

        output.status = 0; // Success
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.orderId,
            locals.order.amount,
            0 };
        LOG_INFO(locals.log);
    }

    // Refund an order and unlock tokens
    struct refundOrder_locals
    {
        EthBridgeLogger log;
        id invocatorAddress;
        bit isManagerOperating;
        bit orderFound;
        BridgeOrder order;
        uint64 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(refundOrder)
    {
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);

        // Check if the order is handled by a manager
        if (!locals.isManagerOperating)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::onlyManagersCanRefundOrders,
                input.orderId,
                0, // No amount involved
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::onlyManagersCanRefundOrders; // Error
            return;
        }

        // Retrieve the order
        locals.orderFound = false;
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            if (state.orders.get(locals.i).orderId == input.orderId)
            {
                locals.order = state.orders.get(locals.i);
                locals.orderFound = true;
                break;
            }
        }

        // Order not found
        if (!locals.orderFound)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::orderNotFound; // Error
            return;
        }

        // Check order status
        if (locals.order.status != 0)
        { // Check it is not completed or refunded already
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidOrderState,
                input.orderId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidOrderState; // Error
            return;
        }
        
        // Handle refund based on transfer direction
        if (locals.order.fromQubicToEthereum)
        {
            // Only refund if tokens were received
            if (!locals.order.tokensReceived)
            {
                // No tokens to return - simply cancel the order
                locals.order.status = 2;
                state.orders.set(locals.i, locals.order);
                
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0,
                    input.orderId,
                    0,
                    0 };
                LOG_INFO(locals.log);
                output.status = 0;
                return;
            }

            // Tokens were received and are in lockedTokens
            if (!locals.order.tokensLocked)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::invalidOrderState,
                    input.orderId,
                    locals.order.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::invalidOrderState;
                return;
            }

            // Verify sufficient locked tokens
            if (state.lockedTokens < locals.order.amount)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    input.orderId,
                    locals.order.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::insufficientLockedTokens;
                return;
            }

            // Return tokens to original sender
            if (qpi.transfer(locals.order.qubicSender, locals.order.amount) < 0)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::transferFailed,
                    input.orderId,
                    locals.order.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::transferFailed;
                return;
            }

            // Update locked tokens balance
            state.lockedTokens -= locals.order.amount;
        }
        
        // Mark as refunded
        locals.order.status = 2;
        state.orders.set(locals.i, locals.order);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.orderId,
            locals.order.amount,
            0 };
        LOG_INFO(locals.log);
        output.status = 0; // Success
    }

    // Transfer tokens to the contract
    struct transferToContract_locals
    {
        EthBridgeLogger log;
        TokensLogger logTokens;
        BridgeOrder order;
        bit orderFound;
        uint64 i;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(transferToContract)
    {
        if (input.amount == 0)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                input.orderId,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // Find the order
        locals.orderFound = false;
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            if (state.orders.get(locals.i).orderId == input.orderId)
            {
                locals.order = state.orders.get(locals.i);
                locals.orderFound = true;
                break;
            }
        }

        if (!locals.orderFound)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::orderNotFound,
                input.orderId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::orderNotFound;
            return;
        }

        // Verify sender is the original order creator
        if (locals.order.qubicSender != qpi.invocator())
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::notAuthorized,
                input.orderId,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::notAuthorized;
            return;
        }

        // Verify order state
        if (locals.order.status != 0)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidOrderState,
                input.orderId,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidOrderState;
            return;
        }

        // Verify tokens not already received
        if (locals.order.tokensReceived)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidOrderState,
                input.orderId,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidOrderState;
            return;
        }

        // Verify amount matches order
        if (input.amount != locals.order.amount)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                input.orderId,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // Only for Qubic-to-Ethereum orders need to receive tokens
        if (locals.order.fromQubicToEthereum)
        {
            if (qpi.transfer(SELF, input.amount) < 0)
            {
                output.status = EthBridgeError::transferFailed;
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::transferFailed,
                    input.orderId,
                    input.amount,
                    0 };
                LOG_INFO(locals.log);
                return;
            }

            // Tokens go directly to lockedTokens for this order
            state.lockedTokens += input.amount;
            
            // Mark tokens as received AND locked
            locals.order.tokensReceived = true;
            locals.order.tokensLocked = true;
            state.orders.set(locals.i, locals.order);
            
            locals.logTokens = TokensLogger{
                CONTRACT_INDEX,
                state.lockedTokens,
                state.totalReceivedTokens,
                0 };
            LOG_INFO(locals.logTokens);
        }

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0,
            input.orderId,
            input.amount,
            0 };
        LOG_INFO(locals.log);
        output.status = 0;
    }

    struct withdrawFees_locals
    {
        EthBridgeLogger log;
        uint64 availableFees;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(withdrawFees)
    {
        // DEPRECATED: Use createProposal/approveProposal with PROPOSAL_WITHDRAW_FEES instead
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            EthBridgeError::notAuthorized,
            0,
            0,
            0 };
        LOG_INFO(locals.log);
        output.status = EthBridgeError::notAuthorized;
        return;
    }

    PUBLIC_FUNCTION(getAdminID)
    {
        output.adminId = state.admin;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getTotalLockedTokens)
    {
        // Log for debugging
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0,                  // No error
            0,                  // No order ID involved
            state.lockedTokens, // Amount of locked tokens
            0 };
        LOG_INFO(locals.log);

        // Assign the value of lockedTokens to the output
        output.totalLockedTokens = state.lockedTokens;
    }

    // Structure for the input of the getOrderByDetails function
    struct getOrderByDetails_input
    {
        Array<uint8, 64> ethAddress; // Ethereum address
        uint64 amount;               // Transaction amount
        uint8 status;                // Order status (0 = created, 1 = completed, 2 = refunded)
    };

    // Structure for the output of the getOrderByDetails function
    struct getOrderByDetails_output
    {
        uint8 status;   // Operation status (0 = success, other = error)
        uint64 orderId; // ID of the found order
        id qubicDestination; // Destination address on Qubic (for EVM to Qubic orders)
    };

    // Function to search for an order by details
    struct getOrderByDetails_locals
    {
        uint64 i;
        uint64 j;
        bit addressMatch; // Flag to check if addresses match
        BridgeOrder order;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getOrderByDetails)
    {
        // Validate input parameters
        if (input.amount == 0)
        {
            output.status = 2; // Error: invalid amount
            output.orderId = 0;
            return;
        }

        // Iterate through all orders
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            locals.order = state.orders.get(locals.i);

            // Check if the order matches the criteria
            if (locals.order.status == 255) // Empty slot
                continue;

            // Compare ethAddress arrays element by element
            locals.addressMatch = true;
            for (locals.j = 0; locals.j < 42; ++locals.j)
            {
                if (locals.order.ethAddress.get(locals.j) != input.ethAddress.get(locals.j))
                {
                    locals.addressMatch = false;
                    break;
                }
            }

            // Verify exact match
            if (locals.addressMatch &&
                locals.order.amount == input.amount &&
                locals.order.status == input.status)
            {
                // Found an exact match
                output.status = 0; // Success
                output.orderId = locals.order.orderId;
                return;
            }
        }

        // If no matching order was found
        output.status = 1; // Not found
        output.orderId = 0;
    }

    // Add Liquidity structures
    struct addLiquidity_input
    {
        // No input parameters - amount comes from qpi.invocationReward()
    };

    struct addLiquidity_output
    {
        uint8 status;           // Operation status (0 = success, other = error)
        uint64 addedAmount;     // Amount of tokens added to liquidity
        uint64 totalLocked;     // Total locked tokens after addition
    };

    struct addLiquidity_locals
    {
        EthBridgeLogger log;
        id invocatorAddress;
        bit isManagerOperating;
        uint64 depositAmount;
    };

    // Add liquidity to the bridge (for managers to provide initial/additional liquidity)
    PUBLIC_PROCEDURE_WITH_LOCALS(addLiquidity)
    {
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);

        // Verify that the invocator is a manager or admin
        if (!locals.isManagerOperating && locals.invocatorAddress != state.admin)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::notAuthorized,
                0, // No order ID involved
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::notAuthorized;
            return;
        }

        // Get the amount of tokens sent with this call
        locals.depositAmount = qpi.invocationReward();

        // Validate that some tokens were sent
        if (locals.depositAmount == 0)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                0, // No order ID involved
                0, // No amount involved
                0
            };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // Add the deposited tokens to the locked tokens pool
        state.lockedTokens += locals.depositAmount;
        state.totalReceivedTokens += locals.depositAmount;

        // Log the successful liquidity addition
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            0, // No order ID involved
            locals.depositAmount, // Amount added
            0
        };
        LOG_INFO(locals.log);

        // Set output values
        output.status = 0; // Success
        output.addedAmount = locals.depositAmount;
        output.totalLocked = state.lockedTokens;
    }


    PUBLIC_FUNCTION(getAvailableFees)
    {
        output.availableFees = state._earnedFees - state._distributedFees;
        output.totalEarnedFees = state._earnedFees;
        output.totalDistributedFees = state._distributedFees;
    }


    struct getContractInfo_locals
    {
        uint64 i;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getContractInfo)
    {
        output.admin = state.admin;
        output.managers = state.managers;
        output.nextOrderId = state.nextOrderId;
        output.lockedTokens = state.lockedTokens;
        output.totalReceivedTokens = state.totalReceivedTokens;
        output.earnedFees = state._earnedFees;
        output.tradeFeeBillionths = state._tradeFeeBillionths;
        output.sourceChain = state.sourceChain;


        output.totalOrdersFound = 0;
        output.emptySlots = 0;

        for (locals.i = 0; locals.i < 16 && locals.i < state.orders.capacity(); ++locals.i)
        {
            output.firstOrders.set(locals.i, state.orders.get(locals.i));
        }

        // Count real orders vs empty ones
        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            if (state.orders.get(locals.i).status == 255)
            {
                output.emptySlots++;
            }
            else
            {
                output.totalOrdersFound++;
            }
        }
    }

    // Called at the end of every tick to distribute earned fees
    struct END_TICK_locals
    {
        uint64 feesToDistributeInThisTick;
        uint64 amountPerComputor;
        uint64 vottunFeesToDistribute;
    };

    END_TICK_WITH_LOCALS()
    {
        locals.feesToDistributeInThisTick = state._earnedFeesQubic - state._distributedFeesQubic;

        if (locals.feesToDistributeInThisTick > 0)
        {
            // Distribute fees to computors holding shares of this contract.
            // NUMBER_OF_COMPUTORS is a Qubic global constant (typically 676).
            locals.amountPerComputor = div(locals.feesToDistributeInThisTick, (uint64)NUMBER_OF_COMPUTORS);

            if (locals.amountPerComputor > 0)
            {
                if (qpi.distributeDividends(locals.amountPerComputor))
                {
                    state._distributedFeesQubic += locals.amountPerComputor * NUMBER_OF_COMPUTORS;
                }
            }
        }

        // Distribution of Vottun fees to feeRecipient
        locals.vottunFeesToDistribute = state._earnedFees - state._distributedFees;

        if (locals.vottunFeesToDistribute > 0 && state.feeRecipient != 0)
        {
            if (qpi.transfer(state.feeRecipient, locals.vottunFeesToDistribute))
            {
                state._distributedFees += locals.vottunFeesToDistribute;
            }
        }
    }

    // Register Functions and Procedures
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getOrder, 1);
        REGISTER_USER_FUNCTION(isAdmin, 2);
        REGISTER_USER_FUNCTION(isManager, 3);
        REGISTER_USER_FUNCTION(getTotalReceivedTokens, 4);
        REGISTER_USER_FUNCTION(getAdminID, 5);
        REGISTER_USER_FUNCTION(getTotalLockedTokens, 6);
        REGISTER_USER_FUNCTION(getOrderByDetails, 7);
        REGISTER_USER_FUNCTION(getContractInfo, 8);
        REGISTER_USER_FUNCTION(getAvailableFees, 9);
        REGISTER_USER_FUNCTION(getProposal, 10); // New multisig function

        REGISTER_USER_PROCEDURE(createOrder, 1);
        REGISTER_USER_PROCEDURE(setAdmin, 2);
        REGISTER_USER_PROCEDURE(addManager, 3);
        REGISTER_USER_PROCEDURE(removeManager, 4);
        REGISTER_USER_PROCEDURE(completeOrder, 5);
        REGISTER_USER_PROCEDURE(refundOrder, 6);
        REGISTER_USER_PROCEDURE(transferToContract, 7);
        REGISTER_USER_PROCEDURE(withdrawFees, 8);
        REGISTER_USER_PROCEDURE(addLiquidity, 9);
        REGISTER_USER_PROCEDURE(createProposal, 10); // New multisig procedure
        REGISTER_USER_PROCEDURE(approveProposal, 11); // New multisig procedure
    }

    // Initialize the contract with SECURE ADMIN CONFIGURATION
    struct INITIALIZE_locals
    {
        uint64 i;
        BridgeOrder emptyOrder;
    };

    INITIALIZE_WITH_LOCALS()
    {
        state.admin = ID(_X, _A, _B, _E, _F, _A, _B, _I, _H, _W, _R, _W, _B, _A, _I, _J, _Q, _J, _P, _W, _T, _I, _I, _Q, _B, _U, _C, _B, _H, _B, _V, _W, _Y, _Y, _G, _F, _F, _J, _A, _D, _Q, _B, _K, _W, _F, _B, _O, _R, _R, _V, _X, _W, _S, _C, _V, _B);

        //Initialize the wallet that receives fees (REPLACE WITH YOUR WALLET)
        // state.feeRecipient = ID(_YOUR, _WALLET, _HERE, _PLACEHOLDER, _UNTIL, _YOU, _PUT, _THE, _REAL, _WALLET, _ADDRESS, _FROM, _VOTTUN, _TO, _RECEIVE, _THE, _BRIDGE, _FEES, _BETWEEN, _QUBIC, _AND, _ETHEREUM, _WITH, _HALF, _PERCENT, _COMMISSION, _A, _B, _C, _D, _E, _F, _G, _H, _I, _J, _K, _L, _M, _N, _O, _P, _Q, _R, _S, _T, _U, _V);

        // Initialize the orders array. Good practice to zero first.
        locals.emptyOrder = {};         // Sets all fields to 0 (including orderId and status).
        locals.emptyOrder.status = 255; // Then set your status for empty.

        for (locals.i = 0; locals.i < state.orders.capacity(); ++locals.i)
        {
            state.orders.set(locals.i, locals.emptyOrder);
        }

        // Initialize the managers array with NULL_ID to mark slots as empty
        for (locals.i = 0; locals.i < state.managers.capacity(); ++locals.i)
        {
            state.managers.set(locals.i, NULL_ID);
        }

        // Add the initial manager
        state.managers.set(0, ID(_X, _A, _B, _E, _F, _A, _B, _I, _H, _W, _R, _W, _B, _A, _I, _J, _Q, _J, _P, _W, _T, _I, _I, _Q, _B, _U, _C, _B, _H, _B, _V, _W, _Y, _Y, _G, _F, _F, _J, _A, _D, _Q, _B, _K, _W, _F, _B, _O, _R, _R, _V, _X, _W, _S, _C, _V, _B));

        // Initialize the rest of the state variables
        state.nextOrderId = 1; // Start from 1 to avoid ID 0
        state.lockedTokens = 0;
        state.totalReceivedTokens = 0;
        state.sourceChain = 0; // Arbitrary number. No-EVM chain

        // Initialize fee variables
        state._tradeFeeBillionths = 5000000; // 0.5% == 5,000,000 / 1,000,000,000
        state._earnedFees = 0;
        state._distributedFees = 0;

        state._earnedFeesQubic = 0;
        state._distributedFeesQubic = 0;

        // Initialize multisig admins (3 admins, requires 2 approvals)
        state.numberOfAdmins = 3;
        state.requiredApprovals = 2; // 2 of 3 threshold

        // Initialize admins array (REPLACE WITH ACTUAL ADMIN ADDRESSES)
        state.admins.set(0, ID(_X, _A, _B, _E, _F, _A, _B, _I, _H, _W, _R, _W, _B, _A, _I, _J, _Q, _J, _P, _W, _T, _I, _I, _Q, _B, _U, _C, _B, _H, _B, _V, _W, _Y, _Y, _G, _F, _F, _J, _A, _D, _Q, _B, _K, _W, _F, _B, _O, _R, _R, _V, _X, _W, _S, _C, _V, _B)); // Admin 1
        state.admins.set(1, ID(_E, _Q, _M, _B, _B, _V, _Y, _G, _Z, _O, _F, _U, _I, _H, _E, _X, _F, _O, _X, _K, _T, _F, _T, _A, _N, _E, _K, _B, _X, _L, _B, _X, _H, _A, _Y, _D, _F, _F, _M, _R, _E, _E, _M, _R, _Q, _E, _V, _A, _D, _Y, _M, _M, _E, _W, _A, _C)); // Admin 2 (Manager)
        state.admins.set(2, ID(_H, _Y, _J, _X, _E, _Z, _S, _E, _C, _W, _S, _K, _O, _D, _J, _A, _L, _R, _C, _K, _S, _L, _K, _V, _Y, _U, _E, _B, _M, _A, _H, _D, _O, _D, _Y, _Z, _U, _J, _I, _I, _Y, _D, _P, _A, _G, _F, _K, _L, _M, _O, _T, _H, _T, _J, _X, _E)); // Admin 3 (User)

        // Initialize remaining admin slots
        for (locals.i = 3; locals.i < state.admins.capacity(); ++locals.i)
        {
            state.admins.set(locals.i, NULL_ID);
        }

        // Initialize proposals array
        state.nextProposalId = 1;
        // Don't initialize proposals array - leave as default (all zeros)
    }
};
