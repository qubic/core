using namespace QPI;

/***************************************************/
/******************* CONSTANTS *********************/
/***************************************************/

constexpr uint64 QRWA_MAX_QMINE_HOLDERS = 1048576 * 2 * X_MULTIPLIER; // 2^21
constexpr uint64 QRWA_MAX_GOV_POLLS = 64; // 8 active polls * 8 epochs = 64 slots
constexpr uint64 QRWA_MAX_ASSET_POLLS = 64; // 8 active polls * 8 epochs = 64 slots
constexpr uint64 QRWA_MAX_ASSETS = 1024; // 2^10

constexpr uint64 QRWA_MAX_NEW_GOV_POLLS_PER_EPOCH = 8;
constexpr uint64 QRWA_MAX_NEW_ASSET_POLLS_PER_EPOCH = 8;

// Dividend percentage constants
constexpr uint64 QRWA_QMINE_HOLDER_PERCENT = 900; // 90.0%
constexpr uint64 QRWA_QRWA_HOLDER_PERCENT = 100;  // 10.0%
constexpr uint64 QRWA_PERCENT_DENOMINATOR = 1000; // 100.0%

// Payout Timing Constants
constexpr uint64 QRWA_PAYOUT_DAY = FRIDAY; // Friday
constexpr uint64 QRWA_PAYOUT_HOUR = 12; // 12:00 PM UTC
constexpr uint64 QRWA_MIN_PAYOUT_INTERVAL_MS = 6 * 86400000LL; // 6 days in milliseconds

// STATUS CODES for Procedures
constexpr uint64 QRWA_STATUS_SUCCESS = 1;
constexpr uint64 QRWA_STATUS_FAILURE_GENERAL = 0;
constexpr uint64 QRWA_STATUS_FAILURE_INSUFFICIENT_FEE = 2;
constexpr uint64 QRWA_STATUS_FAILURE_INVALID_INPUT = 3;
constexpr uint64 QRWA_STATUS_FAILURE_NOT_AUTHORIZED = 4;
constexpr uint64 QRWA_STATUS_FAILURE_INSUFFICIENT_BALANCE = 5;
constexpr uint64 QRWA_STATUS_FAILURE_LIMIT_REACHED = 6;
constexpr uint64 QRWA_STATUS_FAILURE_TRANSFER_FAILED = 7;
constexpr uint64 QRWA_STATUS_FAILURE_NOT_FOUND = 8;
constexpr uint64 QRWA_STATUS_FAILURE_ALREADY_VOTED = 9;
constexpr uint64 QRWA_STATUS_FAILURE_POLL_INACTIVE = 10;
constexpr uint64 QRWA_STATUS_FAILURE_WRONG_STATE = 11;

constexpr uint64 QRWA_POLL_STATUS_EMPTY = 0;
constexpr uint64 QRWA_POLL_STATUS_ACTIVE = 1; // poll is live, can be voted
constexpr uint64 QRWA_POLL_STATUS_PASSED_EXECUTED = 2; // poll inactive, result is YES
constexpr uint64 QRWA_POLL_STATUS_FAILED_VOTE = 3; // poll inactive, result is NO
constexpr uint64 QRWA_POLL_STATUS_PASSED_FAILED_EXECUTION = 4; // poll inactive, result is YES but failed to release asset

// QX Fee for releasing management rights
constexpr sint64 QRWA_RELEASE_MANAGEMENT_FEE = 100;

// LOG TYPES
constexpr uint64 QRWA_LOG_TYPE_DISTRIBUTION = 1;
constexpr uint64 QRWA_LOG_TYPE_GOV_VOTE = 2;
constexpr uint64 QRWA_LOG_TYPE_ASSET_POLL_CREATED = 3;
constexpr uint64 QRWA_LOG_TYPE_ASSET_VOTE = 4;
constexpr uint64 QRWA_LOG_TYPE_ASSET_POLL_EXECUTED = 5;
constexpr uint64 QRWA_LOG_TYPE_TREASURY_DONATION = 6;
constexpr uint64 QRWA_LOG_TYPE_ADMIN_ACTION = 7;
constexpr uint64 QRWA_LOG_TYPE_ERROR = 8;
constexpr uint64 QRWA_LOG_TYPE_INCOMING_REVENUE_A = 9;
constexpr uint64 QRWA_LOG_TYPE_INCOMING_REVENUE_B = 10;


/***************************************************/
/**************** CONTRACT STATE *******************/
/***************************************************/

struct QRWA : public ContractBase
{
    friend class ContractTestingQRWA;
    /***************************************************/
    /******************** STRUCTS **********************/
    /***************************************************/

    struct QRWAAsset
    {
        id issuer;
        uint64 assetName;

        operator Asset() const
        {
            return { issuer, assetName };
        }

        bool operator==(const QRWAAsset other) const
        {
            return issuer == other.issuer && assetName == other.assetName;
        }

        bool operator!=(const QRWAAsset other) const
        {
            return issuer != other.issuer || assetName != other.assetName;
        }

        inline void setFrom(const Asset& asset)
        {
            issuer = asset.issuer;
            assetName = asset.assetName;
        }
    };

    // votable governance parameters for the contract.
    struct QRWAGovParams
    {
        // Addresses
        id mAdminAddress; // Only the admin can create release polls
        // Addresses to receive the MINING FEEs
        id electricityAddress;
        id maintenanceAddress;
        id reinvestmentAddress;
        id qmineDevAddress; // Address to receive rewards for moved QMINE during epoch

        // MINING FEE Percentages
        uint64 electricityPercent;
        uint64 maintenancePercent;
        uint64 reinvestmentPercent;
    };

    // Represents a governance poll in a rotating buffer
    struct QRWAGovProposal
    {
        uint64 proposalId; // The unique, increasing ID
        uint64 status; // 0=Empty, 1=Active, 2=Passed, 3=Failed
        uint64 score; // Final score, count at END_EPOCH
        QRWAGovParams params; // The actual proposal data
    };

    // Represents a poll to release assets from the treasury or dividend pool.
    struct AssetReleaseProposal
    {
        uint64 proposalId;
        id proposalName;
        Asset asset;
        uint64 amount;
        id destination;
        uint64 status; // 0=Empty, 1=Active, 2=Passed_Executed, 3=Failed, 4=Passed_Failed_Execution
        uint64 votesYes; // Final score, count at END_EPOCH
        uint64 votesNo; // Final score, count at END_EPOCH
    };

    // Logger for general contract events.
    struct QRWALogger
    {
        uint64 contractId;
        uint64 logType;
        id primaryId; // voter, asset issuer, proposal creator
        uint64 valueA;
        uint64 valueB;
        sint8 _terminator;
    };

protected:
    Asset mQmineAsset;

    // QMINE Shareholder Tracking
    HashMap<id, uint64, QRWA_MAX_QMINE_HOLDERS> mBeginEpochBalances;
    HashMap<id, uint64, QRWA_MAX_QMINE_HOLDERS> mEndEpochBalances;
    uint64 mTotalQmineBeginEpoch; // Total QMINE shares at the start of the current epoch

    // PAYOUT SNAPSHOTS (for distribution)
    // These hold the data from the last epoch, saved at END_EPOCH
    HashMap<id, uint64, QRWA_MAX_QMINE_HOLDERS> mPayoutBeginBalances;
    HashMap<id, uint64, QRWA_MAX_QMINE_HOLDERS> mPayoutEndBalances;
    uint64 mPayoutTotalQmineBegin; // Total QMINE shares from the last epoch's beginning

    // Votable Parameters
    QRWAGovParams mCurrentGovParams; // The live, active parameters

    // Voting state for governance parameters (voted by QMINE holders)
    Array<QRWAGovProposal, QRWA_MAX_GOV_POLLS> mGovPolls;
    HashMap<id, uint64, QRWA_MAX_QMINE_HOLDERS> mShareholderVoteMap; // Maps QMINE holder -> Gov Poll slot index
    uint64 mCurrentGovProposalId;
    uint64 mNewGovPollsThisEpoch;

    // Asset Release Polls
    Array<AssetReleaseProposal, QRWA_MAX_ASSET_POLLS> mAssetPolls;
    HashMap<id, bit_64, QRWA_MAX_QMINE_HOLDERS> mAssetProposalVoterMap; // (Voter -> bitfield of poll slot indices)
    HashMap<id, bit_64, QRWA_MAX_QMINE_HOLDERS> mAssetVoteOptions; // (Voter -> bitfield of options (0=No, 1=Yes))
    uint64 mCurrentAssetProposalId; // Counter for creating new proposal ID
    uint64 mNewAssetPollsThisEpoch;

