using namespace QPI;

constexpr uint64 MSVAULT_MAX_OWNERS = 16;
constexpr uint64 MSVAULT_MAX_COOWNER = 8;
constexpr uint64 MSVAULT_INITIAL_MAX_VAULTS = 131072ULL; // 2^17
constexpr uint64 MSVAULT_MAX_VAULTS = MSVAULT_INITIAL_MAX_VAULTS * X_MULTIPLIER;
// MSVAULT asset name : 23727827095802701, using assetNameFromString("MSVAULT") utility in test_util.h
static constexpr uint64 MSVAULT_ASSET_NAME = 23727827095802701;
constexpr uint64 MSVAULT_MAX_ASSET_TYPES = 8; // Max number of different asset types a vault can hold

constexpr uint64 MSVAULT_REGISTERING_FEE = 5000000ULL;
constexpr uint64 MSVAULT_RELEASE_FEE = 100000ULL;
constexpr uint64 MSVAULT_RELEASE_RESET_FEE = 1000000ULL;
constexpr uint64 MSVAULT_HOLDING_FEE = 500000ULL;
constexpr uint64 MSVAULT_BURN_FEE = 0ULL; // Integer percentage from 1 -> 100
constexpr uint64 MSVAULT_VOTE_FEE_CHANGE_FEE = 10000000ULL; // Deposit fee for adjusting other fees, and refund if shareholders
constexpr uint64 MSVAULT_REVOKE_FEE = 100ULL;
// [TODO]: Turn this assert ON when MSVAULT_BURN_FEE > 0
//static_assert(MSVAULT_BURN_FEE > 0, "SC requires burning qu to operate, the burn fee must be higher than 0!");

static constexpr uint64 MSVAULT_MAX_FEE_VOTES = 64;


struct MSVAULT2
{
};

struct MSVAULT : public ContractBase
{
public:
    // Procedure Status Codes ---
    // 0: GENEREAL_FAILURE
    // 1: SUCCESS
    // 2: FAILURE_INSUFFICIENT_FEE
    // 3: FAILURE_INVALID_VAULT (Invalid vault ID or vault inactive)
    // 4: FAILURE_NOT_AUTHORIZED (not owner, not shareholder, etc.)
    // 5: FAILURE_INVALID_PARAMS (amount is 0, destination is NULL, etc.)
    // 6: FAILURE_INSUFFICIENT_BALANCE
    // 7: FAILURE_LIMIT_REACHED (max vaults, max asset types, etc.)
    // 8: FAILURE_TRANSFER_FAILED
    // 9: PENDING_APPROVAL

    struct AssetBalance
    {
        Asset asset;
        uint64 balance;
    };

    struct Vault
    {
        id vaultName;
        Array<id, MSVAULT_MAX_OWNERS> owners;
        Array<uint64, MSVAULT_MAX_OWNERS> releaseAmounts;
        Array<id, MSVAULT_MAX_OWNERS> releaseDestinations;
        uint64 qubicBalance;
        uint8 numberOfOwners;
        uint8 requiredApprovals;
        bit isActive;
    };

    struct VaultAssetPart
    {
        Array<AssetBalance, MSVAULT_MAX_ASSET_TYPES> assetBalances;
        uint8 numberOfAssetTypes;
        Array<Asset, MSVAULT_MAX_OWNERS> releaseAssets;
        Array<uint64, MSVAULT_MAX_OWNERS> releaseAssetAmounts;
        Array<id, MSVAULT_MAX_OWNERS> releaseAssetDestinations;
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
        // _type corresponds to Procedure Status Codes
        // 0: GENEREAL_FAILURE
        // 1: SUCCESS
        // 2: FAILURE_INSUFFICIENT_FEE
        // 3: FAILURE_INVALID_VAULT
        // 4: FAILURE_NOT_AUTHORIZED
        // 5: FAILURE_INVALID_PARAMS
        // 6: FAILURE_INSUFFICIENT_BALANCE
        // 7: FAILURE_LIMIT_REACHED
        // 8: FAILURE_TRANSFER_FAILED
        // 9: PENDING_APPROVAL
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

    struct getManagedAssetBalance_input
    {
        Asset asset;
        id owner;
    };
    struct getManagedAssetBalance_output
    {
        sint64 balance;
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
        uint64 status;
    };
    struct registerVault_locals
    {
        MSVaultLogger logger;
        uint64 ownerCount;
        uint64 i;
        sint64 ii;
        uint64 j;
        uint64 k;
        uint64 count;
        uint64 found;
        sint64 slotIndex;
        Vault newVault;
        Vault tempVault;
        id proposedOwner;
        Vault newQubicVault;
        VaultAssetPart newAssetVault;
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
        uint64 status;
    };
    struct deposit_locals
    {
        MSVaultLogger logger;
        Vault vault;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
        uint64 amountToDeposit;
    };

    struct releaseTo_input
    {
        uint64 vaultId;
        uint64 amount;
        id destination;
    };
    struct releaseTo_output
    {
        uint64 status;
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
        uint64 status;
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
        uint64 status;
    };
    struct voteFeeChange_locals
    {
        MSVaultLogger logger;
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

    struct getFeeVotes_input
    {
    };
    struct getFeeVotes_output
    {
        uint64 status;
        uint64 numberOfFeeVotes;
        Array<MsVaultFeeVote, MSVAULT_MAX_FEE_VOTES> feeVotes;
    };
    struct getFeeVotes_locals
    {
        uint64 i;
    };

    struct getFeeVotesOwner_input
    {
    };
    struct getFeeVotesOwner_output
    {
        uint64 status;
        uint64 numberOfFeeVotes;
        Array<id, MSVAULT_MAX_FEE_VOTES> feeVotesOwner;
    };
    struct getFeeVotesOwner_locals
    {
        uint64 i;
    };

    struct getFeeVotesScore_input
    {
    };
    struct getFeeVotesScore_output
    {
        uint64 status;
        uint64 numberOfFeeVotes;
        Array<uint64, MSVAULT_MAX_FEE_VOTES> feeVotesScore;
    };
    struct getFeeVotesScore_locals
    {
        uint64 i;
    };

