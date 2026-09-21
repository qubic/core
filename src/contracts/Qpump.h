using namespace QPI;

// Qpump: bonding-curve launchpad for meme coins.
//
// Coin stages:
//   OPENING      one-price opening batch
//   OPEN         linear curve, 1 QU to 10 QU
//   COMPLETE     curve sold out, graduation pending
//   DISTRIBUTING real asset issued, holders being delivered
//   REFUNDING    coin expired, holders refunded pro rata
//
// A graduated coin keeps its record and name forever.
// A refunded coin is removed and frees its name.
// Balances are internal until graduation issues the real asset.
// No admin, no pause. Fees and limits are constants below.

// ---- Curve ----
constexpr sint64 QPUMP_CURVE_SUPPLY = 710000000LL;            // tokens sold on the curve
constexpr sint64 QPUMP_POOL_RESERVE = 290000000LL;            // upper bound on tokens seeded into Qswap
constexpr uint64 QPUMP_START_PRICE_MILLI = 1000ULL;           // 1 QU per token
constexpr uint64 QPUMP_END_PRICE_MILLI = 10000ULL;            // 10 QU per token
constexpr uint64 QPUMP_MILLI = 1000ULL;
// cost(x, y) integrates the linear price line
constexpr uint64 QPUMP_COST_LINEAR = 1420000000000ULL;        // 2 * supply * start price
constexpr uint64 QPUMP_COST_SLOPE = 9000ULL;                  // end price minus start price, in milli QU
constexpr uint64 QPUMP_COST_DENOMINATOR = 1420000000000ULL;   // 2 * supply * 1000
constexpr sint64 QPUMP_CURVE_RAISE = 3905000000LL;            // cost(0, supply)

// ---- Opening batch ----
constexpr uint32 QPUMP_OPENING_TICKS = 1200;             // about 5 minutes at 4 ticks per second
constexpr sint64 QPUMP_OPENING_TOKEN_CAP = 177500000LL;       // a quarter of the curve
constexpr sint64 QPUMP_OPENING_QU_CAP = 377187500LL;          // cost(0, token cap), rounded up

// ---- QDOGE buyback ----
constexpr uint64 QPUMP_QDOGE_NAME = 297549120593ULL;              // QDOGE
constexpr sint64 QPUMP_QDOGE_MIN_BUY = 5000000LL;             // smallest epoch buy, keeps the flat fee small
constexpr uint64 QPUMP_QDOGE_POOL_DIVISOR = 50ULL;            // one buy takes at most 2 percent of reserves

// ---- Fees ----
constexpr sint64 QPUMP_LAUNCH_FEE = 25000000LL;
constexpr sint64 QPUMP_LAUNCH_FEE_BURN = 5000000LL;           // the rest goes to shareholders
constexpr sint64 QPUMP_GRADUATION_REWARD = 50000000LL;        // paid to the creator from the raise
constexpr sint64 QPUMP_GRADUATION_BURN = 10000000LL;          // burned from the raise at graduation
constexpr sint64 QPUMP_TRANSFER_FEE = 100LL;                  // flat fee per internal transfer, burned
constexpr sint64 QPUMP_MIN_TRADE_FEE = 1000LL;
constexpr uint64 QPUMP_BPS = 10000ULL;
constexpr uint64 QPUMP_TRADE_FEE_BPS = 100ULL;                // 1 percent
constexpr uint64 QPUMP_FEE_BURN_SHARE_BPS = 1000ULL;          // 10 percent of the trade fee is burned
constexpr uint64 QPUMP_FEE_QDOGE_SHARE_BPS = 2000ULL;         // 20 percent buys QDOGE, locked for good
constexpr sint64 QPUMP_QSWAP_LIQUIDITY_FEE = 100000LL;        // flat Qswap fee on AddLiquidity
constexpr sint64 QPUMP_DEFAULT_QX_ISSUANCE_FEE = 1000000000LL;
constexpr sint64 QPUMP_DEFAULT_QX_TRANSFER_FEE = 100LL;
constexpr sint64 QPUMP_DEFAULT_QSWAP_POOL_FEE = 200000000LL;

// ---- Lifecycle ----
constexpr uint16 QPUMP_IDLE_EPOCHS = 9;                        // about two months idle, then refunded; no age limit
constexpr uint16 QPUMP_GRADUATION_STALL_EPOCHS = 2;
constexpr uint32 QPUMP_GRADUATION_RETRY_TICKS = 100;
constexpr uint32 QPUMP_MANUAL_RETRY_TICKS = 5;
constexpr uint8 QPUMP_MAX_DELIVERY_FAILURES = 3;
constexpr sint64 QPUMP_DELIVERY_BUDGET_FACTOR = 2;               // covers every holder twice

// ---- Capacities ----
// Sized up front, far under the 1 GB state limit.
constexpr uint64 QPUMP_MAX_COINS = 32768;                     // coins not yet closed
constexpr uint64 QPUMP_ARCHIVE_CAPACITY = 65536;              // permanent records of graduated coins
constexpr uint64 QPUMP_NAME_CAPACITY = 262144;                // live and graduated names
constexpr uint64 QPUMP_CREATOR_CAPACITY = 65536;
constexpr uint64 QPUMP_CREATOR_LOAD_LIMIT = 52428;
constexpr uint64 QPUMP_HOLDER_CAPACITY = 4194304;
constexpr uint64 QPUMP_HOLDER_LOAD_LIMIT = 3355443;           // 80 percent of capacity
constexpr uint64 QPUMP_WALLET_CAPACITY = 1048576;
constexpr uint64 QPUMP_WALLET_LOAD_LIMIT = 838860;            // 80 percent of capacity
constexpr uint32 QPUMP_ARCHIVE_FLAG = 2147483648U;            // name map value marks a graduated record
constexpr uint64 QPUMP_COIN_KEY_MASK = 4294967295ULL;         // holder key: wallet id high, coin id low
constexpr uint32 QPUMP_MAX_LIVE_COINS_PER_CREATOR = 10;   // live at once, not a lifetime cap
constexpr uint32 QPUMP_NO_SLOT = 4294967295U;

// ---- Processing budgets ----
constexpr uint32 QPUMP_TICK_SLOT_SCAN = 256;
constexpr uint32 QPUMP_TICK_COIN_WORK = 4;
constexpr uint32 QPUMP_TICK_PAYOUTS = 16;
constexpr uint32 QPUMP_TICK_SCAN_STEPS = 64;                  // holders visited per tick, failures included
constexpr uint32 QPUMP_PROCESS_MAX_PAYOUTS = 128;
constexpr uint32 QPUMP_PROCESS_SCAN_STEPS = 512;
constexpr uint32 QPUMP_LIST_PAGE = 32;

// ---- Coin status ----
constexpr uint8 QPUMP_STATUS_EMPTY = 0;
constexpr uint8 QPUMP_STATUS_OPENING = 1;
constexpr uint8 QPUMP_STATUS_OPEN = 2;
constexpr uint8 QPUMP_STATUS_COMPLETE = 3;
constexpr uint8 QPUMP_STATUS_DISTRIBUTING = 4;
constexpr uint8 QPUMP_STATUS_REFUNDING = 5;

// ---- Return codes, append only ----
constexpr uint32 QPUMP_OK = 0;
constexpr uint32 QPUMP_ERR_INVALID_INPUT = 1;
constexpr uint32 QPUMP_ERR_INVALID_NAME = 2;
constexpr uint32 QPUMP_ERR_NAME_TAKEN = 3;
constexpr uint32 QPUMP_ERR_CAPACITY = 4;
constexpr uint32 QPUMP_ERR_CREATOR_LIMIT = 5;
constexpr uint32 QPUMP_ERR_INSUFFICIENT_REWARD = 6;
constexpr uint32 QPUMP_ERR_COIN_NOT_FOUND = 7;
constexpr uint32 QPUMP_ERR_WRONG_STAGE = 8;
constexpr uint32 QPUMP_ERR_SLIPPAGE = 9;
constexpr uint32 QPUMP_ERR_INSUFFICIENT_TOKENS = 10;
constexpr uint32 QPUMP_ERR_HOLDER_LIMIT = 11;
constexpr uint32 QPUMP_ERR_USERS_ONLY = 12;
constexpr uint32 QPUMP_ERR_BATCH_FULL = 13;
constexpr uint32 QPUMP_ERR_HOLDER_NOT_FOUND = 14;
constexpr uint32 QPUMP_ERR_EXTERNAL_CALL = 15;
constexpr uint32 QPUMP_ERR_TRANSFER_FAILED = 16;
constexpr uint32 QPUMP_ERR_GRADUATION_BUDGET = 17;
constexpr uint32 QPUMP_ERR_RETRY_LATER = 18;
constexpr uint32 QPUMP_ERR_POOL_FALLBACK = 19;
constexpr uint32 QPUMP_ERR_WALLET_LIMIT = 20;

// ---- Log types ----
constexpr uint32 QPUMP_LOG_CREATED = 1;
constexpr uint32 QPUMP_LOG_OPENING_ORDER = 2;
constexpr uint32 QPUMP_LOG_BATCH_SETTLED = 3;
constexpr uint32 QPUMP_LOG_BUY = 4;
constexpr uint32 QPUMP_LOG_SELL = 5;
constexpr uint32 QPUMP_LOG_COMPLETE = 6;
constexpr uint32 QPUMP_LOG_GRADUATED = 7;
constexpr uint32 QPUMP_LOG_DELIVERED = 8;
constexpr uint32 QPUMP_LOG_REFUND_STARTED = 9;
constexpr uint32 QPUMP_LOG_REFUNDED = 10;
constexpr uint32 QPUMP_LOG_CLOSED = 11;
constexpr uint32 QPUMP_LOG_GRADUATION_FAILED = 12;
constexpr uint32 QPUMP_LOG_REVERTED = 13;
constexpr uint32 QPUMP_LOG_TRANSFER = 14;
constexpr uint32 QPUMP_LOG_QDOGE_BUYBACK = 15;

static_assert(QPUMP_CURVE_SUPPLY + QPUMP_POOL_RESERVE == 1000000000LL);
static_assert(QPUMP_COST_LINEAR == 2ULL * 710000000ULL * QPUMP_START_PRICE_MILLI);
static_assert(QPUMP_COST_SLOPE == QPUMP_END_PRICE_MILLI - QPUMP_START_PRICE_MILLI);
static_assert(QPUMP_COST_DENOMINATOR == 2ULL * 710000000ULL * QPUMP_MILLI);
static_assert(QPUMP_OPENING_TOKEN_CAP * 4 == QPUMP_CURVE_SUPPLY);
static_assert(QPUMP_FEE_BURN_SHARE_BPS + QPUMP_FEE_QDOGE_SHARE_BPS <= QPUMP_BPS);
static_assert(QPUMP_LAUNCH_FEE_BURN <= QPUMP_LAUNCH_FEE);
static_assert((QPUMP_MAX_COINS & (QPUMP_MAX_COINS - 1)) == 0);
static_assert((QPUMP_NAME_CAPACITY & (QPUMP_NAME_CAPACITY - 1)) == 0);
static_assert((QPUMP_CREATOR_CAPACITY & (QPUMP_CREATOR_CAPACITY - 1)) == 0);
static_assert((QPUMP_HOLDER_CAPACITY & (QPUMP_HOLDER_CAPACITY - 1)) == 0);
static_assert(QPUMP_MAX_COINS <= QPUMP_NAME_CAPACITY);
static_assert(QPUMP_HOLDER_LOAD_LIMIT < QPUMP_HOLDER_CAPACITY);
static_assert(QPUMP_CREATOR_LOAD_LIMIT < QPUMP_CREATOR_CAPACITY);
static_assert((QPUMP_WALLET_CAPACITY & (QPUMP_WALLET_CAPACITY - 1)) == 0);
static_assert(QPUMP_WALLET_LOAD_LIMIT < QPUMP_WALLET_CAPACITY);
static_assert(QPUMP_CURVE_SUPPLY <= 4294967295LL);    // balances fit in uint32
static_assert(QPUMP_OPENING_QU_CAP <= 4294967295LL);
static_assert(QPUMP_MAX_COINS <= 65535);               // positions per wallet fit in uint16
static_assert((QPUMP_ARCHIVE_CAPACITY & (QPUMP_ARCHIVE_CAPACITY - 1)) == 0);
static_assert(QPUMP_MAX_COINS + QPUMP_ARCHIVE_CAPACITY <= QPUMP_NAME_CAPACITY);
static_assert(QPUMP_MAX_COINS < QPUMP_ARCHIVE_FLAG && QPUMP_ARCHIVE_CAPACITY < QPUMP_ARCHIVE_FLAG);

struct QPUMP2
{
};

struct QPUMP : public ContractBase
{
    // Keyed by wallet id and coin id. Each coin
    // links its holders, so payouts skip other coins.
    struct HolderEntry
    {
        uint32 tokens;
        uint32 openingQu;    // net QU in the batch until it settles
        uint64 nextKey;
        uint64 prevKey;
    };

    struct Coin
    {
        id creator;
        id metaDigest;       // sha2-256 of the off-chain metadata
        uint64 coinId;
        uint64 name;
        sint64 realQu;       // QU backing the curve, later the delivery budget
        sint64 sold;
        sint64 batchQu;
        sint64 batchTokens;
        sint64 poolQu;
        sint64 poolTokens;
        sint64 snapQu;       // refund snapshot
        sint64 snapSold;
        uint64 scanCursor;   // payout resume key, 0 starts at the head
        uint32 createdTick;
        uint32 holders;
        uint32 lastAttemptTick;
        uint32 gradHolders;  // holders when delivery started
        uint16 createdEpoch;
        uint16 lastTradeEpoch;
        uint16 completeEpoch;
        uint8 status;
        uint8 gradStep;
        uint8 batchSettled;
        uint8 failures;
    };

    // Permanent record of a graduated coin.
    struct GraduatedCoin
    {
        id creator;
        id metaDigest;
        uint64 coinId;
        uint64 name;
        sint64 sold;
        sint64 poolQu;       // QU seeded into the pool, 0 on fallback
        sint64 poolTokens;
        uint32 createdTick;
        uint32 holders;      // holders when delivery started
        uint16 createdEpoch;
        uint16 completeEpoch;
        uint16 closedEpoch;  // every holder delivered
        uint8 poolFallback;  // 1 when pool QU went back to holders
    };

    struct CoinSummary
    {
        uint64 name;
        uint64 coinId;
        sint64 realQu;
        sint64 sold;
        sint64 batchQu;
        uint32 holders;
        uint32 createdTick;
        uint8 status;
    };

    struct QpumpLog
    {
        uint32 _contractIndex;
        uint32 _type;
        uint64 name;
        uint64 coinId;
        id actor;
        sint64 qu;
        sint64 tokens;
        sint64 realQu;
        sint64 sold;
        uint32 code;
        uint32 holders;      // holders after the event
        sint64 fee;          // trade fee charged
        id counterparty;     // CREATED metadata digest, TRANSFER sender
        sint8 _terminator;
    };

    struct StateData
    {
        Array<Coin, QPUMP_MAX_COINS> coins;
        Array<uint64, QPUMP_MAX_COINS> holderHeads;
        Array<uint32, QPUMP_MAX_COINS> freeSlots;
        Array<GraduatedCoin, QPUMP_ARCHIVE_CAPACITY> graduated;
        HashMap<uint64, uint32, QPUMP_NAME_CAPACITY> nameToSlot;
        HashMap<id, uint32, QPUMP_CREATOR_CAPACITY> creatorLive;
        HashMap<uint64, HolderEntry, QPUMP_HOLDER_CAPACITY> holders;
        HashMap<id, uint32, QPUMP_WALLET_CAPACITY> walletIds;
        Array<id, QPUMP_WALLET_CAPACITY> walletAddresses;
        Array<uint16, QPUMP_WALLET_CAPACITY> walletPositions;
        Array<uint32, QPUMP_WALLET_CAPACITY> freeWalletIds;
        uint32 freeWalletCount;
        uint32 nextWalletId;
        uint64 nextCoinId;
        uint32 liveCoins;
        uint32 freeSlotCount;
        uint32 nextFreshSlot;
        uint32 sweepCursor;
        uint32 graduatedCount;
        uint32 activePayoutSlot;
        sint64 shareholderPot;
        sint64 burnPot;
        sint64 qdogePot;
        sint64 totalQuToQdoge;
        sint64 totalQdogeBought;
        sint64 cachedQxIssuanceFee;
        sint64 cachedQxTransferFee;
        sint64 cachedQswapPoolFee;
        uint64 totalLaunched;
        uint64 totalGraduated;
        uint64 totalRefunded;
        sint64 totalVolume;
        sint64 totalDividends;
        sint64 totalBurned;
    };

