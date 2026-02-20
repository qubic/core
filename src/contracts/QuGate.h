// QuGate.h - Programmable Payment Gate Contract
// Short name: QUGATE
// Description: Universal payment routing with predefined gate modes.
//   Create gates with configurable rules, point payments at them,
//   and QU flows according to the logic.
//
// Gate Modes:
//   SPLIT       - Distribute to N addresses by ratio (e.g. 40/30/20/10)
//   ROUND_ROBIN - Cycle through addresses, one per payment
//   THRESHOLD   - Accumulate until amount reached, then forward
//   RANDOM      - Select one recipient per payment using tick-based entropy
//   CONDITIONAL - Only forward if sender matches whitelist, else bounce
//
// Anti-Spam:
//   - Escalating creation fee: cost increases as capacity fills
//   - Gate expiry: inactive gates auto-close after N epochs
//   - Dust burn: sends below minimum are burned
//   - All fees are deflationary (burned, not accumulated)

using namespace QPI;

// Capacity scales with network via X_MULTIPLIER
constexpr uint64 QUGATE_INITIAL_MAX_GATES = 4096;
constexpr uint64 QUGATE_MAX_GATES = QUGATE_INITIAL_MAX_GATES * X_MULTIPLIER;
constexpr uint64 QUGATE_MAX_RECIPIENTS = 8;
constexpr uint64 QUGATE_MAX_RATIO = 10000;       // Max ratio per recipient (prevents overflow)

// Default fees — initial values, changeable via shareholder vote
constexpr uint64 QUGATE_DEFAULT_CREATION_FEE = 100000;
constexpr uint64 QUGATE_DEFAULT_MIN_SEND = 1000;

// Escalating fee: fee = baseFee * (1 + activeGates / FEE_ESCALATION_STEP)
constexpr uint64 QUGATE_FEE_ESCALATION_STEP = 1024;

// Gate expiry: gates with no activity for this many epochs auto-close
constexpr uint64 QUGATE_DEFAULT_EXPIRY_EPOCHS = 50;

// Gate modes
constexpr uint8 QUGATE_MODE_SPLIT = 0;
constexpr uint8 QUGATE_MODE_ROUND_ROBIN = 1;
constexpr uint8 QUGATE_MODE_THRESHOLD = 2;
constexpr uint8 QUGATE_MODE_RANDOM = 3;
constexpr uint8 QUGATE_MODE_CONDITIONAL = 4;

// Status codes — used in procedure outputs and logger _type
constexpr sint64 QUGATE_SUCCESS = 0;
constexpr sint64 QUGATE_INVALID_GATE_ID = -1;
constexpr sint64 QUGATE_GATE_NOT_ACTIVE = -2;
constexpr sint64 QUGATE_UNAUTHORIZED = -3;
constexpr sint64 QUGATE_INVALID_MODE = -4;
constexpr sint64 QUGATE_INVALID_RECIPIENT_COUNT = -5;
constexpr sint64 QUGATE_INVALID_RATIO = -6;
constexpr sint64 QUGATE_INSUFFICIENT_FEE = -7;
constexpr sint64 QUGATE_NO_FREE_SLOTS = -8;
constexpr sint64 QUGATE_DUST_AMOUNT = -9;
constexpr sint64 QUGATE_INVALID_THRESHOLD = -10;
constexpr sint64 QUGATE_INVALID_SENDER_COUNT = -11;
constexpr sint64 QUGATE_CONDITIONAL_REJECTED = -12;

// Log type constants (positive = success events, high numbers = actions)
constexpr uint32 QUGATE_LOG_GATE_CREATED = 1;
constexpr uint32 QUGATE_LOG_GATE_CLOSED = 2;
constexpr uint32 QUGATE_LOG_GATE_UPDATED = 3;
constexpr uint32 QUGATE_LOG_PAYMENT_FORWARDED = 4;
constexpr uint32 QUGATE_LOG_PAYMENT_BOUNCED = 5;
constexpr uint32 QUGATE_LOG_DUST_BURNED = 6;
constexpr uint32 QUGATE_LOG_FEE_CHANGED = 7;
constexpr uint32 QUGATE_LOG_GATE_EXPIRED = 8;    //
// Failure log types use high range
constexpr uint32 QUGATE_LOG_FAIL_INVALID_GATE = 100;
constexpr uint32 QUGATE_LOG_FAIL_NOT_ACTIVE = 101;
constexpr uint32 QUGATE_LOG_FAIL_UNAUTHORIZED = 102;
constexpr uint32 QUGATE_LOG_FAIL_INVALID_PARAMS = 103;
constexpr uint32 QUGATE_LOG_FAIL_INSUFFICIENT_FEE = 104;
constexpr uint32 QUGATE_LOG_FAIL_NO_SLOTS = 105;

