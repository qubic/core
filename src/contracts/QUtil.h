using namespace QPI;
/**************************************/
/**************SC UTILS****************/
/**************************************/
/*
* A collection of useful functions for smart contract on Qubic:
* - SendToManyV1 (STM1): Sending qu from a single address to multiple addresses (upto 25)
* - SendToManyBenchmark: Sending n transfers of 1 qu each to the specified number of addresses
* - CreatePoll: Create a poll with a name, type, min amount, and GitHub link, and list of allowed assets
* - Vote: Cast a Vote in a poll with a specified option (0 to 63), charging 100 QUs
* - GetCurrentResult: Retrieve the current voting results for a poll (options 0 to 63)
* - GetPollsByCreator: Retrieve all poll IDs created by a specific address
* - GetCurrentPollId: Retrieve the current poll ID
*/

// Return code for logger and return struct
constexpr uint64 QUTIL_STM1_SUCCESS = 0;
constexpr uint64 QUTIL_STM1_INVALID_AMOUNT_NUMBER = 1;
constexpr uint64 QUTIL_STM1_WRONG_FUND = 2;
constexpr uint64 QUTIL_STM1_TRIGGERED = 3;
constexpr uint64 QUTIL_STM1_SEND_FUND = 4;
constexpr sint64 QUTIL_STM1_INVOCATION_FEE = 10LL; // fee to be burned and make the SC running

// Voting-specific constants
constexpr uint64 QUTIL_POLL_TYPE_QUBIC = 1;
constexpr uint64 QUTIL_POLL_TYPE_ASSET = 2; // Can be either shares or tokens
constexpr uint64 QUTIL_MAX_POLL = 64;
constexpr uint64 QUTIL_MAX_VOTERS_PER_POLL = 131072;
constexpr uint64 QUTIL_TOTAL_VOTERS = QUTIL_MAX_POLL * QUTIL_MAX_VOTERS_PER_POLL;
constexpr uint64 QUTIL_MAX_OPTIONS = 64; // Maximum voting options (0 to 63)
constexpr uint64 QUTIL_MAX_ASSETS_PER_POLL = 16; // Maximum assets per poll
constexpr sint64 QUTIL_VOTE_FEE = 100LL; // Fee for voting, burnt 100%
constexpr sint64 QUTIL_POLL_CREATION_FEE = 10000000LL; // Fee for poll creation to prevent spam
constexpr uint16 QUTIL_POLL_GITHUB_URL_MAX_SIZE = 256; // Max String Length for Poll's Github URLs
constexpr uint64 QUTIL_MAX_NEW_POLL = div(QUTIL_MAX_POLL, 4ULL); // Max number of new poll per epoch


// Voting log types enum
constexpr uint64 QUTILLogTypePollCreated = 5;                       // Poll created successfully
constexpr uint64 QUTILLogTypeInsufficientFundsForPoll = 6;          // Insufficient funds for poll creation
constexpr uint64 QUTILLogTypeInvalidPollType = 7;                   // Invalid poll type
constexpr uint64 QUTILLogTypeInvalidNumAssetsQubic = 8;             // Invalid number of assets for Qubic poll
constexpr uint64 QUTILLogTypeInvalidNumAssetsAsset = 9;             // Invalid number of assets for Asset poll
constexpr uint64 QUTILLogTypeVoteCast = 10;                         // Vote cast successfully
constexpr uint64 QUTILLogTypeInsufficientFundsForVote = 11;         // Insufficient funds for voting
constexpr uint64 QUTILLogTypeInvalidPollId = 12;                    // Invalid poll ID
constexpr uint64 QUTILLogTypePollInactive = 13;                     // Poll is inactive
constexpr uint64 QUTILLogTypeInsufficientBalance = 14;              // Insufficient voter balance
constexpr uint64 QUTILLogTypeInvalidOption = 15;                    // Invalid voting option
constexpr uint64 QUTILLogTypeInvalidPollIdResult = 16;              // Invalid poll ID in GetCurrentResult
constexpr uint64 QUTILLogTypePollInactiveResult = 17;               // Poll inactive in GetCurrentResult
constexpr uint64 QUTILLogTypeNoPollsByCreator = 18;                 // No polls found in GetPollsByCreator
constexpr uint64 QUTILLogTypePollCancelled = 19;                    // Poll cancelled successfully
constexpr uint64 QUTILLogTypeNotAuthorized = 20;                    // Not authorized to cancel the poll
constexpr uint64 QUTILLogTypeInsufficientFundsForCancel = 21;       // Not have enough funds for poll calcellation
constexpr uint64 QUTILLogTypeMaxPollsReached = 22;                  // Max epoch per epoch reached

// Fee per shareholder for DistributeQuToShareholders()
constexpr sint64 QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER = 5;


struct QUTILLogger
{
    uint32 contractId; // to distinguish bw SCs
    uint32 padding;
    id src;
    id dst;
    sint64 amt;
    uint32 logtype;
    // Other data go here
    sint8 _terminator; // Only data before "_terminator" are logged
};

// Deactivate logger for delay function
#if 0
struct QUTILDFLogger
{
    uint32 contractId; // to distinguish bw SCs
    uint32 padding;
    id dfNonce;
    id dfPubkey;
    id dfMiningSeed;
    id result;
    sint8 _terminator; // Only data before "_terminator" are logged
};
#endif

// poll and voter structs
struct QUTILPoll {
    id poll_name;
    uint64 poll_type; // QUTIL_POLL_TYPE_QUBIC or QUTIL_POLL_TYPE_ASSET
    uint64 min_amount; // Minimum Qubic/asset amount for eligibility
    uint64 is_active;
    id creator; // Address that created the poll
    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets; // List of allowed assets for QUTIL_POLL_TYPE_ASSET
    uint64 num_assets; // Number of assets in allowed assets
};