    // ================= Private function and procedure I/O =================

    struct CurveCost_input
    {
        sint64 from;
        sint64 to;
        bit roundUp;
    };
    struct CurveCost_output
    {
        sint64 qu;
    };
    struct CurveCost_locals
    {
        uint128 linear;
        uint128 slope;
        uint128 numerator;
        uint128 quotient;
    };

    struct TradeFee_input
    {
        sint64 amount;
        bit inclusive;   // 1 when the amount contains the fee
    };
    struct TradeFee_output
    {
        sint64 fee;
    };
    struct TradeFee_locals
    {
        uint64 denominator;
        uint64 numerator;
        uint64 quotient;
    };

    struct TokensForQu_input
    {
        sint64 from;
        sint64 budget;
        sint64 limit;
    };
    struct TokensForQu_output
    {
        sint64 tokens;
        sint64 cost;
    };
    struct TokensForQu_locals
    {
        sint64 lo;
        sint64 hi;
        sint64 mid;
        CurveCost_input costIn;
        CurveCost_output costOut;
    };

    struct TokensForTotal_input
    {
        sint64 from;
        sint64 budget;
        sint64 limit;
    };
    struct TokensForTotal_output
    {
        sint64 tokens;
        sint64 cost;
        sint64 fee;
    };
    struct TokensForTotal_locals
    {
        sint64 lo;
        sint64 hi;
        sint64 mid;
        CurveCost_input costIn;
        CurveCost_output costOut;
        TradeFee_input feeIn;
        TradeFee_output feeOut;
    };

    struct IsValidName_input
    {
        uint64 name;
    };
    struct IsValidName_output
    {
        bit valid;
    };
    struct IsValidName_locals
    {
        uint64 i;
        uint64 ch;
        bit ended;
    };

    struct HolderKeyOf_input
    {
        uint64 coinId;
        uint32 walletId;
    };
    struct HolderKeyOf_output
    {
        uint64 key;
    };

    struct FindWallet_input
    {
        id wallet;
    };
    struct FindWallet_output
    {
        uint32 walletId;
        bit found;
    };

    struct AcquireWallet_input
    {
        id wallet;
    };
    struct AcquireWallet_output
    {
        uint32 walletId;
        bit ok;
    };
    struct AcquireWallet_locals
    {
        uint32 walletId;
    };

    struct ChangePositions_input
    {
        uint32 walletId;
        sint32 delta;       // plus one, minus one, or zero
    };
    struct ChangePositions_output
    {
    };
    struct ChangePositions_locals
    {
        uint16 positions;
        id wallet;
    };

    struct EffectiveTokens_input
    {
        Coin coin;
        HolderEntry entry;
    };
    struct EffectiveTokens_output
    {
        sint64 tokens;
    };
    struct EffectiveTokens_locals
    {
        uint128 share;
    };

    struct FindCoin_input
    {
        uint64 name;
    };
    struct FindCoin_output
    {
        Coin coin;
        uint32 slot;
        uint32 archiveIndex;
        bit found;           // a live coin
        bit graduated;       // a permanent graduated record
    };
    struct FindCoin_locals
    {
        uint32 slot;
    };

    struct PriceAt_input
    {
        sint64 sold;
    };
    struct PriceAt_output
    {
        sint64 priceMilli;
    };

    struct SplitFee_input
    {
        sint64 fee;
    };
    struct SplitFee_output
    {
    };
    struct SplitFee_locals
    {
        sint64 burn;
        sint64 qdoge;
    };

    struct LinkHolder_input
    {
        uint32 slot;
        uint64 key;
    };
    struct LinkHolder_output
    {
    };
    struct LinkHolder_locals
    {
        HolderEntry entry;
        HolderEntry other;
        uint64 head;
    };

    struct UnlinkHolder_input
    {
        uint32 slot;
        uint64 key;
    };
    struct UnlinkHolder_output
    {
    };
    struct UnlinkHolder_locals
    {
        HolderEntry entry;
        HolderEntry other;
    };

    struct PlaceOpeningOrder_input
    {
        id buyer;
        sint64 amount;
        uint32 slot;
    };
    struct PlaceOpeningOrder_output
    {
        sint64 used;
        sint64 net;
        uint32 returnCode;
    };
    struct PlaceOpeningOrder_locals
    {
        LinkHolder_input linkIn;
        LinkHolder_output linkOut;
        Coin coin;
        uint64 key;
        HolderEntry entry;
        FindWallet_input walletIn;
        FindWallet_output walletOut;
        HolderKeyOf_input keyIn;
        HolderKeyOf_output keyOut;
        AcquireWallet_input acquireIn;
        AcquireWallet_output acquireOut;
        ChangePositions_input positionsIn;
        ChangePositions_output positionsOut;
        sint64 available;
        sint64 net;
        sint64 fee;
        sint64 room;
        sint64 used;
        TradeFee_input feeIn;
        TradeFee_output feeOut;
        SplitFee_input splitIn;
        SplitFee_output splitOut;
        QpumpLog log;
        bit found;
    };

    struct SettleOpening_input
    {
        uint32 slot;
    };
    struct SettleOpening_output
    {
        bit settled;
    };
    struct SettleOpening_locals
    {
        Coin coin;
        TokensForQu_input tokensIn;
        TokensForQu_output tokensOut;
        QpumpLog log;
    };

    struct ReleaseCreator_input
    {
        id creator;
    };
    struct ReleaseCreator_output
    {
    };
    struct ReleaseCreator_locals
    {
        uint32 count;
    };

    struct CloseCoin_input
    {
        uint32 slot;
    };
    struct CloseCoin_output
    {
    };
    struct CloseCoin_locals
    {
        Coin coin;
        Coin empty;
        GraduatedCoin record;
        QpumpLog log;
    };

    struct CheckExpiry_input
    {
        uint32 slot;
    };
    struct CheckExpiry_output
    {
        bit changed;
    };
    struct CheckExpiry_locals
    {
        Coin coin;
        ReleaseCreator_input releaseIn;
        ReleaseCreator_output releaseOut;
        CloseCoin_input closeIn;
        CloseCoin_output closeOut;
        QpumpLog log;
        sint64 deliverBudget;
        bit expired;
    };

    struct ContractBalance_input
    {
    };
    struct ContractBalance_output
    {
        sint64 balance;
    };
    struct ContractBalance_locals
    {
        Entity entity;
    };

    struct TryGraduate_input
    {
        uint32 slot;
    };
    struct TryGraduate_output
    {
        uint32 returnCode;
    };
    struct TryGraduate_locals
    {
        sint64 burnAmount;
        Coin coin;
        sint64 fixedCosts;
        sint64 deliverBudget;
        sint64 poolQu;
        sint64 poolTokens;
        sint64 available;
        sint64 balanceBefore;
        ContractBalance_input balanceIn;
        ContractBalance_output balanceOut;
        QX::IssueAsset_input issueIn;
        QX::IssueAsset_output issueOut;
        QX::TransferShareManagementRights_input rightsIn;
        QX::TransferShareManagementRights_output rightsOut;
        QSWAP::GetPoolBasicState_input poolStateIn;
        QSWAP::GetPoolBasicState_output poolStateOut;
        QSWAP::CreatePool_input createIn;
        QSWAP::CreatePool_output createOut;
        QSWAP::AddLiquidity_input addIn;
        QSWAP::AddLiquidity_output addOut;
        ReleaseCreator_input releaseIn;
        ReleaseCreator_output releaseOut;
        QpumpLog log;
    };

    struct PayHolder_input
    {
        uint64 key;
        sint64 callerFee;   // QU from a claimer, used if the budget is short
        uint32 slot;
    };
    struct PayHolder_output
    {
        sint64 tokens;
        sint64 qu;
        sint64 callerFeeUsed;
        uint32 returnCode;
    };
    struct PayHolder_locals
    {
        UnlinkHolder_input unlinkIn;
        UnlinkHolder_output unlinkOut;
        Coin coin;
        HolderEntry entry;
        id holder;
        ChangePositions_input positionsIn;
        ChangePositions_output positionsOut;
        EffectiveTokens_input effectiveIn;
        EffectiveTokens_output effectiveOut;
        sint64 tokens;
        sint64 fee;
        sint64 qu;
        uint128 share;
        QX::TransferShareOwnershipAndPossession_input transferIn;
        QX::TransferShareOwnershipAndPossession_output transferOut;
        CloseCoin_input closeIn;
        CloseCoin_output closeOut;
        QpumpLog log;
        sint64 balanceBefore;
        ContractBalance_input balanceIn;
        ContractBalance_output balanceOut;
        bit fromCaller;
    };

    struct ProcessPayouts_input
    {
        uint32 slot;
        uint32 maxPayouts;
        uint32 maxScanSteps;
    };
    struct ProcessPayouts_output
    {
        uint32 processed;
        uint32 returnCode;
    };
    struct ProcessPayouts_locals
    {
        Coin coin;
        HolderEntry entry;
        uint64 key;
        uint64 next;
        uint32 steps;
        PayHolder_input payIn;
        PayHolder_output payOut;
        bit failed;
        bit systemic;
    };

    // ================= Public I/O =================