// Asset name for shareholder proposals
constexpr uint64 QUGATE_CONTRACT_ASSET_NAME = 76228174763345ULL; // "QUGATE" as uint64 little-endian

// Future extension struct (Qubic convention)
struct QUGATE2
{
};

struct QUGATE : public ContractBase
{
public:
    // =============================================
    // Logging structs
    // =============================================

    struct QuGateLogger
    {
        uint32 _contractIndex;
        uint32 _type;           // maps to QUGATE_LOG_* constants
        uint64 gateId;
        id sender;
        sint64 amount;
        sint8 _terminator;      // Qubic logs data before this field only
    };

    // =============================================
    // Data structures for gate configuration
    // =============================================

    struct GateConfig
    {
        id owner;
        uint8 mode;
        uint8 recipientCount;
        uint8 active;
        uint8 allowedSenderCount;
        uint16 createdEpoch;                            // epoch() returns uint16
        uint16 lastActivityEpoch;                       // updated on create/send/update
        uint64 totalReceived;
        uint64 totalForwarded;
        uint64 currentBalance;
        uint64 threshold;
        uint64 roundRobinIndex;
        Array<id, 8> recipients;
        Array<uint64, 8> ratios;
        Array<id, 8> allowedSenders;
    };

    // =============================================
    // Procedure inputs/outputs
    // =============================================

    struct createGate_input
    {
        uint8 mode;
        uint8 recipientCount;
        Array<id, 8> recipients;
        Array<uint64, 8> ratios;
        uint64 threshold;
        Array<id, 8> allowedSenders;
        uint8 allowedSenderCount;
    };
    struct createGate_output
    {
        sint64 status;          //
        uint64 gateId;
        uint64 feePaid;         // actual fee charged (for transparency)
    };

    struct sendToGate_input
    {
        uint64 gateId;
    };
    struct sendToGate_output
    {
        sint64 status;          //
    };

    struct closeGate_input
    {
        uint64 gateId;
    };
    struct closeGate_output
    {
        sint64 status;          //
    };

    struct updateGate_input
    {
        uint64 gateId;
        uint8 recipientCount;
        Array<id, 8> recipients;
        Array<uint64, 8> ratios;
        uint64 threshold;
        Array<id, 8> allowedSenders;
        uint8 allowedSenderCount;
    };
    struct updateGate_output
    {
        sint64 status;          //
    };

    // =============================================
    // Function inputs/outputs (read-only queries)
    // =============================================

    struct getGate_input
    {
        uint64 gateId;
    };
    struct getGate_output
    {
        uint8 mode;
        uint8 recipientCount;
        uint8 active;
        id owner;
        uint64 totalReceived;
        uint64 totalForwarded;
        uint64 currentBalance;
        uint64 threshold;
        uint16 createdEpoch;                            //
        uint16 lastActivityEpoch;                       //
        Array<id, 8> recipients;
        Array<uint64, 8> ratios;
    };

    struct getGateCount_input
    {
    };
    struct getGateCount_output
    {
        uint64 totalGates;
        uint64 activeGates;
        uint64 totalBurned;     //
    };

    struct getGatesByOwner_input
    {
        id owner;
    };
    struct getGatesByOwner_output
    {
        Array<uint64, 16> gateIds;
        uint64 count;
    };

    // Batch gate query
    struct getGateBatch_input
    {
        Array<uint64, 32> gateIds;
    };
    struct getGateBatch_output
    {
        Array<getGate_output, 32> gates;
    };

    // Fee query — includes current escalated fee and expiry setting
    struct getFees_input
    {
    };
    struct getFees_output
    {
        uint64 creationFee;         // base fee
        uint64 currentCreationFee;  // actual fee right now (after escalation)
        uint64 minSendAmount;
        uint64 expiryEpochs;       //
    };

protected:
    // =============================================
    // Contract state
    // =============================================

    uint64 _gateCount;
    uint64 _activeGates;
    uint64 _totalBurned;        // cumulative QU burned
    Array<GateConfig, QUGATE_MAX_GATES> _gates;

    // Free-list for slot reuse
    Array<uint64, QUGATE_MAX_GATES> _freeSlots;
    uint64 _freeCount;

    // Shareholder-adjustable parameters
    uint64 _creationFee;        // base creation fee
    uint64 _minSendAmount;
    uint64 _expiryEpochs;      // epochs of inactivity before auto-close

    // Shareholder proposal storage
    // DEFINE_SHAREHOLDER_PROPOSAL_STORAGE(4, QUGATE_CONTRACT_ASSET_NAME);
    // NOTE: Uncomment when QUGATE_CONTRACT_ASSET_NAME is set at registration time
    // For now, fees are set in INITIALIZE and cannot be changed until shareholder infra is wired.