struct QUTILVoter {
    id address;
    uint64 amount;
    uint64 chosen_option; // Limited to 0-63 by vote procedure
};

struct QUTIL2
{
};

struct QUTIL : public ContractBase
{
private:
    // registers, logger and other buffer
    sint32 _i0, _i1, _i2, _i3;
    sint64 _i64_0, _i64_1, _i64_2, _i64_3;
    uint64 _r0, _r1, _r2, _r3;
    sint64 total;

    // Voting state
    Array<QUTILPoll, QUTIL_MAX_POLL> polls;
    Array<QUTILVoter, QUTIL_TOTAL_VOTERS> voters; // 1d array for all voters
    Array<uint64, QUTIL_MAX_POLL> poll_ids;
    Array<uint64, QUTIL_MAX_POLL> voter_counts; // tracks number of voters per poll
    Array<Array<uint8, QUTIL_POLL_GITHUB_URL_MAX_SIZE>, QUTIL_MAX_POLL> poll_links; // github links for polls
    uint64 current_poll_id;
    uint64 new_polls_this_epoch;

    // DF function variables
    m256i dfMiningSeed;
    m256i dfCurrentState;

    // Get Qubic Balance
    struct get_qubic_balance_input {
        id address;
    };
    struct get_qubic_balance_output {
        uint64 balance;
    };
    struct get_qubic_balance_locals {
        QPI::Entity entity;
    };

    // Get Asset Balance
    struct get_asset_balance_input {
        id address;
        Asset asset;
    };
    struct get_asset_balance_output {
        uint64 balance;
    };
    struct get_asset_balance_locals {
    };

    // Get QUTILVoter Balance Helper
    struct get_voter_balance_input {
        uint64 poll_idx;
        id address;
    };
    struct get_voter_balance_output {
        uint64 balance;
    };
    struct get_voter_balance_locals {
        uint64 poll_type;
        uint64 balance;
        uint64 max_balance;
        uint64 asset_idx;
        Asset current_asset;
        get_qubic_balance_input gqb_input;
        get_qubic_balance_output gqb_output;
        get_qubic_balance_locals gqb_locals;
        get_asset_balance_input gab_input;
        get_asset_balance_output gab_output;
        get_asset_balance_locals gab_locals;
    };

    // Swap QUTILVoter to the end of the array helper
    struct swap_voter_to_end_input {
        uint64 poll_idx;
        uint64 i;
        uint64 end_idx;
    };
    struct swap_voter_to_end_output {
    };
    struct swap_voter_to_end_locals {
        uint64 voter_index_i;
        uint64 voter_index_end;
        QUTILVoter temp_voter;
    };

public:
    /**************************************/
    /********INPUT AND OUTPUT STRUCTS******/
    /**************************************/
    struct SendToManyV1_input
    {
        id dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, dst8, dst9, dst10, dst11, dst12,
            dst13, dst14, dst15, dst16, dst17, dst18, dst19, dst20, dst21, dst22, dst23, dst24;
        sint64 amt0, amt1, amt2, amt3, amt4, amt5, amt6, amt7, amt8, amt9, amt10, amt11, amt12,
            amt13, amt14, amt15, amt16, amt17, amt18, amt19, amt20, amt21, amt22, amt23, amt24;
    };
    struct SendToManyV1_output
    {
        sint32 returnCode;
    };
    struct SendToManyV1_locals
    {
        QUTILLogger logger;
    };

    struct GetSendToManyV1Fee_input
    {
    };
    struct GetSendToManyV1Fee_output
    {
        sint64 fee;
    };

    struct SendToManyBenchmark_input
    {
        sint64 dstCount;
        sint64 numTransfersEach;
    };
    struct SendToManyBenchmark_output
    {
        sint64 dstCount;
        sint32 returnCode;
        sint64 total;
    };

    struct SendToManyBenchmark_locals
    {
        id currentId;
        sint64 t;
        uint64 useNext;
        uint64 totalNumTransfers;
        QUTILLogger logger;
    };

    struct BurnQubic_input
    {
        sint64 amount;
    };
    struct BurnQubic_output
    {
        sint64 amount;
    };

    typedef Asset GetTotalNumberOfAssetShares_input;
    typedef sint64 GetTotalNumberOfAssetShares_output;

    struct CreatePoll_input
    {
        id poll_name;
        uint64 poll_type; // QUTIL_POLL_TYPE_QUBIC or QUTIL_POLL_TYPE_ASSET
        uint64 min_amount; // Minimum Qubic/asset amount
        Array<uint8, QUTIL_POLL_GITHUB_URL_MAX_SIZE> github_link; // GitHub link
        Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets; // List of allowed assets for QUTIL_POLL_TYPE_ASSET
        uint64 num_assets; // Number of assets in allowed_assets
    };
    struct CreatePoll_output
    {
        uint64 poll_id;
    };
    struct CreatePoll_locals
    {
        uint64 idx;
        QUTILPoll new_poll;
        QUTILVoter default_voter;
        uint64 i;
        QUTILLogger logger;
    };

    struct Vote_input
    {
        uint64 poll_id;
        id address;
        uint64 amount;
        uint64 chosen_option; // Limited to 0-63
    };
    struct Vote_output
    {
        bit success;
    };
    struct Vote_locals
    {
        uint64 idx;
        uint64 balance;
        uint64 poll_type;
        sint64 voter_idx;
        get_voter_balance_input gvb_input;
        get_voter_balance_output gvb_output;
        get_voter_balance_locals gvb_locals;
        swap_voter_to_end_input sve_input;
        swap_voter_to_end_output sve_output;
        swap_voter_to_end_locals sve_locals;
        uint64 i;
        uint64 voter_index;
        QUTILVoter temp_voter;
        uint64 real_vote;
        uint64 end_idx;
        uint64 max_balance;
        QUTILLogger logger;
    };

