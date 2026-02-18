using namespace QPI;

constexpr uint64 QBOND_MAX_EPOCH_COUNT = 1024ULL;
constexpr uint64 QBOND_MBOND_PRICE = 1000000ULL;
constexpr uint64 QBOND_MAX_QUEUE_SIZE = 10ULL;
constexpr uint64 QBOND_MIN_MBONDS_TO_STAKE = 10ULL;
constexpr sint64 QBOND_MBONDS_EMISSION = 1000000000LL;
constexpr uint64 QBOND_STAKE_LIMIT_PER_EPOCH = 1000000ULL;

constexpr uint16 QBOND_START_EPOCH = 182;
constexpr uint16 QBOND_CYCLIC_START_EPOCH = 192;
constexpr uint16 QBOND_FULL_CYCLE_EPOCHS_AMOUNT = 53;

constexpr uint64 QBOND_STAKE_FEE_PERCENT = 50; // 0.5%
constexpr uint64 QBOND_TRADE_FEE_PERCENT = 3; // 0.03%
constexpr uint64 QBOND_MBOND_TRANSFER_FEE = 100;

constexpr uint64 QBOND_QVAULT_DISTRIBUTION_PERCENT = 9900; // 99%

struct QBOND2
{
};

struct QBOND : public ContractBase
{
    struct StakeEntry
    {
        id staker;
        sint64 amount;
    };

    struct MBondInfo
    {
        uint64 name;
        sint64 stakersAmount;
        sint64 totalStaked;
    };

    struct Order
    {
        id owner;
        sint64 epoch;
        sint64 numberOfMBonds;
    };

    typedef Order _Order;

    struct _NumberOfReservedMBonds_input
    {
        id owner;
        sint64 epoch;
    };

    struct _NumberOfReservedMBonds_output
    {
        sint64 amount;
    };

    struct StateData
    {
        Array<StakeEntry, 16> _stakeQueue;
        HashMap<uint16, MBondInfo, QBOND_MAX_EPOCH_COUNT> _epochMbondInfoMap;
        HashMap<id, sint64, 524288> _userTotalStakedMap;
        HashSet<id, 1024> _commissionFreeAddresses;
        uint64 _qearnIncomeAmount;
        uint64 _totalEarnedAmount;
        uint64 _earnedAmountFromTrade;
        uint64 _distributedAmount;
        id _adminAddress;
        id _devAddress;
        Collection<Order, 1048576> _askOrders;
        Collection<Order, 1048576> _bidOrders;
        uint8 _cyclicMbondCounter;
        _NumberOfReservedMBonds_input _numberOfReservedMBonds_input;
        _NumberOfReservedMBonds_output _numberOfReservedMBonds_output;
    };

public:

    struct Stake_input
    {
        sint64 quMillions;
    };
    struct Stake_output
    {
    };

    struct TransferMBondOwnershipAndPossession_input
    {
        id newOwnerAndPossessor;
        sint64 epoch;
        sint64 numberOfMBonds;
    };
    struct TransferMBondOwnershipAndPossession_output
    {
        sint64 transferredMBonds;
    };

    struct AddAskOrder_input
    {
        sint64 epoch;
        sint64 price;
        sint64 numberOfMBonds;
    };
    struct AddAskOrder_output
    {
        sint64 addedMBondsAmount;
    };

    struct RemoveAskOrder_input
    {
        sint64 epoch;
        sint64 price;
        sint64 numberOfMBonds;
    };
    struct RemoveAskOrder_output
    {
        sint64 removedMBondsAmount;
    };

    struct AddBidOrder_input
    {
        sint64 epoch;
        sint64 price;
        sint64 numberOfMBonds;
    };
    struct AddBidOrder_output
    {
        sint64 addedMBondsAmount;
    };

    struct RemoveBidOrder_input
    {
        sint64 epoch;
        sint64 price;
        sint64 numberOfMBonds;
    };
    struct RemoveBidOrder_output
    {
        sint64 removedMBondsAmount;
    };

    struct BurnQU_input
    {
        sint64 amount;
    };
    struct BurnQU_output
    {
        sint64 amount;
    };

    struct UpdateCFA_input
    {
        id user;
        bit operation;  // 0 to remove, 1 to add
    };
    struct UpdateCFA_output
    {
        bit result;
    };

    struct GetFees_input
    {
    };
    struct GetFees_output
    {
        uint64 stakeFeePercent;
        uint64 tradeFeePercent;
        uint64 transferFee;
    };

    struct GetEarnedFees_input
    {
    };
    struct GetEarnedFees_output
    {
        uint64 stakeFees;
        uint64 tradeFees;
    };

    struct GetInfoPerEpoch_input
    {
        sint64 epoch;
    };
    struct GetInfoPerEpoch_output
    {
        uint64 stakersAmount;
        sint64 totalStaked;
        sint64 apy;
    };

    struct GetOrders_input
    {
        sint64 epoch;
        sint64 askOrdersOffset;
        sint64 bidOrdersOffset;
    };
    struct GetOrders_output
    {
        struct Order
        {
            id owner;
            sint64 epoch;
            sint64 numberOfMBonds;
            sint64 price;
        };

        Array<Order, 256> askOrders;
        Array<Order, 256> bidOrders;
    };

    struct GetUserOrders_input
    {
        id owner;
        sint64 askOrdersOffset;
        sint64 bidOrdersOffset;
    };
    struct GetUserOrders_output
    {
        struct Order
        {
            id owner;
            sint64 epoch;
            sint64 numberOfMBonds;
            sint64 price;
        };

        Array<Order, 256> askOrders;
        Array<Order, 256> bidOrders;
    };

    struct GetMBondsTable_input
    {
    };
    struct GetMBondsTable_output
    {
        struct TableEntry
        {
            sint64 epoch;
            sint64 totalStakedQBond;
            sint64 totalStakedQEarn;
            uint64 apy;
        };
        Array<TableEntry, 512> info;
    };

    struct GetUserMBonds_input
    {
        id owner;
    };
    struct GetUserMBonds_output
    {
        sint64 totalMBondsAmount;
        struct MBondEntity
        {
            sint64 epoch;
            sint64 amount;
            uint64 apy;
        };
        Array<MBondEntity, 256> mbonds;
    };

    struct GetCFA_input
    {
    };
    struct GetCFA_output
    {
        Array<id, 1024> commissionFreeAddresses;
    };

protected:

