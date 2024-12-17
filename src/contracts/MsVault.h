using namespace QPI;

constexpr uint16 MAX_OWNERS = 32;
constexpr uint16 INITIAL_MAX_VAULTS = 1024;
constexpr uint16 MAX_VAULTS = INITIAL_MAX_VAULTS * X_MULTIPLIER;

constexpr uint64 REGISTERING_FEE = 10000ULL;
constexpr uint64 RELEASE_FEE = 1000ULL;
constexpr uint64 RELEASE_RESET_FEE = 500ULL;
constexpr uint64 HOLDING_FEE = 100ULL;

constexpr uint16 VAULT_TYPE_QUORUM = 1;
constexpr uint16 VAULT_TYPE_TWO_OUT_OF_X = 2;

struct MSVAULT2
{
};

struct MSVAULT : public ContractBase
{
    struct Vault
    {
        uint16 vaultType;
        id vaultName;
        array<id, MAX_OWNERS> owners;
        uint16 numberOfOwners;
        sint64 balance;
        bit isActive;
        array<uint64, MAX_OWNERS> releaseAmounts;
        array<id, MAX_OWNERS> releaseDestinations;
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
        uint64 vaultID; 
        id ownerID;
        uint64 amount;
        id destination;
        sint8 _terminator;
    };

    array<Vault, MAX_VAULTS> vaults;

    uint32 numberOfActiveVaults;
    uint64 totalRevenue;
    uint64 totalDistributedToShareholders;

    struct findOwnerIndexInVault_input
    {
        Vault vault;
        id ownerID;
    };
    struct findOwnerIndexInVault_output
    {
        sint32 index;
    };
    struct findOwnerIndexInVault_locals
    {
        sint32 i;
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
        sint32 i;
    };

    struct registerVault_input
    {
        uint16 vaultType;
        id vaultName;
        array<id, MAX_OWNERS> owners;
    };
    struct registerVault_output
    {
    };
    struct registerVault_locals
    {
        uint16 ownerCount;
        uint16 i, j;
        sint32 slotIndex;
        Vault newVault;
        Vault tempVault;

        resetReleaseRequests_input rr_in;
        resetReleaseRequests_output rr_out;
        resetReleaseRequests_locals rr_locals;
    };

    struct deposit_input
    {
        uint64 vaultID;
    };
    struct deposit_output
    {
    };
    struct deposit_locals
    {
        Vault vault;
    };

    struct releaseTo_input
    {
        uint64 vaultID;
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

        sint32 ownerIndex;
        uint16 approvals;
        uint16 totalOwners;
        bit releaseApproved;
        uint16 requiredApprovals;
        uint16 i;

        uint64 calc;
        uint64 divResult;

        isOwnerOfVault_input io_in;
        isOwnerOfVault_output io_out;
        isOwnerOfVault_locals io_locals;

        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;

        resetReleaseRequests_input rr_in;
        resetReleaseRequests_output rr_out;
        resetReleaseRequests_locals rr_locals;
    };

    struct resetRelease_input
    {
        uint64 vaultID;
    };
    struct resetRelease_output
    {
    };
    struct resetRelease_locals
    {
        Vault vault;
        MSVaultLogger logger;
        sint32 ownerIndex;

        isOwnerOfVault_input io_in;
        isOwnerOfVault_output io_out;
        isOwnerOfVault_locals io_locals;

        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;

        bit found;
    };

    struct getVaults_input
    {
        id publicKey;
    };
    struct getVaults_output
    {
        uint16 numberOfVaults;
        array<uint64, 1024> vaultIDs;
        array<id, 1024> vaultNames;
    };
    struct getVaults_locals
    {
        uint16 count;
        uint16 i, j;
        Vault v;
    };

    struct getReleaseStatus_input
    {
        uint64 vaultID;
    };
    struct getReleaseStatus_output
    {
        array<uint64, MAX_OWNERS> amounts;
        array<id, MAX_OWNERS> destinations;
    };
    struct getReleaseStatus_locals
    {
        Vault vault;
        uint16 i;
    };