    // =============================================
    // Locals — all variables declared here, not inline
    // =============================================

    struct createGate_locals
    {
        QuGateLogger logger;
        GateConfig newGate;
        uint64 totalRatio;
        uint64 i;
        uint64 slotIdx;
        uint64 currentFee;     // escalated fee
    };

    struct processSplit_input
    {
        uint64 gateIdx;
        sint64 amount;
    };
    struct processSplit_output
    {
        uint64 forwarded;
    };
    struct processSplit_locals
    {
        GateConfig gate;
        uint64 totalRatio;
        uint64 share;
        uint64 distributed;
        uint64 i;
    };

    struct processRoundRobin_input
    {
        uint64 gateIdx;
        sint64 amount;
    };
    struct processRoundRobin_output
    {
        uint64 forwarded;
    };
    struct processRoundRobin_locals
    {
        GateConfig gate;
    };

    struct processThreshold_input
    {
        uint64 gateIdx;
        sint64 amount;
    };
    struct processThreshold_output
    {
        uint64 forwarded;
    };
    struct processThreshold_locals
    {
        GateConfig gate;
    };

    struct processRandom_input
    {
        uint64 gateIdx;
        sint64 amount;
    };
    struct processRandom_output
    {
        uint64 forwarded;
    };
    struct processRandom_locals
    {
        GateConfig gate;
        uint64 recipientIdx;
    };

    struct processConditional_input
    {
        uint64 gateIdx;
        sint64 amount;
    };
    struct processConditional_output
    {
        sint64 status;
        uint64 forwarded;
    };
    struct processConditional_locals
    {
        GateConfig gate;
        uint64 i;
        uint8 senderAllowed;
    };


    struct sendToGate_locals
    {
        QuGateLogger logger;
        GateConfig gate;
        sint64 amount;
        uint64 idx;
        processSplit_input splitIn;
        processSplit_output splitOut;
        processSplit_locals splitLocals;
        processRoundRobin_input rrIn;
        processRoundRobin_output rrOut;
        processRoundRobin_locals rrLocals;
        processThreshold_input threshIn;
        processThreshold_output threshOut;
        processThreshold_locals threshLocals;
        processRandom_input randIn;
        processRandom_output randOut;
        processRandom_locals randLocals;
        processConditional_input condIn;
        processConditional_output condOut;
        processConditional_locals condLocals;
    };

    struct closeGate_locals
    {
        QuGateLogger logger;
        GateConfig gate;
    };

    struct updateGate_locals
    {
        QuGateLogger logger;
        GateConfig gate;
        uint64 totalRatio;
        uint64 i;
    };

    struct getGate_locals
    {
        GateConfig gate;
        uint64 i;
    };

    struct getGateBatch_locals                           //
    {
        uint64 i;
        uint64 j;
        GateConfig gate;
        getGate_output entry;
    };

    struct getGatesByOwner_locals
    {
        uint64 i;
    };

    struct END_EPOCH_locals                               //
    {
        uint64 i;
        QuGateLogger logger;
        GateConfig gate;
    };

    // =============================================
    // Procedures
    // =============================================

    PUBLIC_PROCEDURE_WITH_LOCALS(createGate)
    {
        output.status = QUGATE_SUCCESS;
        output.gateId = 0;
        output.feePaid = 0;

        // Init logger
        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.sender = qpi.invocator();
        locals.logger.gateId = 0;
        locals.logger.amount = qpi.invocationReward();

        // Calculate escalated fee: baseFee * (1 + activeGates / STEP)
        locals.currentFee = state._creationFee * (1 + QPI::div(state._activeGates, QUGATE_FEE_ESCALATION_STEP));

        // Validate creation fee (escalated)
        if (qpi.invocationReward() < (sint64)locals.currentFee)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.status = QUGATE_INSUFFICIENT_FEE;
            locals.logger._type = QUGATE_LOG_FAIL_INSUFFICIENT_FEE;
            LOG_INFO(locals.logger);
            return;
        }

        // Validate mode
        if (input.mode > QUGATE_MODE_CONDITIONAL)
        {
            // Refund all
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_MODE;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Validate recipient count
        if (input.recipientCount == 0 || input.recipientCount > QUGATE_MAX_RECIPIENTS)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_RECIPIENT_COUNT;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Check capacity — try free-list first
        if (state._freeCount == 0 && state._gateCount >= QUGATE_MAX_GATES)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_NO_FREE_SLOTS;
            locals.logger._type = QUGATE_LOG_FAIL_NO_SLOTS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Validate SPLIT ratios
        if (input.mode == QUGATE_MODE_SPLIT)
        {
            locals.totalRatio = 0;
            for (locals.i = 0; locals.i < input.recipientCount; locals.i++)
            {
                if (input.ratios.get(locals.i) > QUGATE_MAX_RATIO)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    output.status = QUGATE_INVALID_RATIO;
                    locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
                    LOG_WARNING(locals.logger);
                    return;
                }
                locals.totalRatio += input.ratios.get(locals.i);
            }
            if (locals.totalRatio == 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.status = QUGATE_INVALID_RATIO;
                locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
                LOG_WARNING(locals.logger);
                return;
            }
        }

