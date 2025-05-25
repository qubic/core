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
constexpr uint16 QUTIL_POLL_INIT_EPOCH = 163; // Epoch to initialize poll-related state


// Voting log types enum
enum QUtilVotingLogInfo {
    PollCreated = 5,                // Poll created successfully
    InsufficientFundsForPoll = 6,   // Insufficient funds for poll creation
    InvalidPollType = 7,            // Invalid poll type
    InvalidNumAssetsQubic = 8,      // Invalid number of assets for Qubic poll
    InvalidNumAssetsAsset = 9,      // Invalid number of assets for Asset poll
    VoteCast = 10,                  // Vote cast successfully
    InsufficientFundsForVote = 11,  // Insufficient funds for voting
    InvalidPollId = 12,             // Invalid poll ID
    PollInactive = 13,              // Poll is inactive
    InsufficientBalance = 14,       // Insufficient voter balance
    InvalidOption = 15,             // Invalid voting option
    InvalidPollIdResult = 16,       // Invalid poll ID in GetCurrentResult
    PollInactiveResult = 17,        // Poll inactive in GetCurrentResult
    NoPollsByCreator = 18           // No polls found in GetPollsByCreator
};

struct QUtilLogger
{
    uint32 contractId; // to distinguish bw SCs
    uint32 padding;
    id src;
    id dst;
    sint64 amt;
    uint32 logtype;
    // Other data go here
    char _terminator; // Only data before "_terminator" are logged
};

// poll and voter structs
struct QUtilPoll {
    id poll_name;
    uint64 poll_type; // QUTIL_POLL_TYPE_QUBIC or QUTIL_POLL_TYPE_ASSET
    uint64 min_amount; // Minimum Qubic/asset amount for eligibility
    uint64 is_active;
    id creator; // Address that created the poll
    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets; // List of allowed assets for QUTIL_POLL_TYPE_ASSET
    uint64 num_assets; // Number of assets in allowed assets
};

struct QUtilVoter {
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
    QUtilLogger logger;

    // Voting state
    Array<QUtilPoll, QUTIL_MAX_POLL> polls;
    Array<QUtilVoter, QUTIL_TOTAL_VOTERS> voters; // 1d array for all voters
    Array<uint64, QUTIL_MAX_POLL> poll_ids;
    Array<uint64, QUTIL_MAX_POLL> poll_results; // stores dominant option per poll
    Array<uint64, QUTIL_MAX_POLL> voter_counts; // tracks number of voters per poll
    Array<Array<uint8, 256>, QUTIL_MAX_POLL> poll_links; // github links for polls
    uint64 current_poll_id;

    // Custom Modulo Function
    struct custom_mod_input {
        uint64 a;
        uint64 b;
    };
    struct custom_mod_output {
        uint64 result;
    };
    struct custom_mod_locals {
        uint64 quotient;
    };

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

    // Get QUtilVoter Balance Helper
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