    struct CancelPoll_input
    {
        uint64 poll_id;
    };

    struct CancelPoll_output
    {
        bit success;
    };

    struct CancelPoll_locals
    {
        uint64 idx;
        QUTILPoll current_poll;
        QUTILLogger logger;
    };

    struct GetCurrentResult_input
    {
        uint64 poll_id;
    };
    struct GetCurrentResult_output
    {
        Array<uint64, QUTIL_MAX_OPTIONS> result;  // Total voting power for each option
        Array<uint64, QUTIL_MAX_OPTIONS> voter_count;  // Number of voters for each option
        uint64 is_active;
    };
    struct GetCurrentResult_locals
    {
        uint64 idx;
        uint64 poll_type;
        uint64 effective_amount;
        QUTILVoter voter;
        uint64 i;
        uint64 voter_index;
        QUTILLogger logger;
    };

    struct GetPollsByCreator_input
    {
        id creator;
    };
    struct GetPollsByCreator_output
    {
        Array<uint64, QUTIL_MAX_POLL> poll_ids;
        uint64 count;
    };
    struct GetPollsByCreator_locals
    {
        uint64 idx;
        QUTILLogger logger;
    };

    struct GetCurrentPollId_input
    {
    };

    struct GetCurrentPollId_output
    {
        uint64 current_poll_id; // All current and past poll IDs must be below this number
        Array<uint64, QUTIL_MAX_POLL> active_poll_ids;
        uint64 active_count;
    };

    struct GetCurrentPollId_locals
    {
        uint64 i;
    };

    struct GetPollInfo_input {
        uint64 poll_id;
    };

    struct GetPollInfo_output {
        uint64 found; // 1 if exists, 0 ig not
        QUTILPoll poll_info;
        Array<uint8, QUTIL_POLL_GITHUB_URL_MAX_SIZE> poll_link;
    };

    struct GetPollInfo_locals {
        uint64 idx;
        QUTILPoll default_poll;  // default values if not found
    };

    struct END_EPOCH_locals
    {
        uint64 i;
        QUTILPoll current_poll;
    };

    /**************************************/
    /***********HELPER FUNCTIONS***********/
    /**************************************/

    PRIVATE_FUNCTION_WITH_LOCALS(get_qubic_balance)
    {
        output.balance = 0;
        if (qpi.getEntity(input.address, locals.entity))
        {
            output.balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
        }
    }

    PRIVATE_FUNCTION_WITH_LOCALS(get_asset_balance)
    {
        output.balance = qpi.numberOfShares(input.asset, AssetOwnershipSelect::byOwner(input.address), AssetPossessionSelect::byPossessor(input.address));
    }

