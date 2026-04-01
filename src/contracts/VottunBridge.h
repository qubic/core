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

    struct getTotalReceivedTokens_input
    {
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
        maxProposalsReached = 15,
        noAvailableSlots = 16,
        belowMinimumAmount = 17
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
    struct StateData
    {
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
        uint64 minimumOrderAmount;       // Minimum order amount to prevent zero-fee spam

        // Multisig state
        Array<id, 16> admins;            // List of multisig admins
        uint8 numberOfAdmins;            // Number of active admins
        uint8 requiredApprovals;         // Threshold: number of approvals needed (2 of 3)
        Array<AdminProposal, 32> proposals; // Pending admin proposals
        uint64 nextProposalId;           // Counter for proposal IDs
    };

    // Internal methods for admin/manager permissions
    typedef id isManager_input;
    typedef bit isManager_output;

    struct isManager_locals
    {
        uint64 i;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(isManager)
    {
        for (locals.i = 0; locals.i < state.get().managers.capacity(); ++locals.i)
        {
            if (state.get().managers.get(locals.i) == input)
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
        for (locals.i = 0; locals.i < (uint64)state.get().numberOfAdmins; ++locals.i)
        {
            if (state.get().admins.get(locals.i) == input)
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
        bit recyclableFound;
        uint64 requiredFeeEth;
        uint64 requiredFeeQubic;
        uint64 totalRequiredFee;
        uint64 invReward;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(createOrder)
    {
        // [QVB-11] Capture invocationReward immediately
        locals.invReward = qpi.invocationReward();

        // [QVB-09] Validate minimum order amount (prevents zero-fee spam)
        if (input.amount < state.get().minimumOrderAmount)
        {
            if (locals.invReward > 0) qpi.transfer(qpi.invocator(), locals.invReward);
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::belowMinimumAmount,
                0,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::belowMinimumAmount;
            return;
        }

        // Calculate fees as percentage of amount (0.5% each, 1% total)
        locals.requiredFeeEth = div(input.amount * state.get()._tradeFeeBillionths, 1000000000ULL);
        locals.requiredFeeQubic = div(input.amount * state.get()._tradeFeeBillionths, 1000000000ULL);
        locals.totalRequiredFee = locals.requiredFeeEth + locals.requiredFeeQubic;

        // [QVB-09] Safety check: reject if calculated fee rounds to zero
        if (locals.totalRequiredFee == 0)
        {
            if (locals.invReward > 0) qpi.transfer(qpi.invocator(), locals.invReward);
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

        // [QVB-11] Verify that the fee paid is sufficient
        if (static_cast<sint64>(locals.invReward) < static_cast<sint64>(locals.totalRequiredFee))
        {
            if (locals.invReward > 0) qpi.transfer(qpi.invocator(), locals.invReward);
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

        // [QVB-18] Validate ethAddress is not all zeros (check first 20 bytes)
        locals.slotFound = false; // Reuse temporarily as "has non-zero byte" flag
        for (locals.i = 0; locals.i < 20; ++locals.i)
        {
            if (input.ethAddress.get(locals.i) != 0)
            {
                locals.slotFound = true;
                break;
            }
        }
        if (!locals.slotFound)
        {
            if (locals.invReward > 0) qpi.transfer(qpi.invocator(), locals.invReward);
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                0,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // [QVB-17] Set order fields WITHOUT incrementing nextOrderId yet
        locals.newOrder.orderId = state.get().nextOrderId; // No ++ here
        locals.newOrder.qubicSender = qpi.invocator();

        // Set qubicDestination according to the direction
        if (!input.fromQubicToEthereum)
        {
            // EVM → Qubic
            locals.newOrder.qubicDestination = input.qubicDestination;

            // [QVB-09] Check and RESERVE liquidity
            if (state.get().lockedTokens < input.amount)
            {
                if (locals.invReward > 0) qpi.transfer(qpi.invocator(), locals.invReward);
                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::insufficientLockedTokens,
                    0,
                    input.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::insufficientLockedTokens;
                return;
            }
            state.mut().lockedTokens -= input.amount;
        }
        else
        {
            // Qubic → EVM
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

        // [QVB-02] Single-pass slot allocation: find empty slot OR track first recyclable
        locals.slotFound = false;
        locals.recyclableFound = false;
        locals.j = 0;

        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            if (state.get().orders.get(locals.i).status == 255)
            {
                // Empty slot found - use directly
                locals.newOrder.orderId = state.mut().nextOrderId++; // [QVB-17] Increment only on success
                state.mut().orders.set(locals.i, locals.newOrder);

                state.mut()._earnedFees += locals.requiredFeeEth;
                state.mut()._earnedFeesQubic += locals.requiredFeeQubic;
                state.mut()._reservedFees += locals.requiredFeeEth;
                state.mut()._reservedFeesQubic += locals.requiredFeeQubic;

                // [QVB-11] Refund excess invocationReward
                if (locals.invReward > locals.totalRequiredFee)
                {
                    qpi.transfer(qpi.invocator(), locals.invReward - locals.totalRequiredFee);
                }

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    0,
                    locals.newOrder.orderId,
                    input.amount,
                    0 };
                LOG_INFO(locals.log);
                output.status = 0;
                output.orderId = locals.newOrder.orderId;
                return;
            }
            else if (!locals.recyclableFound &&
                     (state.get().orders.get(locals.i).status == 1 || state.get().orders.get(locals.i).status == 2))
            {
                // Track first recyclable slot (completed or refunded)
                locals.recyclableFound = true;
                locals.j = locals.i;
            }
        }

        // No empty slot. Try recyclable slot.
        if (locals.recyclableFound)
        {
            locals.newOrder.orderId = state.mut().nextOrderId++; // [QVB-17] Increment only on success
            state.mut().orders.set(locals.j, locals.newOrder);

            state.mut()._earnedFees += locals.requiredFeeEth;
            state.mut()._earnedFeesQubic += locals.requiredFeeQubic;
            state.mut()._reservedFees += locals.requiredFeeEth;
            state.mut()._reservedFeesQubic += locals.requiredFeeQubic;

            // [QVB-11] Refund excess invocationReward
            if (locals.invReward > locals.totalRequiredFee)
            {
                qpi.transfer(qpi.invocator(), locals.invReward - locals.totalRequiredFee);
            }

            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                0,
                locals.newOrder.orderId,
                input.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = 0;
            output.orderId = locals.newOrder.orderId;
            return;
        }

        // [QVB-09] Undo liquidity reservation if we can't create the order
        if (!input.fromQubicToEthereum)
        {
            state.mut().lockedTokens += input.amount;
        }

        // No slots available at all - refund everything
        if (locals.invReward > 0) qpi.transfer(qpi.invocator(), locals.invReward);
        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            EthBridgeError::noAvailableSlots,
            0,
            0,
            0 };
        LOG_INFO(locals.log);
        output.status = EthBridgeError::noAvailableSlots;
        return;
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
        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            locals.order = state.get().orders.get(locals.i);
            if (locals.order.orderId == input.orderId && locals.order.status != 255)
            {
                // Populate OrderResponse with BridgeOrder data
                locals.orderResp.orderId = locals.order.orderId;
                locals.orderResp.originAccount = locals.order.qubicSender;
                locals.orderResp.destinationAccount = locals.order.ethAddress;
                locals.orderResp.amount = locals.order.amount;
                locals.orderResp.sourceChain = state.get().sourceChain;
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
        bit recyclableFound;
        uint64 slotIndex;
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
        bit actionSucceeded;
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
                EthBridgeError::invalidAmount,
                0,
                0,
                0 };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // [QVB-18] Validate targetAddress for proposals that require it
        if (input.proposalType == PROPOSAL_SET_ADMIN ||
            input.proposalType == PROPOSAL_ADD_MANAGER ||
            input.proposalType == PROPOSAL_REMOVE_MANAGER)
        {
            if (input.targetAddress == NULL_ID)
            {
                output.status = EthBridgeError::invalidAmount;
                return;
            }
        }
        if (input.proposalType == PROPOSAL_SET_ADMIN)
        {
            if (input.oldAddress == NULL_ID)
            {
                output.status = EthBridgeError::invalidAmount;
                return;
            }
        }

        // [QVB-03] Single-pass: find empty slot AND track first recyclable
        locals.slotFound = false;
        locals.recyclableFound = false;
        locals.slotIndex = 0;
        locals.j = 0;

        for (locals.i = 0; locals.i < state.get().proposals.capacity(); ++locals.i)
        {
            if (!state.get().proposals.get(locals.i).active && state.get().proposals.get(locals.i).proposalId == 0)
            {
                // Empty slot
                if (!locals.slotFound)
                {
                    locals.slotFound = true;
                    locals.slotIndex = locals.i;
                }
            }
            else if (!locals.recyclableFound &&
                     (state.get().proposals.get(locals.i).executed ||
                      (!state.get().proposals.get(locals.i).active && state.get().proposals.get(locals.i).proposalId > 0)))
            {
                // First recyclable slot (executed or abandoned)
                locals.recyclableFound = true;
                locals.j = locals.i;
            }
        }

        // Use empty slot if found, otherwise recycle
        if (!locals.slotFound && locals.recyclableFound)
        {
            // Reuse the recyclable slot directly (will be overwritten with new proposal below)
            locals.slotFound = true;
            locals.slotIndex = locals.j;
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
        locals.newProposal.proposalId = state.mut().nextProposalId++;
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
        state.mut().proposals.set(locals.slotIndex, locals.newProposal);

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
        for (locals.i = 0; locals.i < state.get().proposals.capacity(); ++locals.i)
        {
            locals.proposal = state.get().proposals.get(locals.i);
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
        if (locals.proposal.approvalsCount >= state.get().requiredApprovals)
        {
            // [QVB-20] Track whether the action actually succeeded
            locals.actionSucceeded = false;

            // Execute the proposal based on type
            if (locals.proposal.proposalType == PROPOSAL_SET_ADMIN)
            {
                // Replace existing admin with new admin (max 3 admins: 2 of 3 multisig)
                locals.adminAdded = false;
                for (locals.i = 0; locals.i < (uint64)state.get().numberOfAdmins; ++locals.i)
                {
                    if (state.get().admins.get(locals.i) == locals.proposal.targetAddress)
                    {
                        locals.adminAdded = true;
                        break;
                    }
                }

                if (!locals.adminAdded)
                {
                    for (locals.i = 0; locals.i < state.get().admins.capacity(); ++locals.i)
                    {
                        if (state.get().admins.get(locals.i) == locals.proposal.oldAddress)
                        {
                            state.mut().admins.set(locals.i, locals.proposal.targetAddress);
                            locals.actionSucceeded = true;
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
            }
            else if (locals.proposal.proposalType == PROPOSAL_ADD_MANAGER)
            {
                locals.adminAdded = false;
                locals.managerCount = 0;
                for (locals.i = 0; locals.i < state.get().managers.capacity(); ++locals.i)
                {
                    if (state.get().managers.get(locals.i) == locals.proposal.targetAddress)
                    {
                        locals.adminAdded = true;
                        break;
                    }
                    if (state.get().managers.get(locals.i) != NULL_ID)
                    {
                        locals.managerCount++;
                    }
                }

                if (locals.managerCount >= 3)
                {
                    locals.adminAdded = true;
                }

                if (!locals.adminAdded)
                {
                    for (locals.i = 0; locals.i < state.get().managers.capacity(); ++locals.i)
                    {
                        if (state.get().managers.get(locals.i) == NULL_ID)
                        {
                            state.mut().managers.set(locals.i, locals.proposal.targetAddress);
                            locals.actionSucceeded = true;
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
                for (locals.i = 0; locals.i < state.get().managers.capacity(); ++locals.i)
                {
                    if (state.get().managers.get(locals.i) == locals.proposal.targetAddress)
                    {
                        state.mut().managers.set(locals.i, NULL_ID);
                        locals.actionSucceeded = true;
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
                locals.availableFees = (state.get()._earnedFees > (state.get()._distributedFees + state.get()._reservedFees))
                                          ? (state.get()._earnedFees - state.get()._distributedFees - state.get()._reservedFees)
                                          : 0;
                if (locals.proposal.amount <= locals.availableFees && locals.proposal.amount > 0)
                {
                    if (qpi.transfer(state.get().feeRecipient, locals.proposal.amount) >= 0)
                    {
                        state.mut()._distributedFees += locals.proposal.amount;
                        locals.actionSucceeded = true;
                    }
                }
            }
            else if (locals.proposal.proposalType == PROPOSAL_CHANGE_THRESHOLD)
            {
                if (locals.proposal.amount >= 2 && locals.proposal.amount <= (uint64)state.get().numberOfAdmins)
                {
                    state.mut().requiredApprovals = (uint8)locals.proposal.amount;
                    locals.actionSucceeded = true;
                }
            }

            // [QVB-20] Only mark as executed if action actually succeeded
            if (locals.actionSucceeded)
            {
                locals.proposal.executed = true;
                output.executed = true;
            }
            else
            {
                // Action failed - deactivate proposal so admins can create a new one
                locals.proposal.active = false;
                output.executed = false;

                state.mut().proposals.set(locals.proposalIndex, locals.proposal);

                locals.log = EthBridgeLogger{
                    CONTRACT_INDEX,
                    EthBridgeError::invalidOrderState,
                    input.proposalId,
                    locals.proposal.approvalsCount,
                    0 };
                LOG_INFO(locals.log);
                output.status = EthBridgeError::invalidOrderState;
                return;
            }
        }
        else
        {
            output.executed = false;
        }

        // Update the proposal
        state.mut().proposals.set(locals.proposalIndex, locals.proposal);

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
        for (locals.i = 0; locals.i < state.get().proposals.capacity(); ++locals.i)
        {
            if (state.get().proposals.get(locals.i).proposalId == input.proposalId)
            {
                output.proposal = state.get().proposals.get(locals.i);
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
        for (locals.i = 0; locals.i < state.get().proposals.capacity(); ++locals.i)
        {
            locals.proposal = state.get().proposals.get(locals.i);
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
        state.mut().proposals.set(locals.i, locals.proposal);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0, // No error
            input.proposalId,
            0,
            0 };
        LOG_INFO(locals.log);

        output.status = 0; // Success
    }

    struct getTotalReceivedTokens_locals
    {
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getTotalReceivedTokens)
    {
        output.totalTokens = state.get().totalReceivedTokens;
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
        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            if (state.get().orders.get(locals.i).orderId == input.orderId)
            {
                locals.order = state.get().orders.get(locals.i);
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

        // Handle order based on transfer direction
        if (locals.order.fromQubicToEthereum)
        {
            // Qubic → Ethereum
            // [QVB-21] Only check that tokens were received (tokensLocked is set here as part of two-phase commit)
            if (!locals.order.tokensReceived)
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

            // [QVB-21] Set tokensLocked as part of completion (two-phase commit)
            locals.order.tokensLocked = true;
        }
        else
        {
            // EVM → Qubic
            // [QVB-09] Liquidity already reserved at createOrder - just transfer to user
            // [QVB-04] Use locals.order.amount directly (removed redundant netAmount)
            if (qpi.transfer(locals.order.qubicDestination, locals.order.amount) < 0)
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

            locals.logTokens = TokensLogger{
                CONTRACT_INDEX,
                state.get().lockedTokens,
                state.get().totalReceivedTokens,
                0 };
            LOG_INFO(locals.logTokens);
        }

        // Release reserved fees now that order is completed (fees can now be distributed)
        locals.feeOperator = div(locals.order.amount * state.get()._tradeFeeBillionths, 1000000000ULL);
        locals.feeNetwork = div(locals.order.amount * state.get()._tradeFeeBillionths, 1000000000ULL);

        // UNDERFLOW PROTECTION: Only release if enough reserved
        if (state.get()._reservedFees >= locals.feeOperator)
        {
            state.mut()._reservedFees -= locals.feeOperator;
        }
        if (state.get()._reservedFeesQubic >= locals.feeNetwork)
        {
            state.mut()._reservedFeesQubic -= locals.feeNetwork;
        }

        // Mark the order as completed
        locals.order.status = 1;                      // Completed
        state.mut().orders.set(locals.i, locals.order); // Use the loop index

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
        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            if (state.get().orders.get(locals.i).orderId == input.orderId)
            {
                locals.order = state.get().orders.get(locals.i);
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

        // Calculate fees for this order
        locals.feeOperator = div(locals.order.amount * state.get()._tradeFeeBillionths, 1000000000ULL);
        locals.feeNetwork = div(locals.order.amount * state.get()._tradeFeeBillionths, 1000000000ULL);

        // Handle refund based on transfer direction
        if (locals.order.fromQubicToEthereum)
        {
            // Qubic → Ethereum refund
            if (!locals.order.tokensReceived)
            {
                // Path A: No tokens transferred yet - refund only fees
                // [QVB-12] Calculate refund amount WITHOUT modifying state first
                locals.totalRefund = 0;
                if (state.get()._reservedFees >= locals.feeOperator && state.get()._earnedFees >= locals.feeOperator)
                {
                    locals.totalRefund += locals.feeOperator;
                }
                if (state.get()._reservedFeesQubic >= locals.feeNetwork && state.get()._earnedFeesQubic >= locals.feeNetwork)
                {
                    locals.totalRefund += locals.feeNetwork;
                }

                // Transfer first - if it fails, no state is corrupted
                if (locals.totalRefund > 0)
                {
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
                }

                // [QVB-12] Only update state AFTER successful transfer
                if (state.get()._reservedFees >= locals.feeOperator && state.get()._earnedFees >= locals.feeOperator)
                {
                    state.mut()._reservedFees -= locals.feeOperator;
                    state.mut()._earnedFees -= locals.feeOperator;
                }
                if (state.get()._reservedFeesQubic >= locals.feeNetwork && state.get()._earnedFeesQubic >= locals.feeNetwork)
                {
                    state.mut()._reservedFeesQubic -= locals.feeNetwork;
                    state.mut()._earnedFeesQubic -= locals.feeNetwork;
                }

                locals.order.status = 2;
                state.mut().orders.set(locals.i, locals.order);

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
            else
            {
                // Path B: Tokens received - refund amount + fees
                // [QVB-21] Handles both !tokensLocked (intermediate) and tokensLocked (defensive) states
                if (state.get().lockedTokens < locals.order.amount)
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

                // [QVB-12] Calculate total refund WITHOUT modifying state
                locals.totalRefund = locals.order.amount;
                if (state.get()._reservedFees >= locals.feeOperator && state.get()._earnedFees >= locals.feeOperator)
                {
                    locals.totalRefund += locals.feeOperator;
                }
                if (state.get()._reservedFeesQubic >= locals.feeNetwork && state.get()._earnedFeesQubic >= locals.feeNetwork)
                {
                    locals.totalRefund += locals.feeNetwork;
                }

                // Transfer first
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

                // [QVB-12] Update state only after successful transfer
                state.mut().lockedTokens -= locals.order.amount;
                if (state.get()._reservedFees >= locals.feeOperator && state.get()._earnedFees >= locals.feeOperator)
                {
                    state.mut()._reservedFees -= locals.feeOperator;
                    state.mut()._earnedFees -= locals.feeOperator;
                }
                if (state.get()._reservedFeesQubic >= locals.feeNetwork && state.get()._earnedFeesQubic >= locals.feeNetwork)
                {
                    state.mut()._reservedFeesQubic -= locals.feeNetwork;
                    state.mut()._earnedFeesQubic -= locals.feeNetwork;
                }

                locals.order.status = 2;
                state.mut().orders.set(locals.i, locals.order);

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
        }
        else
        {
            // EVM → Qubic refund
            // [QVB-09] Restore reserved liquidity (was decremented at createOrder)
            state.mut().lockedTokens += locals.order.amount;

            // Release fee reserves (fees stay earned - paid by bridge operator, not end user)
            if (state.get()._reservedFees >= locals.feeOperator)
            {
                state.mut()._reservedFees -= locals.feeOperator;
            }
            if (state.get()._reservedFeesQubic >= locals.feeNetwork)
            {
                state.mut()._reservedFeesQubic -= locals.feeNetwork;
            }

            locals.order.status = 2;
            state.mut().orders.set(locals.i, locals.order);

            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                0,
                input.orderId,
                locals.order.amount,
                0 };
            LOG_INFO(locals.log);
            output.status = 0;
            return;
        }
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
        // [QVB-10] Capture invocationReward immediately so we can refund on any error
        locals.depositAmount = qpi.invocationReward();

        if (input.amount == 0)
        {
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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
        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            if (state.get().orders.get(locals.i).orderId == input.orderId)
            {
                locals.order = state.get().orders.get(locals.i);
                locals.orderFound = true;
                break;
            }
        }

        if (!locals.orderFound)
        {
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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

        // [QVB-22] Reject EVM→Qubic orders (they don't need transferToContract)
        if (!locals.order.fromQubicToEthereum)
        {
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
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

        // Only Qubic→Ethereum orders proceed from here
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
        state.mut().lockedTokens += input.amount;
        state.mut().totalReceivedTokens += input.amount;

        // Refund excess if user sent too much
        if (locals.depositAmount > input.amount)
        {
            qpi.transfer(qpi.invocator(), locals.depositAmount - input.amount);
        }

        // [QVB-21] Only mark tokens as received, NOT locked
        // tokensLocked will be set by completeOrder (two-phase commit)
        locals.order.tokensReceived = true;
        state.mut().orders.set(locals.i, locals.order);

        locals.logTokens = TokensLogger{
            CONTRACT_INDEX,
            state.get().lockedTokens,
            state.get().totalReceivedTokens,
            0 };
        LOG_INFO(locals.logTokens);

        locals.log = EthBridgeLogger{
            CONTRACT_INDEX,
            0,
            input.orderId,
            input.amount,
            0 };
        LOG_INFO(locals.log);
        output.status = 0;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getTotalLockedTokens)
    {
        output.totalLockedTokens = state.get().lockedTokens;
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
        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            locals.order = state.get().orders.get(locals.i);

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

        // [QVB-10] Capture deposit immediately so we can refund on auth failure
        locals.depositAmount = qpi.invocationReward();

        locals.isManagerOperating = false;
        CALL(isManager, locals.invocatorAddress, locals.isManagerOperating);

        locals.isMultisigAdminResult = false;
        CALL(isMultisigAdmin, locals.invocatorAddress, locals.isMultisigAdminResult);

        // Verify that the invocator is a manager or multisig admin
        if (!locals.isManagerOperating && !locals.isMultisigAdminResult)
        {
            // [QVB-10] Refund before returning
            if (locals.depositAmount > 0) qpi.transfer(qpi.invocator(), locals.depositAmount);
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::notAuthorized,
                0,
                0,
                0
            };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::notAuthorized;
            return;
        }

        // Validate that some tokens were sent
        if (locals.depositAmount == 0)
        {
            locals.log = EthBridgeLogger{
                CONTRACT_INDEX,
                EthBridgeError::invalidAmount,
                0,
                0,
                0
            };
            LOG_INFO(locals.log);
            output.status = EthBridgeError::invalidAmount;
            return;
        }

        // Add the deposited tokens to the locked tokens pool
        state.mut().lockedTokens += locals.depositAmount;
        state.mut().totalReceivedTokens += locals.depositAmount;

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
        output.totalLocked = state.get().lockedTokens;
    }


    PUBLIC_FUNCTION(getAvailableFees)
    {
        // Available fees exclude those reserved for pending orders
        output.availableFees = (state.get()._earnedFees > (state.get()._distributedFees + state.get()._reservedFees))
                                 ? (state.get()._earnedFees - state.get()._distributedFees - state.get()._reservedFees)
                                 : 0;
        output.totalEarnedFees = state.get()._earnedFees;
        output.totalDistributedFees = state.get()._distributedFees;
    }


    struct getContractInfo_locals
    {
        uint64 i;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(getContractInfo)
    {
        output.managers = state.get().managers;
        output.nextOrderId = state.get().nextOrderId;
        output.lockedTokens = state.get().lockedTokens;
        output.totalReceivedTokens = state.get().totalReceivedTokens;
        output.earnedFees = state.get()._earnedFees;
        output.tradeFeeBillionths = state.get()._tradeFeeBillionths;
        output.sourceChain = state.get().sourceChain;


        output.totalOrdersFound = 0;
        output.emptySlots = 0;

        for (locals.i = 0; locals.i < 16 && locals.i < state.get().orders.capacity(); ++locals.i)
        {
            output.firstOrders.set(locals.i, state.get().orders.get(locals.i));
        }

        // Count real orders vs empty ones
        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            if (state.get().orders.get(locals.i).status == 255)
            {
                output.emptySlots++;
            }
            else
            {
                output.totalOrdersFound++;
            }
        }

        // Multisig info
        output.multisigAdmins = state.get().admins;
        output.numberOfAdmins = state.get().numberOfAdmins;
        output.requiredApprovals = state.get().requiredApprovals;

        // Count active proposals
        output.totalProposals = 0;
        for (locals.i = 0; locals.i < state.get().proposals.capacity(); ++locals.i)
        {
            if (state.get().proposals.get(locals.i).active && state.get().proposals.get(locals.i).proposalId > 0)
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
        locals.feesToDistributeInThisTick = (state.get()._earnedFeesQubic > (state.get()._distributedFeesQubic + state.get()._reservedFeesQubic))
                                              ? (state.get()._earnedFeesQubic - state.get()._distributedFeesQubic - state.get()._reservedFeesQubic)
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
                    state.mut()._distributedFeesQubic += locals.amountPerComputor * NUMBER_OF_COMPUTORS;
                }
            }
        }

        // Distribution of Vottun fees to feeRecipient (excluding reserved fees)
        locals.vottunFeesToDistribute = (state.get()._earnedFees > (state.get()._distributedFees + state.get()._reservedFees))
                                          ? (state.get()._earnedFees - state.get()._distributedFees - state.get()._reservedFees)
                                          : 0;

        if (locals.vottunFeesToDistribute > 0 && state.get().feeRecipient != NULL_ID)
        {
            // [QVB-13] Check for non-negative return (success), not truthy (which -1 also satisfies)
            if (qpi.transfer(state.get().feeRecipient, locals.vottunFeesToDistribute) >= 0)
            {
                state.mut()._distributedFees += locals.vottunFeesToDistribute;
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
        REGISTER_USER_PROCEDURE(completeOrder, 2);
        REGISTER_USER_PROCEDURE(refundOrder, 3);
        REGISTER_USER_PROCEDURE(transferToContract, 4);
        REGISTER_USER_PROCEDURE(addLiquidity, 5);
        REGISTER_USER_PROCEDURE(createProposal, 6);
        REGISTER_USER_PROCEDURE(approveProposal, 7);
        REGISTER_USER_PROCEDURE(cancelProposal, 8);
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
        state.mut().feeRecipient = ID(_M, _A, _G, _K, _B, _C, _B, _I, _X, _N, _W, _S, _K, _C, _I, _G, _J, _Y, _K, _G, _S, _N, _F, _S, _F, _R, _W, _A, _L, _H, _D, _F, _D, _B, _K, _K, _P, _C, _U, _N, _S, _E, _R, _I, _K, _L, _J, _G, _M, _D, _K, _L, _Z, _V, _V, _D);

        // Initialize the orders array. Good practice to zero first.
        locals.emptyOrder = {};         // Sets all fields to 0 (including orderId and status).
        locals.emptyOrder.status = 255; // Then set your status for empty.

        for (locals.i = 0; locals.i < state.get().orders.capacity(); ++locals.i)
        {
            state.mut().orders.set(locals.i, locals.emptyOrder);
        }

        // Initialize the managers array with NULL_ID to mark slots as empty
        for (locals.i = 0; locals.i < state.get().managers.capacity(); ++locals.i)
        {
            state.mut().managers.set(locals.i, NULL_ID);
        }

        // Add the initial manager
        state.mut().managers.set(0, ID(_U, _X, _V, _K, _B, _Y, _L, _Q, _Z, _I, _U, _L, _C, _B, _F, _F, _I, _L, _P, _T, _X, _O, _W, _Y, _M, _Y, _H, _D, _K, _P, _R, _Y, _W, _R, _E, _Y, _Q, _T, _Y, _Y, _A, _C, _T, _F, _W, _Q, _V, _O, _N, _P, _F, _E, _L, _P, _G, _A));

        // Initialize the rest of the state variables
        state.mut().nextOrderId = 1; // Start from 1 to avoid ID 0
        state.mut().lockedTokens = 0;
        state.mut().totalReceivedTokens = 0;
        state.mut().sourceChain = 0; // Arbitrary number. No-EVM chain

        // Initialize fee variables
        state.mut()._tradeFeeBillionths = 5000000; // 0.5% == 5,000,000 / 1,000,000,000
        state.mut()._earnedFees = 0;
        state.mut()._distributedFees = 0;

        state.mut()._earnedFeesQubic = 0;
        state.mut()._distributedFeesQubic = 0;

        state.mut()._reservedFees = 0;
        state.mut()._reservedFeesQubic = 0;

        // [QVB-09] Minimum order amount to prevent zero-fee spam
        state.mut().minimumOrderAmount = 200; // Minimum 200 QU (fee = 1 QU per direction)

        // Initialize multisig admins (3 admins, requires 2 approvals)
        state.mut().numberOfAdmins = 3;
        state.mut().requiredApprovals = 2; // 2 of 3 threshold

        // Initialize admins array (REPLACE WITH ACTUAL ADMIN ADDRESSES)
        state.mut().admins.set(0, ID(_U, _X, _V, _K, _B, _Y, _L, _Q, _Z, _I, _U, _L, _C, _B, _F, _F, _I, _L, _P, _T, _X, _O, _W, _Y, _M, _Y, _H, _D, _K, _P, _R, _Y, _W, _R, _E, _Y, _Q, _T, _Y, _Y, _A, _C, _T, _F, _W, _Q, _V, _O, _N, _P, _F, _E, _L, _P, _G, _A)); // Admin 1
        state.mut().admins.set(1, ID(_D, _F, _A, _C, _O, _J, _K, _F, _A, _F, _M, _V, _J, _B, _I, _X, _K, _C, _K, _N, _A, _Y, _S, _T, _B, _S, _D, _D, _X, _Y, _I, _A, _Y, _J, _Q, _P, _H, _Y, _D, _V, _D, _H, _A, _S, _B, _A, _T, _H, _A, _L, _D, _C, _O, _Q, _K, _F)); // Admin 2 (Manager)
        state.mut().admins.set(2, ID(_O, _U, _T, _P, _L, _M, _H, _I, _P, _V, _D, _X, _P, _D, _J, _R, _S, _O, _D, _R, _A, _O, _U, _L, _V, _D, _V, _A, _T, _I, _D, _W, _X, _X, _L, _Q, _O, _F, _X, _O, _D, _D, _X, _P, _J, _M, _Q, _G, _C, _S, _J, _Y, _Q, _Q, _V, _D)); // Admin 3 (User)

        // Initialize remaining admin slots
        for (locals.i = 3; locals.i < state.get().admins.capacity(); ++locals.i)
        {
            state.mut().admins.set(locals.i, NULL_ID);
        }

        // Initialize proposals array properly (like orders array)
        state.mut().nextProposalId = 1;

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
        for (locals.i = 0; locals.i < state.get().proposals.capacity(); ++locals.i)
        {
            state.mut().proposals.set(locals.i, locals.emptyProposal);
        }
    }
};
