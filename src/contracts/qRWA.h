using namespace QPI;

/***************************************************/
/******************* CONSTANTS *********************/
/***************************************************/

constexpr uint64 QRWA_MAX_QMINE_HOLDERS = 131072 * X_MULTIPLIER; // 2^17 = 128K unique holders max (563MB → 37MB state)
constexpr uint64 QRWA_MAX_GOV_POLLS = 64; // 8 active polls * 8 epochs = 64 slots

constexpr uint64 QRWA_MAX_ASSETS = 1024; // 2^10

constexpr uint64 QRWA_MAX_NEW_GOV_POLLS_PER_EPOCH = 8;


// Dividend percentage constants
constexpr uint64 QRWA_QMINE_HOLDER_PERCENT = 900; // 90.0%
constexpr uint64 QRWA_QRWA_HOLDER_PERCENT = 100;  // 10.0%
constexpr uint64 QRWA_PERCENT_DENOMINATOR = 1000; // 100.0%
constexpr uint64 QRWA_QMINE_PER_QRWA_SHARE_MIN = 100000ULL;
constexpr uint64 QRWA_CONTRACT_ASSET_NAME = 1096241745ULL; // assetNameFromString("QRWA")

// Payout Timing Constants
constexpr uint32 QRWA_USE_TICK_BASED_PAYOUT = 0;     // 0=UTC-based | 1=tick-based
constexpr uint32 QRWA_PAYOUT_TICK_INTERVAL = 10;     // Every 10 ticks (only when tick-based)

// UTC-based constants
constexpr uint64 QRWA_PAYOUT_DAY = FRIDAY;
constexpr uint64 QRWA_PAYOUT_HOUR_POOL_A = 12;      // Qubic Mining  | Friday 12:00 UTC
constexpr uint64 QRWA_PAYOUT_HOUR_POOL_B = 14;      // SC Assets     | Friday 14:00 UTC
constexpr uint64 QRWA_PAYOUT_HOUR_POOL_C = 13;      // BTC Mining    | Friday 13:00 UTC
constexpr uint64 QRWA_PAYOUT_HOUR_POOL_D = 15;      // MLM Water     | Friday 15:00 UTC
constexpr uint64 QRWA_PAYOUT_MINUTE = 0;             // Trigger at :00
constexpr uint64 QRWA_CHECK_DAY_OF_WEEK = 1;         // 1=Friday only
constexpr uint64 QRWA_MIN_PAYOUT_INTERVAL_MS = 518400000ULL; // 6 days (6 * 86400000)
constexpr uint64 QRWA_MIN_REVENUE_THRESHOLD = 1000ULL; // skip distribution if pool revenue below this

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


// QX Fee for releasing management rights
constexpr sint64 QRWA_RELEASE_MANAGEMENT_FEE = 100;

// LOG TYPES
constexpr uint64 QRWA_LOG_TYPE_DISTRIBUTION = 1;
constexpr uint64 QRWA_LOG_TYPE_GOV_VOTE = 2;

constexpr uint64 QRWA_LOG_TYPE_TREASURY_DONATION = 6;
constexpr uint64 QRWA_LOG_TYPE_ADMIN_ACTION = 7;
constexpr uint64 QRWA_LOG_TYPE_ERROR = 8;
constexpr uint64 QRWA_LOG_TYPE_INCOMING_REVENUE_A = 9;
constexpr uint64 QRWA_LOG_TYPE_INCOMING_REVENUE_B = 10;
constexpr uint64 QRWA_LOG_TYPE_INCOMING_REVENUE_DEDICATED = 11;
constexpr uint64 QRWA_LOG_TYPE_PAYOUT_QMINE_HOLDER = 12; // valueA=amount, valueB=eligible QMINE count
constexpr uint64 QRWA_LOG_TYPE_PAYOUT_QRWA_HOLDER = 13; // valueA=amount, valueB=qRWA shares
constexpr uint64 QRWA_LOG_TYPE_PAYOUT_DEDICATED_QRWA = 14; // valueA=amount, valueB=qRWA shares (Pool C leg)
constexpr uint64 QRWA_LOG_TYPE_INCOMING_SC_DIVIDEND = 15; // SC dividend received → Pool B; valueA=amount, valueB=cumulative
constexpr uint64 QRWA_LOG_TYPE_INCOMING_REVENUE_D = 16;
constexpr uint64 QRWA_LOG_TYPE_PAYOUT_POOL_D_QRWA = 17; // valueA=amount, valueB=qRWA shares (Pool D leg)
constexpr uint64 QRWA_POOL_D_REINVESTMENT_PERCENT = 100; // 10.0% of Pool D revenue goes to reinvestment address

// Ring buffer for tracking the last N individual payouts (queryable via GetLatestPayouts = fn 11)
constexpr uint64 QRWA_PAYOUT_RING_SIZE = 16384; // Must be a power of 2
constexpr uint8 QRWA_PAYOUT_TYPE_QMINE_HOLDER    = 0; // Regular QMINE holder payout
constexpr uint8 QRWA_PAYOUT_TYPE_QMINE_DEV       = 1; // Dev address gets reducer's portion
constexpr uint8 QRWA_PAYOUT_TYPE_QRWA_HOLDER     = 2; // qRWA shareholder (Pool B)
constexpr uint8 QRWA_PAYOUT_TYPE_DEDICATED_QRWA  = 3; // Pool C (BTC Mining) dedicated qRWA holder leg
constexpr uint8 QRWA_PAYOUT_TYPE_POOL_D_QRWA     = 4; // Pool D (MLM Water) dedicated qRWA holder leg
constexpr uint64 QRWA_PAYOUT_PAGE_SIZE = 512;




/***************************************************/
/**************** CONTRACT STATE *******************/
/***************************************************/