    PRIVATE_FUNCTION_WITH_LOCALS(get_voter_balance)
    {
        output.balance = 0;
        locals.poll_type = state.polls.get(input.poll_idx).poll_type;
        if (locals.poll_type == QUTIL_POLL_TYPE_QUBIC)
        {
            locals.gqb_input.address = input.address;
            get_qubic_balance(qpi, state, locals.gqb_input, locals.gqb_output, locals.gqb_locals);
            output.balance = locals.gqb_output.balance;
        }
        else if (locals.poll_type == QUTIL_POLL_TYPE_ASSET)
        {
            locals.max_balance = 0;
            for (locals.asset_idx = 0; locals.asset_idx < state.polls.get(input.poll_idx).num_assets; locals.asset_idx++)
            {
                locals.current_asset = state.polls.get(input.poll_idx).allowed_assets.get(locals.asset_idx);
                locals.gab_input.address = input.address;
                locals.gab_input.asset = locals.current_asset;
                get_asset_balance(qpi, state, locals.gab_input, locals.gab_output, locals.gab_locals);
                if (locals.gab_output.balance > locals.max_balance)
                {
                    locals.max_balance = locals.gab_output.balance;
                }
            }
            output.balance = locals.max_balance;
        }
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(swap_voter_to_end)
    {
        locals.voter_index_i = calculate_voter_index(input.poll_idx, input.i);
        locals.voter_index_end = calculate_voter_index(input.poll_idx, input.end_idx);
        locals.temp_voter = state.voters.get(locals.voter_index_i);
        state.voters.set(locals.voter_index_i, state.voters.get(locals.voter_index_end));
        state.voters.set(locals.voter_index_end, locals.temp_voter);
    }

    // Calculate QUTILVoter Index
    inline static uint64 calculate_voter_index(uint64 poll_idx, uint64 voter_idx)
    {
        return poll_idx * QUTIL_MAX_VOTERS_PER_POLL + voter_idx;
    }

    static inline bit check_github_prefix(const Array<uint8, QUTIL_POLL_GITHUB_URL_MAX_SIZE>& github_link)
    {
        return github_link.get(0) == 104 && // 'h'
            github_link.get(1) == 116 && // 't'
            github_link.get(2) == 116 && // 't'
            github_link.get(3) == 112 && // 'p'
            github_link.get(4) == 115 && // 's'
            github_link.get(5) == 58 && // ':'
            github_link.get(6) == 47 && // '/'
            github_link.get(7) == 47 && // '/'
            github_link.get(8) == 103 && // 'g'
            github_link.get(9) == 105 && // 'i'
            github_link.get(10) == 116 && // 't'
            github_link.get(11) == 104 && // 'h'
            github_link.get(12) == 117 && // 'u'
            github_link.get(13) == 98 && // 'b'
            github_link.get(14) == 46 && // '.'
            github_link.get(15) == 99 && // 'c'
            github_link.get(16) == 111 && // 'o'
            github_link.get(17) == 109 && // 'm'
            github_link.get(18) == 47 && // '/'
            github_link.get(19) == 113 && // 'q'
            github_link.get(20) == 117 && // 'u'
            github_link.get(21) == 98 && // 'b'
            github_link.get(22) == 105 && // 'i'
            github_link.get(23) == 99; // 'c'
    }

    /**************************************/
    /************CORE FUNCTIONS************/
    /**************************************/
    /*
    * @return return SendToManyV1 fee per invocation
    */
    PUBLIC_FUNCTION(GetSendToManyV1Fee)
    {
        output.fee = QUTIL_STM1_INVOCATION_FEE;
    }

    /**
    * Send qu from a single address to multiple addresses
    * @param list of 25 destination addresses (800 bytes): 32 bytes for each address, leave empty(zeroes) for unused memory space
    * @param list of 25 amounts (200 bytes): 8 bytes(long long) for each amount, leave empty(zeroes) for unused memory space
    * @return returnCode (0 means success)
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(SendToManyV1)
    {
        locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_TRIGGERED };
        LOG_INFO(locals.logger);

         // invalid amount (<0 or >= MAX_AMOUNT), return fund and exit
        if ((input.amt0 < 0) || (input.amt0 >= MAX_AMOUNT)
            || (input.amt1 < 0) || (input.amt1 >= MAX_AMOUNT)
            || (input.amt2 < 0) || (input.amt2 >= MAX_AMOUNT)
            || (input.amt3 < 0) || (input.amt3 >= MAX_AMOUNT)
            || (input.amt4 < 0) || (input.amt4 >= MAX_AMOUNT)
            || (input.amt5 < 0) || (input.amt5 >= MAX_AMOUNT)
            || (input.amt6 < 0) || (input.amt6 >= MAX_AMOUNT)
            || (input.amt7 < 0) || (input.amt7 >= MAX_AMOUNT)
            || (input.amt8 < 0) || (input.amt8 >= MAX_AMOUNT)
            || (input.amt9 < 0) || (input.amt9 >= MAX_AMOUNT)
            || (input.amt10 < 0) || (input.amt10 >= MAX_AMOUNT)
            || (input.amt11 < 0) || (input.amt11 >= MAX_AMOUNT)
            || (input.amt12 < 0) || (input.amt12 >= MAX_AMOUNT)
            || (input.amt13 < 0) || (input.amt13 >= MAX_AMOUNT)
            || (input.amt14 < 0) || (input.amt14 >= MAX_AMOUNT)
            || (input.amt15 < 0) || (input.amt15 >= MAX_AMOUNT)
            || (input.amt16 < 0) || (input.amt16 >= MAX_AMOUNT)
            || (input.amt17 < 0) || (input.amt17 >= MAX_AMOUNT)
            || (input.amt18 < 0) || (input.amt18 >= MAX_AMOUNT)
            || (input.amt19 < 0) || (input.amt19 >= MAX_AMOUNT)
            || (input.amt20 < 0) || (input.amt20 >= MAX_AMOUNT)
            || (input.amt21 < 0) || (input.amt21 >= MAX_AMOUNT)
            || (input.amt22 < 0) || (input.amt22 >= MAX_AMOUNT)
            || (input.amt23 < 0) || (input.amt23 >= MAX_AMOUNT)
            || (input.amt24 < 0) || (input.amt24 >= MAX_AMOUNT))
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_INVALID_AMOUNT_NUMBER };
            output.returnCode = QUTIL_STM1_INVALID_AMOUNT_NUMBER;
            LOG_INFO(locals.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
        }

        // Make sure that the sum of all amounts does not overflow and is equal to qpi.invocationReward()
        state.total = qpi.invocationReward();
        state.total -= input.amt0; if (state.total < 0) goto exit;
        state.total -= input.amt1; if (state.total < 0) goto exit;
        state.total -= input.amt2; if (state.total < 0) goto exit;
        state.total -= input.amt3; if (state.total < 0) goto exit;
        state.total -= input.amt4; if (state.total < 0) goto exit;
        state.total -= input.amt5; if (state.total < 0) goto exit;
        state.total -= input.amt6; if (state.total < 0) goto exit;
        state.total -= input.amt7; if (state.total < 0) goto exit;
        state.total -= input.amt8; if (state.total < 0) goto exit;
        state.total -= input.amt9; if (state.total < 0) goto exit;
        state.total -= input.amt10; if (state.total < 0) goto exit;
        state.total -= input.amt11; if (state.total < 0) goto exit;
        state.total -= input.amt12; if (state.total < 0) goto exit;
        state.total -= input.amt13; if (state.total < 0) goto exit;
        state.total -= input.amt14; if (state.total < 0) goto exit;
        state.total -= input.amt15; if (state.total < 0) goto exit;
        state.total -= input.amt16; if (state.total < 0) goto exit;
        state.total -= input.amt17; if (state.total < 0) goto exit;
        state.total -= input.amt18; if (state.total < 0) goto exit;
        state.total -= input.amt19; if (state.total < 0) goto exit;
        state.total -= input.amt20; if (state.total < 0) goto exit;
        state.total -= input.amt21; if (state.total < 0) goto exit;
        state.total -= input.amt22; if (state.total < 0) goto exit;
        state.total -= input.amt23; if (state.total < 0) goto exit;
        state.total -= input.amt24; if (state.total < 0) goto exit;
        state.total -= QUTIL_STM1_INVOCATION_FEE; if (state.total < 0) goto exit;

        // insufficient or too many qubic transferred, return fund and exit (we don't want to return change)
        if (state.total != 0)
        {
        exit:
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_WRONG_FUND };
            LOG_INFO(locals.logger);
            output.returnCode = QUTIL_STM1_WRONG_FUND;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (input.dst0 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst0, input.amt0, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst0, input.amt0);
        }
        if (input.dst1 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst1, input.amt1, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst1, input.amt1);
        }
        if (input.dst2 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst2, input.amt2, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst2, input.amt2);
        }
        if (input.dst3 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst3, input.amt3, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst3, input.amt3);
        }
        if (input.dst4 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst4, input.amt4, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst4, input.amt4);
        }
        if (input.dst5 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst5, input.amt5, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst5, input.amt5);
        }
        if (input.dst6 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst6, input.amt6, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst6, input.amt6);
        }
        if (input.dst7 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst7, input.amt7, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst7, input.amt7);
        }
        if (input.dst8 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst8, input.amt8, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst8, input.amt8);
        }
        if (input.dst9 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst9, input.amt9, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst9, input.amt9);
        }
        if (input.dst10 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst10, input.amt10, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst10, input.amt10);
        }
        if (input.dst11 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst11, input.amt11, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst11, input.amt11);
        }
        if (input.dst12 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst12, input.amt12, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst12, input.amt12);
        }
        if (input.dst13 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst13, input.amt13, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst13, input.amt13);
        }
        if (input.dst14 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst14, input.amt14, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst14, input.amt14);
        }
        if (input.dst15 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst15, input.amt15, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst15, input.amt15);
        }
        if (input.dst16 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst16, input.amt16, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst16, input.amt16);
        }
        if (input.dst17 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst17, input.amt17, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst17, input.amt17);
        }
        if (input.dst18 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst18, input.amt18, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst18, input.amt18);
        }
        if (input.dst19 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst19, input.amt19, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst19, input.amt19);
        }
        if (input.dst20 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst20, input.amt20, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst20, input.amt20);
        }
        if (input.dst21 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst21, input.amt21, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst21, input.amt21);
        }
        if (input.dst22 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst22, input.amt22, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst22, input.amt22);
        }
        if (input.dst23 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst23, input.amt23, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst23, input.amt23);
        }
        if (input.dst24 != NULL_ID)
        {
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), input.dst24, input.amt24, QUTIL_STM1_SEND_FUND };
            LOG_INFO(locals.logger);
            qpi.transfer(input.dst24, input.amt24);
        }
        locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_SUCCESS};
        LOG_INFO(locals.logger);
        output.returnCode = QUTIL_STM1_SUCCESS;
        qpi.burn(QUTIL_STM1_INVOCATION_FEE);
    }

    /**
    * Send n transfers of 1 qu each from a single address to a specified number of addresses.
    * If there are not enough entities in the spectrum, a single entity might be chosen multiple times.
    * @param number of addresses that will be sent n times 1 qubic
    * @param number of transfers for each address
    * @return returnCode (0 means success)
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(SendToManyBenchmark)
    {
        locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_TRIGGERED };
        LOG_INFO(locals.logger);
        output.total = 0;

        // Number of addresses and transfers is > 0 and total transfers do not exceed limit (including 2 transfers from invocator to contract and contract to invocator)
        locals.totalNumTransfers = smul((uint64)input.dstCount, (uint64)input.numTransfersEach);
        if (input.dstCount <= 0 || input.numTransfersEach <= 0 || locals.totalNumTransfers > CONTRACT_ACTION_TRACKER_SIZE - 2)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_INVALID_AMOUNT_NUMBER };
            LOG_INFO(locals.logger);
            output.returnCode = QUTIL_STM1_INVALID_AMOUNT_NUMBER;
            return;
        }

        // Check the fund is enough
        if ((uint64)qpi.invocationReward() < locals.totalNumTransfers)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_INVALID_AMOUNT_NUMBER };
            LOG_INFO(locals.logger);
            output.returnCode = QUTIL_STM1_INVALID_AMOUNT_NUMBER;
            return;
        }

        // Loop through the number of addresses and do the transfers
        locals.currentId = qpi.invocator();
        locals.useNext = 1;
        while (output.dstCount < input.dstCount)
        {
            if (locals.useNext == 1)
                locals.currentId = qpi.nextId(locals.currentId);
            else
                locals.currentId = qpi.prevId(locals.currentId);
            if (locals.currentId == id::zero())
            {
                locals.currentId = qpi.invocator();
                locals.useNext = 1 - locals.useNext;
                continue;
            }

            output.dstCount++;
            for (locals.t = 0; locals.t < input.numTransfersEach; locals.t++)
            {
                qpi.transfer(locals.currentId, 1);
                output.total += 1;
            }
        }

        // Return the change if there is any
        if (output.total < qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - output.total);
        }

        locals.logger = QUTILLogger{ 0,  0, qpi.invocator(), SELF, output.total, QUTIL_STM1_SUCCESS };
        LOG_INFO(locals.logger);
    }

    /**
    * Practicing burning qubic in the QChurch
    * @param the amount of qubic to burn
    * @return the amount of qubic has burned, < 0 if failed to burn
    */
    PUBLIC_PROCEDURE(BurnQubic)
    {
        // lack of fund => return the coins
        if (input.amount < 0) // invalid input amount
        {
            output.amount = -1;
            return;
        }
        if (input.amount == 0)
        {
            output.amount = 0;
            return;
        }
        if (qpi.invocationReward() < input.amount) // not sending enough qu to burn
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.amount = -1;
            return;
        }
        if (qpi.invocationReward() > input.amount) // send more than qu to burn
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount); // return the changes
        }
        qpi.burn(input.amount);
        output.amount = input.amount;
        return;
    }

    /**
    * Create a new poll with min amount, and GitHub link, and list of allowed assets
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(CreatePoll)
    {
        // max new poll exceeded
        if (state.new_polls_this_epoch >= QUTIL_MAX_NEW_POLL)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeMaxPollsReached };
            LOG_INFO(locals.logger);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // insufficient fund
        if (qpi.invocationReward() < QUTIL_POLL_CREATION_FEE)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, qpi.invocationReward(), QUTILLogTypeInsufficientFundsForPoll };
            LOG_INFO(locals.logger);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // invalid poll type
        if (input.poll_type != QUTIL_POLL_TYPE_QUBIC && input.poll_type != QUTIL_POLL_TYPE_ASSET)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidPollType };
            LOG_INFO(locals.logger);
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // invalid number of assets in Qubic poll
        if (input.poll_type == QUTIL_POLL_TYPE_QUBIC && input.num_assets != 0)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidNumAssetsQubic };
            LOG_INFO(locals.logger);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // invalid number of assets in Asset poll
        if (input.poll_type == QUTIL_POLL_TYPE_ASSET && (input.num_assets == 0 || input.num_assets > QUTIL_MAX_ASSETS_PER_POLL))
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidNumAssetsAsset };
            LOG_INFO(locals.logger);
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (!check_github_prefix(input.github_link))
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidPollType }; // reusing existing log type for invalid GitHub link
            LOG_INFO(locals.logger);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        qpi.transfer(qpi.invocator(), qpi.invocationReward() - QUTIL_POLL_CREATION_FEE);
        qpi.burn(QUTIL_POLL_CREATION_FEE);

        locals.idx = mod(state.current_poll_id, QUTIL_MAX_POLL);
        locals.new_poll.poll_name = input.poll_name;
        locals.new_poll.poll_type = input.poll_type;
        locals.new_poll.min_amount = input.min_amount;
        locals.new_poll.is_active = 1;
        locals.new_poll.creator = qpi.invocator();
        locals.new_poll.allowed_assets = input.allowed_assets;
        locals.new_poll.num_assets = input.num_assets;

        state.polls.set(locals.idx, locals.new_poll);
        state.poll_ids.set(locals.idx, state.current_poll_id);
        state.poll_links.set(locals.idx, input.github_link);
        state.voter_counts.set(locals.idx, 0);

        locals.default_voter.address = NULL_ID;
        locals.default_voter.amount = 0;
        locals.default_voter.chosen_option = 0;

        for (locals.i = 0; locals.i < QUTIL_MAX_VOTERS_PER_POLL; locals.i++)
        {
            state.voters.set(calculate_voter_index(locals.idx, locals.i), locals.default_voter);
        }
        output.poll_id = state.current_poll_id;
        state.current_poll_id++;

        state.new_polls_this_epoch++;

        locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, QUTIL_POLL_CREATION_FEE, QUTILLogTypePollCreated };
        LOG_INFO(locals.logger);
    }

    /**
    * Cast a vote in a poll, charging 100 QUs, limiting options to 0-63
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(Vote)
    {
        output.success = false;
        if (qpi.invocationReward() < QUTIL_VOTE_FEE)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, qpi.invocationReward(), QUTILLogTypeInsufficientFundsForVote };
            LOG_INFO(locals.logger);
            return;
        }
        qpi.transfer(qpi.invocator(), qpi.invocationReward() - QUTIL_VOTE_FEE);
        qpi.burn(QUTIL_VOTE_FEE);

        locals.idx = mod(input.poll_id, QUTIL_MAX_POLL);

        if (state.poll_ids.get(locals.idx) != input.poll_id)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidPollId };
            LOG_INFO(locals.logger);
            return;
        }
        if (state.polls.get(locals.idx).is_active == 0)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypePollInactive };
            LOG_INFO(locals.logger);
            return;
        }
        locals.poll_type = state.polls.get(locals.idx).poll_type;

        // Fetch voter balance using helper function
        locals.gvb_input.poll_idx = locals.idx;
        locals.gvb_input.address = input.address;
        get_voter_balance(qpi, state, locals.gvb_input, locals.gvb_output, locals.gvb_locals);
        locals.max_balance = locals.gvb_output.balance;

        if (locals.max_balance < state.polls.get(locals.idx).min_amount || locals.max_balance < input.amount)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInsufficientBalance };
            LOG_INFO(locals.logger);
            return;
        }
        if (input.chosen_option >= QUTIL_MAX_OPTIONS)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidOption };
            LOG_INFO(locals.logger);
            return;
        }

        // Search for existing voter or empty slot
        for (locals.i = 0; locals.i < QUTIL_MAX_VOTERS_PER_POLL; locals.i++)
        {
            locals.voter_index = calculate_voter_index(locals.idx, locals.i);
            if (state.voters.get(locals.voter_index).address == input.address)
            {
                // Update existing voter
                state.voters.set(locals.voter_index, QUTILVoter{ input.address, input.amount, input.chosen_option });
                output.success = true;
                break;
            }
            else if (state.voters.get(locals.voter_index).address == NULL_ID)
            {
                // Add new voter in empty slot
                state.voters.set(locals.voter_index, QUTILVoter{ input.address, input.amount, input.chosen_option });
                state.voter_counts.set(locals.idx, state.voter_counts.get(locals.idx) + 1);
                output.success = true;
                break;
            }
        }

        if (!output.success)
        {
            return;
        }

        // Update voter balances and compact the voter list
        locals.real_vote = 0;
        locals.end_idx = state.voter_counts.get(locals.idx) - 1;
        locals.i = 0;

        while (locals.i <= locals.end_idx)
        {
            locals.voter_index = calculate_voter_index(locals.idx, locals.i);
            locals.temp_voter = state.voters.get(locals.voter_index);
            if (locals.temp_voter.address == NULL_ID)
            {
                // Swap with the last valid voter
                while (locals.end_idx > locals.i && state.voters.get(calculate_voter_index(locals.idx, locals.end_idx)).address == NULL_ID)
                {
                    locals.end_idx--;
                }
                if (locals.end_idx > locals.i)
                {
                    locals.sve_input.poll_idx = locals.idx;
                    locals.sve_input.i = locals.i;
                    locals.sve_input.end_idx = locals.end_idx;
                    swap_voter_to_end(qpi, state, locals.sve_input, locals.sve_output, locals.sve_locals);
                    locals.end_idx--;
                    continue;
                }
                else
                {
                    locals.i++;
                    continue;
                }
            }

            // Update voter balance
            if (locals.temp_voter.address != NULL_ID)
            {
                locals.gvb_input.poll_idx = locals.idx;
                locals.gvb_input.address = locals.temp_voter.address;
                get_voter_balance(qpi, state, locals.gvb_input, locals.gvb_output, locals.gvb_locals);
                locals.max_balance = locals.gvb_output.balance;

                if (locals.max_balance < state.polls.get(locals.idx).min_amount)
                {
                    // Mark as invalid by setting address to NULL_ID
                    state.voters.set(locals.voter_index, QUTILVoter{ NULL_ID, 0, 0 });
                    // Swap with the last valid voter
                    while (locals.end_idx > locals.i && state.voters.get(calculate_voter_index(locals.idx, locals.end_idx)).address == NULL_ID)
                    {
                        locals.end_idx--;
                    }
                    if (locals.end_idx > locals.i)
                    {
                        locals.sve_input.poll_idx = locals.idx;
                        locals.sve_input.i = locals.i;
                        locals.sve_input.end_idx = locals.end_idx;
                        swap_voter_to_end(qpi, state, locals.sve_input, locals.sve_output, locals.sve_locals);
                        locals.end_idx--;
                        continue;
                    }
                }
                else
                {
                    locals.temp_voter.amount = locals.max_balance < locals.temp_voter.amount ? locals.max_balance : locals.temp_voter.amount;
                    state.voters.set(locals.voter_index, locals.temp_voter);
                    locals.real_vote++;
                }
            }
            locals.i++;
        }

        // Update voter count
        state.voter_counts.set(locals.idx, locals.real_vote);

        if (output.success)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, QUTIL_VOTE_FEE, QUTILLogTypeVoteCast };
            LOG_INFO(locals.logger);
        }
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(CancelPoll)
    {
        if (qpi.invocationReward() < QUTIL_POLL_CREATION_FEE)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, qpi.invocationReward(), QUTILLogTypeInsufficientFundsForCancel };
            LOG_INFO(locals.logger);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.success = false;
            return;
        }

        locals.idx = mod(input.poll_id, QUTIL_MAX_POLL);

        if (state.poll_ids.get(locals.idx) != input.poll_id)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidPollId };
            LOG_INFO(locals.logger);
            output.success = false;
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.current_poll = state.polls.get(locals.idx);

        if (locals.current_poll.creator != qpi.invocator())
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeNotAuthorized };
            LOG_INFO(locals.logger);
            output.success = false;
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (locals.current_poll.is_active == 0)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypePollInactive };
            LOG_INFO(locals.logger);
            output.success = false;
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.current_poll.is_active = 0;
        state.polls.set(locals.idx, locals.current_poll);

        locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypePollCancelled };
        LOG_INFO(locals.logger);
        output.success = true;

        qpi.transfer(qpi.invocator(), qpi.invocationReward() - QUTIL_POLL_CREATION_FEE);
        qpi.burn(QUTIL_POLL_CREATION_FEE);
    }

    /**
    * Get the current voting results for a poll
    */
    PUBLIC_FUNCTION_WITH_LOCALS(GetCurrentResult)
    {
        locals.idx = mod(input.poll_id, QUTIL_MAX_POLL);
        if (state.poll_ids.get(locals.idx) != input.poll_id)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeInvalidPollIdResult };
            LOG_INFO(locals.logger);
            return;
        }
        output.is_active = state.polls.get(locals.idx).is_active;
        for (locals.i = 0; locals.i < state.voter_counts.get(locals.idx); locals.i++)
        {
            locals.voter_index = calculate_voter_index(locals.idx, locals.i);
            locals.voter = state.voters.get(locals.voter_index);
            if (locals.voter.address != NULL_ID && locals.voter.chosen_option < QUTIL_MAX_OPTIONS)
            {
                output.result.set(locals.voter.chosen_option, output.result.get(locals.voter.chosen_option) + locals.voter.amount);
                output.voter_count.set(locals.voter.chosen_option, output.voter_count.get(locals.voter.chosen_option) + 1);
            }
        }
    }

    /**
    * Get all poll IDs created by a specific address
    */
    PUBLIC_FUNCTION_WITH_LOCALS(GetPollsByCreator)
    {
        output.count = 0;
        for (locals.idx = 0; locals.idx < QUTIL_MAX_POLL; locals.idx++)
        {
            if (state.polls.get(locals.idx).is_active != 0 && state.polls.get(locals.idx).creator == input.creator)
            {
                output.poll_ids.set(output.count, state.poll_ids.get(locals.idx));
                output.count++;
            }
        }
        if (output.count == 0)
        {
            locals.logger = QUTILLogger{ 0, 0, qpi.invocator(), SELF, 0, QUTILLogTypeNoPollsByCreator };
            LOG_INFO(locals.logger);
        }
    }

    /**
    * Get the current poll ID
    * @return the current value of current_poll_id
    */
    PUBLIC_FUNCTION_WITH_LOCALS(GetCurrentPollId)
    {
        output.current_poll_id = state.current_poll_id;
        output.active_count = 0;
        for (locals.i = 0; locals.i < QUTIL_MAX_POLL; locals.i++)
        {
            if (state.polls.get(locals.i).is_active != 0)
            {
                output.active_poll_ids.set(output.active_count, state.poll_ids.get(locals.i));
                output.active_count++;
            }
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetPollInfo)
    {
        output.found = 0;
        locals.idx = mod(input.poll_id, QUTIL_MAX_POLL);

        // Sanity index check the poll if index matches the requested ID
        if (state.poll_ids.get(locals.idx) == input.poll_id)
        {
            output.poll_info = state.polls.get(locals.idx);
            output.poll_link = state.poll_links.get(locals.idx);
            output.found = 1;
        }
    }

    /**
    * End of epoch processing for polls, sets active polls to inactive
    */
    END_EPOCH_WITH_LOCALS()
    {
        for (locals.i = 0; locals.i < QUTIL_MAX_POLL; locals.i++)
        {
            locals.current_poll = state.polls.get(locals.i);
            if (locals.current_poll.is_active != 0)
            {
                // Deactivate the poll
                locals.current_poll.is_active = 0;
                state.polls.set(locals.i, locals.current_poll);
            }
        }

        state.new_polls_this_epoch = 0;
    }

    BEGIN_EPOCH()
    {
        state.dfMiningSeed = qpi.getPrevSpectrumDigest();
    }

    // Deactivate delay function
    #if 0
    struct BEGIN_TICK_locals
    {
        m256i dfPubkey, dfNonce;
        QUTILDFLogger logger;
    };
    /*
    * A deterministic delay function
    */
    BEGIN_TICK_WITH_LOCALS()
    {
        locals.dfPubkey = qpi.getPrevSpectrumDigest();
        locals.dfNonce = qpi.getPrevComputerDigest();
        state.dfCurrentState = qpi.computeMiningFunction(state.dfMiningSeed, locals.dfPubkey, locals.dfNonce);

        locals.logger = QUTILDFLogger{ 0, 0, locals.dfNonce, locals.dfPubkey, state.dfMiningSeed, state.dfCurrentState};
        LOG_INFO(locals.logger);
    }
    #endif

    /*
    * @return Return total number of shares that currently exist of the asset given as input
    */
    PUBLIC_FUNCTION(GetTotalNumberOfAssetShares)
    {
        output = qpi.numberOfShares(input);
    }

    struct DistributeQuToShareholders_input
    {
        Asset asset;
    };
    struct DistributeQuToShareholders_output
    {
        sint64 shareholders;
        sint64 totalShares;
        sint64 amountPerShare;
        sint64 fees;
    };
    struct DistributeQuToShareholders_locals
    {
        AssetPossessionIterator iter;
        sint64 payBack;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(DistributeQuToShareholders)
    {
        // 1. Compute fee (increases linear with number of shareholders)
        // 1.1. Count shareholders and shares
        for (locals.iter.begin(input.asset); !locals.iter.reachedEnd(); locals.iter.next())
        {
            if (locals.iter.numberOfPossessedShares() > 0)
            {
                ++output.shareholders;
                output.totalShares += locals.iter.numberOfPossessedShares();
            }
        }

        // 1.2. Cancel if there are no shareholders
        if (output.shareholders == 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // 1.3. Compute fee (proportional to number of shareholders)
        output.fees = output.shareholders * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;

        // 1.4. Compute QU per share
        output.amountPerShare = div<sint64>(qpi.invocationReward() - output.fees, output.totalShares);

        // 1.5. Cancel if amount is not sufficient to pay fees and at least one QU per share
        if (output.amountPerShare <= 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // 1.6. compute payback QU (remainder of distribution)
        locals.payBack = qpi.invocationReward() - output.totalShares * output.amountPerShare - output.fees;
        ASSERT(locals.payBack >= 0);

        // 2. Distribute to shareholders
        for (locals.iter.begin(input.asset); !locals.iter.reachedEnd(); locals.iter.next())
        {
            if (locals.iter.numberOfPossessedShares() > 0)
            {
                qpi.transfer(locals.iter.possessor(), locals.iter.numberOfPossessedShares() * output.amountPerShare);
            }
        }

        // 3. Burn fee
        qpi.burn(output.fees);

        // 4. pay back QU that cannot be evenly distributed
        if (locals.payBack > 0)
        {
            qpi.transfer(qpi.invocator(), locals.payBack);
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetSendToManyV1Fee, 1);
        REGISTER_USER_FUNCTION(GetTotalNumberOfAssetShares, 2);
        REGISTER_USER_FUNCTION(GetCurrentResult, 3);
        REGISTER_USER_FUNCTION(GetPollsByCreator, 4);
        REGISTER_USER_FUNCTION(GetCurrentPollId, 5);
        REGISTER_USER_FUNCTION(GetPollInfo, 6);

        REGISTER_USER_PROCEDURE(SendToManyV1, 1);
        REGISTER_USER_PROCEDURE(BurnQubic, 2);
        REGISTER_USER_PROCEDURE(SendToManyBenchmark, 3);
        REGISTER_USER_PROCEDURE(CreatePoll, 4);
        REGISTER_USER_PROCEDURE(Vote, 5);
        REGISTER_USER_PROCEDURE(CancelPoll, 6);
        REGISTER_USER_PROCEDURE(DistributeQuToShareholders, 7);
    }
};