    struct getUniqueFeeVotes_input
    {
    };
    struct getUniqueFeeVotes_output
    {
        uint64 status;
        uint64 numberOfUniqueFeeVotes;
        Array<MsVaultFeeVote, MSVAULT_MAX_FEE_VOTES> uniqueFeeVotes;
    };
    struct getUniqueFeeVotes_locals
    {
        uint64 i;
    };

    struct getUniqueFeeVotesRanking_input
    {
    };
    struct getUniqueFeeVotesRanking_output
    {
        uint64 status;
        uint64 numberOfUniqueFeeVotes;
        Array<uint64, MSVAULT_MAX_FEE_VOTES> uniqueFeeVotesRanking;
    };
    struct getUniqueFeeVotesRanking_locals
    {
        uint64 i;
    };

    struct depositAsset_input
    {
        uint64 vaultId;
        Asset asset;
        uint64 amount;
    };
    struct depositAsset_output
    {
        uint64 status;
    };
    struct depositAsset_locals
    {
        MSVaultLogger logger;
        Vault qubicVault; // Object for the Qubic-related part of the vault
        VaultAssetPart assetVault; // Object for the Asset-related part of the vault
        AssetBalance ab;
        sint64 assetIndex;
        uint64 i;
        sint64 userAssetBalance;
        sint64 tempShares;
        sint64 transferResult;
        sint64 transferedShares;
        QX::TransferShareOwnershipAndPossession_input qx_in;
        QX::TransferShareOwnershipAndPossession_output qx_out;
        sint64 transferredNumberOfShares;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct revokeAssetManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
    };
    struct revokeAssetManagementRights_output
    {
        sint64 transferredNumberOfShares;
        uint64 status;
    };
    struct revokeAssetManagementRights_locals
    {
        MSVaultLogger logger;
        sint64 managedBalance;
        sint64 result;
    };

    struct releaseAssetTo_input
    {
        uint64 vaultId;
        Asset asset;
        uint64 amount;
        id destination;
    };
    struct releaseAssetTo_output
    {
        uint64 status;
    };
    struct releaseAssetTo_locals
    {
        Vault qubicVault;
        VaultAssetPart assetVault;
        MSVaultLogger logger;
        sint64 ownerIndex;
        uint64 approvals;
        bit releaseApproved;
        AssetBalance ab;
        uint64 i;
        sint64 assetIndex;
        isOwnerOfVault_input io_in;
        isOwnerOfVault_output io_out;
        isOwnerOfVault_locals io_locals;
        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
        sint64 remainingShares;
        sint64 releaseResult;
    };

    struct resetAssetRelease_input
    {
        uint64 vaultId;
    };
    struct resetAssetRelease_output
    {
        uint64 status;
    };
    struct resetAssetRelease_locals
    {
        Vault qubicVault;
        VaultAssetPart assetVault;
        sint64 ownerIndex;
        MSVaultLogger logger;
        isOwnerOfVault_input io_in;
        isOwnerOfVault_output io_out;
        isOwnerOfVault_locals io_locals;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
        findOwnerIndexInVault_input fi_in;
        findOwnerIndexInVault_output fi_out;
        findOwnerIndexInVault_locals fi_locals;
        uint64 i;
    };

    struct getAssetReleaseStatus_input
    {
        uint64 vaultId;
    };
    struct getAssetReleaseStatus_output
    {
        uint64 status;
        Array<Asset, MSVAULT_MAX_OWNERS> assets;
        Array<uint64, MSVAULT_MAX_OWNERS> amounts;
        Array<id, MSVAULT_MAX_OWNERS> destinations;
    };
    struct getAssetReleaseStatus_locals
    {
        Vault qubicVault;
        VaultAssetPart assetVault;
        uint64 i;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct getVaultAssetBalances_input
    {
        uint64 vaultId;
    };
    struct getVaultAssetBalances_output
    {
        uint64 status;
        uint64 numberOfAssetTypes;
        Array<AssetBalance, MSVAULT_MAX_ASSET_TYPES> assetBalances;
    };
    struct getVaultAssetBalances_locals
    {
        uint64 i;
        Vault qubicVault;
        VaultAssetPart assetVault;
        isValidVaultId_input iv_in;
        isValidVaultId_output iv_out;
        isValidVaultId_locals iv_locals;
    };

    struct END_EPOCH_locals
    {
        uint64 i;
        uint64 j;
        uint64 k;
        Vault qubicVault;
        VaultAssetPart assetVault;
        sint64 amountToDistribute;
        uint64 feeToBurn;
        AssetBalance ab;
        QX::TransferShareOwnershipAndPossession_input qx_in;
        QX::TransferShareOwnershipAndPossession_output qx_out;
        id qxAdress;
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

    Array<VaultAssetPart, MSVAULT_MAX_VAULTS> vaultAssetParts;

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
        output.status = 0;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.vaultId = -1; // Not yet created

        if (qpi.invocationReward() < (sint64)state.liveRegisteringFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (qpi.invocationReward() > (sint64)state.liveRegisteringFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.liveRegisteringFee);
        }

        locals.ownerCount = 0;
        for (locals.i = 0; locals.i < MSVAULT_MAX_OWNERS; locals.i = locals.i + 1)
        {
            locals.proposedOwner = input.owners.get(locals.i);
            if (locals.proposedOwner != NULL_ID)
            {
                // Check for duplicates
                locals.found = false;
                for (locals.j = 0; locals.j < locals.ownerCount; locals.j++)
                {
                    if (locals.tempOwners.get(locals.j) == locals.proposedOwner)
                    {
                        locals.found = true;
                        break;
                    }
                }
                if (!locals.found)
                {
                    locals.tempOwners.set(locals.ownerCount, locals.proposedOwner);
                    locals.ownerCount++;
                }
            }
        }

