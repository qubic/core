using namespace QPI;

constexpr uint64 MSVAULT_MAX_OWNERS = 16;
constexpr uint64 MSVAULT_MAX_COOWNER = 8;
constexpr uint64 MSVAULT_INITIAL_MAX_VAULTS = 131072ULL; // 2^17
constexpr uint64 MSVAULT_MAX_VAULTS = MSVAULT_INITIAL_MAX_VAULTS * X_MULTIPLIER;
// MSVAULT asset name : 23727827095802701, using assetNameFromString("MSVAULT") utility in test_util.h
static constexpr uint64 MSVAULT_ASSET_NAME = 23727827095802701;

constexpr uint64 MSVAULT_REGISTERING_FEE = 5000000ULL;
constexpr uint64 MSVAULT_RELEASE_FEE = 100000ULL;
constexpr uint64 MSVAULT_RELEASE_RESET_FEE = 1000000ULL;
constexpr uint64 MSVAULT_HOLDING_FEE = 500000ULL;
constexpr uint64 MSVAULT_BURN_FEE = 0ULL; // Integer percentage from 1 -> 100
// [TODO]: Turn this assert ON when MSVAULT_BURN_FEE > 0
//static_assert(MSVAULT_BURN_FEE > 0, "SC requires burning qu to operate, the burn fee must be higher than 0!");

static constexpr uint64 MSVAULT_MAX_FEE_VOTES = 64;


struct MSVAULT2
{
};

struct MSVAULT : public ContractBase
{
public:
    struct Vault
    {
        id vaultName;
        Array<id, MSVAULT_MAX_OWNERS> owners;
        Array<uint64, MSVAULT_MAX_OWNERS> releaseAmounts;
        Array<id, MSVAULT_MAX_OWNERS> releaseDestinations;
        uint64 balance;
        uint8 numberOfOwners;
        uint8 requiredApprovals;
        bit isActive;
    };

    struct MsVaultFeeVote 
    {
        uint64 registeringFee;
        uint64 releaseFee;
        uint64 releaseResetFee;
        uint64 holdingFee;
        uint64 depositFee;
        uint64 burnFee;
    };

    struct MSVaultLogger
    {
        uint32 _contractIndex;
        // 1: Invalid vault ID or vault inactive
        // 2: Caller not an owner
        // 3: Invalid parameters (e.g., amount=0, destination=NULL_ID)
        // 4: Release successful
        // 5: Insufficient balance
        // 6: Release not fully approved
        // 7: Reset release requests successful
        uint32 _type; 
        uint64 vaultId; 
        id ownerID;
        uint64 amount;
        id destination;
        sint8 _terminator;
    };

    struct isValidVaultId_input
    {
        uint64 vaultId;
    };
    struct isValidVaultId_output
    {
        bit result;
    };
    struct isValidVaultId_locals
    {
    };

    struct findOwnerIndexInVault_input
    {
        Vault vault;
        id ownerID;
    };
    struct findOwnerIndexInVault_output
    {
        sint64 index;
    };
    struct findOwnerIndexInVault_locals
    {
        sint64 i;
    };

    struct isOwnerOfVault_input
    {
        Vault vault;
        id ownerID;
    };
    struct isOwnerOfVault_output
    {
        bit result;
    };
    struct isOwnerOfVault_locals
    {
        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;
    };

    struct resetReleaseRequests_input
    {
        Vault vault;
    };
    struct resetReleaseRequests_output
    {
        Vault vault;
    };
    struct resetReleaseRequests_locals
    {
        uint64 i;
    };

    struct isShareHolder_input
    {
        id candidate;
    };
    struct isShareHolder_locals {};
    struct isShareHolder_output
    {
        uint64 result;
    };

    // Procedures and functions' structs
    struct registerVault_input
    {
        id vaultName;
        Array<id, MSVAULT_MAX_OWNERS> owners;
        uint64 requiredApprovals;
    };
    struct registerVault_output
    {
    };
    struct registerVault_locals
    {
        uint64 ownerCount;
        uint64 i;
        sint64 ii;
        uint64 j;
        uint64 k;
        uint64 count;
        sint64 slotIndex;
        Vault newVault;
        Vault tempVault;
        id proposedOwner;

