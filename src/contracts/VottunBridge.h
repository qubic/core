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
        uint8 status;                        // Order status (0=pending, 1=completed, 2=refunded)
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

    struct getContractInfo_input
    {
        // No parameters
    };

    struct getContractInfo_output
    {
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
        // Multisig info
        Array<id, 16> multisigAdmins;       // List of multisig admins
        uint8 numberOfAdmins;               // Number of active admins
        uint8 requiredApprovals;            // Required approvals threshold
        uint64 totalProposals;              // Total number of active proposals
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
        id targetAddress;             // For setAdmin/addManager/removeManager (new admin address)
        id oldAddress;                // For setAdmin: which admin to replace
        uint64 amount;                // For withdrawFees or changeThreshold
        Array<id, 16> approvals;      // Array of owner IDs who approved
        uint8 approvalsCount;         // Count of approvals
        bit executed;                 // Whether proposal was executed
        bit active;                   // Whether proposal is active (not cancelled)
    };

public:
    // Contract State
    Array<BridgeOrder, 1024> orders;
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
    uint64 _reservedFees;            // Fees reserved for pending orders (not distributed yet)
    uint64 _reservedFeesQubic;       // Qubic fees reserved for pending orders (not distributed yet)

    // Multisig state
    Array<id, 16> admins;            // List of multisig admins
    uint8 numberOfAdmins;            // Number of active admins
    uint8 requiredApprovals;         // Threshold: number of approvals needed (2 of 3)
    Array<AdminProposal, 32> proposals; // Pending admin proposals
    uint64 nextProposalId;           // Counter for proposal IDs

    // Internal methods for admin/manager permissions
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
            output.status = EthBridgeError::insufficientTransactionFee;
            return;
        }

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

                // Accumulate fees only after order is successfully created
                state._earnedFees += locals.requiredFeeEth;
                state._earnedFeesQubic += locals.requiredFeeQubic;

                // Reserve fees for this pending order (won't be distributed until complete/refund)
                state._reservedFees += locals.requiredFeeEth;
                state._reservedFeesQubic += locals.requiredFeeQubic;

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
                if (state.orders.get(locals.j).status == 1 || state.orders.get(locals.j).status == 2) // Completed or Refunded
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

                        // Accumulate fees only after order is successfully created
                        state._earnedFees += locals.requiredFeeEth;
                        state._earnedFeesQubic += locals.requiredFeeQubic;

                        // Reserve fees for this pending order (won't be distributed until complete/refund)
                        state._reservedFees += locals.requiredFeeEth;
                        state._reservedFeesQubic += locals.requiredFeeQubic;

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
                locals.orderResp.status = locals.order.status;

                output.status = 0; // Success
                output.order = locals.orderResp;
                return;
            }
        }

        // If order not found
        output.status = 1; // Error
    }

    // Multisig Proposal Functions

    // Create proposal structures
    struct createProposal_input
    {
        uint8 proposalType;      // Type of proposal
        id targetAddress;        // Target address (new admin/manager address)
        id oldAddress;           // Old address (for setAdmin: which admin to replace)
        uint64 amount;           // Amount (for withdrawFees or changeThreshold)
    };

    struct createProposal_output
    {
        uint8 status;
        uint64 proposalId;
    };

    struct createProposal_locals
    {
        EthBridgeLogger log;
        id invocatorAddress;
        uint64 i;
        uint64 j;
        bit slotFound;
        uint64 slotIndex;
        AdminProposal newProposal;
        AdminProposal emptyProposal;
        bit isMultisigAdminResult;
        uint64 freeSlots;
        uint64 cleanedSlots;
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
        id invocatorAddress;
        AddressChangeLogger adminLog;
        AdminProposal proposal;
        uint64 i;
        bit found;
        bit alreadyApproved;
        bit isMultisigAdminResult;
        uint64 proposalIndex;
        uint64 availableFees;
        bit adminAdded;
        uint64 managerCount;
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
        locals.invocatorAddress = qpi.invocator();
        CALL(isMultisigAdmin, locals.invocatorAddress, locals.isMultisigAdminResult);
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

        // Count free slots and find an empty slot for the proposal
        locals.slotFound = false;
        locals.freeSlots = 0;
        locals.slotIndex = 0;
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            if (!state.proposals.get(locals.i).active && state.proposals.get(locals.i).proposalId == 0)
            {
                locals.freeSlots++;
                if (!locals.slotFound)
                {
                    locals.slotFound = true;
                    locals.slotIndex = locals.i; // Save the slot index
                    // Don't break, continue counting free slots
                }
            }
        }

        // If found slot but less than 5 free slots, cleanup executed or inactive proposals
        if (locals.slotFound && locals.freeSlots < 5)
        {
            locals.cleanedSlots = 0;
            for (locals.j = 0; locals.j < state.proposals.capacity(); ++locals.j)
            {
                // Clean executed proposals OR inactive proposals with a proposalId (failed/abandoned)
                if (state.proposals.get(locals.j).executed ||
                    (!state.proposals.get(locals.j).active && state.proposals.get(locals.j).proposalId > 0))
                {
                    // Clear proposal
                    locals.emptyProposal.proposalId = 0;
                    locals.emptyProposal.proposalType = 0;
                    locals.emptyProposal.approvalsCount = 0;
                    locals.emptyProposal.executed = false;
                    locals.emptyProposal.active = false;
                    state.proposals.set(locals.j, locals.emptyProposal);
                    locals.cleanedSlots++;
                }
            }
        }

        // If no slot found at all, try cleanup and search again
        if (!locals.slotFound)
        {
            // Attempt cleanup of executed or inactive proposals
            locals.cleanedSlots = 0;
            for (locals.j = 0; locals.j < state.proposals.capacity(); ++locals.j)
            {
                // Clean executed proposals OR inactive proposals with a proposalId (failed/abandoned)
                if (state.proposals.get(locals.j).executed ||
                    (!state.proposals.get(locals.j).active && state.proposals.get(locals.j).proposalId > 0))
                {
                    // Clear proposal
                    locals.emptyProposal.proposalId = 0;
                    locals.emptyProposal.proposalType = 0;
                    locals.emptyProposal.approvalsCount = 0;
                    locals.emptyProposal.executed = false;
                    locals.emptyProposal.active = false;
                    state.proposals.set(locals.j, locals.emptyProposal);
                    locals.cleanedSlots++;
                }
            }

            // Try to find slot again after cleanup
            if (locals.cleanedSlots > 0)
            {
                for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
                {
                    if (!state.proposals.get(locals.i).active && state.proposals.get(locals.i).proposalId == 0)
                    {
                        locals.slotFound = true;
                        locals.slotIndex = locals.i; // Save the slot index
                        break;
                    }
                }
            }

            // If still no slot available
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
        }

        // Create the new proposal
        locals.newProposal.proposalId = state.nextProposalId++;
        locals.newProposal.proposalType = input.proposalType;
        locals.newProposal.targetAddress = input.targetAddress;
        locals.newProposal.oldAddress = input.oldAddress;
        locals.newProposal.amount = input.amount;
        locals.newProposal.approvalsCount = 1; // Creator automatically approves
        locals.newProposal.executed = false;
        locals.newProposal.active = true;

        // Set creator as first approver
        locals.newProposal.approvals.set(0, qpi.invocator());

        // Store the proposal
        state.proposals.set(locals.slotIndex, locals.newProposal);

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
        locals.invocatorAddress = qpi.invocator();
        CALL(isMultisigAdmin, locals.invocatorAddress, locals.isMultisigAdminResult);
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
                // Replace existing admin with new admin (max 3 admins: 2 of 3 multisig)
                // oldAddress specifies which admin to replace

                // SECURITY: Check that targetAddress is not already an admin (prevent duplicates)
                locals.adminAdded = false;
                for (locals.i = 0; locals.i < (uint64)state.numberOfAdmins; ++locals.i)
                {
                    if (state.admins.get(locals.i) == locals.proposal.targetAddress)
                    {
                        // targetAddress is already an admin, reject to prevent duplicate voting power
                        locals.adminAdded = true; // Reuse flag to indicate rejection
                        break;
                    }
                }

                // Only proceed if targetAddress is not already an admin
                if (!locals.adminAdded)
                {
                    for (locals.i = 0; locals.i < state.admins.capacity(); ++locals.i)
                    {
                        if (state.admins.get(locals.i) == locals.proposal.oldAddress)
                        {
                            // Replace the old admin with the new one
                            state.admins.set(locals.i, locals.proposal.targetAddress);
                            locals.adminAdded = true;
                            locals.adminLog = AddressChangeLogger{
                                locals.proposal.targetAddress,
                                CONTRACT_INDEX,
                                1, // Admin changed
                                0 };
                            LOG_INFO(locals.adminLog);
                            break;
                        }
                    }
                }
                // numberOfAdmins stays the same (we're replacing, not adding)
            }
            else if (locals.proposal.proposalType == PROPOSAL_ADD_MANAGER)
            {
                // SECURITY: Check that targetAddress is not already a manager (prevent duplicates)
                locals.adminAdded = false;
                locals.managerCount = 0;
                for (locals.i = 0; locals.i < state.managers.capacity(); ++locals.i)
                {
                    if (state.managers.get(locals.i) == locals.proposal.targetAddress)
                    {
                        // targetAddress is already a manager, reject
                        locals.adminAdded = true;
                        break;
                    }
                    if (state.managers.get(locals.i) != NULL_ID)
                    {
                        locals.managerCount++;
                    }
                }

                // LIMIT: Check that we don't exceed 3 managers
                if (locals.managerCount >= 3)
                {
                    locals.adminAdded = true; // Reject if already 3 managers
                }

                // Only proceed if targetAddress is not already a manager and limit not reached
                if (!locals.adminAdded)
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
                // Calculate available fees (excluding reserved fees for pending orders)
                locals.availableFees = (state._earnedFees > (state._distributedFees + state._reservedFees))
                                          ? (state._earnedFees - state._distributedFees - state._reservedFees)
                                          : 0;
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
                // Hard limit: minimum threshold is 2 to maintain multisig security
                if (locals.proposal.amount >= 2 && locals.proposal.amount <= (uint64)state.numberOfAdmins)
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

    // Cancel proposal structures
    struct cancelProposal_input
    {
        uint64 proposalId;
    };

    struct cancelProposal_output
    {
        uint8 status;
    };

    struct cancelProposal_locals
    {
        EthBridgeLogger log;
        AdminProposal proposal;
        uint64 i;
        bit found;
    };

    // Cancel a proposal (only the creator can cancel)
    PUBLIC_PROCEDURE_WITH_LOCALS(cancelProposal)
    {
        // Find the proposal
        locals.found = false;
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            locals.proposal = state.proposals.get(locals.i);
            if (locals.proposal.proposalId == input.proposalId && locals.proposal.active)
            {
                locals.found = true;
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
            return;
        }

        // Verify that the invocator is the creator (first approver)
        if (locals.proposal.approvals.get(0) != qpi.invocator())
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::notAuthorized,
                input.proposalId,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::notAuthorized;
            return;
        }

        // Cancel the proposal by marking it as inactive
        locals.proposal.active = false;
        state.proposals.set(locals.i, locals.proposal);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.proposalId,
            0,
            0 };
        LOG_INFO(locals.log);

        output.status = 0; // Success
    }

    // Admin Functions (now deprecated - use multisig proposals)
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
        uint64 feeOperator;
        uint64 feeNetwork;
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

        // Release reserved fees now that order is completed (fees can now be distributed)
        locals.feeOperator = div(locals.order.amount * state._tradeFeeBillionths, 1000000000ULL);
        locals.feeNetwork = div(locals.order.amount * state._tradeFeeBillionths, 1000000000ULL);

        // UNDERFLOW PROTECTION: Only release if enough reserved
        if (state._reservedFees >= locals.feeOperator)
        {
            state._reservedFees -= locals.feeOperator;
        }
        if (state._reservedFeesQubic >= locals.feeNetwork)
        {
            state._reservedFeesQubic -= locals.feeNetwork;
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
        uint64 feeOperator;
        uint64 feeNetwork;
        uint64 totalRefund;
        uint64 availableFeesOperator;
        uint64 availableFeesNetwork;
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
                // No tokens to return, but refund fees
                // Calculate fees to refund (theoretical)
                locals.feeOperator = div(locals.order.amount * state._tradeFeeBillionths, 1000000000ULL);
                locals.feeNetwork = div(locals.order.amount * state._tradeFeeBillionths, 1000000000ULL);

                // Track actually refunded amounts
                locals.totalRefund = 0;

                // Release reserved fees and deduct from earned (UNDERFLOW PROTECTION)
                if (state._reservedFees >= locals.feeOperator && state._earnedFees >= locals.feeOperator)
                {
                    state._reservedFees -= locals.feeOperator;
                    state._earnedFees -= locals.feeOperator;
                    locals.totalRefund += locals.feeOperator;
                }
                if (state._reservedFeesQubic >= locals.feeNetwork && state._earnedFeesQubic >= locals.feeNetwork)
                {
                    state._reservedFeesQubic -= locals.feeNetwork;
                    state._earnedFeesQubic -= locals.feeNetwork;
                    locals.totalRefund += locals.feeNetwork;
                }

                // Transfer fees back to user
                if (qpi.transfer(locals.order.qubicSender, locals.totalRefund) < 0)
                {
                    locals.log = EthBridgeLogger{
                        CONTRACT_INDEX,
                        EthBridgeError::transferFailed,
                        input.orderId,
                        locals.totalRefund,
                        0 };
                    LOG_INFO(locals.log);
                    output.status = EthBridgeError::transferFailed;
                    return;
                }

                // Mark order as refunded
                locals.order.status = 2;
                state.orders.set(locals.i, locals.order);

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0,
                    input.orderId,
                    locals.totalRefund,
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

            // Calculate fees to refund (theoretical)
            locals.feeOperator = div(locals.order.amount * state._tradeFeeBillionths, 1000000000ULL);
            locals.feeNetwork = div(locals.order.amount * state._tradeFeeBillionths, 1000000000ULL);

            // Start with order amount
            locals.totalRefund = locals.order.amount;

            // Release reserved fees and deduct from earned (UNDERFLOW PROTECTION)
            if (state._reservedFees >= locals.feeOperator && state._earnedFees >= locals.feeOperator)
            {
                state._reservedFees -= locals.feeOperator;
                state._earnedFees -= locals.feeOperator;
                locals.totalRefund += locals.feeOperator;
            }
            if (state._reservedFeesQubic >= locals.feeNetwork && state._earnedFeesQubic >= locals.feeNetwork)
            {
                state._reservedFeesQubic -= locals.feeNetwork;
                state._earnedFeesQubic -= locals.feeNetwork;
                locals.totalRefund += locals.feeNetwork;
            }

            // Return tokens + fees to original sender
            if (qpi.transfer(locals.order.qubicSender, locals.totalRefund) < 0)
            {
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::transferFailed,
                    input.orderId,
                    locals.totalRefund,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::transferFailed;
                return;
            }

            // Update locked tokens balance
            state.lockedTokens -= locals.order.amount;
        }
        // Note: For EVM to Qubic orders, tokens were already transferred in completeOrder
        // No refund needed on Qubic side (fees were paid on Ethereum side)

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
        uint64 depositAmount;
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
            // Tokens must be provided with the invocation (invocationReward)
            locals.depositAmount = qpi.invocationReward();

            // Check if user sent enough tokens
            if (locals.depositAmount < input.amount)
            {
                // Not enough - refund everything and return error
                if (locals.depositAmount > 0)
                {
                    qpi.transfer(qpi.invocator(), locals.depositAmount);
                }
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

            // Lock only the required amount
            state.lockedTokens += input.amount;
            state.totalReceivedTokens += input.amount;

            // Refund excess if user sent too much
            if (locals.depositAmount > input.amount)
            {
                qpi.transfer(qpi.invocator(), locals.depositAmount - input.amount);
            }
            
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


    PUBLIC_FUNCTION_WITH_LOCALS(getTotalLockedTokens)
    {
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
        bit isMultisigAdminResult;
        uint64 depositAmount;
    };

    // Add liquidity to the bridge (for managers or multisig admins to provide initial/additional liquidity)
    PUBLIC_PROCEDURE_WITH_LOCALS(addLiquidity)
    {
        locals.invocatorAddress = qpi.invocator();
        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);

        locals.isMultisigAdminResult = false;
        CALL(isMultisigAdmin, locals.invocatorAddress, locals.isMultisigAdminResult);

        // Verify that the invocator is a manager or multisig admin
        if (!locals.isManagerOperating && !locals.isMultisigAdminResult)
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
        // Available fees exclude those reserved for pending orders
        output.availableFees = (state._earnedFees > (state._distributedFees + state._reservedFees))
                                 ? (state._earnedFees - state._distributedFees - state._reservedFees)
                                 : 0;
        output.totalEarnedFees = state._earnedFees;
        output.totalDistributedFees = state._distributedFees;
    }


    struct getContractInfo_locals
    {
        uint64 i;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getContractInfo)
    {
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

        // Multisig info
        output.multisigAdmins = state.admins;
        output.numberOfAdmins = state.numberOfAdmins;
        output.requiredApprovals = state.requiredApprovals;

        // Count active proposals
        output.totalProposals = 0;
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            if (state.proposals.get(locals.i).active && state.proposals.get(locals.i).proposalId > 0)
            {
                output.totalProposals++;
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
        // Calculate available fees for distribution (earned - distributed - reserved for pending orders)
        locals.feesToDistributeInThisTick = (state._earnedFeesQubic > (state._distributedFeesQubic + state._reservedFeesQubic))
                                              ? (state._earnedFeesQubic - state._distributedFeesQubic - state._reservedFeesQubic)
                                              : 0;

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

        // Distribution of Vottun fees to feeRecipient (excluding reserved fees)
        locals.vottunFeesToDistribute = (state._earnedFees > (state._distributedFees + state._reservedFees))
                                          ? (state._earnedFees - state._distributedFees - state._reservedFees)
                                          : 0;

        if (locals.vottunFeesToDistribute > 0 && state.feeRecipient != NULL_ID)
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
        REGISTER_USER_FUNCTION(isManager, 2);
        REGISTER_USER_FUNCTION(getTotalReceivedTokens, 3);
        REGISTER_USER_FUNCTION(getTotalLockedTokens, 4);
        REGISTER_USER_FUNCTION(getOrderByDetails, 5);
        REGISTER_USER_FUNCTION(getContractInfo, 6);
        REGISTER_USER_FUNCTION(getAvailableFees, 7);
        REGISTER_USER_FUNCTION(getProposal, 8);

        REGISTER_USER_PROCEDURE(createOrder, 1);
        REGISTER_USER_PROCEDURE(addManager, 2);
        REGISTER_USER_PROCEDURE(removeManager, 3);
        REGISTER_USER_PROCEDURE(completeOrder, 4);
        REGISTER_USER_PROCEDURE(refundOrder, 5);
        REGISTER_USER_PROCEDURE(transferToContract, 6);
        REGISTER_USER_PROCEDURE(withdrawFees, 7);
        REGISTER_USER_PROCEDURE(addLiquidity, 8);
        REGISTER_USER_PROCEDURE(createProposal, 9);
        REGISTER_USER_PROCEDURE(approveProposal, 10);
        REGISTER_USER_PROCEDURE(cancelProposal, 11);
    }

    // Initialize the contract with SECURE ADMIN CONFIGURATION
    struct INITIALIZE_locals
    {
        uint64 i;
        BridgeOrder emptyOrder;
        AdminProposal emptyProposal;
    };

    INITIALIZE_WITH_LOCALS()
    {
        // Initialize the wallet that receives operator fees (Vottun)
        state.feeRecipient = ID(_W, _N, _J, _B, _D, _V, _U, _C, _V, _P, _I, _W, _X, _B, _M, _R, _C, _K, _Z, _E, _C, _Y, _L, _G, _E, _V, _A, _D, _S, _Q, _M, _Y, _S, _R, _F, _Q, _I, _U, _S, _V, _O, _G, _C, _G, _M, _K, _P, _I, _Y, _J, _F, _C, _Z, _F, _B, _A);

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

        state._reservedFees = 0;
        state._reservedFeesQubic = 0;

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

        // Initialize proposals array properly (like orders array)
        state.nextProposalId = 1;

        // Initialize emptyProposal fields explicitly (avoid memset)
        locals.emptyProposal.proposalId = 0;
        locals.emptyProposal.proposalType = 0;
        locals.emptyProposal.targetAddress = NULL_ID;
        locals.emptyProposal.amount = 0;
        locals.emptyProposal.approvalsCount = 0;
        locals.emptyProposal.executed = false;
        locals.emptyProposal.active = false;
        // Initialize approvals array with NULL_ID
        for (locals.i = 0; locals.i < locals.emptyProposal.approvals.capacity(); ++locals.i)
        {
            locals.emptyProposal.approvals.set(locals.i, NULL_ID);
        }

        // Set all proposal slots with the empty proposal
        for (locals.i = 0; locals.i < state.proposals.capacity(); ++locals.i)
        {
            state.proposals.set(locals.i, locals.emptyProposal);
        }
    }
};