    struct getBalanceOf_input
    {
        uint64 vaultID;
    };
    struct getBalanceOf_output
    {
        sint64 balance;
    };
    struct getBalanceOf_locals
    {
        Vault vault;
    };

    struct getVaultName_input
    {
        uint64 vaultID;
    };
    struct getVaultName_output
    {
        id vaultName;
    };
    struct getVaultName_locals
    {
        Vault vault;
    };

    struct getRevenueInfo_input {};
    struct getRevenueInfo_output
    {
        uint32 numberOfActiveVaults;
        uint64 totalRevenue;
        uint64 totalBurned;
        uint64 totalDistributedToShareholders;
    };

    // Helper Functions
    PRIVATE_FUNCTION_WITH_LOCALS(findOwnerIndexInVault)
        output.index = -1;
        for (locals.i = 0; locals.i < input.vault.numberOfOwners; locals.i++)
        {
            if (input.vault.owners.get(locals.i) == input.ownerID)
            {
                output.index = locals.i;
                break;
            }
        }
    _

    PRIVATE_FUNCTION_WITH_LOCALS(isOwnerOfVault)
        locals.fi_in.vault = input.vault;
        locals.fi_in.ownerID = input.ownerID;
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        output.result = (locals.fi_out.index != -1);
    _

    PRIVATE_FUNCTION_WITH_LOCALS(resetReleaseRequests)
        for (locals.i = 0; locals.i < MAX_OWNERS; locals.i++)
        {
            input.vault.releaseAmounts.set(locals.i, 0);
            input.vault.releaseDestinations.set(locals.i, NULL_ID);
        }
        output.vault = input.vault;
    _

    // Procedures and functions
    PUBLIC_PROCEDURE_WITH_LOCALS(registerVault)
        if (qpi.invocationReward() < REGISTERING_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.ownerCount = 0;
        for (locals.i = 0; locals.i < MAX_OWNERS; locals.i++)
        {
            if (input.owners.get(locals.i) != NULL_ID)
                locals.ownerCount++;
        }

        if (locals.ownerCount <= 1)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Find empty slot
        locals.slotIndex = -1;
        for (locals.i = 0; locals.i < MAX_VAULTS; locals.i++)
        {
            locals.tempVault = state.vaults.get(locals.i);
            if (!locals.tempVault.isActive && locals.tempVault.numberOfOwners == 0 && locals.tempVault.balance == 0)
            {
                locals.slotIndex = (sint32)locals.i;
                break;
            }
        }

        if (locals.slotIndex == -1)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.newVault.vaultType = input.vaultType;
        locals.newVault.vaultName = input.vaultName;
        locals.newVault.numberOfOwners = locals.ownerCount;
        locals.newVault.balance = 0;
        locals.newVault.isActive = true;

        locals.rr_in.vault = locals.newVault;
        resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
        locals.newVault = locals.rr_out.vault;

        for (locals.i = 0; locals.i < locals.ownerCount; locals.i++)
        {
            locals.newVault.owners.set(locals.i, input.owners.get(locals.i));
        }

        state.vaults.set((uint64)locals.slotIndex, locals.newVault);

        //qpi.burn(REGISTERING_FEE);
        if (qpi.invocationReward() > REGISTERING_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - REGISTERING_FEE);
        }

        state.numberOfActiveVaults++;
        state.totalRevenue += REGISTERING_FEE;
    _

    PUBLIC_PROCEDURE_WITH_LOCALS(deposit)
        if (input.vaultID >= MAX_VAULTS)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.vault = state.vaults.get(input.vaultID);
        if (!locals.vault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.vault.balance += qpi.invocationReward();
        state.vaults.set(input.vaultID, locals.vault);
    _

    PUBLIC_PROCEDURE_WITH_LOCALS(releaseTo)
        if (qpi.invocationReward() > RELEASE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - RELEASE_FEE);
        }
        state.totalRevenue += RELEASE_FEE;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger._type = 0;
        locals.logger.vaultID = input.vaultID;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = input.amount;
        locals.logger.destination = input.destination;