        Array<id, MSVAULT_MAX_OWNERS> tempOwners;

        resetReleaseRequests_input rr_in;
        resetReleaseRequests_output rr_out;
        resetReleaseRequests_locals rr_locals;
    };

    struct deposit_input
    {
        uint64 vaultId;
    };
    struct deposit_output
    {
    };
    struct deposit_locals
    {
        Vault vault;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct releaseTo_input
    {
        uint64 vaultId;
        uint64 amount;
        id destination;
    };
    struct releaseTo_output
    {
    };
    struct releaseTo_locals
    {
        Vault vault;
        MSVaultLogger logger;

        sint64 ownerIndex;
        uint64 approvals;
        uint64 totalOwners;
        bit releaseApproved;
        uint64 i;

        isOwnerOfVault_input io_in;
        isOwnerOfVault_output io_out;
        isOwnerOfVault_locals io_locals;

        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;

        resetReleaseRequests_input rr_in;
        resetReleaseRequests_output rr_out;
        resetReleaseRequests_locals rr_locals;

        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct resetRelease_input
    {
        uint64 vaultId;
    };
    struct resetRelease_output
    {
    };
    struct resetRelease_locals
    {
        Vault vault;
        MSVaultLogger logger;
        sint64 ownerIndex;

        isOwnerOfVault_input io_in;
        isOwnerOfVault_output io_out;
        isOwnerOfVault_locals io_locals;

        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;

        bit found;

        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct voteFeeChange_input 
    {
        uint64 newRegisteringFee;
        uint64 newReleaseFee;
        uint64 newReleaseResetFee;
        uint64 newHoldingFee;
        uint64 newDepositFee;
        uint64 burnFee;
    };
    struct voteFeeChange_output
    {
    };
    struct voteFeeChange_locals
    {
        uint64 i;
        uint64 sumVote;
        bit needNewRecord;
        uint64 nShare;
        MsVaultFeeVote fs;

        id currentAddr;
        uint64 realScore;
        MsVaultFeeVote currentVote;
        MsVaultFeeVote uniqueVote;

        bit found;
        uint64 uniqueIndex;
        uint64 j;
        uint64 currentRank;

        isShareHolder_input ish_in;
        isShareHolder_output ish_out;
        isShareHolder_locals ish_locals;
    };

    struct getVaults_input
    {
        id publicKey;
    };
    struct getVaults_output
    {
        uint64 numberOfVaults;
        Array<uint64, MSVAULT_MAX_COOWNER> vaultIds;
        Array<id, MSVAULT_MAX_COOWNER> vaultNames;
    };
    struct getVaults_locals
    {
        uint64 count;
        uint64 i, j;
        Vault v;
    };

    struct getReleaseStatus_input
    {
        uint64 vaultId;
    };
    struct getReleaseStatus_output
    {
        uint64 status;
        Array<uint64, MSVAULT_MAX_OWNERS> amounts;
        Array<id, MSVAULT_MAX_OWNERS> destinations;
    };
    struct getReleaseStatus_locals
    {
        Vault vault;
        uint64 i;

        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct getBalanceOf_input
    {
        uint64 vaultId;
    };
    struct getBalanceOf_output
    {
        uint64 status;
        sint64 balance;
    };
    struct getBalanceOf_locals
    {
        Vault vault;

        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct getVaultName_input
    {
        uint64 vaultId;
    };
    struct getVaultName_output
    {
        uint64 status;
        id vaultName;
    };
    struct getVaultName_locals
    {
        Vault vault;

        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct getRevenueInfo_input {};
    struct getRevenueInfo_output
    {
        uint64 numberOfActiveVaults;
        uint64 totalRevenue;
        uint64 totalDistributedToShareholders;
        uint64 burnedAmount;
    };

    struct getFees_input 
    {
    };
    struct getFees_output
    {
        uint64 registeringFee;
        uint64 releaseFee;
        uint64 releaseResetFee;
        uint64 holdingFee;
        uint64 depositFee; // currently always 0
        uint64 burnFee;
    };

    struct getVaultOwners_input
    {
        uint64 vaultId;
    };
    struct getVaultOwners_locals
    {
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;