    // Treasury & Asset Release
    uint64 mTreasuryBalance; // QMINE token balance holds by SC
    HashMap<QRWAAsset, uint64, QRWA_MAX_ASSETS> mGeneralAssetBalances; // Balances for other assets (e.g., SC shares)

    // Payouts and Dividend Accounting
    DateAndTime mLastPayoutTime; // Tracks the last payout time

    // Dividend Pools
    uint64 mRevenuePoolA; // Mined funds from Qubic farm (from SCs)
    uint64 mRevenuePoolB; // Other dividend funds (from user wallets)

    // Processed dividend pools awaiting distribution
    uint64 mQmineDividendPool; // QUs for QMINE holders
    uint64 mQRWADividendPool; // QUs for qRWA shareholders

    // Total distributed tracking
    uint64 mTotalQmineDistributed;
    uint64 mTotalQRWADistributed;

public:
    /***************************************************/
    /**************** PUBLIC PROCEDURES ****************/
    /***************************************************/

    // Treasury
    struct DonateToTreasury_input
    {
        uint64 amount;
    };
    struct DonateToTreasury_output
    {
        uint64 status;
    };
    struct DonateToTreasury_locals
    {
        sint64 transferResult;
        sint64 balance;
        QRWALogger logger;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(DonateToTreasury)
    {
        // NOTE: This procedure transfers QMINE from the invoker's *managed* balance (managed by this SC)
        // to the SC's internal treasury.
        // A one-time setup by the donor is required:
        // 1. Call QX::TransferShareManagementRights to give this SC management rights over the QMINE.
        // 2. Call this DonateToTreasury procedure to transfer ownership to the SC.

        // This procedure has no fee, refund any invocation reward
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        output.status = QRWA_STATUS_FAILURE_GENERAL;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_TREASURY_DONATION;
        locals.logger.primaryId = qpi.invocator();
        locals.logger.valueA = input.amount;

        if (state.mQmineAsset.issuer == NULL_ID)
        {
            output.status = QRWA_STATUS_FAILURE_WRONG_STATE; // QMINE asset not set
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }
        if (input.amount == 0)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Check if user has granted management rights to this SC
        locals.balance = qpi.numberOfShares(state.mQmineAsset,
            { qpi.invocator(), SELF_INDEX },
            { qpi.invocator(), SELF_INDEX });

        if (locals.balance < static_cast<sint64>(input.amount))
        {
            output.status = QRWA_STATUS_FAILURE_INSUFFICIENT_BALANCE; // Not enough managed shares
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Transfer QMINE from invoker (managed by SELF) to SELF (owned by SELF)
        locals.transferResult = qpi.transferShareOwnershipAndPossession(
            state.mQmineAsset.assetName,
            state.mQmineAsset.issuer,
            qpi.invocator(), // current owner
            qpi.invocator(), // current possessor
            input.amount,
            SELF             // new owner and possessor
        );

        if (locals.transferResult >= 0) // Transfer successful
        {
            state.mTreasuryBalance = sadd(state.mTreasuryBalance, input.amount);
            output.status = QRWA_STATUS_SUCCESS;
        }
        else
        {
            output.status = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
        }

        locals.logger.valueB = output.status;
        LOG_INFO(locals.logger);
    }

    // Governance: Param Voting
    struct VoteGovParams_input
    {
        QRWAGovParams proposal;
    };
    struct VoteGovParams_output
    {
        uint64 status;
    };
    struct VoteGovParams_locals
    {
        uint64 currentBalance;
        uint64 i;
        uint64 foundProposal;
        uint64 proposalIndex;
        QRWALogger logger;
        QRWAGovProposal poll;
        sint64 rawBalance;
        QRWAGovParams existing;
        uint64 status;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(VoteGovParams)
    {
        output.status = QRWA_STATUS_FAILURE_GENERAL;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_GOV_VOTE;
        locals.logger.primaryId = qpi.invocator();

        // Get voter's current QMINE balance
        locals.rawBalance = qpi.numberOfShares(state.mQmineAsset, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator()));
        locals.currentBalance = (locals.rawBalance > 0) ? static_cast<uint64>(locals.rawBalance) : 0;

        if (locals.currentBalance <= 0)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_AUTHORIZED;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Validate proposal percentages
        if (sadd(sadd(input.proposal.electricityPercent, input.proposal.maintenancePercent), input.proposal.reinvestmentPercent) > QRWA_PERCENT_DENOMINATOR)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }
        if (input.proposal.mAdminAddress == NULL_ID)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Now process the new/updated vote
        locals.foundProposal = 0;
        locals.proposalIndex = NULL_INDEX;

        // Check if the current proposal matches any existing unique proposal
        for (locals.i = 0; locals.i < QRWA_MAX_GOV_POLLS; locals.i++)
        {
            locals.existing = state.mGovPolls.get(locals.i).params;
            locals.status = state.mGovPolls.get(locals.i).status;

            if (locals.status == QRWA_POLL_STATUS_ACTIVE &&
                locals.existing.electricityAddress == input.proposal.electricityAddress &&
                locals.existing.maintenanceAddress == input.proposal.maintenanceAddress &&
                locals.existing.reinvestmentAddress == input.proposal.reinvestmentAddress &&
                locals.existing.qmineDevAddress == input.proposal.qmineDevAddress &&
                locals.existing.mAdminAddress == input.proposal.mAdminAddress &&
                locals.existing.electricityPercent == input.proposal.electricityPercent &&
                locals.existing.maintenancePercent == input.proposal.maintenancePercent &&
                locals.existing.reinvestmentPercent == input.proposal.reinvestmentPercent)
            {
                locals.foundProposal = 1;
                locals.proposalIndex = locals.i; // This is the proposal we are voting for
                break;
            }
        }

        // If proposal not found, create it in a new slot
        if (locals.foundProposal == 0)
        {
            if (state.mNewGovPollsThisEpoch >= QRWA_MAX_NEW_GOV_POLLS_PER_EPOCH)
            {
                output.status = QRWA_STATUS_FAILURE_LIMIT_REACHED;
                locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                locals.logger.valueB = output.status;
                LOG_INFO(locals.logger);
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                return;
            }

            locals.proposalIndex = mod(state.mCurrentGovProposalId, QRWA_MAX_GOV_POLLS);

            // Clear old data at this slot
            locals.poll = state.mGovPolls.get(locals.proposalIndex);
            locals.poll.proposalId = state.mCurrentGovProposalId;
            locals.poll.params = input.proposal;
            locals.poll.score = 0; // Will be count at END_EPOCH
            locals.poll.status = QRWA_POLL_STATUS_ACTIVE;

            state.mGovPolls.set(locals.proposalIndex, locals.poll);

            state.mCurrentGovProposalId++;
            state.mNewGovPollsThisEpoch++;
        }

        state.mShareholderVoteMap.set(qpi.invocator(), locals.proposalIndex);
        output.status = QRWA_STATUS_SUCCESS;

        locals.logger.valueA = locals.proposalIndex; // Log the index voted for/added
        locals.logger.valueB = output.status;
        LOG_INFO(locals.logger);
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    }