        // Validate THRESHOLD > 0
        if (input.mode == QUGATE_MODE_THRESHOLD && input.threshold == 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_THRESHOLD;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Validate allowedSenderCount
        if (input.allowedSenderCount > QUGATE_MAX_RECIPIENTS)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_SENDER_COUNT;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Build the gate config
        locals.newGate.owner = qpi.invocator();
        locals.newGate.mode = input.mode;
        locals.newGate.recipientCount = input.recipientCount;
        locals.newGate.active = 1;
        locals.newGate.allowedSenderCount = input.allowedSenderCount;
        locals.newGate.createdEpoch = qpi.epoch();       // uint16
        locals.newGate.lastActivityEpoch = qpi.epoch();  //
        locals.newGate.totalReceived = 0;
        locals.newGate.totalForwarded = 0;
        locals.newGate.currentBalance = 0;
        locals.newGate.threshold = input.threshold;
        locals.newGate.roundRobinIndex = 0;

        for (locals.i = 0; locals.i < QUGATE_MAX_RECIPIENTS; locals.i++)
        {
            if (locals.i < input.recipientCount)
            {
                locals.newGate.recipients.set(locals.i, input.recipients.get(locals.i));
                locals.newGate.ratios.set(locals.i, input.ratios.get(locals.i));
            }
            else
            {
                locals.newGate.recipients.set(locals.i, id::zero());
                locals.newGate.ratios.set(locals.i, 0);
            }
        }

        for (locals.i = 0; locals.i < QUGATE_MAX_RECIPIENTS; locals.i++)
        {
            if (locals.i < input.allowedSenderCount)
            {
                locals.newGate.allowedSenders.set(locals.i, input.allowedSenders.get(locals.i));
            }
            else
            {
                locals.newGate.allowedSenders.set(locals.i, id::zero());
            }
        }

        // Allocate slot — free-list first, then fresh
        if (state._freeCount > 0)
        {
            state._freeCount -= 1;
            locals.slotIdx = state._freeSlots.get(state._freeCount);
        }
        else
        {
            locals.slotIdx = state._gateCount;
            state._gateCount += 1;
        }

        state._gates.set(locals.slotIdx, locals.newGate);
        output.gateId = locals.slotIdx + 1;              // 1-indexed for users
        state._activeGates += 1;

        // Burn the escalated creation fee
        qpi.burn(locals.currentFee);
        state._totalBurned += locals.currentFee;          //
        output.feePaid = locals.currentFee;               //

        // Refund any excess
        if (qpi.invocationReward() > (sint64)locals.currentFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)locals.currentFee);
        }

        output.status = QUGATE_SUCCESS;

        // Log success
        locals.logger._type = QUGATE_LOG_GATE_CREATED;
        locals.logger.gateId = output.gateId;
        LOG_INFO(locals.logger);
    }

    // =============================================
    // Private mode processors
    // =============================================

    PRIVATE_PROCEDURE_WITH_LOCALS(processSplit)
    {
        locals.gate = state._gates.get(input.gateIdx);

        locals.totalRatio = 0;
        for (locals.i = 0; locals.i < locals.gate.recipientCount; locals.i++)
        {
            locals.totalRatio += locals.gate.ratios.get(locals.i);
        }

        locals.distributed = 0;
        for (locals.i = 0; locals.i < locals.gate.recipientCount; locals.i++)
        {
            if (locals.i == locals.gate.recipientCount - 1)
            {
                locals.share = input.amount - locals.distributed;
            }
            else
            {
                locals.share = QPI::div((uint64)input.amount, locals.totalRatio) * locals.gate.ratios.get(locals.i)
                    + QPI::div(QPI::mod((uint64)input.amount, locals.totalRatio) * locals.gate.ratios.get(locals.i), locals.totalRatio);
            }

            if (locals.share > 0)
            {
                qpi.transfer(locals.gate.recipients.get(locals.i), locals.share);
                locals.distributed += locals.share;
            }
        }

        locals.gate.totalForwarded += locals.distributed;
        state._gates.set(input.gateIdx, locals.gate);
        output.forwarded = locals.distributed;
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(processRoundRobin)
    {
        locals.gate = state._gates.get(input.gateIdx);

        qpi.transfer(locals.gate.recipients.get(locals.gate.roundRobinIndex), input.amount);
        locals.gate.totalForwarded += input.amount;
        locals.gate.roundRobinIndex = QPI::mod(locals.gate.roundRobinIndex + 1, (uint64)locals.gate.recipientCount);

        state._gates.set(input.gateIdx, locals.gate);
        output.forwarded = input.amount;
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(processThreshold)
    {
        locals.gate = state._gates.get(input.gateIdx);

        locals.gate.currentBalance += input.amount;
        output.forwarded = 0;

        if (locals.gate.currentBalance >= locals.gate.threshold)
        {
            qpi.transfer(locals.gate.recipients.get(0), locals.gate.currentBalance);
            output.forwarded = locals.gate.currentBalance;
            locals.gate.totalForwarded += locals.gate.currentBalance;
            locals.gate.currentBalance = 0;
        }

        state._gates.set(input.gateIdx, locals.gate);
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(processRandom)
    {
        locals.gate = state._gates.get(input.gateIdx);

        locals.recipientIdx = QPI::mod(locals.gate.totalReceived + qpi.tick(), (uint64)locals.gate.recipientCount);
        qpi.transfer(locals.gate.recipients.get(locals.recipientIdx), input.amount);
        locals.gate.totalForwarded += input.amount;

        state._gates.set(input.gateIdx, locals.gate);
        output.forwarded = input.amount;
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(processConditional)
    {
        locals.gate = state._gates.get(input.gateIdx);
        output.status = QUGATE_SUCCESS;
        output.forwarded = 0;

        locals.senderAllowed = 0;
        for (locals.i = 0; locals.i < locals.gate.allowedSenderCount; locals.i++)
        {
            if (locals.senderAllowed == 0 && locals.gate.allowedSenders.get(locals.i) == qpi.invocator())
            {
                locals.senderAllowed = 1;
            }
        }

        if (locals.senderAllowed)
        {
            qpi.transfer(locals.gate.recipients.get(0), input.amount);
            locals.gate.totalForwarded += input.amount;
            output.forwarded = input.amount;
        }
        else
        {
            qpi.transfer(qpi.invocator(), input.amount);
            output.status = QUGATE_CONDITIONAL_REJECTED;
        }

        state._gates.set(input.gateIdx, locals.gate);
    }

    // =============================================
    // Procedures
    // =============================================

    PUBLIC_PROCEDURE_WITH_LOCALS(sendToGate)
    {
        output.status = QUGATE_SUCCESS;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.sender = qpi.invocator();
        locals.logger.amount = qpi.invocationReward();
        locals.logger.gateId = input.gateId;

        if (input.gateId == 0 || input.gateId > state._gateCount)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.status = QUGATE_INVALID_GATE_ID;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_GATE;
            LOG_WARNING(locals.logger);
            return;
        }

        locals.idx = input.gateId - 1;
        locals.gate = state._gates.get(locals.idx);

        if (locals.gate.active == 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.status = QUGATE_GATE_NOT_ACTIVE;
            locals.logger._type = QUGATE_LOG_FAIL_NOT_ACTIVE;
            LOG_WARNING(locals.logger);
            return;
        }

        locals.amount = qpi.invocationReward();
        if (locals.amount <= 0)
        {
            output.status = QUGATE_DUST_AMOUNT;
            return;
        }

        if (locals.amount < (sint64)state._minSendAmount)
        {
            qpi.burn(locals.amount);
            state._totalBurned += locals.amount;
            output.status = QUGATE_DUST_AMOUNT;
            locals.logger._type = QUGATE_LOG_DUST_BURNED;
            LOG_INFO(locals.logger);
            return;
        }

        // Update activity and track received
        locals.gate.lastActivityEpoch = qpi.epoch();
        locals.gate.totalReceived += locals.amount;
        state._gates.set(locals.idx, locals.gate);

        // Dispatch to mode-specific handler
        if (locals.gate.mode == QUGATE_MODE_SPLIT)
        {
            locals.splitIn.gateIdx = locals.idx;
            locals.splitIn.amount = locals.amount;
            processSplit(qpi, state, locals.splitIn, locals.splitOut, locals.splitLocals);
            locals.logger._type = QUGATE_LOG_PAYMENT_FORWARDED;
            LOG_INFO(locals.logger);
        }
        else if (locals.gate.mode == QUGATE_MODE_ROUND_ROBIN)
        {
            locals.rrIn.gateIdx = locals.idx;
            locals.rrIn.amount = locals.amount;
            processRoundRobin(qpi, state, locals.rrIn, locals.rrOut, locals.rrLocals);
            locals.logger._type = QUGATE_LOG_PAYMENT_FORWARDED;
            LOG_INFO(locals.logger);
        }
        else if (locals.gate.mode == QUGATE_MODE_THRESHOLD)
        {
            locals.threshIn.gateIdx = locals.idx;
            locals.threshIn.amount = locals.amount;
            processThreshold(qpi, state, locals.threshIn, locals.threshOut, locals.threshLocals);
            if (locals.threshOut.forwarded > 0)
            {
                locals.logger._type = QUGATE_LOG_PAYMENT_FORWARDED;
                LOG_INFO(locals.logger);
            }
        }
        else if (locals.gate.mode == QUGATE_MODE_RANDOM)
        {
            locals.randIn.gateIdx = locals.idx;
            locals.randIn.amount = locals.amount;
            processRandom(qpi, state, locals.randIn, locals.randOut, locals.randLocals);
            locals.logger._type = QUGATE_LOG_PAYMENT_FORWARDED;
            LOG_INFO(locals.logger);
        }
        else if (locals.gate.mode == QUGATE_MODE_CONDITIONAL)
        {
            locals.condIn.gateIdx = locals.idx;
            locals.condIn.amount = locals.amount;
            processConditional(qpi, state, locals.condIn, locals.condOut, locals.condLocals);
            if (locals.condOut.status == QUGATE_SUCCESS)
            {
                locals.logger._type = QUGATE_LOG_PAYMENT_FORWARDED;
                LOG_INFO(locals.logger);
            }
            else
            {
                output.status = locals.condOut.status;
                locals.logger._type = QUGATE_LOG_PAYMENT_BOUNCED;
                LOG_INFO(locals.logger);
            }
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(closeGate)
    {
        output.status = QUGATE_SUCCESS;

        // Init logger
        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.sender = qpi.invocator();
        locals.logger.gateId = input.gateId;
        locals.logger.amount = 0;

        if (input.gateId == 0 || input.gateId > state._gateCount)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_GATE_ID;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_GATE;
            LOG_WARNING(locals.logger);
            return;
        }

        locals.gate = state._gates.get(input.gateId - 1);

        if (locals.gate.owner != qpi.invocator())
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_UNAUTHORIZED;
            locals.logger._type = QUGATE_LOG_FAIL_UNAUTHORIZED;
            LOG_WARNING(locals.logger);
            return;
        }

        if (locals.gate.active == 0)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_GATE_NOT_ACTIVE;
            locals.logger._type = QUGATE_LOG_FAIL_NOT_ACTIVE;
            LOG_WARNING(locals.logger);
            return;
        }

        // Refund any held balance (THRESHOLD mode)
        if (locals.gate.currentBalance > 0)
        {
            qpi.transfer(locals.gate.owner, locals.gate.currentBalance);
            locals.gate.currentBalance = 0;
        }

        locals.gate.active = 0;
        state._gates.set(input.gateId - 1, locals.gate);
        state._activeGates -= 1;

        // Push slot onto free-list for reuse
        state._freeSlots.set(state._freeCount, input.gateId - 1);
        state._freeCount += 1;

        // Refund invocation reward
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        // Log success
        locals.logger._type = QUGATE_LOG_GATE_CLOSED;
        LOG_INFO(locals.logger);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(updateGate)
    {
        output.status = QUGATE_SUCCESS;

        // Init logger
        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.sender = qpi.invocator();
        locals.logger.gateId = input.gateId;
        locals.logger.amount = 0;

        if (input.gateId == 0 || input.gateId > state._gateCount)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_GATE_ID;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_GATE;
            LOG_WARNING(locals.logger);
            return;
        }

        locals.gate = state._gates.get(input.gateId - 1);

        if (locals.gate.owner != qpi.invocator())
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_UNAUTHORIZED;
            locals.logger._type = QUGATE_LOG_FAIL_UNAUTHORIZED;
            LOG_WARNING(locals.logger);
            return;
        }

        if (locals.gate.active == 0)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_GATE_NOT_ACTIVE;
            locals.logger._type = QUGATE_LOG_FAIL_NOT_ACTIVE;
            LOG_WARNING(locals.logger);
            return;
        }

        // Validate new recipient count
        if (input.recipientCount == 0 || input.recipientCount > QUGATE_MAX_RECIPIENTS)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_RECIPIENT_COUNT;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Validate allowedSenderCount bounds
        if (input.allowedSenderCount > QUGATE_MAX_RECIPIENTS)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_SENDER_COUNT;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Validate SPLIT ratios if gate is SPLIT mode
        if (locals.gate.mode == QUGATE_MODE_SPLIT)
        {
            locals.totalRatio = 0;
            for (locals.i = 0; locals.i < input.recipientCount; locals.i++)
            {
                if (input.ratios.get(locals.i) > QUGATE_MAX_RATIO)
                {
                    if (qpi.invocationReward() > 0)
                        qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    output.status = QUGATE_INVALID_RATIO;
                    locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
                    LOG_WARNING(locals.logger);
                    return;
                }
                locals.totalRatio += input.ratios.get(locals.i);
            }
            if (locals.totalRatio == 0)
            {
                if (qpi.invocationReward() > 0)
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.status = QUGATE_INVALID_RATIO;
                locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
                LOG_WARNING(locals.logger);
                return;
            }
        }

        // Validate THRESHOLD if gate is THRESHOLD mode
        if (locals.gate.mode == QUGATE_MODE_THRESHOLD && input.threshold == 0)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = QUGATE_INVALID_THRESHOLD;
            locals.logger._type = QUGATE_LOG_FAIL_INVALID_PARAMS;
            LOG_WARNING(locals.logger);
            return;
        }

        // Update last activity epoch
        locals.gate.lastActivityEpoch = qpi.epoch();

        // Update configuration (mode stays the same)
        locals.gate.recipientCount = input.recipientCount;
        locals.gate.threshold = input.threshold;
        locals.gate.allowedSenderCount = input.allowedSenderCount;

        // Use locals.i, zero stale slots
        for (locals.i = 0; locals.i < QUGATE_MAX_RECIPIENTS; locals.i++)
        {
            if (locals.i < input.recipientCount)
            {
                locals.gate.recipients.set(locals.i, input.recipients.get(locals.i));
                locals.gate.ratios.set(locals.i, input.ratios.get(locals.i));
            }
            else
            {
                locals.gate.recipients.set(locals.i, id::zero());
                locals.gate.ratios.set(locals.i, 0);
            }

            if (locals.i < input.allowedSenderCount)
            {
                locals.gate.allowedSenders.set(locals.i, input.allowedSenders.get(locals.i));
            }
            else
            {
                locals.gate.allowedSenders.set(locals.i, id::zero());
            }
        }

        state._gates.set(input.gateId - 1, locals.gate);

        if (qpi.invocationReward() > 0)
            qpi.transfer(qpi.invocator(), qpi.invocationReward());

        // Log success
        locals.logger._type = QUGATE_LOG_GATE_UPDATED;
        LOG_INFO(locals.logger);
    }

    // =============================================
    // Functions (read-only)
    // =============================================

    PUBLIC_FUNCTION_WITH_LOCALS(getGate)
    {
        if (input.gateId == 0 || input.gateId > state._gateCount)
        {
            output.active = 0;
            return;
        }

        locals.gate = state._gates.get(input.gateId - 1);
        output.mode = locals.gate.mode;
        output.recipientCount = locals.gate.recipientCount;
        output.active = locals.gate.active;
        output.owner = locals.gate.owner;
        output.totalReceived = locals.gate.totalReceived;
        output.totalForwarded = locals.gate.totalForwarded;
        output.currentBalance = locals.gate.currentBalance;
        output.threshold = locals.gate.threshold;
        output.createdEpoch = locals.gate.createdEpoch;
        output.lastActivityEpoch = locals.gate.lastActivityEpoch;  //

        for (locals.i = 0; locals.i < QUGATE_MAX_RECIPIENTS; locals.i++)
        {
            output.recipients.set(locals.i, locals.gate.recipients.get(locals.i));
            output.ratios.set(locals.i, locals.gate.ratios.get(locals.i));
        }
    }

    PUBLIC_FUNCTION(getGateCount)
    {
        output.totalGates = state._gateCount;
        output.activeGates = state._activeGates;
        output.totalBurned = state._totalBurned;         //
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getGatesByOwner)
    {
        output.count = 0;
        for (locals.i = 0; locals.i < state._gateCount && output.count < 16; locals.i++)
        {
            if (state._gates.get(locals.i).owner == input.owner)
            {
                output.gateIds.set(output.count, locals.i + 1);
                output.count += 1;
            }
        }
    }

    // Batch gate query — fetch up to 32 gates in one call
    PUBLIC_FUNCTION_WITH_LOCALS(getGateBatch)
    {
        for (locals.i = 0; locals.i < 32; locals.i++)
        {
            if (input.gateIds.get(locals.i) == 0 || input.gateIds.get(locals.i) > state._gateCount)
            {
                // Zero the entry for invalid IDs — clear all fields to avoid stale data
                locals.entry.mode = 0;
                locals.entry.recipientCount = 0;
                locals.entry.active = 0;
                locals.entry.owner = id::zero();
                locals.entry.totalReceived = 0;
                locals.entry.totalForwarded = 0;
                locals.entry.currentBalance = 0;
                locals.entry.threshold = 0;
                locals.entry.createdEpoch = 0;
                locals.entry.lastActivityEpoch = 0;
                for (locals.j = 0; locals.j < QUGATE_MAX_RECIPIENTS; locals.j++)
                {
                    locals.entry.recipients.set(locals.j, id::zero());
                    locals.entry.ratios.set(locals.j, 0);
                }
                output.gates.set(locals.i, locals.entry);
            }
            else
            {
                locals.gate = state._gates.get(input.gateIds.get(locals.i) - 1);

                locals.entry.mode = locals.gate.mode;
                locals.entry.recipientCount = locals.gate.recipientCount;
                locals.entry.active = locals.gate.active;
                locals.entry.owner = locals.gate.owner;
                locals.entry.totalReceived = locals.gate.totalReceived;
                locals.entry.totalForwarded = locals.gate.totalForwarded;
                locals.entry.currentBalance = locals.gate.currentBalance;
                locals.entry.threshold = locals.gate.threshold;
                locals.entry.createdEpoch = locals.gate.createdEpoch;
                locals.entry.lastActivityEpoch = locals.gate.lastActivityEpoch;

                for (locals.j = 0; locals.j < QUGATE_MAX_RECIPIENTS; locals.j++)
                {
                    locals.entry.recipients.set(locals.j, locals.gate.recipients.get(locals.j));
                    locals.entry.ratios.set(locals.j, locals.gate.ratios.get(locals.j));
                }

                output.gates.set(locals.i, locals.entry);
            }
        }
    }

    // Fee query
    PUBLIC_FUNCTION(getFees)
    {
        output.creationFee = state._creationFee;
        output.currentCreationFee = state._creationFee * (1 + QPI::div(state._activeGates, QUGATE_FEE_ESCALATION_STEP));
        output.minSendAmount = state._minSendAmount;
        output.expiryEpochs = state._expiryEpochs;
    }

    // =============================================
    // Registration
    // =============================================

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(createGate, 1);
        REGISTER_USER_PROCEDURE(sendToGate, 2);
        REGISTER_USER_PROCEDURE(closeGate, 3);
        REGISTER_USER_PROCEDURE(updateGate, 4);
        REGISTER_USER_FUNCTION(getGate, 5);
        REGISTER_USER_FUNCTION(getGateCount, 6);
        REGISTER_USER_FUNCTION(getGatesByOwner, 7);
        REGISTER_USER_FUNCTION(getGateBatch, 8);         //
        REGISTER_USER_FUNCTION(getFees, 9);               //
    }

    // =============================================
    // System procedures
    // =============================================

    INITIALIZE()
    {
        state._gateCount = 0;
        state._activeGates = 0;
        state._freeCount = 0;
        state._totalBurned = 0;                           //
        state._creationFee = QUGATE_DEFAULT_CREATION_FEE;  //
        state._minSendAmount = QUGATE_DEFAULT_MIN_SEND;    //
        state._expiryEpochs = QUGATE_DEFAULT_EXPIRY_EPOCHS; //
    }

    BEGIN_EPOCH() {}

    END_EPOCH_WITH_LOCALS()
    {
        // Expire inactive gates
        for (locals.i = 0; locals.i < state._gateCount; locals.i++)
        {
            locals.gate = state._gates.get(locals.i);

            if (locals.gate.active == 1 && state._expiryEpochs > 0)
            {
                if (qpi.epoch() - locals.gate.lastActivityEpoch >= state._expiryEpochs)
                {
                    // Refund any held balance (THRESHOLD mode)
                    if (locals.gate.currentBalance > 0)
                    {
                        qpi.transfer(locals.gate.owner, locals.gate.currentBalance);
                        locals.gate.currentBalance = 0;
                    }

                    locals.gate.active = 0;
                    state._gates.set(locals.i, locals.gate);
                    state._activeGates -= 1;

                    // Push slot onto free-list
                    state._freeSlots.set(state._freeCount, locals.i);
                    state._freeCount += 1;

                    // Log expiry
                    locals.logger._contractIndex = CONTRACT_INDEX;
                    locals.logger._type = QUGATE_LOG_GATE_EXPIRED;
                    locals.logger.gateId = locals.i + 1;
                    locals.logger.sender = locals.gate.owner;
                    locals.logger.amount = 0;
                    LOG_INFO(locals.logger);
                }
            }
        }

        // TODO: Check shareholder proposals and apply fee/expiry changes
        // When DEFINE_SHAREHOLDER_PROPOSAL_STORAGE is enabled:
        //   - Check if any proposal passed quorum
        //   - If so, update state._creationFee, state._minSendAmount, state._expiryEpochs
        //   - Log fee change event
    }

    BEGIN_TICK() {}
    END_TICK() {}

};