        Vault v;
        uint64 i;
    };
    struct getVaultOwners_output
    {
        uint64 status;
        uint64 numberOfOwners;
        Array<id, MSVAULT_MAX_OWNERS> owners;

        uint64 requiredApprovals;
    };

    struct END_EPOCH_locals
    {
        uint64 i;
        uint64 j;
        Vault v;
        sint64 amountToDistribute;
        uint64 feeToBurn;
    };

protected:
    // Contract states
    Array<Vault, MSVAULT_MAX_VAULTS> vaults;

    uint64 numberOfActiveVaults;
    uint64 totalRevenue;
    uint64 totalDistributedToShareholders;
    uint64 burnedAmount;

    Array<MsVaultFeeVote, MSVAULT_MAX_FEE_VOTES> feeVotes;
    Array<id, MSVAULT_MAX_FEE_VOTES> feeVotesOwner;
    Array<uint64, MSVAULT_MAX_FEE_VOTES> feeVotesScore;
    uint64 feeVotesAddrCount;

    Array<MsVaultFeeVote, MSVAULT_MAX_FEE_VOTES> uniqueFeeVotes;
    Array<uint64, MSVAULT_MAX_FEE_VOTES> uniqueFeeVotesRanking;
    uint64 uniqueFeeVotesCount;

    uint64 liveRegisteringFee;
    uint64 liveReleaseFee;
    uint64 liveReleaseResetFee;
    uint64 liveHoldingFee;
    uint64 liveDepositFee;
    uint64 liveBurnFee;

    // Helper Functions
    PRIVATE_FUNCTION_WITH_LOCALS(isValidVaultId)
    {
        if (input.vaultId < MSVAULT_MAX_VAULTS)
        {
            output.result = true;
        }
        else
        {
            output.result = false;
        }
    }

    PRIVATE_FUNCTION_WITH_LOCALS(findOwnerIndexInVault)
    {
        output.index = -1;
        for (locals.i = 0; locals.i < (sint64)input.vault.numberOfOwners; locals.i++)
        {
            if (input.vault.owners.get(locals.i) == input.ownerID)
            {
                output.index = locals.i;
                break;
            }
        }
    }

    PRIVATE_FUNCTION_WITH_LOCALS(isOwnerOfVault)
    {
        locals.fi_in.vault = input.vault;
        locals.fi_in.ownerID = input.ownerID;
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        output.result = (locals.fi_out.index != -1);
    }

    PRIVATE_FUNCTION_WITH_LOCALS(resetReleaseRequests)
    {
        for (locals.i = 0; locals.i < MSVAULT_MAX_OWNERS; locals.i++)
        {
            input.vault.releaseAmounts.set(locals.i, 0);
            input.vault.releaseDestinations.set(locals.i, NULL_ID);
        }
        output.vault = input.vault;
    }