    // Governance: Asset Release
    struct CreateAssetReleasePoll_input
    {
        id proposalName;
        Asset asset;
        uint64 amount;
        id destination;
    };
    struct CreateAssetReleasePoll_output
    {
        uint64 status;
        uint64 proposalId;
    };
    struct CreateAssetReleasePoll_locals
    {
        uint64 newPollIndex;
        AssetReleaseProposal newPoll;
        QRWALogger logger;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(CreateAssetReleasePoll)
    {
        output.status = QRWA_STATUS_FAILURE_GENERAL;
        output.proposalId = -1;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_ASSET_POLL_CREATED;
        locals.logger.primaryId = qpi.invocator();
        locals.logger.valueA = 0;

        if (qpi.invocator() != state.mCurrentGovParams.mAdminAddress)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_AUTHORIZED;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Check poll limit
        if (state.mNewAssetPollsThisEpoch >= QRWA_MAX_NEW_ASSET_POLLS_PER_EPOCH)
        {
            output.status = QRWA_STATUS_FAILURE_LIMIT_REACHED;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (input.amount == 0 || input.destination == NULL_ID || input.asset.issuer == NULL_ID)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.newPollIndex = mod(state.mCurrentAssetProposalId, QRWA_MAX_ASSET_POLLS);

        // Create and store the new poll, overwriting the oldest one
        locals.newPoll.proposalId = state.mCurrentAssetProposalId;
        locals.newPoll.proposalName = input.proposalName;
        locals.newPoll.asset = input.asset;
        locals.newPoll.amount = input.amount;
        locals.newPoll.destination = input.destination;
        locals.newPoll.status = QRWA_POLL_STATUS_ACTIVE;
        locals.newPoll.votesYes = 0;
        locals.newPoll.votesNo = 0;

        state.mAssetPolls.set(locals.newPollIndex, locals.newPoll);

        output.proposalId = state.mCurrentAssetProposalId;
        output.status = QRWA_STATUS_SUCCESS;
        state.mCurrentAssetProposalId++; // Increment for the next proposal
        state.mNewAssetPollsThisEpoch++;

        locals.logger.valueA = output.proposalId;
        locals.logger.valueB = output.status;
        LOG_INFO(locals.logger);
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    }


    struct VoteAssetRelease_input
    {
        uint64 proposalId;
        uint64 option; /* 0=No, 1=Yes */
    };
    struct VoteAssetRelease_output
    {
        uint64 status;
    };
    struct VoteAssetRelease_locals
    {
        uint64 currentBalance;
        AssetReleaseProposal poll;
        uint64 pollIndex;
        QRWALogger logger;
        uint64 foundPoll;
        bit_64 voterBitfield;
        bit_64 voterOptions;
        sint64 rawBalance;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(VoteAssetRelease)
    {
        output.status = QRWA_STATUS_FAILURE_GENERAL;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_ASSET_VOTE;
        locals.logger.primaryId = qpi.invocator();
        locals.logger.valueA = input.proposalId;
        locals.logger.valueB = input.option;

        if (input.option > 1)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status; // Overwrite valueB with status
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Get voter's current QMINE balance
        locals.rawBalance = qpi.numberOfShares(state.mQmineAsset, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator()));
        locals.currentBalance = (locals.rawBalance > 0) ? static_cast<uint64>(locals.rawBalance) : 0;


        if (locals.currentBalance <= 0)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_AUTHORIZED; // Not a QMINE holder or balance is zero
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Find the poll
        locals.pollIndex = mod(input.proposalId, QRWA_MAX_ASSET_POLLS);
        locals.poll = state.mAssetPolls.get(locals.pollIndex);

        if (locals.poll.proposalId != input.proposalId)
        {
            locals.foundPoll = 0;
        }
        else {
            locals.foundPoll = 1;
        }

        if (locals.foundPoll == 0)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_FOUND;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (locals.poll.status != QRWA_POLL_STATUS_ACTIVE) // Check if poll is active
        {
            output.status = QRWA_STATUS_FAILURE_POLL_INACTIVE;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Now process the new vote
        state.mAssetProposalVoterMap.get(qpi.invocator(), locals.voterBitfield); // Get or default (all 0s)
        state.mAssetVoteOptions.get(qpi.invocator(), locals.voterOptions);

        // Record vote
        locals.voterBitfield.set(locals.pollIndex, 1);
        locals.voterOptions.set(locals.pollIndex, (input.option == 1) ? 1 : 0);

        // Update voter's maps
        state.mAssetProposalVoterMap.set(qpi.invocator(), locals.voterBitfield);
        state.mAssetVoteOptions.set(qpi.invocator(), locals.voterOptions);

        output.status = QRWA_STATUS_SUCCESS;
        locals.logger.valueB = output.status; // Log final status
        LOG_INFO(locals.logger);
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    }

    // deposit general assets
    struct DepositGeneralAsset_input
    {
        Asset asset;
        uint64 amount;
    };
    struct DepositGeneralAsset_output
    {
        uint64 status;
    };
    struct DepositGeneralAsset_locals
    {
        sint64 transferResult;
        sint64 balance;
        uint64 currentAssetBalance;
        QRWALogger logger;
        QRWAAsset wrapper;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(DepositGeneralAsset)
    {
        // This procedure has no fee
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        output.status = QRWA_STATUS_FAILURE_GENERAL;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_ADMIN_ACTION;
        locals.logger.primaryId = qpi.invocator();
        locals.logger.valueA = input.asset.assetName;

        if (qpi.invocator() != state.mCurrentGovParams.mAdminAddress)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_AUTHORIZED;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (input.amount == 0 || input.asset.issuer == NULL_ID)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Check if admin has granted management rights to this SC
        locals.balance = qpi.numberOfShares(input.asset,
            { qpi.invocator(), SELF_INDEX },
            { qpi.invocator(), SELF_INDEX });

        if (locals.balance < static_cast<sint64>(input.amount))
        {
            output.status = QRWA_STATUS_FAILURE_INSUFFICIENT_BALANCE; // Not enough managed shares
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Transfer asset from admin (managed by SELF) to SELF (owned by SELF)
        locals.transferResult = qpi.transferShareOwnershipAndPossession(
            input.asset.assetName,
            input.asset.issuer,
            qpi.invocator(), // current owner
            qpi.invocator(), // current possessor
            input.amount,
            SELF             // new owner and possessor
        );

        if (locals.transferResult >= 0) // Transfer successful
        {
            locals.wrapper.setFrom(input.asset);
            state.mGeneralAssetBalances.get(locals.wrapper, locals.currentAssetBalance); // 0 if not exist
            locals.currentAssetBalance = sadd(locals.currentAssetBalance, input.amount);
            state.mGeneralAssetBalances.set(locals.wrapper, locals.currentAssetBalance);
            output.status = QRWA_STATUS_SUCCESS;
        }
        else
        {
            output.status = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
        }

        locals.logger.valueB = output.status;
        LOG_INFO(locals.logger);
    }

    struct RevokeAssetManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
    };
    struct RevokeAssetManagementRights_output
    {
        sint64 transferredNumberOfShares;
        uint64 status;
    };
    struct RevokeAssetManagementRights_locals
    {
        QRWALogger logger;
        sint64 managedBalance;
        sint64 result;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(RevokeAssetManagementRights)
    {
        // This procedure allows a user to revoke asset management rights from qRWA
        // and transfer them back to QX, which is the default manager for trading
        // Ref: MSVAULT

        output.status = QRWA_STATUS_FAILURE_GENERAL;
        output.transferredNumberOfShares = 0;

        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.primaryId = qpi.invocator();
        locals.logger.valueA = input.asset.assetName;
        locals.logger.valueB = input.numberOfShares;

        if (qpi.invocationReward() < (sint64)QRWA_RELEASE_MANAGEMENT_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.transferredNumberOfShares = 0;
            output.status = QRWA_STATUS_FAILURE_INSUFFICIENT_FEE;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (qpi.invocationReward() > (sint64)QRWA_RELEASE_MANAGEMENT_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)QRWA_RELEASE_MANAGEMENT_FEE);
        }

        // must transfer a positive number of shares.
        if (input.numberOfShares <= 0)
        {
            // Refund the fee if params are invalid
            qpi.transfer(qpi.invocator(), (sint64)QRWA_RELEASE_MANAGEMENT_FEE);
            output.transferredNumberOfShares = 0;
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Check if qRWA actually manages the specified number of shares for the caller.
        locals.managedBalance = qpi.numberOfShares(
            input.asset,
            { qpi.invocator(), SELF_INDEX },
            { qpi.invocator(), SELF_INDEX }
        );

        if (locals.managedBalance < input.numberOfShares)
        {
            // The user is trying to revoke more shares than are managed by qRWA.
            qpi.transfer(qpi.invocator(), (sint64)QRWA_RELEASE_MANAGEMENT_FEE);
            output.transferredNumberOfShares = 0;
            output.status = QRWA_STATUS_FAILURE_INSUFFICIENT_BALANCE;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
        }
        else
        {
            // The balance check passed. Proceed to release the management rights to QX.
            locals.result = qpi.releaseShares(
                input.asset,
                qpi.invocator(), // owner
                qpi.invocator(), // possessor
                input.numberOfShares,
                QX_CONTRACT_INDEX,   // destination ownership managing contract
                QX_CONTRACT_INDEX,   // destination possession managing contract
                QRWA_RELEASE_MANAGEMENT_FEE // offered fee to QX
            );

            if (locals.result < 0)
            {
                // Transfer failed
                output.transferredNumberOfShares = 0;
                output.status = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            }
            else
            {
                // Success, the fee was spent.
                output.transferredNumberOfShares = input.numberOfShares;
                output.status = QRWA_STATUS_SUCCESS;
                locals.logger.logType = QRWA_LOG_TYPE_ADMIN_ACTION; // Or a more specific type

                // Since the invocation reward (100 QU) was added to mRevenuePoolB 
                // via POST_INCOMING_TRANSFER, but we just spent it in releaseShares,
                // we must remove it from the pool to keep the accountant in sync 
                // with the actual balance.
                if (state.mRevenuePoolB >= QRWA_RELEASE_MANAGEMENT_FEE)
                {
                    state.mRevenuePoolB -= QRWA_RELEASE_MANAGEMENT_FEE;
                }
            }
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
        }
    }

    /***************************************************/
    /***************** PUBLIC FUNCTIONS ****************/
    /***************************************************/

    // Governance: Param Voting
    struct GetGovParams_input {};
    struct GetGovParams_output
    {
        QRWAGovParams params;
    };
    PUBLIC_FUNCTION(GetGovParams)
    {
        output.params = state.mCurrentGovParams;
    }

    struct GetGovPoll_input
    {
        uint64 proposalId;
    };
    struct GetGovPoll_output
    {
        QRWAGovProposal proposal;
        uint64 status; // 0=NotFound, 1=Found
    };
    struct GetGovPoll_locals
    {
        uint64 pollIndex;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetGovPoll)
    {
        output.status = QRWA_STATUS_FAILURE_NOT_FOUND;

        locals.pollIndex = mod(input.proposalId, QRWA_MAX_GOV_POLLS);
        output.proposal = state.mGovPolls.get(locals.pollIndex);

        if (output.proposal.proposalId == input.proposalId)
        {
            output.status = QRWA_STATUS_SUCCESS;
        }
        else
        {
            // Clear output if not the poll we're looking for
            setMemory(output.proposal, 0);
        }
    }

    // Governance: Asset Release
    struct GetAssetReleasePoll_input
    {
        uint64 proposalId;
    };
    struct GetAssetReleasePoll_output
    {
        AssetReleaseProposal proposal;
        uint64 status; // 0=NotFound, 1=Found
    };
    struct GetAssetReleasePoll_locals
    {
        uint64 pollIndex;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetAssetReleasePoll)
    {
        output.status = QRWA_STATUS_FAILURE_NOT_FOUND;

        locals.pollIndex = mod(input.proposalId, QRWA_MAX_ASSET_POLLS);
        output.proposal = state.mAssetPolls.get(locals.pollIndex);

        if (output.proposal.proposalId == input.proposalId)
        {
            output.status = QRWA_STATUS_SUCCESS;
        }
        else
        {
            // Clear output if not the poll we're looking for
            setMemory(output.proposal, 0);
        }
    }

    // Balances & Info
    struct GetTreasuryBalance_input {};
    struct GetTreasuryBalance_output
    {
        uint64 balance;
    };
    PUBLIC_FUNCTION(GetTreasuryBalance)
    {
        output.balance = state.mTreasuryBalance;
    }

    struct GetDividendBalances_input {};
    struct GetDividendBalances_output
    {
        uint64 revenuePoolA;
        uint64 revenuePoolB;
        uint64 qmineDividendPool;
        uint64 qrwaDividendPool;
    };
    PUBLIC_FUNCTION(GetDividendBalances)
    {
        output.revenuePoolA = state.mRevenuePoolA;
        output.revenuePoolB = state.mRevenuePoolB;
        output.qmineDividendPool = state.mQmineDividendPool;
        output.qrwaDividendPool = state.mQRWADividendPool;
    }

    struct GetTotalDistributed_input {};
    struct GetTotalDistributed_output
    {
        uint64 totalQmineDistributed;
        uint64 totalQRWADistributed;
    };
    PUBLIC_FUNCTION(GetTotalDistributed)
    {
        output.totalQmineDistributed = state.mTotalQmineDistributed;
        output.totalQRWADistributed = state.mTotalQRWADistributed;
    }

    struct GetActiveAssetReleasePollIds_input {};

    struct GetActiveAssetReleasePollIds_output
    {
        uint64 count;
        Array<uint64, QRWA_MAX_ASSET_POLLS> ids;
    };

    struct GetActiveAssetReleasePollIds_locals
    {
        uint64 i;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetActiveAssetReleasePollIds)
    {
        output.count = 0;
        for (locals.i = 0; locals.i < QRWA_MAX_ASSET_POLLS; locals.i++)
        {
            if (state.mAssetPolls.get(locals.i).status == QRWA_POLL_STATUS_ACTIVE)
            {
                output.ids.set(output.count, state.mAssetPolls.get(locals.i).proposalId);
                output.count++;
            }
        }
    }

    struct GetActiveGovPollIds_input {};
    struct GetActiveGovPollIds_output
    {
        uint64 count;
        Array<uint64, QRWA_MAX_GOV_POLLS> ids;
    };
    struct GetActiveGovPollIds_locals
    {
        uint64 i;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetActiveGovPollIds)
    {
        output.count = 0;
        for (locals.i = 0; locals.i < QRWA_MAX_GOV_POLLS; locals.i++)
        {
            if (state.mGovPolls.get(locals.i).status == QRWA_POLL_STATUS_ACTIVE)
            {
                output.ids.set(output.count, state.mGovPolls.get(locals.i).proposalId);
                output.count++;
            }
        }
    }

    struct GetGeneralAssetBalance_input
    {
        Asset asset;
    };
    struct GetGeneralAssetBalance_output
    {
        uint64 balance;
        uint64 status;
    };
    struct GetGeneralAssetBalance_locals
    {
        uint64 balance;
        QRWAAsset wrapper;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetGeneralAssetBalance) {
        locals.balance = 0;
        locals.wrapper.setFrom(input.asset);
        if (state.mGeneralAssetBalances.get(locals.wrapper, locals.balance)) {
            output.balance = locals.balance;
            output.status = 1;
        }
        else {
            output.balance = 0;
            output.status = 0;
        }
    }

    struct GetGeneralAssets_input {};
    struct GetGeneralAssets_output
    {
        uint64 count;
        Array<Asset, QRWA_MAX_ASSETS> assets;
        Array<uint64, QRWA_MAX_ASSETS> balances;
    };
    struct GetGeneralAssets_locals
    {
        sint64 iterIndex;
        QRWAAsset currentAsset;
        uint64 currentBalance;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetGeneralAssets)
    {
        output.count = 0;
        locals.iterIndex = NULL_INDEX;

        while (true)
        {
            locals.iterIndex = state.mGeneralAssetBalances.nextElementIndex(locals.iterIndex);

            if (locals.iterIndex == NULL_INDEX)
            {
                break;
            }

            locals.currentAsset = state.mGeneralAssetBalances.key(locals.iterIndex);
            locals.currentBalance = state.mGeneralAssetBalances.value(locals.iterIndex);

            // Only return "active" assets (balance > 0)
            if (locals.currentBalance > 0)
            {
                output.assets.set(output.count, locals.currentAsset);
                output.balances.set(output.count, locals.currentBalance);
                output.count++;

                if (output.count >= QRWA_MAX_ASSETS)
                {
                    break;
                }
            }
        }
    }

    /***************************************************/
    /***************** SYSTEM PROCEDURES ***************/
    /***************************************************/

    INITIALIZE()
    {
        // QMINE Asset Constant
        // Issuer: QMINEQQXYBEGBHNSUPOUYDIQKZPCBPQIIHUUZMCPLBPCCAIARVZBTYKGFCWM
        // Name: 297666170193 ("QMINE")
        state.mQmineAsset.assetName = 297666170193ULL;
        state.mQmineAsset.issuer = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );
        state.mTreasuryBalance = 0;
        state.mCurrentAssetProposalId = 0;
        setMemory(state.mLastPayoutTime, 0);

        // Initialize default governance parameters
        state.mCurrentGovParams.mAdminAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        ); // Admin set to QMINE Issuer by default, subject to change via Gov Voting
        state.mCurrentGovParams.electricityAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        ); // Electricity address set to QMINE Issuer by default, subject to change via Gov Voting
        state.mCurrentGovParams.maintenanceAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        ); // Maintenance address set to QMINE Issuer by default, subject to change via Gov Voting
        state.mCurrentGovParams.reinvestmentAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        ); // Reinvestment address set to QMINE Issuer by default, subject to change via Gov Voting