        if (locals.ownerCount <= 1)
        {
            qpi.transfer(qpi.invocator(), (sint64)state.liveRegisteringFee);
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // requiredApprovals must be > 1 and <= numberOfOwners
        if (input.requiredApprovals <= 1 || input.requiredApprovals > locals.ownerCount)
        {
            qpi.transfer(qpi.invocator(), (sint64)state.liveRegisteringFee);
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Check co-ownership limits
        for (locals.i = 0; locals.i < locals.ownerCount; locals.i = locals.i + 1)
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
                        }
                    }
                }
            }
            if (locals.count >= MSVAULT_MAX_COOWNER)
            {
                qpi.transfer(qpi.invocator(), (sint64)state.liveRegisteringFee);
                output.status = 7; // FAILURE_LIMIT_REACHED
                locals.logger._type = (uint32)output.status;
                LOG_INFO(locals.logger);
                return;
            }
        }

        // Find empty slot
        locals.slotIndex = -1;
        for (locals.ii = 0; locals.ii < MSVAULT_MAX_VAULTS; locals.ii++)
        {
            locals.tempVault = state.vaults.get(locals.ii);
            if (!locals.tempVault.isActive && locals.tempVault.numberOfOwners == 0 && locals.tempVault.qubicBalance == 0)
            {
                locals.slotIndex = locals.ii;
                break;
            }
        }