    // Swap QUtilVoter to the end of the array helper
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
        QUtilVoter temp_voter;
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
        bit useNext;
    };

    struct BurnQubic_input
    {
        sint64 amount;
    };
    struct BurnQubic_output
    {
        sint64 amount;
    };

    struct CreatePoll_input
    {
        id poll_name;
        uint64 poll_type; // QUTIL_POLL_TYPE_QUBIC or QUTIL_POLL_TYPE_ASSET
        uint64 min_amount; // Minimum Qubic/asset amount
        Array<uint8, 256> github_link; // GitHub link
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
        custom_mod_input cm_input;
        custom_mod_output cm_output;
        custom_mod_locals cm_locals;
        QUtilPoll new_poll;
        QUtilVoter default_voter;
        uint64 i;
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
        custom_mod_input cm_input;
        custom_mod_output cm_output;
        custom_mod_locals cm_locals;
        get_voter_balance_input gvb_input;
        get_voter_balance_output gvb_output;
        get_voter_balance_locals gvb_locals;
        swap_voter_to_end_input sve_input;
        swap_voter_to_end_output sve_output;
        swap_voter_to_end_locals sve_locals;
        uint64 i;
        uint64 voter_index;
        QUtilVoter temp_voter;
        uint64 real_vote;
        uint64 end_idx;
        uint64 max_balance;
    };

    struct GetCurrentResult_input
    {
        uint64 poll_id;
    };
    struct GetCurrentResult_output
    {
        Array<uint64, QUTIL_MAX_OPTIONS> result;  // Total voting power for each option
        Array<uint64, QUTIL_MAX_OPTIONS> voter_count;  // Number of voters for each option
    };
    struct GetCurrentResult_locals
    {
        uint64 idx;
        uint64 poll_type;
        uint64 effective_amount;
        QUtilVoter voter;
        custom_mod_input cm_input;
        custom_mod_output cm_output;
        custom_mod_locals cm_locals;
        uint64 i;
        uint64 voter_index;
        QUtilLogger logger;
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
        QUtilLogger logger;
    };

    struct END_EPOCH_locals
    {
        uint64 i;
        uint64 d;
        GetCurrentResult_input gcr_input;
        GetCurrentResult_output gcr_output;
        GetCurrentResult_locals gcr_locals;
        QUtilPoll current_poll;
    };

    struct BEGIN_EPOCH_locals
    {
        uint64 i;
        uint64 j;
        QUtilPoll default_poll;
        QUtilVoter default_voter;
        Array<uint8, 256> zero_link;
    };

    /**************************************/
    /***********HELPER FUNCTIONS***********/
    /**************************************/

    PRIVATE_FUNCTION_WITH_LOCALS(custom_mod)
    {
        locals.quotient = div(input.a, input.b);
        output.result = input.a - input.b * locals.quotient;
    }

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

    // Calculate QUtilVoter Index
    inline static uint64 calculate_voter_index(uint64 poll_idx, uint64 voter_idx)
    {
        return poll_idx * QUTIL_MAX_VOTERS_PER_POLL + voter_idx;
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
    PUBLIC_PROCEDURE(SendToManyV1)
    {
        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_TRIGGERED };
        LOG_INFO(state.logger);
        state.total = input.amt0 + input.amt1 + input.amt2 + input.amt3 + input.amt4 + input.amt5 + input.amt6 + input.amt7 + input.amt8 + input.amt9 + input.amt10 + input.amt11 + input.amt12 + input.amt13 + input.amt14 + input.amt15 + input.amt16 + input.amt17 + input.amt18 + input.amt19 + input.amt20 + input.amt21 + input.amt22 + input.amt23 + input.amt24 + QUTIL_STM1_INVOCATION_FEE;
        // invalid amount (<0), return fund and exit
        if ((input.amt0 < 0) || (input.amt1 < 0) || (input.amt2 < 0) || (input.amt3 < 0) || (input.amt4 < 0) || (input.amt5 < 0) || (input.amt6 < 0) || (input.amt7 < 0) || (input.amt8 < 0) || (input.amt9 < 0) || (input.amt10 < 0) || (input.amt11 < 0) || (input.amt12 < 0) || (input.amt13 < 0) || (input.amt14 < 0) || (input.amt15 < 0) || (input.amt16 < 0) || (input.amt17 < 0) || (input.amt18 < 0) || (input.amt19 < 0) || (input.amt20 < 0) || (input.amt21 < 0) || (input.amt22 < 0) || (input.amt23 < 0) || (input.amt24 < 0))
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_INVALID_AMOUNT_NUMBER };
            output.returnCode = QUTIL_STM1_INVALID_AMOUNT_NUMBER;
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
        }
        // insufficient or too many qubic transferred, return fund and exit (we don't want to return change)
        if (qpi.invocationReward() != state.total)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_WRONG_FUND };
            LOG_INFO(state.logger);
            output.returnCode = QUTIL_STM1_WRONG_FUND;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (input.dst0 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst0, input.amt0, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst0, input.amt0);
        }
        if (input.dst1 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst1, input.amt1, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst1, input.amt1);
        }
        if (input.dst2 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst2, input.amt2, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst2, input.amt2);
        }
        if (input.dst3 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst3, input.amt3, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst3, input.amt3);
        }
        if (input.dst4 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst4, input.amt4, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst4, input.amt4);
        }
        if (input.dst5 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst5, input.amt5, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst5, input.amt5);
        }
        if (input.dst6 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst6, input.amt6, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst6, input.amt6);
        }
        if (input.dst7 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst7, input.amt7, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst7, input.amt7);
        }
        if (input.dst8 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst8, input.amt8, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst8, input.amt8);
        }
        if (input.dst9 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst9, input.amt9, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst9, input.amt9);
        }
        if (input.dst10 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst10, input.amt10, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst10, input.amt10);
        }
        if (input.dst11 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst11, input.amt11, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst11, input.amt11);
        }
        if (input.dst12 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst12, input.amt12, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst12, input.amt12);
        }
        if (input.dst13 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst13, input.amt13, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst13, input.amt13);
        }
        if (input.dst14 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst14, input.amt14, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst14, input.amt14);
        }
        if (input.dst15 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst15, input.amt15, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst15, input.amt15);
        }
        if (input.dst16 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst16, input.amt16, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst16, input.amt16);
        }
        if (input.dst17 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst17, input.amt17, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst17, input.amt17);
        }
        if (input.dst18 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst18, input.amt18, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst18, input.amt18);
        }
        if (input.dst19 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst19, input.amt19, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst19, input.amt19);
        }
        if (input.dst20 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst20, input.amt20, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst20, input.amt20);
        }
        if (input.dst21 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst21, input.amt21, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst21, input.amt21);
        }
        if (input.dst22 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst22, input.amt22, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst22, input.amt22);
        }
        if (input.dst23 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst23, input.amt23, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst23, input.amt23);
        }
        if (input.dst24 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst24, input.amt24, QUTIL_STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst24, input.amt24);
        }
        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, state.total, QUTIL_STM1_SUCCESS };
        LOG_INFO(state.logger);
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
        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_TRIGGERED };
        LOG_INFO(state.logger);
        output.total = 0;

        // Number of addresses and transfers is > 0 and total transfers do not exceed limit (including 2 transfers from invocator to contract and contract to invocator)
        if (input.dstCount <= 0 || input.numTransfersEach <= 0 || input.dstCount * input.numTransfersEach + 2 > CONTRACT_ACTION_TRACKER_SIZE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_INVALID_AMOUNT_NUMBER };
            LOG_INFO(state.logger);
            output.returnCode = QUTIL_STM1_INVALID_AMOUNT_NUMBER;
            return;
        }

        // Check the fund is enough
        if (qpi.invocationReward() < input.dstCount * input.numTransfersEach)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), QUTIL_STM1_INVALID_AMOUNT_NUMBER };
            LOG_INFO(state.logger);
            output.returnCode = QUTIL_STM1_INVALID_AMOUNT_NUMBER;
            return;
        }

        // Loop through the number of addresses and do the transfers
        locals.currentId = qpi.invocator();
        locals.useNext = true;
        while (output.dstCount < input.dstCount)
        {
            if (locals.useNext)
                locals.currentId = qpi.nextId(locals.currentId);
            else
                locals.currentId = qpi.prevId(locals.currentId);
            if (locals.currentId == m256i::zero())
            {
                locals.currentId = qpi.invocator();
                locals.useNext = !locals.useNext;
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

        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, output.total, QUTIL_STM1_SUCCESS };
        LOG_INFO(state.logger);
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
        if (qpi.invocationReward() < QUTIL_POLL_CREATION_FEE)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, qpi.invocationReward(), InsufficientFundsForPoll };
            LOG_INFO(state.logger);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (input.poll_type != QUTIL_POLL_TYPE_QUBIC && input.poll_type != QUTIL_POLL_TYPE_ASSET)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InvalidPollType };
            LOG_INFO(state.logger);
            return;
        }

        if (input.poll_type == QUTIL_POLL_TYPE_QUBIC && input.num_assets != 0)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InvalidNumAssetsQubic };
            LOG_INFO(state.logger);
            return;
        }

        if (input.poll_type == QUTIL_POLL_TYPE_ASSET && (input.num_assets == 0 || input.num_assets > QUTIL_MAX_ASSETS_PER_POLL))
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InvalidNumAssetsAsset };
            LOG_INFO(state.logger);
            return;
        }

        locals.cm_input.a = state.current_poll_id;
        locals.cm_input.b = QUTIL_MAX_POLL;
        custom_mod(qpi, state, locals.cm_input, locals.cm_output, locals.cm_locals);

        locals.idx = locals.cm_output.result;
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

        state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, QUTIL_POLL_CREATION_FEE, PollCreated };
        LOG_INFO(state.logger);
    }

    /**
    * Cast a vote in a poll, charging 100 QUs, limiting options to 0-63
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(Vote)
    {
        output.success = false;
        if (qpi.invocationReward() < QUTIL_VOTE_FEE)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, qpi.invocationReward(), InsufficientFundsForVote };
            LOG_INFO(state.logger);
            return;
        }
        qpi.transfer(qpi.invocator(), qpi.invocationReward() - QUTIL_VOTE_FEE);
        qpi.burn(QUTIL_VOTE_FEE);

        locals.cm_input.a = input.poll_id;
        locals.cm_input.b = QUTIL_MAX_POLL;
        custom_mod(qpi, state, locals.cm_input, locals.cm_output, locals.cm_locals);
        locals.idx = locals.cm_output.result;

        if (state.poll_ids.get(locals.idx) != input.poll_id)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InvalidPollId };
            LOG_INFO(state.logger);
            return;
        }
        if (state.polls.get(locals.idx).is_active == 0)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, PollInactive };
            LOG_INFO(state.logger);
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
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InsufficientBalance };
            LOG_INFO(state.logger);
            return;
        }
        if (input.chosen_option >= QUTIL_MAX_OPTIONS)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InvalidOption };
            LOG_INFO(state.logger);
            return;
        }

        // Search for existing voter or empty slot
        for (locals.i = 0; locals.i < QUTIL_MAX_VOTERS_PER_POLL; locals.i++)
        {
            locals.voter_index = calculate_voter_index(locals.idx, locals.i);
            if (state.voters.get(locals.voter_index).address == input.address)
            {
                // Update existing voter
                state.voters.set(locals.voter_index, QUtilVoter{ input.address, input.amount, input.chosen_option });
                output.success = true;
                break;
            }
            else if (state.voters.get(locals.voter_index).address == NULL_ID)
            {
                // Add new voter in empty slot
                state.voters.set(locals.voter_index, QUtilVoter{ input.address, input.amount, input.chosen_option });
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

        for (locals.i = 0; locals.i <= locals.end_idx; locals.i++)
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
                    locals.temp_voter = state.voters.get(locals.voter_index);
                }
                else
                {
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
                    state.voters.set(locals.voter_index, QUtilVoter{ NULL_ID, 0, 0 });
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
                    }
                }
                else
                {
                    locals.temp_voter.amount = locals.max_balance < locals.temp_voter.amount ? locals.max_balance : locals.temp_voter.amount;
                    state.voters.set(locals.voter_index, locals.temp_voter);
                    locals.real_vote++;
                }
            }
        }

        // Update voter count
        state.voter_counts.set(locals.idx, locals.real_vote);

        if (output.success)
        {
            state.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, QUTIL_VOTE_FEE, VoteCast };
            LOG_INFO(state.logger);
        }
    }

    /**
    * Get the current voting results for a poll
    */
    PUBLIC_FUNCTION_WITH_LOCALS(GetCurrentResult)
    {
        locals.cm_input.a = input.poll_id;
        locals.cm_input.b = QUTIL_MAX_POLL;
        custom_mod(qpi, state, locals.cm_input, locals.cm_output, locals.cm_locals);
        locals.idx = locals.cm_output.result;

        if (state.poll_ids.get(locals.idx) != input.poll_id)
        {
            locals.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, InvalidPollIdResult };
            LOG_INFO(locals.logger);
            return;
        }
        if (state.polls.get(locals.idx).is_active == 0)
        {
            locals.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, PollInactiveResult };
            LOG_INFO(locals.logger);
            return;
        }

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
            locals.logger = QUtilLogger{ 0, 0, qpi.invocator(), SELF, 0, NoPollsByCreator };
            LOG_INFO(locals.logger);
        }
    }

    /**
    * End of epoch processing for polls, computes dominant option for options 0-63
    */
    END_EPOCH_WITH_LOCALS()
    {
        for (locals.i = 0; locals.i < QUTIL_MAX_POLL; locals.i++)
        {
            if (state.polls.get(locals.i).is_active != 0)
            {
                locals.gcr_input.poll_id = state.poll_ids.get(locals.i);
                GetCurrentResult(qpi, state, locals.gcr_input, locals.gcr_output, locals.gcr_locals);
                uint64 max_votes = 0;
                uint64 dominant_decision = 0;
                for (locals.d = 0; locals.d < QUTIL_MAX_OPTIONS; locals.d++)
                {
                    if (locals.gcr_output.result.get(locals.d) > max_votes)
                    {
                        max_votes = locals.gcr_output.result.get(locals.d);
                        dominant_decision = locals.d;
                    }
                }
                state.poll_results.set(locals.i, dominant_decision);

                // Deactivate the poll
                locals.current_poll = state.polls.get(locals.i);
                locals.current_poll.is_active = 0;
                state.polls.set(locals.i, locals.current_poll);
            }
        }
    }

    BEGIN_EPOCH_WITH_LOCALS()
    {
        if (qpi.epoch() == QUTIL_POLL_INIT_EPOCH)
        {
            state.current_poll_id = 0;

            // Set default values for default_poll
            locals.default_poll.is_active = 0;
            locals.default_poll.poll_name = NULL_ID;
            locals.default_poll.poll_type = 0;
            locals.default_poll.min_amount = 0;
            locals.default_poll.creator = NULL_ID;
            locals.default_poll.num_assets = 0;

            // Initialize zero_link to all zeros for poll_links
            for (locals.j = 0; locals.j < 256; locals.j++)
            {
                locals.zero_link.set(locals.j, 0);
            }

            // Initialize all poll-related arrays (except voters, handled by CreatePoll)
            for (locals.i = 0; locals.i < QUTIL_MAX_POLL; locals.i++)
            {
                state.polls.set(locals.i, locals.default_poll);
                state.poll_ids.set(locals.i, 0);
                state.poll_results.set(locals.i, 0);
                state.voter_counts.set(locals.i, 0);
                state.poll_links.set(locals.i, locals.zero_link);
            }
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetSendToManyV1Fee, 1);
        REGISTER_USER_FUNCTION(GetCurrentResult, 2);
        REGISTER_USER_FUNCTION(GetPollsByCreator, 3);

        REGISTER_USER_PROCEDURE(SendToManyV1, 1);
        REGISTER_USER_PROCEDURE(BurnQubic, 2);
        REGISTER_USER_PROCEDURE(SendToManyBenchmark, 3);
        REGISTER_USER_PROCEDURE(CreatePoll, 4);
        REGISTER_USER_PROCEDURE(Vote, 5);
    }
};