        // QMINE DEV's Address for receiving rewards from moved QMINE tokens
        // ZOXXIDCZIMGCECCFAXDDCMBBXCDAQJIHGOOATAFPSBFIOFOYECFKUFPBEMWC
        state.mCurrentGovParams.qmineDevAddress = ID(
            _Z, _O, _X, _X, _I, _D, _C, _Z, _I, _M, _G, _C, _E, _C, _C, _F,
            _A, _X, _D, _D, _C, _M, _B, _B, _X, _C, _D, _A, _Q, _J, _I, _H,
            _G, _O, _O, _A, _T, _A, _F, _P, _S, _B, _F, _I, _O, _F, _O, _Y,
            _E, _C, _F, _K, _U, _F, _P, _B
        ); // Default QMINE_DEV address
        state.mCurrentGovParams.electricityPercent = 350;
        state.mCurrentGovParams.maintenancePercent = 50;
        state.mCurrentGovParams.reinvestmentPercent = 100;

        state.mCurrentGovProposalId = 0;
        state.mCurrentAssetProposalId = 0;
        state.mNewGovPollsThisEpoch = 0;
        state.mNewAssetPollsThisEpoch = 0;

        // Initialize revenue pools
        state.mRevenuePoolA = 0;
        state.mRevenuePoolB = 0;
        state.mQmineDividendPool = 0;
        state.mQRWADividendPool = 0;