    struct _NumberOfReservedMBonds_locals
    {
        sint64 elementIndex;
        id mbondIdentity;
        _Order order;
        MBondInfo tempMbondInfo;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(_NumberOfReservedMBonds)
    {
        output.amount = 0;
        if (!state.get()._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo))
        {
            return;
        }

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
        if (input.epoch >= QBOND_CYCLIC_START_EPOCH)
        {
            locals.mbondIdentity.u16._3 = (uint16) input.epoch;
        }

        locals.elementIndex = state.get()._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            locals.order = state.get()._askOrders.element(locals.elementIndex);
            if (locals.order.epoch == input.epoch && locals.order.owner == input.owner)
            {
                output.amount += locals.order.numberOfMBonds;
            }

            locals.elementIndex = state.get()._askOrders.nextElementIndex(locals.elementIndex);
        }
    }

    struct Stake_locals
    {
        sint64 amountInQueue;
        sint64 userMBondsAmount;
        sint64 tempAmount;
        uint64 counter;
        sint64 amountToStake;
        uint64 amountAndFee;
        uint64 stakeLimitPerUser;
        StakeEntry tempStakeEntry;
        MBondInfo tempMbondInfo;
        QEARN::lock_input lock_input;
        QEARN::lock_output lock_output;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Stake)
    {
        locals.amountAndFee = sadd(smul((uint64) input.quMillions, QBOND_MBOND_PRICE), div(smul(smul((uint64) input.quMillions, QBOND_MBOND_PRICE), QBOND_STAKE_FEE_PERCENT), 10000ULL));

        if (input.quMillions <= 0
                || input.quMillions >= MAX_AMOUNT
                || !state.get()._epochMbondInfoMap.get(qpi.epoch(), locals.tempMbondInfo)
                || qpi.invocationReward() < 0
                || (uint64) qpi.invocationReward() < locals.amountAndFee
                || locals.tempMbondInfo.totalStaked + QBOND_MIN_MBONDS_TO_STAKE > QBOND_STAKE_LIMIT_PER_EPOCH)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if ((uint64) qpi.invocationReward() > locals.amountAndFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.amountAndFee);
        }

        if (state.get()._commissionFreeAddresses.getElementIndex(qpi.invocator()) != NULL_INDEX)
        {
            qpi.transfer(qpi.invocator(), div(smul((uint64) input.quMillions, QBOND_MBOND_PRICE) * QBOND_STAKE_FEE_PERCENT, 10000ULL));
        }
        else
        {
            state.mut()._totalEarnedAmount += div(smul((uint64) input.quMillions, QBOND_MBOND_PRICE) * QBOND_STAKE_FEE_PERCENT, 10000ULL);
        }

        locals.amountInQueue = input.quMillions;
        for (locals.counter = 0; locals.counter < QBOND_MAX_QUEUE_SIZE; locals.counter++)
        {
            if (state.get()._stakeQueue.get(locals.counter).staker != NULL_ID)
            {
                locals.amountInQueue += state.get()._stakeQueue.get(locals.counter).amount;
            }
            else
            {
                locals.stakeLimitPerUser = input.quMillions;
                if (locals.tempMbondInfo.totalStaked + locals.amountInQueue > QBOND_STAKE_LIMIT_PER_EPOCH)
                {
                    locals.stakeLimitPerUser = QBOND_STAKE_LIMIT_PER_EPOCH - locals.tempMbondInfo.totalStaked - (locals.amountInQueue - input.quMillions);
                    qpi.transfer(qpi.invocator(), (input.quMillions - locals.stakeLimitPerUser) * QBOND_MBOND_PRICE);
                }
                locals.tempStakeEntry.staker = qpi.invocator();
                locals.tempStakeEntry.amount = locals.stakeLimitPerUser;
                state.mut()._stakeQueue.set(locals.counter, locals.tempStakeEntry);
                break;
            }
        }

        if (locals.amountInQueue < QBOND_MIN_MBONDS_TO_STAKE)
        {
            return;
        }

        locals.tempStakeEntry.staker = NULL_ID;
        locals.tempStakeEntry.amount = 0;
        locals.amountToStake = 0;
        for (locals.counter = 0; locals.counter < QBOND_MAX_QUEUE_SIZE; locals.counter++)
        {
            if (state.get()._stakeQueue.get(locals.counter).staker == NULL_ID)
            {
                break;
            }

            if (state.get()._userTotalStakedMap.get(state.get()._stakeQueue.get(locals.counter).staker, locals.userMBondsAmount))
            {
                state.mut()._userTotalStakedMap.replace(state.get()._stakeQueue.get(locals.counter).staker, locals.userMBondsAmount + state.get()._stakeQueue.get(locals.counter).amount);
            }
            else
            {
                state.mut()._userTotalStakedMap.set(state.get()._stakeQueue.get(locals.counter).staker, state.get()._stakeQueue.get(locals.counter).amount);
            }

            if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, state.get()._stakeQueue.get(locals.counter).staker, state.get()._stakeQueue.get(locals.counter).staker, SELF_INDEX, SELF_INDEX) <= 0)
            {
                locals.tempMbondInfo.stakersAmount++;
            }
            qpi.transferShareOwnershipAndPossession(locals.tempMbondInfo.name, SELF, SELF, SELF, state.get()._stakeQueue.get(locals.counter).amount, state.get()._stakeQueue.get(locals.counter).staker);
            locals.amountToStake += state.get()._stakeQueue.get(locals.counter).amount;
            state.mut()._stakeQueue.set(locals.counter, locals.tempStakeEntry);
        }

        locals.tempMbondInfo.totalStaked += locals.amountToStake;
        state.mut()._epochMbondInfoMap.replace(qpi.epoch(), locals.tempMbondInfo);

        INVOKE_OTHER_CONTRACT_PROCEDURE(QEARN, lock, locals.lock_input, locals.lock_output, locals.amountToStake * QBOND_MBOND_PRICE);
    }

    struct TransferMBondOwnershipAndPossession_locals
    {
        MBondInfo tempMbondInfo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferMBondOwnershipAndPossession)
    {
        if (input.numberOfMBonds >= MAX_AMOUNT || input.numberOfMBonds <= 0 || qpi.invocationReward() < QBOND_MBOND_TRANSFER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (qpi.invocationReward() > QBOND_MBOND_TRANSFER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QBOND_MBOND_TRANSFER_FEE);
        }

        state.mut()._numberOfReservedMBonds_input.epoch = input.epoch;
        state.mut()._numberOfReservedMBonds_input.owner = qpi.invocator();
        CALL(_NumberOfReservedMBonds, state.mut()._numberOfReservedMBonds_input, state.mut()._numberOfReservedMBonds_output);

        if (state.get()._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo)
                && qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state.get()._numberOfReservedMBonds_output.amount < input.numberOfMBonds)
        {
            output.transferredMBonds = 0;
            qpi.transfer(qpi.invocator(), QBOND_MBOND_TRANSFER_FEE);
        }
        else
        {
            if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, input.newOwnerAndPossessor, input.newOwnerAndPossessor, SELF_INDEX, SELF_INDEX) <= 0)
            {
                locals.tempMbondInfo.stakersAmount++;
            }
            output.transferredMBonds = qpi.transferShareOwnershipAndPossession(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), input.numberOfMBonds, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfMBonds;
            if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) <= 0)
            {
                locals.tempMbondInfo.stakersAmount--;
            }
            state.mut()._epochMbondInfoMap.replace((uint16)input.epoch, locals.tempMbondInfo);
            if (state.get()._commissionFreeAddresses.getElementIndex(qpi.invocator()) != NULL_INDEX)
            {
                qpi.transfer(qpi.invocator(), QBOND_MBOND_TRANSFER_FEE);
            }
            else
            {
                state.mut()._totalEarnedAmount += QBOND_MBOND_TRANSFER_FEE;
            }
        }
    }

    struct AddAskOrder_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        sint64 nextElementIndex;
        sint64 fee;
        _Order tempAskOrder;
        _Order tempBidOrder;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AddAskOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (input.price <= 0 || input.price >= MAX_AMOUNT || input.numberOfMBonds <= 0 || input.numberOfMBonds >= MAX_AMOUNT || !state.get()._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo))
        {
            output.addedMBondsAmount = 0;
            return;
        }

        state.mut()._numberOfReservedMBonds_input.epoch = input.epoch;
        state.mut()._numberOfReservedMBonds_input.owner = qpi.invocator();
        CALL(_NumberOfReservedMBonds, state.mut()._numberOfReservedMBonds_input, state.mut()._numberOfReservedMBonds_output);
        if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state.get()._numberOfReservedMBonds_output.amount < input.numberOfMBonds)
        {
            output.addedMBondsAmount = 0;
            return;
        }

        output.addedMBondsAmount = input.numberOfMBonds;

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
        if (input.epoch >= QBOND_CYCLIC_START_EPOCH)
        {
            locals.mbondIdentity.u16._3 = (uint16) input.epoch;
        }

        locals.elementIndex = state.get()._bidOrders.headIndex(locals.mbondIdentity);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price > state.get()._bidOrders.priority(locals.elementIndex))
            {
                break;
            }

            locals.tempBidOrder = state.get()._bidOrders.element(locals.elementIndex);
            if (input.numberOfMBonds <= locals.tempBidOrder.numberOfMBonds)
            {
                qpi.transferShareOwnershipAndPossession(
                    locals.tempMbondInfo.name,
                    SELF,
                    qpi.invocator(),
                    qpi.invocator(),
                    input.numberOfMBonds,
                    locals.tempBidOrder.owner);

                locals.fee = div(input.numberOfMBonds * state.get()._bidOrders.priority(locals.elementIndex) * QBOND_TRADE_FEE_PERCENT, 10000ULL);
                qpi.transfer(qpi.invocator(), input.numberOfMBonds * state.get()._bidOrders.priority(locals.elementIndex) - locals.fee);
                if (state.get()._commissionFreeAddresses.getElementIndex(qpi.invocator()) != NULL_INDEX)
                {
                    qpi.transfer(qpi.invocator(), locals.fee);
                }
                else
                {
                    state.mut()._totalEarnedAmount += locals.fee;
                    state.mut()._earnedAmountFromTrade += locals.fee;
                }

                if (input.numberOfMBonds < locals.tempBidOrder.numberOfMBonds)
                {
                    locals.tempBidOrder.numberOfMBonds -= input.numberOfMBonds;
                    state.mut()._bidOrders.replace(locals.elementIndex, locals.tempBidOrder);
                }
                else if (input.numberOfMBonds == locals.tempBidOrder.numberOfMBonds)
                {
                    state.mut()._bidOrders.remove(locals.elementIndex);
                }
                return;
            }
            else if (input.numberOfMBonds > locals.tempBidOrder.numberOfMBonds)
            {
                qpi.transferShareOwnershipAndPossession(
                    locals.tempMbondInfo.name,
                    SELF,
                    qpi.invocator(),
                    qpi.invocator(),
                    locals.tempBidOrder.numberOfMBonds,
                    locals.tempBidOrder.owner);

                locals.fee = div(locals.tempBidOrder.numberOfMBonds * state.get()._bidOrders.priority(locals.elementIndex) * QBOND_TRADE_FEE_PERCENT, 10000ULL);
                qpi.transfer(qpi.invocator(), locals.tempBidOrder.numberOfMBonds * state.get()._bidOrders.priority(locals.elementIndex) - locals.fee);
                if (state.get()._commissionFreeAddresses.getElementIndex(qpi.invocator()) != NULL_INDEX)
                {
                    qpi.transfer(qpi.invocator(), locals.fee);
                }
                else
                {
                    state.mut()._totalEarnedAmount += locals.fee;
                    state.mut()._earnedAmountFromTrade += locals.fee;
                }
                locals.elementIndex = state.mut()._bidOrders.remove(locals.elementIndex);
                input.numberOfMBonds -= locals.tempBidOrder.numberOfMBonds;
            }
        }

        if (state.get()._askOrders.population(locals.mbondIdentity) == 0)
        {
            locals.tempAskOrder.epoch = input.epoch;
            locals.tempAskOrder.numberOfMBonds = input.numberOfMBonds;
            locals.tempAskOrder.owner = qpi.invocator();
            state.mut()._askOrders.add(locals.mbondIdentity, locals.tempAskOrder, -input.price);
            return;
        }

        locals.elementIndex = state.get()._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price < -state.get()._askOrders.priority(locals.elementIndex))
            {
                locals.tempAskOrder.epoch = input.epoch;
                locals.tempAskOrder.numberOfMBonds = input.numberOfMBonds;
                locals.tempAskOrder.owner = qpi.invocator();
                state.mut()._askOrders.add(locals.mbondIdentity, locals.tempAskOrder, -input.price);
                break;
            }
            else if (input.price == -state.get()._askOrders.priority(locals.elementIndex))
            {
                if (state.get()._askOrders.element(locals.elementIndex).owner == qpi.invocator())
                {
                    locals.tempAskOrder = state.get()._askOrders.element(locals.elementIndex);
                    locals.tempAskOrder.numberOfMBonds += input.numberOfMBonds;
                    state.mut()._askOrders.replace(locals.elementIndex, locals.tempAskOrder);
                    break;
                }
            }

            if (state.get()._askOrders.nextElementIndex(locals.elementIndex) == NULL_INDEX)
            {
                locals.tempAskOrder.epoch = input.epoch;
                locals.tempAskOrder.numberOfMBonds = input.numberOfMBonds;
                locals.tempAskOrder.owner = qpi.invocator();
                state.mut()._askOrders.add(locals.mbondIdentity, locals.tempAskOrder, -input.price);
                break;
            }

            locals.elementIndex = state.get()._askOrders.nextElementIndex(locals.elementIndex);
        }
    }

    struct RemoveAskOrder_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        _Order order;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveAskOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        output.removedMBondsAmount = 0;

        if (input.price <= 0 || input.price >= MAX_AMOUNT || input.numberOfMBonds <= 0 || input.numberOfMBonds >= MAX_AMOUNT || !state.get()._epochMbondInfoMap.get((uint16) input.epoch, locals.tempMbondInfo))
        {
            return;
        }

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
        if (input.epoch >= QBOND_CYCLIC_START_EPOCH)
        {
            locals.mbondIdentity.u16._3 = (uint16) input.epoch;
        }

        locals.elementIndex = state.get()._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price == -state.get()._askOrders.priority(locals.elementIndex) && state.get()._askOrders.element(locals.elementIndex).owner == qpi.invocator())
            {
                if (state.get()._askOrders.element(locals.elementIndex).numberOfMBonds <= input.numberOfMBonds)
                {
                    output.removedMBondsAmount = state.get()._askOrders.element(locals.elementIndex).numberOfMBonds;
                    state.mut()._askOrders.remove(locals.elementIndex);
                }
                else
                {
                    locals.order = state.get()._askOrders.element(locals.elementIndex);
                    locals.order.numberOfMBonds -= input.numberOfMBonds;
                    state.mut()._askOrders.replace(locals.elementIndex, locals.order);
                    output.removedMBondsAmount = input.numberOfMBonds;
                }
                break;
            }

            locals.elementIndex = state.get()._askOrders.nextElementIndex(locals.elementIndex);
        }
    }

    struct AddBidOrder_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        sint64 fee;
        _Order tempAskOrder;
        _Order tempBidOrder;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AddBidOrder)
    {
        if (qpi.invocationReward() < smul(input.numberOfMBonds, input.price)
                || input.price <= 0
                || input.price >= MAX_AMOUNT
                || input.numberOfMBonds <= 0
                || input.numberOfMBonds >= MAX_AMOUNT
                || !state.get()._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo))
        {
            output.addedMBondsAmount = 0;
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (qpi.invocationReward() > smul(input.numberOfMBonds, input.price))
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - smul(input.numberOfMBonds, input.price));
        }

        output.addedMBondsAmount = input.numberOfMBonds;

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
        if (input.epoch >= QBOND_CYCLIC_START_EPOCH)
        {
            locals.mbondIdentity.u16._3 = (uint16) input.epoch;
        }

        locals.elementIndex = state.get()._askOrders.headIndex(locals.mbondIdentity);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price < -state.get()._askOrders.priority(locals.elementIndex))
            {
                break;
            }

            locals.tempAskOrder = state.get()._askOrders.element(locals.elementIndex);
            if (input.numberOfMBonds <= locals.tempAskOrder.numberOfMBonds)
            {
                qpi.transferShareOwnershipAndPossession(
                    locals.tempMbondInfo.name,
                    SELF,
                    locals.tempAskOrder.owner,
                    locals.tempAskOrder.owner,
                    input.numberOfMBonds,
                    qpi.invocator());

                if (state.get()._commissionFreeAddresses.getElementIndex(locals.tempAskOrder.owner) != NULL_INDEX)
                {
                    qpi.transfer(locals.tempAskOrder.owner, -(input.numberOfMBonds * state.get()._askOrders.priority(locals.elementIndex)));
                }
                else
                {
                    locals.fee = div(input.numberOfMBonds * -state.get()._askOrders.priority(locals.elementIndex) * QBOND_TRADE_FEE_PERCENT, 10000ULL);
                    qpi.transfer(locals.tempAskOrder.owner, -(input.numberOfMBonds * state.get()._askOrders.priority(locals.elementIndex)) - locals.fee);
                    state.mut()._totalEarnedAmount += locals.fee;
                    state.mut()._earnedAmountFromTrade += locals.fee;
                }

                if (input.price > -state.get()._askOrders.priority(locals.elementIndex))
                {
                    qpi.transfer(qpi.invocator(), input.numberOfMBonds * (input.price + state.get()._askOrders.priority(locals.elementIndex)));   // ask orders priotiry is always negative
                }

                if (input.numberOfMBonds < locals.tempAskOrder.numberOfMBonds)
                {
                    locals.tempAskOrder.numberOfMBonds -= input.numberOfMBonds;
                    state.mut()._askOrders.replace(locals.elementIndex, locals.tempAskOrder);
                }
                else if (input.numberOfMBonds == locals.tempAskOrder.numberOfMBonds)
                {
                    state.mut()._askOrders.remove(locals.elementIndex);
                }
                return;
            }
            else if (input.numberOfMBonds > locals.tempAskOrder.numberOfMBonds)
            {
                qpi.transferShareOwnershipAndPossession(
                    locals.tempMbondInfo.name,
                    SELF,
                    locals.tempAskOrder.owner,
                    locals.tempAskOrder.owner,
                    locals.tempAskOrder.numberOfMBonds,
                    qpi.invocator());

                if (state.get()._commissionFreeAddresses.getElementIndex(locals.tempAskOrder.owner) != NULL_INDEX)
                {
                    qpi.transfer(locals.tempAskOrder.owner, -(locals.tempAskOrder.numberOfMBonds * state.get()._askOrders.priority(locals.elementIndex)));
                }
                else
                {
                    locals.fee = div(locals.tempAskOrder.numberOfMBonds * -state.get()._askOrders.priority(locals.elementIndex) * QBOND_TRADE_FEE_PERCENT, 10000ULL);
                    qpi.transfer(locals.tempAskOrder.owner, -(locals.tempAskOrder.numberOfMBonds * state.get()._askOrders.priority(locals.elementIndex)) - locals.fee);
                    state.mut()._totalEarnedAmount += locals.fee;
                    state.mut()._earnedAmountFromTrade += locals.fee;
                }

                if (input.price > -state.get()._askOrders.priority(locals.elementIndex))
                {
                    qpi.transfer(qpi.invocator(), locals.tempAskOrder.numberOfMBonds * (input.price + state.get()._askOrders.priority(locals.elementIndex)));   // ask orders priotiry is always negative
                }

                locals.elementIndex = state.mut()._askOrders.remove(locals.elementIndex);
                input.numberOfMBonds -= locals.tempAskOrder.numberOfMBonds;
            }
        }

        if (state.get()._bidOrders.population(locals.mbondIdentity) == 0)
        {
            locals.tempBidOrder.epoch = input.epoch;
            locals.tempBidOrder.numberOfMBonds = input.numberOfMBonds;
            locals.tempBidOrder.owner = qpi.invocator();
            state.mut()._bidOrders.add(locals.mbondIdentity, locals.tempBidOrder, input.price);
            return;
        }

        locals.elementIndex = state.get()._bidOrders.headIndex(locals.mbondIdentity);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price > state.get()._bidOrders.priority(locals.elementIndex))
            {
                locals.tempBidOrder.epoch = input.epoch;
                locals.tempBidOrder.numberOfMBonds = input.numberOfMBonds;
                locals.tempBidOrder.owner = qpi.invocator();
                state.mut()._bidOrders.add(locals.mbondIdentity, locals.tempBidOrder, input.price);
                break;
            }
            else if (input.price == state.get()._bidOrders.priority(locals.elementIndex))
            {
                if (state.get()._bidOrders.element(locals.elementIndex).owner == qpi.invocator())
                {
                    locals.tempBidOrder = state.get()._bidOrders.element(locals.elementIndex);
                    locals.tempBidOrder.numberOfMBonds += input.numberOfMBonds;
                    state.mut()._bidOrders.replace(locals.elementIndex, locals.tempBidOrder);
                    break;
                }
            }

            if (state.get()._bidOrders.nextElementIndex(locals.elementIndex) == NULL_INDEX)
            {
                locals.tempBidOrder.epoch = input.epoch;
                locals.tempBidOrder.numberOfMBonds = input.numberOfMBonds;
                locals.tempBidOrder.owner = qpi.invocator();
                state.mut()._bidOrders.add(locals.mbondIdentity, locals.tempBidOrder, input.price);
                break;
            }

            locals.elementIndex = state.get()._bidOrders.nextElementIndex(locals.elementIndex);
        }
    }

    struct RemoveBidOrder_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        _Order order;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveBidOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        output.removedMBondsAmount = 0;

        if (input.price <= 0 || input.price >= MAX_AMOUNT || input.numberOfMBonds <= 0 || input.numberOfMBonds >= MAX_AMOUNT || !state.get()._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo))
        {
            return;
        }

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
        if (input.epoch >= QBOND_CYCLIC_START_EPOCH)
        {
            locals.mbondIdentity.u16._3 = (uint16) input.epoch;
        }

        locals.elementIndex = state.get()._bidOrders.headIndex(locals.mbondIdentity);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price == state.get()._bidOrders.priority(locals.elementIndex) && state.get()._bidOrders.element(locals.elementIndex).owner == qpi.invocator())
            {
                if (state.get()._bidOrders.element(locals.elementIndex).numberOfMBonds <= input.numberOfMBonds)
                {
                    output.removedMBondsAmount = state.get()._bidOrders.element(locals.elementIndex).numberOfMBonds;
                    state.mut()._bidOrders.remove(locals.elementIndex);
                }
                else
                {
                    locals.order = state.get()._bidOrders.element(locals.elementIndex);
                    locals.order.numberOfMBonds -= input.numberOfMBonds;
                    state.mut()._bidOrders.replace(locals.elementIndex, locals.order);
                    output.removedMBondsAmount = input.numberOfMBonds;
                }
                qpi.transfer(qpi.invocator(), output.removedMBondsAmount * input.price);
                break;
            }

            locals.elementIndex = state.get()._bidOrders.nextElementIndex(locals.elementIndex);
        }
    }

    PUBLIC_PROCEDURE(BurnQU)
    {
        if (input.amount <= 0 || input.amount >= MAX_AMOUNT || qpi.invocationReward() < input.amount)
        {
            output.amount = -1;
            if (input.amount == 0)
            {
                output.amount = 0;
            }

            if (qpi.invocationReward() > 0 && qpi.invocationReward() < MAX_AMOUNT)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (qpi.invocationReward() > input.amount)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount);
        }

        qpi.burn(input.amount);
        output.amount = input.amount;
    }

    PUBLIC_PROCEDURE(UpdateCFA)
    {
        if (qpi.invocationReward() > 0 && qpi.invocationReward() <= MAX_AMOUNT)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (qpi.invocator() != state.get()._adminAddress)
        {
            return;
        }

        if (input.operation == 0)
        {
            output.result = state.mut()._commissionFreeAddresses.remove(input.user);
        }
        else
        {
            output.result = state.mut()._commissionFreeAddresses.add(input.user);
        }
    }

    struct GetInfoPerEpoch_locals
    {
        sint64 index;
        QEARN::getLockInfoPerEpoch_input tempInput;
        QEARN::getLockInfoPerEpoch_output tempOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetInfoPerEpoch)
    {
        output.totalStaked = 0;
        output.stakersAmount = 0;
        output.apy = 0;

        locals.index = state.get()._epochMbondInfoMap.getElementIndex((uint16)input.epoch);

        if (locals.index == NULL_INDEX)
        {
            return;
        }

        locals.tempInput.Epoch = (uint32) input.epoch;
        CALL_OTHER_CONTRACT_FUNCTION(QEARN, getLockInfoPerEpoch, locals.tempInput, locals.tempOutput);

        output.totalStaked = state.get()._epochMbondInfoMap.value(locals.index).totalStaked;
        output.stakersAmount = state.get()._epochMbondInfoMap.value(locals.index).stakersAmount;
        output.apy = locals.tempOutput.yield;
    }

    PUBLIC_FUNCTION(GetFees)
    {
        output.stakeFeePercent = QBOND_STAKE_FEE_PERCENT;
        output.tradeFeePercent = QBOND_TRADE_FEE_PERCENT;
        output.transferFee = QBOND_MBOND_TRANSFER_FEE;
    }

    PUBLIC_FUNCTION(GetEarnedFees)
    {
        output.stakeFees = state.get()._totalEarnedAmount - state.get()._earnedAmountFromTrade;
        output.tradeFees = state.get()._earnedAmountFromTrade;
    }

    struct GetOrders_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        sint64 arrayElementIndex;
        sint64 arrayElementIndex2;
        sint64 startEpoch;
        sint64 endEpoch;
        sint64 epochCounter;
        GetOrders_output::Order tempOrder;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetOrders)
    {
        if (input.epoch != 0 && !state.get()._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo))
        {
            return;
        }

        locals.arrayElementIndex = 0;
        locals.arrayElementIndex2 = 0;
        locals.mbondIdentity = SELF;

        if (input.epoch == 0)
        {
            locals.startEpoch = QBOND_START_EPOCH;
            locals.endEpoch = qpi.epoch();
        }
        else
        {
            locals.startEpoch = input.epoch;
            locals.endEpoch = input.epoch;
        }

        for (locals.epochCounter = locals.startEpoch; locals.epochCounter <= locals.endEpoch; locals.epochCounter++)
        {
            if (!state.get()._epochMbondInfoMap.get((uint16)locals.epochCounter, locals.tempMbondInfo))
            {
                continue;
            }
            locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
            if (locals.epochCounter >= QBOND_CYCLIC_START_EPOCH)
            {
                locals.mbondIdentity.u16._3 = (uint16) locals.epochCounter;
            }

            locals.elementIndex = state.get()._askOrders.headIndex(locals.mbondIdentity, 0);
            while (locals.elementIndex != NULL_INDEX && locals.arrayElementIndex < 256)
            {
                if (input.askOrdersOffset > 0)
                {
                    input.askOrdersOffset--;
                    locals.elementIndex = state.get()._askOrders.nextElementIndex(locals.elementIndex);
                    continue;
                }

                locals.tempOrder.owner = state.get()._askOrders.element(locals.elementIndex).owner;
                locals.tempOrder.epoch = state.get()._askOrders.element(locals.elementIndex).epoch;
                locals.tempOrder.numberOfMBonds = state.get()._askOrders.element(locals.elementIndex).numberOfMBonds;
                locals.tempOrder.price = -state.get()._askOrders.priority(locals.elementIndex);
                output.askOrders.set(locals.arrayElementIndex, locals.tempOrder);
                locals.arrayElementIndex++;
                locals.elementIndex = state.get()._askOrders.nextElementIndex(locals.elementIndex);
            }

            locals.elementIndex = state.get()._bidOrders.headIndex(locals.mbondIdentity);
            while (locals.elementIndex != NULL_INDEX && locals.arrayElementIndex2 < 256)
            {
                if (input.bidOrdersOffset > 0)
                {
                    input.bidOrdersOffset--;
                    locals.elementIndex = state.get()._bidOrders.nextElementIndex(locals.elementIndex);
                    continue;
                }

                locals.tempOrder.owner = state.get()._bidOrders.element(locals.elementIndex).owner;
                locals.tempOrder.epoch = state.get()._bidOrders.element(locals.elementIndex).epoch;
                locals.tempOrder.numberOfMBonds = state.get()._bidOrders.element(locals.elementIndex).numberOfMBonds;
                locals.tempOrder.price = state.get()._bidOrders.priority(locals.elementIndex);
                output.bidOrders.set(locals.arrayElementIndex2, locals.tempOrder);
                locals.arrayElementIndex2++;
                locals.elementIndex = state.get()._bidOrders.nextElementIndex(locals.elementIndex);
            }
        }
    }

    struct GetUserOrders_locals
    {
        sint64 epoch;
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex1;
        sint64 arrayElementIndex1;
        sint64 elementIndex2;
        sint64 arrayElementIndex2;
        GetUserOrders_output::Order tempOrder;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserOrders)
    {
        for (locals.epoch = QBOND_START_EPOCH; locals.epoch <= qpi.epoch(); locals.epoch++)
        {
            if (!state.get()._epochMbondInfoMap.get((uint16)locals.epoch, locals.tempMbondInfo))
            {
                continue;
            }

            locals.mbondIdentity = SELF;
            locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
            if (locals.epoch >= QBOND_CYCLIC_START_EPOCH)
            {
                locals.mbondIdentity.u16._3 = (uint16) locals.epoch;
            }

            locals.elementIndex1 = state.get()._askOrders.headIndex(locals.mbondIdentity, 0);
            while (locals.elementIndex1 != NULL_INDEX && locals.arrayElementIndex1 < 256)
            {
                if (state.get()._askOrders.element(locals.elementIndex1).owner != input.owner)
                {
                    locals.elementIndex1 = state.get()._askOrders.nextElementIndex(locals.elementIndex1);
                    continue;
                }

                if (input.askOrdersOffset > 0)
                {
                    input.askOrdersOffset--;
                    locals.elementIndex1 = state.get()._askOrders.nextElementIndex(locals.elementIndex1);
                    continue;
                }

                locals.tempOrder.owner = input.owner;
                locals.tempOrder.epoch = state.get()._askOrders.element(locals.elementIndex1).epoch;
                locals.tempOrder.numberOfMBonds = state.get()._askOrders.element(locals.elementIndex1).numberOfMBonds;
                locals.tempOrder.price = -state.get()._askOrders.priority(locals.elementIndex1);
                output.askOrders.set(locals.arrayElementIndex1, locals.tempOrder);
                locals.arrayElementIndex1++;
                locals.elementIndex1 = state.get()._askOrders.nextElementIndex(locals.elementIndex1);
            }

            locals.elementIndex2 = state.get()._bidOrders.headIndex(locals.mbondIdentity);
            while (locals.elementIndex2 != NULL_INDEX && locals.arrayElementIndex2 < 256)
            {
                if (state.get()._bidOrders.element(locals.elementIndex2).owner != input.owner)
                {
                    locals.elementIndex2 = state.get()._bidOrders.nextElementIndex(locals.elementIndex2);
                    continue;
                }

                if (input.bidOrdersOffset > 0)
                {
                    input.bidOrdersOffset--;
                    locals.elementIndex2 = state.get()._bidOrders.nextElementIndex(locals.elementIndex2);
                    continue;
                }

                locals.tempOrder.owner = input.owner;
                locals.tempOrder.epoch = state.get()._bidOrders.element(locals.elementIndex2).epoch;
                locals.tempOrder.numberOfMBonds = state.get()._bidOrders.element(locals.elementIndex2).numberOfMBonds;
                locals.tempOrder.price = state.get()._bidOrders.priority(locals.elementIndex2);
                output.bidOrders.set(locals.arrayElementIndex2, locals.tempOrder);
                locals.arrayElementIndex2++;
                locals.elementIndex2 = state.get()._bidOrders.nextElementIndex(locals.elementIndex2);
            }
        }
    }

    struct GetMBondsTable_locals
    {
        sint64 epoch;
        sint64 index;
        MBondInfo tempMBondInfo;
        GetMBondsTable_output::TableEntry tempTableEntry;
        QEARN::getLockInfoPerEpoch_input tempInput;
        QEARN::getLockInfoPerEpoch_output tempOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetMBondsTable)
    {
        for (locals.epoch = QBOND_START_EPOCH; locals.epoch <= qpi.epoch(); locals.epoch++)
        {
            if (state.get()._epochMbondInfoMap.get((uint16)locals.epoch, locals.tempMBondInfo))
            {
                locals.tempInput.Epoch = (uint32) locals.epoch;
                CALL_OTHER_CONTRACT_FUNCTION(QEARN, getLockInfoPerEpoch, locals.tempInput, locals.tempOutput);
                locals.tempTableEntry.epoch = locals.epoch;
                locals.tempTableEntry.totalStakedQBond = locals.tempMBondInfo.totalStaked * QBOND_MBOND_PRICE;
                locals.tempTableEntry.totalStakedQEarn = locals.tempOutput.currentLockedAmount;
                locals.tempTableEntry.apy = locals.tempOutput.yield;
                output.info.set(locals.index, locals.tempTableEntry);
                locals.index++;
            }
        }
    }

    struct GetUserMBonds_locals
    {
        GetUserMBonds_output::MBondEntity tempMbondEntity;
        sint64 epoch;
        sint64 index;
        sint64 mbondsAmount;
        MBondInfo tempMBondInfo;
        QEARN::getLockInfoPerEpoch_input tempInput;
        QEARN::getLockInfoPerEpoch_output tempOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserMBonds)
    {
        output.totalMBondsAmount = 0;
        if (state.get()._userTotalStakedMap.get(input.owner, locals.mbondsAmount))
        {
            output.totalMBondsAmount = locals.mbondsAmount;
        }

        for (locals.epoch = QBOND_START_EPOCH; locals.epoch <= qpi.epoch(); locals.epoch++)
        {
            if (!state.get()._epochMbondInfoMap.get((uint16)locals.epoch, locals.tempMBondInfo))
            {
                continue;
            }

            locals.mbondsAmount = qpi.numberOfPossessedShares(locals.tempMBondInfo.name, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX);
            if (locals.mbondsAmount <= 0)
            {
                continue;
            }

            locals.tempInput.Epoch = (uint32) locals.epoch;
            CALL_OTHER_CONTRACT_FUNCTION(QEARN, getLockInfoPerEpoch, locals.tempInput, locals.tempOutput);

            locals.tempMbondEntity.epoch = locals.epoch;
            locals.tempMbondEntity.amount = locals.mbondsAmount;
            locals.tempMbondEntity.apy = locals.tempOutput.yield;
            output.mbonds.set(locals.index, locals.tempMbondEntity);
            locals.index++;
        }
    }

    struct GetCFA_locals
    {
        sint64 index;
        sint64 counter;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetCFA)
    {
        locals.index = state.get()._commissionFreeAddresses.nextElementIndex(NULL_INDEX);
        while (locals.index != NULL_INDEX)
        {
            output.commissionFreeAddresses.set(locals.counter, state.get()._commissionFreeAddresses.key(locals.index));
            locals.counter++;
            locals.index = state.get()._commissionFreeAddresses.nextElementIndex(locals.index);
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(Stake, 1);
        REGISTER_USER_PROCEDURE(TransferMBondOwnershipAndPossession, 2);
        REGISTER_USER_PROCEDURE(AddAskOrder, 3);
        REGISTER_USER_PROCEDURE(RemoveAskOrder, 4);
        REGISTER_USER_PROCEDURE(AddBidOrder, 5);
        REGISTER_USER_PROCEDURE(RemoveBidOrder, 6);
        REGISTER_USER_PROCEDURE(BurnQU, 7);
        REGISTER_USER_PROCEDURE(UpdateCFA, 8);

        REGISTER_USER_FUNCTION(GetFees, 1);
        REGISTER_USER_FUNCTION(GetEarnedFees, 2);
        REGISTER_USER_FUNCTION(GetInfoPerEpoch, 3);
        REGISTER_USER_FUNCTION(GetOrders, 4);
        REGISTER_USER_FUNCTION(GetUserOrders, 5);
        REGISTER_USER_FUNCTION(GetMBondsTable, 6);
        REGISTER_USER_FUNCTION(GetUserMBonds, 7);
        REGISTER_USER_FUNCTION(GetCFA, 8);
    }

    INITIALIZE()
    {
        state.mut()._devAddress = ID(_B, _O, _N, _D, _D, _J, _N, _U, _H, _O, _G, _Y, _L, _A, _A, _A, _C, _V, _X, _C, _X, _F, _G, _F, _R, _C, _S, _D, _C, _U, _W, _C, _Y, _U, _N, _K, _M, _P, _G, _O, _I, _F, _E, _P, _O, _E, _M, _Y, _T, _L, _Q, _L, _F, _C, _S, _B);
        state.mut()._adminAddress = ID(_B, _O, _N, _D, _A, _A, _F, _B, _U, _G, _H, _E, _L, _A, _N, _X, _G, _H, _N, _L, _M, _S, _U, _I, _V, _B, _K, _B, _H, _A, _Y, _E, _Q, _S, _Q, _B, _V, _P, _V, _N, _B, _H, _L, _F, _J, _I, _A, _Z, _F, _Q, _C, _W, _W, _B, _V, _E);
        state.mut()._commissionFreeAddresses.add(state.get()._adminAddress);
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        sint8 chunk;
        uint64 currentName;
        StakeEntry emptyEntry;
        sint64 totalReward;
        sint64 rewardPerMBond;
        Asset tempAsset;
        MBondInfo tempMbondInfo;
        AssetOwnershipIterator assetIt;
        id mbondIdentity;
        sint64 elementIndex;
        uint64 counter;
        _Order tempOrder;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        if (state.get()._qearnIncomeAmount > 0 && state.get()._epochMbondInfoMap.get((uint16) (qpi.epoch() - 53), locals.tempMbondInfo))
        {
            locals.totalReward = state.get()._qearnIncomeAmount - locals.tempMbondInfo.totalStaked * QBOND_MBOND_PRICE;
            locals.rewardPerMBond = QPI::div(locals.totalReward, locals.tempMbondInfo.totalStaked);

            locals.tempAsset.assetName = locals.tempMbondInfo.name;
            locals.tempAsset.issuer = SELF;
            locals.assetIt.begin(locals.tempAsset);
            while (!locals.assetIt.reachedEnd())
            {
                if (locals.assetIt.owner() == SELF)
                {
                    locals.assetIt.next();
                    continue;
                }
                qpi.transfer(locals.assetIt.owner(), (QBOND_MBOND_PRICE + locals.rewardPerMBond) * locals.assetIt.numberOfOwnedShares());

                if (qpi.epoch() - 53 < QBOND_CYCLIC_START_EPOCH)
                {
                    qpi.transferShareOwnershipAndPossession(
                        locals.tempMbondInfo.name,
                        SELF,
                        locals.assetIt.owner(),
                        locals.assetIt.owner(),
                        locals.assetIt.numberOfOwnedShares(),
                        NULL_ID);
                }
                else
                {
                    qpi.transferShareOwnershipAndPossession(
                        locals.tempMbondInfo.name,
                        SELF,
                        locals.assetIt.owner(),
                        locals.assetIt.owner(),
                        locals.assetIt.numberOfOwnedShares(),
                        SELF);
                }

                locals.assetIt.next();
            }
            state.mut()._qearnIncomeAmount = 0;

            locals.mbondIdentity = SELF;
            locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;
            if ((uint16) (qpi.epoch() - 53) >= QBOND_CYCLIC_START_EPOCH)
            {
                locals.mbondIdentity.u16._3 = (uint16) (qpi.epoch() - 53);
            }

            locals.elementIndex = state.get()._askOrders.headIndex(locals.mbondIdentity);
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.elementIndex = state.mut()._askOrders.remove(locals.elementIndex);
            }

            locals.elementIndex = state.get()._bidOrders.headIndex(locals.mbondIdentity);
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempOrder = state.get()._bidOrders.element(locals.elementIndex);
                qpi.transfer(locals.tempOrder.owner, locals.tempOrder.numberOfMBonds * state.get()._bidOrders.priority(locals.elementIndex));
                locals.elementIndex = state.mut()._bidOrders.remove(locals.elementIndex);
            }
        }

        if (state.get()._cyclicMbondCounter >= QBOND_FULL_CYCLE_EPOCHS_AMOUNT)
        {
            state.mut()._cyclicMbondCounter = 1;
        }
        else
        {
            state.mut()._cyclicMbondCounter++;
        }

        if (qpi.epoch() == QBOND_CYCLIC_START_EPOCH)
        {
            state.mut()._cyclicMbondCounter = 1;
            for (locals.counter = 1; locals.counter <= QBOND_FULL_CYCLE_EPOCHS_AMOUNT; locals.counter++)
            {
                locals.currentName = 1145979469ULL;   // MBND

                locals.chunk = (sint8) (48 + div(locals.counter, 10ULL));
                locals.currentName |= (uint64)locals.chunk << (4 * 8);

                locals.chunk = (sint8) (48 + mod(locals.counter, 10ULL));
                locals.currentName |= (uint64)locals.chunk << (5 * 8);

                qpi.issueAsset(locals.currentName, SELF, 0, QBOND_MBONDS_EMISSION, 0);
            }
        }

        locals.currentName = 1145979469ULL;   // MBND
        locals.chunk = (sint8) (48 + div(state.get()._cyclicMbondCounter, (uint8) 10));
        locals.currentName |= (uint64)locals.chunk << (4 * 8);

        locals.chunk = (sint8) (48 + mod(state.get()._cyclicMbondCounter, (uint8) 10));
        locals.currentName |= (uint64)locals.chunk << (5 * 8);

        locals.tempMbondInfo.name = locals.currentName;
        locals.tempMbondInfo.totalStaked = 0;
        locals.tempMbondInfo.stakersAmount = 0;
        state.mut()._epochMbondInfoMap.set(qpi.epoch(), locals.tempMbondInfo);

        locals.emptyEntry.staker = NULL_ID;
        locals.emptyEntry.amount = 0;
        state.mut()._stakeQueue.setAll(locals.emptyEntry);
    }

    struct POST_INCOMING_TRANSFER_locals
    {
        MBondInfo tempMbondInfo;
    };

    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        if (input.sourceId == id(QEARN_CONTRACT_INDEX, 0, 0, 0) && state.get()._epochMbondInfoMap.get(qpi.epoch() - 52, locals.tempMbondInfo))
        {
            state.mut()._qearnIncomeAmount = input.amount;
        }
    }

    struct END_EPOCH_locals
    {
        sint64 availableMbonds;
        MBondInfo tempMbondInfo;
        sint64 counter;
        StakeEntry tempStakeEntry;
        sint64 amountToQvault;
        sint64 amountToDev;
    };

    END_EPOCH_WITH_LOCALS()
    {
        locals.amountToQvault = div((state.get()._totalEarnedAmount - state.get()._distributedAmount) * QBOND_QVAULT_DISTRIBUTION_PERCENT, 10000ULL);
        locals.amountToDev = state.get()._totalEarnedAmount - state.get()._distributedAmount - locals.amountToQvault;
        qpi.transfer(id(QVAULT_CONTRACT_INDEX, 0, 0, 0), locals.amountToQvault);
        qpi.transfer(state.get()._devAddress, locals.amountToDev);
        state.mut()._distributedAmount += locals.amountToQvault;
        state.mut()._distributedAmount += locals.amountToDev;

        locals.tempStakeEntry.staker = NULL_ID;
        locals.tempStakeEntry.amount = 0;
        for (locals.counter = 0; locals.counter < QBOND_MAX_QUEUE_SIZE; locals.counter++)
        {
            if (state.get()._stakeQueue.get(locals.counter).staker == NULL_ID)
            {
                break;
            }

            qpi.transfer(state.get()._stakeQueue.get(locals.counter).staker, state.get()._stakeQueue.get(locals.counter).amount * QBOND_MBOND_PRICE);
            state.mut()._stakeQueue.set(locals.counter, locals.tempStakeEntry);
        }

        if (state.get()._commissionFreeAddresses.needsCleanup()) { state.mut()._commissionFreeAddresses.cleanup(); }
        if (state.get()._askOrders.needsCleanup()) { state.mut()._askOrders.cleanup(); }
        if (state.get()._bidOrders.needsCleanup()) { state.mut()._bidOrders.cleanup(); }
    }
};