        if (locals.slotIndex == -1)
        {
            qpi.transfer(qpi.invocator(), (sint64)state.liveRegisteringFee);
            output.status = 7; // FAILURE_LIMIT_REACHED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Initialize the new vault
        locals.newQubicVault.vaultName = input.vaultName;
        locals.newQubicVault.numberOfOwners = (uint8)locals.ownerCount;
        locals.newQubicVault.requiredApprovals = (uint8)input.requiredApprovals;
        locals.newQubicVault.qubicBalance = 0;
        locals.newQubicVault.isActive = true;

        // Set owners
        for (locals.i = 0; locals.i < locals.ownerCount; locals.i++)
        {
            locals.newQubicVault.owners.set(locals.i, locals.tempOwners.get(locals.i));
        }
        // Clear remaining owner slots
        for (locals.i = locals.ownerCount; locals.i < MSVAULT_MAX_OWNERS; locals.i++)
        {
            locals.newQubicVault.owners.set(locals.i, NULL_ID);
        }

        // Reset release requests for both Qubic and Assets
        locals.rr_in.vault = locals.newQubicVault;
        resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
        locals.newQubicVault = locals.rr_out.vault;

        // Init the Asset part of the vault
        locals.newAssetVault.numberOfAssetTypes = 0;
        for (locals.i = 0; locals.i < locals.ownerCount; locals.i++)
        {
            locals.newAssetVault.releaseAssets.set(locals.i, { NULL_ID, 0 });
            locals.newAssetVault.releaseAssetAmounts.set(locals.i, 0);
            locals.newAssetVault.releaseAssetDestinations.set(locals.i, NULL_ID);
        }

        state.vaults.set((uint64)locals.slotIndex, locals.newQubicVault);
        state.vaultAssetParts.set((uint64)locals.slotIndex, locals.newAssetVault);

        state.numberOfActiveVaults++;

        state.totalRevenue += state.liveRegisteringFee;
        output.status = 1; // SUCCESS
        locals.logger.vaultId = locals.slotIndex;
        locals.logger._type = (uint32)output.status;
        LOG_INFO(locals.logger);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(deposit)
    {
        output.status = 0; // FAILURE_GENERAL

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.vaultId = input.vaultId;

        if (qpi.invocationReward() < (sint64)state.liveDepositFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // calculate the actual amount to deposit into the vault
        locals.amountToDeposit = qpi.invocationReward() - state.liveDepositFee;

        // make sure the deposit amount is greater than zero
        if (locals.amountToDeposit == 0)
        {
            // The user only send the exact fee amount, with nothing left to deposit
            // this is an invalid operation, so we refund everything
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultId);
        if (!locals.vault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // add the collected fee to the total revenue
        state.totalRevenue += state.liveDepositFee;

        // add the remaining amount to the specified vault's balance
        locals.vault.qubicBalance += locals.amountToDeposit;

        state.vaults.set(input.vaultId, locals.vault);
        output.status = 1; // SUCCESS
        locals.logger._type = (uint32)output.status;
        LOG_INFO(locals.logger);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(revokeAssetManagementRights)
    {
        // This procedure allows a user to revoke asset management rights from MsVault
        // and transfer them back to QX, which is the default manager for trading.

        output.status = 0; // FAILURE_GENERAL
        output.transferredNumberOfShares = 0;

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = input.numberOfShares;

        if (qpi.invocationReward() < (sint64)MSVAULT_REVOKE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.transferredNumberOfShares = 0;
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (qpi.invocationReward() > (sint64)MSVAULT_REVOKE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)MSVAULT_REVOKE_FEE);
        }

        // must transfer a positive number of shares.
        if (input.numberOfShares <= 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.transferredNumberOfShares = 0;
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Check if MsVault actually manages the specified number of shares for the caller.
        locals.managedBalance = qpi.numberOfShares(
            input.asset,
            { qpi.invocator(), SELF_INDEX },
            { qpi.invocator(), SELF_INDEX }
        );

        if (locals.managedBalance < input.numberOfShares)
        {
            // The user is trying to revoke more shares than are managed by MsVault.
            output.transferredNumberOfShares = 0;
            output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
        }
        else
        {
            // The balance check passed. Proceed to release the management rights.
            locals.result = qpi.releaseShares(
                input.asset,
                qpi.invocator(), // owner
                qpi.invocator(), // possessor
                input.numberOfShares,
                QX_CONTRACT_INDEX,
                QX_CONTRACT_INDEX,
                MSVAULT_REVOKE_FEE
            );

            if (locals.result < 0)
            {
                output.transferredNumberOfShares = 0;
                output.status = 8; // FAILURE_TRANSFER_FAILED
            }
            else
            {
                output.transferredNumberOfShares = input.numberOfShares;
                output.status = 1; // SUCCESS
            }
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(depositAsset)
    {
        output.status = 0; // GENEREAL_FAILURE

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.vaultId = input.vaultId;
        locals.logger.amount = input.amount;

        if (qpi.invocationReward() < (sint64)state.liveDepositFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (qpi.invocationReward() > (sint64)state.liveDepositFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.liveDepositFee);
        }

        locals.userAssetBalance = qpi.numberOfShares(input.asset,
            { qpi.invocator(), SELF_INDEX },
            { qpi.invocator(), SELF_INDEX });

        if (locals.userAssetBalance < (sint64)input.amount || input.amount == 0)
        {
            // User does not have enough shares, or is trying to deposit zero. Abort and refund the fee.
            output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
            qpi.transfer(qpi.invocator(), state.liveDepositFee);
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // check if vault id is valid and the vault is active
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);
        if (!locals.iv_out.result)
        {
            output.status = 3; // FAILURE_INVALID_VAULT
            qpi.transfer(qpi.invocator(), state.liveDepositFee);
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return; // invalid vault id
        }

        locals.qubicVault = state.vaults.get(input.vaultId);
        locals.assetVault = state.vaultAssetParts.get(input.vaultId);
        if (!locals.qubicVault.isActive)
        {
            output.status = 3; // FAILURE_INVALID_VAULT
            qpi.transfer(qpi.invocator(), state.liveDepositFee);
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return; // vault is not active
        }

        // check if the vault has room for a new asset type
        locals.assetIndex = -1;
        for (locals.i = 0; locals.i < locals.assetVault.numberOfAssetTypes; locals.i++)
        {
            locals.ab = locals.assetVault.assetBalances.get(locals.i);
            if (locals.ab.asset.assetName == input.asset.assetName && locals.ab.asset.issuer == input.asset.issuer)
            {
                locals.assetIndex = locals.i;
                break;
            }
        }

        // if the asset is new to this vault, check if there's an empty slot.
        if (locals.assetIndex == -1 && locals.assetVault.numberOfAssetTypes >= MSVAULT_MAX_ASSET_TYPES)
        {
            // no more new asset
            output.status = 7; // FAILURE_LIMIT_REACHED
            qpi.transfer(qpi.invocator(), state.liveDepositFee);
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // All checks passed, now perform the transfer of ownership.
        state.totalRevenue += state.liveDepositFee;

        locals.tempShares = qpi.numberOfShares(
            input.asset,
            { SELF, SELF_INDEX },
            { SELF, SELF_INDEX }
        );

        locals.qx_in.assetName = input.asset.assetName;
        locals.qx_in.issuer = input.asset.issuer;
        locals.qx_in.numberOfShares = input.amount;
        locals.qx_in.newOwnerAndPossessor = SELF;

        locals.transferResult = qpi.transferShareOwnershipAndPossession(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), input.amount, SELF);

        if (locals.transferResult < 0)
        {
            output.status = 8; // FAILURE_TRANSFER_FAILED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.transferedShares = qpi.numberOfShares(input.asset, { SELF, SELF_INDEX }, { SELF, SELF_INDEX }) - locals.tempShares;

        if (locals.transferedShares != (sint64)input.amount)
        {
            output.status = 8; // FAILURE_TRANSFER_FAILED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // If the transfer succeeds, update the vault's internal accounting.
        if (locals.assetIndex != -1)
        {
            // Asset type exists, update balance
            locals.ab = locals.assetVault.assetBalances.get(locals.assetIndex);
            locals.ab.balance += input.amount;
            locals.assetVault.assetBalances.set(locals.assetIndex, locals.ab);
        }
        else
        {
            // Add the new asset type to the vault's balance list
            locals.ab.asset = input.asset;
            locals.ab.balance = input.amount;
            locals.assetVault.assetBalances.set(locals.assetVault.numberOfAssetTypes, locals.ab);
            locals.assetVault.numberOfAssetTypes++;
        }

        state.vaults.set(input.vaultId, locals.qubicVault);
        state.vaultAssetParts.set(input.vaultId, locals.assetVault);

        output.status = 1; // SUCCESS
        locals.logger._type = (uint32)output.status;
        LOG_INFO(locals.logger);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(releaseTo)
    {
        output.status = 0; // GENEREAL_FAILURE

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.vaultId = input.vaultId;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = input.amount;
        locals.logger.destination = input.destination;

        if (qpi.invocationReward() < (sint64)state.liveReleaseFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (qpi.invocationReward() > (sint64)state.liveReleaseFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.liveReleaseFee);
        }

        state.totalRevenue += state.liveReleaseFee;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultId);

        if (!locals.vault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.io_in.vault = locals.vault;
        locals.io_in.ownerID = qpi.invocator();
        isOwnerOfVault(qpi, state, locals.io_in, locals.io_out, locals.io_locals);
        if (!locals.io_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 4; // FAILURE_NOT_AUTHORIZED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (input.amount == 0 || input.destination == NULL_ID)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (locals.vault.qubicBalance < input.amount)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.fi_in.vault = locals.vault;
        locals.fi_in.ownerID = qpi.invocator();
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        locals.ownerIndex = locals.fi_out.index;

        locals.vault.releaseAmounts.set(locals.ownerIndex, input.amount);
        locals.vault.releaseDestinations.set(locals.ownerIndex, input.destination);

        // Check for approvals
        locals.approvals = 0;
        for (locals.i = 0; locals.i < (uint64)locals.vault.numberOfOwners; locals.i++)
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
            if (locals.vault.qubicBalance >= input.amount)
            {
                qpi.transfer(input.destination, input.amount);
                locals.vault.qubicBalance -= input.amount;

                locals.rr_in.vault = locals.vault;
                resetReleaseRequests(qpi, state, locals.rr_in, locals.rr_out, locals.rr_locals);
                locals.vault = locals.rr_out.vault;

                state.vaults.set(input.vaultId, locals.vault);

                output.status = 1; // SUCCESS
                locals.logger._type = (uint32)output.status;
                LOG_INFO(locals.logger);
            }
            else
            {
                output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
                locals.logger._type = (uint32)output.status;
                LOG_INFO(locals.logger);
            }
        }
        else
        {
            state.vaults.set(input.vaultId, locals.vault);
            output.status = 9; // PENDING_APPROVAL
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(releaseAssetTo)
    {
        output.status = 0; // GENEREAL_FAILURE

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.vaultId = input.vaultId;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = input.amount;
        locals.logger.destination = input.destination;

        if (qpi.invocationReward() < (sint64)state.liveReleaseFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (qpi.invocationReward() > (sint64)state.liveReleaseFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.liveReleaseFee);
        }

        state.totalRevenue += state.liveReleaseFee;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.qubicVault = state.vaults.get(input.vaultId);
        locals.assetVault = state.vaultAssetParts.get(input.vaultId);

        if (!locals.qubicVault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.io_in.vault = locals.qubicVault;
        locals.io_in.ownerID = qpi.invocator();
        isOwnerOfVault(qpi, state, locals.io_in, locals.io_out, locals.io_locals);
        if (!locals.io_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 4; // FAILURE_NOT_AUTHORIZED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (locals.qubicVault.qubicBalance < MSVAULT_REVOKE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (input.amount == 0 || input.destination == NULL_ID)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Find the asset in the vault
        locals.assetIndex = -1;
        for (locals.i = 0; locals.i < locals.assetVault.numberOfAssetTypes; locals.i++)
        {
            locals.ab = locals.assetVault.assetBalances.get(locals.i);
            if (locals.ab.asset.assetName == input.asset.assetName && locals.ab.asset.issuer == input.asset.issuer)
            {
                locals.assetIndex = locals.i;
                break;
            }
        }
        if (locals.assetIndex == -1)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 5; // FAILURE_INVALID_PARAMS
            locals.logger._type = (uint32)output.status; // Asset not found
            LOG_INFO(locals.logger);
            return;
        }

        if (locals.assetVault.assetBalances.get(locals.assetIndex).balance < input.amount)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Record the release request
        locals.fi_in.vault = locals.qubicVault;
        locals.fi_in.ownerID = qpi.invocator();
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        locals.ownerIndex = locals.fi_out.index;

        locals.assetVault.releaseAssets.set(locals.ownerIndex, input.asset);
        locals.assetVault.releaseAssetAmounts.set(locals.ownerIndex, input.amount);
        locals.assetVault.releaseAssetDestinations.set(locals.ownerIndex, input.destination);

        // Check for approvals
        locals.approvals = 0;
        for (locals.i = 0; locals.i < (uint64)locals.qubicVault.numberOfOwners; locals.i++)
        {
            if (locals.assetVault.releaseAssetAmounts.get(locals.i) == input.amount &&
                locals.assetVault.releaseAssetDestinations.get(locals.i) == input.destination &&
                locals.assetVault.releaseAssets.get(locals.i).assetName == input.asset.assetName &&
                locals.assetVault.releaseAssets.get(locals.i).issuer == input.asset.issuer)
            {
                locals.approvals++;
            }
        }

        locals.releaseApproved = false;
        if (locals.approvals >= (uint64)locals.qubicVault.requiredApprovals)
        {
            locals.releaseApproved = true;
        }

        if (locals.releaseApproved)
        {
            // Re-check balance before transfer
            if (locals.assetVault.assetBalances.get(locals.assetIndex).balance >= input.amount)
            {
                locals.remainingShares = qpi.transferShareOwnershipAndPossession(
                    input.asset.assetName,
                    input.asset.issuer,
                    SELF, // owner
                    SELF, // possessor
                    input.amount,
                    input.destination // new owner & possessor
                );
                if (locals.remainingShares >= 0)
                {
                    // Update internal asset balance
                    locals.ab = locals.assetVault.assetBalances.get(locals.assetIndex);
                    locals.ab.balance -= input.amount;
                    locals.assetVault.assetBalances.set(locals.assetIndex, locals.ab);

                    // Release management rights from MsVault to QX for the recipient
                    locals.releaseResult = qpi.releaseShares(
                        input.asset,
                        input.destination, // new owner
                        input.destination, // new possessor
                        input.amount,
                        QX_CONTRACT_INDEX,
                        QX_CONTRACT_INDEX,
                        MSVAULT_REVOKE_FEE
                    );

                    if (locals.releaseResult >= 0)
                    {
                        // Deduct the fee from the vault's balance upon success
                        locals.qubicVault.qubicBalance -= MSVAULT_REVOKE_FEE;
                        output.status = 1; // SUCCESS
                    }
                    else
                    {
                        // Log an error if management rights transfer fails
                        output.status = 8; // FAILURE_TRANSFER_FAILED
                    }
                    locals.logger._type = (uint32)output.status;
                    LOG_INFO(locals.logger);

                    // Reset all asset release requests
                    for (locals.i = 0; locals.i < MSVAULT_MAX_OWNERS; locals.i++)
                    {
                        locals.assetVault.releaseAssets.set(locals.i, { NULL_ID, 0 });
                        locals.assetVault.releaseAssetAmounts.set(locals.i, 0);
                        locals.assetVault.releaseAssetDestinations.set(locals.i, NULL_ID);
                    }
                }
                else
                {
                    output.status = 8; // FAILURE_TRANSFER_FAILED
                    locals.logger._type = (uint32)output.status;
                    LOG_INFO(locals.logger);
                }
                state.vaults.set(input.vaultId, locals.qubicVault);
                state.vaultAssetParts.set(input.vaultId, locals.assetVault);
            }
            else
            {
                output.status = 6; // FAILURE_INSUFFICIENT_BALANCE
                state.vaults.set(input.vaultId, locals.qubicVault);
                state.vaultAssetParts.set(input.vaultId, locals.assetVault);
                locals.logger._type = (uint32)output.status;
                LOG_INFO(locals.logger);
            }
        }
        else
        {
            state.vaults.set(input.vaultId, locals.qubicVault);
            state.vaultAssetParts.set(input.vaultId, locals.assetVault);
            output.status = 9; // PENDING_APPROVAL
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(resetRelease)
    {
        output.status = 0; // GENEREAL_FAILURE

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.vaultId = input.vaultId;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = 0;
        locals.logger.destination = NULL_ID;

        if (qpi.invocationReward() < (sint64)state.liveReleaseResetFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }
        if (qpi.invocationReward() > (sint64)state.liveReleaseResetFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.liveReleaseResetFee);
        }

        state.totalRevenue += state.liveReleaseResetFee;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.vault = state.vaults.get(input.vaultId);

        if (!locals.vault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.io_in.vault = locals.vault;
        locals.io_in.ownerID = qpi.invocator();
        isOwnerOfVault(qpi, state, locals.io_in, locals.io_out, locals.io_locals);
        if (!locals.io_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 4; // FAILURE_NOT_AUTHORIZED
            locals.logger._type = (uint32)output.status;
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

        output.status = 1; // SUCCESS
        locals.logger._type = (uint32)output.status;
        LOG_INFO(locals.logger);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(resetAssetRelease)
    {
        output.status = 0; // GENEREAL_FAILURE

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.vaultId = input.vaultId;
        locals.logger.ownerID = qpi.invocator();
        locals.logger.amount = 0;
        locals.logger.destination = NULL_ID;

        if (qpi.invocationReward() < (sint64)state.liveReleaseResetFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }
        if (qpi.invocationReward() > (sint64)state.liveReleaseResetFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.liveReleaseResetFee);
        }

        state.totalRevenue += state.liveReleaseResetFee;

        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.qubicVault = state.vaults.get(input.vaultId);
        locals.assetVault = state.vaultAssetParts.get(input.vaultId);

        if (!locals.qubicVault.isActive)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 3; // FAILURE_INVALID_VAULT
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.io_in.vault = locals.qubicVault;
        locals.io_in.ownerID = qpi.invocator();
        isOwnerOfVault(qpi, state, locals.io_in, locals.io_out, locals.io_locals);
        if (!locals.io_out.result)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 4; // FAILURE_NOT_AUTHORIZED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.fi_in.vault = locals.qubicVault;
        locals.fi_in.ownerID = qpi.invocator();
        findOwnerIndexInVault(qpi, state, locals.fi_in, locals.fi_out, locals.fi_locals);
        locals.ownerIndex = locals.fi_out.index;

        locals.assetVault.releaseAssets.set(locals.ownerIndex, { NULL_ID, 0 });
        locals.assetVault.releaseAssetAmounts.set(locals.ownerIndex, 0);
        locals.assetVault.releaseAssetDestinations.set(locals.ownerIndex, NULL_ID);

        state.vaults.set(input.vaultId, locals.qubicVault);
        state.vaultAssetParts.set(input.vaultId, locals.assetVault);

        output.status = 1; // SUCCESS
        locals.logger._type = (uint32)output.status;
        LOG_INFO(locals.logger);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(voteFeeChange)
    {
        output.status = 0; // GENEREAL_FAILURE

        locals.logger._contractIndex = CONTRACT_INDEX;
        locals.logger.ownerID = qpi.invocator();

        if (qpi.invocationReward() < (sint64)MSVAULT_VOTE_FEE_CHANGE_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.status = 2; // FAILURE_INSUFFICIENT_FEE
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }

        locals.ish_in.candidate = qpi.invocator();
        isShareHolder(qpi, state, locals.ish_in, locals.ish_out, locals.ish_locals);
        if (!locals.ish_out.result)
        {
            output.status = 4; // FAILURE_NOT_AUTHORIZED
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }
    
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        locals.nShare = qpi.numberOfShares({ NULL_ID, MSVAULT_ASSET_NAME }, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator()));
    
        locals.fs.registeringFee = input.newRegisteringFee;
        locals.fs.releaseFee = input.newReleaseFee;
        locals.fs.releaseResetFee = input.newReleaseResetFee;
        locals.fs.holdingFee = input.newHoldingFee;
        locals.fs.depositFee = input.newDepositFee;
        // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
        //locals.fs.burnFee = input.burnFee;
    
        locals.needNewRecord = true;
        for (locals.i = 0; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
        {
            locals.currentAddr = state.feeVotesOwner.get(locals.i);
            locals.realScore = qpi.numberOfShares({ NULL_ID, MSVAULT_ASSET_NAME }, AssetOwnershipSelect::byOwner(locals.currentAddr), AssetPossessionSelect::byPossessor(locals.currentAddr));
            state.feeVotesScore.set(locals.i, locals.realScore);
            if (locals.currentAddr == qpi.invocator())
            {
                locals.needNewRecord = false;
                state.feeVotes.set(locals.i, locals.fs); // Update existing vote
            }
        }
        if (locals.needNewRecord && state.feeVotesAddrCount < MSVAULT_MAX_FEE_VOTES)
        {
            state.feeVotes.set(state.feeVotesAddrCount, locals.fs);
            state.feeVotesOwner.set(state.feeVotesAddrCount, qpi.invocator());
            state.feeVotesScore.set(state.feeVotesAddrCount, locals.nShare);
            state.feeVotesAddrCount = state.feeVotesAddrCount + 1;
        }
    
        locals.sumVote = 0;
        for (locals.i = 0; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
        {
            locals.sumVote = locals.sumVote + state.feeVotesScore.get(locals.i);
        }
        if (locals.sumVote < QUORUM)
        {
            output.status = 1; // SUCCESS
            locals.logger._type = (uint32)output.status;
            LOG_INFO(locals.logger);
            return;
        }
    
        state.uniqueFeeVotesCount = 0;
        // Reset unique vote ranking
        for (locals.i = 0; locals.i < MSVAULT_MAX_FEE_VOTES; locals.i = locals.i + 1)
        {
            state.uniqueFeeVotesRanking.set(locals.i, 0);
        }

        for (locals.i = 0; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
        {
            locals.currentVote = state.feeVotes.get(locals.i);
            locals.found = false;
            locals.uniqueIndex = 0;
            for (locals.j = 0; locals.j < state.uniqueFeeVotesCount; locals.j = locals.j + 1)
            {
                locals.uniqueVote = state.uniqueFeeVotes.get(locals.j);
                if (locals.uniqueVote.registeringFee == locals.currentVote.registeringFee &&
                    locals.uniqueVote.releaseFee == locals.currentVote.releaseFee &&
                    locals.uniqueVote.releaseResetFee == locals.currentVote.releaseResetFee &&
                    locals.uniqueVote.holdingFee == locals.currentVote.holdingFee &&
                    locals.uniqueVote.depositFee == locals.currentVote.depositFee
                    // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
                    //&& locals.uniqueVote.burnFee == locals.currentVote.burnFee
                    )
                {
                    locals.found = true;
                    locals.uniqueIndex = locals.j;
                    break;
                }
            }
            if (locals.found)
            {
                locals.currentRank = state.uniqueFeeVotesRanking.get(locals.uniqueIndex);
                state.uniqueFeeVotesRanking.set(locals.uniqueIndex, locals.currentRank + state.feeVotesScore.get(locals.i));
            }
            else if (state.uniqueFeeVotesCount < MSVAULT_MAX_FEE_VOTES)
            {
                state.uniqueFeeVotes.set(state.uniqueFeeVotesCount, locals.currentVote);
                state.uniqueFeeVotesRanking.set(state.uniqueFeeVotesCount, state.feeVotesScore.get(locals.i));
                state.uniqueFeeVotesCount = state.uniqueFeeVotesCount + 1;
            }
        }
    
        for (locals.i = 0; locals.i < state.uniqueFeeVotesCount; locals.i = locals.i + 1)
        {
            if (state.uniqueFeeVotesRanking.get(locals.i) >= QUORUM)
            {
                state.liveRegisteringFee = state.uniqueFeeVotes.get(locals.i).registeringFee;
                state.liveReleaseFee = state.uniqueFeeVotes.get(locals.i).releaseFee;
                state.liveReleaseResetFee = state.uniqueFeeVotes.get(locals.i).releaseResetFee;
                state.liveHoldingFee = state.uniqueFeeVotes.get(locals.i).holdingFee;
                state.liveDepositFee = state.uniqueFeeVotes.get(locals.i).depositFee;
                // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
                //state.liveBurnFee = state.uniqueFeeVotes.get(locals.i).burnFee;

                state.feeVotesAddrCount = 0;
                state.uniqueFeeVotesCount = 0;
                output.status = 1; // SUCCESS
                locals.logger._type = (uint32)output.status;
                LOG_INFO(locals.logger);
                return;
            }
        }
        output.status = 1; // SUCCESS
        locals.logger._type = (uint32)output.status;
        LOG_INFO(locals.logger);
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getVaults)
    {
        output.numberOfVaults = 0ULL;
        locals.count = 0ULL;
        for (locals.i = 0ULL; locals.i < MSVAULT_MAX_VAULTS && locals.count < MSVAULT_MAX_COOWNER; locals.i++)
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

    PUBLIC_FUNCTION_WITH_LOCALS(getAssetReleaseStatus)
    {
        output.status = 0ULL;
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            return; // output.status = false
        }

        locals.qubicVault = state.vaults.get(input.vaultId);
        if (!locals.qubicVault.isActive)
        {
            return; // output.status = false
        }
        locals.assetVault = state.vaultAssetParts.get(input.vaultId);

        for (locals.i = 0; locals.i < (uint64)locals.qubicVault.numberOfOwners; locals.i++)
        {
            output.assets.set(locals.i, locals.assetVault.releaseAssets.get(locals.i));
            output.amounts.set(locals.i, locals.assetVault.releaseAssetAmounts.get(locals.i));
            output.destinations.set(locals.i, locals.assetVault.releaseAssetDestinations.get(locals.i));
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
        output.balance = locals.vault.qubicBalance;
        output.status = 1ULL;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getVaultAssetBalances)
    {
        output.status = 0ULL;
        locals.iv_in.vaultId = input.vaultId;
        isValidVaultId(qpi, state, locals.iv_in, locals.iv_out, locals.iv_locals);

        if (!locals.iv_out.result)
        {
            return; // output.status = false
        }

        locals.qubicVault = state.vaults.get(input.vaultId);
        if (!locals.qubicVault.isActive)
        {
            return; // output.status = false
        }
        locals.assetVault = state.vaultAssetParts.get(input.vaultId);
        output.numberOfAssetTypes = locals.assetVault.numberOfAssetTypes;
        for (locals.i = 0; locals.i < locals.assetVault.numberOfAssetTypes; locals.i++)
        {
            output.assetBalances.set(locals.i, locals.assetVault.assetBalances.get(locals.i));
        }
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
        output.burnedAmount = 0;
    }

    PUBLIC_FUNCTION(getFees)
    {
        output.registeringFee = state.liveRegisteringFee;
        output.releaseFee = state.liveReleaseFee;
        output.releaseResetFee = state.liveReleaseResetFee;
        output.holdingFee = state.liveHoldingFee;
        output.depositFee = state.liveDepositFee;
        // [TODO]: Turn this ON when MSVAULT_BURN_FEE > 0
        //output.burnFee = state.liveBurnFee;
        output.burnFee = MSVAULT_BURN_FEE;
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
    
    PUBLIC_FUNCTION_WITH_LOCALS(isShareHolder)
    {
        if (qpi.numberOfShares({ NULL_ID, MSVAULT_ASSET_NAME }, AssetOwnershipSelect::byOwner(input.candidate),
            AssetPossessionSelect::byPossessor(input.candidate)) > 0)
        {
            output.result = 1ULL;
        }
        else
        {
            output.result = 0ULL;
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getFeeVotes)
    {
        output.status = 0ULL;

        for (locals.i = 0ULL; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
        {
            output.feeVotes.set(locals.i, state.feeVotes.get(locals.i));
        }

        output.numberOfFeeVotes = state.feeVotesAddrCount;

        output.status = 1ULL;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getFeeVotesOwner)
    {
        output.status = 0ULL;

        for (locals.i = 0ULL; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
        {
            output.feeVotesOwner.set(locals.i, state.feeVotesOwner.get(locals.i));
        }
        output.numberOfFeeVotes = state.feeVotesAddrCount;

        output.status = 1ULL;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getFeeVotesScore)
    {
        output.status = 0ULL;

        for (locals.i = 0ULL; locals.i < state.feeVotesAddrCount; locals.i = locals.i + 1)
        {
            output.feeVotesScore.set(locals.i, state.feeVotesScore.get(locals.i));
        }
        output.numberOfFeeVotes = state.feeVotesAddrCount;

        output.status = 1ULL;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getUniqueFeeVotes)
    {
        output.status = 0ULL;

        for (locals.i = 0ULL; locals.i < state.uniqueFeeVotesCount; locals.i = locals.i + 1)
        {
            output.uniqueFeeVotes.set(locals.i, state.uniqueFeeVotes.get(locals.i));
        }
        output.numberOfUniqueFeeVotes = state.uniqueFeeVotesCount;

        output.status = 1ULL;
    }

    PUBLIC_FUNCTION(getManagedAssetBalance)
    {
        // Get management rights balance the owner transferred to MsVault
        output.balance = qpi.numberOfShares(
            input.asset,
            { input.owner, SELF_INDEX },
            { input.owner, SELF_INDEX }
        );
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getUniqueFeeVotesRanking)
    {
        output.status = 0ULL;

        for (locals.i = 0ULL; locals.i < state.uniqueFeeVotesCount; locals.i = locals.i + 1)
        {
            output.uniqueFeeVotesRanking.set(locals.i, state.uniqueFeeVotesRanking.get(locals.i));
        }
        output.numberOfUniqueFeeVotes = state.uniqueFeeVotesCount;

        output.status = 1ULL;
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
        locals.qxAdress = id(QX_CONTRACT_INDEX, 0, 0, 0);
        for (locals.i = 0ULL; locals.i < MSVAULT_MAX_VAULTS; locals.i++)
        {
            locals.qubicVault = state.vaults.get(locals.i);
            if (locals.qubicVault.isActive)
            {
                if (locals.qubicVault.qubicBalance >= state.liveHoldingFee)
                {
                    locals.qubicVault.qubicBalance -= state.liveHoldingFee;
                    state.totalRevenue += state.liveHoldingFee;
                    state.vaults.set(locals.i, locals.qubicVault);
                }
                else
                {
                    // Not enough funds to pay holding fee
                    if (locals.qubicVault.qubicBalance > 0)
                    {
                        state.totalRevenue += locals.qubicVault.qubicBalance;
                    }

                    locals.assetVault = state.vaultAssetParts.get(locals.i);
                    for (locals.k = 0; locals.k < locals.assetVault.numberOfAssetTypes; locals.k++)
                    {
                        locals.ab = locals.assetVault.assetBalances.get(locals.k);
                        if (locals.ab.balance > 0)
                        {
                            // Prepare the transfer request to QX
                            locals.qx_in.assetName = locals.ab.asset.assetName;
                            locals.qx_in.issuer = locals.ab.asset.issuer;
                            locals.qx_in.numberOfShares = locals.ab.balance;
                            locals.qx_in.newOwnerAndPossessor = locals.qxAdress;

                            INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareOwnershipAndPossession, locals.qx_in, locals.qx_out, 0);
                        }
                    }
                    locals.qubicVault.isActive = false;
                    locals.qubicVault.qubicBalance = 0;
                    locals.qubicVault.requiredApprovals = 0;
                    locals.qubicVault.vaultName = NULL_ID;
                    locals.qubicVault.numberOfOwners = 0;
                    for (locals.j = 0; locals.j < MSVAULT_MAX_OWNERS; locals.j++)
                    {
                        locals.qubicVault.owners.set(locals.j, NULL_ID);
                        locals.qubicVault.releaseAmounts.set(locals.j, 0);
                        locals.qubicVault.releaseDestinations.set(locals.j, NULL_ID);
                    }

                    // clear asset release proposals
                    locals.assetVault.numberOfAssetTypes = 0;
                    for (locals.j = 0; locals.j < MSVAULT_MAX_ASSET_TYPES; locals.j++)
                    {
                        locals.assetVault.assetBalances.set(locals.j, { { NULL_ID, 0 }, 0 });
                    }
                    for (locals.j = 0; locals.j < MSVAULT_MAX_OWNERS; locals.j++)
                    {
                        locals.assetVault.releaseAssets.set(locals.j, { NULL_ID, 0 });
                        locals.assetVault.releaseAssetAmounts.set(locals.j, 0);
                        locals.assetVault.releaseAssetDestinations.set(locals.j, NULL_ID);
                    }

                    if (state.numberOfActiveVaults > 0)
                    {
                        state.numberOfActiveVaults--;
                    }
                    state.vaults.set(locals.i, locals.qubicVault);
                    state.vaultAssetParts.set(locals.i, locals.assetVault);
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
        REGISTER_USER_FUNCTION(getFeeVotes, 14);
        REGISTER_USER_FUNCTION(getFeeVotesOwner, 15);
        REGISTER_USER_FUNCTION(getFeeVotesScore, 16);
        REGISTER_USER_FUNCTION(getUniqueFeeVotes, 17);
        REGISTER_USER_FUNCTION(getUniqueFeeVotesRanking, 18);
        // New asset-related functions and procedures
        REGISTER_USER_PROCEDURE(depositAsset, 19);
        REGISTER_USER_PROCEDURE(releaseAssetTo, 20);
        REGISTER_USER_PROCEDURE(resetAssetRelease, 21);
        REGISTER_USER_FUNCTION(getVaultAssetBalances, 22);
        REGISTER_USER_FUNCTION(getAssetReleaseStatus, 23);
        REGISTER_USER_FUNCTION(getManagedAssetBalance, 24);
        REGISTER_USER_PROCEDURE(revokeAssetManagementRights, 25);

    }

    PRE_ACQUIRE_SHARES()
    {
        output.requestedFee = 0;
        output.allowTransfer = true;
    }

    POST_ACQUIRE_SHARES()
    {
    }
};