        if (input.vaultID >= MAX_VAULTS)
        {
            locals.logger._type = 1;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultID);

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

        locals.fi_in.vault = locals.vault;
        locals.fi_in.ownerID = qpi.invocator();
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        locals.ownerIndex = locals.fi_out.index;

        locals.vault.releaseAmounts.set(locals.ownerIndex, input.amount);
        locals.vault.releaseDestinations.set(locals.ownerIndex, input.destination);

        locals.approvals = 0;
        locals.totalOwners = locals.vault.numberOfOwners;
        for (locals.i = 0; locals.i < locals.totalOwners; locals.i++)
        {
            if (locals.vault.releaseAmounts.get(locals.i) == input.amount &&
                locals.vault.releaseDestinations.get(locals.i) == input.destination)
            {
                locals.approvals++;
            }
        }

        locals.releaseApproved = false;
        if (locals.vault.vaultType == VAULT_TYPE_QUORUM)
        {
            locals.calc = ((uint64)locals.totalOwners * 2ULL) + 2ULL;
            locals.divResult = QPI::div(locals.calc, 3ULL);
            locals.requiredApprovals = (uint16)locals.divResult;

            if (locals.approvals >= locals.requiredApprovals)
            {
                locals.releaseApproved = true;
            }
        }
        else if (locals.vault.vaultType == VAULT_TYPE_TWO_OUT_OF_X)
        {
            if (locals.approvals >= 2)
            {
                locals.releaseApproved = true;
            }
        }

        if (locals.releaseApproved)
        {
            if (locals.vault.balance >= (sint64)input.amount)
            {
                qpi.transfer(input.destination, input.amount);
                locals.vault.balance -= (sint64)input.amount;

                locals.rr_in.vault = locals.vault;
                resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
                locals.vault = locals.rr_out.vault;

                state.vaults.set(input.vaultID, locals.vault);

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
            state.vaults.set(input.vaultID, locals.vault);
            locals.logger._type = 6;
            LOG_INFO(locals.logger);
        }
    _

    PUBLIC_PROCEDURE_WITH_LOCALS(resetRelease)
        if (qpi.invocationReward() > RELEASE_RESET_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - RELEASE_RESET_FEE);
        }
        state.totalRevenue += RELEASE_RESET_FEE;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger._type = 0;
        locals.logger.vaultID = input.vaultID;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = 0;
        locals.logger.destination = NULL_ID;

        if (input.vaultID >= MAX_VAULTS)
        {
            locals.logger._type = 1;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultID);

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

        state.vaults.set(input.vaultID, locals.vault);

        locals.logger._type = 7;
        LOG_INFO(locals.logger);
    _

    PUBLIC_FUNCTION_WITH_LOCALS(getVaults)
        output.numberOfVaults = 0;
        locals.count = 0;
        for (locals.i = 0; locals.i < 1024; locals.i++)
        {
            locals.v = state.vaults.get(locals.i);
            if (locals.v.isActive)
            {
                for (locals.j = 0; locals.j < locals.v.numberOfOwners; locals.j++)
                {
                    if (locals.v.owners.get(locals.j) == input.publicKey)
                    {
                        output.vaultIDs.set(locals.count, (uint64)locals.i);
                        output.vaultNames.set(locals.count, locals.v.vaultName);
                        locals.count++;
                        break;
                    }
                }
            }
        }
        output.numberOfVaults = locals.count;
    _

    PUBLIC_FUNCTION_WITH_LOCALS(getReleaseStatus)
        if (input.vaultID >= MAX_VAULTS)
        {
            for (locals.i = 0; locals.i < MAX_OWNERS; locals.i++)
            {
                output.amounts.set(locals.i, 0);
                output.destinations.set(locals.i, NULL_ID);
            }
            return;
        }

        locals.vault = state.vaults.get(input.vaultID);
        if (!locals.vault.isActive)
        {
            for (locals.i = 0; locals.i < MAX_OWNERS; locals.i++)
            {
                output.amounts.set(locals.i, 0);
                output.destinations.set(locals.i, NULL_ID);
            }
            return;
        }