struct QRWA2
{
};

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
        id qmineDevAddress; // Address to receive rewards for moved QMINE during epoch (NOT changeable via voting)

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

    // Single entry in the per-pool payout ring buffers (mPayoutsPoolA, mPayoutsPoolB, mPayoutsPoolC).
    struct QRWAPayoutEntry
    {
        id recipient;          // Who received the payment
        uint64 amount;         // Amount in QU
        uint64 qmineHolding;   // Recipient's QMINE shares at payout time
        uint64 qrwaHolding;    // Recipient's qRWA shares at payout time
        uint32 tick;           // Network tick of the payout
        uint16 epoch;          // Epoch of the payout
        uint8 payoutType;      // QRWA_PAYOUT_TYPE_* constant
        uint8 _pad0;
    };



    // All persistent state fields must be declared inside StateData.
    // Access state with state.get().field (read) and state.mut().field (write).
    // state.mut() marks the contract state as dirty for automatic change detection.
    struct StateData
    {
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



        // Treasury & Asset Release
        uint64 mTreasuryBalance; // QMINE token balance holds by SC
        HashMap<QRWAAsset, uint64, QRWA_MAX_ASSETS> mGeneralAssetBalances; // Balances for other assets (e.g., SC shares)
        HashMap<id, uint64, QRWA_MAX_ASSETS> mScDividendTracker; // SC contract ID → cumulative dividends received (routed to Pool B)

        // Per-pool payout timestamps (UTC time-based, used when QRWA_USE_TICK_BASED_PAYOUT == 0)
        DateAndTime mLastPayoutTimePoolA; // Pool A (Qubic Mining)
        DateAndTime mLastPayoutTimePoolB; // Pool B (SC Assets)
        DateAndTime mLastPayoutTimePoolC; // Pool C (BTC Mining)
        DateAndTime mLastPayoutTimePoolD; // Pool D (MLM Water)

        // Per-pool payout tick tracking (used when QRWA_USE_TICK_BASED_PAYOUT == 1)
        uint32 mLastPayoutTickPoolA;
        uint32 mLastPayoutTickPoolB;
        uint32 mLastPayoutTickPoolC;
        uint32 mLastPayoutTickPoolD;
        uint32 _paddingTick; // Alignment padding

        // Revenue Pools (incoming, before splitting into QMINE/qRWA)
        uint64 mRevenuePoolA; // Mined funds from Qubic farm (from SCs) — gov fees deducted first
        uint64 mRevenuePoolB; // Other dividend funds (from user wallets) — no gov fees
        uint64 mDedicatedRevenuePool; // Pool C (BTC Mining) revenue from dedicated address
        uint64 mPoolDRevenuePool; // Pool D (MLM Water) revenue from dedicated address

        // Per-pool dividend sub-pools (populated from revenue, split 90% QMINE / 10% qRWA)
        uint64 mPoolAQmineDividend;    // 90% of Pool A revenue (after gov fees)
        uint64 mPoolAQrwaDividend;     // 10% of Pool A revenue (after gov fees)
        uint64 mPoolBQmineDividend;    // 90% of Pool B revenue (no gov fees)
        uint64 mPoolBQrwaDividend;     // 10% of Pool B revenue (no gov fees)
        uint64 mPoolCQmineDividend;    // 90% of Pool C revenue
        uint64 mPoolCQrwaDividend;     // 10% of Pool C revenue (dedicated, requires >= 100K QMINE/share)
        uint64 mPoolDQmineDividend;    // 80% of Pool D revenue (after 10% reinvestment)
        uint64 mPoolDQrwaDividend;     // 10% of Pool D revenue (dedicated, requires >= 100K QMINE/share)

        // Pool C (BTC Mining) revenue configuration
        id mDedicatedRevenueAddress;

        // Pool D (MLM Water) revenue configuration
        id mPoolDRevenueAddress;

        // Pool A revenue address (QMINE issuer or configured mining address)
        id mPoolARevenueAddress;

        // Fundraising address — excluded from ALL distributions
        id mFundraisingAddress;

        // Exchange address (safe.trade) — excluded from ALL distributions
        id mExchangeAddress;

        // Per-pool total distributed tracking
        uint64 mTotalPoolADistributed;
        uint64 mTotalPoolBDistributed;
        uint64 mTotalPoolCDistributed;
        uint64 mTotalPoolDDistributed;

        // Per-pool ring buffers (one per pool, each contains all payout types for that pool)
        Array<QRWAPayoutEntry, QRWA_PAYOUT_RING_SIZE> mPayoutsPoolA;   // Pool A: QMINE + qRWA payouts
        uint16 mPayoutsPoolANextIdx;
        Array<QRWAPayoutEntry, QRWA_PAYOUT_RING_SIZE> mPayoutsPoolB;   // Pool B: QMINE + qRWA payouts
        uint16 mPayoutsPoolBNextIdx;
        Array<QRWAPayoutEntry, QRWA_PAYOUT_RING_SIZE> mPayoutsPoolC;   // Pool C: QMINE + dedicated qRWA payouts
        uint16 mPayoutsPoolCNextIdx;
        Array<QRWAPayoutEntry, QRWA_PAYOUT_RING_SIZE> mPayoutsPoolD;   // Pool D: QMINE + dedicated qRWA payouts (MLM Water)
        uint16 mPayoutsPoolDNextIdx;
    }; // end StateData



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

        if (state.get().mQmineAsset.issuer == NULL_ID)
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
        locals.balance = qpi.numberOfShares(state.get().mQmineAsset,
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
            state.get().mQmineAsset.assetName,
            state.get().mQmineAsset.issuer,
            qpi.invocator(), // current owner
            qpi.invocator(), // current possessor
            input.amount,
            SELF             // new owner and possessor
        );

        if (locals.transferResult >= 0) // Transfer successful
        {
            state.mut().mTreasuryBalance = sadd(state.get().mTreasuryBalance, input.amount);
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
        locals.rawBalance = qpi.numberOfShares(state.get().mQmineAsset, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator()));
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
            locals.existing = state.get().mGovPolls.get(locals.i).params;
            locals.status = state.get().mGovPolls.get(locals.i).status;

            if (locals.status == QRWA_POLL_STATUS_ACTIVE &&
                locals.existing.electricityAddress == input.proposal.electricityAddress &&
                locals.existing.maintenanceAddress == input.proposal.maintenanceAddress &&
                locals.existing.reinvestmentAddress == input.proposal.reinvestmentAddress &&
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
            if (state.get().mNewGovPollsThisEpoch >= QRWA_MAX_NEW_GOV_POLLS_PER_EPOCH)
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

            locals.proposalIndex = mod(state.get().mCurrentGovProposalId, QRWA_MAX_GOV_POLLS);

            // Clear old data at this slot
            locals.poll = state.get().mGovPolls.get(locals.proposalIndex);
            locals.poll.proposalId = state.get().mCurrentGovProposalId;
            locals.poll.params = input.proposal;
            locals.poll.score = 0; // Will be count at END_EPOCH
            locals.poll.status = QRWA_POLL_STATUS_ACTIVE;

            state.mut().mGovPolls.set(locals.proposalIndex, locals.poll);

            state.mut().mCurrentGovProposalId++;
            state.mut().mNewGovPollsThisEpoch++;
        }

        state.mut().mShareholderVoteMap.set(qpi.invocator(), locals.proposalIndex);
        output.status = QRWA_STATUS_SUCCESS;

        locals.logger.valueA = locals.proposalIndex; // Log the index voted for/added
        locals.logger.valueB = output.status;
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

        if (qpi.invocator() != state.get().mCurrentGovParams.mAdminAddress)
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
            state.get().mGeneralAssetBalances.get(locals.wrapper, locals.currentAssetBalance); // 0 if not exist
            locals.currentAssetBalance = sadd(locals.currentAssetBalance, input.amount);
            state.mut().mGeneralAssetBalances.set(locals.wrapper, locals.currentAssetBalance);
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
                if (state.get().mRevenuePoolB >= QRWA_RELEASE_MANAGEMENT_FEE)
                {
                    state.mut().mRevenuePoolB -= QRWA_RELEASE_MANAGEMENT_FEE;
                }
            }
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
        }
    }

    // ── SetPoolARevenueAddress: Admin-only setter for mPoolARevenueAddress ──
    struct SetPoolARevenueAddress_input
    {
        id newAddress;
    };
    struct SetPoolARevenueAddress_output
    {
        uint64 status;
    };
    struct SetPoolARevenueAddress_locals
    {
        QRWALogger logger;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(SetPoolARevenueAddress)
    {
        output.status = QRWA_STATUS_FAILURE_GENERAL;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_ADMIN_ACTION;
        locals.logger.primaryId = qpi.invocator();

        // Refund invocation reward
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        // Admin-only check
        if (qpi.invocator() != state.get().mCurrentGovParams.mAdminAddress)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_AUTHORIZED;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Validate new address is not NULL
        if (input.newAddress == NULL_ID)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        // Set the new Pool A revenue address
        state.mut().mPoolARevenueAddress = input.newAddress;
        output.status = QRWA_STATUS_SUCCESS;
        locals.logger.valueA = 1; // signals address was set
        locals.logger.valueB = output.status;
        LOG_INFO(locals.logger);
    }

    // ── SetPoolDRevenueAddress: Admin-only setter for mPoolDRevenueAddress ──
    struct SetPoolDRevenueAddress_input
    {
        id newAddress;
    };
    struct SetPoolDRevenueAddress_output
    {
        uint64 status;
    };
    struct SetPoolDRevenueAddress_locals
    {
        QRWALogger logger;
    };
    PUBLIC_PROCEDURE_WITH_LOCALS(SetPoolDRevenueAddress)
    {
        output.status = QRWA_STATUS_FAILURE_GENERAL;
        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_ADMIN_ACTION;
        locals.logger.primaryId = qpi.invocator();

        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (qpi.invocator() != state.get().mCurrentGovParams.mAdminAddress)
        {
            output.status = QRWA_STATUS_FAILURE_NOT_AUTHORIZED;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        if (input.newAddress == NULL_ID)
        {
            output.status = QRWA_STATUS_FAILURE_INVALID_INPUT;
            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
            locals.logger.valueB = output.status;
            LOG_INFO(locals.logger);
            return;
        }

        state.mut().mPoolDRevenueAddress = input.newAddress;
        output.status = QRWA_STATUS_SUCCESS;
        locals.logger.valueA = 1;
        locals.logger.valueB = output.status;
        LOG_INFO(locals.logger);
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
        output.params = state.get().mCurrentGovParams;
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
        output.proposal = state.get().mGovPolls.get(locals.pollIndex);

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
        output.balance = state.get().mTreasuryBalance;
    }

    struct GetDividendBalances_input {};
    struct GetDividendBalances_output
    {
        uint64 revenuePoolA;
        uint64 revenuePoolB;
        uint64 dedicatedRevenuePool;
        uint64 poolAQmineDividend;
        uint64 poolAQrwaDividend;
        uint64 poolBQmineDividend;
        uint64 poolBQrwaDividend;
        uint64 poolCQmineDividend;
        uint64 poolCQrwaDividend;
        uint64 poolDRevenuePool;
        uint64 poolDQmineDividend;
        uint64 poolDQrwaDividend;
    };
    PUBLIC_FUNCTION(GetDividendBalances)
    {
        output.revenuePoolA = state.get().mRevenuePoolA;
        output.revenuePoolB = state.get().mRevenuePoolB;
        output.dedicatedRevenuePool = state.get().mDedicatedRevenuePool;
        output.poolAQmineDividend = state.get().mPoolAQmineDividend;
        output.poolAQrwaDividend = state.get().mPoolAQrwaDividend;
        output.poolBQmineDividend = state.get().mPoolBQmineDividend;
        output.poolBQrwaDividend = state.get().mPoolBQrwaDividend;
        output.poolCQmineDividend = state.get().mPoolCQmineDividend;
        output.poolCQrwaDividend = state.get().mPoolCQrwaDividend;
        output.poolDRevenuePool = state.get().mPoolDRevenuePool;
        output.poolDQmineDividend = state.get().mPoolDQmineDividend;
        output.poolDQrwaDividend = state.get().mPoolDQrwaDividend;
    }

    struct GetTotalDistributed_input {};
    struct GetTotalDistributed_output
    {
        uint64 totalPoolADistributed;
        uint64 totalPoolBDistributed;
        uint64 totalPoolCDistributed;
        uint64 totalPoolDDistributed;
        uint64 payoutTotalQmineBegin;
    };
    PUBLIC_FUNCTION(GetTotalDistributed)
    {
        output.totalPoolADistributed = state.get().mTotalPoolADistributed;
        output.totalPoolBDistributed = state.get().mTotalPoolBDistributed;
        output.totalPoolCDistributed = state.get().mTotalPoolCDistributed;
        output.totalPoolDDistributed = state.get().mTotalPoolDDistributed;
        output.payoutTotalQmineBegin = state.get().mPayoutTotalQmineBegin;
    }

    // Diagnostic: Query configured contract addresses
    struct GetContractAddresses_input {};
    struct GetContractAddresses_output
    {
        id dedicatedRevenueAddress;
        id poolARevenueAddress;
        id poolDRevenueAddress;
        id fundraisingAddress;
        id exchangeAddress;
    };
    PUBLIC_FUNCTION(GetContractAddresses)
    {
        output.dedicatedRevenueAddress = state.get().mDedicatedRevenueAddress;
        output.poolARevenueAddress = state.get().mPoolARevenueAddress;
        output.poolDRevenueAddress = state.get().mPoolDRevenueAddress;
        output.fundraisingAddress = state.get().mFundraisingAddress;
        output.exchangeAddress = state.get().mExchangeAddress;
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
            if (state.get().mGovPolls.get(locals.i).status == QRWA_POLL_STATUS_ACTIVE)
            {
                output.ids.set(output.count, state.get().mGovPolls.get(locals.i).proposalId);
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
        if (state.get().mGeneralAssetBalances.get(locals.wrapper, locals.balance)) {
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

        for (locals.iterIndex = state.get().mGeneralAssetBalances.nextElementIndex(NULL_INDEX);
             locals.iterIndex != NULL_INDEX;
             locals.iterIndex = state.get().mGeneralAssetBalances.nextElementIndex(locals.iterIndex))
        {
            locals.currentAsset = state.get().mGeneralAssetBalances.key(locals.iterIndex);
            locals.currentBalance = state.get().mGeneralAssetBalances.value(locals.iterIndex);

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

    // GetScDividendTracking (fn 15): Lists all SC contract IDs from which dividends were received
    // and their cumulative totals. All SC dividends are routed to Pool B.
    struct GetScDividendTracking_input {};
    struct GetScDividendTracking_output
    {
        uint64 count;
        Array<id, QRWA_MAX_ASSETS> scContractIds;
        Array<uint64, QRWA_MAX_ASSETS> cumulativeDividends;
    };
    struct GetScDividendTracking_locals
    {
        sint64 iterIndex;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetScDividendTracking)
    {
        output.count = 0;

        for (locals.iterIndex = state.get().mScDividendTracker.nextElementIndex(NULL_INDEX);
             locals.iterIndex != NULL_INDEX;
             locals.iterIndex = state.get().mScDividendTracker.nextElementIndex(locals.iterIndex))
        {

            output.scContractIds.set(output.count, state.get().mScDividendTracker.key(locals.iterIndex));
            output.cumulativeDividends.set(output.count, state.get().mScDividendTracker.value(locals.iterIndex));
            output.count++;

            if (output.count >= QRWA_MAX_ASSETS)
            {
                break;
            }
        }
    }

    // Per-pool payout ring buffer queries (paginated).
    // Ring buffer stores QRWA_PAYOUT_RING_SIZE entries, query returns max QRWA_PAYOUT_PAGE_SIZE per call.
    // Entries are returned newest-first. page=0 → most recent, page=1 → next QRWA_PAYOUT_PAGE_SIZE per call.

    // GetPayoutsPoolA (fn 11): Pool A payouts — QMINE + qRWA holders (types 0+1+2, after gov fees)
    struct GetPayoutsQmine_input
    {
        uint16 page; // 0 = newest entries
    };
    struct GetPayoutsQmine_output
    {
        Array<QRWAPayoutEntry, QRWA_PAYOUT_PAGE_SIZE> payouts;
        uint16 nextIdx;        // Write cursor in full ring buffer
        uint16 returnedCount;  // Number of valid entries in this page
        uint16 page;           // Echoed page number
        uint16 totalPages;     // Total available pages
    };
    struct GetPayoutsQmine_locals
    {
        uint64 i;
        uint64 ringIdx;
        uint64 count;
        uint64 startOffset;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetPayoutsQmine)
    {
        output.nextIdx = state.get().mPayoutsPoolANextIdx;
        output.page = input.page;
        output.totalPages = (uint16)(div<uint64>(QRWA_PAYOUT_RING_SIZE + QRWA_PAYOUT_PAGE_SIZE - 1, QRWA_PAYOUT_PAGE_SIZE));
        locals.startOffset = (uint64)input.page * QRWA_PAYOUT_PAGE_SIZE;
        locals.count = 0;
        for (locals.i = 0; locals.i < QRWA_PAYOUT_PAGE_SIZE && (locals.startOffset + locals.i) < QRWA_PAYOUT_RING_SIZE; locals.i++)
        {
            locals.ringIdx = ((uint64)state.get().mPayoutsPoolANextIdx - 1 - locals.startOffset - locals.i + QRWA_PAYOUT_RING_SIZE) & (QRWA_PAYOUT_RING_SIZE - 1);
            output.payouts.set(locals.count, state.get().mPayoutsPoolA.get(locals.ringIdx));
            locals.count++;
        }
        output.returnedCount = (uint16)locals.count;
    }

    // GetPayoutsPoolB (fn 13): Pool B payouts — QMINE + qRWA holders (types 0+1+2, no gov fees)
    struct GetPayoutsQrwa_input
    {
        uint16 page;
    };
    struct GetPayoutsQrwa_output
    {
        Array<QRWAPayoutEntry, QRWA_PAYOUT_PAGE_SIZE> payouts;
        uint16 nextIdx;
        uint16 returnedCount;
        uint16 page;
        uint16 totalPages;
    };
    struct GetPayoutsQrwa_locals
    {
        uint64 i;
        uint64 ringIdx;
        uint64 count;
        uint64 startOffset;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetPayoutsQrwa)
    {
        output.nextIdx = state.get().mPayoutsPoolBNextIdx;
        output.page = input.page;
        output.totalPages = (uint16)(div<uint64>(QRWA_PAYOUT_RING_SIZE + QRWA_PAYOUT_PAGE_SIZE - 1, QRWA_PAYOUT_PAGE_SIZE));
        locals.startOffset = (uint64)input.page * QRWA_PAYOUT_PAGE_SIZE;
        locals.count = 0;
        for (locals.i = 0; locals.i < QRWA_PAYOUT_PAGE_SIZE && (locals.startOffset + locals.i) < QRWA_PAYOUT_RING_SIZE; locals.i++)
        {
            locals.ringIdx = ((uint64)state.get().mPayoutsPoolBNextIdx - 1 - locals.startOffset - locals.i + QRWA_PAYOUT_RING_SIZE) & (QRWA_PAYOUT_RING_SIZE - 1);
            output.payouts.set(locals.count, state.get().mPayoutsPoolB.get(locals.ringIdx));
            locals.count++;
        }
        output.returnedCount = (uint16)locals.count;
    }

    // GetPayoutsPoolC (fn 14): Pool C payouts — QMINE + dedicated qRWA holders (types 0+1+3)
    struct GetPayoutsDedicated_input
    {
        uint16 page;
    };
    struct GetPayoutsDedicated_output
    {
        Array<QRWAPayoutEntry, QRWA_PAYOUT_PAGE_SIZE> payouts;
        uint16 nextIdx;
        uint16 returnedCount;
        uint16 page;
        uint16 totalPages;
    };
    struct GetPayoutsDedicated_locals
    {
        uint64 i;
        uint64 ringIdx;
        uint64 count;
        uint64 startOffset;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetPayoutsDedicated)
    {
        output.nextIdx = state.get().mPayoutsPoolCNextIdx;
        output.page = input.page;
        output.totalPages = (uint16)(div<uint64>(QRWA_PAYOUT_RING_SIZE + QRWA_PAYOUT_PAGE_SIZE - 1, QRWA_PAYOUT_PAGE_SIZE));
        locals.startOffset = (uint64)input.page * QRWA_PAYOUT_PAGE_SIZE;
        locals.count = 0;
        for (locals.i = 0; locals.i < QRWA_PAYOUT_PAGE_SIZE && (locals.startOffset + locals.i) < QRWA_PAYOUT_RING_SIZE; locals.i++)
        {
            locals.ringIdx = ((uint64)state.get().mPayoutsPoolCNextIdx - 1 - locals.startOffset - locals.i + QRWA_PAYOUT_RING_SIZE) & (QRWA_PAYOUT_RING_SIZE - 1);
            output.payouts.set(locals.count, state.get().mPayoutsPoolC.get(locals.ringIdx));
            locals.count++;
        }
        output.returnedCount = (uint16)locals.count;
    }

    // GetPayoutsPoolD (fn 16): Pool D payouts — QMINE + dedicated qRWA holders (MLM Water)
    struct GetPayoutsPoolD_input
    {
        uint16 page;
    };
    struct GetPayoutsPoolD_output
    {
        Array<QRWAPayoutEntry, QRWA_PAYOUT_PAGE_SIZE> payouts;
        uint16 nextIdx;
        uint16 returnedCount;
        uint16 page;
        uint16 totalPages;
    };
    struct GetPayoutsPoolD_locals
    {
        uint64 i;
        uint64 ringIdx;
        uint64 count;
        uint64 startOffset;
    };
    PUBLIC_FUNCTION_WITH_LOCALS(GetPayoutsPoolD)
    {
        output.nextIdx = state.get().mPayoutsPoolDNextIdx;
        output.page = input.page;
        output.totalPages = (uint16)(div<uint64>(QRWA_PAYOUT_RING_SIZE + QRWA_PAYOUT_PAGE_SIZE - 1, QRWA_PAYOUT_PAGE_SIZE));
        locals.startOffset = (uint64)input.page * QRWA_PAYOUT_PAGE_SIZE;
        locals.count = 0;
        for (locals.i = 0; locals.i < QRWA_PAYOUT_PAGE_SIZE && (locals.startOffset + locals.i) < QRWA_PAYOUT_RING_SIZE; locals.i++)
        {
            locals.ringIdx = ((uint64)state.get().mPayoutsPoolDNextIdx - 1 - locals.startOffset - locals.i + QRWA_PAYOUT_RING_SIZE) & (QRWA_PAYOUT_RING_SIZE - 1);
            output.payouts.set(locals.count, state.get().mPayoutsPoolD.get(locals.ringIdx));
            locals.count++;
        }
        output.returnedCount = (uint16)locals.count;
    }



    /***************************************************/
    /***************** SYSTEM PROCEDURES ***************/
    /***************************************************/


    INITIALIZE()
    {
        // QMINE Asset Constant
        // Issuer: QMINEQQXYBEGBHNSUPOUYDIQKZPCBPQIIHUUZMCPLBPCCAIARVZBTYKGFCWM
        // Name: 297666170193 ("QMINE")
        state.mut().mQmineAsset.assetName = 297666170193ULL;
        state.mut().mQmineAsset.issuer = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );
        state.mut().mTreasuryBalance = 0;
        setMemory(state.mut().mLastPayoutTimePoolA, 0);
        setMemory(state.mut().mLastPayoutTimePoolB, 0);
        setMemory(state.mut().mLastPayoutTimePoolC, 0);
        setMemory(state.mut().mLastPayoutTimePoolD, 0);
        state.mut().mLastPayoutTickPoolA = 0;
        state.mut().mLastPayoutTickPoolB = 0;
        state.mut().mLastPayoutTickPoolC = 0;
        state.mut().mLastPayoutTickPoolD = 0;

        // Initialize default governance parameters
        // QMINEQQXYBEGBHNSUPOUYDIQKZPCBPQIIHUUZMCPLBPCCAIARVZBTYKGFCWM
        state.mut().mCurrentGovParams.mAdminAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );
        state.mut().mCurrentGovParams.electricityAddress = ID(
            _M, _Z, _P, _T, _Y, _A, _P, _N, _C, _D, _K, _U, _O, _G, _I, _A,
            _Z, _C, _T, _E, _R, _U, _Q, _U, _R, _I, _X, _C, _H, _V, _F, _W,
            _B, _C, _R, _N, _C, _J, _P, _G, _H, _A, _B, _S, _S, _V, _Q, _H,
            _R, _S, _J, _O, _P, _U, _N, _B
        );
        state.mut().mCurrentGovParams.maintenanceAddress = ID(
            _Z, _F, _P, _Y, _I, _R, _A, _Q, _A, _P, _N, _K, _N, _E, _D, _D,
            _W, _G, _K, _N, _K, _J, _Q, _I, _Y, _C, _F, _B, _J, _R, _L, _G,
            _V, _Q, _O, _T, _Y, _N, _W, _O, _G, _C, _N, _P, _P, _B, _Z, _K,
            _J, _P, _G, _D, _C, _A, _N, _E
        );
        state.mut().mCurrentGovParams.reinvestmentAddress = ID(
            _M, _D, _U, _X, _D, _C, _V, _I, _D, _G, _H, _T, _J, _G, _H, _E,
            _S, _A, _P, _R, _A, _Q, _G, _I, _H, _Y, _F, _D, _D, _I, _S, _T,
            _T, _K, _F, _G, _K, _O, _I, _O, _T, _B, _U, _N, _M, _P, _N, _Q,
            _J, _I, _Y, _W, _A, _B, _M, _G
        );

        // QMINE DEV's Address for receiving rewards from moved QMINE tokens
        // ZOXXIDCZIMGCECCFAXDDCMBBXCDAQJIHGOOATAFPSBFIOFOYECFKUFPBEMWC
        state.mut().mCurrentGovParams.qmineDevAddress = ID(
            _Z, _O, _X, _X, _I, _D, _C, _Z, _I, _M, _G, _C, _E, _C, _C, _F,
            _A, _X, _D, _D, _C, _M, _B, _B, _X, _C, _D, _A, _Q, _J, _I, _H,
            _G, _O, _O, _A, _T, _A, _F, _P, _S, _B, _F, _I, _O, _F, _O, _Y,
            _E, _C, _F, _K, _U, _F, _P, _B
        );
        state.mut().mCurrentGovParams.electricityPercent = 350;
        state.mut().mCurrentGovParams.maintenancePercent = 50;
        state.mut().mCurrentGovParams.reinvestmentPercent = 100;

        state.mut().mCurrentGovProposalId = 0;
        state.mut().mNewGovPollsThisEpoch = 0;

        // Initialize revenue pools
        state.mut().mRevenuePoolA = 0;
        state.mut().mRevenuePoolB = 0;
        state.mut().mDedicatedRevenuePool = 0;
        state.mut().mPoolDRevenuePool = 0;
        state.mut().mPoolAQmineDividend = 0;
        state.mut().mPoolAQrwaDividend = 0;
        state.mut().mPoolBQmineDividend = 0;
        state.mut().mPoolBQrwaDividend = 0;
        state.mut().mPoolCQmineDividend = 0;
        state.mut().mPoolCQrwaDividend = 0;
        state.mut().mPoolDQmineDividend = 0;
        state.mut().mPoolDQrwaDividend = 0;

        // Dedicated BTC revenue address (Pool C)
        // QTDSQGIEAPPMMDDSEHBHHETEUZHBUZXRYFKKTICWAAUXVEWNPCTGCAFBYWWB
        state.mut().mDedicatedRevenueAddress = ID(
            _Q, _T, _D, _S, _Q, _G, _I, _E, _A, _P, _P, _M, _M, _D, _D, _S,
            _E, _H, _B, _H, _H, _E, _T, _E, _U, _Z, _H, _B, _U, _Z, _X, _R,
            _Y, _F, _K, _K, _T, _I, _C, _W, _A, _A, _U, _X, _V, _E, _W, _N,
            _P, _C, _T, _G, _C, _A, _F, _B
        );

        // Fundraising address — excluded from ALL distributions
        // QTDSQGIEAPPMMDDSEHBHHETEUZHBUZXRYFKKTICWAAUXVEWNPCTGCAFBYWWB
        state.mut().mFundraisingAddress = ID(
            _Q, _T, _D, _S, _Q, _G, _I, _E, _A, _P, _P, _M, _M, _D, _D, _S,
            _E, _H, _B, _H, _H, _E, _T, _E, _U, _Z, _H, _B, _U, _Z, _X, _R,
            _Y, _F, _K, _K, _T, _I, _C, _W, _A, _A, _U, _X, _V, _E, _W, _N,
            _P, _C, _T, _G, _C, _A, _F, _B
        );

        // Exchange address (safe.trade) — excluded from ALL distributions
        // CLJHHZVRWAVBEAECXMCEIZOZOJLAMWCQMNYXXHQXOBGRUCXKVZCMBQECQJPE
        state.mut().mExchangeAddress = ID(
            _C, _L, _J, _H, _H, _Z, _V, _R, _W, _A, _V, _B, _E, _A, _E, _C,
            _X, _M, _C, _E, _I, _Z, _O, _Z, _O, _J, _L, _A, _M, _W, _C, _Q,
            _M, _N, _Y, _X, _X, _H, _Q, _X, _O, _B, _G, _R, _U, _C, _X, _K,
            _V, _Z, _C, _M, _B, _Q, _E, _C
        );

        // Pool A revenue address (Qubic Mining)
        // USALFUZBICLZIEMYPSKLYDZJZRFBKYEONUGSWFXOIGRMWSJHLIPMEGZCVCMG
        state.mut().mPoolARevenueAddress = ID(
            _U, _S, _A, _L, _F, _U, _Z, _B, _I, _C, _L, _Z, _I, _E, _M, _Y,
            _P, _S, _K, _L, _Y, _D, _Z, _J, _Z, _R, _F, _B, _K, _Y, _E, _O,
            _N, _U, _G, _S, _W, _F, _X, _O, _I, _G, _R, _M, _W, _S, _J, _H,
            _L, _I, _P, _M, _E, _G, _Z, _C
        );

        // Pool D revenue address (MLM Water)
        // QMINEQQXYBEGBHNSUPOUYDIQKZPCBPQIIHUUZMCPLBPCCAIARVZBTYKGFCWM
        state.mut().mPoolDRevenueAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );

        // Initialize total distributed
        state.mut().mTotalPoolADistributed = 0;
        state.mut().mTotalPoolBDistributed = 0;
        state.mut().mTotalPoolCDistributed = 0;
        state.mut().mTotalPoolDDistributed = 0;

        // Initialize QMINE totals
        state.mut().mTotalQmineBeginEpoch = 0;
        state.mut().mPayoutTotalQmineBegin = 0;

        // Initialize maps/arrays
        state.mut().mBeginEpochBalances.reset();
        state.mut().mEndEpochBalances.reset();
        state.mut().mPayoutBeginBalances.reset();
        state.mut().mPayoutEndBalances.reset();
        state.mut().mGeneralAssetBalances.reset();
        state.mut().mScDividendTracker.reset();
        state.mut().mShareholderVoteMap.reset();

        setMemory(state.mut().mGovPolls, 0);

        // Initialize per-pool ring buffers
        setMemory(state.mut().mPayoutsPoolA, 0);
        state.mut().mPayoutsPoolANextIdx = 0;
        setMemory(state.mut().mPayoutsPoolB, 0);
        state.mut().mPayoutsPoolBNextIdx = 0;
        setMemory(state.mut().mPayoutsPoolC, 0);
        state.mut().mPayoutsPoolCNextIdx = 0;
        setMemory(state.mut().mPayoutsPoolD, 0);
        state.mut().mPayoutsPoolDNextIdx = 0;
    }

    struct Reinit_input
    {
    };

    struct Reinit_output
    {
    };

    struct BEGIN_EPOCH_locals
    {
        AssetPossessionIterator iter;
        uint64 balance;
        QRWALogger logger;
        id holder;
        uint64 existingBalance;
        Reinit_input reinitInput;
        Reinit_output reinitOutput;
    };

    // One-time re-initialization called from BEGIN_EPOCH on the upgrade epoch.
    // INITIALIZE() is not called again for already-deployed contracts.
    PRIVATE_PROCEDURE(Reinit)
    {
        // QMINE Asset Constant
        // Issuer: QMINEQQXYBEGBHNSUPOUYDIQKZPCBPQIIHUUZMCPLBPCCAIARVZBTYKGFCWM
        // Name: 297666170193 ("QMINE")
        state.mut().mQmineAsset.assetName = 297666170193ULL;
        state.mut().mQmineAsset.issuer = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );

        // Initialize default governance parameters
        state.mut().mCurrentGovParams.mAdminAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );
        state.mut().mCurrentGovParams.electricityAddress = ID(
            _M, _Z, _P, _T, _Y, _A, _P, _N, _C, _D, _K, _U, _O, _G, _I, _A,
            _Z, _C, _T, _E, _R, _U, _Q, _U, _R, _I, _X, _C, _H, _V, _F, _W,
            _B, _C, _R, _N, _C, _J, _P, _G, _H, _A, _B, _S, _S, _V, _Q, _H,
            _R, _S, _J, _O, _P, _U, _N, _B
        );
        state.mut().mCurrentGovParams.maintenanceAddress = ID(
            _Z, _F, _P, _Y, _I, _R, _A, _Q, _A, _P, _N, _K, _N, _E, _D, _D,
            _W, _G, _K, _N, _K, _J, _Q, _I, _Y, _C, _F, _B, _J, _R, _L, _G,
            _V, _Q, _O, _T, _Y, _N, _W, _O, _G, _C, _N, _P, _P, _B, _Z, _K,
            _J, _P, _G, _D, _C, _A, _N, _E
        );
        state.mut().mCurrentGovParams.reinvestmentAddress = ID(
            _M, _D, _U, _X, _D, _C, _V, _I, _D, _G, _H, _T, _J, _G, _H, _E,
            _S, _A, _P, _R, _A, _Q, _G, _I, _H, _Y, _F, _D, _D, _I, _S, _T,
            _T, _K, _F, _G, _K, _O, _I, _O, _T, _B, _U, _N, _M, _P, _N, _Q,
            _J, _I, _Y, _W, _A, _B, _M, _G
        );
        state.mut().mCurrentGovParams.qmineDevAddress = ID(
            _Z, _O, _X, _X, _I, _D, _C, _Z, _I, _M, _G, _C, _E, _C, _C, _F,
            _A, _X, _D, _D, _C, _M, _B, _B, _X, _C, _D, _A, _Q, _J, _I, _H,
            _G, _O, _O, _A, _T, _A, _F, _P, _S, _B, _F, _I, _O, _F, _O, _Y,
            _E, _C, _F, _K, _U, _F, _P, _B
        );
        state.mut().mCurrentGovParams.electricityPercent = 350;
        state.mut().mCurrentGovParams.maintenancePercent = 50;
        state.mut().mCurrentGovParams.reinvestmentPercent = 100;

        // Dedicated BTC revenue address (Pool C)
        // QTDSQGIEAPPMMDDSEHBHHETEUZHBUZXRYFKKTICWAAUXVEWNPCTGCAFBYWWB
        state.mut().mDedicatedRevenueAddress = ID(
            _Q, _T, _D, _S, _Q, _G, _I, _E, _A, _P, _P, _M, _M, _D, _D, _S,
            _E, _H, _B, _H, _H, _E, _T, _E, _U, _Z, _H, _B, _U, _Z, _X, _R,
            _Y, _F, _K, _K, _T, _I, _C, _W, _A, _A, _U, _X, _V, _E, _W, _N,
            _P, _C, _T, _G, _C, _A, _F, _B
        );

        // Fundraising address — excluded from ALL distributions
        state.mut().mFundraisingAddress = ID(
            _Q, _T, _D, _S, _Q, _G, _I, _E, _A, _P, _P, _M, _M, _D, _D, _S,
            _E, _H, _B, _H, _H, _E, _T, _E, _U, _Z, _H, _B, _U, _Z, _X, _R,
            _Y, _F, _K, _K, _T, _I, _C, _W, _A, _A, _U, _X, _V, _E, _W, _N,
            _P, _C, _T, _G, _C, _A, _F, _B
        );

        // Exchange address (safe.trade) — excluded from ALL distributions
        state.mut().mExchangeAddress = ID(
            _C, _L, _J, _H, _H, _Z, _V, _R, _W, _A, _V, _B, _E, _A, _E, _C,
            _X, _M, _C, _E, _I, _Z, _O, _Z, _O, _J, _L, _A, _M, _W, _C, _Q,
            _M, _N, _Y, _X, _X, _H, _Q, _X, _O, _B, _G, _R, _U, _C, _X, _K,
            _V, _Z, _C, _M, _B, _Q, _E, _C
        );

        // Pool A revenue address (Qubic Mining)
        state.mut().mPoolARevenueAddress = ID(
            _U, _S, _A, _L, _F, _U, _Z, _B, _I, _C, _L, _Z, _I, _E, _M, _Y,
            _P, _S, _K, _L, _Y, _D, _Z, _J, _Z, _R, _F, _B, _K, _Y, _E, _O,
            _N, _U, _G, _S, _W, _F, _X, _O, _I, _G, _R, _M, _W, _S, _J, _H,
            _L, _I, _P, _M, _E, _G, _Z, _C
        );

        // Pool D revenue address (MLM Water)
        state.mut().mPoolDRevenueAddress = ID(
            _Q, _M, _I, _N, _E, _Q, _Q, _X, _Y, _B, _E, _G, _B, _H, _N, _S,
            _U, _P, _O, _U, _Y, _D, _I, _Q, _K, _Z, _P, _C, _B, _P, _Q, _I,
            _I, _H, _U, _U, _Z, _M, _C, _P, _L, _B, _P, _C, _C, _A, _I, _A,
            _R, _V, _Z, _B, _T, _Y, _K, _G
        );
    }

    BEGIN_EPOCH_WITH_LOCALS()
    {
        // Run one-time re-initialization exactly on the deployment upgrade epoch.
        if (qpi.epoch() == 211)
        {
            CALL(Reinit, locals.reinitInput, locals.reinitOutput);
        }

        // Reset new poll counters
        state.mut().mNewGovPollsThisEpoch = 0;

        state.mut().mEndEpochBalances.reset();

        // Take snapshot of begin balances for QMINE holders
        state.mut().mBeginEpochBalances.reset();
        state.mut().mTotalQmineBeginEpoch = 0;

        if (state.get().mQmineAsset.issuer != NULL_ID)
        {
            for (locals.iter.begin(state.get().mQmineAsset); !locals.iter.reachedEnd(); locals.iter.next())
            {
                // Exclude SELF (Treasury) from dividend snapshot
                if (locals.iter.possessor() == SELF)
                {
                    continue;
                }
                // Exclude fundraising address from all distributions
                if (state.get().mFundraisingAddress != NULL_ID && locals.iter.possessor() == state.get().mFundraisingAddress)
                {
                    continue;
                }
                // Exclude exchange address (safe.trade) from all distributions
                if (state.get().mExchangeAddress != NULL_ID && locals.iter.possessor() == state.get().mExchangeAddress)
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
                    state.get().mBeginEpochBalances.get(locals.holder, locals.existingBalance);

                    locals.balance = sadd(locals.existingBalance, locals.balance);

                    if (state.mut().mBeginEpochBalances.set(locals.holder, locals.balance) != NULL_INDEX)
                    {
                        state.mut().mTotalQmineBeginEpoch = sadd(state.get().mTotalQmineBeginEpoch, (uint64)locals.iter.numberOfPossessedShares());
                    }
                    else
                    {
                        // Log error - Max holders reached for snapshot
                        locals.logger.contractId = CONTRACT_INDEX;
                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                        locals.logger.primaryId = locals.holder;
                        locals.logger.valueA = 11; // Error code: Begin Epoch Snapshot full
                        locals.logger.valueB = state.get().mBeginEpochBalances.population();
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

        // Gov Params Voting
        uint64 i;
        uint64 topScore;
        uint64 topProposalIndex;
        uint64 totalQminePower;
        Array<uint64, QRWA_MAX_GOV_POLLS> govPollScores;
        uint64 govPassed;
        uint64 quorumThreshold;
        id savedQmineDevAddress;

        QRWALogger logger;
        uint64 epoch;

        sint64 copyIndex;
        id copyHolder;
        uint64 copyBalance;

        QRWAGovProposal govPoll;

        id holder;
        uint64 existingBalance;
    };
    END_EPOCH_WITH_LOCALS()
    {
        locals.epoch = qpi.epoch(); // Get current epoch for history records

        // Take snapshot of end balances for QMINE holders
        if (state.get().mQmineAsset.issuer != NULL_ID)
        {
            for (locals.iter.begin(state.get().mQmineAsset); !locals.iter.reachedEnd(); locals.iter.next())
            {
                // Exclude SELF (Treasury) from dividend snapshot
                if (locals.iter.possessor() == SELF)
                {
                    continue;
                }
                // Exclude fundraising address from all distributions
                if (state.get().mFundraisingAddress != NULL_ID && locals.iter.possessor() == state.get().mFundraisingAddress)
                {
                    continue;
                }
                // Exclude exchange address (safe.trade) from all distributions
                if (state.get().mExchangeAddress != NULL_ID && locals.iter.possessor() == state.get().mExchangeAddress)
                {
                    continue;
                }
                locals.balance = locals.iter.numberOfPossessedShares();
                locals.holder = locals.iter.possessor();

                if (locals.balance > 0)
                {
                    // Check if holder already exists (multiple SC management)
                    locals.existingBalance = 0;
                    state.get().mEndEpochBalances.get(locals.holder, locals.existingBalance);

                    locals.balance = sadd(locals.existingBalance, locals.balance);

                    if (state.mut().mEndEpochBalances.set(locals.holder, locals.balance) == NULL_INDEX)
                    {
                        // Log error - Max holders reached for snapshot
                        locals.logger.contractId = CONTRACT_INDEX;
                        locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                        locals.logger.primaryId = locals.holder;
                        locals.logger.valueA = 12; // Error code: End Epoch Snapshot full
                        locals.logger.valueB = state.get().mEndEpochBalances.population();
                        LOG_INFO(locals.logger);
                    }
                }
            }
        }

        // Process Governance Parameter Voting (voted by QMINE holders)
        // Recount all votes from scratch using snapshots

        locals.totalQminePower = state.get().mTotalQmineBeginEpoch;
        locals.govPollScores.setAll(0); // Reset scores to zero.

        for (locals.iterIndex = state.get().mShareholderVoteMap.nextElementIndex(NULL_INDEX);
             locals.iterIndex != NULL_INDEX;
             locals.iterIndex = state.get().mShareholderVoteMap.nextElementIndex(locals.iterIndex))
        {

            locals.iterVoter = state.get().mShareholderVoteMap.key(locals.iterIndex);

            // Get true voting power from snapshots
            locals.beginBalance = 0;
            locals.endBalance = 0;
            state.get().mBeginEpochBalances.get(locals.iterVoter, locals.beginBalance);
            state.get().mEndEpochBalances.get(locals.iterVoter, locals.endBalance);

            locals.votingPower = (locals.beginBalance < locals.endBalance) ? locals.beginBalance : locals.endBalance; // min(begin, end)

            if (locals.votingPower > 0) // Apply voting power
            {
                state.get().mShareholderVoteMap.get(locals.iterVoter, locals.proposalIndex);
                if (locals.proposalIndex < QRWA_MAX_GOV_POLLS)
                {
                    if (state.get().mGovPolls.get(locals.proposalIndex).status == QRWA_POLL_STATUS_ACTIVE)
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
            if (state.get().mGovPolls.get(locals.i).status == QRWA_POLL_STATUS_ACTIVE)
            {
                locals.currentScore = locals.govPollScores.get(locals.i);
                if (locals.currentScore > locals.topScore)
                {
                    locals.topScore = locals.currentScore;
                    locals.topProposalIndex = locals.i;
                }
            }
        }

        // Calculate simple majority threshold (>50% of total voting power)
        locals.quorumThreshold = 0;
        if (locals.totalQminePower > 0)
        {
            locals.quorumThreshold = sadd(div<uint64>(locals.totalQminePower, 2ULL), 1ULL);
        }

        // Finalize Gov Vote (check against simple majority threshold)
        locals.govPassed = 0;
        if (locals.topScore >= locals.quorumThreshold && locals.topProposalIndex != NULL_INDEX)
        {
            // Proposal passes — apply new params but preserve immutable qmineDevAddress
            locals.govPassed = 1;
            locals.savedQmineDevAddress = state.get().mCurrentGovParams.qmineDevAddress;
            state.mut().mCurrentGovParams = state.get().mGovPolls.get(locals.topProposalIndex).params;
            state.mut().mCurrentGovParams.qmineDevAddress = locals.savedQmineDevAddress;

            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_GOV_VOTE;
            locals.logger.primaryId = NULL_ID; // System event
            locals.logger.valueA = state.get().mGovPolls.get(locals.topProposalIndex).proposalId;
            locals.logger.valueB = QRWA_STATUS_SUCCESS; // Indicate params updated
            LOG_INFO(locals.logger);
        }

        // Update status for all active gov polls (for history)
        for (locals.i = 0; locals.i < QRWA_MAX_GOV_POLLS; locals.i++)
        {
            locals.govPoll = state.get().mGovPolls.get(locals.i);
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
                state.mut().mGovPolls.set(locals.i, locals.govPoll);
            }
        }

        // Reset governance voter map for the next epoch
        state.mut().mShareholderVoteMap.reset();

        // Copy the finalized epoch snapshots to the payout buffers
        state.mut().mPayoutBeginBalances.reset();
        state.mut().mPayoutEndBalances.reset();
        state.mut().mPayoutTotalQmineBegin = state.get().mTotalQmineBeginEpoch;

        // Copy mBeginEpochBalances -> mPayoutBeginBalances
        for (locals.copyIndex = state.get().mBeginEpochBalances.nextElementIndex(NULL_INDEX);
             locals.copyIndex != NULL_INDEX;
             locals.copyIndex = state.get().mBeginEpochBalances.nextElementIndex(locals.copyIndex))
        {
            locals.copyHolder = state.get().mBeginEpochBalances.key(locals.copyIndex);
            locals.copyBalance = state.get().mBeginEpochBalances.value(locals.copyIndex);
            state.mut().mPayoutBeginBalances.set(locals.copyHolder, locals.copyBalance);
        }

        // Copy mEndEpochBalances -> mPayoutEndBalances
        for (locals.copyIndex = state.get().mEndEpochBalances.nextElementIndex(NULL_INDEX);
             locals.copyIndex != NULL_INDEX;
             locals.copyIndex = state.get().mEndEpochBalances.nextElementIndex(locals.copyIndex))
        {
            locals.copyHolder = state.get().mEndEpochBalances.key(locals.copyIndex);
            locals.copyBalance = state.get().mEndEpochBalances.value(locals.copyIndex);
            state.mut().mPayoutEndBalances.set(locals.copyHolder, locals.copyBalance);
        }
    }


    struct END_TICK_locals
    {
        DateAndTime now;
        uint64 durationMicros;

        // Per-pool payout readiness flags
        uint64 poolAReady;
        uint64 poolBReady;
        uint64 poolCReady;
        uint64 poolDReady;
        uint64 poolAHadRevenue;
        uint64 poolBHadRevenue;
        uint64 poolCHadRevenue;
        uint64 poolDHadRevenue;
        uint64 dayMatch;

        // Gov fee locals
        uint64 totalGovPercent;
        uint64 totalFeeAmount;
        uint64 electricityPayout;
        uint64 maintenancePayout;
        uint64 reinvestmentPayout;

        // Per-pool revenue splitting
        uint64 qminePortion;
        uint64 qrwaPortion;

        // qRWA distribution (Pool A + B)
        uint64 eligibleShares;
        uint64 poolAAmountPerShare;
        uint64 poolBAmountPerShare;
        uint64 poolAQrwaDistributed;
        uint64 poolBQrwaDistributed;

        // Dedicated qRWA distribution (Pool C)
        uint64 dedicatedEligibleShares;
        uint64 dedicatedEligibleSharesHolder;
        uint64 dedicatedAmountPerShare;
        uint64 dedicatedDistributed;
        uint64 qrwaShares;
        uint64 requiredQmine;
        sint64 qmineBalance;

        // Pool D dedicated qRWA distribution
        uint64 poolDEligibleShares;
        uint64 poolDEligibleSharesHolder;
        uint64 poolDAmountPerShare;
        uint64 poolDDistributed;
        uint64 poolDReinvestment;

        // QMINE distribution
        sint64 qminePayoutIndex;
        id holder;
        uint64 beginBalance;
        uint64 endBalance;
        uint64 eligibleBalance;
        uint128 scaledPayout_128;
        uint128 eligiblePayout_128;
        uint128 poolAQmine_128;
        uint128 poolBQmine_128;
        uint128 poolCQmine_128;
        uint128 poolDQmine_128;
        uint64 payout_u64;
        uint64 foundEnd;

        // Last paid holder per pool (for distributing rounding remainder)
        id lastPaidHolderA;
        id lastPaidHolderB;
        id lastPaidHolderC;
        id lastPaidHolderD;

        QRWALogger logger;
        QRWAPayoutEntry payoutEntry;
        AssetPossessionIterator qrwaIter;
        Asset qrwaAsset;


    };
    END_TICK_WITH_LOCALS()
    {
        locals.now = qpi.now();

        // ── Per-pool payout scheduling ──
        locals.poolAReady = 0;
        locals.poolBReady = 0;
        locals.poolCReady = 0;
        locals.poolDReady = 0;

        if (QRWA_USE_TICK_BASED_PAYOUT == 1)
        {
            // Tick-based payout: fire every QRWA_PAYOUT_TICK_INTERVAL ticks
            if (qpi.tick() >= state.get().mLastPayoutTickPoolA + QRWA_PAYOUT_TICK_INTERVAL || state.get().mLastPayoutTickPoolA == 0)
            {
                locals.poolAReady = 1;
            }
            if (qpi.tick() >= state.get().mLastPayoutTickPoolB + QRWA_PAYOUT_TICK_INTERVAL || state.get().mLastPayoutTickPoolB == 0)
            {
                locals.poolBReady = 1;
            }
            if (qpi.tick() >= state.get().mLastPayoutTickPoolC + QRWA_PAYOUT_TICK_INTERVAL || state.get().mLastPayoutTickPoolC == 0)
            {
                locals.poolCReady = 1;
            }
            if (qpi.tick() >= state.get().mLastPayoutTickPoolD + QRWA_PAYOUT_TICK_INTERVAL || state.get().mLastPayoutTickPoolD == 0)
            {
                locals.poolDReady = 1;
            }
        }
        else
        {
            // UTC time-based payout scheduling
            // Day-of-week gate
            locals.dayMatch = 1;
            if (QRWA_CHECK_DAY_OF_WEEK == 1)
            {
                locals.dayMatch = (qpi.dayOfWeek(
                    (uint8)mod(locals.now.getYear(), (uint16)100),
                    locals.now.getMonth(),
                    locals.now.getDay()) == QRWA_PAYOUT_DAY) ? 1 : 0;
            }

            if (locals.dayMatch == 1)
            {
                // Pool A: Qubic Mining
                if (locals.now.getHour() == QRWA_PAYOUT_HOUR_POOL_A && locals.now.getMinute() >= QRWA_PAYOUT_MINUTE)
                {
                    if (state.get().mLastPayoutTimePoolA.getYear() == 0)
                    {
                        locals.poolAReady = 1;
                    }
                    else
                    {
                        locals.durationMicros = state.get().mLastPayoutTimePoolA.durationMicrosec(locals.now);
                        if (div<uint64>(locals.durationMicros, 1000) >= QRWA_MIN_PAYOUT_INTERVAL_MS)
                        {
                            locals.poolAReady = 1;
                        }
                    }
                }

                // Pool B: SC Assets
                if (locals.now.getHour() == QRWA_PAYOUT_HOUR_POOL_B && locals.now.getMinute() >= QRWA_PAYOUT_MINUTE)
                {
                    if (state.get().mLastPayoutTimePoolB.getYear() == 0)
                    {
                        locals.poolBReady = 1;
                    }
                    else
                    {
                        locals.durationMicros = state.get().mLastPayoutTimePoolB.durationMicrosec(locals.now);
                        if (div<uint64>(locals.durationMicros, 1000) >= QRWA_MIN_PAYOUT_INTERVAL_MS)
                        {
                            locals.poolBReady = 1;
                        }
                    }
                }

                // Pool C: BTC Mining
                if (locals.now.getHour() == QRWA_PAYOUT_HOUR_POOL_C && locals.now.getMinute() >= QRWA_PAYOUT_MINUTE)
                {
                    if (state.get().mLastPayoutTimePoolC.getYear() == 0)
                    {
                        locals.poolCReady = 1;
                    }
                    else
                    {
                        locals.durationMicros = state.get().mLastPayoutTimePoolC.durationMicrosec(locals.now);
                        if (div<uint64>(locals.durationMicros, 1000) >= QRWA_MIN_PAYOUT_INTERVAL_MS)
                        {
                            locals.poolCReady = 1;
                        }
                    }
                }

                // Pool D: MLM Water
                if (locals.now.getHour() == QRWA_PAYOUT_HOUR_POOL_D && locals.now.getMinute() >= QRWA_PAYOUT_MINUTE)
                {
                    if (state.get().mLastPayoutTimePoolD.getYear() == 0)
                    {
                        locals.poolDReady = 1;
                    }
                    else
                    {
                        locals.durationMicros = state.get().mLastPayoutTimePoolD.durationMicrosec(locals.now);
                        if (div<uint64>(locals.durationMicros, 1000) >= QRWA_MIN_PAYOUT_INTERVAL_MS)
                        {
                            locals.poolDReady = 1;
                        }
                    }
                }
            }
        }

        if (locals.poolAReady == 1 || locals.poolBReady == 1 || locals.poolCReady == 1 || locals.poolDReady == 1)
        {
                // Track whether each pool has revenue before distribution
                locals.poolAHadRevenue = (locals.poolAReady == 1 && state.get().mRevenuePoolA >= QRWA_MIN_REVENUE_THRESHOLD) ? 1 : 0;
                locals.poolBHadRevenue = (locals.poolBReady == 1 && state.get().mRevenuePoolB >= QRWA_MIN_REVENUE_THRESHOLD) ? 1 : 0;
                locals.poolCHadRevenue = (locals.poolCReady == 1 && state.get().mDedicatedRevenuePool >= QRWA_MIN_REVENUE_THRESHOLD) ? 1 : 0;
                locals.poolDHadRevenue = (locals.poolDReady == 1 && state.get().mPoolDRevenuePool >= QRWA_MIN_REVENUE_THRESHOLD) ? 1 : 0;
                locals.logger.contractId = CONTRACT_INDEX;
                locals.logger.logType = QRWA_LOG_TYPE_DISTRIBUTION;

                // Calculate and pay out governance fees from Pool A (mined funds) — only when Pool A fires
                // gov_percentage = electricity_percent + maintenance_percent + reinvestment_percent
                if (locals.poolAReady == 1)
                {
                locals.totalGovPercent = sadd(sadd(state.get().mCurrentGovParams.electricityPercent, state.get().mCurrentGovParams.maintenancePercent), state.get().mCurrentGovParams.reinvestmentPercent);
                locals.totalFeeAmount = 0;

                if (locals.totalGovPercent > 0 && locals.totalGovPercent <= QRWA_PERCENT_DENOMINATOR && state.get().mRevenuePoolA >= QRWA_MIN_REVENUE_THRESHOLD)
                {
                    locals.electricityPayout = div<uint64>(smul(state.get().mRevenuePoolA, state.get().mCurrentGovParams.electricityPercent), QRWA_PERCENT_DENOMINATOR);
                    if (locals.electricityPayout > 0 && state.get().mCurrentGovParams.electricityAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.get().mCurrentGovParams.electricityAddress, locals.electricityPayout) >= 0)
                        {
                            locals.totalFeeAmount = sadd(locals.totalFeeAmount, locals.electricityPayout);
                        }
                        else
                        {
                            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                            locals.logger.primaryId = state.get().mCurrentGovParams.electricityAddress;
                            locals.logger.valueA = locals.electricityPayout;
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                            LOG_INFO(locals.logger);
                        }
                    }

                    locals.maintenancePayout = div<uint64>(smul(state.get().mRevenuePoolA, state.get().mCurrentGovParams.maintenancePercent), QRWA_PERCENT_DENOMINATOR);
                    if (locals.maintenancePayout > 0 && state.get().mCurrentGovParams.maintenanceAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.get().mCurrentGovParams.maintenanceAddress, locals.maintenancePayout) >= 0)
                        {
                            locals.totalFeeAmount = sadd(locals.totalFeeAmount, locals.maintenancePayout);
                        }
                        else
                        {
                            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                            locals.logger.primaryId = state.get().mCurrentGovParams.maintenanceAddress;
                            locals.logger.valueA = locals.maintenancePayout;
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                            LOG_INFO(locals.logger);
                        }
                    }

                    locals.reinvestmentPayout = div<uint64>(smul(state.get().mRevenuePoolA, state.get().mCurrentGovParams.reinvestmentPercent), QRWA_PERCENT_DENOMINATOR);
                    if (locals.reinvestmentPayout > 0 && state.get().mCurrentGovParams.reinvestmentAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.get().mCurrentGovParams.reinvestmentAddress, locals.reinvestmentPayout) >= 0)
                        {
                            locals.totalFeeAmount = sadd(locals.totalFeeAmount, locals.reinvestmentPayout);
                        }
                        else
                        {
                            locals.logger.logType = QRWA_LOG_TYPE_ERROR;
                            locals.logger.primaryId = state.get().mCurrentGovParams.reinvestmentAddress;
                            locals.logger.valueA = locals.reinvestmentPayout;
                            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
                            LOG_INFO(locals.logger);
                        }
                    }
                    state.mut().mRevenuePoolA = (state.get().mRevenuePoolA > locals.totalFeeAmount) ? (state.get().mRevenuePoolA - locals.totalFeeAmount) : 0;
                }

                // Pool A: split remaining revenue (after gov fees) into 90% QMINE / 10% qRWA
                if (state.get().mRevenuePoolA >= QRWA_MIN_REVENUE_THRESHOLD)
                {
                    locals.qminePortion = div<uint64>(smul(state.get().mRevenuePoolA, QRWA_QMINE_HOLDER_PERCENT), QRWA_PERCENT_DENOMINATOR);
                    locals.qrwaPortion = state.get().mRevenuePoolA - locals.qminePortion;
                    state.mut().mPoolAQmineDividend = sadd(state.get().mPoolAQmineDividend, locals.qminePortion);
                    state.mut().mPoolAQrwaDividend = sadd(state.get().mPoolAQrwaDividend, locals.qrwaPortion);
                    state.mut().mRevenuePoolA = 0;
                }
                } // end poolAReady gov fees + split

                // Pool B: split revenue (no gov fees) into 90% QMINE / 10% qRWA — only when Pool B fires
                if (locals.poolBReady == 1 && state.get().mRevenuePoolB >= QRWA_MIN_REVENUE_THRESHOLD)
                {
                    locals.qminePortion = div<uint64>(smul(state.get().mRevenuePoolB, QRWA_QMINE_HOLDER_PERCENT), QRWA_PERCENT_DENOMINATOR);
                    locals.qrwaPortion = state.get().mRevenuePoolB - locals.qminePortion;
                    state.mut().mPoolBQmineDividend = sadd(state.get().mPoolBQmineDividend, locals.qminePortion);
                    state.mut().mPoolBQrwaDividend = sadd(state.get().mPoolBQrwaDividend, locals.qrwaPortion);
                    state.mut().mRevenuePoolB = 0;
                }

                // Pool C: split dedicated revenue into 90% QMINE / 10% dedicated qRWA — only when Pool C fires
                if (locals.poolCReady == 1 && state.get().mDedicatedRevenuePool >= QRWA_MIN_REVENUE_THRESHOLD)
                {
                    locals.qminePortion = div<uint64>(smul(state.get().mDedicatedRevenuePool, QRWA_QMINE_HOLDER_PERCENT), QRWA_PERCENT_DENOMINATOR);
                    locals.qrwaPortion = state.get().mDedicatedRevenuePool - locals.qminePortion;
                    state.mut().mPoolCQmineDividend = sadd(state.get().mPoolCQmineDividend, locals.qminePortion);
                    state.mut().mPoolCQrwaDividend = sadd(state.get().mPoolCQrwaDividend, locals.qrwaPortion);
                    state.mut().mDedicatedRevenuePool = 0;
                }

                // Pool D: deduct 10% reinvestment, then split remaining into 90% QMINE / 10% dedicated qRWA — only when Pool D fires
                if (locals.poolDReady == 1 && state.get().mPoolDRevenuePool >= QRWA_MIN_REVENUE_THRESHOLD)
                {
                    // 10% reinvestment fee
                    locals.poolDReinvestment = div<uint64>(smul(state.get().mPoolDRevenuePool, QRWA_POOL_D_REINVESTMENT_PERCENT), QRWA_PERCENT_DENOMINATOR);
                    if (locals.poolDReinvestment > 0 && state.get().mCurrentGovParams.reinvestmentAddress != NULL_ID)
                    {
                        if (qpi.transfer(state.get().mCurrentGovParams.reinvestmentAddress, locals.poolDReinvestment) >= 0)
                        {
                            state.mut().mPoolDRevenuePool = (state.get().mPoolDRevenuePool > locals.poolDReinvestment) ? (state.get().mPoolDRevenuePool - locals.poolDReinvestment) : 0;
                        }
                    }
                    // Split remaining 90% into QMINE (90%) / qRWA (10%)
                    locals.qminePortion = div<uint64>(smul(state.get().mPoolDRevenuePool, QRWA_QMINE_HOLDER_PERCENT), QRWA_PERCENT_DENOMINATOR);
                    locals.qrwaPortion = state.get().mPoolDRevenuePool - locals.qminePortion;
                    state.mut().mPoolDQmineDividend = sadd(state.get().mPoolDQmineDividend, locals.qminePortion);
                    state.mut().mPoolDQrwaDividend = sadd(state.get().mPoolDQrwaDividend, locals.qrwaPortion);
                    state.mut().mPoolDRevenuePool = 0;
                }

                // ──── QMINE distribution: single pass over holders, distribute from active pools ────
                locals.poolAQmine_128 = (locals.poolAReady == 1) ? (uint128)state.get().mPoolAQmineDividend : (uint128)0;
                locals.poolBQmine_128 = (locals.poolBReady == 1) ? (uint128)state.get().mPoolBQmineDividend : (uint128)0;
                locals.poolCQmine_128 = (locals.poolCReady == 1) ? (uint128)state.get().mPoolCQmineDividend : (uint128)0;
                locals.poolDQmine_128 = (locals.poolDReady == 1) ? (uint128)state.get().mPoolDQmineDividend : (uint128)0;

                if ((locals.poolAQmine_128 > (uint128)0 || locals.poolBQmine_128 > (uint128)0 || locals.poolCQmine_128 > (uint128)0 || locals.poolDQmine_128 > (uint128)0) && state.get().mPayoutTotalQmineBegin > 0)
                {
                    for (locals.qminePayoutIndex = state.get().mPayoutBeginBalances.nextElementIndex(NULL_INDEX);
                         locals.qminePayoutIndex != NULL_INDEX;
                         locals.qminePayoutIndex = state.get().mPayoutBeginBalances.nextElementIndex(locals.qminePayoutIndex))
                    {

                        locals.holder = state.get().mPayoutBeginBalances.key(locals.qminePayoutIndex);
                        locals.beginBalance = state.get().mPayoutBeginBalances.value(locals.qminePayoutIndex);

                        if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress)
                        {
                            continue;
                        }
                        if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress)
                        {
                            continue;
                        }

                        locals.foundEnd = state.get().mPayoutEndBalances.get(locals.holder, locals.endBalance) ? 1 : 0;
                        if (locals.foundEnd == 0)
                        {
                            locals.endBalance = 0;
                        }

                        locals.eligibleBalance = (locals.endBalance >= locals.beginBalance) ? locals.beginBalance : 0;

                        if (locals.eligibleBalance > 0)
                        {
                            // ── Pool A QMINE payout ──
                            if (locals.poolAQmine_128 > (uint128)0)
                            {
                                locals.scaledPayout_128 = (uint128)locals.eligibleBalance * (uint128)state.get().mPoolAQmineDividend;
                                locals.eligiblePayout_128 = div<uint128>(locals.scaledPayout_128, state.get().mPayoutTotalQmineBegin);
                                if (locals.eligiblePayout_128 > locals.poolAQmine_128)
                                {
                                    locals.eligiblePayout_128 = locals.poolAQmine_128;
                                }
                                if (locals.eligiblePayout_128 > (uint128)0 && locals.eligiblePayout_128.high == 0)
                                {
                                    locals.payout_u64 = locals.eligiblePayout_128.low;
                                    if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, (sint64)locals.payout_u64) >= 0)
                                    {
                                        locals.poolAQmine_128 -= locals.eligiblePayout_128;
                                        state.mut().mTotalPoolADistributed = sadd(state.get().mTotalPoolADistributed, locals.payout_u64);
                                        locals.lastPaidHolderA = locals.holder;
                                        locals.payoutEntry.recipient = locals.holder;
                                        locals.payoutEntry.amount = locals.payout_u64;
                                        locals.payoutEntry.qmineHolding = locals.eligibleBalance;
                                        locals.payoutEntry.qrwaHolding = 0;
                                        locals.payoutEntry.tick = qpi.tick();
                                        locals.payoutEntry.epoch = qpi.epoch();
                                        locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_QMINE_HOLDER;
                                        state.mut().mPayoutsPoolA.set(state.get().mPayoutsPoolANextIdx, locals.payoutEntry);
                                        state.mut().mPayoutsPoolANextIdx = (state.get().mPayoutsPoolANextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                    }
                                }
                            }

                            // ── Pool B QMINE payout ──
                            if (locals.poolBQmine_128 > (uint128)0)
                            {
                                locals.scaledPayout_128 = (uint128)locals.eligibleBalance * (uint128)state.get().mPoolBQmineDividend;
                                locals.eligiblePayout_128 = div<uint128>(locals.scaledPayout_128, state.get().mPayoutTotalQmineBegin);
                                if (locals.eligiblePayout_128 > locals.poolBQmine_128)
                                {
                                    locals.eligiblePayout_128 = locals.poolBQmine_128;
                                }
                                if (locals.eligiblePayout_128 > (uint128)0 && locals.eligiblePayout_128.high == 0)
                                {
                                    locals.payout_u64 = locals.eligiblePayout_128.low;
                                    if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, (sint64)locals.payout_u64) >= 0)
                                    {
                                        locals.poolBQmine_128 -= locals.eligiblePayout_128;
                                        state.mut().mTotalPoolBDistributed = sadd(state.get().mTotalPoolBDistributed, locals.payout_u64);
                                        locals.lastPaidHolderB = locals.holder;
                                        locals.payoutEntry.recipient = locals.holder;
                                        locals.payoutEntry.amount = locals.payout_u64;
                                        locals.payoutEntry.qmineHolding = locals.eligibleBalance;
                                        locals.payoutEntry.qrwaHolding = 0;
                                        locals.payoutEntry.tick = qpi.tick();
                                        locals.payoutEntry.epoch = qpi.epoch();
                                        locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_QMINE_HOLDER;
                                        state.mut().mPayoutsPoolB.set(state.get().mPayoutsPoolBNextIdx, locals.payoutEntry);
                                        state.mut().mPayoutsPoolBNextIdx = (state.get().mPayoutsPoolBNextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                    }
                                }
                            }

                            // ── Pool C QMINE payout ──
                            if (locals.poolCQmine_128 > (uint128)0)
                            {
                                locals.scaledPayout_128 = (uint128)locals.eligibleBalance * (uint128)state.get().mPoolCQmineDividend;
                                locals.eligiblePayout_128 = div<uint128>(locals.scaledPayout_128, state.get().mPayoutTotalQmineBegin);
                                if (locals.eligiblePayout_128 > locals.poolCQmine_128)
                                {
                                    locals.eligiblePayout_128 = locals.poolCQmine_128;
                                }
                                if (locals.eligiblePayout_128 > (uint128)0 && locals.eligiblePayout_128.high == 0)
                                {
                                    locals.payout_u64 = locals.eligiblePayout_128.low;
                                    if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, (sint64)locals.payout_u64) >= 0)
                                    {
                                        locals.poolCQmine_128 -= locals.eligiblePayout_128;
                                        state.mut().mTotalPoolCDistributed = sadd(state.get().mTotalPoolCDistributed, locals.payout_u64);
                                        locals.lastPaidHolderC = locals.holder;
                                        locals.payoutEntry.recipient = locals.holder;
                                        locals.payoutEntry.amount = locals.payout_u64;
                                        locals.payoutEntry.qmineHolding = locals.eligibleBalance;
                                        locals.payoutEntry.qrwaHolding = 0;
                                        locals.payoutEntry.tick = qpi.tick();
                                        locals.payoutEntry.epoch = qpi.epoch();
                                        locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_QMINE_HOLDER;
                                        state.mut().mPayoutsPoolC.set(state.get().mPayoutsPoolCNextIdx, locals.payoutEntry);
                                        state.mut().mPayoutsPoolCNextIdx = (state.get().mPayoutsPoolCNextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                    }
                                }
                            }

                            // ── Pool D QMINE payout ──
                            if (locals.poolDQmine_128 > (uint128)0)
                            {
                                locals.scaledPayout_128 = (uint128)locals.eligibleBalance * (uint128)state.get().mPoolDQmineDividend;
                                locals.eligiblePayout_128 = div<uint128>(locals.scaledPayout_128, state.get().mPayoutTotalQmineBegin);
                                if (locals.eligiblePayout_128 > locals.poolDQmine_128)
                                {
                                    locals.eligiblePayout_128 = locals.poolDQmine_128;
                                }
                                if (locals.eligiblePayout_128 > (uint128)0 && locals.eligiblePayout_128.high == 0)
                                {
                                    locals.payout_u64 = locals.eligiblePayout_128.low;
                                    if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, (sint64)locals.payout_u64) >= 0)
                                    {
                                        locals.poolDQmine_128 -= locals.eligiblePayout_128;
                                        state.mut().mTotalPoolDDistributed = sadd(state.get().mTotalPoolDDistributed, locals.payout_u64);
                                        locals.lastPaidHolderD = locals.holder;
                                        locals.payoutEntry.recipient = locals.holder;
                                        locals.payoutEntry.amount = locals.payout_u64;
                                        locals.payoutEntry.qmineHolding = locals.eligibleBalance;
                                        locals.payoutEntry.qrwaHolding = 0;
                                        locals.payoutEntry.tick = qpi.tick();
                                        locals.payoutEntry.epoch = qpi.epoch();
                                        locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_QMINE_HOLDER;
                                        state.mut().mPayoutsPoolD.set(state.get().mPayoutsPoolDNextIdx, locals.payoutEntry);
                                        state.mut().mPayoutsPoolDNextIdx = (state.get().mPayoutsPoolDNextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                    }
                                }
                            }
                        }
                    }

                    // QMINE remainder: send to last paid holder (rounding dust)
                    // Pool A remainder
                    if (locals.poolAQmine_128 > (uint128)0 && locals.poolAQmine_128.high == 0 && locals.lastPaidHolderA != NULL_ID)
                    {
                        locals.payout_u64 = locals.poolAQmine_128.low;
                        if (locals.payout_u64 > 0 && qpi.transfer(locals.lastPaidHolderA, (sint64)locals.payout_u64) >= 0)
                        {
                            state.mut().mTotalPoolADistributed = sadd(state.get().mTotalPoolADistributed, locals.payout_u64);
                            locals.poolAQmine_128 = 0;
                        }
                    }
                    // Pool B remainder
                    if (locals.poolBQmine_128 > (uint128)0 && locals.poolBQmine_128.high == 0 && locals.lastPaidHolderB != NULL_ID)
                    {
                        locals.payout_u64 = locals.poolBQmine_128.low;
                        if (locals.payout_u64 > 0 && qpi.transfer(locals.lastPaidHolderB, (sint64)locals.payout_u64) >= 0)
                        {
                            state.mut().mTotalPoolBDistributed = sadd(state.get().mTotalPoolBDistributed, locals.payout_u64);
                            locals.poolBQmine_128 = 0;
                        }
                    }
                    // Pool C remainder
                    if (locals.poolCQmine_128 > (uint128)0 && locals.poolCQmine_128.high == 0 && locals.lastPaidHolderC != NULL_ID)
                    {
                        locals.payout_u64 = locals.poolCQmine_128.low;
                        if (locals.payout_u64 > 0 && qpi.transfer(locals.lastPaidHolderC, (sint64)locals.payout_u64) >= 0)
                        {
                            state.mut().mTotalPoolCDistributed = sadd(state.get().mTotalPoolCDistributed, locals.payout_u64);
                            locals.poolCQmine_128 = 0;
                        }
                    }
                    // Pool D remainder
                    if (locals.poolDQmine_128 > (uint128)0 && locals.poolDQmine_128.high == 0 && locals.lastPaidHolderD != NULL_ID)
                    {
                        locals.payout_u64 = locals.poolDQmine_128.low;
                        if (locals.payout_u64 > 0 && qpi.transfer(locals.lastPaidHolderD, (sint64)locals.payout_u64) >= 0)
                        {
                            state.mut().mTotalPoolDDistributed = sadd(state.get().mTotalPoolDDistributed, locals.payout_u64);
                            locals.poolDQmine_128 = 0;
                        }
                    }

                    // Clear QMINE dividends for active pools (fully distributed)
                    if (locals.poolAReady == 1) state.mut().mPoolAQmineDividend = 0;
                    if (locals.poolBReady == 1) state.mut().mPoolBQmineDividend = 0;
                    if (locals.poolCReady == 1) state.mut().mPoolCQmineDividend = 0;
                    if (locals.poolDReady == 1) state.mut().mPoolDQmineDividend = 0;
                } // End QMINE distribution

                // ──── qRWA distribution (Pool A + Pool B): single pass over qRWA holders ────
                if ((locals.poolAReady == 1 && state.get().mPoolAQrwaDividend > 0) || (locals.poolBReady == 1 && state.get().mPoolBQrwaDividend > 0))
                {
                    locals.qrwaAsset.issuer = id::zero();
                    locals.qrwaAsset.assetName = QRWA_CONTRACT_ASSET_NAME;
                    locals.eligibleShares = 0;

                    // First pass: count eligible shares
                    for (locals.qrwaIter.begin(locals.qrwaAsset); !locals.qrwaIter.reachedEnd(); locals.qrwaIter.next())
                    {
                        locals.qrwaShares = static_cast<uint64>(locals.qrwaIter.numberOfPossessedShares());
                        if (locals.qrwaShares == 0) continue;
                        locals.holder = locals.qrwaIter.possessor();
                        if (locals.holder == SELF) continue;
                        if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress) continue;
                        if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress) continue;
                        locals.eligibleShares = sadd(locals.eligibleShares, locals.qrwaShares);
                    }

                    if (locals.eligibleShares > 0)
                    {
                        locals.poolAAmountPerShare = (locals.poolAReady == 1 && state.get().mPoolAQrwaDividend > 0) ? div<uint64>(state.get().mPoolAQrwaDividend, locals.eligibleShares) : 0;
                        locals.poolBAmountPerShare = (locals.poolBReady == 1 && state.get().mPoolBQrwaDividend > 0) ? div<uint64>(state.get().mPoolBQrwaDividend, locals.eligibleShares) : 0;

                        if (locals.poolAAmountPerShare > 0 || locals.poolBAmountPerShare > 0)
                        {
                            locals.poolAQrwaDistributed = 0;
                            locals.poolBQrwaDistributed = 0;

                            // Second pass: distribute to eligible holders
                            for (locals.qrwaIter.begin(locals.qrwaAsset); !locals.qrwaIter.reachedEnd(); locals.qrwaIter.next())
                            {
                                locals.qrwaShares = static_cast<uint64>(locals.qrwaIter.numberOfPossessedShares());
                                if (locals.qrwaShares == 0) continue;
                                locals.holder = locals.qrwaIter.possessor();
                                if (locals.holder == SELF) continue;
                                if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress) continue;
                                if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress) continue;

                                // Pool A qRWA payout
                                if (locals.poolAAmountPerShare > 0)
                                {
                                    locals.payout_u64 = smul(locals.poolAAmountPerShare, locals.qrwaShares);
                                    if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, static_cast<sint64>(locals.payout_u64)) >= 0)
                                    {
                                        locals.poolAQrwaDistributed = sadd(locals.poolAQrwaDistributed, locals.payout_u64);
                                        state.mut().mTotalPoolADistributed = sadd(state.get().mTotalPoolADistributed, locals.payout_u64);
                                        locals.lastPaidHolderA = locals.holder;
                                        locals.payoutEntry.recipient = locals.holder;
                                        locals.payoutEntry.amount = locals.payout_u64;
                                        locals.payoutEntry.qmineHolding = 0;
                                        locals.payoutEntry.qrwaHolding = locals.qrwaShares;
                                        locals.payoutEntry.tick = qpi.tick();
                                        locals.payoutEntry.epoch = qpi.epoch();
                                        locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_QRWA_HOLDER;
                                        state.mut().mPayoutsPoolA.set(state.get().mPayoutsPoolANextIdx, locals.payoutEntry);
                                        state.mut().mPayoutsPoolANextIdx = (state.get().mPayoutsPoolANextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                    }
                                }

                                // Pool B qRWA payout
                                if (locals.poolBAmountPerShare > 0)
                                {
                                    locals.payout_u64 = smul(locals.poolBAmountPerShare, locals.qrwaShares);
                                    if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, static_cast<sint64>(locals.payout_u64)) >= 0)
                                    {
                                        locals.poolBQrwaDistributed = sadd(locals.poolBQrwaDistributed, locals.payout_u64);
                                        state.mut().mTotalPoolBDistributed = sadd(state.get().mTotalPoolBDistributed, locals.payout_u64);
                                        locals.lastPaidHolderB = locals.holder;
                                        locals.payoutEntry.recipient = locals.holder;
                                        locals.payoutEntry.amount = locals.payout_u64;
                                        locals.payoutEntry.qmineHolding = 0;
                                        locals.payoutEntry.qrwaHolding = locals.qrwaShares;
                                        locals.payoutEntry.tick = qpi.tick();
                                        locals.payoutEntry.epoch = qpi.epoch();
                                        locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_QRWA_HOLDER;
                                        state.mut().mPayoutsPoolB.set(state.get().mPayoutsPoolBNextIdx, locals.payoutEntry);
                                        state.mut().mPayoutsPoolBNextIdx = (state.get().mPayoutsPoolBNextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                    }
                                }
                            }

                            // Pool A qRWA: send remainder to last paid holder
                            if (locals.poolAReady == 1)
                            {
                                state.mut().mPoolAQrwaDividend = (state.get().mPoolAQrwaDividend > locals.poolAQrwaDistributed)
                                    ? (state.get().mPoolAQrwaDividend - locals.poolAQrwaDistributed) : 0;
                                if (state.get().mPoolAQrwaDividend > 0 && locals.lastPaidHolderA != NULL_ID)
                                {
                                    if (qpi.transfer(locals.lastPaidHolderA, static_cast<sint64>(state.get().mPoolAQrwaDividend)) >= 0)
                                    {
                                        state.mut().mTotalPoolADistributed = sadd(state.get().mTotalPoolADistributed, state.get().mPoolAQrwaDividend);
                                        state.mut().mPoolAQrwaDividend = 0;
                                    }
                                }
                            }
                            // Pool B qRWA: send remainder to last paid holder
                            if (locals.poolBReady == 1)
                            {
                                state.mut().mPoolBQrwaDividend = (state.get().mPoolBQrwaDividend > locals.poolBQrwaDistributed)
                                    ? (state.get().mPoolBQrwaDividend - locals.poolBQrwaDistributed) : 0;
                                if (state.get().mPoolBQrwaDividend > 0 && locals.lastPaidHolderB != NULL_ID)
                                {
                                    if (qpi.transfer(locals.lastPaidHolderB, static_cast<sint64>(state.get().mPoolBQrwaDividend)) >= 0)
                                    {
                                        state.mut().mTotalPoolBDistributed = sadd(state.get().mTotalPoolBDistributed, state.get().mPoolBQrwaDividend);
                                        state.mut().mPoolBQrwaDividend = 0;
                                    }
                                }
                            }
                        }
                    }
                }

                // ──── Dedicated qRWA distribution (Pool C): partial eligibility based on QMINE ────
                // Each qRWA share requires 100K QMINE. Holders get payout for min(qrwaShares, qmineBalance/100K).
                // Shares not covered by QMINE are redistributed to all eligible holders.
                if (locals.poolCReady == 1 && state.get().mPoolCQrwaDividend > 0)
                {
                    locals.qrwaAsset.issuer = id::zero();
                    locals.qrwaAsset.assetName = QRWA_CONTRACT_ASSET_NAME;
                    locals.dedicatedEligibleShares = 0;

                    for (locals.qrwaIter.begin(locals.qrwaAsset); !locals.qrwaIter.reachedEnd(); locals.qrwaIter.next())
                    {
                        locals.qrwaShares = static_cast<uint64>(locals.qrwaIter.numberOfPossessedShares());
                        if (locals.qrwaShares == 0) continue;
                        locals.holder = locals.qrwaIter.possessor();
                        if (locals.holder == SELF) continue;
                        if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress) continue;
                        if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress) continue;

                        locals.qmineBalance = qpi.numberOfShares(
                            state.get().mQmineAsset,
                            AssetOwnershipSelect::byOwner(locals.holder),
                            AssetPossessionSelect::byPossessor(locals.holder)
                        );
                        if (locals.qmineBalance <= 0) continue;

                        // Partial eligibility: eligible shares = min(qrwaShares, qmineBalance / 100K)
                        locals.dedicatedEligibleSharesHolder = div<uint64>(static_cast<uint64>(locals.qmineBalance), QRWA_QMINE_PER_QRWA_SHARE_MIN);
                        if (locals.dedicatedEligibleSharesHolder > locals.qrwaShares)
                            locals.dedicatedEligibleSharesHolder = locals.qrwaShares;
                        if (locals.dedicatedEligibleSharesHolder > 0)
                        {
                            locals.dedicatedEligibleShares = sadd(locals.dedicatedEligibleShares, locals.dedicatedEligibleSharesHolder);
                        }
                    }

                    if (locals.dedicatedEligibleShares > 0)
                    {
                        locals.dedicatedAmountPerShare = div<uint64>(state.get().mPoolCQrwaDividend, locals.dedicatedEligibleShares);
                        if (locals.dedicatedAmountPerShare > 0)
                        {
                            locals.dedicatedDistributed = 0;
                            for (locals.qrwaIter.begin(locals.qrwaAsset); !locals.qrwaIter.reachedEnd(); locals.qrwaIter.next())
                            {
                                locals.qrwaShares = static_cast<uint64>(locals.qrwaIter.numberOfPossessedShares());
                                if (locals.qrwaShares == 0) continue;
                                locals.holder = locals.qrwaIter.possessor();
                                if (locals.holder == SELF) continue;
                                if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress) continue;
                                if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress) continue;

                                locals.qmineBalance = qpi.numberOfShares(
                                    state.get().mQmineAsset,
                                    AssetOwnershipSelect::byOwner(locals.holder),
                                    AssetPossessionSelect::byPossessor(locals.holder)
                                );
                                if (locals.qmineBalance <= 0) continue;

                                // Partial eligibility: same calculation as counting loop
                                locals.dedicatedEligibleSharesHolder = div<uint64>(static_cast<uint64>(locals.qmineBalance), QRWA_QMINE_PER_QRWA_SHARE_MIN);
                                if (locals.dedicatedEligibleSharesHolder > locals.qrwaShares)
                                    locals.dedicatedEligibleSharesHolder = locals.qrwaShares;
                                if (locals.dedicatedEligibleSharesHolder == 0) continue;

                                locals.payout_u64 = smul(locals.dedicatedAmountPerShare, locals.dedicatedEligibleSharesHolder);
                                if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, static_cast<sint64>(locals.payout_u64)) >= 0)
                                {
                                    locals.dedicatedDistributed = sadd(locals.dedicatedDistributed, locals.payout_u64);
                                    state.mut().mTotalPoolCDistributed = sadd(state.get().mTotalPoolCDistributed, locals.payout_u64);
                                    locals.lastPaidHolderC = locals.holder;
                                    locals.payoutEntry.recipient = locals.holder;
                                    locals.payoutEntry.amount = locals.payout_u64;
                                    locals.payoutEntry.qmineHolding = static_cast<uint64>(locals.qmineBalance);
                                    locals.payoutEntry.qrwaHolding = locals.dedicatedEligibleSharesHolder; // eligible shares (may be < total held)
                                    locals.payoutEntry.tick = qpi.tick();
                                    locals.payoutEntry.epoch = qpi.epoch();
                                    locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_DEDICATED_QRWA;
                                    state.mut().mPayoutsPoolC.set(state.get().mPayoutsPoolCNextIdx, locals.payoutEntry);
                                    state.mut().mPayoutsPoolCNextIdx = (state.get().mPayoutsPoolCNextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                }
                            }

                            state.mut().mPoolCQrwaDividend = (state.get().mPoolCQrwaDividend > locals.dedicatedDistributed)
                                ? (state.get().mPoolCQrwaDividend - locals.dedicatedDistributed) : 0;
                            // Pool C dedicated: send remainder to last paid holder
                            if (state.get().mPoolCQrwaDividend > 0 && locals.lastPaidHolderC != NULL_ID)
                            {
                                if (qpi.transfer(locals.lastPaidHolderC, static_cast<sint64>(state.get().mPoolCQrwaDividend)) >= 0)
                                {
                                    state.mut().mTotalPoolCDistributed = sadd(state.get().mTotalPoolCDistributed, state.get().mPoolCQrwaDividend);
                                    state.mut().mPoolCQrwaDividend = 0;
                                }
                            }
                        }
                    }
                }

                // ──── Dedicated qRWA distribution (Pool D / MLM Water): same partial eligibility as Pool C ────
                if (locals.poolDReady == 1 && state.get().mPoolDQrwaDividend > 0)
                {
                    locals.qrwaAsset.issuer = id::zero();
                    locals.qrwaAsset.assetName = QRWA_CONTRACT_ASSET_NAME;
                    locals.poolDEligibleShares = 0;

                    for (locals.qrwaIter.begin(locals.qrwaAsset); !locals.qrwaIter.reachedEnd(); locals.qrwaIter.next())
                    {
                        locals.qrwaShares = static_cast<uint64>(locals.qrwaIter.numberOfPossessedShares());
                        if (locals.qrwaShares == 0) continue;
                        locals.holder = locals.qrwaIter.possessor();
                        if (locals.holder == SELF) continue;
                        if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress) continue;
                        if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress) continue;

                        locals.qmineBalance = qpi.numberOfShares(
                            state.get().mQmineAsset,
                            AssetOwnershipSelect::byOwner(locals.holder),
                            AssetPossessionSelect::byPossessor(locals.holder)
                        );
                        if (locals.qmineBalance <= 0) continue;

                        locals.poolDEligibleSharesHolder = div<uint64>(static_cast<uint64>(locals.qmineBalance), QRWA_QMINE_PER_QRWA_SHARE_MIN);
                        if (locals.poolDEligibleSharesHolder > locals.qrwaShares)
                            locals.poolDEligibleSharesHolder = locals.qrwaShares;
                        if (locals.poolDEligibleSharesHolder > 0)
                        {
                            locals.poolDEligibleShares = sadd(locals.poolDEligibleShares, locals.poolDEligibleSharesHolder);
                        }
                    }

                    if (locals.poolDEligibleShares > 0)
                    {
                        locals.poolDAmountPerShare = div<uint64>(state.get().mPoolDQrwaDividend, locals.poolDEligibleShares);
                        if (locals.poolDAmountPerShare > 0)
                        {
                            locals.poolDDistributed = 0;
                            for (locals.qrwaIter.begin(locals.qrwaAsset); !locals.qrwaIter.reachedEnd(); locals.qrwaIter.next())
                            {
                                locals.qrwaShares = static_cast<uint64>(locals.qrwaIter.numberOfPossessedShares());
                                if (locals.qrwaShares == 0) continue;
                                locals.holder = locals.qrwaIter.possessor();
                                if (locals.holder == SELF) continue;
                                if (state.get().mFundraisingAddress != NULL_ID && locals.holder == state.get().mFundraisingAddress) continue;
                                if (state.get().mExchangeAddress != NULL_ID && locals.holder == state.get().mExchangeAddress) continue;

                                locals.qmineBalance = qpi.numberOfShares(
                                    state.get().mQmineAsset,
                                    AssetOwnershipSelect::byOwner(locals.holder),
                                    AssetPossessionSelect::byPossessor(locals.holder)
                                );
                                if (locals.qmineBalance <= 0) continue;

                                locals.poolDEligibleSharesHolder = div<uint64>(static_cast<uint64>(locals.qmineBalance), QRWA_QMINE_PER_QRWA_SHARE_MIN);
                                if (locals.poolDEligibleSharesHolder > locals.qrwaShares)
                                    locals.poolDEligibleSharesHolder = locals.qrwaShares;
                                if (locals.poolDEligibleSharesHolder == 0) continue;

                                locals.payout_u64 = smul(locals.poolDAmountPerShare, locals.poolDEligibleSharesHolder);
                                if (locals.payout_u64 > 0 && qpi.transfer(locals.holder, static_cast<sint64>(locals.payout_u64)) >= 0)
                                {
                                    locals.poolDDistributed = sadd(locals.poolDDistributed, locals.payout_u64);
                                    state.mut().mTotalPoolDDistributed = sadd(state.get().mTotalPoolDDistributed, locals.payout_u64);
                                    locals.lastPaidHolderD = locals.holder;
                                    locals.payoutEntry.recipient = locals.holder;
                                    locals.payoutEntry.amount = locals.payout_u64;
                                    locals.payoutEntry.qmineHolding = static_cast<uint64>(locals.qmineBalance);
                                    locals.payoutEntry.qrwaHolding = locals.poolDEligibleSharesHolder;
                                    locals.payoutEntry.tick = qpi.tick();
                                    locals.payoutEntry.epoch = qpi.epoch();
                                    locals.payoutEntry.payoutType = QRWA_PAYOUT_TYPE_POOL_D_QRWA;
                                    state.mut().mPayoutsPoolD.set(state.get().mPayoutsPoolDNextIdx, locals.payoutEntry);
                                    state.mut().mPayoutsPoolDNextIdx = (state.get().mPayoutsPoolDNextIdx + 1) & (QRWA_PAYOUT_RING_SIZE - 1);
                                }
                            }

                            state.mut().mPoolDQrwaDividend = (state.get().mPoolDQrwaDividend > locals.poolDDistributed)
                                ? (state.get().mPoolDQrwaDividend - locals.poolDDistributed) : 0;
                            // Pool D dedicated: send remainder to last paid holder
                            if (state.get().mPoolDQrwaDividend > 0 && locals.lastPaidHolderD != NULL_ID)
                            {
                                if (qpi.transfer(locals.lastPaidHolderD, static_cast<sint64>(state.get().mPoolDQrwaDividend)) >= 0)
                                {
                                    state.mut().mTotalPoolDDistributed = sadd(state.get().mTotalPoolDDistributed, state.get().mPoolDQrwaDividend);
                                    state.mut().mPoolDQrwaDividend = 0;
                                }
                            }
                        }
                    }
                }

                // Update per-pool payout timestamps / ticks — only when pool actually had revenue
                if (locals.poolAHadRevenue == 1)
                { state.mut().mLastPayoutTimePoolA = locals.now; state.mut().mLastPayoutTickPoolA = qpi.tick(); }
                if (locals.poolBHadRevenue == 1)
                { state.mut().mLastPayoutTimePoolB = locals.now; state.mut().mLastPayoutTickPoolB = qpi.tick(); }
                if (locals.poolCHadRevenue == 1)
                { state.mut().mLastPayoutTimePoolC = locals.now; state.mut().mLastPayoutTickPoolC = qpi.tick(); }
                if (locals.poolDHadRevenue == 1)
                { state.mut().mLastPayoutTimePoolD = locals.now; state.mut().mLastPayoutTickPoolD = qpi.tick(); }
                locals.logger.logType = QRWA_LOG_TYPE_DISTRIBUTION;
                locals.logger.primaryId = NULL_ID;
                locals.logger.valueA = locals.poolAReady + locals.poolBReady * 2 + locals.poolCReady * 4 + locals.poolDReady * 8; // bitmask: A=1, B=2, C=4, D=8
                locals.logger.valueB = 0;
                LOG_INFO(locals.logger);
        }
    }

    struct POST_INCOMING_TRANSFER_locals
    {
        QRWALogger logger;
        uint64 prevCumulative;
    };
    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        // SC dividend routing: if a held SC distributes dividends (type 3), always → Pool B
        if (input.type == TransferType::qpiDistributeDividends)
        {
            state.mut().mRevenuePoolB = sadd(state.get().mRevenuePoolB, static_cast<uint64>(input.amount));
            // Update cumulative SC dividend tracker
            state.get().mScDividendTracker.get(input.sourceId, locals.prevCumulative);
            state.mut().mScDividendTracker.set(input.sourceId, sadd(locals.prevCumulative, static_cast<uint64>(input.amount)));
            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_INCOMING_SC_DIVIDEND;
            locals.logger.primaryId = input.sourceId;
            locals.logger.valueA = input.amount;
            locals.logger.valueB = sadd(locals.prevCumulative, static_cast<uint64>(input.amount));
            LOG_INFO(locals.logger);
            return;
        }

        // Revenue routing:
        // Pool A: mPoolARevenueAddress (QMINE issuer / mining revenue)
        // Pool C: Dedicated BTC revenue address (mDedicatedRevenueAddress)
        // Pool D: MLM Water revenue address (mPoolDRevenueAddress)
        // Pool B: Everything else (users, other contracts)
        if (state.get().mDedicatedRevenueAddress != NULL_ID && input.sourceId == state.get().mDedicatedRevenueAddress)
        {
            // Pool C: Dedicated BTC revenue address
            state.mut().mDedicatedRevenuePool = sadd(state.get().mDedicatedRevenuePool, static_cast<uint64>(input.amount));
            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_INCOMING_REVENUE_DEDICATED;
            locals.logger.primaryId = input.sourceId;
            locals.logger.valueA = input.amount;
            locals.logger.valueB = input.type;
            LOG_INFO(locals.logger);
        }
        else if (state.get().mPoolDRevenueAddress != NULL_ID && input.sourceId == state.get().mPoolDRevenueAddress)
        {
            // Pool D: MLM Water revenue address
            state.mut().mPoolDRevenuePool = sadd(state.get().mPoolDRevenuePool, static_cast<uint64>(input.amount));
            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_INCOMING_REVENUE_D;
            locals.logger.primaryId = input.sourceId;
            locals.logger.valueA = input.amount;
            locals.logger.valueB = input.type;
            LOG_INFO(locals.logger);
        }
        else if (state.get().mPoolARevenueAddress != NULL_ID && input.sourceId == state.get().mPoolARevenueAddress)
        {
            // Pool A: Direct transfer from Pool A revenue address only
            state.mut().mRevenuePoolA = sadd(state.get().mRevenuePoolA, static_cast<uint64>(input.amount));
            locals.logger.contractId = CONTRACT_INDEX;
            locals.logger.logType = QRWA_LOG_TYPE_INCOMING_REVENUE_A;
            locals.logger.primaryId = input.sourceId;
            locals.logger.valueA = input.amount;
            locals.logger.valueB = input.type;
            LOG_INFO(locals.logger);
        }
        else if (input.sourceId != NULL_ID)
        {
            // Pool B: All other sources (users, other contracts)
            state.mut().mRevenuePoolB = sadd(state.get().mRevenuePoolB, static_cast<uint64>(input.amount));
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

    struct POST_ACQUIRE_SHARES_locals
    {
        sint64 transferResult;
        uint64 currentAssetBalance;
        QRWAAsset wrapper;
        QRWALogger logger;
    };
    POST_ACQUIRE_SHARES_WITH_LOCALS()
    {
        // Automatically lock shares permanently: transfer ownership+possession from
        // the previous owner/possessor to SELF and record in mGeneralAssetBalances.
        // This allows any user to deposit SC shares in 2 steps:
        //   1. Buy shares on QX
        //   2. Transfer management rights to qRWA (this callback fires automatically)
        locals.transferResult = qpi.transferShareOwnershipAndPossession(
            input.asset.assetName,
            input.asset.issuer,
            input.owner,          // current owner
            input.possessor,      // current possessor
            input.numberOfShares,
            SELF                  // new owner and possessor (permanent lock)
        );

        locals.logger.contractId = CONTRACT_INDEX;
        locals.logger.logType = QRWA_LOG_TYPE_ADMIN_ACTION;
        locals.logger.primaryId = input.owner;
        locals.logger.valueA = input.asset.assetName;

        if (locals.transferResult >= 0)
        {
            locals.wrapper.setFrom(input.asset);
            state.get().mGeneralAssetBalances.get(locals.wrapper, locals.currentAssetBalance); // 0 if not present
            locals.currentAssetBalance = sadd(locals.currentAssetBalance, (uint64)input.numberOfShares);
            state.mut().mGeneralAssetBalances.set(locals.wrapper, locals.currentAssetBalance);

            // Register the asset issuer (SC contract ID) in the dividend tracker
            // so POST_INCOMING_TRANSFER can recognize its dividends.
            if (!state.get().mScDividendTracker.get(input.asset.issuer, locals.currentAssetBalance))
            {
                state.mut().mScDividendTracker.set(input.asset.issuer, 0);
            }

            locals.logger.valueB = QRWA_STATUS_SUCCESS;
        }
        else
        {
            locals.logger.valueB = QRWA_STATUS_FAILURE_TRANSFER_FAILED;
        }

        LOG_INFO(locals.logger);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        // PROCEDURES
        REGISTER_USER_PROCEDURE(DonateToTreasury, 3);
        REGISTER_USER_PROCEDURE(VoteGovParams, 4);
        REGISTER_USER_PROCEDURE(DepositGeneralAsset, 7);
        REGISTER_USER_PROCEDURE(RevokeAssetManagementRights, 8);
        REGISTER_USER_PROCEDURE(SetPoolARevenueAddress, 9);
        REGISTER_USER_PROCEDURE(SetPoolDRevenueAddress, 10);

        // FUNCTIONS
        REGISTER_USER_FUNCTION(GetGovParams, 1);
        REGISTER_USER_FUNCTION(GetGovPoll, 2);
        REGISTER_USER_FUNCTION(GetTreasuryBalance, 4);
        REGISTER_USER_FUNCTION(GetDividendBalances, 5);
        REGISTER_USER_FUNCTION(GetTotalDistributed, 6);
        REGISTER_USER_FUNCTION(GetActiveGovPollIds, 8);
        REGISTER_USER_FUNCTION(GetGeneralAssetBalance, 9);
        REGISTER_USER_FUNCTION(GetGeneralAssets, 10);
        REGISTER_USER_FUNCTION(GetPayoutsQmine, 11);
        REGISTER_USER_FUNCTION(GetContractAddresses, 12);
        REGISTER_USER_FUNCTION(GetPayoutsQrwa, 13);
        REGISTER_USER_FUNCTION(GetPayoutsDedicated, 14);
        REGISTER_USER_FUNCTION(GetScDividendTracking, 15);
        REGISTER_USER_FUNCTION(GetPayoutsPoolD, 16);

    }
};