    struct GetCoin_input
    {
        uint64 name;
    };
    struct GetCoin_output
    {
        Coin coin;
        GraduatedCoin record;  // set when graduated
        sint64 priceMilli;
        sint64 progressBps;
        sint64 marketCapQu;
        uint32 openingTicksLeft;
        bit found;
        bit graduated;
    };
    struct GetCoin_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        PriceAt_input priceIn;
        PriceAt_output priceOut;
    };

    struct GetHolder_input
    {
        id holder;
        uint64 name;
    };
    struct GetHolder_output
    {
        sint64 tokens;
        sint64 openingQu;
        sint64 refundQu;
        uint8 status;
        bit found;
    };
    struct GetHolder_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        uint64 key;
        HolderEntry entry;
        FindWallet_input walletIn;
        FindWallet_output walletOut;
        HolderKeyOf_input keyIn;
        HolderKeyOf_output keyOut;
        EffectiveTokens_input effectiveIn;
        EffectiveTokens_output effectiveOut;
        uint128 share;
    };

    struct QuoteBuy_input
    {
        uint64 name;
        sint64 tokens;
    };
    struct QuoteBuy_output
    {
        sint64 tokens;
        sint64 cost;
        sint64 fee;
        sint64 total;
        uint32 returnCode;
    };
    struct QuoteBuy_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        CurveCost_input costIn;
        CurveCost_output costOut;
        TradeFee_input feeIn;
        TradeFee_output feeOut;
    };

    struct QuoteBudget_input
    {
        uint64 name;
        sint64 quBudget;
    };
    struct QuoteBudget_output
    {
        sint64 tokens;
        sint64 total;
        uint32 returnCode;
    };
    struct QuoteBudget_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        TokensForTotal_input totalIn;
        TokensForTotal_output totalOut;
    };

    struct QuoteSell_input
    {
        uint64 name;
        sint64 tokens;
    };
    struct QuoteSell_output
    {
        sint64 gross;
        sint64 fee;
        sint64 payout;
        uint32 returnCode;
    };
    struct QuoteSell_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        CurveCost_input costIn;
        CurveCost_output costOut;
        TradeFee_input feeIn;
        TradeFee_output feeOut;
    };

    struct GetFees_input
    {
    };
    struct GetFees_output
    {
        sint64 launchFee;
        sint64 graduationReward;
        sint64 graduationBurn;
        sint64 minTradeFee;
        uint64 tradeFeeBps;
        uint64 feeBurnShareBps;
        uint64 feeQdogeShareBps;
        sint64 qxIssuanceFee;
        sint64 qxTransferFee;
        sint64 qswapPoolFee;
        sint64 curveSupply;
        sint64 poolReserve;
        uint64 startPriceMilli;
        uint64 endPriceMilli;
        sint64 curveRaise;
        sint64 openingQuCap;
        uint32 openingTicks;
    };

    struct ListCoins_input
    {
        uint32 offset;
    };
    struct ListCoins_output
    {
        Array<CoinSummary, 32> items;
        uint32 count;
        uint32 nextOffset;
    };
    struct ListCoins_locals
    {
        Coin coin;
        CoinSummary summary;
        uint32 slot;
    };

    struct ListGraduated_input
    {
        uint32 offset;
    };
    struct ListGraduated_output
    {
        Array<GraduatedCoin, 32> items;
        uint32 count;
        uint32 total;
    };
    struct ListGraduated_locals
    {
        uint32 i;
    };

    struct GetStats_input
    {
    };
    struct GetStats_output
    {
        uint64 holderEntries;
        uint64 totalLaunched;
        uint64 totalGraduated;
        uint64 totalRefunded;
        sint64 totalVolume;
        sint64 shareholderPot;
        sint64 burnPot;
        sint64 qdogePot;
        sint64 totalQuToQdoge;
        sint64 totalQdogeBought;
        sint64 feeReserve;
        sint64 totalDividends;
        sint64 totalBurned;
        uint64 wallets;
        uint32 liveCoins;
        uint32 graduatedRecords;
    };

    struct CreateCoin_input
    {
        id metaDigest;
        uint64 name;
    };
    struct CreateCoin_output
    {
        uint64 coinId;
        sint64 openingQu;
        uint32 returnCode;
    };
    struct CreateCoin_locals
    {
        IsValidName_input nameIn;
        IsValidName_output nameOut;
        Coin coin;
        sint64 extra;
        uint32 creatorCount;
        uint32 slot;
        PlaceOpeningOrder_input orderIn;
        PlaceOpeningOrder_output orderOut;
        QpumpLog log;
    };

    struct ExecuteBuy_input
    {
        uint64 name;
        sint64 tokens;
        sint64 minTokens;
        bit byQu;
    };
    struct ExecuteBuy_output
    {
        sint64 tokens;
        sint64 quSpent;
        uint32 returnCode;
    };
    struct ExecuteBuy_locals
    {
        LinkHolder_input linkIn;
        LinkHolder_output linkOut;
        TokensForTotal_input totalIn;
        TokensForTotal_output totalOut;
        FindCoin_input findIn;
        FindCoin_output findOut;
        SettleOpening_input settleIn;
        SettleOpening_output settleOut;
        CheckExpiry_input expiryIn;
        CheckExpiry_output expiryOut;
        PlaceOpeningOrder_input orderIn;
        PlaceOpeningOrder_output orderOut;
        Coin coin;
        uint64 key;
        HolderEntry entry;
        FindWallet_input walletIn;
        FindWallet_output walletOut;
        HolderKeyOf_input keyIn;
        HolderKeyOf_output keyOut;
        AcquireWallet_input acquireIn;
        AcquireWallet_output acquireOut;
        ChangePositions_input positionsIn;
        ChangePositions_output positionsOut;
        EffectiveTokens_input effectiveIn;
        EffectiveTokens_output effectiveOut;
        CurveCost_input costIn;
        CurveCost_output costOut;
        TradeFee_input feeIn;
        TradeFee_output feeOut;
        SplitFee_input splitIn;
        SplitFee_output splitOut;
        sint64 tokens;
        sint64 total;
        QpumpLog log;
        bit found;
    };

    struct Buy_input
    {
        uint64 name;
        sint64 tokens;    // exact tokens, ignored during the opening batch
    };
    struct Buy_output
    {
        sint64 tokens;
        sint64 quSpent;
        uint32 returnCode;
    };
    struct Buy_locals
    {
        ExecuteBuy_input executeIn;
        ExecuteBuy_output executeOut;
    };

    struct BuyWithQu_input
    {
        uint64 name;
        sint64 minTokens; // fewest tokens accepted for the attached QU
    };
    struct BuyWithQu_output
    {
        sint64 tokens;
        sint64 quSpent;
        uint32 returnCode;
    };
    struct BuyWithQu_locals
    {
        ExecuteBuy_input executeIn;
        ExecuteBuy_output executeOut;
    };

    struct Sell_input
    {
        uint64 name;
        sint64 tokens;
        sint64 minQuOut;
    };
    struct Sell_output
    {
        sint64 quOut;
        uint32 returnCode;
    };
    struct Sell_locals
    {
        UnlinkHolder_input unlinkIn;
        UnlinkHolder_output unlinkOut;
        FindCoin_input findIn;
        FindCoin_output findOut;
        SettleOpening_input settleIn;
        SettleOpening_output settleOut;
        CheckExpiry_input expiryIn;
        CheckExpiry_output expiryOut;
        Coin coin;
        uint64 key;
        HolderEntry entry;
        FindWallet_input walletIn;
        FindWallet_output walletOut;
        HolderKeyOf_input keyIn;
        HolderKeyOf_output keyOut;
        ChangePositions_input positionsIn;
        ChangePositions_output positionsOut;
        EffectiveTokens_input effectiveIn;
        EffectiveTokens_output effectiveOut;
        CurveCost_input costIn;
        CurveCost_output costOut;
        TradeFee_input feeIn;
        TradeFee_output feeOut;
        SplitFee_input splitIn;
        SplitFee_output splitOut;
        sint64 gross;
        sint64 payout;
        QpumpLog log;
    };

    struct Process_input
    {
        uint64 name;
        uint32 maxPayouts;
    };
    struct Process_output
    {
        uint32 returnCode;
        uint32 processed;
        uint8 status;
    };
    struct Process_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        SettleOpening_input settleIn;
        SettleOpening_output settleOut;
        CheckExpiry_input expiryIn;
        CheckExpiry_output expiryOut;
        TryGraduate_input graduateIn;
        TryGraduate_output graduateOut;
        ProcessPayouts_input payoutsIn;
        ProcessPayouts_output payoutsOut;
        Coin coin;
    };

    struct Claim_input
    {
        uint64 name;
    };
    struct Claim_output
    {
        sint64 tokens;
        sint64 qu;
        uint32 returnCode;
    };
    struct Claim_locals
    {
        FindCoin_input findIn;
        FindCoin_output findOut;
        FindWallet_input walletIn;
        FindWallet_output walletOut;
        HolderKeyOf_input keyIn;
        HolderKeyOf_output keyOut;
        PayHolder_input payIn;
        PayHolder_output payOut;
    };

    struct Transfer_input
    {
        id recipient;
        uint64 name;
        sint64 tokens;
    };
    struct Transfer_output
    {
        sint64 fee;
        uint32 returnCode;
    };
    struct Transfer_locals
    {
        UnlinkHolder_input unlinkIn;
        UnlinkHolder_output unlinkOut;
        LinkHolder_input linkIn;
        LinkHolder_output linkOut;
        FindCoin_input findIn;
        FindCoin_output findOut;
        SettleOpening_input settleIn;
        SettleOpening_output settleOut;
        CheckExpiry_input expiryIn;
        CheckExpiry_output expiryOut;
        Coin coin;
        uint64 fromKey;
        uint64 toKey;
        HolderEntry fromEntry;
        HolderEntry toEntry;
        FindWallet_input walletIn;
        FindWallet_output fromWallet;
        FindWallet_output toWallet;
        HolderKeyOf_input keyIn;
        HolderKeyOf_output keyOut;
        AcquireWallet_input acquireIn;
        AcquireWallet_output acquireOut;
        ChangePositions_input positionsIn;
        ChangePositions_output positionsOut;
        EffectiveTokens_input effectiveIn;
        EffectiveTokens_output effectiveOut;
        QpumpLog log;
        bit toFound;
    };

    struct BEGIN_EPOCH_locals
    {
        QX::Fees_input qxFeesIn;
        QX::Fees_output qxFeesOut;
        QSWAP::Fees_input qswapFeesIn;
        QSWAP::Fees_output qswapFeesOut;
    };

    struct BuyQdoge_input
    {
    };
    struct BuyQdoge_output
    {
    };
    struct BuyQdoge_locals
    {
        QSWAP::GetPoolBasicState_input poolIn;
        QSWAP::GetPoolBasicState_output poolOut;
        QSWAP::QuoteExactQuInput_input quoteIn;
        QSWAP::QuoteExactQuInput_output quoteOut;
        QSWAP::SwapExactQuForAsset_input swapIn;
        QSWAP::SwapExactQuForAsset_output swapOut;
        ContractBalance_input balanceIn;
        ContractBalance_output balanceOut;
        sint64 balanceBefore;
        sint64 amount;
        sint64 poolCap;
        QpumpLog log;
    };

    struct END_TICK_locals
    {
        Coin coin;
        uint32 slot;
        uint32 scanned;
        uint32 work;
        SettleOpening_input settleIn;
        SettleOpening_output settleOut;
        CheckExpiry_input expiryIn;
        CheckExpiry_output expiryOut;
        TryGraduate_input graduateIn;
        TryGraduate_output graduateOut;
        ProcessPayouts_input payoutsIn;
        ProcessPayouts_output payoutsOut;
    };

    struct END_EPOCH_locals
    {
        BuyQdoge_input qdogeIn;
        BuyQdoge_output qdogeOut;
        sint64 perShare;
    };

    // ================= Private functions =================

    // Integer area under the linear price line.
    PRIVATE_FUNCTION_WITH_LOCALS(CurveCost)
    {
        output.qu = 0;
        if (input.from < 0 || input.to <= input.from || input.to > QPUMP_CURVE_SUPPLY)
        {
            return;
        }
        locals.linear = uint128(QPUMP_COST_LINEAR) * uint128(static_cast<uint64>(input.to - input.from));
        locals.slope = (uint128(static_cast<uint64>(input.to)) * uint128(static_cast<uint64>(input.to))
            - uint128(static_cast<uint64>(input.from)) * uint128(static_cast<uint64>(input.from)))
            * uint128(QPUMP_COST_SLOPE);
        locals.numerator = locals.linear + locals.slope;
        locals.quotient = div(locals.numerator, uint128(QPUMP_COST_DENOMINATOR));
        if (input.roundUp && locals.quotient * uint128(QPUMP_COST_DENOMINATOR) < locals.numerator)
        {
            locals.quotient = locals.quotient + uint128(1ULL);
        }
        output.qu = static_cast<sint64>(locals.quotient.low);
    }

    PRIVATE_FUNCTION_WITH_LOCALS(TradeFee)
    {
        output.fee = QPUMP_MIN_TRADE_FEE;
        if (input.amount <= 0)
        {
            return;
        }
        locals.denominator = input.inclusive ? QPUMP_BPS + QPUMP_TRADE_FEE_BPS : QPUMP_BPS;
        locals.numerator = static_cast<uint64>(input.amount) * QPUMP_TRADE_FEE_BPS;
        locals.quotient = div(locals.numerator, locals.denominator);
        if (locals.quotient * locals.denominator < locals.numerator)
        {
            locals.quotient = locals.quotient + 1ULL;
        }
        if (static_cast<sint64>(locals.quotient) > QPUMP_MIN_TRADE_FEE)
        {
            output.fee = static_cast<sint64>(locals.quotient);
        }
    }

    // Most tokens whose curve cost fits the budget.
    PRIVATE_FUNCTION_WITH_LOCALS(TokensForQu)
    {
        output.tokens = 0;
        output.cost = 0;
        locals.lo = 0;
        locals.hi = input.limit;
        if (locals.hi > QPUMP_CURVE_SUPPLY - input.from)
        {
            locals.hi = QPUMP_CURVE_SUPPLY - input.from;
        }
        if (locals.hi <= 0 || input.budget <= 0)
        {
            return;
        }
        while (locals.lo < locals.hi)
        {
            locals.mid = locals.lo + ((locals.hi - locals.lo + 1) >> 1);
            locals.costIn.from = input.from;
            locals.costIn.to = input.from + locals.mid;
            locals.costIn.roundUp = 1;
            CALL(CurveCost, locals.costIn, locals.costOut);
            if (locals.costOut.qu <= input.budget)
            {
                locals.lo = locals.mid;
            }
            else
            {
                locals.hi = locals.mid - 1;
            }
        }
        output.tokens = locals.lo;
        if (locals.lo > 0)
        {
            locals.costIn.from = input.from;
            locals.costIn.to = input.from + locals.lo;
            locals.costIn.roundUp = 1;
            CALL(CurveCost, locals.costIn, locals.costOut);
            output.cost = locals.costOut.qu;
        }
    }

    // Most tokens whose cost plus fee fits the budget.
    PRIVATE_FUNCTION_WITH_LOCALS(TokensForTotal)
    {
        output.tokens = 0;
        output.cost = 0;
        output.fee = 0;
        locals.lo = 0;
        locals.hi = input.limit;
        if (locals.hi > QPUMP_CURVE_SUPPLY - input.from)
        {
            locals.hi = QPUMP_CURVE_SUPPLY - input.from;
        }
        if (locals.hi <= 0 || input.budget <= 0)
        {
            return;
        }
        while (locals.lo < locals.hi)
        {
            locals.mid = locals.lo + ((locals.hi - locals.lo + 1) >> 1);
            locals.costIn.from = input.from;
            locals.costIn.to = input.from + locals.mid;
            locals.costIn.roundUp = 1;
            CALL(CurveCost, locals.costIn, locals.costOut);
            locals.feeIn.amount = locals.costOut.qu;
            locals.feeIn.inclusive = 0;
            CALL(TradeFee, locals.feeIn, locals.feeOut);
            if (locals.costOut.qu + locals.feeOut.fee <= input.budget)
            {
                locals.lo = locals.mid;
            }
            else
            {
                locals.hi = locals.mid - 1;
            }
        }
        output.tokens = locals.lo;
        if (locals.lo > 0)
        {
            locals.costIn.from = input.from;
            locals.costIn.to = input.from + locals.lo;
            locals.costIn.roundUp = 1;
            CALL(CurveCost, locals.costIn, locals.costOut);
            locals.feeIn.amount = locals.costOut.qu;
            locals.feeIn.inclusive = 0;
            CALL(TradeFee, locals.feeIn, locals.feeOut);
            output.cost = locals.costOut.qu;
            output.fee = locals.feeOut.fee;
        }
    }

    // Mirrors qpi.issueAsset name rules.
    PRIVATE_FUNCTION_WITH_LOCALS(IsValidName)
    {
        output.valid = 0;
        if (input.name > 72057594037927935ULL)
        {
            return;
        }
        locals.ch = input.name & 255ULL;
        if (locals.ch < 65ULL || locals.ch > 90ULL)
        {
            return;
        }
        locals.ended = 0;
        for (locals.i = 1; locals.i < 7; locals.i++)
        {
            locals.ch = (input.name >> (locals.i * 8ULL)) & 255ULL;
            if (locals.ch == 0)
            {
                locals.ended = 1;
            }
            else
            {
                if (locals.ended)
                {
                    return;
                }
                if (!((locals.ch >= 48ULL && locals.ch <= 57ULL) || (locals.ch >= 65ULL && locals.ch <= 90ULL)))
                {
                    return;
                }
            }
        }
        output.valid = 1;
    }

    PRIVATE_FUNCTION(HolderKeyOf)
    {
        output.key = (static_cast<uint64>(input.walletId) << 32) | (input.coinId & QPUMP_COIN_KEY_MASK);
    }

    PRIVATE_FUNCTION(FindWallet)
    {
        output.walletId = 0;
        output.found = state.get().walletIds.get(input.wallet, output.walletId);
    }

    PRIVATE_FUNCTION_WITH_LOCALS(EffectiveTokens)
    {
        output.tokens = input.entry.tokens;
        if (input.entry.openingQu > 0 && input.coin.batchSettled && input.coin.batchQu > 0)
        {
            locals.share = div(uint128(static_cast<uint64>(input.coin.batchTokens)) * uint128(static_cast<uint64>(input.entry.openingQu)),
                uint128(static_cast<uint64>(input.coin.batchQu)));
            output.tokens = static_cast<sint64>(input.entry.tokens) + static_cast<sint64>(locals.share.low);
        }
    }

    PRIVATE_FUNCTION_WITH_LOCALS(FindCoin)
    {
        output.found = 0;
        output.graduated = 0;
        output.slot = QPUMP_NO_SLOT;
        if (!state.get().nameToSlot.get(input.name, locals.slot))
        {
            return;
        }
        if ((locals.slot & QPUMP_ARCHIVE_FLAG) != 0)
        {
            output.archiveIndex = locals.slot & (QPUMP_ARCHIVE_FLAG - 1);
            output.graduated = output.archiveIndex < state.get().graduatedCount ? 1 : 0;
            return;
        }
        if (locals.slot >= QPUMP_MAX_COINS)
        {
            return;
        }
        output.coin = state.get().coins.get(locals.slot);
        if (output.coin.status == QPUMP_STATUS_EMPTY || output.coin.name != input.name)
        {
            return;
        }
        output.slot = locals.slot;
        output.found = 1;
    }

    PRIVATE_FUNCTION_WITH_LOCALS(ContractBalance)
    {
        output.balance = 0;
        if (qpi.getEntity(SELF, locals.entity))
        {
            output.balance = locals.entity.incomingAmount - locals.entity.outgoingAmount;
        }
    }

    PRIVATE_FUNCTION(PriceAt)
    {
        output.priceMilli = static_cast<sint64>(QPUMP_START_PRICE_MILLI
            + div(QPUMP_COST_SLOPE * static_cast<uint64>(input.sold), static_cast<uint64>(QPUMP_CURVE_SUPPLY)));
    }

    // ================= Private procedures =================

    // Returns a wallet id, registering the wallet if new.
    PRIVATE_PROCEDURE_WITH_LOCALS(AcquireWallet)
    {
        output.ok = 0;
        if (state.get().walletIds.get(input.wallet, output.walletId))
        {
            output.ok = 1;
            return;
        }
        if (state.get().walletIds.population() >= QPUMP_WALLET_LOAD_LIMIT)
        {
            return;
        }
        if (state.get().freeWalletCount > 0)
        {
            state.mut().freeWalletCount -= 1;
            locals.walletId = state.get().freeWalletIds.get(state.get().freeWalletCount);
        }
        else if (state.get().nextWalletId < QPUMP_WALLET_CAPACITY)
        {
            locals.walletId = state.get().nextWalletId;
            state.mut().nextWalletId += 1;
        }
        else
        {
            return;
        }
        if (state.mut().walletIds.set(input.wallet, locals.walletId) == NULL_INDEX)
        {
            state.mut().freeWalletIds.set(state.get().freeWalletCount, locals.walletId);
            state.mut().freeWalletCount += 1;
            return;
        }
        state.mut().walletAddresses.set(locals.walletId, input.wallet);
        state.mut().walletPositions.set(locals.walletId, 0);
        output.walletId = locals.walletId;
        output.ok = 1;
    }

    // Tracks positions and frees the wallet id at zero.
    PRIVATE_PROCEDURE_WITH_LOCALS(ChangePositions)
    {
        if (input.walletId >= QPUMP_WALLET_CAPACITY)
        {
            return;
        }
        locals.wallet = state.get().walletAddresses.get(input.walletId);
        if (locals.wallet == NULL_ID)
        {
            return;
        }
        locals.positions = state.get().walletPositions.get(input.walletId);
        if (input.delta > 0)
        {
            locals.positions += 1;
        }
        else if (input.delta < 0 && locals.positions > 0)
        {
            locals.positions -= 1;
        }
        state.mut().walletPositions.set(input.walletId, locals.positions);
        if (locals.positions == 0)
        {
            state.mut().walletIds.removeByKey(locals.wallet);
            state.mut().walletAddresses.set(input.walletId, NULL_ID);
            state.mut().freeWalletIds.set(state.get().freeWalletCount, input.walletId);
            state.mut().freeWalletCount += 1;
        }
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(SplitFee)
    {
        if (input.fee <= 0)
        {
            return;
        }
        locals.burn = static_cast<sint64>(div(static_cast<uint64>(input.fee) * QPUMP_FEE_BURN_SHARE_BPS, QPUMP_BPS));
        locals.qdoge = static_cast<sint64>(div(static_cast<uint64>(input.fee) * QPUMP_FEE_QDOGE_SHARE_BPS, QPUMP_BPS));
        state.mut().burnPot += locals.burn;
        state.mut().qdogePot += locals.qdoge;
        state.mut().shareholderPot += input.fee - locals.burn - locals.qdoge;
    }

    // Puts a holder entry at the head of its list.
    PRIVATE_PROCEDURE_WITH_LOCALS(LinkHolder)
    {
        if (!state.get().holders.get(input.key, locals.entry))
        {
            return;
        }
        locals.head = state.get().holderHeads.get(input.slot);
        locals.entry.prevKey = 0;
        locals.entry.nextKey = locals.head;
        state.mut().holders.set(input.key, locals.entry);
        if (locals.head != 0 && state.get().holders.get(locals.head, locals.other))
        {
            locals.other.prevKey = input.key;
            state.mut().holders.set(locals.head, locals.other);
        }
        state.mut().holderHeads.set(input.slot, input.key);
    }

    // Unlinks a holder entry. Call before removing it.
    PRIVATE_PROCEDURE_WITH_LOCALS(UnlinkHolder)
    {
        if (!state.get().holders.get(input.key, locals.entry))
        {
            return;
        }
        if (locals.entry.prevKey != 0)
        {
            if (state.get().holders.get(locals.entry.prevKey, locals.other))
            {
                locals.other.nextKey = locals.entry.nextKey;
                state.mut().holders.set(locals.entry.prevKey, locals.other);
            }
        }
        else
        {
            state.mut().holderHeads.set(input.slot, locals.entry.nextKey);
        }
        if (locals.entry.nextKey != 0 && state.get().holders.get(locals.entry.nextKey, locals.other))
        {
            locals.other.prevKey = locals.entry.prevKey;
            state.mut().holders.set(locals.entry.nextKey, locals.other);
        }
    }

    // Adds an order to the batch. Caller refunds the rest.
    PRIVATE_PROCEDURE_WITH_LOCALS(PlaceOpeningOrder)
    {
        output.returnCode = QPUMP_OK;
        output.used = 0;
        output.net = 0;
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status != QPUMP_STATUS_OPENING)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        locals.room = QPUMP_OPENING_QU_CAP - locals.coin.batchQu;
        if (locals.room <= 0)
        {
            output.returnCode = QPUMP_ERR_BATCH_FULL;
            return;
        }
        locals.walletIn.wallet = input.buyer;
        CALL(FindWallet, locals.walletIn, locals.walletOut);
        locals.found = 0;
        if (locals.walletOut.found)
        {
            locals.keyIn.walletId = locals.walletOut.walletId;
            locals.keyIn.coinId = locals.coin.coinId;
            CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
            locals.key = locals.keyOut.key;
            locals.found = state.get().holders.get(locals.key, locals.entry);
        }
        if (!locals.found)
        {
            if (state.get().holders.population() >= QPUMP_HOLDER_LOAD_LIMIT)
            {
                output.returnCode = QPUMP_ERR_HOLDER_LIMIT;
                return;
            }
            if (!locals.walletOut.found && state.get().walletIds.population() >= QPUMP_WALLET_LOAD_LIMIT)
            {
                output.returnCode = QPUMP_ERR_WALLET_LIMIT;
                return;
            }
            locals.entry.tokens = 0;
            locals.entry.openingQu = 0;
        }

        locals.available = input.amount;
        locals.feeIn.amount = locals.available;
        locals.feeIn.inclusive = 1;
        CALL(TradeFee, locals.feeIn, locals.feeOut);
        locals.fee = locals.feeOut.fee;
        locals.net = locals.available - locals.fee;
        if (locals.net > locals.room)
        {
            locals.net = locals.room;
            locals.feeIn.amount = locals.net;
            locals.feeIn.inclusive = 0;
            CALL(TradeFee, locals.feeIn, locals.feeOut);
            locals.fee = locals.feeOut.fee;
        }
        locals.used = locals.net + locals.fee;
        if (locals.used > input.amount)
        {
            locals.net = locals.net - (locals.used - input.amount);
            locals.used = input.amount;
        }
        if (locals.net <= 0)
        {
            output.returnCode = QPUMP_ERR_INSUFFICIENT_REWARD;
            return;
        }

        if (!locals.walletOut.found)
        {
            locals.acquireIn.wallet = input.buyer;
            CALL(AcquireWallet, locals.acquireIn, locals.acquireOut);
            if (!locals.acquireOut.ok)
            {
                output.returnCode = QPUMP_ERR_WALLET_LIMIT;
                return;
            }
            locals.keyIn.walletId = locals.acquireOut.walletId;
            locals.keyIn.coinId = locals.coin.coinId;
            CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
            locals.key = locals.keyOut.key;
        }
        locals.entry.openingQu += static_cast<uint32>(locals.net);
        if (state.mut().holders.set(locals.key, locals.entry) == NULL_INDEX)
        {
            locals.positionsIn.walletId = static_cast<uint32>(locals.key >> 32);
            locals.positionsIn.delta = 0;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
            output.returnCode = QPUMP_ERR_CAPACITY;
            return;
        }
        if (!locals.found)
        {
            locals.coin.holders += 1;
            locals.positionsIn.walletId = static_cast<uint32>(locals.key >> 32);
            locals.positionsIn.delta = 1;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
            locals.linkIn.slot = input.slot;
            locals.linkIn.key = locals.key;
            CALL(LinkHolder, locals.linkIn, locals.linkOut);
        }
        locals.coin.batchQu += locals.net;
        locals.coin.lastTradeEpoch = qpi.epoch();
        state.mut().coins.set(input.slot, locals.coin);

        locals.splitIn.fee = locals.fee;
        CALL(SplitFee, locals.splitIn, locals.splitOut);
        state.mut().totalVolume += locals.net;

        output.used = locals.used;
        output.net = locals.net;

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_OPENING_ORDER;
        locals.log.fee = locals.fee;
        locals.log.holders = locals.coin.holders;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = input.buyer;
        locals.log.qu = locals.net;
        locals.log.tokens = 0;
        locals.log.realQu = locals.coin.batchQu;
        locals.log.sold = 0;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);
    }

    // Converts the batch into tokens once the window ends.
    PRIVATE_PROCEDURE_WITH_LOCALS(SettleOpening)
    {
        output.settled = 0;
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status != QPUMP_STATUS_OPENING)
        {
            return;
        }
        if (qpi.tick() < locals.coin.createdTick + QPUMP_OPENING_TICKS)
        {
            return;
        }
        locals.coin.batchTokens = 0;
        if (locals.coin.batchQu > 0)
        {
            locals.tokensIn.from = 0;
            locals.tokensIn.budget = locals.coin.batchQu;
            locals.tokensIn.limit = QPUMP_OPENING_TOKEN_CAP;
            CALL(TokensForQu, locals.tokensIn, locals.tokensOut);
            locals.coin.batchTokens = locals.tokensOut.tokens;
        }
        locals.coin.realQu = locals.coin.batchQu;
        locals.coin.sold = locals.coin.batchTokens;
        locals.coin.batchSettled = 1;
        locals.coin.status = QPUMP_STATUS_OPEN;
        locals.coin.lastTradeEpoch = qpi.epoch();
        state.mut().coins.set(input.slot, locals.coin);
        output.settled = 1;

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_BATCH_SETTLED;
        locals.log.holders = locals.coin.holders;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = locals.coin.creator;
        locals.log.qu = locals.coin.batchQu;
        locals.log.tokens = locals.coin.batchTokens;
        locals.log.realQu = locals.coin.realQu;
        locals.log.sold = locals.coin.sold;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);
    }

    PRIVATE_PROCEDURE_WITH_LOCALS(ReleaseCreator)
    {
        if (state.get().creatorLive.get(input.creator, locals.count))
        {
            if (locals.count <= 1)
            {
                state.mut().creatorLive.removeByKey(input.creator);
            }
            else
            {
                state.mut().creatorLive.set(input.creator, locals.count - 1);
            }
        }
    }

    // Frees a fully paid coin slot and burns unspent QU.
    // Graduated coins leave a record and keep their name.
    PRIVATE_PROCEDURE_WITH_LOCALS(CloseCoin)
    {
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status == QPUMP_STATUS_EMPTY)
        {
            return;
        }
        if (locals.coin.realQu > 0)
        {
            state.mut().burnPot += locals.coin.realQu;
        }
        if (locals.coin.status == QPUMP_STATUS_DISTRIBUTING && state.get().graduatedCount < QPUMP_ARCHIVE_CAPACITY)
        {
            locals.record.creator = locals.coin.creator;
            locals.record.metaDigest = locals.coin.metaDigest;
            locals.record.coinId = locals.coin.coinId;
            locals.record.name = locals.coin.name;
            locals.record.sold = locals.coin.sold;
            locals.record.poolFallback = locals.coin.gradStep < 4 ? 1 : 0;
            locals.record.poolQu = locals.record.poolFallback ? 0 : locals.coin.poolQu;
            locals.record.poolTokens = locals.record.poolFallback ? 0 : locals.coin.poolTokens;
            locals.record.createdTick = locals.coin.createdTick;
            locals.record.holders = locals.coin.gradHolders;
            locals.record.createdEpoch = locals.coin.createdEpoch;
            locals.record.completeEpoch = locals.coin.completeEpoch;
            locals.record.closedEpoch = qpi.epoch();
            state.mut().graduated.set(state.get().graduatedCount, locals.record);
            state.mut().nameToSlot.set(locals.coin.name, QPUMP_ARCHIVE_FLAG | state.get().graduatedCount);
            state.mut().graduatedCount += 1;
        }
        else
        {
            // Refunded names are freed, graduated names stay taken.
            state.mut().nameToSlot.removeByKey(locals.coin.name);
        }
        state.mut().holderHeads.set(input.slot, 0);
        state.mut().freeSlots.set(state.get().freeSlotCount, input.slot);
        state.mut().freeSlotCount += 1;
        if (state.get().liveCoins > 0)
        {
            state.mut().liveCoins -= 1;
        }
        if (state.get().activePayoutSlot == input.slot)
        {
            state.mut().activePayoutSlot = QPUMP_NO_SLOT;
        }

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_CLOSED;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = locals.coin.creator;
        locals.log.qu = locals.coin.realQu;
        locals.log.tokens = 0;
        locals.log.realQu = 0;
        locals.log.sold = locals.coin.sold;
        locals.log.code = locals.coin.status;
        LOG_INFO(locals.log);

        locals.empty = {};
        state.mut().coins.set(input.slot, locals.empty);
    }

    // Refunds a stale coin, reverts a graduation never started.
    PRIVATE_PROCEDURE_WITH_LOCALS(CheckExpiry)
    {
        output.changed = 0;
        locals.coin = state.get().coins.get(input.slot);

        if (locals.coin.status == QPUMP_STATUS_COMPLETE && locals.coin.gradStep == 0
            && qpi.epoch() >= locals.coin.completeEpoch + QPUMP_GRADUATION_STALL_EPOCHS)
        {
            locals.coin.status = QPUMP_STATUS_OPEN;
            locals.coin.lastTradeEpoch = qpi.epoch();
            state.mut().coins.set(input.slot, locals.coin);
            output.changed = 1;

            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QPUMP_LOG_REVERTED;
            locals.log.name = locals.coin.name;
            locals.log.coinId = locals.coin.coinId;
            locals.log.actor = locals.coin.creator;
            locals.log.qu = 0;
            locals.log.tokens = 0;
            locals.log.realQu = locals.coin.realQu;
            locals.log.sold = locals.coin.sold;
            locals.log.code = QPUMP_OK;
            LOG_INFO(locals.log);
            return;
        }

        // Pool steps keep failing: deliver tokens anyway and
        // return the unused pool QU to holders pro rata.
        if (locals.coin.status == QPUMP_STATUS_COMPLETE && locals.coin.gradStep >= 1 && locals.coin.gradStep < 4
            && qpi.epoch() >= locals.coin.completeEpoch + QPUMP_GRADUATION_STALL_EPOCHS)
        {
            locals.deliverBudget = static_cast<sint64>(locals.coin.holders) * state.get().cachedQxTransferFee * QPUMP_DELIVERY_BUDGET_FACTOR;
            if (locals.coin.realQu >= QPUMP_GRADUATION_REWARD + locals.deliverBudget)
            {
                qpi.transfer(locals.coin.creator, QPUMP_GRADUATION_REWARD);
                locals.coin.realQu -= QPUMP_GRADUATION_REWARD;
            }
            locals.coin.snapQu = locals.coin.realQu - locals.deliverBudget;
            if (locals.coin.snapQu < 0)
            {
                locals.coin.snapQu = 0;
            }
            locals.coin.snapSold = locals.coin.sold;
            locals.coin.status = QPUMP_STATUS_DISTRIBUTING;
            locals.coin.gradHolders = locals.coin.holders;
            locals.coin.scanCursor = 0;
            locals.coin.failures = 0;
            state.mut().coins.set(input.slot, locals.coin);
            state.mut().totalGraduated += 1;
            if (state.get().activePayoutSlot == QPUMP_NO_SLOT)
            {
                state.mut().activePayoutSlot = input.slot;
            }
            output.changed = 1;

            locals.releaseIn.creator = locals.coin.creator;
            CALL(ReleaseCreator, locals.releaseIn, locals.releaseOut);

            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QPUMP_LOG_GRADUATED;
            locals.log.holders = locals.coin.holders;
            locals.log.name = locals.coin.name;
            locals.log.coinId = locals.coin.coinId;
            locals.log.actor = locals.coin.creator;
            locals.log.qu = locals.coin.snapQu;
            locals.log.tokens = 0;
            locals.log.realQu = locals.coin.realQu;
            locals.log.sold = locals.coin.sold;
            locals.log.code = QPUMP_ERR_POOL_FALLBACK;
            LOG_INFO(locals.log);
            return;
        }

        if (locals.coin.status != QPUMP_STATUS_OPEN)
        {
            return;
        }
        locals.expired = qpi.epoch() >= locals.coin.lastTradeEpoch + QPUMP_IDLE_EPOCHS;
        if (!locals.expired)
        {
            return;
        }

        locals.coin.status = QPUMP_STATUS_REFUNDING;
        locals.coin.snapQu = locals.coin.realQu;
        locals.coin.snapSold = locals.coin.sold;
        locals.coin.scanCursor = 0;
        locals.coin.failures = 0;
        state.mut().coins.set(input.slot, locals.coin);
        state.mut().totalRefunded += 1;
        output.changed = 1;

        locals.releaseIn.creator = locals.coin.creator;
        CALL(ReleaseCreator, locals.releaseIn, locals.releaseOut);

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_REFUND_STARTED;
        locals.log.holders = locals.coin.holders;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = locals.coin.creator;
        locals.log.qu = locals.coin.snapQu;
        locals.log.tokens = locals.coin.snapSold;
        locals.log.realQu = locals.coin.realQu;
        locals.log.sold = locals.coin.sold;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);

        if (locals.coin.holders == 0)
        {
            locals.closeIn.slot = input.slot;
            CALL(CloseCoin, locals.closeIn, locals.closeOut);
        }
    }

    // Resumable graduation, since nothing rolls back.
    //   0: QX IssueAsset, this contract as issuer
    //   1: Qswap CreatePool, skipped if someone already created it
    //   2: QX TransferShareManagementRights of the pool tokens to Qswap
    //   3: Qswap AddLiquidity at exactly the final curve price
    //   4: creator reward, then DISTRIBUTING
    PRIVATE_PROCEDURE_WITH_LOCALS(TryGraduate)
    {
        output.returnCode = QPUMP_OK;
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status != QPUMP_STATUS_COMPLETE)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        locals.coin.lastAttemptTick = qpi.tick();
        state.mut().coins.set(input.slot, locals.coin);

        // Spending is measured from the balance, so kept fees
        // and refunds are always accounted exactly.
        if (locals.coin.gradStep == 0)
        {
            if (qpi.queryFeeReserve(QX_CONTRACT_INDEX) <= 0 || qpi.queryFeeReserve(QSWAP_CONTRACT_INDEX) <= 0)
            {
                output.returnCode = QPUMP_ERR_EXTERNAL_CALL;
            }
            else if (qpi.isAssetIssued(SELF, locals.coin.name))
            {
                output.returnCode = QPUMP_ERR_NAME_TAKEN;
            }
            else
            {
                locals.deliverBudget = static_cast<sint64>(locals.coin.holders) * state.get().cachedQxTransferFee * QPUMP_DELIVERY_BUDGET_FACTOR;
                locals.fixedCosts = state.get().cachedQxIssuanceFee + QPUMP_GRADUATION_REWARD + QPUMP_GRADUATION_BURN + state.get().cachedQswapPoolFee
                    + QPUMP_QSWAP_LIQUIDITY_FEE + locals.deliverBudget;
                locals.poolQu = locals.coin.realQu - locals.fixedCosts;
                locals.poolTokens = static_cast<sint64>(div(static_cast<uint64>(locals.poolQu > 0 ? locals.poolQu : 0) * QPUMP_MILLI, QPUMP_END_PRICE_MILLI));
                if (locals.poolTokens > QPUMP_POOL_RESERVE)
                {
                    locals.poolTokens = QPUMP_POOL_RESERVE;
                }
                if (locals.poolQu <= 0 || locals.poolTokens <= 0)
                {
                    output.returnCode = QPUMP_ERR_GRADUATION_BUDGET;
                }
                else
                {
                    locals.issueIn.assetName = locals.coin.name;
                    locals.issueIn.numberOfShares = locals.coin.sold + locals.poolTokens;
                    locals.issueIn.unitOfMeasurement = 0;
                    locals.issueIn.numberOfDecimalPlaces = 0;
                    CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                    locals.balanceBefore = locals.balanceOut.balance;
                    INVOKE_OTHER_CONTRACT_PROCEDURE_E(QX, IssueAsset, locals.issueIn, locals.issueOut, state.get().cachedQxIssuanceFee, issueError);
                    CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                    if (locals.balanceBefore > locals.balanceOut.balance)
                    {
                        locals.coin.realQu -= locals.balanceBefore - locals.balanceOut.balance;
                    }
                    if (issueError != NoCallError)
                    {
                        output.returnCode = QPUMP_ERR_EXTERNAL_CALL;
                    }
                    else if (locals.issueOut.issuedNumberOfShares != locals.coin.sold + locals.poolTokens)
                    {
                        output.returnCode = QPUMP_ERR_RETRY_LATER;
                    }
                    else
                    {
                        locals.coin.poolQu = locals.poolQu;
                        locals.coin.poolTokens = locals.poolTokens;
                        locals.coin.gradStep = 1;
                    }
                    state.mut().coins.set(input.slot, locals.coin);
                }
            }
        }

        // Create the pool first, so failure strands no tokens.
        if (output.returnCode == QPUMP_OK && locals.coin.gradStep == 1)
        {
            locals.poolStateIn.assetIssuer = SELF;
            locals.poolStateIn.assetName = locals.coin.name;
            CALL_OTHER_CONTRACT_FUNCTION_E(QSWAP, GetPoolBasicState, locals.poolStateIn, locals.poolStateOut, poolStateError);
            if (poolStateError != NoCallError)
            {
                output.returnCode = QPUMP_ERR_EXTERNAL_CALL;
            }
            else if (locals.poolStateOut.poolExists != 0)
            {
                if (locals.poolStateOut.totalLiquidity != 0)
                {
                    output.returnCode = QPUMP_ERR_RETRY_LATER;
                }
                else
                {
                    locals.coin.gradStep = 2;
                    state.mut().coins.set(input.slot, locals.coin);
                }
            }
            else if (locals.coin.realQu < state.get().cachedQswapPoolFee)
            {
                output.returnCode = QPUMP_ERR_GRADUATION_BUDGET;
            }
            else
            {
                locals.createIn.assetIssuer = SELF;
                locals.createIn.assetName = locals.coin.name;
                CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                locals.balanceBefore = locals.balanceOut.balance;
                INVOKE_OTHER_CONTRACT_PROCEDURE_E(QSWAP, CreatePool, locals.createIn, locals.createOut, state.get().cachedQswapPoolFee, createError);
                CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                if (locals.balanceBefore > locals.balanceOut.balance)
                {
                    locals.coin.realQu -= locals.balanceBefore - locals.balanceOut.balance;
                }
                if (createError != NoCallError)
                {
                    output.returnCode = QPUMP_ERR_EXTERNAL_CALL;
                }
                else if (!locals.createOut.success)
                {
                    output.returnCode = QPUMP_ERR_RETRY_LATER;
                }
                else
                {
                    locals.coin.gradStep = 2;
                }
                state.mut().coins.set(input.slot, locals.coin);
            }
        }

        if (output.returnCode == QPUMP_OK && locals.coin.gradStep == 2)
        {
            locals.rightsIn.asset.issuer = SELF;
            locals.rightsIn.asset.assetName = locals.coin.name;
            locals.rightsIn.numberOfShares = locals.coin.poolTokens;
            locals.rightsIn.newManagingContractIndex = QSWAP_CONTRACT_INDEX;
            INVOKE_OTHER_CONTRACT_PROCEDURE_E(QX, TransferShareManagementRights, locals.rightsIn, locals.rightsOut, 0, rightsError);
            if (rightsError != NoCallError)
            {
                output.returnCode = QPUMP_ERR_EXTERNAL_CALL;
            }
            else if (locals.rightsOut.transferredNumberOfShares != locals.coin.poolTokens)
            {
                output.returnCode = QPUMP_ERR_RETRY_LATER;
            }
            else
            {
                locals.coin.gradStep = 3;
                state.mut().coins.set(input.slot, locals.coin);
            }
        }

        if (output.returnCode == QPUMP_OK && locals.coin.gradStep == 3)
        {
            // If fees moved, seed only what this coin can afford.
            locals.deliverBudget = static_cast<sint64>(locals.coin.holders) * state.get().cachedQxTransferFee * QPUMP_DELIVERY_BUDGET_FACTOR;
            locals.available = locals.coin.realQu - QPUMP_GRADUATION_REWARD - QPUMP_GRADUATION_BURN - locals.deliverBudget - QPUMP_QSWAP_LIQUIDITY_FEE;
            if (locals.available < locals.coin.poolQu)
            {
                locals.coin.poolQu = locals.available;
            }
            if (locals.coin.poolQu <= 0)
            {
                output.returnCode = QPUMP_ERR_GRADUATION_BUDGET;
            }
            else
            {
                locals.addIn.assetIssuer = SELF;
                locals.addIn.assetName = locals.coin.name;
                locals.addIn.assetAmountDesired = locals.coin.poolTokens;
                locals.addIn.quAmountMin = locals.coin.poolQu;
                locals.addIn.assetAmountMin = locals.coin.poolTokens;
                CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                locals.balanceBefore = locals.balanceOut.balance;
                INVOKE_OTHER_CONTRACT_PROCEDURE_E(QSWAP, AddLiquidity, locals.addIn, locals.addOut, locals.coin.poolQu + QPUMP_QSWAP_LIQUIDITY_FEE, addError);
                CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                if (locals.balanceBefore > locals.balanceOut.balance)
                {
                    locals.coin.realQu -= locals.balanceBefore - locals.balanceOut.balance;
                }
                if (addError != NoCallError)
                {
                    output.returnCode = QPUMP_ERR_EXTERNAL_CALL;
                }
                else if (locals.addOut.userIncreaseLiquidity <= 0 || locals.addOut.assetAmount != locals.coin.poolTokens)
                {
                    output.returnCode = QPUMP_ERR_RETRY_LATER;
                }
                else
                {
                    locals.coin.gradStep = 4;
                }
                state.mut().coins.set(input.slot, locals.coin);
            }
        }

        if (output.returnCode == QPUMP_OK && locals.coin.gradStep == 4)
        {
            if (locals.coin.realQu >= QPUMP_GRADUATION_REWARD)
            {
                qpi.transfer(locals.coin.creator, QPUMP_GRADUATION_REWARD);
                locals.coin.realQu -= QPUMP_GRADUATION_REWARD;
            }
            // Burn from the raise, never the delivery budget.
            locals.deliverBudget = static_cast<sint64>(locals.coin.holders) * state.get().cachedQxTransferFee * QPUMP_DELIVERY_BUDGET_FACTOR;
            locals.burnAmount = locals.coin.realQu - locals.deliverBudget;
            if (locals.burnAmount > QPUMP_GRADUATION_BURN)
            {
                locals.burnAmount = QPUMP_GRADUATION_BURN;
            }
            if (locals.burnAmount > 0)
            {
                state.mut().burnPot += locals.burnAmount;
                locals.coin.realQu -= locals.burnAmount;
            }
            locals.coin.status = QPUMP_STATUS_DISTRIBUTING;
            locals.coin.gradHolders = locals.coin.holders;
            locals.coin.snapQu = 0;
            locals.coin.snapSold = 0;
            locals.coin.scanCursor = 0;
            locals.coin.failures = 0;
            state.mut().coins.set(input.slot, locals.coin);
            state.mut().totalGraduated += 1;
            if (state.get().activePayoutSlot == QPUMP_NO_SLOT)
            {
                state.mut().activePayoutSlot = input.slot;
            }

            locals.releaseIn.creator = locals.coin.creator;
            CALL(ReleaseCreator, locals.releaseIn, locals.releaseOut);

            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QPUMP_LOG_GRADUATED;
            locals.log.holders = locals.coin.holders;
            locals.log.name = locals.coin.name;
            locals.log.coinId = locals.coin.coinId;
            locals.log.actor = locals.coin.creator;
            locals.log.qu = locals.coin.poolQu;
            locals.log.tokens = locals.coin.poolTokens;
            locals.log.realQu = locals.coin.realQu;
            locals.log.sold = locals.coin.sold;
            locals.log.code = QPUMP_OK;
            LOG_INFO(locals.log);
            return;
        }

        if (output.returnCode != QPUMP_OK)
        {
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QPUMP_LOG_GRADUATION_FAILED;
            locals.log.name = locals.coin.name;
            locals.log.coinId = locals.coin.coinId;
            locals.log.actor = locals.coin.creator;
            locals.log.qu = locals.coin.realQu;
            locals.log.tokens = locals.coin.gradStep;
            locals.log.realQu = locals.coin.realQu;
            locals.log.sold = locals.coin.sold;
            locals.log.code = output.returnCode;
            LOG_INFO(locals.log);
        }
    }

    // Delivers or refunds one holder, then removes the entry.
    PRIVATE_PROCEDURE_WITH_LOCALS(PayHolder)
    {
        output.returnCode = QPUMP_OK;
        output.tokens = 0;
        output.qu = 0;
        output.callerFeeUsed = 0;
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status != QPUMP_STATUS_DISTRIBUTING && locals.coin.status != QPUMP_STATUS_REFUNDING)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        if ((input.key & QPUMP_COIN_KEY_MASK) != (locals.coin.coinId & QPUMP_COIN_KEY_MASK) || !state.get().holders.get(input.key, locals.entry))
        {
            output.returnCode = QPUMP_ERR_HOLDER_NOT_FOUND;
            return;
        }
        locals.holder = state.get().walletAddresses.get(input.key >> 32);
        locals.effectiveIn.coin = locals.coin;
        locals.effectiveIn.entry = locals.entry;
        CALL(EffectiveTokens, locals.effectiveIn, locals.effectiveOut);
        locals.tokens = locals.effectiveOut.tokens;

        if (locals.coin.status == QPUMP_STATUS_DISTRIBUTING)
        {
            if (locals.tokens > 0)
            {
                locals.fee = state.get().cachedQxTransferFee;
                locals.fromCaller = 0;
                if (locals.coin.realQu < locals.fee)
                {
                    if (input.callerFee < locals.fee)
                    {
                        output.returnCode = QPUMP_ERR_INSUFFICIENT_REWARD;
                        return;
                    }
                    locals.fromCaller = 1;
                }
                locals.transferIn.issuer = SELF;
                locals.transferIn.newOwnerAndPossessor = locals.holder;
                locals.transferIn.assetName = locals.coin.name;
                locals.transferIn.numberOfShares = locals.tokens;
                CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                locals.balanceBefore = locals.balanceOut.balance;
                INVOKE_OTHER_CONTRACT_PROCEDURE_E(QX, TransferShareOwnershipAndPossession, locals.transferIn, locals.transferOut, locals.fee, transferError);
                CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
                // QX keeps its fee even when the transfer fails.
                locals.qu = locals.balanceBefore > locals.balanceOut.balance ? locals.balanceBefore - locals.balanceOut.balance : 0;
                if (locals.fromCaller)
                {
                    output.callerFeeUsed = locals.qu;
                }
                else
                {
                    locals.coin.realQu -= locals.qu;
                }
                if (transferError != NoCallError || locals.transferOut.transferredNumberOfShares != locals.tokens)
                {
                    state.mut().coins.set(input.slot, locals.coin);
                    output.returnCode = transferError != NoCallError ? QPUMP_ERR_EXTERNAL_CALL : QPUMP_ERR_TRANSFER_FAILED;
                    return;
                }
            }
            output.tokens = locals.tokens;
            // Pool fallback: unused pool QU returns pro rata.
            locals.qu = 0;
            if (locals.coin.snapQu > 0 && locals.coin.snapSold > 0 && locals.tokens > 0)
            {
                locals.share = div(uint128(static_cast<uint64>(locals.coin.snapQu)) * uint128(static_cast<uint64>(locals.tokens)),
                    uint128(static_cast<uint64>(locals.coin.snapSold)));
                locals.qu = static_cast<sint64>(locals.share.low);
                if (locals.qu > locals.coin.realQu)
                {
                    locals.qu = locals.coin.realQu;
                }
                if (locals.qu > 0)
                {
                    qpi.transfer(locals.holder, locals.qu);
                    locals.coin.realQu -= locals.qu;
                }
            }
            output.qu = locals.qu;
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QPUMP_LOG_DELIVERED;
        }
        else
        {
            locals.qu = 0;
            if (locals.coin.snapSold > 0 && locals.tokens > 0)
            {
                locals.share = div(uint128(static_cast<uint64>(locals.coin.snapQu)) * uint128(static_cast<uint64>(locals.tokens)),
                    uint128(static_cast<uint64>(locals.coin.snapSold)));
                locals.qu = static_cast<sint64>(locals.share.low);
            }
            if (locals.qu > locals.coin.realQu)
            {
                locals.qu = locals.coin.realQu;
            }
            if (locals.qu > 0)
            {
                qpi.transfer(locals.holder, locals.qu);
                locals.coin.realQu -= locals.qu;
            }
            output.qu = locals.qu;
            locals.log._contractIndex = SELF_INDEX;
            locals.log._type = QPUMP_LOG_REFUNDED;
        }

        locals.unlinkIn.slot = input.slot;
        locals.unlinkIn.key = input.key;
        CALL(UnlinkHolder, locals.unlinkIn, locals.unlinkOut);
        state.mut().holders.removeByKey(input.key);
        if (locals.coin.holders > 0)
        {
            locals.coin.holders -= 1;
        }
        locals.positionsIn.walletId = static_cast<uint32>(input.key >> 32);
        locals.positionsIn.delta = -1;
        CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
        state.mut().coins.set(input.slot, locals.coin);

        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = locals.holder;
        locals.log.holders = locals.coin.holders;
        locals.log.qu = output.qu;
        locals.log.tokens = output.tokens;
        locals.log.realQu = locals.coin.realQu;
        locals.log.sold = locals.coin.sold;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);

        if (locals.coin.holders == 0)
        {
            locals.closeIn.slot = input.slot;
            CALL(CloseCoin, locals.closeIn, locals.closeOut);
        }
    }

    // Walks the holder list from the cursor, paying each.
    PRIVATE_PROCEDURE_WITH_LOCALS(ProcessPayouts)
    {
        output.processed = 0;
        output.returnCode = QPUMP_OK;
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status != QPUMP_STATUS_DISTRIBUTING && locals.coin.status != QPUMP_STATUS_REFUNDING)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        locals.key = locals.coin.scanCursor;
        if (locals.key == 0 || !state.get().holders.contains(locals.key))
        {
            locals.key = state.get().holderHeads.get(input.slot);
        }
        locals.steps = 0;
        locals.failed = 0;
        locals.systemic = 0;
        while (locals.key != 0 && locals.steps < input.maxScanSteps && output.processed < input.maxPayouts)
        {
            if (!state.get().holders.get(locals.key, locals.entry))
            {
                locals.key = 0;
                break;
            }
            locals.next = locals.entry.nextKey;
            locals.steps += 1;
            locals.payIn.slot = input.slot;
            locals.payIn.key = locals.key;
            locals.payIn.callerFee = 0;
            CALL(PayHolder, locals.payIn, locals.payOut);
            if (locals.payOut.returnCode != QPUMP_OK)
            {
                output.returnCode = locals.payOut.returnCode;
                locals.failed = 1;
                if (locals.payOut.returnCode == QPUMP_ERR_EXTERNAL_CALL)
                {
                    // QX is unavailable, every holder would fail alike.
                    locals.systemic = 1;
                    break;
                }
                locals.key = locals.next;
                continue;
            }
            output.processed += 1;
            if (state.get().coins.get(input.slot).status == QPUMP_STATUS_EMPTY)
            {
                return;
            }
            locals.key = locals.next;
        }
        locals.coin = state.get().coins.get(input.slot);
        if (locals.coin.status == QPUMP_STATUS_EMPTY)
        {
            return;
        }
        locals.coin.scanCursor = locals.key;
        if (locals.failed && output.processed == 0)
        {
            if (locals.coin.failures < 255)
            {
                locals.coin.failures += 1;
            }
        }
        else if (output.processed > 0)
        {
            locals.coin.failures = 0;
        }
        state.mut().coins.set(input.slot, locals.coin);
    }

    // ================= Public functions =================

    PUBLIC_FUNCTION_WITH_LOCALS(GetCoin)
    {
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        output.found = locals.findOut.found;
        output.graduated = locals.findOut.graduated;
        if (output.graduated)
        {
            output.record = state.get().graduated.get(locals.findOut.archiveIndex);
            output.priceMilli = static_cast<sint64>(QPUMP_END_PRICE_MILLI);
            output.progressBps = static_cast<sint64>(QPUMP_BPS);
            return;
        }
        if (!output.found)
        {
            return;
        }
        output.coin = locals.findOut.coin;
        locals.priceIn.sold = output.coin.sold;
        CALL(PriceAt, locals.priceIn, locals.priceOut);
        output.priceMilli = locals.priceOut.priceMilli;
        output.progressBps = static_cast<sint64>(div(static_cast<uint64>(output.coin.sold) * QPUMP_BPS, static_cast<uint64>(QPUMP_CURVE_SUPPLY)));
        output.marketCapQu = static_cast<sint64>(div(static_cast<uint64>(output.priceMilli) * 1000000000ULL, QPUMP_MILLI));
        output.openingTicksLeft = 0;
        if (output.coin.status == QPUMP_STATUS_OPENING && qpi.tick() < output.coin.createdTick + QPUMP_OPENING_TICKS)
        {
            output.openingTicksLeft = output.coin.createdTick + QPUMP_OPENING_TICKS - qpi.tick();
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetHolder)
    {
        output.found = 0;
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            return;
        }
        output.status = locals.findOut.coin.status;
        locals.walletIn.wallet = input.holder;
        CALL(FindWallet, locals.walletIn, locals.walletOut);
        if (!locals.walletOut.found)
        {
            return;
        }
        locals.keyIn.walletId = locals.walletOut.walletId;
        locals.keyIn.coinId = locals.findOut.coin.coinId;
        CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
        locals.key = locals.keyOut.key;
        if (!state.get().holders.get(locals.key, locals.entry))
        {
            return;
        }
        output.found = 1;
        locals.effectiveIn.coin = locals.findOut.coin;
        locals.effectiveIn.entry = locals.entry;
        CALL(EffectiveTokens, locals.effectiveIn, locals.effectiveOut);
        output.tokens = locals.effectiveOut.tokens;
        output.openingQu = locals.findOut.coin.batchSettled ? 0 : static_cast<sint64>(locals.entry.openingQu);
        if (locals.findOut.coin.status == QPUMP_STATUS_REFUNDING && locals.findOut.coin.snapSold > 0)
        {
            locals.share = div(uint128(static_cast<uint64>(locals.findOut.coin.snapQu)) * uint128(static_cast<uint64>(output.tokens)),
                uint128(static_cast<uint64>(locals.findOut.coin.snapSold)));
            output.refundQu = static_cast<sint64>(locals.share.low);
        }
    }

    PUBLIC_FUNCTION_WITH_LOCALS(QuoteBuy)
    {
        output.returnCode = QPUMP_OK;
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        if (locals.findOut.coin.status != QPUMP_STATUS_OPEN)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        output.tokens = input.tokens;
        if (output.tokens > QPUMP_CURVE_SUPPLY - locals.findOut.coin.sold)
        {
            output.tokens = QPUMP_CURVE_SUPPLY - locals.findOut.coin.sold;
        }
        if (output.tokens <= 0)
        {
            output.returnCode = QPUMP_ERR_INVALID_INPUT;
            return;
        }
        locals.costIn.from = locals.findOut.coin.sold;
        locals.costIn.to = locals.findOut.coin.sold + output.tokens;
        locals.costIn.roundUp = 1;
        CALL(CurveCost, locals.costIn, locals.costOut);
        locals.feeIn.amount = locals.costOut.qu;
        locals.feeIn.inclusive = 0;
        CALL(TradeFee, locals.feeIn, locals.feeOut);
        output.cost = locals.costOut.qu;
        output.fee = locals.feeOut.fee;
        output.total = output.cost + output.fee;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(QuoteBudget)
    {
        output.returnCode = QPUMP_OK;
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        if (locals.findOut.coin.status != QPUMP_STATUS_OPEN)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        locals.totalIn.from = locals.findOut.coin.sold;
        locals.totalIn.budget = input.quBudget;
        locals.totalIn.limit = QPUMP_CURVE_SUPPLY;
        CALL(TokensForTotal, locals.totalIn, locals.totalOut);
        output.tokens = locals.totalOut.tokens;
        output.total = output.tokens > 0 ? locals.totalOut.cost + locals.totalOut.fee : 0;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(QuoteSell)
    {
        output.returnCode = QPUMP_OK;
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        if (locals.findOut.coin.status != QPUMP_STATUS_OPEN)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        if (input.tokens <= 0 || input.tokens > locals.findOut.coin.sold)
        {
            output.returnCode = QPUMP_ERR_INVALID_INPUT;
            return;
        }
        locals.costIn.from = locals.findOut.coin.sold - input.tokens;
        locals.costIn.to = locals.findOut.coin.sold;
        locals.costIn.roundUp = 0;
        CALL(CurveCost, locals.costIn, locals.costOut);
        locals.feeIn.amount = locals.costOut.qu;
        locals.feeIn.inclusive = 0;
        CALL(TradeFee, locals.feeIn, locals.feeOut);
        output.gross = locals.costOut.qu;
        output.fee = locals.feeOut.fee;
        output.payout = output.gross > output.fee ? output.gross - output.fee : 0;
    }

    PUBLIC_FUNCTION(GetFees)
    {
        output.launchFee = QPUMP_LAUNCH_FEE;
        output.graduationReward = QPUMP_GRADUATION_REWARD;
        output.graduationBurn = QPUMP_GRADUATION_BURN;
        output.minTradeFee = QPUMP_MIN_TRADE_FEE;
        output.tradeFeeBps = QPUMP_TRADE_FEE_BPS;
        output.feeBurnShareBps = QPUMP_FEE_BURN_SHARE_BPS;
        output.feeQdogeShareBps = QPUMP_FEE_QDOGE_SHARE_BPS;
        output.qxIssuanceFee = state.get().cachedQxIssuanceFee;
        output.qxTransferFee = state.get().cachedQxTransferFee;
        output.qswapPoolFee = state.get().cachedQswapPoolFee;
        output.curveSupply = QPUMP_CURVE_SUPPLY;
        output.poolReserve = QPUMP_POOL_RESERVE;
        output.startPriceMilli = QPUMP_START_PRICE_MILLI;
        output.endPriceMilli = QPUMP_END_PRICE_MILLI;
        output.curveRaise = QPUMP_CURVE_RAISE;
        output.openingQuCap = QPUMP_OPENING_QU_CAP;
        output.openingTicks = QPUMP_OPENING_TICKS;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(ListCoins)
    {
        output.count = 0;
        output.nextOffset = 0;
        locals.slot = input.offset;
        while (locals.slot < state.get().nextFreshSlot && output.count < QPUMP_LIST_PAGE)
        {
            locals.coin = state.get().coins.get(locals.slot);
            if (locals.coin.status != QPUMP_STATUS_EMPTY)
            {
                locals.summary.name = locals.coin.name;
                locals.summary.coinId = locals.coin.coinId;
                locals.summary.realQu = locals.coin.realQu;
                locals.summary.sold = locals.coin.sold;
                locals.summary.batchQu = locals.coin.batchQu;
                locals.summary.holders = locals.coin.holders;
                locals.summary.createdTick = locals.coin.createdTick;
                locals.summary.status = locals.coin.status;
                output.items.set(output.count, locals.summary);
                output.count += 1;
            }
            locals.slot += 1;
        }
        output.nextOffset = locals.slot < state.get().nextFreshSlot ? locals.slot : 0;
    }

    // Pages graduated records, oldest first.
    PUBLIC_FUNCTION_WITH_LOCALS(ListGraduated)
    {
        output.count = 0;
        output.total = state.get().graduatedCount;
        locals.i = input.offset;
        while (locals.i < state.get().graduatedCount && output.count < QPUMP_LIST_PAGE)
        {
            output.items.set(output.count, state.get().graduated.get(locals.i));
            output.count += 1;
            locals.i += 1;
        }
    }

    PUBLIC_FUNCTION(GetStats)
    {
        output.graduatedRecords = state.get().graduatedCount;
        output.liveCoins = state.get().liveCoins;
        output.holderEntries = state.get().holders.population();
        output.totalLaunched = state.get().totalLaunched;
        output.totalGraduated = state.get().totalGraduated;
        output.totalRefunded = state.get().totalRefunded;
        output.totalVolume = state.get().totalVolume;
        output.shareholderPot = state.get().shareholderPot;
        output.burnPot = state.get().burnPot;
        output.feeReserve = qpi.queryFeeReserve(SELF_INDEX);
        output.totalDividends = state.get().totalDividends;
        output.totalBurned = state.get().totalBurned;
        output.qdogePot = state.get().qdogePot;
        output.totalQuToQdoge = state.get().totalQuToQdoge;
        output.totalQdogeBought = state.get().totalQdogeBought;
        output.wallets = state.get().walletIds.population();
    }

    // ================= Public procedures =================

    // Launch a coin. Extra QU joins the opening batch.
    PUBLIC_PROCEDURE_WITH_LOCALS(CreateCoin)
    {
        output.returnCode = QPUMP_OK;
        output.coinId = 0;
        output.openingQu = 0;
        if (qpi.invocator() != qpi.originator())
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_USERS_ONLY;
            return;
        }
        if (qpi.invocationReward() < QPUMP_LAUNCH_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_INSUFFICIENT_REWARD;
            return;
        }
        locals.nameIn.name = input.name;
        CALL(IsValidName, locals.nameIn, locals.nameOut);
        if (!locals.nameOut.valid)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_INVALID_NAME;
            return;
        }
        if (state.get().nameToSlot.contains(input.name) || qpi.isAssetIssued(SELF, input.name))
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_NAME_TAKEN;
            return;
        }
        if (state.get().liveCoins >= QPUMP_MAX_COINS)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_CAPACITY;
            return;
        }
        locals.creatorCount = 0;
        state.get().creatorLive.get(qpi.invocator(), locals.creatorCount);
        if (locals.creatorCount >= QPUMP_MAX_LIVE_COINS_PER_CREATOR)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_CREATOR_LIMIT;
            return;
        }
        if (locals.creatorCount == 0 && state.get().creatorLive.population() >= QPUMP_CREATOR_LOAD_LIMIT)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_CAPACITY;
            return;
        }
        if (state.get().freeSlotCount > 0)
        {
            state.mut().freeSlotCount -= 1;
            locals.slot = state.get().freeSlots.get(state.get().freeSlotCount);
        }
        else if (state.get().nextFreshSlot < QPUMP_MAX_COINS)
        {
            locals.slot = state.get().nextFreshSlot;
            state.mut().nextFreshSlot += 1;
        }
        else
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_CAPACITY;
            return;
        }

        locals.coin = {};
        locals.coin.creator = qpi.invocator();
        locals.coin.metaDigest = input.metaDigest;
        locals.coin.coinId = state.get().nextCoinId;
        locals.coin.name = input.name;
        locals.coin.createdTick = qpi.tick();
        locals.coin.createdEpoch = qpi.epoch();
        locals.coin.lastTradeEpoch = qpi.epoch();
        locals.coin.status = QPUMP_STATUS_OPENING;
        locals.coin.scanCursor = 0;
        state.mut().coins.set(locals.slot, locals.coin);
        state.mut().holderHeads.set(locals.slot, 0);
        state.mut().nameToSlot.set(input.name, locals.slot);
        state.mut().creatorLive.set(qpi.invocator(), locals.creatorCount + 1);
        state.mut().liveCoins += 1;
        state.mut().nextCoinId += 1;
        state.mut().totalLaunched += 1;
        state.mut().shareholderPot += QPUMP_LAUNCH_FEE - QPUMP_LAUNCH_FEE_BURN;
        state.mut().burnPot += QPUMP_LAUNCH_FEE_BURN;
        output.coinId = locals.coin.coinId;

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_CREATED;
        locals.log.counterparty = input.metaDigest;
        locals.log.name = input.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = qpi.invocator();
        locals.log.qu = QPUMP_LAUNCH_FEE;
        locals.log.tokens = QPUMP_CURVE_SUPPLY;
        locals.log.realQu = 0;
        locals.log.sold = 0;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);

        locals.extra = qpi.invocationReward() - QPUMP_LAUNCH_FEE;
        if (locals.extra > 0)
        {
            locals.orderIn.slot = locals.slot;
            locals.orderIn.buyer = qpi.invocator();
            locals.orderIn.amount = locals.extra;
            CALL(PlaceOpeningOrder, locals.orderIn, locals.orderOut);
            if (locals.orderOut.returnCode == QPUMP_OK)
            {
                locals.extra -= locals.orderOut.used;
                output.openingQu = locals.orderOut.net;
            }
        }
        if (locals.extra > 0)
        {
            qpi.transfer(qpi.invocator(), locals.extra);
        }
    }

    // Shared by Buy and BuyWithQu. In the batch the whole
    // reward joins it; on the curve the mode decides.
    PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBuy)
    {
        output.returnCode = QPUMP_OK;
        output.tokens = 0;
        output.quSpent = 0;
        if (qpi.invocator() != qpi.originator())
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_USERS_ONLY;
            return;
        }
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        locals.settleIn.slot = locals.findOut.slot;
        CALL(SettleOpening, locals.settleIn, locals.settleOut);
        locals.expiryIn.slot = locals.findOut.slot;
        CALL(CheckExpiry, locals.expiryIn, locals.expiryOut);
        locals.coin = state.get().coins.get(locals.findOut.slot);

        if (locals.coin.status == QPUMP_STATUS_OPENING)
        {
            locals.orderIn.slot = locals.findOut.slot;
            locals.orderIn.buyer = qpi.invocator();
            locals.orderIn.amount = qpi.invocationReward();
            CALL(PlaceOpeningOrder, locals.orderIn, locals.orderOut);
            if (locals.orderOut.returnCode != QPUMP_OK)
            {
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                output.returnCode = locals.orderOut.returnCode;
                return;
            }
            if (qpi.invocationReward() > locals.orderOut.used)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.orderOut.used);
            }
            output.quSpent = locals.orderOut.used;
            return;
        }

        if (locals.coin.status != QPUMP_STATUS_OPEN)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        if ((!input.byQu && input.tokens <= 0) || (input.byQu && (input.minTokens <= 0 || qpi.invocationReward() <= 0)))
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_INVALID_INPUT;
            return;
        }
        if (input.byQu)
        {
            locals.totalIn.from = locals.coin.sold;
            locals.totalIn.budget = qpi.invocationReward();
            locals.totalIn.limit = QPUMP_CURVE_SUPPLY;
            CALL(TokensForTotal, locals.totalIn, locals.totalOut);
            locals.tokens = locals.totalOut.tokens;
            if (locals.coin.sold < QPUMP_CURVE_SUPPLY && locals.tokens < input.minTokens)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.returnCode = QPUMP_ERR_SLIPPAGE;
                return;
            }
        }
        else
        {
            locals.tokens = input.tokens;
        }
        if (locals.tokens > QPUMP_CURVE_SUPPLY - locals.coin.sold)
        {
            locals.tokens = QPUMP_CURVE_SUPPLY - locals.coin.sold;
        }
        if (locals.tokens <= 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }

        locals.walletIn.wallet = qpi.invocator();
        CALL(FindWallet, locals.walletIn, locals.walletOut);
        locals.found = 0;
        if (locals.walletOut.found)
        {
            locals.keyIn.walletId = locals.walletOut.walletId;
            locals.keyIn.coinId = locals.coin.coinId;
            CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
            locals.key = locals.keyOut.key;
            locals.found = state.get().holders.get(locals.key, locals.entry);
        }
        if (!locals.found)
        {
            if (state.get().holders.population() >= QPUMP_HOLDER_LOAD_LIMIT)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.returnCode = QPUMP_ERR_HOLDER_LIMIT;
                return;
            }
            if (!locals.walletOut.found && state.get().walletIds.population() >= QPUMP_WALLET_LOAD_LIMIT)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.returnCode = QPUMP_ERR_WALLET_LIMIT;
                return;
            }
            locals.entry.tokens = 0;
            locals.entry.openingQu = 0;
        }
        else
        {
            locals.effectiveIn.coin = locals.coin;
            locals.effectiveIn.entry = locals.entry;
            CALL(EffectiveTokens, locals.effectiveIn, locals.effectiveOut);
            locals.entry.tokens = static_cast<uint32>(locals.effectiveOut.tokens);
            locals.entry.openingQu = 0;
        }

        locals.costIn.from = locals.coin.sold;
        locals.costIn.to = locals.coin.sold + locals.tokens;
        locals.costIn.roundUp = 1;
        CALL(CurveCost, locals.costIn, locals.costOut);
        locals.feeIn.amount = locals.costOut.qu;
        locals.feeIn.inclusive = 0;
        CALL(TradeFee, locals.feeIn, locals.feeOut);
        locals.total = locals.costOut.qu + locals.feeOut.fee;
        if (locals.total > qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_SLIPPAGE;
            return;
        }

        if (!locals.walletOut.found)
        {
            locals.acquireIn.wallet = qpi.invocator();
            CALL(AcquireWallet, locals.acquireIn, locals.acquireOut);
            if (!locals.acquireOut.ok)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.returnCode = QPUMP_ERR_WALLET_LIMIT;
                return;
            }
            locals.keyIn.walletId = locals.acquireOut.walletId;
            locals.keyIn.coinId = locals.coin.coinId;
            CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
            locals.key = locals.keyOut.key;
        }
        locals.entry.tokens += static_cast<uint32>(locals.tokens);
        if (state.mut().holders.set(locals.key, locals.entry) == NULL_INDEX)
        {
            locals.positionsIn.walletId = static_cast<uint32>(locals.key >> 32);
            locals.positionsIn.delta = 0;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_CAPACITY;
            return;
        }
        if (!locals.found)
        {
            locals.coin.holders += 1;
            locals.positionsIn.walletId = static_cast<uint32>(locals.key >> 32);
            locals.positionsIn.delta = 1;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
            locals.linkIn.slot = locals.findOut.slot;
            locals.linkIn.key = locals.key;
            CALL(LinkHolder, locals.linkIn, locals.linkOut);
        }
        locals.coin.realQu += locals.costOut.qu;
        locals.coin.sold += locals.tokens;
        locals.coin.lastTradeEpoch = qpi.epoch();
        if (locals.coin.sold >= QPUMP_CURVE_SUPPLY)
        {
            locals.coin.status = QPUMP_STATUS_COMPLETE;
            locals.coin.completeEpoch = qpi.epoch();
            locals.coin.gradStep = 0;
            locals.coin.lastAttemptTick = 0;
        }
        state.mut().coins.set(locals.findOut.slot, locals.coin);

        locals.splitIn.fee = locals.feeOut.fee;
        CALL(SplitFee, locals.splitIn, locals.splitOut);
        state.mut().totalVolume += locals.costOut.qu;

        if (qpi.invocationReward() > locals.total)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.total);
        }
        output.tokens = locals.tokens;
        output.quSpent = locals.total;

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = locals.coin.status == QPUMP_STATUS_COMPLETE ? QPUMP_LOG_COMPLETE : QPUMP_LOG_BUY;
        locals.log.fee = locals.feeOut.fee;
        locals.log.holders = locals.coin.holders;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = qpi.invocator();
        locals.log.qu = locals.costOut.qu;
        locals.log.tokens = locals.tokens;
        locals.log.realQu = locals.coin.realQu;
        locals.log.sold = locals.coin.sold;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(Buy)
    {
        locals.executeIn.name = input.name;
        locals.executeIn.tokens = input.tokens;
        locals.executeIn.minTokens = 0;
        locals.executeIn.byQu = 0;
        CALL(ExecuteBuy, locals.executeIn, locals.executeOut);
        output.tokens = locals.executeOut.tokens;
        output.quSpent = locals.executeOut.quSpent;
        output.returnCode = locals.executeOut.returnCode;
    }

    // Spends the attached QU on tokens, refunding the rest.
    PUBLIC_PROCEDURE_WITH_LOCALS(BuyWithQu)
    {
        locals.executeIn.name = input.name;
        locals.executeIn.tokens = 0;
        locals.executeIn.minTokens = input.minTokens;
        locals.executeIn.byQu = 1;
        CALL(ExecuteBuy, locals.executeIn, locals.executeOut);
        output.tokens = locals.executeOut.tokens;
        output.quSpent = locals.executeOut.quSpent;
        output.returnCode = locals.executeOut.returnCode;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(Sell)
    {
        output.returnCode = QPUMP_OK;
        output.quOut = 0;
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != qpi.originator())
        {
            output.returnCode = QPUMP_ERR_USERS_ONLY;
            return;
        }
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        locals.settleIn.slot = locals.findOut.slot;
        CALL(SettleOpening, locals.settleIn, locals.settleOut);
        locals.expiryIn.slot = locals.findOut.slot;
        CALL(CheckExpiry, locals.expiryIn, locals.expiryOut);
        locals.coin = state.get().coins.get(locals.findOut.slot);
        if (locals.coin.status != QPUMP_STATUS_OPEN)
        {
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }
        if (input.tokens <= 0)
        {
            output.returnCode = QPUMP_ERR_INVALID_INPUT;
            return;
        }
        locals.walletIn.wallet = qpi.invocator();
        CALL(FindWallet, locals.walletIn, locals.walletOut);
        if (!locals.walletOut.found)
        {
            output.returnCode = QPUMP_ERR_INSUFFICIENT_TOKENS;
            return;
        }
        locals.keyIn.walletId = locals.walletOut.walletId;
        locals.keyIn.coinId = locals.coin.coinId;
        CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
        locals.key = locals.keyOut.key;
        if (!state.get().holders.get(locals.key, locals.entry))
        {
            output.returnCode = QPUMP_ERR_INSUFFICIENT_TOKENS;
            return;
        }
        locals.effectiveIn.coin = locals.coin;
        locals.effectiveIn.entry = locals.entry;
        CALL(EffectiveTokens, locals.effectiveIn, locals.effectiveOut);
        locals.entry.tokens = static_cast<uint32>(locals.effectiveOut.tokens);
        locals.entry.openingQu = 0;
        if (input.tokens > static_cast<sint64>(locals.entry.tokens) || input.tokens > locals.coin.sold)
        {
            output.returnCode = QPUMP_ERR_INSUFFICIENT_TOKENS;
            return;
        }

        locals.costIn.from = locals.coin.sold - input.tokens;
        locals.costIn.to = locals.coin.sold;
        locals.costIn.roundUp = 0;
        CALL(CurveCost, locals.costIn, locals.costOut);
        locals.gross = locals.costOut.qu;
        locals.feeIn.amount = locals.gross;
        locals.feeIn.inclusive = 0;
        CALL(TradeFee, locals.feeIn, locals.feeOut);
        if (locals.gross <= locals.feeOut.fee || locals.gross > locals.coin.realQu)
        {
            output.returnCode = QPUMP_ERR_SLIPPAGE;
            return;
        }
        locals.payout = locals.gross - locals.feeOut.fee;
        if (locals.payout < input.minQuOut)
        {
            output.returnCode = QPUMP_ERR_SLIPPAGE;
            return;
        }

        locals.entry.tokens -= static_cast<uint32>(input.tokens);
        if (locals.entry.tokens == 0)
        {
            locals.unlinkIn.slot = locals.findOut.slot;
            locals.unlinkIn.key = locals.key;
            CALL(UnlinkHolder, locals.unlinkIn, locals.unlinkOut);
            state.mut().holders.removeByKey(locals.key);
            if (locals.coin.holders > 0)
            {
                locals.coin.holders -= 1;
            }
            locals.positionsIn.walletId = locals.walletOut.walletId;
            locals.positionsIn.delta = -1;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
        }
        else
        {
            state.mut().holders.set(locals.key, locals.entry);
        }
        locals.coin.realQu -= locals.gross;
        locals.coin.sold -= input.tokens;
        locals.coin.lastTradeEpoch = qpi.epoch();
        state.mut().coins.set(locals.findOut.slot, locals.coin);

        locals.splitIn.fee = locals.feeOut.fee;
        CALL(SplitFee, locals.splitIn, locals.splitOut);
        state.mut().totalVolume += locals.gross;

        qpi.transfer(qpi.invocator(), locals.payout);
        output.quOut = locals.payout;

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_SELL;
        locals.log.fee = locals.feeOut.fee;
        locals.log.holders = locals.coin.holders;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = qpi.invocator();
        locals.log.qu = locals.gross;
        locals.log.tokens = input.tokens;
        locals.log.realQu = locals.coin.realQu;
        locals.log.sold = locals.coin.sold;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);
    }

    // Permissionless: settle, expire, retry graduation or pay out.
    PUBLIC_PROCEDURE_WITH_LOCALS(Process)
    {
        output.returnCode = QPUMP_OK;
        output.processed = 0;
        output.status = QPUMP_STATUS_EMPTY;
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        locals.settleIn.slot = locals.findOut.slot;
        CALL(SettleOpening, locals.settleIn, locals.settleOut);
        locals.expiryIn.slot = locals.findOut.slot;
        CALL(CheckExpiry, locals.expiryIn, locals.expiryOut);
        locals.coin = state.get().coins.get(locals.findOut.slot);

        if (locals.coin.status == QPUMP_STATUS_COMPLETE)
        {
            if (locals.coin.lastAttemptTick != 0 && qpi.tick() < locals.coin.lastAttemptTick + QPUMP_MANUAL_RETRY_TICKS)
            {
                output.returnCode = QPUMP_ERR_RETRY_LATER;
            }
            else
            {
                locals.graduateIn.slot = locals.findOut.slot;
                CALL(TryGraduate, locals.graduateIn, locals.graduateOut);
                output.returnCode = locals.graduateOut.returnCode;
            }
        }
        else if (locals.coin.status == QPUMP_STATUS_DISTRIBUTING || locals.coin.status == QPUMP_STATUS_REFUNDING)
        {
            locals.payoutsIn.slot = locals.findOut.slot;
            locals.payoutsIn.maxPayouts = input.maxPayouts;
            if (locals.payoutsIn.maxPayouts == 0 || locals.payoutsIn.maxPayouts > QPUMP_PROCESS_MAX_PAYOUTS)
            {
                locals.payoutsIn.maxPayouts = QPUMP_PROCESS_MAX_PAYOUTS;
            }
            locals.payoutsIn.maxScanSteps = QPUMP_PROCESS_SCAN_STEPS;
            CALL(ProcessPayouts, locals.payoutsIn, locals.payoutsOut);
            output.processed = locals.payoutsOut.processed;
            output.returnCode = locals.payoutsOut.returnCode;
        }
        output.status = state.get().coins.get(locals.findOut.slot).status;
    }

    // Collect your own delivery or refund now. Attached QU
    // pays the QX fee only if the budget ran out.
    PUBLIC_PROCEDURE_WITH_LOCALS(Claim)
    {
        output.returnCode = QPUMP_OK;
        output.tokens = 0;
        output.qu = 0;
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        locals.walletIn.wallet = qpi.invocator();
        CALL(FindWallet, locals.walletIn, locals.walletOut);
        if (!locals.walletOut.found)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_HOLDER_NOT_FOUND;
            return;
        }
        locals.keyIn.walletId = locals.walletOut.walletId;
        locals.keyIn.coinId = locals.findOut.coin.coinId;
        CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
        locals.payIn.slot = locals.findOut.slot;
        locals.payIn.key = locals.keyOut.key;
        locals.payIn.callerFee = qpi.invocationReward();
        CALL(PayHolder, locals.payIn, locals.payOut);
        if (qpi.invocationReward() > locals.payOut.callerFeeUsed)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.payOut.callerFeeUsed);
        }
        output.returnCode = locals.payOut.returnCode;
        output.tokens = locals.payOut.tokens;
        output.qu = locals.payOut.qu;
    }

    // Moves curve tokens between wallets for a flat fee.
    PUBLIC_PROCEDURE_WITH_LOCALS(Transfer)
    {
        output.returnCode = QPUMP_OK;
        output.fee = 0;
        if (qpi.invocator() != qpi.originator())
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_USERS_ONLY;
            return;
        }
        if (qpi.invocationReward() < QPUMP_TRANSFER_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPUMP_ERR_INSUFFICIENT_REWARD;
            return;
        }
        // Recipients must be ordinary wallets.
        if (input.tokens <= 0 || input.recipient == NULL_ID || input.recipient == qpi.invocator()
            || (input.recipient.u64._1 == 0 && input.recipient.u64._2 == 0 && input.recipient.u64._3 == 0 && input.recipient.u64._0 < 1024))
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_INVALID_INPUT;
            return;
        }
        locals.findIn.name = input.name;
        CALL(FindCoin, locals.findIn, locals.findOut);
        if (!locals.findOut.found)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_COIN_NOT_FOUND;
            return;
        }
        locals.settleIn.slot = locals.findOut.slot;
        CALL(SettleOpening, locals.settleIn, locals.settleOut);
        locals.expiryIn.slot = locals.findOut.slot;
        CALL(CheckExpiry, locals.expiryIn, locals.expiryOut);
        locals.coin = state.get().coins.get(locals.findOut.slot);
        if (locals.coin.status != QPUMP_STATUS_OPEN && locals.coin.status != QPUMP_STATUS_COMPLETE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_WRONG_STAGE;
            return;
        }

        locals.walletIn.wallet = qpi.invocator();
        CALL(FindWallet, locals.walletIn, locals.fromWallet);
        locals.fromEntry.tokens = 0;
        locals.fromEntry.openingQu = 0;
        if (locals.fromWallet.found)
        {
            locals.keyIn.walletId = locals.fromWallet.walletId;
            locals.keyIn.coinId = locals.coin.coinId;
            CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
            locals.fromKey = locals.keyOut.key;
            state.get().holders.get(locals.fromKey, locals.fromEntry);
        }
        locals.effectiveIn.coin = locals.coin;
        locals.effectiveIn.entry = locals.fromEntry;
        CALL(EffectiveTokens, locals.effectiveIn, locals.effectiveOut);
        if (!locals.fromWallet.found || input.tokens > locals.effectiveOut.tokens)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_INSUFFICIENT_TOKENS;
            return;
        }

        locals.walletIn.wallet = input.recipient;
        CALL(FindWallet, locals.walletIn, locals.toWallet);
        locals.toFound = 0;
        locals.toEntry.tokens = 0;
        locals.toEntry.openingQu = 0;
        if (locals.toWallet.found)
        {
            locals.keyIn.walletId = locals.toWallet.walletId;
            locals.keyIn.coinId = locals.coin.coinId;
            CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
            locals.toKey = locals.keyOut.key;
            locals.toFound = state.get().holders.get(locals.toKey, locals.toEntry);
        }
        if (!locals.toFound)
        {
            if (state.get().holders.population() >= QPUMP_HOLDER_LOAD_LIMIT)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                output.returnCode = QPUMP_ERR_HOLDER_LIMIT;
                return;
            }
            if (!locals.toWallet.found)
            {
                locals.acquireIn.wallet = input.recipient;
                CALL(AcquireWallet, locals.acquireIn, locals.acquireOut);
                if (!locals.acquireOut.ok)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                    output.returnCode = QPUMP_ERR_WALLET_LIMIT;
                    return;
                }
                locals.keyIn.walletId = locals.acquireOut.walletId;
                locals.keyIn.coinId = locals.coin.coinId;
                CALL(HolderKeyOf, locals.keyIn, locals.keyOut);
                locals.toKey = locals.keyOut.key;
            }
        }

        // Recipient first, so a failed insert changes nothing.
        locals.toEntry.tokens += static_cast<uint32>(input.tokens);
        if (state.mut().holders.set(locals.toKey, locals.toEntry) == NULL_INDEX)
        {
            locals.positionsIn.walletId = static_cast<uint32>(locals.toKey >> 32);
            locals.positionsIn.delta = 0;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = QPUMP_ERR_CAPACITY;
            return;
        }
        if (!locals.toFound)
        {
            locals.coin.holders += 1;
            locals.positionsIn.walletId = static_cast<uint32>(locals.toKey >> 32);
            locals.positionsIn.delta = 1;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
        }
        // Link the recipient after the sender entry is written,
        // because linking rewrites the current head.

        locals.fromEntry.tokens = static_cast<uint32>(locals.effectiveOut.tokens - input.tokens);
        locals.fromEntry.openingQu = 0;
        if (locals.fromEntry.tokens == 0)
        {
            locals.unlinkIn.slot = locals.findOut.slot;
            locals.unlinkIn.key = locals.fromKey;
            CALL(UnlinkHolder, locals.unlinkIn, locals.unlinkOut);
            state.mut().holders.removeByKey(locals.fromKey);
            if (locals.coin.holders > 0)
            {
                locals.coin.holders -= 1;
            }
            locals.positionsIn.walletId = locals.fromWallet.walletId;
            locals.positionsIn.delta = -1;
            CALL(ChangePositions, locals.positionsIn, locals.positionsOut);
        }
        else
        {
            state.mut().holders.set(locals.fromKey, locals.fromEntry);
        }
        if (!locals.toFound)
        {
            locals.linkIn.slot = locals.findOut.slot;
            locals.linkIn.key = locals.toKey;
            CALL(LinkHolder, locals.linkIn, locals.linkOut);
        }
        state.mut().coins.set(locals.findOut.slot, locals.coin);
        state.mut().burnPot += QPUMP_TRANSFER_FEE;
        if (qpi.invocationReward() > QPUMP_TRANSFER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QPUMP_TRANSFER_FEE);
        }
        output.fee = QPUMP_TRANSFER_FEE;

        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_TRANSFER;
        locals.log.counterparty = qpi.invocator();
        locals.log.holders = locals.coin.holders;
        locals.log.name = locals.coin.name;
        locals.log.coinId = locals.coin.coinId;
        locals.log.actor = input.recipient;
        locals.log.qu = QPUMP_TRANSFER_FEE;
        locals.log.tokens = input.tokens;
        locals.log.realQu = locals.coin.realQu;
        locals.log.sold = locals.coin.sold;
        locals.log.code = QPUMP_OK;
        LOG_INFO(locals.log);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetCoin, 1);
        REGISTER_USER_FUNCTION(GetHolder, 2);
        REGISTER_USER_FUNCTION(QuoteBuy, 3);
        REGISTER_USER_FUNCTION(QuoteSell, 4);
        REGISTER_USER_FUNCTION(QuoteBudget, 5);
        REGISTER_USER_FUNCTION(GetFees, 6);
        REGISTER_USER_FUNCTION(ListCoins, 7);
        REGISTER_USER_FUNCTION(GetStats, 8);
        REGISTER_USER_FUNCTION(ListGraduated, 9);

        REGISTER_USER_PROCEDURE(CreateCoin, 1);
        REGISTER_USER_PROCEDURE(Buy, 2);
        REGISTER_USER_PROCEDURE(Sell, 3);
        REGISTER_USER_PROCEDURE(Process, 4);
        REGISTER_USER_PROCEDURE(Claim, 5);
        REGISTER_USER_PROCEDURE(Transfer, 6);
        REGISTER_USER_PROCEDURE(BuyWithQu, 7);
    }

    INITIALIZE()
    {
        state.mut().nextCoinId = 1;
        state.mut().activePayoutSlot = QPUMP_NO_SLOT;
        state.mut().cachedQxIssuanceFee = QPUMP_DEFAULT_QX_ISSUANCE_FEE;
        state.mut().cachedQxTransferFee = QPUMP_DEFAULT_QX_TRANSFER_FEE;
        state.mut().cachedQswapPoolFee = QPUMP_DEFAULT_QSWAP_POOL_FEE;
    }

    // Plain QU sent here is returned. Contract refunds stay.
    POST_INCOMING_TRANSFER()
    {
        if (input.type == TransferType::standardTransaction && input.amount > 0)
        {
            qpi.transfer(input.sourceId, input.amount);
        }
    }

    BEGIN_EPOCH_WITH_LOCALS()
    {
        CALL_OTHER_CONTRACT_FUNCTION_E(QX, Fees, locals.qxFeesIn, locals.qxFeesOut, qxFeesError);
        if (qxFeesError == NoCallError && locals.qxFeesOut.assetIssuanceFee > 0)
        {
            state.mut().cachedQxIssuanceFee = locals.qxFeesOut.assetIssuanceFee;
            state.mut().cachedQxTransferFee = locals.qxFeesOut.transferFee;
        }
        CALL_OTHER_CONTRACT_FUNCTION_E(QSWAP, Fees, locals.qswapFeesIn, locals.qswapFeesOut, qswapFeesError);
        if (qswapFeesError == NoCallError && locals.qswapFeesOut.poolCreationFee > 0)
        {
            state.mut().cachedQswapPoolFee = locals.qswapFeesOut.poolCreationFee;
        }
    }

    // One Qswap buy per epoch, paying the flat fee once.
    // The bought QDOGE stays here, out of circulation.
    PRIVATE_PROCEDURE_WITH_LOCALS(BuyQdoge)
    {
        if (state.get().qdogePot < QPUMP_QDOGE_MIN_BUY || qpi.queryFeeReserve(QSWAP_CONTRACT_INDEX) <= 0)
        {
            return;
        }

        locals.poolIn.assetIssuer = ID(_Q, _D, _O, _G, _E, _E, _E, _S, _K, _Y, _P, _A, _I, _C, _E, _C, _H, _E, _A, _H, _O, _X, _P, _U, _L, _E, _O, _A, _D, _T, _K, _G, _E, _J, _H, _A, _V, _Y, _P, _F, _K, _H, _L, _E, _W, _G, _X, _X, _Z, _Q, _U, _G, _I, _G, _M, _B);
        locals.poolIn.assetName = QPUMP_QDOGE_NAME;
        CALL_OTHER_CONTRACT_FUNCTION_E(QSWAP, GetPoolBasicState, locals.poolIn, locals.poolOut, poolError);
        if (poolError != NoCallError || locals.poolOut.poolExists == 0 || locals.poolOut.reservedQuAmount <= 0 || locals.poolOut.reservedAssetAmount <= 0
)
        {
            return;
        }

        locals.amount = state.get().qdogePot;
        locals.poolCap = static_cast<sint64>(div(static_cast<uint64>(locals.poolOut.reservedQuAmount), QPUMP_QDOGE_POOL_DIVISOR));
        if (locals.amount > locals.poolCap)
        {
            locals.amount = locals.poolCap;
        }
        if (locals.amount < QPUMP_QDOGE_MIN_BUY)
        {
            return;
        }

        locals.quoteIn.assetIssuer = ID(_Q, _D, _O, _G, _E, _E, _E, _S, _K, _Y, _P, _A, _I, _C, _E, _C, _H, _E, _A, _H, _O, _X, _P, _U, _L, _E, _O, _A, _D, _T, _K, _G, _E, _J, _H, _A, _V, _Y, _P, _F, _K, _H, _L, _E, _W, _G, _X, _X, _Z, _Q, _U, _G, _I, _G, _M, _B);
        locals.quoteIn.assetName = QPUMP_QDOGE_NAME;
        locals.quoteIn.quAmountIn = locals.amount - QPUMP_QSWAP_LIQUIDITY_FEE;
        CALL_OTHER_CONTRACT_FUNCTION_E(QSWAP, QuoteExactQuInput, locals.quoteIn, locals.quoteOut, quoteError);
        if (quoteError != NoCallError || locals.quoteOut.assetAmountOut <= 0)
        {
            return;
        }

        locals.swapIn.assetIssuer = ID(_Q, _D, _O, _G, _E, _E, _E, _S, _K, _Y, _P, _A, _I, _C, _E, _C, _H, _E, _A, _H, _O, _X, _P, _U, _L, _E, _O, _A, _D, _T, _K, _G, _E, _J, _H, _A, _V, _Y, _P, _F, _K, _H, _L, _E, _W, _G, _X, _X, _Z, _Q, _U, _G, _I, _G, _M, _B);
        locals.swapIn.assetName = QPUMP_QDOGE_NAME;
        locals.swapIn.assetAmountOutMin = locals.quoteOut.assetAmountOut;
        CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
        locals.balanceBefore = locals.balanceOut.balance;
        INVOKE_OTHER_CONTRACT_PROCEDURE_E(QSWAP, SwapExactQuForAsset, locals.swapIn, locals.swapOut, locals.amount, swapError);
        CALL(ContractBalance, locals.balanceIn, locals.balanceOut);
        if (locals.balanceBefore > locals.balanceOut.balance)
        {
            state.mut().qdogePot -= locals.balanceBefore - locals.balanceOut.balance;
            state.mut().totalQuToQdoge += locals.balanceBefore - locals.balanceOut.balance;
        }
        if (swapError == NoCallError && locals.swapOut.assetAmountOut > 0)
        {
            state.mut().totalQdogeBought += locals.swapOut.assetAmountOut;
        }
        locals.log._contractIndex = SELF_INDEX;
        locals.log._type = QPUMP_LOG_QDOGE_BUYBACK;
        locals.log.name = QPUMP_QDOGE_NAME;
        locals.log.actor = locals.swapIn.assetIssuer;
        locals.log.qu = locals.balanceBefore > locals.balanceOut.balance ? locals.balanceBefore - locals.balanceOut.balance : 0;
        locals.log.tokens = swapError == NoCallError ? locals.swapOut.assetAmountOut : 0;
        locals.log.realQu = state.get().qdogePot;
        locals.log.code = swapError == NoCallError && locals.swapOut.assetAmountOut > 0 ? QPUMP_OK : QPUMP_ERR_EXTERNAL_CALL;
        LOG_INFO(locals.log);
    }

    END_TICK_WITH_LOCALS()
    {
        if (state.get().liveCoins == 0)
        {
            return;
        }

        // One coin gets payouts pushed each tick.
        if (state.get().activePayoutSlot != QPUMP_NO_SLOT)
        {
            locals.slot = state.get().activePayoutSlot;
            locals.coin = state.get().coins.get(locals.slot);
            if ((locals.coin.status == QPUMP_STATUS_REFUNDING || locals.coin.status == QPUMP_STATUS_DISTRIBUTING)
                && locals.coin.failures < QPUMP_MAX_DELIVERY_FAILURES)
            {
                locals.payoutsIn.slot = locals.slot;
                locals.payoutsIn.maxPayouts = QPUMP_TICK_PAYOUTS;
                locals.payoutsIn.maxScanSteps = QPUMP_TICK_SCAN_STEPS;
                CALL(ProcessPayouts, locals.payoutsIn, locals.payoutsOut);
            }
            else
            {
                state.mut().activePayoutSlot = QPUMP_NO_SLOT;
            }
        }

        // Lifecycle sweep over a rotating window of used slots.
        locals.scanned = 0;
        locals.work = 0;
        while (locals.scanned < QPUMP_TICK_SLOT_SCAN && locals.scanned < state.get().nextFreshSlot && locals.work < QPUMP_TICK_COIN_WORK)
        {
            locals.slot = state.get().sweepCursor;
            if (locals.slot >= state.get().nextFreshSlot)
            {
                locals.slot = 0;
            }
            state.mut().sweepCursor = locals.slot + 1;
            locals.scanned += 1;
            locals.coin = state.get().coins.get(locals.slot);
            if (locals.coin.status == QPUMP_STATUS_OPENING)
            {
                if (qpi.tick() >= locals.coin.createdTick + QPUMP_OPENING_TICKS)
                {
                    locals.settleIn.slot = locals.slot;
                    CALL(SettleOpening, locals.settleIn, locals.settleOut);
                    locals.work += 1;
                }
            }
            else if (locals.coin.status == QPUMP_STATUS_OPEN || locals.coin.status == QPUMP_STATUS_COMPLETE)
            {
                locals.expiryIn.slot = locals.slot;
                CALL(CheckExpiry, locals.expiryIn, locals.expiryOut);
                if (locals.expiryOut.changed)
                {
                    locals.work += 1;
                }
                locals.coin = state.get().coins.get(locals.slot);
                if (locals.coin.status == QPUMP_STATUS_COMPLETE
                    && (locals.coin.lastAttemptTick == 0 || qpi.tick() >= locals.coin.lastAttemptTick + QPUMP_GRADUATION_RETRY_TICKS))
                {
                    locals.graduateIn.slot = locals.slot;
                    CALL(TryGraduate, locals.graduateIn, locals.graduateOut);
                    locals.work += 1;
                }
            }
            locals.coin = state.get().coins.get(locals.slot);
            if ((locals.coin.status == QPUMP_STATUS_REFUNDING || locals.coin.status == QPUMP_STATUS_DISTRIBUTING)
                && locals.coin.failures < QPUMP_MAX_DELIVERY_FAILURES
                && state.get().activePayoutSlot == QPUMP_NO_SLOT)
            {
                state.mut().activePayoutSlot = locals.slot;
            }
        }
    }

    END_EPOCH_WITH_LOCALS()
    {
        CALL(BuyQdoge, locals.qdogeIn, locals.qdogeOut);

        if (state.get().shareholderPot >= static_cast<sint64>(NUMBER_OF_COMPUTORS))
        {
            locals.perShare = static_cast<sint64>(div(static_cast<uint64>(state.get().shareholderPot), static_cast<uint64>(NUMBER_OF_COMPUTORS)));
            if (locals.perShare > 0 && qpi.distributeDividends(locals.perShare))
            {
                state.mut().shareholderPot -= locals.perShare * NUMBER_OF_COMPUTORS;
                state.mut().totalDividends += locals.perShare * NUMBER_OF_COMPUTORS;
            }
        }
        if (state.get().burnPot > 0)
        {
            if (qpi.burn(state.get().burnPot) >= 0)
            {
                state.mut().totalBurned += state.get().burnPot;
                state.mut().burnPot = 0;
            }
        }

        // Lists and cursors use keys, so compaction is safe.
        state.mut().holders.cleanupIfNeeded();
        state.mut().nameToSlot.cleanupIfNeeded();
        state.mut().creatorLive.cleanupIfNeeded();
        state.mut().walletIds.cleanupIfNeeded();
    }
};