    // Procedures and functions
    PUBLIC_PROCEDURE_WITH_LOCALS(registerVault)
    {
        // [TODO]: Change this to
        // if (qpi.invocationReward() < state.liveRegisteringFee)
        if (qpi.invocationReward() < MSVAULT_REGISTERING_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.ownerCount = 0;
        for (locals.i = 0; locals.i < MSVAULT_MAX_OWNERS; locals.i = locals.i + 1)
        {
            locals.proposedOwner = input.owners.get(locals.i);
            if (locals.proposedOwner != NULL_ID)
            {
                locals.tempOwners.set(locals.ownerCount, locals.proposedOwner);
                locals.ownerCount = locals.ownerCount + 1;
            }
        }

        if (locals.ownerCount <= 1)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (locals.ownerCount > MSVAULT_MAX_OWNERS)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        for (locals.i = locals.ownerCount; locals.i < MSVAULT_MAX_OWNERS; locals.i = locals.i + 1)
        {
            locals.tempOwners.set(locals.i, NULL_ID);
        }

        // Check if requiredApprovals is valid: must be <= numberOfOwners, > 1
        if (input.requiredApprovals <= 1 || input.requiredApprovals > locals.ownerCount)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Find empty slot
        locals.slotIndex = -1;
        for (locals.ii = 0; locals.ii < MSVAULT_MAX_VAULTS; locals.ii++)
        {
            locals.tempVault = state.vaults.get(locals.ii);
            if (!locals.tempVault.isActive && locals.tempVault.numberOfOwners == 0 && locals.tempVault.balance == 0)
            {
                locals.slotIndex = locals.ii;
                break;
            }
        }

        if (locals.slotIndex == -1)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        for (locals.i = 0; locals.i < locals.ownerCount; locals.i++)
        {
            locals.proposedOwner = locals.tempOwners.get(locals.i);
            locals.count = 0;
            for (locals.j = 0; locals.j < MSVAULT_MAX_VAULTS; locals.j++)
            {
                locals.tempVault = state.vaults.get(locals.j);
                if (locals.tempVault.isActive)
                {
                    for (locals.k = 0; locals.k < (uint64)locals.tempVault.numberOfOwners; locals.k++)
                    {
                        if (locals.tempVault.owners.get(locals.k) == locals.proposedOwner)
                        {
                            locals.count++;
                            if (locals.count >= MSVAULT_MAX_COOWNER)
                            {
                                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                                return;
                            }
                        }
                    }
                }
            }
        }

        locals.newVault.vaultName = input.vaultName;
        locals.newVault.numberOfOwners = (uint8)locals.ownerCount;
        locals.newVault.requiredApprovals = (uint8)input.requiredApprovals;
        locals.newVault.balance = 0;
        locals.newVault.isActive = true;

        locals.rr_in.vault = locals.newVault;
        resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
        locals.newVault = locals.rr_out.vault;

        for (locals.i = 0; locals.i < locals.ownerCount; locals.i++)
        {
            locals.newVault.owners.set(locals.i, locals.tempOwners.get(locals.i));
        }

        state.vaults.set((uint64)locals.slotIndex, locals.newVault);

        // [TODO]: Change this to
        //if (qpi.invocationReward() > state.liveRegisteringFee)
        //{
        //     qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.liveRegisteringFee);
        // }        
        if (qpi.invocationReward() > MSVAULT_REGISTERING_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - MSVAULT_REGISTERING_FEE);
        }

        state.numberOfActiveVaults++;

        // [TODO]: Change this to
        //state.totalRevenue += state.liveRegisteringFee;
        state.totalRevenue += MSVAULT_REGISTERING_FEE;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(deposit)
    {
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.vault = state.vaults.get(input.vaultId);
        if (!locals.vault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.vault.balance += qpi.invocationReward();
        state.vaults.set(input.vaultId, locals.vault);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(releaseTo)
    {
        // [TODO]: Change this to
        //if (qpi.invocationReward() > state.liveReleaseFee)
        //{
        //    qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.liveReleaseFee);
        //}
        if (qpi.invocationReward() > MSVAULT_RELEASE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - MSVAULT_RELEASE_FEE);
        }
        // [TODO]: Change this to
        //state.totalRevenue += state.liveReleaseFee;
        state.totalRevenue += MSVAULT_RELEASE_FEE;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger._type = 0;
        locals.logger.vaultId = input.vaultId;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = input.amount;
        locals.logger.destination = input.destination;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            locals.logger._type = 1;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultId);

        if (!locals.vault.isActive)
        {
            locals.logger._type = 1;
            LOG_INFO(locals.logger);
            return;
        }

        locals.io_in.vault = locals.vault;
        locals.io_in.ownerID = qpi.invocator();
        isOwnerOfVault(qpi, state, locals.io_in, locals.io_out, locals.io_locals);
        if (!locals.io_out.result)
        {
            locals.logger._type = 2;
            LOG_INFO(locals.logger);
            return;
        }

        if (input.amount == 0 || input.destination == NULL_ID)
        {
            locals.logger._type = 3;
            LOG_INFO(locals.logger);
            return;
        }

        if (locals.vault.balance < input.amount)
        {
            locals.logger._type = 5;
            LOG_INFO(locals.logger);
            return;
        }

        locals.fi_in.vault = locals.vault;
        locals.fi_in.ownerID = qpi.invocator();
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        locals.ownerIndex = locals.fi_out.index;

        locals.vault.releaseAmounts.set(locals.ownerIndex, input.amount);
        locals.vault.releaseDestinations.set(locals.ownerIndex, input.destination);

        locals.approvals = 0;
        locals.totalOwners = (uint64)locals.vault.numberOfOwners;
        for (locals.i = 0; locals.i < locals.totalOwners; locals.i++)
        {
            if (locals.vault.releaseAmounts.get(locals.i) == input.amount &&
                locals.vault.releaseDestinations.get(locals.i) == input.destination)
            {
                locals.approvals++;
            }
        }

        locals.releaseApproved = false;
        if (locals.approvals >= (uint64)locals.vault.requiredApprovals)
        {
            locals.releaseApproved = true;
        }

        if (locals.releaseApproved)
        {
            // Still need to re-check the balance before releasing funds
            if (locals.vault.balance >= input.amount)
            {
                qpi.transfer(input.destination, input.amount);
                locals.vault.balance -= input.amount;

                locals.rr_in.vault = locals.vault;
                resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
                locals.vault = locals.rr_out.vault;

                state.vaults.set(input.vaultId, locals.vault);

                locals.logger._type = 4;
                LOG_INFO(locals.logger);
            }
            else
            {
                locals.logger._type = 5;
                LOG_INFO(locals.logger);
            }
        }
        else
        {
            state.vaults.set(input.vaultId, locals.vault);
            locals.logger._type = 6;
            LOG_INFO(locals.logger);
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(resetRelease)
    {
        // [TODO]: Change this to
        //if (qpi.invocationReward() > state.liveReleaseResetFee)
        //{
        //    qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.liveReleaseResetFee);
        //}
        if (qpi.invocationReward() > MSVAULT_RELEASE_RESET_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - MSVAULT_RELEASE_RESET_FEE);
        }
        // [TODO]: Change this to
        //state.totalRevenue += state.liveReleaseResetFee;
        state.totalRevenue += MSVAULT_RELEASE_RESET_FEE;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger._type = 0;
        locals.logger.vaultId = input.vaultId;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = 0;
        locals.logger.destination = NULL_ID;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            locals.logger._type = 1;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultId);

        if (!locals.vault.isActive)
        {
            locals.logger._type = 1;
            LOG_INFO(locals.logger);
            return;
        }

        locals.io_in.vault = locals.vault;
        locals.io_in.ownerID = qpi.invocator();
        isOwnerOfVault(qpi, state, locals.io_in, locals.io_out, locals.io_locals);
        if (!locals.io_out.result)
        {
            locals.logger._type = 2;
            LOG_INFO(locals.logger);
            return;
        }

        locals.fi_in.vault = locals.vault;
        locals.fi_in.ownerID = qpi.invocator();
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        locals.ownerIndex = locals.fi_out.index;

        locals.vault.releaseAmounts.set(locals.ownerIndex, 0);
        locals.vault.releaseDestinations.set(locals.ownerIndex, NULL_ID);

        state.vaults.set(input.vaultId, locals.vault);

        locals.logger._type = 7;
        LOG_INFO(locals.logger);
    }

    // [TODO]: Uncomment this to enable live fee update
    PUBLIC_PROCEDURE_WITH_LOCALS(voteFeeChange)
    {
        return;
    //    locals.ish_in.candidate = qpi.invocator();
    //    isShareHolder(qpi, state, locals.ish_in, locals.ish_out, locals.ish_locals);
    //    if (!locals.ish_out.result)
    //    {
    //        return;
    //    }
    //
    //    qpi.transfer(qpi.invocator(), qpi.invocationReward());
    //    locals.nShare = qpi.numberOfPossessedShares(MSVAULT_ASSET_NAME, id::zero(), qpi.invocator(), qpi.invocator(), MSVAULT_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX);
    //
    //    locals.fs.registeringFee = input.newRegisteringFee;
    //    locals.fs.releaseFee = input.newReleaseFee;
    //    locals.fs.releaseResetFee = input.newReleaseResetFee;
    //    locals.fs.holdingFee = input.newHoldingFee;
    //    locals.fs.depositFee = input.newDepositFee;
    //    // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
    //    //locals.fs.burnFee = input.burnFee;
    //
    //    locals.needNewRecord = true;
    //    for (locals.i = 0; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
    //    {
    //        locals.currentAddr = state.feeVotesOwner.get(locals.i);
    //        locals.realScore = qpi.numberOfPossessedShares(MSVAULT_ASSET_NAME, id::zero(), locals.currentAddr, locals.currentAddr, MSVAULT_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX);
    //        state.feeVotesScore.set(locals.i, locals.realScore);
    //        if (locals.currentAddr == qpi.invocator())
    //        {
    //            locals.needNewRecord = false;
    //        }
    //    }
    //    if (locals.needNewRecord)
    //    {
    //        state.feeVotes.set(state.feeVotesAddrCount, locals.fs);
    //        state.feeVotesOwner.set(state.feeVotesAddrCount, qpi.invocator());
    //        state.feeVotesScore.set(state.feeVotesAddrCount, locals.nShare);
    //        state.feeVotesAddrCount = state.feeVotesAddrCount + 1;
    //    }
    //
    //    locals.sumVote = 0;
    //    for (locals.i = 0; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
    //    {
    //        locals.sumVote = locals.sumVote + state.feeVotesScore.get(locals.i);
    //    }
    //    if (locals.sumVote < QUORUM)
    //    {
    //        return;
    //    }
    //
    //    state.uniqueFeeVotesCount = 0;
    //    for (locals.i = 0; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
    //    {
    //        locals.currentVote = state.feeVotes.get(locals.i);
    //        locals.found = false;
    //        locals.uniqueIndex = 0;
    //        locals.j;
    //        for (locals.j = 0; locals.j < state.uniqueFeeVotesCount; locals.j = locals.j + 1)
    //        {
    //            locals.uniqueVote = state.uniqueFeeVotes.get(locals.j);
    //            if (locals.uniqueVote.registeringFee == locals.currentVote.registeringFee &&
    //                locals.uniqueVote.releaseFee == locals.currentVote.releaseFee &&
    //                locals.uniqueVote.releaseResetFee == locals.currentVote.releaseResetFee &&
    //                locals.uniqueVote.holdingFee == locals.currentVote.holdingFee &&
    //                locals.uniqueVote.depositFee == locals.currentVote.depositFee
    //                // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
    //                //&& locals.uniqueVote.burnFee == locals.currentVote.burnFee
    //                )
    //            {
    //                locals.found = true;
    //                locals.uniqueIndex = locals.j;
    //                break;
    //            }
    //        }
    //        if (locals.found)
    //        {
    //            locals.currentRank = state.uniqueFeeVotesRanking.get(locals.uniqueIndex);
    //            state.uniqueFeeVotesRanking.set(locals.uniqueIndex, locals.currentRank + state.feeVotesScore.get(locals.i));
    //        }
    //        else
    //        {
    //            state.uniqueFeeVotes.set(state.uniqueFeeVotesCount, locals.currentVote);
    //            state.uniqueFeeVotesRanking.set(state.uniqueFeeVotesCount, state.feeVotesScore.get(locals.i));
    //            state.uniqueFeeVotesCount = state.uniqueFeeVotesCount + 1;
    //        }
    //    }
    //
    //    for (locals.i = 0; locals.i < state.uniqueFeeVotesCount; locals.i = locals.i + 1)
    //    {
    //        if (state.uniqueFeeVotesRanking.get(locals.i) >= QUORUM)
    //        {
    //            state.liveRegisteringFee = state.uniqueFeeVotes.get(locals.i).registeringFee;
    //            state.liveReleaseFee = state.uniqueFeeVotes.get(locals.i).releaseFee;
    //            state.liveReleaseResetFee = state.uniqueFeeVotes.get(locals.i).releaseResetFee;
    //            state.liveHoldingFee = state.uniqueFeeVotes.get(locals.i).holdingFee;
    //            state.liveDepositFee = state.uniqueFeeVotes.get(locals.i).depositFee;
    //            // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
    //            //state.liveBurnFee = state.uniqueFeeVotes.get(locals.i).burnFee;

    //            state.feeVotesAddrCount = 0;
    //            state.uniqueFeeVotesCount = 0;
    //            return;
    //        }
    //    }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getVaults)
    {
        output.numberOfVaults = 0ULL;
        locals.count = 0ULL;
        for (locals.i = 0ULL; locals.i < MSVAULT_MAX_VAULTS; locals.i++)
        {
            locals.v = state.vaults.get(locals.i);
            if (locals.v.isActive)
            {
                for (locals.j = 0ULL; locals.j < (uint64)locals.v.numberOfOwners; locals.j++)
                {
                    if (locals.v.owners.get(locals.j) == input.publicKey)
                    {
                        output.vaultIds.set(locals.count, locals.i);
                        output.vaultNames.set(locals.count, locals.v.vaultName);
                        locals.count++;
                        break;
                    }
                }
            }
        }
        output.numberOfVaults = locals.count;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getReleaseStatus)
    {
        output.status = 0ULL;
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            return; // output.status = false
        }

        locals.vault = state.vaults.get(input.vaultId);
        if (!locals.vault.isActive)
        {
            return; // output.status = false
        }

        for (locals.i = 0; locals.i < (uint64)locals.vault.numberOfOwners; locals.i++)
        {
            output.amounts.set(locals.i, locals.vault.releaseAmounts.get(locals.i));
            output.destinations.set(locals.i, locals.vault.releaseDestinations.get(locals.i));
        }
        output.status = 1ULL;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getBalanceOf)
    {
        output.status = 0ULL;
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            return; // output.status = false
        }

        locals.vault = state.vaults.get(input.vaultId);
        if (!locals.vault.isActive)
        {
            return; // output.status = false
        }
        output.balance = locals.vault.balance;
        output.status = 1ULL;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getVaultName)
    {
        output.status = 0ULL;
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            return; // output.status = false
        }