        // Initialize total distributed
        state.mTotalQmineDistributed = 0;
        state.mTotalQRWADistributed = 0;

        // Initialize maps/arrays
        state.mBeginEpochBalances.reset();
        state.mEndEpochBalances.reset();
        state.mPayoutBeginBalances.reset();
        state.mPayoutEndBalances.reset();
        state.mGeneralAssetBalances.reset();
        state.mShareholderVoteMap.reset();
        state.mAssetProposalVoterMap.reset();
        state.mAssetVoteOptions.reset();

        setMemory(state.mAssetPolls, 0);
        setMemory(state.mGovPolls, 0);
    }

    struct BEGIN_EPOCH_locals
    {
        AssetPossessionIterator iter;
        uint64 balance;
        QRWALogger logger;
        id holder;
        uint64 existingBalance;
    };
    BEGIN_EPOCH_WITH_LOCALS()
    {
        // Reset new poll counters
        state.mNewGovPollsThisEpoch = 0;
        state.mNewAssetPollsThisEpoch = 0;

        state.mEndEpochBalances.reset();

        // Take snapshot of begin balances for QMINE holders
        state.mBeginEpochBalances.reset();
        state.mTotalQmineBeginEpoch = 0;

        if (state.mQmineAsset.issuer != NULL_ID)
        {
            for (locals.iter.begin(state.mQmineAsset); !locals.iter.reachedEnd(); locals.iter.next())
            {
                // Exclude SELF (Treasury) from dividend snapshot
                if (locals.iter.possessor() == SELF)
                {
                    continue;
                }
                locals.balance = locals.iter.numberOfPossessedShares();
                locals.holder = locals.iter.possessor();

                if (locals.balance > 0)
                {
                    // Check if holder already exists in the map (e.g. from a different manager)
                    // If so, add to existing balance.
                    locals.existingBalance = 0;
                    state.mBeginEpochBalances.get(locals.holder, locals.existingBalance);

                    locals.balance = sadd(locals.existingBalance, locals.balance);

                    if (state.mBeginEpochBalances.set(locals.holder, locals.balance) != NULL_INDEX)
                    {
                        state.mTotalQmineBeginEpoch = sadd(state.mTotalQmineBeginEpoch, (uint64)locals.iter.numberOfPossessedShares());
                    }
                    else
                    {
                        // Log error - Max holders reached for snapshot
                        locals.logger.contractId = CONTRACT_INDEX;
                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                        locals.logger.primaryId = locals.holder;
                        locals.logger.valueA = 11; // Error code: Begin Epoch Snapshot full
                        locals.logger.valueB = state.mBeginEpochBalances.population();
                        LOG_INFO(locals.logger);
                    }
                }
            }
        }
    }

    struct END_EPOCH_locals
    {
        AssetPossessionIterator iter;
        uint64 balance;

        sint64 iterIndex;
        id iterVoter;
        uint64 beginBalance;
        uint64 endBalance;
        uint64 votingPower;
        uint64 proposalIndex;
        uint64 currentScore;
        bit_64 voterBitfield;
        bit_64 voterOptions;
        uint64 i_asset;

        // Gov Params Voting
        uint64 i;
        uint64 topScore;
        uint64 topProposalIndex;
        uint64 totalQminePower;
        Array<uint64, QRWA_MAX_GOV_POLLS> govPollScores;
        uint64 govPassed;
        uint64 quorumThreshold;

        // Asset Release Voting
        uint64 pollIndex;
        AssetReleaseProposal poll;
        uint64 yesVotes;
        uint64 noVotes;
        uint64 totalVotes;
        uint64 transferSuccess;
        sint64 transferResult;
        uint64 currentAssetBalance;
        Array<uint64, QRWA_MAX_ASSET_POLLS> assetPollVotesYes;
        Array<uint64, QRWA_MAX_ASSET_POLLS> assetPollVotesNo;
        uint64 assetPassed;

        sint64 releaseFeeResult; // For releaseShares fee

        uint64 ownershipTransferred;
        uint64 managementTransferred;
        uint64 feePaid;
        uint64 sufficientFunds;

        QRWALogger logger;
        uint64 epoch;

        sint64 copyIndex;
        id copyHolder;
        uint64 copyBalance;

        QRWAAsset wrapper;

        QRWAGovProposal govPoll;