        for (locals.i = 0; locals.i < locals.vault.numberOfOwners; locals.i++)
        {
            output.amounts.set(locals.i, locals.vault.releaseAmounts.get(locals.i));
            output.destinations.set(locals.i, locals.vault.releaseDestinations.get(locals.i));
        }
    _

    PUBLIC_FUNCTION_WITH_LOCALS(getBalanceOf)
        if (input.vaultID >= MAX_VAULTS)
        {
            output.balance = 0;
            return;
        }

        locals.vault = state.vaults.get(input.vaultID);
        if (!locals.vault.isActive)
        {
            output.balance = 0;
            return;
        }
        output.balance = locals.vault.balance;
    _

    PUBLIC_FUNCTION_WITH_LOCALS(getVaultName)
        if (input.vaultID >= MAX_VAULTS)
        {
            output.vaultName = NULL_ID;
            return;
        }

        locals.vault = state.vaults.get(input.vaultID);
        if (!locals.vault.isActive)
        {
            output.vaultName = NULL_ID;
            return;
        }
        output.vaultName = locals.vault.vaultName;
    _

    PUBLIC_FUNCTION(getRevenueInfo)
        output.numberOfActiveVaults = state.numberOfActiveVaults;
        output.totalRevenue = state.totalRevenue;
        output.totalDistributedToShareholders = state.totalDistributedToShareholders;
    _

    INITIALIZE
        state.numberOfActiveVaults = 0;
        state.totalRevenue = 0;
        state.totalDistributedToShareholders = 0;
    _

    struct BEGIN_EPOCH_locals
    {
        uint64 i;
        resetReleaseRequests_input rr_in;
        resetReleaseRequests_output rr_out;
        resetReleaseRequests_locals rr_locals;
        Vault v;
    };

    BEGIN_EPOCH_WITH_LOCALS
        for (locals.i = 0; locals.i < MAX_VAULTS; locals.i++)
        {
            locals.v = state.vaults.get(locals.i);
            if (locals.v.isActive)
            {
                locals.rr_in.vault = locals.v;
                resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
                locals.v = locals.rr_out.vault;
                state.vaults.set(locals.i, locals.v);
            }
        }
    _

    struct END_EPOCH_locals
    {
        uint64 i;
        Vault v;
    };

    END_EPOCH_WITH_LOCALS
        for (locals.i = 0; locals.i < MAX_VAULTS; locals.i++)
        {
            locals.v = state.vaults.get(locals.i);
            if (locals.v.isActive)
            {
                if (locals.v.balance >= (sint64)HOLDING_FEE)
                {
                    // pay the holding fee
                    locals.v.balance -= (sint64)HOLDING_FEE;
                    state.totalRevenue += HOLDING_FEE;
                    state.vaults.set(locals.i, locals.v);
                }
                else
                {
                    // ,ot enough funds to pay holding fee
                    locals.v.isActive = false;
                    state.numberOfActiveVaults--;
                    state.vaults.set(locals.i, locals.v);
                }
            }
        }

        {
            sint64 amountToDistribute = QPI::div(state.totalRevenue, (uint64)NUMBER_OF_COMPUTORS);
            if (amountToDistribute > 0)
            {
                if (qpi.distributeDividends(amountToDistribute))
                {
                    state.totalDistributedToShareholders += amountToDistribute * NUMBER_OF_COMPUTORS;
                }
            }
        }
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(registerVault, 1);
        REGISTER_USER_PROCEDURE(deposit, 2);
        REGISTER_USER_PROCEDURE(releaseTo, 3);
        REGISTER_USER_PROCEDURE(resetRelease, 4);
        REGISTER_USER_FUNCTION(getVaults, 5);
        REGISTER_USER_FUNCTION(getReleaseStatus, 6);
        REGISTER_USER_FUNCTION(getBalanceOf, 7);
        REGISTER_USER_FUNCTION(getVaultName, 8);
        REGISTER_USER_FUNCTION(getRevenueInfo, 9);
    _
};