        locals.vault = state.vaults.get(input.vaultId);
        if (!locals.vault.isActive)
        {
            return; // output.status = false
        }
        output.vaultName = locals.vault.vaultName;
        output.status = 1ULL;
    }

    PUBLIC_FUNCTION(getRevenueInfo)
    {
        output.numberOfActiveVaults = state.numberOfActiveVaults;
        output.totalRevenue = state.totalRevenue;
        output.totalDistributedToShareholders = state.totalDistributedToShareholders;
        // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
        //output.burnedAmount = state.burnedAmount;
    }

    PUBLIC_FUNCTION(getFees)
    {
        output.registeringFee = MSVAULT_REGISTERING_FEE;
        output.releaseFee = MSVAULT_RELEASE_FEE;
        output.releaseResetFee = MSVAULT_RELEASE_RESET_FEE;
        output.holdingFee = MSVAULT_HOLDING_FEE;
        output.depositFee = 0ULL;
        // [TODO]: Change this to:
        //output.registeringFee = state.liveRegisteringFee;
        //output.releaseFee = state.liveReleaseFee;
        //output.releaseResetFee = state.liveReleaseResetFee;
        //output.holdingFee = state.liveHoldingFee;
        //output.depositFee = state.liveDepositFee;
        // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
        //output.burnFee = state.liveBurnFee;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getVaultOwners)
    {
        output.status = 0ULL;
        output.numberOfOwners = 0;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);
        if (!locals.iv_out.result)
        {
            return;
        }

        locals.v = state.vaults.get(input.vaultId);

        if (!locals.v.isActive)
        {
            return;
        }

        output.numberOfOwners = (uint64)locals.v.numberOfOwners;

        for (locals.i = 0; locals.i < MSVAULT_MAX_OWNERS; locals.i++)
        {
            output.owners.set(locals.i, locals.v.owners.get(locals.i));
        }

        output.requiredApprovals = (uint64)locals.v.requiredApprovals;

        output.status = 1ULL;
    }
    