        id holder;
        uint64 existingBalance;
    };
    END_EPOCH_WITH_LOCALS()
    {
        locals.epoch = qpi.epoch(); // Get current epoch for history records

        // Take snapshot of end balances for QMINE holders
        if (state.mQmineAsset.issuer != NULL_ID)
        {
            for (locals.iter.begin(state.mQmineAsset); !locals.iter.reachedEnd(); locals.iter.next())
            {
                // Exclude SELF (Treasury) from dividend snapshot
                if (locals.iter.possessor() == SELF)
                {
                    continue;
                }
                locals.balance = locals.iter.numberOfPossessedShares();
                locals.holder = locals.iter.possessor();

                if (locals.balance > 0)
                {
                    // Check if holder already exists (multiple SC management)
                    locals.existingBalance = 0;
                    state.mEndEpochBalances.get(locals.holder, locals.existingBalance);

                    locals.balance = sadd(locals.existingBalance, locals.balance);

                    if (state.mEndEpochBalances.set(locals.holder, locals.balance) == NULL_INDEX)
                    {
                        // Log error - Max holders reached for snapshot
                        locals.logger.contractId = CONTRACT_INDEX;
                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                        locals.logger.primaryId = locals.holder;
                        locals.logger.valueA = 12; // Error code: End Epoch Snapshot full
                        locals.logger.valueB = state.mEndEpochBalances.population();
                        LOG_INFO(locals.logger);
                    }
                }
            }
        }

        // Process Governance Parameter Voting (voted by QMINE holders)
        // Recount all votes from scratch using snapshots

        locals.totalQminePower = state.mTotalQmineBeginEpoch;
        locals.govPollScores.setAll(0); // Reset scores to zero.

        locals.iterIndex = NULL_INDEX; // Iterate all voters
        while (true)
        {
            locals.iterIndex = state.mShareholderVoteMap.nextElementIndex(locals.iterIndex);
            if (locals.iterIndex == NULL_INDEX)
            {
                break;
            }

            locals.iterVoter = state.mShareholderVoteMap.key(locals.iterIndex);

            // Get true voting power from snapshots
            locals.beginBalance = 0;
            locals.endBalance = 0;
            state.mBeginEpochBalances.get(locals.iterVoter, locals.beginBalance);
            state.mEndEpochBalances.get(locals.iterVoter, locals.endBalance);

            locals.votingPower = (locals.beginBalance < locals.endBalance) ? locals.beginBalance : locals.endBalance; // min(begin, end)

            if (locals.votingPower > 0) // Apply voting power
            {
                state.mShareholderVoteMap.get(locals.iterVoter, locals.proposalIndex);
                if (locals.proposalIndex < QRWA_MAX_GOV_POLLS)
                {
                    if (state.mGovPolls.get(locals.proposalIndex).status == QRWA_POLL_STATUS_ACTIVE)
                    {
                        locals.currentScore = locals.govPollScores.get(locals.proposalIndex);
                        locals.govPollScores.set(locals.proposalIndex, sadd(locals.currentScore, locals.votingPower));
                    }
                }
            }
        }

        // Find the winning proposal (max votes)
        locals.topScore = 0;
        locals.topProposalIndex = NULL_INDEX;

        for (locals.i = 0; locals.i < QRWA_MAX_GOV_POLLS; locals.i++)
        {
            if (state.mGovPolls.get(locals.i).status == QRWA_POLL_STATUS_ACTIVE)
            {
                locals.currentScore = locals.govPollScores.get(locals.i);
                if (locals.currentScore > locals.topScore)
                {
                    locals.topScore = locals.currentScore;
                    locals.topProposalIndex = locals.i;
                }
            }
        }

        // Calculate 2/3 quorum threshold
        locals.quorumThreshold = 0;
        if (locals.totalQminePower > 0)
        {
            locals.quorumThreshold = div<uint64>(sadd(smul(locals.totalQminePower, 2ULL), 2ULL), 3ULL);
        }

        // Finalize Gov Vote (check against 2/3 quorum)
        locals.govPassed = 0;
        if (locals.topScore >= locals.quorumThreshold && locals.topProposalIndex != NULL_INDEX)
        {
            // Proposal passes
            locals.govPassed = 1;
            state.mCurrentGovParams = state.mGovPolls.get(locals.topProposalIndex).params;

            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_GOV_VOTE;
            locals.logger.primaryId = NULL_ID; // System event
            locals.logger.valueA = state.mGovPolls.get(locals.topProposalIndex).proposalId;
            locals.logger.valueB = QRWA_STATUS_SUCCESS; // Indicate params updated
            LOG_INFO(locals.logger);
        }

        // Update status for all active gov polls (for history)
        for (locals.i = 0; locals.i < QRWA_MAX_GOV_POLLS; locals.i++)
        {
            locals.govPoll = state.mGovPolls.get(locals.i);
            if (locals.govPoll.status == QRWA_POLL_STATUS_ACTIVE)
            {
                locals.govPoll.score = locals.govPollScores.get(locals.i);
                if (locals.govPassed == 1 && locals.i == locals.topProposalIndex)
                {
                    locals.govPoll.status = QRWA_POLL_STATUS_PASSED_EXECUTED;
                }
                else
                {
                    locals.govPoll.status = QRWA_POLL_STATUS_FAILED_VOTE;
                }
                state.mGovPolls.set(locals.i, locals.govPoll);
            }
        }

        // Reset governance voter map for the next epoch
        state.mShareholderVoteMap.reset();

        // --- Process Asset Release Voting ---
        locals.assetPollVotesYes.setAll(0);
        locals.assetPollVotesNo.setAll(0);

        locals.iterIndex = NULL_INDEX;
        while (true)
        {
            locals.iterIndex = state.mAssetProposalVoterMap.nextElementIndex(locals.iterIndex);
            if (locals.iterIndex == NULL_INDEX)
            {
                break;
            }

            locals.iterVoter = state.mAssetProposalVoterMap.key(locals.iterIndex);

            // Get true voting power
            locals.beginBalance = 0;
            locals.endBalance = 0;
            state.mBeginEpochBalances.get(locals.iterVoter, locals.beginBalance);
            state.mEndEpochBalances.get(locals.iterVoter, locals.endBalance);
            locals.votingPower = (locals.beginBalance < locals.endBalance) ? locals.beginBalance : locals.endBalance; // min(begin, end)

            if (locals.votingPower > 0) // Apply voting power
            {
                state.mAssetProposalVoterMap.get(locals.iterVoter, locals.voterBitfield);
                state.mAssetVoteOptions.get(locals.iterVoter, locals.voterOptions);

                for (locals.i_asset = 0; locals.i_asset < QRWA_MAX_ASSET_POLLS; locals.i_asset++)
                {
                    if (state.mAssetPolls.get(locals.i_asset).status == QRWA_POLL_STATUS_ACTIVE &&
                        locals.voterBitfield.get(locals.i_asset) == 1)
                    {
                        if (locals.voterOptions.get(locals.i_asset) == 1) // Voted Yes
                        {
                            locals.yesVotes = locals.assetPollVotesYes.get(locals.i_asset);
                            locals.assetPollVotesYes.set(locals.i_asset, sadd(locals.yesVotes, locals.votingPower));
                        }
                        else // Voted No
                        {
                            locals.noVotes = locals.assetPollVotesNo.get(locals.i_asset);
                            locals.assetPollVotesNo.set(locals.i_asset, sadd(locals.noVotes, locals.votingPower));
                        }
                    }
                }
            }
        }

        // Finalize Asset Polls
        for (locals.pollIndex = 0; locals.pollIndex < QRWA_MAX_ASSET_POLLS; ++locals.pollIndex)
        {
            locals.poll = state.mAssetPolls.get(locals.pollIndex);

            if (locals.poll.status == QRWA_POLL_STATUS_ACTIVE)
            {
                locals.yesVotes = locals.assetPollVotesYes.get(locals.pollIndex);
                locals.noVotes = locals.assetPollVotesNo.get(locals.pollIndex);

                // Store final scores in the poll struct
                locals.poll.votesYes = locals.yesVotes;
                locals.poll.votesNo = locals.noVotes;

                locals.ownershipTransferred = 0;
                locals.managementTransferred = 0;
                locals.feePaid = 0;
                locals.sufficientFunds = 0;


                if (locals.yesVotes >= locals.quorumThreshold) // YES wins
                {
                    // Check if asset is QMINE treasury
                    if (locals.poll.asset.issuer == state.mQmineAsset.issuer && locals.poll.asset.assetName == state.mQmineAsset.assetName)
                    {
                        // Release from treasury
                        if (state.mTreasuryBalance >= locals.poll.amount)
                        {
                            locals.sufficientFunds = 1;
                            locals.transferResult = qpi.transferShareOwnershipAndPossession(
                                state.mQmineAsset.assetName, state.mQmineAsset.issuer,
                                SELF, SELF, locals.poll.amount, locals.poll.destination);

                            if (locals.transferResult >= 0) // ownership transfer succeeded
                            {
                                locals.ownershipTransferred = 1;
                                // Decrement internal balance
                                state.mTreasuryBalance = (state.mTreasuryBalance > locals.poll.amount) ? (state.mTreasuryBalance - locals.poll.amount) : 0;

                                if (state.mRevenuePoolA >= QRWA_RELEASE_MANAGEMENT_FEE)
                                {
                                    // Release management rights from this SC to QX
                                    locals.releaseFeeResult = qpi.releaseShares(
                                        locals.poll.asset,
                                        locals.poll.destination, // new owner
                                        locals.poll.destination, // new possessor
                                        locals.poll.amount,
                                        QX_CONTRACT_INDEX,
                                        QX_CONTRACT_INDEX,
                                        QRWA_RELEASE_MANAGEMENT_FEE
                                    );

                                    if (locals.releaseFeeResult >= 0) // management transfer succeeded
                                    {
                                        locals.managementTransferred = 1;
                                        locals.feePaid = 1;
                                        state.mRevenuePoolA = (state.mRevenuePoolA > (uint64)locals.releaseFeeResult) ? (state.mRevenuePoolA - (uint64)locals.releaseFeeResult) : 0;
                                    }
                                    // else: Management transfer failed (shares are "stuck").
                                    // The destination ID still owns the transferred asset, but the SC management is currently under qRWA.
                                    // The destination ID must use revokeAssetManagementRights to transfer the asset's SC management to QX
                                }
                            }
                        }
                    }
                    else // Asset is from mGeneralAssetBalances
                    {
                        locals.wrapper.setFrom(locals.poll.asset);
                        if (state.mGeneralAssetBalances.get(locals.wrapper, locals.currentAssetBalance))
                        {
                            if (locals.currentAssetBalance >= locals.poll.amount)
                            {
                                locals.sufficientFunds = 1;
                                // Ownership Transfer
                                locals.transferResult = qpi.transferShareOwnershipAndPossession(
                                    locals.poll.asset.assetName, locals.poll.asset.issuer,
                                    SELF, SELF, locals.poll.amount, locals.poll.destination);

                                if (locals.transferResult >= 0) // Ownership transfer tucceeded
                                {
                                    locals.ownershipTransferred = 1;
                                    // Decrement internal balance
                                    locals.currentAssetBalance = (locals.currentAssetBalance > locals.poll.amount) ? (locals.currentAssetBalance - locals.poll.amount) : 0;
                                    state.mGeneralAssetBalances.set(locals.wrapper, locals.currentAssetBalance);

                                    if (state.mRevenuePoolA >= QRWA_RELEASE_MANAGEMENT_FEE)
                                    {
                                        // Management Transfer
                                        locals.releaseFeeResult = qpi.releaseShares(
                                            locals.poll.asset,
                                            locals.poll.destination, // new owner
                                            locals.poll.destination, // new possessor
                                            locals.poll.amount,
                                            QX_CONTRACT_INDEX,
                                            QX_CONTRACT_INDEX,
                                            QRWA_RELEASE_MANAGEMENT_FEE
                                        );

                                        if (locals.releaseFeeResult >= 0) // management transfer succeeded
                                        {
                                            locals.managementTransferred = 1;
                                            locals.feePaid = 1;
                                            state.mRevenuePoolA = (state.mRevenuePoolA > (uint64)locals.releaseFeeResult) ? (state.mRevenuePoolA - (uint64)locals.releaseFeeResult) : 0;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    locals.transferSuccess = locals.ownershipTransferred & locals.managementTransferred & locals.feePaid;
                    if (locals.transferSuccess == 1) // All steps succeeded
                    {
                        locals.logger.logType = QRWA_LOG_TYPE_ASSET_POLL_EXECUTED;
                        locals.logger.valueB = QRWA_STATUS_SUCCESS;
                        locals.poll.status = QRWA_POLL_STATUS_PASSED_EXECUTED;
                    }
                    else
                    {
                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                        if (locals.sufficientFunds == 0)
                        {
                            locals.logger.valueB = QRWA_STATUS_FAILURE_INSUFFICIENT_BALANCE;
                        }
                        else if (locals.ownershipTransferred == 0)
                        {
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                        }
                        else
                        {
                            // This is the stuck shares case
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                        }
                        locals.poll.status = QRWA_POLL_STATUS_PASSED_FAILED_EXECUTION;
                    }
                    locals.logger.contractId = CONTRACT_INDEX;
                    locals.logger.primaryId = locals.poll.destination;
                    locals.logger.valueA = locals.poll.proposalId;
                    LOG_INFO(locals.logger);
                }
                else // Vote failed (NO wins or < quorum)
                {
                    locals.poll.status = QRWA_POLL_STATUS_FAILED_VOTE;
                }
                state.mAssetPolls.set(locals.pollIndex, locals.poll);
            }
        }
        // Reset voter tracking map for asset polls
        state.mAssetProposalVoterMap.reset();
        state.mAssetVoteOptions.reset();

        // Copy the finalized epoch snapshots to the payout buffers
        state.mPayoutBeginBalances.reset();
        state.mPayoutEndBalances.reset();
        state.mPayoutTotalQmineBegin = state.mTotalQmineBeginEpoch;

        // Copy mBeginEpochBalances -> mPayoutBeginBalances
        locals.copyIndex = NULL_INDEX;
        while (true)
        {
            locals.copyIndex = state.mBeginEpochBalances.nextElementIndex(locals.copyIndex);
            if (locals.copyIndex == NULL_INDEX)
            {
                break;
            }
            locals.copyHolder = state.mBeginEpochBalances.key(locals.copyIndex);
            locals.copyBalance = state.mBeginEpochBalances.value(locals.copyIndex);
            state.mPayoutBeginBalances.set(locals.copyHolder, locals.copyBalance);
        }

        // Copy mEndEpochBalances -> mPayoutEndBalances
        locals.copyIndex = NULL_INDEX;
        while (true)
        {
            locals.copyIndex = state.mEndEpochBalances.nextElementIndex(locals.copyIndex);
            if (locals.copyIndex == NULL_INDEX)
            {
                break;
            }
            locals.copyHolder = state.mEndEpochBalances.key(locals.copyIndex);
            locals.copyBalance = state.mEndEpochBalances.value(locals.copyIndex);
            state.mPayoutEndBalances.set(locals.copyHolder, locals.copyBalance);
        }
    }


    struct END_TICK_locals
    {
        DateAndTime now;
        uint64 durationMicros;
        uint64 msSinceLastPayout;

        uint64 totalGovPercent;
        uint64 totalFeeAmount;
        uint64 electricityPayout;
        uint64 maintenancePayout;
        uint64 reinvestmentPayout;
        uint64 Y_revenue;
        uint64 totalDistribution;
        uint64 qminePayout;
        uint64 qrwaPayout;
        uint64 amountPerQRWAShare;
        uint64 distributedAmount;

        sint64 qminePayoutIndex;
        id holder;
        uint64 beginBalance;
        uint64 endBalance;
        uint64 eligibleBalance;
        // Use uint128 for all payout accounting
        uint128 scaledPayout_128;
        uint128 eligiblePayout_128;
        uint128 totalEligiblePaid_128;
        uint128 movedSharesPayout_128;
        uint128 qmineDividendPool_128;
        uint64 payout_u64;
        uint64 foundEnd;
        QRWALogger logger;
    };
    END_TICK_WITH_LOCALS()
    {
        locals.now = qpi.now();

        // Check payout conditions: Correct day, correct hour, and enough time passed
        if (qpi.dayOfWeek((uint8)mod(locals.now.getYear(), (uint16)100), locals.now.getMonth(), locals.now.getDay()) == QRWA_PAYOUT_DAY &&
            locals.now.getHour() == QRWA_PAYOUT_HOUR)
        {
            // check if mLastPayoutTime is 0 (never initialized)
            if (state.mLastPayoutTime.getYear() == 0)
            {
                // If never paid, treat as if enough time has passed
                locals.msSinceLastPayout = QRWA_MIN_PAYOUT_INTERVAL_MS;
            }
            else
            {
                locals.durationMicros = state.mLastPayoutTime.durationMicrosec(locals.now);

                if (locals.durationMicros != UINT64_MAX)
                {
                    locals.msSinceLastPayout = div<uint64>(locals.durationMicros, 1000);
                }
                else
                {
                    // If it's invalid but NOT zero, something is wrong, so we prevent payout
                    locals.msSinceLastPayout = 0;
                }
            }

            if (locals.msSinceLastPayout >= QRWA_MIN_PAYOUT_INTERVAL_MS)
            {
                locals.logger.contractId = CONTRACT_INDEX;
                locals.logger.logType = QRWA_LOG_TYPE_DISTRIBUTION;

                // Calculate and pay out governance fees from Pool A (mined funds)
                // gov_percentage = electricity_percent + maintenance_percent + reinvestment_percent
                locals.totalGovPercent = sadd(sadd(state.mCurrentGovParams.electricityPercent, state.mCurrentGovParams.maintenancePercent), state.mCurrentGovParams.reinvestmentPercent);
                locals.totalFeeAmount = 0;

                if (locals.totalGovPercent > 0 && locals.totalGovPercent <= QRWA_PERCENT_DENOMINATOR && state.mRevenuePoolA > 0)
                {
                    locals.electricityPayout = div<uint64>(smul(state.mRevenuePoolA, state.mCurrentGovParams.electricityPercent), QRWA_PERCENT_DENOMINATOR);
                    if (locals.electricityPayout > 0 && state.mCurrentGovParams.electricityAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.mCurrentGovParams.electricityAddress, locals.electricityPayout) >= 0)
                        {
                            locals.totalFeeAmount = sadd(locals.totalFeeAmount, locals.electricityPayout);
                        }
                        else
                        {
                            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                            locals.logger.primaryId = state.mCurrentGovParams.electricityAddress;
                            locals.logger.valueA = locals.electricityPayout;
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                            LOG_INFO(locals.logger);
                        }
                    }

                    locals.maintenancePayout = div<uint64>(smul(state.mRevenuePoolA, state.mCurrentGovParams.maintenancePercent), QRWA_PERCENT_DENOMINATOR);
                    if (locals.maintenancePayout > 0 && state.mCurrentGovParams.maintenanceAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.mCurrentGovParams.maintenanceAddress, locals.maintenancePayout) >= 0)
                        {
                            locals.totalFeeAmount = sadd(locals.totalFeeAmount, locals.maintenancePayout);
                        }
                        else
                        {
                            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                            locals.logger.primaryId = state.mCurrentGovParams.maintenanceAddress;
                            locals.logger.valueA = locals.maintenancePayout;
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                            LOG_INFO(locals.logger);
                        }
                    }

                    locals.reinvestmentPayout = div<uint64>(smul(state.mRevenuePoolA, state.mCurrentGovParams.reinvestmentPercent), QRWA_PERCENT_DENOMINATOR);
                    if (locals.reinvestmentPayout > 0 && state.mCurrentGovParams.reinvestmentAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.mCurrentGovParams.reinvestmentAddress, locals.reinvestmentPayout) >= 0)
                        {
                            locals.totalFeeAmount = sadd(locals.totalFeeAmount, locals.reinvestmentPayout);
                        }
                        else
                        {
                            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                            locals.logger.primaryId = state.mCurrentGovParams.reinvestmentAddress;
                            locals.logger.valueA = locals.reinvestmentPayout;
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                            LOG_INFO(locals.logger);
                        }
                    }
                    state.mRevenuePoolA = (state.mRevenuePoolA > locals.totalFeeAmount) ? (state.mRevenuePoolA - locals.totalFeeAmount) : 0;
                }

                // Calculate total distribution pool
                locals.Y_revenue = state.mRevenuePoolA; // Remaining Pool A after fees
                locals.totalDistribution = sadd(locals.Y_revenue, state.mRevenuePoolB);

                // Allocate to QMINE and qRWA pools
                if (locals.totalDistribution > 0)
                {
                    locals.qminePayout = div<uint64>(smul(locals.totalDistribution, QRWA_QMINE_HOLDER_PERCENT), QRWA_PERCENT_DENOMINATOR);
                    locals.qrwaPayout = locals.totalDistribution - locals.qminePayout; // Avoid potential rounding errors

                    state.mQmineDividendPool = sadd(state.mQmineDividendPool, locals.qminePayout);
                    state.mQRWADividendPool = sadd(state.mQRWADividendPool, locals.qrwaPayout);

                    // Reset revenue pools after allocation
                    state.mRevenuePoolA = 0;
                    state.mRevenuePoolB = 0;
                }

                // Distribute QMINE rewards
                if (state.mQmineDividendPool > 0 && state.mPayoutTotalQmineBegin > 0)
                {
                    locals.totalEligiblePaid_128 = 0;
                    locals.qminePayoutIndex = NULL_INDEX; // Start iteration
                    locals.qmineDividendPool_128 = state.mQmineDividendPool; // Create 128-bit copy for accounting

                    // pay eligible holders
                    while (true)
                    {
                        locals.qminePayoutIndex = state.mPayoutBeginBalances.nextElementIndex(locals.qminePayoutIndex);
                        if (locals.qminePayoutIndex == NULL_INDEX)
                        {
                            break;
                        }

                        locals.holder = state.mPayoutBeginBalances.key(locals.qminePayoutIndex);
                        locals.beginBalance = state.mPayoutBeginBalances.value(locals.qminePayoutIndex);

                        locals.foundEnd = state.mPayoutEndBalances.get(locals.holder, locals.endBalance) ? 1 : 0;
                        if (locals.foundEnd == 0)
                        {
                            locals.endBalance = 0;
                        }

                        locals.eligibleBalance = (locals.beginBalance < locals.endBalance) ? locals.beginBalance : locals.endBalance;

                        if (locals.eligibleBalance > 0)
                        {
                            // Payout = (EligibleBalance * DividendPool) / PayoutBase
                            locals.scaledPayout_128 = (uint128)locals.eligibleBalance * (uint128)state.mQmineDividendPool;
                            locals.eligiblePayout_128 = div<uint128>(locals.scaledPayout_128, state.mPayoutTotalQmineBegin);

                            if (locals.eligiblePayout_128 > (uint128)0 && locals.eligiblePayout_128 <= locals.qmineDividendPool_128)
                            {
                                // Cast to uint64 ONLY at the moment of transfer
                                locals.payout_u64 = locals.eligiblePayout_128.low;

                                // Check if the cast truncated the value (if high part was set)
                                if (locals.eligiblePayout_128.high == 0 && locals.payout_u64 > 0)
                                {
                                    if (qpi.transfer(locals.holder, (sint64)locals.payout_u64) >= 0)
                                    {
                                        locals.qmineDividendPool_128 -= locals.eligiblePayout_128;
                                        state.mTotalQmineDistributed = sadd(state.mTotalQmineDistributed, locals.payout_u64);
                                        locals.totalEligiblePaid_128 += locals.eligiblePayout_128;
                                    }
                                    else
                                    {
                                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                                        locals.logger.primaryId = locals.holder;
                                        locals.logger.valueA = locals.payout_u64;
                                        locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                                        LOG_INFO(locals.logger);
                                    }
                                }
                            }
                            else if (locals.eligiblePayout_128 > locals.qmineDividendPool_128)
                            {
                                // Payout is larger than the remaining pool
                                locals.payout_u64 = locals.qmineDividendPool_128.low; // Get remaining pool

                                if (locals.qmineDividendPool_128.high == 0 && locals.payout_u64 > 0)
                                {
                                    if (qpi.transfer(locals.holder, (sint64)locals.payout_u64) >= 0)
                                    {
                                        state.mTotalQmineDistributed = sadd(state.mTotalQmineDistributed, locals.payout_u64);
                                        locals.totalEligiblePaid_128 += locals.qmineDividendPool_128;
                                        locals.qmineDividendPool_128 = 0; // Pool exhausted
                                    }
                                    else
                                    {
                                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                                        locals.logger.primaryId = locals.holder;
                                        locals.logger.valueA = locals.payout_u64;
                                        locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                                        LOG_INFO(locals.logger);
                                    }
                                }
                                break;
                            }
                        }
                    }

                    // Pay QMINE DEV the entire remainder of the pool
                    locals.movedSharesPayout_128 = locals.qmineDividendPool_128;
                    if (locals.movedSharesPayout_128 > (uint128)0 && state.mCurrentGovParams.qmineDevAddress != NULL_ID)
                    {
                        locals.payout_u64 = locals.movedSharesPayout_128.low;
                        if (locals.movedSharesPayout_128.high == 0 && locals.payout_u64 > 0)
                        {
                            if (qpi.transfer(state.mCurrentGovParams.qmineDevAddress, (sint64)locals.payout_u64) >= 0)
                            {
                                state.mTotalQmineDistributed = sadd(state.mTotalQmineDistributed, locals.payout_u64);
                                locals.qmineDividendPool_128 = 0;
                            }
                            else
                            {
                                locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                                locals.logger.primaryId = state.mCurrentGovParams.qmineDevAddress;
                                locals.logger.valueA = locals.payout_u64;
                                locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                                LOG_INFO(locals.logger);
                            }
                        }
                    }

                    // Update the 64-bit state variable from the 128-bit local
                    // If transfers failed, funds remain in qmineDividendPool_128 and will be preserved here.
                    state.mQmineDividendPool = locals.qmineDividendPool_128.low;
                } // End QMINE distribution

                // Distribute qRWA shareholder rewards
                if (state.mQRWADividendPool > 0)
                {
                    locals.amountPerQRWAShare = div<uint64>(state.mQRWADividendPool, NUMBER_OF_COMPUTORS);
                    if (locals.amountPerQRWAShare > 0)
                    {
                        if (qpi.distributeDividends(static_cast<sint64>(locals.amountPerQRWAShare)))
                        {
                            locals.distributedAmount = smul(locals.amountPerQRWAShare, static_cast<uint64>(NUMBER_OF_COMPUTORS));
                            state.mQRWADividendPool -= locals.distributedAmount;
                            state.mTotalQRWADistributed = sadd(state.mTotalQRWADistributed, locals.distributedAmount);
                        }
                    }
                }

                // Update last payout time
                state.mLastPayoutTime = qpi.now();
                locals.logger.logType = QRWA_LOG_TYPE_DISTRIBUTION;
                locals.logger.primaryId = NULL_ID;
                locals.logger.valueA = 1; // Indicate success
                locals.logger.valueB = 0;
                LOG_INFO(locals.logger);
            }
        }
    }