    // [TODO]: Uncomment this to enable live fee update
    PUBLIC_FUNCTION_WITH_LOCALS(isShareHolder)
    {
    //    if (qpi.numberOfPossessedShares(MSVAULT_ASSET_NAME, id::zero(), input.candidate, input.candidate, MSVAULT_CONTRACT_INDEX, MSVAULT_CONTRACT_INDEX) > 0)
    //    {
    //        output.result = 1ULL;
    //    }
    //    else
    //    {
    //        output.result = 0ULL;
    //    }
    }

    INITIALIZE()
    {
        state.numberOfActiveVaults = 0ULL;
        state.totalRevenue = 0ULL;
        state.totalDistributedToShareholders = 0ULL;
        state.burnedAmount = 0ULL;
        state.liveBurnFee = MSVAULT_BURN_FEE;
        state.liveRegisteringFee = MSVAULT_REGISTERING_FEE;
        state.liveReleaseFee = MSVAULT_RELEASE_FEE;
        state.liveReleaseResetFee = MSVAULT_RELEASE_RESET_FEE;
        state.liveHoldingFee = MSVAULT_HOLDING_FEE;
        state.liveDepositFee = 0ULL;
    }

    END_EPOCH_WITH_LOCALS()
    {
        for (locals.i = 0ULL; locals.i < MSVAULT_MAX_VAULTS; locals.i++)
        {
            locals.v = state.vaults.get(locals.i);
            if (locals.v.isActive)
            {
                // [TODO]: Change this to
                //if (locals.v.balance >= state.liveHoldingFee)
                //{
                //    locals.v.balance -= state.liveHoldingFee;
                //    state.totalRevenue += state.liveHoldingFee;
                //    state.vaults.set(locals.i, locals.v);
                //}
                if (locals.v.balance >= MSVAULT_HOLDING_FEE)
                {
                    locals.v.balance -= MSVAULT_HOLDING_FEE;
                    state.totalRevenue += MSVAULT_HOLDING_FEE;
                    state.vaults.set(locals.i, locals.v);
                }
                else
                {
                    // Not enough funds to pay holding fee
                    if (locals.v.balance > 0)
                    {
                        state.totalRevenue += locals.v.balance;
                    }
                    locals.v.isActive = false;
                    locals.v.balance = 0;
                    locals.v.requiredApprovals = 0;
                    locals.v.vaultName = NULL_ID;
                    locals.v.numberOfOwners = 0;
                    for (locals.j = 0; locals.j < MSVAULT_MAX_OWNERS; locals.j++)
                    {
                        locals.v.owners.set(locals.j, NULL_ID);
                        locals.v.releaseAmounts.set(locals.j, 0);
                        locals.v.releaseDestinations.set(locals.j, NULL_ID);
                    }
                    if (state.numberOfActiveVaults > 0)
                    {
                        state.numberOfActiveVaults--;
                    }
                    state.vaults.set(locals.i, locals.v);
                }
            }
        }

        {
            locals.amountToDistribute = QPI::div<uint64>(state.totalRevenue - state.totalDistributedToShareholders, NUMBER_OF_COMPUTORS);

            // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
            //// Burn fee
            //locals.feeToBurn = QPI::div<uint64>(locals.amountToDistribute * state.liveBurnFee, 100ULL);
            //if (locals.feeToBurn > 0)
            //{
            //    qpi.burn(locals.feeToBurn);
            //}
            //locals.amountToDistribute -= locals.feeToBurn;
            //state.burnedAmount += locals.feeToBurn;

            if (locals.amountToDistribute > 0 && state.totalRevenue > state.totalDistributedToShareholders)
            {
                if (qpi.distributeDividends(locals.amountToDistribute))
                {
                    state.totalDistributedToShareholders += locals.amountToDistribute * NUMBER_OF_COMPUTORS;
                }
            }
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(registerVault, 1);
        REGISTER_USER_PROCEDURE(deposit, 2);
        REGISTER_USER_PROCEDURE(releaseTo, 3);
        REGISTER_USER_PROCEDURE(resetRelease, 4);
        REGISTER_USER_FUNCTION(getVaults, 5);
        REGISTER_USER_FUNCTION(getReleaseStatus, 6);
        REGISTER_USER_FUNCTION(getBalanceOf, 7);
        REGISTER_USER_FUNCTION(getVaultName, 8);
        REGISTER_USER_FUNCTION(getRevenueInfo, 9);
        REGISTER_USER_FUNCTION(getFees, 10);
        REGISTER_USER_FUNCTION(getVaultOwners, 11);
        REGISTER_USER_FUNCTION(isShareHolder, 12);
        REGISTER_USER_PROCEDURE(voteFeeChange, 13);
    }
};