    struct POST_INCOMING_TRANSFER_locals
    {
        QRWALogger logger;
    };
    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        // Differentiate revenue streams based on source type
        if (input.sourceId.u64._1 == 0 && input.sourceId.u64._2 == 0 && input.sourceId.u64._3 == 0 && input.sourceId.u64._0 != 0)
        {
            // Source is likely a contract (e.g., QX transfer) -> Pool A
            state.mRevenuePoolA = sadd(state.mRevenuePoolA, static_cast<uint64>(input.amount));
            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_INCOMING_REVENUE_A;
            locals.logger.primaryId = input.sourceId;
            locals.logger.valueA = input.amount;
            locals.logger.valueB = input.type;
            LOG_INFO(locals.logger);
        }
        else if (input.sourceId != NULL_ID)
        {
            // Source is likely a user (EOA) -> Pool B
            state.mRevenuePoolB = sadd(state.mRevenuePoolB, static_cast<uint64>(input.amount));
            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_INCOMING_REVENUE_B;
            locals.logger.primaryId = input.sourceId;
            locals.logger.valueA = input.amount;
            locals.logger.valueB = input.type;
            LOG_INFO(locals.logger);
        }
    }

    PRE_ACQUIRE_SHARES()
    {
        // Allow any entity to transfer asset management rights to this contract
        output.requestedFee = 0;
        output.allowTransfer = true;
    }

    POST_ACQUIRE_SHARES()
    {
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        // PROCEDURES
        REGISTER_USER_PROCEDURE(DonateToTreasury, 3);
        REGISTER_USER_PROCEDURE(VoteGovParams, 4);
        REGISTER_USER_PROCEDURE(CreateAssetReleasePoll, 5);
        REGISTER_USER_PROCEDURE(VoteAssetRelease, 6);
        REGISTER_USER_PROCEDURE(DepositGeneralAsset, 7);
        REGISTER_USER_PROCEDURE(RevokeAssetManagementRights, 8);

        // FUNCTIONS
        REGISTER_USER_FUNCTION(GetGovParams, 1);
        REGISTER_USER_FUNCTION(GetGovPoll, 2);
        REGISTER_USER_FUNCTION(GetAssetReleasePoll, 3);
        REGISTER_USER_FUNCTION(GetTreasuryBalance, 4);
        REGISTER_USER_FUNCTION(GetDividendBalances, 5);
        REGISTER_USER_FUNCTION(GetTotalDistributed, 6);
        REGISTER_USER_FUNCTION(GetActiveAssetReleasePollIds, 7);
        REGISTER_USER_FUNCTION(GetActiveGovPollIds, 8);
        REGISTER_USER_FUNCTION(GetGeneralAssetBalance, 9);
        REGISTER_USER_FUNCTION(GetGeneralAssets, 10);
    }
};
